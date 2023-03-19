
/*
 * Andor driver (optimised/specialised for iXon model
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
 *
 * A serious rewrite motivated by receiving a new iXon Ultra and by
 * long experience with the original backend
 * Copyright (c) 2015 Martin Jelinek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include <time.h>

// XXX camd.h is full of inline defs... not a cool thing for a .h
#include "camd.h"

#ifdef __GNUC__
#if(__GNUC__ > 3 || __GNUC__ ==3)
#define _GNUC3_
#endif
#endif

#ifdef _GNUC3_
#include <iostream>
#include <fstream>
using namespace std;
#else
#include <iostream.h>
#include <fstream.h>
#endif

#include "atmcdLXd.h"
//That's root for andor2.77
#define ANDOR_ROOT          (char *) "/usr/local/etc/andor"

#define EVENT_TE_RAMP         RTS2_LOCAL_EVENT + 678
#define TE_RAMP_INTERVAL 60     // in seconds
#define TE_RAMP_SPEED 100       // in degrees/minute

#define IXON_DEFAULT_GAIN   100
#define IXON_MAX_GAIN       300

#define ANDOR_SHUTTER_AUTO    0
#define ANDOR_SHUTTER_OPEN    1
#define ANDOR_SHUTTER_CLOSED  2

// Acqisition modes: These constants are intended to be identical with the
// underlying driver, because they are referenced as numbers in the
// documentation. So I opted to rather have a "none" in the list than to have
// mess in these numbers
#define ACQMODE_NONE            0
#define ACQMODE_SINGLE          1
#define ACQMODE_ACCUMULATE      2
#define ACQMODE_KINETIC         3
#define ACQMODE_FASTKINETICS    4
#define ACQMODE_VIDEO           5

#define MAX_ADC_MODES     32
#define TEXTBUF_LEN 32

#define OPT_ANDOR_ROOT        OPT_LOCAL + 1

// A macro to report errors while calling andor library
#define checkRet(a,b) {if(ret != DRV_SUCCESS && ret != 0){logStream (MESSAGE_ERROR) << a << ": error communicating with the camera: " << b << " returns " << ret << sendLog;return -1;}}

// #define TEMP_SAFETY // turns out not to be supported ... :(

// There is a bug in Andor Linux SDK 2.102.30045, one of the readout modes on
// iXon Ultra 888 is wrongly reported as not-available (by function
// IsPreAmpGainAvailable() ).
// This is dirty fix to avoid this misbehaviour and include this mode in
// selection.
#define SDK_2_102_U888_FIX

const char outputAmpDescShort[3][3] = { "EM", "CO", "XX" };
const char outputAmpDescLong[3][32] = { "EMCCD", "CONVENTIONAL", "UNKNOWN" };

namespace rts2camd
{

  /*
   * READOUT Mode
   *
   * Andor SDK defines the following different "readout modes" as follows:
   * Full Vertical Binning (FVB) / Single-Track / Multi-Track / Random-Track / Image / Cropped
   */

  /*
   * ACQUISITION Mode
   *
   * Andor SKD defines the following different "acquisition mode types" as follows:
   * Single Scan / Accumulate / Kinetic Series / Run Till Abort / Fast Kinetics
   * If the system is a Frame Transfer CCD, these modes can be enhanced by setting the chip operational mode to Frame Transfer.
   */

    /** Andor camera, as seen by the outside world.
     *
     * This was originally written for an iXon, and doesn't handle anything an iXon can't do. If used with a different Andor
     * camera, it should respond reasonably well to the absence of iXon features
     *
     * @author Petr Kubanek <petr@kubanek.net>
     *
     */
  class Andor:public Camera
  {

  public:
    Andor (int argc, char **argv);
      virtual ~ Andor (void);
    int box (int _x, int _y, int _width, int _height,
             rts2core::ValueRectangle * retv);

  protected:
    double intExposure;
    void setExposure (double exp);

    int setBinning (int in_vert, int in_hori);
    virtual int initChips ();
    virtual int initHardware ();        // virtual from class Device
    int setADChannel (int in_adchan);
    int setVSSpeed (int in_vsspeed);

  private:
      virtual void postEvent (rts2core::Event * event);
    //              virtual bool supportFrameTransfer ();

    // callback functions for Camera alone
    virtual int info ();
    virtual int scriptEnds ();
    virtual int setCoolTemp (float new_temp);
    virtual void afterNight ();
    void initCooling ();

    //            protected:

    virtual int processOption (int in_opt);
    virtual void help ();
    virtual void usage ();

    virtual void initBinnings ()
    {
      Camera::initBinnings ();
      addBinning2D (2, 2);
      addBinning2D (4, 4);
      addBinning2D (8, 8);
    } virtual void initDataTypes ();

    void temperatureCheck ();

    virtual int switchCooling (bool cooling);
    virtual int startExposure ();
    virtual int stopExposure ();
    virtual long isExposing ();

    virtual int setValue (rts2core::Value * old_value,
                          rts2core::Value * new_value);

    virtual int doReadout ();

    //            private:
    char *andorRoot;
    bool printSpeedInfo;
    // number of AD channels
    int chanNum;

    AndorCapabilities cap;
    int andor_shutter_state;

    int disable_ft;
    int shutter_with_ft;

      rts2core::ValueInteger * XXXgain;
      rts2core::ValueInteger * emccdgain;

    //rts2core::ValueBool * useRunTillAbort;

      rts2core::ValueSelection * VSAmp;
      rts2core::ValueBool * FTShutter;

    // to be removed
      rts2core::ValueDouble * subExposure;

    // ACQUISITION MODE
    int setTiming ();           // Really set and recompute the acq mode
    int setAcqMode (int mode);  // now does nothing, needed to compile the expose, which is not changed yet
    // Values that are really considered when recomputing Timing
    // rts2core::ValueFloat * accCycleGap; // assumed zero
    // rts2core::ValueFloat * kinCycleGap;
    // This apprach will need to be changed, but at the moment it is the way of thinking, I believe there is a
    // better way to represent this
      rts2core::ValueFloat * accCycle;
      rts2core::ValueFloat * kinCycle;
      rts2core::ValueInteger * accNumber;
      rts2core::ValueInteger * kinNumber;
      rts2core::ValueBool * useFT;
      rts2core::ValueFloat * imgFreq;

    // informational values
      rts2core::ValueSelection * adcMode;
      rts2core::ValueSelection * ADChannel;
      rts2core::ValueBool * EMOn;
      rts2core::ValueFloat * HSpeed;
      rts2core::ValueSelection * VSpeed;

      rts2core::ValueInteger * bitDepth;
      rts2core::ValueSelection * acqMode;

      rts2core::ValueSelection * outputAmp;
      rts2core::ValueFloat * outPreAmpGain;

      rts2core::ValueBool * emAdvanced;
      rts2core::ValueSelection * emGainMode;
      rts2core::ValueSelection * fanMode;

      rts2core::ValueBool * filterCr;

      rts2core::ValueBool * baselineClamp;
      rts2core::ValueInteger * baselineOff;
      rts2core::ValueFloat * Sensitivity;

    int defaultGain;

    void updateFlip ();

    float getTemp ();
    int setGain (int in_gain);
    int setEMCCDGain (int in_gain);
    int setVSAmplitude (int in_vsamp);
    int setHSSpeed (int in_amp, int in_hsspeed);
    int setFTShutter (bool force);
    int setUseFT (bool force);

    int printInfo ();
    void printCapabilities ();
    int printNumberADCs ();
    int printHSSpeeds (int ad_channel, int amplifier);
    int updateHSSpeeds ();
    int printVSSpeeds ();

    int initAndorID ();
    int initAndorADCModes ();
    int initAndorValues ();

    // TEMP control
      rts2core::ValueFloat * tempRamp;
      rts2core::ValueFloat * tempTarget;
      rts2core::ValueSelection * tempStatus;
#ifdef TEMP_SAFETY
      rts2core::ValueSelection * tempSafety;
#endif

    void closeShutter ();

    int adcModes;
    int defaultADCMode;
    int adcModeCodes[4][MAX_ADC_MODES];
    int setADCMode (int mode);

    FILE *kinSeriesTarget = NULL;
    rts2core::ValueString * kinFile;
    //char *kinFile = NULL;
    uint64_t *kinSeriesBuffer = NULL;
    uint64_t kinSeriesNumber = 0;
  };

}

using namespace rts2camd;

/*
 * EXPOSURE + READOUT CONTROL
 */

int
Andor::startExposure ()
{
  int ret;
  time_t s = time (NULL);

  // camd.cpp creates an image unless told we would make it ourself
  switch (acqMode->getValueInteger ())
    {
    case ACQMODE_KINETIC:
      //                        dataChannels->setValueInteger (kinNumber->getValueInteger ());
      //          startExposureConnImageData ();
      char *outfile;
      struct tm st;

      gmtime_r (&s, &st);
      asprintf (&outfile, "/var/tmp/kin-%04d%02d%02d%02d%02d%02d.out",
                st.tm_year + 1900, st.tm_mon + 1, st.tm_mday,
                st.tm_hour, st.tm_min, st.tm_sec);
      kinFile->setValueCharArr (outfile);
      kinSeriesTarget = fopen (outfile, "w");
      free (outfile);
      if (!kinSeriesBuffer)
        kinSeriesBuffer =
          (uint64_t *) calloc (chipUsedSize (), sizeof (uint64_t));
      kinSeriesNumber = 0;

      realTimeDataTransferCount = -1;
      break;

    case ACQMODE_VIDEO: // video is still broken
      kinFile->setValueCharArr ("none");
      realTimeDataTransferCount = 0;
      break;

    default:
      kinFile->setValueCharArr ("none");
      realTimeDataTransferCount = -1;
      break;
    }

  // This is called by setTiming anyway
  //      ret =
  //          SetImage (binningHorizontal (), binningVertical (), chipTopX () + 1,
  //                    chipTopX () + chipUsedReadout->getWidthInt (), chipTopY () + 1,
  //                    chipTopY () + chipUsedReadout->getHeightInt ());

  //      if (ret != DRV_SUCCESS)
  //      {
  //              if (ret == DRV_ACQUIRING)
  //              {
  //                      quedExpNumber->inc ();
  //                      return 0;
  //              }
  //              logStream (MESSAGE_ERROR) << "andor SetImage return " << ret << sendLog;
  //              return -1;
  //      }

  // may not be necessary
  //setTiming();

  int new_state =
    (getExpType () == 0) ? ANDOR_SHUTTER_AUTO : ANDOR_SHUTTER_CLOSED;

  //      if ((getExpType () == 0) && useFT->getValueBool () && (!FTShutter->getValueBool ()))
  //              new_state = ANDOR_SHUTTER_OPEN;

  if (new_state != andor_shutter_state)
    {
      logStream (MESSAGE_DEBUG) << "SetShutter " << new_state << sendLog;
      ret = SetShutter (1, new_state, 50, 50);
      checkRet ("startExposure()", "SetShutter()");
    }
  andor_shutter_state = new_state;

  getTemp ();

  ret = StartAcquisition ();
  checkRet ("startExposure()", "StartAcquisition()");

  return realTimeDataTransferCount >= 0 ? 1 : 0;
}

int
Andor::stopExposure ()
{
  if (kinSeriesTarget)
    {
      fclose (kinSeriesTarget);
      kinSeriesTarget = NULL;
    }
  if (kinSeriesBuffer)
    {
      free (kinSeriesBuffer);
      kinSeriesBuffer = NULL;
    }
  nullExposureConn ();
  AbortAcquisition ();
  FreeInternalMemory ();
  return Camera::stopExposure ();
}

long
Andor::isExposing ()
{
  int status;
  int ret;

  // KINETIC SERIES
  if (acqMode->getValueInteger () == ACQMODE_KINETIC)
    {
      int n = 0, last = 0;

      ret = GetNumberNewImages (&n, &last);

      if (ret == DRV_NO_NEW_DATA)
        {
          if (n >= kinNumber->getValueInteger ())
            {
              logStream (MESSAGE_INFO) << "isExposing: exposure finished" <<
                sendLog;
              return -2;        // finished acquisition
            }

          return 100;
        }

      size_t size = chipUsedSize ();
      uint16_t buffer[size];

      do                        // This loop needs to be fast, no logging!
        {
          ret = GetOldestImage16 (buffer, size);
          fwrite (buffer, sizeof (buffer), 1, kinSeriesTarget);
          for (unsigned int j = 0; j < size; j++)
            kinSeriesBuffer[j] += buffer[j];
          kinSeriesNumber += 1;
          ret = GetNumberNewImages (&n, &last);
        }
      while (ret == DRV_SUCCESS);

      logStream (MESSAGE_INFO) << "isExposing: n=" << n << "/" <<
        kinNumber->getValueInteger () << " ret=" << ret << sendLog;
    }

  // RUN TILL ABORT
  if (acqMode->getValueInteger () == ACQMODE_VIDEO)
    {

      {
        return -2;
      }
      logStream (MESSAGE_DEBUG) << "new image " << status << sendLog;

      // signal that we have data
      maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | BOP_TEL_MOVE,
                 CAM_NOEXPOSURE | CAM_READING,
                 "chip extended readout started");
      // get the data
      at_32 lAcquired = 0;

      status = GetTotalNumberImagesAcquired (&lAcquired);
      logStream (MESSAGE_DEBUG) << "Circular buffer size " << lAcquired <<
        " status " << status << sendLog;
      switch (getDataType ())
        {
        case RTS2_DATA_LONG:
          if (GetMostRecentImage
              ((at_32 *) getDataBuffer (0), chipUsedSize ()) != DRV_SUCCESS)
            {
              logStream (MESSAGE_ERROR) << "Cannot get long data" << sendLog;
              return -1;
            }
          break;
        default:
          if (GetMostRecentImage16
              ((uint16_t *) getDataBuffer (0),
               chipUsedSize ()) != DRV_SUCCESS)
            {
              logStream (MESSAGE_ERROR) << "Cannot get int data" << sendLog;
              return -1;
            }
        }
      logStream (MESSAGE_DEBUG) << "Image " << ((unsigned short *)
                                                getDataBuffer (0))[0] << " "
        << ((unsigned short *) getDataBuffer (0))[1] << " " <<
        ((unsigned short *) getDataBuffer (0))[2] << " " <<
        ((unsigned short *) getDataBuffer (0))[3] << " " <<
        ((unsigned short *) getDataBuffer (0))[4] << sendLog;
      // now send the data
      ret = sendImage (getDataBuffer (0), chipByteSize ());
      if (ret)
        return ret;
      if (quedExpNumber->getValueInteger () == 0)
        {
          // stop exposure if we do not have any queued values
          AbortAcquisition ();
          FreeInternalMemory ();
          logStream (MESSAGE_INFO) << "Aborting acqusition" << sendLog;
          maskState (CAM_MASK_READING | BOP_TEL_MOVE, CAM_NOTREADING,
                     "extended readout finished");
          return -3;
        }
      maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | BOP_TEL_MOVE,
                 CAM_EXPOSING | CAM_NOTREADING, "extended readout finished");
      quedExpNumber->dec ();
      sendValueAll (quedExpNumber);
      incExposureNumber ();
      return 100;
    }

  if ((ret = Camera::isExposing ()) != 0)
    {
      markReadoutStart ();
      // if only "return ret;" we get too long delays
      if (ret < 100)
        return ret;
      else
        return 100;
    }

  if (GetStatus (&status) != DRV_SUCCESS)
    return -1;

  // is it just me to think this is never true
  // because DRV_ACQUIRING!=DRV_SUCCESS? -m. 
  if (status == DRV_ACQUIRING)
    {
      markReadoutStart ();
      return 100;
    }

  return -2;
}

// For each exposure, the first time this function is called, it reads out
// the entire image from the camera (into dest).  Subsequent calls return
// lines from dest.
int
Andor::doReadout ()
{
  int ret;
  int status;

  if (GetStatus (&status) != DRV_SUCCESS)
    return -1;

  // if camera is still acquiring
  if (status == DRV_ACQUIRING)
    return 0;

  logStream (MESSAGE_INFO) << "doReadout: lets read out :)" << sendLog;
  // 10 seconds timeout for acqusition
  if (getNow () > getExposureEnd () + 10)
    {
      logStream (MESSAGE_ERROR) <<
        "doReadout: timeout waiting for exposure to end" << sendLog;
      ret = AbortAcquisition ();
      checkRet ("doReadout()", "AbortAcquisition()");
      return -1;
    }

  if (acqMode->getValueInteger () == ACQMODE_KINETIC)
    {

      switch (getDataType ())
        {
        case RTS2_DATA_FLOAT:
          logStream (MESSAGE_INFO) << "doReadout: GetAcquiredFloatData" <<
            sendLog;
          ret =
            GetAcquiredFloatData ((float *) getDataBuffer (0),
                                  chipUsedSize ());
          break;
        case RTS2_DATA_LONG:
          logStream (MESSAGE_INFO) << "doReadout: GetAcquiredData" << sendLog;
          ret =
            GetAcquiredData ((at_32 *) getDataBuffer (0), chipUsedSize ());
          break;
          // case RTS2_DATA_SHORT:
        default:
          logStream (MESSAGE_INFO) << "doReadout: GetAcquiredData16" <<
            sendLog;

          short unsigned *buf = (short unsigned *) getDataBuffer (0);

          for (unsigned int j = 0; j < chipUsedSize (); j++)
            buf[j] = kinSeriesBuffer[j] / kinSeriesNumber;
          ret = DRV_SUCCESS;    // of course :)

          break;
        }

      if (kinSeriesTarget)
        {
          fclose (kinSeriesTarget);
          kinSeriesTarget = NULL;
        }
      if (kinSeriesBuffer)
        {
          free (kinSeriesBuffer);
          kinSeriesBuffer = NULL;
        }

      updateReadoutSpeed (getReadoutPixels ());

      if (ret != DRV_SUCCESS)
        {
          // acquisition in progress is NOT an error if it occurs early
          // but we should neverget this far
          if ((ret == DRV_ACQUIRING) && (getNow () < getExposureEnd () + 100))
            {
              logStream (MESSAGE_INFO) <<
                "doReadout: could not get still acquiring, ret=" << ret <<
                sendLog;
              return 100;
            }
          logStream (MESSAGE_ERROR) << "andor GetAcquiredXXXX " <<
            getDataType () << " return " << ret << sendLog;
          return -1;
        }

      ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
      logStream (MESSAGE_INFO) << "doReadout: after sendReadoutData, ret=" <<
        ret << sendLog;
      if (ret < 0)
        return -1;
      if (getWriteBinaryDataSize () == 0)
        return -2;
    }

  if (acqMode->getValueInteger () == ACQMODE_SINGLE
      || acqMode->getValueInteger () == ACQMODE_ACCUMULATE)
    {
      switch (getDataType ())
        {
        case RTS2_DATA_FLOAT:
          logStream (MESSAGE_INFO) << "doReadout: GetAcquiredFloatData" <<
            sendLog;
          ret =
            GetAcquiredFloatData ((float *) getDataBuffer (0),
                                  chipUsedSize ());
          break;
        case RTS2_DATA_LONG:
          logStream (MESSAGE_INFO) << "doReadout: GetAcquiredData" << sendLog;
          ret =
            GetAcquiredData ((at_32 *) getDataBuffer (0), chipUsedSize ());
          break;
          // case RTS2_DATA_SHORT:
        default:
          logStream (MESSAGE_INFO) << "doReadout: GetAcquiredData16" <<
            sendLog;
          ret =
            GetAcquiredData16 ((short unsigned *) getDataBuffer (0),
                               chipUsedSize ());
          break;
        }

      updateReadoutSpeed (getReadoutPixels ());

      if (ret != DRV_SUCCESS)
        {
          // acquisition in progress is NOT an error if it occurs early
          // but we should neverget this far
          if ((ret == DRV_ACQUIRING) && (getNow () < getExposureEnd () + 100))
            {
              logStream (MESSAGE_INFO) <<
                "doReadout: could not get still acquiring, ret=" << ret <<
                sendLog;
              return 100;
            }
          logStream (MESSAGE_ERROR) << "andor GetAcquiredXXXX " <<
            getDataType () << " return " << ret << sendLog;
          return -1;
        }

      ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
      logStream (MESSAGE_INFO) << "doReadout: after sendReadoutData, ret=" <<
        ret << sendLog;
      if (ret < 0)
        return -1;
      if (getWriteBinaryDataSize () == 0)
        return -2;
    }
  else
    return -2;                  // -2 = readout ended, but the peer would still have no data, so it would freeze
  return 0;
}

void
Andor::closeShutter ()
{
  SetShutter (1, ANDOR_SHUTTER_CLOSED, 50, 50);
  andor_shutter_state = ANDOR_SHUTTER_CLOSED;
}

// scriptEnds
// Ensure that we definitely leave the shutter closed.
int
Andor::scriptEnds ()
{
  logStream (MESSAGE_INFO) << "scriptEnds: reset values" << sendLog;
  // did something else than thought
  //      if (!isnan (defaultGain) && gain)
  //              changeValue (gain, defaultGain);

  // Reset AcqMode, there will be more...
  //accCycleGap->setValueFloat (0.0);
  //kinCycleGap->setValueFloat (0.0);
  accNumber->setValueInteger (1);
  kinNumber->setValueInteger (1);
  intExposure = 0;
  changeValue (acqMode, 1);     // Single scan default
  //      setTiming();

  // set default values..
  changeValue (adcMode, defaultADCMode);
  changeValue (VSpeed, 1);

  changeValue (FTShutter, false);
  changeValue (useFT, true);

  // changeValue (useRunTillAbort, false);

  //      closeShutter ();
  return Camera::scriptEnds ();
}

// *** recalculate CCD timing, this function should be called after any change of parameters involved
int
Andor::setTiming ()
{
  int ret = DRV_SUCCESS;

  //double e;
  float expt = intExposure;
  float acct = 0.0;
  float kint = 0.0;

  float acqt;

  int binh = binningHorizontal ();
  int binv = binningVertical ();

  // prevent error messages at startup when binning is not initialized
  binh = binh < 1 ? 1 : binh;
  binv = binv < 1 ? 1 : binv;

  // This complains when the camera is acquiring (20072)
  ret = SetImage (binh, binv, chipTopX () + 1,
                  chipTopX () + chipUsedReadout->getWidthInt (),
                  chipTopY () + 1,
                  chipTopY () + chipUsedReadout->getHeightInt ());
  logStream (MESSAGE_INFO) << "setTiming: x=" << chipTopX () +
    1 << "  y=" << chipTopY () + 1 << sendLog;
  checkRet ("setTiming()", "SetImage()");

  ret = SetAcquisitionMode (acqMode->getValueInteger ());
  checkRet ("setTiming()", "SetAcquisitionMode()");

  ret = SetExposureTime (expt);
  checkRet ("setTiming()", "SetExposureTime()");

  ret = SetAccumulationCycleTime (0);
  checkRet ("setTiming()", "SetAccumulationCycleTime()");

  ret = SetNumberAccumulations (accNumber->getValueInteger ());
  checkRet ("setTiming()", "SetNumberAccumulations()");

  ret = SetKineticCycleTime (0);
  checkRet ("setTiming()", "SetKineticCycleTime()");

  ret = SetNumberKinetics (kinNumber->getValueInteger ());
  checkRet ("setTiming()", "SetNumberKinetics()");

  ret = GetAcquisitionTimings (&expt, &acct, &kint);
  checkRet ("setTiming()", "GetAcquisitionTimings()");

  // When somene decides to support gaps between exposures, here is a chunk...
  //        acct+=accCycleGap->getValueFloat();
  //        kint+=kinCycleGap->getValueFloat();
  //        if (ret==DRV_SUCCESS) ret = SetAccumulationCycleTime(acct);
  //        if (ret==DRV_SUCCESS) ret = SetKineticCycleTime(kint);
  //        if (ret==DRV_SUCCESS) ret = GetAcquisitionTimings(&expt, &acct, &kint);

  float hz;

  if (kint > 0.0)
    hz = 1.0 / kint;
  else
    hz = NAN;
  logStream (MESSAGE_INFO) << "setTiming: (expt,acct,kint) " << expt << "," <<
    acct << "," << kint << "," << hz << sendLog;
  accCycle->setValueFloat (acct);
  kinCycle->setValueFloat (kint);

  if (kint <= 0)
    imgFreq->setValueFloat (NAN);
  else
    imgFreq->setValueFloat (1.0 / kint);

  // This replaces exposure in computation of time that will be spent waiting for data.
  Camera::readoutTime->setValueDouble (acct - expt);

  // calculate time to perorm a given acquisition
  // I would like to see "all exp+all readouts", but right now it is everything - one reaout
  switch (acqMode->getValueInteger ())
    {
    case 1:
      acqt = acct;
      break;
    case 2:
      acqt = kint;
      break;
    case 3:
      acqt = kint * kinNumber->getValueInteger ();
      break;
    case 4:
    case 5:
      logStream (MESSAGE_INFO) << "setTiming: modes 4&5 not implemented" <<
        ret << sendLog;
      acqt = kint * kinNumber->getValueInteger ();
      break;
    default:
      logStream (MESSAGE_ERROR) << "setTiming: impossible default " << ret <<
        sendLog;
    }

  acquireTime->setValueDouble (acqt - Camera::readoutTime->getValueDouble ());

  //        Camera::pixelsSecond->setValueDouble(computedPixels/(kint-expt));

  Camera::setExposure (expt);
  sendValueAll (exposure);      // ale tohle bude zrejme fungovat jen pokud se zrovna nenastavuje exposure XXX
  sendValueAll (readoutTime);
  sendValueAll (acquireTime);
  sendValueAll (imgFreq);
  sendValueAll (accCycle);
  sendValueAll (kinCycle);

  return 0;
}

// camd has a mechanism to handle the parity of image transformation, and different AD channels provide different directions of
// readout this is to handle it

void
Andor::updateFlip ()
{
  switch (ADChannel->getValueInteger ())
    {
    case 0:
      changeAxisDirections (true, true);
      break;
    case 1:
      changeAxisDirections (false, false);
      break;
    }
}

/* ***** SET VALUES ***** */

int
Andor::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
  if ((cap.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)
      && (old_value == emccdgain))
    return setEMCCDGain (new_value->getValueInteger ()) == 0 ? 0 : -2;
  if (old_value == VSAmp)
    return setVSAmplitude (new_value->getValueInteger ()) == 0 ? 0 : -2;

  // EMON is purely an alias to adcMode (switching btw 2 modes)
  if (old_value == EMOn)
    {
      if (defaultADCMode == 0)
        return 0;               // no EM capability

      int new_mode =
        ((rts2core::ValueBool *) new_value)->getValueBool () ==
        0 ? defaultADCMode : 0;
      changeValue (adcMode, new_mode);
      return 0;
    }

  if (old_value == adcMode)     // sets also read-only values of outputAmp, ADChannel and HSpeed
    return setADCMode (new_value->getValueInteger ()) == 0 ? 0 : -2;
  if (old_value == VSpeed)
    return setVSSpeed (new_value->getValueInteger ()) == 0 ? 0 : -2;

  if (old_value == FTShutter)
    return setFTShutter (((rts2core::ValueBool *) new_value)->getValueBool ())
      == 0 ? 0 : -2;
  if (old_value == useFT)
    return setUseFT (((rts2core::ValueBool *) new_value)->getValueBool ()) ==
      0 ? 0 : -2;
  if (old_value == acqMode)
    {
      //              return setAcqMode (((rts2core::ValueSelection *) new_value->getValueInteger ()) == 0 ? 0 : -2;
      // 0 not permitted
      if (new_value->getValueInteger () == 0)
        return 0;
      acqMode->setValueInteger (new_value->getValueInteger ());
      setTiming ();
    }
  if (old_value == accNumber)
    {
      accNumber->setValueInteger (new_value->getValueInteger ());
      setTiming ();
    }
  if (old_value == kinNumber)
    {
      kinNumber->setValueInteger (new_value->getValueInteger ());
      setTiming ();
    }

  //      if (old_value == useRunTillAbort)
  //              return 0;
  if (old_value == filterCr)
    {
      return
        SetFilterMode (((rts2core::ValueBool *) new_value)->
                       getValueBool ()? 2 : 0) == DRV_SUCCESS ? 0 : -2;
    }
  if (old_value == baselineClamp)
    {
      return
        SetBaselineClamp (((rts2core::ValueBool *) new_value)->
                          getValueBool ()? 1 : 0) == DRV_SUCCESS ? 0 : -2;
    }
  if (old_value == baselineOff)
    {
      return SetBaselineOffset (new_value->getValueInteger ()) ==
        DRV_SUCCESS ? 0 : -2;
    }
  if (old_value == emAdvanced)
    {
      return
        SetEMAdvanced (((rts2core::ValueBool *) new_value)->
                       getValueBool ()? 1 : 0) == DRV_SUCCESS ? 0 : -2;
    }
  if (old_value == emGainMode)
    {
      return SetEMGainMode (new_value->getValueInteger ()) ==
        DRV_SUCCESS ? 0 : -2;
    }
  if (old_value == fanMode)
    {
      return SetFanMode (new_value->getValueInteger ()) ==
        DRV_SUCCESS ? 0 : -2;
    }

  return Camera::setValue (old_value, new_value);
}

int
Andor::setBinning (int in_vert, int in_hori)
{
  logStream (MESSAGE_INFO) << "setBinning" << sendLog;
  int ret = Camera::setBinning (in_vert, in_hori);

  setTiming ();
  return ret;
}

// window=(471,471,82,82) binning=0 acqmode=3 adcmode=0 accnum=1 E 0
// window=(432,432,160,160) binning=1 acqmode=3 adcmode=0 accnum=1 E 0
// give 200Hz
int
Andor::box (int _x, int _y, int _width, int _height,
            rts2core::ValueRectangle * retv)
{
  logStream (MESSAGE_INFO) << "Andor::box() called" << sendLog;
  int ret = Camera::box (_x, _y, _width, _height, retv);

  setTiming ();
  return ret;
}

int
Andor::setUseFT (bool force)
{
  int status;

  if (!(cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER))
    return -2;
  status = SetFrameTransferMode (force ? 1 : 0);
  if (status != DRV_SUCCESS)
    return -1;
  return 0;
}

/* This sets the acquisition mode and also the MINIMUM times, actual values, if desired to be changed, must be changed later
   This is so because I do not expect anyone to really want to go for gaps between exposures On the contrary, if you want a
   minimum exptime, it must be set to 0 prior to this call. */

void
Andor::setExposure (double expt)
{
  intExposure = expt;
  setTiming ();
  //        sendValueAll(exposure); // ale tohle nefunguje... :(
}

//bool Andor::supportFrameTransfer ()
//{
//      return useFT->getValueBool () && (cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER) && useRunTillAbort->getValueBool ();
//}

int
Andor::setVSSpeed (int in_vsspeed)
{
  int ret;

  ret = SetVSSpeed (in_vsspeed);
  checkRet ("setVSSpeed()", "SetVSSpeed()");
  logStream (MESSAGE_INFO) << "SetVSSpeed: VSSpeed set to " << in_vsspeed <<
    sendLog;
  setTiming ();
  return 0;
}

int
Andor::setEMCCDGain (int in_gain)
{
  int ret;

  ret = SetEMCCDGain (in_gain);
  checkRet ("setEMCCDGain()", "SetEMCCDGain()");
  emccdgain->setValueInteger (in_gain);
  return 0;
}

int
Andor::setVSAmplitude (int in_vsamp)
{
  int ret;

  ret = SetVSAmplitude (in_vsamp);
  checkRet ("setVSAmplitude()", "SetVSAmplitude()");
  return 0;
}

int
Andor::setFTShutter (bool force)
{
  FTShutter->setValueBool (force);
  return 0;
}

int
Andor::setAcqMode (int mode)
{
  // driver predpoklada, ze chci delat nulovu mezery mezi expozicema, coz se reprezentuje v tomhle pripade jako nulovy mezery
  // mezi zacatkama expozic a pak se dopocita, kolik to vlastne opravdu je, cili opravdova hodnota, ktera se objevi, bude vetsi
  // nebo rovna tomu, co se zadalo, ale pokud se zmeni podminky ktere maji na volbu vliv, bude se to dal menit.
  // bude na to funkce setTiming(), ktera by se mela zavolat po kazde akci, ktera ma vliv na zmenu parametru

  // nelibi se mi to... chce to udelat nejak lip... asi nejaky zapamatovany hodnoty, co jsem tam zadal (i kdyz nebudou videt) a
  // pri zmenach se tam budou cpat napocitany cisla vetsi nebo rovny tem, ktery se tam driv zadaly
  // myslenka je ta, ze stejne default vsech tech hodnot bude 0/1 a jen v pripade pozadavku se to zmeni
  // potrebuju se dostat i do Camera::setExposure(), protoze v pripade kinseries ma smysl zadat exposure=0, ale ve skutecnosti se
  // tam napocita neco jinyho... chjo...
  // Mimoto by asi stalo za to mit moznost zadat FPS v Hz jako nezavislou promennou, na zaklade ktery se vypocita vsechno ostatni
  // pro marketingovy pripady = kdy je jedno, jaka presne je expozice, ale hodi se rict, ze to je (presne) 25fps
  // NumberAccumulations by mel mit vazbu na NCOMBINE
  return 0;
}

int
Andor::setADCMode (int requestedMode)
{
  int ret, ad, oa, hs, pa;
  float val;

  if (requestedMode >= adcModes || requestedMode < 0)
    {
      logStream (MESSAGE_ERROR) << "SetADCMode: Wrong mode number mode=" <<
        requestedMode << sendLog;
      return -1;
    }

  // Lets make the code a little more readable
  ad = adcModeCodes[0][requestedMode];
  oa = adcModeCodes[1][requestedMode];
  hs = adcModeCodes[2][requestedMode];
  pa = adcModeCodes[3][requestedMode];

  // Update the various dependent values
  EMOn->setValueBool (defaultADCMode > 0 ? (oa == 0 ? true : false) : false);

  // These two are selections = easy, were completely prepared during init
  ret = SetADChannel (ad);
  checkRet ("setADCMode()", "SetADChannel()");
  ADChannel->setValueInteger (ad);
  sendValueAll (ADChannel);

  ret = SetOutputAmplifier (oa);
  checkRet ("setADCMode()", "SetOutputAmplifier()");
  outputAmp->setValueInteger (oa);
  sendValueAll (outputAmp);

  // What does the hs mean is taken from the driver each time
  ret = GetHSSpeed (ad, oa, hs, &val);
  checkRet ("setADCMode()", "GetHSSpeed()");

  ret = SetHSSpeed (oa, hs);
  checkRet ("setADCMode()", "SetHSSpeed()");
  HSpeed->setValueFloat (val);
  sendValueAll (HSpeed);

  // I (mates) would like HSpeed to be a selection, but it would have to be remade each time a different ad/oa selection
  // is chosen, and I cannot make work Selection->clear, so for now:

  if (cap.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN)
    {
      float pag;

      ret = GetPreAmpGain (pa, &pag);
      checkRet ("setADCMode()", "GetPreAmpGain()");
      ret = SetPreAmpGain (pa);
      checkRet ("setADCMode()", "SetPreAmpGain()");
      outPreAmpGain->setValueFloat (pag);
      sendValueAll (outPreAmpGain);
    }

  // And eventually the electronic gain info
  float sens;

  ret = GetSensitivity (ad, hs, oa, pa, &sens);
  checkRet ("setADCMode()", "GetSensitivity()");
  Sensitivity->setValueFloat (sens);
  sendValueAll (Sensitivity);

  // let camd know the image orientation changed (podnos -> zonboq)
  updateFlip ();

  logStream (MESSAGE_INFO) << "SetADCMode: ADC mode set to " << requestedMode
    << " (" << ad << oa << hs << pa << ")" << sendLog;

  setTiming ();

  return 0;
}

/*
 * TEMPERATURE CONTROL
 *
 * Ramping implemented to maintain user-level compatibility with other drivers, but not really necessary, as Andor thinks for
 * us and where needed, implemented in camera. If you are oversupersticious, change the value #define TE_RAMP_SPEED or TERAMP
 * in modefile/monitor/anyothertool to whatever you desire (like 0.06 if you want the cooling process to last for 24h)
 */

int
Andor::initChips ()
{
  int ret;
  float x_um, y_um;
  int x_pix, y_pix;

  // Set Read Mode to --Image-- (we support no other)
  ret = SetReadMode (4);
  checkRet ("initChips()", "SetReadMode()");

  ret = SetShutter (1, ANDOR_SHUTTER_CLOSED, 50, 50);
  checkRet ("initChips()", "SetShutter()");

  // iXon+ cameras can do "real gain", indicated by a flag.
  if (cap.ulEMGainCapability & 0x08)
    {
      ret = SetEMGainMode (3);
      checkRet ("initHardware()", "SetEMGainMode()");
    }

  andor_shutter_state = ANDOR_SHUTTER_CLOSED;

  //GetPixelSize returns floats, pixel[XY] are doubles
  ret = GetPixelSize (&x_um, &y_um);
  checkRet ("initChips()", "GetPixelSize()");

  pixelX = x_um;
  pixelY = y_um;

  ret = GetDetector (&x_pix, &y_pix);
  checkRet ("initChips()", "GetDetector()");

  setSize (x_pix, y_pix, 0, 0);

  // use frame transfer mode
  if (useFT->getValueBool () && (cap.ulAcqModes & AC_ACQMODE_FRAMETRANSFER))
    {
      ret = SetFrameTransferMode (1);
      checkRet ("initChips()", "SetFrameTransferMode()");
    }

  if (baselineClamp != NULL)
    {
      // This clamps the baseline level of kinetic series frames.  Didn't exist
      // in older SDK versions, is not a problem for non-KS exposures.
      ret = SetBaselineClamp (0);
      checkRet ("initHardware()", "SetBaselineClamp()");
    }

  return 0;
}

void
Andor::initCooling ()
{
  tempSet->setMin (-100);
  tempSet->setMax (40);
  updateMetaInformations (tempSet);
}

void
Andor::temperatureCheck ()
{
  if (!isIdle ())
    return;

  addTempCCDHistory (getTemp ());
}

void
Andor::postEvent (rts2core::Event * event)
{
  float change = 0;
  int status;

  switch (event->getType ())
    {
    case EVENT_TE_RAMP:
      if (isnan (tempTarget->getValueFloat ()))
        {
          if (isnan (tempCCD->getValueFloat ()))
            {
              addTimer (10, event);     // give info () time to get tempCCD
              return;
            }
          else
            tempTarget->setValueFloat (tempCCD->getValueFloat ());
        }
      if (tempTarget->getValueFloat () < tempSet->getValueFloat ())
        change = tempRamp->getValueFloat () * TE_RAMP_INTERVAL / 60.;
      else if (tempTarget->getValueFloat () > tempSet->getValueFloat ())
        change = -1 * tempRamp->getValueFloat () * TE_RAMP_INTERVAL / 60.;
      if (change != 0)
        {
          tempTarget->setValueFloat (tempTarget->getValueFloat () + change);
          if (fabs (tempTarget->getValueFloat () - tempSet->getValueFloat ())
              < fabs (change))
            tempTarget->setValueFloat (tempSet->getValueFloat ());
          sendValueAll (tempTarget);
          if ((status =
               SetTemperature ((int) tempTarget->getValueFloat ())) !=
              DRV_SUCCESS)
            logStream (MESSAGE_ERROR) << "postEvent: SetTemperature error=" <<
              status << sendLog;
          addTimer (TE_RAMP_INTERVAL, event);
          return;
        }
      break;
    }
  Camera::postEvent (event);
}

float
Andor::getTemp ()
{
  int ret;
  float tmpTemp;

  ret = GetTemperatureF (&tmpTemp);
  if (!
      (ret == DRV_ACQUIRING || ret == DRV_NOT_INITIALIZED
       || ret == DRV_ERROR_ACK))
    {
      tempCCD->setValueDouble (tmpTemp);
      switch (ret)
        {
        case DRV_TEMPERATURE_OFF:
          tempStatus->setValueInteger (0);
          break;
        case DRV_TEMPERATURE_NOT_STABILIZED:
          tempStatus->setValueInteger (1);
          break;
        case DRV_TEMPERATURE_STABILIZED:
          tempStatus->setValueInteger (2);
          break;
        case DRV_TEMPERATURE_NOT_REACHED:
          tempStatus->setValueInteger (3);
          break;
        case DRV_TEMPERATURE_OUT_RANGE:
          tempStatus->setValueInteger (4);
          break;
        case DRV_TEMPERATURE_NOT_SUPPORTED:
          tempStatus->setValueInteger (5);
          break;
        case DRV_TEMPERATURE_DRIFT:
          tempStatus->setValueInteger (6);
          break;
        default:
          tempStatus->setValueInteger (7);
          logStream (MESSAGE_DEBUG) << "getTemp: unknown TEC status " << ret
            << sendLog;
        }
    }
  else
    {
      if (ret == DRV_NOT_INITIALIZED || ret == DRV_ERROR_ACK)
        logStream (MESSAGE_ERROR) << "getTemp: GetTemperatureF error=" << ret
          << sendLog;
      // else it is acquiring, no need to report
    }

#ifdef TEMP_SAFETY
  int trip;

  ret = GetTECStatus (&trip);
  if (ret != DRV_SUCCESS)
    logStream (MESSAGE_ERROR) << "getTemp: GetTECStatus error=" << ret <<
      sendLog;
  if ((trip == 1) && (tempSafety->getValueInteger () == 0))
    {
      logStream (MESSAGE_ERROR) << "System overheating protection tripped !!!"
        << ret << sendLog;
      tempSafety->setValueInteger (1);
    }
  if ((trip == 0) && (tempSafety->getValueInteger () == 1))
    {
      logStream (MESSAGE_ERROR) << "System overheating protection is now off."
        << ret << sendLog;
      tempSafety->setValueInteger (0);
    }
#endif
  return tmpTemp;
}

int
Andor::switchCooling (bool cooling)
{
  int ret;

  if (cooling)
    {
      if ((ret = CoolerON ()) != DRV_SUCCESS)
        {
          logStream (MESSAGE_ERROR) << "switchCooling: CoolerON error=" << ret
            << sendLog;
          return -1;
        }
    }
  else if ((ret = CoolerOFF ()) != DRV_SUCCESS)
    {
      logStream (MESSAGE_ERROR) << "switchCooling: CoolerOFF error=" << ret <<
        sendLog;
      return -1;
    }

  Camera::switchCooling (cooling);

  return ret;
}

int
Andor::setCoolTemp (float new_temp)
{
  if (coolingOnOff->getValueBool () || new_temp == 40.0)
    {
      deleteTimers (EVENT_TE_RAMP);
      tempTarget->setValueFloat (tempCCD->getValueFloat ());
      addTimer (1, new rts2core::Event (EVENT_TE_RAMP));
      // temperature will be changed in postEvent() one second later
    }
  return Camera::setCoolTemp (new_temp);
}

/***************************/

int
Andor::info ()
{
  if (isIdle ())
    {
      getTemp ();
    }
  return Camera::info ();
}

void
Andor::afterNight ()
{
  CoolerOFF ();
  SetTemperature (20);
  tempSet->setValueDouble (+50);
  closeShutter ();
}

// ***** INITIALIZATION ***** //

Andor::Andor (int in_argc, char **in_argv):
Camera (in_argc, in_argv)
{
  /*
   * INIT TC TEMP management
   * These things cannot be called anywhere else than in a constructor: I find this ugly :(
   * apart of that, these functions are weird .h "inlines", they get compiled each time the .h is used
   */
  createTempCCD ();
  createTempSet ();
  createTempCCDHistory ();

  createValue (tempRamp, "TERAMP", "[C/min] temperature ramping", false,
               RTS2_VALUE_WRITABLE);
  tempRamp->setValueFloat (TE_RAMP_SPEED);
  createValue (tempTarget, "TETAR", "[C] current target temperature", false);

  // this is not necessary here, but if I dont, it would appear away from the rest of TE related stuff
  createValue (tempStatus, "TCSTATUS", "Thermoelectric cooling status",
               false);
  tempStatus->addSelVal ("OFF");
  tempStatus->addSelVal ("NOT_STABILIZED");
  tempStatus->addSelVal ("STABILIZED");
  tempStatus->addSelVal ("NOT_REACHED");
  tempStatus->addSelVal ("OUT_RANGE");
  tempStatus->addSelVal ("NOT_SUPPORTED");
  tempStatus->addSelVal ("DRIFT");
  tempStatus->addSelVal ("UNKNOWN");
#ifdef TEMP_SAFETY
  createValue (tempSafety, "TCSAFETY", "TC overheating safety mechanism",
               false);
  tempStatus->addSelVal ("OK");
  tempStatus->addSelVal ("OVERHEATED");
#endif

  andorRoot = ANDOR_ROOT;

  VSAmp = NULL;
  baselineClamp = NULL;
  baselineOff = NULL;

  createExpType ();
  createDataChannels ();
  setNumChannels (1);

  createValue (FTShutter, "FTSHUT", "Use shutter, even with FT", true,
               RTS2_VALUE_WRITABLE, CAM_WORKING);
  FTShutter->setValueBool (false);

  // ACQ Modes
  createValue (acqMode, "ACQMODE", "acqusition mode", true,
               RTS2_VALUE_WRITABLE, CAM_WORKING);
  acqMode->addSelVal ("Reserved");      // 0 included to offset the numbers to be in agreement with andor docs
  acqMode->addSelVal ("Single Scan");   // 1 single image, normal mode
  acqMode->addSelVal ("Accumulate");    // 2 behaves as a single image, only time delays are different
  acqMode->addSelVal ("Kinetic Series");        // 3 behaves as 2, but spits data during acquisition
  acqMode->addSelVal ("Fast Kinetics"); // 4 behaves as 1, requires special configuration, result is a single image
  acqMode->addSelVal ("Run Till Abort");        // 5 behaves as 3, but does not stop (i.e. time delays are infinite)
  acqMode->setValueInteger (1);

  // createValue (Exptime, "EXPTIME", "Exposure length (acqMode=all)", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
  // If you think this is crazy, I agree. :)
  //      createValue (accCycleGap, "ACGAP", "[s] delay between accumulations (acqMode=[2,3,4])", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
  createValue (accCycle, "ACCCYCLE",
               "[s] Andor accumulation cycle (acqMode=[2,3,4])", true, 0,
               CAM_WORKING);
  createValue (accNumber, "ACCNUM", "Number of accumulations (acqMode=[2,3])",
               true, RTS2_VALUE_WRITABLE, CAM_WORKING);
  //      createValue (kinCycleGap, "KCGAP", "[s] Kinetic cycle delay (acqMode=[3,5])", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
  createValue (kinCycle, "KINCYCLE", "Kinetic cycle time (acqMode=[3,5])",
               true, 0, CAM_WORKING);
  createValue (kinNumber, "KINNUM",
               "Number in kinetic series (acqMode=[3,4])", true,
               RTS2_VALUE_WRITABLE, CAM_WORKING);
  createValue (kinFile, "KINFILE",
               "Name of the kinetic series datafile (acqMode=3)", true,
               RTS2_VALUE_AUTOSAVE);
  createValue (imgFreq, "IMGFREQ",
               "[Hz][fps] Andor imaging frequency (acqMode=[2,3,4])", true, 0,
               CAM_WORKING);
  createValue (acquireTime, "ACQTIME", "One complete cycle acquire time",
               true, 0, CAM_WORKING);
  //accCycleGap->setValueFloat (0.0);
  //kinCycleGap->setValueFloat (0.0);
  accCycle->setValueFloat (0.0);
  accNumber->setValueInteger (1);
  kinCycle->setValueFloat (0.0);
  kinNumber->setValueInteger (1);
  acquireTime->setValueDouble (0.0);
  intExposure = 0;

  createValue (useFT, "USEFT", "Use FT", true, RTS2_VALUE_WRITABLE,
               CAM_WORKING);
  useFT->setValueBool (true);

  // it is disabled, as it produces wrong data
  // createValue (useRunTillAbort, "USERTA", "Use run till abort mode of the CCD", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
  // useRunTillAbort->setValueBool (false);

  // outPreAmpGain = NULL;

  emAdvanced = NULL;
  emGainMode = NULL;

  fanMode = NULL;

  createValue (filterCr, "FILTCR", "filter cosmic ray events", true,
               RTS2_VALUE_WRITABLE, CAM_WORKING);
  filterCr->setValueBool (false);

  defaultGain = IXON_DEFAULT_GAIN;

  printSpeedInfo = false;

  addOption (OPT_ANDOR_ROOT, "root", 1,
             "directory with Andor detector.ini file");
  addOption ('g', "gain", 1, "set camera gain level (0-255)");
  addOption ('N', "noft", 0, "do not use frame transfer mode");
  addOption ('I', "speed_info", 0,
             "print speed info - information about speed available");
  addOption ('S', "ft-uses-shutter", 0, "force use of shutter with FT");

  //      Camera::setBinning(0,0);
}

Andor::~Andor (void)
{
  ShutDown ();
}

void
Andor::initDataTypes ()
{
  Camera::initDataTypes ();
  addDataType (RTS2_DATA_LONG);
  addDataType (RTS2_DATA_FLOAT);
}

/*
 * Camera ID, serial number etc.
 *
 * pretty boring stuff independent on anything else
 */

int
Andor::initAndorID ()
{
  char textbuf[TEXTBUF_LEN];
  char name[128];
  int n = 0, ret;

  // Set camera type etc.
  switch (cap.ulCameraType)
    {                           // only tested types, see definitions for more
    case AC_CAMERATYPE_IXON:
      n = snprintf (textbuf, TEXTBUF_LEN, "ANDOR iXon");
      break;
    case AC_CAMERATYPE_IXONULTRA:
      n = snprintf (textbuf, TEXTBUF_LEN, "ANDOR iXon-Ultra");
      break;
    default:
      n =
        snprintf (textbuf, TEXTBUF_LEN, "ANDOR type %lu",
                  (unsigned long) cap.ulCameraType);
      break;
    }

  GetHeadModel (name);
  snprintf (textbuf + n, TEXTBUF_LEN - n, " %s", name);
  ccdRealType->setValueCharArr (textbuf);

  // Serial Number
  int serNum;

  ret = GetCameraSerialNumber (&serNum);
  checkRet ("initAndorID()", "GetCameraSerialNumber()");
  serialNumber->setValueInteger (serNum);

  // XXX Missing: the 1024x1024 iXon uses CCD201-20, the 512x512 uses
  // CCD97-00, any other cases could be later fixed here within a case, as
  // Andor does not reveal the CCD type via the API

  // end Set camera type etc.
  return 0;
}

int
Andor::initAndorValues ()
{
  if (cap.ulSetFunctions & AC_SETFUNCTION_VSAMPLITUDE)
    {
      createValue (VSAmp, "SAMPLI", "Used andor shift amplitude", true,
                   RTS2_VALUE_WRITABLE, CAM_WORKING);
      VSAmp->addSelVal ("normal");
      VSAmp->addSelVal ("+1");
      VSAmp->addSelVal ("+2");
      VSAmp->addSelVal ("+3");
      VSAmp->addSelVal ("+4");
      VSAmp->setValueInteger (0);
    }

  if (cap.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)
    {
      createValue (emccdgain, "EMCCDGAIN", "EM CCD gain", true,
                   RTS2_VALUE_WRITABLE, CAM_WORKING);
      setEMCCDGain (defaultGain);

      if (cap.ulEMGainCapability != 0)
        {
          createValue (emAdvanced, "EMADV",
                       "permit EM more than 300x (use with caution)", true,
                       RTS2_VALUE_WRITABLE, CAM_WORKING);
          emAdvanced->setValueBool (false);

          createValue (emGainMode, "GAINMODE", "EM gain mode", true,
                       RTS2_VALUE_WRITABLE, CAM_WORKING);
          emGainMode->addSelVal ("DAC 8bit");
          emGainMode->addSelVal ("DAC 12bit");
          emGainMode->addSelVal ("LINEAR 12bit");
          emGainMode->addSelVal ("REAL 12bit");
        }
    }

  if (cap.ulSetFunctions & AC_SETFUNCTION_BASELINECLAMP)
    {
      createValue (baselineClamp, "BASECLAM", "if baseline clamp is activer",
                   true, RTS2_VALUE_WRITABLE, CAM_WORKING);
      baselineClamp->setValueBool (false);
    }

  if (cap.ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET)
    {
      createValue (baselineOff, "BASEOFF", "baseline offset value", true,
                   RTS2_VALUE_WRITABLE, CAM_WORKING);
      baselineOff->setValueInteger (0);
    }

  // XXX this is in 5 drivers, could be unified
  if (cap.ulFeatures & AC_FEATURES_FANCONTROL)
    {
      createValue (fanMode, "FAN", "FAN mode", true, RTS2_VALUE_WRITABLE,
                   CAM_WORKING);
      fanMode->addSelVal ("FULL");
      fanMode->addSelVal ("LOW");
      fanMode->addSelVal ("OFF");
    }
  return 0;
}

/*
 * ADC Configuration
 *
 * Andor SDK has no term to describe the digitization and charge transfer process, I call it here ADC Modes, it includes both
 * charge transfer and AD conversion, as they are closely related.
 */

int
Andor::initAndorADCModes ()
{
  char textbuf[TEXTBUF_LEN];
  int ret;

  // Get modes (AD Channels/outputAmp/HSpeed)
  createValue (adcMode, "ADCMODE", "ADC mode (head specific)", true,
               RTS2_VALUE_WRITABLE, CAM_WORKING);
  createValue (EMOn, "EMON", "Electron Multiplying status", true,
               RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
  createValue (ADChannel, "ADCHANEL", "Used andor AD Channel", true, 0,
               CAM_WORKING);
  createValue (outputAmp, "OUTAMP", "[bit] Output amplifier", true, 0,
               CAM_WORKING);
  createValue (HSpeed, "HSPEED", "[MHz] Horizontal shift speed (string)",
               true, 0, CAM_WORKING);

  if (cap.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN)
    {
      createValue (outPreAmpGain, "PREAMP", "output preamp gain", true, 0,
                   CAM_WORKING);
      outPreAmpGain->setValueInteger (0);
    }

  createValue (Sensitivity, "GAIN", "[e-/ADU] (conventional) CCD gain", true,
               0, CAM_WORKING);

  // Get AD Channels
  int n_ad;

  ret = GetNumberADChannels (&n_ad);
  checkRet ("initAndorADCModes()", "GetNumberADChannels()");

  for (int ad = 0; ad < n_ad; ad++)
    {
      int depth;

      ret = GetBitDepth (ad, &depth);
      checkRet ("initAndorADCModes()", "GetBitDepth()");

      snprintf (textbuf, TEXTBUF_LEN, "%d-bit", depth);
      ADChannel->addSelVal (textbuf);
    }

  // Get Output Aplifiers
  int n_oa;

  ret = GetNumberAmp (&n_oa);
  checkRet ("initAndorADCModes()", "GetBitDepth()");

  for (int oa = 0; oa < n_oa; oa++)
    outputAmp->addSelVal (outputAmpDescLong[oa < 2 ? oa : 2]);

  // Get Horizontal Frequencies for all combinations of ad/oa
  adcModes = 0;
  defaultADCMode = -1;
  for (int ad = 0; ad < n_ad; ad++)
    {
      int depth;

      ret = GetBitDepth (ad, &depth);
      checkRet ("initAndorADCModes()", "GetBitDepth()");

      for (int oa = 0; oa < n_oa; oa++)
        {
          int nhs;

          ret = GetNumberHSSpeeds (ad, oa, &nhs);
          checkRet ("initAndorADCModes()", "GetBitDepth()");

          for (int hs = 0; hs < nhs; hs++)
            {
              float val;

              // this selects the first (fastest) non-em mode as the default
              // or the fastest mode if there is only one oa
              if (defaultADCMode < 0 && (n_oa <= 1 || oa > 0))
                defaultADCMode = adcModes;

              ret = GetHSSpeed (ad, oa, hs, &val);
              checkRet ("initAndorADCModes()", "GetHSSpeed()");

              float sens;
              int npa;

              ret = GetNumberPreAmpGains (&npa);
              checkRet ("initAndorADCModes()", "GetNumberPreAmpGains()");

              // prints available preAmpGains
              for (int pa = 0; pa < npa; pa++)
                {
                  float pag;
                  int isPreAmp;

                  GetPreAmpGain (pa, &pag);
                  IsPreAmpGainAvailable (ad, oa, hs, pa, &isPreAmp);
                  GetSensitivity (ad, hs, oa, pa, &sens);

                  //fprintf (stderr, "+ mode:%d ad:%d oa:%d hs:%d pa:%d -- pag:%f, isPreAmp: %d, sens: %f\n", adcModes, ad, oa, hs, pa, pag, isPreAmp, sens);

#ifdef SDK_2_102_U888_FIX
                  if (isPreAmp || (ad == 0 && oa == 1 && hs == 0 && pa == 1))
#else
                  if (isPreAmp)
#endif

                    {
                      if (adcModes < MAX_ADC_MODES)
                        {
                          adcModeCodes[0][adcModes] = ad;
                          adcModeCodes[1][adcModes] = oa;
                          adcModeCodes[2][adcModes] = hs;
                          adcModeCodes[3][adcModes] = pa;

                          //fprintf (stderr, "mode:%d ad:%d oa:%d hs:%d pa:%d\n", adcModes, ad, oa, hs, pa);

                          adcModes++;
                        }
                      else
                        {
                          logStream (MESSAGE_ERROR) <<
                            "Camera has too many readout modes (" << adcModes
                            <<
                            "), modify MAX_ADC_MODES in andor.cpp and recompile"
                            << sendLog;
                          adcModes++;
                        }

                      snprintf (textbuf, TEXTBUF_LEN, "%s-%04.1f-%d-%.1f",
                                outputAmpDescShort[oa < 2 ? oa : 2], val,
                                depth, pag);
                      adcMode->addSelVal (textbuf);
                    }
                }
            }
        }
    }                           //  end of output amplifiers
  logStream (MESSAGE_INFO) << "defaultADCMode = " << defaultADCMode <<
    sendLog;

  // We have to wait with setting the default mode for chip initialization (to know the CCD size)

  // get VSpeed(s) (luckily these are well independent on the rest)
  createValue (VSpeed, "VSPEED", "Vertical shift speed", true,
               RTS2_VALUE_WRITABLE, CAM_WORKING);
  int vspeeds;

  ret = GetNumberVSSpeeds (&vspeeds);
  checkRet ("initAndorADCModes()", "GetNumberPreAmpGains()");
  for (int s = 0; s < vspeeds; s++)
    {
      //std::ostringstream os;
      float val;

      GetVSSpeed (s, &val);
      //os << val << " usec/pix";
      snprintf (textbuf, TEXTBUF_LEN, "%.2f usec/pix", val);
      // VSpeed->addSelVal (os.str ());
      VSpeed->addSelVal (textbuf);
    }

  // Possibly make a configurable default value
  VSpeed->setValueInteger (1);
  // end VSpeed
  return 0;
}

int
Andor::initHardware ()
{
  int ret;

  ret = Initialize (andorRoot);
  checkRet ("initHardware()", "Initialize()");

  sleep (2);                    //sleep to allow initialization to complete

  SetCoolerMode (1);            // maintain temperature after shutdown

  cap.ulSize = sizeof (AndorCapabilities);
  ret = GetCapabilities (&cap);
  checkRet ("initHardware()", "GetCapabilities()");

  ret = initAndorID ();
  checkRet ("initHardware()", "initAndorID()");

  ret = initAndorADCModes ();
  checkRet ("initHardware()", "initAndorADCModes()");

  ret = initAndorValues ();
  checkRet ("initHardware()", "initAndorValues()");

  ret = initChips ();
  checkRet ("initHardware()", "initChips()");

  // After we know the chip parameters, we can set default adcMode, which also does setTiming () implicitly...
  //changeValue (adcMode, defaultADCMode);
  adcMode->setValueInteger (defaultADCMode);
  ret = setADCMode (defaultADCMode);
  checkRet ("initHardware()", "setADCMode()");

  kinFile->setValueCharArr ("none");

  // added the following code to get quick updates in rts2-mon.. (SG)
  setIdleInfoInterval (2);

  return 0;
}

/*
 * EXECUTABLE MAIN
 */

void
Andor::help ()
{
  std::cout << "Driver for Andor CCDs (iXon & others)" << std::endl << std::
    endl <<
    "\tOptimal values for speeds on iXon are: HSPEED=0 VSPEED=0. Those can be changed by modefile, specified by --modefile argument. There is an example modefile, which specify two modes:\n"
    << std::
    endl << "[default]\n" "EMON=on\n" "HSPEED=1\n" "VSPEED=1\n" "\n"
    "[classic]\n" "EMON=off\n" "HSPEED=3\n" "VSPEED=1\n" << std::endl;
  Camera::help ();
}

void
Andor::usage ()
{
  std::cout << "\t" << getAppName () << " -c -70 -d C1" << std::endl;
}

int
Andor::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'g':
      defaultGain = atoi (optarg);
      if (defaultGain > IXON_MAX_GAIN || defaultGain < 0)
        {
          printf ("gain must be in 0-%d range\n", IXON_MAX_GAIN);
          exit (EXIT_FAILURE);
        }
      break;
    case OPT_ANDOR_ROOT:
      andorRoot = optarg;
      break;
    case 'I':
      printSpeedInfo = true;
      break;
    case 'N':
      useFT->setValueBool (false);
      break;
    case 'S':
      shutter_with_ft = true;
    default:
      return Camera::processOption (in_opt);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  Andor device (argc, argv);

  return device.run ();
}

#define _CCD_C

#ifdef RTS2_HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <sys/io.h>
#include <unistd.h>

#ifndef NULL
#define NULL 0
#endif

#include "urvc.h"
//#include "ccd_macros.h"

//      inline void 
//CameraPulse(int a)
//{
//      CameraOut(0x30,a|8);
//      CameraOut(0x30,a);
//}

PAR_ERROR
SPPGetPixel (unsigned short *vidPtr, CCD_ID chip)
{
  int ret = CE_NO_ERROR;
  unsigned short u;

  // asi to tu bejt nemusi... (skoro vzdycky se to stiha driv, chyba je ovsem problem... :(
  if ((ret = CameraReady ()))
    return ret;

  CameraOut (0x30, chip | 8);
  CameraOut (0x30, chip);

  // Get short
  u = CameraIn (0x00);
  u += CameraIn (0x10) << 4;
  u += CameraIn (0x20) << 8;
  u += CameraIn (0x30) << 12;

  *vidPtr = u;

  return ret;
}

PAR_ERROR
ClockAD (int len)
{
  short i;			//, t0;
  PAR_ERROR res = CE_NO_ERROR;
  unsigned short u;

  CameraOut (0, 4);
  for (i = 0; i < len; i++)
    if ((res = SPPGetPixel (&u, MAIN_CCD)))
      return res;
  return CameraReady ();
}

void
SetVdd (MY_LOGICAL raiseIt)
{
  unsigned char ic;

  ic = imaging_clocks_out;
  if (raiseIt)
    CameraOut (0x10, 0);
  else
    CameraOut (0x10, 4);

  if (raiseIt && (ic & 4))
    TimerDelay (200000);
}

PAR_ERROR
CCDExpose (CAMERA * C, int Time, int Shutter)
{
  PAR_ERROR ret = CE_NO_ERROR;
//    int vertTotal=C->vertBefore+C->vertImage+C->vertAfter;
//    int horzTotal=C->horzBefore+C->horzImage+C->horzAfter;
  StartExposureParams sep;

  sep.ccd = C->ccd;
  sep.exposureTime = Time;
  sep.openShutter = Shutter;
  sep.abgState = 0;

  if (C->cameraID == ST237A_CAMERA || C->cameraID == ST237A_CAMERA)
    {
      // setup TC-237 exposure...
      PulseOutParams pop;
      pop.numberPulses = 0;
      pop.pulseWidth = 0;
      pop.pulsePeriod = Shutter;
      ret = MicroCommand (MC_PULSE, C->cameraID, &pop, NULL);
      usleep (700000);		// to turn it...

      sep.abgState = 2;
      ret = MicroCommand (MC_CONTROL_CCD, C->cameraID, NULL, NULL);
      CameraOut (0, 4);
    }
  else
    // readout the array to clean it up...
  if ((ret = C->ClearArray (C->vertTotal, 4, C->horzTotal)))
    return ret;

  ret = MicroCommand (MC_START_EXPOSURE, C->cameraID, &sep, NULL);

  return ret;
}

int
CCDImagingState (CAMERA * C)
{
  PAR_ERROR ret = CE_NO_ERROR;
  StatusResults sr;

  ret = MicroCommand (MC_STATUS, C->cameraID, NULL, &sr);

  if (ret)
    return -1;

  if (C->ccd)
    return sr.trackingState;
  else
    return sr.imagingState;
}

PAR_ERROR
OpenCCD (int CCD_num, CAMERA ** cptr)
{
  GetVersionResults st;
  PAR_ERROR ret = CE_NO_ERROR;
  EEPROMContents eePtr;
  CameraDescriptionType *C;
  int chip = CCD_num & 1;
  int allframe = CCD_num & 2;

  (*cptr) = (CAMERA *) malloc (sizeof (CAMERA));

  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  if ((ret = GetEEPROM (st.cameraID, &eePtr)))
    return ret;
  C = &(Cams[eePtr.model]);

  // Tracking chip
  if ((st.cameraID == ST7_CAMERA) && (chip == 1))
    C = &(Cams[0]);

  if (allframe)
    {
      (*cptr)->horzTotal =
	C->horzDummy + C->horzBefore + C->horzAfter + C->horzImage;
      (*cptr)->vertTotal = C->vertBefore + C->vertAfter + C->vertImage;

      (*cptr)->horzBefore = C->horzDummy;
      (*cptr)->vertBefore = 0;
      (*cptr)->horzAfter = 0;
      (*cptr)->vertAfter = 0;
      (*cptr)->horzImage = C->horzBefore + C->horzAfter + C->horzImage;
      (*cptr)->vertImage = C->vertBefore + C->vertAfter + C->vertImage;
    }
  else
    {
      (*cptr)->horzTotal =
	C->horzDummy + C->horzBefore + C->horzAfter + C->horzImage;
      (*cptr)->vertTotal = C->vertBefore + C->vertAfter + C->vertImage;

      (*cptr)->horzBefore =
	C->horzDummy + C->horzBefore, (*cptr)->vertBefore = C->vertBefore;
      (*cptr)->horzAfter = C->horzAfter, (*cptr)->vertAfter = C->vertAfter;
      (*cptr)->horzImage = C->horzImage, (*cptr)->vertImage = C->vertImage;
    }

  (*cptr)->ccd = CCD_num;
  (*cptr)->cameraID = st.cameraID;

  if (st.cameraID == ST237_CAMERA || st.cameraID == ST237A_CAMERA)
    {
      (*cptr)->DigitizeLine = DigitizeLine_st237;
      (*cptr)->DumpPixels = DumpPixels_st237;
      (*cptr)->DumpLines = DumpLines_st237;
      (*cptr)->ClearArray = ClearArray_st237;
    }
  else if (chip)
    {
      (*cptr)->DigitizeLine = DigitizeLine_trk;
      (*cptr)->DumpPixels = DumpPixels_trk;
      (*cptr)->DumpLines = DumpLines_trk;
      (*cptr)->ClearArray = ClearArray_trk;
    }
  else
    {
      (*cptr)->DigitizeLine = DigitizeLine_st7;
      (*cptr)->DumpPixels = DumpPixels_st7;
      (*cptr)->DumpLines = DumpLines_st7;
      (*cptr)->ClearArray = ClearArray_st7;
    }
  return ret;
}

void
CloseCCD (CAMERA * C)
{
  if (C)
    free (C);
}

PAR_ERROR
CCDReadout (unsigned short *buffer, CAMERA * C, int tx, int ty, int tw,
	    int th, int bin)
{
  PAR_ERROR ret = CE_NO_ERROR;
  int i;
  EndExposureParams eep;
  QueryTemperatureStatusResults qtsr;
  MicroTemperatureRegulationParams cool;

  eep.ccd = C->ccd;
  ret = MicroCommand (MC_END_EXPOSURE, C->cameraID, &eep, NULL);

  /* freeze cooling to avoid noise */
  ret = MicroCommand (MC_TEMP_STATUS, C->cameraID, NULL, &qtsr);

  if (qtsr.enabled)
    {
      cool.regulation = 2;
      cool.ccdSetpoint = qtsr.power;
      cool.preload = 0;
      ret = MicroCommand (MC_REGULATE_TEMP, C->cameraID, &cool, NULL);
    }

  /* dump lines which are not part of an active area in binning 1, others
   * in an appropriate binning */
  if (C->vertBefore)
    if ((ret = C->DumpLines (C->horzTotal, C->vertBefore, 1)))
      return ret;
  if (ty)
    if ((ret = C->DumpLines (C->horzTotal / bin, ty, bin)))
      return ret;

  /* disable interrupts to avoid scratches */
  disable ();

  // cleanup vertical CCD
  C->DumpPixels (C->horzTotal);

  /* read out lines in cycle */
  for (i = 0; i < th; i++)
    {
      CameraOut (0x60, 1);
      SetVdd (1);
      if ((ret =
	   C->DigitizeLine (tx, tw, C->horzBefore, bin, buffer + i * tw,
			    C->vertTotal)))
	goto readout_end;
    }

readout_end:
  enable ();

  /* Unfreeze cooling */
  if (qtsr.enabled)
    {
      cool.regulation = 1;
      cool.ccdSetpoint = qtsr.ccdSetpoint;
      cool.preload = qtsr.power;
      ret = MicroCommand (MC_REGULATE_TEMP, C->cameraID, &cool, NULL);
    }

  /* Close ST237 vane */
  if (C->cameraID == ST237A_CAMERA || C->cameraID == ST237A_CAMERA)
    {				// close fw
      PulseOutParams pop;
      pop.numberPulses = 0;
      pop.pulseWidth = 0;
      pop.pulsePeriod = 2;
      ret = MicroCommand (MC_PULSE, C->cameraID, &pop, NULL);
      CameraReady ();
//              usleep(600000); // to turn it...
    }

  return ret;
}

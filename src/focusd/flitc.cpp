/**
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Markus Wildi <markus.wildi@one-arcsec.org>
 *
 * temperature compensation: 2011, Markus Wildi <markus.wildi@one-arcsec.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "focusd.h"
#include "libfli.h"
#define  OPT_FLI_METEO_DEVICE      OPT_LOCAL + 135
#define  OPT_FLI_METEO_TEMPERATURE OPT_LOCAL + 136
#define  OPT_FLI_TC_TEMP_REF       OPT_LOCAL + 137
#define  OPT_FLI_TC_OFFSET         OPT_LOCAL + 138
#define  OPT_FLI_TC_SLOPE          OPT_LOCAL + 139
#define  OPT_FLI_TC_MODE           OPT_LOCAL + 140
#define  ABSOLUTE_TC 0
#define  RELATIVE_TC 1
#define  NO_TC 2

namespace rts2focusd
{

/**
 * FLI focuser driver. You will need FLIlib and option to ./configure
 * (--with-fli=<llibflidir>) to get that running. Please read ../../INSTALL.fli
 * for instructions.
 * @file Driver for Finger Lake Instruments CCD, temperature compensation
 * @author Petr Kubanek <petr@kubanek.net>, Markus Wildi wildi.markus@bluewin.ch
 */
class Fli:public Focusd
{
    public:
        Fli (int argc, char **argv);
        virtual ~ Fli (void);

        virtual int commandAuthorized (rts2core::Connection * conn);

    protected:
                virtual int willConnect (rts2core::NetworkAddress * in_addr);
        virtual int isFocusing ();
        virtual bool isAtStartPosition ();

        virtual int processOption (int in_opt);
        virtual int initHardware ();
        virtual int initValues ();
        virtual int idle ();
        virtual int info ();
        virtual int setTo (double num);
                virtual double tcOffset () ;
        virtual void meteo ();
        virtual void valueChanged (rts2core::Value *changed_value);

    private:
        flidev_t dev;
        flidomain_t deviceDomain;
        const char *name;

        int fliDebug;
                char *meteoDevice;
                char *meteoVariable;
                rts2core::ValueDouble *TCtemperatureRef;
                rts2core::ValueDouble *TCoffset;
                rts2core::ValueDouble *TCslope;
                rts2core::ValueSelection *TCmode;
                rts2core::ValueFloat *TCFocOffset;

        int TCmodeValue;
                char *TCmodeStr ;

                rts2core::ValueDouble *temperatureMeteo;
                rts2core::ValueDouble *temperatureFLI;
};

};

using namespace rts2focusd;

Fli::Fli (int argc, char **argv):Focusd (argc, argv)
{
    dev = -1;
    deviceDomain = FLIDEVICE_FOCUSER | FLIDOMAIN_USB;
    fliDebug = FLIDEBUG_NONE;
    name = NULL;

    createValue (temperatureFLI, "TEMP_FLI", "temperature from the FLI device", true); //go to FITS
    createValue (temperatureMeteo, "TEMP_METEO", "temperature from the meteo device", true); //go to FITS
    createValue (TCtemperatureRef, "TC_TEMP_REF", "temperature at the time when FOC_DEF was set", false, RTS2_VALUE_WRITABLE);
    createValue (TCFocOffset, "FOC_TC", "absolute position or offset calculated by temperature compensation", false);
    createValue (TCoffset, "TCP0", "y-axis offset of the linear temperature model", false);
    createValue (TCslope, "TCP1", "slope of the linear temperature model", false);
    createValue (TCmode, "TCMODE", "temperature compensation absolute, relative to FOC_DEF, no tc", false, RTS2_VALUE_WRITABLE);

    addOption ('D', "domain", 1, "CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
    addOption ('b', "fli_debug", 1, "FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");
    addOption ('f', NULL, 1, "FLI device path");

    addOption (OPT_FLI_METEO_DEVICE, "meteoDevice",  1, "meteo device name to monitor its temperature");
    addOption (OPT_FLI_METEO_TEMPERATURE, "meteoVariable",  1, "meteo device temperature variable name");
    addOption (OPT_FLI_TC_TEMP_REF, "TCtempRef",  1, "temperature compensation reference temperature (when FOC_DEF was set)");
    addOption (OPT_FLI_TC_OFFSET, "TCoffset",  1, "temperature compensation y-axis offset");
    addOption (OPT_FLI_TC_SLOPE, "TCslope",  1, "temperature compensation slope");
    addOption (OPT_FLI_TC_MODE, "TCmode",  1, "temperature compensation absolute, relative, default: none");

    TCmode->addSelVal("absolute") ;
    TCmode->addSelVal("relative") ;
    TCmode->addSelVal("none") ;

}

Fli::~Fli (void)
{
    FLIClose (dev);
}

int Fli::processOption (int in_opt)
{
    switch (in_opt)
    {
        case 'D':
            deviceDomain = FLIDEVICE_FOCUSER;
            if (!strcasecmp ("USB", optarg))
                deviceDomain |= FLIDOMAIN_USB;
            else if (!strcasecmp ("LPT", optarg))
                deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
            else if (!strcasecmp ("SERIAL", optarg))
                deviceDomain |= FLIDOMAIN_SERIAL;
            else if (!strcasecmp ("INET", optarg))
                deviceDomain |= FLIDOMAIN_INET;
            else if (!strcasecmp ("SERIAL_19200", optarg))
                deviceDomain |= FLIDOMAIN_SERIAL_19200;
            else if (!strcasecmp ("SERIAL_1200", optarg))
                deviceDomain |= FLIDOMAIN_SERIAL_1200;
            else
                return -1;
            break;
        case 'b':
            switch (atoi (optarg))
            {
                case 1:
                    fliDebug = FLIDEBUG_FAIL;
                    break;
                case 2:
                    fliDebug = FLIDEBUG_FAIL | FLIDEBUG_WARN;
                    break;
                case 3:
                    fliDebug = FLIDEBUG_ALL;
                    break;
            }
            break;
        case 'f':
            name = optarg;
            break;


            case OPT_FLI_METEO_DEVICE:
                        meteoDevice = optarg;
                        break;
            case OPT_FLI_METEO_TEMPERATURE:
                        meteoVariable = optarg;
                        break;

            case OPT_FLI_TC_TEMP_REF:
                TCtemperatureRef->setValueDouble(atof(optarg)) ;
                        break;
            case OPT_FLI_TC_OFFSET:
                TCoffset->setValueDouble(atof(optarg)) ;
                        break;

            case OPT_FLI_TC_SLOPE:
                TCslope->setValueDouble(atof(optarg)) ;
                        break;

            case OPT_FLI_TC_MODE:
                        TCmodeStr = optarg;
            if( ! strcmp(TCmodeStr, "absolute")) {
              TCmode->setValueInteger( ABSOLUTE_TC) ;
            } else if(! strcmp(TCmodeStr, "relative")) {
              TCmode->setValueInteger( RELATIVE_TC)  ;
            } else {
              TCmode->setValueInteger( NO_TC)  ;
            }
                        break;

        default:
            return Focusd::processOption (in_opt);
    }
    return 0;
}
int Fli::willConnect (rts2core::NetworkAddress * in_addr)
{
  if (meteoDevice && in_addr->getType () == DEVICE_TYPE_SENSOR && in_addr->isAddress (meteoDevice)) {
      logStream (MESSAGE_DEBUG) << "FLI::willConnect to DEVICE_TYPE_SENSOR: "<< meteoDevice << ", variable: "<< meteoVariable<< sendLog;
      return 1;
    }
    return Focusd::willConnect (in_addr);
}

int Fli::initHardware ()
{
    LIBFLIAPI ret;
    char **names;
    char *nam_sep;

    if (fliDebug)
        FLISetDebugLevel (NULL, FLIDEBUG_ALL);

    if (dev > 0)
    {
        FLIClose (dev);
        dev = -1;
    }

    if (name == NULL)
    {
        ret = FLIList (deviceDomain, &names);
        if (ret)
            return -1;

        if (names[0] == NULL)
        {
            logStream (MESSAGE_ERROR) << "Fli::init No device found!" << sendLog;
            return -1;
        }

        nam_sep = strchr (names[0], ';');
        if (nam_sep)
            *nam_sep = '\0';

        ret = FLIOpen (&dev, names[0], deviceDomain);
        FLIFreeList (names);
    }
    else
    {
        ret = FLIOpen (&dev, (char*)name, deviceDomain);
    }
    if (ret)
    {
        logStream (MESSAGE_ERROR) << "cannot open device " << (name == NULL ? names[0] : name) << ":" << strerror (errno) << sendLog;
        return -1;
    }

    if (ret)
        return -1;

    long extent;
    ret = FLIGetFocuserExtent (dev, &extent);
    if (ret)
        return -1;
    setFocusExtent (0, extent);
#define OFFSET_MIN -3000.
    focusingOffset->setMin( OFFSET_MIN) ;
    tempOffset->setMin(OFFSET_MIN) ;

    if (!std::isnan (defaultPosition->getValueFloat ()))
    {
        float def = defaultPosition->getValueFloat ();
        if (def < 0)
        {
            ret = info ();
            if (ret)
                return -1;
            def = getPosition ();
        }
        // calibrate by moving to home position, then move to default position
        ret = FLIHomeFocuser (dev);
        if (ret)
        {
            logStream (MESSAGE_ERROR) << "Cannot home focuser, return value: " << ret << sendLog;
            return -1;
        }
        setPosition (defaultPosition->getValueInteger ());
    }

    return 0;
}

int Fli::initValues ()
{
    LIBFLIAPI ret;

    char ft[200];

    ret = FLIGetModel (dev, ft, 200);
    if (ret)
    {
        logStream (MESSAGE_ERROR) << "Cannot get focuser model, error: " << ret << sendLog;
        return -1;
    }
    focType = std::string (ft);

    if( TCmode->getValueInteger () != NO_TC) {
      meteo() ;
    }
    return Focusd::initValues ();
}
int Fli::idle ()
{
    if( TCmode->getValueInteger () != NO_TC) {
      meteo() ;
    }
    return Focusd::idle ();
}
int Fli::info ()
{
    LIBFLIAPI ret;

    long steps;
    double tempFLI;

    ret = FLIGetStepperPosition (dev, &steps);
    if (ret)
    {
        ret = initHardware ();
        if (ret)
            return -1;
        ret = FLIGetStepperPosition (dev, &steps);
        if (ret)
            return -1;
    }

    position->setValueInteger ((int) steps);
    ret= FLIReadTemperature(dev, FLI_TEMPERATURE_INTERNAL, &tempFLI) ;
    if (ret)
      return -1;

    temperatureFLI->setValueDouble(tempFLI);
    if( TCmode->getValueInteger () != NO_TC) {
      meteo() ;
    }


    return Focusd::info ();
}
void Fli::meteo()
{
    rts2core::Connection * connMeteo = getOpenConnection (meteoDevice);
    if( connMeteo != NULL) {
      rts2core::Value *dome_tmp = connMeteo->getValue (meteoVariable);
      if( dome_tmp != NULL){
        //logStream (MESSAGE_ERROR) << "DOME_TMP " << dome_tmp->getValueDouble () << sendLog;
        temperatureMeteo->setValueDouble(dome_tmp->getValueDouble ());
      }
    }  else {
      logStream (MESSAGE_ERROR) << "FLI::meteo failed on device: "<< meteoDevice<< sendLog;
    }
}

int Fli::setTo (double num)
{
    LIBFLIAPI ret;
    ret = info ();
    if (ret)
        return ret;

    ret= isFocusing ();
    if( ret != -2){
      logStream (MESSAGE_ERROR) << "Fli::setTo ignore, still moving the focuser" << sendLog;
      return -1;
    }
    long s = (long)num - (long)position->getValueDouble ();
    fprintf(stderr,"     setto  FLI diff s=%ld, num=%ld, position=%ld\n", s, (long)num, (long)position->getValueDouble ());

    //ToDo was ret = FLIStepMotorAsync (dev, s);
    // the async version can not travel more than 4095 steps
    // this one can:
    ret = FLIStepMotor (dev, s);
    if (ret)
        return -1;

    ret = FLIGetStepsRemaining (dev, &s);

    // wait while move starts..
    double timeout = getNow () + 2;
    do
    {
        ret = FLIGetStepsRemaining (dev, &s);
        if (ret)
            return -1;
        if (s != 0)
            return 0;

        ret = FLIGetStepperPosition (dev, &s);
        if (ret)
            return -1;

        if (((s+1) > num) && ((s-1) < num))
            return 0;

    } while (getNow () < timeout);

    logStream (MESSAGE_ERROR) << "timeout during moving focuser to " << num << ", actual position is " << s << sendLog;

    return -1;
}
double Fli::tcOffset ()
{
        double tcFocOffset= 0.;
    double tmpPosition= 0.;

    if( TCmode->getValueInteger () == NO_TC) {

      logStream (MESSAGE_DEBUG) << "Fli::tcOffset: no tcFocOffset calculated (mode=NO_TC) "<< sendLog;
      return 0. ;
    } else {
      // get the temperature from meteo device
      meteo();

      if( !( std::isnan(temperatureMeteo->getValueDouble()) ||
         std::isnan(TCtemperatureRef->getValueDouble()) ||
         std::isnan(TCoffset->getValueDouble()) ||
         std::isnan(TCslope->getValueDouble()) ||
         TCslope->getValueDouble()==0.))
      {
        if( TCmode->getValueInteger ()== ABSOLUTE_TC) {
          // absolute FOC_POS(T):
          tcFocOffset=  temperatureMeteo->getValueDouble() * TCslope->getValueDouble() +  TCoffset->getValueDouble() ;
          // although it is an absolute position an offset is required in the caller method
          tmpPosition= tcFocOffset - defaultPosition->getValueFloat();

          logStream (MESSAGE_DEBUG) << "Fli::tcOffset: tcFocOffset: absolute: " << tcFocOffset <<",  tmpPosition: " << tmpPosition << ", defaultPosition: "<< defaultPosition->getValueFloat () << ",  focusingOffset: " << focusingOffset->getValueFloat () << ", tempOffset: "<< tempOffset->getValueFloat () <<  sendLog;

        } else {
          // relative FOC_POS(T):
          tcFocOffset=  (temperatureMeteo->getValueDouble()- TCtemperatureRef->getValueDouble()) * TCslope->getValueDouble() ;
          tmpPosition= tcFocOffset;

          logStream (MESSAGE_DEBUG) << "Fli::tcOffset: tcFocOffset: relative: "<< tmpPosition << ", defaultPosition: "<< defaultPosition->getValueFloat () << ",  focusingOffset: " << focusingOffset->getValueFloat () << ", tempOffset:"<< tempOffset->getValueFloat () <<  sendLog;
        }

        TCFocOffset->setValueFloat(tmpPosition) ;
        sendValueAll (TCFocOffset);
        return tmpPosition ;
      } else {
        //focuserPosition= num;
        logStream (MESSAGE_DEBUG) << "Fli::tcOffset: tcFocOffset: one of the values was NaN or 0., setting focuser to: " << 0. <<sendLog;
        return 0. ;
      }
    }

      logStream (MESSAGE_DEBUG) << "Fli::tcOffset " << sendLog;
      return 0. ;
}

void Fli::valueChanged (rts2core::Value *changed_value)
{
  long steps= 0  ;
  long positionTc= 0 ;
  int ret ;

  if ((changed_value == TCmode) || (changed_value ==TCtemperatureRef)) {

    positionTc= + defaultPosition->getValueFloat () + focusingOffset->getValueFloat () + tempOffset->getValueFloat ()+ tcOffset() ;
    ret= setTo( positionTc) ;
    if( ret){
      logStream (MESSAGE_ERROR) << "Fli::valueChanged: error: "<< ret << ", error positionTc: "<< positionTc << sendLog;
    } else {
      // ToDo: ret does not show up in the log files
      if( ret == 0) {
    logStream (MESSAGE_ERROR) << "Fli::valueChanged: ok: "<< 0 << " (zero), positionTc: "<< positionTc<< sendLog;
      } else {
    logStream (MESSAGE_ERROR) << "Fli::valueChanged: ok: "<< ret << ", positionTc: "<< positionTc<< sendLog;
      }
      position->setValueInteger ((int) steps);
    }
    target->setValueDouble (positionTc) ;
    sendValueAll (target);
  }

  Focusd::valueChanged (changed_value);
}

int Fli::isFocusing ()
{
    LIBFLIAPI ret;
    long rem;

    ret = FLIGetStepsRemaining (dev, &rem);
    if (ret)
    {
        logStream (MESSAGE_ERROR) << "Cannot get FLI steps remaining, return value:" << ret << sendLog;
        return -1;
    }
    if (rem == 0)
        return -2;
    return USEC_SEC / 100;
}

bool Fli::isAtStartPosition ()
{
    int ret;
    ret = info ();
    if (ret)
        return false;
    return getPosition () == defaultPosition->getValueFloat ();
}

int Fli::commandAuthorized (rts2core::Connection * conn)
{
    if (conn->isCommand ("home"))
    {
        LIBFLIAPI ret;
        int foc= isFocusing ();
        if( foc != -2){
          logStream (MESSAGE_ERROR) << "Fli::commandAuthorized ignore, still moving the focuser" << sendLog;
          return -1;
        }
        ret = FLIHomeFocuser (dev);
        if (ret)
        {
            conn->sendCommandEnd (DEVDEM_E_HW, "cannot home focuser");
            return -1;
        }
        return 0;
    }
    return Focusd::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
    Fli device = Fli (argc, argv);
    return device.run ();
}

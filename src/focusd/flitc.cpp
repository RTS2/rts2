/**
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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
/*!
 * @file Driver for Finger Lake Instruments CCD, temperature compensation
 * 
 * @author  Petr Kubanek, Markus Wildi
 */


#include "focusd.h"
#include <limits>
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
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Fli:public Focusd
{
	private:
		flidev_t dev;
		flidomain_t deviceDomain;

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

	protected:
		virtual int isFocusing ();
		virtual bool isAtStartPosition ();

		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual void meteo ();
		virtual int setTo (float num);
		rts2core::ValueLong *focExtent;
		virtual void valueChanged (rts2core::Value *changed_value);
	public:
		Fli (int argc, char **argv);
		virtual ~ Fli (void);

		virtual int commandAuthorized (Rts2Conn * conn);
                virtual int willConnect (Rts2Address * in_addr);

};

};

using namespace rts2focusd;

Fli::Fli (int argc, char **argv):Focusd (argc, argv)
{
	deviceDomain = FLIDEVICE_FOCUSER | FLIDOMAIN_USB;
	fliDebug = FLIDEBUG_NONE;

	createValue (TCFocOffset, "FOC_TC", "absolute position or offset calculated by temperature compensation", false);
	createValue (focExtent, "FOC_EXTENT", "focuser extent value in steps", false);
	createValue (temperatureMeteo, "TEMP_METEO", "temperature from the meteo device", true); //go to FITS
	createValue (TCtemperatureRef, "TC_TEMP_REF", "temperature at the time when FOC_DEF was set", false, RTS2_VALUE_WRITABLE);
	createValue (TCoffset, "TCP0", "y-axis offset of the linear temperature model", false);
	createValue (TCslope, "TCP1", "slope of the linear temperature model", false);
	createValue (TCmode, "TCMODE", "temperature compensation absolute, relative to FOC_DEF, no tc", false, RTS2_VALUE_WRITABLE);

	addOption ('D', "domain", 1,
		"CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
	addOption ('b', "fli_debug", 1,
		"FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");

	addOption (OPT_FLI_METEO_DEVICE, "meteoDevice",  1, "meteo device name to monitor its temperature");
	addOption (OPT_FLI_METEO_TEMPERATURE, "meteoVariable",  1, "meteo device temperature variable name");
	addOption (OPT_FLI_TC_TEMP_REF, "TCtempRef",  1, "temperature compensation reference temperature (when FOC_DEF was set)");
	addOption (OPT_FLI_TC_OFFSET, "TCoffset",  1, "temperature compensation y-axis offset");
	addOption (OPT_FLI_TC_SLOPE, "TCslope",  1, "temperature compensation slope");
	addOption (OPT_FLI_TC_MODE, "TCmode",  1, "temperature compensation absolute, relative, default: none");


	TCtemperatureRef->setValueDouble( 10.) ; 
	TCoffset->setValueDouble( 0.) ;  
	TCslope->setValueDouble(  1.) ; 
	
	TCmode->addSelVal("absolute") ;
	TCmode->addSelVal("relative") ;
	TCmode->addSelVal("none") ;
	TCmode->setValueInteger( NO_TC)  ;

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
int 
Fli::willConnect (Rts2Address * in_addr)
{
    if (meteoDevice && in_addr->getType () == DEVICE_TYPE_SENSOR) {
      logStream (MESSAGE_DEBUG) << "FLI::willConnect to DEVICE_TYPE_SENSOR: "<< meteoDevice << ", variable: "<< meteoVariable<< sendLog;
      return 1;
    }
    return Focusd::willConnect (in_addr);
}
int Fli::init ()
{
	LIBFLIAPI ret;
	int ret_f;
	char **names;
	char *nam_sep;

	ret_f = Focusd::init ();
	if (ret_f)
		return ret_f;

	if (fliDebug)
		FLISetDebugLevel (NULL, FLIDEBUG_ALL);

	ret = FLIList (deviceDomain, &names);
	if (ret)
		return -1;

	if (names[0] == NULL)
	{
		logStream (MESSAGE_ERROR) << "Fli::init No device found!"
			<< sendLog;
		return -1;
	}

	nam_sep = strchr (names[0], ';');
	if (nam_sep)
		*nam_sep = '\0';

	ret = FLIOpen (&dev, names[0], deviceDomain);
	FLIFreeList (names);
	if (ret)
		return -1;

	long extent;
	ret = FLIGetFocuserExtent (dev, &extent);
	if (ret)
		return -1;
	focExtent->setValueInteger (extent);

	// calibrate by moving to home position, then move to default position
	ret = FLIHomeFocuser (dev);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot home focuser, return value: " << ret << sendLog;
		return -1;
	}
	setPosition (defaultPosition->getValueInteger ());
	// connect to the meteo device to retrieve the temperature
	addConstValue (meteoDevice, meteoDevice, "meteo device name to monitor its temperature");
	addConstValue (meteoVariable, meteoVariable, "meteo device temperature variable name");
	meteo();

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
	return Focusd::initValues ();
}

int Fli::info ()
{
	LIBFLIAPI ret;

	long steps;

	ret = FLIGetStepperPosition (dev, &steps);
	if (ret)
		return -1;

	position->setValueInteger ((int) steps);
	meteo();
	return Focusd::info ();
}
void Fli::meteo()
{
	Rts2Conn * connMeteo = getOpenConnection (meteoDevice);
	if( connMeteo) {
	  rts2core::Value *meteoDeviceTemperature =  connMeteo->getValue ( meteoVariable);
	  if( meteoDeviceTemperature) {
	    if( meteoDeviceTemperature->getValueType()== RTS2_VALUE_DOUBLE) {

	      //logStream (MESSAGE_DEBUG) << "Fli::meteo: temperature is: " <<  meteoDeviceTemperature->getValue() << sendLog;
	      temperatureMeteo->setValueDouble( meteoDeviceTemperature->getValueDouble());
	    }  
	  } else {
	      logStream (MESSAGE_ERROR) << "FLI::meteo meteoDeviceTemperature=NULL : "<< sendLog;
	  }
	}  else {
	  logStream (MESSAGE_ERROR) << "FLI::meteo failed on device: "<< meteoDevice<< sendLog;
	}
}
int Fli::setTo (float num)
{
        float tcFocOffset= 0.;
        float focuserPosition= -1. ; // better fail with "Desired..."
	LIBFLIAPI ret;
	ret = info ();
	if (ret)
		return ret;

	if( TCmode->getValueInteger () == NO_TC) {
	  focuserPosition= num;
	  logStream (MESSAGE_DEBUG) << "Fli::setTo: no tcFocOffset: "<< focuserPosition  << " was: " << num << sendLog;
	} else {

	  meteo();

	  if( !( isnan(temperatureMeteo->getValueDouble()) || 
		 isnan(TCtemperatureRef->getValueDouble()) ||
		 isnan(TCoffset->getValueDouble()) ||
		 isnan(TCslope->getValueDouble()) ||
		 TCslope->getValueDouble()==0.))
	  {

	    if( TCmode->getValueInteger ()== ABSOLUTE_TC) {

	      tcFocOffset=  (temperatureMeteo->getValueDouble()-  TCoffset->getValueDouble())/TCslope->getValueDouble() ; 
	      focuserPosition= tcFocOffset;
	    } else {

	      tcFocOffset=  (temperatureMeteo->getValueDouble()- TCtemperatureRef->getValueDouble())/TCslope->getValueDouble() ; 
	      focuserPosition= num + tcFocOffset;
	    }

	    logStream (MESSAGE_DEBUG) << "Fli::setTo: tcFocOffset: "<< focuserPosition  << " instead of: " << num << sendLog;
	  } else {
	  
	    focuserPosition= num;
	    logStream (MESSAGE_DEBUG) << "Fli::setTo: tcFocOffset: one of the values was NaN or 0., setting focuser to: " << focuserPosition <<sendLog;
	  }
	}

	if (focuserPosition < 0 || focuserPosition > focExtent->getValueInteger())
	{
		logStream (MESSAGE_ERROR) << "Desired position not in focuser's extent: " << focuserPosition << sendLog;
		return -1;
	}

	long focuserPositionOffset = focuserPosition - position->getValueInteger ();

	ret = FLIStepMotorAsync (dev, focuserPositionOffset);
	if (ret)
	{
	        logStream (MESSAGE_ERROR) << "Fli::setTo: error: "<< ret <<", setting position: " << focuserPosition << sendLog;
		return -1;
	} else{
	  
	  target->setValueFloat(focuserPosition) ;
	  sendValueAll (target);
	  TCFocOffset->setValueFloat(tcFocOffset) ;
	  sendValueAll (TCFocOffset);
	}
	return 0;
}
void Fli::valueChanged (rts2core::Value *changed_value)
{
        bool set=false ;
        long currentOffset=0 ; // in case TC_TEMP_REF is set, it will be calculated in setTo
        long steps;
        int ret ;

        if (changed_value == TCmode) {
	  set=true ;

	  switch (TCmode->getValueInteger())
	  {

		case ABSOLUTE_TC:
			break;
		case RELATIVE_TC:
			break;
		case NO_TC:
		default:
		{
		  currentOffset= (long) TCFocOffset->getValueFloat() ;
		  TCFocOffset->setValueFloat(0.) ;
		  sendValueAll (TCFocOffset);
		}
	  }

	}  else if(changed_value ==TCtemperatureRef) {
	  set=true ;
	}

	if( set) {

	  ret = FLIGetStepperPosition (dev, &steps);
	  if (ret) {

	    logStream (MESSAGE_ERROR) << "Fli::valueChanged: error: "<< ret << ", retrieving position: " << sendLog;
	  } else {
	    ret= setTo( steps- currentOffset) ;
	    if( ! ret){
	      position->setValueInteger ((int) steps);
	    }
	  }
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
	return getPosition () == 0;
}

int Fli::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("home"))
	{
		LIBFLIAPI ret;
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

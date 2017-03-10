/* 
 * sensor daemon for cloudsensor AAG designed by António Alberto Peres Gomes 
 * Copyright (C) 2009, Markus Wildi, Petr Kubanek <petr@kubanek.net>
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

#include <string.h>
#include "sensord.h"
#include "tps534-oak.h"
#define  OPT_TPS534_DEVICE      OPT_LOCAL + 133
#define  OPT_TPS534_SKY_TRIGGER OPT_LOCAL + 134
#define  OPT_TPS534_DOOR_DEVICE OPT_LOCAL + 135


tps534_state tps534State ;
tps534_state tps534LastState ;
extern int read_oak_cnt ;

namespace rts2sensord
{
/*
 * Class for cloudsensor.
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
class TPS534: public SensorWeather
{
	private:
		char default_device_file[64] ;
		char *device_file ;
		rts2core::ValueDouble *temperatureSky;
		rts2core::ValueDouble *ambientTemperatureBeta;
		rts2core::ValueDouble *ambientTemperatureLUT;
		rts2core::ValueDouble *rTheta;
		rts2core::ValueDouble *triggerSky;
                char *doorDevice;

     	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();

	public:
		TPS534 (int in_argc, char **in_argv);
		virtual ~TPS534 (void);
                virtual int willConnect (rts2core::NetworkAddress * in_addr);
};

};

using namespace rts2sensord;
int
TPS534::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_TPS534_DEVICE:
			device_file = optarg;
			break;
		case OPT_TPS534_SKY_TRIGGER:
			triggerSky->setValueCharArr (optarg);
			break;
	        case OPT_TPS534_DOOR_DEVICE:
                        doorDevice = optarg;
                        break;

		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}
int 
TPS534::willConnect (rts2core::NetworkAddress * in_addr)
{
    if (doorDevice && in_addr->getType () == DEVICE_TYPE_DOME) {
      logStream (MESSAGE_INFO) << "TPS534::willConnect to DEVICE_TYPE_DOME: "<< doorDevice << sendLog;
      return 1;
    }
    return Sensor::willConnect (in_addr);
}
int
TPS534::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;
	// make sure it forks before creating threads
	ret = doDaemonize ();

	if (ret) {
	  logStream (MESSAGE_ERROR) << "Doorvermes::initValues could not daemonize"<< sendLog ;
	  return ret;
	}

	addConstValue ("DOOR", doorDevice, "door device name to monitor its state (open/closed)");
	connectDevice(device_file, 1);

	if (!isnan (triggerSky->getValueDouble ()))
		setWeatherState (false, "cloud trigger unspecified");

	
	for( int i = 0; i < 6; i++) {
	  tps534LastState.analogIn[i]= 0. ;
	}
	return 0;
}
int
TPS534::info ()
{
  static int last_read_oak_cnt= 0;
  rTheta->setValueDouble (tps534State.calcIn[0]) ;
  ambientTemperatureBeta->setValueDouble (tps534State.calcIn[1]) ;
  ambientTemperatureLUT->setValueDouble (tps534State.calcIn[2]) ;
  temperatureSky->setValueDouble (tps534State.calcIn[3]) ;

  //  Measurment make only sense in case the door is open
  //  otherwise the temperature of the door is read
  bool doorOpen= false ;
  rts2core::Rts2Connection * conn_door = getOpenConnection (doorDevice);
  if( conn_door) {
    rts2core::Value * doorState =  conn_door->getValue ("DOORSTATE");
    if( doorState) {
      if( doorState->getValueType()== RTS2_VALUE_STRING) {
	if( ! strcmp(doorState->getDisplayValue(), "stopped, open")) {
	  doorOpen= true ;
        } else {
	  doorOpen= false ;
      	}
      }  
    } else {
      logStream (MESSAGE_DEBUG) << "TPS534::info doorState=NULL : "<< sendLog;
    }
  }  

  if( doorOpen) {
    // Check if thread died  
    if( last_read_oak_cnt >= read_oak_cnt) {
      logStream (MESSAGE_ERROR) << "TPS534::info oak thread died" << sendLog;
    } else {
      // set the weather state
      if (temperatureSky->getValueDouble() > triggerSky->getValueDouble ()) {
	if (getWeatherState () == true)  {
	  logStream (MESSAGE_DEBUG) << "TPS534::info setting weather to bad, sky temperature: " << temperatureSky->getValueDouble ()
				    << " trigger: " << triggerSky->getValueDouble ()
				    << sendLog;
	}
	setWeatherTimeout (TPS534_WEATHER_TIMEOUT_BAD, "sky temperature");
      }
    }
    last_read_oak_cnt= read_oak_cnt ;
  }
  return SensorWeather::info ();
}
TPS534::TPS534 (int argc, char **argv):SensorWeather (argc, argv)
{
        strcpy( default_device_file, "/dev/usb/hiddev5");
	device_file= default_device_file ;
	addOption (OPT_TPS534_DEVICE, "device", 1, "HID dev TPS534 cloud sensor");
	addOption (OPT_TPS534_SKY_TRIGGER, "cloud", 1, "cloud trigger point [deg C]");
	addOption (OPT_TPS534_DOOR_DEVICE, "door_device",  1, "door device name to monitor its state (open/closed)");

	createValue (temperatureSky,         "TEMP_SKY",     "temperature sky", true); // go to FITS
	createValue (ambientTemperatureBeta, "AMB_TEMP_BETA","ambient temperature", false);
	createValue (ambientTemperatureLUT,  "AMB_TEMP_LUT", "ambient temperature", false);
	createValue (rTheta,                 "R_THETA",      "Theta", false);
	createValue (triggerSky,             "SKY_TRIGGER",  "if sky temperature gets below this value, weather is not bad [deg C]", false, RTS2_VALUE_WRITABLE);
	triggerSky->setValueDouble (TPS534_THRESHOLD_CLOUDY);

	setIdleInfoInterval (TPS534_POLLING_TIME); // best choice for TPS534
	doorDevice = NULL;
}
TPS534::~TPS534 (void)
{

}
int
main (int argc, char **argv)
{
	TPS534 device = TPS534 (argc, argv);
	return device.run ();
}

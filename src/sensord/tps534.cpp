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

#include "sensord.h"
#include "aag.h"
#define  OPT_TPS534_DEVICE      OPT_LOCAL + 133
#define  OPT_TPS534_SKY_TRIGGER OPT_LOCAL  + 134
#define TPS534_POLLING_TIME    1.      /* seconds */
#define TPS534_WEATHER_TIMEOUT 60.      /* seconds */
#define TPS534_WEATHER_TIMEOUT_BAD 300. /* seconds */

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
		char *device_file;
		Rts2ValueDouble *tempSky;
		Rts2ValueDouble *tempIRSensor;
		Rts2ValueDouble *triggerSky;
		Rts2ValueDouble *tempSkyCorrected;
		/*
		 * Read sensor values and caculate.
		 */
		int TPS534GetSkyIRTemperature ();
		int TPS534GetIRSensorTemperature ();
                int TPS534SkyTemperatureCorrection(double sky, double ambient, double *cor) ;
     	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();
		virtual int idle ();
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		TPS534 (int in_argc, char **in_argv);
		virtual ~TPS534 (void);
};

};

using namespace rts2sensord;
int
TPS534::TPS534SkyTemperatureCorrection( double sky, double ambient, double *cor)
{
    double td ;

    td= cor[0]/100. * (ambient - cor[1]/10.) + pow(( cor[2]/100.) * exp(cor[3]/1000. * ambient), (cor[4]/100.)) ;

    tempSkyCorrected->setValueDouble( sky - td) ;
    return 0;
}
int
TPS534::TPS534GetSkyIRTemperature ()
{
	int ret;
	int value ;
 
	tempSky->setValueDouble ((double) value/100.);

	return 0;
}
int
TPS534::TPS534GetIRSensorTemperature ()
{
	int ret;
	char buf[51];
	int value ;
        int x ;
 
	tempIRSensor->setValueDouble ((double) value/100.);

	return 0;
}
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
		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}
int
TPS534::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	if (!isnan (triggerSky->getValueDouble ()))
		setWeatherState (false, "cloud trigger unspecified");

	return 0;
}
int
TPS534::info ()
{
// These values should be made available for setting if required

    double cor[]= {COR_0, COR_1, COR_2, COR_3, COR_4} ;
    double polling_time= TPS534_POLLING_TIME;
    int ret;

    ret = TPS534GetSkyIRTemperature ();
    if (ret)
    {
	if (getLastInfoTime () > TPS534_WEATHER_TIMEOUT)
	    setWeatherTimeout (TPS534_WEATHER_TIMEOUT, "cannot read from device");
	return -1 ;
    }
    ret = TPS534GetIRSensorTemperature ();
    if (ret)
    {
	if (getLastInfoTime () > TPS534_WEATHER_TIMEOUT)
	    setWeatherTimeout (TPS534_WEATHER_TIMEOUT, "cannot read from device");
	return -1 ;
    }
    // check the state of the rain sensor
    if ((tempSky->getValueDouble() > triggerSky->getValueDouble ()) && (tempSkyCorrected->getValueDouble() > triggerSky->getValueDouble ()))
    {
	if (getWeatherState () == true) 
	{
	    logStream (MESSAGE_DEBUG) << "setting weather to bad, sky temperature: " << tempSky->getValueDouble ()
				      << " trigger: " << triggerSky->getValueDouble ()
				      << sendLog;
	}
	setWeatherTimeout (TPS534_WEATHER_TIMEOUT_BAD, "sky temperature");
    }
    return SensorWeather::info ();
}
int
TPS534::idle ()
{
	return SensorWeather::idle ();
}
int
TPS534::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	return SensorWeather::setValue (old_value, new_value);
}
TPS534::TPS534 (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (tempSky,          "TEMP_SKY",     "temperature sky", true); // go to FITS
	createValue (tempSkyCorrected, "TEMP_SKY_CORR","temperature sky corrected", false);
	createValue (tempIRSensor,     "TEMP_IRS",     "temperature ir sensor", false);
	createValue (triggerSky,       "SKY_TRIGGER",  "if sky temperature gets below this value, weather is not bad [deg C]", false, RTS2_VALUE_WRITABLE);
	triggerSky->setValueDouble (THRESHOLD_CLOUDY);

	addOption (OPT_TPS534_DEVICE, "device", 1, "serial port TPS534 cloud sensor");
	addOption (OPT_TPS534_SKY_TRIGGER, "cloud", 1, "cloud trigger point [deg C]");

	setIdleInfoInterval (TPS534_POLLING_TIME); // best choice for TPS534
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

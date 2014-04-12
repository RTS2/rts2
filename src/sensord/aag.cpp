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

#include <sys/time.h>
#include <time.h>

#include "sensord.h"
#include "connection/serial.h"
#include "aag.h"


#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>


namespace rts2sensord
{
/*
 * Class for cloudsensor.
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
class AAG: public SensorWeather
{
	public:
		AAG (int in_argc, char **in_argv);
		virtual ~AAG (void);

     	protected:
		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int info ();
		virtual int idle ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
		char *device_file;
		rts2core::ConnSerial *aagConn;
                rts2core::ValueDoubleStat *tempSky;
                rts2core::ValueDoubleStat *tempIRSensor;
		rts2core::ValueDouble *tempSkyCorrected;
                rts2core::ValueDouble *intVoltage ; 
                rts2core::ValueDouble *ldrResistance ;
                rts2core::ValueDouble *tempRain ;
                rts2core::ValueDouble *rainFrequency ;
                rts2core::ValueDouble* pwmValue ;
		rts2core::ValueDouble *triggerDry;
		rts2core::ValueDouble *triggerWet;
		rts2core::ValueDouble *triggerClear;
		rts2core::ValueDouble *triggerCloud;
		rts2core::ValueDouble *triggerNoSnow;
		rts2core::ValueInteger *numberOfMeasurements;
		rts2core::ValueDoubleStat *windSpeed;
		rts2core::ValueFloat *windSpeedLimit;
  
		/*
		 * Read sensor values and calculate.
		 */
		int AAGGetSkyIRTemperature ();
		int AAGGetIRSensorTemperature ();
                int AAGGetValues () ;
                int AAGGetRainFrequency() ;
                int AAGGetPWMValue() ;
                int AAGSkyTemperatureCorrection(double sky, double ambient, double *cor) ;
                int AAGSetPWMValue( double value) ;
                int AAGRainState( double rain, double ambient, double rain_frequency, double polling_time) ;
                double AAGduty_on_frequency( double rain_frequency, int heat_state, double polling_time) ;
                int AAGGetWindSpeed ();
  
                time_t real_rain_timeout ;
};

};

using namespace rts2sensord;

AAG::AAG (int argc, char **argv):SensorWeather (argc, argv)
{
	aagConn = NULL;

	windSpeed = NULL;
	windSpeedLimit = NULL;

	createValue (tempSky,          "TEMP_SKY",     "temperature sky", true); // go to FITS
	createValue (tempSkyCorrected, "TEMP_SKY_CORR","temperature sky corrected", true);
	createValue (tempIRSensor,     "TEMP_IRS",     "temperature ir sensor", true);
	createValue (tempRain,         "TEMP_RAIN",    "rain sensor temperature", false);
	createValue (rainFrequency,    "RAIN",         "rain frequency", true);
	createValue (pwmValue,         "PWM",          "pwm value", false);
	createValue (ldrResistance,    "LDR_RES",      "pullup resistancetrue", false);
	createValue (intVoltage,       "INT_VOLT",     "internal voltage", false);
	createValue (triggerDry,       "DRY_TRIGGER",  "if rain frequency gets above this value, rain stopped [a.u.]", false, RTS2_VALUE_WRITABLE);
	createValue (triggerWet,       "WET_TRIGGER",  "if rain frequency gets below this value, rain started [a.u.]", false, RTS2_VALUE_WRITABLE);
	createValue (triggerClear,     "CLEAR_TRIGGER","if sky temperature gets below this value, weather is good [deg C]", false, RTS2_VALUE_WRITABLE);
	createValue (triggerCloud,     "CLOUD_TRIGGER","if sky temperature gets above this value, weather is bad [deg C]", false, RTS2_VALUE_WRITABLE);
	createValue (triggerNoSnow,    "NO_SNOW",       "difference (TEMP_SKY- TEMP_IRS), if larger, assume that there is no snow on sensor [abs deg C]", false, RTS2_VALUE_WRITABLE);

	createValue (numberOfMeasurements, "numOfMeasurements", "number of measurements for weather statistic", false, RTS2_VALUE_WRITABLE);
	numberOfMeasurements->setValueInteger (20);

	triggerDry->setValueDouble (THRESHOLD_DRY);
	triggerWet->setValueDouble (THRESHOLD_WET);
	triggerClear->setValueDouble (THRESHOLD_CLEAR);
	triggerCloud->setValueDouble (THRESHOLD_CLOUD);
	triggerNoSnow->setValueDouble (THRESHOLD_NO_SNOW);

	addOption (OPT_AAG_DEVICE, "device", 1, "serial port AAG cloud sensor");
	addOption (OPT_AAG_DRY_TRIGGER, "dry", 1, "wet trigger point [a.u.]");
	addOption (OPT_AAG_WET_TRIGGER, "wet", 1, "dry trigger point [a.u.]");
	addOption (OPT_AAG_CLEAR_TRIGGER, "clear", 1, "clear sky trigger point [deg C]");
	addOption (OPT_AAG_CLOUD_TRIGGER, "cloud", 1, "cloudy sky trigger point [deg C]");
	addOption (OPT_AAG_NO_SNOW_TRIGGER, "no_snow", 1, "no snow on sensor trigger point [deg C]");

	setIdleInfoInterval (AAG_POLLING_TIME); // best choice for AAG
}

AAG::~AAG (void)
{
	delete aagConn;
}

int AAG::AAGSkyTemperatureCorrection( double sky, double ambient, double *cor)
{
	double td ;

	td = cor[0]/100. * (ambient - cor[1]/10.) + pow(( cor[2]/100.) * exp(cor[3]/1000. * ambient), (cor[4]/100.)) ;

	tempSkyCorrected->setValueDouble( sky - td) ;
	return 0;
}

double AAG::AAGduty_on_frequency( double rain_frequency, int heat_state, double polling_time)
{
    double duty= 0. ;
/* Heat conservatively,*/
/* the whole box becomes warmer and hence ambient temperature */
/* sensor value and depending on it the target temperature increases*/

    if( heat_state==HEAT_NOT)
    {
	duty= 0. ;
    }
    else if( heat_state==HEAT_MAX)
    {
	duty= HEAT * 1. ;
    }
    else if( heat_state==HEAT_DROP_WHILE_DRY)
    {
	duty= HEAT * HEAT_FACTOR_DROP_WHILE_DRY ;
    }
    else if( heat_state==HEAT_REGULAR)
    {
	if(rain_frequency < THRESHOLD_WET)
	{
	    duty= HEAT * 1. ;
	}
	else
	{
	    if ( THRESHOLD_MAX != THRESHOLD_WET)
	    {
		double p= ( HEAT)/( THRESHOLD_MAX- THRESHOLD_WET) ;
		duty= -p * ( rain_frequency - THRESHOLD_MAX) * 10. / polling_time ;
	    }
	    else
	    {
		logStream (MESSAGE_ERROR) << "AAGduty_on_frequency Thresholds THRESHOLD_DRY, THRESHOLD_MAX must differ " 
					  << THRESHOLD_DRY << ", " << THRESHOLD_MAX << sendLog;
		duty= 0. ;
	    }
	}
    }
    else
    {
	duty= 0. ;
    }
    if( duty > 1.)
    {
        duty= 1. ;
    }
    return duty ;
}

int AAG::AAGRainState( double rain, double ambient, double rain_frequency, double polling_time)
{
	static double last_rain_frequency= 0.;
	int ret;
	double duty = 0.;   // PWM Duty cycle
/* define the heating strategy here */
/* If the sun is shining on the box the behaviour rather different, */
/* don't test under theses circumstances*/
/* rain_state transition IS_DRY to IS_WET is used e.g. to close the dome door */
/* check first if rain_frequency, ambient (sensor temperature) are above thresholds, stop heating */
/* define the rain_state and the amount of heat according to the rain frequency value*/

	if ((rain_frequency > THRESHOLD_MAX) || (ambient > MAX_OPERATING_TEMPERATURE) ||( rain > MAX_RAIN_SENSOR_TEMPERATURE)) /* to be on the safe side */
	{
		duty = AAGduty_on_frequency( rain_frequency, HEAT_NOT, polling_time);
//	logStream (MESSAGE_DEBUG) << "AAGRainState Rain_State is EXTRA DRY due to  rain_frequency > THRESHOLD_MAX, ambient > MAX_OPERATING_TEMPERATURE, rain > MAX_RAIN_SENSOR_TEMPERATURE " 
//				  << sendLog;
	}
	else /* rain_states are IS_DRY, IS_WET, IS_RAIN) */
	{
		if (rain_frequency > THRESHOLD_DRY)
		{
			if ((rain_frequency- last_rain_frequency) < THRESHOLD_DROP_WHILE_DRY)
			{
				duty = AAGduty_on_frequency( rain_frequency, HEAT_DROP_WHILE_DRY, polling_time);
// 		logStream (MESSAGE_DEBUG) << "Rain_State is DRY " 
// 					  << rain_frequency 
// 					  << ">" << THRESHOLD_DRY 
// 					  << " , but rain frequequency dropped " 
// 					  << ( rain_frequency- last_rain_frequency) 
// 					  << ",  duty " << duty  << sendLog;
			}
			else
			{
				duty = AAGduty_on_frequency( rain_frequency, HEAT_REGULAR, polling_time);
// 		logStream (MESSAGE_DEBUG) <<"Rain_State is DRY " 
// 					  << rain_frequency << ">" 
// 					  << THRESHOLD_DRY 
// 					  << ", HEAT_REGULAR " 
// 					  << ( rain_frequency- last_rain_frequency) 
// 					  << ", duty " << duty << sendLog;
			}
		}
		else if (rain_frequency > THRESHOLD_WET)
		{
			if ((rain_frequency- last_rain_frequency) < THRESHOLD_DROP_WHILE_DRY)
			{
				duty = AAGduty_on_frequency( rain_frequency, HEAT_DROP_WHILE_DRY, polling_time);
// 		logStream (MESSAGE_DEBUG) << "Rain_State is WET " 
// 					  << rain_frequency 
// 					  << "<" << THRESHOLD_DRY 
// 					  << " , but rain frequequency dropped " 
// 					  << ( rain_frequency- last_rain_frequency) 
// 					  << ",  duty " << duty  << sendLog;
			}
			else
			{
// 	    logStream (MESSAGE_DEBUG) << "Rain_State is WET, from DRY or RAIN " 
// 					  << rain_frequency << ">" 
// 					  << THRESHOLD_WET << sendLog;
			}
			duty =  AAGduty_on_frequency( rain_frequency, HEAT_REGULAR, polling_time);
		}
		else /* everything else is rain */
		{
// 		logStream (MESSAGE_DEBUG) << "Rain_State is RAIN, from DRY or WET " 
// 					  << rain_frequency << "<" 
// 					  << THRESHOLD_WET << sendLog;
			duty = AAGduty_on_frequency( rain_frequency, HEAT_REGULAR, polling_time);
		}
// 	logStream (MESSAGE_DEBUG) << "Normal Duty " 
// 				  << duty  << ", rain " 
// 				  << rain << ", time differneces w" << sendLog;  
	}
	last_rain_frequency = rain_frequency;
	if ((ret= AAGSetPWMValue( duty * 100.))== 0)
	{
// ToDo: can used to display the stat        return rain_state ;
		return 0 ;
	}
	else
	{
		logStream (MESSAGE_ERROR) << "Failed setting duty " << duty *100. << sendLog; 
		return -1 ;
	}
	return 0 ;
}

int AAG::AAGGetSkyIRTemperature ()
{
	int ret;
	char buf[2 * BLOCK_LENGTH + 1];
	int value ;
        int x ;
 
	ret = aagConn->writeRead ("S!", 2, buf, 2 * BLOCK_LENGTH);
	if (ret < 0)
		return ret;

///* Format to read !1        -2399! */

	if ((x = sscanf( buf, "!1 %d!", &value)) != 1) 
	{
		buf[ret]= '\0' ;
		logStream (MESSAGE_ERROR) << "cannot parse SkyIRTemperature reply from AAG cloud sensor, reply was: '" << buf << "', sscanf " << x << sendLog;
		return -1 ;
	}
	/*tempSky->setValueDouble ((double) value/100.);*/
	tempSky->addValue ((double) value/100., numberOfMeasurements->getValueInteger());
	tempSky->calculate ();

	return 0;
}

int AAG::AAGGetIRSensorTemperature ()
{
	int ret;
	char buf[51];
	int value ;
        int x ;
 
	ret = aagConn->writeRead ("T!", 2, buf, 2 * BLOCK_LENGTH);
	if (ret < 0)
		return ret;
// Format to read !2          -93!

	if(( x= sscanf( buf, "!2 %d!", &value)) != 1) 
	{
		buf[ret]= '\0' ;
		logStream (MESSAGE_ERROR) << "cannot parse IRSensorTemperature reply from AAG cloud sensor, reply was: '" << buf << "', sscanf " << x << sendLog;
		return -1 ;
	}
	/*tempIRSensor->setValueDouble ((double) value/100.);*/
	tempIRSensor->addValue((double) value/100., numberOfMeasurements->getValueInteger());
	tempIRSensor->calculate ();

	return 0;
}

int AAG::AAGGetWindSpeed ()    
{
	int ret;
	char buf[51];
        const char wbuf[]= "V!" ;
	int value ;
        int x ;
 
	ret = aagConn->writeRead (wbuf, 2, buf, 2 * BLOCK_LENGTH);
	if (ret < 0)
		return ret;
// Format to read !w          -93!

	if ((x = sscanf (buf, "!w %d!", &value)) != 1)
	{
		buf[ret]= '\0' ;
		logStream (MESSAGE_ERROR) << "cannot parse Anemometer reply from AAG cloud sensor, reply was: '" << buf << "', sscanf " << x << sendLog;
		return -1;
	}
	windSpeed->addValue((double) (value / 3.6), numberOfMeasurements->getValueInteger());
	return 0;
}

int AAG::AAGGetValues ()
{
	int ret;
	char buf[4 * BLOCK_LENGTH + 1];
        const char wbuf[] = "C!";
	int value[3];
        int x;
        double tmp_resistance, resistance;
	double intVoltage_val;
	double ldrResistance_val;
	double tempRain_val;

	ret = aagConn->writeRead (wbuf, 2, buf, 4 * BLOCK_LENGTH);

	if (ret < 0)
		return ret;
        // Format to read !6          536!4            6!5          757!            0
 	if ((x = sscanf( buf, "!6 %d!4 %d!5 %d!", &value[0], &value[1], &value[2])) != 3) 
	{
	    buf[ret] = '\0';
	    logStream (MESSAGE_ERROR) << "cannot parse GetValues reply from AAG cloud sensor, reply was: '"
				      << buf << "', sscanf " << x << sendLog;
	    return -1 ;
	}
        /* Internal power supply, unit: Volt */
        intVoltage_val = (double) 1023. * ZENER_CONSTANT / (double) value[0];
	intVoltage->setValueDouble (intVoltage_val);

	/* LDR value, unit k Ohm */
	if (value[1] > 1022)
		value[1] = 1022;

	if (value[1] < 1)
		value[1] = 1;

	ldrResistance_val = (double) LDRPULLUP_RESISTANCE / (( 1023./ (double) value[1]) -1.);
	ldrResistance->setValueDouble( ldrResistance_val);

	tmp_resistance = RAINPULLUP_RESISTANCE / (( 1023. / value[2])-1);
	resistance = log( tmp_resistance / RAIN_RES_AT_25); /* log basis e */

	tempRain_val = (double) (1. / ( resistance / RAIN_BETA + 1. / (ABSZERO + 25.)) - ABSZERO);
	tempRain->setValueDouble (tempRain_val);
	return 0;
}

int AAG::AAGGetRainFrequency ()
{
	int ret;
	char buf[4 * BLOCK_LENGTH + 1];
	const char wbuf[]= "E!";
	int value;
	int x;

	ret = aagConn->writeRead (wbuf, 2, buf, 2 * BLOCK_LENGTH);
	if (ret < 0)
		return ret;

	/* Format to read !R          -93! */

	if ((x = sscanf (buf, "!R %d!", &value)) != 1)
	{
		buf[ret] = '\0';
		logStream (MESSAGE_ERROR) << "cannot parse RainFrequency reply from AAG cloud sensor, reply was: '"  << buf << "', sscanf " << x << sendLog;
		return -1;
	}
	rainFrequency->setValueDouble ((double) value );
	return 0;
}

int AAG::AAGGetPWMValue ()
{
	int ret;
	char buf[2 * BLOCK_LENGTH + 1];
	const char wbuf[]= "Q!";
	int value;
	int x;

	ret = aagConn->writeRead (wbuf, 2, buf, 2 * BLOCK_LENGTH);
	if (ret < 0)
	{
		return ret;
	}
	/* Format to read !Q            8! */

	if ((x = sscanf (buf, "!Q %d!", &value)) != 1)
	{
		buf[ret] = '\0';
		logStream (MESSAGE_ERROR) << "cannot parse GetPWMValue reply from AAG cloud sensor, reply was: '" << buf << "', sscanf " << x << sendLog;
		return -1;
	}
	pwmValue->setValueDouble ((double) (100. * value/1023.));
	return 0;
}

int AAG::AAGSetPWMValue (double value)
{
	int ret;
	char buf[2 * BLOCK_LENGTH + 1];
	char wbuf[2 * BLOCK_LENGTH + 1];
	int wvalue;
	int rvalue;
	int x;

	wvalue = (int) (1023. / 100. * value);
	if ((x = sprintf( wbuf, "P%04d!", wvalue)) != 6)
	{
		wbuf[x] = '\0';
		logStream (MESSAGE_ERROR) << "cannot sprintf SetPWMValue AAG cloud sensor, tried to send: '" << wbuf << "', sprintf " << x << sendLog;
		return -1;
	}
	ret = aagConn->writeRead (wbuf, 6, buf, 2 * BLOCK_LENGTH); 
	if (ret < 0)
	{
		wbuf[x] = '\0';
		buf[ret] = '\0';
		logStream (MESSAGE_ERROR) << "cannot sprintf or sscanf SetPWMValue AAG cloud sensor, tried to send: '" << wbuf << "', received " << buf << sendLog;
		return ret;
	}
	/* Format to read !Q            8! */

	if ((x = sscanf( buf, "!Q %d!", &rvalue)) != 1)
	{
		buf[ret] = '\0';
		logStream (MESSAGE_ERROR) << "cannot parse GetPWMValue reply from AAG cloud sensor, reply was: '" << buf << "', sscanf " << x << sendLog;
		return -1;
	}
	pwmValue->setValueDouble ((double) (100. * rvalue/1023.));
	return 0;
}

int AAG::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_AAG_DEVICE:
			device_file = optarg;
			break;
		case OPT_AAG_DRY_TRIGGER:
			triggerDry->setValueCharArr (optarg);
			break;
		case OPT_AAG_WET_TRIGGER:
			triggerWet->setValueCharArr (optarg);
			break;
		case OPT_AAG_CLEAR_TRIGGER:
			triggerClear->setValueCharArr (optarg);
			break;
		case OPT_AAG_CLOUD_TRIGGER:
			triggerCloud->setValueCharArr (optarg);
			break;
		case OPT_AAG_NO_SNOW_TRIGGER:
			triggerNoSnow->setValueCharArr (optarg);
			break;
		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}

int AAG::initHardware ()
{
	int ret;
	
	real_rain_timeout= 1 ;
	aagConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 30);
	ret = aagConn->init ();
	if (ret)
		return ret;
	
	aagConn->flushPortIO ();
        aagConn->setDebug(getDebug ());

	// query if windspeed sensor is present
	char buf[2 * BLOCK_LENGTH + 1];
	ret = aagConn->writeRead ("v!", 2, buf, 2 * BLOCK_LENGTH);
	if (ret > 0)
	{
		int vsup = 0;
		ret = sscanf (buf, "!v %d!", &vsup);
		if (ret == 1 && vsup == 1)
		{
			createValue (windSpeed, "WIND_SPEED", "[m/s] wind speed", true);
			createValue (windSpeedLimit, "WIND_SPEED_LIMIT", "[m/s] limit on wind speed", false, RTS2_VALUE_WRITABLE);
			windSpeedLimit->setValueDouble (16);
		}
	}

	if (!isnan (triggerDry->getValueDouble ()))
		setWeatherState (false, "rain trigger unspecified");
	if (!isnan (triggerWet->getValueDouble ()))
		setWeatherState (false, "rain trigger unspecified");
	if (!isnan (triggerClear->getValueDouble ()))
		setWeatherState (false, "cloud trigger unspecified");
	if (!isnan (triggerCloud->getValueDouble ()))
		setWeatherState (false, "cloud trigger unspecified");
	if (!isnan (triggerNoSnow->getValueDouble ()))
		setWeatherState (false, "no snow trigger unspecified");
	return 0;
}

int AAG::info ()
{
// These values should be made available for setting if required
	double cor[]= {COR_0, COR_1, COR_2, COR_3, COR_4} ;
	double polling_time= AAG_POLLING_TIME;
	int ret;

	ret = AAGGetSkyIRTemperature ();
	if (ret)
	{
		if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
		return -1;
	}

	ret = AAGGetIRSensorTemperature ();
	if (ret)
	{
		if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
		return -1;
	}

	ret = AAGGetValues ();
	if (ret)
	{
		if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
		return -1;
	}

	ret = AAGGetRainFrequency();
	if (ret)
	{
		if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
		return -1;
	}

	ret = AAGSkyTemperatureCorrection(tempSky->getValueDouble(), tempIRSensor->getValueDouble(), cor);
	if (ret)
 	{
		if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
		return -1;
	}

	ret = AAGRainState( tempRain->getValueDouble(), tempIRSensor->getValueDouble(), rainFrequency->getValueDouble(), polling_time) ;
	if (ret)
	{
		if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
		return -1;
	}

	if (windSpeed != NULL)
	{
 		ret = AAGGetWindSpeed ();
		if (ret)
		{
			if (getLastInfoTime () > AAG_WEATHER_TIMEOUT)
			{
				valueError (windSpeed);
				setWeatherTimeout (AAG_WEATHER_TIMEOUT, "cannot read from device");
			}
			else
			{
				valueWarning (windSpeed);
			}
			return -1;
		}
		if (windSpeed->getValueDouble () >= windSpeedLimit->getValueDouble ())
		{
			setWeatherTimeout (600, "wind speed is above save limit");
			valueError (windSpeed);
		}
		else
		{
			valueGood (windSpeed);
		}
	}
    
	static int count_bad_weather;
	if (rainFrequency->getValueDouble () < triggerWet->getValueDouble ()) {
		count_bad_weather++;
	} else {
		count_bad_weather = 0;
	}

	bool set_to_bad = true ;
#define BAD_WEATHER_COUNT 8 
	if( count_bad_weather > BAD_WEATHER_COUNT) {
		set_to_bad = true ;
	} else {
		set_to_bad = false ;
	}

	// check the state of the rain sensor
	if (abs(tempSky->getValueDouble() - tempIRSensor->getValueDouble()) < triggerNoSnow->getValueDouble())
	{
		if (getWeatherState () == true) // true: good weather, if now bad, notify
		{
			logStream (MESSAGE_DEBUG) << "setting weather to bad, no snow : " << abs(tempSky->getValueDouble()- tempIRSensor->getValueDouble()) << " trigger: " << triggerNoSnow->getValueDouble () << sendLog;
		}
		valueError (tempSky);
		setWeatherTimeout (AAG_WEATHER_TIMEOUT_BAD, "snow on sensor"); // set to bad now
	}
	else if (rainFrequency->getValueDouble () < triggerWet->getValueDouble ()) 
	{
		if (getWeatherState () == true) 
		{
			if (set_to_bad)
			{
				logStream (MESSAGE_DEBUG) << "setting weather to bad, rainFrequency: " << rainFrequency->getValueDouble () << " trigger: " << triggerWet->getValueDouble () << sendLog;
			} else {
				logStream (MESSAGE_DEBUG) << "NOT yet setting weather to bad, rainFrequency: " << rainFrequency->getValueDouble () << " trigger: " << triggerWet->getValueDouble () << " counter " << count_bad_weather << sendLog;
			}
		}

		if (set_to_bad)
		{
			setWeatherTimeout (AAG_WEATHER_TIMEOUT_BAD, "raining");
			valueError(rainFrequency);
		}
		else
		{
			valueGood (rainFrequency); 
		}
	}
	// check the state of the cloud sensor
	else if ((tempSky->getValueDouble() > triggerCloud->getValueDouble ()) && (tempSkyCorrected->getValueDouble() > triggerCloud->getValueDouble ()))
	{
		if (getWeatherState () == true) 
		{
			logStream (MESSAGE_DEBUG) << "setting weather to bad, sky temperature: " << tempSky->getValueDouble () << " trigger: " << triggerCloud->getValueDouble () << sendLog;
		}
		setWeatherTimeout (AAG_WEATHER_TIMEOUT_BAD, "sky temperature");
		valueError (tempSkyCorrected);
	}
	else
	{
		valueGood (tempSky);
		valueGood (tempSkyCorrected);
		valueGood (rainFrequency);
	}
	return SensorWeather::info ();
}

int AAG::idle ()
{
	return SensorWeather::idle ();
}

int AAG::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	return SensorWeather::setValue (old_value, new_value);
}

int main (int argc, char **argv)
{
	AAG device (argc, argv);
	return device.run ();
}

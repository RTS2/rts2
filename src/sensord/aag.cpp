/* 
 * Sensor daemon for cloudsensor AAG designed by António Alberto Peres Gomes 
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
#include "../utils/rts2connserial.h"
#include "aag.h"

namespace rts2sensord
{
/*
 * Class for cloudsensor.
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
class AAG: public SensorWeather
{
	private:
		char *device_file;
		Rts2ConnSerial *aagConn;
		Rts2ValueDouble *tempSky;
		Rts2ValueDouble *tempIRSensor;
		Rts2ValueDouble *tempSkyCorrected;
                Rts2ValueDouble *intVoltage ; 
                Rts2ValueDouble *ldrResistance ;
                Rts2ValueDouble *tempRain ;
                Rts2ValueDouble *tempRainTarget ;
                Rts2ValueDouble *rainFrequency ;
                Rts2ValueDouble* pwmValue ;
		Rts2ValueInteger *numVal;
		Rts2ValueDouble *triggerBad;
		Rts2ValueDouble *triggerGood;
		/*
		 * Read sensor values and caculate.
		 */
		int AAGGetSkyIRTemperature ();
		int AAGGetIRSensorTemperature ();
                int AAGGetValues () ;
                int AAGGetRainFrequency() ;
                int AAGGetPWMValue() ;
                int AAGSkyTemperatureCorrection(double sky, double ambient, double *cor) ;
                int AAGSetPWMValue( double value) ;
                int AAGTargetTemperature( double ambient, double *threshold, double *delta, double impulse_delta_temperature, double rain_frequency) ;
                int AAGRainState( double rain, double ambient, double *threshold, double *delta, double *impulse, double rain_frequency, double polling_time) ;
                float duty_on_time( double ambient, double *threshold, double deltalow, double rain, double target, int heat_state, double polling_time, double min) ;
     	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();
		virtual int idle ();
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		AAG (int in_argc, char **in_argv);
		virtual ~AAG (void);
};

};

using namespace rts2sensord;
int
AAG::AAGSkyTemperatureCorrection( double sky, double ambient, double *cor)
{
    double td ;

    td= cor[0]/100. * (ambient - cor[1]/10.) + pow(( cor[2]/100.) * exp(cor[3]/1000. * ambient), (cor[4]/100.)) ;

    tempSkyCorrected->setValueDouble( sky - td) ;
    return 0;
}
int
AAG::AAGTargetTemperature( double ambient, double *threshold, double *delta, double impulse_delta_temperature, double rain_frequency)
{
    double target ;
    double p ; // slope of the straight line
    double q ; // offset
    int thr_state= IS_UNDEFINED ;

/* define current state */
    if( rain_frequency > THRESHOLD_MAX)
    {
        thr_state= IS_MAX ;
    }
    else if( rain_frequency > THRESHOLD_DRY)
    {
        thr_state= IS_DRY ;
    }
    else if( rain_frequency > THRESHOLD_WET)
    {
        thr_state= IS_WET ;
    }
    else
    {
        thr_state= IS_RAIN ;
    }

    if( ambient < threshold[0])
    {
        target= delta[0] ;
    }
    else
    {
        if( thr_state== IS_MAX)
        {
	    target= ambient ;
        }
        else if( thr_state== IS_DRY)
        {
            if( ambient > threshold[1])
            {
                if ( THRESHOLD_DRY != THRESHOLD_MAX)
                {
		    p= ( TEMPERATURE_DIFFFERENCE_AT_THRESHOLD_DRY)/( THRESHOLD_MAX- THRESHOLD_DRY) ;
		    target= -p * ( rain_frequency - THRESHOLD_DRY) + ambient + TEMPERATURE_DIFFFERENCE_AT_THRESHOLD_DRY ; // minus sign ok
                }
                else
                {

		    logStream (MESSAGE_ERROR) << "AAGTargetTemperature Thresholds THRESHOLD_DRY, THRESHOLD_MAX must differ " << THRESHOLD_DRY << ", " << THRESHOLD_MAX << sendLog;
                    return -1 ;
                }
            }
            else
            {
                if (threshold[1] != threshold[0])
                {
                    p= (delta[1]- delta[0])/(threshold[1]- threshold[0]) ;
                    q=  delta[0] - p * threshold[0] ;

                    target= ambient + p * ambient + q ;
                }
                else
                {
		    logStream (MESSAGE_ERROR) << "AAGTargetTemperature Threshold must differ " << threshold[0] << ", " << threshold[1] << sendLog;
                    return -1 ;
                }
            }
        }
        else if( thr_state== IS_WET)
        {
            target= ambient +  impulse_delta_temperature;
        }
        else if( thr_state== IS_RAIN)
        {
            target= ambient + impulse_delta_temperature ;
        }
    }
    tempRainTarget->setValueDouble( target) ;
    return 0 ;
}
float 
AAG::duty_on_time( double ambient, double *threshold, double deltalow, double rain, double target, int heat_state, double polling_time, double min)
{
    double duty= 0. ;
    double heat= 0. ;   // reference heat quantity from the manual e.g. 40

/* Heat conservatively,*/
/* the whole box becomes warmer and hence ambient temperature */
/* sensor value and depending on it the target temperature increases*/
    if( heat_state==HEAT_MAX)
    {
	duty= HEAT * 1. ;
    }
    else if( heat_state==HEAT_NOT)
    {
	duty= 0. ;
    }
    else if( heat_state==HEAT_MIN) // 2009-07-30, currently not used
    {
        duty= min ;  /* min=[0,1] */
    }
    else
    {
        if(( ambient < threshold[0]) && (rain < deltalow))
        {
            heat= HEAT * 0.6 ; /* probably ice or snow */
        }
        else if((rain -target) < -8.)
        {
            heat=  HEAT * 0.4 ;
        }
        else if((rain -target) < -4.)
        {
            heat=  HEAT * 0.3 ;
        }
        else if((rain -target) < -3.)
        {
            heat=  HEAT * 0.2 ;
        }
        else if((rain -target) < -2.)
        {
            heat=  HEAT * 0.1 ;
        }
        else if((rain -target) < -1.)
        {
            heat=  HEAT * 0.09 ;
        }
        else if((rain -target) < -0.5)
        {
            heat=  HEAT * 0.07 ;
        }
        else if((rain -target) < -0.3)
        {
            heat=  HEAT * 0.05 ;
        }
        else
        {
            heat=  HEAT * 0. ;
        }
        duty= heat * 10./ polling_time ; // 10. : from the manual
    }
    if( duty > 1.)
    {
        duty= 1. ;
    }
    return duty ;
}
int 
AAG::AAGRainState( double rain, double ambient, double *threshold, double *delta, double *impulse, double rain_frequency, double polling_time)
{
    static double last_rain_frequency= 0. ;
    static int rain_state= IS_UNDEFINED ;
    static time_t start_wet= 0. ;
    static time_t start_rain= 0. ;
    time_t current_time ;
    int ret ;
    double duty=0. ;   // PWM Duty cycle
    double target=0. ;

    current_time= time( NULL);
    
/* The target temperature is defined according to the current rain_state, see  int AAGTargetTemperature */
    AAGTargetTemperature( ambient, threshold, delta, impulse[IMP_DELTA], rain_frequency) ;
    target= tempRainTarget->getValueDouble() ;
 
/* define the heating strategy here */
/* If the sun is shining on the box the behaviour rather different, */
/* don't test under theses circumstances*/
/* rain_state IS_MAX is the normal target */
/* rain_state transition IS_DRY to IS_WET is used e.g. to close the dome door */
/* check first if rain_frequency, ambient (sensor temperature) are above thresholds, stop heating */
/* then check if a heating cycle is in progress */
/* else: define the rain_state and the amount of heat according to the rain frequency values*/
/* ToDo Currently  unused are IMP_CYCLE_WET, IMP_DURATION_RAIN */

    if(( rain_frequency > THRESHOLD_MAX) || (ambient > MAX_OPERATING_TEMPERATURE) ||( rain > MAX_RAIN_SENSOR_TEMPERATURE)) /* to be on the safe side */
    {
        rain_state= IS_MAX ;
        start_wet= 0. ;
        start_rain= 0. ;
        duty=  duty_on_time( ambient, threshold, delta[0], rain, target, HEAT_NOT, polling_time, 0.);
#ifdef LIB_DEBUG_STATE

	logStream (MESSAGE_DEBUG) << "AAGRainState Rain_State is EXTRA DRY due to  rain_frequency > THRESHOLD_MAX, ambient > MAX_OPERATING_TEMPERATURE, rain > MAX_RAIN_SENSOR_TEMPERATURE " << sendLog;
#endif
    }
    else if( rain > (target + impulse[IMP_DELTA])) /* ToDo: impulse[IMP_DELTA]) to be revisited, target is always positive*/
    {
	/* here the new rain_state could be anything beside IS_MAX */
	if( rain_frequency > THRESHOLD_DRY)
	{
	    rain_state= IS_DRY ;
	}
	else if( rain_frequency > THRESHOLD_WET)
	{
	    rain_state= IS_WET ;
	}
	else
	{
	    rain_state= IS_RAIN ;
	}
	start_wet= 0. ;
	start_rain= 0. ;
	duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_NOT, polling_time, impulse[IMP_MIN]);
#ifdef LIB_DEBUG_STATE
        logStream (MESSAGE_DEBUG) << "Rain_State is set to DRY due to rain " << rain << " > target " << target << " +" << impulse[IMP_DELTA] << ", but not necessarily " << rain_frequency << "<" <<  THRESHOLD_DRY << ", duty " << duty << sendLog;
#endif
    }
    else if(((current_time- start_wet- impulse[IMP_DURATION_WET]) < 0.) && ( start_wet > 0.))
    {
/* Heat for the period  impulse[IMP_DURATION_WET] regardless of possible rain_state transition (to IS_DRY or IS_RAIN)*/
	duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_MAX, polling_time, impulse[IMP_MIN]);
#ifdef LIB_DEBUG_STATE

	logStream (MESSAGE_DEBUG) << "Rain_State is WET, from WET, impulse duration " << impulse[IMP_DURATION_WET] << ", " rain_frequency << ">" << THRESHOLD_WET << "," (current_time- start_wet- impulse[IMP_DURATION_WET]) << ", " << (current_time- start_rain- impulse[IMP_CYCLE_RAIN]) << sendLog;
#endif
    }
    else if(((current_time- start_rain- impulse[IMP_CYCLE_RAIN]) < 0.) && ( start_rain > 0.))
    {
/* Heat for the period impulse[IMP_CYCLE_RAIN], if rain_state transition to IS_WET occurs, go to regular heating */
        if(( rain_frequency- last_rain_frequency) > THRESHOLD_DRY_OUT)
        {
/* if the heater was successfull this could be the end of the rain, try to dry out */
#ifdef LIB_DEBUG_STATE

	    logStream (MESSAGE_DEBUG) << "Rain_State is RAIN, from RAIN, impulse cycle duration  +" << impulse[IMP_CYCLE_RAIN] << ", " << rain_frequency << "<" << THRESHOLD_WET << ",  time differneces w " << (current_time- start_wet- impulse[IMP_DURATION_WET]) << ", " << (current_time- start_rain- impulse[IMP_CYCLE_RAIN]) << sendLog;
#endif
            duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_MAX, polling_time, impulse[IMP_MIN]);
        }
        else if( rain_frequency< THRESHOLD_WET)
        {
            /* do not waste energy, the heater can not dry the sensor while it is raining */

            duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_NOT, polling_time, impulse[IMP_MIN]);
#ifdef LIB_DEBUG_STATE
	    logStream (MESSAGE_DEBUG) << "Rain NOT Duty  " << duty << ", delta " << rain- target << " target " << target << " rain " << rain << "time differneces w " << " r " << (current_time- start_rain- impulse[IMP_CYCLE_RAIN])  << sendLog;
#endif
        }
        else
        {
#ifdef LIB_DEBUG_STATE
	    logStream (MESSAGE_DEBUG) << "Rain_State is at least WET, from RAIN " << rain_frequency << ">" << THRESHOLD_WET << sendLog;
#endif
            rain_state= IS_WET ;
            start_rain= 0. ;
            start_wet= current_time ;
            duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_REGULAR, polling_time, impulse[IMP_MIN]);
        }
    }
    else /* rain_states are IS_DRY, IS_WET, IS_RAIN) */
    {
        if( rain_frequency > THRESHOLD_DRY)
        {
            rain_state= IS_DRY ;
            start_wet= 0. ;
            start_rain= 0. ;
            if(( rain_frequency- last_rain_frequency) < THRESHOLD_DROP_WHILE_DRY)
            {
                duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_MAX, polling_time, impulse[IMP_MIN]);
#ifdef LIB_DEBUG_STATE

		logStream (MESSAGE_DEBUG) << "Rain_State is DRY " << rain_frequency << ">" << THRESHOLD_DRY << " , but rain frequequency dropped " << ( rain_frequency- last_rain_frequency) << ",  duty " << duty  << sendLog;
#endif
	    }
	    else
	    {
		duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_REGULAR, polling_time, 0.);
#ifdef LIB_DEBUG_STATE

		logStream (MESSAGE_DEBUG) <<"Rain_State is DRY " << rain_frequency << ">" << THRESHOLD_DRY << ", HEAT_REGULAR " << ( rain_frequency- last_rain_frequency) << ", duty " << duty << sendLog;
#endif
	    }
        }
        else if( rain_frequency > THRESHOLD_WET)
        {
#ifdef LIB_DEBUG_STATE
            if(( rain_state== IS_DRY) || (( rain_state== IS_RAIN)))
            {
		logStream (MESSAGE_DEBUG) << "Rain_State is WET, from DRY or RAIN " << rain_frequency << ">" << THRESHOLD_WET << sendLog;
            }
            else if ((current_time- start_wet- 2. * impulse[IMP_DURATION_WET])> 0.)
            {
		logStream (MESSAGE_DEBUG) << "Rain_State is WET, from WET, restarting timers +" << impulse[IMP_DURATION_WET] << ", " << rain_frequency << ">" << THRESHOLD_WET << sendLog;
            }
#endif
            rain_state= IS_WET ;
            start_rain= 0. ;
            start_wet= current_time ;
            duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_REGULAR, polling_time, impulse[IMP_MIN]);
        }
        else /* everything else is rain */
        {
#ifdef LIB_DEBUG_STATE
            if(( rain_state== IS_DRY) || (rain_state== IS_WET))
            {
		logStream (MESSAGE_DEBUG) << "Rain_State is RAIN, from DRY or WET " << rain_frequency << "<" << THRESHOLD_WET << sendLog;
            }
            else
            {
		logStream (MESSAGE_DEBUG) << "Rain_State is RAIN starting timers +" << impulse[IMP_DURATION_RAIN] << ", rain " << rain_frequency << "<" <<  THRESHOLD_WET << sendLog;
            }
#endif
            rain_state= IS_RAIN ;
            start_wet= 0. ;
            start_rain= current_time ;
            duty=  duty_on_time( ambient, threshold, delta[0], rain, target,  HEAT_REGULAR, polling_time, impulse[IMP_MIN]);
        }
#ifdef LIB_DEBUG_STATE
	logStream (MESSAGE_DEBUG) << "Normal Duty " << duty << ", delta " << rain- target << ", target " << target << ", rain " << rain << ", time differneces w" << (current_time- start_wet- impulse[IMP_DURATION_WET]) << ", r" << (current_time- start_rain- impulse[IMP_DURATION_RAIN])<< sendLog;  
#endif
    }
    last_rain_frequency= rain_frequency ;
    if(( ret= AAGSetPWMValue( duty * 100.))== 0)
    {
#ifdef LIB_DEBUG_STATE
	logStream (MESSAGE_ERROR) << "Setting duty " << duty *100. << ", imp_min " << impulse[IMP_MIN] << sendLog; 
#endif
// ToDo: can used to display the stat        return rain_state ;
         return 0 ;
     }
     else
     {
         return -1 ;
     }
	return 0 ;
}
int
AAG::AAGGetSkyIRTemperature ()
{
	int ret;
	char buf[51];
        const char wbuf[]= "S!" ;
	int value ;
        int x ;
 
	ret= aagConn->writePort(wbuf, 2) ;
	ret= aagConn->readPort(buf, 2 * BLOCK_LENGTH) ;
	if (ret < 0)
		return ret;

///* Format to read !1        -2399! */

	if(( x= sscanf( buf, "!1        %d!", &value)) != 1) 
	{
	    buf[ret]= '\0' ;
	    logStream (MESSAGE_ERROR) << "cannot parse SkyIRTemperature reply from AAG cloud sensor, reply was: '"
				      << buf << "', sscanf " << x << sendLog;
	    return -1 ;
	}
	tempSky->setValueDouble ((double) value/100.);

	return 0;
}
int
AAG::AAGGetIRSensorTemperature ()
{
	int ret;
	char buf[51];
        const char wbuf[]= "T!" ;
	int value ;
        int x ;
 
	ret = aagConn->writeRead (wbuf, 2, buf, 2 * BLOCK_LENGTH);
	if (ret < 0)
		return ret;
// Format to read !2          -93!

	if(( x= sscanf( buf, "!2        %d!", &value)) != 1) 
	{
	    buf[ret]= '\0' ;
	    logStream (MESSAGE_ERROR) << "cannot parse IRSensorTemperature reply from AAG cloud sensor, reply was: '"
				      << buf << "', sscanf " << x << sendLog;
	    return -1 ;
	}
	tempIRSensor->setValueDouble ((double) value/100.);

	return 0;
}
int
AAG::AAGGetValues ()
{
	int ret;
	char buf[4 * BLOCK_LENGTH + 1];
        const char wbuf[]= "C!" ;
	int value[3] ;
        int x ;
        double tmp_resistance, resistance ;
	double intVoltage_val ;
	double ldrResistance_val ;
	double tempRain_val ;

	ret = aagConn->writeRead (wbuf, 2, buf, 4 * BLOCK_LENGTH);

	if (ret < 0)
		return ret;
        // Format to read !6          536!4            6!5          757!            0
 	if(( x= sscanf( buf, "!6          %d!4            %d!5          %d!", &value[0], &value[1], &value[2])) != 3) 
	{
	    buf[ret] = '\0';
	    logStream (MESSAGE_ERROR) << "cannot parse GetValues reply from AAG cloud sensor, reply was: '"
				      << buf << "', sscanf " << x << sendLog;
	    return -1 ;
	}
        /* Internal power supply, unit: Volt */
        intVoltage_val= (double) 1023. * ZENER_CONSTANT / (double) value[0] ;
	intVoltage->setValueDouble (intVoltage_val);

       /* LDR value, unit k Ohm */
      if( value[1] > 1022)
          value[1]= 1022 ;

      if( value[1] < 1)
          value[1]= 1 ;

      ldrResistance_val= (double) LDRPULLUP_RESISTANCE / (( 1023./ (double) value[1]) -1.) ;
      ldrResistance->setValueDouble( ldrResistance_val) ;

      tmp_resistance=  RAINPULLUP_RESISTANCE / (( 1023. / value[2])-1) ;
      resistance= log( tmp_resistance / RAIN_RES_AT_25) ; /* log basis e */

      tempRain_val= (double) (1. / ( resistance / RAIN_BETA + 1. / (ABSZERO + 25.)) - ABSZERO) ;
      tempRain->setValueDouble (tempRain_val);
      return 0;
}
int 
AAG::AAGGetRainFrequency()
{
    int ret ;
    char buf[4 * BLOCK_LENGTH + 1];
    const char wbuf[]= "E!" ;
    int value ;
    int x ;

    ret = aagConn->writeRead (wbuf, 2, buf, 2 * BLOCK_LENGTH);
    if (ret < 0)
	return ret;

    /* Format to read !R          -93! */

    if(( x= sscanf( buf, "!R        %d!", &value)) != 1)
    {
	buf[ret] = '\0';
        logStream (MESSAGE_ERROR) << "cannot parse RainFrequency reply from AAG cloud sensor, reply was: '"
				  << buf << "', sscanf " << x << sendLog;

        return -1 ;
    }
    rainFrequency->setValueDouble ((double) value );
    return 0 ;
}
int 
AAG::AAGGetPWMValue()
{
    int ret ;
    char buf[2 * BLOCK_LENGTH + 1];
    const char wbuf[]= "Q!" ;
    int value ;
    int x ;

    ret = aagConn->writeRead (wbuf, 2, buf, 2 * BLOCK_LENGTH);
    if (ret < 0)
    {
	return ret;
    }
    /* Format to read !Q            8! */

    if(( x= sscanf( buf, "!Q        %d!", &value)) != 1)
    {
	buf[ret] = '\0';
        logStream (MESSAGE_ERROR) << "cannot parse GetPWMValue reply from AAG cloud sensor, reply was: '"
				  << buf << "', sscanf " << x << sendLog;
        return -1 ;
    }
    pwmValue->setValueDouble ((double) (100. * value/1023.));
    return 0 ;
}
int 
AAG::AAGSetPWMValue( double value)
{
    int ret ;
    char buf[2 * BLOCK_LENGTH + 1];
    char wbuf[2 * BLOCK_LENGTH + 1] ;
    int wvalue ;
    int rvalue ;
    int x ;

    wvalue= (int) (1023. / 100. * value) ;
    if(( x=sprintf( wbuf, "P%04d!", wvalue)) !=6 )
    {
	wbuf[x] = '\0';
        logStream (MESSAGE_ERROR) << "cannot sprintf SetPWMValue AAG cloud sensor, tried to send: '"
				  << wbuf << "', sprintf " << x << sendLog;
        return -1 ;
    }
    ret = aagConn->writeRead (wbuf, 6, buf, 2 * BLOCK_LENGTH); 
    if (ret < 0)
    {
        wbuf[x] = '\0';
	buf[ret] = '\0';
	logStream (MESSAGE_ERROR) << "cannot sprintf or sscanf SetPWMValue AAG cloud sensor, tried to send: '"
                                  << wbuf << "', received " << buf << sendLog;	
	return ret;
    }
    /* Format to read !Q            8! */

    if(( x= sscanf( buf, "!Q        %d!", &rvalue)) != 1)
    {
	buf[ret] = '\0';
        logStream (MESSAGE_ERROR) << "cannot parse GetPWMValue reply from AAG cloud sensor, reply was: '"
				  << buf << "', sscanf " << x << sendLog;
        return -1 ;
    }
    pwmValue->setValueDouble ((double) (100. * rvalue/1023.));

    return 0 ;
}
int
AAG::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 'b':
			triggerBad->setValueCharArr (optarg);
			break;
		case 'g':
			triggerGood->setValueCharArr (optarg);
			break;
		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}
int
AAG::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	aagConn = new Rts2ConnSerial (device_file, this, BS9600, C8, NONE, 30);
	ret = aagConn->init ();
	if (ret)
		return ret;
	
	aagConn->flushPortIO ();
        //aagConn->setDebug() ;
	//aagConn->setLogAsHex(true) ; //ToDo ?it works after a restart of centrald?
	if (!isnan (triggerGood->getValueDouble ()))
		setWeatherState (false);

	return 0;
}
int
AAG::info ()
{
// These values should be made available for setting if required

    double cor[]= {COR_0, COR_1, COR_2, COR_3, COR_4} ;
    double threshold[]={THRESHOLD_0, THRESHOLD_1} ;
    double delta[]={DELTA_0, DELTA_1} ;
    double impulse[]={IMPULSE_0,IMPULSE_1,IMPULSE_2,IMPULSE_3,IMPULSE_4,IMPULSE_5} ;
    double polling_time= POLLING_TIME;
    int ret;

	ret = AAGGetSkyIRTemperature ();
	if (ret)
	{
		if (getLastInfoTime () > WEATHER_TIMEOUT)
			setWeatherTimeout (WEATHER_TIMEOUT);
		return -1 ;
	}
	ret = AAGGetIRSensorTemperature ();
	if (ret)
	{
		if (getLastInfoTime () > WEATHER_TIMEOUT)
			setWeatherTimeout (WEATHER_TIMEOUT);
		return -1 ;
	}
  	ret = AAGGetValues ();
   	if (ret)
   	{
   		if (getLastInfoTime () > WEATHER_TIMEOUT)
   			setWeatherTimeout (WEATHER_TIMEOUT);
		return -1 ;
   	}

	ret = AAGGetRainFrequency();
        if (ret)
        {
	    if (getLastInfoTime () > WEATHER_TIMEOUT)
		setWeatherTimeout (WEATHER_TIMEOUT);
	    return -1 ;
        }

		if (rainFrequency->getValueDouble () <= triggerBad->getValueDouble ())
		{
			if (getWeatherState () == true)
			{
				logStream (MESSAGE_INFO) << "setting weather to bad. rainFrequency: " << rainFrequency->getValueDouble ()
					<< " trigger: " << triggerBad->getValueDouble ()
					<< sendLog;
			}
			setWeatherTimeout (WEATHER_TIMEOUT_BAD);
		}
		else if (rainFrequency->getValueDouble () >= triggerGood->getValueDouble ())
		{
		    if (getWeatherState () == false && rainFrequency->getValueDouble () > triggerGood->getValueDouble ())
			{
				logStream (MESSAGE_INFO) << "setting weather to good. rainFrequency: " << rainFrequency->getValueDouble ()
					<< " trigger: " << triggerGood->getValueDouble ()
					<< sendLog;
			}
		}

	ret = AAGSkyTemperatureCorrection(tempSky->getValueDouble(), tempIRSensor->getValueDouble(), cor);
        if (ret)
        {
	    if (getLastInfoTime () > WEATHER_TIMEOUT)
		setWeatherTimeout (WEATHER_TIMEOUT);

	    return -1 ;
        }
	ret= AAGRainState( tempRain->getValueDouble(), tempIRSensor->getValueDouble(), threshold, delta, impulse, rainFrequency->getValueDouble(), polling_time) ;
	if (ret)
        {
            if (getLastInfoTime () > WEATHER_TIMEOUT)
                setWeatherTimeout (WEATHER_TIMEOUT);

	    return -1 ;
        }
	return SensorWeather::info ();
}
int
AAG::idle ()
{
	return SensorWeather::idle ();
}
int
AAG::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
// 	if (old_value == triggerBad || old_value == triggerGood)
// 	{
// 		return 0;
// 	}
	return SensorWeather::setValue (old_value, new_value);
}
AAG::AAG (int argc, char **argv):SensorWeather (argc, argv)
{
	aagConn = NULL;

	createValue (tempSky, "TEMP_SKY", "temperature sky", false);
	createValue (tempIRSensor, "TEMP_IRS", "temperature ir sensor", false);
	createValue (tempSkyCorrected, "TEMP_SKY_CORR", "temperature sky corrected", false);
	createValue (intVoltage, "INT_VOLT", "internal voltage", false);
	createValue (ldrResistance, "LDR_RES", "pullup resistancetrue", false);
	createValue (tempRain, "TEMP_RAIN", "rain sensor temperature", false);
	createValue (tempRainTarget, "TEMP_RAIN_TARGET", "target temperature rain", false);
	createValue (rainFrequency, "RAIN", "rain frequency", false);
	createValue (pwmValue, "PWM", "pwm value", false);

	createValue (triggerBad, "TRIGBAD", "if rain frequency drops bellow this value, set bad weather", false);
	triggerBad->setValueDouble (nan ("f"));
	createValue (triggerGood, "TRIGGOOD", "if rain frequency gets above this value, drop bad weather flag", false);
	triggerGood->setValueDouble (nan ("f"));

	addOption ('f', NULL, 1, "serial port AAG cloud sensor");
	addOption ('b', NULL, 1, "bad trigger point");
	addOption ('g', NULL, 1, "good trigger point");

	setIdleInfoInterval ( POLLING_TIME); // best choice for AAG
}
AAG::~AAG (void)
{
	delete aagConn;
}
int
main (int argc, char **argv)
{
	AAG device = AAG (argc, argv);
	return device.run ();
}

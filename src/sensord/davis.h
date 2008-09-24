#ifndef __RTS2_DAVIS__
#define __RTS2_DAVIS__

#include "sensord.h"
#include "davisudp.h"

namespace rts2sensor
{

/**
 * Davis Vantage (and other) meteo stations.
 * Needs modified Davis driver, which sends data over UDP.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Davis: public Rts2DevSensor
{
	private:
		DavisUdp *weatherConn;
		double cloud_bad;

		int maxWindSpeed;
		int maxPeekWindspeed;

		Rts2ValueFloat *temperature;
		Rts2ValueFloat *humidity;
		Rts2ValueTime *nextOpen;
		Rts2ValueInteger *rain;
		Rts2ValueFloat *windspeed;
		Rts2ValueDouble *cloud;

		Rts2ValueInteger *udpPort;

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

	public:
		Davis (int argc, char **argv);

		void setTemperature (float in_temp)
		{
			temperature->setValueFloat (in_temp);
		}
		float getTemperature ()
		{
			return temperature->getValueFloat ();
		}
		void setHumidity (float in_humidity)
		{
			humidity->setValueFloat (in_humidity);
		}
		float getHumidity ()
		{
			return humidity->getValueFloat ();
		}
		void setRain (int in_rain)
		{
			rain->setValueInteger (in_rain);
		}
		virtual void setRainWeather (int in_rain)
		{
			setRain (in_rain);
		}
		int getRain ()
		{
			return rain->getValueInteger ();
		}
		void setWindSpeed (float in_windpseed)
		{
			windspeed->setValueFloat (in_windpseed);
		}
		float getWindSpeed ()
		{
			return windspeed->getValueFloat ();
		}
		void setCloud (double in_cloud)
		{
			cloud->setValueDouble (in_cloud);
		}
		double getCloud ()
		{
			return cloud->getValueDouble ();
		}
		void setWeatherTimeout (time_t wait_time);

		int getMaxPeekWindspeed ()
		{
			return maxPeekWindspeed;
		}

		int getMaxWindSpeed ()
		{
			return maxWindSpeed;
		}
};

}

#endif // !__RTS2_DAVIS__

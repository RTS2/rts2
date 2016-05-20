#ifndef __RTS2_CONNBUFWEATHER__
#define __RTS2_CONNBUFWEATHER__

#include "udpweather.h"
#include "dome.h"

class WeatherVal
{
	private:
		const char *name;
	public:
		float value;

		WeatherVal (const char *in_name, float in_value)
		{
			name = in_name;
			value = in_value;
		}

		bool isValue (const char *in_name)
		{
			return !strcmp (name, in_name);
		}
};

class WeatherBuf
{
	private:
		std::vector < WeatherVal > values;
	public:
		WeatherBuf ();
		virtual ~ WeatherBuf (void);

		int parse (char *buf);
		void getValue (const char *name, float &val, int &status);
};

class Rts2ConnBufWeather:public Rts2ConnFramWeather
{
	private:
		Rts2DevDome * master;
		int conn_timeout;
		int bad_weather_timeout;
		int bad_windspeed_timeout;
	public:
		Rts2ConnBufWeather (int in_weather_port, int in_weather_timeout,
			int in_conn_timeout, int in_bad_weather_timeout,
			int in_bad_windspeed_timeout,
			Rts2DevDome * in_master);
		virtual int receive (rts2core::Block *block);
};
#endif							 /* !__RTS2_CONNBUFWEATHER__ */

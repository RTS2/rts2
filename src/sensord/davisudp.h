#ifndef __RTS2_DAVISUDP__
#define __RTS2_DAVISUDP__

#include "../utils/rts2connnosend.h"

namespace rts2sensor
{

class Davis;

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

class DavisUdp:public Rts2ConnNoSend
{
	private:
		Davis *master;
		int weather_port;
		int weather_timeout;
		int conn_timeout;
		int bad_weather_timeout;
		int bad_windspeed_timeout;

		int rain;
		float windspeed;
		time_t lastWeatherStatus;
		time_t lastBadWeather;

	protected:
		void setWeatherTimeout (time_t wait_time);

		virtual void connectionError (int last_data_size)
		{
			// do NOT call Rts2Conn::connectionError. Weather connection must be kept even when error occurs.
			return;
		}

	public:
		DavisUdp (int _weather_port, int _weather_timeout,
			int _conn_timeout, int _bad_weather_timeout,
			int _bad_windspeed_timeout, Davis * _master);

		virtual int init ();

		virtual int receive (fd_set * set);
};

}

#endif // !__RTS2_DAVISUDP__

/* 
 * Dome driver skeleton.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

#define DEF_WEATHER_TIMEOUT 600

class Rts2DevDome:public Rts2Device
{
	private:
		Rts2ValueBool * ignoreMeteo;

	protected:
		const char *domeModel;
		Rts2ValueInteger *sw_state;

		Rts2ValueFloat *temperature;
		Rts2ValueFloat *humidity;
		Rts2ValueTime *nextOpen;
		Rts2ValueInteger *rain;
		Rts2ValueFloat *windspeed;
		Rts2ValueDouble *cloud;

		Rts2ValueInteger *observingPossible;
		int maxWindSpeed;
		int maxPeekWindspeed;
		bool weatherCanOpenDome;

		time_t nextGoodWeather;

		virtual int processOption (int in_opt);

		virtual void cancelPriorityOperations ()
		{
			// we don't want to get back to not-moving state if we were moving..so we don't request to reset our state
		}

		void domeWeatherGood ();
		virtual int isGoodWeather ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Rts2DevDome (int argc, char **argv, int in_device_type = DEVICE_TYPE_DOME);
		virtual int openDome ()
		{
			maskState (DOME_DOME_MASK, DOME_OPENING, "opening dome");
			logStream (MESSAGE_INFO) << "opening dome" << sendLog;
			return 0;
		}
		virtual int endOpen ()
		{
			maskState (DOME_DOME_MASK, DOME_OPENED, "dome opened");
			return 0;
		};
		virtual long isOpened ()
		{
			return -2;
		};
		int closeDomeWeather ();

		virtual int closeDome ()
		{
			maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome");
			logStream (MESSAGE_INFO) << "closing dome" << sendLog;
			return 0;
		};
		virtual int endClose ()
		{
			maskState (DOME_DOME_MASK, DOME_CLOSED, "dome closed");
			return 0;
		};
		virtual long isClosed ()
		{
			return -2;
		};
		int checkOpening ();
		virtual int init ();
		virtual int initValues ();
		virtual int idle ();

		virtual int info ();

		// callback function from dome connection

		virtual int observing ();
		virtual int standby ();
		virtual int off ();

		int setMasterStandby ();
		int setMasterOn ();
		virtual int changeMasterState (int new_state);

		int setIgnoreMeteo (bool newIgnore)
		{
			ignoreMeteo->setValueBool (newIgnore);
			infoAll ();
			return 0;
		}

		bool getIgnoreMeteo ()
		{
			return ignoreMeteo->getValueBool ();
		}

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
		void setSwState (int in_sw_state)
		{
			sw_state->setValueInteger (in_sw_state);
		}

		int getMaxPeekWindspeed ()
		{
			return maxPeekWindspeed;
		}

		int getMaxWindSpeed ()
		{
			return maxWindSpeed;
		}
		time_t getNextOpen ()
		{
			return nextGoodWeather;
		}

		virtual int commandAuthorized (Rts2Conn * conn);
};
#endif							 /* ! __RTS2_DOME__ */

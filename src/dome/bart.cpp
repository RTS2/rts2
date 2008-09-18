/* 
 * Driver for Ford boards.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include <termios.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <vector>

#include "ford.h"
#include "rts2connbufweather.h"

#define CAS_NA_OTEVRENI              30

// we should get packets every minute; 5 min timeout, as data from meteo station
// aren't as precise as we want and we observe dropouts quite often
#define BART_WEATHER_TIMEOUT        360

// how long after weather was bad can weather be good again; in
// seconds
#define BART_BAD_WEATHER_TIMEOUT    360
#define BART_BAD_WINDSPEED_TIMEOUT  360
#define BART_CONN_TIMEOUT           360

typedef enum
{
	ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_4, ZASUVKA_5, ZASUVKA_6, MOTOR,
	SMER, SVETLO, KONCAK_ZAVRENI_JIH, KONCAK_OTEVRENI_JIH,
	KONCAK_OTEVRENI_SEVER, KONCAK_ZAVRENI_SEVER
} vystupy;

#define NUM_ZAS   5

#define OFF   0
#define STANDBY   1
#define OBSERVING 2

// zasuvka c.1 kamera, c.3 kamera, c.5 montaz. c.6 topeni /Ford 21.10.04
int zasuvky_index[NUM_ZAS] =
{ ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_5, ZASUVKA_6 };

enum stavy
{ ZAS_VYP, ZAS_ZAP };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
enum stavy zasuvky_stavy[3][NUM_ZAS] =
{
	// off
	{ZAS_ZAP, ZAS_ZAP, ZAS_VYP, ZAS_ZAP, ZAS_VYP},
	// standby
	{ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP},
	// observnig
	{ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

/**
 * Class for BART dome control.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevDomeBart:public Rts2DomeFord
{
	private:
		int rain_port;
		char *rain_detector;
		FILE *mrak2_log;

		unsigned char spinac[2];

		Rts2ConnBufWeather *weatherConn;

		int handle_zasuvky (int zas);

		char *cloud_dev;
		int cloud_port;

		time_t nextCloudMeas;

		// cloud sensor functions
		int cloudHeating (char perc);
		// adjust cloud heating by air temperature.
		int cloudHeating ();
		int cloudMeasure (char angle);
		int cloudMeasureAll ();

		void checkCloud ();

		bool closeAfterOpen;

	protected:
		virtual int processOption (int in_opt);
		virtual int isGoodWeather ();

	public:
		Rts2DevDomeBart (int argc, char **argv);
		virtual ~ Rts2DevDomeBart (void);

		virtual int init ();

		virtual int idle ();

		virtual int ready ();
		virtual int info ();

		// only use weather rain when we don't have rain detection
		// from sensor mounted at BART
		virtual void setRainWeather (int in_rain)
		{
			if (getRain () == 0)
				setRain (in_rain);
		}

		virtual int openDome ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int closeDome ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual int observing ();
		virtual int standby ();
		virtual int off ();

		virtual int changeMasterState (int new_state);
};

Rts2DevDomeBart::Rts2DevDomeBart (int in_argc, char **in_argv):
Rts2DomeFord (in_argc, in_argv)
{
	addOption ('R', "rain_detector", 1, "/dev/file for rain detector");
	addOption ('c', "cloud_sensor", 1, "/dev/file for cloud sensor");
	rain_detector = NULL;
	rain_port = -1;

	domeModel = "BART_FORD_2";

	weatherConn = NULL;

	cloud_dev = NULL;
	cloud_port = -1;

	nextCloudMeas = 0;

	closeAfterOpen = false;

	// oteviram file pro mrakomer2_log...
	mrak2_log = fopen ("/var/log/mrakomer2", "a");
}


Rts2DevDomeBart::~Rts2DevDomeBart (void)
{
	close (rain_port);
	fclose (mrak2_log);
}


int
Rts2DevDomeBart::openDome ()
{
	if (!isOn (KONCAK_OTEVRENI_JIH))
		return endOpen ();
	if (!isGoodWeather ())
		return -1;
	VYP (MOTOR);
	sleep (1);
	VYP (SMER);
	sleep (1);
	ZAP (MOTOR);
	logStream (MESSAGE_DEBUG) << "oteviram strechu" << sendLog;
	return Rts2DomeFord::openDome ();
}


long
Rts2DevDomeBart::isOpened ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return ret;
	if (isOn (KONCAK_OTEVRENI_JIH))
		return USEC_SEC;
	return -2;
}


int
Rts2DevDomeBart::endOpen ()
{
	VYP (MOTOR);
	zjisti_stav_portu ();		 //kdyz se to vynecha, neposle to posledni prikaz nebo znak
	setTimeout (USEC_SEC);
	if (closeAfterOpen)
	{
		sleep (2);
		return closeDome ();
	}
	return Rts2DomeFord::endOpen ();
}


int
Rts2DevDomeBart::closeDome ()
{
	int motor;
	int smer;
	if (!isOn (KONCAK_ZAVRENI_JIH))
		return endClose ();
	motor = isOn (MOTOR);
	smer = isOn (SMER);
	if (motor == -1 || smer == -1)
	{
		// errror
		return -1;
	}
	if (!motor)
	{
		// closing in progress
		if (!smer)
			return 0;
		// let it go to open state and then close it..
		if (closeAfterOpen == false)
		{
			closeAfterOpen = true;
			logStream (MESSAGE_DEBUG) << "Commanded to close just afer dome finished opening." << sendLog;
		}
		return 0;
	}
	ZAP (SMER);
	sleep (1);
	ZAP (MOTOR);
	logStream (MESSAGE_DEBUG) << "zaviram strechu" << sendLog;

	return Rts2DomeFord::closeDome ();
}


long
Rts2DevDomeBart::isClosed ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return ret;
	if (isOn (KONCAK_ZAVRENI_JIH))
		return USEC_SEC;
	return -2;
}


int
Rts2DevDomeBart::endClose ()
{
	int motor;
	motor = isOn (MOTOR);
	closeAfterOpen = false;
	if (motor == -1)
		return -1;
	if (motor)
		return Rts2DomeFord::endClose ();
	VYP (MOTOR);
	sleep (1);
	VYP (SMER);
	zjisti_stav_portu ();
	return Rts2DomeFord::endClose ();
}


int
Rts2DevDomeBart::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'R':
			rain_detector = optarg;
			break;
		case 'c':
			cloud_dev = optarg;
			break;
		default:
			return Rts2DomeFord::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevDomeBart::isGoodWeather ()
{
	int flags;
	int ret;
	if (rain_port > 0)
	{
		ret = ioctl (rain_port, TIOCMGET, &flags);
		logStream (MESSAGE_DEBUG) <<
			"Rts2DevDomeBart::isGoodWeather flags: " << flags << " rain: " <<
			(flags & TIOCM_RI) << sendLog;
		// ioctl failed or it's raining..
		if (ret || !(flags & TIOCM_RI))
		{
			setRain (1);
			setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT);
			if (getIgnoreMeteo () == true)
				return 1;
			return 0;
		}
		setRain (0);
	}
	if (getIgnoreMeteo () == true)
		return 1;
	if (weatherConn)
		return weatherConn->isGoodWeather ();
	return 0;
}


int
Rts2DevDomeBart::init ()
{
	struct termios newtio;

	int ret = Rts2DomeFord::init ();
	if (ret)
		return ret;

	if (rain_detector)
	{
		// init rain detector
		int flags;
		rain_port = open (rain_detector, O_RDWR | O_NOCTTY);
		if (rain_port == -1)
		{
			logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init cannot open " <<
				rain_detector << " " << strerror (errno) << sendLog;
			return -1;
		}
		ret = ioctl (rain_port, TIOCMGET, &flags);
		if (ret)
		{
			logStream (MESSAGE_ERROR) <<
				"Rts2DevDomeBart::init cannot get flags: " << strerror (errno) <<
				sendLog;
			return -1;
		}
		flags &= ~TIOCM_DTR;
		flags |= TIOCM_RTS;
		ret = ioctl (rain_port, TIOCMSET, &flags);
		if (ret)
		{
			logStream (MESSAGE_ERROR) <<
				"Rts2DevDomeBart::init cannot set flags: " << strerror (errno) <<
				sendLog;
			return -1;
		}
	}

	// get state
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	if (isOn (KONCAK_OTEVRENI_JIH) && !isOn (KONCAK_ZAVRENI_JIH))
		setState (DOME_CLOSED, "dome is closed");
	else if (!isOn (KONCAK_OTEVRENI_JIH) && isOn (KONCAK_ZAVRENI_JIH))
		setState (DOME_OPENED, "dome is opened");

	if (cloud_dev)
	{
		cloud_port = open (cloud_dev, O_RDWR | O_NOCTTY);
		if (cloud_port == -1)
		{
			logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init cannot open " <<
				cloud_dev << " " << strerror (errno) << sendLog;
			return -1;
		}
		// setup values..
		newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
		newtio.c_iflag = IGNPAR;
		newtio.c_oflag = 0;
		newtio.c_lflag = 0;
		newtio.c_cc[VMIN] = 0;
		newtio.c_cc[VTIME] = 1;

		tcflush (cloud_port, TCIOFLUSH);
		ret = tcsetattr (cloud_port, TCSANOW, &newtio);
		if (ret < 0)
		{
			logStream (MESSAGE_ERROR) <<
				"Rts2DevDomeBart::init cloud tcsetattr: " << strerror (errno) <<
				sendLog;
			return -1;
		}
	}

	if (getIgnoreMeteo () == true)
		return 0;

	weatherConn =
		new Rts2ConnBufWeather (1500, BART_WEATHER_TIMEOUT,
		BART_CONN_TIMEOUT,
		BART_BAD_WEATHER_TIMEOUT,
		BART_BAD_WINDSPEED_TIMEOUT, this);
	weatherConn->init ();
	addConnection (weatherConn);

	return 0;
}


int
Rts2DevDomeBart::idle ()
{
	// check for weather..
	checkCloud ();
	if (isGoodWeather ())
	{
		if (((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
			&& ((getState () & DOME_DOME_MASK) == DOME_CLOSED))
		{
			// after centrald reply, that he switched the state, dome will
			// open
			domeWeatherGood ();
		}
	}
	else
	{
		int ret;
		// close dome - don't thrust centrald to be running and closing
		// it for us
		ret = closeDome ();
		if (ret == -1)
		{
			setTimeout (10 * USEC_SEC);
		}
		setMasterStandby ();
	}
	return Rts2DomeFord::idle ();
}


int
Rts2DevDomeBart::handle_zasuvky (int zas)
{
	int i;
	for (i = 0; i < NUM_ZAS; i++)
	{
		int zasuvka_num = zasuvky_index[i];
		if (zasuvky_stavy[zas][i] == ZAS_VYP)
		{
			ZAP (zasuvka_num);
		}
		else
		{
			VYP (zasuvka_num);
		}
		sleep (1);				 // doplnil Ford
	}
	return 0;
}


int
Rts2DevDomeBart::cloudHeating (char perc)
{
	int ret;
	char buf[35];
								 // "flush"
	ret = read (cloud_port, buf, 34);
	ret = write (cloud_port, &perc, 1);
	if (ret != 1)
		return -1;
	sleep (1);
	ret = read (cloud_port, buf, 14);
	if (ret <= 0)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::cloudHeating read: " <<
			strerror (errno) << " ret: " << ret << sendLog;
		return -1;
	}
	buf[ret] = '\0';
	logStream (MESSAGE_DEBUG) << "Rts2DevDomeBart::cloudHeating read: " << buf
		<< sendLog;
	return 0;
}


int
Rts2DevDomeBart::cloudHeating ()
{
	char step = 'b';
	if (getTemperature () > 5)
		return 0;
	step += (char) ((-getTemperature () + 5) / 4.0);
	if (step > 'k')
		step = 'k';
	return cloudHeating (step);
}


int
Rts2DevDomeBart::cloudMeasure (char angle)
{
	int ret;
	char buf[35];
	int ang, ground, space;
								 // "flush"
	ret = read (cloud_port, buf, 34);
	ret = write (cloud_port, &angle, 1);
	if (ret != 1)
		return -1;
	sleep (4);
	ret = read (cloud_port, buf, 20);
	if (ret <= 0)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::cloudMeasure read: " <<
			strerror (errno) << "ret:" << ret << sendLog;
		return -1;
	}
	buf[ret] = '\0';
	// now parse readed values
	// A 1;G 18;S 18
	ret = sscanf (buf, "A %i;G %i; S %i", &ang, &ground, &space);
	if (ret != 3)
	{
		logStream (MESSAGE_ERROR) <<
			"Rts2DevDomeBart::cloudMeasure invalid cloud sensor return: " << buf
			<< sendLog;
		return -1;
	}
	logStream (MESSAGE_DEBUG) <<
		"Rts2DevDomeBart::cloudMeasure angle: " << ang << " ground: " << ground <<
		" space: " << space << sendLog;
	return 0;
}


// posle
int
Rts2DevDomeBart::cloudMeasureAll ()
{
	time_t now;

	int ret;
	char buf[35];
	char buf_m = 'm';
	int ground, s45, s90, s135;
	ret = read (cloud_port, buf, 34);
	ret = write (cloud_port, &buf_m, 1);
	if (ret != 1)
		return -1;
	sleep (4);
	ret = read (cloud_port, buf, 34);
	if (ret <= 0)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::cloudMeasure read: " <<
			strerror (errno) << " ret: " << ret << sendLog;
		return -1;
	}
	buf[ret] = '\0';
	// now parse readed values
	// G 0;S45 -31;S90 -38;S135 -36
	ret =
		sscanf (buf, "G %i;S45 %i;S90 %i;S135 %i", &ground, &s45, &s90, &s135);
	if (ret != 4)
	{
		logStream (MESSAGE_ERROR) <<
			"Rts2DevDomeBart::cloudMeasure invalid cloud sensor return: " << buf
			<< sendLog;
		return -1;
	}
	logStream (MESSAGE_DEBUG) <<
		"Rts2DevDomeBart::cloudMeasure ground: " << ground << " S45: " << s45 <<
		" S90: " << s90 << " S135: " << s135 << sendLog;
	time (&now);
	fprintf (mrak2_log,
		"%li - G %i;S45 %i;S90 %i;S135 %i;Temp %.1f;Hum %.0f;Rain %i\n",
		(long int) now, ground, s45, s90, s135, getTemperature (),
		getHumidity (), getRain ());
	setCloud (s90 - ground);
	fflush (mrak2_log);
	return 0;
}


void
Rts2DevDomeBart::checkCloud ()
{
	time_t now;
	if (cloud_port < 0)
		return;
	time (&now);
	if (now < nextCloudMeas)
		return;

	if (getRain ())
	{
		fprintf (mrak2_log,
			"%li - G nan;S45 nan;S90 nan;S135 nan;Temp %.1f;Hum %.0f;Rain %i\n",
			(long int) now, getTemperature (), getHumidity (), getRain ());
		fflush (mrak2_log);
		nextCloudMeas = now + 300;
		return;
	}

	// check that master is in right state..
	switch (getMasterState ())
	{
		case SERVERD_EVENING:
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			cloudHeating ();
			cloudMeasureAll ();
								 // TODO doresit dopeni kazdych 10 sec
			nextCloudMeas = now + 60;
			break;
		default:
			cloudHeating ();
			cloudMeasureAll ();
			// 5 minutes mesasurements during the day phase
			nextCloudMeas = now + 300;
	}
}


int
Rts2DevDomeBart::ready ()
{
	return 0;
}


int
Rts2DevDomeBart::info ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	sw_state->setValueInteger (!getPortState (KONCAK_OTEVRENI_JIH));
	sw_state->setValueInteger (sw_state->getValueInteger () | (!getPortState (SMER) << 1));
	sw_state->setValueInteger (sw_state->getValueInteger () | (!getPortState (KONCAK_ZAVRENI_JIH) << 2));
	sw_state->setValueInteger (sw_state->getValueInteger () | (!getPortState (MOTOR) << 3));
	return Rts2DomeFord::info ();
}


int
Rts2DevDomeBart::off ()
{
	closeDome ();
	handle_zasuvky (OFF);
	return 0;
}


int
Rts2DevDomeBart::standby ()
{
	handle_zasuvky (STANDBY);
	closeDome ();
	return 0;
}


int
Rts2DevDomeBart::observing ()
{
	handle_zasuvky (OBSERVING);
	openDome ();
	return 0;
}


int
Rts2DevDomeBart::changeMasterState (int new_state)
{
	observingPossible->setValueInteger (0);
	if ((new_state & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	{
		switch (new_state & SERVERD_STATUS_MASK)
		{
			case SERVERD_EVENING:
			case SERVERD_MORNING:
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				standby ();
				break;
			default:
				off ();
		}
	}
	else
	{
		switch (new_state & SERVERD_STATUS_MASK)
		{
			case SERVERD_EVENING:
			case SERVERD_NIGHT:
			case SERVERD_DUSK:
			case SERVERD_DAWN:
				observing ();
				break;
			case SERVERD_MORNING:
				standby ();
				break;
			default:
				off ();
		}
	}
	return Rts2DomeFord::changeMasterState (new_state);
}


int
main (int argc, char **argv)
{
	Rts2DevDomeBart device = Rts2DevDomeBart (argc, argv);
	return device.run ();
}

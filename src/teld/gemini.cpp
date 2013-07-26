/* 
 * Driver for Gemini systems.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

/**
 * @file Driver file for LOSMANDY Gemini telescope systems
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#define DEBUG_EXTRA
#define L4_GUIDE

#include <libnova/libnova.h>

#include "teld.h"
#include "rts2lx200/hms.h"
#include "configuration.h"
#include "utilsfunc.h"

#include "connection/serial.h"

#define RATE_SLEW                'S'
#define RATE_FIND                'M'
#define RATE_CENTER              'C'
#define RATE_GUIDE               'G'
#define PORT_TIMEOUT             5

#define TCM_DEFAULT_RATE         32768

#define SEARCH_STEPS             16

#define MIN(a,b)                 ((a) < (b) ? (a) : (b))
#define MAX(a,b)                 ((a) > (b) ? (a) : (b))

#define GEMINI_CMD_RATE_GUIDE    150
#define GEMINI_CMD_RATE_CENTER   170

#define OPT_BOOTES               OPT_LOCAL + 10
#define OPT_CORR                 OPT_LOCAL + 11
#define OPT_EXPTYPE              OPT_LOCAL + 12
#define OPT_FORCETYPE            OPT_LOCAL + 13
#define OPT_FORCELATLON          OPT_LOCAL + 14


namespace rts2teld
{

class Gemini:public Telescope
{
	public:
		Gemini (int argc, char **argv);
		virtual ~ Gemini (void);
		virtual int init ();
		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);
		virtual int info ();
		virtual int startResync ();
		virtual int isMoving ();
		virtual int endMove ();
		virtual int stopMove ();
		//		virtual int startMoveFixed (double tar_ha, double tar_dec);
		//		virtual int isMovingFixed ();
		//		virtual int endMoveFixed ();
		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();
		int setTo (double set_ra, double set_dec, int appendModel);
		virtual int setTo (double set_ra, double set_dec);
		int correctOffsets (double cor_ra, double cor_dec, double real_ra, double real_dec);
		virtual int saveModel ();
		virtual int loadModel ();
		virtual int stopWorm ();
		virtual int startWorm ();
		virtual int resetMount ();
		virtual int getFlip ();

	protected:
		virtual int processOption (int in_opt);
		virtual int initValues ();
		virtual int idle ();

	private:
		const char *device_file;

		rts2core::ValueTime *telLocalTime;
		rts2core::ValueFloat *telGuidingSpeed;

		rts2core::ValueSelection *resetState;
		rts2core::ValueInteger *featurePort;

		rts2core::ValueDouble *axRa;
		rts2core::ValueDouble *axDec;

		const char *geminiConfig;

		rts2core::ConnSerial *tel_conn;
		int tel_write_read (const char *buf, int wcount, char *rbuf, int rcount);

		/**
		 * Combine write && read_hash together.
		 *
		 * @see tel_write_read for definition
		 */
		int tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount);

		/**
		 * Get date from Gemini. This call nulls all values stored in _tm.
		 *
		 * @param _tm Structure where date (years, days and monts) will be stored.
		 * 
		 * @return -1 on error, 0 on success.
		 */
		 int readDate (struct tm *_tm, const char *command);

		/**
		 * Reads some value from lx200 in HMS format.
		 *
		 * Utility function for all those read_ra and other.
		 *
		 * @param hmsptr	where hms will be stored
		 *
		 * @return -1 and set errno on error, otherwise 0
		 */
		int readHMS (double *hmsptr, const char *command);

		/**
		 * Computes gemini checksum
		 *
		 * @param buf checksum to compute
		 *
		 * @return computed checksum
		 */
		unsigned char tel_gemini_checksum (const char *buf);

		// higher level I/O functions
		/**
		 * Read gemini local parameters
		 *
		 * @param id	id to get
		 * @param buf	pointer where to store result (char[15] buf)
		 *
		 * @return -1 and set errno on error, otherwise 0
		 */
		int tel_gemini_getch (int id, char *in_buf);

		/**
		 * Write command to set gemini local parameters
		 *
		 * @param id	id to set
		 * @param buf	value to set (char buffer; should be 15 char long)
		 *
		 * @return -1 on error, otherwise 0
		 */
		int tel_gemini_setch (int id, char *in_buf);

		/**
		 * Write command to set gemini local parameters
		 *
		 * @param id	id to set
		 * @param val	value to set
		 *
		 * @return -1 on error, otherwise 0
		 */
		int tel_gemini_set (int id, int32_t val);
		int tel_gemini_set (int id, double val);

		/**
		 * Read gemini local parameters
		 *
		 * @param id	id to get
		 * @param val	pointer where to store result
		 *
		 * @return -1 and set errno on error, otherwise 0
		 */
		int tel_gemini_get (int id, int32_t & val);
		int tel_gemini_get (int id, double &val);
		int tel_gemini_get (int id, int &val1, int &val2);

		/**
		 * Reset and home losmandy telescope
		 */
		int tel_gemini_reset ();

		/**
		 * Match gemini time with system time
		 */
		int matchTime ();

		/**
		 * Reads lx200 right ascenation.
		 *
		 * @param raptr		where ra will be stored
		 *
		 * @return -1 and set errno on error, otherwise 0
		 */
		int tel_read_ra ();

		/**
		 * Reads losmandy declination.
		 *
		 * @param decptr	where dec will be stored
		 *
		 * @return -1 and set errno on error, otherwise 0
		 */
		int tel_read_dec ();

		/**
		 * Returns losmandy local time.
		 *
		 * @param tptr		where time will be stored
		 *
		 * @return -1 and errno on error, otherwise 0
		 */
		int getLocaltime ();

		/**
		 * Reads losmandy longtitude.
		 *
		 * @param latptr	where longtitude will be stored
		 *
		 * @return -1 and errno on error, otherwise 0
		 */
		int tel_read_longtitude ();

		/**
		 * Reads losmandy latitude.
		 *
		 * @param latptr	here latitude will be stored
		 *
		 * @return -1 and errno on error, otherwise 0
		 */
		int tel_read_latitude ();

		/**
		 * Repeat losmandy write.
		 *
		 * Handy for setting ra and dec.
		 * Meade tends to have problems with that.
		 *
		 * @param command	command to write on port
		 */
		int tel_rep_write (char *command);

		/**
		 * Set losmandy right ascenation.
		 *
		 * @param ra		right ascenation to set in decimal degrees
		 *
		 * @return -1 and errno on error, otherwise 0
		 */
		int tel_write_ra (double ra);

		/**
		 * Set losmandy declination.
		 *
		 * @param dec		declination to set in decimal degrees
		 *
		 * @return -1 and errno on error, otherwise 0
		 */
		int tel_write_dec (double dec);

		/**
		 * Set slew rate.
		 *
		 * @param new_rate	new rate to set. Uses RATE_<SPEED> constant.
		 *
		 * @return -1 on failure & set errno, 5 (>=0) otherwise
		 */
		int tel_set_rate (char new_rate);

		/**
		 * Start slew.
		 *
		 * @see tel_set_rate() for speed.
		 *
		 * @param direction 	direction
		 *
		 * @return -1 on failure, 0 on sucess.
		 */
		int telescope_start_move (char direction);

		/**
		 * Stop sleew.
		 *
		 * @see tel_start_slew for direction.
		 */
		int telescope_stop_move (char direction);

		void telescope_stop_goto ();

		int32_t raRatio;
		int32_t decRatio;

		double maxPrecGuideRa;
		double maxPrecGuideDec;

		int readRatiosInter (int startId);
		int readRatios ();
		int readLimits ();
		int setCorrection ();

		int geminiInit ();

		double fixed_ha;
		int fixed_ntries;

		//		int startMoveFixedReal ();
	#ifdef L4_GUIDE
		bool isGuiding (struct timeval *now);
		int guide (char direction, unsigned int val);
		int changeDec ();
	#endif
		int change_real (double chng_ra, double chng_dec);

		int forcedReparking;	 // used to count reparking, when move fails
		double lastMoveRa;
		double lastMoveDec;

		int tel_start_move ();

		enum
		{ TEL_OK, TEL_BLOCKED_RESET, TEL_BLOCKED_PARKING }
		telMotorState;
		int32_t lastMotorState;
		time_t moveTimeout;

		int infoCount;
		int matchCount;
		int bootesSensors;
	#ifdef L4_GUIDE
		struct timeval changeTime;
	#else
		struct timeval changeTimeRa;
		struct timeval changeTimeDec;

		int change_ra (double chng_ra);
		int change_dec (double chng_dec);
	#endif
		bool guideDetected;

		void getAxis ();
		int parkBootesSensors ();

		int32_t worm;
		int worm_move_needed;	 // 1 if we need to put mount to 130 tracking (we are performing RA move)

		double nextChangeDec;
		bool decChanged;

		char gem_version;
		char gem_subver;

		int decFlipLimit;

		double haMinusLimit;
		double haPlusLimit;

		rts2core::ValueInteger *centeringSpeed;
		rts2core::ValueFloat *guidingSpeed;
		rts2core::ValueDouble *guideLimit;

		int expectedType;
		int forceType;
		bool forceLatLon;

		const char *getGemType (int gem_type);
};

}

using namespace rts2teld;

int Gemini::tel_write_read (const char *buf, int wcount, char *rbuf, int rcount)
{
	int ret;
	ret = tel_conn->writeRead (buf, wcount, rbuf, rcount);
	usleep (USEC_SEC / 15);
	if (ret < 0)
	{
		// try rebooting
		tel_gemini_reset ();
		usleep (USEC_SEC / 5);
		ret = tel_conn->writeRead (buf, wcount, rbuf, rcount);
		usleep (USEC_SEC / 15);
	}
	return ret;
}

int Gemini::tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount)
{
	int tmp_rcount = tel_conn->writeRead (wbuf, wcount, rbuf, rcount, '#');
	usleep (USEC_SEC / 15);
	if (tmp_rcount < 0)
	{
		tel_gemini_reset ();
		usleep (USEC_SEC / 5);
		return -1;
	}
	// end hash..
	rbuf[tmp_rcount - 1] = '\0';
	return tmp_rcount;
}

int Gemini::readDate (struct tm *_tm, const char *command)
{
	char wbuf[11];
	int ret;
	_tm->tm_hour = _tm->tm_min = _tm->tm_sec = _tm->tm_isdst = 0;
	if (tel_write_read_hash (command, strlen (command), wbuf, 10) < 9)
		return -1;
	ret = sscanf (wbuf, "%u/%u/%u", &(_tm->tm_mon), &(_tm->tm_mday), &(_tm->tm_year));
	if (ret != 3)
	{
		logStream (MESSAGE_ERROR) << "invalid date read " << wbuf << " " << ret << sendLog;
		return -1;
	}
	// date will always be after 2000..
	_tm->tm_year += 100;
	_tm->tm_mon --;
	return 0;
}

int Gemini::readHMS (double *hmsptr, const char *command)
{
	char wbuf[11];
	if (tel_write_read_hash (command, strlen (command), wbuf, 10) < 6)
		return -1;
	*hmsptr = hmstod (wbuf);
	if (isnan (*hmsptr))
		return -1;
	return 0;
}

unsigned char Gemini::tel_gemini_checksum (const char *buf)
{
	unsigned char checksum = 0;
	for (; *buf; buf++)
		checksum ^= *buf;
	checksum += 64;
	checksum %= 128;			 // modulo 128
	return checksum;
}

int Gemini::tel_gemini_setch (int id, char *in_buf)
{
	int len;
	char *buf;
	int ret;
	len = strlen (in_buf);
	buf = (char *) malloc (len + 10);
	len = sprintf (buf, ">%i:%s", id, in_buf);
	buf[len] = tel_gemini_checksum (buf);
	len++;
	buf[len] = '#';
	len++;
	buf[len] = '\0';
	ret = tel_conn->writePort (buf, len);
	usleep (USEC_SEC / 15);
	free (buf);
	return ret;
}

int Gemini::tel_gemini_set (int id, int32_t val)
{
	char buf[25];
	int len;
	int ret;
	len = sprintf (buf, ">%i:%i", id, val);
	buf[len] = tel_gemini_checksum (buf);
	len++;
	buf[len] = '#';
	len++;
	buf[len] = '\0';
	ret = tel_conn->writePort (buf, len);
	usleep (USEC_SEC / 15);
	return ret;
}

int Gemini::tel_gemini_set (int id, double val)
{
	char buf[25];
	int len;
	int ret;
	len = sprintf (buf, ">%i:%0.1f", id, val);
	buf[len] = tel_gemini_checksum (buf);
	len++;
	buf[len] = '#';
	len++;
	buf[len] = '\0';
	ret = tel_conn->writePort (buf, len);
	usleep (USEC_SEC / 15);
	return ret;
}

int Gemini::tel_gemini_getch (int id, char *buf)
{
	char *ptr;
	unsigned char checksum, cal;
	int len, ret;
	len = sprintf (buf, "<%i:", id);
	buf[len] = tel_gemini_checksum (buf);
	len++;
	buf[len] = '#';
	len++;
	buf[len] = '\0';
	ret = tel_write_read_hash (buf, len, buf, 20);
	if (ret < 0)
		return ret;
	ptr = buf + ret - 1;
	checksum = *ptr;
	*ptr = '\0';
	cal = tel_gemini_checksum (buf);
	if (cal != checksum)
	{
		logStream (MESSAGE_ERROR) << "invalid gemini checksum: should be " <<
			cal << " is " << checksum << " " << ret << sendLog;
		if (*buf || checksum)
			sleep (5);
		tel_gemini_reset ();
		usleep (USEC_SEC / 5);
		*buf = '\0';
		return -1;
	}
	return 0;
}

int Gemini::tel_gemini_get (int id, int32_t & val)
{
	char buf[19], *ptr, checksum;
	int len, ret;
	len = sprintf (buf, "<%i:", id);
	buf[len] = tel_gemini_checksum (buf);
	len++;
	buf[len] = '#';
	len++;
	buf[len] = '\0';
	ret = tel_write_read_hash (buf, len, buf, 8);
	if (ret < 0)
		return ret;
	for (ptr = buf; *ptr && (isdigit (*ptr) || *ptr == '.' || *ptr == '-');
		ptr++);
	checksum = *ptr;
	*ptr = '\0';
	if (tel_gemini_checksum (buf) != checksum)
	{
		logStream (MESSAGE_ERROR) << "invalid gemini checksum: should be " <<
			tel_gemini_checksum (buf) << " is " << checksum << sendLog;
		if (*buf)
			sleep (5);
		tel_conn->flushPortIO ();
		usleep (USEC_SEC / 15);
		return -1;
	}
	val = atol (buf);
	return 0;
}

int Gemini::tel_gemini_get (int id, double &val)
{
	char buf[19], *ptr, checksum;
	int len, ret;
	len = sprintf (buf, "<%i:", id);
	buf[len] = tel_gemini_checksum (buf);
	len++;
	buf[len] = '#';
	len++;
	buf[len] = '\0';
	ret = tel_write_read_hash (buf, len, buf, 8);
	if (ret < 0)
		return ret;
	for (ptr = buf; *ptr && (isdigit (*ptr) || *ptr == '.' || *ptr == '-');
		ptr++);
	checksum = *ptr;
	*ptr = '\0';
	if (tel_gemini_checksum (buf) != checksum)
	{
		logStream (MESSAGE_ERROR) << "invalid gemini checksum: should be " <<
			tel_gemini_checksum (buf) << " is " << checksum << sendLog;
		if (*buf)
			sleep (5);
		tel_conn->flushPortIO ();
		usleep (USEC_SEC / 15);
		return -1;
	}
	val = atof (buf);
	return 0;
}

int Gemini::tel_gemini_get (int id, int &val1, int &val2)
{
	char buf[20], *ptr;
	int ret;
	ret = tel_gemini_getch (id, buf);
	if (ret)
		return ret;
	// parse returned string
	ptr = strchr (buf, ';');
	if (!ptr)
		return -1;
	*ptr = '\0';
	ptr++;
	val1 = atoi (buf);
	val2 = atoi (ptr);
	return 0;
}

int Gemini::tel_gemini_reset ()
{
	char rbuf[50];

	// write_read_hash
	if (tel_conn->flushPortIO () < 0)
		return -1;
	usleep (USEC_SEC / 15);

	if (tel_conn->writeRead ("\x06", 1, rbuf, 47, '#') < 0)
	{
		tel_conn->flushPortIO ();
		usleep (USEC_SEC / 15);
		return -1;
	}

	if (*rbuf == 'b')			 // booting phase, select warm reboot
	{
		switch (resetState->getValueInteger ())
		{
			case 0:
				tel_conn->writePort ("bR#", 3);
				usleep (USEC_SEC / 15);
				break;
			case 1:
				tel_conn->writePort ("bW#", 3);
				usleep (USEC_SEC / 15);
				break;
			case 2:
				tel_conn->writePort ("bC#", 3);
				usleep (USEC_SEC / 15);
				break;
		}
		resetState->setValueInteger (0);
		setTimeout (USEC_SEC);
	}
	else if (*rbuf != 'G')
	{
		// something is wrong, reset all comm
		sleep (10);
		tel_conn->flushPortIO ();
	}
	return -1;
}


int
Gemini::matchTime ()
{
	struct tm ts;
	time_t t;
	char buf[55];
	int ret;
	char rep;
	if (matchCount)
		return 0;
	t = time (NULL);
	gmtime_r (&t, &ts);
	// set time
	ret = tel_write_read (":SG+00#", 7, &rep, 1);
	if (ret < 0)
		return ret;
	snprintf (buf, 14, ":SL%02d:%02d:%02d#", ts.tm_hour, ts.tm_min, ts.tm_sec);
	ret = tel_write_read (buf, strlen (buf), &rep, 1);
	if (ret < 0)
		return ret;
	snprintf (buf, 14, ":SC%02d/%02d/%02d#", ts.tm_mon + 1, ts.tm_mday,
		ts.tm_year - 100);
	ret = tel_write_read_hash (buf, strlen (buf), buf, 55);
	if (ret < 0)
		return ret;
	if (*buf != '1')
	{
		return -1;
	}
	// read spaces
	ret = tel_conn->readPort (buf, 26, '#');
	usleep (USEC_SEC / 15);
	if (ret)
	{
		matchCount = 1;
		return ret;
	}
	logStream (MESSAGE_INFO) << "GEMINI: time match" << sendLog;
	return 0;
}

int Gemini::tel_read_ra ()
{
	double t_telRa;
	if (readHMS (&t_telRa, ":GR#"))
		return -1;
	t_telRa *= 15.0;
	setTelRa (t_telRa);
	return 0;
}

int Gemini::tel_read_dec ()
{
	double t_telDec;
	if (readHMS (&t_telDec, ":GD#"))
		return -1;
	setTelDec (t_telDec);
	return 0;
}

int Gemini::getLocaltime ()
{
	double t_telLocalTime;
	struct tm _tm;
	if (readDate (&_tm, ":GC#"))
		return -1;
	if (readHMS (&t_telLocalTime, ":GL#"))
		return -1;
	// change hours to seconds..
	t_telLocalTime *= 3600;
	t_telLocalTime += mktime (&_tm);
	telLocalTime->setValueDouble (t_telLocalTime);
	return 0;
}

int Gemini::tel_read_longtitude ()
{
	int ret;
	double t_telLongitude;
	ret = readHMS (&t_telLongitude, ":Gg#");
	if (ret)
		return ret;
	telLongitude->setValueDouble (-1 * t_telLongitude);
	return ret;
}

int Gemini::tel_read_latitude ()
{
	double t_telLatitude;
	if (readHMS (&t_telLatitude, ":Gt#"))
		return -1;
	telLatitude->setValueDouble (t_telLatitude);
	return 0;
}

int Gemini::tel_rep_write (char *command)
{
	int count;
	char retstr;
	for (count = 0; count < 3; count++)
	{
		if (tel_write_read (command, strlen (command), &retstr, 1) < 0)
			return -1;
		if (retstr == '1')
			break;
		sleep (1);
		tel_conn->flushPortIO ();
		usleep (USEC_SEC / 15);
		logStream (MESSAGE_DEBUG) << "Losmandy tel_rep_write - for " << count <<
			" time" << sendLog;
	}
	if (count == 200)
	{
		logStream (MESSAGE_ERROR) <<
			"losmandy tel_rep_write unsucessful due to incorrect return." <<
			sendLog;
		return -1;
	}
	return 0;
}

int Gemini::tel_write_ra (double ra)
{
	char command[14];
	int h, m, s;
	ra = ra / 15.0;
	dtoints (ra, &h, &m, &s);
	if (snprintf (command, 14, ":Sr%02d:%02d:%02d#", h, m, s) < 0)
		return -1;
	return tel_rep_write (command);
}

int Gemini::tel_write_dec (double dec)
{
	char command[15];
	struct ln_dms dh;
	char sign = '+';
	if (dec < 0)
	{
		sign = '-';
		dec = -1 * dec;
	}
	ln_deg_to_dms (dec, &dh);
	if (snprintf
		(command, 15, ":Sd%c%02d*%02d:%02.0f#", sign, dh.degrees, dh.minutes,
		dh.seconds) < 0)
		return -1;
	return tel_rep_write (command);
}

Gemini::Gemini (int in_argc, char **in_argv):Telescope (in_argc, in_argv)
{
	createValue (telLocalTime, "localtime", "telescope local time", false);
	createValue (telGuidingSpeed, "guiding_speed", "telescope guiding speed", false);

	createValue (resetState, "next_reset", "next reset state", false, RTS2_VALUE_WRITABLE);
	resetState->addSelVal ("RESTART");
	resetState->addSelVal ("WARM_START");
	resetState->addSelVal ("COLD_START");

	createValue (featurePort, "feature_port", "state of feature port", false);

	createValue (axRa, "CNT_RA", "RA axis ticks", true);
	createValue (axDec, "CNT_DEC", "DEC axis ticks", true);

	device_file = "/dev/ttyS0";
	geminiConfig = "/etc/rts2/gemini.ini";
	bootesSensors = 0;
	expectedType = 0;
	forceType = 0;
	forceLatLon = false;

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttySx");
	addOption ('c', NULL, 1, "config file (with model parameters)");
	addOption (OPT_BOOTES, "bootes", 0,
		"BOOTES G-11 (with 1/0 pointing sensor)");
	addOption (OPT_CORR, "corrections", 1,
		"level of correction done in Gemini - 0 none, 3 all");
	addOption (OPT_EXPTYPE, "expected-type", 1,
		"expected Gemini type (1 GM8, 2 G11, 3 HGM-200, 4 CI700, 5 Titan, 6 Titan50)");
	addOption (OPT_FORCETYPE, "force-type", 1,
		"force Gemini type (1 GM8, 2 G11, 3 HGM-200, 4 CI700, 5 Titan, 6 Titan50)");
	addOption (OPT_FORCELATLON, "force-latlon", 0, "set observing longitude and latitude from configuration file");

	lastMotorState = 0;
	telMotorState = TEL_OK;
	infoCount = 0;
	matchCount = 0;

	tel_conn = NULL;

	worm = 0;
	worm_move_needed = 0;

	#ifdef L4_GUIDE
	timerclear (&changeTime);
	#else
	timerclear (&changeTimeRa);
	timerclear (&changeTimeDec);
	#endif

	guideDetected = false;

	nextChangeDec = 0;
	decChanged = false;
	// default guiding speed
	telGuidingSpeed->setValueDouble (0.2);

	fixed_ha = NAN;

	decFlipLimit = 4000;

	createValue (centeringSpeed, "centeringSpeed", "speed used for centering mount on target", false);
	centeringSpeed->setValueInteger (2);

	createValue (guidingSpeed, "guidingSpeed", "speed used for guiding mount on target", false);
	guidingSpeed->setValueFloat (0.8);

	createValue (guideLimit, "guideLimit", "limit to to change between ", false);
	guideLimit->setValueDouble (5.0 / 60.0);
}

Gemini::~Gemini ()
{
}

int Gemini::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 'c':
			geminiConfig = optarg;
			break;
		case OPT_BOOTES:
			bootesSensors = 1;
			break;
		case OPT_CORR:
			switch (*optarg)
			{
				case '0':
					setCorrections (true, true, true);
					break;
				case '1':
					setCorrections (false, false, true);
					break;
				case '2':
					setCorrections (true, true, false);
					break;
				case '3':
					setCorrections (false, false, false);
					break;
				default:
					std::cerr << "Invalid correction option " << optarg << std::endl;
					return -1;
			}
		case OPT_EXPTYPE:
			expectedType = atoi (optarg);
			break;
		case OPT_FORCETYPE:
			forceType = atoi (optarg);
			break;
		case OPT_FORCELATLON:
			forceLatLon = true;
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int Gemini::geminiInit ()
{
	char rbuf[10];
	int ret;

	tel_gemini_reset ();

	// 12/24 hours trick..
	if (tel_write_read (":Gc#", 4, rbuf, 5) < 0)
		return -1;
	rbuf[5] = 0;
	if (strncmp (rbuf, "(24)#", 5))
		return -1;

	// we get 12:34:4# while we're in short mode
	// and 12:34:45 while we're in long mode
	if (tel_write_read_hash (":GR#", 4, rbuf, 9) < 0)
		return -1;
	if (rbuf[7] == '\0')
	{
		// that could be used to distinguish between long
		// short mode
		// we are in short mode, set the long on
		if (tel_write_read (":U#", 5, rbuf, 0) < 0)
			return -1;
	}
	tel_write_read_hash (":Ml#", 5, rbuf, 1);
	if (bootesSensors)
		tel_gemini_set (311, 15);// reset feature port

	ret = tel_write_read_hash (":GV#", 4, rbuf, 4);
	if (ret <= 0)
		return -1;

	gem_version = rbuf[0] - '0';
	gem_subver = (rbuf[1] - '0') * 10 + (rbuf[2] - '0');

	return 0;
}

int Gemini::readRatiosInter (int startId)
{
	int32_t t;
	int res = 1;
	int id;
	int ret;
	for (id = startId; id < startId + 5; id += 2)
	{
		ret = tel_gemini_get (id, t);
		if (ret)
			return -1;
		res *= t;
	}
	return res;
}

int Gemini::readRatios ()
{
	raRatio = 0;
	decRatio = 0;

	if (gem_version < 4)
	{
		maxPrecGuideRa = 5 / 60.0;
		maxPrecGuideDec = 5 / 60.0;
		return 0;
	}

	raRatio = readRatiosInter (21);
	decRatio = readRatiosInter (22);

	if (raRatio == -1 || decRatio == -1)
		return -1;

	maxPrecGuideRa = raRatio;
	maxPrecGuideRa = (255 * 360.0 * 3600.0) / maxPrecGuideRa;
	maxPrecGuideRa /= 3600.0;

	maxPrecGuideDec = decRatio;
	maxPrecGuideDec = (255 * 360.0 * 3600.0) / maxPrecGuideDec;
	maxPrecGuideDec /= 3600.0;

	return 0;
}

int Gemini::readLimits ()
{
	// TODO read from 221 and 222
	haMinusLimit = -94;
	haPlusLimit = 97;
	return 0;
}

int Gemini::setCorrection ()
{
	if (gem_version < 4)
		return 0;
	
	if (calculateAberation () && calculatePrecession () && calculateRefraction ())
		return tel_conn->writePort (":p0#", 4);
	if (!calculateAberation () && !calculatePrecession () && calculateRefraction ())	
		return tel_conn->writePort (":p1#", 4);
	if (calculateAberation () && calculatePrecession () && !calculateRefraction ())
		return tel_conn->writePort (":p2#", 4);
	if (!calculateAberation () && !calculatePrecession () && !calculateRefraction ())
		return tel_conn->writePort (":p3#", 4);
	usleep (USEC_SEC / 15);
	return -1;
}

int Gemini::init ()
{
	int ret;

	ret = Telescope::init ();
	if (ret)
		return ret;

	tel_conn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 90);
	tel_conn->setDebug ();

	ret = tel_conn->init ();
	if (ret)
		return ret;

	while (1)
	{
		tel_conn->flushPortIO ();

		ret = geminiInit ();
		if (!ret)
		{
			ret = readRatios ();
			if (ret)
				return ret;
			ret = readLimits ();
			if (ret)
				return ret;
			setCorrection ();

			tel_conn->writePort (":hW#", 4);
			usleep (USEC_SEC / 15);

			return ret;
		}

		sleep (60);
	}
	return 0;
}

const char * Gemini::getGemType (int gem_type)
{
	switch (gem_type)
	{
		case 1:
			return "GM8";
		case 2:
			return "G-11";
		case 3:
			return "HGM-200";
		case 4:
			return "CI700";
		case 5:
			return "Titan";
		case 6:
			return "Titan50";
	}
	return "UNK";
}

int Gemini::initValues ()
{
	int32_t gem_type;
	char buf[5];
	int ret;

	if (tel_read_longtitude () || tel_read_latitude ())
		return -1;
	if (forceType > 0)
	{
		ret = tel_gemini_set (forceType, forceType);
		if (ret)
			return ret;
	}

	// Set observing site longitude and latitude from configuration file
	if (forceLatLon)
	{
		char command[20];
		struct ln_dms dlat;
		char sign = '+';
		rts2core::Configuration *config = rts2core::Configuration::instance ();
		ret = config->loadFile ();
		if (ret)
			return ret;

		double tdeg = config->getObserver ()->lat;
		if (tdeg < 0)
		{
			sign = '-';
			tdeg *= -1;
		}
		ln_deg_to_dms (tdeg, &dlat);
		sprintf (command, ":St%c%02d*%02d#", sign, dlat.degrees, dlat.minutes);
		ret = tel_write_read (command, strlen (command), command, 1);
		if (ret != 1)
			return -1;

		sign = '+';
		tdeg = config->getObserver ()->lng * -1;
		if (tdeg < 0)
		{
			sign = '-';
			tdeg *= -1;
		}
		ln_deg_to_dms (tdeg, &dlat);
		sprintf (command, ":Sg%c%03d*%02d#", sign, dlat.degrees, dlat.minutes);
		ret = tel_write_read (command, strlen (command), command, 1);
		if (ret != 1)
			return -1;
	}

	ret = tel_gemini_get (0, gem_type);
	if (ret)
		return ret;
	strcpy (telType, getGemType (gem_type));
	switch (gem_type)
	{
		case 6:
			decFlipLimit = 6750;
	}
	if (expectedType > 0 && gem_type != expectedType)
	{
		logStream (MESSAGE_ERROR) <<
			"Cannot init teld, because, it's type is not expected. Expected " <<
			expectedType << " (" << getGemType (expectedType) << "), get " <<
			gem_type << " (" << telType << ")." << sendLog;
		return -1;
	}
	ret = tel_write_read_hash (":GV#", 4, buf, 4);
	if (ret <= 0)
		return -1;
	buf[4] = '\0';
	strcat (telType, "_");
	strcat (telType, buf);
	telAltitude->setValueDouble (600);

	return Telescope::initValues ();
}

int Gemini::idle ()
{
	int ret;
	tel_gemini_get (130, worm);
	ret = tel_gemini_get (99, lastMotorState);
	// if moving, not on limits & need info..
	if ((lastMotorState & 8) && (!(lastMotorState & 16)) && infoCount % 25 == 0)
	{
		info ();
	}
	infoCount++;
	if (telMotorState == TEL_OK
		&& ((ret && (getState () & TEL_MASK_MOVING) == TEL_MASK_MOVING)
		|| (lastMotorState & 16)))
	{
		stopMove ();
		resetState->setValueInteger (0);
		resetMount ();
		telMotorState = TEL_BLOCKED_RESET;
	}
	else if (!ret && telMotorState == TEL_BLOCKED_RESET)
	{
		// we get telescope back from reset state
		telMotorState = TEL_BLOCKED_PARKING;
	}
	if (telMotorState == TEL_BLOCKED_PARKING)
	{
		if (forcedReparking < 10)
		{
			// ignore for the moment tel state, pretends, that everything
			// is OK
			telMotorState = TEL_OK;
			if (forcedReparking % 2 == 0)
			{
				startPark ();
				forcedReparking++;
			}
			else
			{
				ret = isParking ();
				switch (ret)
				{
					case -2:
						// parked, let's move us to location where we belong
						forcedReparking = 0;
						if ((getState () & TEL_MASK_MOVING) == TEL_MOVING)
						{
							// not ideal; lastMoveRa contains (possibly corrected) move values
							// but we don't care much about that as we have reparked..
							setTarget (lastMoveRa, lastMoveDec);
							tel_start_move ();
							tel_gemini_get (130, worm);
							ret = tel_gemini_get (99, lastMotorState);
							if (ret)
							{
								// still not responding, repark
								resetState->setValueInteger (0);
								resetMount ();
								telMotorState = TEL_BLOCKED_RESET;
								forcedReparking++;
							}
						}
						break;
					case -1:
						forcedReparking++;
						break;
				}
			}
			if (forcedReparking > 0)
				telMotorState = TEL_BLOCKED_PARKING;
			setTimeout (USEC_SEC);
		}
		else
		{
			stopWorm ();
			stopMove ();
			ret = -1;
		}
	}
	//sleep (5);
	return Telescope::idle ();
}

void Gemini::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	matchCount = 0;
	return Telescope::changeMasterState (old_state, new_state);
}

void Gemini::getAxis ()
{
	int feature;
	tel_gemini_get (311, feature);
	featurePort->setValueInteger (feature);
	telFlip->setValueInteger ((feature & 2) ? 1 : 0);
}

int Gemini::info ()
{
	telFlip->setValueInteger (0);

	if (tel_read_ra () || tel_read_dec () || getLocaltime ())
		return -1;
	if (bootesSensors)
	{
		getAxis ();
	}
	else
	{
		telFlip->setValueInteger (getFlip ());
	}

	return Telescope::info ();
}

int Gemini::tel_set_rate (char new_rate)
{
	char command[6];
	int ret;
	sprintf (command, ":R%c#", new_rate);
	ret = tel_conn->writePort (command, 5);
	usleep (USEC_SEC / 15);
	return ret;
}

int Gemini::telescope_start_move (char direction)
{
	char command[6];
	// start worm if moving in RA DEC..
	/*  if (worm == 135 && (direction == DIR_EAST || direction == DIR_WEST))
		{
		  // G11 (version 3.x) have some problems with starting worm..give it some time
		  // to react
		  startWorm ();
		  usleep (USEC_SEC / 10);
		  startWorm ();
		  usleep (USEC_SEC / 10);
		  worm_move_needed = 1;
		}
	  else
		{
		  worm_move_needed = 0;
		} */
	sprintf (command, ":M%c#", direction);
	return tel_conn->writePort (command, 5);
	// workaround suggested by Rene Goerlich
	//if (worm_move_needed == 1)
	//  stopWorm ();
}

int Gemini::telescope_stop_move (char direction)
{
	char command[6];
	int ret;
	sprintf (command, ":Q%c#", direction);
	if (worm_move_needed && (direction == DIR_EAST || direction == DIR_WEST))
	{
		worm_move_needed = 0;
		stopWorm ();
	}
	ret = tel_conn->writePort (command, 5);
	usleep (USEC_SEC / 15);
	return ret;
}

void Gemini::telescope_stop_goto ()
{
	tel_gemini_get (99, lastMotorState);
	tel_conn->writePort (":Q#", 3);
	usleep (USEC_SEC / 15);
	if (lastMotorState & 8)
	{
		lastMotorState &= ~8;
		tel_gemini_set (99, lastMotorState);
	}
	worm_move_needed = 0;
}

int Gemini::tel_start_move ()
{
	char retstr;
	char buf[55];

	telescope_stop_goto ();

	if ((tel_write_ra (lastMoveRa) < 0) || (tel_conn->writePort (":ONtest#", 8) < 0)
		|| (tel_write_dec (lastMoveDec) < 0))
		return -1;
	if (tel_write_read (":MS#", 4, &retstr, 1) < 0)
		return -1;

	if (retstr == '0')
	{
		sleep (5);
		return 0;
	}
	// otherwise read reply..
	tel_conn->readPort (buf, 53, '#');
	usleep (USEC_SEC / 15);
	if (retstr == '3')			 // manual control..
		return 0;
	return -1;
}

int Gemini::startResync ()
{
	int newFlip = telFlip->getValueInteger ();
	bool willFlip = false;
	double ra_diff, ra_diff_flip;
	double dec_diff, dec_diff_flip;
	double ha;

	double max_not_flip, max_flip;

	struct ln_equ_posn pos;
	struct ln_equ_posn model_change;

	getTarget (&pos);

	normalizeRaDec (pos.ra, pos.dec);

	ra_diff = ln_range_degrees (getTelTargetRa () - pos.ra);
	if (ra_diff > 180.0)
		ra_diff -= 360.0;

	dec_diff = getTelTargetDec () - pos.dec;

	// get diff when we flip..
	ra_diff_flip =
		ln_range_degrees (180 + getTelTargetRa () - pos.ra);
	if (ra_diff_flip > 180.0)
		ra_diff_flip -= 360.0;

	if (telLatitude->getValueDouble () > 0)
		dec_diff_flip = (90 - getTelTargetDec () + 90 - pos.dec);
	else
		dec_diff_flip = (getTelTargetDec () - 90 + pos.dec - 90);

	// decide which path is closer

	max_not_flip = MAX (fabs (ra_diff), fabs (dec_diff));
	max_flip = MAX (fabs (ra_diff_flip), fabs (dec_diff_flip));

	if (max_flip < max_not_flip)
		willFlip = true;

	logStream (MESSAGE_DEBUG) << "Losmandy start move ra_diff " << ra_diff <<
		" dec_diff " << dec_diff << " ra_diff_flip " << ra_diff_flip <<
		" dec_diff_flip  " << dec_diff_flip << " max_not_flip " << max_not_flip <<
		" max_flip " << max_flip << " willFlip " << willFlip << sendLog;

	// "do" flip
	if (willFlip)
		newFlip = !newFlip;

	// calculate current HA
	ha = getLocSidTime () - lastMoveRa;

	// normalize HA to meridian angle
	if (newFlip)
		ha = 90.0 - ha;
	else
		ha = ha + 90.0;

	ha = ln_range_degrees (ha);

	if (ha > 180.0)
		ha = 360.0 - ha;

	if (ha > 90.0)
		ha = ha - 180.0;

	logStream (MESSAGE_DEBUG) << "Losmandy start move newFlip " << newFlip <<
		" ha " << ha << sendLog;

	if (willFlip)
		ha += ra_diff_flip;
	else
		ha += ra_diff;

	logStream (MESSAGE_DEBUG) << "Losmandy start move newFlip " << newFlip <<
		" ha " << ha << sendLog;

	if (ha < haMinusLimit || ha > haPlusLimit)
	{
		willFlip = !willFlip;
		newFlip = !newFlip;
	}

	logStream (MESSAGE_DEBUG) << "Losmandy start move ha " << ha
		<< " ra_diff " << ra_diff
		<< " lastMoveRa " << lastMoveRa
		<< " telRa " << getTelTargetRa ()
		<< " newFlip  " << newFlip
		<< " willFlip " << willFlip
		<< sendLog;

	// we fit to limit, aply model

	applyModel (&pos, &model_change, newFlip, ln_get_julian_from_sys ());

	ra_diff = ln_range_degrees (lastMoveRa - pos.ra);
	if (ra_diff > 180.0)
		ra_diff -= 360.0;

	dec_diff = lastMoveDec - pos.dec;

	lastMoveRa = pos.ra;
	lastMoveDec = pos.dec;

	// check for small movements and perform them using change command..
	#ifdef L4_GUIDE
	if (fabs (ra_diff) <= maxPrecGuideRa && fabs (dec_diff) <= maxPrecGuideDec)
	#else
	if (fabs (ra_diff) <= 20 / 60.0 && fabs (dec_diff) <= 20 / 60.0)
	#endif
	{
		logStream (MESSAGE_DEBUG) << "calling change_real (" << ra_diff << " " << dec_diff << ")" << sendLog;
		return change_real (-1 * ra_diff, -1 * dec_diff);
	}

	fixed_ha = NAN;

	if (telMotorState != TEL_OK) // lastMoveRa && lastMoveDec will bring us to correct location after we finish rebooting/reparking
		return 0;

	startWorm ();

	struct ln_equ_posn telPos;
	getTelTargetRaDec (&telPos);

	double sep = ln_get_angular_separation (&pos, &telPos);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Losmandy change sep " << sep << sendLog;
	#endif
	if (sep > 1)
	{
		tel_set_rate (RATE_SLEW);
		tel_gemini_set (GEMINI_CMD_RATE_CENTER, 8);
	}
	else
	{
		tel_set_rate (RATE_CENTER);
		tel_gemini_set (GEMINI_CMD_RATE_CENTER,	centeringSpeed->getValueInteger ());
	}
	worm_move_needed = 0;

	forcedReparking = 0;

	time (&moveTimeout);
	moveTimeout += 300;

	return tel_start_move ();
}

int Gemini::isMoving ()
{
	struct timeval now;
	if (telMotorState != TEL_OK)
		return USEC_SEC;
	gettimeofday (&now, NULL);
	// change move..
	#ifdef L4_GUIDE
	if (changeTime.tv_sec > 0)
	{
		if (isGuiding (&now))
			return USEC_SEC / 10;
		if (nextChangeDec != 0)
		{
			// worm need to be started - most probably due to error in Gemini
			startWorm ();
			// initiate dec change
			if (changeDec ())
				return -1;
			return USEC_SEC / 10;
		}
		return -2;
	}
	#else
	int ret;
	if (changeTimeRa.tv_sec > 0 || changeTimeRa.tv_usec > 0
		|| changeTimeDec.tv_sec > 0 || changeTimeDec.tv_usec > 0)
	{
		if (changeTimeRa.tv_sec > 0 || changeTimeRa.tv_usec > 0)
		{
			if (timercmp (&changeTimeRa, &now, <))
			{
				ret = telescope_stop_move (DIR_EAST);
				if (ret == -1)
					return ret;
				ret = telescope_stop_move (DIR_WEST);
				if (ret == -1)
					return ret;
				timerclear (&changeTimeRa);
				if (nextChangeDec != 0)
				{
					ret = change_dec (nextChangeDec);
					if (ret)
						return ret;
				}
			}
		}
		if (changeTimeDec.tv_sec > 0 || changeTimeDec.tv_usec > 0)
		{
			if (timercmp (&changeTimeDec, &now, <))
			{
				nextChangeDec = 0;
				ret = telescope_stop_move (DIR_NORTH);
				if (ret == -1)
					return ret;
				ret = telescope_stop_move (DIR_SOUTH);
				if (ret == -1)
					return ret;
				timerclear (&changeTimeDec);
			}
		}
		if (changeTimeRa.tv_sec == 0 && changeTimeRa.tv_usec == 0
			&& changeTimeDec.tv_sec == 0 && changeTimeDec.tv_usec == 0)
			return -2;
		return USEC_SEC / 10;
	}
	#endif
	if (now.tv_sec > moveTimeout)
	{
		stopMove ();
		return -2;
	}
	if (lastMotorState & 8)
		return USEC_SEC / 10;
	return -2;
}

int Gemini::endMove ()
{
	int32_t track;
	#ifdef L4_GUIDE
	// don't start tracking while we are performing only change - it seems to
	// disturb RA tracking
	if (changeTime.tv_sec > 0)
	{
		if (!decChanged)
			startWorm ();
		decChanged = false;
		timerclear (&changeTime);
		return Telescope::endMove ();
	}
	#endif
	tel_gemini_get (130, track);
	setTimeout (USEC_SEC);
	return Telescope::endMove ();
}

int Gemini::stopMove ()
{
	telescope_stop_goto ();
	#ifdef L4_GUIDE
	timerclear (&changeTime);
	#else
	timerclear (&changeTimeRa);
	timerclear (&changeTimeDec);
	#endif
	return 0;
}


/*
int
Gemini::startMoveFixedReal ()
{
	int ret;

	// compute ra
	lastMoveRa = getLocSidTime () * 15.0 - fixed_ha;

	normalizeRaDec (lastMoveRa, lastMoveDec);

	if (telMotorState != TEL_OK) // same as for startMove - after repark. we will get to correct location
		return 0;

	stopWorm ();

	time (&moveTimeout);
	moveTimeout += 300;

	ret = tel_start_move ();
	if (!ret)
	{
		setTarget (lastMoveRa, lastMoveDec);
		fixed_ntries++;
	}
	return ret;
}
*/

/*
int Gemini::startMoveFixed (double tar_ha, double tar_dec)
{
	int32_t ra_ind;
	int32_t dec_ind;
	int ret;

	double ha_diff;
	double dec_diff;

	ret = tel_gemini_get (205, ra_ind);
	if (ret)
		return ret;
	ret = tel_gemini_get (206, dec_ind);
	if (ret)
		return ret;

	// apply offsets
	//tar_ha += ((double) ra_ind) / 3600;
	//tar_dec += ((double) dec_ind) / 3600.0;

	// we moved to fixed ra before..
	if (!isnan (fixed_ha))
	{
		// check if we can only change..
		// if we want to go to W, diff have to be negative, as RA is decreasing to W,
		// so we need to substract tar_ha from last one (smaller - bigger number)
		ha_diff = ln_range_degrees (fixed_ha - tar_ha);
		dec_diff = tar_dec - lastMoveDec;
		if (ha_diff > 180)
			ha_diff = ha_diff - 360;
		// do changes smaller then max change arc min using precision guide command
		#ifdef L4_GUIDE
		if (fabs (ha_diff) < maxPrecGuideRa
			&& fabs (dec_diff) < maxPrecGuideDec)
		#else
			if (fabs (ha_diff) < 5 / 60.0 && fabs (dec_diff) < 5 / 60.0)
		#endif
		{
			ret = change_real (ha_diff, dec_diff);
			if (!ret)
			{
				fixed_ha = tar_ha;
				lastMoveDec += dec_diff;
				// move_fixed = 0;        // we are performing change, not moveFixed
				maskState (TEL_MASK_MOVING, TEL_MOVING, "change started");
				return ret;
			}
		}
	}

	fixed_ha = tar_ha;
	lastMoveDec = tar_dec;

	fixed_ntries = 0;

	ret = startMoveFixedReal ();
	// move OK
	if (!ret)
		return Telescope::startMoveFixed (tar_ha, tar_dec);
	// try to do small change..
	#ifndef L4_GUIDE
	if (!isnan (fixed_ha) && fabs (ha_diff) < 5 && fabs (dec_diff) < 5)
	{
		ret = change_real (ha_diff, dec_diff);
		if (!ret)
		{
			fixed_ha = tar_ha;
			lastMoveDec += dec_diff;
			// move_fixed = 0;    // we are performing change, not moveFixed
			maskState (TEL_MASK_MOVING, TEL_MOVING, "change started");
			return Telescope::startMoveFixed (tar_ha, tar_dec);
		}
	}
	#endif
	return ret;
}

int Gemini::isMovingFixed ()
{
	int ret;
	ret = isMoving ();
	// move ended
	if (ret == -2 && fixed_ntries < 3)
	{
		struct ln_equ_posn pos1, pos2;
		double sep;
		// check that we reach destination..
		info ();
		pos1.ra = getLocSidTime () * 15.0 - fixed_ha;
		pos1.dec = lastMoveDec;

		pos2.ra = telRaDec->getRa ();
		pos2.dec = telRaDec->getDec ();
		sep = ln_get_angular_separation (&pos1, &pos2);
								 // 15 seconds..
		if (sep > 15.0 / 60.0 / 4.0)
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "Losmandy isMovingFixed sep " << sep *
				3600 << " arcsec" << sendLog;
			#endif
			// reque move..
			ret = startMoveFixedReal ();
			if (ret)			 // end in case of error
				return -2;
			return USEC_SEC;
		}
	}
	return ret;
}

int Gemini::endMoveFixed ()
{
	int32_t track;
	stopWorm ();
	tel_gemini_get (130, track);
	setTimeout (USEC_SEC);
	if (tel_conn->writePort (":ONfixed#", 9) == 0)
		return Telescope::endMoveFixed ();
	return -1;
}
*/

int Gemini::isParking ()
{
	char buf = '3';
	if (telMotorState != TEL_OK)
		return USEC_SEC;
	if (lastMotorState & 8)
		return USEC_SEC;
	tel_write_read (":h?#", 4, &buf, 1);
	switch (buf)
	{
		case '2':
		case ' ':
			return USEC_SEC;
		case '1':
			return -2;
		case '0':
			logStream (MESSAGE_ERROR) <<
				"Losmandy isParking called without park command" << sendLog;
		default:
			return -1;
	}
}

int Gemini::endPark ()
{
	if (getMasterState () != SERVERD_NIGHT)
		matchTime ();
	setTimeout (USEC_SEC * 10);
	return stopWorm ();
}

int Gemini::setTo (double set_ra, double set_dec, int appendModel)
{
	char readback[101];

	normalizeRaDec (set_ra, set_dec);

	if ((tel_write_ra (set_ra) < 0) || (tel_write_dec (set_dec) < 0))
		return -1;

	if (appendModel)
	{
		if (tel_write_read_hash (":Cm#", 4, readback, 100) < 0)
			return -1;
	}
	else
	{
		int32_t v205;
		int32_t v206;
		int32_t v205_new;
		int32_t v206_new;
		tel_gemini_get (205, v205);
		tel_gemini_get (206, v206);
		if (tel_write_read_hash (":CM#", 4, readback, 100) < 0)
			return -1;
		tel_gemini_get (205, v205_new);
		tel_gemini_get (206, v206_new);
	}
	return 0;
}

int Gemini::setTo (double set_ra, double set_dec)
{
	return setTo (set_ra, set_dec, 0);
}

int Gemini::correctOffsets (double cor_ra, double cor_dec, double real_ra, double real_dec)
{
	int32_t v205;
	int32_t v206;
	int ret;
	// change allign parameters..
	tel_gemini_get (205, v205);
	tel_gemini_get (206, v206);
								 // convert from degrees to arcminutes
	v205 += (int) (cor_ra * 3600);
	v206 += (int) (cor_dec * 3600);
	tel_gemini_set (205, v205);
	ret = tel_gemini_set (206, v206);
	if (ret)
		return -1;
	return 0;
}

#ifdef L4_GUIDE
bool Gemini::isGuiding (struct timeval * now)
{
	char guiding;
	tel_write_read (":Gv#", 4, &guiding, 1);
	if (guiding == 'G')
		guideDetected = true;
	if ((guideDetected && guiding != 'G') || (!guideDetected && timercmp (&changeTime, now, <)))
		return false;
	return true;
}

int Gemini::guide (char direction, unsigned int val)
{
	// convert to arcsec
	char buf[20];
	int len;
	int ret;
	len = sprintf (buf, ":Mi%c%i#", direction, val);
	ret = tel_conn->writePort (buf, len);
	usleep (USEC_SEC / 15);
	return ret;
}

int Gemini::changeDec ()
{
	char direction;
	struct timeval chng_time;
	int ret;
	direction = nextChangeDec > 0 ? DIR_NORTH : DIR_SOUTH;
	ret = guide (direction,
		(unsigned int) (fabs (nextChangeDec) * 255.0 /
		maxPrecGuideDec));
	if (ret)
	{
		nextChangeDec = 0;
		return ret;
	}
	decChanged = true;
	chng_time.tv_sec = (int) (ceil (fabs (nextChangeDec) * 240.0));
	gettimeofday (&changeTime, NULL);
	timeradd (&changeTime, &chng_time, &changeTime);
	guideDetected = false;
	nextChangeDec = 0;
	return 0;
}

int Gemini::change_real (double chng_ra, double chng_dec)
{
	char direction;
	struct timeval chng_time;
	int ret = 0;

	// smaller then 30 arcsec
	if (chng_dec < 60.0 / 3600.0 && chng_ra < 60.0 / 3600.0)
	{
		// set smallest rate..
		tel_gemini_set (GEMINI_CMD_RATE_GUIDE, 0.2);
		if (chng_ra < 30.0 / 3600.0)
			chng_ra = 0;
	}
	else
	{
		tel_gemini_set (GEMINI_CMD_RATE_GUIDE, guidingSpeed->getValueFloat ());
		if (!getFlip ())
		{
			chng_dec *= -1;
		}
		chng_ra *= -1;
		logStream (MESSAGE_DEBUG) << "after getFlipI chng_ra " << chng_ra << sendLog;
		if (chng_ra != 0)
		{
			// first - RA direction
			// slew speed to 1 - 0.25 arcmin / sec
			if (chng_ra > 0)
			{
				direction = DIR_EAST;
			}
			else
			{
				direction = DIR_WEST;
			}
			ret =
				guide (direction,
				(unsigned int) (fabs (chng_ra) * 255.0 / maxPrecGuideRa));
			if (ret)
				return ret;

								 // * 0.8 * some constand, but that will left enought margin..
			chng_time.tv_sec = (int) (ceil (fabs (chng_ra) * 240.0));
			nextChangeDec = chng_dec;
			gettimeofday (&changeTime, NULL);
			timeradd (&changeTime, &chng_time, &changeTime);
			guideDetected = false;
			decChanged = false;
			return 0;
		}
		else if (chng_dec != 0)
		{
			nextChangeDec = chng_dec;
			// don't update guide,,
			// that will be chandled in changeDec
			return 0;
		}
		return -1;
	}
	return 0;
}

#else

int Gemini::change_ra (double chng_ra)
{
	char direction;
	long u_sleep;
	struct timeval now;
	int ret;
	// slew speed to 1 - 0.25 arcmin / sec
	direction = (chng_ra > 0) ? DIR_EAST : DIR_WEST;
	if (!isnan (fixed_ha))
	{
		if (fabs (chng_ra) > guideLimit->getValueDouble ())
			centeringSpeed->setValueInteger (20);
		else
			centeringSpeed->setValueInteger (2);
		// divide by sidereal speed we will use..
		u_sleep =
			(long) (((fabs (chng_ra) * 60.0) *
			(4.0 / ((float) centeringSpeed->getValueInteger ()))) *
			USEC_SEC);
	}
	else
	{
		u_sleep =
			(long) (((fabs (chng_ra) * 60.0) *
			(4.0 / guidingSpeed->getValueFloat ())) * USEC_SEC);
	}
	changeTimeRa.tv_sec = (long) (u_sleep / USEC_SEC);
	changeTimeRa.tv_usec = (long) (u_sleep - changeTimeRa.tv_sec);
	if (!isnan (fixed_ha))
	{
		if (direction == DIR_EAST)
		{
			ret = tel_set_rate (RATE_CENTER);
			if (ret)
				return ret;
			ret =
				tel_gemini_set (GEMINI_CMD_RATE_CENTER,
				centeringSpeed->getValueInteger ());
			if (ret)
				return ret;
			worm_move_needed = 1;
			ret = telescope_start_move (direction);
		}
		// west..only turn worm on
		else
		{
			ret = tel_set_rate (RATE_CENTER);
			if (ret)
				return ret;
			ret =
				tel_gemini_set (GEMINI_CMD_RATE_CENTER,
				centeringSpeed->getValueInteger ());
			if (ret)
				return ret;
			worm_move_needed = 1;
			ret = telescope_start_move (direction);
		}
	}
	else
	{
		ret = tel_set_rate (RATE_GUIDE);
		if (ret)
			return ret;
		ret =
			tel_gemini_set (GEMINI_CMD_RATE_GUIDE,
			guidingSpeed->getValueFloat ());
		if (ret)
			return ret;
		ret = telescope_start_move (direction);
	}
	gettimeofday (&now, NULL);
	timeradd (&changeTimeRa, &now, &changeTimeRa);
	return ret;
}

int Gemini::change_dec (double chng_dec)
{
	char direction;
	long u_sleep;
	struct timeval now;
	int ret;
	// slew speed to 20 - 5 arcmin / sec
	direction = chng_dec > 0 ? DIR_NORTH : DIR_SOUTH;
	if (fabs (chng_dec) > guideLimit->getValueDouble ())
	{
		centeringSpeed->setValueInteger (20);
		ret = tel_set_rate (RATE_CENTER);
		if (ret == -1)
			return ret;
		ret =
			tel_gemini_set (GEMINI_CMD_RATE_CENTER,
			centeringSpeed->getValueInteger ());
		if (ret)
			return ret;
		u_sleep =
			(long) ((fabs (chng_dec) * 60.0) *
			(4.0 / ((float) centeringSpeed->getValueInteger ())) *
			USEC_SEC);
	}
	else
	{
		ret = tel_set_rate (RATE_GUIDE);
		if (ret == -1)
			return ret;
		ret =
			tel_gemini_set (GEMINI_CMD_RATE_GUIDE,
			guidingSpeed->getValueFloat ());
		if (ret)
			return ret;
		u_sleep =
			(long) ((fabs (chng_dec) * 60.0) *
			(4.0 / guidingSpeed->getValueFloat ()) * USEC_SEC);
	}
	changeTimeDec.tv_sec = (long) (u_sleep / USEC_SEC);
	changeTimeDec.tv_usec = (long) (u_sleep - changeTimeDec.tv_sec);
	ret = telescope_start_move (direction);
	if (ret)
		return ret;
	gettimeofday (&now, NULL);
	timeradd (&changeTimeDec, &now, &changeTimeDec);
	return ret;
}

int Gemini::change_real (double chng_ra, double chng_dec)
{
	int ret;
	// center rate
	nextChangeDec = 0;
	if (!getFlip ())
	{
		chng_dec *= -1;
	}
	logStream (MESSAGE_DEBUG) << "change_real  " << chng_ra << " "  << chng_dec << sendLog;
	if (chng_ra != 0)
	{
		ret = change_ra (chng_ra);
		nextChangeDec = chng_dec;
		return ret;
	}
	else if (chng_dec != 0)
	{
		return change_dec (chng_dec);
	}
	return 0;
}
#endif

int Gemini::startPark ()
{
	int ret;
	fixed_ha = NAN;
	if (telMotorState != TEL_OK)
		return -1;
	stopMove ();
	ret = tel_conn->writePort (":hP#", 4);
	usleep (USEC_SEC / 15);
	if (ret < 0)
		return -1;
	sleep (1);
	return 0;
}

static int save_registers[] =
{
	0,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,							 // mount type
	120,						 // manual slewing speed
	140,						 // goto slewing speed
	GEMINI_CMD_RATE_GUIDE,		 // guiding speed
	GEMINI_CMD_RATE_CENTER,		 // centering speed
	200,						 // TVC stepout
	201,						 // modeling parameter A (polar axis misalignment in azimuth) in seconds of arc
	202,						 // modeling parameter E (polar axis misalignment in elevation) in seconds of arc
	203,						 // modeling parameter NP (axes non-perpendiculiraty at the pole) in seconds of arc
	204,						 // modeling parameter NE (axes non-perpendiculiraty at the Equator) in seconds of arc
	205,						 // modeling parameter IH (index error in hour angle) in seconds of arc
	206,						 // modeling parameter ID (index error in declination) in seconds of arc
	207,						 // modeling parameter FR (mirror flop/gear play in RE) in seconds of arc
	208,						 // modeling parameter FD (mirror flop/gear play in declination) in seconds of arc
	209,						 // modeling paremeter CF (counterweight & RA axis flexure) in seconds of arc
	211,						 // modeling parameter TF (tube flexure) in seconds of arc
	221,						 // ra limit 1
	222,						 // ra limit 2
	223,						 // ra limit 3
	411,						 // RA track divisor
	412,						 // DEC tracking divisor
	-1
};

int Gemini::saveModel ()
{
	int *reg = save_registers;
	char buf[20];
	FILE *config_file;
	config_file = fopen (geminiConfig, "w");
	if (!config_file)
	{
		logStream (MESSAGE_ERROR) << "Gemini saveModel cannot open file " <<
			geminiConfig << sendLog;
		return -1;
	}

	while (*reg >= 0)
	{
		int ret;
		ret = tel_gemini_getch (*reg, buf);
		if (ret)
		{
			fprintf (config_file, "%i:error", *reg);
		}
		else
		{
			fprintf (config_file, "%i:%s\n", *reg, buf);
		}
		reg++;
	}
	fclose (config_file);
	return 0;
}

extern int Gemini::loadModel ()
{
	FILE *config_file;
	char *line;
	size_t numchar;
	int id;
	int ret;
	config_file = fopen (geminiConfig, "r");
	if (!config_file)
	{
		logStream (MESSAGE_ERROR) << "Gemini loadModel cannot open file " <<
			geminiConfig << sendLog;
		return -1;
	}

	numchar = 100;
	line = (char *) malloc (numchar);
	while (getline (&line, &numchar, config_file) != -1)
	{
		char *buf;
		ret = sscanf (line, "%i:%as", &id, &buf);
		if (ret == 2)
		{
			ret = tel_gemini_setch (id, buf);
			if (ret)
				logStream (MESSAGE_ERROR) << "Gemini loadModel setch return " <<
					ret << " on " << buf << sendLog;

			free (buf);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "Gemini loadModel invalid line " <<
				line << sendLog;
		}
	}
	free (line);
	return 0;
}

int Gemini::stopWorm ()
{
	worm = 135;
	return tel_gemini_set (135, 1);
}

int Gemini::startWorm ()
{
	worm = 131;
	return tel_gemini_set (131, 1);
}

int Gemini::parkBootesSensors ()
{
	int ret;
	char direction;
	time_t timeout;
	time_t now;
	double old_tel_axis;
	ret = info ();
	startWorm ();
	// first park in RA
	old_tel_axis = featurePort->getValueInteger () & 1;
	direction = old_tel_axis ? DIR_EAST : DIR_WEST;
	time (&timeout);
	now = timeout;
	timeout += 20;
	ret = tel_set_rate (RATE_SLEW);
	if (ret)
		return ret;
	ret = telescope_start_move (direction);
	if (ret)
	{
		telescope_stop_move (direction);
		return ret;
	}
	while ((featurePort->getValueInteger () & 1) == old_tel_axis && now < timeout)
	{
		getAxis ();
		time (&now);
	}
	telescope_stop_move (direction);
	// then in dec
	//
	old_tel_axis = featurePort->getValueInteger () & 2;
	direction = old_tel_axis ? DIR_NORTH : DIR_SOUTH;
	time (&timeout);
	now = timeout;
	timeout += 20;
	ret = telescope_start_move (direction);
	if (ret)
	{
		telescope_stop_move (direction);
		return ret;
	}
	while ((featurePort->getValueInteger () & 2) == old_tel_axis && now < timeout)
	{
		getAxis ();
		time (&now);
	}
	telescope_stop_move (direction);
	tel_gemini_set (205, 0);
	tel_gemini_set (206, 0);
	return 0;
}

int Gemini::resetMount ()
{
	int ret;
	if ((resetState->getValueInteger () == 1 || resetState->getValueInteger () == 2)
		&& bootesSensors)
	{
		// park using optical things
		parkBootesSensors ();
	}
	ret = tel_gemini_set (65535, 65535);
	if (ret)
		return ret;
	return Telescope::resetMount ();
}

int Gemini::getFlip ()
{
	int32_t raTick, decTick;
	int ret;
	if (gem_version < 4)
	{
		double ha;
		ha = ln_range_degrees (getLocSidTime () * 15.0 - getTelRa ());

		return ha < 180.0 ? 1 : 0;
	}
	ret = tel_gemini_get (235, raTick, decTick);
	if (ret)
		return -1;
	axRa->setValueDouble (raTick);
	axDec->setValueDouble (decTick);
	if (decTick > decFlipLimit)
		return 1;
	return 0;
}

int main (int argc, char **argv)
{
	Gemini device = Gemini (argc, argv);
	return device.run ();
}

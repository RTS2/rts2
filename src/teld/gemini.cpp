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

#include "rts2lx200/tellx200.h"
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
#define OPT_EVENINGRESET         OPT_LOCAL + 15
#define OPT_GEMINIMODELFILE      OPT_LOCAL + 16


namespace rts2teld
{

class Gemini:public TelLX200
{
	public:
		Gemini (int argc, char **argv);
		virtual ~ Gemini (void);
		virtual int initHardware ();
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
		virtual int setTracking (int track, bool addTrackingTimer = false, bool send = true);
		int stopWorm ();
		int startWorm ();
		virtual int resetMount ();
		virtual int getFlip ();

	protected:
		virtual int processOption (int in_opt);
		virtual int initValues ();
		virtual int idle ();

	private:
		const char *device_file;

		rts2core::ValueFloat *telGuidingSpeed;

		rts2core::ValueSelection *resetState;
		rts2core::ValueInteger *featurePort;

		rts2core::ValueDouble *axRa;
		rts2core::ValueDouble *axDec;

		rts2core::ValueDouble *haLimitE;
		rts2core::ValueDouble *haLimitW;
		rts2core::ValueDouble *haLimitW2;

		const char *geminiConfig;

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
		 * Computes gemini checksum
		 *
		 * @param buf checksum to compute
		 *
		 * @return computed checksum
		 */
		unsigned char tel_gemini_checksum (const char *buf);

		/**
		 * Computes gemini checksum - this variant is because of buggy firmware in gemini, which responses with checksum, computed with wrong combination of operations in algorithm, but still expects the right algorithm on input.
		 *
		 * @param buf checksum to compute
		 *
		 * @return computed checksum
		 */
		unsigned char tel_gemini_checksum2 (const char *buf);

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
		int tel_gemini_setch (int id, const char *in_buf);

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
		 * Read gemini local parameter, transform degrees to double
		 *
		 * @param id	id to get
		 * @param val	pointer where to store result
		 *
		 * @return -1 and set errno on error, otherwise 0
		 */
		int tel_gemini_get_deg (int id, double &val);

		/**
		 * Reset and home losmandy telescope
		 */
		int tel_gemini_reset ();

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

		void tel_stop_goto ();
		int tel_start_move ();

		enum
		{ TEL_OK, TEL_BLOCKED_RESET, TEL_BLOCKED_PARKING }
		telMotorState;
		int32_t lastMotorState;
		time_t moveTimeout;

		int infoCount;
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

		rts2core::ValueInteger *centeringSpeed;
		rts2core::ValueFloat *guidingSpeed;
		rts2core::ValueDouble *guideLimit;
		rts2core::ValueBool *syncAppendModel;

		int expectedType;
		int forceType;
		bool forceLatLon;

		const char *getGemType (int gem_type);
		bool willFlip;

		bool eveningReset;
		bool parkAndReset;
};

}

using namespace rts2teld;

int Gemini::tel_write_read (const char *buf, int wcount, char *rbuf, int rcount)
{
	int ret;
	ret = serConn->writeRead (buf, wcount, rbuf, rcount);
	usleep (USEC_SEC / 15);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "tel_write_read: writeRead error, making tel_gemini_reset" << sendLog;
		// try rebooting
		tel_gemini_reset ();
		usleep (USEC_SEC / 5);
		ret = serConn->writeRead (buf, wcount, rbuf, rcount);
		usleep (USEC_SEC / 15);
	}
	return ret;
}

int Gemini::tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount)
{
	int tmp_rcount = serConn->writeRead (wbuf, wcount, rbuf, rcount, '#');
	usleep (USEC_SEC / 15);
	if (tmp_rcount < 0)
	{
		logStream (MESSAGE_ERROR) << "tel_write_read_hash: writeRead error, making tel_gemini_reset" << sendLog;
		tel_gemini_reset ();
		usleep (USEC_SEC / 5);
		return -1;
	}
	// end hash..
	rbuf[tmp_rcount - 1] = '\0';
	return tmp_rcount - 1;
}

int Gemini::readDate (struct tm *_tm, const char *command)
{
	char wbuf[11];
	int ret;
	_tm->tm_hour = _tm->tm_min = _tm->tm_sec = _tm->tm_isdst = 0;
	if (tel_write_read_hash (command, strlen (command), wbuf, 10) < 8)
	{
		logStream (MESSAGE_ERROR) << "readDate: short read length" << sendLog;
		return -1;
	}
	ret = sscanf (wbuf, "%u/%u/%u", &(_tm->tm_mon), &(_tm->tm_mday), &(_tm->tm_year));
	if (ret != 3)
	{
		logStream (MESSAGE_ERROR) << "readDate: invalid date read " << wbuf << " " << ret << sendLog;
		return -1;
	}
	// date will always be after 2000..
	_tm->tm_year += 100;
	_tm->tm_mon --;
	return 0;
}

unsigned char Gemini::tel_gemini_checksum (const char *buf)
{
	unsigned char checksum = 0;
	for (; *buf; buf++)
		checksum ^= *buf;
	checksum %= 128;			 // modulo 128
	checksum += 64;
	return checksum;
}

unsigned char Gemini::tel_gemini_checksum2 (const char *buf)
{
	unsigned char checksum = 0;
	for (; *buf; buf++)
		checksum ^= *buf;
	checksum += 64;
	checksum %= 128;			 // modulo 128
	return checksum;
}

int Gemini::tel_gemini_setch (int id, const char *in_buf)
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
	ret = serConn->writePort (buf, len);
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
	ret = serConn->writePort (buf, len);
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
	ret = serConn->writePort (buf, len);
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
	#ifdef DEBUG_EXTRA_CHECKSUM
	logStream (MESSAGE_DEBUG) << "+++++++ read response: " << buf << ", length " << ret << " +++++" << sendLog;
	#endif
	if (ret < 0)
		return ret;
	ptr = buf + ret - 1;
	checksum = *ptr;
	*ptr = '\0';
	cal = tel_gemini_checksum2 (buf);
	#ifdef DEBUG_EXTRA_CHECKSUM
	logStream (MESSAGE_DEBUG) << "+++++++ after checksum cut: " << buf << ", checksum " << checksum << " +++++" << sendLog;
	#endif
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
	for (ptr = buf; *ptr && (isdigit (*ptr) || *ptr == '.' || *ptr == '-'); ptr++);
	checksum = *ptr;
	*ptr = '\0';
	if (tel_gemini_checksum2 (buf) != checksum)
	{
		logStream (MESSAGE_ERROR) << "invalid gemini checksum: should be " <<
			tel_gemini_checksum2 (buf) << " is " << checksum << sendLog;
		sleep (5);
		serConn->flushPortIO ();
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
	for (ptr = buf; *ptr && (isdigit (*ptr) || *ptr == '.' || *ptr == '-');	ptr++);
	checksum = *ptr;
	*ptr = '\0';
	if (tel_gemini_checksum2 (buf) != checksum)
	{
		logStream (MESSAGE_ERROR) << "invalid gemini checksum: should be " <<
			tel_gemini_checksum2 (buf) << " is " << checksum << sendLog;
		sleep (5);
		serConn->flushPortIO ();
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

int Gemini::tel_gemini_get_deg (int id, double &val)
{
	char buf[20];
	double val2;
	int ret;
	ret = tel_gemini_getch (id, buf);
	if (ret)
		return ret;
	// parse returned string
	val2 = hmstod (buf);
	if (isnan (val))
		return -1;
	val = val2;
	return 0;
}

int Gemini::tel_gemini_reset ()
{
	char rbuf[50];

	logStream (MESSAGE_INFO) << "making gemini reset (trying to make startup config)..." << sendLog;

	sleep (5);

	// write_read_hash
	if (serConn->flushPortIO () < 0)
	{
		logStream (MESSAGE_ERROR) << "flushPortIO error" << sendLog;
		return -1;
	}
	usleep (USEC_SEC / 15);

	if (serConn->writeRead ("\x06", 1, rbuf, 47, '#') < 0)
	{
		sleep (5);
		serConn->flushPortIO ();
		usleep (USEC_SEC / 15);
		return -1;
	}
	usleep (USEC_SEC / 15);

	if (*rbuf == 'b')			 // booting phase, select restart/warm start/cold start
	{
		switch (resetState->getValueInteger ())
		{
			case 0:
				serConn->writePort ("bR#", 3);
				sleep (5);
				break;
			case 1:
				serConn->writePort ("bW#", 3);
				sleep (5);
				break;
			case 2:
				serConn->writePort ("bC#", 3);
				sleep (20);
				break;
		}
		resetState->setValueInteger (0);
		setTimeout (USEC_SEC);
	}
	else if (*rbuf != 'G')
	{
		// something is wrong, reset all comm
		logStream (MESSAGE_ERROR) << "system is not after reboot nor completed startup, making 10s sleep & flushPortIO" << sendLog;
		sleep (10);
		serConn->flushPortIO ();
		usleep (USEC_SEC / 15);
	}
	return -1;
}

Gemini::Gemini (int in_argc, char **in_argv):TelLX200 (in_argc, in_argv, false, true)
{
	createValue (telGuidingSpeed, "guiding_speed", "telescope guiding speed", false);

	createValue (resetState, "next_reset", "next reset state", false, RTS2_VALUE_WRITABLE);
	resetState->addSelVal ("RESTART");
	resetState->addSelVal ("WARM_START");
	resetState->addSelVal ("COLD_START");

	createValue (featurePort, "feature_port", "state of feature port", false);

	createValue (axRa, "CNT_RA", "RA axis ticks", true);
	createValue (axDec, "CNT_DEC", "DEC axis ticks", true);

	createValue (haLimitE, "HA_LIMIT_E", "eastern HA safety limit, distance from CWD position", false, RTS2_DT_DEG_DIST_180);
	createValue (haLimitW, "HA_LIMIT_W", "western HA safety limit, distance from CWD position", false, RTS2_DT_DEG_DIST_180);
	createValue (haLimitW2, "HA_LIMIT_W2", "western HA goto limit, distance from CWD position", false, RTS2_DT_DEG_DIST_180);
	haLimitE->setValueDouble (94.0);
	haLimitW->setValueDouble (97.0);
	haLimitW2->setValueDouble (2.5);

	device_file = "/dev/ttyS0";
	geminiConfig = "/etc/rts2/gemini.ini";
	bootesSensors = 0;
	expectedType = 0;
	forceType = 0;
	forceLatLon = false;

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttySx");
	addOption (OPT_GEMINIMODELFILE, "gemini-model", 1, "gemini config file (with internal model parameters)");
	addOption (OPT_BOOTES, "bootes", 0,
		"BOOTES G-11 (with 1/0 pointing sensor)");
	addOption (OPT_CORR, "corrections", 1,
		"level of correction done in Gemini - 0 none, 3 all");
	addOption (OPT_EXPTYPE, "expected-type", 1,
		"expected Gemini type (1 GM8, 2 G11, 3 HGM-200, 4 CI700, 5 Titan, 6 Titan50)");
	addOption (OPT_FORCETYPE, "force-type", 1,
		"force Gemini type (1 GM8, 2 G11, 3 HGM-200, 4 CI700, 5 Titan, 6 Titan50)");
	addOption (OPT_FORCELATLON, "force-latlon", 0, "set observing longitude and latitude from configuration file");
	addOption (OPT_EVENINGRESET, "evening-reset", 0, "park and reset mount at the beginning of every evening");

	lastMotorState = 0;
	telMotorState = TEL_OK;
	infoCount = 0;

	serConn = NULL;

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

	createValue (centeringSpeed, "centeringSpeed", "speed used for centering mount on target", false, RTS2_VALUE_WRITABLE);
	centeringSpeed->setValueInteger (2);

	createValue (guidingSpeed, "guidingSpeed", "speed used for guiding mount on target", false);
	guidingSpeed->setValueFloat (0.8);

	createValue (guideLimit, "guideLimit", "limit to to change between ", false);
	guideLimit->setValueDouble (5.0 / 60.0);

	createValue (syncAppendModel, "appendModel", "sync appends model", false, RTS2_VALUE_WRITABLE);
	syncAppendModel->setValueBool (false);

	connDebug = false;
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
		case OPT_GEMINIMODELFILE:
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
		case OPT_EVENINGRESET:
			eveningReset = true;
			break;
		default:
			return TelLX200::processOption (in_opt);
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
	int ret;
	double val;

	ret = tel_gemini_get_deg (221, val);
	if (ret)
		return -1;
	haLimitE->setValueDouble (val);

	ret = tel_gemini_get_deg (222, val);
	if (ret)
		return -1;
	haLimitW->setValueDouble (val);

	ret = tel_gemini_get_deg (223, val);
	if (ret)
		return -1;
	if (val == 0.0)
		haLimitW2->setValueDouble (2.5);
	else
		haLimitW2->setValueDouble (val);

	//TODO: we don't take care of "swapping" (as written in gemini manual) of these values on south hemisphere. This should be solved when needed, but it will also need tests etc.

	return 0;
}

int Gemini::setCorrection ()
{
	if (gem_version < 4)
		return 0;
	
	int ret = -1;
	if (calculateAberation () && calculatePrecession () && calculateRefraction ())
		ret = serConn->writePort (":p0#", 4);
	else if (!calculateAberation () && !calculatePrecession () && calculateRefraction ())	
		ret = serConn->writePort (":p1#", 4);
	else if (calculateAberation () && calculatePrecession () && !calculateRefraction ())
		ret = serConn->writePort (":p2#", 4);
	else if (!calculateAberation () && !calculatePrecession () && !calculateRefraction ())
		ret = serConn->writePort (":p3#", 4);
	usleep (USEC_SEC / 15);
	return ret;
}

int Gemini::initHardware ()
{
	int ret;

	serConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 90);
	serConn->setDebug (getDebug ());

	ret = serConn->init ();
	if (ret)
		return ret;

	while (1)
	{
		serConn->flushPortIO ();
		usleep (USEC_SEC / 15);

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

			serConn->writePort (":hW#", 4);
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

	if (tel_read_longitude () || usleep (USEC_SEC / 15) || tel_read_latitude () || usleep (USEC_SEC / 15))
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
	telAltitude->setValueDouble (500);

	telFlip->setValueInteger (0);

	return TelLX200::initValues ();
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
							tel_stop_goto ();
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
			stopTracking ();
			ret = -1;
		}
	}

	if (parkAndReset == true && (getState () & TEL_MASK_MOVING) == TEL_PARKED)
	{
		parkAndReset = false;
		resetMount ();
	}

	//sleep (5);
	return TelLX200::idle ();
}

void Gemini::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_EVENING:
			if ((eveningReset == true) && ((old_state & SERVERD_STATUS_MASK) == SERVERD_DAY))
			{
				parkAndReset = true;
				startPark ();
			}
			break;
	}

	return TelLX200::changeMasterState (old_state, new_state);
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
	if (tel_read_ra () || usleep (USEC_SEC / 15) || tel_read_dec () || usleep (USEC_SEC / 15) || tel_read_local_time () || usleep (USEC_SEC / 15))
		return -1;
	if (bootesSensors)
	{
		getAxis ();
	}
	else
	{
		telFlip->setValueInteger (getFlip ());
	}

	return TelLX200::info ();
}

void Gemini::tel_stop_goto ()
{
	tel_gemini_get (99, lastMotorState);
	serConn->writePort (":Q#", 3);
	usleep (USEC_SEC * 2.5);	// give time to stop mount... (#99 seems not to reflect real state of motors, checking it is meaningless)
	tel_gemini_get (99, lastMotorState);
	//if (lastMotorState & 8)
	//{
	//	lastMotorState &= ~8;
	//	tel_gemini_set (99, lastMotorState);
	//}
	worm_move_needed = 0;
}

int Gemini::tel_start_move ()
{
	char retstr;
	char buf[55];

	if ((tel_write_ra (lastMoveRa) < 0) || usleep (USEC_SEC / 15)
		|| (serConn->writePort (":ONtest#", 8) < 0) || usleep (USEC_SEC / 15)
		|| (tel_write_dec (lastMoveDec) < 0) || usleep (USEC_SEC / 15))
		return -1;
	if (willFlip)
	{
		willFlip = false;
		if (tel_write_read (":MM#", 4, &retstr, 1) < 0)
			return -1;
	}
	else
	{
		if (tel_write_read (":MS#", 4, &retstr, 1) < 0)
			return -1;
	}

	if (retstr == '0')
	{
		sleep (1);
		return 0;
	}
	// otherwise read reply..
	serConn->readPort (buf, 53, '#');
	usleep (USEC_SEC / 15);
	if (retstr == '3')			 // manual control..
		return 0;
	return -1;
}

int Gemini::startResync ()
{
	int newFlip = telFlip->getValueInteger ();
	willFlip = false;
	double ra_diff, ra_diff_flip;
	double dec_diff, dec_diff_flip;
	double ha, haActual;
	double JD;
	double locSidTimeDeg;

	struct ln_equ_posn pos;
	struct ln_equ_posn pos_flip[2];
	struct ln_equ_posn model_change_flip[2];

	bool flip_possible[2];
	double travel_distance = 360.0;
	int i;

	getTarget (&pos);

	normalizeRaDec (pos.ra, pos.dec);

	pos_flip[0] = pos;
	pos_flip[1] = pos;

	// second flip needs to be set to 180 - dec or 180 + dec, depending if the dec is > 0 or < 0
	// dec is in -90 .. +90 range, given it was normalized by normalizeRaDec call
	if (pos_flip[1].dec < 0)
		pos_flip[1].dec = -180 - pos_flip[1].dec;
	else
		pos_flip[1].dec = 180 - pos_flip[1].dec;

	JD = ln_get_julian_from_sys ();
	locSidTimeDeg = getLocSidTime () * 15.0;

	haActual = ln_range_degrees (locSidTimeDeg - getTelRa ());
	if (haActual > 180.0)
		haActual -= 360.0;

	// for both flips....
	for (i = 0; i <= 1; i++)
	{
		computeModel (&pos_flip[i], &model_change_flip[i], JD);
		applyCorrRaDec (&pos_flip[i]);
		// We don't take care of dec 90deg overflow, as it is not necessary here, we only want to measure distances and feasibility.
		// Also, we don't want to solve funny things like flip change because of model or corrections... But gemini can handle these values, hopefully, so we don't care :-).

		ha = ln_range_degrees (locSidTimeDeg - pos_flip[i].ra);
		if (ha > 180.0)
			ha -= 360.0;
		// check if current position is reachable with particular mount flip, testing HA limits...
		// TODO: this is now solved only for northern hemispere.
		if ( ((i == 0) && (ha > 90.0 - haLimitE->getValueDouble ()) && (ha < 90.0 + haLimitW->getValueDouble ())) \
		|| ((i == 1) && (ha > -90.0 - haLimitE->getValueDouble ()) && (ha < -90.0 + haLimitW->getValueDouble () - haLimitW2->getValueDouble ())) )
			flip_possible[i] = true;
		else
			flip_possible[i] = false;

		if (flip_possible[i] == true)
		{
			if (telFlip->getValueInteger () == i)	// the same flip
			{
				//ra_diff = pos.ra - getTelRa ();
				ra_diff = - ha + haActual;
				dec_diff = pos.dec - getTelDec ();
				if (MAX (fabs (ra_diff), fabs (dec_diff)) < travel_distance)
				{
					travel_distance = MAX (fabs (ra_diff), fabs (dec_diff));
					willFlip = false;
					newFlip = i;
				}
			}
			else	// oposite flip (flip the mount during slew)
			{
				if (i == 1)
				{
					//ra_diff_flip = pos.ra - getTelRa () - 180.0;
					ra_diff_flip = - ha + haActual - 180.0;
				}
				else
				{
					//ra_diff_flip = pos.ra - getTelRa () + 180.0;
					ra_diff_flip = - ha + haActual + 180.0;
				}
					
				if (telLatitude->getValueDouble () > 0)
					dec_diff_flip = (90.0 - getTelDec () + 90.0 - pos.dec);
				else
					dec_diff_flip = (-90.0 - getTelDec () - 90.0 - pos.dec);

				if (MAX (fabs (ra_diff_flip), fabs (dec_diff_flip)) < travel_distance)
				{
					travel_distance = MAX (fabs (ra_diff_flip), fabs (dec_diff_flip));
					willFlip = true;
					newFlip = i;
				}
			}
		}
	}

	logStream (MESSAGE_DEBUG) << "Losmandy start move: oldflip " << telFlip->getValueInteger () <<
		", flip_possible[0] " << flip_possible[0] << ", flip_possible[1] " << flip_possible[1] <<
		", ra_diff " << ra_diff << ", dec_diff " << dec_diff << ", ra_diff_flip " << ra_diff_flip <<
		", dec_diff_flip " << dec_diff_flip << ", travel_distance " << travel_distance <<
		", newFlip" << newFlip << ", willFlip " << willFlip << sendLog;

	// OK, we have just taken decision which flip to choose, now really do it...
	// hard way:
	// smart way:
	applyModelPrecomputed (&pos_flip[newFlip], &model_change_flip[newFlip], false);	// false - we have already made corrections

	ra_diff = ln_range_degrees (lastMoveRa - pos_flip[newFlip].ra);
	if (ra_diff > 180.0)
		ra_diff -= 360.0;

	dec_diff = lastMoveDec - pos_flip[newFlip].dec;

	lastMoveRa = pos_flip[newFlip].ra;
	lastMoveDec = pos_flip[newFlip].dec;

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

	struct ln_equ_posn telPos;
	getTelRaDec (&telPos);

	double sep = ln_get_angular_separation (&pos, &telPos);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Losmandy change sep " << sep << sendLog;
	#endif
	if (sep > 1)
	{
		tel_set_slew_rate (RATE_SLEW);
		usleep (USEC_SEC / 15);
		tel_gemini_set (GEMINI_CMD_RATE_CENTER, 8);
	}
	else
	{
		tel_set_slew_rate (RATE_CENTER);
		usleep (USEC_SEC / 15);
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
			startTracking ();
			// initiate dec change
			if (changeDec ())
				return -1;
			return USEC_SEC / 5;
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
				ret = tel_stop_slew_move (DIR_EAST);
				usleep (USEC_SEC / 15);
				if (ret == -1)
					return ret;
				ret = tel_stop_slew_move (DIR_WEST);
				usleep (USEC_SEC / 15);
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
				ret = tel_stop_slew_move (DIR_NORTH);
				usleep (USEC_SEC / 15);
				if (ret == -1)
					return ret;
				ret = tel_stop_slew_move (DIR_SOUTH);
				usleep (USEC_SEC / 15);
				if (ret == -1)
					return ret;
				timerclear (&changeTimeDec);
			}
		}
		if (changeTimeRa.tv_sec == 0 && changeTimeRa.tv_usec == 0
			&& changeTimeDec.tv_sec == 0 && changeTimeDec.tv_usec == 0)
			return -2;
		return USEC_SEC / 5;
	}
	#endif
	if (now.tv_sec > moveTimeout)
	{
		stopMove ();
		return -2;
	}
	if (lastMotorState & 8)
		return USEC_SEC / 5;
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
			startTracking ();
		decChanged = false;
		timerclear (&changeTime);
		return TelLX200::endMove ();
	}
	#endif
	tel_gemini_get (130, track);
	setTimeout (USEC_SEC);
	return TelLX200::endMove ();
}

int Gemini::stopMove ()
{
	tel_stop_goto ();
	#ifdef L4_GUIDE
	timerclear (&changeTime);
	#else
	timerclear (&changeTimeRa);
	timerclear (&changeTimeDec);
	#endif
	return 0;
}

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
	{
		matchTime ();
		usleep (USEC_SEC / 15);
	}
	setTimeout (USEC_SEC);
	return 0;
}

int Gemini::setTo (double set_ra, double set_dec, int appendModel)
{
	char readback[101];
	int32_t v205, v206, v205_new, v206_new;
	struct ln_equ_posn pos, model_change;
	double JD;

	normalizeRaDec (set_ra, set_dec);

	pos.ra = set_ra;
	pos.dec = set_dec;
	JD = ln_get_julian_from_sys ();

	zeroCorrRaDec ();
	applyModel (&pos, &model_change, JD);

	if ((tel_write_ra (pos.ra) < 0) || usleep (USEC_SEC / 15) || (tel_write_dec (pos.dec) < 0) || usleep (USEC_SEC / 15))
		return -1;

	if (appendModel)
	{
		logStream (MESSAGE_INFO) << "appending to model..." << sendLog;
		if (tel_write_read_hash (":Cm#", 4, readback, 100) < 0)
			return -1;
	}
	else
	{
		tel_gemini_get (205, v205);
		tel_gemini_get (206, v206);
		if (tel_write_read_hash (":CM#", 4, readback, 100) < 0)
			return -1;
		tel_gemini_get (205, v205_new);
		tel_gemini_get (206, v206_new);
	}
	logStream (MESSAGE_INFO) << "Gemini synchronized, (" << v205 << "," << v206 << ") -> (" << v205_new << "," << v206_new << ")..." << sendLog;
	return 0;
}

int Gemini::setTo (double set_ra, double set_dec)
{
	return setTo (set_ra, set_dec, syncAppendModel->getValueBool ());
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
	ret = serConn->writePort (buf, len);
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
			ret = tel_set_slew_rate (RATE_CENTER);
			usleep (USEC_SEC / 15);
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
			ret = tel_set_slew_rate (RATE_CENTER);
			usleep (USEC_SEC / 15);
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
		ret = tel_set_slew_rate (RATE_GUIDE);
		usleep (USEC_SEC / 15);
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
		ret = tel_set_slew_rate (RATE_CENTER);
		usleep (USEC_SEC / 15);
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
		ret = tel_set_slew_rate (RATE_GUIDE);
		usleep (USEC_SEC / 15);
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
	ret = serConn->writePort (":hP#", 4);
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
	int mount_type=0;
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
			#ifdef DEBUG_EXTRA_CHECKSUM
			logStream (MESSAGE_DEBUG) << "------- writing: id " << id << ", value " << buf << " -----" << sendLog;
			#endif

			if (id == 0)
			{
				mount_type = atoi (buf);
				ret = tel_gemini_setch (mount_type, "0");
				// TODO: here should be also somehow solved setting of parameters for different mounts, same as in init section. For now, I'll let it be, it is still only for emergency/problem purposes, the restart of teld daemon is easy and straightforward solution.
			}
			else if (id < 100 && mount_type == 0)	// custom mount type
				ret = tel_gemini_setch (id, buf);
			else if (id >= 100)
				ret = tel_gemini_setch (id, buf);
			else
				ret = 0;

			if (ret)
				logStream (MESSAGE_ERROR) << "Gemini loadModel setch return " << ret << " on " << buf << sendLog;

			free (buf);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "Gemini loadModel invalid line " << line << sendLog;
		}
	}
	free (line);
	usleep (USEC_SEC * 5);
	if (serConn->flushPortIO () < 0)
	{
		logStream (MESSAGE_ERROR) << "flushPortIO error" << sendLog;
		return -1;
	}
	return 0;
}


int Gemini::setTracking (int track, bool addTrackingTimer, bool send)
{
	int ret;

	if (track)
		ret = startWorm ();
	else
		ret = stopWorm ();

	if (ret)
		return ret;

	return TelLX200::setTracking (track, addTrackingTimer, send);
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
	startTracking ();
	// first park in RA
	old_tel_axis = featurePort->getValueInteger () & 1;
	direction = old_tel_axis ? DIR_EAST : DIR_WEST;
	time (&timeout);
	now = timeout;
	timeout += 20;
	ret = tel_set_slew_rate (RATE_SLEW);
	usleep (USEC_SEC / 15);
	if (ret)
		return ret;
	ret = tel_start_slew_move (direction);
	usleep (USEC_SEC / 15);
	if (ret)
	{
		tel_stop_slew_move (direction);
		usleep (USEC_SEC / 15);
		return ret;
	}
	while ((featurePort->getValueInteger () & 1) == old_tel_axis && now < timeout)
	{
		getAxis ();
		time (&now);
	}
	tel_stop_slew_move (direction);
	usleep (USEC_SEC / 15);
	// then in dec
	//
	old_tel_axis = featurePort->getValueInteger () & 2;
	direction = old_tel_axis ? DIR_NORTH : DIR_SOUTH;
	time (&timeout);
	now = timeout;
	timeout += 20;
	ret = tel_start_slew_move (direction);
	usleep (USEC_SEC / 15);
	if (ret)
	{
		tel_stop_slew_move (direction);
		usleep (USEC_SEC / 15);
		return ret;
	}
	while ((featurePort->getValueInteger () & 2) == old_tel_axis && now < timeout)
	{
		getAxis ();
		time (&now);
	}
	tel_stop_slew_move (direction);
	usleep (USEC_SEC / 15);
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
	logStream (MESSAGE_INFO) << "Starting gemini reboot (resetMount)." << sendLog;
	if (resetState->getValueInteger () == 2)
	{
		resetState->setValueInteger (0);
		ret = tel_gemini_set (65533, 0);
		// cold start needs really much more time to complete
		sleep (20);
	}
	else
		ret = tel_gemini_set (65535, 0);
	if (ret)
		return ret;
	sleep (20);
	tel_gemini_reset ();
	return TelLX200::resetMount ();
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

/*!
 * @file Driver for generic LX200 protocol based telescope.
 *
 * LX200 was based on original c library for XEphem writen by Ken
 * Shouse <ken@kshouse.engine.swri.edu> and modified by Carlos Guirao
 * <cguirao@eso.org>. But most of the code was completly
 * rewriten and you will barely find any piece from XEphem.
 *
 * @author john,petr
 */

#include "rts2lx200/tellx200.h"
#include "configuration.h"

#define RATE_FIND 'M'
#define DIR_NORTH 'n'
#define DIR_EAST  'e'
#define DIR_SOUTH 's'
#define DIR_WEST  'w'

#define OPT_MATCHTIME      OPT_LOCAL + 201
#define OPT_MATCHTIMEZONE  OPT_LOCAL + 202
#define OPT_SETTIMEZONE    OPT_LOCAL + 203

namespace rts2teld
{

class LX200:public TelLX200
{
	public:
		LX200 (int argc, char **argv);
		virtual ~ LX200 (void);

		virtual int initHardware ();
		virtual int initValues ();
		virtual int info ();

		virtual int setTo (double set_ra, double set_dec);
		virtual int correct (double cor_ra, double cor_dec, double real_ra, double real_dec);

		virtual int startResync ();
		virtual int isMoving ();
		virtual int stopMove ();

		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();

		virtual int startDir (char *dir);
		virtual int stopDir (char *dir);

	protected:
		virtual void usage ();

		virtual int processOption (int in_opt);

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int commandAuthorized (rts2core::Connection *conn);

	private:
		int motors;

		time_t move_timeout;

		int tel_set_rate (char new_rate);
		int tel_slew_to (double ra, double dec);

		int tel_check_coords (double ra, double dec);

		void set_move_timeout (time_t plus_time);

		rts2core::ValueTime *timeZone;

		rts2core::ValueString *productName;
		rts2core::ValueString *mntflip;

		bool autoMatchTime;
		bool autoMatchTimeZone;
		float defaultTimeZone;
		bool hasAstroPhysicsExtensions;

		int matchTime ();

		int matchTimeZone ();

		int setTimeZone (float offset);
		int getTimeZone ();
};

};

using namespace rts2teld;

LX200::LX200 (int in_argc, char **in_argv):TelLX200 (in_argc, in_argv)
{
	motors = 0;
	autoMatchTime = false;
	autoMatchTimeZone = false;
	defaultTimeZone = NAN;
	hasAstroPhysicsExtensions = false;

	createValue (productName, "product_name", "reported product name", false);
	createValue (mntflip, "flip", "telescope flip");
	createValue (timeZone, "timezone", "telescope local time offset", false, RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL);

	addOption (OPT_MATCHTIME, "match-time", 0, "match telescope clocks to local time");
	addOption (OPT_MATCHTIMEZONE, "match-timezone", 0, "match telescope timezone with local timezone");
	addOption (OPT_SETTIMEZONE, "set-timezone", 1, "set telescope timezone to the provided value");
}


LX200::~LX200 (void)
{
}

int LX200::initHardware ()
{
	int ret = TelLX200::initHardware ();
	if (ret)
		return ret;

	int ovtime = serConn->getVTime ();

	serConn->setVTime (100);

	char rbuf[100];
	// we get 12:34:4# while we're in short mode
	// and 12:34:45 while we're in long mode
	if (serConn->writeRead ("#:Gr#", 5, rbuf, 9, '#') < 0)
		return -1;

	if (rbuf[7] == '\0' || rbuf[7] == '#')
	{
		// that could be used to distinguish between long
		// short mode
		// we are in short mode, set the long on
		if (serConn->writeRead ("#:U#", 5, rbuf, 0) < 0)
			return -1;
		
		if (serConn->writeRead ("#:Gr#", 5, rbuf, 9, '#') < 0)
			return -1;
		if (rbuf[7] == '\0' || rbuf[7] == '#')
		{
			std::cerr << "cannot switch LX200 communication to long format" << std::endl;
			return -1;
		}
	}

	// get product name
	ret = serConn->writeRead (":GVP#", 5, rbuf, 99, '#');
	if (ret < 0)
		return -1;
	if (ret > 0)
		rbuf[ret - 1] = '\0';
	else
		rbuf[0] = '\0';

	productName->setValueCharArr (rbuf);

	if (strncmp (productName->getValue (), "10micron", 8) == 0)
		hasAstroPhysicsExtensions = true;

	serConn->setVTime (ovtime);

	if (autoMatchTime)
	{
		ret = matchTime ();
		if (ret)
			return ret;
	}

	if (!isnan (defaultTimeZone))
	{
		ret = setTimeZone (defaultTimeZone);
		if (ret)
			return ret;
	}
	else if (autoMatchTimeZone)
	{
		ret = matchTimeZone ();
		if (ret)
			return ret;
	}

	getTimeZone ();

	return 0;
}

/*!
 * Reads information about telescope.
 *
 */
int LX200::initValues ()
{
	int ret = -1 ;

        rts2core::Configuration *config = rts2core::Configuration::instance ();
        ret = config->loadFile ();
        if (ret)
	  return -1;

        telLongitude->setValueDouble (config->getObserver ()->lng);
        telLatitude->setValueDouble (config->getObserver ()->lat);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	if (tel_read_longitude () || tel_read_latitude ())
		return -1;

	strcpy (telType, "LX200");
	telAltitude->setValueDouble (600);

	telFlip->setValueInteger (0);

	return Telescope::initValues ();
}

int LX200::info ()
{
	if (tel_read_ra () || tel_read_dec () || tel_read_local_time ())
		return -1;
	
	char rbuff[100];	
	int ret = serConn->writeRead (":pS#", 4, rbuff, 99, '#');
	if (ret < 0)
		return -1;
	if (ret > 0)
		rbuff[ret] = '\0';
	else
		rbuff[0] = '\0';
        
        mntflip->setValueCharArr (rbuff);

	telFlip->setValueInteger (strcmp (rbuff, "East#") == 0);

	return Telescope::info ();
}

void LX200::usage ()
{
	std::cout << "   LX200 compatible telescope driver. You probaly should provide -f options to specify serial port:" << std::endl
		<< "\t" << getAppName () << " -f /dev/ttyS3" << std::endl;
}

int LX200::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_MATCHTIME:
			autoMatchTime = true;
			return 0;
		case OPT_MATCHTIMEZONE:
			autoMatchTimeZone = true;
			return 0;
		case OPT_SETTIMEZONE:
			defaultTimeZone = strtof (optarg, NULL);
			return 0;
		default:
			return TelLX200::processOption (in_opt);
	}
	return 0;
}

int LX200::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == timeZone)
	{
		return setTimeZone (newValue->getValueFloat () / 3600.0) ? -2 : 0;
	}
	return TelLX200::setValue (oldValue, newValue);
}

int LX200::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("matchtime"))
	{
		return matchTime () ? -2 : 0;
	}
	return TelLX200::commandAuthorized (conn);
}

/*!
 * Set slew rate. For completness?
 *
 * This functions are there IMHO mainly for historical reasons. They
 * don't have any use, since if you start move and then die, telescope
 * will move forewer till it doesn't hurt itself. So it's quite
 * dangerous to use them for automatic observation. Better use quide
 * commands from attached CCD, since it defines timeout, which rules
 * CCD.
 *
 * @param new_rate	new rate to set. Uses RATE_<SPEED> constant.
 *
 * @return -1 on failure & set errno, 5 (>=0) otherwise
 */
int LX200::tel_set_rate (char new_rate)
{
	char command[6];
	sprintf (command, "#:R%c#", new_rate);
	return serConn->writePort (command, 5);
}

/*!
 * Slew (=set) LX200 to new coordinates.
 *
 * @param ra 		new right ascenation
 * @param dec 		new declination
 *
 * @return -1 on error, otherwise 0
 */
int LX200::tel_slew_to (double ra, double dec)
{
	char retstr;

	normalizeRaDec (ra, dec);

	if (tel_write_ra (ra) < 0 || tel_write_dec (dec) < 0)
		return -1;
	if (serConn->writeRead ("#:MS#", 5, &retstr, 1) < 0)
		return -1;
	if (retstr == '0')
		return 0;
	return -1;
}

/*!
 * Check, if telescope match given coordinates.
 *
 * @param ra		target right ascenation
 * @param dec		target declination
 *
 * @return -1 on error, 0 if not matched, 1 if matched, 2 if timeouted
 */
int LX200::tel_check_coords (double ra, double dec)
{
	// ADDED BY JF
	double JD;

	double sep;
	time_t now;

	struct ln_equ_posn object, target;
	struct ln_lnlat_posn observer;
	struct ln_hrz_posn hrz;

	time (&now);
	if (now > move_timeout)
		return 2;

	if ((tel_read_ra () < 0) || (tel_read_dec () < 0))
		return -1;

	// ADDED BY JF
	// CALCULATE & PRINT ALT/AZ & HOUR ANGLE TO LOG
	object.ra = getTelRa ();
	object.dec = getTelDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	JD = ln_get_julian_from_sys ();

	ln_get_hrz_from_equ (&object, &observer, JD, &hrz);

	logStream (MESSAGE_DEBUG) << "LX200 tel_check_coords TELESCOPE ALT " << hrz.
		alt << " AZ " << hrz.az << sendLog;

	target.ra = ra;
	target.dec = dec;

	sep = ln_get_angular_separation (&object, &target);

	if (sep > 0.1)
		return 0;

	return 1;
}

void LX200::set_move_timeout (time_t plus_time)
{
	time_t now;
	time (&now);

	move_timeout = now + plus_time;
}

int LX200::startResync ()
{
	if (hasAstroPhysicsExtensions)
		serConn->writePort (":PO#", 4);

	tel_slew_to (getTelTargetRa (), getTelTargetDec ());

	set_move_timeout (100);
	return 0;
}

int LX200::isMoving ()
{
	char buf[2];
	serConn->writeRead (":D#", 3, buf, 2, '#');
	switch (*buf)
	{
		case '#':
			return -2;
		default:
			return USEC_SEC;
	}
}

int LX200::stopMove ()
{
	char dirs[] = { 'e', 'w', 'n', 's' };
	int i;
	for (i = 0; i < 4; i++)
	{
		if (tel_stop_move (dirs[i]) < 0)
			return -1;
	}
	return 0;
}

/*!
 * Set telescope to match given coordinates
 *
 * This function is mainly used to tell the telescope, where it
 * actually is at the beggining of observation (remember, that LX200
 * doesn't have absolute position sensors)
 *
 * @param ra		setting right ascennation
 * @param dec		setting declination
 *
 * @return -1 and set errno on error, otherwise 0
 */
int LX200::setTo (double ra, double dec)
{
	char readback[101];
	int ret;

	normalizeRaDec (ra, dec);

	if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
		return -1;
	if (serConn->writeRead ("#:CM#", 5, readback, 100, '#') < 0)
		return -1;
	// since we are carring operation critical for next movements of telescope,
	// we are obliged to check its correctness
	set_move_timeout (10);
	ret = tel_check_coords (ra, dec);
	return ret == 1;
}


/*!
 * Correct telescope coordinates.
 *
 * Used for closed loop coordinates correction based on astronometry
 * of obtained images.
 *
 * @param ra		ra correction
 * @param dec		dec correction
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int LX200::correct (double cor_ra, double cor_dec, double real_ra, double real_dec)
{
	if (setTo (real_ra, real_dec))
		return -1;
	return 0;
}

/*!
 * Park telescope to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
int LX200::startPark ()
{
	int ret = serConn->writePort (":KA#", 4);
	if (ret < 0)
		return -1;
	sleep (1);
	return 0;
}

int LX200::isParking ()
{
	char buf[2];
	serConn->writeRead (":D#", 3, buf, 2, '#');
	switch (*buf)
	{
		case '#':
			return -2;
		default:
			return USEC_SEC;
	}
}

int LX200::endPark ()
{
	int ret = serConn->writePort (":AL#", 4);
	if (ret < 0)
		return -1;
	sleep (1);
	return 0;
}

int LX200::startDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			tel_set_rate (RATE_FIND);
			return tel_start_move (*dir);
	}
	return -2;
}

int LX200::stopDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			return tel_stop_move (*dir);
	}
	return -2;
}

int LX200::matchTime ()
{
	struct tm ts;
	time_t t;
	char buf[55];
	int ret;
	char rep;

	int ovtime = serConn->getVTime ();
	serConn->setVTime (10);

	t = time (NULL);
	gmtime_r (&t, &ts);

	// set local zone to 0
	ret = serConn->writeRead (":SG+00.0#", 9, &rep, 1);
	if (ret < 0)
		return ret;
	if (rep != '1')
	{
		logStream (MESSAGE_ERROR) << "cannot set hour offset to 0. Reply was " << rep << sendLog;
		return -1;
	}
	snprintf (buf, 14, ":SL%02d:%02d:%02d#", ts.tm_hour, ts.tm_min, ts.tm_sec);
	ret = serConn->writeRead (buf, strlen (buf), &rep, 1);
	if (ret < 0)
		return ret;
	if (rep != '1')
	{
		logStream (MESSAGE_ERROR) << "cannot set time. Reply was " << rep << sendLog;
		return -1;
	}
	snprintf (buf, 14, ":SC%02d/%02d/%02d#", ts.tm_mon + 1, ts.tm_mday, ts.tm_year - 100);
	ret = serConn->writeRead (buf, strlen (buf), buf, 55, '#');
	if (ret < 0)
		return ret;
	if (*buf != '1')
	{
		logStream (MESSAGE_ERROR) << "cannot set date. Reply was " << *buf << sendLog;
		return -1;
	}
	usleep (USEC_SEC / 15);
	// read spaces
	ret = serConn->readPort (buf, 40, '#');
	if (ret <= 0)
	{
		return ret;
	}
	logStream (MESSAGE_INFO) << "matched telescope time to UT time" << sendLog;

	serConn->setVTime (ovtime);

	return 0;
}

int LX200::matchTimeZone ()
{
	tzset ();

	float offset = timezone / 3600.0 + daylight;
	return setTimeZone (offset);
}

int LX200::setTimeZone (float offset)
{
	char buf[11];
	char rep;

	snprintf (buf, 10, ":SG%+05.1f#", offset);
	int ret = serConn->writeRead (buf, 9, &rep, 1);
	if (ret < 0)
		return ret;
	if (rep != '1')
	{
		logStream (MESSAGE_ERROR) << "cannot set hour offset to 0. Reply was " << rep << sendLog;
		return -1;
	}

	// after setting timezone, we need to wait till the setting propagates through TCS
	sleep (1);

	return getTimeZone ();
}

int LX200::getTimeZone ()
{
	char buf[10];
	float offset;

	int ret = serConn->writeRead (":GG#", 4, buf, 9, '#');
	if (ret < 0)
		return ret;

	offset = strtof (buf, NULL);
	timeZone->setValueDouble (offset * 3600);
	sendValueAll (timeZone);

	return 0;
}

int main (int argc, char **argv)
{
	LX200 device = LX200 (argc, argv);
	return device.run ();
}

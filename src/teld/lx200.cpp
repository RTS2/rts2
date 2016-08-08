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
#include "iostream"

using namespace std;

#define RATE_FIND 'M'
#define DIR_NORTH 'n'
#define DIR_EAST  'e'
#define DIR_SOUTH 's'
#define DIR_WEST  'w'

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

	private:
		int motors;

		time_t move_timeout;

		int tel_set_rate (char new_rate);
		int tel_slew_to (double ra, double dec);

		int tel_check_coords (double ra, double dec);

		void set_move_timeout (time_t plus_time);

		rts2core::ValueString *productName;
		rts2core::ValueString *mntflip;

		bool hasAstroPhysicsExtensions;
};

};

using namespace rts2teld;

LX200::LX200 (int in_argc, char **in_argv):TelLX200 (in_argc, in_argv)
{
	motors = 0;
	hasAstroPhysicsExtensions = false;

	createValue (productName, "product_name", "reported product name", false);
	createValue (mntflip, "flip", "telescope flip");
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
	setTelAltitude (config->getObservatoryAltitude ());

	if (tel_read_longitude () || tel_read_latitude ())
		return -1;

	telFlip->setValueInteger (0);

	return Telescope::initValues ();
}

int LX200::info ()
{
	if (tel_read_ra () || tel_read_dec () || tel_read_local_time ())
		return -1;
	
	//char rbuff[100];	
	//int ret = serConn->writeRead (":hS#", 4, rbuff, 99, '#');
	//int ret = serConn->writeRead (":pS#", 4, rbuff, 99, '#');
	//cout << "ret " << ret << endl;
	//	if (ret < 0)
	//	return -1;
	//if (ret > 0)
	//	rbuff[ret] = '\0';
	//else
	//	rbuff[0] = '\0';
	
        //mntflip->setValueCharArr (rbuff);

	//telFlip->setValueInteger (strcmp (rbuff, "East#") == 0);

	return Telescope::info ();
}

void LX200::usage ()
{
	std::cout << "   LX200 compatible telescope driver. You probaly should provide -f options to specify serial port:" << std::endl
		<< "\t" << getAppName () << " -f /dev/ttyS3" << std::endl;
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
		if (tel_stop_slew_move (dirs[i]) < 0)
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
  //int ret = serConn->writePort (":KA#", 4);
  int ret = serConn->writePort (":hP#", 4);
  if (ret < 0)
    return -1;
  sleep (1);
  return 0;
}

int LX200::isParking ()
{
	char buf[2];
	//serConn->writeRead (":D#", 3, buf, 2, '#');
	//serConn->writeRead (":h?#", 3, buf, 2, '#');
	serConn->writeRead (":h?#", 4, buf, 1);

	switch (*buf)
	{
	case '1':
	  return -2;
        case '0':
	  return -1;
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
			return tel_start_slew_move (*dir);
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
			return tel_stop_slew_move (*dir);
	}
	return -2;
}

int main (int argc, char **argv)
{
	LX200 device = LX200 (argc, argv);
	return device.run ();
}

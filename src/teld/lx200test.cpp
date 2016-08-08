#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*!
 * @file Test driver for Meade generic LX200 protocol based telescope.
 * 
 * This driver is used to test cuppola driver of Observatory Vermes
 * and the pier collision library.
 * Changed methods: isMoving, startResync to avoid the move to
 * HOME_RA, HOME_DEC.
 * 
 * LX200 was based on original c library for XEphem writen by Ken
 * Shouse <ken@kshouse.engine.swri.edu> and modified by Carlos Guirao
 * <cguirao@eso.org>. But most of the code was completly
 * rewriten and you will barely find any piece from XEphem.
 *
 * @author Markus Wildi based on john,petr
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <libnova/libnova.h>
#include <sys/ioctl.h>

#include "teld.h"
#include "rts2lx200/hms.h"
#include "status.h"
#include "configuration.h"
#include "clicupola.h"
#include "rts2lx200/pier-collision.h"

#include <termios.h>
// uncomment following line, if you want all tel_desc read logging (will
// at about 10 30-bytes lines to syslog for every query).
// #define DEBUG_ALL_PORT_COMM
//#define DEBUG

#define RATE_SLEW 'S'
#define RATE_FIND 'M'
#define RATE_CENTER 'C'
#define RATE_GUIDE  'G'
#define DIR_NORTH 'n'
#define DIR_EAST  'e'
#define DIR_SOUTH 's'
#define DIR_WEST  'w'
#define PORT_TIMEOUT  5

#define IS_WEST 0
#define IS_EAST 1

namespace rts2teld
{

class LX200TEST:public Telescope
{
	private:
		const char *device_file;
		int tel_desc;
		int motors;

		double lastMoveRa, lastMoveDec;

		enum
		  { NOTMOVE, MOVE_REAL }
		move_state;
		time_t move_timeout;

		// low-level functions..
		int tel_read (char *buf, int count);
		int tel_read_hash (char *buf, int count);
		int tel_write (const char *buf, int count);
		int tel_write_read (const char *wbuf, int wcount, char *rbuf, int rcount);
		int tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount);
		int tel_read_hms (double *hmsptr, const char *command);

		int tel_read_ra ();
		int tel_read_dec ();
		int tel_read_latitude ();
		int tel_read_longtitude ();
		int tel_rep_write (char *command);

		int tel_write_ra (double ra);
		int tel_write_dec (double dec);

		int tel_set_rate (char new_rate);
		int telescope_start_move (char direction);
		int telescope_stop_move (char direction);

		int tel_slew_to (double ra, double dec);

		int tel_check_coords (double ra, double dec);

		void set_move_timeout (time_t plus_time);

                rts2core::ValueInteger  *DECaxis_HAcoordinate ; // see pier_collision.c 

	public:
		LX200TEST (int argc, char **argv);
		virtual ~ LX200TEST (void);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();

		virtual int setTo (double set_ra, double set_dec);
		virtual int correct (double cor_ra, double cor_dec, double real_ra,
			double real_dec);

		virtual int startResync ();
                virtual int isMoving ();
		virtual int stopMove ();

		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();

		virtual int startDir (char *dir);
		virtual int stopDir (char *dir);
};

};

using namespace rts2teld;

/*!
 * Reads some data directly from tel_desc.
 *
 * Log all flow as LOG_DEBUG to syslog
 *
 * @exception EIO when there aren't data from tel_desc
 *
 * @param buf 		buffer to read in data
 * @param count 	how much data will be readed
 *
 * @return -1 on failure, otherwise number of read data
 */
int
LX200TEST::tel_read (char *buf, int count)
{
	int readed;

	for (readed = 0; readed < count; readed++)
	{
		int ret = read (tel_desc, &buf[readed], 1);
		if (ret == 0)
		{
			ret = -1;
		}
		if (ret < 0)
		{
			logStream (MESSAGE_DEBUG) << "LX200TEST tel_read: tel_desc read error "
				<< errno << sendLog;
			return -1;
		}
		#ifdef DEBUG_ALL_PORT_COMM
		logStream (MESSAGE_DEBUG) << "LX200TEST tel_read: readed " << buf[readed] <<
			sendLog;
		#endif
	}
	return readed;
}


/*!
 * Will read from tel_desc till it encoutered # character.
 *
 * Read ending #, but doesn't return it.
 *
 * @see tel_read() for description
 */
int
LX200TEST::tel_read_hash (char *buf, int count)
{
	int readed;
	buf[0] = 0;

	for (readed = 0; readed < count; readed++)
	{
		if (tel_read (&buf[readed], 1) < 0)
			return -1;
		if (buf[readed] == '#')
			break;
	}
	if (buf[readed] == '#')
		buf[readed] = 0;
	//	logStream (MESSAGE_DEBUG) << "LX200TEST tel_read_hash: Hash-readed: " << buf <<
	//	sendLog;
	return readed;
}


/*!
 * Will write on telescope tel_desc string.
 *
 * @exception EIO, .. common write exceptions
 *
 * @param buf 		buffer to write
 * @param count 	count to write
 *
 * @return -1 on failure, count otherwise
 */

int
LX200TEST::tel_write (const char *buf, int count)
{
  //logStream (MESSAGE_DEBUG) << "LX200TEST tel_write :will write: " << buf <<
  //		sendLog;
	return write (tel_desc, buf, count);
}


/*!
 * Combine write && read together.
 *
 * Flush tel_desc to clear any gargabe.
 *
 * @exception EINVAL and other exceptions
 *
 * @param wbuf		buffer to write on tel_desc
 * @param wcount	write count
 * @param rbuf		buffer to read from tel_desc
 * @param rcount	maximal number of characters to read
 *
 * @return -1 and set errno on failure, rcount otherwise
 */

int
LX200TEST::tel_write_read (const char *wbuf, int wcount, char *rbuf, int rcount)
{
	int tmp_rcount;
	char *buf;

	if (tcflush (tel_desc, TCIOFLUSH) < 0)
		return -1;
	if (tel_write (wbuf, wcount) < 0)
		return -1;

	tmp_rcount = tel_read (rbuf, rcount);
	if (tmp_rcount > 0)
	{
		buf = (char *) malloc (rcount + 1);
		memcpy (buf, rbuf, rcount);
		buf[rcount] = 0;
		//logStream (MESSAGE_DEBUG) << "LX200TEST tel_write_read: readed " <<
		//	tmp_rcount << " " << buf << sendLog;
		free (buf);
	}
	else
	{
	  //	logStream (MESSAGE_DEBUG) << "LX200TEST tel_write_read: readed returns " <<
	  //		tmp_rcount << sendLog;
	}

	return tmp_rcount;
}


/*!
 * Combine write && read_hash together.
 *
 * @see tel_write_read for definition
 */
int
LX200TEST::tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount)
{
	int tmp_rcount;

	if (tcflush (tel_desc, TCIOFLUSH) < 0)
		return -1;
	if (tel_write (wbuf, wcount) < 0)
		return -1;

	tmp_rcount = tel_read_hash (rbuf, rcount);

	return tmp_rcount;
}


/*!
 * Reads some value from LX200 in hms format.
 *
 * Utility function for all those read_ra and other.
 *
 * @param hmsptr	where hms will be stored
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
LX200TEST::tel_read_hms (double *hmsptr, const char *command)
{
	char wbuf[11];
	if (tel_write_read_hash (command, strlen (command), wbuf, 10) < 6)
		return -1;
	*hmsptr = hmstod (wbuf);
	if (errno)
		return -1;
	return 0;
}


/*!
 * Reads LX200 right ascenation.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
LX200TEST::tel_read_ra ()
{
	double new_ra;
	if (tel_read_hms (&new_ra, "#:GR#"))
		return -1;
	setTelRa (new_ra * 15.0);
	return 0;
}


/*!
 * Reads LX200 declination.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
LX200TEST::tel_read_dec ()
{
	double t_telDec;
	if (tel_read_hms (&t_telDec, "#:GD#"))
		return -1;
	setTelDec (t_telDec);
	return 0;
}


/*!
 * Reads LX200 latitude.
 *
 * @return -1 on error, otherwise 0
 *
 * MY EDIT LX200 latitude
 *
 * Hardcode latitude and return 0
 */
int
LX200TEST::tel_read_latitude ()
{
	return 0;
}


/*!
 * Reads LX200 longtitude.
 *
 * @return -1 on error, otherwise 0
 *
 * MY EDIT LX200 longtitude
 *
 * Hardcode longtitude and return 0
 */
int
LX200TEST::tel_read_longtitude ()
{
	return 0;
}


/*!
 * Repeat LX200 write.
 *
 * Handy for setting ra and dec.
 * Meade tends to have problems with that, don't know about LX200.
 *
 * @param command	command to write on tel_desc
 */
int
LX200TEST::tel_rep_write (char *command)
{
	int count;
	char retstr;
	for (count = 0; count < 200; count++)
	{
		if (tel_write_read (command, strlen (command), &retstr, 1) < 0)
			return -1;
		if (retstr == '1')
			break;
		sleep (1);
		//logStream (MESSAGE_DEBUG) << "LX200TEST tel_rep_write - for " << count <<
		//	" time" << sendLog;
	}
	if (count == 200)
	{
		logStream (MESSAGE_ERROR) <<
			"LX200TEST tel_rep_write unsucessful due to incorrect return." << sendLog;
		return -1;
	}
	return 0;
}

/*!
 * Set LX200 right ascenation.
 *
 * @param ra		right ascenation to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
LX200TEST::tel_write_ra (double ra)
{
	char command[14];
	int h, m, s;
	if (ra < 0 || ra > 360.0)
	{
		return -1;
	}
	ra = ra / 15.;
	dtoints (ra, &h, &m, &s);
	if (snprintf (command, 14, "#:Sr%02d:%02d:%02d#", h, m, s) < 0)
		return -1;
	return tel_rep_write (command);
}


/*!
 * Set LX200 declination.
 *
 * @param dec		declination to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
LX200TEST::tel_write_dec (double dec)
{
	char command[15];
	int h, m, s;
	if (dec < -90.0 || dec > 90.0)
	{
		return -1;
	}
	dtoints (dec, &h, &m, &s);
	if (snprintf (command, 15, "#:Sd%+02d*%02d:%02d#", h, m, s) < 0)
		return -1;
	return tel_rep_write (command);
}
LX200TEST::LX200TEST (int in_argc, char **in_argv):Telescope (in_argc,in_argv)
{
	device_file = "/dev/ttyS0";

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
	addOption ('x', "DECaxis_HA",  1, "DEC axis HA coordinate, valid: East, West");

	createValue (DECaxis_HAcoordinate,  "DECXHA",  "DEC axis HA coordinate, West (==-1), East (==1)", false, RTS2_VALUE_WRITABLE);

	motors = 0;
	tel_desc = -1;

	move_state = NOTMOVE;
}


LX200TEST::~LX200TEST (void)
{
	close (tel_desc);
}


int
LX200TEST::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 'x':
		  if( !( strcmp(optarg, "East")))
		    {
		      DECaxis_HAcoordinate->setValueInteger(IS_EAST) ;
		    }
		  else if( !( strcmp(optarg, "West")))
		    {
		       DECaxis_HAcoordinate->setValueInteger(IS_WEST) ;
		    }
		  else
		    {
		      logStream (MESSAGE_ERROR) << "LX200TEST::processOption no valid DEC axis HA coordinate given :"<< optarg << sendLog;
		      exit(1) ;
		    }
		  break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}


/*!
 * Init telescope, connect on given tel_desc.
 *
 * @param device_name		pointer to device name
 * @param telescope_id		id of telescope, for LX200 ignored
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int
LX200TEST::init ()
{
	struct termios tel_termios;
	char rbuf[10];

	int status;

	status = Telescope::init ();
	if (status)
		return status;

	tel_desc = open (device_file, O_RDWR);

	if (tel_desc < 0)
		return -1;

	if (tcgetattr (tel_desc, &tel_termios) < 0)
		return -1;

	if (cfsetospeed (&tel_termios, B9600) < 0 ||
		cfsetispeed (&tel_termios, B9600) < 0)
		return -1;

	tel_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
	tel_termios.c_oflag = 0;
	tel_termios.c_cflag =
		((tel_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
	tel_termios.c_lflag = 0;
	tel_termios.c_cc[VMIN] = 0;
	tel_termios.c_cc[VTIME] = 5;

	if (tcsetattr (tel_desc, TCSANOW, &tel_termios) < 0)
	{
		logStream (MESSAGE_ERROR) << "LX200TEST init tcsetattr" << sendLog;
		return -1;
	}

	// get current state of control signals
	ioctl (tel_desc, TIOCMGET, &status);

	// Drop DTR
	status &= ~TIOCM_DTR;
	ioctl (tel_desc, TIOCMSET, &status);

	logStream (MESSAGE_DEBUG) << "LX200TEST init initialization complete on port "
		<< device_file << sendLog;

	// we get 12:34:4# while we're in short mode
	// and 12:34:45 while we're in long mode
	if (tel_write_read_hash ("#:Gr#", 5, rbuf, 9) < 0)
		return -1;

	if (rbuf[7] == '\0')
	{
		// that could be used to distinguish between long
		// short mode
		// we are in short mode, set the long on
		if (tel_write_read ("#:U#", 5, rbuf, 0) < 0)
			return -1;
		return 0;
	}
	return 0;
}


/*!
 * Reads information about telescope.
 *
 */
int LX200TEST::initValues ()
{
	int ret = -1 ;

        rts2core::Configuration *config = rts2core::Configuration::instance ();
        ret = config->loadFile ();
        if (ret)
	  return -1;

        telLongitude->setValueDouble (config->getObserver ()->lng);
        telLatitude->setValueDouble (config->getObserver ()->lat);
	setTelAltitude (config->getObservatoryAltitude ());

	if (tel_read_longtitude () || tel_read_latitude ())
		return -1;

	telFlip->setValueInteger (0);

	return Telescope::initValues ();
}

int LX200TEST::info ()
{
	if (tel_read_ra () || tel_read_dec ())
		return -1;

	return Telescope::info ();
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
int LX200TEST::tel_set_rate (char new_rate)
{
	char command[6];
	sprintf (command, "#:R%c#", new_rate);
	return tel_write (command, 5);
}

int LX200TEST::telescope_start_move (char direction)
{
	char command[6];
	tel_set_rate (RATE_FIND);
	sprintf (command, "#:M%c#", direction);
	return tel_write (command, 5) == 1 ? -1 : 0;
}

int LX200TEST::telescope_stop_move (char direction)
{
	char command[6];
	sprintf (command, "#:Q%c#", direction);
	return tel_write (command, 5) < 0 ? -1 : 0;
}

/*!
 * Slew (=set) LX200 to new coordinates.
 *
 * @param ra 		new right ascenation
 * @param dec 		new declination
 *
 * @return -1 on error, otherwise 0
 */
int LX200TEST::tel_slew_to (double ra, double dec)
{
  int ret ;
  char retstr;
  struct ln_lnlat_posn observer ;		  
  struct ln_equ_posn target_equ;

  normalizeRaDec (ra, dec);
  target_equ.ra= ra;
  target_equ.dec= dec ;

  observer.lng = telLongitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();

  if(( ret= DECaxis_HAcoordinate->getValueInteger()) == IS_WEST) {
    ret= pier_collision( &target_equ, &observer) ;
  } else if(( ret=DECaxis_HAcoordinate->getValueInteger()) == IS_EAST) {
    //really target_equ.dec += 180. ;
    struct ln_equ_posn t_equ;
    t_equ.ra = target_equ.ra ;
    t_equ.dec= target_equ.dec + 180. ;
    ret= pier_collision( &t_equ, &observer) ;
  } else {
    logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to neither IS_WEST nor IS_EAST "<< dec  << sendLog;
    return -1;
  }
  if(ret != NO_COLLISION) {
    if( ret== COLLIDING) {
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " dec " << target_equ.dec << " COLLIDING, NOT syncing cupola"  << sendLog;
    } else if( ret== WITHIN_DANGER_ZONE){
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " dec " << target_equ.dec << " within DANGER zone, NOT syncing cupola"  << sendLog;
    } else {
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " target_equ.dec " << target_equ.dec << " invalid condition, exiting"  << sendLog;
      exit(1) ;
    }
      return -1;
    }
  logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to cleared geometry  slewing ra "<< target_equ.ra << " target_equ.dec " << dec  << sendLog;


  if (tel_write_ra (target_equ.ra) < 0 || tel_write_dec (target_equ.dec) < 0)
    return -1;
  if (tel_write_read ("#:MS#", 5, &retstr, 1) < 0)
    return -1;
  logStream (MESSAGE_ERROR) << "LX200TEST::tel_slew_to not colliding slewing ra "<< target_equ.ra << " dec " << target_equ.dec  << sendLog;
  if (retstr == '0')
    {
    return 0;
    }
  
  logStream (MESSAGE_ERROR) << "LX200TEST::tel_slew_to NOT slewing ra "<< target_equ.ra << " dec " << target_equ.dec << " got '0'!= >"<< retstr<<"<END, NOT syncing cupola"  << sendLog;
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
int LX200TEST::tel_check_coords (double ra, double dec)
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

	//	logStream (MESSAGE_DEBUG) << "LX200TEST tel_check_coords TELESCOPE ALT " << hrz.
	//	alt << " AZ " << hrz.az << sendLog;

	target.ra = ra;
	target.dec = dec;

	sep = ln_get_angular_separation (&object, &target);

	if (sep > 0.1)
		return 0;

	return 1;
}

void LX200TEST::set_move_timeout (time_t plus_time)
{
	time_t now;
	time (&now);

	move_timeout = now + plus_time;
}

int LX200TEST::startResync ()
{
	int ret;

	lastMoveRa = getTelTargetRa ();
	lastMoveDec = getTelTargetDec ();

	ret = tel_slew_to (lastMoveRa, lastMoveDec);
	if (ret)
	  return -1;
	move_state = MOVE_REAL;
	set_move_timeout (100);
	return 0 ; 
}

int LX200TEST::isMoving ()
{
  int ret;
  
  switch (move_state)
    {
    case MOVE_REAL:
      ret = tel_check_coords (lastMoveRa, lastMoveDec);
      switch (ret)
	{
	case -1:
	  return -1;
	case 0:
	  return USEC_SEC / 10;
	case 1:
	case 2:
	  move_state = NOTMOVE;
	  return -2;
	}
      break;
    default:
      logStream (MESSAGE_ERROR) << "LX200TEST::isMoving NO case >" << sendLog ; // << move_state << "<END" << sendLog;
      break;
    }
  return -1;
}

int LX200TEST::stopMove ()
{
	char dirs[] = { 'e', 'w', 'n', 's' };
	int i;
	for (i = 0; i < 4; i++)
	{
		if (telescope_stop_move (dirs[i]) < 0)
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
int LX200TEST::setTo (double ra, double dec)
{
	char readback[101];
	int ret;

	normalizeRaDec (ra, dec);

	if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
		return -1;
	if (tel_write_read_hash ("#:CM#", 5, readback, 100) < 0)
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
int LX200TEST::correct (double cor_ra, double cor_dec, double real_ra, double real_dec)
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
int LX200TEST::startPark ()
{
  double JD= ln_get_julian_from_sys ();
  double local_sidereal_time= ln_get_mean_sidereal_time( JD) * 15. + telLongitude->getValueDouble ();  // longitude positive to the East
  
  return tel_slew_to (local_sidereal_time, -(90. -telLatitude->getValueDouble()));
}

int LX200TEST::isParking ()
{
	return -2;
}

int LX200TEST::endPark ()
{
	return 0;
}

int LX200TEST::startDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			tel_set_rate (RATE_FIND);
			return telescope_start_move (*dir);
	}
	return -2;
}

int LX200TEST::stopDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			return telescope_stop_move (*dir);
	}
	return -2;
}

int main (int argc, char **argv)
{
	LX200TEST device = LX200TEST (argc, argv);
	return device.run ();
}

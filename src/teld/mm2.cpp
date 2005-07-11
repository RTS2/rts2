#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*! 
 * @file Driver for Dynostar telescope.
 * 
 * $Id$
 * 
 * It's something similar to LX200, with some changes.
 *
 * LX200 was based on original c library for XEphem writen by Ken
 * Shouse <ken@kshouse.engine.swri.edu> and modified by Carlos Guirao
 * <cguirao@eso.org>. Althought most of the code was completly
 * rewriten and you will barely find any piece from XEphem.
 *
 * @author john
 */

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <libnova/libnova.h>
#include <sys/ioctl.h>

#include "telescope.h"
#include "../utils/hms.h"
#include "status.h"

#include <termios.h>
// uncomment following line, if you want all tel_desc read logging (will
// at about 10 30-bytes lines to syslog for every query). 
// #define DEBUG_ALL_PORT_COMM
#define DEBUG

#define RATE_SLEW	'S'
#define RATE_FIND	'M'
#define RATE_CENTER	'C'
#define RATE_GUIDE	'G'
#define DIR_NORTH	'n'
#define DIR_EAST	'e'
#define DIR_SOUTH	's'
#define DIR_WEST	'w'
#define PORT_TIMEOUT	5

#define MOTORS_ON	1
#define MOTORS_OFF	-1

#define HOME_RA		37.9542
#define HOME_DEC	87.3075

// hard-coded LOT & LAT
#define TEL_LONG 	-6.239166667
#define TEL_LAT		53.3155555555

class Rts2DevTelescopeMM2:public Rts2DevTelescope
{
private:
  char *device_file;
  int tel_desc;
  int motors;

  double lastMoveRa, lastMoveDec;

  enum
  { NOT_TOGLING, TOGLE_1, TOGLE_2 } togle_state;
  enum
  { NOTMOVE, MOVE_HOME, MOVE_REAL } move_state;
  time_t move_timeout;

  time_t next_togle;
  int togle_count;

  // low-level functions..
  int tel_read (char *buf, int count);
  int tel_read_hash (char *buf, int count);
  int tel_write (char *buf, int count);
  int tel_write_read (char *wbuf, int wcount, char *rbuf, int rcount);
  int tel_write_read_hash (char *wbuf, int wcount, char *rbuf, int rcount);
  int tel_read_hms (double *hmsptr, char *command);

  int tel_read_ra ();
  int tel_read_dec ();
  int tel_read_localtime ();
  int tel_read_siderealtime ();
  int tel_read_latitude ();
  int tel_read_longtitude ();
  int tel_rep_write (char *command);
  void tel_normalize (double *ra, double *dec);

  int tel_write_ra (double ra);
  int tel_write_dec (double dec);

  int tel_set_rate (char new_rate);
  int telescope_start_move (char direction);
  int telescope_stop_move (char direction);

  int tel_slew_to (double ra, double dec);

  int tel_check_coords (double ra, double dec);

  double get_hour_angle (double RA);

  void toggle_mode (int in_togle_count);
  void set_move_timeout (time_t plus_time);
public:
    Rts2DevTelescopeMM2 (int argc, char **argv);
    virtual ~ Rts2DevTelescopeMM2 (void);
  virtual int processOption (int in_opt);
  virtual int idle ();
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int setTo (double set_ra, double set_dec);
  virtual int correct (double cor_ra, double cor_dec, double real_ra,
		       double real_dec);

  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int endMove ();
  virtual int stopMove ();

  virtual int startPark ();
  virtual int isParking ();
  virtual int endPark ();

  virtual int startDir (char *dir);
  virtual int stopDir (char *dir);
};

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
Rts2DevTelescopeMM2::tel_read (char *buf, int count)
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
	  syslog (LOG_DEBUG,
		  "Rts2DevTelescopeMM2::tel_read: tel_desc read error %i (%m)",
		  errno);
	  return -1;
	}
#ifdef DEBUG_ALL_PORT_COMM
      syslog (LOG_DEBUG, "Rts2DevTelescopeMM2::tel_read: readed '%c'",
	      buf[readed]);
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
Rts2DevTelescopeMM2::tel_read_hash (char *buf, int count)
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
  syslog (LOG_DEBUG, "Rts2DevTelescopeMM2::tel_read_hash: Hash-readed:'%s'",
	  buf);
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
Rts2DevTelescopeMM2::tel_write (char *buf, int count)
{
  syslog (LOG_DEBUG, "Rts2DevTelescopeMM2::tel_write :will write:'%s'", buf);
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
Rts2DevTelescopeMM2::tel_write_read (char *wbuf, int wcount, char *rbuf,
				     int rcount)
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
      syslog (LOG_DEBUG, "Rts2DevTelescopeMM2::tel_write_read: readed %i %s",
	      tmp_rcount, buf);
      free (buf);
    }
  else
    {
      syslog (LOG_DEBUG,
	      "Rts2DevTelescopeMM2::tel_write_read: readed returns %i",
	      tmp_rcount);
    }

  return tmp_rcount;
}

/*! 
 * Combine write && read_hash together.
 *
 * @see tel_write_read for definition
 */
int
Rts2DevTelescopeMM2::tel_write_read_hash (char *wbuf, int wcount, char *rbuf,
					  int rcount)
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
 * Reads some value from MM2 in hms format.
 * 
 * Utility function for all those read_ra and other.
 * 
 * @param hmsptr	where hms will be stored
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeMM2::tel_read_hms (double *hmsptr, char *command)
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
 * Reads MM2 right ascenation.
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeMM2::tel_read_ra ()
{
  double new_ra;
  if (tel_read_hms (&new_ra, "#:GR#"))
    return -1;
  telRa = new_ra * 15.0;
  return 0;
}

/*!
 * Reads MM2 declination.
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeMM2::tel_read_dec ()
{
  return tel_read_hms (&telDec, "#:GD#");
}

/*! 
 * Returns MM2 local time.
 * 
 * @return -1 and errno on error, otherwise 0
 *
 * TEMPORARY
 * MY EDIT MM2 local time
 *
 * Hardcode local time and return 0
 */
int
Rts2DevTelescopeMM2::tel_read_localtime ()
{
  time_t curtime;
  struct tm *loctime;

  /* Get the current time. */
  time (&curtime);

  /* Convert it to local time representation. */
  loctime = localtime (&curtime);

  telLocalTime =
    loctime->tm_hour + loctime->tm_min / 60 + loctime->tm_sec / 3600;

  return 0;
}

/*! 
 * Returns MM2 sidereal time.
 * 
 * @return -1 and errno on error, otherwise 0
 *
 * TEMPORARY
 * MY EDIT MM2 sidereal time
 *
 * Hardcode sidereal time and return 0
 * Dynostar doesn't suptel_desc reading Sidereal time, 
 * so read sidereal time from system
 */
int
Rts2DevTelescopeMM2::tel_read_siderealtime ()
{
  tel_read_longtitude ();
  telSiderealTime = ln_get_mean_sidereal_time
    (ln_get_julian_from_sys ()) * 15.0 - telLongtitude;
  telSiderealTime = ln_range_degrees (telSiderealTime) / 15.0;
}

/*! 
 * Reads MM2 latitude.
 * 
 * @return -1 on error, otherwise 0
 *
 * MY EDIT MM2 latitude
 *
 * Hardcode latitude and return 0
 */
int
Rts2DevTelescopeMM2::tel_read_latitude ()
{
  return 0;
}

/*! 
 * Reads MM2 longtitude.
 * 
 * @return -1 on error, otherwise 0
 *
 * MY EDIT MM2 longtitude
 *
 * Hardcode longtitude and return 0
 */
int
Rts2DevTelescopeMM2::tel_read_longtitude ()
{
  return 0;
}

/*! 
 * Repeat MM2 write.
 * 
 * Handy for setting ra and dec.
 * Meade tends to have problems with that, don't know about MM2.
 *
 * @param command	command to write on tel_desc
 */
int
Rts2DevTelescopeMM2::tel_rep_write (char *command)
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
      syslog (LOG_DEBUG, "Rts2DevTelescopeMM2::tel_rep_write - for %i time.",
	      count);
    }
  if (count == 200)
    {
      syslog (LOG_ERR,
	      "Rts2DevTelescopeMM2::tel_rep_write unsucessful due to incorrect return.");
      return -1;
    }
  return 0;
}

/*!
 * Normalize ra and dec,
 *
 * @param ra		rigth ascenation to normalize in decimal hours
 * @param dec		rigth declination to normalize in decimal degrees
 *
 * @return 0
 */
void
Rts2DevTelescopeMM2::tel_normalize (double *ra, double *dec)
{
  if (*ra < 0)
    *ra = floor (*ra / 360) * -360 + *ra;	//normalize ra
  if (*ra > 360)
    *ra = *ra - floor (*ra / 360) * 360;

  if (*dec < -90)
    *dec = floor (*dec / 90) * -90 + *dec;	//normalize dec
  if (*dec > 90)
    *dec = *dec - floor (*dec / 90) * 90;
}

/*! 
 * Set MM2 right ascenation.
 *
 * @param ra		right ascenation to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeMM2::tel_write_ra (double ra)
{
  char command[14];
  int h, m, s;
  if (ra < 0 || ra > 360.0)
    {
      return -1;
    }
  ra = ra / 15;
  dtoints (ra, &h, &m, &s);
  if (snprintf (command, 14, "#:Sr%02d:%02d:%02d#", h, m, s) < 0)
    return -1;
  return tel_rep_write (command);
}

/*! 
 * Set MM2 declination.
 *
 * @param dec		declination to set in decimal degrees
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeMM2::tel_write_dec (double dec)
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

Rts2DevTelescopeMM2::Rts2DevTelescopeMM2 (int argc, char **argv):Rts2DevTelescope (argc,
		  argv)
{
  device_file = "/dev/ttyS0";

  addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");

  motors = 0;
  tel_desc = -1;

  move_state = NOTMOVE;

  togle_state = NOT_TOGLING;
  togle_count = 0;

  telLongtitude = TEL_LONG;
  telLatitude = TEL_LAT;
}

Rts2DevTelescopeMM2::~Rts2DevTelescopeMM2 (void)
{
  close (tel_desc);
}

int
Rts2DevTelescopeMM2::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      device_file = optarg;
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
}

int
Rts2DevTelescopeMM2::idle ()
{
  time_t now;
  time (&now);

  int status;

  switch (togle_state)
    {
    case TOGLE_1:
      if (now > next_togle)
	{
	  // DTR low 
	  status &= ~TIOCM_DTR;
	  ioctl (tel_desc, TIOCMSET, &status);
	  next_togle = now + 2;
	  togle_state = TOGLE_2;
	}
      break;
    case TOGLE_2:
      if (now > next_togle)
	{
	  if (togle_count > 1)
	    toggle_mode (togle_count - 1);
	  else
	    togle_state = NOT_TOGLING;
	}
      break;
    }

  return Rts2DevTelescope::idle ();
}

/*!
* Init telescope, connect on given tel_desc.
* 
* @param device_name		pointer to device name
* @param telescope_id		id of telescope, for MM2 ignored
* 
* @return 0 on succes, -1 & set errno otherwise
*/
int
Rts2DevTelescopeMM2::init ()
{
  struct termios tel_termios;
  char rbuf[10];

  int status;

  status = Rts2DevTelescope::init ();
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
      syslog (LOG_ERR, "Rts2DevTelescopeMM2::init tcsetattr: %m");
      return -1;
    }

// get current state of control signals 
  ioctl (tel_desc, TIOCMGET, &status);

// Drop DTR  
  status &= ~TIOCM_DTR;
  ioctl (tel_desc, TIOCMSET, &status);

  syslog (LOG_DEBUG,
	  "Rts2DevTelescopeMM2::init initialization complete (on port '%s')",
	  device_file);

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

int
Rts2DevTelescopeMM2::ready ()
{
  return tel_desc > 0 ? 0 : -1;
}

/*!
* Reads information about telescope.
*
*/
int
Rts2DevTelescopeMM2::baseInfo ()
{
  if (tel_read_longtitude () || tel_read_latitude ())
    return -1;

  strcpy (telType, "MM2");
  strcpy (telSerialNumber, "000001");
  telAltitude = 600;

  telFlip = 0;

  return 0;
}

int
Rts2DevTelescopeMM2::info ()
{
  if (tel_read_ra ()
      || tel_read_dec () || tel_read_siderealtime () || tel_read_localtime ())
    return -1;

  return 0;
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
int
Rts2DevTelescopeMM2::tel_set_rate (char new_rate)
{
  char command[6];
  sprintf (command, "#:R%c#", new_rate);
  return tel_write (command, 5);
}

int
Rts2DevTelescopeMM2::telescope_start_move (char direction)
{
  char command[6];
  tel_set_rate (RATE_FIND);
  sprintf (command, "#:M%c#", direction);
  return tel_write (command, 5) == 1 ? -1 : 0;
}

int
Rts2DevTelescopeMM2::telescope_stop_move (char direction)
{
  char command[6];
  sprintf (command, "#:Q%c#", direction);
  return tel_write (command, 5) < 0 ? -1 : 0;
}

/*! 
* Slew (=set) MM2 to new coordinates.
*
* @param ra 		new right ascenation
* @param dec 		new declination
*
* @return -1 on error, otherwise 0
*/
int
Rts2DevTelescopeMM2::tel_slew_to (double ra, double dec)
{
  char retstr;

  tel_normalize (&ra, &dec);

  if (tel_write_ra (ra) < 0 || tel_write_dec (dec) < 0)
    return -1;
  if (tel_write_read ("#:MS#", 5, &retstr, 1) < 0)
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
int
Rts2DevTelescopeMM2::tel_check_coords (double ra, double dec)
{
  // ADDED BY JF
  double JD;
  double HA;

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
  object.ra = telRa;
  object.dec = telDec;

  observer.lng = telLongtitude;
  observer.lat = telLatitude;

  JD = ln_get_julian_from_sys ();

  ln_get_hrz_from_equ (&object, &observer, JD, &hrz);

  HA = get_hour_angle (object.ra);

  syslog (LOG_DEBUG,
	  "Rts2DevTelescopeMM2::tel_check_coords TELESCOPE ALT = %f, AZ = %f",
	  hrz.alt, hrz.az);
  syslog (LOG_DEBUG,
	  "Rts2DevTelescopeMM2::tel_check_coords TELESCOPE HOUR ANGLE = %f",
	  HA);

  target.ra = ra;
  target.dec = dec;

  sep = ln_get_angular_separation (&object, &target);

  if (sep > 0.1)
    return 0;

  return 1;
}


/* Convert RA to Hour Angle
* 
*/
double
Rts2DevTelescopeMM2::get_hour_angle (double RA)
{
  tel_read_siderealtime ();
  return telSiderealTime - (RA / 15.0);
}

void
Rts2DevTelescopeMM2::toggle_mode (int in_togle_count)
{
  int status;

// get current state of control signals 
  ioctl (tel_desc, TIOCMGET, &status);

// DTR high
  status |= TIOCM_DTR;
  ioctl (tel_desc, TIOCMSET, &status);

// get current state of control signals 
  ioctl (tel_desc, TIOCMGET, &status);

  togle_state = TOGLE_1;

  next_togle = time (NULL) + 4;
  togle_count = in_togle_count;
}

void
Rts2DevTelescopeMM2::set_move_timeout (time_t plus_time)
{
  time_t now;
  time (&now);

  move_timeout = now + plus_time;
}

int
Rts2DevTelescopeMM2::startMove (double tar_ra, double tar_dec)
{
  int ret;

  stopMove ();

  ret = tel_slew_to (HOME_RA, HOME_DEC);

  if (ret)
    {
      move_state = NOTMOVE;
      return -1;
    }

  move_state = MOVE_HOME;
  set_move_timeout (100);
  lastMoveRa = tar_ra;
  lastMoveDec = tar_dec;
  return 0;
}

int
Rts2DevTelescopeMM2::isMoving ()
{
  int ret;

  switch (move_state)
    {
    case MOVE_HOME:
      ret = tel_check_coords (HOME_RA, HOME_DEC);
      switch (ret)
	{
	case -1:
	  return -1;
	case 0:
	  return USEC_SEC / 10;
	case 1:
	case 2:
	  stopMove ();
	  ret = tel_slew_to (lastMoveRa, lastMoveDec);
	  if (ret)
	    return -1;
	  move_state = MOVE_REAL;
	  set_move_timeout (100);
	  return USEC_SEC / 10;
	}
      break;
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
    }
  return -1;
}

int
Rts2DevTelescopeMM2::endMove ()
{
// TEST 
// TURN OFF TRACKING AFTER MOVE
  toggle_mode (2);
  return 0;
}

int
Rts2DevTelescopeMM2::stopMove ()
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
 * actually is at the beggining of observation (remember, that MM2
 * doesn't have absolute position sensors)
 * 
 * @param ra		setting right ascennation
 * @param dec		setting declination
 * 
 * @return -1 and set errno on error, otherwise 0
 */

int
Rts2DevTelescopeMM2::setTo (double ra, double dec)
{
  char readback[101];
  int ret;

  tel_normalize (&ra, &dec);

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
int
Rts2DevTelescopeMM2::correct (double cor_ra, double cor_dec, double real_ra,
			      double real_dec)
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
int
Rts2DevTelescopeMM2::startPark ()
{
// turn off tracking    
  toggle_mode (1);
  return 0;
}

int
Rts2DevTelescopeMM2::isParking ()
{
  return -2;
}

int
Rts2DevTelescopeMM2::endPark ()
{
  return 0;
}

int
Rts2DevTelescopeMM2::startDir (char *dir)
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

int
Rts2DevTelescopeMM2::stopDir (char *dir)
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

Rts2DevTelescopeMM2 *device;

void
killSignal (int sig)
{
  if (device)
    delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevTelescopeMM2 (argc, argv);

  int ret = -1;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot find telescope, exiting\n");
      exit (1);
    }
  device->run ();
  delete device;
}

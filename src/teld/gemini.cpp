#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*! 
 * @file Driver file for LOSMANDY Gemini telescope systems 
 * 
 * @author petr 
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <mcheck.h>

#include <libnova/libnova.h>

#include "telescope.h"
#include "../utils/hms.h"
#include "status.h"

// uncomment following line, if you want all port read logging (will
// at about 10 30-bytes lines to syslog for every query). 
// #define DEBUG_ALL_PORT_COMM

#define RATE_SLEW	'S'
#define RATE_FIND	'M'
#define RATE_CENTER	'C'
#define RATE_GUIDE	'G'
#define DIR_NORTH	'n'
#define DIR_EAST	'e'
#define DIR_SOUTH	's'
#define DIR_WEST	'w'
#define PORT_TIMEOUT	5

#define TCM_DEFAULT_RATE	32768

//! telescope parking declination
#define PARK_DEC	0

class Rts2DevTelescopeGemini:public Rts2DevTelescope
{
private:
  char *device_file_io;
  int tel_desc;
  // utility I/O functions
  int tel_read (char *buf, int count);
  int tel_read_hash (char *buf, int count);
  int tel_write (char *buf, int count);
  int tel_write_read_no_reset (char *wbuf, int wcount, char *rbuf,
			       int rcount);
  int tel_write_read (char *buf, int wcount, char *rbuf, int rcount);
  int tel_write_read_hash (char *wbuf, int wcount, char *rbuf, int rcount);
  int tel_read_hms (double *hmsptr, char *command);
  char tel_gemini_checksum (const char *buf);
  // higher level I/O functions
  int tel_gemini_set (int id, int val);
  int tel_gemini_get (int id, int32_t * val);
  int tel_gemini_reset ();
  int tel_gemini_match_time ();
  int tel_read_ra ();
  int tel_read_dec ();
  int tel_read_localtime ();
  int tel_read_longtitude ();
  int tel_read_siderealtime ();
  int tel_read_latitude ();
  int tel_rep_write (char *command);
  void tel_normalize (double *ra, double *dec);
  int tel_write_ra (double ra);
  int tel_write_dec (double dec);
  int tel_set_to (double ra, double dec);

  int tel_set_rate (char new_rate);
  int telescope_start_move (char direction);
  int telescope_stop_move (char direction);
public:
    Rts2DevTelescopeGemini (int argc, char **argv);
   ~Rts2DevTelescopeGemini (void);
  int processOption (int in_opt);
  int init ();
  int ready ();
  int baseInfo ();
  int info ();
  int startMove (double tar_ra, double tar_dec);
  int isMoving ();
  int isParked ();
  int endMove ();
  int setTo (double set_ra, double set_dec);
  int correct (double cor_ra, double cor_dec);
  int change (double chng_ra, double chng_dec);
  int stop ();
  int park ();

  int save ();
  int load ();
};

/*! 
 * Reads some data directly from port.
 * 
 * Log all flow as LOG_DEBUG to syslog
 * 
 * @exception EIO when there aren't data from port
 * 
 * @param buf 		buffer to read in data
 * @param count 	how much data will be readed
 * 
 * @return -1 on failure, otherwise number of read data 
 */

int
Rts2DevTelescopeGemini::tel_read (char *buf, int count)
{
  int readed;

  for (readed = 0; readed < count; readed++)
    {
      int ret = read (tel_desc, &buf[readed], 1);
      printf ("read_from: %i size:%i\n", tel_desc, ret);
      if (ret <= 0)
	{
	  return -1;
	}
#ifdef DEBUG_ALL_PORT_COMM
      syslog (LOG_DEBUG, "Losmandy: readed '%c'", buf[readed]);
#endif
    }
  return readed;
}

/*! 
 * Will read from port till it encoutered # character.
 * 
 * Read ending #, but doesn't return it.
 * 
 * @see tel_read() for description
 */
int
Rts2DevTelescopeGemini::tel_read_hash (char *buf, int count)
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
  syslog (LOG_DEBUG, "Losmandy:Hash-readed:'%s'", buf);
  return readed;
}

/*! 
 * Will write on telescope port string.
 *
 * @exception EIO, .. common write exceptions 
 * 
 * @param buf 		buffer to write
 * @param count 	count to write
 * 
 * @return -1 on failure, count otherwise
 */
int
Rts2DevTelescopeGemini::tel_write (char *buf, int count)
{
  int ret;
  syslog (LOG_DEBUG, "Losmandy:will write:'%s'", buf);
  ret = write (tel_desc, buf, count);
  tcflush (tel_desc, TCIFLUSH);
  return ret;
}

/*! 
 * Combine write && read together.
 * 
 * Flush port to clear any gargabe.
 *
 * @exception EINVAL and other exceptions
 * 
 * @param wbuf		buffer to write on port
 * @param wcount	write count
 * @param rbuf		buffer to read from port
 * @param rcount	maximal number of characters to read
 * 
 * @return -1 and set errno on failure, rcount otherwise
 */
int
Rts2DevTelescopeGemini::tel_write_read_no_reset (char *wbuf, int wcount,
						 char *rbuf, int rcount)
{
  int tmp_rcount = -1;
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
      syslog (LOG_DEBUG, "Losmandy:readed %i %s", tmp_rcount, buf);
      free (buf);
    }
  else
    {
      syslog (LOG_DEBUG, "Losmandy:readed returns %i", tmp_rcount);
    }
}

int
Rts2DevTelescopeGemini::tel_write_read (char *buf, int wcount, char *rbuf,
					int rcount)
{
  int ret;
  ret = tel_write_read_no_reset (buf, wcount, rbuf, rcount);
  if (ret <= 0)
    {
      // try rebooting
      tel_gemini_reset ();
      ret = tel_write_read_no_reset (buf, wcount, rbuf, rcount);
    }
  return ret;
}

/*! 
 * Combine write && read_hash together.
 *
 * @see tel_write_read for definition
 */
int
Rts2DevTelescopeGemini::tel_write_read_hash (char *wbuf, int wcount,
					     char *rbuf, int rcount)
{
  int tmp_rcount = -1;

  if (tcflush (tel_desc, TCIOFLUSH) < 0)
    return -1;
  if (tel_write (wbuf, wcount) < 0)
    return -1;

  tmp_rcount = tel_read_hash (rbuf, rcount);

  return tmp_rcount;
}

/*! 
 * Reads some value from lx200 in hms format.
 * 
 * Utility function for all those read_ra and other.
 * 
 * @param hmsptr	where hms will be stored
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_hms (double *hmsptr, char *command)
{
  char wbuf[11];
  if (tel_write_read_hash (command, strlen (command), wbuf, 10) < 6)
    return -1;
  *hmsptr = hmstod (wbuf);
  if (isnan (*hmsptr))
    return -1;
  return 0;
}

/*!
 * Computes gemini checksum
 *
 * @param buf checksum to compute
 * 
 * @return computed checksum
 */
char
Rts2DevTelescopeGemini::tel_gemini_checksum (const char *buf)
{
  char checksum = 0;
  for (; *buf; buf++)
    checksum ^= *buf;
  checksum &= ~128;		// modulo 128
  checksum += 64;
  return checksum;
}

/*!
 * Write command to set gemini local parameters
 *
 * @param id	id to set
 * @param val	value to set
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_gemini_set (int id, int val)
{
  char buf[15];
  int len;
  len = sprintf (buf, ">%i:%i", id, val);
  buf[len] = tel_gemini_checksum (buf);
  len++;
  buf[len] = '#';
  len++;
  buf[len] = '\0';
  return tel_write (buf, len);
}


/*!
 * Read gemini local parameters
 *
 * @param id	id to get
 * @param val	pointer where to store result
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_gemini_get (int id, int32_t * val)
{
  char buf[9], *ptr, checksum;
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
  for (ptr = buf; *ptr && *ptr >= '0' && *ptr <= '9'; ptr++);
  checksum = *ptr;
  *ptr = '\0';
  if (tel_gemini_checksum (buf) != checksum)
    {
      syslog (LOG_ERR, "invalid gemini checksum: should be '%c', is '%c'",
	      tel_gemini_checksum (buf), checksum);
      return -1;
    }
  *val = atol (buf);
  return 0;
}

/*!
 * Reset and home losmandy telescope
 */
int
Rts2DevTelescopeGemini::tel_gemini_reset ()
{
  char rbuf[3];
  tel_write_read_hash ("\x06", 1, rbuf, 2);
  if (*rbuf == 'b')		// booting phase, select warm reboot
    return tel_write ("bR#", 3);
  return 0;
}

/*!
 * Match gemini time with system time
 */
int
Rts2DevTelescopeGemini::tel_gemini_match_time ()
{
  struct tm ts;
  time_t t;
  char buf[55];
  int ret;
  char rep;
  tel_write ("#:SB8#", 6);
  t = time (NULL);
  gmtime_r (&t, &ts);
  // set time
  ret = tel_write_read ("#:SG+00#", 8, &rep, 1);
  if (ret < 0)
    return ret;
  snprintf (buf, 14, "#:SL%02d:%02d:%02d#", ts.tm_hour, ts.tm_min, ts.tm_sec);
  ret = tel_write_read (buf, strlen (buf), &rep, 1);
  if (ret < 0)
    return ret;
  snprintf (buf, 14, "#:SC%02d/%02d/%02d#", ts.tm_mon + 1, ts.tm_mday,
	    ts.tm_year - 100);
  ret = tel_write_read_hash (buf, strlen (buf), buf, 55);
  if (ret < 0)
    return ret;
  if (*buf != '1')
    {
      return -1;
    }
  syslog (LOG_INFO, "GEMINI: time match");
  return 0;
}

/*! 
 * Reads lx200 right ascenation.
 * 
 * @param raptr		where ra will be stored
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_ra ()
{
  if (tel_read_hms (&telRa, "#:GR#"))
    return -1;
  telRa *= 15.0;
  return 0;
}

/*!
 * Reads losmandy declination.
 * 
 * @param decptr	where dec will be stored
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_dec ()
{
  return tel_read_hms (&telDec, "#:GD#");
}

/*! 
 * Returns losmandy local time.
 * 
 * @param tptr		where time will be stored
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_localtime ()
{
  return tel_read_hms (&telLocalTime, "#:GL#");
}

/*! 
 * Reads losmandy longtitude.
 * 
 * @param latptr	where longtitude will be stored  
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_longtitude ()
{
  return tel_read_hms (&telLongtitude, "#:Gg#");
}

/*! 
 * Returns losmandy sidereal time.
 * 
 * @param tptr		where time will be stored
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_siderealtime ()
{
  tel_read_longtitude ();
  telSiderealTime = ln_get_mean_sidereal_time
    (ln_get_julian_from_sys ()) * 15.0 - telLongtitude;
  telSiderealTime = ln_range_degrees (telSiderealTime) / 15.0;
  return 0;
}

/*! 
 * Reads losmandy latitude.
 * 
 * @param latptr	here latitude will be stored  
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_read_latitude ()
{
  return tel_read_hms (&telLatitude, "#:Gt#");
}

/*! 
 * Repeat losmandy write.
 * 
 * Handy for setting ra and dec.
 * Meade tends to have problems with that.
 *
 * @param command	command to write on port
 */
int
Rts2DevTelescopeGemini::tel_rep_write (char *command)
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
      syslog (LOG_DEBUG, "Losmandy:tel_rep_write - for %i time.", count);
    }
  if (count == 200)
    {
      syslog (LOG_ERR,
	      "Losmandy:tel_rep_write unsucessful due to incorrect return.");
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
Rts2DevTelescopeGemini::tel_normalize (double *ra, double *dec)
{
  *ra = ln_range_degrees (*ra);
  if (*dec < -90)
    *dec = floor (*dec / 90) * -90 + *dec;	//normalize dec
  if (*dec > 90)
    *dec = *dec - floor (*dec / 90) * 90;
}

/*! 
 * Set losmandy right ascenation.
 *
 * @param ra		right ascenation to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_write_ra (double ra)
{
  char command[14];
  int h, m, s;
  ra = ra / 15;
  dtoints (ra, &h, &m, &s);
  if (snprintf (command, 14, "#:Sr%02d:%02d:%02d#", h, m, s) < 0)
    return -1;
  syslog (LOG_INFO, "command: %s", command);
  return tel_rep_write (command);
}

/*! 
 * Set losmandy declination.
 *
 * @param dec		declination to set in decimal degrees
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_write_dec (double dec)
{
  char command[15];
  int h, m, s;
  dtoints (dec, &h, &m, &s);
  if (snprintf (command, 15, "#:Sd%+02d*%02d:%02d#", h, m, s) < 0)
    return -1;
  return tel_rep_write (command);
}

Rts2DevTelescopeGemini::Rts2DevTelescopeGemini (int argc, char **argv):Rts2DevTelescope (argc,
		  argv)
{
  device_file = "/dev/ttyS0";

  addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
}

Rts2DevTelescopeGemini::~Rts2DevTelescopeGemini ()
{
  close (tel_desc);
}

int
Rts2DevTelescopeGemini::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      device_file = optarg;
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
  return 0;
}

/*!
 * Init telescope, connect on given port.
 * 
 * @return 0 on succes, -1 & set errno otherwise
 */
int
Rts2DevTelescopeGemini::init ()
{
  struct termios tel_termios;
  char rbuf[10];
  int ret;

  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

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
  tel_termios.c_cc[VTIME] = 15;

  if (tcsetattr (tel_desc, TCSANOW, &tel_termios) < 0)
    return -1;

  tel_gemini_reset ();

  // 12/24 hours trick..
  if (tel_write_read ("#:Gc#", 5, rbuf, 5) < 0)
    return -1;
  rbuf[5] = 0;
  if (strncmp (rbuf, "(24)#", 5))
    return -1;

  // we get 12:34:4# while we're in short mode
  // and 12:34:45 while we're in long mode
  //if (tel_write_read_hash ("#:Gr#", 5, rbuf, 9) < 0)
  //  return -1;
  if (rbuf[7] == '\0')
    {
      // that could be used to distinguish between long
      // short mode
      // we are in short mode, set the long on
      if (tel_write_read ("#:U#", 5, rbuf, 0) < 0)
	return -1;
      // now set low precision, e.g. we won't wait for user
      // to find a star
      strcpy (rbuf, "HIGH");
      while (strncmp ("HIGH", rbuf, 4))
	{
	  if (tel_write_read ("#:P#", 4, rbuf, 4) < 0)
	    return -1;
	}
    }
  return 0;
}

int
Rts2DevTelescopeGemini::ready ()
{
  return 0;
}

/*!
 * Reads information about telescope.
 *
 */
int
Rts2DevTelescopeGemini::baseInfo ()
{
  int32_t gem_type;
  if (tel_read_longtitude () || tel_read_latitude ())
    return -1;
  tel_gemini_get (0, &gem_type);
  switch (gem_type)
    {
    case 1:
      strcpy (telType, "GM8");
      break;
    case 2:
      strcpy (telType, "G-11");
      break;
    case 3:
      strcpy (telType, "HGM-200");
      break;
    case 4:
      strcpy (telType, "CI700");
      break;
    case 5:
      strcpy (telType, "Titan");
      break;
    default:
      sprintf (telType, "UNK %2i", gem_type);
    }
  strcpy (telSerialNumber, "000001");
  telAltitude = 600;

  telParkDec = PARK_DEC;

  return 0;
}

int
Rts2DevTelescopeGemini::info ()
{
  double ha;
  telFlip = 0;
  if (tel_read_ra () || tel_read_dec ()
      || tel_read_siderealtime () || tel_read_localtime ())
    return -1;
  ha = ln_range_degrees (telSiderealTime * 15.0 - telRa);
  telFlip = (ha < 180.0 ? 1 : 0);
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
Rts2DevTelescopeGemini::tel_set_rate (char new_rate)
{
  char command[6];
  sprintf (command, "#:R%c#", new_rate);
  return tel_write (command, 5);
}

/*! 
 * Start slew. Again for completness?
 * 
 * @see tel_set_rate() for speed.
 *
 * @param direction 	direction
 * 
 * @return -1 on failure & set errnom, 5 (>=0) otherwise
 */
int
Rts2DevTelescopeGemini::telescope_start_move (char direction)
{
  char command[6];
  tel_set_rate (RATE_SLEW);
  sprintf (command, "#:M%c#", direction);
  return tel_write (command, 5) == 1 ? -1 : 0;
}

/*! 
 * Stop sleew. Again only for completness?
 * 
 * @see tel_start_slew for direction.	
 */
int
Rts2DevTelescopeGemini::telescope_stop_move (char direction)
{
  char command[6];
  sprintf (command, "#:Q%c#", direction);
  return tel_write (command, 5) < 0 ? -1 : 0;
}

/*! 
 * Move telescope to new location, wait for completition, then send
 * message.
 *
 * @param tar_ra		target right ascenation
 * @param tar_dec		target declination
 * 
 * @return -1 on error, 0 otherwise
 */
int
Rts2DevTelescopeGemini::startMove (double tar_ra, double tar_dec)
{
  char retstr;

  tel_normalize (&tar_ra, &tar_dec);

  if ((tel_write_ra (tar_ra) < 0) || (tel_write_dec (tar_dec) < 0))
    return -1;
  if (tel_write_read ("#:MS#", 5, &retstr, 1) < 0)
    return -1;
  if (retstr == '0')
    return 0;
  return -1;
}

/*! 
 * Check, if telescope is still moving
 *
 * @return -1 on error, 0 if telescope isn't moving, 1 if telescope is moving
 */
int
Rts2DevTelescopeGemini::isMoving ()
{
  int32_t status;
  if (tel_gemini_get (99, &status) < 0)
    return -1;
  if (status & 8)
    return 100;
  return -2;
}

int
Rts2DevTelescopeGemini::isParked ()
{
  char buf = '2';
  int ret;
  ret = tel_write_read ("#:h?#", 5, &buf, 1);
  if (buf != '2')
    return -2;
  return ret;
}

int
Rts2DevTelescopeGemini::endMove ()
{
  int32_t track;
  tel_gemini_get (130, &track);
  syslog (LOG_INFO, "rate: %i", track);
  tel_gemini_set (131, 131);
  tel_gemini_get (130, &track);
  tel_write ("#:ONtest", 8);
}

/*! 
 * Set telescope to match given coordinates
 *
 * This function is mainly used to tell the telescope, where it
 * actually is at the beggining of observation
 * 
 * @param ra		setting right ascennation
 * @param dec		setting declination
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::setTo (double set_ra, double set_dec)
{
  char readback[101];

  tel_normalize (&set_ra, &set_dec);

  if ((tel_write_ra (set_ra) < 0) || (tel_write_dec (set_dec) < 0))
    return -1;
  if (tel_write_read_hash ("#:CM#", 5, readback, 100) < 0)
    return -1;
  return 0;
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
Rts2DevTelescopeGemini::correct (double cor_ra, double cor_dec)
{
  double ra_act, dec_act;

  if (tel_read_ra () || tel_read_dec ())
    return -1;

  ra_act -= telRa;
  dec_act -= telDec;

  return setTo (ra_act, dec_act);
}

int
Rts2DevTelescopeGemini::change (double chng_ra, double chng_dec)
{
#define GEM_RA_DIV  411		// value for Gemini RA div
  int ret;
  info ();
  if (fabs (chng_dec) < 87)
    chng_ra = chng_dec / cos (ln_deg_to_rad (telDec));
  // decide, if we make change, or move using move command
  if (fabs (chng_ra) > 1 && fabs (chng_dec) > 1)
    {
      ret = startMove (telRa + chng_ra, telDec + chng_dec);
    }
  else
    {
      char direction;
      struct timespec sec;
      sec.tv_sec = 0;
      sec.tv_nsec = 0;
      // center rate
      tel_set_rate (RATE_CENTER);
      if (chng_ra != 0)
	{
	  // first - RA direction
	  if (chng_ra > 0)
	    {
	      // slew speed to 1 - 0.25 arcmin / sec
	      tel_gemini_set (GEM_RA_DIV, 256);
	      sec.tv_sec = (long) ((fabs (chng_ra) * 60.0) / 5.0) * 1000000;
	      nanosleep (&sec, NULL);
	      tel_gemini_set (GEM_RA_DIV, TCM_DEFAULT_RATE);
	    }
	  else
	    {
	      tel_gemini_set (GEM_RA_DIV, 65535);
	      sec.tv_sec = (long) ((fabs (chng_ra) * 60.0) / 5.0) * 1000000;
	      nanosleep (&sec, NULL);
	      tel_gemini_set (GEM_RA_DIV, TCM_DEFAULT_RATE);
	    }
	}
      if (chng_dec != 0)
	{
	  // second - dec direction
	  tel_gemini_set (170, 20);
	  // slew speed to 20 - 5 arcmin / sec
	  direction = chng_dec > 0 ? 'n' : 's';
	  telescope_start_move (direction);
	  sec.tv_nsec = (long) ((fabs (chng_dec) * 60.0) / 5.0) * 1000000;
	  nanosleep (&sec, NULL);
	  telescope_stop_move (direction);
	}
    }
  return info ();
}

/*!
 * Stop telescope slewing at any direction
 *
 * @return 0 on success, -1 and set errno on error
 */
int
Rts2DevTelescopeGemini::stop ()
{
  char dirs[] = { 'e', 'w', 'n', 's' };
  int i;
  for (i = 0; i < 4; i++)
    if (telescope_stop_move (dirs[i]) < 0)
      return -1;
  return 0;
}

/*! 
 * Park telescope to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
int
Rts2DevTelescopeGemini::park ()
{
  char buf = '2';
  int count = 0;
  tel_gemini_reset ();
  tel_gemini_match_time ();
  tel_write ("#:hP#", 5);
  if (tel_write_read ("#:h?#", 5, &buf, 1) != 1)
    return -1;
  count++;
}

int save_registers[] = {
  120,				// manual slewing speed
  140,				// goto slewing speed
  150,				// guiding speed
  170,				// centering speed
  200,				// TVC stepout
  201,				// modeling parameter A (polar axis misalignment in azimuth) in seconds of arc
  202,				// modeling parameter E (polar axis misalignment in elevation) in seconds of arc
  203,				// modeling parameter NP (axes non-perpendiculiraty at the pole) in seconds of arc
  204,				// modeling parameter NE (axes non-perpendiculiraty at the Equator) in seconds of arc
  205,				// modeling parameter IH (index error in hour angle) in seconds of arc
  206,				// modeling parameter ID (index error in declination) in seconds of arc
  207,				// modeling parameter FR (mirror flop/gear play in RE) in seconds of arc
  208,				// modeling parameter FD (mirror flop/gear play in declination) in seconds of arc
  209,				// modeling paremeter CF (counterweight & RA axis flexure) in seconds of arc
  211, -1
};				// modeling parameter TF (tube flexure) in seconds of arc

int
Rts2DevTelescopeGemini::save ()
{
  int *reg = save_registers;
  int32_t val;
  FILE *config_file;

  config_file = fopen (".losmandy.ini", "w");

  while (*reg >= 0)
    {
      tel_gemini_get (*reg, &val);
      fprintf (config_file, "%i:%i\n", *reg, val);
    }
  fclose (config_file);
  return 0;
}

extern int
Rts2DevTelescopeGemini::load ()
{
  return 0;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevTelescopeGemini *device = new Rts2DevTelescopeGemini (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize telescope - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}

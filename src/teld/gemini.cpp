#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*! 
 * @file Driver file for LOSMANDY Gemini telescope systems 
 * 
 * @author petr 
 */

#include <ctype.h>
#include <signal.h>
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

#include <libnova/libnova.h>

#include "telescope.h"
#include "status.h"
#include "../utils/hms.h"

// uncomment following line, if you want all port read logging (will
// add about 10 30-bytes lines to syslog for every query). 
// #define DEBUG_ALL_PORT_COMM

#define RATE_SLEW	'S'
#define RATE_FIND	'M'
#define RATE_CENTER	'C'
#define RATE_GUIDE	'G'
#define PORT_TIMEOUT	5

#define TCM_DEFAULT_RATE	32768

#define SEARCH_STEPS	16

int arr[] = { 0, 1, 2, 3 };

struct
{
  short raDiv;
  short decDiv;
} searchDirs[SEARCH_STEPS] =
{
  {
  0, 1},
  {
  0, -1},
  {
  0, -1},
  {
  0, 1},
  {
  1, 0},
  {
  -1, 0},
  {
  -1, 0},
  {
  1, 0},
  {
  1, 1},
  {
  -1, -1},
  {
  -1, -1},
  {
  1, 1},
  {
  -1, 1},
  {
  1, -1},
  {
  1, -1},
  {
-1, 1}};

class Rts2DevTelescopeGemini:public Rts2DevTelescope
{
private:
  char *device_file_io;
  char *geminiConfig;
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
  int tel_gemini_getch (int id, char *in_buf);
  int tel_gemini_setch (int id, char *in_buf);
  int tel_gemini_set (int id, int32_t val);
  int tel_gemini_set (int id, double val);
  int tel_gemini_get (int id, int32_t * val);
  int tel_gemini_get (int id, double *val);
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

  int tel_set_rate (char new_rate);
  int telescope_start_move (char direction);
  int telescope_stop_move (char direction);

  void telescope_stop_goto ();

  int geminiInit ();

  double fixed_ha;
  int fixed_ntries;

  int startMoveFixedReal ();

  int forcedReparking;		// used to count reparking, when move fails
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
  struct timeval changeTimeRa;
  struct timeval changeTimeDec;

  void clearSearch ();
  // execute next search step, if necessary
  int changeSearch ();

  int searchStep;
  long searchUsecTime;

  short lastSearchRaDiv;
  short lastSearchDecDiv;
  struct timeval nextSearchStep;

  int32_t worm;
  int worm_move_needed;		// 1 if we need to put mount to 130 tracking (we are performing RA move)
public:
    Rts2DevTelescopeGemini (int argc, char **argv);
    virtual ~ Rts2DevTelescopeGemini (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int idle ();
  virtual int changeMasterState (int new_state);
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int endMove ();
  virtual int stopMove ();
  virtual int startMoveFixed (double tar_ha, double tar_dec);
  virtual int isMovingFixed ();
  virtual int endMoveFixed ();
  virtual int startSearch ();
  virtual int isSearching ();
  virtual int stopSearch ();
  virtual int endSearch ();
  virtual int startPark ();
  virtual int isParking ();
  virtual int endPark ();
  int setTo (double set_ra, double set_dec, int appendModel);
  virtual int setTo (double set_ra, double set_dec);
  virtual int correctOffsets (double cor_ra, double cor_dec, double real_ra,
			      double real_dec);
  virtual int correct (double cor_ra, double cor_dec, double real_ra,
		       double real_dec);
  virtual int change (double chng_ra, double chng_dec);
  virtual int saveModel ();
  virtual int loadModel ();
  virtual int stopWorm ();
  virtual int startWorm ();
  virtual int resetMount (resetStates reset_state);
  virtual int startDir (char *dir);
  virtual int stopDir (char *dir);
  virtual int startGuide (char dir, double dir_dist);
  virtual int stopGuide (char dir);
  virtual int stopGuideAll ();
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
#ifdef DEBUG_ALL_PORT_COMM
      syslog (LOG_DEBUG, "read_from: %i size:%i\n", tel_desc, ret);
#endif
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
  int retr = 0;
  buf[0] = 0;

  for (readed = 0; readed < count;)
    {
      if (tel_read (&buf[readed], 1) < 0)
	{
	  buf[readed] = 0;
	  syslog (LOG_DEBUG, "Losmandy: Hash-read error:'%s'", buf);
	  // we get something, let's wait a bit for all data to
	  // flow-in so we can really flush them
	  if (*buf)
	    sleep (5);
	  tcflush (tel_desc, TCIOFLUSH);
	  // try to read something in case we flush partialy full
	  // buffer; sometimes something hang somewhere, and to keep
	  // communication going, we need to be sure to read as much
	  // as possible..
	  if (*buf && retr < 2)
	    retr++;
	  else
	    return -1;
	}
      else
	{
	  if (buf[readed] == '#')
	    break;
	  readed++;
	}
    }
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

//  if (tcflush (tel_desc, TCIOFLUSH) < 0)
//    return -1;
  if (tel_write (wbuf, wcount) < 0)
    return -1;

  tmp_rcount = tel_read (rbuf, rcount);

  if (tmp_rcount > 0)
    {
      buf = (char *) malloc (rcount + 1);
      memcpy (buf, rbuf, rcount);
      buf[rcount] = 0;
      syslog (LOG_DEBUG, "Losmandy:readed %i '%s'", tmp_rcount, buf);
      free (buf);
      return 0;
    }
  syslog (LOG_DEBUG, "Losmandy:readed returns %i", tmp_rcount);
  return -1;
}

int
Rts2DevTelescopeGemini::tel_write_read (char *buf, int wcount, char *rbuf,
					int rcount)
{
  int ret;
  ret = tel_write_read_no_reset (buf, wcount, rbuf, rcount);
  if (ret < 0)
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

//  if (tcflush (tel_desc, TCIOFLUSH) < 0)
//    return -1;
  if (tel_write (wbuf, wcount) < 0)
    return -1;

  tmp_rcount = tel_read_hash (rbuf, rcount);
  if (tmp_rcount < 0)
    {
      tel_gemini_reset ();
      return -1;
    }

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
  unsigned char checksum = 0;
  for (; *buf; buf++)
    checksum ^= *buf;
  checksum %= 128;		// modulo 128
  checksum += 64;
  return checksum;
}

/*!
 * Write command to set gemini local parameters
 *
 * @param id	id to set
 * @param buf	value to set (char buffer; should be 15 char long)
 *
 * @return -1 on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_gemini_setch (int id, char *in_buf)
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
  ret = tel_write (buf, len);
  free (buf);
  if (ret == len)
    return 0;
  return -1;
}

/*!
 * Write command to set gemini local parameters
 *
 * @param id	id to set
 * @param val	value to set
 *
 * @return -1 on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_gemini_set (int id, int32_t val)
{
  char buf[15];
  int len;
  len = sprintf (buf, ">%i:%i", id, val);
  buf[len] = tel_gemini_checksum (buf);
  len++;
  buf[len] = '#';
  len++;
  buf[len] = '\0';
  if (tel_write (buf, len) > 0)
    return 0;
  return -1;
}

int
Rts2DevTelescopeGemini::tel_gemini_set (int id, double val)
{
  char buf[15];
  int len;
  len = sprintf (buf, ">%i:%f0.1", id, val);
  buf[len] = tel_gemini_checksum (buf);
  len++;
  buf[len] = '#';
  len++;
  buf[len] = '\0';
  if (tel_write (buf, len) > 0)
    return 0;
  return -1;
}


/*!
 * Read gemini local parameters
 *
 * @param id	id to get
 * @param buf	pointer where to store result (char[15] buf)
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
Rts2DevTelescopeGemini::tel_gemini_getch (int id, char *buf)
{
  char *ptr, checksum;
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
      syslog (LOG_ERR, "invalid gemini checksum: should be '%c', is '%c'",
	      tel_gemini_checksum (buf), checksum);
      if (*buf || checksum)
	sleep (5);
      tcflush (tel_desc, TCIOFLUSH);
      *buf = '\0';
      return -1;
    }
  return 0;
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
  for (ptr = buf; *ptr && (isdigit (*ptr) || *ptr == '.' || *ptr == '-');
       ptr++);
  checksum = *ptr;
  *ptr = '\0';
  if (tel_gemini_checksum (buf) != checksum)
    {
      syslog (LOG_ERR, "invalid gemini checksum: should be '%c', is '%c'",
	      tel_gemini_checksum (buf), checksum);
      if (*buf)
	sleep (5);
      tcflush (tel_desc, TCIOFLUSH);
      return -1;
    }
  *val = atol (buf);
  return 0;
}

int
Rts2DevTelescopeGemini::tel_gemini_get (int id, double *val)
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
  for (ptr = buf; *ptr && (isdigit (*ptr) || *ptr == '.' || *ptr == '-');
       ptr++);
  checksum = *ptr;
  *ptr = '\0';
  if (tel_gemini_checksum (buf) != checksum)
    {
      syslog (LOG_ERR, "invalid gemini checksum: should be '%c', is '%c'",
	      tel_gemini_checksum (buf), checksum);
      if (*buf)
	sleep (5);
      tcflush (tel_desc, TCIOFLUSH);
      return -1;
    }
  *val = atof (buf);
  return 0;
}

/*!
 * Reset and home losmandy telescope
 */
int
Rts2DevTelescopeGemini::tel_gemini_reset ()
{
  char rbuf[50];

  // write_read_hash
  if (tcflush (tel_desc, TCIOFLUSH) < 0)
    return -1;

  if (tel_write ("\x06", 1) < 0)
    return -1;

  if (tel_read_hash (rbuf, 47) < 1)
    return -1;

  if (*rbuf == 'b')		// booting phase, select warm reboot
    {
      switch (nextReset)
	{
	case RESET_RESTART:
	  tel_write ("bR#", 3);
	  break;
	case RESET_WARM_START:
	  tel_write ("bW#", 3);
	  break;
	case RESET_COLD_START:
	  tel_write ("bC#", 3);
	  break;
	}
      nextReset = RESET_RESTART;
      setTimeout (USEC_SEC);
    }
  else if (*rbuf != 'G')
    {
      // something is wrong, reset all comm
      sleep (10);
      tcflush (tel_desc, TCIOFLUSH);
    }
  return -1;
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
  if (matchCount)
    return 0;
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
  // read blanck
  ret = tel_read_hash (buf, 26);
  if (ret)
    {
      matchCount = 1;
      return ret;
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
  int ret;
  ret = tel_read_hms (&telLongtitude, "#:Gg#");
  if (ret)
    return ret;
  telLongtitude = -1 * telLongtitude;
  return ret;
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
  telSiderealTime = get_loc_sid_time ();
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
  struct ln_dms dh;
  char sign = '+';
  if (dec < 0)
    {
      sign = '-';
      dec = -1 * dec;
    }
  ln_deg_to_dms (dec, &dh);
  if (snprintf
      (command, 15, "#:Sd%c%02d*%02d:%02.0f#", sign, dh.degrees, dh.minutes,
       dh.seconds) < 0)
    return -1;
  return tel_rep_write (command);
}

Rts2DevTelescopeGemini::Rts2DevTelescopeGemini (int in_argc, char **in_argv):Rts2DevTelescope (in_argc,
		  in_argv)
{
  device_file = "/dev/ttyS0";
  geminiConfig = "/etc/rts2/gemini.ini";
  bootesSensors = 0;

  addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
  addOption ('c', "config_file", 1, "config file (with model parameters)");
  addOption ('b', "bootes", 0, "BOOTES G-11 (with 1/0 pointing sensor)");

  lastMotorState = 0;
  telMotorState = TEL_OK;
  infoCount = 0;
  matchCount = 0;
  tel_desc = -1;

  worm = 0;
  worm_move_needed = 0;

  timerclear (&changeTimeRa);
  timerclear (&changeTimeDec);

  clearSearch ();
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
    case 'c':
      geminiConfig = optarg;
      break;
    case 'b':
      bootesSensors = 1;
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelescopeGemini::geminiInit ()
{
  char rbuf[10];

  tel_gemini_reset ();

  // 12/24 hours trick..
  if (tel_write_read ("#:Gc#", 5, rbuf, 5) < 0)
    return -1;
  rbuf[5] = 0;
  if (strncmp (rbuf, "(24)#", 5))
    return -1;

  // we get 12:34:4# while we're in short mode
  // and 12:34:45 while we're in long mode
  if (tel_write_read_hash ("#:GR#", 5, rbuf, 9) < 0)
    return -1;
  if (rbuf[7] == '\0')
    {
      // that could be used to distinguish between long
      // short mode
      // we are in short mode, set the long on
      if (tel_write_read ("#:U#", 5, rbuf, 0) < 0)
	return -1;
    }
  tel_write_read_hash ("#:Ml#", 5, rbuf, 1);
  if (bootesSensors)
    tel_gemini_set (311, 15);	// reset feature port
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
  int ret;

  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

  while (1)
    {
      syslog (LOG_DEBUG, "Rts2DevTelescopeGemini::init open: %s",
	      device_file);

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
      tel_termios.c_cc[VTIME] = 40;

      if (tcsetattr (tel_desc, TCSANOW, &tel_termios) < 0)
	return -1;

      ret = geminiInit ();
      if (!ret)
	return ret;

      close (tel_desc);		// try again
      sleep (60);
    }

  return 0;
}

int
Rts2DevTelescopeGemini::idle ()
{
  int ret;
  tel_gemini_get (130, &worm);
  ret = tel_gemini_get (99, &lastMotorState);
  // if moving, not on limits & need info..
  if ((lastMotorState & 8) && (!(lastMotorState & 16)) && infoCount % 25 == 0)
    {
      info ();
    }
  infoCount++;
  if (telMotorState == TEL_OK
      && ((ret && (getState (0) & TEL_MASK_MOVING) == TEL_MASK_MOVING)
	  || (lastMotorState & 16)))
    {
      stopMove ();
      resetMount (RESET_RESTART);
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
		  if ((getState (0) & TEL_MASK_MOVING) == TEL_MOVING)
		    {
		      // not ideal; lastMoveRa contains (possibly corrected) move values
		      // but we don't care much about that as we have reparked..
		      setTarget (lastMoveRa, lastMoveDec);
		      tel_start_move ();
		      tel_gemini_get (130, &worm);
		      ret = tel_gemini_get (99, &lastMotorState);
		      if (ret)
			{
			  // still not responding, repark
			  resetMount (RESET_RESTART);
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
  return Rts2DevTelescope::idle ();
}

int
Rts2DevTelescopeGemini::changeMasterState (int new_state)
{
  matchCount = 0;
  return Rts2DevTelescope::changeMasterState (new_state);
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
  char buf[5];
  int ret;
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
    case 6:
      strcpy (telType, "Titan50");
      break;
    default:
      sprintf (telType, "UNK_%2i", gem_type);
    }
  ret = tel_write_read ("#:GV#", 5, buf, 3);
  if (ret)
    return -1;
  buf[4] = '\0';
  strcat (telType, "_");
  strcat (telType, buf);
  strcpy (telSerialNumber, "000001");
  telAltitude = 600;

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
  if (bootesSensors)
    {
      int feature;
      tel_gemini_get (311, &feature);
      telAxis[0] = (double) ((feature & 1) ? 1 : 0);
      telAxis[1] = (double) ((feature & 2) ? 1 : 0);
      telFlip = (feature & 2) ? 1 : 0;
    }
  else
    {
      telFlip = (ha < 180.0 ? 1 : 0);
    }
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
  int ret;
  // start worm if moving in RA DEC..
  if (worm == 135 && (direction == DIR_EAST || direction == DIR_WEST))
    {
      // G11 (version 3.x) have some problems with starting worm..give it some time
      // to react
      startWorm ();
      usleep (USEC_SEC / 10);
      startWorm ();
      usleep (USEC_SEC / 10);
      worm_move_needed = 1;
    }
  sprintf (command, "#:M%c#", direction);
  ret = tel_write (command, 5) == 1 ? -1 : 0;
  // workaround suggested by Rene Goerlich
  if (worm == 135)
    stopWorm ();
  return ret;
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
  if (worm_move_needed && (direction == DIR_EAST || direction == DIR_WEST))
    {
      worm_move_needed = 0;
      stopWorm ();
    }
  return tel_write (command, 5) < 0 ? -1 : 0;
}

void
Rts2DevTelescopeGemini::telescope_stop_goto ()
{
  tel_gemini_get (99, &lastMotorState);
  tel_write ("#:Q#", 4);
  if (lastMotorState & 8)
    {
      lastMotorState &= ~8;
      tel_gemini_set (99, lastMotorState);
    }
  if (worm_move_needed)
    {
      worm_move_needed = 0;
    }
}

int
Rts2DevTelescopeGemini::tel_start_move ()
{
  char retstr;
  char buf[55];

  telescope_stop_goto ();

  if ((tel_write_ra (lastMoveRa) < 0) || (tel_write_dec (lastMoveDec) < 0))
    return -1;
  if (tel_write_read ("#:MS#", 5, &retstr, 1) < 0)
    return -1;

  if (retstr == '0')
    {
      return 0;
    }
  // otherwise read reply..
  tel_read_hash (buf, 53);
  if (retstr == '3')		// manual control..
    return 0;
  return -1;
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
  tel_normalize (&tar_ra, &tar_dec);

  lastMoveRa = tar_ra;
  lastMoveDec = tar_dec;

  if (telMotorState != TEL_OK)	// lastMoveRa && lastMoveDec will bring us to correct location after we finish rebooting/reparking
    return 0;

  startWorm ();

  forcedReparking = 0;

  time (&moveTimeout);
  moveTimeout += 300;

  return tel_start_move ();
}

/*! 
 * Check, if telescope is still moving
 *
 * @return -1 on error, 0 if telescope isn't moving, 100 if telescope is moving
 */
int
Rts2DevTelescopeGemini::isMoving ()
{
  int ret;
  struct timeval now;
  if (telMotorState != TEL_OK)
    return USEC_SEC;
  gettimeofday (&now, NULL);
  // change move..
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
	    }
	}
      if (changeTimeDec.tv_sec > 0 || changeTimeDec.tv_usec > 0)
	{
	  if (timercmp (&changeTimeDec, &now, <))
	    {
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
      return 0;
    }
  if (now.tv_sec > moveTimeout)
    {
      stopMove ();
      return -2;
    }
  if (lastMotorState & 8)
    return USEC_SEC / 10;
  return -2;
}

int
Rts2DevTelescopeGemini::endMove ()
{
  int32_t track;
  tel_gemini_get (130, &track);
  syslog (LOG_INFO, "rate: %i", track);
  tel_gemini_set (131, 1);
  tel_gemini_get (130, &track);
  setTimeout (USEC_SEC);
  if (tel_write ("#:ONtest#", 9) > 0)
    return 0;
  return -1;
}

int
Rts2DevTelescopeGemini::stopMove ()
{
  telescope_stop_goto ();
  timerclear (&changeTimeRa);
  timerclear (&changeTimeDec);
  return Rts2DevTelescope::stopMove ();
}

int
Rts2DevTelescopeGemini::startMoveFixedReal ()
{
  int ret;

  // compute ra
  tel_read_siderealtime ();

  lastMoveRa = telSiderealTime * 15.0 - fixed_ha;

  tel_normalize (&lastMoveRa, &lastMoveDec);

  if (telMotorState != TEL_OK)	// same as for startMove - after repark. we will get to correct location
    return 0;

  stopWorm ();

  time (&moveTimeout);
  moveTimeout += 300;

  ret = tel_start_move ();
  if (!ret)
    fixed_ntries++;
  return ret;
}

int
Rts2DevTelescopeGemini::startMoveFixed (double tar_ha, double tar_dec)
{
  fixed_ha = tar_ha;
  lastMoveDec = tar_dec;
  fixed_ntries = 0;

  return startMoveFixedReal ();
}

int
Rts2DevTelescopeGemini::isMovingFixed ()
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
      pos1.ra = fixed_ha + telSiderealTime * 15.0;
      pos1.dec = lastMoveDec;

      pos2.ra = telRa;
      pos2.dec = telDec;
      sep = ln_get_angular_separation (&pos1, &pos2);
      if (sep > 15 / 60 / 4)	// 15 seconds..
	{
	  syslog (LOG_DEBUG,
		  "Rts2DevTelescopeGemini::isMovingFixed sep: %f arcsec",
		  sep * 3600);
	  // reque move..
	  ret = startMoveFixedReal ();
	  if (ret)		// end in case of error
	    return -2;
	  return USEC_SEC;
	}
      return ret;
    }
  return ret;
}

int
Rts2DevTelescopeGemini::endMoveFixed ()
{
  int32_t track;
  stopWorm ();
  tel_gemini_get (130, &track);
  syslog (LOG_DEBUG, "endMoveFixed track: %i", track);
  setTimeout (USEC_SEC);
  if (tel_write ("#:ONfixed#", 10) > 0)
    return 0;
  return -1;
}

void
Rts2DevTelescopeGemini::clearSearch ()
{
  searchStep = 0;
  searchUsecTime = 0;
  lastSearchRaDiv = 0;
  lastSearchDecDiv = 0;
  timerclear (&nextSearchStep);
  worm_move_needed = 0;
}

int
Rts2DevTelescopeGemini::startSearch ()
{
  int ret;
  char *searchSpeedCh;
  clearSearch ();
  if (searchSpeed > 0.8)
    searchSpeed = 0.8;
  else if (searchSpeed < 0.2)
    searchSpeed = 0.2;
  // maximal guiding speed
  asprintf (&searchSpeedCh, "%lf", searchSpeed);
  ret = tel_gemini_setch (150, searchSpeedCh);
  free (searchSpeedCh);
  if (ret)
    return -1;
  ret = tel_gemini_set (170, 1);
  if (ret)
    return -1;
  ret = tel_set_rate (RATE_GUIDE);
  searchUsecTime =
    (long int) (USEC_SEC * (searchRadius * 3600 / (searchSpeed * 15.0)));
  return changeSearch ();
}

int
Rts2DevTelescopeGemini::changeSearch ()
{
  int ret;
  struct timeval add;
  struct timeval now;
  if (telMotorState != TEL_OK)
    return -1;
  if (searchStep >= SEARCH_STEPS)
    {
      // that will stop move in all directions
      telescope_stop_goto ();
      return -2;
    }
  ret = 0;
  gettimeofday (&now, NULL);
  // change in RaDiv..
  if (lastSearchRaDiv != searchDirs[searchStep].raDiv)
    {
      // stop old in RA
      if (lastSearchRaDiv == 1)
	{
	  ret = telescope_stop_move (DIR_WEST);
	}
      else if (lastSearchRaDiv == -1)
	{
	  ret = telescope_stop_move (DIR_EAST);
	}
      if (ret)
	return -1;
    }
  // change in DecDiv..
  if (lastSearchDecDiv != searchDirs[searchStep].decDiv)
    {
      // stop old in DEC
      if (lastSearchDecDiv == 1)
	{
	  ret = telescope_stop_move (DIR_NORTH);
	}
      else if (lastSearchDecDiv == -1)
	{
	  ret = telescope_stop_move (DIR_SOUTH);
	}
      if (ret)
	return -1;
    }
  // send current location
  infoAll ();

  // change to new..
  searchStep++;
  add.tv_sec = searchUsecTime / USEC_SEC;
  add.tv_usec = searchUsecTime % USEC_SEC;
  timeradd (&now, &add, &nextSearchStep);

  if (lastSearchRaDiv != searchDirs[searchStep].raDiv)
    {
      lastSearchRaDiv = searchDirs[searchStep].raDiv;
      // start new in RA
      if (lastSearchRaDiv == 1)
	{
	  ret = telescope_start_move (DIR_WEST);
	}
      else if (lastSearchRaDiv == -1)
	{
	  ret = telescope_start_move (DIR_EAST);
	}
      if (ret)
	return -1;
    }
  if (lastSearchDecDiv != searchDirs[searchStep].decDiv)
    {
      lastSearchDecDiv = searchDirs[searchStep].decDiv;
      // start new in DEC
      if (lastSearchDecDiv == 1)
	{
	  ret = telescope_start_move (DIR_NORTH);
	}
      else if (lastSearchDecDiv == -1)
	{
	  ret = telescope_start_move (DIR_SOUTH);
	}
      if (ret)
	return -1;
    }
  return 0;
}

int
Rts2DevTelescopeGemini::isSearching ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  if (timercmp (&now, &nextSearchStep, >))
    {
      return changeSearch ();
    }
  return 100;
}

int
Rts2DevTelescopeGemini::stopSearch ()
{
  clearSearch ();
  telescope_stop_goto ();
  return Rts2DevTelescope::stopSearch ();
}

int
Rts2DevTelescopeGemini::endSearch ()
{
  clearSearch ();
  telescope_stop_goto ();
  return Rts2DevTelescope::endSearch ();
}

int
Rts2DevTelescopeGemini::isParking ()
{
  char buf = '3';
  if (telMotorState != TEL_OK)
    return USEC_SEC;
  if (lastMotorState & 8)
    return USEC_SEC;
  tel_write_read ("#:h?#", 5, &buf, 1);
  switch (buf)
    {
    case '2':
    case ' ':
      return USEC_SEC;
    case '1':
      return -2;
    case '0':
      syslog (LOG_ERR,
	      "Rts2DevTelescopeGemini::isParking called without park command");
    default:
      return -1;
    }
}

int
Rts2DevTelescopeGemini::endPark ()
{
  if (getMasterState () != SERVERD_NIGHT)
    tel_gemini_match_time ();
  setTimeout (USEC_SEC * 10);
  return stopWorm ();
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
Rts2DevTelescopeGemini::setTo (double set_ra, double set_dec, int appendModel)
{
  char readback[101];

  tel_normalize (&set_ra, &set_dec);

  if ((tel_write_ra (set_ra) < 0) || (tel_write_dec (set_dec) < 0))
    return -1;

  if (appendModel)
    {
      if (tel_write_read_hash ("#:Cm#", 5, readback, 100) < 0)
	return -1;
    }
  else
    {
      int32_t v205;
      int32_t v206;
      int32_t v205_new;
      int32_t v206_new;
      tel_gemini_get (205, &v205);
      tel_gemini_get (206, &v206);
      if (tel_write_read_hash ("#:CM#", 5, readback, 100) < 0)
	return -1;
      tel_gemini_get (205, &v205_new);
      tel_gemini_get (206, &v206_new);
    }
  return 0;
}

int
Rts2DevTelescopeGemini::setTo (double set_ra, double set_dec)
{
  return setTo (set_ra, set_dec, 0);
}

int
Rts2DevTelescopeGemini::correctOffsets (double cor_ra, double cor_dec,
					double real_ra, double real_dec)
{
  int32_t v205;
  int32_t v206;
  int ret;
  // change allign parameters..
  tel_gemini_get (205, &v205);
  tel_gemini_get (206, &v206);
  v205 += (int) (cor_ra * 3600);	// convert from degrees to arcminutes
  v206 += (int) (cor_dec * 3600);
  tel_gemini_set (205, v205);
  ret = tel_gemini_set (206, v206);
  if (ret)
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
 * This routine can be called from teld.cpp only when we haven't moved
 * from last location. However, we recalculate angular separation and
 * check if we are closer then 5 degrees to be 100% sure we will not
 * load mess to the telescope.
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int
Rts2DevTelescopeGemini::correct (double cor_ra, double cor_dec,
				 double real_ra, double real_dec)
{
  struct ln_equ_posn pos1;
  struct ln_equ_posn pos2;
  double sep;
  int ret;

  if (tel_read_ra () || tel_read_dec ())
    return -1;

  // do not change if we are too close to poles
  if (fabs (telDec) > 85)
    return -1;

  telRa -= cor_ra;
  telDec -= cor_dec;

  // do not change if we are too close to poles
  if (fabs (telDec) > 85)
    return -1;

  pos1.ra = telRa;
  pos1.dec = telDec;
  pos2.ra = real_ra;
  pos2.dec = real_dec;

  sep = ln_get_angular_separation (&pos1, &pos2);
  syslog (LOG_DEBUG, "Rts2DevTelescopeGemini::correct separation: %f", sep);
  if (sep > 5)
    return -1;

  ret = setTo (real_ra, real_dec, getNumCorr () < 2 ? 0 : 1);
  if (ret)
    {
      // mount correction failed - do local one

    }
  return ret;
}

int
Rts2DevTelescopeGemini::change (double chng_ra, double chng_dec)
{
  int ret;
  long u_sleep;
  struct timeval now;
  ret = info ();
  if (ret)
    return ret;
  syslog (LOG_INFO, "Rts2DevTelescopeGemini::change before: ra %f dec %f",
	  telRa, telDec);
  if (fabs (chng_dec) < 87)
    chng_ra = chng_ra / cos (ln_deg_to_rad (chng_dec));
  syslog (LOG_INFO, "Losmandy: calculated ra %f dec: %f", chng_ra, chng_dec);
  // decide, if we make change, or move using move command
  if (fabs (chng_ra) > 3 / 60.0 && fabs (chng_dec) > 3 / 60.0)
    {
      int c = 0;
      ret = startMove (telRa + chng_ra, telDec + chng_dec);
      if (ret)
	return ret;
      // move ewait ended .. log results
      syslog (LOG_DEBUG, "Rts2DevTelescopeGemini::change move: %i c: %i", ret,
	      c);
    }
  else
    {
      ret = -1;
      char direction;
      // center rate
      ret = tel_set_rate (RATE_CENTER);
      if (ret == -1)
	return ret;
      ret = tel_gemini_set (170, 1);
      if (ret == -1)
	return ret;
      if (chng_ra != 0)
	{
	  // first - RA direction
	  // slew speed to 1 - 0.25 arcmin / sec
	  direction = chng_ra > 0 ? DIR_EAST : DIR_WEST;
	  u_sleep = (long) (((fabs (chng_ra) * 60.0) * 4.0) * USEC_SEC +
			    USEC_SEC / 10.0);
	  changeTimeRa.tv_sec = (long) (u_sleep / USEC_SEC);
	  changeTimeRa.tv_usec = (long) (u_sleep - changeTimeRa.tv_sec);
	  ret = telescope_start_move (direction);
	  gettimeofday (&now, NULL);
	  timeradd (&changeTimeRa, &now, &changeTimeRa);
	}
      if (chng_dec != 0)
	{
	  // slew speed to 20 - 5 arcmin / sec
	  direction = chng_dec > 0 ? 'n' : 's';
	  u_sleep = (long) ((fabs (chng_dec) * 60.0) * 4.0 * USEC_SEC);
	  changeTimeDec.tv_sec = (long) (u_sleep / USEC_SEC);
	  changeTimeDec.tv_usec = (long) (u_sleep - changeTimeDec.tv_sec);
	  ret = telescope_start_move (direction);
	  gettimeofday (&now, NULL);
	  timeradd (&changeTimeDec, &now, &changeTimeDec);
	}
    }
  if (ret == -1)
    return -1;
  return 0;
}

/*! 
 * Park telescope to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
int
Rts2DevTelescopeGemini::startPark ()
{
  int ret;
  if (telMotorState != TEL_OK)
    return -1;

  stopMove ();

  ret = tel_write ("#:hP#", 5);
  if (ret <= 0)
    return -1;
  return 0;
}

static int save_registers[] = {
  0,				// mount type
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
  211,				// modeling parameter TF (tube flexure) in seconds of arc 
  411,				// RA track divisor 
  412,				// DEC tracking divisor
  -1
};

int
Rts2DevTelescopeGemini::saveModel ()
{
  int *reg = save_registers;
  char buf[10];
  FILE *config_file;

  config_file = fopen (geminiConfig, "w");
  if (!config_file)
    {
      syslog (LOG_ERR,
	      "Rts2DevTelescopeGemini::saveModel cannot open file '%s' : %m",
	      geminiConfig);
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

extern int
Rts2DevTelescopeGemini::loadModel ()
{
  FILE *config_file;
  char *line;
  size_t numchar;
  int id;
  int ret;

  config_file = fopen (geminiConfig, "r");
  if (!config_file)
    {
      syslog (LOG_ERR,
	      "Rts2DevTelescopeGemini::loadModel cannot open file '%s' : %m",
	      geminiConfig);
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
	    syslog (LOG_ERR,
		    "Rts2DevTelescopeGemini::loadModel setch return: %i on '%s'",
		    ret, buf);
	  free (buf);
	}
      else
	{
	  syslog (LOG_ERR,
		  "Rts2DevTelescopeGemini::loadModel invalid line: '%s'",
		  line);
	}
    }
  free (line);

  return 0;
}

int
Rts2DevTelescopeGemini::stopWorm ()
{
  worm = 135;
  return tel_gemini_set (135, 1);
}

int
Rts2DevTelescopeGemini::startWorm ()
{
  worm = 131;
  return tel_gemini_set (131, 1);
}

int
Rts2DevTelescopeGemini::resetMount (resetStates reset_mount)
{
  int ret;
  ret = tel_gemini_set (65535, 65535);
  if (ret)
    return ret;
  return Rts2DevTelescope::resetMount (reset_mount);
}

int
Rts2DevTelescopeGemini::startDir (char *dir)
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
Rts2DevTelescopeGemini::stopDir (char *dir)
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

int
Rts2DevTelescopeGemini::startGuide (char dir, double dir_dist)
{
  int ret;
  switch (dir)
    {
    case DIR_EAST:
    case DIR_WEST:
    case DIR_NORTH:
    case DIR_SOUTH:
      tel_set_rate (RATE_GUIDE);
      // set smallest rate..
      tel_gemini_set (150, 0.2);
      ret = telescope_start_move (dir);
      if (ret)
	return ret;
      return Rts2DevTelescope::startGuide (dir, dir_dist);
    }
  return -2;
}

int
Rts2DevTelescopeGemini::stopGuide (char dir)
{
  int ret;
  switch (dir)
    {
    case DIR_EAST:
    case DIR_WEST:
    case DIR_NORTH:
    case DIR_SOUTH:
      ret = telescope_stop_move (dir);
      if (ret)
	return ret;
      return Rts2DevTelescope::stopGuide (dir);
    }
  return -2;
}

int
Rts2DevTelescopeGemini::stopGuideAll ()
{
  telescope_stop_goto ();
  return Rts2DevTelescope::stopGuideAll ();
}


Rts2DevTelescopeGemini *device;

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
  device = new Rts2DevTelescopeGemini (argc, argv);

  signal (SIGINT, killSignal);
  signal (SIGTERM, killSignal);

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

/*! 
 * @file Driver file for LOSMANDY Gemini telescope systems 
 * 
 * $Id$
 * 
 * Based on LX200 driver, which is based on c library for xephem writen by Ken
 * Shouse <ken@kshouse.engine.swri.edu> and modified by Carlos Guirao
 * <cguirao@eso.org>
 *
 * This is JUST A DRIVER, you shall consult daemon for network interface.
 *
 * Use semaphores to control file acces.
 *
 * @author petr 
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/time.h>
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

#include <sys/ipc.h>
#include <sys/sem.h>

#include "../telescope.h"
#include "../../utils/hms.h"
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

int port = -1;

//! sempahore id structure, we have two semaphores...
int semid = -1;

//! one semaphore for direct port control..
#define SEM_TEL 	0
//! and second for control of the move operations
#define SEM_MOVE 	1

//! telescope parking declination
#define PARK_DEC	0

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun
{
  int val;			/* value for SETVAL */
  struct semid_ds *buf;		/* buffer for IPC_STAT, IPC_SET */
  unsigned short int *array;	/* array for GETALL, SETALL */
  struct seminfo *__buf;	/* buffer for IPC_INFO */
};
#endif

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
tel_read (char *buf, int count)
{
  int readed;

  for (readed = 0; readed < count; readed++)
    {
      int ret = read (port, &buf[readed], 1);
      if (ret == 0)
	{
	  errno = ETIMEDOUT;
	  ret = -1;
	}
      if (ret < 0)
	{
	  syslog (LOG_DEBUG, "LX200: port read error %m");
	  errno = EIO;
	  return -1;
	}
#ifdef DEBUG_ALL_PORT_COMM
      syslog (LOG_DEBUG, "LX200: readed '%c'", buf[readed]);
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
tel_read_hash (char *buf, int count)
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
  syslog (LOG_DEBUG, "LX200:Hash-readed:'%s'", buf);
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
tel_write (char *buf, int count)
{
  syslog (LOG_DEBUG, "LX200:will write:'%s'", buf);
  return write (port, buf, count);
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
tel_write_read (char *wbuf, int wcount, char *rbuf, int rcount)
{
  int tmp_rcount = -1;
  struct sembuf sem_buf;
  char *buf;

  sem_buf.sem_num = SEM_TEL;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  if (semop (semid, &sem_buf, 1) < 0)
    return -1;
  if (tcflush (port, TCIOFLUSH) < 0)
    goto unlock;		// we need to unlock
  if (tel_write (wbuf, wcount) < 0)
    goto unlock;

  tmp_rcount = tel_read (rbuf, rcount);
  if (tmp_rcount > 0)
    {
      buf = (char *) malloc (rcount + 1);
      memcpy (buf, rbuf, rcount);
      buf[rcount] = 0;
      syslog (LOG_DEBUG, "LX200:readed %i %s", tmp_rcount, buf);
      free (buf);
    }
  else
    {
      syslog (LOG_DEBUG, "LX200:readed returns %i", tmp_rcount);
    }

unlock:

  sem_buf.sem_op = 1;

  if (semop (semid, &sem_buf, 1) < 0)
    {
      syslog (LOG_EMERG, "LX200:Cannot perform semop in tel_write_read:%m");
      return -1;
    }

  return tmp_rcount;
}

/*! 
 * Combine write && read_hash together.
 *
 * @see tel_write_read for definition
 */
int
tel_write_read_hash (char *wbuf, int wcount, char *rbuf, int rcount)
{
  int tmp_rcount = -1;
  struct sembuf sem_buf;

  sem_buf.sem_num = SEM_TEL;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  if (semop (semid, &sem_buf, 1) < 0)
    return -1;
  if (tcflush (port, TCIOFLUSH) < 0)
    goto unlock;		// we need to unlock
  if (tel_write (wbuf, wcount) < 0)
    goto unlock;

  tmp_rcount = tel_read_hash (rbuf, rcount);

unlock:

  sem_buf.sem_op = 1;

  if (semop (semid, &sem_buf, 1) < 0)
    {
      syslog (LOG_EMERG,
	      "LX200:Cannot perform semop in tel_write_read_hash:%m");
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
tel_read_hms (double *hmsptr, char *command)
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
 * Computes gemini checksum
 *
 * @param buf checksum to compute
 * 
 * @return computed checksum
 */
char
tel_gemini_checksum (const char *buf)
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
tel_gemini_set (int id, int val)
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
tel_gemini_get (int id, int *val)
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
      errno = ENODEV;
      return -1;
    }
  *val = atoi (buf);
  return 0;
}

/*!
 * Reset and home losmandy telescope
 */
int
tel_gemini_reset (void)
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
tel_gemini_match_time (void)
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
      errno = -ENODEV;
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
tel_read_ra (double *raptr)
{
  if (tel_read_hms (raptr, "#:GR#"))
    return -1;
  *raptr *= 15.0;
  return 0;
}

/*!
 * Reads LX200 declination.
 * 
 * @param decptr	where dec will be stored
 * 
 * @return -1 and set errno on error, otherwise 0
 */
int
tel_read_dec (double *decptr)
{
  return tel_read_hms (decptr, "#:GD#");
}

/*! 
 * Returns LX200 local time.
 * 
 * @param tptr		where time will be stored
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
tel_read_localtime (double *tptr)
{
  return tel_read_hms (tptr, "#:GL#");
}

/*! 
 * Returns LX200 sidereal time.
 * 
 * @param tptr		where time will be stored
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
tel_read_siderealtime (double *tptr)
{
  *tptr = 0;
  return 0;
}

/*! 
 * Reads LX200 latitude.
 * 
 * @param latptr	here latitude will be stored  
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
tel_read_latitude (double *tptr)
{
  return tel_read_hms (tptr, "#:Gt#");
}

/*! 
 * Reads LX200 longtitude.
 * 
 * @param latptr	where longtitude will be stored  
 * 
 * @return -1 and errno on error, otherwise 0
 */
int
tel_read_longtitude (double *tptr)
{
  return tel_read_hms (tptr, "#:Gg#");
}

/*! 
 * Repeat LX200 write.
 * 
 * Handy for setting ra and dec.
 * Meade tends to have problems with that.
 *
 * @param command	command to write on port
 */
int
tel_rep_write (char *command)
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
      syslog (LOG_DEBUG, "LX200:tel_rep_write - for %i time.", count);
    }
  if (count == 200)
    {
      syslog (LOG_ERR,
	      "LX200:tel_rep_write unsucessful due to incorrect return.");
      errno = EIO;
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
tel_normalize (double *ra, double *dec)
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
 * Set LX200 right ascenation.
 *
 * @param ra		right ascenation to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
tel_write_ra (double ra)
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
  syslog (LOG_INFO, "command: %s", command);
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
tel_write_dec (double dec)
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

/*!
 * Init telescope, connect on given port.
 * 
 * @param device_name		pointer to device name
 * @param telescope_id		id of telescope, for LX200 ignored
 * 
 * @return 0 on succes, -1 & set errno otherwise
 */
extern int
telescope_init (const char *device_name, int telescope_id)
{
  struct termios tel_termios;
  union semun sem_un;
  unsigned short int sem_arr[] = { 1, 1 };
  char rbuf[10];

  if (port < 0)
    port = open (device_name, O_RDWR);
  if (port < 0)
    return -1;

  if ((semid = semget (ftok (device_name, 0), 2, 0644)) == -1)
    {
      if ((semid = semget (ftok (device_name, 0), 2, IPC_CREAT | 0644)) == -1)
	{
	  syslog (LOG_ERR, "semget: %m");
	  return -1;
	}
      sem_un.array = sem_arr;

      if (semctl (semid, 0, SETALL, sem_un) < 0)
	{
	  syslog (LOG_ERR, "semctl init: %m");
	  return -1;
	}

      if (tcgetattr (port, &tel_termios) < 0)
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

      if (tcsetattr (port, TCSANOW, &tel_termios) < 0)
	{
	  syslog (LOG_ERR, "tcsetattr: %m");
	  return -1;
	}
      tel_gemini_reset ();
    }

  syslog (LOG_DEBUG, "LX200:Initialization complete");

  // 12/24 hours trick..
  if (tel_write_read ("#:Gc#", 5, rbuf, 5) < 0)
    return -1;
  rbuf[5] = 0;
  if (strncmp (rbuf, "(24)#", 5))
    {
      errno = EIO;
      return -1;
    }

  // we get 12:34:4# while we're in short mode
  // and 12:34:45 while we're in long mode
  //if (tel_write_read_hash ("#:Gr#", 5, rbuf, 9) < 0)
  //  return -1;
  if (rbuf[7] == '\0')
    {
      struct sembuf sem_buf;
      sem_buf.sem_num = SEM_MOVE;
      sem_buf.sem_op = -1;
      sem_buf.sem_flg = SEM_UNDO;
      if (semop (semid, &sem_buf, 1) == -1)
	return -1;
      // that could be used to distinguish between long
      // short mode
      // we are in short mode, set the long on
      if (tel_write_read ("#:U#", 5, rbuf, 0) < 0)
	goto sem_high_err;
      // now set low precision, e.g. we won't wait for user
      // to find a star
      strcpy (rbuf, "HIGH");
      while (strncmp ("HIGH", rbuf, 4))
	{
	  if (tel_write_read ("#:P#", 4, rbuf, 4) < 0)
	    goto sem_high_err;
	}
      sem_buf.sem_op = +1;
      semop (semid, &sem_buf, 1);

      return 0;

    sem_high_err:

      sem_buf.sem_op = +1;
      semop (semid, &sem_buf, 1);
      return -1;
    }
  return 0;
}

/*!
 * Detach from telescope
 */
extern void
telescope_done ()
{
  semctl (semid, 1, IPC_RMID);
  syslog (LOG_DEBUG, "lx200: telescope_done called %i", semid);
}

/*!
 * Reads information about telescope.
 *
 */
extern int
telescope_base_info (struct telescope_info *info)
{
  int type;
  if (tel_read_longtitude (&info->longtitude)
      || tel_read_latitude (&info->latitude))
    return -1;
  tel_gemini_get (0, &type);
  switch (type)
    {
    case 1:
      strcpy (info->type, "GM8");
      break;
    case 2:
      strcpy (info->type, "G-11");
      break;
    case 3:
      strcpy (info->type, "HGM-200");
      break;
    case 4:
      strcpy (info->type, "CI700");
      break;
    case 5:
      strcpy (info->type, "Titan");
      break;
    default:
      sprintf (info->type, "UNK %2i", type);
    }
  strcpy (info->serial_number, "000001");
  info->altitude = 600;

  info->park_dec = PARK_DEC;

  return 0;
}

extern int
telescope_info (struct telescope_info *info)
{
  info->flip = 0;
  if (tel_read_ra (&info->ra) || tel_read_dec (&info->dec)
      || tel_read_siderealtime (&info->siderealtime)
      || tel_read_localtime (&info->localtime))
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
tel_set_rate (char new_rate)
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
telescope_start_move (char direction)
{
  char command[6];
  tel_set_rate (RATE_FIND);
  sprintf (command, "#:M%c#", direction);
  return tel_write (command, 5) == 1 ? -1 : 0;
}

/*! 
 * Stop sleew. Again only for completness?
 * 
 * @see tel_start_slew for direction.	
 */
int
telescope_stop_move (char direction)
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
 * @exception EINVAL 	when telescope is below horizont, or upper limit was reached
 *
 * @return -1 and errno on exception, otherwise 0
 */
int
tel_slew_to (double ra, double dec)
{
  char retstr;

  tel_normalize (&ra, &dec);

  if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
    return -1;
  if (tel_write_read ("#:MS#", 5, &retstr, 1) < 0)
    return -1;
  if (retstr == '0')
    return 0;
  retstr = 0;
  errno = EINVAL;
  return -1;
}

/*! 
 * Check, if telescope is still moving
 *
 * @return -1 and errno on exception, 0 if telescope isn't moving, 1 if goto is performed,
 */
int
tel_is_still (void)
{
  int status;
  if (tel_gemini_get (99, &status) < 0)
    return -1;
  return !!(status & 8);
}

/*! 
 * Move telescope to new location, wait for completition, then send
 * message.
 *
 * @param ra		target right ascenation
 * @param dec		target declination
 * 
 * @return -1 and errno on error, 0 otherwise
 * 
 * @execption ETIMEDOUT when timeout occurs
 */
int
tel_move_to (double ra, double dec)
{
  int timeout;
  int ret, track;
  struct sembuf sem_buf;

  sem_buf.sem_num = SEM_MOVE;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  syslog (LOG_INFO, "LX200:tel move_to ra:%f dec:%f", ra, dec);

  if (semop (semid, &sem_buf, 1) == -1)
    return -1;

  sem_buf.sem_op = 1;

  if ((ret = tel_slew_to (ra, dec)) < 0)
    {
      goto unlock;
    }
  timeout = time (NULL) + 100;	// it's quite reasonably to hope that call will not fail
  while ((time (NULL) <= timeout) && (tel_is_still ()))
    sleep (2);
  // wait for a bit, telescope get confused?? when you check for ra 
  // and dec often and sometime doesn't move to required location
  // this is still just a theory, which waits for final proof

  if (time (NULL) > timeout)
    {
      syslog (LOG_ERR, "LX200:Timeout during moving to ra:%f dec:%f.", ra, dec);	// mayby will be handy to add call to tel_get_ra+dec
      // to obtain current ra and dec
      if (semop (semid, &sem_buf, 1) == -1)
	{
	  syslog (LOG_EMERG, "LX200:Cannot perform semop in tel_move_to:%m");
	  return -1;
	}
      errno = ETIMEDOUT;
      return -1;
    }
  tel_gemini_get (130, &track);
  syslog (LOG_INFO, "rate: %i", track);
  tel_gemini_set (131, 131);
  tel_gemini_get (130, &track);
  tel_write ("#:ONtest", 8);
  syslog (LOG_INFO, "rate: %i", track);
  sleep (1);
  // need to sleep a while, waiting for final precise adjustement 
  // of the scope, which could be checked by checkcoords - the scope 
  // newer gets to precise position, it just get to something about 
  // that, so we could program checkcoords to check for precise position.  
  // Also good to wait till the things settle down.
unlock:

  if (semop (semid, &sem_buf, 1) < 0)
    {
      syslog (LOG_EMERG, "LX200:Cannot perform semop in tel_move_to:%m");
      return -1;
    }

  syslog (LOG_DEBUG, "LX200:tel_move_to ra:%f dec:%f returns %i", ra, dec,
	  ret);
  return ret;
}

/*! 
 * Repeat move_to 5 times, with a bit changed location.
 *
 * Used to overcome LX200 error.
 *
 * @see tel_move_to
 */
int
telescope_move_to (double ra, double dec)
{
  int i;
  for (i = 0; i < 2; i++)
    if (!tel_move_to (ra, dec))
      {
	return 0;		// we finished move without error
      }
    else
      {
	if (errno != ETIMEDOUT)
	  {
	    return -1;		// some error occured
	  }
	// when we have ETIMEDOUT, let's try again.
	ra = ra + i * 0.05;	// magic value, most often helps
      }
  // when we got so far, something realy wrong must happen
  syslog (LOG_EMERG, "LX200:Too much timeouts during moving to ra:%f dec:%f",
	  ra, dec);
  return -1;
}

/*!
 * Internal, set to new coordinates.
 */
int
tel_set_to (double ra, double dec)
{
  char readback[101];

  tel_normalize (&ra, &dec);

  if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
    return -1;
  if (tel_write_read_hash ("#:CM#", 5, readback, 100) < 0)
    return -1;
  return 0;
}

/*! 
 * Set telescope to match given coordinates
 *
 * This function is mainly used to tell the telescope, where it
 * actually is at the beggining of observation (remember, that lx200
 * doesn't have absolute position sensors)
 * 
 * @param ra		setting right ascennation
 * @param dec		setting declination
 * 
 * @return -1 and set errno on error, otherwise 0
 */
extern int
telescope_set_to (double ra, double dec)
{
  struct sembuf sem_buf;
  sem_buf.sem_num = SEM_MOVE;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  syslog (LOG_INFO, "LX200:tel set_to ra:%f dec:%f", ra, dec);

  if (semop (semid, &sem_buf, 1) == -1)
    return -1;

  sem_buf.sem_op = 1;

  if (tel_set_to (ra, dec))
    {
      semop (semid, &sem_buf, 1);
      return -1;
    }

  semop (semid, &sem_buf, 1);

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
extern int
telescope_correct (double ra, double dec)
{
  struct sembuf sem_buf;
  double ra_act, dec_act;

  sem_buf.sem_num = SEM_MOVE;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  if (semop (semid, &sem_buf, 1) == -1)
    return -1;

  syslog (LOG_INFO, "LX200:tel corect_to ra:%f dec:%f", ra, dec);

  sem_buf.sem_op = 1;

  if (tel_read_ra (&ra_act) || tel_read_dec (&dec_act))
    goto err;

  ra_act -= ra;
  dec_act -= dec;

  if (tel_set_to (ra_act, dec_act))
    goto err;

  semop (semid, &sem_buf, 1);
  return 0;

err:
  if (semop (semid, &sem_buf, 1))
    syslog (LOG_ERR, "+1 op with %i failed", sem_buf.sem_num);
  return -1;
}

/*!
 * Stop telescope slewing at any direction
 *
 * @return 0 on success, -1 and set errno on error
 */
extern int
telescope_stop ()
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
 * Park telescope to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
extern int
telescope_park ()
{
  char buf = '2';
  int count = 0;
  tel_gemini_reset ();
  tel_write ("#:hP#", 5);
  tel_gemini_set (135, 135);
  tel_gemini_match_time ();
  while (buf == '2' && count < 200)
    {
      sleep (1);
      if (tel_write_read ("#:h?#", 5, &buf, 1) != 1)
	return -1;
      count++;
    }
  return count == 200 ? -1 : 0;
}

/*!
 * TODO implemet it!!!!
 */
extern int
telescope_off ()
{
  return 0;
}

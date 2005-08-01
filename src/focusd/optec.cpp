/*!
 * $Id$
 *
 * @author standa
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#define BAUDRATE B19200
#define FOCUSER_PORT "/dev/ttyS0"

struct termios oldtio, newtio;

static char *focuser_port = NULL;

#include "focuser.h"

class Rts2DevFocuserOptec:public Rts2DevFocuser
{
private:
  char *device_file_io;
  int foc_desc;
  int status;
  // low-level I/O functions
  int foc_read (char *buf, int count);
  int foc_read_hash (char *buf, int count);
  int foc_write (char *buf, int count);
  int foc_write_read_no_reset (char *wbuf, int wcount, char *rbuf,
			       int rcount);
  int foc_write_read (char *buf, int wcount, char *rbuf, int rcount);

  // high-level I/O functions
  int getPos (int *position);
  int getTemp (float *temperature);
public:
    Rts2DevFocuserOptec (int argc, char **argv);
   ~Rts2DevFocuserOptec (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int stepOut (int num);
  virtual int isFocusing ();
};

/*! 
 * Reads some data directly from port.
 *
 * Log all flow as LOG_DEBUG to syslog
 *
 * @exception EIO when there aren't data from port
 *
 * @param buf           buffer to read in data
 * @param count         how much data will be readed
 *
 * @return -1 on failure, otherwise number of read data 
 */

int
Rts2DevFocuserOptec::foc_read (char *buf, int count)
{
  int readed;

  for (readed = 0; readed < count; readed++)
    {
      int ret = read (foc_desc, &buf[readed], 1);
      printf ("read_from: %i size:%i\n", foc_desc, ret);
      if (ret <= 0)
	{
	  return -1;
	}
#ifdef DEBUG_ALL_PORT_COMM
      syslog (LOG_DEBUG, "Optec: readed '%c'", buf[readed]);
#endif
    }
  return readed;
}

/*! 
 * Will write on telescope port string.   
 *
 * @exception EIO, .. common write exceptions 
 *  
 * @param buf           buffer to write
 * @param count         count to write
 *  
 * @return -1 on failure, count otherwise
 */
int
Rts2DevFocuserOptec::foc_write (char *buf, int count)
{
  int ret;
  syslog (LOG_DEBUG, "Optec:will write:'%s'", buf);
  ret = write (foc_desc, buf, count);
  tcflush (foc_desc, TCIFLUSH);
  return ret;
}

/*!
 * Combine write && read together.
 *
 * Flush port to clear any gargabe.
 *
 * @exception EINVAL and other exceptions
 *
 * @param wbuf          buffer to write on port
 * @param wcount        write count
 * @param rbuf          buffer to read from port
 * @param rcount        maximal number of characters to read
 *
 * @return -1 and set errno on failure, rcount otherwise
 */
int
Rts2DevFocuserOptec::foc_write_read_no_reset (char *wbuf, int wcount,
					      char *rbuf, int rcount)
{
  int tmp_rcount = -1;
  char *buf;

  if (tcflush (foc_desc, TCIOFLUSH) < 0)
    return -1;

  if (foc_write (wbuf, wcount) < 0)
    return -1;

  tmp_rcount = foc_read (rbuf, rcount);

  if (tmp_rcount > 0)
    {
      buf = (char *) malloc (rcount + 1);
      memcpy (buf, rbuf, rcount);
      buf[rcount] = 0;
      syslog (LOG_DEBUG, "Optec:readed %i %s", tmp_rcount, buf);
      free (buf);
    }
  else
    {
      syslog (LOG_DEBUG, "Optec:readed returns %i", tmp_rcount);
    }
}

int
Rts2DevFocuserOptec::foc_write_read (char *buf, int wcount, char *rbuf,
				     int rcount)
{
  int ret;

  ret = foc_write_read_no_reset (buf, wcount, rbuf, rcount);

  if (ret <= 0)
    {
      ret = foc_write_read_no_reset (buf, wcount, rbuf, rcount);
    }
  return ret;
}



Rts2DevFocuserOptec::Rts2DevFocuserOptec (int argc, char **argv):Rts2DevFocuser (argc,
		argv)
{
  device_file = FOCUSER_PORT;

  addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
}

Rts2DevFocuserOptec::~Rts2DevFocuserOptec ()
{
  close (foc_desc);
}

int
Rts2DevFocuserOptec::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      device_file = optarg;
      break;
    default:
      return Rts2DevFocuser::processOption (in_opt);
    }
  return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int
Rts2DevFocuserOptec::init ()
{
  struct termios foc_termios;
  char rbuf[10];
  int ret;

  syslog (LOG_DEBUG, "init");

  ret = Rts2DevFocuser::init ();

  if (ret)
    return ret;

  syslog (LOG_DEBUG, "open port");

  foc_desc = open (device_file, O_RDWR);

  if (foc_desc < 0)
    return -1;

  if (tcgetattr (foc_desc, &foc_termios) < 0)
    return -1;

  if (cfsetospeed (&foc_termios, BAUDRATE) < 0 ||
      cfsetispeed (&foc_termios, BAUDRATE) < 0)
    return -1;

  foc_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
  foc_termios.c_oflag = 0;
  foc_termios.c_cflag =
    ((foc_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
  foc_termios.c_lflag = 0;
  foc_termios.c_cc[VMIN] = 0;
  foc_termios.c_cc[VTIME] = 15;

  if (tcsetattr (foc_desc, TCSANOW, &foc_termios) < 0)
    return -1;

  // set manual
  if (foc_write_read ("FMMODE", 6, rbuf, 1) < 0)
    return -1;
  syslog (LOG_DEBUG, "write read");
  if (rbuf[0] != '!')
    return -1;

  return 0;
}

int
Rts2DevFocuserOptec::getPos (int *position)
{
  char command[6], rbuf[6], tbuf[6];

  if (foc_write_read ("FPOSRO", 6, rbuf, 6) < 1)
    return -1;
  else
    {
      tbuf[0] = rbuf[2];
      tbuf[1] = rbuf[3];
      tbuf[2] = rbuf[4];
      tbuf[3] = rbuf[5];
      tbuf[4] = '\0';
      *position = atoi (tbuf);
    }
  return 0;
}

int
Rts2DevFocuserOptec::getTemp (float *temp)
{
  char command[6], rbuf[6], tbuf[6];

  if (foc_write_read ("FTMPRO", 6, rbuf, 6) < 1)
    return -1;
  else
    {
      tbuf[0] = rbuf[2];
      tbuf[1] = rbuf[3];
      tbuf[2] = rbuf[4];
      tbuf[3] = rbuf[5];
      tbuf[4] = '\0';
      *temp = atof (tbuf);
    }
  return 0;
}

int
Rts2DevFocuserOptec::ready ()
{
  return 0;
}

int
Rts2DevFocuserOptec::baseInfo ()
{
  strcpy (focType, "OPTEC_TCF");
  return 0;
}

int
Rts2DevFocuserOptec::info ()
{
  int ret;
  ret = getPos (&focPos);
  if (ret)
    return ret;
  ret = getTemp (&focTemp);
  if (ret)
    return ret;
  return 0;
}

int
Rts2DevFocuserOptec::stepOut (int num)
{
  char command[7], rbuf[2];
  char add = ' ';
  int ret;

  ret = getPos (&focPos);
  if (ret)
    return ret;

  if (focPos + num > 7000 || focPos + num < 0)
    return -1;

  if (num < 0)
    {
      add = 'I';
      num *= -1;
    }
  else
    {
      add = 'O';
    }

  sprintf (command, "F%c%04d", add, num);

  if (foc_write_read (command, 6, rbuf, 1) < 0)
    return -1;
  if (rbuf[0] != '*')
    return -1;

  return 0;
}

int
Rts2DevFocuserOptec::isFocusing ()
{
  // stepout command waits till focusing end
  return -2;
}

int
main (int argc, char **argv)
{
  Rts2DevFocuserOptec *device = new Rts2DevFocuserOptec (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize focuser - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}

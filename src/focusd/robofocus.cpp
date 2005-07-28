
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
#include <ctype.h>

#define BAUDRATE B9600
#define FOCUSER_PORT "/dev/ttyS1"


#define CMD_FOCUS_MOVE_IN		"FI"
#define CMD_FOCUS_MOVE_OUT		"FO"
#define CMD_FOCUS_GET			"FG"
#define CMD_TEMP_GET			"FT"
#define CMD_FOCUS_GOTO			"FG"

struct termios oldtio, newtio;

static char *focuser_port = NULL;

#include "focuser.h"


class Rts2DevFocuserRobofocus:public Rts2DevFocuser
{
private:
  char *device_file_io;
  int foc_desc;
  char checksum;
  int status;
  // low-level I/O functions
  int foc_read (char *buf, int count);
  int foc_write (char *buf, int count);
  int foc_write_read_no_reset (char *wbuf, int wcount, char *rbuf,
			       int rcount);
  int foc_write_read (char *buf, int wcount, char *rbuf, int rcount);
  // high-level I/O functions
  int focus_move (char *cmd, int steps);
  void compute_checksum (char *cmd);
public:
    Rts2DevFocuserRobofocus (int argc, char **argv);
   ~Rts2DevFocuserRobofocus (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int stepOut (int num, int direction);
  virtual int getPos (int *position);
  virtual int getTemp (float *temperature);
  virtual int isFocusing ();
  virtual int focus ();
};


Rts2DevFocuserRobofocus::Rts2DevFocuserRobofocus (int argc, char **argv):
Rts2DevFocuser (argc, argv)
{
  device_file = FOCUSER_PORT;

  addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
}

Rts2DevFocuserRobofocus::~Rts2DevFocuserRobofocus ()
{
  close (foc_desc);
}

/*! 
 * Will write on focuser port string.   
 *
 * @exception EIO, .. common write exceptions 
 *  
 * @param buf           buffer to write
 * @param count         count to write
 *  
 * @return -1 on failure, count otherwise
 */
int
Rts2DevFocuserRobofocus::foc_write (char *buf, int count)
{
  int ret;
  syslog (LOG_DEBUG, "Robofocus:will write:'%s'", buf);
  ret = write (foc_desc, buf, count);
  tcflush (foc_desc, TCIFLUSH);
  return ret;
}

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
Rts2DevFocuserRobofocus::foc_read (char *buf, int count)
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
      syslog (LOG_DEBUG, "Robofocus: readed '%c'", buf[readed]);
#endif
    }
  return readed;
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
Rts2DevFocuserRobofocus::foc_write_read_no_reset (char *wbuf, int wcount,
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
      syslog (LOG_DEBUG, "Robofocus:readed %i %s", tmp_rcount, buf);
      free (buf);
    }
  else
    {
      syslog (LOG_DEBUG, "Robofocus:readed returns %i", tmp_rcount);
    }
}

int
Rts2DevFocuserRobofocus::foc_write_read (char *buf, int wcount, char *rbuf,
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


int
Rts2DevFocuserRobofocus::processOption (int in_opt)
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
Rts2DevFocuserRobofocus::init ()
{
  int ret;

  ret = Rts2DevFocuser::init ();

  if (ret)
    return ret;

  struct termios oldtio, newtio;
  foc_desc = open (device_file, O_RDWR | O_NOCTTY);
  if (foc_desc == -1)
    {
      perror ("focus_open");
      return -1;
    }

  tcgetattr (foc_desc, &oldtio);

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 50;

  tcflush (foc_desc, TCIOFLUSH);
  tcsetattr (foc_desc, TCSANOW, &newtio);

  return 0;

}

int
Rts2DevFocuserRobofocus::ready ()
{
  return 0;
}

int
Rts2DevFocuserRobofocus::baseInfo ()
{
  strcpy (focType, "ROBOFOCUS");
  return 0;
}


int
Rts2DevFocuserRobofocus::info ()
{
  getPos (&focPos);
  getTemp (&focTemp);
  return 0;
}


int
Rts2DevFocuserRobofocus::getPos (int *position)
{
  char command[9], rbuf[9], tbuf[7];
  char command_buffer[8];

// Put together command with neccessary checksum
  sprintf (command_buffer, "FG000000");
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);

  if (foc_write_read (command, 9, rbuf, 9) < 1)
    return -1;
  else
    {
      tbuf[0] = rbuf[2];
      tbuf[1] = rbuf[3];
      tbuf[2] = rbuf[4];
      tbuf[3] = rbuf[5];
      tbuf[4] = rbuf[6];
      tbuf[5] = rbuf[7];
      tbuf[6] = '\0';
      *position = atoi (tbuf);
    }
  return 0;
}


int
Rts2DevFocuserRobofocus::getTemp (float *temp)
{
  char command[9], rbuf[9], tbuf[7];
  char command_buffer[8];

// Put together command with neccessary checksum
  sprintf (command_buffer, "FT000000");
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);


  if (foc_write_read (command, 9, rbuf, 9) < 1)
    return -1;
  else
    {
      tbuf[0] = rbuf[3];
      tbuf[1] = rbuf[4];
      tbuf[2] = rbuf[5];
      tbuf[3] = rbuf[6];
      tbuf[4] = rbuf[7];
      tbuf[5] = '\0';
      *temp = ((atof (tbuf) / 2) - 273.15);	// return temp in Celsius
    }
  return 0;
}


int
Rts2DevFocuserRobofocus::isFocusing ()
{
  return 0;
}


int
Rts2DevFocuserRobofocus::focus ()
{
  // devcli_server_command (NULL, "priority 137");
}

int
Rts2DevFocuserRobofocus::stepOut (int num, int direction)
{

  if (direction == 1)
    return focus_move (CMD_FOCUS_MOVE_IN, num);
  else if (direction == -1)
    return focus_move (CMD_FOCUS_MOVE_OUT, num);
}



int
Rts2DevFocuserRobofocus::focus_move (char *cmd, int steps)
{
  int i;
  char *ticks[1];
  int num_steps;

  char command[10], rbuf[num_steps + 9], tbuf[7];
  char command_buffer[9];

  if (steps == 0)
    {
      return (0);
    }

  // Number of steps moved must account for backlash compensation
  if (strcmp (cmd, CMD_FOCUS_MOVE_OUT) == 0)
    num_steps = steps + 40;
  else
    num_steps = steps;

  ticks[1] = '\0';

  // Pad out command with leading zeros
  sprintf (command_buffer, "%s%06i", cmd, steps);

  //Get checksum character
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);

  // Send command to focuser

  if (foc_write_read (command, 9, rbuf, num_steps + 9) < 1)
    return -1;

  return 0;

}

// Calculate checksum (according to RoboFocus spec.)
void
Rts2DevFocuserRobofocus::compute_checksum (char *cmd)
{
  int i, bytesum = 0;
  unsigned int size;

  size = strlen (cmd);

  for (i = 0; i < size; i++)
    bytesum = bytesum + (int) (cmd[i]);

  checksum = toascii ((bytesum % 340));

  return;

}

int
main (int argc, char **argv)
{
  Rts2DevFocuserRobofocus *device = new Rts2DevFocuserRobofocus (argc, argv);

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

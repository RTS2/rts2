
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

#include "focuser.h"

class Rts2DevFocuserRobofocus:public Rts2DevFocuser
{
private:
  char *device_file_io;
  int foc_desc;
  char checksum;
  int step_num;

  // low-level I/O functions
  int foc_read (char *buf, int count);
  int foc_write (char *buf, int count);
  int foc_write_read_no_reset (char *wbuf, int wcount, char *rbuf,
			       int rcount);
  int foc_write_read (char *buf, int wcount, char *rbuf, int rcount);
  // high-level I/O functions
  int focus_move (char *cmd, int steps);
  void compute_checksum (char *cmd);

  int getPos (int *position);
  int getTemp (float *temperature);
  int getSwitchState ();
protected:
    virtual int processOption (int in_opt);
  virtual int isFocusing ();
public:
    Rts2DevFocuserRobofocus (int argc, char **argv);
   ~Rts2DevFocuserRobofocus (void);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int stepOut (int num);
  virtual int setTo (int num);
  virtual int setSwitch (int switch_num, int new_state);
};

Rts2DevFocuserRobofocus::Rts2DevFocuserRobofocus (int in_argc,
						  char **in_argv):
Rts2DevFocuser (in_argc, in_argv)
{
  device_file = FOCUSER_PORT;

  addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");

  switchNum = 4;
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
  syslog (LOG_DEBUG, "Robofocus:will write:'%s'", buf);
  return write (foc_desc, buf, count);
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

  for (readed = 0; readed < count;)
    {
      int ret = read (foc_desc, &buf[readed], count - readed);
      if (ret <= 0)
	{
	  tcflush (foc_desc, TCIOFLUSH);
	  sleep (1);
	  return -1;
	}
      readed += ret;
    }
#ifdef DEBUG_ALL_PORT_COMM
  syslog (LOG_DEBUG, "Robofocus: readed '%s'", buf[readed]);
#endif
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

  if (foc_write (wbuf, wcount) < 0)
    return -1;

  tmp_rcount = foc_read (rbuf, rcount);

  if (tmp_rcount > 0)
    {
      buf = (char *) malloc (tmp_rcount + 1);
      memcpy (buf, rbuf, rcount);
      buf[rcount] = 0;
      syslog (LOG_DEBUG, "Robofocus:readed %i %s", tmp_rcount, buf);
      free (buf);
    }
  else
    {
      syslog (LOG_DEBUG, "Robofocus:readed returns %i", tmp_rcount);
    }
  return tmp_rcount;
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

  struct termios tio;
  foc_desc = open (device_file, O_RDWR | O_NOCTTY);
  if (foc_desc == -1)
    {
      perror ("focus_open");
      return -1;
    }

  ret = tcgetattr (foc_desc, &tio);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2DevFocuserRobofocus::init tcgetattr %m");
      return -1;
    }

  if (cfsetospeed (&tio, B9600) < 0 || cfsetispeed (&tio, B9600) < 0)
    return -1;

  tio.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
  tio.c_oflag = 0;
  tio.c_cflag = ((tio.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
  tio.c_lflag = 0;
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 40;


  ret = tcsetattr (foc_desc, TCSANOW, &tio);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2DevFocuserRobofocus::init tcsetattr %m");
      return -1;
    }
  tcflush (foc_desc, TCIOFLUSH);

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
  // querry for switch state 
  int state = getSwitchState ();
  if (state >= 0)
    focSwitches = state;
  return 0;
}


int
Rts2DevFocuserRobofocus::getPos (int *position)
{
  char command[10], rbuf[10];
  char command_buffer[9];

// Put together command with neccessary checksum
  sprintf (command_buffer, "FG000000");
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);

  if (foc_write_read (command, 9, rbuf, 9) < 1)
    return -1;
  *position = atoi (rbuf + 2);
  return 0;
}


int
Rts2DevFocuserRobofocus::getTemp (float *temp)
{
  char command[10], rbuf[10];
  char command_buffer[9];

// Put together command with neccessary checksum
  sprintf (command_buffer, "FT000000");
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);

  if (foc_write_read (command, 9, rbuf, 9) < 1)
    return -1;
  *temp = ((atof (rbuf + 2) / 2) - 273.15);	// return temp in Celsius
  return 0;
}

int
Rts2DevFocuserRobofocus::getSwitchState ()
{
  char command[10], rbuf[10];
  char command_buffer[9];
  int ret;

  sprintf (command_buffer, "FP000000");
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);
  if (foc_write_read (command, 9, rbuf, 9) < 1)
    return -1;
  ret = 0;
  for (int i = 0; i < switchNum; i++)
    {
      if (rbuf[i + 4] == '2')
	ret |= (1 << i);
    }
  return ret;
}

int
Rts2DevFocuserRobofocus::stepOut (int num)
{
  if (num < 0)
    return focus_move (CMD_FOCUS_MOVE_IN, -1 * num);
  return focus_move (CMD_FOCUS_MOVE_OUT, num);
}

int
Rts2DevFocuserRobofocus::setTo (int num)
{
  char command[9], command_buf[10];
  sprintf (command, "FG%06i", num);
  compute_checksum (command);
  sprintf (command_buf, "%s%c", command, checksum);
  if (foc_write (command_buf, 9) < 1)
    return -1;
  return 0;
}

int
Rts2DevFocuserRobofocus::setSwitch (int switch_num, int new_state)
{
  char command[10], rbuf[10];
  char command_buffer[9] = "FP001111";
  if (switch_num >= switchNum)
    return -1;
  int state = getSwitchState ();
  if (state < 0)
    return -1;
  for (int i = 0; i < switchNum; i++)
    {
      if (state & (1 << i))
	command_buffer[i + 4] = '2';
    }
  command_buffer[switch_num + 4] = (new_state == 1 ? '2' : '1');
  compute_checksum (command_buffer);
  sprintf (command, "%s%c", command_buffer, checksum);

  if (foc_write_read (command, 9, rbuf, 9) < 1)
    return -1;

  infoAll ();
  return 0;
}

int
Rts2DevFocuserRobofocus::focus_move (char *cmd, int steps)
{
  char command[10];
  char command_buffer[9];

  // Number of steps moved must account for backlash compensation
//  if (strcmp (cmd, CMD_FOCUS_MOVE_OUT) == 0)
//    num_steps = steps + 40;
//  else
//    num_steps = steps;

  // Pad out command with leading zeros
  sprintf (command_buffer, "%s%06i", cmd, steps);

  //Get checksum character
  compute_checksum (command_buffer);

  sprintf (command, "%s%c", command_buffer, checksum);

  // Send command to focuser

  if (foc_write (command, 9) < 1)
    return -1;

  step_num = steps;

  return 0;
}

int
Rts2DevFocuserRobofocus::isFocusing ()
{
  char rbuf[10];
  int ret;
  ret = foc_read (rbuf, 1);
  if (ret == -1)
    return ret;
  // if we get F, read out end command
  printf ("%c\n", *rbuf);
  if (*rbuf == 'F')
    {
      ret = foc_read (rbuf + 1, 8);
      if (ret != 8)
	return -1;
      return -2;
    }
  return 0;
}

// Calculate checksum (according to RoboFocus spec.)
void
Rts2DevFocuserRobofocus::compute_checksum (char *cmd)
{
  int bytesum = 0;
  unsigned int size, i;

  size = strlen (cmd);

  for (i = 0; i < size; i++)
    bytesum = bytesum + (int) (cmd[i]);

  checksum = toascii ((bytesum % 340));
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

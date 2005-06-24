#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "filter_ifw.h"

int
Rts2FilterIfw::writePort (char *buf, size_t len)
{
  int ret;
  ret = write (dev_port, buf, len);
  if (ret != len)
    {
      syslog (LOG_ERR, "Rts2FilterIfw::writePort '%s' %m", buf);
      return -1;
    }
  return 0;
}

int
Rts2FilterIfw::readPort (size_t len)
{
  int ret;
  ret = read (dev_port, filter_buff, len);
  if (ret == -1)
    {
      syslog (LOG_ERR, "Rts2FilterIfw::readPort %m");
      filter_buff[0] = '\0';
      return -1;
    }
  filter_buff[ret] = '\0';
  return 0;
}

int
Rts2FilterIfw::shutdown (void)
{
  int n;

  /* shutdown filter wheel communications */
  n = writePort ("WEXITS\r", 7);

  readPort (4);

  /* Check for correct response from filter wheel */
  if (strcmp (filter_buff, "END"))
    {
      syslog (LOG_ERR, "Rts2FilterIfw::shutdown FILTER WHEEL ERROR: %s",
	      filter_buff);
      tcflush (dev_port, TCIFLUSH);
    }
  else
    {
      tcflush (dev_port, TCIFLUSH);
      syslog (LOG_DEBUG,
	      "Rts2FilterIfw::shutdown Filter wheel shutdown: %s",
	      filter_buff);
    }
  close (dev_port);
  dev_port = -1;
}

Rts2FilterIfw::Rts2FilterIfw (char *in_dev_file)
{
  dev_file = in_dev_file;
  dev_port = -1;
}

Rts2FilterIfw::~Rts2FilterIfw (void)
{
  if (dev_port)
    {
      shutdown ();
    }
}

int
Rts2FilterIfw::init (void)
{
  struct termios options;	/* structure to hold serial port configuration */
  int n;

  int ret;

  dev_port = open (dev_file, O_RDWR | O_NOCTTY | O_NDELAY);

  if (dev_port == -1)
    {
      syslog (LOG_ERR, "Rts2FilterIfw::init cannot open '%s': %m", dev_file);
      return -1;
    }
  ret = fcntl (dev_port, F_SETFL, 0);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2FilterIfw::init cannot fcntl: %m");
    }
  /* get current serial port configuration */
  if (tcgetattr (dev_port, &options) < 0)
    {
      syslog (LOG_ERR,
	      "Rts2FilterIfw::init error reading serial port configuration: %m");
      return -1;
    }

  /*
   * Set the baud rates to 19200...
   */
  if (cfsetospeed (&options, B19200) < 0
      || cfsetispeed (&options, B19200) < 0)
    {
      syslog (LOG_ERR, "Rts2FilterIfw::init error setting baud rate: %m");
      return -1;
    }

  /*
   * Enable the receiver and set local mode...
   */
  options.c_cflag |= (CLOCAL | CREAD);

  /* Choose raw input */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  /* set port to 8 data bits, 1 stop bit, no parity */
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;

  /* set timeout  to 10 seconds */
  options.c_cc[VTIME] = 80;
  options.c_cc[VMIN] = 0;

  /*
   * Set the new options for the port...
   */
  tcsetattr (dev_port, TCSANOW, &options);

  /* initialise filter wheel */
  n = writePort ("WSMODE\r", 7);
  if (n == -1)
    return n;

  readPort (4);

  /* Check for correct response from filter wheel */
  if (filter_buff[0] != '!')
    {
      syslog (LOG_DEBUG, "Rts2FilterIfw::init FILTER WHEEL ERROR: %s",
	      filter_buff);
      tcflush (dev_port, TCIFLUSH);
      return -1;
    }
  syslog (LOG_DEBUG, "Rts2FilterIfw::init Filter wheel initialised: %s",
	  filter_buff);
  tcflush (dev_port, TCIFLUSH);
  return 0;
}

int
Rts2FilterIfw::getFilterNum (void)
{
  int filter_number;
  int n;
  int read_result;
  char filter_name[1];
  char buffer[1];

  n = writePort ("WFILTR\r", 7);
  if (n == -1)
    return -1;

  readPort (4);

  if (strstr (filter_buff, "ER"))
    {
      syslog (LOG_DEBUG, "Rts2FilterIfw::getFilterNum FILTER WHEEL ERROR: %s",
	      filter_buff);
      filter_number = -1;
    }
  else
    {
      filter_number = atoi (filter_buff);
    }
  tcflush (dev_port, TCIFLUSH);
  return filter_number;
}

int
Rts2FilterIfw::setFilterNum (int new_filter)
{
  char set_filter[] = "WGOTOx\r";
  int ret;

  if (new_filter > 5 || new_filter < 1)
    {
      syslog (LOG_ERR, "Rts2FilterIfw::setFilterNum bad filter number: %i",
	      new_filter);
      return -1;
    }

  set_filter[5] = (char) new_filter + '0';

  ret = writePort (set_filter, 7);
  if (ret == -1)
    return -1;

  readPort (4);

  if (filter_buff[0] != '*')
    {
      syslog (LOG_ERR, "Rts2FilterIfw::setFilterNum FILTER WHEEL ERROR: %s",
	      filter_buff);
      ret = -1;
    }
  else
    {
      syslog (LOG_DEBUG, "Rts2FilterIfw::setFilterNum Set filter: %s\n",
	      filter_buff);
      ret = 0;
    }
  tcflush (dev_port, TCIFLUSH);

  return ret;
}

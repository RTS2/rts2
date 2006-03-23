#include "filterd.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*! 
 * Class for OPTEC filter wheel.
 *
 * @author john, petr
 */
class Rts2DevFilterdIfw:public Rts2DevFilterd
{
  char *dev_file;
  char filter_buff[5];
  int dev_port;
  int writePort (char *buf, size_t len);
  int readPort (size_t len);
  int shutdown ();
public:
    Rts2DevFilterdIfw (int in_argc, char **in_argv);
    virtual ~ Rts2DevFilterdIfw (void);
  virtual int processOption (int in_opt);
  virtual int init (void);
  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);
};

int
Rts2DevFilterdIfw::writePort (char *buf, size_t len)
{
  size_t ret;
  ret = write (dev_port, buf, len);
  if (ret != len)
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::writePort '%s' %m", buf);
      return -1;
    }
  return 0;
}

int
Rts2DevFilterdIfw::readPort (size_t len)
{
  int ret;
  ret = read (dev_port, filter_buff, len);
  if (ret == -1)
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::readPort %m");
      filter_buff[0] = '\0';
      return -1;
    }
  filter_buff[ret] = '\0';
  return 0;
}

int
Rts2DevFilterdIfw::shutdown (void)
{
  int n;

  /* shutdown filter wheel communications */
  n = writePort ("WEXITS\r", 7);

  readPort (4);

  /* Check for correct response from filter wheel */
  if (strcmp (filter_buff, "END"))
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::shutdown FILTER WHEEL ERROR: %s",
	      filter_buff);
      tcflush (dev_port, TCIFLUSH);
    }
  else
    {
      tcflush (dev_port, TCIFLUSH);
      syslog (LOG_DEBUG,
	      "Rts2DevFilterdIfw::shutdown Filter wheel shutdown: %s",
	      filter_buff);
    }
  close (dev_port);
  dev_port = -1;
  return 0;
}

Rts2DevFilterdIfw::Rts2DevFilterdIfw (int in_argc, char **in_argv):Rts2DevFilterd (in_argc,
		in_argv)
{
  dev_port = -1;

  addOption ('f', "device_name", 1, "device name (/dev..)");
}

Rts2DevFilterdIfw::~Rts2DevFilterdIfw (void)
{
  if (dev_port)
    {
      shutdown ();
    }
}

int
Rts2DevFilterdIfw::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      dev_file = optarg;
      break;
    default:
      return Rts2DevFilterd::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevFilterdIfw::init (void)
{
  struct termios term_options;	/* structure to hold serial port configuration */
  int n;

  int ret;

  ret = Rts2DevFilterd::init ();
  if (ret)
    return ret;

  dev_port = open (dev_file, O_RDWR | O_NOCTTY | O_NDELAY);

  if (dev_port == -1)
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::init cannot open '%s': %m",
	      dev_file);
      return -1;
    }
  ret = fcntl (dev_port, F_SETFL, 0);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::init cannot fcntl: %m");
    }
  /* get current serial port configuration */
  if (tcgetattr (dev_port, &term_options) < 0)
    {
      syslog (LOG_ERR,
	      "Rts2DevFilterdIfw::init error reading serial port configuration: %m");
      return -1;
    }

  /*
   * Set the baud rates to 19200...
   */
  if (cfsetospeed (&term_options, B19200) < 0
      || cfsetispeed (&term_options, B19200) < 0)
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::init error setting baud rate: %m");
      return -1;
    }

  /*
   * Enable the receiver and set local mode...
   */
  term_options.c_cflag |= (CLOCAL | CREAD);

  /* Choose raw input */
  term_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  /* set port to 8 data bits, 1 stop bit, no parity */
  term_options.c_cflag &= ~PARENB;
  term_options.c_cflag &= ~CSTOPB;
  term_options.c_cflag &= ~CSIZE;
  term_options.c_cflag |= CS8;

  /* set timeout  to 10 seconds */
  term_options.c_cc[VTIME] = 80;
  term_options.c_cc[VMIN] = 0;

  tcflush (dev_port, TCIFLUSH);

  /*
   * Set the new options for the port...
   */
  if (tcsetattr (dev_port, TCSANOW, &term_options))
    {
      syslog (LOG_ERR, "Rts2DevFilterdIfw::init tcsetattr %m");
      return -1;
    }

  /* initialise filter wheel */
  n = writePort ("WSMODE\r", 7);
  if (n == -1)
    return n;

  readPort (4);

  /* Check for correct response from filter wheel */
  if (filter_buff[0] != '!')
    {
      syslog (LOG_DEBUG, "Rts2DevFilterdIfw::init FILTER WHEEL ERROR: %s",
	      filter_buff);
      tcflush (dev_port, TCIFLUSH);
      return -1;
    }
  syslog (LOG_DEBUG, "Rts2DevFilterdIfw::init Filter wheel initialised: %s",
	  filter_buff);
  return 0;
}

int
Rts2DevFilterdIfw::getFilterNum (void)
{
  int filter_number;
  int n;

  n = writePort ("WFILTR\r", 7);
  if (n == -1)
    return -1;

  readPort (4);

  if (strstr (filter_buff, "ER"))
    {
      syslog (LOG_DEBUG,
	      "Rts2DevFilterdIfw::getFilterNum FILTER WHEEL ERROR: %s",
	      filter_buff);
      filter_number = -1;
    }
  else
    {
      filter_number = atoi (filter_buff) - 1;
    }
  return filter_number;
}

int
Rts2DevFilterdIfw::setFilterNum (int new_filter)
{
  char set_filter[] = "WGOTOx\r";
  int ret;

  if (new_filter > 4 || new_filter < 0)
    {
      syslog (LOG_ERR,
	      "Rts2DevFilterdIfw::setFilterNum bad filter number: %i",
	      new_filter);
      return -1;
    }

  set_filter[5] = (char) new_filter + '1';

  ret = writePort (set_filter, 7);
  if (ret == -1)
    return -1;

  readPort (4);

  if (filter_buff[0] != '*')
    {
      syslog (LOG_ERR,
	      "Rts2DevFilterdIfw::setFilterNum FILTER WHEEL ERROR: %s",
	      filter_buff);
      ret = -1;
    }
  else
    {
      syslog (LOG_DEBUG, "Rts2DevFilterdIfw::setFilterNum Set filter: %s\n",
	      filter_buff);
      ret = 0;
    }

  return ret;
}

Rts2DevFilterdIfw *device = NULL;

void
killSignal (int sig)
{
  delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevFilterdIfw (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize Optec filter wheel - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}

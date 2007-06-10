#include "sensord.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

class Rts2DevSensorMS257:public Rts2DevSensor
{
private:
  Rts2ValueDouble * msVer;
  Rts2ValueDouble *wavelenght;

  int dev_port;

  void resetDevice ();
  int writePort (const char *str);
  int readPort (char **rstr);

  // specialized readPort functions..
  int readPort (int &ret);
  int readPort (double &ret);

    template < typename T > int writeValue (const char *valueName, T val,
					    char qStr = '=');
    template < typename T > int readValue (const char *valueName, T & val);

  int readRts2Value (const char *valueName, Rts2Value * val);

  char *dev;
protected:
    virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

public:
    Rts2DevSensorMS257 (int in_argc, char **in_argv);

  virtual int init ();
  virtual int info ();
};

void
Rts2DevSensorMS257::resetDevice ()
{
  sleep (5);
  tcflush (dev_port, TCIOFLUSH);
  logStream (MESSAGE_DEBUG) << "Device " << dev << " reseted." << sendLog;
}

int
Rts2DevSensorMS257::writePort (const char *str)
{
  int ret;
  int ret1;
  ret = write (dev_port, str, strlen (str));
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Writing " << str << sendLog;
#endif
  if (ret != (int) strlen (str))
    {
      resetDevice ();
      return ret;
    }
  ret1 = write (dev_port, "\n", 1);
  if (ret1 != 1)
    {
      resetDevice ();
      return -1;
    }
  return ret;
}

int
Rts2DevSensorMS257::readPort (char **rstr)
{
  static char buf[20];
  int i = 0;
  int ret;

  while (true)
    {
      ret = read (dev_port, buf + i, 1);
      if (ret != 1)
	{
	  logStream (MESSAGE_ERROR) << "Cannot read from device: " << errno <<
	    " (" << strerror (errno) << ")" << sendLog;
	  resetDevice ();
	  return -1;
	}
      if (buf[i] == '>')
	break;
      i++;
      if (i >= 20)
	{
	  logStream (MESSAGE_ERROR) <<
	    "Runaway reply string (more then 20 characters)" << sendLog;
	  resetDevice ();
	  return -1;
	}
    }
  // check that we match \cr\lf[E|value]>
  if (buf[0] != '\r' || buf[1] != '\n')
    {
      logStream (MESSAGE_ERROR) << "Reply string does not start with CR LF" <<
	sendLog;
      return -1;
    }
  *rstr = buf + 2;
  buf[i] = '\0';
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_ERROR) << "Read from port " << buf << sendLog;
#endif
  if (**rstr == 'E')
    {
      logStream (MESSAGE_ERROR) << "Error: " << *rstr << sendLog;
      return -1;
    }
  return i;
}

int
Rts2DevSensorMS257::readPort (int &ret)
{
  int r;
  char *rstr;
  r = readPort (&rstr);
  if (r <= 0)
    return r;
  ret = atoi (rstr);
  return 0;
}

int
Rts2DevSensorMS257::readPort (double &ret)
{
  int r;
  char *rstr;
  r = readPort (&rstr);
  if (r <= 0)
    return r;
  ret = atof (rstr);
  return 0;
}

template < typename T > int
Rts2DevSensorMS257::writeValue (const char *valueName, T val, char qStr)
{
  int ret;
  char *rstr;
  std::ostringstream _os;
  _os << qStr << valueName << ' ' << val;
  ret = writePort (_os.str ().c_str ());
  if (ret)
    return ret;
  ret = readPort (&rstr);
  return ret;
}

template < typename T > int
Rts2DevSensorMS257::readValue (const char *valueName, T & val)
{
  int ret;
  char buf[strlen (valueName + 1)];
  *buf = '?';
  strcpy (buf + 1, valueName);
  ret = writePort (buf);
  if (ret)
    return ret;
  ret = readPort (val);
  return ret;
}

int
Rts2DevSensorMS257::readRts2Value (const char *valueName, Rts2Value * val)
{
  int ret;
  int iret;
  double dret;
  switch (val->getValueType ())
    {
    case RTS2_VALUE_INTEGER:
      ret = readValue (valueName, iret);
      ((Rts2ValueInteger *) val)->setValueInteger (iret);
      return ret;
    case RTS2_VALUE_DOUBLE:
      ret = readValue (valueName, dret);
      ((Rts2ValueDouble *) val)->setValueDouble (dret);
      return ret;
    }
  logStream (MESSAGE_ERROR) << "Reading unknow value type " << val->
    getValueType () << sendLog;
  return -1;
}

Rts2DevSensorMS257::Rts2DevSensorMS257 (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
  dev_port = -1;
  dev = "/dev/ttyS0";

  createValue (msVer, "version", "version of MS257", false);
  createValue (wavelenght, "WAVELENG", "monochromator wavelength", true);

  addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}

int
Rts2DevSensorMS257::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == wavelenght)
    {
      return writeValue ("GW", new_value->getValueDouble (), '!');
    }
  return Rts2DevSensor::setValue (old_value, new_value);
}

int
Rts2DevSensorMS257::init ()
{
  struct termios term_options;	/* structure to hold serial port configuration */
  int ret;

  ret = Rts2DevSensor::init ();
  if (ret)
    return ret;

  dev_port = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);

  if (dev_port == -1)
    {
      logStream (MESSAGE_ERROR) << "filter ifw init cannot open: " << dev
	<< strerror (errno) << sendLog;
      return -1;
    }
  ret = fcntl (dev_port, F_SETFL, 0);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "filter ifw init cannot fcntl " <<
	strerror (errno) << sendLog;
    }
  /* get current serial port configuration */
  if (tcgetattr (dev_port, &term_options) < 0)
    {
      logStream (MESSAGE_ERROR) <<
	"filter ifw init error reading serial port configuration: " <<
	strerror (errno) << sendLog;
      return -1;
    }

  /*
   * Set the baud rates to 9600
   */
  if (cfsetospeed (&term_options, B9600) < 0
      || cfsetispeed (&term_options, B9600) < 0)
    {
      logStream (MESSAGE_ERROR) << "filter ifw init error setting baud rate: "
	<< strerror (errno) << sendLog;
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

  /* set timeout  to 20 seconds */
  term_options.c_cc[VTIME] = 200;
  term_options.c_cc[VMIN] = 0;

  tcflush (dev_port, TCIFLUSH);

  /*
   * Set the new options for the port...
   */
  if (tcsetattr (dev_port, TCSANOW, &term_options))
    {
      logStream (MESSAGE_ERROR) << "filter ifw init tcsetattr " <<
	strerror (errno) << sendLog;
      return -1;
    }

  char *rstr;

  ret = writePort ("!DL");
  if (ret)
    return ret;
  ret = readPort (&rstr);
  if (ret)
    return ret;

  ret = readRts2Value ("VER", msVer);
  if (ret)
    return -1;

  // sets correct units
  ret = writeValue ("UNITS", "nn");
  if (ret)
    return ret;

  return 0;
}

int
Rts2DevSensorMS257::info ()
{
  int ret;
  ret = readRts2Value ("PW", wavelenght);
  if (ret)
    return ret;
  return Rts2DevSensor::info ();
}

int
main (int argc, char **argv)
{
  Rts2DevSensorMS257 device = Rts2DevSensorMS257 (argc, argv);
  return device.run ();
}

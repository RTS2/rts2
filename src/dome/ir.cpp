#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <comedilib.h>

#include "dome.h"

#define BAUDRATE B9600

#define LEFT_CLOSE	0
#define RIGHT_CLOSE	3
#define LEFT_OPEN	1
#define RIGHT_OPEN	2

class Rts2DevDomeIR:public Rts2DevDome
{
private:
  char *dome_file;
  int dome_port;
  comedi_t *it;

  int initIrDevice ();
  int getPort (int channel, double *value);
  int getSNOW (float *temp, float *humi, float *wind);

  int openLeft ();
  int openRight ();
  int closeLeft ();
  int closeRight ();

  int leftClosed ();
  int rightClosed ();
  int leftOpened ();
  int rightOpened ();

protected:
//   virtual int processOption ();
//   virtual int isGoodWeather ();

public:
    Rts2DevDomeIR (int argc, char **argv);
    virtual ~ Rts2DevDomeIR (void);
  virtual int init ();
  virtual int idle ();

  virtual int ready ();
  virtual int off ();
  virtual int standby ();
  virtual int observing ();
  virtual int baseInfo ();
  virtual int info ();

//  virtual int openDome ();
//  virtual long isOpened ();
//  virtual int closeDome ();
//  virtual long isClosed ();

};

int
Rts2DevDomeIR::initIrDevice ()
{
  it = comedi_open ("/dev/comedi0");

  return 0;
}

int
Rts2DevDomeIR::getPort (int channel, double *value)
{
  int subdev = 0;
  int max, range = 0;
  lsampl_t data;
  comedi_range *rqn;
  int aref = AREF_GROUND;
  double tmp;

  max = comedi_get_maxdata (it, subdev, channel);
  rqn = comedi_get_range (it, subdev, channel, range);
  comedi_data_read (it, subdev, channel, range, aref, &data);
  tmp = comedi_to_phys (data, rqn, max);

  if (isnan (tmp))
    *value = -10;
  else
    *value = 10 * tmp;

  return 0;
}

int
Rts2DevDomeIR::leftClosed ()
{
  int ret;
  double val;

  ret = getPort (LEFT_CLOSE, &val);

  if (val == -10)
    return 0;
  else
    return 1;
}

int
Rts2DevDomeIR::rightClosed ()
{
  int ret;
  double val;

  ret = getPort (RIGHT_CLOSE, &val);

  if (val == -10)
    return 0;
  else
    return 1;
}

int
Rts2DevDomeIR::leftOpened ()
{
  int ret;
  double val;

  ret = getPort (LEFT_OPEN, &val);

  if (val == -10)
    return 0;
  else
    return 1;
}

int
Rts2DevDomeIR::rightOpened ()
{
  int ret;
  double val;

  ret = getPort (RIGHT_OPEN, &val);

  if (val == -10)
    return 0;
  else
    return 1;
}

Rts2DevDomeIR::Rts2DevDomeIR (int in_argc, char **in_argv):Rts2DevDome (in_argc,
	     in_argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");
  // addOption ('w', "wdc_file", 1, "/dev file with watch-dog card");
  // addOption ('t', "wdc_timeout", 1, "WDC timeout (default to 30 seconds");

  dome_file = "/dev/ttyS0";
  // wdc_file = NULL;
  // wdc_port = -1;
  // wdcTimeOut = 30.0;


  // movingState = MOVE_NONE;

  // weatherConn = NULL;

  // lastClosing = 0;
  // closingNum = 0;
  // lastClosingNum = -1;
}

Rts2DevDomeIR::~Rts2DevDomeIR (void)
{

}

int
Rts2DevDomeIR::getSNOW (float *temp, float *humi, float *wind)
{
  int sockfd, bytes_read, ret, i, j = 0;
  struct sockaddr_in dest;
  char buffer[300], buff[300];
  fd_set rfds;
  struct timeval tv;
  double wind1, wind2;
  char *token;

  if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
      fprintf (stderr, "unable to create socket");
      return 1;
    }

  bzero (&dest, sizeof (dest));
  dest.sin_family = PF_INET;
  dest.sin_addr.s_addr = inet_addr ("193.146.151.70");
  dest.sin_port = htons (6341);
  ret = connect (sockfd, (struct sockaddr *) &dest, sizeof (dest));
  if (ret)
    {
      fprintf (stderr, "unable to connect to SNOW");
      return 1;
    }

  /* send command to SNOW */
  sprintf (buffer, "%s", "RCD");
  ret = write (sockfd, buffer, strlen (buffer));
  if (!ret)
    {
      fprintf (stderr, "unable to send command");
      return 1;
    }

  FD_ZERO (&rfds);
  FD_SET (sockfd, &rfds);

  tv.tv_sec = 10;
  tv.tv_usec = 0;

  ret = select (FD_SETSIZE, &rfds, NULL, NULL, &tv);

  if (ret == 1)
    {
      bytes_read = read (sockfd, buffer, sizeof (buffer));
      if (bytes_read > 0)
	{
	  for (i = 5; i < bytes_read; i++)
	    {
	      if (buffer[i] == ',')
		buff[j++] = '.';
	      else
		buff[j++] = buffer[i];
	    }
	}
    }
  else
    {
      fprintf (stderr, ":-( no data");
      return 1;
    }

  close (sockfd);

  token = strtok ((char *) buff, "|");
  for (j = 1; j < 40; j++)
    {
      token = (char *) strtok (NULL, "|#");
      if (j == 3)
	wind1 = atof (token);
      if (j == 6)
	wind2 = atof (token);
      if (j == 18)
	*temp = atof (token);
      if (j == 24)
	*humi = atof (token);
    }

  if (wind1 > wind2)
    *wind = wind1;
  else
    *wind = wind2;

  return 0;
}

int
Rts2DevDomeIR::info ()
{
  /*
     int ret;
     ret = zjisti_stav_portu ();
     if (ret)
     return -1;
   */
  /*
     sw_state = getPortState (KONCAK_OTEVRENI_PRAVY);
     sw_state |= (getPortState (KONCAK_OTEVRENI_LEVY) << 1);
     sw_state |= (getPortState (KONCAK_ZAVRENI_PRAVY) << 2);
     sw_state |= (getPortState (KONCAK_ZAVRENI_LEVY) << 3);
   */
  if (leftClosed () && rightClosed ())
    sw_state = 4;
  if (leftOpened () && rightOpened ())
    sw_state = 1;
  //rain = weatherConn->getRain ();

  getSNOW (&temperature, &humidity, &windspeed);
  nextOpen = getNextOpen ();
  return Rts2DevDome::info ();
}

int
Rts2DevDomeIR::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeIR::off ()
{
  closeDome ();
  return 0;
}

int
Rts2DevDomeIR::standby ()
{
  closeDome ();
  return 0;
}

int
Rts2DevDomeIR::ready ()
{
  return 0;
}

int
Rts2DevDomeIR::idle ()
{
  return Rts2DevDome::idle ();
}

int
Rts2DevDomeIR::init ()
{
  struct termios oldtio, newtio;

  int ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  dome_port = open (dome_file, O_RDWR | O_NOCTTY);
  if (dome_port == -1)
    return -1;

  ret = tcgetattr (dome_port, &oldtio);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeIR::init tcgetattr " <<
	strerror (errno) << sendLog;
      return -1;
    }

  newtio = oldtio;

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 1;

  tcflush (dome_port, TCIOFLUSH);
  ret = tcsetattr (dome_port, TCSANOW, &newtio);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeIR::init tcsetattr %m" <<
	strerror (errno) << sendLog;
      return -1;
    }

  return 0;
}

int
Rts2DevDomeIR::openLeft ()
{
  return 0;
}

int
Rts2DevDomeIR::openRight ()
{
  return 0;
}

int
Rts2DevDomeIR::closeLeft ()
{
  return 0;
}

int
Rts2DevDomeIR::closeRight ()
{
  return 0;
}

int
Rts2DevDomeIR::observing ()
{
  if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
    return openDome ();
  return 0;
}

int
main (int argc, char **argv)
{
  Rts2DevDomeIR *device = new Rts2DevDomeIR (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}

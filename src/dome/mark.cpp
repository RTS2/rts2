#include "copula.h"
#include "udpweather.h"

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define M16			0xA001	/* crc-16 mask */
#define SLAVE			0x02

#define REG_READ		0x03
#define REG_WRITE		0x10

#define REG_STATE		0x01
#define REG_POSITION		0x02
#define REG_COP_CONTROL		0x03
#define REG_SPLIT_CONTROL	0x04

// average az size of one step (in arcdeg)
#define STEP_AZ_SIZE		0.2

/*!
 * Copula for MARK telescope on Prague Observatory - http://www.observatory.cz
 *
 * @author petr
 */
class Rts2DevCopulaMark:public Rts2DevCopula
{
private:
  char *device_file;
  int cop_desc;

  uint16_t getMsgBufCRC16 (char *msgBuf, int msgLen);
  // communication functions
  int write_read (char *w_buf, int w_buf_len, char *r_buf, int r_buf_len);
  int readReg (int reg_id, uint16_t * reg_val);
  int writeReg (int reg_id, uint16_t reg_val);

  int weatherPort;
  Rts2ConnFramWeather *weatherConn;
public:
    Rts2DevCopulaMark (int argc, char **argv);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int idle ();

  virtual int openDome ();
  virtual long isOpened ();

  virtual int closeDome ();
  virtual long isClosed ();

  virtual int moveStop ();
};

uint16_t
Rts2DevCopulaMark::getMsgBufCRC16 (char *msgBuf, int msgLen)
{
  uint16_t ret = 0xff;
  for (int l = 0; l < msgLen; l++)
    {
      char znakp = msgBuf[l];
      for (int i = 0; i < 8; i++)
	{
	  if ((ret ^ znakp) & 0x01)
	    ret = (ret >> 1) ^ M16;
	  else
	    ret >>= 1;
	  znakp >>= 1;
	}
    }
  return ret;
}

// last 2 bits are reserved for crc! They are counted in w_buf_len and r_buf_len, and
// will be modified by this function
int
Rts2DevCopulaMark::write_read (char *w_buf, int w_buf_len, char *r_buf,
			       int r_buf_len)
{
  int ret;
  uint16_t crc16;
  crc16 = getMsgBufCRC16 (w_buf, w_buf_len - 2);
  w_buf[w_buf_len - 2] = (crc16 & 0xff00) >> 8;
  w_buf[w_buf_len - 1] = (crc16 & 0x00ff);
  ret = write (cop_desc, w_buf, w_buf_len);
  if (ret != w_buf_len)
    {
      syslog (LOG_ERR, "Rts2DevCopulaMark::write_read ret != w_buf_len %i %i",
	      ret, w_buf_len);
      return -1;
    }
  ret = read (cop_desc, r_buf, r_buf_len);
  if (ret != r_buf_len)
    {
      syslog (LOG_ERR, "Rts2DevCopulaMark::write_read ret != r_buf_len %i %i",
	      ret, r_buf_len);
      return -1;
    }
  // get checksum
  crc16 = getMsgBufCRC16 (r_buf, r_buf_len - 2);
  if (r_buf[r_buf_len - 2] != (crc16 & 0xff00) >> 8
      || r_buf[r_buf_len - 1] != (crc16 & 0x00ff))
    {
      syslog (LOG_ERR,
	      "Rts2DevCopulaMark::write_read invalid checksum! (should be %xi, is %xi)",
	      crc16, (uint16_t) r_buf[r_buf_len - 2]);
      return -1;
    }
  usleep (USEC_SEC / 10);
  return 0;
}

int
Rts2DevCopulaMark::readReg (int reg, uint16_t * reg_val)
{
  int ret;
  char wbuf[8];
  char rbuf[7];
  wbuf[0] = SLAVE;
  wbuf[1] = REG_READ;
  wbuf[2] = (reg & 0xff00) >> 8;
  wbuf[3] = (reg & 0x00ff);
  wbuf[4] = 0x00;
  wbuf[5] = 0x01;
  ret = write_read (wbuf, 8, rbuf, 7);
  if (ret)
    return ret;
  *reg_val = (rbuf[3] << 8) || (rbuf[4]);
  return 0;
}

int
Rts2DevCopulaMark::writeReg (int reg, uint16_t reg_val)
{
  char wbuf[11];
  char rbuf[8];
  wbuf[0] = SLAVE;
  wbuf[1] = REG_WRITE;
  wbuf[2] = (reg & 0xff00) >> 8;
  wbuf[3] = (reg & 0x00ff);
  wbuf[4] = 0x00;
  wbuf[5] = 0x01;
  wbuf[6] = 0x02;
  wbuf[7] = (reg_val & 0xff00) >> 8;
  wbuf[8] = (reg_val & 0x00ff);
  return write_read (wbuf, 11, rbuf, 8);
}

Rts2DevCopulaMark::Rts2DevCopulaMark (int in_argc, char **in_argv):Rts2DevCopula (in_argc,
	       in_argv)
{
  weatherPort = 5002;
  weatherConn = NULL;

  device_file = "/dev/ttyS0";

  addOption ('f', "device", 1, "device filename (default to /dev/ttyS0");
  addOption ('W', "mark_weather", 1,
	     "UDP port number of packets from meteo (default to 5002)");
}

int
Rts2DevCopulaMark::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      device_file = optarg;
      break;
    case 'W':
      weatherPort = atoi (optarg);
      break;
    default:
      return Rts2DevCopula::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCopulaMark::init ()
{
  struct termios cop_termios;
  int ret;
  ret = Rts2DevCopula::init ();
  if (ret)
    return ret;

  syslog (LOG_DEBUG, "Rts2DevCopulaMark::init open: %s", device_file);

  cop_desc = open (device_file, O_RDWR);
  if (cop_desc < 0)
    return -1;

  if (tcgetattr (cop_desc, &cop_termios) < 0)
    return -1;

  if (cfsetospeed (&cop_termios, B4800) < 0 ||
      cfsetispeed (&cop_termios, B4800) < 0)
    return -1;

  cop_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
  cop_termios.c_oflag = 0;
  cop_termios.c_cflag =
    ((cop_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
  cop_termios.c_lflag = 0;
  cop_termios.c_cc[VMIN] = 0;
  cop_termios.c_cc[VTIME] = 10;

  if (tcsetattr (cop_desc, TCSANOW, &cop_termios) < 0)
    return -1;

  weatherConn = new Rts2ConnFramWeather (weatherPort, this);
  weatherConn->init ();

  ret = addConnection (weatherConn);
  if (ret)
    {
      delete weatherConn;
      return -1;
    }

  setTimeout (USEC_SEC);

  return 0;
}

int
Rts2DevCopulaMark::idle ()
{
  int ret;
  uint16_t az_val;
  // read az state..
  ret = readReg (REG_POSITION, &az_val);
  if (!ret)
    {
      setCurrentAz (az_val * STEP_AZ_SIZE);
    }

  return Rts2DevCopula::idle ();
}

int
Rts2DevCopulaMark::openDome ()
{
  int ret;
  ret = writeReg (REG_SPLIT_CONTROL, 0x0001);
  if (ret)
    return ret;
  return Rts2DevCopula::openDome ();
}

long
Rts2DevCopulaMark::isOpened ()
{
  return -2;
}

int
Rts2DevCopulaMark::closeDome ()
{
  int ret;
  ret = writeReg (REG_SPLIT_CONTROL, 0x0000);
  if (ret)
    return ret;
  return Rts2DevCopula::closeDome ();
}

long
Rts2DevCopulaMark::isClosed ()
{
  int ret;
  uint16_t reg_val;
  ret = readReg (REG_STATE, &reg_val);
  if (ret)
    return ret;
  // 5 bit is split state
  if (reg_val & 0x10)
    {
      // still open
      return USEC_SEC;
    }
  return -2;
}

int
Rts2DevCopulaMark::moveStop ()
{
  writeReg (REG_COP_CONTROL, 0x00);
  return Rts2DevCopula::moveStop ();
}

Rts2DevCopulaMark *device = NULL;

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
  int ret;
  device = new Rts2DevCopulaMark (argc, argv);
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize dummy copula - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}

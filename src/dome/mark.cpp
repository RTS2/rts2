#include "cupola.h"
#include "udpweather.h"

#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define M16			0xA001	/* crc-16 mask */
#define SLAVE			0x01

#define REG_READ		0x03
#define REG_WRITE		0x06

#define REG_STATE		0x01
#define REG_POSITION		0x02
#define REG_COP_CONTROL		0x03
#define REG_SPLIT_CONTROL	0x04

// average az size of one step (in arcdeg)
#define STEP_AZ_SIZE		1

// AZ offset - AZ of 0 position (in libnova notation - 0 is South, 90 is West)
#define STEP_AZ_OFFSET		270

// how many deg to turn before switching from fast to non-fast mode
#define FAST_TIMEOUT		1

// how many arcdeg we can be off to be considerd to hit position
#define MIN_ERR			0.6

/*!
 * Cupola for MARK telescope on Prague Observatory - http://www.observatory.cz
 *
 * @author petr
 */
class Rts2DevCupolaMark:public Rts2DevCupola
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

  double lastFast;

  enum
  { UNKNOW, IN_PROGRESS, DONE, ERROR } initialized;

  int slew ();
  void parkCupola ();
protected:
    virtual int moveStart ();
  virtual long isMoving ();
  virtual int moveEnd ();
public:
    Rts2DevCupolaMark (int argc, char **argv);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int idle ();

  virtual int ready ();
  virtual int info ();

  virtual int openDome ();
  virtual long isOpened ();

  virtual int closeDome ();
  virtual long isClosed ();

  virtual int moveStop ();

  // park copula
  virtual int standby ();
  virtual int off ();

  virtual double getSplitWidth (double alt)
  {
    return 5;
  }
};

uint16_t Rts2DevCupolaMark::getMsgBufCRC16 (char *msgBuf, int msgLen)
{
  uint16_t
    ret = 0xffff;
  for (int l = 0; l < msgLen; l++)
    {
      char
	znakp = msgBuf[l];
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
Rts2DevCupolaMark::write_read (char *w_buf, int w_buf_len, char *r_buf,
			       int r_buf_len)
{
  int ret;
  uint16_t crc16;
  crc16 = getMsgBufCRC16 (w_buf, w_buf_len - 2);
  w_buf[w_buf_len - 2] = (crc16 & 0x00ff);
  w_buf[w_buf_len - 1] = (crc16 & 0xff00) >> 8;
  ret = write (cop_desc, w_buf, w_buf_len);
#ifdef DEBUG_ALL
  for (int i = 0; i < w_buf_len; i++)
    {
      logStream (MESSAGE_DEBUG) <<
	"Rts2DevCupolaMark::write_read write byte " << i << "value " <<
	w_buf[i] << sendLog;
    }
#endif
  if (ret != w_buf_len)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevCupolaMark::write_read ret != w_buf_len " << ret << " " <<
	w_buf_len << sendLog;
      return -1;
    }
  ret = read (cop_desc, r_buf, r_buf_len);
#ifdef DEBUG_ALL
  for (int i = 0; i < r_buf_len; i++)
    {
      logStream (MESSAGE_DEBUG) <<
	"Rts2DevCupolaMark::write_read read byte " << i << " value " <<
	r_buf[i] << sendLog;
    }
#endif
  if (ret != r_buf_len)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevCupolaMark::write_read ret != r_buf_len " << ret << " " <<
	r_buf_len << sendLog;
      return -1;
    }
  // get checksum
  crc16 = getMsgBufCRC16 (r_buf, r_buf_len - 2);
  if ((r_buf[r_buf_len - 1] & ((crc16 & 0xff00) >> 8)) !=
      ((crc16 & 0xff00) >> 8)
      || (r_buf[r_buf_len - 2] & (crc16 & 0x00ff)) != (crc16 & 0x00ff))
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevCupolaMark::write_read invalid checksum! (should be " <<
	((crc16 & 0xff00) >> 8) << " " << (crc16 & 0x00ff) << " is " <<
	r_buf[r_buf_len - 1] << " " << r_buf[r_buf_len - 2] << " ( " <<
	(r_buf[r_buf_len - 1] & ((crc16 & 0xff00) >> 8)) << " " <<
	(r_buf[r_buf_len - 2] & (crc16 & 0x00ff)) << " ))" << sendLog;
      return -1;
    }
  usleep (USEC_SEC / 10);
  return 0;
}

int
Rts2DevCupolaMark::readReg (int reg, uint16_t * reg_val)
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
  *reg_val = rbuf[3];
  *reg_val = *reg_val << 8;
  *reg_val |= (rbuf[4]);
  logStream (MESSAGE_DEBUG) << "Rts2DevCupolaMark::readReg reg " << reg <<
    " val " << *reg_val << sendLog;
  return 0;
}

int
Rts2DevCupolaMark::writeReg (int reg, uint16_t reg_val)
{
  char wbuf[8];
  char rbuf[8];
  // we must either be initialized or
  // or we must issue initialization request
  if (!(initialized == DONE || (reg == REG_COP_CONTROL && reg_val == 0x08)))
    return -1;
  wbuf[0] = SLAVE;
  wbuf[1] = REG_WRITE;
  wbuf[2] = (reg & 0xff00) >> 8;
  wbuf[3] = (reg & 0x00ff);
  wbuf[4] = (reg_val & 0xff00) >> 8;
  wbuf[5] = (reg_val & 0x00ff);
  logStream (MESSAGE_DEBUG) << "Rts2DevCupolaMark::writeReg reg " << reg <<
    " value " << reg_val << sendLog;
  return write_read (wbuf, 8, rbuf, 8);
}

Rts2DevCupolaMark::Rts2DevCupolaMark (int in_argc, char **in_argv):Rts2DevCupola (in_argc,
	       in_argv)
{
  weatherPort = 5002;
  weatherConn = NULL;

  lastFast = nan ("f");

  device_file = "/dev/ttyS0";

  initialized = UNKNOW;

  addOption ('f', "device", 1, "device filename (default to /dev/ttyS0");
  addOption ('W', "mark_weather", 1,
	     "UDP port number of packets from meteo (default to 5002)");
}

int
Rts2DevCupolaMark::processOption (int in_opt)
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
      return Rts2DevCupola::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCupolaMark::init ()
{
  struct termios cop_termios;
  int ret;
  ret = Rts2DevCupola::init ();
  if (ret)
    return ret;

  logStream (MESSAGE_DEBUG) << "Rts2DevCupolaMark::init open: " << device_file
    << sendLog;

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
  cop_termios.c_cc[VTIME] = 40;

  if (tcsetattr (cop_desc, TCSANOW, &cop_termios) < 0)
    return -1;

  weatherConn = new Rts2ConnFramWeather (weatherPort, 20, this);
  weatherConn->init ();

  addConnection (weatherConn);

  setTimeout (USEC_SEC);

  return 0;
}

int
Rts2DevCupolaMark::idle ()
{
  int ret;
  uint16_t copState;
  // check for weather..
  if (weatherConn->isGoodWeather ())
    {
      if (((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	  && ((getState () & DOME_DOME_MASK) == DOME_CLOSED))
	{
	  // after centrald reply, that he switched the state, dome will
	  // open
	  sendMaster ("on");
	}
    }
  else
    {
      // close dome - don't thrust centrald to be running and closing
      // it for us
      ret = closeDome ();
      if (ret == -1)
	{
	  setTimeout (10 * USEC_SEC);
	}
      setMasterStandby ();
    }
  if (initialized == UNKNOW || initialized == DONE
      || initialized == IN_PROGRESS || initialized == ERROR)
    {
      // check if initialized
      ret = readReg (REG_STATE, &copState);
      if (ret)
	{
	  logStream (MESSAGE_ERROR) << "Rts2DevCupola::idle cannot read reg!"
	    << sendLog;
	  sleep (2);
	  tcflush (cop_desc, TCIOFLUSH);
	  initialized = ERROR;
	}
      else if ((copState & 0x0200) == 0)
	{
	  if (initialized != IN_PROGRESS && initialized != DONE)
	    {
	      // not initialized, initialize
	      writeReg (REG_COP_CONTROL, 0x08);
	      initialized = IN_PROGRESS;
	    }
	}
      else
	{
	  initialized = DONE;
	}
    }
  return Rts2DevCupola::idle ();
}

int
Rts2DevCupolaMark::openDome ()
{
  int ret;
  if (!weatherConn->isGoodWeather ())
    return -1;
  ret = writeReg (REG_SPLIT_CONTROL, 0x0001);
  if (ret)
    return ret;
  return Rts2DevCupola::openDome ();
}

long
Rts2DevCupolaMark::isOpened ()
{
  return -2;
}

int
Rts2DevCupolaMark::closeDome ()
{
  int ret;
  ret = writeReg (REG_SPLIT_CONTROL, 0x0000);
  if (ret)
    return ret;
  return Rts2DevCupola::closeDome ();
}

long
Rts2DevCupolaMark::isClosed ()
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
Rts2DevCupolaMark::ready ()
{
  return 0;
}

int
Rts2DevCupolaMark::info ()
{
  int ret;
  int16_t az_val;
  setRain (weatherConn->getRain ());
  setWindSpeed (weatherConn->getWindspeed ());
  ret = readReg (REG_POSITION, (uint16_t *) & az_val);
  if (!ret)
    {
      setCurrentAz (ln_range_degrees
		    ((az_val * STEP_AZ_SIZE) + STEP_AZ_OFFSET));
      return Rts2DevCupola::info ();
    }
  return Rts2DevCupola::info ();
}

/**
 * @return -1 on error, -2 when move complete (on target position), 0 when nothing interesting happen
 */
int
Rts2DevCupolaMark::slew ()
{
  int ret;
  uint16_t copControl;

  if (initialized != DONE)
    return -1;

  ret = readReg (REG_COP_CONTROL, &copControl);
  // we need to move in oposite direction..do slow change
  if (((copControl & 0x01) && getTargetDistance () < 0)
      || ((copControl & 0x02) && getTargetDistance () > 0))
    {
      // stop faster movement..
      if (copControl & 0x04)
	{
	  copControl &= ~(0x04);
	  lastFast = getCurrentAz ();
	  return writeReg (REG_COP_CONTROL, copControl);
	}
      // wait for copula to reach some distance in slow mode..
      if (fabs (lastFast - getCurrentAz ()) < FAST_TIMEOUT
	  || fabs (lastFast - getCurrentAz ()) > (360 - FAST_TIMEOUT))
	{
	  return 0;
	}
      // can finaly stop movement..
      copControl &= ~(0x03);
      writeReg (REG_COP_CONTROL, copControl);
      lastFast = nan ("f");
    }
  // going our direction..
  if ((copControl & 0x01) || (copControl & 0x02))
    {
      // already in fast mode..
      if (copControl & 0x04)
	{
	  // try to slow down when needed
	  if (fabs (getTargetDistance ()) < FAST_TIMEOUT)
	    {
	      copControl &= ~(0x04);
	      ret = writeReg (REG_COP_CONTROL, copControl);
	      if (ret)
		return -1;
	    }
	  logStream (MESSAGE_DEBUG) << "Rts2DevCupolaMark::slew slow down" <<
	    sendLog;
	}
      // test if we hit target destination
      if (fabs (getTargetDistance ()) < MIN_ERR)
	{
	  copControl &= ~(0x03);
	  ret = writeReg (REG_COP_CONTROL, copControl);
	  if (ret)
	    return -1;
	  return -2;
	}
    }
  // not move at all
  if ((copControl & 0x03) == 0x00)
    {
      copControl = (getTargetDistance () < 0) ? 0x02 : 0x01;
      ret = writeReg (REG_COP_CONTROL, copControl);
      if (ret)
	return -1;
      return 0;
    }
  return 0;
}

int
Rts2DevCupolaMark::moveStart ()
{
  int ret;
  ret = needSplitChange ();
  if (ret == 0 || ret == -1)
    return ret;			// pretend we change..so other devices can sync on our command
  slew ();
  return Rts2DevCupola::moveStart ();
}

long
Rts2DevCupolaMark::isMoving ()
{
  int ret;
  ret = needSplitChange ();
  if (ret == -1)
    return ret;
  ret = slew ();
  if (ret == 0)
    return USEC_SEC / 100;
  return ret;
}

int
Rts2DevCupolaMark::moveEnd ()
{
  writeReg (REG_COP_CONTROL, 0x00);
  return Rts2DevCupola::moveEnd ();
}

int
Rts2DevCupolaMark::moveStop ()
{
  writeReg (REG_COP_CONTROL, 0x00);
  return Rts2DevCupola::moveStop ();
}

void
Rts2DevCupolaMark::parkCupola ()
{
  if (initialized != IN_PROGRESS)
    {
      // reinitialize copula
      writeReg (REG_COP_CONTROL, 0x08);
      initialized = IN_PROGRESS;
    }
}

int
Rts2DevCupolaMark::standby ()
{
  parkCupola ();
  return Rts2DevCupola::standby ();
}

int
Rts2DevCupolaMark::off ()
{
  parkCupola ();
  return Rts2DevCupola::off ();
}

int
main (int argc, char **argv)
{
  Rts2DevCupolaMark device = Rts2DevCupolaMark (argc, argv);
  return device.run ();
}

#include <iostream>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#if defined __GNUC__ && (__GNUC__ >=3)
#	include <iostream>
#	include <fstream>
using namespace std;
#else
#	include <iostream.h>
#	include <fstream.h>
#endif

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#include "udpweather.h"
#include "dome.h"

#define BAUDRATE B9600
#define ZAPIS_NA_PORT 4
#define STAV_PORTU 0
#define CHYBA_NEZNAMY_PRIKAZ 0x80
#define CHYBA_PRETECENI_TX_BUFFERU 0x81
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2

// how long we will keep lastWeatherStatus as actual (in second)
#define FRAM_WEATHER_TIMEOUT	40

// check time in usec (1000000 ms = 1s)
#define FRAM_CHECK_TIMEOUT 1000

// following times are in seconds!
#define FRAM_TIME_OPEN_RIGHT	24
#define FRAM_TIME_OPEN_LEFT	26
#define FRAM_TIME_CLOSE_RIGHT	28
#define FRAM_TIME_CLOSE_LEFT	32

#define FRAM_TIME_RECLOSE_RIGHT 5
#define FRAM_TIME_RECLOSE_LEFT  5

// try to close dome again after n second
#define FRAM_TIME_CLOSING_RETRY	200

// if closing fails for n times, do not close again; send warning
// e-mail
#define FRAM_MAX_CLOSING_RETRY  3

// how many times to try to reclose dome
#define FRAM_RECLOSING_MAX	3

typedef enum
{ VENTIL_AKTIVACNI,
  // tohle je prohozeny, je to spatne!! az nam prohozej ventily, je to treba zmenit
  // petr 15.8.2006
  // prohozeni je konecne, nebudeme to menit
  // petr 17.11.2006
  VENTIL_OTEVIRANI_LEVY,
  VENTIL_OTEVIRANI_PRAVY,
  VENTIL_ZAVIRANI_PRAVY,
  VENTIL_ZAVIRANI_LEVY,
  KOMPRESOR,
  ZASUVKA_PRAVA,
  ZASUVKA_LEVA,
  // tohle je prohozeny, je to spatne!! az nam prohozej koncaky, je to treba zmenit
  // petr 15.8.2006
  // prohozeni je konecne, nebudeme to menit
  // petr 17.11.2006
  KONCAK_OTEVRENI_LEVY,
  KONCAK_OTEVRENI_PRAVY,
  KONCAK_ZAVRENI_PRAVY,
  KONCAK_ZAVRENI_LEVY
}
vystupy;

struct typ_a
{
  unsigned char port;
  unsigned char pin;
} adresa[] =
{
  {
  PORT_A, 32},
  {
  PORT_B, 1},
  {
  PORT_B, 2},
  {
  PORT_B, 4},
  {
  PORT_B, 8},
  {
  PORT_A, 1},
  {
  PORT_A, 2},
  {
  PORT_A, 4},
  {
  PORT_B, 16},
  {
  PORT_B, 32},
  {
  PORT_B, 64},
  {
  PORT_B, 128}
};

#define NUM_ZAS		3

#define OFF		0
#define STANDBY		1
#define OBSERVING	2

// prvni zasuvka dalekohled, druha kamery, treti chlazeni
int zasuvky_index[NUM_ZAS] = { 0, 1, 2 };

enum stavy
{ ZAS_VYP, ZAS_ZAP };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
enum stavy zasuvky_stavy[3][NUM_ZAS] =
{
  // off
  {ZAS_VYP, ZAS_VYP, ZAS_VYP},
  // standby
  {ZAS_ZAP, ZAS_ZAP, ZAS_VYP},
  // observnig
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

/***********************************************************************************************
 * 
 * Class for dome controller.
 *
 **********************************************************************************************/

class Rts2DevDomeFram:public Rts2DevDome
{
private:
  int dome_port;
  char *dome_file;

  int wdc_port;
  char *wdc_file;
  double wdcTimeOut;

  int reclosing_num;

  unsigned char spinac[2];
  unsigned char stav_portu[3];

  Rts2ConnFramWeather *weatherConn;

  int zjisti_stav_portu_int ();
  int zjisti_stav_portu ();
  void zapni_pin (unsigned char c_port, unsigned char pin);
  void vypni_pin (unsigned char c_port, unsigned char pin);
  int getPortState (int c_port)
  {
    return !!(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
  };

  int isOn (int c_port);
  const char *isOnString (int c_port);
  int handle_zasuvky (int zas);

  /**
   * 
   * Handles move commands without changing our state.
   *
   */

  int openLeftMove ();
  int openRightMove ();

  int closeRightMove ();
  int closeLeftMove ();

  /**
   * 
   * Add state handling for basic operations..
   *
   */

  int openLeft ();
  int openRight ();

  int closeRight ();
  int closeLeft ();

  int stopMove ();

  time_t timeoutEnd;

  void setMotorTimeout (time_t timeout);

  /** 
   * Check if timeout was exceeded.
   *
   * @return 0 when timeout wasn't reached, 1 when timeout was
   * exceeded.
   */

  int checkMotorTimeout ();

  time_t lastClosing;
  int closingNum;
  int lastClosingNum;

  bool sendContFailMail;

  int openWDC ();
  int closeWDC ();

  int resetWDC ();
  int getWDCTimeOut ();
  int setWDCTimeOut (int on, double timeout);
  int getWDCTemp (int id);

  enum
  { MOVE_NONE, MOVE_OPEN_LEFT, MOVE_OPEN_LEFT_WAIT, MOVE_OPEN_RIGHT,
    MOVE_OPEN_RIGHT_WAIT,
    MOVE_CLOSE_RIGHT, MOVE_CLOSE_RIGHT_WAIT, MOVE_CLOSE_LEFT,
    MOVE_CLOSE_LEFT_WAIT,
    MOVE_RECLOSE_RIGHT_WAIT, MOVE_RECLOSE_LEFT_WAIT
  } movingState;

protected:
  virtual int processOption (int in_opt);
  virtual int isGoodWeather ();
public:
  Rts2DevDomeFram (int argc, char **argv);
  virtual ~ Rts2DevDomeFram (void);
  virtual int init ();
  virtual int idle ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int openDome ();
  virtual long isOpened ();
  virtual int endOpen ();
  virtual int closeDome ();
  virtual long isClosed ();
  virtual int endClose ();

  virtual int observing ();
  virtual int standby ();
  virtual int off ();

  int sendFramMail (char *subject);
};

int
Rts2DevDomeFram::zjisti_stav_portu_int ()
{
  unsigned char t1, t2, c;
  int ret;
  usleep (USEC_SEC / 1000);
  c = STAV_PORTU | PORT_A;
  ret = write (dome_port, &c, 1);
  if (ret != 1)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeFram::zjisti_stav_portu writeA " << strerror (errno) <<
	" (" << ret << ")" << sendLog;
      return -1;
    }
  ret = read (dome_port, &t1, 1);
  if (ret != 1)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeFram::zjisti_stav_portu readA "
	<< strerror (errno) << " (" << ret << ")" << sendLog;
      return -1;
    }
  ret = read (dome_port, &stav_portu[PORT_A], 1);
  if (ret != 1)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeFram::zjisti_stav_portu read PORT_A" << strerror (errno)
	<< " (" << ret << ")" << sendLog;
      return -1;
    }
  c = STAV_PORTU | PORT_B;
  write (dome_port, &c, 1);
  if (ret != 1)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeFram::zjisti_stav_portu writeB " << strerror (errno) <<
	" (" << ret << ")" << sendLog;
      return -1;
    }
  ret = read (dome_port, &t2, 1);
  if (ret != 1)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeFram::zjisti_stav_portu write PORT_B " << strerror (errno)
	<< " (" << ret << ")" << sendLog;
      return -1;
    }
  ret = read (dome_port, &stav_portu[PORT_B], 1);
  if (ret != 1)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeFram::zjisti_stav_portu read PORT_B " << strerror (errno)
	<< " (" << ret << ")" << sendLog;
      return -1;
    }
  logStream (MESSAGE_DEBUG) <<
    "Rts2DevDomeFram::zjisti_stav_portu A: "
    << std::hex << t1
    << " stav_portu[PORT_A]: "
    << std::hex << stav_portu[PORT_A]
    << " B: "
    << std::hex << t2
    << " stav_portu[PORT_B]: " << std::hex << stav_portu[PORT_B] << sendLog;
  return 0;
}

int
Rts2DevDomeFram::zjisti_stav_portu ()
{
  int ret;
  int timeout = 0;
  do
    {
      ret = zjisti_stav_portu_int ();
      if (ret == 0)
	break;
      timeout++;
      tcflush (dome_port, TCIOFLUSH);
      usleep (USEC_SEC / 2);
    }
  while (timeout < 10);
  return ret;
}

void
Rts2DevDomeFram::zapni_pin (unsigned char in_port, unsigned char pin)
{
  int ret;
  unsigned char c;
  ret = zjisti_stav_portu ();
  if (ret)
    return;
  c = ZAPIS_NA_PORT | in_port;
  logStream (MESSAGE_DEBUG) << "port: "
    << std::hex << in_port
    << " pin: " << std::hex << pin << " write: " << std::hex << c << sendLog;
  write (dome_port, &c, 1);
  c = stav_portu[in_port] | pin;
  logStream (MESSAGE_DEBUG) << "zapni_pin: " << std::hex << c << sendLog;
  write (dome_port, &c, 1);
}

void
Rts2DevDomeFram::vypni_pin (unsigned char in_port, unsigned char pin)
{
  int ret;
  unsigned char c;
  ret = zjisti_stav_portu ();
  if (ret)
    return;
  c = ZAPIS_NA_PORT | in_port;
  logStream (MESSAGE_DEBUG) << "port: "
    << std::hex << in_port
    << " pin: " << std::hex << pin << " write: " << std::hex << c << sendLog;
  write (dome_port, &c, 1);
  c = stav_portu[in_port] & (~pin);
  logStream (MESSAGE_DEBUG) << "vypni_pin: " << std::hex << c << sendLog;
  write (dome_port, &c, 1);
}

int
Rts2DevDomeFram::isOn (int c_port)
{
  zjisti_stav_portu ();
  return !(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}

const char *
Rts2DevDomeFram::isOnString (int c_port)
{
  return (stav_portu[adresa[c_port].port] & adresa[c_port].
	  pin) ? "off" : "on ";
}

void
Rts2DevDomeFram::setMotorTimeout (time_t timeout)
{
  time_t now;
  time (&now);
  timeoutEnd = now + timeout;
}

int
Rts2DevDomeFram::checkMotorTimeout ()
{
  time_t now;
  time (&now);
  if (now >= timeoutEnd)
//    logStream (MESSAGE_DEBUG) << "timeout reached: " << now -
//      timeoutEnd << " state: " << movingState << sendLog;
    logStream (MESSAGE_DEBUG) << "timeout reached: " << now -
      timeoutEnd << " state: " << int (movingState) << sendLog;
  return (now >= timeoutEnd);
}

int
Rts2DevDomeFram::openWDC ()
{
  struct termios oldtio, newtio;
  int ret;

  wdc_port = open (wdc_file, O_RDWR | O_NOCTTY);
  if (wdc_port < 0)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeFram::openWDC Can't open device " << wdc_file << " :" <<
	strerror (errno) << sendLog;
      return -1;
    }
  ret = tcgetattr (wdc_port, &oldtio);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeFram::openWDC tcgetattr " <<
	strerror (errno) << sendLog;
      return -1;
    }
  newtio = oldtio;
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 100;
  tcflush (wdc_port, TCIOFLUSH);
  ret = tcsetattr (wdc_port, TCSANOW, &newtio);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeFram::openWDC tcsetattr " <<
	strerror (errno) << sendLog;
      return -1;
    }

  return setWDCTimeOut (1, wdcTimeOut);
}

int
Rts2DevDomeFram::closeWDC ()
{
  setWDCTimeOut (1, 120.0);
  close (wdc_port);
  return 0;
}

int
Rts2DevDomeFram::resetWDC ()
{
  int t;
  char reply[128];

  t = sprintf (reply, "~**\r");
  write (wdc_port, reply, t);
  tcflush (wdc_port, TCOFLUSH);
  return 0;
}

int
Rts2DevDomeFram::getWDCTimeOut ()
{
  int i, q, r, t;
  char reply[128];

  t = sprintf (reply, "~012\r" /*,i,q */ );
  write (wdc_port, reply, t);
  tcflush (wdc_port, TCOFLUSH);

  q = i = t = 0;
  do
    {
      r = read (wdc_port, &q, 1);
      if (r == 1)
	reply[i++] = q;
      else
	{
	  t++;
	  if (t > 10)
	    break;
	}
    }
  while (q > 20);

  reply[i] = 0;

  if (reply[0] == '!')
    logStream (MESSAGE_ERROR) << "Rts2DevDomeFram::getWDCTimeOut reply " <<
      reply + 1 << sendLog;

  return 0;

}

int
Rts2DevDomeFram::setWDCTimeOut (int on, double timeout)
{
  int i, q, r, t;
  char reply[128];

  q = (int) (timeout / 0.03);
  if (q > 0xffff)
    q = 0xffff;
  if (q < 0)
    q = 0;
  if (on)
    i = '1';
  else
    i = '0';

  t = sprintf (reply, "~013%c%04X\r", i, q);
  write (wdc_port, reply, t);
  tcflush (wdc_port, TCOFLUSH);

  q = i = t = 0;
  do
    {
      r = read (wdc_port, &q, 1);
      if (r == 1)
	reply[i++] = q;
      else
	{
	  t++;
	  if (t > 10)
	    break;
	}
    }
  while (q > 20);

  logStream (MESSAGE_DEBUG) <<
    "Rts2DevDomeFram::setWDCTimeOut on: " << on << " timeout: " << timeout <<
    " q: " << q << sendLog;

  reply[i] = 0;

  if (reply[0] == '!')
    logStream (MESSAGE_DEBUG) << "Rts2DevDomeFram::setWDCTimeOut reply: " <<
      reply + 1 << sendLog;

  return 0;
}

int
Rts2DevDomeFram::getWDCTemp (int id)
{
  int i, q, r, t;
  char reply[128];

  if ((id < 0) || (id > 2))
    return -1;

  t = sprintf (reply, "~017%X\r", id + 5);
  write (wdc_port, reply, t);
  tcflush (wdc_port, TCOFLUSH);

  q = i = t = 0;
  do
    {
      r = read (wdc_port, &q, 1);
      if (r == 1)
	reply[i++] = q;
      else
	{
	  t++;
	  if (t > 10)
	    return -1;
	}
    }
  while (q > 20);

  reply[i] = 0;

  if (reply[0] != '!')
    return -1;

  i = atoi (reply + 1);

  return i;
}

#define ZAP(i) zapni_pin(adresa[i].port,adresa[i].pin)
#define VYP(i) vypni_pin(adresa[i].port,adresa[i].pin)

int
Rts2DevDomeFram::openLeftMove ()
{
  ZAP (KOMPRESOR);
  sleep (1);
  logStream (MESSAGE_DEBUG) << "opening left door" << sendLog;
  ZAP (VENTIL_OTEVIRANI_LEVY);
  ZAP (VENTIL_AKTIVACNI);
  return 0;
}

int
Rts2DevDomeFram::openRightMove ()
{
  VYP (VENTIL_AKTIVACNI);
  VYP (VENTIL_OTEVIRANI_LEVY);
  ZAP (KOMPRESOR);
  sleep (1);
  logStream (MESSAGE_DEBUG) << "opening right door" << sendLog;
  ZAP (VENTIL_OTEVIRANI_PRAVY);
  ZAP (VENTIL_AKTIVACNI);
  return 0;
}

int
Rts2DevDomeFram::closeRightMove ()
{
  ZAP (KOMPRESOR);
  sleep (1);
  logStream (MESSAGE_DEBUG) << "closing right door" << sendLog;
  ZAP (VENTIL_ZAVIRANI_PRAVY);
  ZAP (VENTIL_AKTIVACNI);
  return 0;
}

int
Rts2DevDomeFram::closeLeftMove ()
{
  VYP (VENTIL_AKTIVACNI);
  VYP (VENTIL_ZAVIRANI_PRAVY);
  ZAP (KOMPRESOR);
  sleep (1);
  logStream (MESSAGE_DEBUG) << "closing left door" << sendLog;
  ZAP (VENTIL_ZAVIRANI_LEVY);
  ZAP (VENTIL_AKTIVACNI);
  return 0;
}


int
Rts2DevDomeFram::openLeft ()
{
  movingState = MOVE_OPEN_LEFT;
  openLeftMove ();
  movingState = MOVE_OPEN_LEFT_WAIT;
  setMotorTimeout (FRAM_TIME_OPEN_LEFT);
  return 0;
}

int
Rts2DevDomeFram::openRight ()
{
  // otevri pravou strechu
  movingState = MOVE_OPEN_RIGHT;
  openRightMove ();
  movingState = MOVE_OPEN_RIGHT_WAIT;
  setMotorTimeout (FRAM_TIME_OPEN_RIGHT);
  return 0;
}

int
Rts2DevDomeFram::closeRight ()
{
  closeRightMove ();
  movingState = MOVE_CLOSE_RIGHT_WAIT;
  setMotorTimeout (FRAM_TIME_CLOSE_RIGHT);
  return 0;
}

int
Rts2DevDomeFram::closeLeft ()
{
  closeLeftMove ();
  movingState = MOVE_CLOSE_LEFT_WAIT;
  setMotorTimeout (FRAM_TIME_CLOSE_LEFT);
  return 0;
}

int
Rts2DevDomeFram::stopMove ()
{
  vypni_pin (adresa[VENTIL_AKTIVACNI].port,
	     adresa[VENTIL_AKTIVACNI].pin | adresa[KOMPRESOR].pin);
  movingState = MOVE_NONE;
  return 0;
}

int
Rts2DevDomeFram::openDome ()
{
  if (movingState != MOVE_NONE)
    return -1;
  if (!isGoodWeather ())
    return -1;

  lastClosing = 0;
  closingNum = 0;
  lastClosingNum = -1;

  tcflush (dome_port, TCIOFLUSH);

  if (isOn (KONCAK_OTEVRENI_LEVY) && isOn (KONCAK_OTEVRENI_LEVY))
    {
      return endOpen ();
    }

  vypni_pin (adresa[VENTIL_OTEVIRANI_PRAVY].port,
	     adresa[VENTIL_OTEVIRANI_PRAVY].pin
	     | adresa[VENTIL_ZAVIRANI_PRAVY].pin
	     | adresa[VENTIL_ZAVIRANI_LEVY].pin);

  if (!isOn (KONCAK_OTEVRENI_LEVY))
    {
      // otevri levou strechu
      openLeft ();
    }
  else				// like !isOn (KONCAK_OTEVRENI_PRAVY - see && at begining
    {
      openRight ();
    }

  setTimeout (10);
  return Rts2DevDome::openDome ();
}

long
Rts2DevDomeFram::isOpened ()
{
  int flag = 0;
  logStream (MESSAGE_DEBUG) << "isOpened " << (int) movingState << sendLog;
  switch (movingState)
    {
    case MOVE_OPEN_LEFT_WAIT:
      // check for timeout..
      if (!(isOn (KONCAK_OTEVRENI_LEVY) || checkMotorTimeout ()))
	// go to end return..
	break;
      flag = 1;
      // follow on..
    case MOVE_OPEN_RIGHT:
      openRight ();
      break;
    case MOVE_OPEN_RIGHT_WAIT:
      if (!(isOn (KONCAK_OTEVRENI_PRAVY) || checkMotorTimeout ()))
	break;
    default:
      return -2;
    }
  if (flag)
    infoAll ();
  return FRAM_CHECK_TIMEOUT;
}

int
Rts2DevDomeFram::endOpen ()
{
  int ret;
  stopMove ();
  VYP (VENTIL_OTEVIRANI_PRAVY);
  VYP (VENTIL_OTEVIRANI_LEVY);
  ret = zjisti_stav_portu ();	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  if (!ret && isOn (KONCAK_OTEVRENI_PRAVY) && isOn (KONCAK_OTEVRENI_LEVY) &&
      !isOn (KONCAK_ZAVRENI_PRAVY) && !isOn (KONCAK_ZAVRENI_LEVY))
    {
      sendFramMail ("FRAM dome opened");
    }
  else
    {
      sendFramMail ("WARNING FRAM dome opened with wrong end swithes status");
    }
  return Rts2DevDome::endOpen ();
}

int
Rts2DevDomeFram::closeDome ()
{
  time_t now;
  if (movingState == MOVE_CLOSE_RIGHT
      || movingState == MOVE_CLOSE_RIGHT_WAIT
      || movingState == MOVE_CLOSE_LEFT
      || movingState == MOVE_CLOSE_LEFT_WAIT)
    {
      // closing already in progress
      return 0;
    }

  tcflush (dome_port, TCIOFLUSH);

  if (zjisti_stav_portu () == -1)
    {
      logStream (MESSAGE_ERROR) << "Cannot read from port at close!" <<
	sendLog;
      if (!sendContFailMail)
	{
	  sendFramMail
	    ("ERROR FRAM DOME CANNOT BE CLOSED DUE TO ROOF CONTROLLER FAILURE!!");
	  sendContFailMail = true;
	}
      return -1;
    }
  sendContFailMail = false;
  if (movingState != MOVE_NONE)
    {
      stopMove ();
    }
  if (isOn (KONCAK_ZAVRENI_PRAVY) && isOn (KONCAK_ZAVRENI_LEVY))
    return endClose ();
  time (&now);
  if (lastClosing + FRAM_TIME_CLOSING_RETRY > now)
    {
      logStream (MESSAGE_WARNING) <<
	"closeDome ignored - closing timeout not reached" << sendLog;
      return -1;
    }

  vypni_pin (adresa[VENTIL_OTEVIRANI_PRAVY].port,
	     adresa[VENTIL_OTEVIRANI_PRAVY].pin
	     | adresa[VENTIL_ZAVIRANI_LEVY].pin
	     | adresa[VENTIL_OTEVIRANI_LEVY].pin);

  if (closingNum > FRAM_MAX_CLOSING_RETRY)
    {
      logStream (MESSAGE_WARNING) <<
	"max closing retry reached, do not try closing again" << sendLog;
      sendFramMail ("FRAM WARNING - !!max closing retry reached!!");
      return -1;
    }

  if (!isOn (KONCAK_ZAVRENI_PRAVY))
    {
      closeRight ();
    }
  else
    {
      closeLeft ();
    }
  closingNum++;
  reclosing_num = 0;
  return Rts2DevDome::closeDome ();
}

long
Rts2DevDomeFram::isClosed ()
{
  int flag = 0;			// send infoAll at end
  logStream (MESSAGE_DEBUG) << "isClosed " << (int) movingState << sendLog;
  switch (movingState)
    {
    case MOVE_CLOSE_RIGHT_WAIT:
      if (checkMotorTimeout ())
	{
	  if (reclosing_num >= FRAM_RECLOSING_MAX)
	    {
	      // reclosing number exceeded, move to next door
	      movingState = MOVE_CLOSE_LEFT;
	      break;
	    }
	  setMotorTimeout (FRAM_TIME_RECLOSE_RIGHT);
	  VYP (VENTIL_AKTIVACNI);
	  VYP (VENTIL_ZAVIRANI_PRAVY);
	  movingState = MOVE_RECLOSE_RIGHT_WAIT;
	  openRightMove ();
	  break;
	}
      if (!(isOn (KONCAK_ZAVRENI_PRAVY)))
	break;
      flag = 1;
      // close dome..
    case MOVE_CLOSE_LEFT:
      closeLeft ();
      break;
    case MOVE_CLOSE_LEFT_WAIT:
      if (checkMotorTimeout ())
	{
	  if (reclosing_num >= FRAM_RECLOSING_MAX)
	    {
	      return -2;
	    }
	  // reclose left..
	  setMotorTimeout (FRAM_TIME_RECLOSE_LEFT);
	  VYP (VENTIL_AKTIVACNI);
	  VYP (VENTIL_ZAVIRANI_LEVY);
	  movingState = MOVE_RECLOSE_LEFT_WAIT;
	  openLeftMove ();
	  break;
	}
      if (!(isOn (KONCAK_ZAVRENI_LEVY)))
	break;
      return -2;
    case MOVE_RECLOSE_RIGHT_WAIT:
      if (!(isOn (KONCAK_OTEVRENI_PRAVY) || checkMotorTimeout ()))
	break;
      reclosing_num++;
      VYP (VENTIL_AKTIVACNI);
      VYP (VENTIL_OTEVIRANI_PRAVY);
      closeRight ();
      break;
    case MOVE_RECLOSE_LEFT_WAIT:
      if (!(isOn (KONCAK_OTEVRENI_LEVY) || checkMotorTimeout ()))
	break;
      reclosing_num++;
      VYP (VENTIL_AKTIVACNI);
      VYP (VENTIL_OTEVIRANI_LEVY);
      closeLeft ();
      break;
    default:
      return -2;
    }
  if (flag)
    infoAll ();
  return FRAM_CHECK_TIMEOUT;
}

int
Rts2DevDomeFram::endClose ()
{
  int ret;
  stopMove ();
  VYP (VENTIL_ZAVIRANI_LEVY);
  VYP (VENTIL_ZAVIRANI_PRAVY);
  ret = zjisti_stav_portu ();	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  time (&lastClosing);
  if (closingNum != lastClosingNum)
    {
      if (!ret && !isOn (KONCAK_OTEVRENI_PRAVY)
	  && !isOn (KONCAK_OTEVRENI_LEVY) && isOn (KONCAK_ZAVRENI_PRAVY)
	  && isOn (KONCAK_ZAVRENI_LEVY))
	{
	  sendFramMail ("FRAM dome closed");
	}
      else
	{
	  sendFramMail
	    ("WARNING FRAM dome closed with wrong end switches state");
	}
      lastClosingNum = closingNum;
    }
  return Rts2DevDome::endClose ();
}

int
Rts2DevDomeFram::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      dome_file = optarg;
      break;
    case 'w':
      wdc_file = optarg;
      break;
    case 't':
      wdcTimeOut = atof (optarg);
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevDomeFram::isGoodWeather ()
{
  if (ignoreMeteo)
    return 1;
  if (weatherConn)
    return weatherConn->isGoodWeather ();
  return 0;
}

int
Rts2DevDomeFram::init ()
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
      logStream (MESSAGE_ERROR) << "Rts2DevDomeFram::init tcgetattr " <<
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
      logStream (MESSAGE_ERROR) << "Rts2DevDomeFram::init tcsetattr " <<
	strerror (errno) << sendLog;
      return -1;
    }

  vypni_pin (adresa[VENTIL_AKTIVACNI].port,
	     adresa[VENTIL_AKTIVACNI].pin | adresa[KOMPRESOR].pin);

  movingState = MOVE_NONE;

  if (wdc_file)
    {
      ret = openWDC ();
      if (ret)
	return ret;
    }

  weatherConn = new Rts2ConnFramWeather (5002, FRAM_WEATHER_TIMEOUT, this);
  weatherConn->init ();
  addConnection (weatherConn);

  return 0;
}

int
Rts2DevDomeFram::idle ()
{
  // resetWDC
  if (wdc_file)
    resetWDC ();
  // check for weather..
  if (isGoodWeather ())
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
      int ret;
      // close dome - don't thrust centrald to be running and closing
      // it for us
      ret = closeDome ();
      if (ret == -1)
	{
	  setTimeout (10 * USEC_SEC);
	}
      setMasterStandby ();
    }
  return Rts2DevDome::idle ();
}

Rts2DevDomeFram::Rts2DevDomeFram (int in_argc, char **in_argv):Rts2DevDome (in_argc,
	     in_argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");
  addOption ('w', "wdc_file", 1, "/dev file with watch-dog card");
  addOption ('t', "wdc_timeout", 1, "WDC timeout (default to 30 seconds");

  dome_file = "/dev/ttyS0";
  wdc_file = NULL;
  wdc_port = -1;
  wdcTimeOut = 30.0;

  domeModel = "FRAM_FORD_2";

  movingState = MOVE_NONE;

  weatherConn = NULL;

  lastClosing = 0;
  closingNum = 0;
  lastClosingNum = -1;

  sendContFailMail = false;
}

Rts2DevDomeFram::~Rts2DevDomeFram (void)
{
  stopMove ();
  if (wdc_file)
    closeWDC ();
  stopMove ();
  close (dome_port);
}

int
Rts2DevDomeFram::handle_zasuvky (int zas)
{
  int i;
  for (i = 0; i < NUM_ZAS; i++)
    {
      int zasuvka_num = zasuvky_index[i];
      if (zasuvky_stavy[zas][i] == ZAS_VYP)
	{
	  vypni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
      else
	{
	  zapni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
    }
  return 0;
}

int
Rts2DevDomeFram::ready ()
{
  return 0;
}

int
Rts2DevDomeFram::info ()
{
  int ret;
  ret = zjisti_stav_portu ();
  if (ret)
    return -1;
  sw_state = getPortState (KONCAK_OTEVRENI_PRAVY);
  sw_state |= (getPortState (KONCAK_OTEVRENI_LEVY) << 1);
  sw_state |= (getPortState (KONCAK_ZAVRENI_PRAVY) << 2);
  sw_state |= (getPortState (KONCAK_ZAVRENI_LEVY) << 3);
  rain = weatherConn->getRain ();
  windspeed = weatherConn->getWindspeed ();
  if (wdc_port > 0)
    temperature = getWDCTemp (2);
  return Rts2DevDome::info ();
}

int
Rts2DevDomeFram::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeFram::off ()
{
  closeDome ();
  //handle_zasuvky (OFF);
  return 0;
}

int
Rts2DevDomeFram::standby ()
{
  //handle_zasuvky (STANDBY);
  closeDome ();
  return 0;
}

int
Rts2DevDomeFram::observing ()
{
  //handle_zasuvky (OBSERVING);
  if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
    return openDome ();
  return 0;
}

int
Rts2DevDomeFram::sendFramMail (char *subject)
{
  char *openText;
  int ret;
  ret = zjisti_stav_portu ();
  asprintf (&openText, "%s.\n"
	    "End switch status:\n"
	    "CLOSE SWITCH RIGHT:%s CLOSE SWITCH LEFT:%s\n"
	    " OPEN SWITCH RIGHT:%s  OPEN SWITCH LEFT:%s\n"
	    "next good weather: %s UT\n"
	    "Weather::isGoodWeather %i\n"
	    "raining: %i\n"
	    "windspeed: %.2f km/h\n"
	    "port state: %i\n"
	    "closingNum: %i lastClosing: %s",
	    subject,
	    isOnString (KONCAK_ZAVRENI_PRAVY),
	    isOnString (KONCAK_ZAVRENI_LEVY),
	    isOnString (KONCAK_OTEVRENI_PRAVY),
	    isOnString (KONCAK_OTEVRENI_LEVY),
	    ctime (&nextGoodWeather),
	    isGoodWeather (), rain,
	    windspeed, ret, closingNum, ctime (&lastClosing));
  ret = sendMail (subject, openText);
  free (openText);
  return ret;
}

int
main (int argc, char **argv)
{
  Rts2DevDomeFram *device = new Rts2DevDomeFram (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}

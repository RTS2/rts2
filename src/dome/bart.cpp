#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <mcheck.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "dome.h"

#define BAUDRATE B9600
#define CTENI_PORTU 0
#define ZAPIS_NA_PORT 4
#define STAV_PORTU 0
#define CHYBA_NEZNAMY_PRIKAZ 0x80
#define CHYBA_PRETECENI_TX_BUFFERU 0x81
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2

#define CAS_NA_OTEVRENI 30

typedef enum
{ ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_4, ZASUVKA_5, ZASUVKA_6, MOTOR,
  SMER, SVETLO, TOPENI, KONCAK_OTEVRENI_JIH, KONCAK_ZAVRENI_JIH,
  KONCAK_OTEVRENI_SEVER, KONCAK_ZAVRENI_SEVER, RESET_1, RESET_2
} vystupy;

struct typ_a
{
  unsigned char port;
  unsigned char pin;
} adresa[] =
{
  {
  PORT_A, 1},
  {
  PORT_A, 2},
  {
  PORT_A, 4},
  {
  PORT_A, 8},
  {
  PORT_A, 16},
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
  PORT_B, 16},
  {
  PORT_B, 32},
  {
  PORT_B, 64},
  {
  PORT_B, 128},
  {
  PORT_C, 1},
  {
PORT_C, 32}};

#define NUM_ZAS		4

#define OFF		0
#define STANDBY		1
#define OBSERVING	2

// zasuvka c.1 kamera, c.3 kamera, c.5 montaz. c.6 topeni /Ford 21.10.04
int zasuvky_index[NUM_ZAS] = { ZASUVKA_1, ZASUVKA_3, ZASUVKA_5, ZASUVKA_6 };

enum stavy
{ ZAS_VYP, ZAS_ZAP };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
enum stavy zasuvky_stavy[3][NUM_ZAS] =
{
  // off
  {ZAS_VYP, ZAS_VYP, ZAS_VYP, ZAS_VYP},
  // standby
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP},
  // observnig
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

class Rts2DevDomeBart:public Rts2DevDome
{
private:
  int dome_port;
  char *dome_file;

  unsigned char spinac[2];
  unsigned char stav_portu[3];

  int zjisti_stav_portu (unsigned char *stav);
  void zapni_pin (unsigned char c_port, unsigned char pin);
  void vypni_pin (unsigned char c_port, unsigned char pin);
  void cekej_port (int c_port);
  int handle_zasuvky (int state);
public:
    Rts2DevDomeBart (int argc, char **argv);
    virtual ~ Rts2DevDomeBart (void);
  virtual int processOption (int in_opt);

  virtual int init ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int openDome ();
  virtual int closeDome ();

  virtual int observing ();
  virtual int standby ();
  virtual int off ();
};

Rts2DevDomeBart::Rts2DevDomeBart (int argc, char **argv):
Rts2DevDome (argc, argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");
  dome_file = "/dev/ttyS0";
}

Rts2DevDomeBart::~Rts2DevDomeBart (void)
{
  close (dome_port);
}

int
Rts2DevDomeBart::zjisti_stav_portu (unsigned char *stav)
{
  unsigned char t, c = STAV_PORTU | PORT_A;
  write (dome_port, &c, 1);
  if (read (dome_port, &t, 1) < 1)
    fprintf (stderr, "read error 0\n");
  else
    fprintf (stderr, "stav: A: %x:", t);
  read (dome_port, &stav[PORT_A], 1);
  fprintf (stderr, "%x ", stav[PORT_A]);
  c = STAV_PORTU | PORT_B;
  write (dome_port, &c, 1);
  if (read (dome_port, &t, 1) < 1)
    fprintf (stderr, "read error 1\n");
  else
    fprintf (stderr, " B: %x:", t);
  read (dome_port, &stav[PORT_B], 1);
  fprintf (stderr, "%x\n", stav[PORT_B]);
  return 0;
}

void
Rts2DevDomeBart::zapni_pin (unsigned char c_port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu (stav_portu);
  c = ZAPIS_NA_PORT | c_port;
  fprintf (stderr, "port:%xh pin:%xh zapis: %x:", c_port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[c_port] | pin;
  fprintf (stderr, "%xh\n", c);
  write (dome_port, &c, 1);
}

void
Rts2DevDomeBart::vypni_pin (unsigned char c_port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu (stav_portu);
  c = ZAPIS_NA_PORT | c_port;
  fprintf (stderr, "port:%xh pin:%xh zapis: %x:", c_port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[c_port] & (~pin);
  fprintf (stderr, "%xh\n", c);
  write (dome_port, &c, 1);
}

//
// pocká s timeoutem
void
Rts2DevDomeBart::cekej_port (int c_port)
{
  time_t t, act_t;
  time (&t);
  t += 40;			// timeout pro strechu
  do
    {
      zjisti_stav_portu (stav_portu);
      time (&act_t);
    }
  while ((stav_portu[adresa[c_port].port] & adresa[c_port].pin) && act_t < t);
}

#define ZAP(i) zapni_pin(adresa[i].port,adresa[i].pin)
#define VYP(i) vypni_pin(adresa[i].port,adresa[i].pin)

int
Rts2DevDomeBart::openDome ()
{
  VYP (MOTOR);
  sleep (1);
  VYP (SMER);
  sleep (1);
  ZAP (MOTOR);
  fprintf (stderr, "oteviram strechu\n");

  cekej_port (KONCAK_OTEVRENI_JIH);

  fprintf (stderr, "otevreno\n");
  VYP (MOTOR);
  zjisti_stav_portu (stav_portu);	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  return 0;
}

int
Rts2DevDomeBart::closeDome ()
{
  VYP (MOTOR);
  sleep (1);
  ZAP (SMER);
  sleep (1);
  ZAP (MOTOR);
  fprintf (stderr, "zaviram strechu\n");

  cekej_port (KONCAK_ZAVRENI_JIH);

  fprintf (stderr, "zavreno\n");
  VYP (MOTOR);
  sleep (1);
  VYP (SMER);
  zjisti_stav_portu (stav_portu);	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  return 0;
}

int
Rts2DevDomeBart::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      dome_file = optarg;
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevDomeBart::init ()
{
  struct termios oldtio, newtio;

  int ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  dome_port = open (dome_file, O_RDWR | O_NOCTTY);

  if (dome_port == -1)
    return -1;

  tcgetattr (dome_port, &oldtio);

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 1;

  tcflush (dome_port, TCIOFLUSH);
  tcsetattr (dome_port, TCSANOW, &newtio);

  return 0;
}

int
Rts2DevDomeBart::handle_zasuvky (int state)
{
  int i;
  for (i = 0; i < NUM_ZAS; i++)
    {
      int zasuvka_num = zasuvky_index[i];
      if (zasuvky_stavy[state][i] == ZAS_VYP)
	{
	  zapni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
      else
	{
	  vypni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
      sleep (1);		// doplnil Ford
    }
  return 0;
}

int
Rts2DevDomeBart::ready ()
{
  return 0;
}

int
Rts2DevDomeBart::info ()
{
  return 0;
}

int
Rts2DevDomeBart::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeBart::off ()
{
  closeDome ();
  handle_zasuvky (OFF);
  return 0;
}

int
Rts2DevDomeBart::standby ()
{
  handle_zasuvky (STANDBY);
  closeDome ();
  return 0;
}

int
Rts2DevDomeBart::observing ()
{
  handle_zasuvky (OBSERVING);
  openDome ();
  return 0;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevDomeBart *device = new Rts2DevDomeBart (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize dome - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}

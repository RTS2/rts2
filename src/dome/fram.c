#define _GNU_SOURCE

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

typedef enum
{ VENTIL_AKTIVACNI,
  VENTIL_OTEVIRANI_PRAVY,
  VENTIL_OTEVIRANI_LEVY,
  VENTIL_ZAVIRANI_PRAVY,
  VENTIL_ZAVIRANI_LEVY,
  KOMPRESOR,
  ZASUVKA_PRAVA,
  ZASUVKA_LEVA,
  KONCAK_OTEVRENI_PRAVY,
  KONCAK_OTEVRENI_LEVY,
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

static struct dome_info d_info;

char device[] = "/dev/ttyS1";
unsigned char spinac[2];
unsigned char stav_portu[3];
int dome_port;

int
zjisti_stav_portu (unsigned char *stav)
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
zapni_pin (unsigned char port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu (stav_portu);
  c = ZAPIS_NA_PORT | port;
  fprintf (stderr, "port:%xh pin:%xh zapis: %x:", port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[port] | pin;
  fprintf (stderr, "%xh\n", c);
  write (dome_port, &c, 1);
}

void
vypni_pin (unsigned char port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu (stav_portu);
  c = ZAPIS_NA_PORT | port;
  fprintf (stderr, "port:%xh pin:%xh zapis: %x:", port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[port] & (~pin);
  fprintf (stderr, "%xh\n", c);
  write (dome_port, &c, 1);
}

int
otevri_strechu ()
{
  zapni_pin (adresa[KOMPRESOR].port, adresa[KOMPRESOR].pin);
  sleep (1);
  zapni_pin (adresa[VENTIL_OTEVIRANI_LEVY].port,
	     adresa[VENTIL_OTEVIRANI_LEVY].pin);
  zapni_pin (adresa[VENTIL_AKTIVACNI].port, adresa[VENTIL_AKTIVACNI].pin);
  do
    zjisti_stav_portu (stav_portu);	//hodil by se jeste timeout
  while (stav_portu[adresa[KONCAK_OTEVRENI_LEVY].port] &
	 adresa[KONCAK_OTEVRENI_LEVY].pin);
//    while((stav_portu[adresa[KONCAK_OTEVRENI_PRAVY].port]) & adresa[KONCAK_ZAVRENI_LEVY].pin);
  vypni_pin (adresa[VENTIL_AKTIVACNI].port, adresa[VENTIL_AKTIVACNI].pin);
  vypni_pin (adresa[VENTIL_OTEVIRANI_LEVY].port,
	     adresa[VENTIL_OTEVIRANI_LEVY].pin);
  sleep (1);
  zapni_pin (adresa[VENTIL_OTEVIRANI_PRAVY].port,
	     adresa[VENTIL_OTEVIRANI_PRAVY].pin);
  zapni_pin (adresa[VENTIL_AKTIVACNI].port, adresa[VENTIL_AKTIVACNI].pin);
  do
    zjisti_stav_portu (stav_portu);
  while (stav_portu[adresa[KONCAK_OTEVRENI_PRAVY].port] &
	 adresa[KONCAK_OTEVRENI_PRAVY].pin);
  vypni_pin (adresa[VENTIL_AKTIVACNI].port,
	     adresa[VENTIL_AKTIVACNI].pin | adresa[KOMPRESOR].pin);
  vypni_pin (adresa[VENTIL_OTEVIRANI_PRAVY].port,
	     adresa[VENTIL_OTEVIRANI_PRAVY].pin);
  zjisti_stav_portu (stav_portu);	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  return 0;
}

int
zavri_strechu ()
{
  zapni_pin (adresa[KOMPRESOR].port, adresa[KOMPRESOR].pin);
  sleep (1);
  zapni_pin (adresa[VENTIL_ZAVIRANI_LEVY].port,
	     adresa[VENTIL_ZAVIRANI_LEVY].pin);
  zapni_pin (adresa[VENTIL_AKTIVACNI].port, adresa[VENTIL_AKTIVACNI].pin);
  do
    zjisti_stav_portu (stav_portu);
  while (stav_portu[adresa[KONCAK_ZAVRENI_LEVY].port] &
	 adresa[KONCAK_ZAVRENI_LEVY].pin);
  vypni_pin (adresa[VENTIL_AKTIVACNI].port, adresa[VENTIL_AKTIVACNI].pin);
  vypni_pin (adresa[VENTIL_ZAVIRANI_LEVY].port,
	     adresa[VENTIL_ZAVIRANI_LEVY].pin);
  sleep (1);
  zapni_pin (adresa[VENTIL_ZAVIRANI_PRAVY].port,
	     adresa[VENTIL_ZAVIRANI_PRAVY].pin);
  zapni_pin (adresa[VENTIL_AKTIVACNI].port, adresa[VENTIL_AKTIVACNI].pin);
  do
    zjisti_stav_portu (stav_portu);
  while (stav_portu[adresa[KONCAK_ZAVRENI_PRAVY].port] &
	 adresa[KONCAK_ZAVRENI_PRAVY].pin);
  vypni_pin (adresa[VENTIL_AKTIVACNI].port,
	     adresa[VENTIL_AKTIVACNI].pin | adresa[KOMPRESOR].pin);
  vypni_pin (adresa[VENTIL_ZAVIRANI_PRAVY].port,
	     adresa[VENTIL_ZAVIRANI_PRAVY].pin);
  zjisti_stav_portu (stav_portu);	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  return 0;
}

int
dome_init (char *dome_file)
{
  struct termios oldtio, newtio;
  dome_port = open (dome_file, O_RDWR | O_NOCTTY);

  tcgetattr (dome_port, &oldtio);

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 1;

  tcflush (dome_port, TCIOFLUSH);
  tcsetattr (dome_port, TCSANOW, &newtio);

  strcpy (d_info.type, "FRAM2_DOME");
  d_info.temperature = nan ("0");
  d_info.humidity = nan ("0");
  d_info.dome = 4;
  d_info.power_telescope = 1;
  d_info.power_cameras = 1;

  return !!dome_port;
}

int
dome_done ()
{
  close (dome_port);
  return 0;
}

int
handle_zasuvky (int state)
{
  int i;
  for (i = 0; i < NUM_ZAS; i++)
    {
      int zasuvka_num = zasuvky_index[i];
      if (zasuvky_stavy[state][i] == ZAS_VYP)
	{
	  vypni_pin (adresa[zasuvky_index[zasuvka_num]].port,
		     adresa[zasuvky_index[zasuvka_num]].pin);
	}
      else
	{
	  zapni_pin (adresa[zasuvky_index[zasuvka_num]].port,
		     adresa[zasuvky_index[zasuvka_num]].pin);
	}
    }
  return 0;
}

int
dome_info (struct dome_info **info)
{
  *info = &d_info;
  return 0;
}

int
dome_off ()
{
  zavri_strechu ();
  handle_zasuvky (OFF);
  d_info.power_telescope = 0;
  d_info.power_cameras = 0;
  return 0;
}

int
dome_standby ()
{
  handle_zasuvky (STANDBY);
  d_info.power_telescope = 0;
  d_info.power_cameras = 1;
  zavri_strechu ();
  return 0;
}

int
dome_observing ()
{
  handle_zasuvky (OBSERVING);
  d_info.power_telescope = 1;
  d_info.power_cameras = 1;
  otevri_strechu ();
  return 0;
}

/*! 
 * @file Dome control deamon. 
 * $Id$
 * @author petr
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mcheck.h>
#include <time.h>
#include <termios.h>
#include <syslog.h>

#include "dome.h"

#define BAUDRATE B9600

/* Co mame k dispozici... ostatni indexy neznam... :) */
char zasuvka[] = { 0x9, 0x8, 0x5, 0x4, 0x3 };

//char zasuvka[]={0x00,0x01,0x02,0x6,0x7,0xc,0xd,0xe,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
#define STR_SMER	0xa	// 0x2a zavrit
#define STR_MOTOR	0xb	// 0x2b zapnout

// Jsou trochu pomichane...
#define ZAVRIT	0x20
#define OTEVRIT	0x40
#define MOTOR_ZAP	0x20
#define MOTOR_VYP	0x40
#define ZAS_ZAP 0x40
#define ZAS_VYP 0x20

#define DOME_SLEEP		20

int dome_port = -1;

#define NUM_ZAS		3

#define OFF		0
#define STANDBY		1
#define OBSERVING	2

// prvni zasuvka dalekohled, druha kamery, treti chlazeni
int zasuvky_index[NUM_ZAS] = { 0, 1, 2 };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
int zasuvky_stavy[3][NUM_ZAS] = {
  // off
  {ZAS_VYP, ZAS_VYP, ZAS_VYP},
  // standby
  {ZAS_ZAP, ZAS_ZAP, ZAS_VYP},
  // observnig
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

static struct dome_info d_info;

// Vraci to, co write: mela by vratit 1, pokud vrati 0
// nebo -1, je neco spatne
int
rele (int rele, int action)
{
  char c;

  c = rele | action;
  usleep (5000);
  syslog (LOG_DEBUG, "writing %x (%x|%x) to %i\n", c, rele, action,
	  dome_port);
  return write (dome_port, &c, 1);
}

int
zavri ()
{
  int ret = 1;
  syslog (LOG_DEBUG, "closing");
  if ((ret = rele (STR_SMER, ZAVRIT)) < 1)
    return ret;
  if ((ret = rele (STR_MOTOR, MOTOR_ZAP)) < 1)
    return ret;
  d_info.dome = 2;
  sleep (DOME_SLEEP);
  if ((ret = rele (STR_MOTOR, MOTOR_VYP)) < 1)
    return ret;
  ret = rele (STR_SMER, OTEVRIT);
  d_info.dome = 0;
  syslog (LOG_DEBUG, "closed");
  return ret;
}

int
otevri ()
{
  int ret = 1;
  syslog (LOG_DEBUG, "opening");
  if ((ret = rele (STR_SMER, OTEVRIT)) < 1)
    return ret;
  if ((ret = rele (STR_MOTOR, MOTOR_ZAP)) < 1)
    return ret;
  d_info.dome = 3;
  sleep (DOME_SLEEP);
  ret = rele (STR_MOTOR, MOTOR_VYP);
  d_info.dome = 1;
  syslog (LOG_DEBUG, "opened");
  return ret;
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
  newtio.c_cc[VTIME] = 0;

  tcflush (dome_port, TCIOFLUSH);
  tcsetattr (dome_port, TCSANOW, &newtio);

  strcpy (d_info.type, "BART_DOME");
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
      rele (zasuvka[zasuvka_num], zasuvky_stavy[state][i]);
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
  zavri ();
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
  zavri ();
  return 0;
}

int
dome_observing ()
{
  handle_zasuvky (OBSERVING);
  d_info.power_telescope = 1;
  d_info.power_cameras = 1;
  otevri ();
  return 0;
}

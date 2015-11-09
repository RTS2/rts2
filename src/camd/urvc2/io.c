/* io.c
 *
 * This file contains definition of realtime setting/resetting functions and
 * debugging versions of inportb/outportb; non-debugging versions of latter
 * functions are as static inline definitions in mardrv.h
 *
 * Martin Jelinek, January-March 2002
 */

#ifndef _IO_C
#define _IO_C
//#define DEBUG


#include <sys/mman.h>		// m[un]lockall()
#include <sched.h>		// for scheduler
#include <stdio.h>		// fprintf, stderr, perror
#include <stdlib.h>		// exit
#include <unistd.h>
#include <sys/time.h>
#include "urvc.h"

// address to work with
unsigned short baseAddress = 0x378;
// parport output cycle duration (in us*16)
unsigned short pp_ospeed = 20;	// default let be 1.25 us
// parport input cycle duration
unsigned short pp_ispeed = 20;	// default let be 1.25 us

#ifndef DEBUG

#include <sys/io.h>		// for iopl()

#else
extern int IO_LOG;		// marmicro.c

unsigned char
inb (int port)
{
  unsigned char _v;
  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (port));
  return _v;
}

void
outb (int value, int port)
{
  __asm__ __volatile__ ("outb %b0,%w1"::"a" (value), "Nd" (port));
}

unsigned char
inportb (int port)
{
  unsigned char data;
  data = inb (port);
  if (IO_LOG)
    {
      if (port == 0x379)	// Ladit pouze na LPT1, nebo tady upravit, nebo spravne
	// predelat
	fprintf (stderr, "-x%02x\n", data >> 3);	// fflush(stdout);
      else
	fprintf (stderr, "%02xX%02x\n", port, data);	// fflush(stdout);
    }
  return data;
}

void
outportb (int port, int data)
{
  if (IO_LOG)
    {
      if (port == 0x378)	// std: LPT1 - parport0
	fprintf (stderr, "%01x%c%01x ", (data >> 4) & 0x7, (data >> 7) ? 'w' : 'o', data & 0xf);	// fflush(stdout);
      else
	fprintf (stderr, "%03xO%02x ", port, data);	// fflush(stdout);
    }
  return outb (data, port);
}
#endif // DEBUG

double
mSecCount ()
{
  struct timeval tv;

  gettimeofday (&tv, 0);
  return ((double) tv.tv_sec * 1000.0 + (double) tv.tv_usec / 1000.0);
}

void
measure_pp ()
{
  double timer2, timer3, timer4;
  int i;

  timer2 = mSecCount ();
  for (i = 0; i < 16000; i++)
    inportb (baseAddress);
  timer3 = mSecCount ();
  for (i = 0; i < 16000; i++)
    outportb (baseAddress, 0);
  timer4 = mSecCount ();

  pp_ispeed = (int) ((timer3 - timer2) + .5);
  pp_ospeed = (int) ((timer4 - timer3) + .5);

  printf ("I/O cycle time: %.2f/%.2fus\n", (timer3 - timer2) / 16,
	  (timer4 - timer3) / 16);
}

void
begin_realtime ()
{
#if _POSIX_PRIORITY_SCHEDULING
  struct sched_param s;
#endif

  if (iopl (3) < 0)
    {
      perror ("iopl(3)");
      fprintf (stderr, "this program must be run suid root (see chmod(1))\n");
      exit (1);
    }

  if (mlockall (1) < 0)
    {
      perror ("mlockall(1)");
      exit (1);
    }

#if _POSIX_PRIORITY_SCHEDULING
  s.sched_priority = 0x32;
  if (sched_setscheduler (0, 1, &s) < 0)
    {
      perror ("sched_setscheduler(0,1,&(..0x32..) )");
      exit (1);
    }
#endif
}

void
end_realtime ()
{
#if _POSIX_PRIORITY_SCHEDULING
  struct sched_param s;

  s.sched_priority = 0;
  sched_setscheduler (0, 0, &s);
#endif

  munlockall ();
  iopl (0);
}

void
CameraInit (int base)
{
#ifdef DEBUG
  if (IO_LOG)
    fprintf (stderr, "DisableLPTInterrupts: base=%x\n", base);
#endif
  baseAddress = base;
  printf ("Using SPP on 0x%x\n", baseAddress);

  CameraOut (0x60, 1);
  //ForceMicroIdle();
  CameraOut (0x30, 0);
  usleep (100000);

  // ECPSetMode(0);
  outportb ((base + 2), 0xe);

//      outportb(baseAddress + 2, 4);
//      outportb(baseAddress + 2, 4);
  outportb (baseAddress + 0x402, 5);
  outportb (baseAddress, 0);
  outportb (baseAddress + 2, 4);

  measure_pp ();
}

unsigned char
CameraIn (unsigned char reg)
{
  unsigned char val;

#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
#endif

  outportb (baseAddress, reg);
  val = inportb (baseAddress + 1) >> 3;

#ifdef DEBUG
  IO_LOG = IO_LOG_bak;
  if (IO_LOG)
    fprintf (stderr, "%02X<%02X ", reg, val);
#endif

  return val;
}

void
CameraOut (unsigned char reg, int val)
{
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
#endif

  outportb (baseAddress, reg + val);
  outportb (baseAddress, reg + val + 0x80);
  outportb (baseAddress, reg + val + 0x80);
  outportb (baseAddress, reg + val);

  if (reg == 0x10)
    imaging_clocks_out = val;
  if (reg == 0x30)
    control_out = val;

#ifdef DEBUG
  IO_LOG = IO_LOG_bak;
  if (IO_LOG)
    fprintf (stderr, "%02X>%02X ", reg, val);
#endif
}

PAR_ERROR
CameraReady ()
{
  int t0 = 0;
  outportb (baseAddress, 0);
  while (1)
    {
      if ((inportb (baseAddress + 1) & 0x80) == 0)
	break;
      if (t0++ > 500)
	return 0xd;
    }
  return CE_NO_ERROR;
}

// Line transfer delay... 1.25us (i.e. one inportb) should be enough...
void
IODelay (short i)
{
  for (; i > 0; i--)
    inportb (baseAddress);
}

#endif

/* mario.c
 *
 * This file contains definition of realtime setting/resetting functions and
 * debugging versions of inportb/outportb; non-debugging versions of latter
 * functions are as static inline definitions in mardrv.h
 *
 * Martin Jelinek, January-March 2002
 */

#define _MARIO_C

#include <sys/io.h>		// for iopl()
#include <sys/mman.h>		// m[un]lockall()
#include <sched.h>		// for scheduler
#include <stdio.h>		// fprintf, stderr, perror
#include <stdlib.h>		// exit
#include <unistd.h>
#include "mardrv.h"

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

#ifdef DEBUG
extern int IO_LOG;		// marmicro.c

/*
unsigned char __inb(int port)
{
    unsigned char _v;
    __asm__ __volatile__("inb %w1,%0":"=a"(_v):"Nd"(port));
    return _v;
}

void __outb(int value, int port)
{
    __asm__ __volatile__("outb %b0,%w1"::"a"(value), "Nd"(port));
}*/

/*unsigned char
inportb (int port)
{
  unsigned char data;
  data = __inb (port);
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
  return __outb (data, port);
}*/
#endif // DEBUG

#define _PARECP_C
//#define DEBUG

#include <stdio.h>
#include <sys/io.h>
#include "mardrv.h"

unsigned char ecpControl = 0;
unsigned char bytesAvail = 0;
unsigned short st7GKHWHandshake;

#ifdef DEBUG
extern int IO_LOG;
#define RETURN(a) {IO_LOG=IO_LOG_bak; return(a);}
#else
#define RETURN(a) return(a);
#endif

void
ECPSetMode (ECP_MODE mode)
	// ECP_MODE mode
{
//      short  i;
//      unsigned char c;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "ECPSetMode %d\n", mode);
#endif
//      printf("ECPSetMode %d",mode);

  // if(mode <= 6) 
  switch (mode)			// goto ( *(mode * 4 + 0 + 0x8057e48));
    {
    case ECP_NORMAL:

      ecpControl |= 4;
      outportb (baseAddress + 2, ecpControl & 0xff);
      ecpControl &= 0xdf;
      outportb (baseAddress + 2, ecpControl & 0xff);
      outportb (baseAddress + 0x402, 5);
      outportb (baseAddress, 0);
      ecpControl = 4;
      outportb (baseAddress + 2, 4);
      break;

    case ECP_CLEAR:		// 1

      ECPSetMode (ECP_NORMAL);
      outportb (baseAddress + 2, 0xc);
      outportb (baseAddress, 2);
      outportb (baseAddress + 2, 0xd);
      outportb (baseAddress + 2, 0xc);
      ECPSetMode (ECP_INPUT);
      ecpControl = 0x28;
      outportb (baseAddress + 2, 0x28);
      break;

    case ECP_BIN1:		// 2

      ECPSetMode (ECP_NORMAL);
      outportb (baseAddress + 2, 0xc);
      outportb (baseAddress, 0);
      outportb (baseAddress + 2, 0xd);
      ecpControl = 0xc;
      outportb (baseAddress + 2, 0xc);
      ECPSetMode (ECP_FIFO_INPUT);
      break;

    case ECP_BIN2:		// 3

      ECPSetMode (ECP_NORMAL);
      outportb (baseAddress + 2, 0xc);
      outportb (baseAddress, 1);
      outportb (baseAddress + 2, 0xd);
      ecpControl = 0xc;
      outportb (baseAddress + 2, 0xc);
      ECPSetMode (ECP_FIFO_INPUT);
      break;

    case ECP_BIN3:		// 4

      ECPSetMode (ECP_NORMAL);
      outportb (baseAddress + 2, 0xc);
      outportb (baseAddress, 3);
      outportb (baseAddress + 2, 0xd);
      ecpControl = 0xc;
      outportb (baseAddress + 2, 0xc);
      ECPSetMode (ECP_FIFO_INPUT);
      break;

    case ECP_FIFO_INPUT:	// 5 

      if (st7GKHWHandshake == 0)
	{
	  ECPSetMode (ECP_INPUT);
	  break;
	}
      ecpControl |= 0x20;
      outportb (baseAddress + 2, ecpControl & 0xff);
      outportb (baseAddress + 0x402, 0x64);
      ECPDumpFifo ();
      ecpControl &= 0xfb;
      outportb (baseAddress + 2, ecpControl & 0xff);
      break;

    case ECP_INPUT:		// 6

      outportb (baseAddress + 0x402, 0x25);
      outportb (baseAddress + 2, 0x2c);
      outportb (baseAddress + 2, 0x2e);
      ecpControl = 0x2a;
      outportb (baseAddress + 2, 0x2a);
      break;
    }
#ifdef DEBUG
  IO_LOG = IO_LOG_bak;
#endif
}

PAR_ERROR
ECPDumpFifo ()
	// -
{
  int t0;			// , i = 0;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "ECPDumpFifo\n");
#endif

  outportb (baseAddress + 0x402, 0xe5);
  outportb (baseAddress + 0x401, 0);
  outportb (baseAddress + 0x402, 0x64);

  for (t0 = 1000; t0 > 0; t0--)
    {
      if (inportb (baseAddress + 0x402) & 1)
	break;
      inportb (baseAddress + 0x400);
    }

  if (t0 != 0)
    {
      RETURN (0);
    }
  else
    {
      RETURN (0xd);
    }
}

PAR_ERROR
ECPGetPixel (unsigned short *vidPtr)
			// unsigned short *vidPtr
{
  int t0 = 0;
  unsigned char c, c1 = 0, c2;
  unsigned short vid = 0;
  int abrt = 0, done = 0, rxState = 0;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "ECPGetPixel\n");
#endif

  if (st7GKHWHandshake)
    {
      if (bytesAvail < 2)
	{
	  do
	    if (t0++ > 499)
	      {
		RETURN (0xd);
	      }
	  while ((inportb (baseAddress + 0x402) & 2) == 0);
	  bytesAvail = 0xe;
	}

      c1 = inportb (baseAddress + 0x400);
      c2 = inportb (baseAddress + 0x400);

      vid = (c2 << 8) + c1;
      vid = vid >> 2;
      vid = vid ^ 0x20;

      bytesAvail -= 2;
    }
  else
    {
      bytesAvail = 0;
      outportb (baseAddress + 2, ecpControl | 2);

      do
	{
	  if (t0++ > 499)
	    abrt = 1;
	  else
	    switch (rxState)
	      {
	      case 0:
	      case 2:
		c = inportb (baseAddress + 1);
		if ((c & 0x40) != 0)
		  break;
		outportb (baseAddress + 2, ecpControl & 0xfd);
		rxState++;
		break;

	      case 1:
		c = inportb (baseAddress + 1);
		if ((c & 0x40) == 0)
		  break;
		c1 = inportb (baseAddress);
		outportb (baseAddress + 2, ecpControl | 2);
		rxState++;
		break;

	      case 3:
		c = inportb (baseAddress + 1);
		if ((c & 0x40) == 0)
		  break;
		c2 = inportb (baseAddress);

		vid = (c2 << 8) + c1;
		vid = vid >> 2;
		vid = vid ^ 0x20;

		done = 1;
		break;
	      }
	}
      while (done == 0 && abrt == 0);
    }

  *vidPtr = vid;

  if (abrt)
    RETURN (0xd);
  RETURN (0);
}

MY_LOGICAL
ECPDetectHardware ()
{
  MY_LOGICAL abrt;
  int j;
  int t0;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "ECPDetectHardware\n");
#endif

  abrt = 0;
  ECPSetMode (ECP_CLEAR);
  for (j = 0; j <= 4; j++)
    {
      if (abrt != 0)
	{
	  ECPSetMode (ECP_NORMAL);
	  RETURN (0xd);
	}

      outportb (baseAddress + 2, ecpControl | 2);
      // *$*\/
      t0 = 0;
      while (1)
	{
	  if ((inportb (baseAddress + 1) & 0x40) == 0)
	    break;
	  if (t0++ > 500)
	    abrt = 1;
	}
      // *$*/\ //

      outportb (baseAddress + 2, ecpControl & 0xfd);
      // *$*\/
      t0 = 0;
      while (1)
	{
	  if ((inportb (baseAddress + 1) & 0x40) == 0)
	    break;
	  if (t0++ > 500)
	    abrt = 1;
	}
      // *$*/\ //
    }
  ECPSetMode (ECP_NORMAL);
  RETURN (abrt);		// Tohle taky bylo jinak, ale nevim... return
  // ECPSetMode(ECP_NORMAL)); ktery je void?
}

PAR_ERROR
ECPClear1Group ()
{
  short t0;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "ECPClear1Group\n");
#endif

  for (t0 = 0; t0 < 500; t0++)	// <=0x1f3 == 499
    if ((inportb (baseAddress + 1) & 0x40) != 0)
      {
	outportb (baseAddress + 2, ecpControl);
	for (t0 = 0; t0 < 500; t0++)	// <=0x1f3 == 499
	  if ((inportb (baseAddress + 1) & 0x40) != 0)
	    RETURN (0);
      }
  ECPSetMode (ECP_NORMAL);
  RETURN (0xd);
}

PAR_ERROR
ECPWaitPCLK1 ()
{
  short t0;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "ECPWaitPCLK1\n");
#endif

  for (t0 = 0; t0 < 500; t0++)	// <=0x1f3 == 499
    if ((inportb (baseAddress + 1) & 0x40) != 0)
      ECPSetMode (ECP_NORMAL);
  RETURN (0xd);
}

#define _CCD_IMAGING_C
//#define DEBUG

#include <stdio.h>
#include <sys/io.h>

#include "mardrv.h"
#include "ccd_macros.h"

#ifdef DEBUG
int IO_LOG2 = 1;
extern int IO_LOG;
#define RETURN(a) {return(a);}
#else
#define RETURN(a) return(a)
#endif

extern unsigned char ecpControl;
extern unsigned short st7GKHWHandshake;
extern int st7GKEnabled;

unsigned short trackingBias = 0, imagingBias = 0;
unsigned char *last_line1;
unsigned short hot_count;

//#define DO_ST5C
//#define DO_ST237

#ifdef DO_ST5C
#include "marccd_st5c.c"
#endif

#ifdef DO_ST237
#include "marccd_st237.c"
#endif

PAR_ERROR
SPPGetPixel (unsigned short *vidPtr, CCD_ID chip)
{
  int t0 = 0;
  // unsigned char c, c1, c2;
  unsigned short u;
  // int abrt = 0, done = 0, rxState = 0;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "SPPGetPixel\n");
#endif

//   outportb(0x200,0);

  // asi to tu bejt nemusi... (skoro vzdycky se to stiha driv, chyba je ovsem problem... :(
  WAIT_4_CAMERA ();
  CAMERA_PULSE (chip);

  // Get short
  outportb (baseAddress, 0x00);
  u = (inportb (baseAddress + 1) & 0x78) >> 3;
  outportb (baseAddress, 0x10);
  u += (inportb (baseAddress + 1) & 0x78) << 1;
  outportb (baseAddress, 0x20);
  u += (inportb (baseAddress + 1) & 0x78) << 5;
  outportb (baseAddress, 0x30);
  u += (inportb (baseAddress + 1) & 0x78) << 9;

  *vidPtr = u;

  RETURN (CE_NO_ERROR);
}

void
IODelay (short i)
{
  for (; i > 0; i--)
    inportb (baseAddress);
}

PAR_ERROR
BlockClearPixels (int len, int readoutMode)
	// short len, readoutMode;
{
  PAR_ERROR res = CE_NO_ERROR;
  short j;
  short t0;
  short bulk;
  short individual;
//      unsigned short u;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "BlockClearPixels\n");
  fflush (stderr);
#endif

/*
	if(st7GKEnabled)	// == 0) { goto L080535c1; }
	{
		bulk = (len + 7) / 8;		// Ale proc vlastne len + 7 ???
		individual = (len + 7) % 8;
*/
  /* Bodlo by vedet co ty vypocty znamenaji :) XXX: L08053437: if(len > = 0)
     { goto L0805343f; } L08053443: bulk = eax + 7 >> 3; if(len > = 0) {
     goto L08053455; } L08053459: 8 = (edx + 7 >> 3) * cx; individual = eax - 
     ecx; */
/*
		if(bulk)
		{
			ECPSetMode(ECP_CLEAR);
			if(bulk == 1) 		// Aargh@ - proc si vybira?
			{
				if( (res = ECPWaitPCLK1()) ) return res;
			}
			else
			{
				for(j = bulk; j > 0; j--)
					if( (res = ECPClear1Group()) )return res;

				if( (res = ECPWaitPCLK1()) )return res;
			}
			ECPSetMode(ECP_NORMAL);	
		}

		if(individual)
		{
		fprintf(stderr,"BCP - B1\n");fflush(stderr);
			st7GKHWHandshake = 0;

			switch(readoutMode)
			{
				case 0:
					ECPSetMode(ECP_BIN1);
					break;
				case 1:
					ECPSetMode(ECP_BIN2);
					break;
				case 2:
					ECPSetMode(ECP_BIN3);
					break;
			}

			for(j = individual; j > 0; j++)
				if( (res = ECPGetPixel(&u)) )
				{	// Chyba - nepovedlo se precist pixel!
					ECPSetMode(ECP_NORMAL);
					st7GKHWHandshake = 0;
					break; // might be return ret for clarity
				}
		}
	}
	else
*/
  {
    CameraOut (0x30, 0);
    CameraOut (0, 8);

    bulk = len / 10;
    individual = len % 10;

    /* Zda se, ze se zase jedna o deleni deseti XXX: 0x6667 = (len &
       0xffff) * ecx; cx = ecx >> 0x10 >> 2; bulk = ecx - ((len & 0xffff)
       >> 0xf); 0x6667 = (len & 0xffff) * ecx; cx = ecx >> 0x10 >> 2;
       individual = (len & 0xffff) - (ecx - ((len & 0xffff) >> 0xf) << 2) + 
       ecx + (ecx - ((len & 0xffff) >> 0xf) << 2) + ecx; */

    for (j = bulk; j > 0; j--)
      {
	CAMERA_PULSE (0);
	WAIT_4_CAMERA ();
      }

    switch (readoutMode)
      {
      case 0:
	CameraOut (0, 0);
	break;
      case 1:
	CameraOut (0, 4);
	break;
      case 2:
	CameraOut (0, 0xc);
	break;
      }

    for (j = individual; j > 0; j--)
      {
	CAMERA_PULSE (0);
	WAIT_4_CAMERA ();
      }
  }

  return res;
}

PAR_ERROR
DigitizeImagingLineGK (int left, int len, int right,
		       int horzBin, int onVertBin, int offVertBin,
		       unsigned short *dest, int subtract, int clearWidth)
{
  fprintf (stderr, "todo: DigitizeImagingLineGK\n");
  return DigitizeImagingLine (left, len, right, horzBin, onVertBin,
			      offVertBin, dest, subtract, clearWidth);
}

PAR_ERROR
DigitizeImagingLine (int left, int len, int right,
		     int horzBin, int onVertBin, int offVertBin,
		     unsigned short *dest, int subtract, int clearWidth)
{
  // short left, len, right, horzBin, onVertBin, offVertBin, clearWidth;
  // MY_LOGICAL subtract;

  // subtract byl vzdy 0; 
  // offVertBin vzdy 1;
  // onVertBin a horzBin si byly rovne a byly rovne zadanemu binu
  // left + right = clearWidth;
  // len = sirka pozadovaneho obrazu == W
  // left = 15 + levy sloupec == 15 + L * bin 

  PAR_ERROR res = CE_NO_ERROR;
  short i, t0;			// , v;
  unsigned short u;
  // unsigned short * up;
  // long d;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "DigitizeImagingLine\n");
#endif

  if (len > 0x5fa)
    len = 0x5fa;

  if ((res = BlockClearPixels (clearWidth, 0)))
    RETURN (res);

  CameraOut (0x30, 0);
  CameraOut (0x10, 8);

  for (i = 0; i < onVertBin; i++)
    {
      CameraOut (0x10, 9);
      IODelay (IODELAYCONST);
      CameraOut (0x10, 0xa);
      IODelay (IODELAYCONST);
      CameraOut (0x10, 9);
      IODelay (IODELAYCONST);
      CameraOut (0x10, 8);
      IODelay (IODELAYCONST);
    }
  CameraOut (0x10, 0);

  if ((res = BlockClearPixels (14, 0)))
    RETURN (res);
//      if( (res = BlockClearPixels(left, 0)) ) RETURN(res);
  if ((res = BlockClearPixels (2, horzBin - 1)))
    RETURN (res);
  if ((res = BlockClearPixels (left, horzBin - 1)))
    RETURN (res);

  switch (horzBin)
    {
    case 2:
      CameraOut (0, 4);
      // @ ECPSetMode(ECP_BIN2);
      break;
    case 3:
      CameraOut (0, 0xc);
      // @ ECPSetMode(ECP_BIN3);
      break;
    default:
      CameraOut (0, 0);
      // @ECPSetMode(ECP_BIN1);
      break;
    }

  WAIT_4_CAMERA ();		// Jsem pridal, kdyz jsem zrusil W4C v SPP_GET_PIXEL
  for (i = 0; i < len; i++)
    {
      if ((res = SPPGetPixel (&u, MAIN_CCD)))
	{
	  /* ECPSetMode(ECP_NORMAL); */
	  RETURN (res);
	}
      // @ if(res = ECPGetPixel( & u)) 
      // @ { 
      // @ ECPSetMode(ECP_NORMAL); 
      // @ RETURN(res); 
      // @ }

      *dest = u;
      dest++;
    }
  WAIT_4_CAMERA ();
  // @ ECPSetMode(ECP_NORMAL);
  // A tady byl jeste nejaky kod nevim na co, mozna na dojeti radky...
  // (Ale to snad neni nutne...) - viz marccd_n.c
  RETURN (res);
}

PAR_ERROR
DumpImagingLines (int width, int len, int vertBin)
	// short width, len, vertBin;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, j;
  unsigned char ic = 4;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  fprintf (stderr, "hole_in: DumpImagingLines\n");
#endif

  CameraOut (0x30, 0);

  for (i = 0; i < len; i++)
    {

      CameraOut (0x10, (ic | 8) & 0xff);

      for (j = 0; j < vertBin; j++)
	{
	  CameraOut (0x10, (ic | 9) & 0xff);
	  IODelay (0xa);
	  CameraOut (0x10, (ic | 0xa) & 0xff);
	  IODelay (0xa);
	  CameraOut (0x10, (ic | 9) & 0xff);
	  IODelay (0xa);
	  CameraOut (0x10, (ic | 8) & 0xff);
	  IODelay (0xa);
	}

      CameraOut (0x10, (ic | 8) & 0xff);

      if (i % 10 == 4 || i >= len - 3)
	if ((res = BlockClearPixels (width, 0)))
	  RETURN (res);

      /* 
       * &&&
       XXX:
       0x6667 = (i & 0xffff) * ecx;
       cx = ecx >> 0x10 >> 1;
       if((i & 0xffff) - (ecx - ((i & 0xffff) >> 0xf) << 2) + ecx == 4) 

       goto L08053200; 
       L080531f6:
       if(i > = len - 3)
       goto L08053200; 
       L080531fa:
       goto L08053250;
       (save)0;
       Vffffffe8 = width + 9;
       ecx = 0x66666667;
       eax = Vffffffe8;
       asm("imul ecx");
       ecx = edx >> 2;
       if(res = BlockClearPixels((ecx - (Vffffffe8 >> 0x1f) << 2) + edx + (ecx - (Vffffffe8 >> 0x1f) << 2) + edx))
       RETURN(res);
       * &&&
       */

    }
  RETURN (res);
}

PAR_ERROR
ClearImagingArray (int height, int width, int times, int left)
	// short height, width, times, left;
{
  PAR_ERROR err = CE_NO_ERROR;
  short i, t0;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "ClearImagingArray\n");
#endif

  CameraOut (0x30, 0);
  CameraOut (0, 8);
  times *= height;

  for (i = 0; i < times; i++)
    {
      CameraOut (0x10, 8);
      IODelay (0xa);
      CameraOut (0x10, 9);
      IODelay (0xa);
      CameraOut (0x10, 0xa);
      IODelay (0xa);
      CameraOut (0x10, 9);
      IODelay (0xa);
      CameraOut (0x10, 8);
      IODelay (0xa);
      CameraOut (0x10, 0);
      IODelay (0xa);

      CAMERA_PULSE (0);
      WAIT_4_CAMERA ();
    }
//      err = MeasureImagingBias(width, left);
  SetVdd (0);
  RETURN (err);
}

PAR_ERROR
ClearImagingArrayGK (int height, int width, int times, int left)
	// short height, width, times, left;
{
  PAR_ERROR err = CE_NO_ERROR;
  short i;			// , t0;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "ClearImagingArrayGK\n");
#endif

  CameraOut (0x30, 0);
  CameraOut (0, 8);
  times *= height;
  for (i = 0; i < times; i++)
    {
      CameraOut (0x10, 8);
      IODelay (0xa);
      CameraOut (0x10, 9);
      IODelay (0xa);
      CameraOut (0x10, 0xa);
      IODelay (0xa);
      CameraOut (0x10, 9);
      IODelay (0xa);
      CameraOut (0x10, 8);
      IODelay (0xa);
      CameraOut (0x10, 0);
      IODelay (0xa);

      ECPSetMode (ECP_CLEAR);
      ECPClear1Group ();
      ECPWaitPCLK1 ();
      ECPSetMode (ECP_NORMAL);
    }
  err = MeasureImagingBiasGK (width, left);
  SetVdd (0);
  RETURN (err);
}

PAR_ERROR
OffsetImagingArray (int height, int width, unsigned short *offset, int left)
	// short height, width, left;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, t0;
  unsigned short u;
  unsigned long ul;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "OffsetImagingArray\n");
#endif

  if ((res = ClearImagingArray (height, width, 1, left)))
    RETURN (res);
  SetVdd (1);
  if ((res = BlockClearPixels (width, 0)))
    RETURN (res);
  if ((res = BlockClearPixels (2, 0)))
    RETURN (res);

  CameraOut (0, 0);
  CameraOut (0x30, 0);
  ul = 0;
  for (i = 0; i <= 0x7f; i++)
    {
      if ((res = SPPGetPixel (&u, MAIN_CCD)))
	RETURN (res);
      ul = ul + u;
    }
  *offset = (ul + 0x40) >> 7;
  WAIT_4_CAMERA ();
  SetVdd (0);
  RETURN (res);
}

PAR_ERROR
OffsetImagingArrayGK (int height, int width, unsigned short *offset, int left)
	// short height, width, left;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i;			// , t0;
  unsigned short u;
  unsigned long ul;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "OffsetImagingArrayGK\n");
#endif

  if ((res = ClearImagingArrayGK (height, width, 1, left)))
    RETURN (res);
  SetVdd (1);
  if ((res = BlockClearPixels (width, 0)))
    RETURN (res);
  if ((res = BlockClearPixels (2, 0)))
    RETURN (res);

  ECPSetMode (ECP_BIN1);
  ul = 0;
  for (i = 0; i <= 0x7f; i++)
    {
      if ((res = ECPGetPixel (&u)))
	{
	  ECPSetMode (ECP_NORMAL);
	  RETURN (res);
	}
      ul = ul + u;
    }
  ECPSetMode (ECP_NORMAL);
  *offset = (ul + 0x40) >> 7;
  SetVdd (0);
  RETURN (res);
}

PAR_ERROR
MeasureImagingBias (int clearWidth, int left)
	// short clearWidth, left;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, t0;
  unsigned short u;
  long ave;
  int width;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  fprintf (stderr, "hole_in: MeasureImagingBias\n");
#endif

  width = 0x64;
  SetVdd (1);
  if ((res = BlockClearPixels (clearWidth, 0)))
    {
      CameraOut (0x30, 0);
      CameraOut (0x10, 8);
      IODelay (0xa);
      CameraOut (0x10, 9);
      IODelay (0xa);
      CameraOut (0x10, 0xa);
      IODelay (0xa);
      CameraOut (0x10, 9);
      IODelay (0xa);
      CameraOut (0x10, 8);
      IODelay (0xa);
      CameraOut (0x10, 0);
      IODelay (0xa);

      if ((res = BlockClearPixels (left, 0)))
	if ((res = BlockClearPixels (2, 0)))
	  {
	    ave = 0;
	    CameraOut (0, 0);
	    for (i = 0; i < width; i++)
	      {
		if ((res = SPPGetPixel (&u, MAIN_CCD)))
		  RETURN (res);
		ave = ave + (u & 0xffff);
	      }

	    /* 
	     * 
	     XXX:
	     eax = width;
	     asm("cdq");    // convert doubleword to quadword: (eax -> edx:eax)
	     eax = ((edx >> 0x1f) + eax >> 1) + ave;        // z edx nejvyssi bit (znamenko?) + eax/2 + ave;
	     asm("cdq");    // a znova?
	     width = width / width; // tady to rec uz uplne zmotal, kdo vi, co jsou jaky registry...
	     edx = width % width;   // taky
	     ave = eax;             // a tohle...
	     *
	     */

	    if (ave > 99)
	      ave -= 99;
	    else
	      ave = 0;
	    imagingBias = ave;

	    WAIT_4_CAMERA ();
	  }
    }
  SetVdd (0);
  RETURN (res);
}

PAR_ERROR
MeasureImagingBiasGK (int clearWidth, int left)
	// short clearWidth, left;
{
  return CE_NO_ERROR;
}

PAR_ERROR
ClockAD (int len)
	// short len;
{
  short i, t0;
  PAR_ERROR res = CE_NO_ERROR;
  unsigned short u;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "ClockAD\n");
#endif

  CameraOut (0, 4);
  for (i = 0; i < len; i++)
    if ((res = SPPGetPixel (&u, MAIN_CCD)))
      RETURN (res);
  WAIT_4_CAMERA ();
  RETURN (res);
}

void
SetVdd (MY_LOGICAL raiseIt)
{
  unsigned char ic;
#ifdef DEBUG
  int IO_LOG2_bak;
  IO_LOG2_bak = IO_LOG2;
  IO_LOG2 = 0;
  if (IO_LOG2_bak)
    fprintf (stderr, "SetVdd(%d);\n", raiseIt);
#endif

  ic = imaging_clocks_out;
  if (raiseIt)
    CameraOut (0x10, 0);
  else
    CameraOut (0x10, 4);

  if (raiseIt && (ic & 4))
    TimerDelay (200000);
#ifdef DEBUG
  IO_LOG2 = IO_LOG2_bak;
#endif
}

PAR_ERROR
DigitizeRAWImagingLine (int len, unsigned short *dest)
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, t0;
  unsigned short u;

  if ((res = BlockClearPixels (len, 0)))
    RETURN (res);

  CameraOut (0x30, 0);
  CameraOut (0x10, 8);
  CameraOut (0x10, 9);
  IODelay (IODELAYCONST);
  CameraOut (0x10, 0xa);
  IODelay (IODELAYCONST);
  CameraOut (0x10, 9);
  IODelay (IODELAYCONST);
  CameraOut (0x10, 8);
  IODelay (IODELAYCONST);
  CameraOut (0x10, 0);
  CameraOut (0, 0);
  WAIT_4_CAMERA ();
  for (i = 0; i < len; i++)
    {
      if ((res = SPPGetPixel (&u, MAIN_CCD)))
	RETURN (res);
      *dest = u;
      dest++;
    }
  WAIT_4_CAMERA ();
  RETURN (res);
}

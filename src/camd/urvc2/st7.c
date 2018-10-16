#define _ST7_C

#include <stdio.h>
#include <sys/io.h>

#include "urvc.h"

PAR_ERROR
DumpPixels2_st7 (int len, int bin)
{
  PAR_ERROR res = CE_NO_ERROR;
  short j;
  short bulk;
  short individual;

  {
    CameraOut (0x30, 0);
    CameraOut (0, 8);

    bulk = len / 10;
    individual = len % 10;

    for (j = bulk; j > 0; j--)
      {
	//CameraPulse (0);
	CameraOut (0x30, 8);
	CameraOut (0x30, 0);
	if ((res = CameraReady ()))
	  return res;
      }

    switch (bin)
      {
      case 1:
	CameraOut (0, 0);
	break;
      case 2:
	CameraOut (0, 4);
	break;
      case 3:
	CameraOut (0, 0xc);
	break;
      }

    for (j = individual; j > 0; j--)
      {
	//CameraPulse (0);
	CameraOut (0x30, 8);
	CameraOut (0x30, 0);
	if ((res = CameraReady ()))
	  return res;
      }
  }

  return res;
}

PAR_ERROR
DumpPixels_st7 (int len)
{
  return DumpPixels2_st7 (len, 1);
}


PAR_ERROR
DumpLines_st7 (int width, int len, int vertBin)
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, j;
  unsigned char ic = 4;

  CameraOut (0x30, 0);

  for (i = 0; i < len; i++)
    {

      CameraOut (0x10, (ic | 8) & 0xff);

      for (j = 0; j < vertBin; j++)
	{
	  CameraOut (0x10, (ic | 9) & 0xff);
	  // sleep ~12.5us? (why, Kodak documentation talks about 2us, min 1us!)
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
	if ((res = DumpPixels_st7 (width)))
	  return res;
    }
  return res;
}

PAR_ERROR
DigitizeLine_st7 (int left, int len, int right,
		  int bin, unsigned short *dest, int clearWidth)
{
// Meaning of the arguments (as it developed):
// left: x coord of left corner
// len: width of an image
// right: [changed] it's vertBefore: the amount of pixels before the
//         imaging area begins
// horzBin: horizontal binning, 1:1 = 1
// onVertBin: vertical binning: 1:1 = 1
// offVertBin: unused, set to 1
// dest: destination pointer buffer
// subtract: unused
// clearWidth: total width of the chip to be read out

  // short left, len, right, horzBin, onVertBin, offVertBin, clearWidth;
  // MY_LOGICAL subtract;

  // subtract byl vzdy 0; 
  // offVertBin vzdy 1;
  // onVertBin a horzBin si byly rovne a byly rovne zadanemu binu
  // left + right = clearWidth;
  // len = sirka pozadovaneho obrazu == W
  // left = 15 + levy sloupec == 15 + L * bin 

  PAR_ERROR res = CE_NO_ERROR;
  short i;			// , v;
  unsigned short u;
  // unsigned short * up;
  // long d;

  if (len > 0x5fa)
    len = 0x5fa;

  if ((res = DumpPixels_st7 (clearWidth)))
    return res;

  CameraOut (0x30, 0);
  CameraOut (0x10, 8);

  for (i = 0; i < bin; i++)
    {
      CameraOut (0x10, 9);
      IODelay (IODELAYCONST);	// = udelay(12);
      CameraOut (0x10, 0xa);
      IODelay (IODELAYCONST);
      CameraOut (0x10, 9);
      IODelay (IODELAYCONST);
      CameraOut (0x10, 8);
      IODelay (IODELAYCONST);
    }
  CameraOut (0x10, 0);

  if ((res = DumpPixels2_st7 (right, 1)))
    return res;
  if ((res = DumpPixels2_st7 (2, bin)))
    return res;
  if ((res = DumpPixels2_st7 (left, bin)))
    return res;

  switch (bin)
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

  // if(( res=CameraReady() ))return res;        // Jsem pridal, kdyz jsem zrusil W4C v SPP_GET_PIXEL
  for (i = 0; i < len; i++)
    {
      if ((res = SPPGetPixel (&u, MAIN_CCD)))
	{
	  /* ECPSetMode(ECP_NORMAL); */
	  return res;
	}
      // @ if(res = ECPGetPixel( & u)) 
      // @ { 
      // @ ECPSetMode(ECP_NORMAL); 
      // @ return res; 
      // @ }

      *dest = u;
      dest++;
    }
  if ((res = CameraReady ()))
    return res;
  // @ ECPSetMode(ECP_NORMAL);

  // Vyhodit pixely ZA oblasti (aby nezlobily)
  DumpPixels2_st7 (clearWidth - bin * (len + left) - right, 1);

  return res;
}

PAR_ERROR
DigitizeRAWImagingLine (int len, unsigned short *dest)
{
  PAR_ERROR res = CE_NO_ERROR;
  short i;
  unsigned short u;

  if ((res = DumpPixels_st7 (len)))
    return res;

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
  if ((res = CameraReady ()))
    return res;
  for (i = 0; i < len; i++)
    {
      if ((res = SPPGetPixel (&u, MAIN_CCD)))
	return res;
      *dest = u;
      dest++;
    }
  if ((res = CameraReady ()))
    return res;
  return res;
}

PAR_ERROR
ClearArray_st7 (int height, int times, __attribute__ ((unused)) int left)
	// short height, width, times, left;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i;

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

      //CameraPulse (0);
      CameraOut (0x30, 8);
      CameraOut (0x30, 0);

      if ((res = CameraReady ()))
	return res;
    }
//      err = MeasureImagingBias(width, left);
  SetVdd (0);
  return res;
}

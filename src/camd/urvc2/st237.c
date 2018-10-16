#define _ST237_C_
#include <sys/io.h>
#include <stdio.h>

#include "urvc.h"

#define CCD_TOTAL_WIDTH 684
#define CCD_TOTAL_HEIGHT 500

PAR_ERROR
DumpPixels_st237 (int len)
{
  PAR_ERROR res = CE_NO_ERROR;
  short j;

  CameraOut (0, 1);

  for (j = len / 10; j > 0; j--)
    {
      //CameraPulse (0);
      CameraOut (0x30, 8);
      CameraOut (0x30, 0);
      if ((res = CameraReady ()))
	return res;
    }

  CameraOut (0, 0);

  for (j = len % 10; j > 0; j--)
    {
      //CameraPulse (0);
      CameraOut (0x30, 8);
      CameraOut (0x30, 0);
      if ((res = CameraReady ()))
	return res;
    }
  return res;
}

PAR_ERROR
DumpLines_st237 (int horzTotal, int num, int bin)
{
  PAR_ERROR res = CE_NO_ERROR;
  short i;

  CameraOut (0, 0);

  for (i = num * bin; i > 0; i--)
    {
      CameraOut (0x10, 2);
      CameraOut (0x10, 6);
      CameraOut (0x10, 4);
      CameraOut (0x10, 0);
    }
  res = DumpPixels_st237 (horzTotal);
  return res;
}

PAR_ERROR
DigitizeLine_st237 (int left, int len, int horzBefore, int bin,
		    unsigned short *dest, int clearWidth)
	// short left, len, readoutMode; MY_LOGICAL subtract;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, j;
  unsigned short u;
  unsigned long u1;

  CameraOut (0, 0);

  // Shodit do vycitaciho registru <bin> radek z CCD
  CameraOut (0x10, 2);
  j = bin;
  while (j--)
    {
      CameraOut (0x10, 0x6);
      CameraOut (0x10, 0x4);
    }
  CameraOut (0x10, 0);

  // vysypani pixelu pred oblasti, ktera nas zajima  horzBefore == 28:kanal+1:kraj CCD
  // radka CCD je: 4-22-658-0  (void-black-light-black)
  if ((res = DumpPixels_st237 (left * bin + horzBefore + 1)))
    return res;

  // Pixely
  for (i = 0; i < len; i++)
    {
      for (u1 = 0, j = 0; j < bin; j++)
	{
	  if ((res = SPPGetPixel (&u, MAIN_CCD)))
	    return res;
	  u1 += u;
	}
      *(dest++) = u1 / bin / bin;
    }

  // Vysypani zbylych pixelu (nutne, jinak se sectou s pristi radkou)
  if ((res = DumpPixels_st237 (clearWidth - bin * (left + len) - horzBefore)))
    return res;

  return CameraReady ();
}

PAR_ERROR
ClearArray_st237 (int height, int times, __attribute__ ((unused)) int left)
{
  return DumpLines_st237 (CCD_TOTAL_WIDTH, height * times, 1);
}

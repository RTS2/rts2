#include <sys/io.h>
#include <stdio.h>

#include "urvc.h"

PAR_ERROR
DumpPixels_trk (int len)
	// short len;
{
  PAR_ERROR res = CE_NO_ERROR;
  int j;

  CameraOut (0x30, 4);
  CameraOut (0, 8);

  for (j = len / 10; j > 0; j--)
    {
      //CameraPulse (4);
      CameraOut (0x30, 12);
      CameraOut (0x30, 4);
      if ((res = CameraReady ()))
	return res;
    }

  CameraOut (0, 0);

  for (j = len % 10; j > 0; j--)
    {
      //CameraPulse (4);
      CameraOut (0x30, 12);
      CameraOut (0x30, 4);
      if ((res = CameraReady ()))
	return res;
    }
  return res;
}

PAR_ERROR
DumpLines_trk (int width, int len, int vertBin)
	// short width, len, vertBin;
{
  PAR_ERROR res = CE_NO_ERROR;
  int i;

  CameraOut (0x30, 4);
  CameraOut (0, 0xa);

  for (i = len * vertBin; i > 0; i--)
    {
      CameraOut (0, 0xf);
      CameraOut (0, 0xe);
      CameraOut (0, 0xa);
    }
  CameraOut (0, 0);
  res = DumpPixels_trk (width);
  return res;
}

PAR_ERROR
DigitizeLine_trk (int left, int len, int horzBefore, int bin,
		  unsigned short *dest, int clearWidth)
//PAR_ERROR
//DigitizeLine_trk (int left, int len, int readoutMode,
//                    unsigned short *dest, int subtract)
	// short left, len, readoutMode; MY_LOGICAL subtract;
{
  PAR_ERROR res = CE_NO_ERROR;
  int i, j;
  unsigned short u;
  unsigned long u1;

  CameraOut (0x30, 4);
  CameraOut (0, 0xa);
  CameraOut (0x30, 4);
  CameraOut (0, 2);
  CameraOut (0, 0xa);
  CameraOut (0, 0xb);

  for (j = 0; j < bin; j++)
    {
      CameraOut (0, 0xf);
      CameraOut (0, 0xe);
    }
  CameraOut (0, 0);

  // vysypani pixelu pred oblasti, ktera nas zajima (+1 readout register :)
  if ((res = DumpPixels_trk (left * bin + horzBefore + 1)))
    return res;

  // Vytazeni toho, co nas zajima
  // dest += len - 1;
  CameraOut (0, 0);
  CameraOut (0x30, 4);
  for (i = 0; i < len; i++)
    {
      for (u1 = 0, j = 0; j < bin; j++)
	{
	  if ((res = SPPGetPixel (&u, TRACKING_CCD)))
	    return res;
	  u1 += u;
	}
      // *(dest--) = u / bin;
      *(dest++) = u1 / bin / bin;
    }

  // Vysypani zbylych pixelu (nutne, jinak se sectou s pristi radkou)
  if ((res = DumpPixels_trk (clearWidth - bin * (left + len))))
    return res;

  if ((res = CameraReady ()))
    return res;
  return res;
}

PAR_ERROR
ClearArray_trk (int height, int times, __attribute__ ((unused)) int left)
	// short height, times, left;
{
  PAR_ERROR res = CE_NO_ERROR;
  int i;

  CameraOut (0x30, 4);
  times *= height;
  for (i = 0; i < times; i++)
    {
      CameraOut (0, 0xb);
      CameraOut (0, 0xf);
      CameraOut (0, 0xe);
      CameraOut (0, 8);

      //CameraPulse (4);
      CameraOut (0x30, 12);
      CameraOut (0x30, 4);
      if ((res = CameraReady ()))
	return res;
    }
  return res;
}

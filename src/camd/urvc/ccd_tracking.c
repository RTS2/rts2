#include <sys/io.h>
#include <stdio.h>

#include "mardrv.h"
#include "ccd_macros.h"

extern unsigned long lost_ticks;

PAR_ERROR
DigitizeTrackingLine (int left, int len, int readoutMode,
		      unsigned short *dest, int subtract)
	// short left, len, readoutMode; MY_LOGICAL subtract;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i, j;
  short t0;
  unsigned short u;
  unsigned long u1;
  int bin;
  // long d;

  bin = readoutMode + 1;	// Mel by ho nahradit... (nikde jinde se
  // readoutMode nepouziva)

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

  // vysypani pixelu pred oblasti, ktera nas zajima
  if ((res = BlockClearTrackingPixels (left * bin + 7)))
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
      *(dest++) = u1 / bin;
    }

  // Vysypani zbylych pixelu (nutne, jinak se sectou s pristi radkou)
  if ((res = BlockClearTrackingPixels (192 + 1 + 10 - bin * (left + len))))
    return res;

  WAIT_4_CAMERA ();
  return res;
}

PAR_ERROR
DumpTrackingLines (int width, int len, int vertBin)
	// short width, len, vertBin;
{
  PAR_ERROR res = CE_NO_ERROR;
  short i;

  CameraOut (0x30, 4);
  CameraOut (0, 0xa);

  for (i = len * vertBin; i > 0; i--)
    {
      CameraOut (0, 0xf);
      CameraOut (0, 0xe);
      CameraOut (0, 0xa);
    }
  CameraOut (0, 0);
  res = BlockClearTrackingPixels (width);
  return res;
}

PAR_ERROR
BlockClearTrackingPixels (int len)
	// short len;
{
  PAR_ERROR res = CE_NO_ERROR;
  short j;
  short t0;

  CameraOut (0x30, 4);
  CameraOut (0, 8);

  for (j = len / 10; j > 0; j--)
    {
      CAMERA_PULSE (4);
      WAIT_4_CAMERA ();
    }

  CameraOut (0, 0);

  for (j = len % 10; j > 0; j--)
    {
      CAMERA_PULSE (4);
      WAIT_4_CAMERA ();
    }
  return res;
}

PAR_ERROR
ClearTrackingPixels (int len)
	// short len;
{
  PAR_ERROR res = CE_NO_ERROR;
  short j, t0;

  CameraOut (0, 0);
  for (j = len; j > 0; j--)
    {
      CAMERA_PULSE (4);
      WAIT_4_CAMERA ();
    }
  return res;
}

PAR_ERROR
ClearTrackingArray (int height, int times, int left)
	// short height, times, left;
{
  PAR_ERROR err = CE_NO_ERROR;
  short i, t0;

  CameraOut (0x30, 4);
  times *= height;
  for (i = 0; i < times; i++)
    {
      CameraOut (0, 0xb);
      CameraOut (0, 0xf);
      CameraOut (0, 0xe);
      CameraOut (0, 8);

      CAMERA_PULSE (4);
      WAIT_4_CAMERA ();
    }
  err = MeasureTrackingBias (left);
  return err;
}

PAR_ERROR
OffsetTrackingArray (int height, int width, unsigned short *offset, int left)
	// short height, width, left;
{
  PAR_ERROR res;
  short i, t0;
  unsigned short u;
  unsigned long ul;

  if ((res = ClearTrackingArray (height, 1, left)))
    return res;
  if ((res = BlockClearTrackingPixels (width)))
    return res;
  if ((res = ClearTrackingPixels (2)))
    return res;
  CameraOut (0, 0);
  CameraOut (0x30, 4);
  ul = 0;
  for (i = 0; i <= 0x7f; i++)
    {
      if ((res = SPPGetPixel (&u, TRACKING_CCD)))
	return res;
      ul = ul + u;
    }
  *offset = (ul + 0x40) >> 7;
  WAIT_4_CAMERA ();
  return res;
}

PAR_ERROR
MeasureTrackingBias (int left)
	// short left;
{
  return CE_NO_ERROR;
}

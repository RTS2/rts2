#ifndef __RTS_CAMERA__
#define __RTS_CAMERA__

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "camera_info.h"
#include "ccd_msg.h"

struct camera_info miniccd;
int fd_ccd = 0;
int *fd_chips = NULL;
int sizeof_pixel = 0;
int *field_rows = NULL;

int ccd_dac_bits;
int interleave = 0;

int exp_loadtime = 3;

char _data[1200][2400];

extern int camera_expose (int chip, float *exposure, int light);

extern int
camera_init (char *device_name, int camera_id)
{
  int msg_len, i;
  int start, stop;
  struct timeval now;
  CCD_ELEM_TYPE msgw[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE],
    msgr[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];

  if (fd_ccd > 0)
    return 0;

  if ((fd_ccd = open (device_name, O_RDWR, 0)) < 0)
    {
      fprintf (stderr, "Unable to open device: %s\n", device_name);
      return (-1);
    }
  /*
   * Request CCD parameters.
   */
  msgw[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msgw[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_QUERY_LEN;
  msgw[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msgw[CCD_MSG_INDEX] = CCD_MSG_QUERY;
  write (fd_ccd, (char *) msgw, CCD_MSG_QUERY_LEN);
  if ((msg_len =
       read (fd_ccd, (char *) msgr, CCD_MSG_CCD_LEN)) != CCD_MSG_CCD_LEN)
    {
      fprintf (stderr, "CCD message length wrong: %d\n", msg_len);
      return (-1);
    }
  /*
   * Response from CCD query.
   */
  if (msgr[CCD_MSG_INDEX] != CCD_MSG_CCD)
    {
      fprintf (stderr, "Wrong message returned from query: 0x%04X",
	       msgr[CCD_MSG_INDEX]);
      return (-1);
    }
  miniccd.chips = msgr[CCD_CCD_FIELDS_INDEX];
  if (miniccd.chips > 1)
    {
      // 0 chip is compostition of two interleaved exposures
      miniccd.chips++;
      interleave = 1;
    }
  else
    {
      interleave = 0;
    }

  field_rows = (int *) malloc (miniccd.chips);
  miniccd.chip_info =
    (struct chip_info *) malloc (sizeof (struct chip_info) * miniccd.chips);
  fd_chips = (int *) malloc (sizeof (int) * miniccd.chips);
  strncpy (miniccd.type, (char *) &msgr[CCD_CCD_NAME_INDEX],
	   CCD_CCD_NAME_LEN / 2);
  for (i = 0; i < CCD_CCD_NAME_LEN / 2; i++)
    if (miniccd.type[i] == ' ')
      miniccd.type[i] = '_';

  miniccd.type[CCD_CCD_NAME_LEN] = '\x0';
  for (i = 0; i < miniccd.chips; i++)
    {
      struct chip_info *minichip = &miniccd.chip_info[i];
      char *chip_device_name;
      field_rows[i] = 0;
      chip_device_name = (char *) malloc (strlen (device_name) + 2);
      strcpy (chip_device_name, device_name);
      if (i == 0)
	{
	  fd_chips[i] = fd_ccd;
	}
      else
	{
	  chip_device_name[strlen (device_name)] = i + '0';
	  chip_device_name[strlen (device_name) + 1] = '\x0';
	  fd_chips[i] = open (chip_device_name, O_RDWR, 0);
	  if (fd_chips[i] < 0)
	    {
	      fprintf (stderr, "Cannot open %s\n", chip_device_name);
	      free (chip_device_name);
	      miniccd.chips = -1;
	      free (miniccd.chip_info);
	      fd_chips[i] = -1;
	      return -1;
	    }
	}
      write (fd_chips[i], (char *) msgw, CCD_MSG_QUERY_LEN);
      if ((msg_len =
	   read (fd_chips[i], (char *) msgr,
		 CCD_MSG_CCD_LEN)) != CCD_MSG_CCD_LEN)
	{
	  fprintf (stderr, "CCD message length wrong: %d\n", msg_len);
	}
      minichip->width = msgr[CCD_CCD_WIDTH_INDEX];
      minichip->height = msgr[CCD_CCD_HEIGHT_INDEX];
      if (i == 0 && interleave)
	{
	  minichip->width /= 2;
	}
      minichip->binning_vertical = 1;
      minichip->binning_horizontal = 1;
      sizeof_pixel = (msgr[CCD_CCD_DEPTH_INDEX] + 7) / 8;
    }

  miniccd.temperature_regulation = -1;
  miniccd.temperature_setpoint = 0;
  miniccd.air_temperature = 0;
  miniccd.ccd_temperature = 0;
  miniccd.cooling_power = 0;
  ccd_dac_bits = msgr[CCD_CCD_DAC_INDEX];
  miniccd.can_df = 0;		// starlight cameras cannot do DF
  return (0);
}

extern void
camera_done ()
{
  return;
}

extern int
camera_info (struct camera_info *info)
{
  *info = miniccd;
  return 0;
}

extern int
camera_fan (int fan_state)
{
  miniccd.fan = fan_state;
  return 0;
}

extern int
dummy_camera_readout_line (int chip_id, short start, short length,
			   void *data);

extern int
camera_expose (int chip, float *exposure, int light)
{
  CCD_ELEM_TYPE msg[CCD_MSG_EXP_LEN / CCD_ELEM_SIZE];
  struct chip_info *minichip = &miniccd.chip_info[chip];
  int exposure_msec = *exposure * 1000;
  fd_set set;
  /*
   * Send the capture request.
   */
  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_EXP_LEN;
  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msg[CCD_MSG_INDEX] = CCD_MSG_EXP;
  msg[CCD_EXP_WIDTH_INDEX] = minichip->width * (interleave
						&& chip == 0 ? 2 : 1);
  msg[CCD_EXP_HEIGHT_INDEX] = minichip->height;
  msg[CCD_EXP_XOFFSET_INDEX] = 0;
  msg[CCD_EXP_YOFFSET_INDEX] = 0;
  msg[CCD_EXP_XBIN_INDEX] = minichip->binning_horizontal;
  msg[CCD_EXP_YBIN_INDEX] = minichip->binning_vertical;
  msg[CCD_EXP_DAC_INDEX] = ccd_dac_bits;
  msg[CCD_EXP_FLAGS_INDEX] = light ? 0 : CCD_EXP_FLAGS_NOOPEN_SHUTTER;
  msg[CCD_EXP_MSEC_LO_INDEX] = exposure_msec & 0xFFFF;
  msg[CCD_EXP_MSEC_HI_INDEX] = exposure_msec >> 16;
  if (chip == 0 && interleave)
    {
      // interleave..the other way. Don't do full exposure, do just a half, and bin it
      write (fd_chips[chip], (char *) msg, CCD_MSG_EXP_LEN);
      field_rows[chip] = 0;
      FD_ZERO (&set);
      FD_SET (fd_chips[chip], &set);
      select (fd_chips[chip] + 1, &set, NULL, NULL, NULL);
      while (dummy_camera_readout_line (chip, 0, minichip->width,
					&_data[field_rows[chip]]) == 0);
      field_rows[chip] = 0;

    }
  else
    {
      write (fd_chips[chip], (char *) msg, CCD_MSG_EXP_LEN);
      field_rows[chip] = 0;
      FD_ZERO (&set);
      FD_SET (fd_chips[chip], &set);
      select (fd_chips[chip] + 1, &set, NULL, NULL, NULL);
      while (dummy_camera_readout_line (chip, 0, minichip->width,
					&_data[field_rows[chip]]) == 0);
      field_rows[chip] = 0;
    }
  return 0;
}

extern int
camera_end_expose (int chip)
{
  CCD_ELEM_TYPE msg[CCD_MSG_ABORT_LEN / CCD_ELEM_SIZE];

  /*
   * Send the abort request.
   */
  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_ABORT_LEN;
  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msg[CCD_MSG_INDEX] = CCD_MSG_ABORT;
  write (fd_chips[chip], (char *) msg, CCD_MSG_ABORT_LEN);
  /*
   * Reset load image state.
   */
  field_rows[chip] = 0;
  return 0;
}

extern int
camera_binning (int chip_id, int vertical, int horizontal)
{
  miniccd.chip_info[chip_id].binning_vertical = vertical;
  miniccd.chip_info[chip_id].binning_horizontal = horizontal;
  return 0;
}

extern int
dummy_camera_readout_line (int chip_id, short start, short length, void *data)
{
  CCD_ELEM_TYPE *msg;
  struct chip_info *minichip = &miniccd.chip_info[chip_id];
  int row_bytes;
  int msg_len;
  char *buf;
  int ret;
  int real_bytes;

  real_bytes =
    (minichip->width / minichip->binning_horizontal) * sizeof_pixel;

  if (chip_id == 0 && interleave)
    {
      row_bytes = 2 * real_bytes;
    }
  else
    {
      row_bytes = real_bytes;
    }

  buf = (char *) malloc (row_bytes + CCD_MSG_IMAGE_LEN);
  msg = (CCD_ELEM_TYPE *) buf;

  if (field_rows[chip_id] == 0)
    {
      /*
       * Get header plus first scanline.
       */
      msg_len = 0;
      while (msg_len == 0)
	{
	  msg_len =
	    read (fd_chips[chip_id], buf, row_bytes + CCD_MSG_IMAGE_LEN);
	  fprintf (stderr, "msg_len : %i\n", msg_len);
	  sleep (1);
	}
      if ((msg_len > 0) && (msg[CCD_MSG_INDEX] == CCD_MSG_IMAGE))
	{
	  /* 
	   * Validate message length.
	   */
	  if ((msg[CCD_MSG_LENGTH_LO_INDEX] +
	       (msg[CCD_MSG_LENGTH_HI_INDEX] << 16)) != row_bytes
	      * (minichip->height / minichip->binning_horizontal) +
	      CCD_MSG_IMAGE_LEN)
	    {
	      fprintf (stderr, "Image size discrepency %i != %i!\n",
		       (msg[CCD_MSG_LENGTH_LO_INDEX] +
			(msg[CCD_MSG_LENGTH_HI_INDEX] << 16)),
		       row_bytes * (minichip->height /
				    minichip->binning_horizontal) +
		       CCD_MSG_IMAGE_LEN);
	      return -1;
	    }
	  /*
	   * Read rest of first scanline if it didn't make it (should never happen).
	   */
	  if (msg_len != row_bytes + CCD_MSG_IMAGE_LEN)
	    msg_len =
	      read (fd_chips[chip_id], &buf[msg_len - CCD_MSG_IMAGE_LEN],
		    row_bytes - msg_len - CCD_MSG_IMAGE_LEN);
	}
      else
	{
	  fprintf (stderr, "invalid length: %i\n", msg_len);
	  goto err;
	}
    }
  else
    {
      read (fd_chips[chip_id], buf + CCD_MSG_IMAGE_LEN, row_bytes);
    }

  field_rows[chip_id]++;
  if (field_rows[chip_id] >= minichip->height / minichip->binning_horizontal)
    {
      field_rows[chip_id] = 0;
      ret = 1;
    }
  else
    {
      ret = 0;
    }
  /*
   * Move image data down to replace header.
   */
  if (chip_id == 0 && interleave)
    {
      if (sizeof_pixel == 2)
	{
	  short *d = (short *) data;
	  char *b = buf + CCD_MSG_IMAGE_LEN + start;
	  int i;
	  for (i = 0; i < length; i++)
	    {
	      *d =
		*((short *) (&b[i * 4])) / 2 +
		*((short *) (&b[i * 4 + 2])) / 2;
	      d++;
	    }
	}
      else
	{
	  char *d = (char *) data;
	  char *b = buf + CCD_MSG_IMAGE_LEN + start;
	  int i;
	  for (i = 0; i < length; i++)
	    {
	      *d =
		*((char *) (&b[i * 2])) / 2 + *((char *) (&b[i * 2 + 1])) / 2;
	      d++;
	    }
	}
    }
  else
    {
      memcpy (data, buf + CCD_MSG_IMAGE_LEN + start, length * sizeof_pixel);
    }
  free (buf);

  return ret;
err:
  free (buf);
  return -1;
}

extern int
camera_readout_line (int chip_id, short start, short length, void *data)
{
  memcpy (data, &_data[field_rows[chip_id]] + start, length * sizeof_pixel);
  field_rows[chip_id]++;
  return 0;
}

extern int
camera_dump_line (int chip_id)
{
  struct chip_info *minichip = &miniccd.chip_info[chip_id];
  int row_bytes =
    (minichip->width * (interleave && chip_id == 0 ? 2 : 1) /
     minichip->binning_horizontal) * sizeof_pixel;
  char *buf = (char *) malloc (row_bytes + CCD_MSG_IMAGE_LEN);
  if (field_rows[chip_id] == 0)
    read (fd_chips[chip_id], buf, row_bytes + CCD_MSG_IMAGE_LEN);
  else
    read (fd_chips[chip_id], buf, row_bytes);
  field_rows[chip_id]++;
  free (buf);
  return 0;
}

extern int
camera_end_readout (int chip_id)
{
  return -1;
}

extern int
camera_cool_max ()
{
  CCD_ELEM_TYPE msg[CCD_MSG_TEMP_LEN / CCD_ELEM_SIZE];

  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_TEMP_LEN;
  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msg[CCD_MSG_INDEX] = CCD_MSG_TEMP;
  msg[CCD_TEMP_SET_HI_INDEX] = 0;
  msg[CCD_TEMP_SET_LO_INDEX] = 0;
  write (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
  read (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
  return 0;
}

extern int
camera_cool_hold ()
{
  return 0;
}

extern int
camera_cool_shutdown ()
{
  return 0;
}

extern int
camera_cool_setpoint (float coolpoint)
{
  return 0;
}

extern int
camera_set_filter (int filter)
{
  return 0;
}

#endif /* __RTS_CAMERA__ */

/*! 
 * @file Prototype implementation of sbig driver.
 * $Id$
 *
 * That one is an quick-hack to test camera function. We
 * will have to write kernel driver to make it working the way
 * we would like to work.
 *
 * @author petr
 */
#define __GNU_LIBRARY__

#include "../camera.h"
#include "status.h"
#include "sbig.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

struct sbig_init init;
int sbig_id = -1;
int *sbig_modes = NULL;
int sbig_reg;

struct chip_info *ch_info = NULL;

extern int
camera_init (char *device_name, int camera_id)
{
  struct sbig_status status;
  int i;

  if (sbig_id != -1)
    {
      errno = ENODEV;
      return -1;
    }
  sbig_id = camera_id;
  if (sbig_init (camera_id, 5, &init) || sbig_get_status (&status))
    {
      errno = ENODEV;
      return -1;
    }
  ch_info =
    (struct chip_info *) malloc (init.nmbr_chips * sizeof (struct chip_info));
  sbig_modes = (int *) malloc (sizeof (int) * init.nmbr_chips);
  for (i = 0; i < init.nmbr_chips; i++)
    sbig_modes[i] = 0;
  // determine temperature regulation state
  switch (status.temperature_regulation)
    {
    case 0:
      sbig_reg =
	(status.cooling_power > 250) ? CAMERA_COOL_MAX : CAMERA_COOL_OFF;
      break;
    case 1:
      sbig_reg = CAMERA_COOL_HOLD;
      break;
      break;
    default:
      sbig_reg = CAMERA_COOL_OFF;
#ifdef DEBUG
      printf ("unknow sbig cool status: %i", status.temperature_regulation);
#endif /* DEBUG */

    }
  return 0;
}

extern void
camera_done ()
{
  sbig_id = -1;
  free (ch_info);
  ch_info = NULL;
  free (sbig_modes);
  sbig_modes = NULL;
}

extern int
camera_info (struct camera_info *info)
{
  int i;
  struct sbig_status status;

  snprintf (info->type, 64, "%s fv %i", init.camera_name,
	    init.firmware_version);
  strncpy (info->serial_number, init.serial_number, 64);
  info->chips = init.nmbr_chips;

  info->chip_info = ch_info;
  for (i = 0; i < init.nmbr_chips; i++)
    {

      readout_mode_t *rdm =
	&init.sbig_camera_info[i].readout_mode[sbig_modes[i]];
      struct chip_info *cpi = &ch_info[i];
      // it's SBIG convention to have 1x1 mode as mode #0
      cpi->width = init.sbig_camera_info[i].readout_mode[0].width;
      cpi->height = init.sbig_camera_info[i].readout_mode[0].height;
      cpi->binning_vertical = cpi->width / rdm->width;
      cpi->binning_horizontal = cpi->height / rdm->height;
      cpi->image_type = CAMERA_USHORT_IMG;
      cpi->gain = rdm->gain;
    }

  if (sbig_get_status (&status))
    {
      errno = ENODEV;
      return -1;
    }
  info->temperature_regulation = sbig_reg;
  info->temperature_setpoint = status.temperature_setpoint / 10.0;
  info->air_temperature = status.air_temperature / 10.0;
  info->ccd_temperature = status.ccd_temperature / 10.0;
  info->cooling_power =
    (status.cooling_power == 255) ? 1000 : status.cooling_power * 3.906;
  info->fan = status.fan_on;

  return 0;
}

extern int
camera_fan (int fan_state)
{
  struct sbig_control sbc;

  sbc.fan_on = fan_state ? 1 : 0;
  sbc.shutter = 0;
  sbc.led = 1;
  if (sbig_control (&sbc))
    {
      errno = ENODEV;
      return -1;
    }
  return 0;
}

extern int
camera_expose (int chip, float *exposure, int light)
{
  struct sbig_expose expose;

  expose.ccd = chip;
  expose.exposure_time = (int) (100.0 * *exposure);
  expose.abg_state = 2;		// sbig default???
  if (chip)
    {
      expose.shutter = (light) ? 1 : 0;	// no change for dark..
    }
  else
    {
      expose.shutter = (light) ? 1 : 2;
    }

  if (sbig_expose (&expose))
    {
      errno = ENODEV;
      return -1;
    }
  return 0;
}

extern int
camera_end_expose (int chip)
{
  if (sbig_end_expose (chip))
    {
      errno = ENODEV;
      return -1;
    }
  return 0;
}

extern int
camera_binning (int chip_id, int vertical, int horizontal)
{
  int i;
  for (i = 0; i < init.sbig_camera_info[chip_id].nmbr_readout_modes; i++)
    {
      int v =
	init.sbig_camera_info[chip_id].readout_mode[0].width /
	init.sbig_camera_info[chip_id].readout_mode[i].width;
      int h =
	init.sbig_camera_info[chip_id].readout_mode[0].height /
	init.sbig_camera_info[chip_id].readout_mode[i].height;

      if (v == vertical && h == horizontal)
	{
	  sbig_modes[chip_id] = i;
	  return 0;
	}

    }
  errno = EINVAL;
  return -1;

}

extern int
camera_readout_line (int chip_id, short start, short length, void *data)
{
  struct sbig_readout_line rdl;

  rdl.ccd = chip_id;
  rdl.readoutMode = sbig_modes[chip_id];
  rdl.pixelStart = start;
  rdl.pixelLength = length;
  rdl.data = data;
  if (sbig_readout_line (&rdl))
    {
      errno = ENODEV;
      return -1;
    }
  return 0;
}

extern int
camera_dump_line (int chip_id)
{
  struct sbig_dump_lines dl;

  dl.ccd = chip_id;
  dl.readoutMode = sbig_modes[chip_id];
  dl.lineLength = 1;
  if (sbig_dump_lines (&dl))
    {
      errno = ENODEV;
      return -1;
    }
  return 0;
}

extern int
camera_end_readout (int chip_id)
{
  if (sbig_end_readout (chip_id))
    {
      errno = ENODEV;
      return -1;
    }
  return 0;
}

extern int
camera_cool_max ()
{
  struct sbig_cool cl;

  cl.regulation = 2;
  cl.direct_drive = 255;

  if (sbig_set_cooling (&cl))
    {
      errno = ENODEV;
      return -1;
    }
  sbig_reg = CAMERA_COOL_MAX;
  return 0;
}


extern int
camera_cool_hold ()
{
  struct sbig_status st;
  float ot;			// optimal temperature

  // go the ccd temp..
  if (sbig_get_status (&st))
    {
      errno = ENODEV;
      return -1;
    }

  ot = ((int) (st.ccd_temperature + 50) / 50) * 50 / 10;
  return camera_cool_setpoint (ot);
}

extern int
camera_cool_shutdown ()
{
  struct sbig_cool cl;
  cl.regulation = 0;
  cl.direct_drive = 0;

  if (sbig_set_cooling (&cl))
    {
      errno = ENODEV;
      return -1;
    }
  sbig_reg = CAM_COOL_OFF;
  return 0;
}

extern int
camera_cool_setpoint (float coolpoint)
{
  struct sbig_cool cl;

  cl.regulation = 1;
  cl.temperature = coolpoint * 10.0;
  if (sbig_set_cooling (&cl))
    {
      errno = ENODEV;
      return -1;
    }
  sbig_reg = CAM_COOL_TEMP;
  return 0;
}

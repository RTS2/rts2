#include "devhnd.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

struct supp_info devhnd_devices[] = {
  {NULL, NULL},			// DEVICE_TYPE_UNKNOW
  {(devcli_handle_response_t) telescope_command_handler, (devcli_handle_response_t) telescope_message_handler},	// DEVICE_TYPE_MOUNT
  {(devcli_handle_response_t) camera_command_handler, (devcli_handle_response_t) camera_message_handler},	// DEVICE_TYPE_CCD
  {NULL, NULL}			// DEVICE_TYPE_DOME
  // etc..
};

int
telescope_command_handler (struct param_status *params,
			   struct telescope_info *info)
{
#ifdef DEBUG
  printf ("telescope get command: %s\n", params->param_argv);
#endif
  if (!strcmp (params->param_argv, "name"))
    return param_next_string_copy (params, info->name, 64);
  if (!strcmp (params->param_argv, "serial"))
    return param_next_string_copy (params, info->serial_number, 64);
  if (!strcmp (params->param_argv, "ra"))
    return param_next_double (params, &info->ra);
  if (!strcmp (params->param_argv, "dec"))
    return param_next_double (params, &info->dec);
  if (!strcmp (params->param_argv, "moving"))
    return param_next_integer (params, &info->moving);
  if (!strcmp (params->param_argv, "park_dec"))
    return param_next_double (params, &info->park_dec);
  if (!strcmp (params->param_argv, "longtitude"))
    return param_next_double (params, &info->longtitude);
  if (!strcmp (params->param_argv, "latitude"))
    return param_next_double (params, &info->latitude);
  if (!strcmp (params->param_argv, "siderealtime"))
    return param_next_double (params, &info->siderealtime);
  if (!strcmp (params->param_argv, "localtime"))
    return param_next_double (params, &info->localtime);
  if (!strcmp (params->param_argv, "correction_mark"))
    return param_next_integer (params, &info->correction_mark);
  errno = EINVAL;
  return -1;
};

int
telescope_message_handler (struct param_status *params,
			   struct telescope_info *info)
{
  printf ("telescope get message: %s\n", params->param_argv);
  return 0;
};

int
camera_command_handler (struct param_status *params, struct camera_info *info)
{
#ifdef DEBUG
  printf ("camera get command: %s %p '%f'\n", params->param_argv, info,
	  info->ccd_temperature);
#endif
  if (!strcmp (params->param_argv, "name"))
    return param_next_string_copy (params, info->name, 64);
  if (!strcmp (params->param_argv, "serial"))
    return param_next_string_copy (params, info->serial_number, 64);
  if (!strcmp (params->param_argv, "chips"))
    {
      if (param_next_integer (params, &info->chips))
	return -1;
      free (info->chip_info);
      info->chip_info =
	(struct chip_info *) malloc (info->chips * sizeof (struct chip_info));
      return 0;
    }
  if (!strcmp (params->param_argv, "temperature_regulation"))
    return param_next_integer (params, &info->temperature_regulation);
  if (!strcmp (params->param_argv, "temperature_setpoint"))
    return param_next_float (params, &info->temperature_setpoint);
  if (!strcmp (params->param_argv, "air_temperature"))
    return param_next_float (params, &info->air_temperature);
  if (!strcmp (params->param_argv, "ccd_temperature"))
    return param_next_float (params, &info->ccd_temperature);
  if (!strcmp (params->param_argv, "cooling_power"))
    return param_next_integer (params, &info->cooling_power);
  if (!strcmp (params->param_argv, "fan"))
    return param_next_integer (params, &info->fan);
  if (!strcmp (params->param_argv, "chip"))
    {
      int chip_n;
      struct chip_info *cinfo;
      char *cname;
      if (param_next_integer (params, &chip_n)
	  || param_next_string (params, &cname))
	return -1;
      cinfo = &info->chip_info[chip_n];
      if (!strcmp (cname, "width"))
	return param_next_integer (params, &cinfo->width);
      if (!strcmp (cname, "height"))
	return param_next_integer (params, &cinfo->height);
      if (!strcmp (cname, "binning_vertical"))
	return param_next_integer (params, &cinfo->binning_vertical);
      if (!strcmp (cname, "binning_horizontal"))
	return param_next_integer (params, &cinfo->binning_horizontal);
      if (!strcmp (cname, "pixelX"))
	return param_next_integer (params, &cinfo->pixelX);
      if (!strcmp (cname, "pixelY"))
	return param_next_integer (params, &cinfo->pixelY);
      if (!strcmp (cname, "gain"))
	return param_next_float (params, &cinfo->gain);
      errno = EINVAL;
      return 0;
    }
  errno = EINVAL;
  printf ("error by get\n");
  return -1;
};

int
camera_message_handler (struct param_status *params, struct camera_info *info)
{
  printf ("camera get message: %s\n", params->param_argv);
  return 0;
};

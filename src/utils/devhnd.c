#include "camera_info.h"
#include "telescope_info.h"
#include "dome_info.h"
#include "phot_info.h"


#include "devhnd.h"
#include <errno.h>
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

/*!
 * Find device with given name.
 *
 * @param device_name	name of the device we are looking for
 *
 * @return	device structure, NULL if not found
 */
struct device *
get_device (struct device **devices, const char *device_name)
{
  struct device *dev = *devices;

  while (dev)
    {
      if (!strcmp (dev->name, device_name))
	return dev;
      dev = dev->next;
    }

  dev = (struct device *) malloc (sizeof (struct device));
  // new device initialization
  dev->data_handler = NULL;
  dev->status_notifier = NULL;
  dev->notifier_data = NULL;
  dev->next = *devices;
  dev->name[0] = 0;
  dev->statutes = NULL;
  dev->priority = 0;
  dev->response_handler = NULL;
  pthread_mutex_init (&dev->status_lock, NULL);
  pthread_cond_init (&dev->status_cond, NULL);
  dev->channel.socket = -1;
  *devices = dev;
  return dev;
}

struct client *
get_client (struct client **clients, int id)
{
  struct client *cli = *clients;

  while (cli)
    {
      if (cli->id == id)
	return cli;
      cli = cli->next;
    }

  cli = (struct client *) malloc (sizeof (struct client));
  cli->id = id;
  cli->login[0] = 0;
  cli->priority = 0;
  cli->status_txt[0] = 0;
  cli->next = *clients;
  *clients = cli;
  return cli;
}

int
serverd_command_handler (struct param_status *params,
			 struct serverd_info *info)
{
  char *str;
  struct device *dev;
#ifdef DEBUG
  printf ("server get response: %s\n", params->param_argv);
#endif
  if (strcmp (params->param_argv, "logged_as") == 0)
    {
      if (param_get_length (params) != 1
	  || param_next_integer (params, &info->id))
	{
#ifdef DEBUG
	  printf ("invalid paramaters");
#endif /* DEBUG */
	  return -1;
	}
      return 0;
    }
  if (strcmp (params->param_argv, "authorization_key") == 0)
    {
      char *dev_name;
      int key;
      if (param_get_length (params) != 2)
	return -1;
      if (param_next_string (params, &dev_name)
	  || param_next_integer (params, &key))
	{
#ifdef DEBUG
	  printf ("invalid param value\n");
#endif /* DEBUG */
	  return -1;
	}
      dev = get_device (&info->devices, dev_name);
      if (!dev)
	{
	  errno = ENODEV;
	  return -1;
	}

      dev->key = key;

#ifdef DEBUG
      printf ("auth device: %s key: %i\n", dev->name, key);
#endif
      return 0;
    }
  if (!strcmp (params->param_argv, "device"))
    {
      int i;
      if (param_get_length (params) != 4 || param_next_integer (params, &i)
	  || param_next_string (params, &str))
	return -1;
      dev = get_device (&info->devices, str);

      strncpy (dev->name, str, DEVICE_NAME_SIZE);
      if (param_next_ip_address (params, &str, &dev->port)
	  || param_next_integer (params, &dev->type))
	return -1;
      strncpy (dev->hostname, str, DEVICE_URI_SIZE);
      return 0;
    }
  if (!strcmp (params->param_argv, "user"))
    {
      int id;
      struct client *cli;

      if (param_next_integer (params, &id))
	return -1;

      cli = get_client (&info->clients, id);
      if (param_next_integer (params, &cli->priority) ||
	  param_next_char (params, &cli->have_priority)
	  || param_next_string_copy (params, cli->login, CLIENT_LOGIN_SIZE))
	return -1;

      param_next_string_copy (params, cli->status_txt, MAX_STATUS_TXT);

      return 0;
    }
  errno = EINVAL;
  return -1;
}

int
telescope_command_handler (struct param_status *params,
			   struct telescope_info *info)
{
#ifdef DEBUG
  printf ("telescope get response: %s\n", params->param_argv);
#endif
  if (!strcmp (params->param_argv, "type"))
    return param_next_string_copy (params, info->type, 64);
  if (!strcmp (params->param_argv, "serial"))
    return param_next_string_copy (params, info->serial_number, 64);
  if (!strcmp (params->param_argv, "ra"))
    return param_next_double (params, &info->ra);
  if (!strcmp (params->param_argv, "dec"))
    return param_next_double (params, &info->dec);
  if (!strcmp (params->param_argv, "park_dec"))
    return param_next_double (params, &info->park_dec);
  if (!strcmp (params->param_argv, "longtitude"))
    return param_next_double (params, &info->longtitude);
  if (!strcmp (params->param_argv, "latitude"))
    return param_next_double (params, &info->latitude);
  if (!strcmp (params->param_argv, "altitude"))
    return param_next_float (params, &info->altitude);
  if (!strcmp (params->param_argv, "siderealtime"))
    return param_next_double (params, &info->siderealtime);
  if (!strcmp (params->param_argv, "localtime"))
    return param_next_double (params, &info->localtime);
  if (!strcmp (params->param_argv, "correction_mark"))
    return param_next_integer (params, &info->correction_mark);
  if (!strcmp (params->param_argv, "flip"))
    return param_next_integer (params, &info->flip);
  if (!strcmp (params->param_argv, "axis0_counts"))
    return param_next_double (params, &info->axis0_counts);
  if (!strcmp (params->param_argv, "axis1_counts"))
    return param_next_double (params, &info->axis1_counts);
  errno = EINVAL;
  return -1;
};

int
camera_command_handler (struct param_status *params, struct camera_info *info)
{
#ifdef DEBUG
  printf ("camera get response: %s %p '%f'\n",
	  params->param_argv, info, info->ccd_temperature);
#endif
  if (!strcmp (params->param_argv, "type"))
    return param_next_string_copy (params, info->type, 64);
  if (!strcmp (params->param_argv, "serial"))
    return param_next_string_copy (params, info->serial_number, 64);
  if (!strcmp (params->param_argv, "chips"))
    {
      int new_chips;
      if (param_next_integer (params, &new_chips))
	return -1;
      if (info->chips != new_chips)
	{
	  free (info->chip_info);
	  info->chip_info =
	    (struct chip_info *) malloc (new_chips *
					 sizeof (struct chip_info));
	  info->chips = new_chips;
	}
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
#ifdef DEBUG
  fprintf (stderr, "unkow command %s\n", params->param_argv);
#endif
  errno = EINVAL;
  return -1;
};

int
dome_command_handler (struct param_status *params, struct dome_info *info)
{
#ifdef DEBUG
  printf ("dome get response: %s\n", params->param_argv);
#endif
  if (!strcmp (params->param_argv, "temperature"))
    return param_next_float (params, &info->temperature);
  if (!strcmp (params->param_argv, "humidity"))
    return param_next_float (params, &info->humidity);
  errno = EINVAL;
  return -1;
};

int
phot_command_handler (struct param_status *params, struct phot_info *info)
{
#ifdef DEBUG
  printf ("phot get response: %s\n", params->param_argv);
#endif
  if (!strcmp (params->param_argv, "count"))
    return param_next_integer (params, &info->count);
  errno = EINVAL;
  return -1;
};

struct supp_info devhnd_devices[] = {
  {
   NULL, NULL}
  ,				// DEVICE_TYPE_UNKNOW
  {
   (devcli_handle_response_t) serverd_command_handler, NULL}
  ,				// SERVERD
  {
   (devcli_handle_response_t) telescope_command_handler, NULL}
  ,				// DEVICE_TYPE_MOUNT
  {
   (devcli_handle_response_t) camera_command_handler, NULL}
  ,				// DEVICE_TYPE_CCD
  {
   (devcli_handle_response_t) dome_command_handler, NULL}
  ,				// DEVICE_TYPE_DOME
  {
   NULL, NULL}
  ,				// DEVICE_TYPE_WEATHER
  {
   NULL, NULL}
  ,				// DEVICE_TYPE_ARCH
  {
   (devcli_handle_response_t) phot_command_handler, NULL}
  // DEVICE_TYPE_PHOT

  // etc..
};

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <math.h>
#include <stdarg.h>

#include <argz.h>

#include "sbig.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "../status.h"

#include "../utils/devcli.h"	// client for connection to server

#define PORT    5556


int port;

struct sbig_init info;
struct sbig_readout readout[2];

float readout_comp[2];

int
complete (int ccd, float percent_complete)
{
  readout_comp[ccd] = percent_complete;
  return 1;
}

/* expose functions */
#define SBIG_EXPOSE ((struct sbig_expose *) arg)

// expose cleanup functions
void
clean_expose_cancel (void *arg)
{
  devdem_status_mask (SBIG_EXPOSE->ccd,
		      CAM_MASK_EXPOSE,
		      CAM_NOEXPOSURE, "exposure chip canceled");
}

// wrapper to call sbig expose in thread, test for results
void *
start_expose (void *arg)
{
  int ret;
  devdem_status_mask (SBIG_EXPOSE->ccd,
		      CAM_MASK_EXPOSE, CAM_EXPOSING, "exposure chip started");
  if ((ret = sbig_expose (SBIG_EXPOSE)) < 0)
    {
      char *err;
      err = sbig_show_error (ret);
      syslog (LOG_ERR, "error during chip %i exposure: %s",
	      SBIG_EXPOSE->ccd, err);
      free (err);
      devdem_status_mask (SBIG_EXPOSE->ccd,
			  CAM_MASK_EXPOSE,
			  CAM_NOEXPOSURE, "exposure chip error");
      return NULL;
    }
  syslog (LOG_INFO, "exposure chip %i finished.", SBIG_EXPOSE->ccd);
  devdem_status_mask (SBIG_EXPOSE->ccd,
		      CAM_MASK_EXPOSE,
		      CAM_NOEXPOSURE, "exposure chip finished");
  return NULL;
}

#undef SBIG_EXPOSE

/* readout functions */
#define SBIG_READOUT ((struct sbig_readout *) arg)

// readout cleanup functions 
void
clean_readout_cancel (void *arg)
{
  devdem_status_mask (SBIG_READOUT->ccd,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_NOTREADING | CAM_NODATA, "reading chip canceled");
}

// wrapper to call sbig readout in thread 
void *
start_readout (void *arg)
{
  int ret;
  devdem_status_mask (SBIG_READOUT->ccd,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_READING | CAM_NODATA, "reading chip started");
  if ((ret = sbig_readout (SBIG_READOUT)) < 0)
    {
      char *err;
      err = sbig_show_error (ret);
      syslog (LOG_ERR, "error during chip %i readout: %s",
	      SBIG_READOUT->ccd, err);
      free (err);
      devdem_status_mask (SBIG_READOUT->ccd,
			  CAM_MASK_READING | CAM_MASK_DATA,
			  CAM_NOTREADING | CAM_NODATA, "reading chip error");
      return NULL;
    }
  syslog (LOG_INFO, "reading chip %i finished.", SBIG_READOUT->ccd);
  devdem_status_mask (SBIG_READOUT->ccd,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_NOTREADING | CAM_DATA, "reading chip finished");
  return NULL;
}

#undef SBIG_READOUT

// macro for chip test
#define get_chip  \
      if (devdem_param_next_integer (&chip)) \
      	return -1; \
      if ((chip < 0) || (chip >= info.nmbr_chips)) \
        {      \
	  devdem_write_command_end (DEVDEM_E_PARAMSVAL, "invalid chip: %i", chip);\
	  return -1;\
	}

// Macro for camera call
# define cam_call(call) if ((ret = call) < 0)\
{\
	char *err; \
	err = sbig_show_error(ret); \
	devdem_write_command_end (DEVDEM_E_HW, "camera error: %s", err);\
	free (err); \
        return -1; \
}


/*! Handle camd command.
 *
 * @param command	received command
 * @return -2 on exit, -1 and set errno on HW failure, 0 otherwise
 */
int
camd_handle_command (char *command)
{
  int ret;
  int chip;

  if (strcmp (command, "ready") == 0)
    {
      cam_call (sbig_init (port, 5, &info));
    }
  else if (strcmp (command, "info") == 0)
    {
      cam_call (sbig_init (port, 5, &info));
      devdem_dprintf ("name %s", info.camera_name);
      devdem_dprintf ("type %i", info.camera_type);
      devdem_dprintf ("serial %10s", info.serial_number);
      devdem_dprintf ("abg %i", info.imaging_abg_type);
      devdem_dprintf ("chips %i", info.nmbr_chips);
    }
  else if (strcmp (command, "chipinfo") == 0)
    {
      int i;
      readout_mode_t *mode;
      if (devdem_param_test_length (1))
	return -1;
      get_chip;
      mode = (readout_mode_t *) (&info.camera_info[chip].readout_mode[0]);
      devdem_dprintf ("binning %i", readout[chip].binning);
      devdem_dprintf ("readout_modes %i",
		      info.camera_info[chip].nmbr_readout_modes);
      for (i = 0; i < info.camera_info[chip].nmbr_readout_modes; i++)
	{
	  devdem_dprintf ("mode %i %i %i %i %i %i", mode->mode, mode->width,
			  mode->height, mode->gain, mode->pixel_width,
			  mode->pixel_height);
	  mode++;
	}
      ret = 0;
    }
  else if (strcmp (command, "expose") == 0)
    {
      float exptime;
      if (devdem_param_test_length (2))
	return -1;
      get_chip;
      if (devdem_param_next_float (&exptime))
	return -1;
      exptime = exptime * 100;	// convert to sbig required milliseconds
      if ((exptime <= 0) || (exptime > 330000))
	{
	  devdem_write_command_end (DEVDEM_E_PARAMSVAL,
				    "invalid exposure time (must be in <0, 330000>): %f",
				    exptime);
	}
      else
	{
	  struct sbig_expose expose;
	  expose.ccd = chip;
	  expose.exposure_time = exptime;
	  expose.abg_state = 0;
	  expose.shutter = 0;
	  /* priority block start here */
	  if (devdem_priority_block_start ())
	    return -1;

	  if ((ret =
	       devdem_thread_create (start_expose, (void *) &expose,
				     sizeof expose, NULL,
				     clean_expose_cancel)) < 0)
	    {
	      devdem_write_command_end (DEVDEM_E_SYSTEM,
					"while creating thread for execution: %s",
					strerror (errno));
	      return -1;
	    }
	  devdem_priority_block_end ();
	  /* priority block ends here */
	}
    }
  else if (strcmp (command, "stopexpo") == 0)
    {
      if (devdem_param_test_length (1))
	return -1;
      get_chip;

      /* priority block starts here */
      if (devdem_priority_block_start ())
	return -1;
      cam_call (sbig_end_expose (chip));
      devdem_priority_block_end ();
      /* priority block stops here */

    }
  else if (strcmp (command, "progexpo") == 0)
    {
      if (devdem_param_test_length (1))
	return -1;
      get_chip;
      /* TODO add suport for that */
      devdem_write_command_end (DEVDEM_E_SYSTEM, "not supported (yet)");
      return -1;
    }
  else if (strcmp (command, "readout") == 0)
    {
      int mode;
      if (devdem_param_test_length (1))
	return -1;
      get_chip;
      readout[chip].ccd = chip;
      readout[chip].x = readout[chip].y = 0;
      mode = readout[chip].binning;
      readout[chip].width = info.camera_info[chip].readout_mode[mode].width;
      readout[chip].height = info.camera_info[chip].readout_mode[mode].height;
      readout[chip].callback = complete;

      /* start priority block */
      if (devdem_priority_block_start ())
	return -1;

      if ((ret =
	   devdem_thread_create (start_readout,
				 (void *) &readout[chip], 0,
				 &readout[chip].thread_id,
				 clean_readout_cancel)))
	{
	  devdem_write_command_end (DEVDEM_E_SYSTEM,
				    "while creating thread for execution: %s",
				    strerror (errno));
	  return -1;
	}
      devdem_priority_block_end ();
      /* here ends priority block */
    }
  else if (strcmp (command, "binning") == 0)
    {
      int new_bin;
      if (devdem_param_test_length (2))
	return -1;
      get_chip;
      if (devdem_param_next_integer (&new_bin))
	return -1;

      if (new_bin < 0 || new_bin >= info.camera_info[chip].nmbr_readout_modes)
	{
	  devdem_write_command_end (DEVDEM_E_PARAMSVAL, "invalid binning: %i",
				    new_bin);
	  return -1;
	}
      readout[chip].binning = (unsigned int) new_bin;
      ret = 0;
    }
  else if (strcmp (command, "stopread") == 0)
    {
      if (devdem_param_test_length (1))
	return -1;
      get_chip;

      /* priority block starts here */
      if (devdem_priority_block_start ())
	return -1;
      if ((ret = devdem_thread_cancel (readout[chip].thread_id)) < 0)
	{
	  devdem_write_command_end (DEVDEM_E_SYSTEM,
				    "while canceling thread: %s",
				    strerror (errno));
	  return -1;
	}
      readout_comp[chip] = -1;
      devdem_priority_block_end ();
      /* priorioty block end here */

    }
  else if (strcmp (command, "status") == 0)
    {
      int i;
      if (devdem_param_test_length (1))
	return -1;
      get_chip;
      for (i = 0; i < 3; i++)
	{
	  devdem_status_message (i, "status request");
	}
      devdem_dprintf ("readout %f", readout_comp[chip]);
      ret = 0;
    }
  else if (strcmp (command, "data") == 0)
    {
      if (devdem_param_test_length (1))
	return -1;
      get_chip;
      if (readout[chip].data_size_in_bytes == 0)
	{
	  devdem_write_command_end (DEVDEM_E_SYSTEM, "data not available");
	  return -1;
	}
      ret =
	devdem_send_data (NULL, readout[chip].data,
			  readout[chip].data_size_in_bytes);
    }
  else if (strcmp (command, "cool_temp") == 0)
    {
      struct sbig_cool cool;
      float new_temp;

      if (devdem_param_test_length (1))
	return -1;
      if (devdem_param_next_float (&new_temp))
	return -1;
      cool.temperature = round (new_temp * 10);	// convert to sbig 1/10 o C

      cool.regulation = 1;

      /* priority block starts here */
      if (devdem_priority_block_start ())
	return -1;
      cam_call (sbig_set_cooling (&cool));
      devdem_priority_block_end ();
      /* priority block ends here */
    }
  else if (strcmp (command, "exit") == 0)
    {
      return -2;
    }
  else if (strcmp (command, "help") == 0)
    {
      devdem_dprintf ("ready - is camera ready?");
      devdem_dprintf ("info - information about camera");
      devdem_dprintf ("chipinfo <chip> - information about chip");
      devdem_dprintf
	("expose <chip> <exposure> - start exposition on given chip");
      devdem_dprintf ("stopexpo <chip> - stop exposition on given chip");
      devdem_dprintf ("progexpo <chip> - query exposition progress");
      devdem_dprintf ("readout <chip> - start reading given chip");
      devdem_dprintf
	("binning <chip> <binning_id> - set new binning; actual from next readout on");
      devdem_dprintf ("stopread <chip> - stop reading given chip");
      devdem_dprintf
	("data <chip> - establish data connection on given port and address");
      devdem_dprintf ("cool_temp <temp> - cooling temperature");
      devdem_dprintf ("exit - exit from connection");
      devdem_dprintf ("help - print, what you are reading just now");
      ret = errno = 0;
      goto end;
    }
  else
    {
      devdem_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s",
				command);
      return -1;
    }

end:
  return ret;
}

int
main (void)
{
  int i;
  char *stats[] = { "img_chip", "trc_chip" };
  struct devcli_channel server_channel;

  port = 2;
#ifdef DEBUG
  mtrace ();
#endif
  for (i = 0; i < 2; i++)
    readout[i].data = NULL;

  // open syslog
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  if (devdem_register (&server_channel, "camd", "localhost", 5557) < 0)
    {
      perror ("devdem_register");
      exit (EXIT_FAILURE);
    }

  info.nmbr_chips = 2;

  return devdem_run (PORT, camd_handle_command, stats, 2, sizeof (pid_t));
}

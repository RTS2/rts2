#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <math.h>
#include <stdarg.h>
#include <getopt.h>

#include <argz.h>

#include "sbig.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "../status.h"

#include "../writers/imghdr.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_PORT		5556	// default camera TCP/IP port
#define DEVICE_NAME "camd"	// default camera name
int sbig_port = 2;		// default sbig camera port is 2

struct sbig_init info;

struct readout
{
  int conn_id;			// data connection
  int x, y, width, height;	// image offset and size
  int ccd;
  float complete;
  int thread_id;
  struct imghdr header;
};
struct readout readouts[2];

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
#define READOUT ((struct readout *) arg)

// readout cleanup functions 
void
clean_readout_cancel (void *arg)
{
  devdem_status_mask (READOUT->ccd,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_NOTREADING | CAM_NODATA, "reading chip canceled");
}

// wrapper to call sbig readout in thread 
void *
start_readout (void *arg)
{
  int ret;
  struct sbig_readout_line line;
  unsigned int y;
  int result;
  unsigned short line_buff[5000];

  devdem_status_mask (READOUT->ccd,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_READING | CAM_NODATA, "reading chip started");

  if ((ret = sbig_end_expose (READOUT->ccd)))
    {
      goto err;
    }

  if ((READOUT->y) > 0)
    {
      struct sbig_dump_lines dump;

      dump.ccd = READOUT->ccd;
      dump.readoutMode = READOUT->header.binnings[1];
      dump.lineLength = READOUT->y;

      if ((ret = sbig_dump_lines (&dump)) < 0)
	goto err;
    }

  line.ccd = READOUT->ccd;
  line.pixelStart = READOUT->x;
  line.pixelLength = READOUT->width;
  // TODO probrat s matesem vycitani a jeho binningy
  line.readoutMode = 0;		//READOUT->header.binnings[0];
  line.data = line_buff;

  for (y = 0; y < (READOUT->height); y++)
    {
      int line_size = 2 * line.pixelLength;
      if ((result = sbig_readout_line (&line)) != 0)
	goto err;

      devser_data_put (READOUT->conn_id, line_buff, line_size);
    }

  sbig_end_readout (READOUT->ccd);

  syslog (LOG_INFO, "reading chip %i finished.", READOUT->ccd);
  devdem_status_mask (READOUT->ccd,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_NOTREADING | CAM_DATA, "reading chip finished");
  return NULL;

err:
  devser_data_done (READOUT->conn_id);
  {
    char *err;
    err = sbig_show_error (ret);
    syslog (LOG_ERR, "error during chip %i readout: %s", READOUT->ccd, err);
    free (err);
    devdem_status_mask (READOUT->ccd,
			CAM_MASK_READING | CAM_MASK_DATA,
			CAM_NOTREADING | CAM_NODATA, "reading chip error");
    return NULL;
  }
}

#undef READOUT

// macro for chip test
#define get_chip  \
      if (devser_param_next_integer (&chip)) \
      	return -1; \
      if ((chip < 0) || (chip >= info.nmbr_chips)) \
	{      \
	  devser_write_command_end (DEVDEM_E_PARAMSVAL, "invalid chip: %i", chip);\
	  return -1;\
	}

// Macro for camera call
# define cam_call(call) if ((ret = call) < 0)\
{\
	char *err; \
	err = sbig_show_error(ret); \
	devser_write_command_end (DEVDEM_E_HW, "camera error: %s", err);\
	free (err); \
        return -1; \
}


/*! 
 * Handle camd command.
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
      cam_call (sbig_init (sbig_port, 5, &info));
    }
  else if (strcmp (command, "info") == 0)
    {
      cam_call (sbig_init (sbig_port, 5, &info));
      devser_dprintf ("name %s", info.camera_name);
      devser_dprintf ("type %i", info.camera_type);
      devser_dprintf ("serial %10s", info.serial_number);
      devser_dprintf ("abg %i", info.imaging_abg_type);
      devser_dprintf ("chips %i", info.nmbr_chips);
    }
  else if (strcmp (command, "chipinfo") == 0)
    {
      int i;
      readout_mode_t *mode;
      if (devser_param_test_length (1))
	return -1;
      get_chip;
      mode = (readout_mode_t *) (&info.camera_info[chip].readout_mode[0]);
      devser_dprintf ("binning %i", readouts[chip].header.binnings[0]);
      devser_dprintf ("readout_modes %i",
		      info.camera_info[chip].nmbr_readout_modes);
      for (i = 0; i < info.camera_info[chip].nmbr_readout_modes; i++)
	{
	  devser_dprintf ("mode %i %i %i %i %i %i", mode->mode, mode->width,
			  mode->height, mode->gain, mode->pixel_width,
			  mode->pixel_height);
	  mode++;
	}
      ret = 0;
    }
  else if (strcmp (command, "expose") == 0)
    {
      float exptime;
      if (devser_param_test_length (2))
	return -1;
      get_chip;
      if (devser_param_next_float (&exptime))
	return -1;
      exptime = exptime * 100;	// convert to sbig required milliseconds
      if ((exptime <= 0) || (exptime > 330000))
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL,
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
	       devser_thread_create (start_expose, (void *) &expose,
				     sizeof expose, NULL,
				     clean_expose_cancel)) < 0)
	    {
	      devser_write_command_end (DEVDEM_E_SYSTEM,
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
      if (devser_param_test_length (1))
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
      if (devser_param_test_length (1))
	return -1;
      get_chip;
      /* TODO add suport for that */
      devser_write_command_end (DEVDEM_E_SYSTEM, "not supported (yet)");
      return -1;
    }
  else if (strcmp (command, "readout") == 0)
    {
      size_t data_size;
      struct readout *rd;
      struct imghdr *header;

      if (devser_param_test_length (1))
	return -1;
      /* start priority block */
      if (devdem_priority_block_start ())
	return -1;

      get_chip;

      rd = &readouts[chip];
      rd->ccd = chip;
      rd->x = rd->y = 0;
      rd->width = info.camera_info[chip].readout_mode[0].width;
      rd->height = info.camera_info[chip].readout_mode[0].height;

      // set data header
      header = &(rd->header);
      header->data_type = 1;
      header->naxes = 2;
      header->sizes[0] = rd->width;
      header->sizes[1] = rd->height;
      header->binnings[0] = 1;
      header->binnings[1] = 1;

      data_size =
	sizeof (struct imghdr) +
	sizeof (unsigned short) * rd->width * rd->height;

      // open data connection
      if (devser_data_init
	  (2 * sizeof (unsigned short) * rd->width, data_size, &rd->conn_id))
	return -1;

      if (devser_data_put (rd->conn_id, header, sizeof (*header)))
	return -1;

      if ((ret =
	   devser_thread_create (start_readout,
				 (void *) rd, 0,
				 &rd->thread_id, clean_readout_cancel)))
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "while creating thread for execution: %s",
				    strerror (errno));
	  devser_data_done (rd->conn_id);
	  return -1;
	}
      devdem_priority_block_end ();
      /* here ends priority block */
    }
  else if (strcmp (command, "binning") == 0)
    {
      int new_bin;
      if (devser_param_test_length (2))
	return -1;
      /* priority block start */
      if (devdem_priority_block_start ())
	return -1;
      get_chip;
      if (devser_param_next_integer (&new_bin))
	return -1;

      if (new_bin < 0 || new_bin >= info.camera_info[chip].nmbr_readout_modes)
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL, "invalid binning: %i",
				    new_bin);
	  devdem_priority_block_end ();
	  return -1;
	}
      readouts[chip].header.binnings[0] = (unsigned int) new_bin;
      readouts[chip].header.binnings[1] = (unsigned int) new_bin;
      ret = 0;
      devdem_priority_block_end ();
      /* end of priority block */
    }
  else if (strcmp (command, "stopread") == 0)
    {
      if (devser_param_test_length (1))
	return -1;
      get_chip;

      /* priority block starts here */
      if (devdem_priority_block_start ())
	return -1;
      if ((ret = devser_thread_cancel (readouts[chip].thread_id)) < 0)
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "while canceling thread: %s",
				    strerror (errno));
	  return -1;
	}
      devdem_priority_block_end ();
      /* priorioty block end here */

    }
  else if (strcmp (command, "status") == 0)
    {
      int i;
      if (devser_param_test_length (1))
	return -1;
      get_chip;
      for (i = 0; i < 3; i++)
	{
	  devdem_status_message (i, "status request");
	}
      devser_dprintf ("readout %f", readouts[chip].complete);
      ret = 0;
    }
  else if (strcmp (command, "cooltemp") == 0)
    {
      struct sbig_cool cool;
      float new_temp;

      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_float (&new_temp))
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
      devser_dprintf ("ready - is camera ready?");
      devser_dprintf ("info - information about camera");
      devser_dprintf ("chipinfo <chip> - information about chip");
      devser_dprintf
	("expose <chip> <exposure> - start exposition on given chip");
      devser_dprintf ("stopexpo <chip> - stop exposition on given chip");
      devser_dprintf ("progexpo <chip> - query exposition progress");
      devser_dprintf ("readout <chip> - start reading given chip");
      devser_dprintf
	("binning <chip> <binning_id> - set new binning; actual from next readout on");
      devser_dprintf ("stopread <chip> - stop reading given chip");
      devser_dprintf ("cool_temp <temp> - cooling temperature");
      devser_dprintf ("exit - exit from connection");
      devser_dprintf ("help - print, what you are reading just now");
      ret = errno = 0;
      goto end;
    }
  else
    {
      devser_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s",
				command);
      return -1;
    }

end:
  return ret;
}

int
main (int argc, char **argv)
{
  char *stats[] = { "img_chip", "trc_chip" };

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  char *device_name = DEVICE_NAME;
  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;

#ifdef DEBUG
  mtrace ();
#endif

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"sbig_port", 1, 0, 'l'},
	{"port", 1, 0, 'p'},
	{"serverd_host", 1, 0, 's'},
	{"serverd_port", 1, 0, 'q'},
	{"device_name", 1, 0, 'd'},
	{"help", 0, 0, 0},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "l:p:s:q:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'l':
	  sbig_port = atoi (optarg);
	  if (sbig_port < 1 || sbig_port > 3)
	    {
	      printf ("invalid sbig_port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'p':
	  device_port = atoi (optarg);
	  if (device_port < 1 || device_port == UINT_MAX)
	    {
	      printf ("invalid device port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 's':
	  serverd_host = optarg;
	  break;
	case 'q':
	  serverd_port = atoi (optarg);
	  if (serverd_port < 1 || serverd_port == UINT_MAX)
	    {
	      printf ("invalid serverd port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'd':
	  device_name = optarg;
	  break;
	case 0:
	  printf
	    ("Options:\n\tserverd_port|p <port_num>\t\tport of the serverd");
	  exit (EXIT_SUCCESS);
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }

  if (optind != argc - 1)
    {
      printf ("hostname wasn't specified\n");
      exit (EXIT_FAILURE);
    }

  hostname = argv[argc - 1];

  // open syslog
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  if (devdem_init (stats, 2))
    {
      syslog (LOG_ERR, "devdem_init: %m");
      exit (EXIT_FAILURE);
    }

  syslog (LOG_DEBUG, "registering on %s:%i as %s", serverd_host, serverd_port,
	  device_name);

  if (devdem_register
      (serverd_host, serverd_port, device_name, DEVICE_TYPE_CCD, hostname,
       device_port) < 0)
    {
      syslog (LOG_ERR, "devdem_register: %m");
      perror ("devdem_register");
      exit (EXIT_FAILURE);
    }

  info.nmbr_chips = 2;

  return devdem_run (device_port, camd_handle_command);
}

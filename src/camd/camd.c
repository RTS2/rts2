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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <argz.h>

#include "camera.h"
#include "../utils/devdem.h"
#include "status.h"

#include "imghdr.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_PORT		5556	// default camera TCP/IP port
#define DEVICE_NAME 		"C0"	// default camera name

#define MAX_CHIPS		2

int sbig_port;			// default sbig camera port is 2

struct camera_info info;

struct camd_expose
{
  int chip;
  float exposure;
  int light;
};

struct readout
{
  int conn_id;			// data connection
  int x, y, width, height;	// image offset and size
  int chip;
  float complete;
  int thread_id;
  struct imghdr header;
  int d_x, d_y, d_width, d_height;
};
struct readout readouts[MAX_CHIPS];

char device_name[64];

/* expose functions */
#define CAMD_EXPOSE ((struct camd_expose *) arg)

// expose cleanup functions
void
clean_expose_cancel (void *arg)
{
  devdem_status_mask (CAMD_EXPOSE->chip,
		      CAM_MASK_EXPOSE,
		      CAM_NOEXPOSURE, "exposure chip canceled");
}

// wrapper to call sbig expose in thread, test for results
void *
start_expose (void *arg)
{
  int ret;
  camera_init ("/dev/ccd1", sbig_port);
  if ((ret =
       camera_expose (CAMD_EXPOSE->chip, &CAMD_EXPOSE->exposure,
		      CAMD_EXPOSE->light)) < 0)
    {
      syslog (LOG_ERR, "error during chip %i exposure", CAMD_EXPOSE->chip);
      devdem_status_mask (CAMD_EXPOSE->chip,
			  CAM_MASK_EXPOSE,
			  CAM_NOEXPOSURE, "exposure chip error");
      return NULL;
    }
  syslog (LOG_INFO, "exposure chip %i finished.", CAMD_EXPOSE->chip);
  devdem_status_mask (CAMD_EXPOSE->chip,
		      CAM_MASK_EXPOSE | CAM_MASK_DATA,
		      CAM_NOEXPOSURE | CAM_DATA, "exposure chip finished");
  return NULL;
}

#undef SBIG_EXPOSE

/* readout functions */
#define READOUT ((struct readout *) arg)

// readout cleanup functions 
void
clean_readout_cancel (void *arg)
{
  devdem_status_mask (READOUT->chip,
		      CAM_MASK_READING,
		      CAM_NOTREADING, "reading chip canceled");
}

// wrapper to call sbig readout in thread 
void *
start_readout (void *arg)
{
  int ret;
  int i;
  unsigned int y;
  int result;
  unsigned short *line_buf = NULL;

  int line_size = sizeof (unsigned short) * READOUT->width;

  line_buf = (unsigned short *) malloc (line_size);

  if (!line_buf)
    {
      ret = -1;
      goto err;
    }

  if ((ret = camera_end_expose (READOUT->chip)))
    {
      goto err;
    }

  syslog (LOG_DEBUG, "reading out: [%i,%i:%i,%i]", READOUT->x, READOUT->width,
	  READOUT->y, READOUT->height);

  for (i = 0; i < READOUT->y; i++)
    if ((ret = camera_dump_line (READOUT->chip)) < 0)
      goto err;

  for (y = 0; y < (READOUT->height); y++)
    {
      if ((result =
	   camera_readout_line (READOUT->chip, READOUT->x, READOUT->width,
				line_buf)) != 0)
	goto err;

      if (devser_data_put (READOUT->conn_id, line_buf, line_size))
	goto err;
    }

  free (line_buf);
  camera_end_readout (READOUT->chip);

  syslog (LOG_INFO, "reading chip %i finished.", READOUT->chip);
  devdem_status_mask (READOUT->chip,
		      CAM_MASK_READING,
		      CAM_NOTREADING, "reading chip finished");
  return NULL;

err:
  syslog (LOG_ERR, "error during chip %i readout", READOUT->chip);
  devdem_status_mask (READOUT->chip,
		      CAM_MASK_READING | CAM_MASK_DATA,
		      CAM_NOTREADING | CAM_NODATA, "reading chip error");
  free (line_buf);
  devser_data_done (READOUT->conn_id);
  return NULL;
}

#undef READOUT

// macro for chip test
#define get_chip  \
      if (devser_param_next_integer (&chip)) \
      	return -1; \
      if ((chip < 0) || (chip >= info.chips)) \
	{      \
	  devser_write_command_end (DEVDEM_E_PARAMSVAL, "invalid chip: %i", chip);\
	  return -1;\
	}

// Macro for camera call
# define cam_call(call) if ((ret = call) < 0)\
{\
	devser_write_command_end (DEVDEM_E_HW, "camera error!");\
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
  int ret = -1;
  int chip;

  if (strcmp (command, "ready") == 0)
    {
      int i;
      cam_call (camera_init ("/dev/ccd1", sbig_port));
      cam_call (camera_info (&info));
      atexit (camera_done);
    }
  else if (strcmp (command, "info") == 0)
    {
      cam_call (camera_info (&info));
      devser_dprintf ("type %s", info.type);
      devser_dprintf ("serial %10s", info.serial_number);
      devser_dprintf ("chips %i", info.chips);
      devser_dprintf ("temperature_regulation %i",
		      info.temperature_regulation);
      devser_dprintf ("temperature_setpoint %0.02f",
		      info.temperature_setpoint);
      devser_dprintf ("air_temperature %0.02f", info.air_temperature);
      devser_dprintf ("ccd_temperature %0.02f", info.ccd_temperature);
      devser_dprintf ("cooling_power %4i", (int) info.cooling_power);
      devser_dprintf ("fan %i", info.fan);
    }
  else if (strcmp (command, "chipinfo") == 0)
    {
      if (devser_param_test_length (1))
	return -1;
      get_chip;
      cam_call (camera_info (&info));

      devser_dprintf ("chip %i width %i", chip, info.chip_info[chip].width);
      devser_dprintf ("chip %i height %i", chip, info.chip_info[chip].height);
      devser_dprintf ("chip %i binning_vertical %i", chip,
		      info.chip_info[chip].binning_vertical);
      devser_dprintf ("chip %i binning_horizontal %i", chip,
		      info.chip_info[chip].binning_horizontal);
      devser_dprintf ("chip %i pixelX %i", chip, info.chip_info[chip].pixelX);
      devser_dprintf ("chip %i pixelY %i", chip, info.chip_info[chip].pixelY);
      devser_dprintf ("chip %i gain %0.2f", chip, info.chip_info[chip].gain);

      ret = 0;
    }
  else if (strcmp (command, "expose") == 0)
    {
      float exptime;
      int light;
      if (devser_param_test_length (3))
	return -1;
      get_chip;
      if (devser_param_next_integer (&light))
	return -1;
      if (devser_param_next_float (&exptime))
	return -1;
      if ((exptime <= 0) || (exptime > 330000))
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL,
				    "invalid exposure time (must be in <0, 330000>): %f",
				    exptime);
	}
      else
	{
	  struct camd_expose expose;
	  expose.chip = chip;
	  expose.exposure = exptime;
	  expose.light = light;
	  /* priority block start here */
	  if (devdem_priority_block_start ())
	    return -1;

	  devdem_status_mask (chip,
			      CAM_MASK_EXPOSE | CAM_MASK_DATA,
			      CAM_EXPOSING | CAM_NODATA,
			      "exposure chip started");

	  if ((ret =
	       devser_thread_create (start_expose, (void *) &expose,
				     sizeof expose, NULL,
				     clean_expose_cancel)) < 0)
	    {
	      devdem_status_mask (chip,
				  CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
				  "thread create error");
	    }
	  devdem_priority_block_end ();
	  /* priority block ends here */
	  return ret;
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
      cam_call (camera_end_expose (chip));
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
  else if (strcmp (command, "box") == 0)
    {
      struct readout *rd;
      if (devser_param_test_length (5))
	return -1;
      get_chip;
      rd = &readouts[chip];
      if (devser_param_next_integer (&rd->d_x) ||
	  devser_param_next_integer (&rd->d_y) ||
	  devser_param_next_integer (&rd->d_width) ||
	  devser_param_next_integer (&rd->d_height))
	return -1;

      return 0;
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
      rd->chip = chip;
      if (rd->d_height >= 0)
	{
	  rd->x = rd->d_x / info.chip_info[chip].binning_vertical;
	  rd->y = rd->d_y / info.chip_info[chip].binning_horizontal;
	  rd->width = rd->d_width / info.chip_info[chip].binning_vertical;
	  rd->height = rd->d_height / info.chip_info[chip].binning_horizontal;
	}
      else
	{
	  // full screen
	  rd->x = rd->y = 0;
	  rd->width =
	    info.chip_info[chip].width /
	    info.chip_info[chip].binning_vertical;
	  rd->height =
	    info.chip_info[chip].height /
	    info.chip_info[chip].binning_horizontal;
	}

      // set data header
      header = &(rd->header);
      header->data_type = 1;
      header->naxes = 2;
      header->sizes[0] = rd->width;
      header->sizes[1] = rd->height;
      header->binnings[0] = 1;
      header->binnings[1] = 1;
      header->status = STATUS_FLIP;

      data_size =
	sizeof (struct imghdr) +
	sizeof (unsigned short) * rd->width * rd->height;

      // open data connection
      if (devser_data_init
	  (2 * sizeof (unsigned short) * rd->width, data_size, &rd->conn_id))
	return -1;

      if (devser_data_put (rd->conn_id, header, sizeof (*header)))
	{
	  devser_data_done (rd->conn_id);
	  return -1;
	}

      devdem_status_mask (chip,
			  CAM_MASK_READING | CAM_MASK_DATA,
			  CAM_READING | CAM_NODATA, "reading chip started");

      if ((ret =
	   devser_thread_create (start_readout,
				 (void *) rd, 0,
				 &rd->thread_id, clean_readout_cancel)))
	{
	  devdem_status_mask (chip,
			      CAM_MASK_READING,
			      CAM_NOTREADING,
			      "error creating readout thread");

	  devser_data_done (rd->conn_id);

	}
      devdem_priority_block_end ();
      /* here ends priority block */
      return ret;
    }
  else if (strcmp (command, "binning") == 0)
    {
      int vertical, horizontal;
      if (devser_param_test_length (3))
	return -1;
      /* priority block start */
      if (devdem_priority_block_start ())
	return -1;
      get_chip;
      if (devser_param_next_integer (&vertical)
	  || devser_param_next_integer (&horizontal))
	return -1;

      if (camera_binning (chip, vertical, horizontal))
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL,
				    "invalid binning: %ix%i", vertical,
				    horizontal);
	  devdem_priority_block_end ();
	  return -1;
	}
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
      for (i = 0; i < 2; i++)
	{
	  devdem_status_message (i, "status request");
	}
      devser_dprintf ("readout %f", readouts[chip].complete);
      ret = 0;
    }
  else if (strcmp (command, "coolmax") == 0)
    {
      if (devser_param_test_length (0))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      cam_call (camera_cool_max ());
      devdem_priority_block_end ();
    }
  else if (strcmp (command, "coolhold") == 0)
    {
      if (devser_param_test_length (0))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      cam_call (camera_cool_hold ());
      devdem_priority_block_end ();
    }
  else if (strcmp (command, "cooltemp") == 0)
    {
      float new_temp;

      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_float (&new_temp))
	return -1;
      // we don't need priority for that
      cam_call (camera_cool_setpoint (new_temp));
    }
  else if (strcmp (command, "coolshutdown") == 0)
    {
      if (devser_param_test_length (0))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      cam_call (camera_cool_shutdown ());
      devdem_priority_block_end ();
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
      devser_dprintf ("cooltemp <temp> - cooling temperature");
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
camd_handle_status (int status, int old_status)
{
  int ret;

  if (camera_init ("/dev/ccd1", sbig_port))
    return -1;

  switch (status & SERVERD_STATUS_MASK)
    {
    case SERVERD_EVENING:
      ret = camera_cool_max ();
      break;
    case SERVERD_NIGHT:
    case SERVERD_DUSK:
    case SERVERD_DAWN:
      ret = camera_cool_hold ();
      break;
    default:			/* SERVERD_DAY, SERVERD_DUSK, SERVERD_MAINTANCE, SERVERD_OFF */
      ret = camera_cool_shutdown ();
    }
  camera_done ();
  return ret;
}

int
main (int argc, char **argv)
{
  char *stats[] = { "img_chip", "trc_chip" };

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;

//#ifdef DEBUG
  mtrace ();
//#endif

  strcpy (device_name, DEVICE_NAME);

  sbig_port = 0;

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
      c = getopt_long (argc, argv, "l:p:s:q:d:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'l':
	  sbig_port = atoi (optarg);
	  if (sbig_port < 0 || sbig_port > 3)
	    {
	      printf ("invalid sbig_port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'p':
	  device_port = atoi (optarg);
	  break;
	case 's':
	  serverd_host = optarg;
	  break;
	case 'q':
	  serverd_port = atoi (optarg);
	  break;
	case 'd':
	  strncpy (device_name, optarg, 64);
	  device_name[63] = 0;
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

  for (c = 0; c < MAX_CHIPS; c++)
    {
      // init d_.*s
      readouts[c].d_x = readouts[c].d_y = readouts[c].d_width =
	readouts[c].d_height = -1;
    }

  if (devdem_init (stats, 2, camd_handle_status))
    {
      syslog (LOG_ERR, "devdem_init: %m");
      return EXIT_FAILURE;
    }

  syslog (LOG_DEBUG, "registering on %s:%i as %s", serverd_host, serverd_port,
	  device_name);

  if (devdem_register
      (serverd_host, serverd_port, device_name, DEVICE_TYPE_CCD, hostname,
       device_port) < 0)
    {
      devdem_done ();
      syslog (LOG_ERR, "devdem_register: %m");
      perror ("devdem_register");
      return EXIT_FAILURE;
    }

  devdem_run (device_port, camd_handle_command);
  return EXIT_SUCCESS;
}

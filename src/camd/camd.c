#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "camera.h"
#include "../utils/devdem.h"
#include "status.h"

#include "imghdr.h"

#ifdef FOCUSING
#include "focusing.h"
#include "tcf.h"
#include "sex_interface.h"

// Should be short: to beat the telescope drift because of bad tracking
#define FOCUS_EXPOSURE 10.0
// Something like 20*[f-ratio]
#define FOCUS_USTEP	200
#endif /* FOCUSING */

#ifdef MIRROR
#include "mirror.h"
#endif /* MIRROR */

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

char *device_file = "/dev/ccda";

#ifdef MIRROR
static char *mirror = "";
#endif

#ifdef FOCUSING
static char *focuser_port = NULL;
#endif

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
  camera_init (device_file, sbig_port);

#ifdef MIRROR
  if (CAMD_EXPOSE->light)
    mirror_open ();
#endif

  ret =
    camera_expose (CAMD_EXPOSE->chip, &CAMD_EXPOSE->exposure,
		   CAMD_EXPOSE->light);
#ifdef MIRROR
  if (CAMD_EXPOSE->light)
    mirror_close ();
#endif

  if (ret < 0)
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

#ifdef FOCUSING

/**
 * Read from camera whole image
 *
 * @param struct readout *	readout info
 * @param unsigned short *	bufer for readout; must be malloced
 * @return	<0 on error, 0 otherwise
 */
int
camera_readout (struct readout *readout, unsigned short *img)
{
  int y;
  for (y = 0; y < readout->y; y++)
    if (camera_dump_line (readout->chip) < 0)
      return -1;

  for (y = 0; y < readout->height; y++)
    {
      if (camera_readout_line (readout->chip, readout->x, readout->width,
			       img) < 0)
	return -1;
      img += readout->width;
    }

  camera_end_readout (readout->chip);
  return 0;
}

int
focus_expose_and_readout (float exposure, int light, struct readout *readout,
			  unsigned short *img)
{
  int ret;
  devser_dprintf ("foc: expose");
#ifdef MIRROR
  if (light)
    mirror_open ();
#endif
  ret = camera_expose (readout->chip, &exposure, light);
#ifdef MIRROR
  if (light)
    mirror_close ();
#endif
  if (ret)
    return -1;
  devser_dprintf ("foc: readout");
  return camera_readout (readout, img);
}

#define MAXTRIES		200

void *
start_focusing (void *arg)
{
  int chip = *(int *) arg;
  float exp_time = FOCUS_EXPOSURE;
  int x = 256;			// square size
  int tcfret;
  int j, i, r;

  unsigned short *img1 = NULL;
  unsigned short *img2 = NULL;
  unsigned short *img3 = NULL;
  struct readout readout;

  double fwhm[MAXTRIES];
  double posi[MAXTRIES];

  double M = 0, Q = 0;

  int add = 0;
  char filename[64];

  static int filen = 1;

  double A, B, C = 0, min = 0;
  double pp;			// position change on focuser

  tcfret = tcf_set_manual ();
  if (tcfret < 1)
    {
      devser_dprintf ("focuser: unable to initialize %i\n", tcfret);
      goto err;
    }

  posi[0] = 0;

  // limit x size
  x = (x > info.chip_info[chip].height) ? info.chip_info[chip].height : x;
  x = (x > info.chip_info[chip].width) ? info.chip_info[chip].width : x;

  // Make space for the image
  img1 = (unsigned short *) malloc (x * x * sizeof (unsigned short));
  img2 = (unsigned short *) malloc (x * x * sizeof (unsigned short));
  img3 = (unsigned short *) malloc (x * x * sizeof (unsigned short));

  readout.chip = chip;
  readout.x = (info.chip_info[chip].height - x) / 2;
  readout.y = (info.chip_info[chip].width - x) / 2;
  readout.height = x;
  readout.width = x;

  devser_dprintf ("readout: %i %i %i %i %i", readout.chip, readout.x,
		  readout.y, readout.height, readout.width);

  if (focus_expose_and_readout (exp_time, 0, &readout, img2))
    goto err;

  for (j = 0; j < 100;)
    {
      // save previous image if needed
      if (add)
	{
	  i = x * x;
	  while (i--)
	    img3[i] = img1[i];
	}

      if (focus_expose_and_readout (exp_time, 1, &readout, img1))
	goto err;
      i = x * x;
      while (i--)
	img1[i] = (img2[i] > img1[i]) ? 0 : img1[i] - img2[i];	// dark substract

      // add previous image if requested
      if (add)
	{
	  i = x * x;
	  while (i--)
	    img1[i] += img3[i];
	}

      getmeandisp (img1, x * x, &M, &Q);
      devser_dprintf ("Pixel value: %lf +/- %lf", M, Q);

      sprintf (filename, "!/tmp/fo_%i_%03di.fits", getpid (), filen++);
      write_fits (filename, exp_time, x, x, img1);

      r = sexi_fwhm (filename + 1, &fwhm[j]);
      devser_dprintf ("%s", sexi_text);

      if (r)
	goto err;

      if (fwhm[j] < 0)
	{
	  devser_dprintf ("coadding images");
	  add = 1;
	}
      else
	{
	  add = 0;		// enough stars:= reset adding
	  switch (j)
	    {
	    case 0:		// setup first step
	      pp = FOCUS_USTEP;
	      break;
	    case 1:
	      // it was worse before
	      if (fwhm[0] > fwhm[1])
		pp = FOCUS_USTEP;
	      // it was better
	      else
		pp = -2 * FOCUS_USTEP;
	      break;

	    case 2:
	      if (fwhm[0] > fwhm[1])	// x10 1x0 10x
		{
		  if (fwhm[2] > fwhm[0])
		    pp = (posi[1] + posi[0]) / 2;	// 102 
		  else
		    pp = (posi[1] + posi[2]) / 2;	// 120 210
		}
	      else		// 0x1 x01 01x
		{
		  if (fwhm[2] > fwhm[1])
		    pp = (posi[1] + posi[0]) / 2;	// 012
		  else
		    pp = (posi[2] + posi[0]) / 2;	// 201 021

		}
	      pp = pp - posi[2];
	      break;
	    default:		// We have 3 values, we may start fitting...

	      //if(j>2)
	      regr_q (posi, fwhm, j + 1, &A, &B, &C);
	      /*else
	         {
	         A=( (fwhm[0]-fwhm[1])/(posi[0]-posi[1]) - 
	         (fwhm[0]-fwhm[2])/(posi[0]-posi[2]))
	         /(posi[1]-posi[2]);
	         B=( (fwhm[1]-fwhm[1])/(posi[0]-posi[1]) - 
	         A*(posi[0]+posi[1]));
	         } */

	      min = -B / (2 * A);

	      // ted jsem na pos[j], chci se dostat na min, pokud
	      // fabs(min-pos[j])<3*FOCUS_USTEP

	      pp = min - posi[j];
	      if (pp > 3 * FOCUS_USTEP)
		pp = 3 * FOCUS_USTEP;
	      if (pp < (-3 * FOCUS_USTEP))
		pp = -3 * FOCUS_USTEP;

	      break;

	    }

	  devser_dprintf ("%02d: posi:%f, fwhm:%f -> min=%f, pp=%f", j,
			  posi[j], fwhm[j], min, pp);

	  if (j > 6 && pp < 1.0)
	    break;

	  posi[j + 1] = posi[j] + pp;

	  tcfret = tcf_step_out ((pp > 0) ? pp : -pp, (pp > 0) ? 1 : -1);
	  j++;
	}
    }

  free (img1);
  free (img2);
  free (img3);
  syslog (LOG_INFO, "focusing chip %i finished", chip);
  devdem_status_mask (chip,
		      CAM_MASK_FOCUSINGS,
		      CAM_NOFOCUSING, "focusing finished");
  return NULL;

err:
  free (img1);
  free (img2);
  free (img3);

  syslog (LOG_ERR, "error during chip %i focusing", chip);
  devdem_status_mask (chip,
		      CAM_MASK_FOCUSINGS,
		      CAM_NOFOCUSING, "focusing chip error");
  return NULL;
}

// focus cleanup functions 
void
clean_focusing_cancel (void *arg)
{
  devdem_status_mask (*(int *) arg,
		      CAM_MASK_FOCUSINGS,
		      CAM_NOFOCUSING, "focusing chip canceled");
}

#endif /* FOCUSING */


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
      cam_call (camera_init (device_file, sbig_port));
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
      devser_dprintf ("filter %i", info.filter);
      devser_dprintf ("can_df %i", info.can_df || (*mirror != 0));
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
      cam_call (camera_init ("/dev/ccd1", sbig_port));
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
	  devser_dprintf ("exposure %f", exptime);

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
      header->binnings[0] = info.chip_info[chip].binning_vertical;
      header->binnings[1] = info.chip_info[chip].binning_horizontal;
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
#ifdef FOCUSING
  else if (strcmp (command, "focus") == 0)
    {
      int focusing_chip;

      if (devser_param_test_length (1))
	return -1;
      /* priority block start */
      if (devdem_priority_block_start ())
	return -1;
      get_chip;

      devdem_status_mask (chip,
			  CAM_MASK_FOCUSINGS,
			  CAM_FOCUSING, "focusing chip started");
      focusing_chip = chip;

      if ((ret =
	   devser_thread_create (start_focusing,
				 (void *) &focusing_chip,
				 sizeof (focusing_chip), NULL,
				 clean_focusing_cancel)))
	{
	  devdem_status_mask (chip,
			      CAM_MASK_FOCUSINGS,
			      CAM_NOFOCUSING,
			      "error creating focusing thread");
	}
      devdem_priority_block_end ();
      return ret;
    }
#endif /* FOCUSING */
#if MIRROR
  else if (strcmp (command, "mirror") == 0)
    {
      char *dir;
      if (!*mirror)
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM, "mirror not configured");
	  return -1;
	}
      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_string (&dir))
	return -1;
      if (strcasecmp (dir, "open") == 0)
	ret = mirror_open ();
      else if (strcasecmp (dir, "close") == 0)
	ret = mirror_close ();
      else
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL,
				    "unknow mirror command '%s'", dir);
	  return -1;
	}
      if (ret < 0)
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "error during mirror operation: %i", ret);
	  return -1;
	}
      return ret;
    }
#endif /* MIRROR */
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
      devser_dprintf ("chip %i binning_vertical %i", chip, vertical);
      devser_dprintf ("chip %i binning_horizontal %i", chip, horizontal);
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
  else if (strcmp (command, "filter") == 0)
    {
      int filter;
      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_integer (&filter))
	return -1;

      if (devdem_priority_block_start ())
	return -1;
      cam_call (camera_set_filter (filter));
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
#ifdef FOCUSING
      devser_dprintf ("focus <chip> - try to focus image");
#endif /* FOCUSING */
#ifdef MIRROR
      devser_dprintf ("mirror <open|close> - open/close mirror");
#endif
      devser_dprintf
	("binning <chip> <binning_id> - set new binning; actual from next readout on");
      devser_dprintf ("stopread <chip> - stop reading given chip");
      devser_dprintf ("cooltemp <temp> - cooling temperature");
      devser_dprintf ("focus <steps> - change focus to the given steps");
      devser_dprintf ("autofocus - try to autofocus picture");
      devser_dprintf ("filter <filter number> - set camera filter");
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

  if (camera_init (device_file, sbig_port))
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
  int deamonize = 1;

  int ret;

#ifdef DEBUG
  mtrace ();
#endif

  strcpy (device_name, DEVICE_NAME);

  sbig_port = 0;

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"sbig_port", 1, 0, 'l'},
	{"port", 1, 0, 'p'},
	{"interactive", 0, 0, 'i'},
	{"serverd_host", 1, 0, 's'},
	{"serverd_port", 1, 0, 'q'},
	{"device_name", 1, 0, 'd'},
	{"device_file", 1, 0, 'f'},
#ifdef FOCUSING
	{"focuser_dev", 1, 0, 'o'},
#endif /* FOCUSING */
#ifdef MIRROR
	{"mirror_dev", 1, 0, 'm'},
#endif /* MIRROR */
	{"help", 0, 0, 0},
	{0, 0, 0, 0}
      };
#ifdef FOCUSING
#ifdef MIRROR
      c = getopt_long (argc, argv, "l:o:m:p:is:q:d:f:h", long_option, NULL);
#else
      c = getopt_long (argc, argv, "l:o:p:is:q:d:f:h", long_option, NULL);
#endif /* MIRROR */
#else
#ifdef MIRROR
      c = getopt_long (argc, argv, "l:p:m:is:q:d:f:h", long_option, NULL);
#else
      c = getopt_long (argc, argv, "l:p:is:q:d:f:h", long_option, NULL);
#endif /* MIRROR */
#endif /* FOCUSING */

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
	case 'i':
	  deamonize = 0;
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
	case 'f':
	  device_file = optarg;
	  break;
#ifdef FOCUSING
	case 'o':
	  focuser_port = optarg;
	  set_focuser_port (focuser_port);
	  break;
#endif /* FOCUSING */
#ifdef MIRROR
	case 'm':
	  mirror = optarg;
	  ret = mirror_open_dev (mirror);
	  mirror_close ();
	  if (ret < 0)
	    {
	      fprintf (stderr, "cannot open mirror port\n");
	      exit (EXIT_FAILURE);
	    }
	  break;
#endif /* MIRROR */
	case 0:
	  printf
	    ("Options:\n\tserverd_port|p <port_num>\t\tport of the serverd\n");
#ifdef FOCUSING
	  printf
	    ("\tfocuser_dev|o <dev-entry>\t\twhere focuser is connected. If not defined -> not focusing\n");
#endif /* FOCUSING */
#ifdef MIRROR
	  printf
	    ("\tmirror_port|m <dev-entry>\t\twhere mirror is connected. If not defined -> will not use mirror\n");
#endif /* MIRROR */

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

  if (devdem_init (stats, 2, camd_handle_status, deamonize))
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

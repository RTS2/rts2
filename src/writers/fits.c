/*! 
 * @file Writer routine for fits image
 *
 * @author petr
 */

#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <libnova.h>
#include <unistd.h>

#include "fitsio.h"
#include "fits.h"
#include "imghdr.h"

#define fits_call(call)\
	if (call) \
	{\
		fits_report_error (stderr, status); \
		errno = EINVAL; \
		return -1; \
	}


/*!
 * Init fits data.
 *
 * @param receiver 	receiver structure
 * @param filename	filename to store result fits, in fitsio
 * 			notation
 *
 * @return 0 on success, -1 and set errno on error
 */
int
fits_create (struct fits_receiver_data *receiver, char *filename)
{
  int status;
  char *fn;
  fitsfile *fptr;

  fits_clear_errmsg ();

  receiver->offset = 0;
  receiver->size = 0;
  receiver->header_processed = 0;

  status = 0;
  printf ("create '%s'\n", filename);

  fn = (char *) malloc (strlen (filename) + 2);
  strcpy (fn + 1, filename);
  fn[0] = '!';

  fits_call (fits_create_file (&receiver->ffile, fn, &status));
  fptr = receiver->ffile;
  return 0;
}

#define write_key(type, key, value, comment)\
	if (fits_update_key (fptr, type, key, value, comment, &status)) \
	{ \
		fits_report_error (stderr, status); \
		errno = EINVAL; \
		return -1; \
	}

int
write_camera (struct fits_receiver_data *receiver, struct camera_info *camera)
{
  int status = 0;
  float xplate = 17, yplate = 17;
  float rotang = 95.5;
  char *filter = "R";
  long flip = 1;

  fitsfile *fptr = receiver->ffile;

  write_key (TSTRING, "CAMD_TYPE", camera->type, "Camera type");
  write_key (TSTRING, "CAMD_SERIAL", camera->serial_number,
	     "Camera serial number");
  write_key (TFLOAT, "CAMD_SETPOINT", &camera->temperature_setpoint,
	     "Camera regulation setpoint");
  write_key (TFLOAT, "CAMD_CCD", &camera->ccd_temperature,
	     "Camera CCD temperature");
  write_key (TFLOAT, "CAMD_AIR", &camera->air_temperature,
	     "Camera air temperature");
  write_key (TINT, "CAMD_POWER", &camera->cooling_power,
	     "Camera cooling power");
  write_key (TSTRING, "CAMD_FAN", camera->fan ? "on" : "off",
	     "Camera fan status");

  write_key (TFLOAT, "XPLATE", &xplate, "X plate size");
  write_key (TFLOAT, "YPLATE", &yplate, "Y plate size");
  write_key (TFLOAT, "ROTANG", &rotang, "Field rotation");
  write_key (TSTRING, "FILTER", filter, "Filter used");
  write_key (TLONG, "FLIP", &flip, "Image flip");

  return 0;
}

int
write_telescope (struct fits_receiver_data *receiver,
		 struct telescope_info *telescope)
{
  int status = 0;
  fitsfile *fptr = receiver->ffile;

  write_key (TSTRING, "TELD_TYPE", telescope->type, "Telescope type");
  write_key (TSTRING, "TELD_SERIAL", telescope->serial_number,
	     "Telescope serial number");
  write_key (TDOUBLE, "RASC", &telescope->ra, "Telescope ra");
  write_key (TDOUBLE, "DECL", &telescope->dec, "Telescope dec");
  write_key (TDOUBLE, "TELD_LON", &telescope->longtitude,
	     "Telescope longtitude");
  write_key (TDOUBLE, "TELD_LAT", &telescope->latitude, "Telescope latitude");
  write_key (TDOUBLE, "TELD_SIDE", &telescope->siderealtime,
	     "Telescope sidereailtime");
  write_key (TDOUBLE, "T_LOC", &telescope->localtime, "Telescope localtime");
  return 0;
}

int
fits_write_image_info (struct fits_receiver_data *receiver,
		       struct image_info *info, char *dark_name)
{
  int status = 0;
  double jd;
  fitsfile *fptr = receiver->ffile;

  if (*info->telescope.type)
    {
      write_key (TSTRING, "TEL_NAME", info->telescope_name, "Telescope name");
      write_telescope (receiver, &info->telescope);
    }
  if (*info->camera.type)
    {
      write_key (TSTRING, "CAM_NAME", info->camera_name, "Camera name");
      write_camera (receiver, &info->camera);
    }
  if (dark_name)
    write_key (TSTRING, "DARK", dark_name, "Dark image path");

  write_key (TLONG, "CTIME", &info->exposure_time, "Camera exposure start");
  jd = get_julian_from_timet (&info->exposure_time);
  write_key (TDOUBLE, "JD", &jd, "Camera exposure Julian date");
  write_key (TFLOAT, "EXPOSURE", &info->exposure_length,
	     "Camera exposure time in msec");
  write_key (TINT, "TARGET", &info->target_id, "Target id");
  write_key (TINT, "OBSERVAT", &info->observation_id, "Observation id");
  return 0;
}

#undef write_key

/*!
 * @param expected_size	expected data size, including header
 */
int
fits_init (struct fits_receiver_data *receiver, size_t expected_size)
{
  if (expected_size <= 0)
    {
      errno = EINVAL;
      return -1;
    }
  if (!(receiver->data = malloc (expected_size)))
    {
      errno = ENOMEM;
      return -1;
    }
  receiver->size = expected_size;

  return 0;
}

#undef fits_call

/*!
 * Receive callback function.
 * 
 * @param data		received data
 * @param size		size of receive data
 * @param receiver	holds persistent data specific informations
 *
 * @return 0 if we can continue receiving data, < 0 if we don't like to see more data.
 */
int
fits_handler (void *data, size_t size, struct fits_receiver_data *receiver)
{
#define fits_call(call) if (call) fits_report_error(stderr, status);
  memcpy (&(receiver->data[receiver->offset]), data, size);
  receiver->offset += size;
#ifdef DEBUG
  if (isatty (1))
    {
      printf (".");
    };
#endif /* DEBUG */
  if (receiver->offset > sizeof (struct imghdr))
    {
      if (!(receiver->header_processed))	// we receive full image header
	{
	  int status;
	  status = 0;
#ifdef DEBUG
	  printf ("naxes: %i image size: %li x %li                    \n",
		  ((struct imghdr *) receiver->data)->naxes,
		  ((struct imghdr *) receiver->data)->sizes[0],
		  ((struct imghdr *) receiver->data)->sizes[1]);
#endif /* DEBUG */
	  if (((struct imghdr *) receiver->data)->naxes > 0
	      && ((struct imghdr *) receiver->data)->naxes < 5)
	    {
	      fits_call (fits_create_img
			 (receiver->ffile, USHORT_IMG, 2,
			  ((struct imghdr *) receiver->data)->sizes,
			  &status));
	    }
	  else
	    {
	      printf ("bad naxes: %i\n",
		      ((struct imghdr *) receiver->data)->naxes);
	    }
	  receiver->header_processed = 1;
	}
      if (receiver->offset == receiver->size)
	{
	  int status = 0;
	  fits_clear_errmsg ();
	  fits_call (fits_write_img
		     (receiver->ffile, TUSHORT, 1, receiver->size / 2,
		      ((receiver->data) + sizeof (struct imghdr)), &status));
	  free (receiver->data);
#ifdef DEBUG
	  printf ("readed:%i bytes\n", receiver->offset);
#endif /* DEBUG */
	  return 1;
	}
    }
  return 0;
}

int
fits_close (struct fits_receiver_data *receiver)
{
  int status;
  fits_call (fits_close_file (receiver->ffile, &status));
  return 0;
}

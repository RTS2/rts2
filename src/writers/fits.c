/*! 
 * @file Writer routine for fits image
 *
 * @author petr
 */

#include <string.h>
#include <errno.h>
#include <malloc.h>

#include "fitsio.h"
#include "fits.h"
#include "imghdr.h"

#define fits_call(call) if (call) fits_report_error(stderr, status);

/*!
 * Init fits data.
 *
 * @param receiver 	receiver structure
 * @param expected_size	expected data size, including header
 * @param filename	filename to store result fits, in fitsio
 * 			notation
 *
 * @return 0 on success, -1 and set errno on error
 */
int 
fits_init (struct fits_receiver_data *receiver, size_t expected_size, char *filename)
{
   int status;

   fits_clear_errmsg ();

   receiver->offset = 0;
   receiver->size = 0;

   status = 0;
 
   if (fits_create_file (&(receiver->ffile), filename, &status))
   {
     fits_report_error (stderr, status);
     errno = EINVAL;
     return -1;
   }

   if (expected_size <= 0)
   {
	   errno = EINVAL;
	   return -1;
   }
   if (! (receiver->data = malloc (expected_size)))
   {
	   errno = ENOMEM;
	   return -1;
   }
   receiver->size = expected_size;
   return 0;
}

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
  memcpy (&(receiver->data[receiver->offset]), data, size);
  receiver->offset += size;
  printf ("[] readed: %i bytes of %i\r", receiver->offset, receiver->size);
  fflush (stdout);
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
	  fits_call (fits_create_img
		     (receiver->ffile, USHORT_IMG, 2,
		      ((struct imghdr *) receiver->data)->sizes, &status));
	  receiver->header_processed = 1;
	}
      if (receiver->offset == receiver->size)
	{
	  int status = 0;
	  fits_clear_errmsg ();
	  fits_call (fits_write_img
		     (receiver->ffile, TUSHORT, 1, receiver->size / 2,
		      ((receiver->data) + sizeof (struct imghdr)), &status));
#ifdef DEBUG
	  printf ("readed:%i bytes                        \n",
		  receiver->offset);
#endif /* DEBUG */
	  fits_call (fits_close_file (receiver->ffile, &status));
	}
    }
  return 0;
}

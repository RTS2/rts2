/*! 
 * @file Writer routine for fits image
 *
 * @author petr
 */

#include <string.h>

#include "fitsio.h"
#include "fits.h"
#include "imghdr.h"

#define fits_call(call) if (call) fits_report_error(stderr, status);

/*!
 * Receive callback function.
 * 
 * @param data Received data.
 * @param size Size of receive data.
 * @param attrs Attributes passed during call to readout init.
 *
 * @return 0 if we can continue receiving data, < 0 if we don't like to see more data.
 */

int
fits_handler (void *data, size_t size, void *attrs)
{
  struct fits_receiver_data_t *receiver =
    (struct fits_receiver_data_t *) attrs;
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
#endif /* DEBUG */
		  ((struct imghdr *) receiver->data)->naxes,
		  ((struct imghdr *) receiver->data)->sizes[0],
		  ((struct imghdr *) receiver->data)->sizes[1]);
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

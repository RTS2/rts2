/*!
 * @file Useful to everyone willing to write informations in fits format.
 *
 * @author petr
 */

#ifndef __RTS_FITS__
#define __RTS_FITS__

#include <fitsio.h>

/*! 
 * Structure to hold additional fits informations.
 */ 
struct fits_receiver_data_t
{
  int offset;
  size_t size;
  fitsfile *ffile;
  char *data;
  int header_processed;
  pthread_t thread;
};

int fits_handler (void *data, size_t size, void *attrs);

#endif /* !__RTS_FITS__ */

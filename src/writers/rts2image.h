#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#define IMAGE_SAVE	0x01
#define IMAGE_NOT_SAVE  0x00

#include <sys/time.h>
#include <time.h>
#include <fitsio.h>

#include "imghdr.h"

class Rts2Image
{
private:
  unsigned short *data;
  char *filename;
  fitsfile ffile;
  int fits_status;
  int flags;
public:
  // create image
    Rts2Image (char *in_filename, struct timeval *exposureStart);
  // open image from disk..
    Rts2Image (char *in_filename);
    virtual ~ Rts2Image (void);

  void setValue (char *name, int value, char *comment);
  void setValue (char *name, double value, char *comment);
  void setValue (char *name, char *value, char *comment);

  void writeImgHeader (struct imghdr *im_h);
};

#endif /* !__RTS2_IMAGE__ */

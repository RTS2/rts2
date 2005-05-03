#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#define IMAGE_SAVE	0x01
#define IMAGE_NOT_SAVE  0x00

#include <sys/time.h>
#include <time.h>
#include <fitsio.h>

#include "imghdr.h"
#include "../utils/rts2dataconn.h"

class Rts2Image
{
private:
  unsigned short *data;
  fitsfile *ffile;
  int fits_status;
  int flags;
  int targetId;
  int obsId;
  void createImage (char *filename, const struct timeval *exposureStart);
public:
  // create image
    Rts2Image (char *in_filename, const struct timeval *exposureStart);
    Rts2Image (int epochId, int targetId, int obsId,
	       const struct timeval *exposureStart);
  // open image from disk..
    Rts2Image (char *in_filename);
    virtual ~ Rts2Image (void);

  int setValue (char *name, int value, char *comment);
  int setValue (char *name, long value, char *comment);
  int setValue (char *name, double value, char *comment);
  int setValue (char *name, const char *value, char *comment);

  int writeImgHeader (struct imghdr *im_h);
  int writeDate (Rts2ClientTCPDataConn * dataConn);

  inline int fitsStatusValue (char *valname)
  {
    int ret = 0;
    if (fits_status)
      {
	ret = -1;
	fits_report_error (stdout, fits_status);
      }
    fits_status = 0;
    return ret;
  }
};

#endif /* !__RTS2_IMAGE__ */

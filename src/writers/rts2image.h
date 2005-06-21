#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#define IMAGE_SAVE	0x01
#define IMAGE_NOT_SAVE  0x00

#include <sys/time.h>
#include <time.h>
#include <fitsio.h>

#include "imghdr.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2devclient.h"

class Rts2Image
{
private:
  unsigned short *data;
  fitsfile *ffile;
  int fits_status;
  int flags;
  int epochId;
  int targetId;
  int obsId;
  char *cameraName;
  char *imageName;
  struct timeval exposureStart;
  void setImageName (char *in_filename);
  int createImage (char *in_filename);
  int openImage (char *in_filename);
  int writeExposureStart ();
public:
  // create image
    Rts2Image (char *in_filename, const struct timeval *exposureStart);
  // create image in que
    Rts2Image (int in_epoch_id, int in_targetId, Rts2DevClientCamera * camera,
	       int in_obsId, const struct timeval *exposureStart);
  // open image from disk..
    Rts2Image (char *in_filename);
    virtual ~ Rts2Image (void);

  int toQue ();
  int toAcquisition ();
  int toArchive ();
  int toTrash ();

  int renameImage (char *new_filename);

  int setValue (char *name, int value, char *comment);
  int setValue (char *name, long value, char *comment);
  int setValue (char *name, double value, char *comment);
  int setValue (char *name, const char *value, char *comment);

  int getValue (char *name, int &value, char *comment = NULL);
  int getValue (char *name, long &value, char *comment = NULL);
  int getValue (char *name, double &value, char *comment = NULL);
  int getValue (char *name, char *value, char *comment = NULL);

  int writeImgHeader (struct imghdr *im_h);
  int writeDate (Rts2ClientTCPDataConn * dataConn);

  inline int fitsStatusValue (char *valname)
  {
    int ret = 0;
    if (fits_status)
      {
	ret = -1;
	fprintf (stderr, "error when setting value '%s'\n", valname);
	fits_report_error (stderr, fits_status);
      }
    fits_status = 0;
    return ret;
  }

  int saveImage ();

  char *getImageName ()
  {
    return imageName;
  }
};

#endif /* !__RTS2_IMAGE__ */

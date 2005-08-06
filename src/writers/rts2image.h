#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#define IMAGE_SAVE	0x01
#define IMAGE_NOT_SAVE  0x00
#define IMAGE_KEEP_DATA 0x02

#include <sys/time.h>
#include <time.h>
#include <fitsio.h>

#include "imghdr.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2devclient.h"
#include "../utils/mkpath.h"

typedef enum
{ IMGTYPE_UNKNOW, IMGTYPE_DARK, IMGTYPE_FLAT, IMGTYPE_OBJECT, IMGTYPE_ZERO,
  IMGTYPE_COMP
} img_type_t;

class Rts2Image
{
private:
  unsigned short *data;
  fitsfile *ffile;
  int fits_status;
  int flags;
  int filter;
  struct timeval exposureStart;
  void setImageName (const char *in_filename);
  int createImage (char *in_filename);
  int openImage (const char *in_filename);
  int writeExposureStart ();
  char *getImageBase (int in_epoch_id);
  unsigned short *imageData;
protected:
  int epochId;
  int targetId;
  int obsId;
  int imgId;
  char *cameraName;
  char *mountName;
  char *focName;
  char *imageName;
  img_type_t imageType;
  double mean;
public:
  // create image
    Rts2Image (char *in_filename, const struct timeval *exposureStart);
  // create image in que
    Rts2Image (int in_epoch_id, int in_targetId, Rts2DevClientCamera * camera,
	       int in_obsId, const struct timeval *exposureStart,
	       int in_imgId);
  // open image from disk..
    Rts2Image (const char *in_filename);
    virtual ~ Rts2Image (void);

  virtual int toQue ();
  virtual int toAcquisition ();
  virtual int toArchive ();
  virtual int toDark ();
  virtual int toFlat ();
  virtual int toTrash ();

  img_type_t getType ()
  {
    return imageType;
  }

  int renameImage (char *new_filename);

  int setValue (char *name, int value, char *comment);
  int setValue (char *name, long value, char *comment);
  int setValue (char *name, double value, char *comment);
  int setValue (char *name, const char *value, char *comment);
  int setValueImageType (int shutter_state);

  int getValue (char *name, int &value, char *comment = NULL);
  int getValue (char *name, long &value, char *comment = NULL);
  int getValue (char *name, double &value, char *comment = NULL);
  int getValue (char *name, char *value, char *comment = NULL);
  int getValueImageType ();

  int getValues (char *name, int *values, int num, int nstart = 1);
  int getValues (char *name, long *values, int num, int nstart = 1);
  int getValues (char *name, double *values, int num, int nstart = 1);
  int getValues (char *name, char **values, int num, int nstart = 1);

  int writeImgHeader (struct imghdr *im_h);
  int writeDate (Rts2ClientTCPDataConn * dataConn);

  int fitsStatusValue (char *valname);

  virtual int saveImage ();
  virtual int deleteImage ();

  char *getImageName ()
  {
    return imageName;
  }

  void setMountName (const char *in_mountName);

  const char *getCameraName ()
  {
    if (cameraName)
      return cameraName;
    return "(null)";
  }

  const char *getMountName ()
  {
    if (mountName)
      return mountName;
    return "(null)";
  }

  void setFocuserName (const char *in_focuserName);

  const char *getFocuserName ()
  {
    if (focName)
      return focName;
    return "(null)";
  }

  long getExposureSec ()
  {
    return exposureStart.tv_sec;
  }

  long getExposureUsec ()
  {
    return exposureStart.tv_usec;
  }

  int getTargetId ()
  {
    return targetId;
  }

  int getObsId ()
  {
    return obsId;
  }

  int getImgId ()
  {
    return imgId;
  }

  int getFilter ()
  {
    return filter;
  }

  double getMean ()
  {
    return mean;
  }

  void keepImage ()
  {
    flags |= IMAGE_KEEP_DATA;
  }

  unsigned short *getDataUShortInt ();
  int substractDark (Rts2Image * darkImage);
};

#endif /* !__RTS2_IMAGE__ */

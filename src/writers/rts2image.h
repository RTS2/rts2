#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#define IMAGE_SAVE	0x01
#define IMAGE_NOT_SAVE  0x00
#define IMAGE_KEEP_DATA 0x02

#include <sys/time.h>
#include <time.h>
#include <fitsio.h>
#include <list>

#include "imghdr.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2devclient.h"
#include "../utils/mkpath.h"
#include "../utilsdb/target.h"

struct stardata
{
  double X, Y, F, Fe, fwhm;
  int flags;
};

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
  // when in_filename == NULL, we take image name stored in this->imageName
  int openImage (const char *in_filename = NULL);
  int writeExposureStart ();
  char *getImageBase (int in_epoch_id);
  unsigned short *imageData;
  int focPos;
  // we assume that image is 2D
  long naxis[2];
  float signalNoise;
  int getFailed;
  double average;
  short int min;
  short int max;
  short int mean;
  int *histogram;

  void initData ();
protected:
  int epochId;
  int targetId;
  int targetIdSel;
  char targetType;
  char *targetName;
  int obsId;
  int imgId;
  char *cameraName;
  char *mountName;
  char *focName;
  char *imageName;
  img_type_t imageType;

  virtual int isGoodForFwhm (struct stardata *sr);
public:
  // list of sex results..
  struct stardata *sexResults;
  int sexResultNum;

  // memory-only image..
    Rts2Image ();
  // create image
    Rts2Image (char *in_filename, const struct timeval *exposureStart);
  // create image in que
    Rts2Image (Target * currTarget, Rts2DevClientCamera * camera,
	       const struct timeval *exposureStart);
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
  int setValue (char *name, char value, char *comment);
  int setValue (char *name, const char *value, char *comment);
  int setValueImageType (int shutter_state);

  int getValue (char *name, int &value, char *comment = NULL);
  int getValue (char *name, long &value, char *comment = NULL);
  int getValue (char *name, float &value, char *comment = NULL);
  int getValue (char *name, double &value, char *comment = NULL);
  int getValue (char *name, char &value, char *command = NULL);
  int getValue (char *name, char *value, int valLen, char *comment = NULL);
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

  int getTargetIdSel ()
  {
    return targetIdSel;
  }

  char getTargetType ()
  {
    return targetType;
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

  void computeStatistics ();

  double getAverage ()
  {
    return average;
  }

  int getFocPos ()
  {
    return focPos;
  }

  void setFocPos (int new_pos)
  {
    focPos = new_pos;
    setValue ("FOC_POS", focPos, "focuser position");
  }

  void keepImage ()
  {
    flags |= IMAGE_KEEP_DATA;
  }

  void closeData ()
  {
    if (imageData)
      delete imageData;
    imageData = NULL;
  }

  unsigned short *getDataUShortInt ();
  int substractDark (Rts2Image * darkImage);

  int setAstroResults (double ra, double dec, double ra_err, double dec_err);

  int addStarData (struct stardata *sr);
  double getFWHM ();

  double getPrecision ()
  {
    double val;
    int ret;
    ret = getValue ("POS_ERR", val);
    if (ret)
      return nan ("f");
    return val;
  }
  // assumes, that on image is displayed strong light source,
  // it find position of it's center
  // return 0 if center was found , -1 in case of error.
  // Put center coordinates in pixels to x and y

  // helpers, get row & line center
  int getCenterRow (long row, int row_width, double &x);
  int getCenterCol (long col, int col_width, double &y);

  int getCenter (double &x, double &y, int bin);

  long getWidth ()
  {
    return naxis[0];
  }

  long getHeight ()
  {
    return naxis[1];
  }

  // returns ra & dec distance in degrees of pixel [x,y] from device axis
  // I think it's better to use double precission even in pixel coordinates, as I'm then sure I'll not 
  // loose precision somewhere in calculation
  int getOffset (double x, double y, double &chng_ra, double &chng_dec,
		 double &sep_angle);

  // returns pixels [x1,y1] and [x2,y2] offset in ra and dec degrees
  int getOffset (double x1, double y1, double x2, double y2, double &chng_ra,
		 double &chng_dec, double &sep_angle);

  // this function is good only for HAM source detection on FRAM telescope in Argentina.
  // HAM is calibration source, which is used test target for photometer to measure it's sesitivity 
  // (and other things, such as athmospheric dispersion..).
  // You most probably don't need it.
  int getHam (double &x, double &y);

  int getRaDec (double x, double y, double &ra, double &dec);

  // get xoa and yoa coeficients
  double getXoA ();
  double getYoA ();

  // get rotang - get value from WCS when available; ROTANG is in radians, and is true rotang of image
  // (assumig top is N, left is E - e.g. is corrected for telescope flip)
  double getRotang ();

  double getCenterRa ();
  double getCenterDec ();

  // get xplate and yplate coeficients - in degrees!
  double getXPlate ();
  double getYPlate ();

  // mnt flip value
  int getMountFlip ();

  // image flip value - ussually 1
  int getFlip ();
};

#endif /* !__RTS2_IMAGE__ */

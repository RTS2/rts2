#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#define IMAGE_SAVE		0x01
#define IMAGE_NOT_SAVE  	0x00
#define IMAGE_KEEP_DATA 	0x02
#define IMAGE_DONT_DELETE_DATA	0x04

#include <sys/time.h>
#include <time.h>
#include <fitsio.h>
#include <list>
#include <ostream>
#include <vector>
#include <config.h>

#include "imghdr.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2devclient.h"
#include "../utils/mkpath.h"
#include "../utils/rts2target.h"

struct pixel
{
  int x;
  int y;
  unsigned short value;
};

struct stardata
{
  double X, Y, F, Fe, fwhm;
  int flags;
};

typedef enum
{ IMGTYPE_UNKNOW, IMGTYPE_DARK, IMGTYPE_FLAT, IMGTYPE_OBJECT, IMGTYPE_ZERO,
  IMGTYPE_COMP
} img_type_t;

typedef enum
{ SHUT_UNKNOW, SHUT_OPENED, SHUT_CLOSED, SHUT_SYNCHRO } shutter_t;

class Rts2Image
{
private:
  unsigned short *data;
  fitsfile *ffile;
  int fits_status;
  int flags;
  int filter_i;
  char *filter;
  struct timeval exposureStart;
  struct tm exposureGmTime;
  float exposureLength;
  void setImageName (const char *in_filename);
  int createImage (std::string in_filename);
  int createImage (char *in_filename);
  // when in_filename == NULL, we take image name stored in this->imageName
  int openImage ();
  int openImage (const char *in_filename);
  int writeExposureStart ();
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
  int isAcquiring;
  // that value is nan when rotang was already set; it's same double when rotang was set (by camera)
  // and become nan once we save image (so mnt_flip can be applied properly).
  double config_rotang;

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
  shutter_t shutter;

  struct ln_equ_posn pos_astr;
  double ra_err;
  double dec_err;
  double img_err;

  virtual int isGoodForFwhm (struct stardata *sr);
  char *getImageBase (void);

  void setFitsFile (fitsfile * in_ffile)
  {
    ffile = in_ffile;
  }

  std::string getFitsErrors ();
public:
  // list of sex results..
  struct stardata *sexResults;
  int sexResultNum;

  // memory-only image..
  Rts2Image ();
  // copy constructor
  Rts2Image (Rts2Image * in_image);
  // memory-only with exposure time
  Rts2Image (const struct timeval *in_exposureStart);
  // skeleton for DB image
  Rts2Image (long in_img_date, int in_img_usec, float in_img_exposure);
  // create image
  Rts2Image (char *in_filename, const struct timeval *in_exposureStart);
  // create image in que
  Rts2Image (Rts2Target * currTarget, Rts2DevClientCamera * camera,
	     const struct timeval *in_exposureStart);
  // open image from disk..
  Rts2Image (const char *in_filename);
  virtual ~ Rts2Image (void);

  // expand expression to image path
  virtual std::string expandPath (std::string expression);

  virtual int toQue ();
  virtual int toAcquisition ();
  virtual int toArchive ();
  virtual int toDark ();
  virtual int toFlat ();
  virtual int toMasterFlat ();
  virtual int toTrash ();

  virtual img_type_t getImageType ()
  {
    return IMGTYPE_UNKNOW;
  }

  shutter_t getShutter ()
  {
    return shutter;
  }

  int renameImage (const char *new_filename);
  int renameImageExpand (std::string new_ex);
  int copyImage (const char *copy_filename);
  int copyImageExpand (std::string copy_ex);

  int setValue (char *name, int value, char *comment);
  int setValue (char *name, long value, char *comment);
  int setValue (char *name, float value, char *comment);
  int setValue (char *name, double value, char *comment);
  int setValue (char *name, char value, char *comment);
  int setValue (char *name, const char *value, char *comment);
  int setValue (char *name, time_t * sec, long usec, char *comment);
  // that method is used to update DATE - creation date entry - for other file then ffile
  int setCreationDate (fitsfile * out_file = NULL);

  int getValue (char *name, int &value, bool required = false, char *comment =
		NULL);
  int getValue (char *name, long &value, bool required =
		false, char *comment = NULL);
  int getValue (char *name, float &value, bool required =
		false, char *comment = NULL);
  int getValue (char *name, double &value, bool required =
		false, char *comment = NULL);
  int getValue (char *name, char &value, bool required =
		false, char *command = NULL);
  int getValue (char *name, char *value, int valLen, bool required =
		false, char *comment = NULL);

  int getValues (char *name, int *values, int num, bool required =
		 false, int nstart = 1);
  int getValues (char *name, long *values, int num, bool required =
		 false, int nstart = 1);
  int getValues (char *name, double *values, int num, bool required =
		 false, int nstart = 1);
  int getValues (char *name, char **values, int num, bool required =
		 false, int nstart = 1);

  int writeImgHeader (struct imghdr *im_h);
  int writeDate (Rts2ClientTCPDataConn * dataConn);

  int fitsStatusValue (char *valname, const char *operation, bool required);
  int fitsStatusSetValue (char *valname, bool required = true)
  {
    return fitsStatusValue (valname, "SetValue", required);
  }
  int fitsStatusGetValue (char *valname, bool required)
  {
    return fitsStatusValue (valname, "GetValue", required);
  }

  double getAstrometryErr ();

  int closeFile ();
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

  void setExposureStart (const struct timeval *in_exposureStart)
  {
    setExposureStart (&in_exposureStart->tv_sec, in_exposureStart->tv_usec);
  }

  void setExposureStart (const time_t * in_sec, long in_usec)
  {
    exposureStart.tv_sec = *in_sec;
    exposureStart.tv_usec = in_usec;
    setExposureStart ();
  }

  void setExposureStart ()
  {
    gmtime_r (&exposureStart.tv_sec, &exposureGmTime);
  }

  long getExposureSec ()
  {
    return exposureStart.tv_sec;
  }

  long getExposureUsec ()
  {
    return exposureStart.tv_usec;
  }

  void setExposureLength (float in_exposureLength)
  {
    exposureLength = in_exposureLength;
    setValue ("EXPOSURE", exposureLength, "exposure time");
  }

  float getExposureLength ()
  {
    return exposureLength;
  }

  int getTargetId ()
  {
    return targetId;
  }

  std::string getEpochString ();
  std::string getTargetString ();
  std::string getTargetSelString ();
  std::string getObsString ();
  std::string getImgIdString ();

  virtual std::string getProcessingString ();

  // date related functions
  std::string getStartYearString ();
  std::string getStartMonthString ();
  std::string getStartDayString ();
  std::string getStartYDayString ();

  std::string getStartHourString ();
  std::string getStartMinString ();
  std::string getStartSecString ();
  std::string getStartMSecString ();

  // image parameter functions
  std::string getExposureLengthString ();

  int getStartYear ()
  {
    return exposureGmTime.tm_year + 1900;
  }

  int getStartMonth ()
  {
    return exposureGmTime.tm_mon + 1;
  }

  int getStartDay ()
  {
    return exposureGmTime.tm_mday;
  }

  int getStartYDay ()
  {
    return exposureGmTime.tm_yday;
  }

  int getStartHour ()
  {
    return exposureGmTime.tm_hour;
  }

  int getStartMin ()
  {
    return exposureGmTime.tm_min;
  }

  int getStartSec ()
  {
    return exposureGmTime.tm_sec;
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

  const char *getFilter ()
  {
    return filter;
  }

  void setFilter (const char *in_filter);

  int getFilterNum ()
  {
    return filter_i;
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
  }

  int getIsAcquiring ()
  {
    return isAcquiring;
  }

  void keepImage ()
  {
    flags |= IMAGE_KEEP_DATA;
  }

  bool shouldSaveImage ()
  {
    return (flags & IMAGE_SAVE);
  }

  void closeData ()
  {
    if (imageData)
      delete imageData;
    imageData = NULL;
  }

  unsigned short *getDataUShortInt ();
  void setDataUShortInt (unsigned short *in_data, long in_naxis[2]);

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
  /**
   * Returns ra & dec distance in degrees of pixel [x,y] from device axis (XOA and YOA coordinates)
   *
   * Hint: if you want to guide telescope, you need to use that value.
   * If target is W from telescope axis, it will be negative, and you need to move to W, so you need
   * to decrease RA.
   */

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

  // return offset & flux of the brightest star in field
  int getBrightestOffset (double &x, double &y, float &flux);

  int getRaDec (double x, double y, double &ra, double &dec);

  // get xoa and yoa coeficients
  double getXoA ();
  double getYoA ();

  // get rotang - get value from WCS when available; ROTANG is in radians, and is true rotang of image
  // (assumig top is N, left is E - e.g. is corrected for telescope flip)
  // it's WCS style - counterclockwise
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

  int getError (double &eRa, double &eDec, double &eRad);

  void setConfigRotang (double in_config_rotang)
  {
    config_rotang = in_config_rotang;
  }

  int getCoord (LibnovaRaDec & radec, char *ra_name, char *dec_name);
  int getCoordTarget (LibnovaRaDec & radec);
  int getCoordAstrometry (LibnovaRaDec & radec);
  int getCoordMount (LibnovaRaDec & radec);

  std::string getOnlyFileName ();

  virtual bool haveOKAstrometry ()
  {
    return false;
  }

  virtual bool isProcessed ()
  {
    return false;
  }

  virtual void printFileName (std::ostream & _os);

  virtual void print (std::ostream & _os, int in_flags = 0);

  /**
   * Get fits file for use by other image.
   *
   * File pointer will be discarded and will not be closed - usefull for copy
   * constructor, but for nothing else.
   */
  fitsfile *getFitsFile ()
  {
    fitsfile *ret = ffile;
    ffile = NULL;
    return ret;
  }

  int setInstrument (char *instr)
  {
    return setValue ("INSTRUME", instr, "instrument used for acqusition");
  }

  int setTelescope (char *tel)
  {
    return setValue ("TELESCOP", tel, "telescope used for acqusition");
  }

  int setObserver ()
  {
    return setValue ("OBSERVER", "RTS2 " VERSION, "observer");
  }

  int setOrigin (char *orig)
  {
    return setValue ("ORIGIN", orig, "organisation responsible for data");
  }

  void writeClient (Rts2DevClient * client);

  friend std::ostream & operator << (std::ostream & _os, Rts2Image & image);
  // image processing routines and values
  double classicMedian (double *q, int n, double *sigma);
  int findMaxIntensity (unsigned short *in_data, struct pixel *ret);
  unsigned short getPixel (unsigned short *data, int x, int y);
  int findStar (unsigned short *data);
  int aperture (unsigned short *data, struct pixel pix, struct pixel *ret);
  int centroid (unsigned short *data, struct pixel pix, float *px, float *py);
  int radius (unsigned short *data, double px, double py, int rmax);
  int integrate (unsigned short *data, double px, double py, int size,
		 float *ret);
  int evalAF (float *result, float *error);

  std::vector < pixel > list;
  double median, sigma;
};

std::ostream & operator << (std::ostream & _os, Rts2Image & image);

//Rts2Image & operator - (Rts2Image & img_1, Rts2Image & img_2);
//Rts2Image & operator + (Rts2Image & img_, Rts2Image & img_2);
//Rts2Image & operator -= (Rts2Image & img_2);
//Rts2Image & operator += (Rts2Image & img_2);

#endif /* !__RTS2_IMAGE__ */

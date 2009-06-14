/* 
 * Class which represents image.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __RTS2_IMAGE__
#define __RTS2_IMAGE__

#include <list>
#include <ostream>
#include <vector>
#include <config.h>

#include "imghdr.h"

#include "rts2fitsfile.h"

#include "../utils/libnova_cpp.h"
#include "../utils/rts2devclient.h"
#include "../utils/expander.h"
#include "../utils/rts2target.h"

/**
 * One pixel at the image, with coordinates and a value.
 */
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
{
	IMGTYPE_UNKNOW, IMGTYPE_DARK, IMGTYPE_FLAT, IMGTYPE_OBJECT, IMGTYPE_ZERO,
	IMGTYPE_COMP
} img_type_t;

typedef enum
{ SHUT_UNKNOW, SHUT_OPENED, SHUT_CLOSED }
shutter_t;

typedef enum
{ EXPOSURE_START, INFO_CALLED, EXPOSURE_END }
imageWriteWhich_t;

/**
 * Generic class which represents an image.
 *
 * This class represents image. It has functions for accessing various image
 * properties, as well as method to perform some image operations.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Image:public Rts2FitsFile
{
	private:
		int filter_i;
		char *filter;
		float exposureLength;
		
		int createImage ();
		int createImage (std::string in_filename);
		int createImage (char *in_filename);
		// if filename is NULL, will take name stored in this->getFileName ()
		int openImage (const char *_filename = NULL, bool readOnly = false);

		int writeExposureStart ();
		char *imageData;
		int imageType;
		int focPos;
		// we assume that image is 2D
		long naxis[2];
		float signalNoise;
		int getFailed;
		double average;
		double stdev;
		double bg_stdev;
		short int min;
		short int max;
		short int mean;
		int isAcquiring;
		// that value is nan when rotang was already set;
		// it is calculated as sum of partial rotangs.
		// For change of total rotang, addRotang function is provided.
		double total_rotang;

		double xoa;
		double yoa;

		int mnt_flip;

		int expNum;

		void initData ();

		void writeConnBaseValue (const char *name, Rts2Value * val, const char *desc);

		// writes one value to image
		void writeConnValue (Rts2Conn * conn, Rts2Value * val);

		// record value changes
		void recordChange (Rts2Conn * conn, Rts2Value * val);
	protected:
		int targetId;
		int targetIdSel;
		char targetType;
		char *targetName;
		int obsId;
		int imgId;
		char *cameraName;
		char *mountName;
		char *focName;
		shutter_t shutter;

		struct ln_equ_posn pos_astr;
		double ra_err;
		double dec_err;
		double img_err;

		virtual int isGoodForFwhm (struct stardata *sr);
		char *getImageBase (void);

		// expand expression to image path
		virtual std::string expandVariable (char expression);
		virtual std::string expandVariable (std::string expression);

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
		Rts2Image (const struct timeval *in_exposureS, float in_img_exposure);
		// skeleton for DB image
		Rts2Image (long in_img_date, int in_img_usec, float in_img_exposure);
		// create image
		Rts2Image (char *in_filename, const struct timeval *in_exposureStart);
		/**
		 * Create image from expand path.
		 *
		 * @param in_expression     Expresion containing characters which will be expanded.
		 * @param in_expNum         Exposure number.
		 * @param in_exposureStart  Starting time of the exposure.
		 * @param in_connection     Connection of camera requesting exposure.
		 */
		Rts2Image (const char *in_expression, int in_expNum, const struct timeval *in_exposureStart,
			Rts2Conn * in_connection);
		// create image in que
		Rts2Image (Rts2Target * currTarget, Rts2DevClientCamera * camera,
			const struct timeval *in_exposureStart);
		// open image from disk..
		Rts2Image (const char *in_filename, bool verbose = true, bool readOnly = false);
		virtual ~ Rts2Image (void);

		virtual int closeFile ();

		virtual int toQue ();
		virtual int toAcquisition ();
		virtual int toArchive ();
		virtual int toDark ();
		virtual int toFlat ();
		virtual int toMasterFlat ();
		virtual int toTrash ();

		virtual img_type_t getImageType ();

		shutter_t getShutter ()
		{
			return shutter;
		}

		/**
		 * Rename images to new path.
		 */
		virtual int renameImage (const char *new_filename);
		int renameImageExpand (std::string new_ex);

		/**
		 * Copy image to given given location.
		 *
		 * @param copy_filename  Path where image will be copied.
		 *
		 * @return 0 on success, otherwise returns error code.
		 */
		int copyImage (const char *copy_filename);

		/**
		 * Copy image to given given location. The location is specified as expansion string.
		 *
		 * @param copy_filename  Path where image will be copied.
		 *
		 * @return 0 on success, otherwise returns system error code.
		 */
		int copyImageExpand (std::string copy_ex);

		/**
		 * Create softlink of the image.
		 *
		 * @param link_name  Pathe where link will be created.
		 * 
		 * @return 0 on success, otherwise returns system error code.
		 */
		int symlinkImage (const char *link_name);

		/**
		 * Create softlink of the image. The location is specified as expansion string.
		 *
		 * @param link_ex   Path where image link will be created. Any 
		 * 
		 * @return 0 on success, otherwise returns system errror code.
		 */
		int symlinkImageExpand (std::string link_ex);


		/**
		 * Create hardlink of the image.
		 *
		 * @param link_name  Pathe where link will be created.
		 * 
		 * @return 0 on success, otherwise returns system error code.
		 */
		int linkImage (const char *link_name);

		/**
		 * Create hardlink of the image. The location is specified as expansion string.
		 *
		 * @param link_ex   Path where image link will be created. Any 
		 * 
		 * @return 0 on success, otherwise returns system errror code.
		 */
		int linkImageExpand (std::string link_ex);

		int saveImageData (const char *save_filename, unsigned short *in_data);

		int setValue (const char *name, bool value, const char *comment);
		int setValue (const char *name, int value, const char *comment);
		int setValue (const char *name, long value, const char *comment);
		int setValue (const char *name, float value, const char *comment);
		int setValue (const char *name, double value, const char *comment);
		int setValue (const char *name, char value, const char *comment);
		int setValue (const char *name, const char *value, const char *comment);
		int setValue (const char *name, time_t * sec, long usec, const char *comment);
		// that method is used to update DATE - creation date entry - for other file then ffile
		int setCreationDate (fitsfile * out_file = NULL);

		int getValue (const char *name, bool & value, bool required = false, char *comment = NULL);
		int getValue (const char *name, int &value, bool required = false, char *comment = NULL);
		int getValue (const char *name, long &value, bool required = false, char *comment = NULL);
		int getValue (const char *name, float &value, bool required = false, char *comment = NULL);
		int getValue (const char *name, double &value, bool required = false, char *comment = NULL);
		int getValue (const char *name, char &value, bool required = false, char *command = NULL);
		int getValue (const char *name, char *value, int valLen, bool required = false, char *comment = NULL);
		int getValue (const char *name, char **value, int valLen, bool required = false, char *comment = NULL);

		/**
		 * Get double value from image.
		 *
		 * @param name Value name.
		 * @return Value
		 * @throw KeyNotFound
		 */
		double getValue (const char *name);

		int getValues (const char *name, int *values, int num, bool required = false, int nstart = 1);
		int getValues (const char *name, long *values, int num, bool required = false, int nstart = 1);
		int getValues (const char *name, double *values, int num, bool required = false, int nstart = 1);
		int getValues (const char *name, char **values, int num, bool required = false, int nstart = 1);

		int writeImgHeader (struct imghdr *im_h);

		/**
		 * Record image physical coordinates.
		 *
		 * @param x     X offset of the image.
		 * @param y     Y offset of the image.
		 * @param bin_x Binning along X axis.
		 * @param bin_y Binning along Y axis.
		 */
		void writePhysical (int x, int y, int bin_x, int bin_y);

		int writeData (char *in_data, char *fullTop);

		/**
		 * Build image histogram.
		 *
		 * @param histogram Array of size nbins.
		 * @param nbins     Number of histogram bins.
		 */
		void getHistogram (long *histogram, long nbins);

#ifdef HAVE_LIBJPEG
		/**
		 * Write image as JPEG to provided data buffer.
		 * Buffer will be allocated by this call and should
		 * be free afterwards.
		 *
		 * @param expand_str Expand string for image name
		 * @param quantiles  Quantiles in 0-1 range for image scaling
		 * @param quality    Image quality
		 *
		 */
		int writeAsJPEG (std::string expand_str, float quantiles=0.005, int quality = 100);
#endif

		double getAstrometryErr ();

		virtual int saveImage ();
		virtual int deleteImage ();


		void setMountName (const char *in_mountName);

		void setCameraName (const char *new_name);

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

		void setExposureStart (const struct timeval *tv)
		{
			setExpandDate (tv);
		}

		double getExposureStart ()
		{
			return getExpandDateCtime ();
		}

		long getExposureSec ()
		{
			return getCtimeSec ();
		}

		long getExposureUsec ()
		{
			return getCtimeUsec ();
		}

		void setExposureLength (float in_exposureLength)
		{
			exposureLength = in_exposureLength;
			setValue ("EXPOSURE", exposureLength, "exposure length in seconds");
			setValue ("EXPTIME", exposureLength, "exposure length in seconds");
		}

		float getExposureLength ()
		{
			return exposureLength;
		}

		int getTargetId ()
		{
			return targetId;
		}

		std::string getTargetString ();
		std::string getTargetSelString ();

		std::string getExposureNumberString ();
		std::string getObsString ();
		std::string getImgIdString ();

		virtual std::string getProcessingString ();

		// image parameter functions
		std::string getExposureLengthString ();

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

		double getStdDev ()
		{
			return stdev;
		}

		/**
		 * Return
		 */
		double getBgStdDev ()
		{
			return bg_stdev;
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

		void closeData ()
		{
			delete imageData;
			imageData = NULL;
		}

		int loadData ();

		void *getData ();
		unsigned short *getDataUShortInt ();

		int getPixelByteSize ()
		{
			if (imageType == RTS2_DATA_ULONG)
				return 4;
			return abs (imageType) / 8;
		}

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

		/**
		 * Returns number of pixels.
		 */
		long getNPixels ()
		{
			return getWidth () * getHeight ();
		}

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

		/**
		 * This function is good only for HAM source detection on FRAM telescope in Argentina.
		 * HAM is calibration source, which is used test target for photometer to measure it's sesitivity
		 * (and other things, such as athmospheric dispersion..).
		 * You most probably don't need it.
		 */
		int getHam (double &x, double &y);

		// return offset & flux of the brightest star in field
		int getBrightestOffset (double &x, double &y, float &flux);

		int getRaDec (double x, double y, double &ra, double &dec);

		// get xoa and yoa coeficients
		double getXoA ();
		double getYoA ();

		void setXoA (double in_xoa);
		void setYoA (double in_yoa);

		// get rotang - get value from WCS when available; ROTANG is in radians, and is true rotang of image
		// (assumig top is N, left is E - e.g. is corrected for telescope flip)
		// it's WCS style - counterclockwise
		double getRotang ();

		double getCenterRa ();
		double getCenterDec ();

		// get xplate and yplate coeficients - in degrees!
		double getXPlate ();
		double getYPlate ();

		/**
		 * Set mount flip value.
		 *
		 * @param in_mnt_flip New mount flip value (0 or 1).
		 */
		void setMountFlip (int in_mnt_flip)
		{
			mnt_flip = in_mnt_flip;
		}

		/**
		 * Increase image rotang.
		 */
		void addRotang (double rotAdd)
		{
			if (isnan (total_rotang))
				total_rotang = rotAdd;
			else
				total_rotang += rotAdd;
		}

		/**
		 * Return image mount flip.
		 *
		 * @return Image mount flip.
		 */
		int getMountFlip ();

		// image flip value - ussually 1
		int getFlip ();

		int getError (double &eRa, double &eDec, double &eRad);

		/**
		 * Returns coordinates, which are stored in two FITS keys, as struct ln_equ_posn.
		 *
		 * @param radec    Struct which will hold resulting coordinates.
		 * @param ra_name  Name of the RA coordinate key.
		 * @param dec_name Name of the DEC coordinate key.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int getCoord (struct ln_equ_posn &radec, const char *ra_name, const char *dec_name);

		/**
		 * Return libnova RA DEC with coordinates.
		 *
		 * @param prefix String prefixin RADEC coordinate. Suffix is RA and DEC.
		 *
		 * @return LibnovaRaDec object with coordinates.
		 *
		 * @throw KeyNotFound
		 */
		LibnovaRaDec getCoord (const char *prefix);

		/**
		 * Get object coordinates. Object coordinates are
		 * J2000 coordinates of object which observer would like to
		 * observe.
		 */
		int getCoordObject (struct ln_equ_posn &radec);
		int getCoordTarget (struct ln_equ_posn &radec);
		int getCoordAstrometry (struct ln_equ_posn &radec);
		int getCoordMount (struct ln_equ_posn &radec);

		int getCoordBest (struct ln_equ_posn &radec);

		int getCoord (LibnovaRaDec & radec, const char *ra_name, const char *dec_name);
		int getCoordTarget (LibnovaRaDec & radec);
		int getCoordAstrometry (LibnovaRaDec & radec);
		int getCoordMount (LibnovaRaDec & radec);

		/**
		 * Retrieves from FITS headers best target coordinates.
		 *
		 * Those are taken from RASC and DECL keywords, which are filled by
		 * astrometry value, if astrometry was processed sucessfully from this image,
		 * and target values corrected by any know offsets if astrometry is not know
		 * or invalid.
		 */
		int getCoordBest (LibnovaRaDec & radec);

		std::string getOnlyFileName ();

		virtual bool haveOKAstrometry ()
		{
			return false;
		}

		virtual bool isProcessed ()
		{
			return false;
		}

		void printFileName (std::ostream & _os);

		virtual void print (std::ostream & _os, int in_flags = 0);

		int setInstrument (const char *instr)
		{
			return setValue ("INSTRUME", instr, "instrument used for acqusition");
		}

		int setTelescope (const char *tel)
		{
			return setValue ("TELESCOP", tel, "telescope used for acqusition");
		}

		int setObserver ()
		{
			return setValue ("OBSERVER", "RTS2 " VERSION, "observer");
		}

		int setOrigin (const char *orig)
		{
			return setValue ("ORIGIN", orig, "organisation responsible for data");
		}

		/**
		 * Save environmental variables, as specified by environmental config entry.
		 */
		void setEnvironmentalValues ();

		void writeConn (Rts2Conn * conn, imageWriteWhich_t which =
			EXPOSURE_START);

		/**
		 * This will create WCS from record available at the FITS file.
		 *
		 * @param x_off  offset of the X coordinate (most probably computed from bright star location)
		 * @param y_off  offset of the Y coordinate (most probably computed from bright star location)
		 *
		 * @return 0 on success, -1 on error.
		 */
		int createWCS (double x_off = 0, double y_off = 0);

		friend std::ostream & operator << (std::ostream & _os, Rts2Image & image);

		double getLongtitude ();

		/**
		 * Gets julian data of exposure start
		 */
		double getExposureJD ();

		/**
		 * Get image LST (local sidereal time).
		 */
		double getExposureLST ();

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

namespace rts2image
{

/**
 * Thrown where we cannot find header in the image.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class KeyNotFound
{
	private:
		const char *fileName;
		const char *header;
	public:
		KeyNotFound (Rts2Image *_image, const char *_header)
		{
			header = _header;
			fileName = _image->getFileName ();
		}
		
		friend std::ostream & operator << (std::ostream &_os, KeyNotFound &_err)
		{
			_os << "keyword " << _err.header << " missing in fits file " << _err.fileName;
			return _os;
		}
};

};

//Rts2Image & operator - (Rts2Image & img_1, Rts2Image & img_2);
//Rts2Image & operator + (Rts2Image & img_, Rts2Image & img_2);
//Rts2Image & operator -= (Rts2Image & img_2);
//Rts2Image & operator += (Rts2Image & img_2);
#endif							 /* !__RTS2_IMAGE__ */

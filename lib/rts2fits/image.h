/* 
 * Class which represents image.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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
#include <fitsio.h>

#include "imghdr.h"

#include "fitsfile.h"
#include "channel.h"

#include "libnova_cpp.h"
#include "rts2devclient.h"
#include "../rts2/expander.h"
#include "../rts2/rts2target.h"

#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1
#include <Magick++.h>
#endif // HAVE_LIBJPEG

// TODO remove this once Libnova 0.13.0 becomes mainstream
#if !HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF
double ln_get_heliocentric_time_diff (double JD, struct ln_equ_posn *object);
#endif

namespace rts2image
{

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
{ EXPOSURE_START, INFO_CALLED, EXPOSURE_END, TRIGGERED }
imageWriteWhich_t;

/**
 * Generic class which represents an image.
 *
 * This class represents image. It has functions for accessing various image
 * properties, as well as method to perform some image operations.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Image:public FitsFile
{
	public:
		// list of sex results..
		struct stardata *sexResults;
		int sexResultNum;

		// memory-only image..
		Image ();
		// copy constructor
		Image (Image * in_image);
		// memory-only with exposure time
		Image (const struct timeval *in_exposureStart);
		Image (const struct timeval *in_exposureS, float in_img_exposure);
		// skeleton for DB image
		Image (long in_img_date, int in_img_usec, float in_img_exposure);
		// create image
		Image (char *in_filename, const struct timeval *in_exposureStart);
		/**
		 * Create image from expand path.
		 *
		 * @param in_expression     Expresion containing characters which will be expanded.
		 * @param in_expNum         Exposure number.
		 * @param in_exposureStart  Starting time of the exposure.
		 * @param in_connection     Connection of camera requesting exposure.
		 */
		Image (const char *in_expression, int in_expNum, const struct timeval *in_exposureStart, Rts2Conn * in_connection);
		// create image in que
		Image (Rts2Target * currTarget, rts2core::Rts2DevClientCamera * camera, const struct timeval *in_exposureStart);
		virtual ~ Image (void);

		virtual int closeFile ();

		void openImage (const char *_filename = NULL, bool _verbose = true, bool readOnly = false);
		void getHeaders ();
		/**
		 * Retrieve from image target related headers.
		 *
		 * @throw rts2image::KeyNotFound
		 */
		void getTargetHeaders ();

		void setTargetHeaders (int _tar_id, int _obs_id, int _img_id, char _obs_subtype);

		virtual int toQue ();
		virtual int toAcquisition ();
		virtual int toArchive ();
		virtual int toDark ();
		virtual int toFlat ();
		virtual int toMasterFlat ();
		virtual int toTrash ();

		virtual img_type_t getImageType ();

		shutter_t getShutter () { return shutter; }

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

		// int saveImageData (const char *save_filename, unsigned short *in_data);

		void setValue (const char *name, bool value, const char *comment);
		void setValue (const char *name, int value, const char *comment);
		void setValue (const char *name, long value, const char *comment);
		void setValue (const char *name, float value, const char *comment);
		void setValue (const char *name, double value, const char *comment);
		void setValue (const char *name, char value, const char *comment);
		void setValue (const char *name, const char *value, const char *comment);
		void setValue (const char *name, time_t * sec, long usec, const char *comment);
		// that method is used to update DATE - creation date entry - for other file then ffile
		void setCreationDate (fitsfile * out_file = NULL);

		void getValue (const char *name, bool & value, bool required = false, char *comment = NULL);
		void getValue (const char *name, int &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, long &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, float &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, double &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, char &value, bool required = false, char *command = NULL);
		void getValue (const char *name, char *value, int valLen, const char* defVal = NULL, bool required = false, char *comment = NULL);
		void getValue (const char *name, char **value, int valLen, bool required = false, char *comment = NULL);

		/**
		 * Get double value from image.
		 *
		 * @param name Value name.
		 * @return Value
		 * @throw KeyNotFound
		 */
		double getValue (const char *name);

		void getValues (const char *name, int *values, int num, bool required = false, int nstart = 1);
		void getValues (const char *name, long *values, int num, bool required = false, int nstart = 1);
		void getValues (const char *name, double *values, int num, bool required = false, int nstart = 1);
		void getValues (const char *name, char **values, int num, bool required = false, int nstart = 1);

		void writeMetaData (struct imghdr *im_h, double xoa, double yoa);

		int writeData (char *in_data, char *fullTop, int nchan);

		/**
		 * Build image histogram.
		 *
		 * @param histogram array for calculated histogram
		 * @param nbins     number of histogram bins
		 */
		void getHistogram (long *histogram, long nbins);

		/**
		 * Build channel histogram.
		 *
		 * @param chan      channel number 
		 * @param histogram array for calculated histogram
		 * @param nbins     number of histogram bins
		 */
		void getChannelHistogram (int chan, long *histogram, long nbins);

		/**
		 * Returns image grayscaled buffer. Black have value equal to black parameter, white is 0.
		 *
		 * @param chan       channel number
		 * @param buf        buffer (will be allocated by image routine). You must delete it.
		 * @param black      black value.
		 * @param quantiles  quantiles in 0-1 range for image scaling.
		 * @param offset     offset after each line
		 */
		template <typename bt> void getChannelGrayscaleBuffer (int chan, bt * &buf, bt black, float quantiles=0.005, size_t offset = 0);

#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1
		/**
		 * Return image data as Magick::Image class.
		 *
		 * @param quantiles  Quantiles in 0-1 range for image scaling.
		 *
		 * @throw Exception
		 */
		Magick::Image getMagickImage (const char *label = NULL, float quantiles=0.005, int chan = -1);

		/**
		 * Write lable to given position. Label text will be expanded.
		 *
		 * @param mimage    Image to which write label.
		 * @param x         X coordinate of rectangle with label.
		 * @param y         Y coordinate of rectangle with label.
		 * @param labelText Text of label. It will be expanded through expandVariable.
		 */
		void writeLabel (Magick::Image &mimage, int x, int y, unsigned int fs, const char *labelText);

		/**
		 * Write image as JPEG to provided data buffer.
		 * Buffer will be allocated by this call and should
		 * be free afterwards.
		 *
		 * @param expand_str    Expand string for image name
		 * @param zoom          zoom image (before writing its label)
		 * @param label         label added to image box (expand character).
		 * @param quantiles     Quantiles in 0-1 range for image scaling.
		 *
		 * @throw Exception
		 */
		void writeAsJPEG (std::string expand_str, double zoom = 1.0, const char * label = NULL, float quantiles=0.005);

		/**
		 * Store image to blob, which can be used to get data etc..
		 */
		void writeAsBlob (Magick::Blob &blob, const char * label = NULL, float quantiles=0.005);
#endif

		double getAstrometryErr ();

		virtual int saveImage ();

		virtual int deleteFromDB () { return 0; }
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

		void setExposureStart (const struct timeval *tv) { setExpandDate (tv); }

		double getExposureStart () { return getExpandDateCtime (); }

		long getExposureSec () { return getCtimeSec ();	}

		long getExposureUsec () { return getCtimeUsec (); }

		void setExposureLength (float in_exposureLength)
		{
			exposureLength = in_exposureLength;
			setValue ("EXPOSURE", exposureLength, "exposure length in seconds");
			setValue ("EXPTIME", exposureLength, "exposure length in seconds");
		}

		float getExposureLength () { return exposureLength; }

		int getTargetId () { if (targetId < 0) getTargetHeaders (); return targetId; }

		std::string getTargetString ();
		std::string getTargetSelString ();

		std::string getExposureNumberString ();
		std::string getObsString ();
		std::string getImgIdString ();

		virtual std::string getProcessingString ();

		// image parameter functions
		std::string getExposureLengthString ();

		int getTargetIdSel () { if (targetIdSel < 0) getTargetHeaders (); return targetIdSel; }

		char getTargetType (bool do_load = true) { if (do_load && targetType == TYPE_UNKNOW) getTargetHeaders (); return targetType; }

		int getObsId () { if(obsId < 0) getTargetHeaders (); return obsId; }
		void setObsId (int _obsid) { obsId = _obsid; }

		int getImgId ();
		void setImgId (int _imgid) { imgId = _imgid; }

		const char *getFilter () { return filter; }

		void setFilter (const char *in_filter);

		int getFilterNum () { return filter_i; }

		void computeStatistics ();

		double getAverage () { return average; }

		double getStdDev () { return stdev; }

		/**
		 * Return
		 */
		double getBgStdDev () { return bg_stdev; }

		int getFocPos () { return focPos; }

		void setFocPos (int new_pos) { focPos = new_pos; }

		int getIsAcquiring () { return isAcquiring; }

		void keepImage () { flags |= IMAGE_KEEP_DATA; }

		void closeData () { channels.clear (); }

		// remove pointer to camera dataa
		void deallocate () { channels.clear (); }

		/**
		 * @throw rts2core::Error
		 */
		void loadChannels ();

		const void *getChannelData (int chan);
		unsigned short *getChannelDataUShortInt (int chan);

		int getPixelByteSize ()
		{
			if (dataType == RTS2_DATA_ULONG)
				return 4;
			return abs (dataType) / 8;
		}

		/**
		 * Return data type, as one of the RTS2_DATA_XXXX constants.
		 */
		int getDataType () { return dataType; }

		//void setDataUShortInt (unsigned short *in_data, long in_naxis[2]);

		//int substractDark (Image * darkImage);

		int setAstroResults (double ra, double dec, double ra_err, double dec_err);

		int addStarData (struct stardata *sr);
		double getFWHM ();

		double getPrecision ()
		{
			double val = rts2_nan("f");
			getValue ("POS_ERR", val, false);
			return val;
		}
		// assumes, that on image is displayed strong light source,
		// it find position of it's center
		// return 0 if center was found , -1 in case of error.
		// Put center coordinates in pixels to x and y

		// helpers, get row & line center
		//int getCenterRow (long row, int row_width, double &x);
		//int getCenterCol (long col, int col_width, double &y);

		//int getCenter (double &x, double &y, int bin);

		long getChannelWidth (int chan) { return channels[chan]->getWidth (); }

		long getChannelHeight (int chan) { return channels[chan]->getHeight (); }

		/**
		 * Returns number of pixels in a given channel.
		 */
		long getChannelNPixels (int chan) { return channels[chan]->getNPixels (); }

		/**
		 * Returns sum of pixel sizes of all channels.
		 */
		long getSumNPixels ();

		/**
		 * Returns ra & dec distance in degrees of pixel [x,y] from device axis (XOA and YOA coordinates)
		 *
		 * Hint: if you want to guide telescope, you need to use that value.
		 * If target is W from telescope axis, it will be negative, and you need to move to W, so you need
		 * to decrease RA.
		 */

		int getOffset (double x, double y, double &chng_ra, double &chng_dec, double &sep_angle);

		// returns pixels [x1,y1] and [x2,y2] offset in ra and dec degrees
		int getOffset (double x1, double y1, double x2, double y2, double &chng_ra, double &chng_dec, double &sep_angle);

		/**
		 * This function is good only for HAM source detection on FRAM telescope in Argentina.
		 * HAM is calibration source, which is used test target for photometer to measure it's sesitivity
		 * (and other things, such as athmospheric dispersion..).
		 * You most probably don't need it.
		 */
		int getHam (double &x, double &y);

		// return offset & flux of the brightest star in field
		int getBrightestOffset (double &x, double &y, float &flux);

		/**
		 * Computes RA DEC of given pixel. Uses telescope information. This method does not use WCS,
		 * point RA DEC is computed from telescope coordinates recorded at image headers.
		 *
		 * @param x    X coordinate of the pixel
		 * @param y    Y coordinate of the pixel
		 * @param ra   resulted RA
		 * @param dec  resulted DEC
		 *
		 * @return -1 on errror, 0 on success
		 */
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
		void setMountFlip (int in_mnt_flip) { mnt_flip = in_mnt_flip; }

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
		 * @throw KeyNotFound
		 */
		void getCoord (struct ln_equ_posn &radec, const char *ra_name, const char *dec_name);

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
		void getCoordObject (struct ln_equ_posn &radec);
		void getCoordTarget (struct ln_equ_posn &radec);
		void getCoordAstrometry (struct ln_equ_posn &radec);
		void getCoordMount (struct ln_equ_posn &radec);

		void getCoordBest (struct ln_equ_posn &radec);
		void getCoordBestAltAz (struct ln_hrz_posn &hrz, struct ln_lnlat_posn *observer);

		void getCoord (LibnovaRaDec & radec, const char *ra_name, const char *dec_name);
		void getCoordTarget (LibnovaRaDec & radec);
		void getCoordAstrometry (LibnovaRaDec & radec);
		void getCoordMount (LibnovaRaDec & radec);

		/**
		 * Retrieves from FITS headers best target coordinates.
		 *
		 * Those are taken from RASC and DECL keywords, which are filled by
		 * astrometry value, if astrometry was processed sucessfully from this image,
		 * and target values corrected by any know offsets if astrometry is not know
		 * or invalid.
		 */
		void getCoordBest (LibnovaRaDec & radec);

		std::string getOnlyFileName ();

		virtual bool haveOKAstrometry () { return false; }

		virtual bool isProcessed () { return false; }

		void printFileName (std::ostream & _os);

		virtual void print (std::ostream & _os, int in_flags = 0);

		void setInstrument (const char *instr)
		{
			setValue ("INSTRUME", instr, "name of the data acqusition instrument");
		}

		void setTelescope (const char *tel)
		{
			setValue ("TELESCOP", tel, "name of the data acqusition telescope");
		}

		void setObserver ()
		{
			setValue ("OBSERVER", "RTS2 " VERSION, "observer");
		}

		void setOrigin (const char *orig)
		{
			setValue ("ORIGIN", orig, "organisation responsible for data");
		}

		/**
		 * Save environmental variables, as specified by environmental config entry.
		 */
		void setEnvironmentalValues ();

		void writeConn (Rts2Conn * conn, imageWriteWhich_t which = EXPOSURE_START);

		/**
		 * This will create WCS from record available at the FITS file.
		 *
		 * @param x_off  offset of the X coordinate (most probably computed from bright star location)
		 * @param y_off  offset of the Y coordinate (most probably computed from bright star location)
		 *
		 * @return 0 on success, -1 on error.
		 */
		int createWCS (double x_off = 0, double y_off = 0);

		friend std::ostream & operator << (std::ostream & _os, Image & image)
		{
			return image.printImage (_os);
		}

		std::ostream & printImage (std::ostream & _os);

		double getLongtitude ();

		/**
		 * Gets julian data of exposure start
		 */
		double getExposureJD ();

		double getMidExposureJD () { return getExposureJD () + getExposureLength () / 2.0 / 86400; }

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
		int integrate (unsigned short *data, double px, double py, int size, float *ret);
		int evalAF (float *result, float *error);

		std::vector < pixel > list;
		double median, sigma;

	protected:
		char *cameraName;
		char *mountName;
		char *focName;

		shutter_t shutter;

		struct ln_equ_posn pos_astr;
		double ra_err;
		double dec_err;
		double img_err;

		int createImage ();

		int writeExposureStart ();

		virtual int isGoodForFwhm (struct stardata *sr);
		char *getImageBase (void);

		// expand expression to image path
		virtual std::string expandVariable (char expression, size_t beg);
		virtual std::string expandVariable (std::string expression);

	private:
		int targetId;
		int targetIdSel;
		char targetType;
		char *targetName;
		int obsId;
		int imgId;

		int filter_i;
		char *filter;
		float exposureLength;
		
		int createImage (std::string in_filename);
		int createImage (char *in_filename);
		// if filename is NULL, will take name stored in this->getFileName ()
		// if openImage should load header..
		bool loadHeader;
		bool verbose;

		rts2image::Channels channels;
		int16_t dataType;
		int focPos;
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

		std::map <int, TableData *> arrayGroups;

		void initData ();

		int writeImgHeader (struct imghdr *im_h, int nchan);

		/**
		 * Record image physical coordinates.
		 *
		 * @param x     X offset of the image.
		 * @param y     Y offset of the image.
		 * @param bin_x Binning along X axis.
		 * @param bin_y Binning along Y axis.
		 */
		void writePhysical (int x, int y, int bin_x, int bin_y);

		void writeConnBaseValue (const char *name, rts2core::Value *val, const char *desc);

		/**
		 * Either prepare array data to be written, or write them to header if those are simple
		 * data.
		 */
		void prepareArrayData (const char *name, Rts2Conn *conn, rts2core::Value *val);

		void writeConnArray (TableData *tableData);

		// writes one value to image
		void writeConnValue (Rts2Conn *conn, rts2core::Value *val);

		// record value changes
		void recordChange (Rts2Conn *conn, rts2core::Value *val);
};

}

//Image & operator - (Image & img_1, Image & img_2);
//Image & operator + (Image & img_, Image & img_2);
//Image & operator -= (Image & img_2);
//Image & operator += (Image & img_2);
#endif							 /* !__RTS2_IMAGE__ */

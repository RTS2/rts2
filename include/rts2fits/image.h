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
#include <rts2-config.h>
#include <fitsio.h>

#include "imghdr.h"

#include "rts2fits/fitsfile.h"
#include "rts2fits/channel.h"

#include "libnova_cpp.h"
#include "devclient.h"
#include "expander.h"
#include "rts2target.h"

#define NUM_WCS_VALUES    7

#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1
#include <Magick++.h>
#endif // HAVE_LIBJPEG

// TODO remove this once Libnova 0.13.0 becomes mainstream
#if !RTS2_HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF
double ln_get_heliocentric_time_diff (double JD, struct ln_equ_posn *object);
#endif

namespace rts2image
{

/** Image scaling functions. */
typedef enum { SCALING_LINEAR, SCALING_LOG, SCALING_SQRT, SCALING_POW } scaling_type;

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

const void * getScaledData (int dataType, const void *data, size_t numpix, long smin, long smax, scaling_type scaling, int newType);

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
		Image (const char *in_filename, const struct timeval *in_exposureStart, bool _overwrite = false, bool _writeConnection = true, bool _writeRTS2Values = true);
		/**
		 * Create image from expand path.
		 *
		 * @param in_expression     Expresion containing characters which will be expanded.
		 * @param in_expNum         Exposure number.
		 * @param in_exposureStart  Starting time of the exposure.
		 * @param in_connection     Connection of camera requesting exposure.
		 */
		Image (const char *in_expression, int in_expNum, const struct timeval *in_exposureStart, rts2core::Connection * in_connection, bool _overwrite = false, bool _writeConnection = true, bool _writeRTS2Values = true);

		/**
		 * Create image for a given target.
		 *
		 * 
		 *
		 */
		Image (Rts2Target * currTarget, rts2core::DevClientCamera * camera, const struct timeval *in_exposureStart, const char *expand_path = NULL, bool overwrite = false);

		virtual ~ Image (void);

		virtual void openFile (const char *_filename = NULL, bool readOnly = false, bool _verbose = false);
		virtual int closeFile ();

		/**
		 * Retrieve from image target related headers.
		 *
		 * @throw rts2image::KeyNotFound
		 */
		void getTargetHeaders ();

		void setTargetHeaders (int _tar_id, int _obs_id, int _img_id, char _obs_subtype);
		void writeTargetHeaders (Rts2Target *target, bool set_values = true);

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

		/**
		 * Write image primary header from template.
		 */
		void writePrimaryHeader (const char *devname);

		void writeMetaData (struct imghdr *im_h);

		/**
		 * Write WCS headers to active channel.
		 *
		 * @param mods  modificators to add to WCS values (CRVALs[12], cRPIXs[12], CDELTs[12], CROTA)
		 */
		void writeWCS (double mods[NUM_WCS_VALUES]);

		void setRTS2Value (const char *name, int value, const char *comment)
		{
			if (writeRTS2Values)
				setValue (name, value, comment);
		}
		void setRTS2Value (const char *name, double value, const char *comment)
		{
			if (writeRTS2Values)
				setValue (name, value, comment);
		}
		void setRTS2Value (const char *name, const char *value, const char *comment)
		{
			if (writeRTS2Values)
				setValue (name, value, comment);
		}

		int writeData (char *in_data, char *fullTop, int nchan);

		/**
		 * Fill image header structure.
		 */
		void getImgHeader (struct imghdr *im_h, int chan);

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


		template <typename bt, typename dt> void getChannelGrayscaleByteBuffer (int chan, bt * &buf, bt black, dt low, dt high, long s, size_t offset, bool invert_y);

		/**
		 * Returns image grayscaled buffer. Black have value equal to black parameter, white is 0.
		 *
		 * @param chan       channel number
		 * @param buf        buffer (will be allocated by image routine). You must delete it.
		 * @param black      black value.
		 * @param quantiles  quantiles in 0-1 range for image scaling.
		 * @param offset     offset after each line
		 * @param invert_y   invert with Y axis (rows)
		 */
		template <typename bt, typename dt> void getChannelGrayscaleBuffer (int chan, bt * &buf, bt black, dt minval, dt mval, float quantiles=0.005, size_t offset = 0, bool invert_y = false);

		void getChannelGrayscaleImage (int _dataType, int chan, unsigned char * &buf, float quantiles, size_t offset);

#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1
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

		void setObjectName (rts2core::Connection *camera_conn);

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
			setRTS2Value ("EXPOSURE", exposureLength, "exposure length in seconds");
			setRTS2Value ("EXPTIME", exposureLength, "exposure length in seconds");
		}

		float getExposureLength () { return exposureLength; }

		int getTargetId () { if (targetId < 0) getTargetHeaders (); return targetId; }

		std::string getTargetName ();

		std::string getTargetIDString ();
		std::string getTargetIDSelString ();

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

		void computeStatistics (size_t _from = 0, size_t _dataSize = 0);

		double getAverage () { return average; }

		double getAvgStdDev () { return avg_stdev; }

		int getFocPos () { return focPos; }

		void setFocPos (int new_pos) { focPos = new_pos; }

		int getIsAcquiring () { return isAcquiring; }

		void keepImage () { flags |= IMAGE_KEEP_DATA; }
		void unkeepImage () { flags &= ~IMAGE_KEEP_DATA; }
		bool hasKeepImage () { return flags & IMAGE_KEEP_DATA; }

		void setUserFlag () { flags |= IMAGE_FLAG_USER1; }
		bool hasUserFlag () { return flags & IMAGE_FLAG_USER1; }

		void closeData () { channels.clear (); }

		// remove pointer to camera dataa
		void deallocate () { channels.clear (); }

		/**
		 * @throw rts2core::Error
		 */
		void loadChannels ();

		const void *getChannelData (int chan);
		const void *getChannelDataScaled (int chan, long smin, long smax, scaling_type scaling, int newType);

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

		int setAstroResults (double ra, double dec, double ra_err, double dec_err);

		int addStarData (struct stardata *sr);

		double getPrecision ()
		{
			double val = NAN;
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

		/**
		 * Returns number of channels.
		 *
		 * @return number of channels
		 */
		int getChannelSize () { return channels.size (); }

		/**
		 * Return physical channel number of virtual channel.
		 */
		int getChannelNumber (int chan) { return channels[chan]->getChannelNumber (); }

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

		int getError (double &eRa, double &eDec, double &eRad);

		/**
		 * Increase WCS value 
		 */
		void addWcs (double delta, int i)
		{
			if (isnan (total_wcs[i]))
				total_wcs[i] = delta;
			else
				total_wcs[i] += delta;
		}

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
			setRTS2Value ("INSTRUME", instr, "name of the data acqusition instrument");
		}

		void setTelescope (const char *tel)
		{
			setRTS2Value ("TELESCOP", tel, "name of the data acqusition telescope");
		}

		void setObserver ()
		{
			setRTS2Value ("OBSERVER", "RTS2 " RTS2_VERSION, "observer");
		}

		void setOrigin (const char *orig)
		{
			setRTS2Value ("ORIGIN", orig, "organisation responsible for data");
		}

		/**
		 * Save environmental variables, as specified by environmental config entry.
		 */
		void setEnvironmentalValues ();

		void writeConn (rts2core::Connection * conn, imageWriteWhich_t which = EXPOSURE_START);

		/**
		 * Sets image errors.
		 */
		void setErrors (double i_r, double i_d, double i_e)
		{
			ra_err = i_r;
			dec_err = i_d;
			img_err = i_e;
		}

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

		std::vector < pixel > list;
		double median, sigma;

		/**
		 * Sets which values should be written to the image.
		 *
		 * @param write_conn   Write all connection values (values marked as RTS2_FITS_WRITE)
		 * @param rts2_write   Write extra RTS2 values (exposure time,..)
		 */
		void setWriteConnection (bool write_conn, bool rts2_write)
		{
			writeConnection = write_conn;
			writeRTS2Values = rts2_write;
		}

		int writeExposureStart ();

		void setAUXWCS (rts2core::StringArray * wcsaux);

	protected:
		char *cameraName;
		char *mountName;
		char *focName;

		shutter_t shutter;

		struct ln_equ_posn pos_astr;
		double ra_err;
		double dec_err;
		double img_err;

		int createImage (bool _overwrite = false);

		// expand expression to image path
		virtual std::string expandVariable (char expression, size_t beg, bool &replaceNonAlpha);
		virtual std::string expandVariable (std::string expression);

	private:
		// if connection values should be written to FITS file. Connection values can also be written by template mechanism
		bool writeConnection;

		// if write RTS2 extended values
		bool writeRTS2Values;
	
		// if OBJECT keyword was written
		bool objectNameWritten;

		int targetId;
		int targetIdSel;
		char targetType;
		char *templateDeviceName;
		char *targetName;
		int obsId;
		int imgId;

		int filter_i;
		char *filter;
		float exposureLength;
		
		int createImage (char *in_filename, bool _overwrite = false);
		int createImage (std::string in_filename, bool _overwrite = false);

		void getHeaders ();

		// if filename is NULL, will take name stored in this->getFileName ()
		// if openFile will load header..
		bool loadHeader;
		bool verbose;

		rts2image::Channels channels;
		int16_t dataType;
		int focPos;
		float signalNoise;
		int getFailed;
		double average;
		double avg_stdev;
		short int min;
		short int max;
		short int mean;
		int isAcquiring;
		// that value is nan when rotang was already set;
		// it is calculated as sum of partial rotangs.
		// For change of total rotang, addRotang function is provided.
		// CRVALs, CRPIXs, CDELTAs, CROTAs
		double total_wcs[NUM_WCS_VALUES];
		std::list <rts2core::ValueString> string_wcs;

		std::list <std::string> wcsauxs;

		// Multiple WCS to add rotang to
		char wcs_multi_rotang;

		int expNum;

		std::map <int, TableData *> arrayGroups;

		void initData ();

		/**
		 * Write data channel (image) header).
		 */
		int writeImgHeader (struct imghdr *im_h, int nchan);

		void writeConnBaseValue (const char *name, rts2core::Value *val, const char *desc);

		/**
		 * Either prepare array data to be written, or write them to header if those are simple
		 * data.
		 */
		void prepareArrayData (const char *name, rts2core::Connection *conn, rts2core::Value *val);

		void writeConnArray (TableData *tableData);

		// writes one value to image
		void writeConnValue (rts2core::Connection *conn, rts2core::Value *val);

		// record value changes
		void recordChange (rts2core::Connection *conn, rts2core::Value *val);
};

}

//Image & operator - (Image & img_1, Image & img_2);
//Image & operator + (Image & img_, Image & img_2);
//Image & operator -= (Image & img_2);
//Image & operator += (Image & img_2);
#endif							 /* !__RTS2_IMAGE__ */

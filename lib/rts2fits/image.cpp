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

#include <libnova/libnova.h>

#include "rts2fits/image.h"
#include "imghdr.h"

#include "expander.h"
#include "configuration.h"
#include "timestamp.h"
#include "rts2target.h"
#include "valuestat.h"
#include "valuerectangle.h"

#include "imgdisplay.h"

#include <iomanip>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

using namespace rts2image;

// TODO remove this once Libnova 0.13.0 becomes mainstream
#if !RTS2_HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF
double ln_get_heliocentric_time_diff (double JD, struct ln_equ_posn *object)
{
	struct ln_nutation nutation;
	struct ln_helio_posn earth;

	ln_get_nutation (JD, &nutation);
	ln_get_earth_helio_coords (JD, &earth);

	double theta = ln_deg_to_rad (ln_range_degrees (earth.L + 180));
	double ra = ln_deg_to_rad (object->ra);
	double dec = ln_deg_to_rad (object->dec);
	double c_dec = cos (dec);
	double obliq = ln_deg_to_rad (nutation.ecliptic);

	/* L.Binnendijk Properties of Double Stars, Philadelphia, University of Pennselvania Press, pp. 228-232, 1960 */
	return -0.0057755 * earth.R * (
		cos (theta) * cos (ra) * c_dec
		+ sin (theta) * (sin (obliq) * sin (dec) + cos (obliq) * c_dec * sin (ra)));
}
#endif

void Image::initData ()
{
	exposureLength = NAN;

	oldImageName = std::string ("");

	loadHeader = false;
	verbose = true;

	templateDeviceName = NULL;
	
	setFileName (NULL);
	setFitsFile (NULL);
	targetId = -1;
	targetIdSel = -1;
	targetType = TYPE_UNKNOW;
	targetName = NULL;
	imgId = -1;
	obsId = -1;
	cameraName = NULL;
	mountName = NULL;
	focName = NULL;
	filter_i = -1;
	filter = NULL;
	avg_stdev = 0;
	min = max = mean = pixelSum = 0;
	dataType = RTS2_DATA_USHORT;
	sexResults = NULL;
	sexResultNum = 0;
	focPos = -1;
	signalNoise = 17;
	getFailed = 0;
	expNum = 1;

	pos_astr.ra = NAN;
	pos_astr.dec = NAN;
	ra_err = NAN;
	dec_err = NAN;
	img_err = NAN;

	isAcquiring = 0;

	for (int i = 0; i < NUM_WCS_VALUES; i++)
		total_wcs[i] = NAN;
	wcs_multi_rotang = '\0';

	shutter = SHUT_UNKNOW;

	flags = 0;

	writeConnection = true;
	writeRTS2Values = true;
}


Image::Image ():FitsFile ()
{
	initData ();
	flags = IMAGE_KEEP_DATA;
}

Image::Image (Image * in_image):FitsFile (in_image)
{
	flags = in_image->flags;
	filter_i = in_image->filter_i;
	filter = in_image->filter;
	in_image->filter = NULL;

	loadHeader = in_image->loadHeader;
	verbose = in_image->verbose;

	templateDeviceName = NULL;

	exposureLength = in_image->exposureLength;
	
	channels = in_image->channels;
	dataType = in_image->dataType;
	in_image->channels.clear ();
	focPos = in_image->focPos;
	signalNoise = in_image->signalNoise;
	getFailed = in_image->getFailed;
	avg_stdev = in_image->avg_stdev;
	min = in_image->min;
	max = in_image->max;
	mean = in_image->mean;
	isAcquiring = in_image->isAcquiring;
	for (int i = 0; i < NUM_WCS_VALUES; i++)
		total_wcs[i] = in_image->total_wcs[i];
	wcs_multi_rotang = in_image->wcs_multi_rotang;

	targetId = in_image->targetId;
	targetIdSel = in_image->targetIdSel;
	targetType = in_image->targetType;
	targetName = in_image->targetName;
	in_image->targetName = NULL;
	obsId = in_image->obsId;
	imgId = in_image->imgId;
	cameraName = in_image->cameraName;
	in_image->cameraName = NULL;
	mountName = in_image->mountName;
	in_image->mountName = NULL;
	focName = in_image->focName;
	in_image->focName = NULL;

	pos_astr.ra = in_image->pos_astr.ra;
	pos_astr.dec = in_image->pos_astr.dec;
	ra_err = in_image->ra_err;
	dec_err = in_image->dec_err;
	img_err = in_image->img_err;

	sexResults = in_image->sexResults;
	in_image->sexResults = NULL;
	sexResultNum = in_image->sexResultNum;

	shutter = in_image->getShutter ();

	// other image will be saved!
	flags = in_image->flags;
	//in_image->flags &= ~IMAGE_SAVE;
}

Image::Image (const struct timeval *in_exposureStart):FitsFile (in_exposureStart)
{
	initData ();
	createImage ();
	flags = IMAGE_KEEP_DATA;
	writeExposureStart ();
}

Image::Image (const struct timeval *in_exposureStart, float in_img_exposure):FitsFile (in_exposureStart)
{
	initData ();
	writeExposureStart ();
	exposureLength = in_img_exposure;
}

Image::Image (long in_img_date, int in_img_usec, float in_img_exposure):FitsFile ()
{
	struct timeval tv;
	initData ();
	flags |= IMAGE_NOT_SAVE;
	tv.tv_sec = in_img_date;
	tv.tv_usec = in_img_usec;
	setExposureStart (&tv);
	exposureLength = in_img_exposure;
}

Image::Image (const char *_filename, const struct timeval *in_exposureStart, bool _overwrite, bool _writeConnection, bool _writeRTS2Values):FitsFile (in_exposureStart)
{
	initData ();
	setWriteConnection (_writeConnection, _writeRTS2Values);

	createImage (_filename, _overwrite);
	writeExposureStart ();
}

Image::Image (const char *in_expression, int in_expNum, const struct timeval *in_exposureStart, rts2core::Connection * in_connection, bool _overwrite, bool _writeConnection, bool _writeRTS2Values):FitsFile (in_exposureStart)
{
	initData ();
	setWriteConnection (_writeConnection, _writeRTS2Values);

	setCameraName (in_connection->getName ());
	expNum = in_expNum;

	createImage (expandPath (in_expression), _overwrite);
	writeExposureStart ();
}

Image::Image (Rts2Target * currTarget, rts2core::DevClientCamera * camera, const struct timeval *in_exposureStart, const char *expand_path, bool overwrite):FitsFile (in_exposureStart)
{
	std::string in_filename;

	initData ();

	setExposureStart (in_exposureStart);

	setCameraName (camera->getName ());

	mountName = NULL;
	focName = NULL;

	targetId = currTarget->getTargetID ();
	targetIdSel = currTarget->getObsTargetID ();
	targetType = currTarget->getTargetType ();
	obsId = currTarget->getObsId ();
	imgId = currTarget->getNextImgId ();

	isAcquiring = currTarget->isAcquiring ();
	if (expand_path != NULL)
	{
		in_filename = expandPath (expand_path);
	}
	else if (isAcquiring)
	{
		// put acqusition images to acqusition que
		in_filename = expandPath ("%b/acqusition/que/%c/%f");
	}
	else
	{
		in_filename = expandPath (rts2core::Configuration::instance ()->observatoryQuePath ());
	}

	createImage (in_filename, overwrite);

	writeExposureStart ();

	writeTargetHeaders (currTarget, false);

	setValue ("CCD_NAME", camera->getName (), "camera name");

	setEnvironmentalValues ();

	currTarget->writeToImage (this, getExposureJD ());
}

Image::~Image (void)
{
	saveImage ();

	for (std::map <int, TableData *>::iterator iter = arrayGroups.begin (); iter != arrayGroups.end ();)
	{
		delete iter->second;
		arrayGroups.erase (iter++);
	}

	delete[]templateDeviceName;
	delete[]targetName;
	delete[]cameraName;
	delete[]mountName;
	delete[]focName;

	dataType = RTS2_DATA_USHORT;
	delete[]filter;
	if (sexResults)
		free (sexResults);
}

std::string Image::expandVariable (char expression, size_t beg, bool &replaceNonAlpha)
{
	switch (expression)
	{
		case 'b':
			replaceNonAlpha = false;
			return rts2core::Configuration::instance ()->observatoryBasePath ();
		case 'c':
			return getCameraName ();
		case 'E':
			return getExposureLengthString ();
		case 'F':
			return getFilter ();
		case 'f':
			return getOnlyFileName ();
		case 'i':
			return getImgIdString ();
		case 'o':
			return getObsString ();
		case 'p':
			return getProcessingString ();
		case 'R':
			return getTargetIDSelString ();
		case 'T':
			return getTargetName ();
		case 't':
			return getTargetIDString ();
		case 'n':
			return getExposureNumberString ();
		case '_':
			return (oldImageName.length() > 0) ? oldImageName : getOnlyFileName ();
		default:
			return rts2core::Expander::expandVariable (expression, beg, replaceNonAlpha);
	}
}

std::string Image::expandVariable (std::string expression)
{
	std::string ret;
	char valB[200];

	if (templateDeviceName)
	{
		// : is for sub-values
		size_t dblc = expression.find (':');
		if (dblc != std::string::npos)
		{
			rts2core::Value *val = ((rts2core::Block *) getMasterApp ())->getValueExpression (expression.substr (0, dblc), templateDeviceName);
			if (val != NULL)
				return val->getDisplaySubValue (expression.substr (dblc + 1).c_str ());
		}
		else
		{
			rts2core::Value *val = ((rts2core::Block *) getMasterApp ())->getValueExpression (expression, templateDeviceName);
			if (val != NULL)
				return val->getDisplayValue ();
		}
	}

	try
	{
		getValue (expression.c_str (), valB, 200, NULL, true);
	}
	catch (rts2core::Error &er)
	{
		std::cerr << "cannot expand variable " << expression << std::endl;
		return rts2core::Expander::expandVariable (expression);
	}
	return std::string (valB);
}

int Image::createImage (bool _overwrite)
{
	if (createFile (_overwrite))
		return -1;

	flags = IMAGE_NOT_SAVE;
	shutter = SHUT_UNKNOW;

	fits_create_hdu (getFitsFile (), &fits_status);
	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "cannot create primary HDU: " << getFitsErrors () << sendLog;
		return -1;
	}

	fits_write_key_log (getFitsFile (), (char *) "SIMPLE", 1, (char *) "conform to FITS standard", &fits_status);
	fits_write_key_lng (getFitsFile (), (char *) "BITPIX", 16, (char *) "unsigned short data", &fits_status);
	fits_write_key_lng (getFitsFile (), (char *) "NAXIS", 0, (char *) "number of axes", &fits_status);
	fits_write_key_log (getFitsFile (), (char *) "EXTEND", 1, (char *) "this is FITS with extensions", &fits_status);
	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "cannot write keys to primary HDU: " << getFitsErrors () << sendLog;
		return -1;
	}

	// add history
	writeHistory ("Created with RTS2 version " RTS2_VERSION " build on " __DATE__ " " __TIME__ ".");

	logStream (MESSAGE_DEBUG) << "creating image " << getFileName () << sendLog;

	flags = IMAGE_SAVE;
	return 0;
}

int Image::createImage (std::string in_filename, bool _overwrite)
{
	setFileName (in_filename.c_str ());

	return createImage (_overwrite);
}

int Image::createImage (char *in_filename, bool _overwrite)
{
	setFileName (in_filename);

	return createImage (_overwrite);
}

void Image::getHeaders ()
{
	struct timeval tv;

	try
	{
		getValue ("CTIME", tv.tv_sec, true);
		getValue ("USEC", tv.tv_usec, true);
	}
	catch (rts2core::Error )
	{
		char daobs[100];
		getValue ("DATE-OBS", daobs, 100, NULL, verbose);
		parseDate (daobs, &tv.tv_sec, true);
		tv.tv_usec = 0;
	}
	setExposureStart (&tv);
	// if EXPTIM fails..
	try
	{
		getValue ("EXPTIME", exposureLength, false);
	}
	catch (rts2core::Error)
	{
		getValue ("EXPOSURE", exposureLength, verbose);
	}

	cameraName = new char[DEVICE_NAME_SIZE + 1];
	getValue ("CCD_NAME", cameraName, DEVICE_NAME_SIZE, "UNK");

	mountName = new char[DEVICE_NAME_SIZE + 1];
	getValue ("MNT_NAME", mountName, DEVICE_NAME_SIZE, "UNK");

	focName = new char[DEVICE_NAME_SIZE + 1];
	getValue ("FOC_NAME", focName, DEVICE_NAME_SIZE, "UNK");

	focPos = -1;
	getValue ("FOC_POS", focPos, false);

	getValue ("CAM_FILT", filter_i, false);

	filter = new char[5];
	getValue ("FILTER", filter, 5, "UNK", verbose);
	getValue ("AVERAGE", average, false);
	getValue ("AVGSTDEV", avg_stdev, false);
	getValue ("RA_ERR", ra_err, false);
	getValue ("DEC_ERR", dec_err, false);
	getValue ("POS_ERR", img_err, false);
	// astrometry get !!
	getValue ("CRVAL1", pos_astr.ra, false);
	getValue ("CRVAL2", pos_astr.dec, false);
	getValue ("CROTA2", total_wcs[4], false);
}

void Image::getTargetHeaders ()
{
	// get IRAF info..
	targetName = new char[FLEN_VALUE];
	getValue ("OBJECT", targetName, FLEN_VALUE, NULL, verbose);
	// get info
	getValue ("TARGET", targetId, verbose);
	getValue ("TARSEL", targetIdSel, verbose);
	getValue ("TARTYPE", targetType, verbose);
	getValue ("OBSID", obsId, verbose);
	getValue ("IMGID", imgId, verbose);
}

void Image::setTargetHeaders (int _tar_id, int _obs_id, int _img_id, char _obs_subtype)
{
	targetId = _tar_id;
	targetIdSel = _tar_id;
	targetType = _obs_subtype;
	targetName = NULL;
	obsId = _obs_id;
	imgId = _img_id;
}

void Image::writeTargetHeaders (Rts2Target *target, bool set_values)
{
	if (set_values)
	{
		targetId = target->getTargetID ();
		targetIdSel = target->getObsTargetID ();
		targetType = target->getTargetType ();
		obsId = target->getObsId ();
		imgId = target->getNextImgId ();
	}

	setValue ("TARGET", targetId, "target id");
	setValue ("TARSEL", targetIdSel, "selector target id");
	setValue ("TARTYPE", targetType, "target type");
	setValue ("OBSID", obsId, "observation id");
	setValue ("IMGID", imgId, "image id");
	setValue ("PROC", 0, "image processing status");
}

void Image::openFile (const char *_filename, bool readOnly, bool _verbose)
{
  	verbose = _verbose;

	FitsFile::openFile (_filename, readOnly, _verbose);

	if (readOnly == false)
		flags |= IMAGE_SAVE;
	getHeaders ();
}

int Image::closeFile ()
{
	if (shouldSaveImage () && getFitsFile ())
	{
		try
		{
			// save astrometry error
			if (!std::isnan (ra_err) && !std::isnan (dec_err))
			{
				setValue ("RA_ERR", ra_err, "RA error in position");
				setValue ("DEC_ERR", dec_err, "DEC error in position");
				setValue ("POS_ERR", getAstrometryErr (), "error in position");
			}
			// save array data
			for (std::map <int, TableData *>::iterator iter = arrayGroups.begin (); iter != arrayGroups.end ();)
			{
				writeConnArray (iter->second);
				delete iter->second;
				arrayGroups.erase (iter++);
			}

			moveHDU (1);
			setCreationDate ();
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_WARNING) << "error saving " << getAbsoluteFileName () << ":" << er << sendLog;
		}
	}
	return FitsFile::closeFile ();
}

int Image::toQue ()
{
	return renameImageExpand (rts2core::Configuration::instance ()->observatoryQuePath ());
}

int Image::toAcquisition ()
{
	return renameImageExpand (rts2core::Configuration::instance ()->observatoryAcqPath ());
}

int Image::toArchive ()
{
	return renameImageExpand (rts2core::Configuration::instance ()->observatoryArchivePath ());
}

// move to dark images area..
int Image::toDark ()
{
	return renameImageExpand (rts2core::Configuration::instance ()->observatoryDarkPath ());
}

int Image::toFlat ()
{
	return 0;
	//return renameImageExpand (rts2core::Configuration::instance ()->observatoryFlatPath ());
}

int Image::toMasterFlat ()
{
	return renameImageExpand ("%b/flat/%c/master/%f");
}

int Image::toTrash ()
{
	return renameImageExpand (rts2core::Configuration::instance ()->observatoryTrashPath ());
}

img_type_t Image::getImageType ()
{
	char t_type[50];
	try
	{
		getValue ("IMAGETYP", t_type, 50, NULL, true);
	}
	catch (rts2core::Error &er)
	{
		return IMGTYPE_UNKNOW;
	}
	if (!strcmp (t_type, "flat"))
		return IMGTYPE_FLAT;
	if (!strcmp (t_type, "dark") || !(strcmp (t_type, "zero")))
	  	return IMGTYPE_DARK;
	return IMGTYPE_OBJECT;
}

int Image::renameImage (const char *new_filename)
{
	int ret = 0;
	if (strcmp (new_filename, getFileName ()))
	{
		logStream (MESSAGE_DEBUG) << "move " << getFileName () << " to " << new_filename << sendLog;
		saveImage ();
		ret = mkpath (new_filename, 0777);
		if (ret)
			return ret;
		ret = rename (getFileName (), new_filename);
		if (!ret)
		{
			openFile (new_filename);
		}
		else
		{
			// it is "problem" with external device..
			if (errno == EXDEV)
			{
				ret = copyImage (new_filename);
				if (ret)
				{
					logStream (MESSAGE_ERROR) << "Cannot copy image to external directory: " << ret << sendLog;
				}
				else
				{
					unlink (getFileName ());
					openFile (new_filename);
				}
			}
			else
			{
				logStream (MESSAGE_ERROR) << "Cannot move " << getFileName () << " to " << new_filename << ": " << strerror (errno) << sendLog;
			}
		}
	}
	return ret;
}

int Image::renameImageExpand (std::string new_ex)
{
	if (new_ex.length () == 0)
		return -1;
	std::string new_filename;

	const char *fn = getFileName ();

	if (!fn)
		return -1;

	size_t fl = strlen (fn);
	char* pb = new char[fl + 1];
	memcpy (pb, fn, fl + 1);

	oldImageName = std::string (basename (pb));

	delete[] pb;

	new_filename = expandPath (new_ex);
	return renameImage (new_filename.c_str ());
}

int Image::copyImage (const char *copy_filename)
{
	int ret = saveImage ();
	if (ret)
		return ret;
	ret = mkpath (copy_filename, 0777);
	if (ret)
		return ret;

	int fd_from = open (getAbsoluteFileName (), O_RDONLY);
	if (fd_from == -1)
	{
		logStream (MESSAGE_ERROR) << "Cannot open " << getAbsoluteFileName () << ": " << strerror (errno) << sendLog;
		return -1;
	}

	struct stat stb;
	ret = fstat (fd_from, &stb);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot get file mode " << getAbsoluteFileName () << ": " << strerror (errno) << sendLog;
		return -1;
	}

	int fd_to = open (copy_filename, O_WRONLY | O_CREAT | O_TRUNC, stb.st_mode);
	if (fd_to == -1)
	{
		logStream (MESSAGE_ERROR) << "Cannot create " << copy_filename << ": " << strerror (errno) << sendLog;
		close (fd_from);
		return -1;
	}

	char buf[4086];
	while (true)
	{
		ret = read (fd_from, buf, 4086);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			logStream (MESSAGE_ERROR) << "Cannot read from file: " << strerror (errno) << sendLog;
			break;
		}
		if (ret == 0)
		{
			close (fd_to);
			close (fd_from);
			return 0;
		}
		int wl;
		while (ret > 0)
		{
			wl = write (fd_to, buf, ret);
			if (wl < 0)
			{
				logStream (MESSAGE_ERROR) << "Cannot write to target file: " << strerror (errno) << sendLog;
				break;
			}
			ret -= wl;
		}
		if (wl < 0)
			break;
	}
	close (fd_to);
	close (fd_from);
	return -1;
}

int Image::copyImageExpand (std::string copy_ex)
{
	std::string copy_filename;

	const char *fn = getFileName ();

	if (!fn)
		return -1;

	size_t fl = strlen (fn);
	char* pb = new char[fl + 1];
	memcpy (pb, fn, fl + 1);

	oldImageName = std::string (basename (pb));

	delete[] pb;

	copy_filename = expandPath (copy_ex);
	return copyImage (copy_filename.c_str ());
}

int Image::symlinkImage (const char *link_filename)
{
	int ret;

	if (!getFileName ())
		return -1;

	ret = mkpath (link_filename, 0777);
	if (ret)
		return ret;
	return symlink (getFileName (), link_filename);
}

int Image::symlinkImageExpand (std::string link_ex)
{
	std::string link_filename;

	if (!getFileName ())
		return -1;

	link_filename = expandPath (link_ex);
	return symlinkImage (link_filename.c_str ());
}

int Image::linkImage (const char *link_filename)
{
	int ret;

	if (!getFileName ())
		return -1;

	ret = mkpath (link_filename, 0777);
	if (ret)
		return ret;
	return link (getFileName (), link_filename);
}

int Image::linkImageExpand (std::string link_ex)
{
	std::string link_filename;

	if (!getFileName ())
		return -1;

	link_filename = expandPath (link_ex);
	return linkImage (link_filename.c_str ());
}

int Image::writeExposureStart ()
{
	if (writeRTS2Values)
	{
		setValue ("CTIME", getCtimeSec (), "exposure start (seconds since 1.1.1970)");
		setValue ("USEC", getCtimeUsec (), "exposure start micro seconds");
		time_t tim = getCtimeSec ();
		setValue ("JD", getExposureJD (), "exposure JD");
		setValue ("DATE-OBS", &tim, getCtimeUsec (), "start of exposure");
	}
	return 0;
}

void Image::setAUXWCS (rts2core::StringArray * wcsaux)
{
	wcsauxs.clear ();
	for (std::vector <std::string>::iterator iter = wcsaux->valueBegin (); iter != wcsaux->valueEnd (); iter++)
		wcsauxs.push_back (*iter);
}

int Image::writeImgHeader (struct imghdr *im_h, int nchan)
{
	if (nchan != 1)
	{
		setValue ("INHERIT", true, "inherit key-values pairs from master HDU");
		setValue ("CHANNEL", ntohs (im_h->channel), "channel number");
	}
	if (templateFile)
	{
		// write header from template..
		std::ostringstream sec;
		sec << "CHANNEL" << ntohs (im_h->channel);

		rts2core::IniSection *hc = templateFile->getSection (sec.str ().c_str (), false);
		if (hc)
			writeTemplate (hc);
	}
	return 0;
}

void Image::writePrimaryHeader (const char *devname)
{
	if (templateFile)
	{
		templateDeviceName = new char[strlen (devname) + 1];
		strcpy (templateDeviceName, devname);

		rts2core::IniSection *hc = templateFile->getSection ("PRIMARY", false);
		if (hc)
			writeTemplate (hc);
	}
}

void Image::writeMetaData (struct imghdr *im_h)
{
	if (writeRTS2Values)
	{
		if (!getFitsFile () || !(flags & IMAGE_SAVE))
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "not saving data " << getFitsFile () << " " << (flags & IMAGE_SAVE) << sendLog;
			#endif					 /* DEBUG_EXTRA */
			return;
		}

		filter_i = ntohs (im_h->filter);

		setValue ("CAM_FILT", filter_i, "filter used for image");
		setValue ("SHUTTER", ntohs (im_h->shutter), "shutter state (0 - opened, 1 - closed)");
		// dark images don't need to wait till imgprocess will pick them up for reprocessing
		switch (ntohs (im_h->shutter))
		{
			case 0:
				shutter = SHUT_OPENED;
				break;
			case 1:
				shutter = SHUT_CLOSED;
				break;
			default:
				shutter = SHUT_UNKNOW;
		}
	}
}

void Image::writeWCS (double mods[NUM_WCS_VALUES])
{
	// write WCS values
	for (std::list <rts2core::ValueString>::iterator iter = string_wcs.begin (); iter != string_wcs.end (); iter++)
		writeConnBaseValue (iter->getName ().c_str (), &(*iter), iter->getDescription ().c_str ());

	const char *wcs_names[NUM_WCS_VALUES] = {"CRVAL1", "CRVAL2", "CRPIX1", "CRPIX2", "CDELT1", "CDELT2", "CROTA2"};
	const char *wcs_desc[NUM_WCS_VALUES] = {"reference value on 1st axis", "reference value on 2nd axis", "reference pixel of the 1st axis", "reference pixel of the 2nd axis", "delta along 1st axis", "delta along 2nd axis", "rotational angle"};
	for (int i = 0; i < NUM_WCS_VALUES; i++)
	{
		if (!std::isnan (total_wcs[i]))
		{
			double v = total_wcs[i];
			if (i == 4 || i == 5)
				v *= mods[i];
			// CRPIX is in detector (physical) coordinates, needs to transform to image
			else if ((i == 2 || i == 3) && mods[i + 2] != 0)
				v = v / mods[i + 2] + mods[i];
			else
				v += mods[i];
				
			setValue (multiWCS (wcs_names[i], wcs_multi_rotang) , v, wcs_desc[i]);
		}
	}
}

int Image::writeData (char *in_data, char *fullTop, int nchan)
{
	struct imghdr *im_h = (struct imghdr *) in_data;
	int ret;

	average = 0;
	avg_stdev = 0;

	// we have to copy data to FITS anyway, so let's do it right now..
	if (im_h->naxes != 2)
	{
		logStream (MESSAGE_ERROR) << "Image::writeDate not 2D image " << im_h->naxes << sendLog;
		return -1;
	}
	flags |= IMAGE_SAVE;
	dataType = ntohs (im_h->data_type);

	long sizes[2];
	sizes[0] = ntohl (im_h->sizes[0]);
	sizes[1] = ntohl (im_h->sizes[1]);

	long dataSize = (fullTop - in_data) - sizeof (struct imghdr);
	char *pixelData = in_data + sizeof (struct imghdr);

	Channel *ch;

	if (flags & IMAGE_KEEP_DATA)
	{
		ch = new Channel (ntohs (im_h->channel), pixelData, dataSize, 2, sizes, dataType);
	}
	else
	{
		ch = new Channel (ntohs (im_h->channel), pixelData, 2, sizes, dataType, false);
	}

	channels.push_back (ch);

	if (!getFitsFile () || !(flags & IMAGE_SAVE))
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "not saving data " << getFitsFile () << " " << (flags & IMAGE_SAVE) << sendLog;
		#endif					 /* DEBUG_EXTRA */
		return 0;
	}

	// either put it as a new extension, or keep it in primary..

	if (nchan == 1)
	{
		if (dataType == RTS2_DATA_SBYTE)
			fits_resize_img (getFitsFile (), RTS2_DATA_BYTE, 2, sizes, &fits_status);
		else
			fits_resize_img (getFitsFile (), dataType, 2, sizes, &fits_status);
		if (fits_status)
		{
			logStream (MESSAGE_ERROR) << "cannot resize image: " << getFitsErrors () << "dataType " << dataType << sendLog;
			return -1;
		}
	}
	else if (nchan > 1)
	{
		if (dataType == RTS2_DATA_SBYTE)
		{
			fits_create_img (getFitsFile (), RTS2_DATA_BYTE, 2, sizes, &fits_status);
		}
		else
		{
			fits_create_img (getFitsFile (), dataType, 2, sizes, &fits_status);
		}
		if (fits_status)
		{
			logStream (MESSAGE_ERROR) << "cannot create image: " << getFitsErrors () << "dataType " << dataType << sendLog;
			return -1;
		}
	}

	ret = writeImgHeader (im_h, abs (nchan));

	long pixelSize = dataSize / getPixelByteSize ();

	if (nchan > 0)
	{
		switch (dataType)
		{
			case RTS2_DATA_BYTE:
				fits_write_img_byt (getFitsFile (), 0, 1, pixelSize, (unsigned char *) pixelData, &fits_status);
				break;
			case RTS2_DATA_SHORT:
				fits_write_img_sht (getFitsFile (), 0, 1, pixelSize, (int16_t *) pixelData, &fits_status);
				break;
			case RTS2_DATA_LONG:
				fits_write_img_int (getFitsFile (), 0, 1, pixelSize, (int *) pixelData, &fits_status);
				break;
			case RTS2_DATA_LONGLONG:
				fits_write_img_lnglng (getFitsFile (), 0, 1, pixelSize, (LONGLONG *) pixelData, &fits_status);
				break;
			case RTS2_DATA_FLOAT:
				fits_write_img_flt (getFitsFile (), 0, 1, pixelSize, (float *) pixelData, &fits_status);
				break;
			case RTS2_DATA_DOUBLE:
				fits_write_img_dbl (getFitsFile (), 0, 1, pixelSize, (double *) pixelData, &fits_status);
				break;
			case RTS2_DATA_SBYTE:
				fits_write_img_sbyt (getFitsFile (), 0, 1, pixelSize, (signed char *) pixelData, &fits_status);
				break;
			case RTS2_DATA_USHORT:
				fits_write_img_usht (getFitsFile (), 0, 1, pixelSize, (short unsigned int *) pixelData, &fits_status);
				break;
			case RTS2_DATA_ULONG:
				fits_write_img_uint (getFitsFile (), 0, 1, pixelSize, (unsigned int *) pixelData, &fits_status);
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknow dataType " << dataType << sendLog;
				return -1;
		}
		if (fits_status)
		{
			logStream (MESSAGE_ERROR) << "cannot write data: " << getFitsErrors () << sendLog;
			return -1;
		}
	}

	if (writeRTS2Values)
	{
		ch->computeStatistics (0, pixelSize);

		setValue ("AVERAGE", ch->getAverage (), "average value of image");
		setValue ("STDEV", ch->getStDev (), "standard deviation value of image");
	}
	return ret;
}

void Image::getImgHeader (struct imghdr *im_h, int chan)
{
	int i;

	Channel *cha = channels[chan];

	im_h->data_type = htons (cha->getDataType ());

	int naxes = cha->getNaxis ();
	im_h->naxes = naxes;

	for (i = 0; i < naxes; i++)
	{
		im_h->sizes[i] = htonl(cha->getSize (i));
	}

	double ti;

	ti = 1;
	getValue ("LTM1_1", ti, false);
	if (ti != 0)
		im_h->binnings[0] = htons (1 / ti);
	else
		im_h->binnings[0] = htons (1);

	ti = 1;
	getValue ("LTM2_2", ti, false);
	if (ti != 0)
		im_h->binnings[1] = htons (1 / ti);
	else
		im_h->binnings[1] = htons (1);

	im_h->filter = ntohs (filter_i);
	im_h->shutter = 0;

	ti = 0;
	getValue ("LTV1", ti, false);
	im_h->x = htons (ti);

	ti = 0;
	getValue ("LTV2", ti, false);
	im_h->y = htons (ti);

	im_h->channel = htons (chan);
}

void Image::getHistogram (long *histogram, long nbins)
{
	memset (histogram, 0, nbins * sizeof(int));
	int bins;
	if (channels.size () == 0)
		loadChannels ();

	switch (dataType)
	{
		case RTS2_DATA_USHORT:
			bins = 65536 / nbins;
			for (Channels::iterator iter = channels.begin (); iter != channels.end (); iter++)
			{
				for (uint16_t *d = (uint16_t *)((*iter)->getData ()); d < ((uint16_t *)((*iter)->getData ())) + ((*iter)->getNPixels ()); d++)
				{
					histogram[*d / bins]++;
				}
			}
			break;
		case RTS2_DATA_FLOAT:
			bins = 65536 / nbins;
			for (Channels::iterator iter = channels.begin (); iter != channels.end (); iter++)
			{
				for (float *d = (float *)((*iter)->getData ()); d < ((float *)((*iter)->getData ())) + ((*iter)->getNPixels ()); d++)
				{
					histogram[((uint16_t)*d) / bins]++;
				}
			}
			break;
		default:
			break;
	}
}

void Image::getChannelHistogram (int chan, long *histogram, long nbins)
{
	memset (histogram, 0, nbins * sizeof(int));
	int bins;
	if (channels.size () == 0)
		loadChannels ();

	switch (dataType)
	{
		case RTS2_DATA_USHORT:
			bins = 65536 / nbins;
			for (uint16_t *d = (uint16_t *)(channels[chan]->getData ()); d < ((uint16_t *)(channels[chan]->getData ()) + channels[chan]->getNPixels ()); d++)
			{
				histogram[*d / bins]++;
			}
			break;
		case RTS2_DATA_FLOAT:
			bins = 65536 / nbins;
			for (float *d = (float *)(channels[chan]->getData ()); d < ((float *)(channels[chan]->getData ()) + channels[chan]->getNPixels ()); d++)
			{
				histogram[((uint16_t)*d) / bins]++;
			}
			break;
		default:
			break;
	}
}


template <typename bt, typename dt> void Image::getChannelGrayscaleByteBuffer (int chan, bt * &buf, bt black, dt low, dt high, long s, size_t offset, bool invert_y)
{
	if (buf == NULL)
		buf = new bt[s];
	
	bt *k = buf;

	if (invert_y)
		k += (getChannelWidth (chan) + offset) * (getChannelHeight (chan) - 1);

	const void *imageData = getChannelData (chan);

	int chw = getChannelWidth (chan);
	int j = chw;

	for (int i = 0; (long) i < s; i++)
	{
		bt n;
		dt pix = ((dt *)imageData)[i];
		if (pix <= low)
		{
			n = black;
		}
		else if (pix >= high)
		{
			n = 0;
		}
		else
		{
			// linear scaling
			n = black - black * ((double (pix - low)) / (high - low));
		}
		*k = n;
		k++;
		if (offset != 0 || invert_y)
		{
			j--;
			if (j == 0)
			{
				if (invert_y)
					k -= 2 * chw + offset;
				else
			  		k += offset;
				j = chw;
			}
		}
	}
}


template <typename bt, typename dt> void Image::getChannelGrayscaleBuffer (int chan, bt * &buf, bt black, dt minval, dt mval, float quantiles, size_t offset, bool invert_y)
{
	long hist[65536];
	getChannelHistogram (chan, hist, 65536);

	long psum = 0;
	dt low = minval;
	dt high = minval;

	uint32_t i;

	long s = getChannelNPixels (chan);

	// find quantiles
	for (i = 0; (dt) i < mval; i++)
	{
		psum += hist[i];
		if (psum > s * quantiles)
		{
			low = i;
			break;
		}
	}

	if (low == minval)
	{
		low = minval;
		high = mval;
	}
	else
	{
		for (; (dt) i < mval; i++)
		{
			psum += hist[i];
			if (psum > s * (1 - quantiles))
			{
				high = i;
				break;
			}
		}
		if (high == minval)
		{
			high = mval;
		}
	}

	getChannelGrayscaleByteBuffer (chan, buf, black, low, high, s, offset, invert_y);
}

void Image::getChannelGrayscaleImage (int _dataType, int chan, unsigned char * &buf, float quantiles, size_t offset)
{
	switch (_dataType)
	{
		case RTS2_DATA_BYTE:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (int8_t) 0, (int8_t) 255, quantiles, offset, true);
			break;
		case RTS2_DATA_SHORT:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (int16_t) SHRT_MIN, (int16_t) SHRT_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_LONG:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (int) INT_MIN, (int) INT_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_LONGLONG:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (LONGLONG) LLONG_MIN, (LONGLONG) LLONG_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_FLOAT:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (float) SHRT_MIN, (float) SHRT_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_DOUBLE:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (double) INT_MIN, (double) INT_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_SBYTE:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (signed char) SHRT_MIN, (signed char) SHRT_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_USHORT:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (short unsigned int) 0, (short unsigned int) USHRT_MAX, quantiles, offset, true);
			break;
		case RTS2_DATA_ULONG:
			getChannelGrayscaleBuffer (chan, buf, (unsigned char) 255, (unsigned int) 0, (unsigned int) UINT_MAX, quantiles, offset, true);
			break;
		default:
			logStream (MESSAGE_ERROR) << "Unknow dataType " << dataType << sendLog;
	}
}




template <typename bt, typename dt> void Image::getChannelPseudocolourByteBuffer (int chan, bt * &buf, bt black, dt low, dt high, long s, size_t offset, bool invert_y, int colourVariant)
{
	if (buf == NULL)
		buf = new bt[3 * s];
	
	double n;
	bt nR, nG, nB;

	bt *k = buf;

	if (invert_y)
		k += 3 * (getChannelWidth (chan) + offset) * (getChannelHeight (chan) - 1);

	const void *imageData = getChannelData (chan);

	int chw = getChannelWidth (chan);
	int j = chw;

	for (int i = 0; (long) i < s; i++)
	{
		dt pix = ((dt *)imageData)[i];

		if ( pix < low )
			pix = low;
		if ( pix > high )
			pix = high;

		switch (colourVariant)
		{
			case PSEUDOCOLOUR_VARIANT_GREY:
				n = 255.0 * double (pix - low) / double (high - low);
				nR = n;
				nG = nR;
				nB = nR;
				break;
			case PSEUDOCOLOUR_VARIANT_GREY_INV:
				n = 255.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = n;
				nG = nR;
				nB = nR;
				break;
			case PSEUDOCOLOUR_VARIANT_BLUE:
				n = 511.0 * double (pix - low) / double (high - low);
				nR = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nG = n / 2.0;
				nB = (n < 256.0) ? n : 255;
				break;
			case PSEUDOCOLOUR_VARIANT_BLUE_INV:
				n = 511.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nG = n / 2.0;
				nB = (n < 256.0) ? n : 255;
				break;
			case PSEUDOCOLOUR_VARIANT_RED:
				n = 511.0 * double (pix - low) / double (high - low);
				nR = (n < 256.0) ? n : 255;
				nG = n / 2.0;
				nB = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				break;
			case PSEUDOCOLOUR_VARIANT_RED_INV:
				n = 511.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = (n < 256.0) ? n : 255;
				nG = n / 2.0;
				nB = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				break;
			case PSEUDOCOLOUR_VARIANT_GREEN:
				n = 511.0 * double (pix - low) / double (high - low);
				nR = n / 2.0;
				nG = (n < 256.0) ? n : 255;
				nB = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				break;
			case PSEUDOCOLOUR_VARIANT_GREEN_INV:
				n = 511.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = n / 2.0;
				nG = (n < 256.0) ? n : 255;
				nB = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				break;
			case PSEUDOCOLOUR_VARIANT_VIOLET:
				n = 511.0 * double (pix - low) / double (high - low);
				nR = n / 2.0;
				nG = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nB = (n < 256.0) ? n : 255;
				break;
			case PSEUDOCOLOUR_VARIANT_VIOLET_INV:
				n = 511.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = n / 2.0;
				nG = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nB = (n < 256.0) ? n : 255;
				break;
			case PSEUDOCOLOUR_VARIANT_MAGENTA:
				n = 511.0 * double (pix - low) / double (high - low);
				nR = (n < 256.0) ? n : 255;
				nG = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nB = n / 2.0;
				break;
			case PSEUDOCOLOUR_VARIANT_MAGENTA_INV:
				n = 511.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = (n < 256.0) ? n : 255;
				nG = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nB = n / 2.0;
				break;
			case PSEUDOCOLOUR_VARIANT_MALACHIT:
				n = 511.0 * double (pix - low) / double (high - low);
				nR = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nG = (n < 256.0) ? n : 255;
				nB = n / 2.0;
				break;
			case PSEUDOCOLOUR_VARIANT_MALACHIT_INV:
				n = 511.0 * ( 1.0 - double (pix - low) / double (high - low) );
				nR = ((n - 256.0) > 0.0) ? n - 256.0 : 0;
				nG = (n < 256.0) ? n : 255;
				nB = n / 2.0;
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknown colourVariant" << colourVariant << sendLog;
		}

		*k = nR;
		k++;
		*k = nG;
		k++;
		*k = nB;
		k++;
		if (offset != 0 || invert_y)
		{
			j--;
			if (j == 0)
			{
				if (invert_y)
					k -= 3 * (2 * chw + offset);
				else
			  		k += 3 * offset;
				j = chw;
			}
		}
	}
}


template <typename bt, typename dt> void Image::getChannelPseudocolourBuffer (int chan, bt * &buf, bt black, dt minval, dt mval, float quantiles, size_t offset, bool invert_y, int colourVariant)
{
	long hist[65536];
	getChannelHistogram (chan, hist, 65536);

	long psum = 0;
	dt low = minval;
	dt high = minval;

	uint32_t i;

	long s = getChannelNPixels (chan);

	// find quantiles
	for (i = 0; (dt) i < mval; i++)
	{
		psum += hist[i];
		if (psum > s * quantiles)
		{
			low = i;
			break;
		}
	}

	if (low == minval)
	{
		low = minval;
		high = mval;
	}
	else
	{
		for (; (dt) i < mval; i++)
		{
			psum += hist[i];
			if (psum > s * (1 - quantiles))
			{
				high = i;
				break;
			}
		}
		if (high == minval)
		{
			high = mval;
		}
	}

	getChannelPseudocolourByteBuffer (chan, buf, black, low, high, s, offset, invert_y, colourVariant);
}


void Image::getChannelPseudocolourImage (int _dataType, int chan, unsigned char * &buf, float quantiles, size_t offset, int colourVariant)
{
	switch (_dataType)
	{
		case RTS2_DATA_BYTE:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (int8_t) 0, (int8_t) 255, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_SHORT:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (int16_t) SHRT_MIN, (int16_t) SHRT_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_LONG:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (int) INT_MIN, (int) INT_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_LONGLONG:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (LONGLONG) LLONG_MIN, (LONGLONG) LLONG_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_FLOAT:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (float) INT_MIN, (float) INT_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_DOUBLE:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (double) INT_MIN, (double) INT_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_SBYTE:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (signed char) SHRT_MIN, (signed char) SHRT_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_USHORT:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (short unsigned int) 0, (short unsigned int) USHRT_MAX, quantiles, offset, true, colourVariant);
			break;
		case RTS2_DATA_ULONG:
			getChannelPseudocolourBuffer (chan, buf, (unsigned char) 255, (unsigned int) 0, (unsigned int) UINT_MAX, quantiles, offset, true, colourVariant);
			break;
		default:
			logStream (MESSAGE_ERROR) << "Unknow dataType " << dataType << sendLog;
	}
}

#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1
Magick::Image *Image::getMagickImage (const char *label, float quantiles, int chan, int colourVariant)
{
	unsigned char *buf = NULL;
	Magick::Image *image = NULL;
	try
	{
		int tw = 0;
		int th = 0;
		int maxh = 0;

	  	if (chan >= 0)
		{
			if (channels.size () == 0)
				loadChannels ();
			if ((size_t) chan >= channels.size ())
				throw rts2core::Error ("invalid channel specified");

			if (colourVariant == PSEUDOCOLOUR_VARIANT_GREY)
				getChannelGrayscaleImage (dataType, chan, buf, quantiles, 0);
			else
				getChannelPseudocolourImage (dataType, chan, buf, quantiles, 0, colourVariant);
				
		  	// single channel
			tw = getChannelWidth (chan);
			th = getChannelHeight (chan);
		}
		else
		{
		  	if (channels.size () == 0)
			  	loadChannels ();
			// all channels
			int w = floor (sqrt (channels.size ()));
			if (w <= 0)
				w = 1;
			int lw = 0;
			int lh = 0;
			int n = 0;

			// retrieve total image size
			for (Channels::iterator iter = channels.begin (); iter != channels.end (); iter++, n++)
			{
			  	if (n % w == 0)
				{
				  	if (lw > tw)
					  	tw = lw;
					th ++;
				  	lw = 0;
					lh = 0;
				}
				lw += (*iter)->getWidth ();
				if ((*iter)->getHeight () > lh)
				  	lh = (*iter)->getHeight ();
				if (lh > maxh)
					maxh = lh;

			}

			if (n % w == 0)
			{
				if (lw > tw)
				  	tw = lw;
			}

			// change total height from segments to pixels
			th = maxh * th;

			// copy grayscales
			if (colourVariant == PSEUDOCOLOUR_VARIANT_GREY)
				buf = new unsigned char[tw * th];
			else
				buf = new unsigned char[3 * tw * th];

			lw = 0;
			lh = 0;
			if (channels.size () == 1)
				n = 0;
			else
				n = 2;

			int loff = tw;

			for (Channels::iterator iter = channels.begin (); iter != channels.end (); iter++, n = (n + 1) % 4)
			{
			  	if (iter != channels.begin () && (n % w == 0))
				{
				  	lh += maxh;
					lw = 0;
					loff = tw - (*iter)->getWidth ();
				}
				else if (iter == channels.begin ())
				{
					loff = tw - (*iter)->getWidth ();
				}

				size_t offset = tw - (*iter)->getWidth ();


				if (colourVariant == PSEUDOCOLOUR_VARIANT_GREY)
				{
					unsigned char *bstart = buf + lh * tw + loff;
				  	getChannelGrayscaleImage (dataType, (*iter)->getChannelNumber (), bstart, quantiles, offset);
				}
				else
				{
					unsigned char *bstart = buf + 3 * (lh * tw + loff);
				  	getChannelPseudocolourImage (dataType, (*iter)->getChannelNumber (), bstart, quantiles, offset, colourVariant);
				}

				if ((*iter)->getHeight () > lw)
				  	lw = (*iter)->getHeight ();
				loff -= (*iter)->getWidth ();	
			}

		}

		if (colourVariant == PSEUDOCOLOUR_VARIANT_GREY)
			image = new Magick::Image (tw, th, "K", Magick::CharPixel, buf);
		else
			image = new Magick::Image (tw, th, "RGB", Magick::CharPixel, buf);

		if (label && label[0] != '\0')
		{
			image->font("helvetica");
			image->strokeColor (Magick::Color (MaxRGB, MaxRGB, MaxRGB));
			image->fillColor (Magick::Color (MaxRGB, MaxRGB, MaxRGB));

			writeLabel (image, 2, image->size ().height () - 2, 20, label);
		}

		delete[] buf;
		return image;
	}
	catch (Magick::Exception &ex)
	{
		delete[] buf;
		delete image;
		throw ex;
	}
}

void Image::writeLabel (Magick::Image *mimage, int x, int y, unsigned int fs, const char *labelText)
{
	// no label, no work
	if (labelText == NULL || labelText[0] == '\0')
		return;
	mimage->fontPointsize (fs);
	mimage->fillColor (Magick::Color (0, 0, 0, MaxRGB / 2));
	mimage->draw (Magick::DrawableRectangle (x, y - fs - 4, mimage->size (). width () - x - 2, y));

	mimage->fillColor (Magick::Color (MaxRGB, MaxRGB, MaxRGB));
	mimage->draw (Magick::DrawableText (x + 2, y - 3, expand (labelText)));
}

void Image::writeAsJPEG (std::string expand_str, double zoom, const char *label, float quantiles , int chan, int colourVariant)
{
	std::string new_filename = expandPath (expand_str);
	
	int ret = mkpath (new_filename.c_str (), 0777);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot create directory for file '" << new_filename << "'" << sendLog;
		throw rts2core::Error ("Cannot create directory");
	}

	Magick::Image *image = NULL;

	try {
		image = getMagickImage (NULL, quantiles, chan, colourVariant);
		if (zoom != 1.0)
			image->zoom (Magick::Geometry (image->size ().height () * zoom, image->size ().width () * zoom));
		writeLabel (image, 2, image->size ().height () - 2, 20, label);
		image->write (new_filename.c_str ());
		delete image;
	}
	catch (Magick::Exception &ex)
	{
		logStream (MESSAGE_ERROR) << "Cannot create image " << new_filename << ", " << ex.what () << sendLog;
		delete image;
		throw ex;
	}
}

void Image::writeAsBlob (Magick::Blob &blob, const char * label, float quantiles, int chan, int colourVariant)
{
	Magick::Image *image = NULL;
	try
	{
		image = getMagickImage (label, quantiles, chan, colourVariant);
		image->write (&blob, "jpeg");
		delete image;
	}
	catch (Magick::Exception &ex)
	{
		logStream (MESSAGE_ERROR) << "Cannot create image " << ex.what () << sendLog;
		delete image;
		throw ex;
	}
}

#endif /* RTS2_HAVE_LIBJPEG */

double Image::getAstrometryErr ()
{
	struct ln_equ_posn pos2;
	pos2.ra = pos_astr.ra + ra_err;
	pos2.dec = pos_astr.dec + dec_err;
	img_err = ln_get_angular_separation (&pos_astr, &pos2);
	return img_err;
}

int Image::saveImage ()
{
	return closeFile ();
}

int Image::deleteImage ()
{
	int ret;
	fits_close_file (getFitsFile (), &fits_status);
	setFitsFile (NULL);
	flags &= ~IMAGE_SAVE;
	ret = unlink (getFileName ());
	return ret;
}

void Image::setMountName (const char *in_mountName)
{
	delete[]mountName;
	mountName = new char[strlen (in_mountName) + 1];
	strcpy (mountName, in_mountName);
	if (writeRTS2Values)
		setValue ("MNT_NAME", mountName, "name of mount");
}

void Image::setCameraName (const char *new_name)
{
	delete[]cameraName;
	cameraName = new char[strlen (new_name) + 1];
	strcpy (cameraName, new_name);
}

std::string Image::getTargetName ()
{
	if (targetName == NULL || *targetName == '\0')
		getTargetHeaders ();
	return std::string (targetName);
}

std::string Image::getTargetIDString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (5) << getTargetId ();
	return _os.str ();
}

std::string Image::getTargetIDSelString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (5) << getTargetIdSel ();
	return _os.str ();
}

std::string Image::getExposureNumberString ()
{
	std::ostringstream _os;
	_os << expNum;
	return _os.str ();
}

std::string Image::getObsString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (5) << getObsId ();
	return _os.str ();
}

std::string Image::getImgIdString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (5) << getImgId ();
	return _os.str ();
}

// Image can be only raw..
std::string Image::getProcessingString ()
{
	return std::string ("RA");
}

std::string Image::getExposureLengthString ()
{
	std::ostringstream _os;
	_os << getExposureLength ();
	return _os.str ();
}

int Image::getImgId ()
{
	try
	{
		if (imgId < 0)
			getTargetHeaders ();
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "cannot read image header " << er << sendLog;
	}
	return imgId;
}

void Image::setFocuserName (const char *in_focuserName)
{
	if (focName)
		delete[]focName;
	focName = new char[strlen (in_focuserName) + 1];
	strcpy (focName, in_focuserName);
	setValue ("FOC_NAME", focName, "name of focuser");
}

void Image::setFilter (const char *in_filter)
{
	filter = new char[strlen (in_filter) + 1];
	strcpy (filter, in_filter);
	if (writeRTS2Values)
		setValue ("FILTER", filter, "camera filter as string");
}

void Image::computeStatistics (size_t _from, size_t _dataSize)
{
	if (channels.size () == 0)
		loadChannels ();

	pixelSum = 0;
	long totalSize = 0;

	avg_stdev = 0;

	for (Channels::iterator iter = channels.begin (); iter != channels.end (); iter++)
	{
		(*iter)->computeStatistics (_from, _dataSize);

		totalSize += (*iter)->getNPixels ();
		pixelSum += (*iter)->getPixelSum ();
		avg_stdev += (*iter)->getStDev ();
	}

	if (totalSize > 0)
	{
		average = pixelSum / totalSize;
		avg_stdev /= channels.size ();
	}
}

void Image::loadChannels ()
{
	// try to load data..
	int anyNull = 0;
	if (!getFitsFile ())
		openFile (NULL, false, true);

	dataType = 0;
	int hdutype;

	// get number of channels
	fits_status = 0;
	int tothdu = getTotalHDUs ();
	tothdu++;
	tothdu *= -1;

	int hdunum = 0;

	// open all channels
	while (fits_status == 0)
	{
	  	if (tothdu < 0)
		{
			// first HDU is already opened..
			moveHDU (1, &hdutype);
			tothdu *= -1;
			tothdu--;
		}
		else if (tothdu > 1)
		{

			fits_movrel_hdu (getFitsFile (), 1, &hdutype, &fits_status);
			tothdu--;
		}
		else
		{
		  	break;
		}
		// first check that it is image
		if (hdutype != IMAGE_HDU)
			continue;

		// check that it has some axis..
		int naxis = 0;
		getValue ("NAXIS", naxis, true);
		if (naxis == 0)
			continue;

		// check it type (bits per pixel)
		int it;
		fits_get_img_equivtype (getFitsFile (), &it, &fits_status);
		if (fits_status)
		{
			logStream (MESSAGE_ERROR) << "cannot retrieve image type: " << getFitsErrors () << sendLog;
			return;
		}
		if (dataType == 0)
		{
			dataType = it;
		}
		else if (dataType != it)
		{
			logStream (MESSAGE_ERROR) << getFileName () << " has extension with different image type, which is not supported" << sendLog;
			return;
		}

		// get its size..
		long sizes[naxis];
		getValues ("NAXIS", sizes, naxis, true);

		long pixelSize = sizes[0];
		for (int i = 1; i < naxis; i++)
			pixelSize *= sizes[i];

		char *imageData = new char[pixelSize * getPixelByteSize ()];
		switch (dataType)
		{
			case RTS2_DATA_BYTE:
				fits_read_img_byt (getFitsFile (), 0, 1, pixelSize, 0, (unsigned char *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_SHORT:
				fits_read_img_sht (getFitsFile (), 0, 1, pixelSize, 0, (int16_t *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_LONG:
				fits_read_img_int (getFitsFile (), 0, 1, pixelSize, 0, (int *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_LONGLONG:
				fits_read_img_lnglng (getFitsFile (), 0, 1, pixelSize, 0, (LONGLONG *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_FLOAT:
				fits_read_img_flt (getFitsFile (), 0, 1, pixelSize, 0, (float *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_DOUBLE:
				fits_read_img_dbl (getFitsFile (), 0, 1, pixelSize, 0, (double *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_SBYTE:
				fits_read_img_sbyt (getFitsFile (), 0, 1, pixelSize, 0, (signed char *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_USHORT:
				fits_read_img_usht (getFitsFile (), 0, 1, pixelSize, 0,(short unsigned int *) imageData, &anyNull, &fits_status);
				break;
			case RTS2_DATA_ULONG:
				fits_read_img_uint (getFitsFile (), 0, 1, pixelSize, 0, (unsigned int *) imageData, &anyNull, &fits_status);
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknow dataType " << dataType << sendLog;
				delete[] imageData;
				dataType = 0;
				throw ErrorOpeningFitsFile (getFileName ());
		}
		if (fits_status)
		{
			delete[] imageData;
			dataType = 0;
			throw ErrorOpeningFitsFile (getFileName ());
		}
		fitsStatusGetValue ("image loadChannels", true);

		int ch;
		try
		{
			getValue ("CHANNEL", ch, true);
			ch -= 1;
		}
		catch (KeyNotFound &er)
		{
			char extn[50];
			// extension name
			try
			{
				getValue ("EXTNAME", extn, 50, NULL, false);
				if (extn[0] != '\0' && extn[0] == 'I' && extn[1] == 'M' && isdigit (extn[2]))
				{
					ch = atoi (extn + 2) - 1;
					if (ch < 0 || ch > getTotalHDUs ())
						ch = hdunum;
				}
				else
				{
					ch = hdunum;
				}
			}
			catch (KeyNotFound &er1)
			{
				ch = hdunum;
			}
		}
		channels.push_back (new Channel (ch, imageData, naxis, sizes, dataType, true));
		hdunum++;
	}
	moveHDU (1);
}

const void* Image::getChannelData (int chan)
{
	if (channels.size () == 0)
	{
		try
		{
			loadChannels ();
		}
		catch (rts2core::Error er)
		{
			logStream (MESSAGE_ERROR) << er << sendLog;
			return NULL;
		}
	}
	return channels[chan]->getData ();
}

template <typename bt, typename dt> const bt * scaleData (dt * data, size_t numpix, dt smin, dt smax, scaling_type scaling, bt white)
{
	dt * end = data + numpix;
	dt * p = data;
	// overwrite data..
	bt *nd = (bt *) data;

	double l = smax - smin;

// scaling macro to keep switch outside from main loop
#define doscaling(scaling_func)                 \
	for (; p < end; p++, nd++)              \
	{                                       \
		if (*p < smin)                  \
		{                               \
			*nd = 0;                \
			continue;               \
		}                               \
		if (*p > smax)                  \
		{                               \
			*nd = white;            \
			continue;               \
		}                               \
		double d = *p;                  \
		d = white * (d - smin) / l;     \
		scaling_func;                   \
		*nd = (bt) d;                   \
	}

	switch (scaling)
	{
		case SCALING_LINEAR:
			doscaling(;)
			break;
		case SCALING_LOG:
			doscaling(d=log(d))
			break;
		case SCALING_POW:
			doscaling(d*=d)
			break; 
		case SCALING_SQRT:
			doscaling(d=sqrt (d))
			break;
	}

#undef doscaling

	return (bt *) data;
}

const void * rts2image::getScaledData (int dataType, const void *data, size_t numpix, long smin, long smax, scaling_type scaling, int newType)
{
	switch (dataType)
	{
		case RTS2_DATA_USHORT:
			return scaleData ((uint16_t *) data, numpix, (uint16_t) smin, (uint16_t) smax, scaling, (uint8_t) 0xff);
		case RTS2_DATA_ULONG:
			switch (newType)
			{
				case RTS2_DATA_BYTE:
					return scaleData ((uint32_t *) data, numpix, (uint32_t) smin, (uint32_t) smax, scaling, (uint8_t) 0xff);
				case RTS2_DATA_USHORT:
					return scaleData ((uint32_t *) data, numpix, (uint32_t) smin, (uint32_t) smax, scaling, (uint16_t) 0xffff);
			}
			return scaleData ((uint32_t *) data, numpix, (uint32_t) smin, (uint32_t) smax, scaling, (uint8_t) 0xff);
	}
	return data;
}

const void * Image::getChannelDataScaled (int chan, long smin, long smax, scaling_type scaling, int newType)
{
	return getScaledData (dataType, getChannelData (chan), getChannelNPixels (chan), smin, smax, scaling, newType);
}

int Image::setAstroResults (double in_ra, double in_dec, double in_ra_err, double in_dec_err)
{
	pos_astr.ra = in_ra;
	pos_astr.dec = in_dec;
	ra_err = in_ra_err;
	dec_err = in_dec_err;
	flags |= IMAGE_SAVE;
	return 0;
}

int Image::addStarData (struct stardata *sr)
{
	if (sr->F <= 0)				 // flux is not significant..
		return -1;
	// do not take stars on edge..
	if (sr->X == 0 || sr->X == getChannelWidth (0) || sr->Y == 0 || sr->Y == getChannelHeight (0))
		return -1;

	sexResultNum++;
	if (sexResultNum > 1)
	{
		sexResults =
			(struct stardata *) realloc ((void *) sexResults,
			sexResultNum * sizeof (struct stardata));
	}
	else
	{
		sexResults = (struct stardata *) malloc (sizeof (struct stardata));
	}
	sexResults[sexResultNum - 1] = *sr;
	return 0;
}

static int shortcompare (const void *val1, const void *val2)
{
	return (*(unsigned short *) val1 < *(unsigned short *) val2) ? -1 :
	((*(unsigned short *) val1 == *(unsigned short *) val2) ? 0 : 1);
}

unsigned short getShortMean (unsigned short *averageData, int sub)
{
	qsort (averageData, sub, sizeof (unsigned short), shortcompare);
	if (sub % 2)
		return averageData[sub / 2];
	return (averageData[(sub / 2) - 1] + averageData[(sub / 2)]) / 2;
}

long Image::getSumNPixels ()
{
	long ret = 0;
	for (Channels::iterator iter = channels.begin (); iter != channels.end (); iter++)
		ret += (*iter)->getNPixels ();
	return ret;
}

int Image::getError (double &eRa, double &eDec, double &eRad)
{
	if (std::isnan (ra_err) || std::isnan (dec_err) || std::isnan (img_err))
		return -1;
	eRa = ra_err;
	eDec = dec_err;
	eRad = img_err;
	return 0;
}

std::string Image::getOnlyFileName ()
{
	return expandPath ("%y%m%d%H%M%S-%s-%p.fits");
}

void Image::printFileName (std::ostream & _os)
{
	_os << getFileName ();
}

void Image::print (std::ostream & _os, int in_flags, const char *imageFormat)
{
	if (in_flags & DISPLAY_FILENAME)
	{
		printFileName (_os);
		_os << SEP;
	}
	if (in_flags & DISPLAY_SHORT)
	{
		printFileName (_os);
		if (imageFormat)
		{
			_os << SEP << expand (std::string (imageFormat));
		}
		_os << std::endl;
		return;
	}

	if (in_flags & DISPLAY_OBS)
		_os << std::setw (5) << getObsId ();

	if (!(in_flags & DISPLAY_ALL))
	{
		if (imageFormat)
			_os << SEP << expand (std::string (imageFormat));
		return;
	}

	std::ios_base::fmtflags old_settings = _os.flags ();
	int old_precision = _os.precision (2);

	_os
		<< SEP
		<< std::setw (5) << getCameraName () << SEP
		<< std::setw (4) << getImgId () << SEP
		<< Timestamp (getExposureSec () + (double) getExposureUsec () /	USEC_SEC) << SEP
		<< std::setw (10) << getFilter () << SEP
		<< std::setw (8) << std::fixed << TimeDiff (getExposureLength ()) << SEP
		<< LibnovaDegArcMin (ra_err) << SEP << LibnovaDegArcMin (dec_err) << SEP
		<< LibnovaDegArcMin (img_err);

	if (imageFormat)
		_os << SEP << expand (std::string (imageFormat));

	_os << std::endl;

	_os.flags (old_settings);
	_os.precision (old_precision);
}

void Image::writeConnBaseValue (const std::string sname, rts2core::Value * val, const std::string sdesc)
{
	const char *desc = sdesc.c_str ();
	const char *name = sname.c_str ();
	switch (val->getValueBaseType ())
	{
		case RTS2_VALUE_STRING:
			{
				switch (val->getValueDisplayType ())
				{
					case RTS2_DT_HISTORY:
						if (strlen (val->getValue ()))
							writeHistory (val->getValue ());
						break;
					case RTS2_DT_COMMENT:
						if (strlen (val->getValue ()))
							writeComment (val->getValue ());
						break;
					default:
						setValue (name, val->getValue (), desc);
				}
			}
			break;
		case RTS2_VALUE_INTEGER:
			setValue (name, val->getValueInteger (), desc);
			break;
		case RTS2_VALUE_TIME:
			setValue (name, val->getValueDouble (), desc);
			break;
		case RTS2_VALUE_DOUBLE:
			switch (val->getValueDisplayType ())
			{
				case RTS2_DT_RA:
					if (rts2core::Configuration::instance ()->getStoreSexadecimals ())
					{
						std::ostringstream os;
						os << LibnovaRa (val->getValueDouble ());
						setValue (name, os.str ().c_str (), desc);
						break;
					}
				default:
					setValue (name, val->getValueDouble (), desc);
					break;
			}
			break;
		case RTS2_VALUE_FLOAT:
			setValue (name, val->getValueFloat (), desc);
			break;
		case RTS2_VALUE_BOOL:
			setValue (name, ((rts2core::ValueBool *) val)->getValueBool (), desc);
			break;
		case RTS2_VALUE_SELECTION:
			setValue (name, ((rts2core::ValueSelection *) val)->getSelName (), desc);
			break;
		case RTS2_VALUE_LONGINT:
			setValue (name, ((rts2core::ValueLong *) val)->getValueLong (), desc);
			break;
		case RTS2_VALUE_RADEC:
			{
				// construct RADEC string and desc, write it down
				char *v_name = new char[strlen (name) + 4];
				char *v_desc = new char[strlen (desc) + 5];
				// write RA
				strcpy (v_name, name);
				strcat (v_name, "RA");
				strcpy (v_desc, desc);
				strcat (v_desc, " RA");
				if (rts2core::Configuration::instance ()->getStoreSexadecimals ())
				{
					std::ostringstream _ra;
					_ra << LibnovaRa (((rts2core::ValueRaDec *) val)->getRa ());
					setValue (v_name, _ra.str ().c_str (), v_desc);
				}
				else
				{
					setValue (v_name, ((rts2core::ValueRaDec *) val)->getRa (), v_desc);
				}

				// now DEC
				strcpy (v_name, name);
				strcat (v_name, "DEC");
				strcpy (v_desc, desc);
				strcat (v_desc, " DEC");
				if (rts2core::Configuration::instance ()->getStoreSexadecimals ())
				{
					std::ostringstream _dec;
					_dec << LibnovaDec (((rts2core::ValueRaDec *) val)->getDec ());
					setValue (v_name, _dec.str ().c_str (), v_desc);
				}
				else
				{
					setValue (v_name, ((rts2core::ValueRaDec *) val)->getDec (), v_desc);
				}
				// if it is mount ra dec - write heliocentric time
				if (!strcmp ("TEL", name))
				{
					double JD = getMidExposureJD ();
					struct ln_equ_posn equ;
					equ.ra = ((rts2core::ValueRaDec *) val)->getRa ();
					equ.dec = ((rts2core::ValueRaDec *) val)->getDec ();
					setValue ("JD_HELIO", JD + ln_get_heliocentric_time_diff (JD, &equ), "heliocentric JD");
				}

				// free memory
				delete[] v_name;
				delete[] v_desc;
			}
			break;
		case RTS2_VALUE_ALTAZ:
			{
				// construct RADEC string and desc, write it down
				char *v_name = new char[strlen (name) + 4];
				char *v_desc = new char[strlen (desc) + 10];
				// write RA
				strcpy (v_name, name);
				strcat (v_name, "ALT");
				strcpy (v_desc, desc);
				strcat (v_desc, " altitude");
				setValue (v_name, ((rts2core::ValueAltAz *) val)->getAlt (), v_desc);
				// now DEC
				strcpy (v_name, name);
				strcat (v_name, "AZ");
				strcpy (v_desc, desc);
				strcat (v_desc, " azimuth");
				setValue (v_name, ((rts2core::ValueAltAz *) val)->getAz (), v_desc);
				// free memory
				delete[] v_name;
				delete[] v_desc;
			}
			break;
		case RTS2_VALUE_PID:
			setValue (name, val->getDisplayValue (), desc);
			break;
		default:
			logStream (MESSAGE_ERROR) << "unable to write to FITS file header value '" << name << "' of type " << val->getValueType () << sendLog;
			break;
	}
}

ColumnData *getColumnData (const char *name, rts2core::Value * val)
{
	switch  (val->getValueBaseType ())
	{
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_TIME:
			return new ColumnData (name, ((rts2core::DoubleArray *) val)->getValueVector ());
			break;
		case RTS2_VALUE_INTEGER:
			return new ColumnData (name, ((rts2core::IntegerArray *) val)->getValueVector (), false);
			break;
		case RTS2_VALUE_BOOL:
			return new ColumnData (name, ((rts2core::IntegerArray *) val)->getValueVector (), true);
			break;
	}
	throw rts2core::Error ("unknow array datatype");
}

void Image::prepareArrayData (const std::string sname, rts2core::Connection *conn, rts2core::Value *val)
{
	const char *name = sname.c_str ();
	// if it's simple array, just write as header cards
	if (val->onlyFitsHeader ())
	{
		int s = (int)(((rts2core::ValueArray *)val)->size ());
		if (!val->prefixWithDevice ())
			name = val->getName ().c_str ();
		setValue (name, s, val->getDescription ().c_str ());
		size_t l = strlen (name);
		char *indexname = new char[l + (s / 10) + 2];
		memcpy (indexname, name, l + 1);
		char *ip = indexname + strlen(name);
		for (int i = 0; i < s; i++)
		{
			sprintf (ip, "%d", i + 1);
			switch (val->getValueBaseType ())
			{
				case RTS2_VALUE_DOUBLE:
					setValue (indexname, (*((rts2core::DoubleArray *)val))[i],"");
					break;
				case RTS2_VALUE_TIME:
					setValue (indexname, (*((rts2core::TimeArray *)val))[i],"");
					break;
				case RTS2_VALUE_INTEGER:
					setValue (indexname, (*((rts2core::IntegerArray *)val))[i],"");
					break;
				case RTS2_VALUE_STRING:
					setValue (indexname, (*((rts2core::StringArray *)val))[i].c_str (),"");
					break;
				case RTS2_VALUE_BOOL:
					setValue (indexname, (*((rts2core::BoolArray *)val))[i],"");
					break;
			}
		}
		delete[] indexname;
		return;
	}
	// otherwise, prepare data structure to be written
	std::map <int, TableData *>::iterator ai = arrayGroups.find (val->getWriteGroup ());
	
	if (ai == arrayGroups.end ())
	{
		rts2core::Value *infoTime = conn->getValue (RTS2_VALUE_INFOTIME);
		if (infoTime)
		{
			TableData *td = new TableData (name, infoTime->getValueDouble ());
			td->push_back (getColumnData (name, val));
			arrayGroups[val->getWriteGroup ()] = td;
		}
	}
	else
	{
		ai->second->push_back (getColumnData (name, val));
	}
}

void Image::writeConnArray (TableData *tableData)
{
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	writeArray (tableData->getName (), tableData);
	setValue ("TSTART", tableData->getDate (), "data are recorded from this time");
}

void Image::writeConnValue (rts2core::Connection * conn, rts2core::Value * val)
{
	std::string desc = val->getDescription ();
	std::string name = val->getName ();
	char *name_stat;
	char *n_top;

	// array groups to write. First they are created, then they are written at the end

	if (conn->getOtherType () == DEVICE_TYPE_SENSOR || conn->getOtherType () == DEVICE_TYPE_ROTATOR || val->prefixWithDevice () || val->getValueExtType () == RTS2_VALUE_ARRAY)
	{
		name = std::string (conn->getName ()) + "." + val->getName ();
	}

	switch (val->getValueExtType ())
	{
		case 0:
			writeConnBaseValue (name, val, desc);
			break;
		case RTS2_VALUE_ARRAY:
			prepareArrayData (name, conn, val);
			break;
		case RTS2_VALUE_STAT:
			writeConnBaseValue (name, val, desc);
			setValue (name.c_str (), val->getValueDouble (), desc.c_str ());
			name_stat = new char[name.length () + 6];
			n_top = name_stat + name.length ();
			strcpy (name_stat, name.c_str ());
			*n_top = '.';
			n_top++;
			strcpy (n_top, "MODE");
			setValue (name_stat, ((rts2core::ValueDoubleStat *) val)->getMode (), desc.c_str ());
			strcpy (n_top, "MIN");
			setValue (name_stat, ((rts2core::ValueDoubleStat *) val)->getMin (), desc.c_str ());
			strcpy (n_top, "MAX");
			setValue (name_stat, ((rts2core::ValueDoubleStat *) val)->getMax (), desc.c_str ());
			strcpy (n_top, "STD");
			setValue (name_stat, ((rts2core::ValueDoubleStat *) val)->getStdev (), desc.c_str ());
			strcpy (n_top, "NUM");
			setValue (name_stat, ((rts2core::ValueDoubleStat *) val)->getNumMes (), desc.c_str ());
			delete[]name_stat;
			break;
		case RTS2_VALUE_RECTANGLE:
			name_stat = new char[name.length () + 8];
			n_top = name_stat + name.length ();
			strcpy (name_stat, name.c_str ());
			*n_top = '.';
			n_top++;
			strcpy (n_top, "X");
			writeConnBaseValue (
				name_stat,
				((rts2core::ValueRectangle *)val)->getX (),
				((rts2core::ValueRectangle *)val)->getX ()->getDescription ().c_str ()
				);

			strcpy (n_top, "Y");
			writeConnBaseValue (
				name_stat,
				((rts2core::ValueRectangle *)val)->getY (),
				((rts2core::ValueRectangle *)val)->getY ()->getDescription ().c_str ()
				);

			strcpy (n_top, "HEIGHT");
			writeConnBaseValue (
				name_stat,
				((rts2core::ValueRectangle *)val)->getHeight (),
				((rts2core::ValueRectangle *)val)->getHeight ()->getDescription ().c_str ()
				);

			strcpy (n_top, "WIDTH");
			writeConnBaseValue (
				name_stat,
				((rts2core::ValueRectangle *)val)->getWidth (),
				((rts2core::ValueRectangle *)val)->getWidth ()->getDescription ().c_str ()
				);

			delete[]name_stat;
			break;
		case RTS2_VALUE_MMAX:
			writeConnBaseValue (name, val, desc);
			break;
	}
}

void Image::recordChange (rts2core::Connection * conn, rts2core::Value * val)
{
	char *name;
	// construct name
	if (conn->getOtherType () == DEVICE_TYPE_SENSOR || val->prefixWithDevice ())
	{
		name = new char[strlen (conn->getName ()) + strlen (val->getName ().c_str ()) + 10];
		strcpy (name, conn->getName ());
		strcat (name, ".");
		strcat (name, val->getName ().c_str ());
	}
	else
	{
		name = new char[strlen (val->getName ().c_str ()) + 9];
		strcpy (name, val->getName ().c_str ());
	}
	strcat (name, ".CHANGED");
	setValue (name, val->wasChanged (), "true if value was changed during exposure");
	delete[]name;
}

void Image::setEnvironmentalValues ()
{
	// record any environmental variables..
	std::vector <std::string> envWrite;
	rts2core::Configuration::instance ()->deviceWriteEnvVariables (getCameraName (), envWrite);
	for (std::vector <std::string>::iterator iter = envWrite.begin (); iter != envWrite.end (); iter++)
	{
		char *value = getenv ((*iter).c_str ());
		std::string comment;
		std::string section =  std::string ("env_") + (*iter) + std::string ("_comment");
		// check for comment
		if (rts2core::Configuration::instance ()->getString (getCameraName (), section.c_str (), comment) != 0)
			comment = std::string ("enviromental variable");

		if (value != NULL)
			setValue ((*iter).c_str (), value, comment.c_str ());
	}
}

void Image::writeConn (rts2core::Connection * conn, imageWriteWhich_t which)
{
	if (writeConnection)
	{
		for (rts2core::ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
		{
			rts2core::Value *val = *iter;
			if (val->getWriteToFits ())
			{
				switch (which)
				{
					case EXPOSURE_START:
						if (val->getValueWriteFlags () == RTS2_VWHEN_BEFORE_EXP)
							writeConnValue (conn, val);
						val->resetValueChanged ();
						break;
					case INFO_CALLED:
						if (val->getValueWriteFlags () == RTS2_VWHEN_BEFORE_END)
							writeConnValue (conn, val);
						break;
					case TRIGGERED:
						if (val->getValueWriteFlags () == RTS2_VWHEN_TRIGGERED)
						  	writeConnValue (conn, val);
						break;
					case EXPOSURE_END:
						// check to write change of value
						if (val->writeWhenChanged ())
							recordChange (conn, val);
						break;
				}
			}
			// record rotang even if it is not writable
			if (which == EXPOSURE_START && (val->getValueDisplayType () & RTS2_DT_WCS_MASK))
			{
				switch (val->getValueDisplayType ())
				{
					case RTS2_DT_WCS_CRVAL1:
					case RTS2_DT_WCS_CRVAL2:
					case RTS2_DT_WCS_CRPIX1:
					case RTS2_DT_WCS_CRPIX2:
					case RTS2_DT_WCS_CDELT1:
					case RTS2_DT_WCS_CDELT2:
						addWcs (val->getValueDouble (), ((val->getValueDisplayType () & RTS2_DT_WCS_SUBTYPE) >> 16) - 1);
						break;
					case RTS2_DT_AUXWCS_CRPIX1:
					case RTS2_DT_AUXWCS_CRPIX2:
						// check if it is in aux WCS..
						for (std::list <std::string>::iterator wcsi = wcsauxs.begin (); wcsi != wcsauxs.end (); wcsi++)
						{
							// only write if suffix matched what is in list
							if (val->getName ().substr (val->getName ().length () - wcsi->length ()) == *wcsi)
								addWcs (val->getValueDouble (), (((val->getValueDisplayType () & RTS2_DT_WCS_SUBTYPE) - RTS2_DT_AUXWCS_OFFSET) >> 16));
						}
						break;
					case RTS2_DT_WCS_ROTANG:
						addWcs (val->getValueDouble (), 6);
						if (strncmp (val->getName ().c_str (), "CROTA2", 6) == 0)
							wcs_multi_rotang = val->getName ()[6];
						break;
					default:
						if (val->getValueBaseType () == RTS2_VALUE_STRING)
							string_wcs.push_back (*((rts2core::ValueString *) val));
						break;
				}
			}
		}
	}
}

double Image::getLongtitude ()
{
	double lng = NAN;
	getValue ("LONGITUD", lng, true);
	return lng;
}

double Image::getExposureJD ()
{
	time_t tim = getCtimeSec ();
	return ln_get_julian_from_timet (&tim) + getCtimeUsec () / USEC_SEC / 86400.0;
}

double Image::getExposureLST ()
{
	double ret;
	ret = 15.0 * ln_get_apparent_sidereal_time (getExposureJD ()) + getLongtitude ();
	return ln_range_degrees (ret) / 15.0;
}

std::ostream & Image::printImage (std::ostream & _os)
{
	_os << "C " << getCameraName ()
		<< " [" << getChannelWidth (0) << ":"
		<< getChannelHeight (0) << "]"
		<< " RA (" << LibnovaDegArcMin (ra_err)
		<< ") DEC (" << LibnovaDegArcMin (dec_err)
		<< ") IMG_ERR " << LibnovaDegArcMin (img_err);
	return _os;
}

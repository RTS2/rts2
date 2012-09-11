/* 
 * Image manipulation program.
 * Copyright (C) 2006-2009 Petr Kubanek <petr@kubanek.net>
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

#include <rts2-config.h>
#include <strings.h>

#ifdef RTS2_HAVE_PGSQL
#include "rts2fits/appdbimage.h"
#else
#include "rts2fits/appimage.h"
#endif							 /* RTS2_HAVE_PGSQL */
#include "configuration.h"
#include "rts2format.h"

#include <iostream>
#include <iomanip>

#include <list>

#ifdef RTS2_HAVE_LIBJPEG
#include <Magick++.h>
#endif // RTS2_HAVE_LIBJPEG

#define IMAGEOP_NOOP            0x0000
#define IMAGEOP_ADDDATE         0x0001
#ifdef RTS2_HAVE_PGSQL
#define IMAGEOP_INSERT          0x0002
#endif							 /* RTS2_HAVE_PGSQL */
#define IMAGEOP_TEST            0x0004
#define IMAGEOP_PRINT           0x0008
#define IMAGEOP_FPRINT          0x0010
#define IMAGEOP_COPY            0x0020
#define IMAGEOP_SYMLINK	        0x0040
#define IMAGEOP_MOVE            0x0080
#define IMAGEOP_ADDHELIO        0x0400
#define IMAGEOP_MODEL           0x0800
#define IMAGEOP_JPEG            0x1000
#define IMAGEOP_STAT            0x2000
#define IMAGEOP_RTS2OPERA_WCS   0x4000
#define IMAGEOP_ADD_TEMPLATE    0x8000

#define OPT_ADDDATE             OPT_LOCAL + 5
#define OPT_ADDHELIO            OPT_LOCAL + 6
#define OPT_OBSID               OPT_LOCAL + 7
#define OPT_IMGID               OPT_LOCAL + 8
#define OPT_CAMNAME             OPT_LOCAL + 9
#define OPT_MOUNTNAME           OPT_LOCAL + 10
#define OPT_LABEL               OPT_LOCAL + 11
#define OPT_ZOOM                OPT_LOCAL + 12
#define OPT_ERR_RA              OPT_LOCAL + 13
#define OPT_ERR_DEC             OPT_LOCAL + 14
#define OPT_ERR                 OPT_LOCAL + 15
#define OPT_RTS2OPERA_WCS       OPT_LOCAL + 16
#define OPT_ADD_TEMPLATE        OPT_LABEL + 17

namespace rts2image
{

#ifdef RTS2_HAVE_PGSQL
class AppImage:public AppDbImage
#else
class AppImage:public rts2image::AppImageCore
#endif							 /* RTS2_HAVE_PGSQL */
{
	public:
		AppImage (int in_argc, char **in_argv, bool in_readOnly);
		virtual ~AppImage ();

	protected:
		virtual int processOption (int in_opt);
#ifdef RTS2_HAVE_LIBJPEG
		virtual int init ();
#endif
#ifdef RTS2_HAVE_PGSQL
		virtual bool doInitDB ();

		virtual int processImage (rts2image::ImageDb * image);
#else
		virtual int processImage (rts2image::Image * image);
#endif						 /* RTS2_HAVE_PGSQL */

		virtual void usage ();

	private:
		int operation;

		void printOffset (double x, double y, rts2image::Image * image);

		int addDate (rts2image::Image * image);

		char rts2opera_ext;
		int writeRTS2OperaHeaders (rts2image::Image * image, char ext);
#ifdef RTS2_HAVE_PGSQL
		int insert (rts2image::ImageDb * image);
#endif
		void testImage (rts2image::Image * image);
		void printModel (rts2image::Image * image);
		void printStat (rts2image::Image * image);

		double d_x1, d_y1, d_x2, d_y2;

		const char* print_expr;
		const char* copy_expr;
		const char* distance_expr;
		const char* link_expr;
		const char* move_expr;
#ifdef RTS2_HAVE_LIBJPEG
		const char* jpeg_expr;
		const char* label;
		double zoom;
#endif

		int obsid;
		int imgid;

		const char *cameraName;
		const char *mountName;

		double err_ra;
		double err_dec;
		double err;

		std::vector <rts2core::IniParser *> fitsTemplates;
};

}

using namespace rts2image;

void AppImage::printOffset (double x, double y, Image * image)
{
	std::cout << "not implemented" << std::endl;
}

int AppImage::addDate (Image * image)
{
	int ret;
	time_t t;
	std::cout << "Adding date " << image->getFileName () << "..";
	t = image->getExposureSec ();
	image->setValue ("DATE-OBS", &t, image->getExposureUsec (),"date of observation");
	ret = image->saveImage ();
	std::cout << (ret ? "failed" : "OK") << std::endl;
	return ret;
}

int AppImage::writeRTS2OperaHeaders (Image * image, char ext)
{
	double cdelt1, cdelt2, crpix1, crpix2, crota2;
	int flip = 1;
	char buf[8];
	memcpy (buf, "CDELT1", 7);
	if (ext != '-')
	{
		buf[6] = ext;
		buf[7] = '\0';
	}
	image->getValue (buf, cdelt1);
	buf[5] = '2';
	image->getValue (buf, cdelt2);
	memcpy (buf, "CRPIX1", 6);
	image->getValue (buf, crpix1);
	buf[5] = '2';
	image->getValue (buf, crpix2);
	memcpy (buf, "CROTA2", 6);
	image->getValue (buf, crota2);

	if (cdelt1 < 0 && cdelt2 < 0)
	{
		crota2 = ln_range_degrees (crota2 + 180);
	}
	else if ((cdelt1 < 0 && cdelt2 > 0) || (cdelt1 > 0 && cdelt2 < 0))
	{
		flip = 0;
	}

	image->setValue ("CAM_XOA", crpix1, "X reference pixel");
	image->setValue ("CAM_YOA", crpix2, "Y reference pixel");
	image->setValue ("XPLATE", fabs (cdelt1) * 3600.0, "X plate scale");
	image->setValue ("YPLATE", fabs (cdelt2) * 3600.0, "Y plate scale");
	image->setValue ("FLIP", flip, "camera flip");
	image->setValue ("ROTANG", ln_range_degrees (crota2), "camera rotantion angle");
	return image->rts2image::Image::saveImage ();
}


#ifdef RTS2_HAVE_PGSQL
int AppImage::insert (ImageDb * image)
{
  	if (obsid > 0)
	  	image->setObsId (obsid);
	if (imgid > 0)
	  	image->setImgId (imgid);
	if (cameraName)
	  	image->setCameraName (cameraName);
	if (mountName)
	  	image->setMountName (mountName);
	image->setErrors (err_ra, err_dec, err);	

	return image->saveImage ();
}
#endif							 /* RTS2_HAVE_PGSQL */

void AppImage::testImage (Image * image)
{
	std::cout
		<< image << std::endl
		<< "average " << image->getAverage () << std::endl
		<< "stdev " << image->getAvgStdDev () << std::endl
		//<< image->getRaDec (0, 0, ra, dec) << "RA and DEC of [0:0]: " << ra << ", " << dec << std::endl
		//<< image->getRaDec (image->getChannelWidth (0), 0, ra, dec) << "RA and DEC of [W:0]: " << ra << ", " << dec << std::endl
		//<< image->getRaDec (0, image->getChannelHeight (0), ra, dec) << "RA and DEC of [0:H]: " << ra << ", " << dec << std::endl
		//<< image->getRaDec (image->getChannelWidth (0), image->getChannelHeight (0), ra, dec) << "RA and DEC of [W:H]: " << ra << ", " << dec << std::endl
		// << "Image::getCenterRow " << image->getCenter (x, y, 3) << " " << x << ":" << y << std::endl
		<< "Expression %b/%t/%i/%c/%f '" << image->expandPath (std::string ("%b/%t/%i/%c/%f")) << '\'' << std::endl
		<< "Expression $DATE-OBS$/%b/%e/%E/%f/%F/%t/%i/%y/%m/%d/%D/%H/%M/%S/%s.fits '" << image->expandPath (std::string ("$DATE-OBS$/%b/%e/%E/%f/%F/%t/%i/%y/%m/%d/%D/%H/%M/%S/%s.fits")) << '\'' << std::endl
		<< "Target Type " << image->getTargetType () << std::endl;
}

void AppImage::printModel (Image *image)
{
	try
	{
  		std::cout << image->getCoord ("OBJ") << " " << image->getCoord ("TAR")
			<< '\t' << LibnovaDegDist (image->getValue ("RA_ERR"))
			<< '\t' << LibnovaDegDist (image->getValue ("DEC_ERR"))
			<< '\t' << LibnovaDegDist (image->getValue ("POS_ERR"))
			<< '\t' << LibnovaDegDist (image->getValue ("CORR_RA"))
			<< '\t' << LibnovaDegDist (image->getValue ("CORR_DEC"))
			<< std::endl;
	}
	catch (KeyNotFound er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
	}
}

void AppImage::printStat (Image *image)
{
	image->computeStatistics ();
	std::cout << image->getFileName ()
		<< " " << image->getAverage ()
		<< " " << image->getAvgStdDev () << std::endl;
}

int AppImage::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'p':
			if (print_expr)
				return -1;
			operation |= IMAGEOP_PRINT;
			print_expr = optarg;
			break;
		case 'P':
			if (print_expr)
				return -1;
			operation |= IMAGEOP_FPRINT;
			print_expr = optarg;
			break;
		case 'r':
			operation |= IMAGEOP_MODEL;
			break;
		case 's':
			operation |= IMAGEOP_STAT;
			break;
		case 'n':
			std::cout << pureNumbers;
			break;
		case 'c':
			if (copy_expr)
				return -1;
			operation |= IMAGEOP_COPY;
			copy_expr = optarg;
			break;
		case OPT_ADDDATE:
			operation |= IMAGEOP_ADDDATE;
			readOnly = false;
			break;
		case OPT_ADDHELIO:
			operation |= IMAGEOP_ADDHELIO;
			readOnly = false;
			break;
#ifdef RTS2_HAVE_PGSQL
		case 'i':
			operation |= IMAGEOP_INSERT;
			break;
#endif					 /* RTS2_HAVE_PGSQL */
		case OPT_OBSID:
			obsid = atoi (optarg);
			break;
		case OPT_IMGID:
			imgid = atoi (optarg);
			break;
		case OPT_CAMNAME:
			cameraName = optarg;
			break;
		case OPT_MOUNTNAME:
			mountName = optarg;
			break;
		case OPT_ERR_RA:
			err_ra = atof (optarg);
			break;
		case OPT_ERR_DEC:
			err_dec = atof (optarg);
			break;
		case OPT_ERR:
			err = atof (optarg);
			break;	
		case 'm':
			if (move_expr)
				return -1;
			operation |= IMAGEOP_MOVE;
			move_expr = optarg;
			break;
		case 'l':
			if (link_expr)
				return -1;
			operation |= IMAGEOP_SYMLINK;
			link_expr = optarg;
			break;
		case 't':
			operation |= IMAGEOP_TEST;
			break;
		#ifdef RTS2_HAVE_LIBJPEG
		case 'j':
			if (jpeg_expr)
				return -1;
			operation |= IMAGEOP_JPEG;
			jpeg_expr = optarg;
			break;
		case OPT_LABEL:
			label = optarg;
			break;
		case OPT_ZOOM:
			zoom = atof (optarg);
			break;
		#endif /* RTS2_HAVE_LIBJPEG */
		case OPT_RTS2OPERA_WCS:
			operation |= IMAGEOP_RTS2OPERA_WCS;
			if (optarg[1] != '\0')
			{
				std::cerr << "WCS extension must be a single character" << std::endl;
				return -1;
			}
			rts2opera_ext = optarg[0];
			readOnly = false;
			break;
		case OPT_ADD_TEMPLATE:
			operation |= IMAGEOP_ADD_TEMPLATE;
			{
				rts2core::IniParser *tf = new rts2core::IniParser ();
				fitsTemplates.push_back (tf);
				if (tf->loadFile (optarg, true))
				{
					std::cerr << "cannot load FITS template from file " << optarg << std::endl;
					return -1;
				}
				readOnly = false;
			}
			break;
		default:

		#ifdef RTS2_HAVE_PGSQL
			return AppDbImage::processOption (in_opt);
		#else
			return rts2image::AppImageCore::processOption (in_opt);
		#endif /* RTS2_HAVE_PGSQL */
	}
	return 0;
}

#ifdef RTS2_HAVE_LIBJPEG
int AppImage::init ()
{
#ifdef RTS2_HAVE_PGSQL
	int ret = AppDbImage::init ();
#else
	int ret = rts2image::AppImageCore::init ();
#endif /* RTS2_HAVE_PGSQL */
	if (ret)
		return ret;
	Magick::InitializeMagick (".");
	return 0;
}
#endif /* RTS2_HAVE_LIBJPEG */

#ifdef RTS2_HAVE_PGSQL
bool AppImage::doInitDB ()
{
	return (operation & IMAGEOP_MOVE) || (operation & IMAGEOP_INSERT);
}
#endif

#ifdef RTS2_HAVE_PGSQL
int AppImage::processImage (ImageDb * image)
#else
int AppImage::processImage (Image * image)
#endif							 /* RTS2_HAVE_PGSQL */
{
	if (operation == IMAGEOP_NOOP)
		help ();
	if (operation & IMAGEOP_ADDDATE)
		addDate (image);
	if (operation & IMAGEOP_ADDHELIO)
	{
		struct ln_equ_posn tel;
		double JD = image->getExposureJD () + image->getExposureLength () / 2.0 / 86400;
		image->getCoordMount (tel);
		image->setValue ("JD_HELIO", JD + ln_get_heliocentric_time_diff (JD, &tel), "heliocentric JD");
	}
	#ifdef RTS2_HAVE_PGSQL
	if (operation & IMAGEOP_INSERT)
		insert (image);
	#endif
	if (operation & IMAGEOP_TEST)
		testImage (image);
	if (operation & IMAGEOP_PRINT)
		std::cout << image->expandPath (print_expr, false) << std::endl;
	if (operation & IMAGEOP_FPRINT)
	  	std::cout << image->getFileName () << " " << image->expandPath (print_expr, false) << std::endl;
	if (operation & IMAGEOP_MODEL)
	  	printModel (image);
	if (operation & IMAGEOP_STAT)
	  	printStat (image);
	if (operation & IMAGEOP_COPY)
		image->copyImageExpand (copy_expr);
	if (operation & IMAGEOP_MOVE)
		image->renameImageExpand (move_expr);
	if (operation & IMAGEOP_SYMLINK)
	  	image->symlinkImageExpand (link_expr);
	if (operation & IMAGEOP_RTS2OPERA_WCS)
		writeRTS2OperaHeaders (image, rts2opera_ext);
	if (operation & IMAGEOP_ADD_TEMPLATE)
	{
		for (std::vector <rts2core::IniParser *>::iterator iter = fitsTemplates.begin (); iter != fitsTemplates.end (); iter++)
			image->addTemplate (*iter);
	}
#ifdef RTS2_HAVE_LIBJPEG
	if (operation & IMAGEOP_JPEG)
	  	image->writeAsJPEG (jpeg_expr, zoom, label);
#endif /* RTS2_HAVE_LIBJPEG */
	return 0;
}

void AppImage::usage ()
{
	std::cout 
		<< "  rts2-image -w 123.fits                     .. write WCS to file 123, based on information stored by RTS2 in the file"	<< std::endl
		<< "  rts2-image -w -o 20.12:10.56 123.fits      .. same as above, but add X offset of 20.12 pixels and Y offset of 10.56 pixels to WCS" << std::endl
		<< "  rts2-image -P @DATE_OBS/@POS_ERR 123.fits  .. prints DATE_OBS and POS_ERR keywords" << std::endl
		<< "  rts2-image -d 10:15-20:25 123.fits         .. prints RA DEC distance between pixel (10,15) and (20,25)" << std::endl
		<< "  rts2-image --label '%H:%M' -j out.jpeg 123.fits  .. creates out.jpeg, add label consisting of exposure hour and minute" << std::endl;
}

AppImage::AppImage (int in_argc, char **in_argv, bool in_readOnly):
#ifdef RTS2_HAVE_PGSQL
AppDbImage (in_argc, in_argv, in_readOnly)
#else
rts2image::AppImageCore (in_argc, in_argv, in_readOnly)
#endif
{
	operation = IMAGEOP_NOOP;

	print_expr = NULL;
	copy_expr = NULL;
	distance_expr = NULL;
	link_expr = NULL;
	move_expr = NULL;
#ifdef RTS2_HAVE_LIBJPEG
	jpeg_expr = NULL;
	label = NULL;
	zoom = 1;
#endif

	obsid = -1;
	imgid = -1;

	cameraName = NULL;
	mountName = NULL;

	rts2opera_ext = '-';

	err_ra = err_dec = err = NAN;

	addOption ('p', NULL, 1, "print image expression");
	addOption ('P', NULL, 1, "print filename followed by expression");
	addOption ('r', NULL, 0, "print referencig status - usefull for modelling checks");
	addOption ('s', NULL, 0, "print image statistics - average, median, min & max values,...");
	addOption ('n', NULL, 0, "print numbers only - do not pretty print degrees,..");
	addOption ('c', NULL, 1, "copy image(s) to path expression given as argument");
	addOption (OPT_ADDDATE, "add-date", 0, "add DATE-OBS to image header");
	addOption (OPT_ADDHELIO, "add-heliocentric", 0, "add JD_HELIO to image header (contains heliocentrict time)");
	addOption ('i', NULL, 0, "insert/update image(s) in the database");
	addOption (OPT_OBSID, "obsid", 1, "force observation ID for image operations");
	addOption (OPT_IMGID, "imgid", 1, "force image ID for image operations");
	addOption (OPT_CAMNAME, "camera", 1, "force camera name for image operations");
	addOption (OPT_MOUNTNAME, "telescope", 1, "force telescope name for image operations");
	addOption (OPT_ERR_RA, "ra-err", 1, "force image RA error");
	addOption (OPT_ERR_DEC, "dec-err", 1, "force image DEC error");
	addOption (OPT_ERR, "err", 1, "force image position error");
	addOption ('m', NULL, 1, "move image(s) to path expression given as argument");
	addOption ('l', NULL, 1, "soft link images(s) to path expression given as argument");
	addOption ('t', NULL, 0, "test various image routines");
	addOption (OPT_RTS2OPERA_WCS, "rts2opera-fix", 1, "add headers necessary for RTS2opera functionality");
	addOption (OPT_ADD_TEMPLATE, "add-template", 1, "add fixed-value headers from template file specified as an argument");
#ifdef RTS2_HAVE_LIBJPEG
	addOption ('j', NULL, 1, "export image(s) to JPEGs, specified by expansion string");
	addOption (OPT_LABEL, "label", 1, "label (expansion string) for image(s) JPEGs");
	addOption (OPT_ZOOM, "zoom", 1, "zoom the image before writing its label");
#endif /* RTS2_HAVE_LIBJPEG */
}

AppImage::~AppImage ()
{

	for (std::vector <rts2core::IniParser *>::iterator iter = fitsTemplates.begin (); iter != fitsTemplates.end (); iter++)
		delete *iter;

#ifdef RTS2_HAVE_LIBJPEG
  	MagickLib::DestroyMagick ();
#endif /* RTS2_HAVE_LIBJPEG */
}

int main (int argc, char **argv)
{
	AppImage app = AppImage (argc, argv, true);
	return app.run ();
}

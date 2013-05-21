/* 
 * Image preview and download classes.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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

/**
 * @page XMLRPCD_filedownload API calls for downloading images
 *
 * <a href="http://rts2.org">RTS2</a> supports multiple options for accessing
 * images through HTTP protocol, run by <b>rts2-xmlrpcd</b>. One can get access
 * to either raw data, FITS formated data, headers or to JPEG grayscale
 * previews of the images.
 *
 * @section XMLRPCD_filedownload_points Access points
 *
 * Every call assume that method name is the first virtual subdirectory in the
 * call. It is followed by full path to the file, and possibly call parameters
 * after standard ?.
 *
 * As an example, let's assume there is <b>rts2-xmlrpcd</b> server on host
 * <b>server</b>, running on port <b>8889</b>, accepting <b>preview</b> request
 * for image on path <b>/images/2011.1210/0001.fits</b>, with parameter
 * <b>ps</b> set to <b>200</b>.  Then this is the URL one would like to use to
 * extract the image:
 *
 * http://server:8889/preview/images/2011.1210/0001.fits?ps=200
 *
 * Following are description of various image related services, with their
 * parameters. Format of the documentation is similar to @ref JSON_description.
 *
 * <hr/>
 *
 * @section XMLRPCD_filedownload_preview preview
 *
 * Generates zoomed JPEG images. This is primary usefull for quick access to
 * small images to be put onto preview webpages.
 *
 * @subsection Example
 *
 * http://localhost:8889/preview/images/2011.1210/0001.fits?ps=200&lb=@FOC_POS
 * http://localhost:8889/preview/images/2011.1210/0001.fits?ps=200&lb=&q=0.005
 *
 * @subsection Parameters
 *  - <i><b>ps</b> size of the longest preview axis in pixels. Default to 128 pixels.</i>
 *  - <i><b>lb</b> label. Can include expansion characters, please see <b>man rts2</b> for details. If it is ommited, default label specified in events XML configuration file is used. If empty, e.g. if URL containts lb=& string, don't draw label.</i>
 *  - <i><b>q</b> quantiles for image display. Default to 0.005, which means that 0.5% of pixel values will be cut before the algorithm progress to generate ADU to pixel values transformation.</i>
 *  - <i><b>chan</b> channel of multiple extenstion image. Specify which extension (=channel) should be displayed (counted from 0), or use -1 to display all channels.</i>
 *
 * @subsection Return
 *
 * <b>image/jpeg</b> sized to have bigger axis equal to <b>ps</b> parameter.
 *
 * @section XMLRPCD_filedownload_fits fits
 *
 * Allow access to FITS images.
 *
 * @subsection Example
 *
 * http://localhost:8889/fits/images/2011.1210/0001.fits
 *
 * @subsection Parameters
 *
 * None.
 *
 * @subsection Return
 *
 * <b>image/fits</b> file with the image.
 *
 * <hr/>
 *
 * @section XMLRPCD_filedownload_data data
 *
 * Return raw data. Properly service sockets, so it will send new data only if
 * socket is available for read.
 *
 * @subsection Example
 *
 * http://localhost:8889/data/
 */

#ifdef __linux__
#define	_FILE_OFFSET_BITS 64
#endif

#include "rts2fits/image.h"
#include "rts2json/bsc.h"
#include "rts2json/imgpreview.h"
#include "dirsupport.h"
#ifdef RTS2_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#include "xmlrpc++/urlencoding.h"

using namespace rts2json;

const char *Previewer::style ()
{
	return ".normal { border: 5px solid white; } .hig { border: 5px solid navy; }";
}

void Previewer::script (std::ostringstream& _os, const char *label_encoded, float quantiles, int chan, int colourVariant)
{
	_os << "<script language='javascript'>\n"
"function high_off(files,name) {"
  "var i; for (i = files.length - 1; i >=0; i--)"
  "{\n"
    "if (files.options[i].value == name) { files.remove(i); i = -1; }\n"
  "}\n"
  "document.images[name].className='normal';\n"
"}\n"

"function high_on(files,name) {"
  "var o = document.createElement('option');\n"
  "o.selected=1;\n"
  "o.text=name;\n"
  "o.value=name;\n"
  "try { files.add(o,files.options[0]);} catch (ex) { files.add(o,0); }\n"
  "document.images[name].className='hig';\n"
"}\n"

"function highlight(name) {\n"
  "if (document.forms['download'].elements['act'][1].checked)\n"
  "{ var files = document.getElementById('files');\n"
    "if (document.images[name].className == 'hig') {"
      "high_off(files,name);\n"
    "} else\n"
    "{\n"
      "high_on(files,name);\n"
    "}\n"
  "}\n"
  "else if (document.forms['download'].elements['act'][2].checked)\n"
  "{ window.open('" << getServer ()->getPagePrefix () << "/fits' + escape(name),'FITS file');\n"
  "}\n"
  "else\n"
  "{ w2 = window.open('" << getServer ()->getPagePrefix () << "/jpeg' + escape(name) + '?lb=" << label_encoded << "&q=" << quantiles << "&chan=" << chan << "&cv=" << colourVariant << "', 'Preview');\n"
    "w2.focus ();"
  "}\n"
"}\n"

"function select_all()"
"{"
   "var files = document.getElementById('files');\n"
   "var but = document.getElementById('selectAll');\n"
   "var on = (but.innerHTML == 'Select all');\n"
   "for (var i in document.images) {\n"
     "var img = document.images[i];\n"
     "if (img.name && img.name[0] == '/')\n"
       "if (on && img.className == 'normal') { high_on(files,img.name);}\n"
       "if (!on && img.className == 'hig') { high_off(files,img.name);}\n"
   "}\n"
   "if (on) { but.innerHTML = 'Unselect all'; } else { but.innerHTML = 'Select all'; }\n"
"}</script>" << std::endl;
}

void Previewer::form (std::ostringstream &_os, int page, int ps, int s, int c, const char *label)
{
	_os << "<form name='download' method='post' action='" << getServer ()->getPagePrefix () << "/download'><input type='radio' name='act' value='v' checked='checked'>View</input><input type='radio' name='act' value='d'>Download</input><input type='radio' name='act' value='f'>Single FITS file</input>\n"
	"<select id='files' name='files' size='10' multiple='multiple' style='display:none'></select><input type='submit' value='Download'></input></form>\n"
	"<form name='label' method='get' action='./'>"
#ifdef CHANNELS
	"\nChannels <select name='chan'><option value='-1'";

	if (c < 0)
		_os << " selected";
	
	_os << ">All</option>";	

	for (int i = 0; i < CHANNELS; i++)
	{
		_os << "<option value='" << i << "'";
		if (c == i)
			_os << "selected";
		_os << ">" << (i + 1) << "</option>";
	}

	_os << "</select> \n" 
#endif
	"Label <input type='text' textwidth='20' name='lb' value='" << label << "'></input><input type='hidden' name='p' value='" << page << "'></input><input type='hidden' name='ps' value='" << ps << "'></input><input type='hidden' name='s' value='" << s << "'></input><input type='submit' value='Redraw'></input>&nbsp;\n"
        << "<button type='button' id='selectAll' onclick='select_all();'>Select all</button></form>\n";
}

void Previewer::imageHref (std::ostringstream& _os, int i, const char *fpath, int prevsize, const char *label, float quantiles, int chan, int colourVariant)
{
	std::string fp (fpath);
	XmlRpc::urlencode (fp, true);
	_os << "<img class='normal' name='" << fpath << "' onClick='highlight (\"" << fpath << "\")";
	if (prevsize > 0)
		_os << "' width='" << prevsize;
	_os << "' src='" << getServer ()->getPagePrefix () << "/preview" << fp << "?ps=" << prevsize << "&lb=" << label << "&chan=" << chan << "&q=" << quantiles << "&cv=" << colourVariant << "'/>" << std::endl;
}

void Previewer::pageLink (std::ostringstream& _os, int i, int pagesiz, int prevsize, const char *label, bool selected, float quantiles, int chan, int colourVariant)
{
	if (selected)
		_os << "  <b>" << i << "</b>" << std::endl;
	else
		_os << "  <a href='?p=" << i << "&s=" << pagesiz << "&ps=" << prevsize << "&lb=" << label << "&chan=" << chan << "&q=" << quantiles << "&cv=" << colourVariant << "'>" << i << "</a>" << std::endl;
}

#ifdef RTS2_HAVE_LIBJPEG

#include <Magick++.h>
using namespace Magick;

void JpegImageRequest::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/jpeg";
	rts2image::Image image;
	image.openFile (path.c_str (), true, false);
	Blob blob;

	const char * label = params->getString ("lb", getServer ()->getDefaultImageLabel ());

	float quantiles = params->getDouble ("q", DEFAULT_QUANTILES);
	int chan = params->getInteger ("chan", getServer ()->getDefaultChannel ());
	int colourVariant = params->getInteger ("cv", DEFAULT_COLOURVARIANT);

	Magick::Image *mimage = image.getMagickImage (label, quantiles, chan, colourVariant);

	cacheMaxAge (CACHE_MAX_STATIC);

	mimage->write (&blob, "jpeg");
	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);

	delete mimage;
}

void JpegPreview::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// size of previews
	int prevsize = params->getInteger ("ps", 128);
	// image type
	// const char *t = params->getString ("t", "p");

	const char *label = params->getString ("lb", getServer ()->getDefaultImageLabel ());
	
	std::string lb (label);
	XmlRpc::urlencode (lb);
	const char * label_encoded = lb.c_str ();

	float quantiles = params->getDouble ("q", DEFAULT_QUANTILES);
	int chan = params->getInteger ("chan", getServer ()->getDefaultChannel ());
	int colourVariant = params->getInteger ("cv", DEFAULT_COLOURVARIANT);

	std::string absPathStr = dirPath + path;
	const char *absPath = absPathStr.c_str ();

	// if it is a fits file..
	if (path.length () > 6 && (path.substr (path.length () - 5)) == std::string (".fits"))
	{
		response_type = "image/jpeg";

		rts2image::Image image;
		image.openFile (absPath, true, false);
		Blob blob;

		Magick::Image *mimage = image.getMagickImage (NULL, quantiles, chan, colourVariant);
		if (prevsize > 0)
		{
			mimage->zoom (Magick::Geometry (prevsize, prevsize));
			image.writeLabel (mimage, 0, mimage->size ().height (), 10, label);
		}
		else
		{
			image.writeLabel (mimage, 1, mimage->rows () - 2, 10, label);
		}

		cacheMaxAge (CACHE_MAX_STATIC);

		mimage->write (&blob, "jpeg");
		response_length = blob.length();
		response = new char[response_length];
		memcpy (response, blob.data(), response_length);

		delete mimage;
		return;
	}

	// get page number and size of page
	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	std::ostringstream _os;
	Previewer preview = Previewer (getServer ());

	printHeader (_os, (std::string ("Preview of ") + path).c_str (), preview.style() );

	preview.script (_os, label_encoded, quantiles, chan, colourVariant);

	_os << "<p>";

	preview.form (_os, pageno, prevsize, pagesiz, chan, label);
	
	_os << "</p><p>";

	struct dirent **namelist;
	int n;
	int i;
	int ret;

	const char *pagesort = params->getString ("o", "filename");

	enum {SORT_FILENAME, SORT_DATE} sortby = SORT_FILENAME;
	if (!strcmp (pagesort, "date"))
		sortby = SORT_DATE;

	switch (sortby)
	{
	 	case SORT_DATE:
			/* if following fails to compile, please have a look to value of your
			 * _POSIX_C_SOURCE #define, record it and send it to petr@kubanek.net.
			 * Please contact petr@kubanek.net if you don't know how to get
			 * _POSIX_C_SOURCE. */
			n = scandir (absPath, &namelist, NULL, cdatesort);
			break;
		case SORT_FILENAME:
		default:
		  	n = scandir (absPath, &namelist, NULL, alphasort);
			break;
	}

	if (n < 0)
	{
		throw XmlRpc::XmlRpcException ("Cannot open directory");
	}

	// first show directories..
	_os << "<p>";
	for (i = 0; i < n; i++)
	{
		char *fname = namelist[i]->d_name;
		struct stat sbuf;
		ret = stat ((absPathStr + fname).c_str (), &sbuf);
		if (ret)
			continue;
		if (S_ISDIR (sbuf.st_mode) && strcmp (fname, ".") != 0)
		{
			_os << "<a href='" << getServer ()->getPagePrefix () << getPrefix () << path << fname << "/?ps=" << prevsize << "&lb=" << label_encoded << "&chan=" << chan << "&q=" << quantiles << "&cv=" << colourVariant << "'>" << fname << "</a> ";
		}
	}

	_os << "</p><p>";

	int is = (pageno - 1) * pagesiz;
	int ie = is + pagesiz;
	int in = 0;


	for (i = 0; i < n; i++)
	{
		char *fname = namelist[i]->d_name;
		if (strstr (fname + strlen (fname) - 6, ".fits") == NULL)
			continue;
		in++;
		if (in <= is || in > ie)
			continue;
		std::string fpath = absPathStr + '/' + fname;
		preview.imageHref (_os, i, fpath.c_str (), prevsize, label_encoded, quantiles, chan, colourVariant);
	}

	for (i = 0; i < n; i++)
	{
		free (namelist[i]);
	}

	free (namelist);
			
	// print pages..
	_os << "</p><p>Page ";
	for (i = 1; i <= in / pagesiz; i++)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno, quantiles, chan, colourVariant);
	if (in % pagesiz)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno, quantiles, chan, colourVariant);
	_os << "</p>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* RTS2_HAVE_LIBJPEG */

void FitsImageRequest::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/fits";
	int f = open (path.c_str (), O_RDONLY);
	if (f == -1)
	{
		throw XmlRpc::XmlRpcException ("Cannot open file");
	}
	struct stat st;
	if (fstat (f, &st) == -1)
		throw XmlRpc::XmlRpcException ("Cannot get file properties");
	
	response_length = st.st_size;
	response = new char[response_length];
	ssize_t ret;
	ret = read (f, response, response_length);
	if (ret != (ssize_t) response_length)
	{
		delete[] response;
		throw XmlRpc::XmlRpcException ("Cannot read data");
	}
	close (f);
}

void DownloadRequest::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{

#ifndef RTS2_HAVE_LIBARCHIVE
	throw XmlRpc::XmlRpcException ("missing libArchive - cannot download data");
}

#else
	response_type = "application/x-gtar";

	addExtraHeader ("Content-disposition","attachment; filename=images.tar.bz2");

	struct ::archive *a;
	struct ::archive_entry *entry;

	a = archive_write_new ();
	archive_write_set_compression_bzip2 (a);
	archive_write_set_format_ustar (a);
	archive_write_set_bytes_in_last_block (a, 1);

	archive_write_open (a, this, &open_callback, &write_callback, &close_callback);

	for (XmlRpc::HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
	{
		if (!strcmp (iter->getName (), "files"))
		{
			entry = archive_entry_new ();
			struct stat st;

			char fn[strlen (iter->getValue ()) + 1];
			strcpy (fn, iter->getValue ());

			int fd = open (fn, O_RDONLY);
			if (fd < 0)
				throw XmlRpc::XmlRpcException ("Cannot open file for packing");
			fstat (fd, &st);
			archive_entry_copy_stat (entry, &st);
			archive_entry_set_pathname (entry, basename (fn));
			archive_write_header (a, entry);

			int len;
			char buff[8196];

			while ((len = read (fd, buff, sizeof (buff))) > 0)
				archive_write_data (a, buff, len);

			close (fd);
			archive_entry_free (entry);
		}
	}

	archive_write_finish (a);

	response_length = buf_size;
	response = buf;

	buf = NULL;
}

int open_callback (struct archive *a, void *client_data)
{
	DownloadRequest *dr = (DownloadRequest *) client_data;

	if (dr->buf)
		free (dr->buf);
	dr->buf = NULL;
	dr->buf_size = 0;

	return ARCHIVE_OK;
}

ssize_t write_callback (struct archive *a, void *client_data, const void *buffer, size_t length)
{
	DownloadRequest * dr = (DownloadRequest *) client_data;
	dr->buf = (char *) realloc (dr->buf, dr->buf_size + length);
	memcpy (dr->buf + dr->buf_size, buffer, length);
	dr->buf_size += length;

	return length;
}

int close_callback (struct archive *a, void *client_data)
{
	return ARCHIVE_OK;
}

#endif // RTS2_HAVE_LIBARCHIVE

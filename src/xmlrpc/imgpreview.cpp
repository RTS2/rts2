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

#ifdef __linux__
#define	_FILE_OFFSET_BITS 64
#endif

#include "imgpreview.h"
#include "../writers/rts2image.h"
#include "bsc.h"
#include "dirsupport.h"
#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#include "xmlrpc++/urlencoding.h"

using namespace rts2xmlrpc;

void Previewer::script (std::ostringstream& _os, const char *label_encoded)
{
	_os  << "<style type='text/css'>.normal { border: 5px solid white; } .hig { border: 5px solid navy; }</style></head><body>"
	<< "<script language='javascript'>\n function highlight (name, path) {\n if (document.forms['download'].elements['act'][1].checked)\n { var files = document.getElementById('files'); nc='hig';\n if (document.images[name].className == 'hig')\n { nc='normal'; var i; for (i = files.length - 1; i >=0; i--) { if (files.options[i].value == path) { files.remove(i); i = -1; } } }\nelse\n{\nvar o = document.createElement('option');\no.selected=1;\no.text=path;\no.value=path;\ntry { files.add(o,files.options[0]);\n} catch (ex) { files.add(o,0); }\n }\ndocument.images[name].className=nc;\n }\n else\n { w2 = window.open('" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/jpeg' + path + '?lb=" << label_encoded << "', 'Preview'); w2.focus (); }\n }</script>" << std::endl;
}

void Previewer::form (std::ostringstream &_os, int page, int ps, int s, const char *label)
{
	_os << "<form name='download' method='post' action='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/download'><input type='radio' name='act' value='v' checked='checked'>View</input><input type='radio' name='act' value='d'>Download</input>" << std::endl
	<< "<select id='files' name='files' size='10' multiple='multiple' style='display:none'></select><input type='submit' value='Download'/></form><br/>\n"
	<< "<form name='label' method='get' action='./'><input type='text' textwidth='20' name='lb' value='" << label << "'/><input type='hidden' name='p' value='" << page << "'/><input type='hidden' name='ps' value='" << ps << "'/><input type='hidden' name='s' value='" << s << "'/><input type='submit' value='Label'/></form>\n";
}

void Previewer::imageHref (std::ostringstream& _os, int i, const char *fpath, int prevsize, const char *label)
{
	_os << "<img class='normal' name='p" << i << "' onClick='highlight (\"p" << i << "\", \"" << fpath << "\")' width='" << prevsize << "' height='" << prevsize << "' src='" << ((XmlRpcd *)getMasterApp())->getPagePrefix () << "/preview" << fpath << "?ps=" << prevsize << "&lb=" << label << "'/>" << std::endl;
}

void Previewer::pageLink (std::ostringstream& _os, int i, int pagesiz, int prevsize, const char *label, bool selected)
{
	if (selected)
		_os << "  <b>" << i << "</b>" << std::endl;
	else
		_os << "  <a href='?p=" << i << "&s=" << pagesiz << "&ps=" << prevsize << "&lb=" << label << "'>" << i << "</a>" << std::endl;
}

#ifdef HAVE_LIBJPEG

#include <Magick++.h>
using namespace Magick;

void JpegImageRequest::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/jpeg";
	Rts2Image image (path.c_str (), false, true);
	image.openImage ();
	Blob blob;

	const char * label = params->getString ("lb", "%Y-%m-%d %H:%M:%S @OBJECT");

	Magick::Image mimage = image.getMagickImage (label);

	mimage.write (&blob, "jpeg");
	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

void JpegPreview::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// size of previews
	int prevsize = params->getInteger ("ps", 128);
	// image type
	// const char *t = params->getString ("t", "p");

	const char *label = params->getString ("lb", "%Y-%m-%d %H:%M:%S");
	
	std::string lb (label);
	XmlRpc::urlencode (lb);
	const char * label_encoded = lb.c_str ();

	std::string absPathStr = dirPath + path;
	const char *absPath = absPathStr.c_str ();

	// if it is a fits file..
	if (path.length () > 6 && (path.substr (path.length () - 5)) == std::string (".fits"))
	{
		response_type = "image/jpeg";

		Rts2Image image (absPath, false, true);
		image.openImage ();
		Blob blob;
		Magick::Image mimage = image.getMagickImage ();
		mimage.zoom (Magick::Geometry (prevsize, prevsize));

		image.writeLabel (mimage, 1, prevsize - 2, 10, label);

		mimage.write (&blob, "jpeg");
		response_length = blob.length();
		response = new char[response_length];
		memcpy (response, blob.data(), response_length);
		return;
	}

	// get page number and size of page
	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	std::ostringstream _os;
	_os << "<html><head><title>Preview of " << path << "</title>";

	Previewer preview = Previewer ();
	preview.script (_os, label_encoded);

	_os << "</head><body><p>";

	preview.form (_os, pageno, prevsize, pagesiz, label);
	
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
		throw XmlRpcException ("Cannot open directory");
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
			_os << "<a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << getPrefix () << path << fname << "/?ps=" << prevsize << "?lb=" << label_encoded << "'>" << fname << "</a> ";
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
		preview.imageHref (_os, i, fpath.c_str (), prevsize, label_encoded);
	}

	for (i = 0; i < n; i++)
	{
		free (namelist[i]);
	}

	free (namelist);
			
	// print pages..
	_os << "</p><p>Page ";
	for (i = 1; i <= in / pagesiz; i++)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno);
	if (in % pagesiz)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno);
	_os << "</p></body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* HAVE_LIBJPEG */

void FitsImageRequest::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/fits";
	int f = open (path.c_str (), O_RDONLY);
	if (f == -1)
	{
		throw XmlRpcException ("Cannot open file");
	}
	struct stat st;
	if (fstat (f, &st) == -1)
	{
		throw XmlRpcException ("Cannot get file properties");
	}
	response_length = st.st_size;
	response = new char[response_length];
	int ret;
	ret = read (f, response, response_length);
	if (ret != int (response_length))
	{
		delete[] response;
		throw XmlRpcException ("Cannot read data");
	}
	close (f);
}

void DownloadRequest::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{

#ifndef HAVE_LIBARCHIVE
	throw XmlRpcException ("missing libArchive - cannot download data");
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

	for (HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
	{
		if (!strcmp (iter->getName (), "files"))
		{
			entry = archive_entry_new ();
			struct stat st;

			char fn[strlen (iter->getValue ()) + 1];
			strcpy (fn, iter->getValue ());

			int fd = open (fn, O_RDONLY);
			if (fd < 0)
				throw XmlRpcException ("Cannot open file for packing");
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
	rts2xmlrpc::DownloadRequest *dr = (rts2xmlrpc::DownloadRequest *) client_data;

	if (dr->buf)
		free (dr->buf);
	dr->buf = NULL;
	dr->buf_size = 0;

	return ARCHIVE_OK;
}

ssize_t write_callback (struct archive *a, void *client_data, const void *buffer, size_t length)
{
	rts2xmlrpc::DownloadRequest * dr = (rts2xmlrpc::DownloadRequest *) client_data;
	dr->buf = (char *) realloc (dr->buf, dr->buf_size + length);
	memcpy (dr->buf + dr->buf_size, buffer, length);
	dr->buf_size += length;

	return length;
}

int close_callback (struct archive *a, void *client_data)
{
	return ARCHIVE_OK;
}

#endif // HAVE_LIBARCHIVE

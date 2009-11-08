/* 
 * Image preview and download classes.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "imgpreview.h"
#include "../writers/rts2image.h"

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace rts2xmlrpc;

#ifdef HAVE_LIBJPEG

#include <Magick++.h>
using namespace Magick;

void JpegImageRequest::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	response_type = "image/jpeg";
	Rts2Image image (path.c_str (), false, true);
	image.openImage ();
	Blob blob;
	Magick::Image mimage = image.getMagickImage (true);

	mimage.write (&blob, "jpeg");
	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#ifndef HAVE_SCANDIR
int scandir (const char *dirp, struct dirent ***namelist, int (*filter)(const struct dirent *), int (*compar)(const void *, const void *))
{
	DIR *d = opendir (dirp);

	int nl_size = 20;
	*namelist = (struct dirent **) malloc (sizeof (dirent *) * 20);
	int nmeb = 0;

	while (struct dirent *de = readdir (d))
	{
		if (filter (de))
		{
			struct dirent *dn = (struct dirent *) malloc (sizeof (dirent));
			*dn = *de;
			if (nl_size == 0)
			{
				*namelist = (struct dirent **) realloc (namelist, sizeof (dirent *) * (nmeb + 20));
				nl_size = 20;
			}
			(*namelist)[nmeb] = dn;
			nmeb++;
			nl_size--;
		}
	}

	closedir (d);

	if (*namelist)
		// sort it..
		qsort (*namelist, nmeb, sizeof (dirent *), compar);
	return nmeb;
}
#endif // !HAVE_SCANDIR

/**
 * Sort two file structure entries by cdate.
 */
#if _POSIX_C_SOURCE > 200200L && defined(HAVE_SCANDIR)
int cdatesort(const struct dirent **a, const struct dirent **b)
{
	struct stat s_a, s_b;
        if (stat ((*a)->d_name, &s_a))
                return 1;
        if (stat ((*b)->d_name, &s_b))
                return -1;
        if (s_a.st_ctime == s_b.st_ctime)
                return 0;
        if (s_a.st_ctime > s_b.st_ctime)
                return 1;
        return -1;
}
#else
int cdatesort(const void *a, const void *b)
{
	struct stat s_a, s_b;
        const struct dirent * d_a = *((dirent**)a);
        const struct dirent * d_b = *((dirent**)b);
        if (stat (d_a->d_name, &s_a))
                return 1;
        if (stat (d_b->d_name, &s_b))
                return -1;
        if (s_a.st_ctime == s_b.st_ctime)
                return 0;
        if (s_a.st_ctime > s_b.st_ctime)
                return 1;
        return -1;
}
#endif  // _POSIX_C_SOURCE

#ifndef HAVE_ALPHASORT

#if _POSIX_C_SOURCE > 200200L && defined(HAVE_SCANDIR)
int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcmp ((*a)->d_name, (*b)->d_name);
}
#else
int alphasort(const void *a, const void *b)
{
        const struct dirent * d_a = *((dirent**)a);
        const struct dirent * d_b = *((dirent**)b);
	return strcmp (d_a->d_name, d_b->d_name);
}
#endif // _POSIX_C_SOURCE

#endif // !HAVE_ALPHASORT

void JpegPreview::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	// if it is a fits file..
	if (path.length () > 6 && (path.substr (path.length () - 5)) == std::string (".fits"))
	{
		response_type = "image/jpeg";

		Rts2Image image (path.c_str (), false, true);
		image.openImage ();
		Blob blob;
		Magick::Image mimage = image.getMagickImage ();
		mimage.zoom (Magick::Geometry (128, 128));

		image.writeLabel (mimage, 1, 126, 10, "%Y-%m-%d %H:%M:%S");

		mimage.write (&blob, "jpeg");
		response_length = blob.length();
		response = new char[response_length];
		memcpy (response, blob.data(), response_length);
		return;
	}
	std::ostringstream _os;
	_os << "<html><head><title>Preview of " << path << "</title></head><body>";

	struct dirent **namelist;
	int n;

	int ret = chdir (path.c_str ());
	if (ret)
	{
	  	throw XmlRpcException ("Invalid directory");
	}

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
			n = scandir (".", &namelist, 0, cdatesort);
			break;
		case SORT_FILENAME:
		default:
		  	n = scandir (".", &namelist, 0, alphasort);
			break;
	}

	if (n < 0)
	{
		throw XmlRpcException ("Cannot open directory");
	}

	// first show directories..
	_os << "<p>";
	for (int i = 0; i < n; i++)
	{
		char *fname = namelist[i]->d_name;
		struct stat sbuf;
		ret = stat (fname, &sbuf);
		if (ret)
			continue;
		if (S_ISDIR (sbuf.st_mode) && strcmp (fname, ".") != 0)
		{
			_os << "<a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/preview" << path << fname << "/'>" << fname << "</a> ";
		}
	}

	_os << "</p><p>";

	// get page number and size of page
	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	int is = (pageno - 1) * pagesiz;
	int ie = is + pagesiz;
	int in = 0;

	int i;

	for (i = 0; i < n; i++)
	{
		char *fname = namelist[i]->d_name;
		if (strstr (fname + strlen (fname) - 6, ".fits") == NULL)
			continue;
		in++;
		if (in <= is)
			continue;
		if (in > ie)
			continue;
		std::string fpath = std::string (path) + '/' + fname;
		_os << "<a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/jpeg" << fpath 
		  << "'><img width='128' height='128' src='" << ((XmlRpcd *)getMasterApp())->getPagePrefix () << "/preview" << fpath << "'/></a>";
	}

	for (i = 0; i < n; i++)
	{
		free (namelist[i]);
	}

	free (namelist);
			
	// print pages..
	_os << "</p><p>Page ";
	for (i = 1; i <= in / pagesiz; i++)
	 	pageLink (_os, path.c_str (), i, pagesiz, i == pageno);
	if (in % pagesiz)
	 	pageLink (_os, path.c_str (), i, pagesiz, i == pageno);
	_os << "</p></body></html>";

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void JpegPreview::pageLink (std::ostringstream& _os, const char* path, int i, int pagesiz, bool selected)
{
	if (selected)
	{
		_os << "<b>" << i << "</b> ";
	}
	else
	{
		_os << "<a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/preview" << path << "?p=" << i << "&s=" << pagesiz << "'>" << i << "</a> ";
	}
}

#endif /* HAVE_LIBJPEG */

void FitsImageRequest::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length)
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
	if (ret != response_length)
	{
		delete[] response;
		throw XmlRpcException ("Cannot read data");
	}
	close (f);
}

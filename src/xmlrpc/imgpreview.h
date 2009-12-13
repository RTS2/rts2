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

#include "config.h"
#include "httpreq.h"
#include "xmlrpcd.h"

namespace rts2xmlrpc
{

#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1

class JpegImageRequest: public GetRequestAuthorized
{
	public:
		JpegImageRequest (const char* prefix, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, s) {}

		virtual void authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length);
};

/**
 * Create page with JPEG previews.
 *
 * @param p Page number. Default to 0.
 * @param s Page size (number of images per page). Defalt to 100.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class JpegPreview:public GetRequestAuthorized
{
	public:
		JpegPreview (const char* prefix, const char *_dirPath, XmlRpcServer *s):GetRequestAuthorized (prefix, s) { dirPath = _dirPath; }

		void pageLink (std::ostringstream& _os, const char* path, int i, int pagesiz, int prevsize, bool selected);

		virtual void authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length);
	private:
		const char *dirPath;
};

#endif // HAVE_LIBJPEG

class FitsImageRequest:public GetRequestAuthorized
{
	public:
		FitsImageRequest (const char* prefix, XmlRpcServer* s):GetRequestAuthorized (prefix, s) {}

		virtual void authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length);
};

class DownloadRequest:public GetRequestAuthorized
{
	public:
		DownloadRequest (const char* prefix, XmlRpcServer* s):GetRequestAuthorized (prefix, s) {}

		virtual void authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, int &response_length);
};

}

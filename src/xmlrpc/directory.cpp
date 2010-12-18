/* 
 * Class providing directory to GET request.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "directory.h"
#include "dirsupport.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

Directory::Directory (const char* prefix, const char *_dirPath, const char *_defaultFile, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, _dirPath, s)
{
	dirPath = std::string (_dirPath);
	defaultFile = std::string (_defaultFile);

	responseTypes["html"] = "text/html";
	responseTypes["htm"] = "text/html";
	responseTypes["css"] = "text/css";
}

void Directory::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	// check if file exists..
	std::string fn = dirPath + '/' + path;
	if (path.length () > 0 && path[path.length () - 1] == '/')
		fn += defaultFile;
	
	// check if file exists..
	int f = open (fn.c_str (), O_RDONLY);
	if (f != -1)
	{
		struct stat st;
		if (fstat (f, &st) == -1)
			throw XmlRpcException ("Cannot get file properties");

		response_length = st.st_size;
		response = new char[response_length];
		ssize_t ret = read (f, response, response_length);
		if (ret != (ssize_t) response_length)
		{
			delete[] response;
			throw XmlRpcException ("Cannot read file");
		}
		close (f);
		// try to find type based on file extension
		size_t extp = path.rfind ('.');
		if (extp != std::string::npos)
		{
			std::map <std::string, const char *>::iterator iter = responseTypes.find (path.substr (extp + 1));
			if (iter != responseTypes.end ())
			{
				response_type = iter->second;
				return;
			}
		}
		response_type = "text/html";
		return;
	}

	printHeader (_os, path.c_str ());

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
			n = scandir ((dirPath + path).c_str (), &namelist, 0, cdatesort);
			break;
		case SORT_FILENAME:
		  	n = scandir ((dirPath + path).c_str (), &namelist, 0, alphasort);
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
		ret = stat ((dirPath + path + fname).c_str (), &sbuf);
		if (ret)
			continue;
		if (S_ISDIR (sbuf.st_mode) && strcmp (fname, ".") != 0)
		{
			_os << "<a href='" << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << getPrefix () << path << fname << "/'>" << fname << "</a> ";
		}
	}

	_os << "</p>";
	printFooter (_os);

	for (i = 0; i < n; i++)
	{
		free (namelist[i]);
	}

	free (namelist);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

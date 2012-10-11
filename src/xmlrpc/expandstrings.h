/* 
 * Expand XML structure.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_EXPANDSTRINGS__
#define __RTS2_EXPANDSTRINGS__

#include <list>
#include <libxml/parser.h>
#include <ostream>
#include <string>
#include <string.h>
#include <sstream>
#include <iostream>

#include "xmlrpc++/XmlRpcServerGetRequest.h"

namespace rts2xmlrpc
{

class XmlRpcd;

class ExpandString
{
	public:
		ExpandString () {}
		virtual void writeTo (std::ostream &os) = 0;
};

class ExpandStringString:public ExpandString
{
	public:
		ExpandStringString (const char * chrt) { str = new char[strlen (chrt) + 1]; strcpy (str, chrt); }
		~ExpandStringString () { delete []str; }
		virtual void writeTo (std::ostream &os) { os << str; }
	private:
		char *str;
};

class ExpandStringTag:public ExpandString
{
	public:
		ExpandStringTag () {}
		virtual void writeTo (std::ostream &os) { std::cout << oss.str () << std::endl; os << oss.str (); }
	protected:
		std::ostringstream oss;
};

/**
 * Expand node back to string.
 */
class ExpandStringOpenTag:public ExpandStringTag
{
	public:
		ExpandStringOpenTag (xmlNodePtr ptr, bool closeTag = false);
};

class ExpandStringSingleTag:public ExpandStringOpenTag
{
	public:
		ExpandStringSingleTag (xmlNodePtr ptr);
};

class ExpandStringCloseTag:public ExpandStringTag
{
	public:
		ExpandStringCloseTag (xmlNodePtr ptr);
};

class ExpandStringDevice:public ExpandString
{
	public:
		ExpandStringDevice (const char *_deviceName);
		~ExpandStringDevice () { delete []deviceName; }
		virtual void writeTo (std::ostream &os);

	private:
		char *deviceName;
};

class ExpandStringValue:public ExpandString
{
	public:
		ExpandStringValue (const char *_deviceName, const char *_valueName);
		~ExpandStringValue () { delete []deviceName; delete []valueName; delete []subName; }
		virtual void writeTo (std::ostream &os);

	private:
		char *deviceName;
		char *valueName;
		char *subName;
};

class ExpandStringScript:public ExpandString
{
	public:
		ExpandStringScript (const char *pagePrefix, const char *_script);
		~ExpandStringScript () { delete []script; }
		virtual void writeTo (std::ostream &os);

	private:
		char *script;
};

class ExpandStringUsername:public ExpandString
{
	public:
		ExpandStringUsername (const char *_username):ExpandString () { username = _username; }
		~ExpandStringUsername () {}

		virtual void writeTo (std::ostream &os);

	private:
		const char *username;
};

class ExpandStrings:public std::list <ExpandString *>
{
	public:
		ExpandStrings (const char *_pagePrefix = NULL, XmlRpc::XmlRpcServerGetRequest *_request = NULL)
		{
			pagePrefix = _pagePrefix;
			request = _request;
		}

		~ExpandStrings () { for (ExpandStrings::iterator iter = begin (); iter != end (); iter++) delete *iter; clear (); }
		void expandXML (xmlNodePtr ptr, const char *defaultDeviceName, bool ignoreUnknownTags = false);
		std::string getString ();

	private:
		const char *pagePrefix;
		XmlRpc::XmlRpcServerGetRequest *request;
};

}

#endif /* !__RTS2_EXPANDSTRINGS__ */

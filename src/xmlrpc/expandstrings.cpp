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

#include "expandstrings.h"
#include "xmlrpcd.h"

#include "../utils/rts2block.h"
#include "../utils/rts2conn.h"
#include "../utils/rts2displayvalue.h"

using namespace rts2xmlrpc;

ExpandStringOpenTag::ExpandStringOpenTag (xmlNodePtr ptr, bool closeTag):ExpandStringTag ()
{
	oss << "<" << ptr->name;

	for (xmlAttrPtr aptr = ptr->properties; aptr != NULL; aptr = aptr->next)
	{
		oss << " " << aptr->name;
		if (aptr->children->content)
			oss << "=" << aptr->children->content;
	}

	if (closeTag)
		oss << "/";
	oss << ">";
}

ExpandStringSingleTag::ExpandStringSingleTag (xmlNodePtr ptr):ExpandStringOpenTag (ptr, true)
{
}

ExpandStringCloseTag::ExpandStringCloseTag (xmlNodePtr ptr):ExpandStringTag ()
{
	oss << "<" << ptr->name << "/>";
}

ExpandStringDevice::ExpandStringDevice (const char *_deviceName)
{
	deviceName = new char[strlen (_deviceName) + 1];
	strcpy (deviceName, _deviceName);
}

void ExpandStringDevice::writeTo (std::ostream &os)
{
	Rts2Conn *conn = ((Rts2Block *) getMasterApp ())->getOpenConnection (deviceName);
	if (conn == NULL)
	{
		os << "unknow device " << deviceName << std::endl;
		return;
	}
	for (Rts2ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		os << (*iter)->getName () << "=" << getDisplayValue (*iter) << std::endl;
	}
}

ExpandStringValue::ExpandStringValue (const char *_deviceName, const char *_valueName)
{
	deviceName = new char[strlen (_deviceName) + 1];
	strcpy (deviceName, _deviceName);
	valueName = new char[strlen (_valueName) + 1];
	strcpy (valueName, _valueName);
}

void ExpandStringValue::writeTo (std::ostream &os)
{
	Rts2Value *val = ((Rts2Block *) getMasterApp ())->getValue (deviceName, valueName);
	if (val == NULL)
	{
		os << "unknow value " << deviceName << "." << valueName;
		return;
	}
	os << getDisplayValue (val);
}

void ExpandStrings::expandXML (xmlNodePtr ptr, const char *defaultDeviceName, bool ignoreUnknownTags)
{
	for (; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->type == XML_COMMENT_NODE)
		{
			continue;
		}
		else if (ptr->children == NULL)
		{
			if (ptr->content != NULL)
			{
				push_back (new ExpandStringString ((char *) ptr->content));
			}
			else if (ignoreUnknownTags)
			{
				push_back (new ExpandStringSingleTag (ptr));
			}
		}
		else if (xmlStrEqual (ptr->name, (xmlChar *) "value"))
		{
			xmlAttrPtr deviceName = xmlHasProp (ptr, (xmlChar *) "device");
			const char *devName;
			if (deviceName == NULL)
			{
			  	devName = defaultDeviceName;
			}
			else
			{
				devName = (const char *) deviceName->children->content;
			}
			if (ptr->children == NULL)
			  	throw XmlEmptyNode (ptr);
		  	push_back (new ExpandStringValue (devName, (char *) ptr->children->content));
		}
		else if (xmlStrEqual (ptr->name, (xmlChar *) "device"))
		{
			if (ptr->children == NULL)
				throw XmlEmptyNode (ptr);
			push_back (new ExpandStringDevice ((char *)ptr->children->content));
		}
		else
		{
			if (ignoreUnknownTags)
			{
				push_back (new ExpandStringOpenTag (ptr));
				for (xmlNodePtr chptr = ptr->children; chptr != NULL; chptr = chptr->next)
				{
					expandXML (chptr, defaultDeviceName, ignoreUnknownTags);
				}
				push_back (new ExpandStringCloseTag (ptr));
			}
			else
			{
				throw XmlUnexpectedNode (ptr);
			}
		}
	}
}

std::string ExpandStrings::getString ()
{
	std::ostringstream os;
	for (ExpandStrings::iterator iter = begin (); iter != end (); iter++)
		(*iter)->writeTo (os);
	return os.str ();
}

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

#include "rts2json/expandstrings.h"
#include "xmlerror.h"

#include "block.h"
#include "connection.h"
#include "displayvalue.h"

using namespace rts2json;

ExpandStringOpenTag::ExpandStringOpenTag (xmlNodePtr ptr, bool closeTag):ExpandStringTag ()
{
	oss << "<" << ptr->name;

	for (xmlAttrPtr aptr = ptr->properties; aptr != NULL; aptr = aptr->next)
	{
		oss << " " << aptr->name;
		if (aptr->children->content)
			oss << "='" << aptr->children->content << "'";
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
	oss << "</" << ptr->name << ">";
}

ExpandStringDevice::ExpandStringDevice (const char *_deviceName)
{
	deviceName = new char[strlen (_deviceName) + 1];
	strcpy (deviceName, _deviceName);
}

void ExpandStringDevice::writeTo (std::ostream &os)
{
	rts2core::Rts2Connection *conn = ((rts2core::Block *) getMasterApp ())->getOpenConnection (deviceName);
	if (conn == NULL)
	{
		os << "unknow device " << deviceName << std::endl;
		return;
	}
	os << "<table>" << std::endl;
	for (rts2core::ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd (); iter++)
	{
		os << " <tr><td>" << (*iter)->getName () << "</td><td id='" << deviceName << "_" << (*iter)->getName () << "'>---</td></tr>" << std::endl;
	}
	os << "</table>" << std::endl << "<script type='text/javascript' language='javascript'>" << std::endl
		<< "refreshDeviceTable('" << deviceName << "');" << std::endl
		<< "</script>" << std::endl;
}

ExpandStringScript::ExpandStringScript (const char *pagePrefix, const char *_script)
{
	script = new char[strlen (pagePrefix ) + strlen (_script) + 5];
	strcpy (script, pagePrefix);
	strcat (script, "/js/");
	strcat (script, _script);
}

void ExpandStringScript::writeTo (std::ostream &os)
{
	os << "<script type='text/javascript' src='" << script << "'></script>";
}

void ExpandStringUsername::writeTo (std::ostream &os)
{
	os << username;
}

ExpandStringValue::ExpandStringValue (const char *_deviceName, const char *_valueName)
{
	deviceName = new char[strlen (_deviceName) + 1];
	strcpy (deviceName, _deviceName);
	// check for . - value pair separator
	const char *p = strchr (_valueName, '.');
	if (p == NULL)
	{
		valueName = new char[strlen (_valueName) + 1];
		strcpy (valueName, _valueName);
		subName = NULL;
	}
	else
	{
		int nlen = p - _valueName;
		valueName = new char[nlen + 1];
		strncpy (valueName, _valueName, nlen);
		valueName[nlen] = '\0';
		subName = new char[strlen (_valueName) - nlen + 1];
		strcpy (subName, p + 1);
	}
}

void ExpandStringValue::writeTo (std::ostream &os)
{
	rts2core::Value *val = ((rts2core::Block *) getMasterApp ())->getValue (deviceName, valueName);
	if (val == NULL)
	{
		os << "unknow value " << deviceName << "." << valueName << " ";
		return;
	}
	if (subName)
	{
		if (val->getValueBaseType () == RTS2_VALUE_RADEC)
		{
			if (strcasecmp (subName, "ra") == 0)
			{
				os << ((rts2core::ValueRaDec *) val)->getRa ();
			}
			else if (strcasecmp (subName, "dec") == 0)
			{
				os << ((rts2core::ValueRaDec *) val)->getDec ();
			}
		}
		else if (val->getValueBaseType () == RTS2_VALUE_ALTAZ)
		{
			if (strcasecmp (subName, "alt") == 0)
			{
				os << ((rts2core::ValueAltAz *) val)->getAlt ();
			}
			else if (strcasecmp (subName, "az") == 0)
			{
				os << ((rts2core::ValueAltAz *) val)->getAz ();
			}
		}
		else
		{
			os << "invalid subname " << subName << " for value " << deviceName << " " << valueName << " ";
		}
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
		else if (ptr->type == XML_TEXT_NODE)
		{
			if (ptr->content != NULL)
			{
				push_back (new ExpandStringString ((char *) ptr->content));
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
			push_back (new ExpandStringDevice ((char *) ptr->children->content));
		}
		else if (xmlStrEqual (ptr->name, (xmlChar *) "rts2script"))
		{
			xmlAttrPtr script = xmlHasProp (ptr, (xmlChar *) "script");
			if (script == NULL)
				throw XmlMissingAttribute (ptr, "script");
			push_back (new ExpandStringScript (pagePrefix, (char *) script->children->content));
		}
		else if (xmlStrEqual (ptr->name, (xmlChar *) "username"))
		{
			push_back (new ExpandStringUsername (request->getUsername ().c_str ()));
		}
		else
		{
			if (ignoreUnknownTags)
			{
				push_back (new ExpandStringOpenTag (ptr));
				expandXML (ptr->children, defaultDeviceName, ignoreUnknownTags);
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

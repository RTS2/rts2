/* 
 * Action items for sending out emails.
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

#include "xmlrpcd.h"

#include "../utils/connfork.h"

using namespace rts2xmlrpc;

ExpandStringValue::ExpandStringValue (const char *_deviceName, const char *_valueName)
{
	deviceName = new char[strlen (_deviceName) + 1];
	strcpy (deviceName, _deviceName);
	valueName = new char[strlen (_valueName) + 1];
	strcpy (valueName, _valueName);
}

const char *ExpandStringValue::getString ()
{
	Rts2Value *val = ((Rts2Block *) getMasterApp ())->getValue (deviceName, valueName);
	if (val == NULL)
		return "unknow value";
	return val->getDisplayValue ();
}

void ExpandStrings::expandXML (xmlNodePtr ptr)
{
	clear ();
	for (; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->children == NULL)
		{
			push_back (new ExpandStringString ((char *) ptr->content));
		}
		else if (xmlStrEqual (ptr->name, (xmlChar *) "value"))
		{
			xmlAttrPtr deviceName = xmlHasProp (ptr, (xmlChar *) "device");
			if (deviceName == NULL)
				throw XmlMissingAttribute (ptr, "device");
		  	push_back (new ExpandStringValue ((char *) deviceName->children->content, (char *) ptr->children->content));
		}
		else
		{
			throw XmlUnexpectedNode (ptr);
		}
	}
}

std::string ExpandStrings::getString ()
{
	std::string ret;
	for (ExpandStrings::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getString ();
	return ret;
}

void EmailAction::parse (xmlNodePtr emailNode)
{
	if (emailNode->children == NULL)
		throw XmlEmptyNode (emailNode);

	for (xmlNodePtr ptr = emailNode->children; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->children == NULL)
			throw XmlEmptyNode (ptr);
		// check for to, cc, bcc, subject and body..
		else if (xmlStrEqual (ptr->name, (xmlChar *) "to"))
			to.push_back (std::string ((char *) ptr->children->content));
		else if (xmlStrEqual (ptr->name, (xmlChar *) "cc"))
			cc.push_back (std::string ((char *) ptr->children->content));
		else if (xmlStrEqual (ptr->name, (xmlChar *) "bcc"))
			bcc.push_back (std::string ((char *) ptr->children->content));
		else if (xmlStrEqual (ptr->name, (xmlChar *) "subject"))
		  	subject.expandXML (ptr->children);
		else if (xmlStrEqual (ptr->name, (xmlChar *) "body"))
		  	body.expandXML (ptr->children);
		else
			throw XmlUnexpectedNode (ptr);
	}
	if (to.size () == 0)
		throw XmlMissingElement (emailNode, "to");
	if (subject.size () == 0)
		throw XmlMissingElement (emailNode, "subject");
	if (body.size () == 0)
		throw XmlMissingElement (emailNode, "body");
}

std::string joinStrings (std::list <std::string> l, char jc = ',')
{
	std::list <std::string>::iterator iter = l.begin ();
	std::string ret = *iter;
	iter++;
	for (; iter != l.end (); iter++)
	{
		ret += jc + *iter;
	}
	return ret;
}

void EmailAction::run (XmlRpcd *_master, Rts2Conn *_conn, int validTime)
{
	if (_master->sendEmails ())
	{
		int ret;
		rts2core::ConnFork *cf = new rts2core::ConnFork (_master, Rts2Config::instance ()->getStringDefault ("xmlrpcd", "mail", "/usr/bin/mail"), true, 100);
		std::list <std::string>::iterator iter;
		if (bcc.size () != 0)
		{
			cf->addArg ("-b");
			cf->addArg (joinStrings (bcc));
		}
		if (cc.size () != 0)
		{
			cf->addArg ("-c");
			cf->addArg (joinStrings (cc));
		}
		cf->addArg ("-s");

		// expand subject..
		cf->addArg (subject.getString ());
		cf->addArg (joinStrings (to));

		// expand body..

		cf->setInput (body.getString () + "\n.\n");

		ret = cf->init ();
		if (ret)
		{
			delete cf;
			return;
		}

		_master->addConnection (cf);
	}
	else
	{
		logStream (MESSAGE_INFO) << "not sending email (sendEmail is disabled) to " << *(to.begin ())
		<< " with subject " << subject.getString () << " and body " << body.getString () << sendLog;
	}
}

/* 
 * State changes triggering infrastructure. 
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

#include "stateevents.h"
#include "message.h"

#include "../utils/rts2block.h"
#include "../utils/rts2logstream.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

using namespace rts2xmlrpc;

void
StateCommands::load (const char *file)
{
	clear ();

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	LIBXML_TEST_VERSION

	doc = xmlReadFile (file, NULL, 0);
	if (doc == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot parse XML file " << file << sendLog;
		return;
	}

	root_element = xmlDocGetRootElement (doc);

	if (strcmp ((const char *) root_element->name, "events"))
	{
		logStream (MESSAGE_ERROR) << "invalid root element name, expected events, is " << root_element->name << sendLog;
		return;
	}

	// traverse triggers..
	xmlNode *event = root_element->children;
	if (event == NULL)
	{
		logStream (MESSAGE_WARNING) << "no event specified" << sendLog;
		return;
	}
	for (; event; event=event->next)
	{
		if (event->type == XML_TEXT_NODE)
			continue;
		if (xmlStrEqual (event->name, (xmlChar *) "device"))
		{
			// parse it...
			std::string deviceName;
			int changeMask = INT_MAX;
			int newStateValue = INT_MAX;
			std::string commandName;

			// look for attributes
			xmlAttr *properties = event->properties;
			for (; properties; properties = properties->next)
			{
				if (properties->children == NULL)
				{
					logStream (MESSAGE_ERROR) << "empty property on line " << event->line << sendLog;
					return;
				}
				if (xmlStrEqual (properties->name, (xmlChar *) "name"))
				{
					deviceName = std::string ((char *) properties->children->content);
				}
				else if (xmlStrEqual (properties->name, (xmlChar *) "state-mask"))
				{
					changeMask = atoi ((char *) properties->children->content);
				}
				else if (xmlStrEqual (properties->name, (xmlChar *) "state"))
				{
					newStateValue = atoi ((char *) properties->children->content);
				}
			}
			xmlNode *action = event->children;
			if (action == NULL)
			{
				logStream (MESSAGE_ERROR) << "device on line " << event->line << " does not specify action type" << sendLog;
				return;
			}
			for (; action != NULL; action = action->next)
			{
				if (action->type == XML_TEXT_NODE)
					continue;
				if (xmlStrEqual (action->name, (xmlChar *) "command"))
				{
					if (action->children == NULL)
					{
						logStream (MESSAGE_ERROR) << "no action specified on line " << action->line << sendLog;
						return;
					}
					commandName = std::string ((char *) action->children->content);
					push_back (StateChangeCommand (deviceName, changeMask, newStateValue, commandName));
				}
				else
				{
					logStream (MESSAGE_ERROR) << "unknow action type on line " << action->line << sendLog;
					return;
				}
			}
		}
		else
		{
			logStream (MESSAGE_ERROR) << "unknow event type " << event->name
				<< " on line " << event->line << sendLog;
			return;
		}
	}

	xmlFreeDoc (doc);
	xmlCleanupParser ();
}

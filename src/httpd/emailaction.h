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
#ifndef __RTS2_EMAILACTION__
#define __RTS2_EMAILACTION__

#include <list>
#include <libxml/parser.h>
#include <string>

#include "rts2json/expandstrings.h"

namespace rts2xmlrpc
{

class HttpD;

class EmailAction
{
	public:
		EmailAction () {}

		void parse (xmlNodePtr emailNode, const char *defaultDeviceName);

		virtual void run (HttpD *_master);

	private:
		std::list <std::string> to;
		std::list <std::string> cc;
		std::list <std::string> bcc;

		rts2json::ExpandStrings subject;
		rts2json::ExpandStrings body;
};

}

#endif /* !__RTS2_EMAILACTION__ */

/*
 * Target from MPEC database.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/mpectarget.h"

#include "xmlrpc++/XmlRpc.h"
#include "xmlrpc++/urlencoding.h"

#include "configuration.h"

using namespace std;
using namespace rts2db;
using namespace XmlRpc;

MPECTarget::MPECTarget (const char *in_name):EllTarget ()
{
	setTargetName (in_name);
}

MPECTarget::~MPECTarget (void)
{
}

void MPECTarget::load ()
{
	const char *_uri;
	char* reply = NULL;
	int reply_length = 0;

	std::ostringstream os;
	std::string name (getTargetName ());
	urlencode (name);

	std::string mpecurl;

	rts2core::Configuration::instance ()->getString ("database", "mpecurl", mpecurl, "http://scully.cfa.harvard.edu/cgi-bin/mpeph2.cgi?ty=e&d=&l=&i=&u=d&uto=0&raty=a&s=t&m=m&adir=S&oed=&e=-1&tit=&bu=&ch=c&ce=f&js=f&TextArea=");

	os << mpecurl << name;

	char url[os.str ().length () + 1];
	strcpy (url, os.str ().c_str ());

	XmlRpcClient httpClient (url, &_uri);


	int ret = httpClient.executeGet (_uri, reply, reply_length);
	if (!ret)
	{
		std::ostringstream err;
	  	err << "error requesting " << url;
		throw rts2core::Error (err.str ());
	}
	ret = orbitFromMPC (reply);
	if (ret)
	{
		std::ostringstream err;
		err << "cannot parse " << reply;
		throw rts2core::Error (err.str ());
	}
}

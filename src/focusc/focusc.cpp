/* 
 * Command line focusing client.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include "focusclient.h"

#include <vector>
#include <iostream>

/**
 * Command-line fits writer.
 *
 * Client to test camera deamon.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class CFocusClient:public FocusClient
{
	protected:
		virtual FocusCameraClient * createFocCamera (rts2core::Rts2Connection * conn);
		virtual void help ();
	public:
		CFocusClient (int argc, char **argv);
};

FocusCameraClient * CFocusClient::createFocCamera (rts2core::Rts2Connection * conn)
{
	return new FocusCameraClient (conn, this);
}

void CFocusClient::help ()
{
	FocusClient::help ();
	std::cout << "Examples:" << std::endl
		<<
		"\trts2-focusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1"
		<< std::endl;
}

CFocusClient::CFocusClient (int in_argc, char **in_argv):FocusClient (in_argc, in_argv)
{
	autoSave = 1;
}

int main (int argc, char **argv)
{
	CFocusClient client = CFocusClient (argc, argv);
	return client.run ();
}

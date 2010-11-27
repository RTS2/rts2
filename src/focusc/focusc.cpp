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

#include "genfoc.h"

#include <vector>
#include <iostream>

/**
 * Command-line fits writer.
 *
 * Client to test camera deamon.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class Rts2focusc:public Rts2GenFocClient
{
	protected:
		virtual Rts2GenFocCamera * createFocCamera (Rts2Conn * conn);
		virtual void help ();
	public:
		Rts2focusc (int argc, char **argv);
};

Rts2GenFocCamera * Rts2focusc::createFocCamera (Rts2Conn * conn)
{
	return new Rts2GenFocCamera (conn, this);
}

void Rts2focusc::help ()
{
	Rts2GenFocClient::help ();
	std::cout << "Examples:" << std::endl
		<<
		"\trts2-focusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1"
		<< std::endl;
}

Rts2focusc::Rts2focusc (int in_argc, char **in_argv):Rts2GenFocClient (in_argc, in_argv)
{
	autoSave = 1;
}

int main (int argc, char **argv)
{
	Rts2focusc client = Rts2focusc (argc, argv);
	return client.run ();
}

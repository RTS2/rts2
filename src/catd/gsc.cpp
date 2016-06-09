/* 
 * GSC access server.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "catd.h"

using namespace rts2catd;

class GSC:public Catd
{
	public:
		GSC (int argc, char **argv);
		virtual ~GSC (void);

	protected:
		virtual int searchCataloge (struct ln_equ_posn *c1, struct ln_equ_posn *c2, int num);
};

GSC::GSC (int argc, char **argv):Catd (argc, argv)
{

}

GSC::~GSC ()
{

}


int GSC::searchCataloge (struct ln_equ_posn *c1, struct ln_equ_posn *c2, int num)
{
	return 0;
}

int main (int argc, char **argv)
{
	GSC gsc (argc, argv);
	return gsc.run ();
}

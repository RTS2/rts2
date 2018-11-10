/* 
 * Dummy rotator for tests.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "rotad.h"

namespace rts2rotad
{

/**
 * Simple dummy rotator. Shows how to use rotad library.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Dummy:public Rotator
{
	public:
		Dummy (int argc, char **argv):Rotator (argc, argv) {}
	protected:
		virtual void setTarget (double tv);
		virtual long isRotating ();
};

}

using namespace rts2rotad;

void Dummy::setTarget (__attribute__ ((unused)) double vt)
{
}

long Dummy::isRotating ()
{
	double diff = getDifference ();
	std::cout << "diff " << diff << " " << getCurrentPosition () << " " << getTargetPosition () << std::endl;
	if (fabs (diff) > 1)
	{
		setCurrentPosition (ln_range_degrees ((diff > 0) ? getCurrentPosition () - 1 : getCurrentPosition () + 1));
		return USEC_SEC * 0.2;
	}
	setCurrentPosition (getTargetPosition ());
	return -2;
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}

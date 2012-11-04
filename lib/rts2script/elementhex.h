/*
 * Script element for hex movement.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SEHEX__
#define __RTS2_SEHEX__

#include "rts2script/elementblock.h"
#include <libnova/libnova.h>

namespace rts2script
{

/**
 * Represents path of the telescope in pattern movement.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Path:public std::vector < struct ln_equ_posn *>
{
	private:
		Rts2Path::iterator top;
	public:
		Rts2Path ()
		{
			top = begin ();
		}
		virtual ~Rts2Path (void);

		void addRaDec (double in_ra, double in_dec);
		void endPath ()
		{
			top = begin ();
		}
		void rewindPath ()
		{
			top = begin ();
		}
		bool getNext ()
		{
			if (top != end ())
				top++;
			return haveNext ();
		}
		bool haveNext ()
		{
			return (top != end ());
		}
		struct ln_equ_posn *getTop ()
		{
			return *top;
		}
		double getRa ()
		{
			return getTop ()->ra;
		}
		double getDec ()
		{
			return getTop ()->dec;
		}
};

/**
 * HEX path change for telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementHex: public ElementBlock
{
	public:
		ElementHex (Script * in_script, char new_device[DEVICE_NAME_SIZE], double in_ra_size, double in_dec_size);
		virtual ~ ElementHex (void);

		virtual void beforeExecuting ();
	protected:
		Rts2Path path;

		virtual bool endLoop ();
		virtual bool getNextLoop ();

		virtual void constructPath ();
		virtual void afterBlockEnd ();
	private:
		double ra_size;
		double dec_size;
		ElementChange *changeEl;
		double getRa ();
		double getDec ();
};

/**
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementFxF: public ElementHex
{
	protected:
		virtual void constructPath ();
	public:
		ElementFxF (Script * in_script, char new_device[DEVICE_NAME_SIZE], double in_ra_size, double in_dec_size);
		virtual ~ ElementFxF (void);
};

}
#endif							 /* !__RTS2_SEHEX__ */

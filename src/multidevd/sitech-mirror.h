/**
 * Driver for sitech mirror rotator
 * Copyright (C) 2016 Petr Kubanek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "connection/sitech.h"
#include "mirror.h"
#include "sitech-multidev.h"

namespace rts2mirror
{

class SitechMirror:public Mirror, public SitechMultidev
{
	public:
		SitechMirror (const char *name, rts2core::ConnSitech *sitech_C, const char *defaults);


	protected:
		virtual int info ();

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual int movePosition (int pos);
		virtual int isMoving ();

		virtual int setValue (rts2core::Value* oldValue, rts2core::Value *newValue);

	private:
		rts2core::ValueLong *posA;
		rts2core::ValueLong *posB;

		rts2core::ValueLong *currPos;
		rts2core::ValueLong *tarPos;

		rts2core::ValueLong *moveSpeed;
};

}

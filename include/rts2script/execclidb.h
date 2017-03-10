/**
 * Executor client for camera with database backend..
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_EXECCLIDB__
#define __RTS2_EXECCLIDB__

#include "rts2script/execcli.h"

namespace rts2script
{

class DevClientCameraExecDb:public DevClientCameraExec
{
	public:
		DevClientCameraExecDb (rts2core::Rts2Connection * in_connection);
		virtual ~ DevClientCameraExecDb (void);
		virtual rts2image::Image *createImage (const struct timeval *expStart);
		virtual void beforeProcess (rts2image::Image * image);

	protected:
		virtual void exposureStarted (bool expectImage);
};

}
#endif							 /* !__RTS2_EXECCLIDB__ */

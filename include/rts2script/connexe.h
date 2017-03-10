/*
 * Abstract class for executor connection.
 * Copyright (C) 2010 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_CONNEXE__
#define __RTS2_CONNEXE__

#include "rts2script/element.h"
#include "connection/fork.h"

namespace rts2script {

/**
 * Abstract class for communicating with external script.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class ConnExe:public rts2core::ConnFork
{
	public:
		ConnExe (rts2core::Block * _master, const char *_exec, bool fillConnEnv, int timeout = 0);
		virtual ~ConnExe ();

		virtual void notActive () { active = false; }

		virtual void processLine ();

	protected:
		virtual void processCommand (char *cmd);

		virtual void processErrorLine (char *errbuf);

		rts2core::Rts2Connection *getConnectionForScript (const char *name);
		int getDeviceType (const char *name);

		bool checkActive (bool report = false);
	
	private:
		std::vector <std::string> tempentries;

		void testWritableVariable (const char *cmd, int32_t vflags, rts2core::Value *v);
		bool active;
};

}
#endif /* !__RTS2_CONNEXE__ */

/*
 * Script element for command execution.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_ELEMENTEXE__
#define __RTS2_ELEMENTEXE__

#include "element.h"
#include "../utils/connfork.h"

namespace rts2script
{

class Execute;

/**
 * Class for communicating with external script.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnExecute:public rts2core::ConnFork
{
	public:
		ConnExecute (Execute *_masterElement, Rts2Block * _master, const char *_exec);
		virtual ~ConnExecute ();

		virtual void processLine ();

		void nullMasterElement () { masterElement = NULL; }

		virtual void exposureEnd ();

		virtual int processImage (Rts2Image *image);

	protected:
		virtual void processErrorLine (char *errbuf);

		virtual void connectionError (int last_data_size);

	private:
		Execute *masterElement;

		// hold images..
		std::list <Rts2Image *> images;

		std::list <Rts2Image *>::iterator findImage (const char *path);

		bool isCentraldName (const char *_name) { return !strcmp (_name, ".") || !strcmp (_name, "centrald"); }
		Rts2Conn *getConnectionForScript (const char *name);
};


/**
 * Element for command execution. Command can communicta with RTS2 through
 * standart input and output.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Execute:public Element
{
	public:
		Execute (Script * _script, Rts2Block * _master, const char *_exec);
		virtual ~Execute ();

		virtual int defnextCommand (Rts2DevClient * _client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		Rts2Conn *getConnection () { return client->getConnection (); }
		Rts2DevClient *getClient () { return client; }

		virtual void exposureEnd ();

		virtual int processImage (Rts2Image *image);

		void deleteExecConn () { connExecute = NULL; }

	private:
		ConnExecute *connExecute;

		Rts2DevClient *client;

		Rts2Block *master;
		const char *exec;
};

}
#endif /* !__RTS2_ELEMENTEXE__ */

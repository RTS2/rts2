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

#include "rts2script/connexe.h"
#include "rts2script/element.h"
#include "connection/fork.h"
#include "rts2target.h"

namespace rts2script
{

class Execute;

/**
 * Class for communicating with external script.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnExecute:public ConnExe
{
	public:
		ConnExecute (Execute *_masterElement, rts2core::Block * _master, const char *_exec);
		virtual ~ConnExecute ();

		virtual void postEvent (rts2core::Event *event);

		virtual void notActive ();

		void nullMasterElement () { masterElement = NULL; }

		void errorReported (int current_state, int old_state);
		void exposureEnd (bool expectImage);
		void exposureFailed ();

		int processImage (rts2image::Image *image);
		bool knowImage (rts2image::Image *image);

	protected:
		virtual void processCommand (char *cmd);

		virtual void connectionError (int last_data_size);

	private:
		Execute *masterElement;

		// hold images..
		std::list <rts2image::Image *> images;

		std::list <rts2image::Image *>::iterator findImage (const char *path);

		int exposure_started;
		// if set, don't autodelete next acquired image
		bool keep_next_image;

		// if set, EVENT_MOVE_OK / FAILED will be handled and response will be printed on stdout
		bool waitTargetMove;

		void deleteImage (rts2image::Image *image)
		{
			if (!image->hasKeepImage ())
				delete image;
		}
};

/**
 * Element for command execution. Command can communicate with RTS2 through
 * standart input and output.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Execute:public Element
{
	public:
		Execute (Script * _script, rts2core::Block * _master, const char *_exec, Rts2Target *_target);
		virtual ~Execute ();

		virtual int defnextCommand (rts2core::DevClient * _client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		rts2core::Rts2Connection *getConnection () { return client->getConnection (); }
		rts2core::DevClient *getClient () { return client; }

		virtual void errorReported (int current_state, int old_state);
		virtual void exposureEnd (bool expectImage);
		virtual void exposureFailed ();

		virtual void notActive ();

		virtual int processImage (rts2image::Image *image);
		virtual bool knowImage (rts2image::Image *image);

		void deleteExecConn () { connExecute = NULL; }

		virtual void printScript (std::ostream &os) { os << COMMAND_EXE << " " << exec; }

		virtual void printXml (std::ostream &os) { os << "  <exe path='" << exec << "'/>"; }
		virtual void printJson (std::ostream &os) { os << "\"cmd\":\"" << COMMAND_EXE << "\",\"path\":\"" << exec << "\""; }

		/**
		 * Ask exe block to end whole script.
		 */
		void requestEndScript () { endScript = true; }

		Rts2Target *getTarget () { return target; }

	private:
		ConnExecute *connExecute;

		rts2core::DevClient *client;

		rts2core::Block *master;
		const char *exec;

		bool endScript;
		Rts2Target *target;
};

}
#endif /* !__RTS2_ELEMENTEXE__ */

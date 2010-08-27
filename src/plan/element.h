/*
 * Script element.
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

#ifndef __RTS2_SCRIPTELEMENT__
#define __RTS2_SCRIPTELEMENT__

#include "operands.h"
#include "rts2spiral.h"
#include "../writers/rts2image.h"
#include "../utils/rts2object.h"
#include "../utils/rts2block.h"

#include "status.h"

using namespace rts2core;

#define EVENT_PRECISION_REACHED   RTS2_LOCAL_EVENT + 250

#define EVENT_MIRROR_SET          RTS2_LOCAL_EVENT + 251
#define EVENT_MIRROR_FINISH       RTS2_LOCAL_EVENT + 252

#define EVENT_ACQUIRE_START       RTS2_LOCAL_EVENT + 253
#define EVENT_ACQUIRE_WAIT        RTS2_LOCAL_EVENT + 254
#define EVENT_ACQUIRE_QUERY       RTS2_LOCAL_EVENT + 255

// send some signal to other device..so they will
// know that something is going on
#define EVENT_SIGNAL              RTS2_LOCAL_EVENT + 256

#define EVENT_SIGNAL_QUERY        RTS2_LOCAL_EVENT + 257

// send when data we received
#define EVENT_STAR_DATA           RTS2_LOCAL_EVENT + 258

#define EVENT_ADD_FIXED_OFFSET    RTS2_LOCAL_EVENT + 259

// guiding data available
#define EVENT_GUIDING_DATA        RTS2_LOCAL_EVENT + 260
// ask for acquire state..
#define EVENT_GET_ACQUIRE_STATE   RTS2_LOCAL_EVENT + 261

namespace rts2script
{

/**
 * Helper class for EVENT_ACQUIRE_QUERY
 */
class AcquireQuery
{
	public:
		int tar_id;
		int count;
		AcquireQuery (int in_tar_id)
		{
			tar_id = in_tar_id;
			count = 0;
		}
};

class Script;

/**
 * This class defines one element in script, which is equal to one command in script.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Element:public Rts2Object
{
	public:
		Element (Script * _script);

		virtual ~ Element (void);

		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientPhot * phot, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		/**
		 * Called after end of exposure.
		 */
		virtual void exposureEnd () {}

		/**
		 * Query image processing.
		 *
		 * @return -1 if not handled, 0 if basic image processing should be performed, > 0 if image should not be deleted (deletion must be handled Element).
		 */
		virtual int processImage (Rts2Image * image);

		/**
		 * Returns 1 if we are waiting for that signal.
		 * Signal is > 0
		 */
		virtual int waitForSignal (int in_sig) { return 0; }

		/**
		 * That method will be called when we currently run that
		 * command and we would like to cancel observation.
		 *
		 * Should be used in children when appopriate.
		 */
		virtual void cancelCommands () {}

		/**
		 * This method will be called once after whole script is finished
		 * and before first item in script should be execute
		 */
		virtual void beforeExecuting ()	{}

		virtual int getStartPos ();

		void setLen (int in_len) { len = in_len; }

		virtual int getLen ();

		virtual int idleCall ();

		void setIdleTimeout (double sec);

		/**
		 * called every n-second, defined by setIdleTimeout function. Return
		 * NEXT_COMMAND_KEEP when we should KEEP command or NEXT_COMMAND_NEXT when
		 * script should advance to next command.
		 */
		virtual int idle ();

		virtual void prettyPrint (std::ostream &os) { os << "unknow element"; }
		virtual void printXml (std::ostream &os) {}

	protected:
		Script * script;
		virtual void getDevice (char new_device[DEVICE_NAME_SIZE]);
	private:
		int startPos;
		int len;
		struct timeval nextIdle;
		struct timeval idleTimeout;
};

class ElementExpose:public Element
{
	public:
		ElementExpose (Script * _script, float in_expTime);
		virtual int nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void prettyPrint (std::ostream &os) { os << "exposure " << expTime; }
		virtual void printXml (std::ostream &os) { os << "  <exposure length='" << expTime << "'/>"; }
	private:
		float expTime;
		enum {first, SHUTTER, EXPOSURE } callProgress;
};

class ElementDark:public Element
{
	public:
		ElementDark (Script * _script, float in_expTime);
		virtual int nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void prettyPrint (std::ostream &os) { os << "dark " << expTime; }
	private:
		float expTime;
		enum {first, SHUTTER, EXPOSURE } callProgress;
};

class ElementBox:public Element
{
	public:
		ElementBox (Script * _script, int in_x, int in_y, int in_w, int in_h);
		virtual int nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void prettyPrint (std::ostream &os) { os << "box " << x << " " << y << " " << w << " " << h; }
	private:
		int x, y, w, h;
};

class ElementCenter:public Element
{
	public:
		ElementCenter (Script * _script, int in_w, int in_h);
		virtual int nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void prettyPrint (std::ostream &os) { os << "camera center "; }
	private:
		int w, h;
};

class ElementChange:public Element
{
	public:
		ElementChange (Script * _script, char new_device[DEVICE_NAME_SIZE], double in_ra, double in_dec);
		virtual ~ElementChange (void) { delete [] deviceName; }
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		void setChangeRaDec (double in_ra, double in_dec)
		{
			ra = in_ra;
			dec = in_dec;
		}

		virtual void prettyPrint (std::ostream &os) { os << "offset " << ra << " " << dec; }
	private:
		char *deviceName;
		double ra;
		double dec;
};

class ElementWait:public Element
{
	public:
		ElementWait (Script * _script);
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void prettyPrint (std::ostream &os) { os << "wait "; }
};

class ElementWaitAcquire:public Element
{
	public:
		ElementWaitAcquire (Script * _script, int in_tar_id);
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
	private:
		// for which target shall we wait
		int tar_id;

};

class ElementPhotometer:public Element
{
	public:
		ElementPhotometer (Script * _script, int in_filter, float in_exposure, int in_count);
		virtual int nextCommand (Rts2DevClientPhot * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
	private:
		int filter;
		float exposure;
		int count;

};

class ElementSendSignal:public Element
{
	public:
		ElementSendSignal (Script * _script, int in_sig);
		virtual ~ ElementSendSignal (void);
		virtual void postEvent (Rts2Event * event);
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
	private:
		int sig;
		bool askedFor;
};

class ElementWaitSignal:public Element
{
	public:
		// in_sig must be > 0
		ElementWaitSignal (Script * _script, int in_sig);
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		virtual int waitForSignal (int in_sig);
	private:
		int sig;
};

/**
 * Set value.
 */
class ElementChangeValue:public Element
{
	public:
		ElementChangeValue (Script * _script, const char *in_device_value, const char *chng_str);
		virtual ~ ElementChangeValue (void);
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void prettyPrint (std::ostream &os) { os << "set on " << deviceName << " value " << valName << " " << op << " " << operands; }
		virtual void printXml (std::ostream &os) { os << "  <set device='" << deviceName << "' value='" << valName << "' op='" << op << "' operands='" << operands << "'/>"; }
	protected:
		virtual void getDevice (char new_device[DEVICE_NAME_SIZE]);
	private:
		char *deviceName;
		std::string valName;
		char op;
		rts2operands::OperandsSet operands;
		bool rawString;
};

/**
 * Class for comment.
 */
class ElementComment:public Element
{
	public:
		ElementComment (Script * _script, const char *in_comment, int in_cnum);
		virtual ~ ElementComment (void);
		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command,	char new_device[DEVICE_NAME_SIZE]);
	private:
		char *comment;
		// comment number
		int cnum;
};

}
#endif							 /* !__RTS2_SCRIPTELEMENT__ */

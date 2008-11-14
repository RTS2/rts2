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

#include "rts2script.h"
#include "rts2spiral.h"
#include "../writers/rts2image.h"
#include "../utils/rts2object.h"
#include "../utils/rts2block.h"

#include "status.h"

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

class Rts2Script;

/**
 * This class defines one element in script, which is equal to one command in script.
 *
 * @author Petr Kubanek <pkubanek@asu.cas.cz>
 */
class Rts2ScriptElement:public Rts2Object
{
	private:
		int startPos;
		int len;
		struct timeval nextIdle;
		struct timeval idleTimeout;
	protected:
		Rts2Script * script;
		virtual void getDevice (char new_device[DEVICE_NAME_SIZE]);
	public:
		Rts2ScriptElement (Rts2Script * in_script);
		virtual ~ Rts2ScriptElement (void);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int nextCommand (Rts2DevClientCamera * camera,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int nextCommand (Rts2DevClientPhot * phot,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int processImage (Rts2Image * image);
		/**
		 * Returns 1 if we are waiting for that signal.
		 * Signal is > 0
		 */
		virtual int waitForSignal (int in_sig)
		{
			return 0;
		}
		/**
		 * That method will be called when we currently run that
		 * command and we would like to cancel observation.
		 *
		 * Should be used in children when appopriate.
		 */
		virtual void cancelCommands ()
		{
		}
		/**
		 * This method will be called once after whole script is finished
		 * and before first item in script should be execute
		 */
		virtual void beforeExecuting ()
		{
		}
		virtual int getStartPos ();
		void setLen (int in_len)
		{
			len = in_len;
		}
		virtual int getLen ();

		virtual int idleCall ();

		void setIdleTimeout (double sec);

		/**
		 * called every n-second, defined by setIdleTimeout function. Return
		 * NEXT_COMMAND_KEEP when we should KEEP command or NEXT_COMMAND_NEXT when
		 * script should advance to next command.
		 */
		virtual int idle ();
};

class Rts2ScriptElementExpose:public Rts2ScriptElement
{
	private:
		float expTime;
	public:
		Rts2ScriptElementExpose (Rts2Script * in_script, float in_expTime);
		virtual int nextCommand (Rts2DevClientCamera * camera,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementDark:public Rts2ScriptElement
{
	private:
		float expTime;
	public:
		Rts2ScriptElementDark (Rts2Script * in_script, float in_expTime);
		virtual int nextCommand (Rts2DevClientCamera * camera,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementBox:public Rts2ScriptElement
{
	private:
		int x, y, w, h;
	public:
		Rts2ScriptElementBox (Rts2Script * in_script, int in_x, int in_y,
			int in_w, int in_h);
		virtual int nextCommand (Rts2DevClientCamera * camera,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementCenter:public Rts2ScriptElement
{
	private:
		int w, h;
	public:
		Rts2ScriptElementCenter (Rts2Script * in_script, int in_w, int in_h);
		virtual int nextCommand (Rts2DevClientCamera * camera,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementChange:public Rts2ScriptElement
{
	private:
		char *deviceName;
		double ra;
		double dec;
	public:
		Rts2ScriptElementChange (Rts2Script * in_script, char new_device[DEVICE_NAME_SIZE],
			double in_ra, double in_dec);
		virtual ~Rts2ScriptElementChange (void)
		{
			delete [] deviceName;
		}

		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		void setChangeRaDec (double in_ra, double in_dec)
		{
			ra = in_ra;
			dec = in_dec;
		}
};

class Rts2ScriptElementWait:public Rts2ScriptElement
{
	public:
		Rts2ScriptElementWait (Rts2Script * in_script);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementFilter:public Rts2ScriptElement
{
	private:
		int filter;
	public:
		Rts2ScriptElementFilter (Rts2Script * in_script, int in_filter);
		virtual int nextCommand (Rts2DevClientCamera * camera,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int nextCommand (Rts2DevClientPhot * phot,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementWaitAcquire:public Rts2ScriptElement
{
	private:
		// for which target shall we wait
		int tar_id;
	public:
		Rts2ScriptElementWaitAcquire (Rts2Script * in_script, int in_tar_id);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementMirror:public Rts2ScriptElement
{
	private:
		int mirror_pos;
		char *mirror_name;
	public:
		Rts2ScriptElementMirror (Rts2Script * in_script, char *in_mirror_name,
			int in_mirror_pos);
		virtual ~ Rts2ScriptElementMirror (void);
		virtual void postEvent (Rts2Event * event);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		void takeJob ()
		{
			mirror_pos = -1;
		}
		int getMirrorPos ()
		{
			return mirror_pos;
		}
		int isMirrorName (const char *in_name)
		{
			return !strcmp (mirror_name, in_name);
		}
};

class Rts2ScriptElementPhotometer:public Rts2ScriptElement
{
	private:
		int filter;
		float exposure;
		int count;
	public:
		Rts2ScriptElementPhotometer (Rts2Script * in_script, int in_filter,
			float in_exposure, int in_count);
		virtual int nextCommand (Rts2DevClientPhot * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementSendSignal:public Rts2ScriptElement
{
	private:
		int sig;
		bool askedFor;
	public:
		Rts2ScriptElementSendSignal (Rts2Script * in_script, int in_sig);
		virtual ~ Rts2ScriptElementSendSignal (void);
		virtual void postEvent (Rts2Event * event);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementWaitSignal:public Rts2ScriptElement
{
	private:
		int sig;
	public:
		// in_sig must be > 0
		Rts2ScriptElementWaitSignal (Rts2Script * in_script, int in_sig);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int waitForSignal (int in_sig);
};


/**
 * Set value.
 */
class Rts2ScriptElementChangeValue:public Rts2ScriptElement
{
	private:
		char *deviceName;
		std::string valName;
		char op;
		std::string operand;
		bool rawString;
	protected:
		virtual void getDevice (char new_device[DEVICE_NAME_SIZE]);
	public:
		Rts2ScriptElementChangeValue (Rts2Script * in_script,
			const char *in_device_value,
			const char *chng_str);
		virtual ~ Rts2ScriptElementChangeValue (void);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
};

/**
 * Class for comment.
 */
class Rts2ScriptElementComment:public Rts2ScriptElement
{
	private:
		char *comment;
		// comment number
		int cnum;
	public:
		Rts2ScriptElementComment (Rts2Script * in_script,
			const char *in_comment, int in_cnum);
		virtual ~ Rts2ScriptElementComment (void);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

};
#endif							 /* !__RTS2_SCRIPTELEMENT__ */

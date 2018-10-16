/* 
 * Abstract target class.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGET__
#define __RTS2_TARGET__

#include "block.h"

#include <libnova/libnova.h>
#include <time.h>
#include <ostream>

#define MAX_COMMAND_LENGTH     2000

// move was executed
#define OBS_BIT_MOVED          0x01
// observation started - expect some nice images in db
#define OBS_BIT_STARTED        0x02
// set while in acquisition
#define OBS_BIT_ACQUSITION     0x04
// when observation was interupted
#define OBS_BIT_INTERUPED      0x10
// when acqusition failed
#define OBS_BIT_ACQUSITION_FAI 0x20
// observation was processed
#define OBS_BIT_PROCESSED      0x40

#define TARGET_NAME_LEN         150

#define TYPE_UNKNOW             'u'

#define TYPE_OPORTUNITY         'O'
#define TYPE_GRB                'G'
#define TYPE_GRB_TEST           'g'
#define TYPE_SKY_SURVEY         'S'
#define TYPE_GPS                'P'
#define TYPE_ELLIPTICAL         'E'
#define TYPE_TLE                'B'
#define TYPE_HETE               'H'
#define TYPE_PHOTOMETRIC        'M'
#define TYPE_TECHNICAL          't'
#define TYPE_TERESTIAL          'T'
#define TYPE_CALIBRATION        'c'
#define TYPE_MODEL              'm'
#define TYPE_PLANET             'L'

#define TYPE_SWIFT_FOV          'W'
#define TYPE_INTEGRAL_FOV       'I'

#define TYPE_AUGER              'A'

#define TYPE_DARK               'd'
#define TYPE_FLAT               'f'

#define TYPE_LANDOLT            'l'

// master plan target
#define TYPE_PLAN               'p'

// send message when observation stars
#define SEND_START_OBS         0x01
// send message when first image from given observation get astrometry
#define SEND_ASTRO_OK          0x02
// send message at the end of observation
#define SEND_END_OBS           0x04
// send message at end of processing
#define SEND_END_PROC          0x08
// send message at end of night, with all observations and number of images obtained/processed
#define SEND_END_NIGHT         0x10


/**
 * Returns mask for target events.
 *
 * @param eventMask   Event which will be printed.
 * @return Character describing event mask.
 */
const char *getEventMaskName (int eventMask);


/**
 * Print all registered events to stream.
 *
 * @param eventMask  Event mask which will be printed.
 * @param _os        Stream which will recevive event mask description.
 */
void printEventMask (int eventMask, std::ostream & _os);

namespace rts2image
{
class Image;
}

typedef enum
{
	OBS_MOVE_FAILED = -1,
	OBS_MOVE = 0,
	OBS_ALREADY_STARTED,
	OBS_DONT_MOVE,
	OBS_MOVE_FIXED,
	OBS_MOVE_UNMODELLED,
	OBS_MOVE_MPEC,
	OBS_MOVE_TLE
} moveType;

/**
 * This abstract class defines target interface.
 * It's indendet to be used when a target is required.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @see Rts2TargetDb, Rts2ScriptTarget
 */
class Rts2Target
{

	public:
		Rts2Target ()
		{
			obs_id = -1;
			moveState = TARGET_NOT_MOVED;
			img_id = 0;
			tar_enabled = true;
			target_id = -1;
			obs_target_id = -1;
			obs_state = 0;
			selected = 0;
			acquired = 0;
			target_type = 0;
			target_name = NULL;
		}

		virtual ~ Rts2Target (void)
		{
		}

		/**
		 * Retrieve script for target. Throw an error if script cannot be found.
		 *
		 * @param device_name Name of the device.
		 * @param buf  Buffer with (part) of script.
		 *
		 * @retun true if script continues (next line,..), otherwise false
		 *
		 * @throw rts2core::Error if some error is encountered
		 */
		virtual bool getScript (const char *device_name, std::string & buf) = 0;

		/**
		 * Return target position at actual time.
		 *
		 * @param pos  Pointer to returned target position.
		 *
		 */
		virtual void getPosition (struct ln_equ_posn *pos) { getPosition (pos, ln_get_julian_from_sys ()); }

		/**
		 * Return target position at given time. This method must be
		 * implemented by target descendants.
		 *
		 * @param pos Pointer to returned target position.
		 * @param JD  Julian date for which target position is calculated.
		 */
		virtual void getPosition (struct ln_equ_posn *pos, double JD) = 0;

		// move functions

		void moveStarted () { moveState = TARGET_MOVE_STARTED; }

		void moveEnded () { moveState = TARGET_MOVE_ENDED; }

		void moveFailed () { moveState = TARGET_MOVE_FAILED; }

		bool moveWasStarted () { return (moveState != TARGET_NOT_MOVED); }

		bool wasMoved () { return (moveState == TARGET_MOVE_ENDED || moveState == TARGET_MOVE_FAILED); }

		int getCurrImgId () { return img_id; }

		int getNextImgId () { return ++img_id; }

		// set imgId to 0
		void nullImgId () { img_id = 0; }

		bool getTargetEnabled () { return tar_enabled; }
		void setTargetEnabled (bool new_en = true, bool logit = false)
		{
			if (tar_enabled != new_en)
			{
				if (logit)
					logStream (MESSAGE_INFO) << (new_en ? "Enable" : "Disable") << " target ID " << getTargetID () << " (" << getTargetName () << ") " << sendLog;
				tar_enabled = new_en;
			}
		}
		virtual int setNextObservable (time_t * time_ch) = 0;
		virtual void setTargetBonus (float new_bonus, time_t * new_time = NULL) = 0;

		virtual int save (bool overwrite) = 0;
		virtual int saveWithID (bool overwrite, int tar_id) = 0;

		int getTargetID () { return target_id; }
		virtual int getObsTargetID ()
		{
			if (obs_target_id > 0)
				return obs_target_id;
			return getTargetID ();
		}

		// called when we can move to next observation - good to generate next target in mosaic observation etc..
		virtual int beforeMove () { return 0; }

		virtual moveType startSlew (struct ln_equ_posn * position, std::string &p1, std::string &p2, bool update_position, int plan_id = -1) = 0;
		virtual int startObservation () = 0;

		int getObsId () { return obs_id; }

		int getSelected () { return selected; }

		/**
		 * Set observation ID and start observation.
		 *
		 * @param _obs_id Target observation ID.
		 */
		void setObsId (int _obs_id)
		{
			obs_id = _obs_id;
			selected++;
			obs_state |= OBS_BIT_MOVED;
		}

		int getObsState () { return obs_state; }

		// called when waiting for acqusition..
		int isAcquired () { return (acquired == 1); }
		int getAcquired () { return acquired; }
		void nullAcquired () { acquired = 0; }

		// return 0 when acquistion isn't running, non 0 when we are currently
		// acquiring target (searching for correct field)
		int isAcquiring () { return (obs_state & OBS_BIT_ACQUSITION); }
		void obsStarted () { obs_state |= OBS_BIT_STARTED; }

		virtual void acqusitionStart () { obs_state |= OBS_BIT_ACQUSITION; }

		void acqusitionEnd ()
		{
			obs_state &= ~OBS_BIT_ACQUSITION;
			acquired = 1;
		}

		void interupted () { obs_state |= OBS_BIT_INTERUPED; }

		void acqusitionFailed ()
		{
			obs_state |= OBS_BIT_ACQUSITION_FAI;
			acquired = -1;
		}

		virtual void writeToImage (rts2image::Image * image, double JD) = 0;

		int getEpoch () { return epochId; }

		void setEpoch (int in_epochId) { epochId = in_epochId; }

		char getTargetType () { return target_type; }
		void setTargetType (char in_target_type) { target_type = in_target_type; }

		/**
		 * Returns true if target position (RA, DEC) does not change with time.
		 */
		virtual bool hasConstantPosition () { return false; }

		const char *getTargetName () { return target_name; }

		/**
		 * Set target name.
		 *
		 * @param _target_name  Target name.
		 */
		void setTargetName (const char *_target_name)
		{
			delete[]target_name;
			target_name = new char[strlen (_target_name) + 1];
			strcpy (target_name, _target_name);
		}

		/**
		 * Check if target ID is equal to given value.
		 *
		 * @param v integer to check with
		 */
		bool operator == (const int &v) { return getTargetID () == v; }
	protected:
		int target_id;
		int obs_target_id;
		char target_type;
		char *target_name;
	private:
		int obs_id;
		enum { TARGET_NOT_MOVED, TARGET_MOVE_STARTED, TARGET_MOVE_ENDED, TARGET_MOVE_FAILED } moveState;
		int img_id;				 // count for images
		bool tar_enabled;
		// mask with 0xf0 - 0x00 - nominal end 0x10 - interupted 0x20 - acqusition don't converge
		int obs_state;			 // 0 - not started 0x01 - slew started 0x02 - images taken 0x04 - acquistion started
		int selected;			 // how many times startObservation was called
		int acquired;
		int epochId;
};

#endif							 /* !__RTS2_TARGET__ */

/* 
 * Basic event definition.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_EVENT__
#define __RTS2_EVENT__

#include <string>

#define EVENT_WRITE_TO_IMAGE        2
#define EVENT_WRITE_TO_IMAGE_ENDS  13
#define EVENT_SET_TARGET            3
#define EVENT_OBSERVE               4
#define EVENT_IMAGE_OK              5

#define EVENT_QUERY_WAIT            6
#define EVENT_ENTER_WAIT            7
#define EVENT_CLEAR_WAIT            8

#define EVENT_GET_RADEC            10
#define EVENT_MOUNT_CHANGE         11

#define EVENT_QUICK_ENABLE         12

// info failed/sucess calls
#define EVENT_INFO_DEVCLI_OK       14
#define EVENT_INFO_DEVCLI_FAILED   15

/** Event issued when command return with OK state. */
#define EVENT_COMMAND_OK           16
/** Event issued when command failed. */
#define EVENT_COMMAND_FAILED       17

/** Number of images which waits for processing. */
#define EVENT_NUMBER_OF_IMAGES     18

/** Notification to refresh device information..*/
#define EVENT_TIMER_INFOALL        19

/** Reconnect for TCP connections. */
#define EVENT_TCP_RECONECT_TIMER   20

// events number below that number shoudl be considered RTS2-reserved
#define RTS2_LOCAL_EVENT         1000

// local events are defined in local headers!
// so far those offeset ranges (from RTS2_LOCAL_EVENT) are reserved for:
//
// rts2execcli.h            50- 99
// rts2script.h            200-249
// rts2scriptelement.h     250-299
// rts2devclifoc.h         500-549
// rts2devclicop.h         550-599
// grbd.h                  600-649
// rts2devcliwheel.h       650-699
// augershooter.h          700-749
// rts2devclifocuser.h     750-799
// rts2loggerbase.h        800-849
// rts2soapclient.h       1000-1200
// telds                  1200-1250
// phot                   1250-1300

/**
 * Event class.
 *
 * This class defines event, which is passed by Rts2Object::postEvent through all
 * classes in RTS2.
 * 
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Event
{
	private:
		int type;
		void *arg;
	public:
		/**
		 * Construct event.
		 *
		 * @param in_type Event type.
		 * @param in_arg  Optional pointer to event argument.
		 */
		Rts2Event (int in_type, void *in_arg = NULL)
		{
			type = in_type;
			arg = in_arg;
		}

		/**
		 * Copy constructor to construct event from the
		 * same event.
		 *
		 * @param event  Event which will be copied.
		 */
		Rts2Event (Rts2Event * event)
		{
			type = event->getType ();
			arg = event->getArg ();
		}

		/**
		 * Return event type.
		 *
		 * @return Event type.
		 */
		int getType ()
		{
			return type;
		}

		/**
		 * Return event argument.
		 *
		 * @return Event argument.
		 */
		void *getArg ()
		{
			return arg;
		}

		/**
		 * Set event argument.
		 *
		 * @param in_arg New event argument.
		 */
		void setArg (void *in_arg)
		{
			arg = in_arg;
		}
};
#endif							 /*! __RTS2_EVENT__ */

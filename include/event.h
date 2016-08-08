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

/**
 * Event fired when FITS headers should be recorded.
 *
 * @param arg  rts2image::CameraImage pointer
 */
#define EVENT_WRITE_TO_IMAGE              2

/**
 * Only collect and write all headers.
 *
 * @param arg  rts2image::Image pointer
 */
#define EVENT_WRITE_ONLY_IMAGE            3

/**
 * Event fired at the end of exposure, when certain headers should be recorded
 * to FITS file.
 *
 * @param arg  rts2image::CameraImage pointer
 */
#define EVENT_WRITE_TO_IMAGE_ENDS         4
#define EVENT_SET_TARGET                  5
#define EVENT_SET_TARGET_NOT_CLEAR        6 
#define EVENT_OBSERVE                     7
#define EVENT_IMAGE_OK                    8

#define EVENT_QUERY_WAIT                  9
#define EVENT_ENTER_WAIT                 10
#define EVENT_CLEAR_WAIT                 11

#define EVENT_GET_RADEC                  12
#define EVENT_MOUNT_CHANGE               13

#define EVENT_QUICK_ENABLE               14

/**
 * Device info command has been sucessfully executed.
 */
#define EVENT_INFO_DEVCLI_OK             15

/**
 * Device info command execution failed.
 */
#define EVENT_INFO_DEVCLI_FAILED         16

/** Event issued when command return with OK state. */
#define EVENT_COMMAND_OK                 17
/** Event issued when command failed. */
#define EVENT_COMMAND_FAILED             18

/** Number of images which waits for processing. */
#define EVENT_NUMBER_OF_IMAGES           19

/** Notification to refresh device information..*/
#define EVENT_TIMER_INFOALL              20

/** Reconnect for TCP connections. */
#define EVENT_TCP_RECONECT_TIMER         21

/** Recconect for UDP connections. */
#define EVENT_UDP_RECONECT_TIMER         22

/** Called on change of the trigger state. */
#define EVENT_TRIGGERED                  23

/** Lost connection to the database. */
#define EVENT_DB_LOST_CONN               24

/** Set target, killall command. */
#define EVENT_SET_TARGET_KILL            25

/** Set target, run killall, don't run scriptends. */
#define EVENT_SET_TARGET_KILL_NOT_CLEAR  26

// events number below that number shoudl be considered RTS2-reserved
#define RTS2_LOCAL_EVENT         1000

// local events are defined in local headers!
// so far those offeset ranges (from RTS2_LOCAL_EVENT) are reserved for:
//
// execcli.h                50- 99
// script.h                200-249
// scriptelement.h         250-299
// devclifoc.h             500-524
// devcliimg.h             525-549
// clicupola.h             550-569
// clirotator.h            570-599
// grbd.h                  600-649
// devcliwheel.h           650-675
// camd.h                  676-699
// augershooter.h          700-749
// devclifocuser.h         750-799
// loggerbase.h            800-849
// xmlrpcd                 850-900
// telds                  1200-1249
// phot                   1250-1299
// clouds                 1300-1349
// zelio                  1350-1399
// executorque.h          1400-1449
// nmonitor.h             1450-1499
// bb.h                   1500-1549
// apm-aux.h              1550-1599

// local (device,..)     10000-

namespace rts2core
{

/**
 * Event class.
 *
 * This class defines event, which is passed by Object::postEvent through all
 * classes in RTS2.
 * 
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Event
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
		Event (int in_type, void *in_arg = NULL)
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
		Event (Event * event)
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

}
#endif							 /*! __RTS2_EVENT__ */

/* 
 * Base RTS2 class.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_OBJECT__
#define __RTS2_OBJECT__

#include "event.h"

namespace rts2core
{

/**
 * Base RTS2 class.
 *
 * Its only important method is Object::postEvent, which works as
 * event hook for descenadands.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Object
{
	public:
		/**
		 * Default object constructor.
		 */
		Object () {}

		/**
		 * Default object destructor.
		 */
		virtual ~ Object (void) {}

		/**
		 * Distribute event throught class.
		 *
		 * This method is hook for descendands to distribute Event. Every descendand
		 * overwriting it should call ancestor postEvent method at the end, so the event object
		 * will be deleted.
		 *
		 * Ussuall implementation looks like:
		 *
		@code
		void MyObject::postEvent (Event * event)
		{
		// test for event type..
		switch (event->getType ())
		{
		  case MY_EVENT_1:
			do_something_with_event_1;
			break;
		  case MY_EVENT_2:
			do_something_with_event_2;
			break;
		}
		MyParent::postEvent (event);
		}
		@endcode
		 *
		 * @warning { If you do not call parent postEvent method, and you do not delete event, event object
		 * will stay in memory indefinitly. Most propably your application will shortly run out of memory and
		 * core dump. }
		 *
		 * @param event Event to process.
		 *
		 * @see Event::getType
		 */
		virtual void postEvent (Event * event)
		{
			// discard event..
			delete event;
		}

		/**
		 * Called forked instance is run. This method can be used to close connecitns which should not
		 * stay open in forked process.
		 */
		virtual void forkedInstance ()
		{
		}
};

}
#endif							 /*! __RTS2_OBJECT__ */

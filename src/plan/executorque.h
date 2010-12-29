/*
 * Executor queue.
 * Copyright (C) 2010      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"

namespace rts2plan
{

/**
 * Executor queue. Used to freely create queue inside executor
 * for queue execution. Allow users to define rules how the queue
 * should be used, provides method to support basic queue operations.
 * 
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ExecutorQueue:public std::list <rts2db::Target *>
{
	public:
		ExecutorQueue (Rts2DeviceDb *master, const char *name);
		virtual ~ExecutorQueue ();

		int addFront (rts2db::Target *nt);
		int addTarget (rts2db::Target *nt);

		void popFront ();

		void clearNext ();

	private:
		Rts2DeviceDb *master;

		rts2core::IntegerArray *nextIds;
		rts2core::StringArray *nextNames;

		// update values from the target list
		void updateVals ();
};

}

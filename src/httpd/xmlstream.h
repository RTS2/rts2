/* 
 * Routines to put variables to XML-RPC.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2__XMLSTREAM__
#define __RTS2__XMLSTREAM__

#include <ostream>
#include "xmlrpc++/XmlRpc.h"
#include "infoval.h"

using namespace XmlRpc;

/**
 * Prints variables to XML-RPC value.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlStream: public Rts2InfoValStream
{
	private:
		XmlRpcValue *rpcval;

	public:
		XmlStream (XmlRpcValue *in_val): Rts2InfoValStream ()
		{
			rpcval = in_val;
		}

		virtual void printInfoVal (const char *desc, const char* val)
		{
			(*rpcval)[desc] = val;
		}

		virtual void printInfoVal (const char *desc, const std::string &val)
		{
			(*rpcval)[desc] = val;
		}

		virtual void printInfoVal (const char *desc, int val)
		{
			(*rpcval)[desc] = val;
		}

		virtual void printInfoVal (const char *desc, unsigned int val)
		{
			(*rpcval)[desc] = (int) val;
		}

		virtual void printInfoVal (const char *desc, double val)
		{
			(*rpcval)[desc] = val;
		}

		virtual void printInfoVal (const char *desc, LibnovaDeg &val)
		{
			(*rpcval)[desc] = val.getDeg ();
		}

		virtual void printInfoVal (const char *desc, LibnovaRa &val)
		{
			(*rpcval)[desc] = val.getRa ();
		}

		virtual void printInfoVal (const char *desc, LibnovaDegArcMin &val)
		{
			(*rpcval)[desc] = val.getDeg ();
		}

		virtual void printInfoVal (const char *desc, Timestamp &val)
		{
			struct tm tm_s;
			if (std::isnan (val.getTs ()))
			{
				(*rpcval)[desc]="NULL";
			}
			else
			{
				time_t tt = (time_t) val.getTs ();
				gmtime_r (&tt, &tm_s);
				(*rpcval)[desc] = XmlRpcValue (&tm_s);
			}
		}

		virtual void printInfoVal (const char *desc, TimeDiff &val)
		{
			(*rpcval)[desc] = val.getTimeDiff ();
		}
};
#endif							 // !__RTS2__XMLSTREAM__

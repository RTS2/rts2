/* 
 * Target display and manipulation.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGETREQ__
#define __RTS2_TARGETREQ__

#include "config.h"
#include "httpreq.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/targetset.h"
#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1
#include <Magick++.h>
#endif // HAVE_LIBJPEG
#include "../utils/rts2config.h"
#include "../utils/rts2device.h"
#else

#endif // HAVE_PGSQL

#include "xmlrpc++/XmlRpc.h"

#include "../utils/libnova_cpp.h"

namespace rts2xmlrpc
{

#ifdef HAVE_PGSQL

class Targets: public GetRequestAuthorized
{
	public:
		Targets (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	
	private:
		void listTargets (const char* &response_type, char* &response, size_t &response_length);
		void printTarget (Target *tar, const char* &response_type, char* &response, size_t &response_length);
		void printTargetImages (Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void printTargetObservations (Target *tar, const char* &response_type, char* &response, size_t &response_length);
#ifdef HAVE_LIBJPEG
		void plotTarget (Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
#endif /* HAVE_LIBJPEG */
};

/**
 * Add target to the database. Consists of those steps:
 *

 * @author Petr Kubanek <petr@kubanek.net>
 */
class AddTarget: public GetRequestAuthorized
{
	public:
		AddTarget (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		void askForTarget (const char* &response_type, char* &response, size_t &response_length);
		void confimTarget (const char *tar, const char* &response_type, char* &response, size_t &response_length);
		void newTarget (const char *oriname, const char *name, int tarid, double ra, double dec, const char* &response_type, char* &response, size_t &response_length);
		void schedule (int tarid, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* HAVE_PGSQL */

}

#endif /* !__RTS2_TARGETREQ__ */

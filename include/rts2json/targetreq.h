/* 
 * Target display and manipulation.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2-config.h"
#include "httpreq.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/targetset.h"
#include "rts2db/camlist.h"
#if defined(RTS2_HAVE_LIBJPEG)
#include <Magick++.h>
#endif // RTS2_HAVE_LIBJPEG
#include "configuration.h"
#else

#endif // RTS2_HAVE_PGSQL

#include "xmlrpc++/XmlRpc.h"

#include "libnova_cpp.h"

namespace rts2json
{

#ifdef RTS2_HAVE_PGSQL

class Targets: public rts2json::GetRequestAuthorized
{
	public:
		Targets (const char *prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer *s);
		virtual void authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	
	private:
		bool displaySeconds;

		void listTargets (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void processForm (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void processAPI (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void printTargetHeader (int tar_id, const char *current, std::ostringstream &_os);
		void callAPI (rts2db::Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void callTargetAPI (rts2db::Target *tar, const std::string &req, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void printTarget (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length);
		void printTargetInfo (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length);
		void printTargetStat (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length);
		void printTargetImages (rts2db::Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void printTargetObservations (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length);
		void printTargetPlan (rts2db::Target *tar, const char* &response_type, char* &response, size_t &response_length);

		rts2db::CamList cameras;

#ifdef RTS2_HAVE_LIBJPEG
		void plotTarget (rts2db::Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
#endif /* RTS2_HAVE_LIBJPEG */
};

#endif /* RTS2_HAVE_PGSQL */

}

#endif /* !__RTS2_TARGETREQ__ */

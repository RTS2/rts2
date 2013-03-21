/* 
 * JSON database API calls.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_JSONDB__
#define __RTS2_JSONDB__

#include "httpreq.h"

#include "rts2db/observationset.h"

#include <string>
#include <vector>

namespace rts2json
{

rts2db::Target * getTarget (XmlRpc::HttpParams *params, const char *paramname = "id");

class JSONDBRequest:public JSONRequest
{
	public:
		JSONDBRequest (const char *prefix, HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):JSONRequest (prefix, _http_server, s) {}

	protected:
		/**
		 * Process JSON API DB requests.
		 */
		void dbJSON (const std::vector <std::string> vals, XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, std::ostringstream &os);

		void jsonTargets (rts2db::TargetSet &tar_set, std::ostringstream &os, XmlRpc::HttpParams *params, struct ln_equ_posn *dfrom = NULL, XmlRpc::XmlRpcServerConnection *chunked = NULL);
		void jsonObservation (rts2db::Observation *obs, std::ostream &os);
		void jsonObservations (rts2db::ObservationSet *obss, std::ostream &os);
		void jsonImages (rts2db::ImageSet *img_set, std::ostream &os, XmlRpc::HttpParams *params);
		void jsonLabels (rts2db::Target *tar, std::ostream &os);
};

}

#endif // !__RTS2_JSONDB__

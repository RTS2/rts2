/* 
 * JavaScript libraries for AJAX web access.
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

#include "libjavascript.h"
#include "utilsfunc.h"
#include "error.h"

using namespace rts2bb;

const char *setGetApi =
"function jsonCall (url, func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET', url, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
 "hr.send(null);\n"
"}\n"

"function getCallFunction (observatory, device, variable, func){\n"
  "jsonCall ('../o/' + observatory + '/api/get?d=' + device, function (ret) { func(ret.d[variable]); });\n"
"}\n"

"function getCall (observatory, device, variable, input){\n"
  "getCallFunction (observatory, device, variable, function(v) { input.value = v;});\n"
"}\n"

"function getCallTime (observatory, device, variable, input, show_diff){\n"
  "getCallFunction (observatory, device, variable, function(v) {"
    "var now = new Date();\n"
    "var d = new Date(v * 1000);\n"
    "var s = d.toString();\n"
    "if (show_diff){ s += ' ' + (now.valueOf() - d.valueOf()) / 1000.0;}\n"
    "input.value = s;\n"
  "});\n"
"}\n"

"function refreshDeviceTable (observatory, device){\n"
  "func = function(ret){\n"
    "for (v in ret.d) {\n"
      "document.getElementById(device + '_' + v).innerHTML = ret.d[v];\n"
    "}\n"  
  "}\n"
  "jsonCall('../api/' + observatory + '/api/get?d=' + device + '&e=1', func);\n"
"}\n"

"function deviceGetAll(observatory, device, func){\n"
  "jsonCall('../api/' + observatory + '/api/get?d=' + device, func);\n"
"}\n"
;

void LibJavaScript::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	const char *reply = NULL;
	
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	cacheMaxAge (CACHE_MAX_STATIC);

	if (vals.size () != 1)
		throw rts2core::Error ("File not found");

	else if (vals[0] == "setgetapi.js")
	  	reply = setGetApi;
	else
		throw rts2core::Error ("JavaScript not found");

	response_length = strlen (reply);
	response = new char[response_length];
	response_type = "text/javascript";
	memcpy (response, reply, response_length);
}

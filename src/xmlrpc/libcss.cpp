/*
 * CSS for AJAX web access.
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

#include "libcss.h"
#include "utilsfunc.h"
#include "error.h"

using namespace rts2xmlrpc;

static const char *calendarCss = 
"#CalendarControlIFrame {\n"
"  display: none;\n"
"  left: 0px;\n"
"  position: absolute;\n"
"  top: 0px;\n"
"  height: 250px;\n"
"  width: 250px;\n"
"  z-index: 99;\n"
"}\n"
"\n"
"#CalendarControl {\n"
"  position:absolute;\n"
"  background-color:#FFF;\n"
"  margin:0;\n"
"  padding:0;\n"
"  display:none;\n"
"  z-index: 100;\n"
"}\n"
"\n"
"#CalendarControl table {\n"
"  font-family: arial, verdana, helvetica, sans-serif;\n"
"  font-size: 8pt;\n"
"  border-left: 1px solid #336;\n"
"  border-right: 1px solid #336;\n"
"}\n"
"\n"
"#CalendarControl th {\n"
"  font-weight: normal;\n"
"}\n"
"\n"
"#CalendarControl th a {\n"
"  font-weight: normal;\n"
"  text-decoration: none;\n"
"  color: #FFF;\n"
"  padding: 1px;\n"
"}\n"
"\n"
"#CalendarControl td {\n"
"  text-align: center;\n"
"}\n"
"\n"
"#CalendarControl .header {\n"
"  background-color: #336;\n"
"}\n"
"\n"
"#CalendarControl .weekday {\n"
"  background-color: #DDD;\n"
"  color: #000;\n"
"}\n"
"\n"
"#CalendarControl .weekend {\n"
"  background-color: #FFC;\n"
"  color: #000;\n"
"}\n"
"\n"
"#CalendarControl .current {\n"
"  border: 1px solid #339;\n"
"  background-color: #336;\n"
"  color: #FFF;\n"
"}\n"
"\n"
"#CalendarControl .weekday,\n"
"#CalendarControl .weekend,\n"
"#CalendarControl .current {\n"
"  display: block;\n"
"  text-decoration: none;\n"
"  border: 1px solid #FFF;\n"
"  width: 2em;\n"
"}\n"
"\n"
"#CalendarControl .weekday:hover,\n"
"#CalendarControl .weekend:hover,\n"
"#CalendarControl .current:hover {\n"
"  color: #FFF;\n"
"  background-color: #336;\n"
"  border: 1px solid #999;\n"
"}\n"
"\n"
"#CalendarControl .previous {\n"
"  text-align: left;\n"
"}\n"
"\n"
"#CalendarControl .next {\n"
"  text-align: right;\n"
"}\n"
"\n"
"#CalendarControl .previous,\n"
"#CalendarControl .next {\n"
"  padding: 1px 3px 1px 3px;\n"
"  font-size: 1.4em;\n"
"}\n"
"\n"
"#CalendarControl .previous a,\n"
"#CalendarControl .next a {\n"
"  color: #FFF;\n"
"  text-decoration: none;\n"
"  font-weight: bold;\n"
"}\n"
"\n"
"#CalendarControl .title {\n"
"  text-align: center;\n"
"  font-weight: bold;\n"
"  color: #FFF;\n"
"}\n"
"\n"
"#CalendarControl .empty {\n"
"  background-color: #CCC;\n"
"  border: 1px solid #FFF;\n"
"}\n";

const char *tableCss =
".time {\n"
  "text-align:right;\n"
"}\n"
".deg {\n"
  "text-align:right;\n"
"}\n"  
".TableHead {\n"
  "background-color: #CCC;\n"
  "border: 1px solid #000;\n"
  "-moz-border-radius: 10px;\n"
  "-webkit-border-radius: 10px;\n"
  "border-radius: 10px;\n"
  "cursor: pointer;\n"
"}\n";

void LibCSS::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	const char *reply = NULL;
	
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
	if (vals.size () != 1)
		throw rts2core::Error ("File not found");

	if (vals[0] == "calendar.css")
		reply = calendarCss;
	else if (vals[0] == "table.css")
		reply = tableCss;
	else 
		throw rts2core::Error ("CSS not found");

	response_length = strlen (reply);
	response = new char[response_length];
	response_type = "text/css";
	memcpy (response, reply, response_length);
}

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

#include "rts2json/libcss.h"
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

"#CalendarControl {\n"
"  position:absolute;\n"
"  background-color:#FFF;\n"
"  margin:0;\n"
"  padding:0;\n"
"  display:none;\n"
"  z-index: 100;\n"
"}\n"

"#CalendarControl table {\n"
"  font-family: arial, verdana, helvetica, sans-serif;\n"
"  font-size: 8pt;\n"
"  border-left: 1px solid #336;\n"
"  border-right: 1px solid #336;\n"
"}\n"

"#CalendarControl th {\n"
"  font-weight: normal;\n"
"}\n"

"#CalendarControl th a {\n"
"  font-weight: normal;\n"
"  text-decoration: none;\n"
"  color: #FFF;\n"
"  padding: 1px;\n"
"}\n"

"#CalendarControl td {\n"
"  text-align: center;\n"
"}\n"

"#CalendarControl .header {\n"
"  background-color: #336;\n"
"}\n"

"#CalendarControl .weekday {\n"
"  background-color: #DDD;\n"
"  color: #000;\n"
"}\n"

"#CalendarControl .weekend {\n"
"  background-color: #FFC;\n"
"  color: #000;\n"
"}\n"

"#CalendarControl .current {\n"
"  border: 1px solid #339;\n"
"  background-color: #336;\n"
"  color: #FFF;\n"
"}\n"

"#CalendarControl .weekday,\n"
"#CalendarControl .weekend,\n"
"#CalendarControl .current {\n"
"  display: block;\n"
"  text-decoration: none;\n"
"  border: 1px solid #FFF;\n"
"  width: 2em;\n"
"}\n"

"#CalendarControl .weekday:hover,\n"
"#CalendarControl .weekend:hover,\n"
"#CalendarControl .current:hover {\n"
"  color: #FFF;\n"
"  background-color: #336;\n"
"  border: 1px solid #999;\n"
"}\n"

"#CalendarControl .previous {\n"
"  text-align: left;\n"
"}\n"

"#CalendarControl .next {\n"
"  text-align: right;\n"
"}\n"

"#CalendarControl .previous,\n"
"#CalendarControl .next {\n"
"  padding: 1px 3px 1px 3px;\n"
"  font-size: 1.4em;\n"
"}\n"

"#CalendarControl .previous a,\n"
"#CalendarControl .next a {\n"
"  color: #FFF;\n"
"  text-decoration: none;\n"
"  font-weight: bold;\n"
"}\n"

"#CalendarControl .title {\n"
"  text-align: center;\n"
"  font-weight: bold;\n"
"  color: #FFF;\n"
"}\n"

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

const char *datatablesCss = 
".dataTables_wrapper {\n"
	"position: relative;\n"
	"clear: both;\n"
	"zoom: 1; /* Feeling sorry for IE */\n"
"}\n"

".dataTables_processing {\n"
	"position: absolute;\n"
	"top: 50%;\n"
	"left: 50%;\n"
	"width: 250px;\n"
	"height: 30px;\n"
	"margin-left: -125px;\n"
	"margin-top: -15px;\n"
	"padding: 14px 0 2px 0;\n"
	"border: 1px solid #ddd;\n"
	"text-align: center;\n"
	"color: #999;\n"
	"font-size: 14px;\n"
	"background-color: white;\n"
"}\n"

".dataTables_length {\n"
	"width: 40%;\n"
	"float: left;\n"
"}\n"

".dataTables_filter {\n"
	"width: 50%;\n"
	"float: right;\n"
	"text-align: right;\n"
"}\n"

".dataTables_info {\n"
	"width: 60%;\n"
	"float: left;\n"
"}\n"

".dataTables_paginate {\n"
	"float: right;\n"
	"text-align: right;\n"
"}\n"

"/* Pagination nested */\n"
".paginate_disabled_previous, .paginate_enabled_previous,\n"
".paginate_disabled_next, .paginate_enabled_next {\n"
	"height: 19px;\n"
	"float: left;\n"
	"cursor: pointer;\n"
	"*cursor: hand;\n"
	"color: #111 !important;\n"
"}\n"
".paginate_disabled_previous:hover, .paginate_enabled_previous:hover,\n"
".paginate_disabled_next:hover, .paginate_enabled_next:hover {\n"
	"text-decoration: none !important;\n"
"}\n"
".paginate_disabled_previous:active, .paginate_enabled_previous:active,\n"
".paginate_disabled_next:active, .paginate_enabled_next:active {\n"
	"outline: none;\n"
"}\n"

".paginate_disabled_previous,\n"
".paginate_disabled_next {\n"
	"color: #666 !important;\n"
"}\n"
".paginate_disabled_previous, .paginate_enabled_previous {\n"
	"padding-left: 23px;\n"
"}\n"
".paginate_disabled_next, .paginate_enabled_next {\n"
	"padding-right: 23px;\n"
	"margin-left: 10px;\n"
"}\n"

".paginate_disabled_previous {\n"
	"background: url('../images/back_disabled.png') no-repeat top left;\n"
"}\n"

".paginate_enabled_previous {\n"
	"background: url('../images/back_enabled.png') no-repeat top left;\n"
"}\n"
".paginate_enabled_previous:hover {\n"
	"background: url('../images/back_enabled_hover.png') no-repeat top left;\n"
"}\n"

".paginate_disabled_next {\n"
	"background: url('../images/forward_disabled.png') no-repeat top right;\n"
"}\n"

".paginate_enabled_next {\n"
	"background: url('../images/forward_enabled.png') no-repeat top right;\n"
"}\n"
".paginate_enabled_next:hover {\n"
	"background: url('../images/forward_enabled_hover.png') no-repeat top right;\n"
"}\n"
"table.display {\n"
	"margin: 0 auto;\n"
	"clear: both;\n"
	"width: 100%;\n"
"}\n"

"table.display thead th {\n"
	"padding: 3px 18px 3px 10px;\n"
	"border-bottom: 1px solid black;\n"
	"font-weight: bold;\n"
	"cursor: pointer;\n"
	"* cursor: hand;\n"
"}\n"

"table.display tfoot th {\n"
	"padding: 3px 18px 3px 10px;\n"
	"border-top: 1px solid black;\n"
	"font-weight: bold;\n"
"}\n"

"table.display tr.heading2 td {\n"
	"border-bottom: 1px solid #aaa;\n"
"}\n"

"table.display td {\n"
	"padding: 3px 10px;\n"
"}\n"

"table.display td.center {\n"
	"text-align: center;\n"
"}\n"

".sorting_asc {\n"
	"background: url('../images/sort_asc.png') no-repeat center right;\n"
"}\n"

".sorting_desc {\n"
	"background: url('../images/sort_desc.png') no-repeat center right;\n"
"}\n"

".sorting {\n"
	"background: url('../images/sort_both.png') no-repeat center right;\n"
"}\n"

".sorting_asc_disabled {\n"
	"background: url('./images/sort_asc_disabled.png') no-repeat center right;\n"
"}\n"

".sorting_desc_disabled {\n"
	"background: url('../images/sort_desc_disabled.png') no-repeat center right;\n"
"}\n"
" \n"
"table.display thead th:active,\n"
"table.display thead td:active {\n"
	"outline: none;\n"
"}\n"

"table.display tr.odd.gradeA {\n"
	"background-color: #ddffdd;\n"
"}\n"

"table.display tr.even.gradeA {\n"
	"background-color: #eeffee;\n"
"}\n"

"table.display tr.odd.gradeC {\n"
	"background-color: #ddddff;\n"
"}\n"

"table.display tr.even.gradeC {\n"
	"background-color: #eeeeff;\n"
"}\n"

"table.display tr.odd.gradeX {\n"
	"background-color: #ffdddd;\n"
"}\n"

"table.display tr.even.gradeX {\n"
	"background-color: #ffeeee;\n"
"}\n"

"table.display tr.odd.gradeU {\n"
	"background-color: #ddd;\n"
"}\n"

"table.display tr.even.gradeU {\n"
	"background-color: #eee;\n"
"}\n"


"tr.odd {\n"
	"background-color: #E2E4FF;\n"
"}\n"

"tr.even {\n"
	"background-color: white;\n"
"}\n"

".dataTables_scroll {\n"
	"clear: both;\n"
"}\n"

".dataTables_scrollBody {\n"
	"*margin-top: -1px;\n"
	"-webkit-overflow-scrolling: touch;\n"
"}\n"

".top, .bottom {\n"
	"padding: 15px;\n"
	"background-color: #F5F5F5;\n"
	"border: 1px solid #CCCCCC;\n"
"}\n"

".top .dataTables_info {\n"
	"float: none;\n"
"}\n"

".clear {\n"
	"clear: both;\n"
"}\n"

".dataTables_empty {\n"
	"text-align: center;\n"
"}\n"

"tfoot input {\n"
	"margin: 0.5em 0;\n"
	"width: 100%;\n"
	"color: #444;\n"
"}\n"

"tfoot input.search_init {\n"
	"color: #999;\n"
"}\n"

"td.group {\n"
	"background-color: #d1cfd0;\n"
	"border-bottom: 2px solid #A19B9E;\n"
	"border-top: 2px solid #A19B9E;\n"
"}\n"

"td.details {\n"
	"background-color: #d1cfd0;\n"
	"border: 2px solid #A19B9E;\n"
"}\n"


".example_alt_pagination div.dataTables_info {\n"
	"width: 40%;\n"
"}\n"

".paging_full_numbers {\n"
	"width: 400px;\n"
	"height: 22px;\n"
	"line-height: 22px;\n"
"}\n"

".paging_full_numbers a:active {\n"
	"outline: none\n"
"}\n"

".paging_full_numbers a:hover {\n"
	"text-decoration: none;\n"
"}\n"

".paging_full_numbers a.paginate_button,\n"
" 	.paging_full_numbers a.paginate_active {\n"
	"border: 1px solid #aaa;\n"
	"-webkit-border-radius: 5px;\n"
	"-moz-border-radius: 5px;\n"
	"padding: 2px 5px;\n"
	"margin: 0 3px;\n"
	"cursor: pointer;\n"
	"*cursor: hand;\n"
	"color: #333 !important;\n"
"}\n"

".paging_full_numbers a.paginate_button {\n"
	"background-color: #ddd;\n"
"}\n"

".paging_full_numbers a.paginate_button:hover {\n"
	"background-color: #ccc;\n"
	"text-decoration: none !important;\n"
"}\n"

".paging_full_numbers a.paginate_active {\n"
	"background-color: #99B3FF;\n"
"}\n"

"table.display tr.even.row_selected td {\n"
	"background-color: #B0BED9;\n"
"}\n"

"table.display tr.odd.row_selected td {\n"
	"background-color: #9FAFD1;\n"
"}\n"


"tr.odd td.sorting_1 {\n"
	"background-color: #D3D6FF;\n"
"}\n"

"tr.odd td.sorting_2 {\n"
	"background-color: #DADCFF;\n"
"}\n"

"tr.odd td.sorting_3 {\n"
	"background-color: #E0E2FF;\n"
"}\n"

"tr.even td.sorting_1 {\n"
	"background-color: #EAEBFF;\n"
"}\n"

"tr.even td.sorting_2 {\n"
	"background-color: #F2F3FF;\n"
"}\n"

"tr.even td.sorting_3 {\n"
	"background-color: #F9F9FF;\n"
"}\n"

"tr.odd.gradeA td.sorting_1 {\n"
	"background-color: #c4ffc4;\n"
"}\n"

"tr.odd.gradeA td.sorting_2 {\n"
	"background-color: #d1ffd1;\n"
"}\n"

"tr.odd.gradeA td.sorting_3 {\n"
	"background-color: #d1ffd1;\n"
"}\n"

"tr.even.gradeA td.sorting_1 {\n"
	"background-color: #d5ffd5;\n"
"}\n"

"tr.even.gradeA td.sorting_2 {\n"
	"background-color: #e2ffe2;\n"
"}\n"

"tr.even.gradeA td.sorting_3 {\n"
	"background-color: #e2ffe2;\n"
"}\n"

"tr.odd.gradeC td.sorting_1 {\n"
	"background-color: #c4c4ff;\n"
"}\n"

"tr.odd.gradeC td.sorting_2 {\n"
	"background-color: #d1d1ff;\n"
"}\n"

"tr.odd.gradeC td.sorting_3 {\n"
	"background-color: #d1d1ff;\n"
"}\n"

"tr.even.gradeC td.sorting_1 {\n"
	"background-color: #d5d5ff;\n"
"}\n"

"tr.even.gradeC td.sorting_2 {\n"
	"background-color: #e2e2ff;\n"
"}\n"

"tr.even.gradeC td.sorting_3 {\n"
	"background-color: #e2e2ff;\n"
"}\n"

"tr.odd.gradeX td.sorting_1 {\n"
	"background-color: #ffc4c4;\n"
"}\n"

"tr.odd.gradeX td.sorting_2 {\n"
	"background-color: #ffd1d1;\n"
"}\n"

"tr.odd.gradeX td.sorting_3 {\n"
	"background-color: #ffd1d1;\n"
"}\n"

"tr.even.gradeX td.sorting_1 {\n"
	"background-color: #ffd5d5;\n"
"}\n"

"tr.even.gradeX td.sorting_2 {\n"
	"background-color: #ffe2e2;\n"
"}\n"

"tr.even.gradeX td.sorting_3 {\n"
	"background-color: #ffe2e2;\n"
"}\n"

"tr.odd.gradeU td.sorting_1 {\n"
	"background-color: #c4c4c4;\n"
"}\n"

"tr.odd.gradeU td.sorting_2 {\n"
	"background-color: #d1d1d1;\n"
"}\n"

"tr.odd.gradeU td.sorting_3 {\n"
	"background-color: #d1d1d1;\n"
"}\n"

"tr.even.gradeU td.sorting_1 {\n"
	"background-color: #d5d5d5;\n"
"}\n"

"tr.even.gradeU td.sorting_2 {\n"
	"background-color: #e2e2e2;\n"
"}\n"

"tr.even.gradeU td.sorting_3 {\n"
	"background-color: #e2e2e2;\n"
"}\n"

".ex_highlight #example tbody tr.even:hover, #example tbody tr.even td.highlighted {\n"
	"background-color: #ECFFB3;\n"
"}\n"

".ex_highlight #example tbody tr.odd:hover, #example tbody tr.odd td.highlighted {\n"
	"background-color: #E6FF99;\n"
"}\n"

".ex_highlight_row #example tr.even:hover {\n"
	"background-color: #ECFFB3;\n"
"}\n"

".ex_highlight_row #example tr.even:hover td.sorting_1 {\n"
	"background-color: #DDFF75;\n"
"}\n"

".ex_highlight_row #example tr.even:hover td.sorting_2 {\n"
	"background-color: #E7FF9E;\n"
"}\n"

".ex_highlight_row #example tr.even:hover td.sorting_3 {\n"
	"background-color: #E2FF89;\n"
"}\n"

".ex_highlight_row #example tr.odd:hover {\n"
	"background-color: #E6FF99;\n"
"}\n"

".ex_highlight_row #example tr.odd:hover td.sorting_1 {\n"
	"background-color: #D6FF5C;\n"
"}\n"

".ex_highlight_row #example tr.odd:hover td.sorting_2 {\n"
	"background-color: #E0FF84;\n"
"}\n"

".ex_highlight_row #example tr.odd:hover td.sorting_3 {\n"
	"background-color: #DBFF70;\n"
"}\n"

"table.KeyTable td {\n"
	"border: 3px solid transparent;\n"
"}\n"

"table.KeyTable td.focus {\n"
	"border: 3px solid #3366FF;\n"
"}\n"

"table.display tr.gradeA {\n"
	"background-color: #eeffee;\n"
"}\n"

"table.display tr.gradeC {\n"
	"background-color: #ddddff;\n"
"}\n"

"table.display tr.gradeX {\n"
	"background-color: #ffdddd;\n"
"}\n"

"table.display tr.gradeU {\n"
	"background-color: #ddd;\n"
"}\n"

"div.box {\n"
	"height: 100px;\n"
	"padding: 10px;\n"
	"overflow: auto;\n"
	"border: 1px solid #8080FF;\n"
	"background-color: #E5E5FF;\n"
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
	else if (vals[0] == "datatables.css")
		reply = datatablesCss;
	else 
		throw rts2core::Error ("CSS not found");

	response_length = strlen (reply);
	response = new char[response_length];
	response_type = "text/css";
	memcpy (response, reply, response_length);
}

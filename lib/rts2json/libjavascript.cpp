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

#include "rts2json/libjavascript.h"
#include "block.h"
#include "utilsfunc.h"
#include "error.h"

#include "xmlrpc++/XmlRpcException.h"

using namespace rts2json;

static const char *equScript = 

// format number
// nd - pl - places (before .); nd - number of decimals (after .)
"Number.prototype.format = function(pl,nd) {\n"
  "if (nd == undefined) nd = 2;\n"
  "var ret = this.toFixed(nd);\n"
  "var i = ret.indexOf('.');\n"
  "if (i == -1) i = ret.length;\n"
  "for (;i < pl;i++) ret = '0' + ret;\n"
  "return ret;\n"
"}\n"

"function ln_range_degrees(angle) {\n"
  "var temp;\n"

  "if (angle >= 0.0 && angle < 360.0)\n"
    "return angle;\n"
 
  "temp = Math.floor(angle / 360);\n"
  "temp = temp * 360;\n"

  "return angle - temp;\n"
"}\n"

"function ln_deg_to_rad(deg) { return deg * 1.7453292519943295769e-2; }\n"

"function ln_rad_to_deg(rad) { return rad * 5.7295779513082320877e1; }\n"

/* convert degrees to dms */
"function ln_deg_to_dms (degrees,dms) {\n"
  "var dtemp;\n"

  "if (degrees >= 0) { dms.neg = 0; }\n"
  "else { dms.neg = 1; }\n"

  "degrees = Math.abs(degrees);\n"
  "dms.degrees = Math.floor(degrees);\n"
	
  /* multiply remainder by 60 to get minutes */
  "dtemp = 60*(degrees - dms.degrees);\n"
  "dms.minutes = Math.floor(dtemp);\n"
    
  /* multiply remainder by 60 to get seconds */
  "dms.seconds = 60*(dtemp - dms.minutes);\n"
    
  /* catch any overflows */
  "if (dms.seconds > 59) {\n"
    "dms.seconds = 0;\n"
    "dms.minutes++;\n"
  "}\n"
  "if (dms.minutes > 59) {\n"
    "dms.minutes = 0;\n"
    "dms.degrees ++;\n"
  "}\n"
"}\n"

"function DMS(deg) {\n"
  "this.neg = 0;\n"
  "this.degrees = Infinity;\n"
  "this.minutes = Infinity;\n"
  "this.seconds = Infinity;\n"
  "ln_deg_to_dms(deg,this);\n"
  "this.toStringSigned = function (dispPlus) { return (this.neg ? '-' : (dispPlus ? '+' : '')) + this.degrees.format(2,0) + '&deg;' + this.minutes.format(2,0) + '\\'' + (Math.ceil(this.seconds * 100)/100).format(2) + '\"'; }\n"
  "this.toString = function () { return this.toStringSigned(true); }\n"
  "this.toStringSignedDeg = function (dispPlus) { return (this.neg ? '-' : (dispPlus ? '+' : '')) + this.degrees.format(2,0) + '&deg;'; }\n"
  "this.toStringDeg = function () { return this.toStringSignedDeg(true); }\n"
"}\n"

/* convert degrees to hms */
"function ln_deg_to_hms (degrees,hms) {\n"
  "var dtemp;\n"

  "degrees /= 15.0;\n"

  "if (degrees >= 0) { hms.neg = 0; }\n"
  "else { hms.neg = 1; }\n"

  "degrees = Math.abs(degrees);\n"
  "hms.hours = Math.floor(degrees);\n"
	
  /* multiply remainder by 60 to get minutes */
  "dtemp = 60*(degrees - hms.hours);\n"
  "hms.minutes = Math.floor(dtemp);\n"
    
  /* multiply remainder by 60 to get seconds */
  "hms.seconds = 60*(dtemp - hms.minutes);\n"
    
  /* catch any overflows */
  "if (hms.seconds > 59) {\n"
    "hms.seconds = 0;\n"
    "hms.minutes++;\n"
  "}\n"
  "if (hms.minutes > 59) {\n"
    "hms.minutes = 0;\n"
    "hms.hours ++;\n"
  "}\n"
"}\n"

"function HMS(hour){\n"
  "this.neg = 0;\n"
  "this.hours = Infinity;\n"
  "this.minutes = Infinity;\n"
  "this.seconds = Infinity;\n"
  "ln_deg_to_hms(hour,this);\n"
  "this.toString = function () { return (this.neg ? '-' : '') + this.hours.format(2,0) + ':' + this.minutes.format(2,0) + ':' + (Math.ceil(this.seconds * 1000)/1000).format(2); }\n"
"}\n"

"function ln_get_julian_from_sys() {\n"
  "var ld = new Date();\n"
  /* check for month = January or February */
  "var lY = ld.getUTCFullYear();\n"
  "var lM = ld.getUTCMonth() + 1;\n"
  "var lD = ld.getUTCDate();\n"

  "if (lM < 3 ) {\n"
    "lY--;\n"
    "lM += 12;\n"
  "}\n"

  "a = Math.floor(lY / 100);\n"

  /* check for Julian or Gregorian calendar (starts Oct 4th 1582) */
  "if (lY > 1582 || (lY == 1582 && (lM > 10 || (lM == 10 && lD >= 4)))) {\n"
  /* Gregorian calendar */    
    "b = 2 - a + (a / 4);\n"
  "} else {\n"
  /* Julian calendar */
    "b = 0;\n"
  "}\n"
	
  /* add a fraction of hours, minutes and secs to days*/
  "var days = lD + ld.getUTCHours() / 24 + ld.getUTCMinutes() / 1440 + ld.getUTCSeconds() / 86400 + ld.getUTCMilliseconds() / 86400000;\n"

  /* now get the JD */
  "return Math.floor(365.25 * (lY + 4716)) + Math.floor(30.6001 * (lM + 1)) + days + b - 1524.5;\n"
"}\n"

"function ln_get_mean_sidereal_time (JD) {\n"
  "var sidereal;\n"
  "var T;\n"

  /* calc mean angle */
  "T = (JD - 2451545.0) / 36525.0;\n"

  "sidereal = 280.46061837 + (360.98564736629 * (JD - 2451545.0)) + (0.000387933 * T * T) - (T * T * T / 38710000.0);\n"
  /* add a convenient multiple of 360 degrees */
    
  "sidereal = ln_range_degrees (sidereal);\n"
    
   /* change to hours */
  "return sidereal * 24.0 / 360.0;\n"
"}\n"

"function ln_get_hrz_from_equ_sidereal_time(object,observer,sidereal,position) {\n"
  "var H, ra, latitude, declination, A, Ac, As, h, Z, Zs;\n"

  /* change sidereal_time from hours to radians*/
  "sidereal *= 2.0 * Math.PI / 24.0;\n"

  /* calculate hour angle of object at observers position */
  "ra = ln_deg_to_rad (object.ra);\n"
  "H = sidereal + ln_deg_to_rad (observer.lng) - ra;\n"

  /* hence formula 12.5 and 12.6 give */
  /* convert to radians - hour angle, observers latitude, object declination */
  "latitude = ln_deg_to_rad (observer.lat);\n"
  "declination = ln_deg_to_rad (object.dec);\n"

  /* formula 12.6 *; missuse of A (you have been warned) */
  "A = Math.sin(latitude) * Math.sin(declination) + Math.cos(latitude) * Math.cos(declination) * Math.cos(H);\n"
  "h = Math.asin(A);\n"

  /* convert back to degrees */
  "position.alt = ln_rad_to_deg(h);\n"

  /* zenith distance, Telescope Control 6.8a */
  "Z = Math.acos(A);\n"

  /* is'n there better way to compute that? */
  "Zs = Math.sin (Z);\n"

  /* sane check for zenith distance; don't try to divide by 0 */
  "if (Math.abs(Zs) < 1e-5) { \n"
    "if (object.dec > 0) { position.az = 180; }	else { position.az = 0; }\n"
    "if ((object.dec > 0 && observer.lat > 0) || (object.dec < 0 && observer.lat < 0))\n"
      "{ position.alt = 90; }\n"
    "else { position.alt = -90; }\n"
    "return;\n"
  "}\n"

  /* formulas TC 6.8d Taff 1991, pp. 2 and 13 - vector transformations */
  "As = (Math.cos(declination)* Math.sin (H))/Zs;\n"
  "Ac = (Math.sin(latitude)*Math.cos(declination)*Math.cos(H)-Math.cos(latitude)*Math.sin(declination))/Zs;\n"

  // don't bloom at atan2
  "if (Ac == 0 && As == 0) {\n"
    "if (object.dec > 0) { position.az = 180.0; }\n"
    "else { position.az = 0.0; }\n"
    "return;\n"
  "}\n"
  "A = Math.atan2(As,Ac);\n"

  /* convert back to degrees */
  "position.az = ln_range_degrees(ln_rad_to_deg (A));\n"
"}\n"

"function AltAz () {\n"
  "this.alt = Infinity;\n"
  "this.az = Infinity;\n"
"}\n"

"function LngLat (lng, lat) {\n"
  "this.lng = lng;\n"
  "this.lat = lat;\n"
"}\n"

"function RaDec (ra, dec, observer) {\n"
  "this.ra = ra;\n"
  "this.dec = dec;\n"
  "this.observer = observer;\n"
  "this.altaz = function () {\n"
    "sidereal = ln_get_mean_sidereal_time (ln_get_julian_from_sys());\n"
    "altaz = new AltAz();\n"
    "ln_get_hrz_from_equ_sidereal_time (this, this.observer, sidereal, altaz);\n"
    "return altaz;\n"
  "}\n"
"}\n";

static const char *dateScript = 
"function positionInfo(object) {\n"
"\n"
"  var p_elm = object;\n"
"\n"
"  this.getElementLeft = getElementLeft;\n"
"  function getElementLeft() {\n"
"    var x = 0;\n"
"    var elm;\n"
"    if(typeof(p_elm) == \"object\"){\n"
"      elm = p_elm;\n"
"    } else {\n"
"      elm = document.getElementById(p_elm);\n"
"    }\n"
"    while (elm != null) {\n"
"      if(elm.style.position == 'relative') {\n"
"        break;\n"
"      }\n"
"      else {\n"
"        x += elm.offsetLeft;\n"
"        elm = elm.offsetParent;\n"
"      }\n"
"    }\n"
"    return parseInt(x);\n"
"  }\n"
"\n"
"  this.getElementWidth = getElementWidth;\n"
"  function getElementWidth(){\n"
"    var elm;\n"
"    if(typeof(p_elm) == \"object\"){\n"
"      elm = p_elm;\n"
"    } else {\n"
"      elm = document.getElementById(p_elm);\n"
"    }\n"
"    return parseInt(elm.offsetWidth);\n"
"  }\n"
"\n"
"  this.getElementRight = getElementRight;\n"
"  function getElementRight(){\n"
"    return getElementLeft(p_elm) + getElementWidth(p_elm);\n"
"  }\n"
"\n"
"  this.getElementTop = getElementTop;\n"
"  function getElementTop() {\n"
"    var y = 0;\n"
"    var elm;\n"
"    if(typeof(p_elm) == \"object\"){\n"
"      elm = p_elm;\n"
"    } else {\n"
"      elm = document.getElementById(p_elm);\n"
"    }\n"
"    while (elm != null) {\n"
"      if(elm.style.position == 'relative') {\n"
"        break;\n"
"      }\n"
"      else {\n"
"        y+= elm.offsetTop;\n"
"        elm = elm.offsetParent;\n"
"      }\n"
"    }\n"
"    return parseInt(y);\n"
"  }\n"
"\n"
"  this.getElementHeight = getElementHeight;\n"
"  function getElementHeight(){\n"
"    var elm;\n"
"    if(typeof(p_elm) == \"object\"){\n"
"      elm = p_elm;\n"
"    } else {\n"
"      elm = document.getElementById(p_elm);\n"
"    }\n"
"    return parseInt(elm.offsetHeight);\n"
"  }\n"
"\n"
"  this.getElementBottom = getElementBottom;\n"
"  function getElementBottom(){\n"
"    return getElementTop(p_elm) + getElementHeight(p_elm);\n"
"  }\n"
"}\n"
"\n"
"function CalendarControl() {\n"
"\n"
"  var calendarId = 'CalendarControl';\n"
"  var currentYear = 0;\n"
"  var currentMonth = 0;\n"
"  var currentDay = 0;\n"
"\n"
"  var selectedYear = 0;\n"
"  var selectedMonth = 0;\n"
"  var selectedDay = 0;\n"
"\n"
"  var months = ['January','February','March','April','May','June','July','August','September','October','November','December'];\n"
"  var dateField = null;\n"
"\n"
"  function getProperty(p_property){\n"
"    var p_elm = calendarId;\n"
"    var elm = null;\n"
"\n"
"    if(typeof(p_elm) == \"object\"){\n"
"      elm = p_elm;\n"
"    } else {\n"
"      elm = document.getElementById(p_elm);\n"
"    }\n"
"    if (elm != null){\n"
"      if(elm.style){\n"
"        elm = elm.style;\n"
"        if(elm[p_property]){\n"
"          return elm[p_property];\n"
"        } else {\n"
"          return null;\n"
"        }\n"
"      } else {\n"
"        return null;\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  function setElementProperty(p_property, p_value, p_elmId){\n"
"    var p_elm = p_elmId;\n"
"    var elm = null;\n"
"\n"
"    if(typeof(p_elm) == \"object\"){\n"
"      elm = p_elm;\n"
"    } else {\n"
"      elm = document.getElementById(p_elm);\n"
"    }\n"
"    if((elm != null) && (elm.style != null)){\n"
"      elm = elm.style;\n"
"      elm[ p_property ] = p_value;\n"
"    }\n"
"  }\n"
"\n"
"  function setProperty(p_property, p_value) {\n"
"    setElementProperty(p_property, p_value, calendarId);\n"
"  }\n"
"\n"
"  function getDaysInMonth(year, month) {\n"
"    return [31,((!(year % 4 ) && ( (year % 100 ) || !( year % 400 ) ))?29:28),31,30,31,30,31,31,30,31,30,31][month-1];\n"
"  }\n"
"\n"
"  function getDayOfWeek(year, month, day) {\n"
"    var date = new Date(year,month-1,day)\n"
"    return date.getDay();\n"
"  }\n"
"\n"
"  this.clearDate = clearDate;\n"
"  function clearDate() {\n"
"    dateField.value = '';\n"
"    hide();\n"
"  }\n"
"\n"
"  this.setDate = setDate;\n"
"  function setDate(year, month, day) {\n"
"    if (dateField) {\n"
"      if (month < 10) {month = \"0\" + month;}\n"
"      if (day < 10) {day = \"0\" + day;}\n"
"\n"
"      var dateString = year+'/'+month+'/'+day;\n"
"      dateField.value = dateString;\n"
"      hide();\n"
"    }\n"
"    return;\n"
"  }\n"
"\n"
"  this.changeMonth = changeMonth;\n"
"  function changeMonth(change) {\n"
"    currentMonth += change;\n"
"    currentDay = 0;\n"
"    if(currentMonth > 12) {\n"
"      currentMonth = 1;\n"
"      currentYear++;\n"
"    } else if(currentMonth < 1) {\n"
"      currentMonth = 12;\n"
"      currentYear--;\n"
"    }\n"
"\n"
"    calendar = document.getElementById(calendarId);\n"
"    calendar.innerHTML = calendarDrawTable();\n"
"  }\n"
"\n"
"  this.changeYear = changeYear;\n"
"  function changeYear(change) {\n"
"    currentYear += change;\n"
"    currentDay = 0;\n"
"    calendar = document.getElementById(calendarId);\n"
"    calendar.innerHTML = calendarDrawTable();\n"
"  }\n"
"\n"
"  function getCurrentYear() {\n"
"    var year = new Date().getYear();\n"
"    if(year < 1900) year += 1900;\n"
"    return year;\n"
"  }\n"
"\n"
"  function getCurrentMonth() {\n"
"    return new Date().getMonth() + 1;\n"
"  } \n"
"\n"
"  function getCurrentDay() {\n"
"    return new Date().getDate();\n"
"  }\n"
"\n"
"  function calendarDrawTable() {\n"
"\n"
"    var dayOfMonth = 1;\n"
"    var validDay = 0;\n"
"    var startDayOfWeek = getDayOfWeek(currentYear, currentMonth, dayOfMonth);\n"
"    var daysInMonth = getDaysInMonth(currentYear, currentMonth);\n"
"    var css_class = null; //CSS class for each day\n"
"\n"
"    var table = \"<table cellspacing='0' cellpadding='0' border='0'>\";\n"
"    table = table + \"<tr class='header'>\";\n"
"    table = table + \"  <td colspan='2' class='previous'><a href='javascript:changeCalendarControlMonth(-1);'>&lt;</a> <a href='javascript:changeCalendarControlYear(-1);'>&laquo;</a></td>\";\n"
"    table = table + \"  <td colspan='3' class='title'>\" + months[currentMonth-1] + \"<br>\" + currentYear + \"</td>\";\n"
"    table = table + \"  <td colspan='2' class='next'><a href='javascript:changeCalendarControlYear(1);'>&raquo;</a> <a href='javascript:changeCalendarControlMonth(1);'>&gt;</a></td>\";\n"
"    table = table + \"</tr>\";\n"
"    table = table + \"<tr><th>S</th><th>M</th><th>T</th><th>W</th><th>T</th><th>F</th><th>S</th></tr>\";\n"
"\n"
"    for(var week=0; week < 6; week++) {\n"
"      table = table + \"<tr>\";\n"
"      for(var dayOfWeek=0; dayOfWeek < 7; dayOfWeek++) {\n"
"        if(week == 0 && startDayOfWeek == dayOfWeek) {\n"
"          validDay = 1;\n"
"        } else if (validDay == 1 && dayOfMonth > daysInMonth) {\n"
"          validDay = 0;\n"
"        }\n"
"\n"
"        if(validDay) {\n"
"          if (dayOfMonth == selectedDay && currentYear == selectedYear && currentMonth == selectedMonth) {\n"
"            css_class = 'current';\n"
"          } else if (dayOfWeek == 0 || dayOfWeek == 6) {\n"
"            css_class = 'weekend';\n"
"          } else {\n"
"            css_class = 'weekday';\n"
"          }\n"
"\n"
"          table = table + \"<td><a class='\"+css_class+\"' href=\\\"javascript:setCalendarControlDate(\"+currentYear+\",\"+currentMonth+\",\"+dayOfMonth+\")\\\">\"+dayOfMonth+\"</a></td>\";\n"
"          dayOfMonth++;\n"
"        } else {\n"
"          table = table + \"<td class='empty'>&nbsp;</td>\";\n"
"        }\n"
"      }\n"
"      table = table + \"</tr>\";\n"
"    }\n"
"\n"
"    table = table + \"<tr class='header'><th colspan='7' style='padding: 3px;'><a href='javascript:clearCalendarControl();'>Clear</a> | <a href='javascript:hideCalendarControl();'>Close</a></td></tr>\";\n"
"    table = table + \"</table>\";\n"
"\n"
"    return table;\n"
"  }\n"
"  this.show = show;\n"
"  function show(field) {\n"
"    can_hide = 0;\n"
"    // If the calendar is visible and associated with\n"
"    // this field do not do anything.\n"
"    if (dateField == field) {\n"
"      return;\n"
"    } else {\n"
"      dateField = field;\n"
"    }\n"
"    if(dateField) {\n"
"      try {\n"
"        var dateString = new String(dateField.value);\n"
"        var dateParts = dateString.split('/');\n"
"        \n"
"        selectedYear = parseInt(dateParts[0],10);\n"
"        selectedMonth = parseInt(dateParts[1],10);\n"
"        selectedDay = parseInt(dateParts[2],10);\n"
"      } catch(e) {}\n"
"    }\n"
"    if (!(selectedYear && selectedMonth && selectedDay)) {\n"
"      selectedMonth = getCurrentMonth();\n"
"      selectedDay = getCurrentDay();\n"
"      selectedYear = getCurrentYear();\n"
"    }\n"
"\n"
"    currentMonth = selectedMonth;\n"
"    currentDay = selectedDay;\n"
"    currentYear = selectedYear;\n"
"\n"
"    if(document.getElementById){\n"
"\n"
"      calendar = document.getElementById(calendarId);\n"
"      calendar.innerHTML = calendarDrawTable(currentYear, currentMonth);\n"
"\n"
"      setProperty('display', 'block');\n"
"\n"
"      var fieldPos = new positionInfo(dateField);\n"
"      var calendarPos = new positionInfo(calendarId);\n"
"\n"
"      var x = fieldPos.getElementLeft();\n"
"      var y = fieldPos.getElementBottom();\n"
"\n"
"      setProperty('left', x + \"px\");\n"
"      setProperty('top', y + \"px\");\n"
" \n"
"      if (document.all) {\n"
"        setElementProperty('display', 'block', 'CalendarControlIFrame');\n"
"        setElementProperty('left', x + \"px\", 'CalendarControlIFrame');\n"
"        setElementProperty('top', y + \"px\", 'CalendarControlIFrame');\n"
"        setElementProperty('width', calendarPos.getElementWidth() + \"px\", 'CalendarControlIFrame');\n"
"        setElementProperty('height', calendarPos.getElementHeight() + \"px\", 'CalendarControlIFrame');\n"
"      }\n"
"    }\n"
"  }\n"
"\n"
"  this.hide = hide;\n"
"  function hide() {\n"
"    if(dateField) {\n"
"      setProperty('display', 'none');\n"
"      setElementProperty('display', 'none', 'CalendarControlIFrame');\n"
"      dateField = null;\n"
"    }\n"
"  }\n"
"\n"
"  this.visible = visible;\n"
"  function visible() {\n"
"    return dateField\n"
"  }\n"
"\n"
"  this.can_hide = can_hide;\n"
"  var can_hide = 0;\n"
"}\n"
"\n"
"var calendarControl = new CalendarControl();\n"
"\n"
"function showCalendarControl(textField) {\n"
"  // textField.onblur = hideCalendarControl;\n"
"  calendarControl.show(textField);\n"
"}\n"
"\n"
"function clearCalendarControl() {\n"
"  calendarControl.clearDate();\n"
"}\n"
"\n"
"function hideCalendarControl() {\n"
"  if (calendarControl.visible()) {\n"
"    calendarControl.hide();\n"
"  }\n"
"}\n"
"\n"
"function setCalendarControlDate(year, month, day) {\n"
"  calendarControl.setDate(year, month, day);\n"
"}\n"
"\n"
"function changeCalendarControlYear(change) {\n"
  "calendarControl.changeYear(change);\n"
"}\n"
"\n"
"function changeCalendarControlMonth(change) {\n"
  "calendarControl.changeMonth(change);\n"
"}\n"
"\n"
"document.write(\"<iframe id='CalendarControlIFrame' src='javascript:false;' frameBorder='0' scrolling='no'></iframe>\");\n"
"document.write(\"<div id='CalendarControl'></div>\");\n";

const char *targetEdit = 
// exposure script object
"function Exposure (num, filter, length){\n"
  "this.num = num;\n"
  "this.filter = filter;\n"
  "this.length = length;\n"
  "this.toScript = function(){\n"
    "var ret = 'filter=' + this.filter;\n"
    "if (this.num > 1) return ret + ' for ' + this.num + ' { E ' + this.length + ' } ';\n"
      "else return ret + ' E ' + this.length;\n"
  "};\n"
"}\n"

"function addScript (formID){\n"
  "var se = document.getElementById(formID);\n"
  "var fi = se.filter.selectedIndex;\n"
  "var ex = new Exposure (se.num.value,se.filter.options[fi].value,se.exposure.value);\n"
  "var o = document.createElement('option');\n"
  "o.text = ex.toScript ();\n"
  "o.value = ex.toScript ();\n"

  "try {\n"
    "se.script.add (o,se.script.options[se.script.options.length]);\n"
  "} catch (exep) {\n"
    "se.script.add (o,se.script.options.length);\n"
  "}\n"
"}\n"

"function removeScripts(formID){\n"
  "var se = document.getElementById(formID).script;\n"
  "var i; for (i = se.length - 1; i >= 0; i--){\n"
    "if (se.options[i].selected) se.remove(i);\n"
  "}\n"
"}\n"

"function scriptToText(formID){\n"
  "var se = document.getElementById(formID).script;\n"
  "var ret = '';\n"
  "var i; for (i = 0; i < se.length; i++){\n"
    "if (i > 0) ret += ' ';\n"
    "ret += se.options[i].value;\n"
  "}\n"
  "return ret;\n"
"}\n"

"function createTarget (tn, ra, dec, desc, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET',encodeURI('../api/create_target?tn=' + tn + '&ra=' + ra + '&dec=' + dec + '&desc=' + desc), true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.id);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function updateTarget (id, tn, ra, dec, desc, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET',encodeURI('../api/update_target?id=' + id + '&tn=' + tn + '&ra=' + ra + '&dec=' + dec + '&desc=' + desc), true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.id);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function executeNow (id,func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET',encodeURI('../api/cmd?d=EXEC&c=now '+id), true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.d.current[1]);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function changeScript (id, cam, scr, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET',encodeURI('../api/change_script?id=' + id + '&c=' + cam + '&s=' + scr), true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.id);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function enableTarget (id, ena){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET',encodeURI('../api/update_target?id=' + id + '&enabled=' + ena), true);\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
  "}\n"
  "hr.send(null);\n"
"}\n"
;

const char *rts2Central = 
// exposure script object
"function rts2SetState(cmd, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET',encodeURI('../api/cmd?d=centrald&c=' + cmd), true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(cmd);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function centraldGetState(func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/status?d=centrald', true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "var state = t.state & 0xff;\n"
    "var s = 2;\n"
    "if (state & 0x30) { s = 1; }\n"
    "else if ((state & 0x0f) == 12 || (state & 0x0f) == 11) { s = 0; }\n"
    "this.func(s);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function getState(device,func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET','../api/status?d=' + device, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.state & 0xff);\n"
  "}\n"
 "hr.send(null);\n"
 "}\n"

"function getMessages(from,to,type,func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET','../api/messages?from=' + from+'&to=' + to +'&type=' + type, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.d);\n"
  "}\n"
 "hr.send(null);\n"
 "}\n"

"function centraldGetAll(func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/status?d=centrald', true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"
;

const char *tableScript = 
"function getDuration(dur){\n"
  "if (dur == null) { return '0'; }\n"
  "ret = '';\n"
  "if (dur > 3600){\n"
    "h = Math.floor(dur/3600);\n"
    "ret += h.toString() + ':';\n"
    "dur -= h * 3600;\n"
  "};\n"
  "if (dur > 60){\n"
    "m = Math.floor(dur/60);\n"
    "ret += m.toString() + ':';\n"
    "dur -= 60 * m;\n"
  "}\n"
  "ret += dur.toString();\n"
  "return ret;\n"
"}\n"
"function Table(api_access, element_id, objectName){\n"
  "this.api_access = api_access;\n"
  "this.element_id = element_id;\n"
  "this.objectName = objectName;\n"
  "this.data = [];\n"
  "this.asc = true;\n"
  "this.sortcol = 0;\n"

  "this.fillTD = function(td,row,header,i){\n"
    "switch(header.t){\n"
      "case 'r':\n"
        "td.className = 'deg';\n"
        "td.innerHTML = (new HMS(row[header.c])).toString();\n"
        "break;\n"
      "case 'd':\n"
        "td.className = 'deg';\n"
        "td.innerHTML = (new DMS(row[header.c])).toString();\n"
        "break;\n"
      "case 'a':\n"
	"if (row[header.href] != -1) {\n"
          "var a = document.createElement('a');\n"
          "a.href = header.prefix + row[header.href];\n"
	  "if (header.suffix !== undefined) a.href += header.suffix; else a.href += '/';\n"
	  "a.innerHTML = row[header.c];\n"
          "td.appendChild(a);\n"
	"} else {\n"
	  "td.innerHTML = '---';\n"
	"}\n"    
        "break;\n"
      "case 'b':\n"
        "var o = document.createElement('input');\n"
        "o.type = 'checkbox';\n"
        "o.checked = row[header.c];\n"
        "o.disabled = true;\n"
        "td.appendChild(o);\n"
        "break;\n"
      "case 't':\n"
        "td.className = 'time'\n;"
	"var d = new Date(row[header.c] * 1000);\n"
	"td.innerHTML = d.toLocaleString();\n"
	"break;\n"
      "case 'tD':\n"
        "td.className = 'time'\n;"
	"var d = new Date(row[header.c] * 1000);\n"
	"td.innerHTML = d.toLocaleDateString();\n"
	"break;\n"
      "case 'tT':\n"
        "td.className = 'time'\n;"
	"var d = new Date(row[header.c] * 1000);\n"
	"td.innerHTML = d.toLocaleTimeString();\n"
	"break;\n"
      "case 'altD':\n"
        "td.className = 'deg';\n"
        "td.innerHTML = new DMS(row[header.c]).toStringSignedDeg();\n"
        "break;\n"
      "case 'azD':\n"
        "td.className = 'deg';\n"
        "td.innerHTML = new DMS(row[header.c]).toStringDeg();\n"
        "break;\n"
      "case 'Alt':\n"
        "td.className = 'deg';\n"
        "td.id = 'alt_' + i;\n"
        "break;"
      "case 'Az':\n"
        "td.className = 'deg';\n"
        "td.id = 'az_' + i;\n"
        "break;"
      "case 'sel':\n"
        "var sel = document.createElement('input');\n"
	"sel.type = 'checkbox';\n"
	"sel.checked = false;\n"
	"sel.name = header.spn;\n"
	"sel.value = row[header.c];\n"
	"sel.disabled = false;\n"
        "td.appendChild(sel);\n"
	"break;\n"
      "case 'dur':\n"
        "td.innerHTML = getDuration(row[header.c]) + 's';\n"
        "break;\n"	
      "default:\n"
        "td.innerHTML = row[header.c];\n"
    "}\n"
  "}\n"

  "this.fillTR = function(tr,row,i,h){\n"
    "for (var j in h){\n"
      "var td = tr.insertCell(-1);\n"
      "this.fillTD(td,row,h[j],i);\n"
    "}\n"
  "}\n"
  "this.fillTable = function(tab){\n"
    "for (var i in this.data) {\n"
      "tr = tab.insertRow(-1);\n"
      "this.fillTR (tr,this.data[i],i,t.h);\n"
    "}\n"
  "}\n"
  "this.resortTable = function(col,type){\n"
    "if (this.sortcol == col) this.asc = !this.asc;\n"
      "else this.asc = true;\n"
    "this.sortcol = col;\n"
    "var e = document.getElementById(this.element_id + '_tab');\n"
    "var l = e.rows.length;\n"
    "for (var i = 1; i < l; i++){\n"
      "e.deleteRow(1);}\n"
    "table = this;\n"
    "switch (type){\n"
      "case 't':\n"
      "case 'a':\n"
      "case 's':\n"
        "this.data.sort(function (a,b){\n"
          "var ret = (a[table.sortcol] < b[table.sortcol]) - (b[table.sortcol] < a[table.sortcol]);\n"
          "if (table.asc) return ret;\n"
            "else return -1 * ret;\n"
        "});\n"
        "break;\n"
      "default:\n"
        "this.data.sort(function (a,b){\n"
          "if (table.asc) return a[table.sortcol] - b[table.sortcol];\n"
             "else return b[table.sortcol] - a[table.sortcol];\n"
        "});\n"
    "}\n"
    "this.fillTable(e);\n"
    "this.updateTable(false);\n"
  "}\n"
  "this.refresh = function(){\n"
    "var hr = new XMLHttpRequest();\n"
    "hr.open('GET',this.api_access,true);\n"
    "var table = this;\n"
    "hr.onreadystatechange = function() {\n"
      "if (this.readyState == 4 && this.status == 200) {\n"
        "t = JSON.parse(this.responseText);\n"
        "table.data = t.d;\n"
        "var tab = document.createElement('table');\n"
        "tab.id = table.element_id + '_tab';\n"
        // write header
        "var th = tab.createTHead();\n"
        "var tr = th.insertRow(0);\n"
        "for (var h in t.h) {\n"
          "var td = tr.insertCell(-1);\n"
          "td.className = 'TableHead';\n"
          "var tdiv = document.createElement('div');\n"
          "tdiv.innerHTML = t.h[h].n;\n"
          "td.appendChild(tdiv);\n"
          "var img_up = new Image (24,34);\n"
          "img_up.src = pagePrefix + '/images/up.png';\n"
          "td.appendChild(img_up);\n"
          "td.setAttribute('onclick', table.objectName + '.resortTable(' + t.h[h].c + ',\"' + t.h[h].t + '\"); return true;');\n"
        "}\n"
        "table.fillTable(tab);\n"
        "var e = document.getElementById(table.element_id);\n"
        "e.innerHTML = '';\n"
        "e.appendChild(tab);\n"
        "table.updateTable(true);\n"
      "}\n"
    "}\n"
    "hr.send(null);\n"
  "}\n"
  "this.updateTable = function(settimer){}\n"
"}\n";

const char *jquery = 
"/*! jQuery v1.9.1 | (c) 2005, 2012 jQuery Foundation, Inc. | jquery.org/license\n"
"//@ sourceMappingURL=jquery.min.map\n"
"*/(function(e,t){var n,r,i=typeof t,o=e.document,a=e.location,s=e.jQuery,u=e.$,l={},c=[],p=\"1.9.1\",f=c.concat,d=c.push,h=c.slice,g=c.indexOf,m=l.toString,y=l.hasOwnProperty,v=p.trim,b=function(e,t){return new b.fn.init(e,t,r)},x=/[+-]?(?:\\d*\\.|)\\d+(?:[eE][+-]?\\d+|)/.source,w=/\\S+/g,T=/^[\\s\\uFEFF\\xA0]+|[\\s\\uFEFF\\xA0]+$/g,N=/^(?:(<[\\w\\W]+>)[^>]*|#([\\w-]*))$/,C=/^<(\\w+)\\s*\\/?>(?:<\\/\\1>|)$/,k=/^[\\],:{}\\s]*$/,E=/(?:^|:|,)(?:\\s*\\[)+/g,S=/\\\\(?:[\"\\\\\\/bfnrt]|u[\\da-fA-F]{4})/g,A=/\"[^\"\\\\\\r\\n]*\"|true|false|null|-?(?:\\d+\\.|)\\d+(?:[eE][+-]?\\d+|)/g,j=/^-ms-/,D=/-([\\da-z])/gi,L=function(e,t){return t.toUpperCase()},H=function(e){(o.addEventListener||\"load\"===e.type||\"complete\"===o.readyState)&&(q(),b.ready())},q=function(){o.addEventListener?(o.removeEventListener(\"DOMContentLoaded\",H,!1),e.removeEventListener(\"load\",H,!1)):(o.detachEvent(\"onreadystatechange\",H),e.detachEvent(\"onload\",H))};b.fn=b.prototype={jquery:p,constructor:b,init:function(e,n,r){var i,a;if(!e)return this;if(\"string\"==typeof e){if(i=\"<\"===e.charAt(0)&&\">\"===e.charAt(e.length-1)&&e.length>=3?[null,e,null]:N.exec(e),!i||!i[1]&&n)return!n||n.jquery?(n||r).find(e):this.constructor(n).find(e);if(i[1]){if(n=n instanceof b?n[0]:n,b.merge(this,b.parseHTML(i[1],n&&n.nodeType?n.ownerDocument||n:o,!0)),C.test(i[1])&&b.isPlainObject(n))for(i in n)b.isFunction(this[i])?this[i](n[i]):this.attr(i,n[i]);return this}if(a=o.getElementById(i[2]),a&&a.parentNode){if(a.id!==i[2])return r.find(e);this.length=1,this[0]=a}return this.context=o,this.selector=e,this}return e.nodeType?(this.context=this[0]=e,this.length=1,this):b.isFunction(e)?r.ready(e):(e.selector!==t&&(this.selector=e.selector,this.context=e.context),b.makeArray(e,this))},selector:\"\",length:0,size:function(){return this.length},toArray:function(){return h.call(this)},get:function(e){return null==e?this.toArray():0>e?this[this.length+e]:this[e]},pushStack:function(e){var t=b.merge(this.constructor(),e);return t.prevObject=this,t.context=this.context,t},each:function(e,t){return b.each(this,e,t)},ready:function(e){return b.ready.promise().done(e),this},slice:function(){return this.pushStack(h.apply(this,arguments))},first:function(){return this.eq(0)},last:function(){return this.eq(-1)},eq:function(e){var t=this.length,n=+e+(0>e?t:0);return this.pushStack(n>=0&&t>n?[this[n]]:[])},map:function(e){return this.pushStack(b.map(this,function(t,n){return e.call(t,n,t)}))},end:function(){return this.prevObject||this.constructor(null)},push:d,sort:[].sort,splice:[].splice},b.fn.init.prototype=b.fn,b.extend=b.fn.extend=function(){var e,n,r,i,o,a,s=arguments[0]||{},u=1,l=arguments.length,c=!1;for(\"boolean\"==typeof s&&(c=s,s=arguments[1]||{},u=2),\"object\"==typeof s||b.isFunction(s)||(s={}),l===u&&(s=this,--u);l>u;u++)if(null!=(o=arguments[u]))for(i in o)e=s[i],r=o[i],s!==r&&(c&&r&&(b.isPlainObject(r)||(n=b.isArray(r)))?(n?(n=!1,a=e&&b.isArray(e)?e:[]):a=e&&b.isPlainObject(e)?e:{},s[i]=b.extend(c,a,r)):r!==t&&(s[i]=r));return s},b.extend({noConflict:function(t){return e.$===b&&(e.$=u),t&&e.jQuery===b&&(e.jQuery=s),b},isReady:!1,readyWait:1,holdReady:function(e){e?b.readyWait++:b.ready(!0)},ready:function(e){if(e===!0?!--b.readyWait:!b.isReady){if(!o.body)return setTimeout(b.ready);b.isReady=!0,e!==!0&&--b.readyWait>0||(n.resolveWith(o,[b]),b.fn.trigger&&b(o).trigger(\"ready\").off(\"ready\"))}},isFunction:function(e){return\"function\"===b.type(e)},isArray:Array.isArray||function(e){return\"array\"===b.type(e)},isWindow:function(e){return null!=e&&e==e.window},isNumeric:function(e){return!isNaN(parseFloat(e))&&isFinite(e)},type:function(e){return null==e?e+\"\":\"object\"==typeof e||\"function\"==typeof e?l[m.call(e)]||\"object\":typeof e},isPlainObject:function(e){if(!e||\"object\"!==b.type(e)||e.nodeType||b.isWindow(e))return!1;try{if(e.constructor&&!y.call(e,\"constructor\")&&!y.call(e.constructor.prototype,\"isPrototypeOf\"))return!1}catch(n){return!1}var r;for(r in e);return r===t||y.call(e,r)},isEmptyObject:function(e){var t;for(t in e)return!1;return!0},error:function(e){throw Error(e)},parseHTML:function(e,t,n){if(!e||\"string\"!=typeof e)return null;\"boolean\"==typeof t&&(n=t,t=!1),t=t||o;var r=C.exec(e),i=!n&&[];return r?[t.createElement(r[1])]:(r=b.buildFragment([e],t,i),i&&b(i).remove(),b.merge([],r.childNodes))},parseJSON:function(n){return e.JSON&&e.JSON.parse?e.JSON.parse(n):null===n?n:\"string\"==typeof n&&(n=b.trim(n),n&&k.test(n.replace(S,\"@\").replace(A,\"]\").replace(E,\"\")))?Function(\"return \"+n)():(b.error(\"Invalid JSON: \"+n),t)},parseXML:function(n){var r,i;if(!n||\"string\"!=typeof n)return null;try{e.DOMParser?(i=new DOMParser,r=i.parseFromString(n,\"text/xml\")):(r=new ActiveXObject(\"Microsoft.XMLDOM\"),r.async=\"false\",r.loadXML(n))}catch(o){r=t}return r&&r.documentElement&&!r.getElementsByTagName(\"parsererror\").length||b.error(\"Invalid XML: \"+n),r},noop:function(){},globalEval:function(t){t&&b.trim(t)&&(e.execScript||function(t){e.eval.call(e,t)})(t)},camelCase:function(e){return e.replace(j,\"ms-\").replace(D,L)},nodeName:function(e,t){return e.nodeName&&e.nodeName.toLowerCase()===t.toLowerCase()},each:function(e,t,n){var r,i=0,o=e.length,a=M(e);if(n){if(a){for(;o>i;i++)if(r=t.apply(e[i],n),r===!1)break}else for(i in e)if(r=t.apply(e[i],n),r===!1)break}else if(a){for(;o>i;i++)if(r=t.call(e[i],i,e[i]),r===!1)break}else for(i in e)if(r=t.call(e[i],i,e[i]),r===!1)break;return e},trim:v&&!v.call(\"\\ufeff\\u00a0\")?function(e){return null==e?\"\":v.call(e)}:function(e){return null==e?\"\":(e+\"\").replace(T,\"\")},makeArray:function(e,t){var n=t||[];return null!=e&&(M(Object(e))?b.merge(n,\"string\"==typeof e?[e]:e):d.call(n,e)),n},inArray:function(e,t,n){var r;if(t){if(g)return g.call(t,e,n);for(r=t.length,n=n?0>n?Math.max(0,r+n):n:0;r>n;n++)if(n in t&&t[n]===e)return n}return-1},merge:function(e,n){var r=n.length,i=e.length,o=0;if(\"number\"==typeof r)for(;r>o;o++)e[i++]=n[o];else while(n[o]!==t)e[i++]=n[o++];return e.length=i,e},grep:function(e,t,n){var r,i=[],o=0,a=e.length;for(n=!!n;a>o;o++)r=!!t(e[o],o),n!==r&&i.push(e[o]);return i},map:function(e,t,n){var r,i=0,o=e.length,a=M(e),s=[];if(a)for(;o>i;i++)r=t(e[i],i,n),null!=r&&(s[s.length]=r);else for(i in e)r=t(e[i],i,n),null!=r&&(s[s.length]=r);return f.apply([],s)},guid:1,proxy:function(e,n){var r,i,o;return\"string\"==typeof n&&(o=e[n],n=e,e=o),b.isFunction(e)?(r=h.call(arguments,2),i=function(){return e.apply(n||this,r.concat(h.call(arguments)))},i.guid=e.guid=e.guid||b.guid++,i):t},access:function(e,n,r,i,o,a,s){var u=0,l=e.length,c=null==r;if(\"object\"===b.type(r)){o=!0;for(u in r)b.access(e,n,u,r[u],!0,a,s)}else if(i!==t&&(o=!0,b.isFunction(i)||(s=!0),c&&(s?(n.call(e,i),n=null):(c=n,n=function(e,t,n){return c.call(b(e),n)})),n))for(;l>u;u++)n(e[u],r,s?i:i.call(e[u],u,n(e[u],r)));return o?e:c?n.call(e):l?n(e[0],r):a},now:function(){return(new Date).getTime()}}),b.ready.promise=function(t){if(!n)if(n=b.Deferred(),\"complete\"===o.readyState)setTimeout(b.ready);else if(o.addEventListener)o.addEventListener(\"DOMContentLoaded\",H,!1),e.addEventListener(\"load\",H,!1);else{o.attachEvent(\"onreadystatechange\",H),e.attachEvent(\"onload\",H);var r=!1;try{r=null==e.frameElement&&o.documentElement}catch(i){}r&&r.doScroll&&function a(){if(!b.isReady){try{r.doScroll(\"left\")}catch(e){return setTimeout(a,50)}q(),b.ready()}}()}return n.promise(t)},b.each(\"Boolean Number String Function Array Date RegExp Object Error\".split(\" \"),function(e,t){l[\"[object \"+t+\"]\"]=t.toLowerCase()});function M(e){var t=e.length,n=b.type(e);return b.isWindow(e)?!1:1===e.nodeType&&t?!0:\"array\"===n||\"function\"!==n&&(0===t||\"number\"==typeof t&&t>0&&t-1 in e)}r=b(o);var _={};function F(e){var t=_[e]={};return b.each(e.match(w)||[],function(e,n){t[n]=!0}),t}b.Callbacks=function(e){e=\"string\"==typeof e?_[e]||F(e):b.extend({},e);var n,r,i,o,a,s,u=[],l=!e.once&&[],c=function(t){for(r=e.memory&&t,i=!0,a=s||0,s=0,o=u.length,n=!0;u&&o>a;a++)if(u[a].apply(t[0],t[1])===!1&&e.stopOnFalse){r=!1;break}n=!1,u&&(l?l.length&&c(l.shift()):r?u=[]:p.disable())},p={add:function(){if(u){var t=u.length;(function i(t){b.each(t,function(t,n){var r=b.type(n);\"function\"===r?e.unique&&p.has(n)||u.push(n):n&&n.length&&\"string\"!==r&&i(n)})})(arguments),n?o=u.length:r&&(s=t,c(r))}return this},remove:function(){return u&&b.each(arguments,function(e,t){var r;while((r=b.inArray(t,u,r))>-1)u.splice(r,1),n&&(o>=r&&o--,a>=r&&a--)}),this},has:function(e){return e?b.inArray(e,u)>-1:!(!u||!u.length)},empty:function(){return u=[],this},disable:function(){return u=l=r=t,this},disabled:function(){return!u},lock:function(){return l=t,r||p.disable(),this},locked:function(){return!l},fireWith:function(e,t){return t=t||[],t=[e,t.slice?t.slice():t],!u||i&&!l||(n?l.push(t):c(t)),this},fire:function(){return p.fireWith(this,arguments),this},fired:function(){return!!i}};return p},b.extend({Deferred:function(e){var t=[[\"resolve\",\"done\",b.Callbacks(\"once memory\"),\"resolved\"],[\"reject\",\"fail\",b.Callbacks(\"once memory\"),\"rejected\"],[\"notify\",\"progress\",b.Callbacks(\"memory\")]],n=\"pending\",r={state:function(){return n},always:function(){return i.done(arguments).fail(arguments),this},then:function(){var e=arguments;return b.Deferred(function(n){b.each(t,function(t,o){var a=o[0],s=b.isFunction(e[t])&&e[t];i[o[1]](function(){var e=s&&s.apply(this,arguments);e&&b.isFunction(e.promise)?e.promise().done(n.resolve).fail(n.reject).progress(n.notify):n[a+\"With\"](this===r?n.promise():this,s?[e]:arguments)})}),e=null}).promise()},promise:function(e){return null!=e?b.extend(e,r):r}},i={};return r.pipe=r.then,b.each(t,function(e,o){var a=o[2],s=o[3];r[o[1]]=a.add,s&&a.add(function(){n=s},t[1^e][2].disable,t[2][2].lock),i[o[0]]=function(){return i[o[0]+\"With\"](this===i?r:this,arguments),this},i[o[0]+\"With\"]=a.fireWith}),r.promise(i),e&&e.call(i,i),i},when:function(e){var t=0,n=h.call(arguments),r=n.length,i=1!==r||e&&b.isFunction(e.promise)?r:0,o=1===i?e:b.Deferred(),a=function(e,t,n){return function(r){t[e]=this,n[e]=arguments.length>1?h.call(arguments):r,n===s?o.notifyWith(t,n):--i||o.resolveWith(t,n)}},s,u,l;if(r>1)for(s=Array(r),u=Array(r),l=Array(r);r>t;t++)n[t]&&b.isFunction(n[t].promise)?n[t].promise().done(a(t,l,n)).fail(o.reject).progress(a(t,u,s)):--i;return i||o.resolveWith(l,n),o.promise()}}),b.support=function(){var t,n,r,a,s,u,l,c,p,f,d=o.createElement(\"div\");if(d.setAttribute(\"className\",\"t\"),d.innerHTML=\"  <link/><table></table><a href='/a'>a</a><input type='checkbox'/>\",n=d.getElementsByTagName(\"*\"),r=d.getElementsByTagName(\"a\")[0],!n||!r||!n.length)return{};s=o.createElement(\"select\"),l=s.appendChild(o.createElement(\"option\")),a=d.getElementsByTagName(\"input\")[0],r.style.cssText=\"top:1px;float:left;opacity:.5\",t={getSetAttribute:\"t\"!==d.className,leadingWhitespace:3===d.firstChild.nodeType,tbody:!d.getElementsByTagName(\"tbody\").length,htmlSerialize:!!d.getElementsByTagName(\"link\").length,style:/top/.test(r.getAttribute(\"style\")),hrefNormalized:\"/a\"===r.getAttribute(\"href\"),opacity:/^0.5/.test(r.style.opacity),cssFloat:!!r.style.cssFloat,checkOn:!!a.value,optSelected:l.selected,enctype:!!o.createElement(\"form\").enctype,html5Clone:\"<:nav></:nav>\"!==o.createElement(\"nav\").cloneNode(!0).outerHTML,boxModel:\"CSS1Compat\"===o.compatMode,deleteExpando:!0,noCloneEvent:!0,inlineBlockNeedsLayout:!1,shrinkWrapBlocks:!1,reliableMarginRight:!0,boxSizingReliable:!0,pixelPosition:!1},a.checked=!0,t.noCloneChecked=a.cloneNode(!0).checked,s.disabled=!0,t.optDisabled=!l.disabled;try{delete d.test}catch(h){t.deleteExpando=!1}a=o.createElement(\"input\"),a.setAttribute(\"value\",\"\"),t.input=\"\"===a.getAttribute(\"value\"),a.value=\"t\",a.setAttribute(\"type\",\"radio\"),t.radioValue=\"t\"===a.value,a.setAttribute(\"checked\",\"t\"),a.setAttribute(\"name\",\"t\"),u=o.createDocumentFragment(),u.appendChild(a),t.appendChecked=a.checked,t.checkClone=u.cloneNode(!0).cloneNode(!0).lastChild.checked,d.attachEvent&&(d.attachEvent(\"onclick\",function(){t.noCloneEvent=!1}),d.cloneNode(!0).click());for(f in{submit:!0,change:!0,focusin:!0})d.setAttribute(c=\"on\"+f,\"t\"),t[f+\"Bubbles\"]=c in e||d.attributes[c].expando===!1;return d.style.backgroundClip=\"content-box\",d.cloneNode(!0).style.backgroundClip=\"\",t.clearCloneStyle=\"content-box\"===d.style.backgroundClip,b(function(){var n,r,a,s=\"padding:0;margin:0;border:0;display:block;box-sizing:content-box;-moz-box-sizing:content-box;-webkit-box-sizing:content-box;\",u=o.getElementsByTagName(\"body\")[0];u&&(n=o.createElement(\"div\"),n.style.cssText=\"border:0;width:0;height:0;position:absolute;top:0;left:-9999px;margin-top:1px\",u.appendChild(n).appendChild(d),d.innerHTML=\"<table><tr><td></td><td>t</td></tr></table>\",a=d.getElementsByTagName(\"td\"),a[0].style.cssText=\"padding:0;margin:0;border:0;display:none\",p=0===a[0].offsetHeight,a[0].style.display=\"\",a[1].style.display=\"none\",t.reliableHiddenOffsets=p&&0===a[0].offsetHeight,d.innerHTML=\"\",d.style.cssText=\"box-sizing:border-box;-moz-box-sizing:border-box;-webkit-box-sizing:border-box;padding:1px;border:1px;display:block;width:4px;margin-top:1%;position:absolute;top:1%;\",t.boxSizing=4===d.offsetWidth,t.doesNotIncludeMarginInBodyOffset=1!==u.offsetTop,e.getComputedStyle&&(t.pixelPosition=\"1%\"!==(e.getComputedStyle(d,null)||{}).top,t.boxSizingReliable=\"4px\"===(e.getComputedStyle(d,null)||{width:\"4px\"}).width,r=d.appendChild(o.createElement(\"div\")),r.style.cssText=d.style.cssText=s,r.style.marginRight=r.style.width=\"0\",d.style.width=\"1px\",t.reliableMarginRight=!parseFloat((e.getComputedStyle(r,null)||{}).marginRight)),typeof d.style.zoom!==i&&(d.innerHTML=\"\",d.style.cssText=s+\"width:1px;padding:1px;display:inline;zoom:1\",t.inlineBlockNeedsLayout=3===d.offsetWidth,d.style.display=\"block\",d.innerHTML=\"<div></div>\",d.firstChild.style.width=\"5px\",t.shrinkWrapBlocks=3!==d.offsetWidth,t.inlineBlockNeedsLayout&&(u.style.zoom=1)),u.removeChild(n),n=d=a=r=null)}),n=s=u=l=r=a=null,t}();var O=/(?:\\{[\\s\\S]*\\}|\\[[\\s\\S]*\\])$/,B=/([A-Z])/g;function P(e,n,r,i){if(b.acceptData(e)){var o,a,s=b.expando,u=\"string\"==typeof n,l=e.nodeType,p=l?b.cache:e,f=l?e[s]:e[s]&&s;if(f&&p[f]&&(i||p[f].data)||!u||r!==t)return f||(l?e[s]=f=c.pop()||b.guid++:f=s),p[f]||(p[f]={},l||(p[f].toJSON=b.noop)),(\"object\"==typeof n||\"function\"==typeof n)&&(i?p[f]=b.extend(p[f],n):p[f].data=b.extend(p[f].data,n)),o=p[f],i||(o.data||(o.data={}),o=o.data),r!==t&&(o[b.camelCase(n)]=r),u?(a=o[n],null==a&&(a=o[b.camelCase(n)])):a=o,a}}function R(e,t,n){if(b.acceptData(e)){var r,i,o,a=e.nodeType,s=a?b.cache:e,u=a?e[b.expando]:b.expando;if(s[u]){if(t&&(o=n?s[u]:s[u].data)){b.isArray(t)?t=t.concat(b.map(t,b.camelCase)):t in o?t=[t]:(t=b.camelCase(t),t=t in o?[t]:t.split(\" \"));for(r=0,i=t.length;i>r;r++)delete o[t[r]];if(!(n?$:b.isEmptyObject)(o))return}(n||(delete s[u].data,$(s[u])))&&(a?b.cleanData([e],!0):b.support.deleteExpando||s!=s.window?delete s[u]:s[u]=null)}}}b.extend({cache:{},expando:\"jQuery\"+(p+Math.random()).replace(/\\D/g,\"\"),noData:{embed:!0,object:\"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000\",applet:!0},hasData:function(e){return e=e.nodeType?b.cache[e[b.expando]]:e[b.expando],!!e&&!$(e)},data:function(e,t,n){return P(e,t,n)},removeData:function(e,t){return R(e,t)},_data:function(e,t,n){return P(e,t,n,!0)},_removeData:function(e,t){return R(e,t,!0)},acceptData:function(e){if(e.nodeType&&1!==e.nodeType&&9!==e.nodeType)return!1;var t=e.nodeName&&b.noData[e.nodeName.toLowerCase()];return!t||t!==!0&&e.getAttribute(\"classid\")===t}}),b.fn.extend({data:function(e,n){var r,i,o=this[0],a=0,s=null;if(e===t){if(this.length&&(s=b.data(o),1===o.nodeType&&!b._data(o,\"parsedAttrs\"))){for(r=o.attributes;r.length>a;a++)i=r[a].name,i.indexOf(\"data-\")||(i=b.camelCase(i.slice(5)),W(o,i,s[i]));b._data(o,\"parsedAttrs\",!0)}return s}return\"object\"==typeof e?this.each(function(){b.data(this,e)}):b.access(this,function(n){return n===t?o?W(o,e,b.data(o,e)):null:(this.each(function(){b.data(this,e,n)}),t)},null,n,arguments.length>1,null,!0)},removeData:function(e){return this.each(function(){b.removeData(this,e)})}});function W(e,n,r){if(r===t&&1===e.nodeType){var i=\"data-\"+n.replace(B,\"-$1\").toLowerCase();if(r=e.getAttribute(i),\"string\"==typeof r){try{r=\"true\"===r?!0:\"false\"===r?!1:\"null\"===r?null:+r+\"\"===r?+r:O.test(r)?b.parseJSON(r):r}catch(o){}b.data(e,n,r)}else r=t}return r}function $(e){var t;for(t in e)if((\"data\"!==t||!b.isEmptyObject(e[t]))&&\"toJSON\"!==t)return!1;return!0}b.extend({queue:function(e,n,r){var i;return e?(n=(n||\"fx\")+\"queue\",i=b._data(e,n),r&&(!i||b.isArray(r)?i=b._data(e,n,b.makeArray(r)):i.push(r)),i||[]):t},dequeue:function(e,t){t=t||\"fx\";var n=b.queue(e,t),r=n.length,i=n.shift(),o=b._queueHooks(e,t),a=function(){b.dequeue(e,t)};\"inprogress\"===i&&(i=n.shift(),r--),o.cur=i,i&&(\"fx\"===t&&n.unshift(\"inprogress\"),delete o.stop,i.call(e,a,o)),!r&&o&&o.empty.fire()},_queueHooks:function(e,t){var n=t+\"queueHooks\";return b._data(e,n)||b._data(e,n,{empty:b.Callbacks(\"once memory\").add(function(){b._removeData(e,t+\"queue\"),b._removeData(e,n)})})}}),b.fn.extend({queue:function(e,n){var r=2;return\"string\"!=typeof e&&(n=e,e=\"fx\",r--),r>arguments.length?b.queue(this[0],e):n===t?this:this.each(function(){var t=b.queue(this,e,n);b._queueHooks(this,e),\"fx\"===e&&\"inprogress\"!==t[0]&&b.dequeue(this,e)})},dequeue:function(e){return this.each(function(){b.dequeue(this,e)})},delay:function(e,t){return e=b.fx?b.fx.speeds[e]||e:e,t=t||\"fx\",this.queue(t,function(t,n){var r=setTimeout(t,e);n.stop=function(){clearTimeout(r)}})},clearQueue:function(e){return this.queue(e||\"fx\",[])},promise:function(e,n){var r,i=1,o=b.Deferred(),a=this,s=this.length,u=function(){--i||o.resolveWith(a,[a])};\"string\"!=typeof e&&(n=e,e=t),e=e||\"fx\";while(s--)r=b._data(a[s],e+\"queueHooks\"),r&&r.empty&&(i++,r.empty.add(u));return u(),o.promise(n)}});var I,z,X=/[\\t\\r\\n]/g,U=/\\r/g,V=/^(?:input|select|textarea|button|object)$/i,Y=/^(?:a|area)$/i,J=/^(?:checked|selected|autofocus|autoplay|async|controls|defer|disabled|hidden|loop|multiple|open|readonly|required|scoped)$/i,G=/^(?:checked|selected)$/i,Q=b.support.getSetAttribute,K=b.support.input;b.fn.extend({attr:function(e,t){return b.access(this,b.attr,e,t,arguments.length>1)},removeAttr:function(e){return this.each(function(){b.removeAttr(this,e)})},prop:function(e,t){return b.access(this,b.prop,e,t,arguments.length>1)},removeProp:function(e){return e=b.propFix[e]||e,this.each(function(){try{this[e]=t,delete this[e]}catch(n){}})},addClass:function(e){var t,n,r,i,o,a=0,s=this.length,u=\"string\"==typeof e&&e;if(b.isFunction(e))return this.each(function(t){b(this).addClass(e.call(this,t,this.className))});if(u)for(t=(e||\"\").match(w)||[];s>a;a++)if(n=this[a],r=1===n.nodeType&&(n.className?(\" \"+n.className+\" \").replace(X,\" \"):\" \")){o=0;while(i=t[o++])0>r.indexOf(\" \"+i+\" \")&&(r+=i+\" \");n.className=b.trim(r)}return this},removeClass:function(e){var t,n,r,i,o,a=0,s=this.length,u=0===arguments.length||\"string\"==typeof e&&e;if(b.isFunction(e))return this.each(function(t){b(this).removeClass(e.call(this,t,this.className))});if(u)for(t=(e||\"\").match(w)||[];s>a;a++)if(n=this[a],r=1===n.nodeType&&(n.className?(\" \"+n.className+\" \").replace(X,\" \"):\"\")){o=0;while(i=t[o++])while(r.indexOf(\" \"+i+\" \")>=0)r=r.replace(\" \"+i+\" \",\" \");n.className=e?b.trim(r):\"\"}return this},toggleClass:function(e,t){var n=typeof e,r=\"boolean\"==typeof t;return b.isFunction(e)?this.each(function(n){b(this).toggleClass(e.call(this,n,this.className,t),t)}):this.each(function(){if(\"string\"===n){var o,a=0,s=b(this),u=t,l=e.match(w)||[];while(o=l[a++])u=r?u:!s.hasClass(o),s[u?\"addClass\":\"removeClass\"](o)}else(n===i||\"boolean\"===n)&&(this.className&&b._data(this,\"__className__\",this.className),this.className=this.className||e===!1?\"\":b._data(this,\"__className__\")||\"\")})},hasClass:function(e){var t=\" \"+e+\" \",n=0,r=this.length;for(;r>n;n++)if(1===this[n].nodeType&&(\" \"+this[n].className+\" \").replace(X,\" \").indexOf(t)>=0)return!0;return!1},val:function(e){var n,r,i,o=this[0];{if(arguments.length)return i=b.isFunction(e),this.each(function(n){var o,a=b(this);1===this.nodeType&&(o=i?e.call(this,n,a.val()):e,null==o?o=\"\":\"number\"==typeof o?o+=\"\":b.isArray(o)&&(o=b.map(o,function(e){return null==e?\"\":e+\"\"})),r=b.valHooks[this.type]||b.valHooks[this.nodeName.toLowerCase()],r&&\"set\"in r&&r.set(this,o,\"value\")!==t||(this.value=o))});if(o)return r=b.valHooks[o.type]||b.valHooks[o.nodeName.toLowerCase()],r&&\"get\"in r&&(n=r.get(o,\"value\"))!==t?n:(n=o.value,\"string\"==typeof n?n.replace(U,\"\"):null==n?\"\":n)}}}),b.extend({valHooks:{option:{get:function(e){var t=e.attributes.value;return!t||t.specified?e.value:e.text}},select:{get:function(e){var t,n,r=e.options,i=e.selectedIndex,o=\"select-one\"===e.type||0>i,a=o?null:[],s=o?i+1:r.length,u=0>i?s:o?i:0;for(;s>u;u++)if(n=r[u],!(!n.selected&&u!==i||(b.support.optDisabled?n.disabled:null!==n.getAttribute(\"disabled\"))||n.parentNode.disabled&&b.nodeName(n.parentNode,\"optgroup\"))){if(t=b(n).val(),o)return t;a.push(t)}return a},set:function(e,t){var n=b.makeArray(t);return b(e).find(\"option\").each(function(){this.selected=b.inArray(b(this).val(),n)>=0}),n.length||(e.selectedIndex=-1),n}}},attr:function(e,n,r){var o,a,s,u=e.nodeType;if(e&&3!==u&&8!==u&&2!==u)return typeof e.getAttribute===i?b.prop(e,n,r):(a=1!==u||!b.isXMLDoc(e),a&&(n=n.toLowerCase(),o=b.attrHooks[n]||(J.test(n)?z:I)),r===t?o&&a&&\"get\"in o&&null!==(s=o.get(e,n))?s:(typeof e.getAttribute!==i&&(s=e.getAttribute(n)),null==s?t:s):null!==r?o&&a&&\"set\"in o&&(s=o.set(e,r,n))!==t?s:(e.setAttribute(n,r+\"\"),r):(b.removeAttr(e,n),t))},removeAttr:function(e,t){var n,r,i=0,o=t&&t.match(w);if(o&&1===e.nodeType)while(n=o[i++])r=b.propFix[n]||n,J.test(n)?!Q&&G.test(n)?e[b.camelCase(\"default-\"+n)]=e[r]=!1:e[r]=!1:b.attr(e,n,\"\"),e.removeAttribute(Q?n:r)},attrHooks:{type:{set:function(e,t){if(!b.support.radioValue&&\"radio\"===t&&b.nodeName(e,\"input\")){var n=e.value;return e.setAttribute(\"type\",t),n&&(e.value=n),t}}}},propFix:{tabindex:\"tabIndex\",readonly:\"readOnly\",\"for\":\"htmlFor\",\"class\":\"className\",maxlength:\"maxLength\",cellspacing:\"cellSpacing\",cellpadding:\"cellPadding\",rowspan:\"rowSpan\",colspan:\"colSpan\",usemap:\"useMap\",frameborder:\"frameBorder\",contenteditable:\"contentEditable\"},prop:function(e,n,r){var i,o,a,s=e.nodeType;if(e&&3!==s&&8!==s&&2!==s)return a=1!==s||!b.isXMLDoc(e),a&&(n=b.propFix[n]||n,o=b.propHooks[n]),r!==t?o&&\"set\"in o&&(i=o.set(e,r,n))!==t?i:e[n]=r:o&&\"get\"in o&&null!==(i=o.get(e,n))?i:e[n]},propHooks:{tabIndex:{get:function(e){var n=e.getAttributeNode(\"tabindex\");return n&&n.specified?parseInt(n.value,10):V.test(e.nodeName)||Y.test(e.nodeName)&&e.href?0:t}}}}),z={get:function(e,n){var r=b.prop(e,n),i=\"boolean\"==typeof r&&e.getAttribute(n),o=\"boolean\"==typeof r?K&&Q?null!=i:G.test(n)?e[b.camelCase(\"default-\"+n)]:!!i:e.getAttributeNode(n);return o&&o.value!==!1?n.toLowerCase():t},set:function(e,t,n){return t===!1?b.removeAttr(e,n):K&&Q||!G.test(n)?e.setAttribute(!Q&&b.propFix[n]||n,n):e[b.camelCase(\"default-\"+n)]=e[n]=!0,n}},K&&Q||(b.attrHooks.value={get:function(e,n){var r=e.getAttributeNode(n);return b.nodeName(e,\"input\")?e.defaultValue:r&&r.specified?r.value:t},set:function(e,n,r){return b.nodeName(e,\"input\")?(e.defaultValue=n,t):I&&I.set(e,n,r)}}),Q||(I=b.valHooks.button={get:function(e,n){var r=e.getAttributeNode(n);return r&&(\"id\"===n||\"name\"===n||\"coords\"===n?\"\"!==r.value:r.specified)?r.value:t},set:function(e,n,r){var i=e.getAttributeNode(r);return i||e.setAttributeNode(i=e.ownerDocument.createAttribute(r)),i.value=n+=\"\",\"value\"===r||n===e.getAttribute(r)?n:t}},b.attrHooks.contenteditable={get:I.get,set:function(e,t,n){I.set(e,\"\"===t?!1:t,n)}},b.each([\"width\",\"height\"],function(e,n){b.attrHooks[n]=b.extend(b.attrHooks[n],{set:function(e,r){return\"\"===r?(e.setAttribute(n,\"auto\"),r):t}})})),b.support.hrefNormalized||(b.each([\"href\",\"src\",\"width\",\"height\"],function(e,n){b.attrHooks[n]=b.extend(b.attrHooks[n],{get:function(e){var r=e.getAttribute(n,2);return null==r?t:r}})}),b.each([\"href\",\"src\"],function(e,t){b.propHooks[t]={get:function(e){return e.getAttribute(t,4)}}})),b.support.style||(b.attrHooks.style={get:function(e){return e.style.cssText||t},set:function(e,t){return e.style.cssText=t+\"\"}}),b.support.optSelected||(b.propHooks.selected=b.extend(b.propHooks.selected,{get:function(e){var t=e.parentNode;return t&&(t.selectedIndex,t.parentNode&&t.parentNode.selectedIndex),null}})),b.support.enctype||(b.propFix.enctype=\"encoding\"),b.support.checkOn||b.each([\"radio\",\"checkbox\"],function(){b.valHooks[this]={get:function(e){return null===e.getAttribute(\"value\")?\"on\":e.value}}}),b.each([\"radio\",\"checkbox\"],function(){b.valHooks[this]=b.extend(b.valHooks[this],{set:function(e,n){return b.isArray(n)?e.checked=b.inArray(b(e).val(),n)>=0:t}})});var Z=/^(?:input|select|textarea)$/i,et=/^key/,tt=/^(?:mouse|contextmenu)|click/,nt=/^(?:focusinfocus|focusoutblur)$/,rt=/^([^.]*)(?:\\.(.+)|)$/;function it(){return!0}function ot(){return!1}b.event={global:{},add:function(e,n,r,o,a){var s,u,l,c,p,f,d,h,g,m,y,v=b._data(e);if(v){r.handler&&(c=r,r=c.handler,a=c.selector),r.guid||(r.guid=b.guid++),(u=v.events)||(u=v.events={}),(f=v.handle)||(f=v.handle=function(e){return typeof b===i||e&&b.event.triggered===e.type?t:b.event.dispatch.apply(f.elem,arguments)},f.elem=e),n=(n||\"\").match(w)||[\"\"],l=n.length;while(l--)s=rt.exec(n[l])||[],g=y=s[1],m=(s[2]||\"\").split(\".\").sort(),p=b.event.special[g]||{},g=(a?p.delegateType:p.bindType)||g,p=b.event.special[g]||{},d=b.extend({type:g,origType:y,data:o,handler:r,guid:r.guid,selector:a,needsContext:a&&b.expr.match.needsContext.test(a),namespace:m.join(\".\")},c),(h=u[g])||(h=u[g]=[],h.delegateCount=0,p.setup&&p.setup.call(e,o,m,f)!==!1||(e.addEventListener?e.addEventListener(g,f,!1):e.attachEvent&&e.attachEvent(\"on\"+g,f))),p.add&&(p.add.call(e,d),d.handler.guid||(d.handler.guid=r.guid)),a?h.splice(h.delegateCount++,0,d):h.push(d),b.event.global[g]=!0;e=null}},remove:function(e,t,n,r,i){var o,a,s,u,l,c,p,f,d,h,g,m=b.hasData(e)&&b._data(e);if(m&&(c=m.events)){t=(t||\"\").match(w)||[\"\"],l=t.length;while(l--)if(s=rt.exec(t[l])||[],d=g=s[1],h=(s[2]||\"\").split(\".\").sort(),d){p=b.event.special[d]||{},d=(r?p.delegateType:p.bindType)||d,f=c[d]||[],s=s[2]&&RegExp(\"(^|\\\\.)\"+h.join(\"\\\\.(?:.*\\\\.|)\")+\"(\\\\.|$)\"),u=o=f.length;while(o--)a=f[o],!i&&g!==a.origType||n&&n.guid!==a.guid||s&&!s.test(a.namespace)||r&&r!==a.selector&&(\"**\"!==r||!a.selector)||(f.splice(o,1),a.selector&&f.delegateCount--,p.remove&&p.remove.call(e,a));u&&!f.length&&(p.teardown&&p.teardown.call(e,h,m.handle)!==!1||b.removeEvent(e,d,m.handle),delete c[d])}else for(d in c)b.event.remove(e,d+t[l],n,r,!0);b.isEmptyObject(c)&&(delete m.handle,b._removeData(e,\"events\"))}},trigger:function(n,r,i,a){var s,u,l,c,p,f,d,h=[i||o],g=y.call(n,\"type\")?n.type:n,m=y.call(n,\"namespace\")?n.namespace.split(\".\"):[];if(l=f=i=i||o,3!==i.nodeType&&8!==i.nodeType&&!nt.test(g+b.event.triggered)&&(g.indexOf(\".\")>=0&&(m=g.split(\".\"),g=m.shift(),m.sort()),u=0>g.indexOf(\":\")&&\"on\"+g,n=n[b.expando]?n:new b.Event(g,\"object\"==typeof n&&n),n.isTrigger=!0,n.namespace=m.join(\".\"),n.namespace_re=n.namespace?RegExp(\"(^|\\\\.)\"+m.join(\"\\\\.(?:.*\\\\.|)\")+\"(\\\\.|$)\"):null,n.result=t,n.target||(n.target=i),r=null==r?[n]:b.makeArray(r,[n]),p=b.event.special[g]||{},a||!p.trigger||p.trigger.apply(i,r)!==!1)){if(!a&&!p.noBubble&&!b.isWindow(i)){for(c=p.delegateType||g,nt.test(c+g)||(l=l.parentNode);l;l=l.parentNode)h.push(l),f=l;f===(i.ownerDocument||o)&&h.push(f.defaultView||f.parentWindow||e)}d=0;while((l=h[d++])&&!n.isPropagationStopped())n.type=d>1?c:p.bindType||g,s=(b._data(l,\"events\")||{})[n.type]&&b._data(l,\"handle\"),s&&s.apply(l,r),s=u&&l[u],s&&b.acceptData(l)&&s.apply&&s.apply(l,r)===!1&&n.preventDefault();if(n.type=g,!(a||n.isDefaultPrevented()||p._default&&p._default.apply(i.ownerDocument,r)!==!1||\"click\"===g&&b.nodeName(i,\"a\")||!b.acceptData(i)||!u||!i[g]||b.isWindow(i))){f=i[u],f&&(i[u]=null),b.event.triggered=g;try{i[g]()}catch(v){}b.event.triggered=t,f&&(i[u]=f)}return n.result}},dispatch:function(e){e=b.event.fix(e);var n,r,i,o,a,s=[],u=h.call(arguments),l=(b._data(this,\"events\")||{})[e.type]||[],c=b.event.special[e.type]||{};if(u[0]=e,e.delegateTarget=this,!c.preDispatch||c.preDispatch.call(this,e)!==!1){s=b.event.handlers.call(this,e,l),n=0;while((o=s[n++])&&!e.isPropagationStopped()){e.currentTarget=o.elem,a=0;while((i=o.handlers[a++])&&!e.isImmediatePropagationStopped())(!e.namespace_re||e.namespace_re.test(i.namespace))&&(e.handleObj=i,e.data=i.data,r=((b.event.special[i.origType]||{}).handle||i.handler).apply(o.elem,u),r!==t&&(e.result=r)===!1&&(e.preventDefault(),e.stopPropagation()))}return c.postDispatch&&c.postDispatch.call(this,e),e.result}},handlers:function(e,n){var r,i,o,a,s=[],u=n.delegateCount,l=e.target;if(u&&l.nodeType&&(!e.button||\"click\"!==e.type))for(;l!=this;l=l.parentNode||this)if(1===l.nodeType&&(l.disabled!==!0||\"click\"!==e.type)){for(o=[],a=0;u>a;a++)i=n[a],r=i.selector+\" \",o[r]===t&&(o[r]=i.needsContext?b(r,this).index(l)>=0:b.find(r,this,null,[l]).length),o[r]&&o.push(i);o.length&&s.push({elem:l,handlers:o})}return n.length>u&&s.push({elem:this,handlers:n.slice(u)}),s},fix:function(e){if(e[b.expando])return e;var t,n,r,i=e.type,a=e,s=this.fixHooks[i];s||(this.fixHooks[i]=s=tt.test(i)?this.mouseHooks:et.test(i)?this.keyHooks:{}),r=s.props?this.props.concat(s.props):this.props,e=new b.Event(a),t=r.length;while(t--)n=r[t],e[n]=a[n];return e.target||(e.target=a.srcElement||o),3===e.target.nodeType&&(e.target=e.target.parentNode),e.metaKey=!!e.metaKey,s.filter?s.filter(e,a):e},props:\"altKey bubbles cancelable ctrlKey currentTarget eventPhase metaKey relatedTarget shiftKey target timeStamp view which\".split(\" \"),fixHooks:{},keyHooks:{props:\"char charCode key keyCode\".split(\" \"),filter:function(e,t){return null==e.which&&(e.which=null!=t.charCode?t.charCode:t.keyCode),e}},mouseHooks:{props:\"button buttons clientX clientY fromElement offsetX offsetY pageX pageY screenX screenY toElement\".split(\" \"),filter:function(e,n){var r,i,a,s=n.button,u=n.fromElement;return null==e.pageX&&null!=n.clientX&&(i=e.target.ownerDocument||o,a=i.documentElement,r=i.body,e.pageX=n.clientX+(a&&a.scrollLeft||r&&r.scrollLeft||0)-(a&&a.clientLeft||r&&r.clientLeft||0),e.pageY=n.clientY+(a&&a.scrollTop||r&&r.scrollTop||0)-(a&&a.clientTop||r&&r.clientTop||0)),!e.relatedTarget&&u&&(e.relatedTarget=u===e.target?n.toElement:u),e.which||s===t||(e.which=1&s?1:2&s?3:4&s?2:0),e}},special:{load:{noBubble:!0},click:{trigger:function(){return b.nodeName(this,\"input\")&&\"checkbox\"===this.type&&this.click?(this.click(),!1):t}},focus:{trigger:function(){if(this!==o.activeElement&&this.focus)try{return this.focus(),!1}catch(e){}},delegateType:\"focusin\"},blur:{trigger:function(){return this===o.activeElement&&this.blur?(this.blur(),!1):t},delegateType:\"focusout\"},beforeunload:{postDispatch:function(e){e.result!==t&&(e.originalEvent.returnValue=e.result)}}},simulate:function(e,t,n,r){var i=b.extend(new b.Event,n,{type:e,isSimulated:!0,originalEvent:{}});r?b.event.trigger(i,null,t):b.event.dispatch.call(t,i),i.isDefaultPrevented()&&n.preventDefault()}},b.removeEvent=o.removeEventListener?function(e,t,n){e.removeEventListener&&e.removeEventListener(t,n,!1)}:function(e,t,n){var r=\"on\"+t;e.detachEvent&&(typeof e[r]===i&&(e[r]=null),e.detachEvent(r,n))},b.Event=function(e,n){return this instanceof b.Event?(e&&e.type?(this.originalEvent=e,this.type=e.type,this.isDefaultPrevented=e.defaultPrevented||e.returnValue===!1||e.getPreventDefault&&e.getPreventDefault()?it:ot):this.type=e,n&&b.extend(this,n),this.timeStamp=e&&e.timeStamp||b.now(),this[b.expando]=!0,t):new b.Event(e,n)},b.Event.prototype={isDefaultPrevented:ot,isPropagationStopped:ot,isImmediatePropagationStopped:ot,preventDefault:function(){var e=this.originalEvent;this.isDefaultPrevented=it,e&&(e.preventDefault?e.preventDefault():e.returnValue=!1)},stopPropagation:function(){var e=this.originalEvent;this.isPropagationStopped=it,e&&(e.stopPropagation&&e.stopPropagation(),e.cancelBubble=!0)},stopImmediatePropagation:function(){this.isImmediatePropagationStopped=it,this.stopPropagation()}},b.each({mouseenter:\"mouseover\",mouseleave:\"mouseout\"},function(e,t){b.event.special[e]={delegateType:t,bindType:t,handle:function(e){var n,r=this,i=e.relatedTarget,o=e.handleObj;\n"
"return(!i||i!==r&&!b.contains(r,i))&&(e.type=o.origType,n=o.handler.apply(this,arguments),e.type=t),n}}}),b.support.submitBubbles||(b.event.special.submit={setup:function(){return b.nodeName(this,\"form\")?!1:(b.event.add(this,\"click._submit keypress._submit\",function(e){var n=e.target,r=b.nodeName(n,\"input\")||b.nodeName(n,\"button\")?n.form:t;r&&!b._data(r,\"submitBubbles\")&&(b.event.add(r,\"submit._submit\",function(e){e._submit_bubble=!0}),b._data(r,\"submitBubbles\",!0))}),t)},postDispatch:function(e){e._submit_bubble&&(delete e._submit_bubble,this.parentNode&&!e.isTrigger&&b.event.simulate(\"submit\",this.parentNode,e,!0))},teardown:function(){return b.nodeName(this,\"form\")?!1:(b.event.remove(this,\"._submit\"),t)}}),b.support.changeBubbles||(b.event.special.change={setup:function(){return Z.test(this.nodeName)?((\"checkbox\"===this.type||\"radio\"===this.type)&&(b.event.add(this,\"propertychange._change\",function(e){\"checked\"===e.originalEvent.propertyName&&(this._just_changed=!0)}),b.event.add(this,\"click._change\",function(e){this._just_changed&&!e.isTrigger&&(this._just_changed=!1),b.event.simulate(\"change\",this,e,!0)})),!1):(b.event.add(this,\"beforeactivate._change\",function(e){var t=e.target;Z.test(t.nodeName)&&!b._data(t,\"changeBubbles\")&&(b.event.add(t,\"change._change\",function(e){!this.parentNode||e.isSimulated||e.isTrigger||b.event.simulate(\"change\",this.parentNode,e,!0)}),b._data(t,\"changeBubbles\",!0))}),t)},handle:function(e){var n=e.target;return this!==n||e.isSimulated||e.isTrigger||\"radio\"!==n.type&&\"checkbox\"!==n.type?e.handleObj.handler.apply(this,arguments):t},teardown:function(){return b.event.remove(this,\"._change\"),!Z.test(this.nodeName)}}),b.support.focusinBubbles||b.each({focus:\"focusin\",blur:\"focusout\"},function(e,t){var n=0,r=function(e){b.event.simulate(t,e.target,b.event.fix(e),!0)};b.event.special[t]={setup:function(){0===n++&&o.addEventListener(e,r,!0)},teardown:function(){0===--n&&o.removeEventListener(e,r,!0)}}}),b.fn.extend({on:function(e,n,r,i,o){var a,s;if(\"object\"==typeof e){\"string\"!=typeof n&&(r=r||n,n=t);for(a in e)this.on(a,n,r,e[a],o);return this}if(null==r&&null==i?(i=n,r=n=t):null==i&&(\"string\"==typeof n?(i=r,r=t):(i=r,r=n,n=t)),i===!1)i=ot;else if(!i)return this;return 1===o&&(s=i,i=function(e){return b().off(e),s.apply(this,arguments)},i.guid=s.guid||(s.guid=b.guid++)),this.each(function(){b.event.add(this,e,i,r,n)})},one:function(e,t,n,r){return this.on(e,t,n,r,1)},off:function(e,n,r){var i,o;if(e&&e.preventDefault&&e.handleObj)return i=e.handleObj,b(e.delegateTarget).off(i.namespace?i.origType+\".\"+i.namespace:i.origType,i.selector,i.handler),this;if(\"object\"==typeof e){for(o in e)this.off(o,n,e[o]);return this}return(n===!1||\"function\"==typeof n)&&(r=n,n=t),r===!1&&(r=ot),this.each(function(){b.event.remove(this,e,r,n)})},bind:function(e,t,n){return this.on(e,null,t,n)},unbind:function(e,t){return this.off(e,null,t)},delegate:function(e,t,n,r){return this.on(t,e,n,r)},undelegate:function(e,t,n){return 1===arguments.length?this.off(e,\"**\"):this.off(t,e||\"**\",n)},trigger:function(e,t){return this.each(function(){b.event.trigger(e,t,this)})},triggerHandler:function(e,n){var r=this[0];return r?b.event.trigger(e,n,r,!0):t}}),function(e,t){var n,r,i,o,a,s,u,l,c,p,f,d,h,g,m,y,v,x=\"sizzle\"+-new Date,w=e.document,T={},N=0,C=0,k=it(),E=it(),S=it(),A=typeof t,j=1<<31,D=[],L=D.pop,H=D.push,q=D.slice,M=D.indexOf||function(e){var t=0,n=this.length;for(;n>t;t++)if(this[t]===e)return t;return-1},_=\"[\\\\x20\\\\t\\\\r\\\\n\\\\f]\",F=\"(?:\\\\\\\\.|[\\\\w-]|[^\\\\x00-\\\\xa0])+\",O=F.replace(\"w\",\"w#\"),B=\"([*^$|!~]?=)\",P=\"\\\\[\"+_+\"*(\"+F+\")\"+_+\"*(?:\"+B+_+\"*(?:(['\\\"])((?:\\\\\\\\.|[^\\\\\\\\])*?)\\\\3|(\"+O+\")|)|)\"+_+\"*\\\\]\",R=\":(\"+F+\")(?:\\\\(((['\\\"])((?:\\\\\\\\.|[^\\\\\\\\])*?)\\\\3|((?:\\\\\\\\.|[^\\\\\\\\()[\\\\]]|\"+P.replace(3,8)+\")*)|.*)\\\\)|)\",W=RegExp(\"^\"+_+\"+|((?:^|[^\\\\\\\\])(?:\\\\\\\\.)*)\"+_+\"+$\",\"g\"),$=RegExp(\"^\"+_+\"*,\"+_+\"*\"),I=RegExp(\"^\"+_+\"*([\\\\x20\\\\t\\\\r\\\\n\\\\f>+~])\"+_+\"*\"),z=RegExp(R),X=RegExp(\"^\"+O+\"$\"),U={ID:RegExp(\"^#(\"+F+\")\"),CLASS:RegExp(\"^\\\\.(\"+F+\")\"),NAME:RegExp(\"^\\\\[name=['\\\"]?(\"+F+\")['\\\"]?\\\\]\"),TAG:RegExp(\"^(\"+F.replace(\"w\",\"w*\")+\")\"),ATTR:RegExp(\"^\"+P),PSEUDO:RegExp(\"^\"+R),CHILD:RegExp(\"^:(only|first|last|nth|nth-last)-(child|of-type)(?:\\\\(\"+_+\"*(even|odd|(([+-]|)(\\\\d*)n|)\"+_+\"*(?:([+-]|)\"+_+\"*(\\\\d+)|))\"+_+\"*\\\\)|)\",\"i\"),needsContext:RegExp(\"^\"+_+\"*[>+~]|:(even|odd|eq|gt|lt|nth|first|last)(?:\\\\(\"+_+\"*((?:-\\\\d)?\\\\d*)\"+_+\"*\\\\)|)(?=[^-]|$)\",\"i\")},V=/[\\x20\\t\\r\\n\\f]*[+~]/,Y=/^[^{]+\\{\\s*\\[native code/,J=/^(?:#([\\w-]+)|(\\w+)|\\.([\\w-]+))$/,G=/^(?:input|select|textarea|button)$/i,Q=/^h\\d$/i,K=/'|\\\\/g,Z=/\\=[\\x20\\t\\r\\n\\f]*([^'\"\\]]*)[\\x20\\t\\r\\n\\f]*\\]/g,et=/\\\\([\\da-fA-F]{1,6}[\\x20\\t\\r\\n\\f]?|.)/g,tt=function(e,t){var n=\"0x\"+t-65536;return n!==n?t:0>n?String.fromCharCode(n+65536):String.fromCharCode(55296|n>>10,56320|1023&n)};try{q.call(w.documentElement.childNodes,0)[0].nodeType}catch(nt){q=function(e){var t,n=[];while(t=this[e++])n.push(t);return n}}function rt(e){return Y.test(e+\"\")}function it(){var e,t=[];return e=function(n,r){return t.push(n+=\" \")>i.cacheLength&&delete e[t.shift()],e[n]=r}}function ot(e){return e[x]=!0,e}function at(e){var t=p.createElement(\"div\");try{return e(t)}catch(n){return!1}finally{t=null}}function st(e,t,n,r){var i,o,a,s,u,l,f,g,m,v;if((t?t.ownerDocument||t:w)!==p&&c(t),t=t||p,n=n||[],!e||\"string\"!=typeof e)return n;if(1!==(s=t.nodeType)&&9!==s)return[];if(!d&&!r){if(i=J.exec(e))if(a=i[1]){if(9===s){if(o=t.getElementById(a),!o||!o.parentNode)return n;if(o.id===a)return n.push(o),n}else if(t.ownerDocument&&(o=t.ownerDocument.getElementById(a))&&y(t,o)&&o.id===a)return n.push(o),n}else{if(i[2])return H.apply(n,q.call(t.getElementsByTagName(e),0)),n;if((a=i[3])&&T.getByClassName&&t.getElementsByClassName)return H.apply(n,q.call(t.getElementsByClassName(a),0)),n}if(T.qsa&&!h.test(e)){if(f=!0,g=x,m=t,v=9===s&&e,1===s&&\"object\"!==t.nodeName.toLowerCase()){l=ft(e),(f=t.getAttribute(\"id\"))?g=f.replace(K,\"\\\\$&\"):t.setAttribute(\"id\",g),g=\"[id='\"+g+\"'] \",u=l.length;while(u--)l[u]=g+dt(l[u]);m=V.test(e)&&t.parentNode||t,v=l.join(\",\")}if(v)try{return H.apply(n,q.call(m.querySelectorAll(v),0)),n}catch(b){}finally{f||t.removeAttribute(\"id\")}}}return wt(e.replace(W,\"$1\"),t,n,r)}a=st.isXML=function(e){var t=e&&(e.ownerDocument||e).documentElement;return t?\"HTML\"!==t.nodeName:!1},c=st.setDocument=function(e){var n=e?e.ownerDocument||e:w;return n!==p&&9===n.nodeType&&n.documentElement?(p=n,f=n.documentElement,d=a(n),T.tagNameNoComments=at(function(e){return e.appendChild(n.createComment(\"\")),!e.getElementsByTagName(\"*\").length}),T.attributes=at(function(e){e.innerHTML=\"<select></select>\";var t=typeof e.lastChild.getAttribute(\"multiple\");return\"boolean\"!==t&&\"string\"!==t}),T.getByClassName=at(function(e){return e.innerHTML=\"<div class='hidden e'></div><div class='hidden'></div>\",e.getElementsByClassName&&e.getElementsByClassName(\"e\").length?(e.lastChild.className=\"e\",2===e.getElementsByClassName(\"e\").length):!1}),T.getByName=at(function(e){e.id=x+0,e.innerHTML=\"<a name='\"+x+\"'></a><div name='\"+x+\"'></div>\",f.insertBefore(e,f.firstChild);var t=n.getElementsByName&&n.getElementsByName(x).length===2+n.getElementsByName(x+0).length;return T.getIdNotName=!n.getElementById(x),f.removeChild(e),t}),i.attrHandle=at(function(e){return e.innerHTML=\"<a href='#'></a>\",e.firstChild&&typeof e.firstChild.getAttribute!==A&&\"#\"===e.firstChild.getAttribute(\"href\")})?{}:{href:function(e){return e.getAttribute(\"href\",2)},type:function(e){return e.getAttribute(\"type\")}},T.getIdNotName?(i.find.ID=function(e,t){if(typeof t.getElementById!==A&&!d){var n=t.getElementById(e);return n&&n.parentNode?[n]:[]}},i.filter.ID=function(e){var t=e.replace(et,tt);return function(e){return e.getAttribute(\"id\")===t}}):(i.find.ID=function(e,n){if(typeof n.getElementById!==A&&!d){var r=n.getElementById(e);return r?r.id===e||typeof r.getAttributeNode!==A&&r.getAttributeNode(\"id\").value===e?[r]:t:[]}},i.filter.ID=function(e){var t=e.replace(et,tt);return function(e){var n=typeof e.getAttributeNode!==A&&e.getAttributeNode(\"id\");return n&&n.value===t}}),i.find.TAG=T.tagNameNoComments?function(e,n){return typeof n.getElementsByTagName!==A?n.getElementsByTagName(e):t}:function(e,t){var n,r=[],i=0,o=t.getElementsByTagName(e);if(\"*\"===e){while(n=o[i++])1===n.nodeType&&r.push(n);return r}return o},i.find.NAME=T.getByName&&function(e,n){return typeof n.getElementsByName!==A?n.getElementsByName(name):t},i.find.CLASS=T.getByClassName&&function(e,n){return typeof n.getElementsByClassName===A||d?t:n.getElementsByClassName(e)},g=[],h=[\":focus\"],(T.qsa=rt(n.querySelectorAll))&&(at(function(e){e.innerHTML=\"<select><option selected=''></option></select>\",e.querySelectorAll(\"[selected]\").length||h.push(\"\\\\[\"+_+\"*(?:checked|disabled|ismap|multiple|readonly|selected|value)\"),e.querySelectorAll(\":checked\").length||h.push(\":checked\")}),at(function(e){e.innerHTML=\"<input type='hidden' i=''/>\",e.querySelectorAll(\"[i^='']\").length&&h.push(\"[*^$]=\"+_+\"*(?:\\\"\\\"|'')\"),e.querySelectorAll(\":enabled\").length||h.push(\":enabled\",\":disabled\"),e.querySelectorAll(\"*,:x\"),h.push(\",.*:\")})),(T.matchesSelector=rt(m=f.matchesSelector||f.mozMatchesSelector||f.webkitMatchesSelector||f.oMatchesSelector||f.msMatchesSelector))&&at(function(e){T.disconnectedMatch=m.call(e,\"div\"),m.call(e,\"[s!='']:x\"),g.push(\"!=\",R)}),h=RegExp(h.join(\"|\")),g=RegExp(g.join(\"|\")),y=rt(f.contains)||f.compareDocumentPosition?function(e,t){var n=9===e.nodeType?e.documentElement:e,r=t&&t.parentNode;return e===r||!(!r||1!==r.nodeType||!(n.contains?n.contains(r):e.compareDocumentPosition&&16&e.compareDocumentPosition(r)))}:function(e,t){if(t)while(t=t.parentNode)if(t===e)return!0;return!1},v=f.compareDocumentPosition?function(e,t){var r;return e===t?(u=!0,0):(r=t.compareDocumentPosition&&e.compareDocumentPosition&&e.compareDocumentPosition(t))?1&r||e.parentNode&&11===e.parentNode.nodeType?e===n||y(w,e)?-1:t===n||y(w,t)?1:0:4&r?-1:1:e.compareDocumentPosition?-1:1}:function(e,t){var r,i=0,o=e.parentNode,a=t.parentNode,s=[e],l=[t];if(e===t)return u=!0,0;if(!o||!a)return e===n?-1:t===n?1:o?-1:a?1:0;if(o===a)return ut(e,t);r=e;while(r=r.parentNode)s.unshift(r);r=t;while(r=r.parentNode)l.unshift(r);while(s[i]===l[i])i++;return i?ut(s[i],l[i]):s[i]===w?-1:l[i]===w?1:0},u=!1,[0,0].sort(v),T.detectDuplicates=u,p):p},st.matches=function(e,t){return st(e,null,null,t)},st.matchesSelector=function(e,t){if((e.ownerDocument||e)!==p&&c(e),t=t.replace(Z,\"='$1']\"),!(!T.matchesSelector||d||g&&g.test(t)||h.test(t)))try{var n=m.call(e,t);if(n||T.disconnectedMatch||e.document&&11!==e.document.nodeType)return n}catch(r){}return st(t,p,null,[e]).length>0},st.contains=function(e,t){return(e.ownerDocument||e)!==p&&c(e),y(e,t)},st.attr=function(e,t){var n;return(e.ownerDocument||e)!==p&&c(e),d||(t=t.toLowerCase()),(n=i.attrHandle[t])?n(e):d||T.attributes?e.getAttribute(t):((n=e.getAttributeNode(t))||e.getAttribute(t))&&e[t]===!0?t:n&&n.specified?n.value:null},st.error=function(e){throw Error(\"Syntax error, unrecognized expression: \"+e)},st.uniqueSort=function(e){var t,n=[],r=1,i=0;if(u=!T.detectDuplicates,e.sort(v),u){for(;t=e[r];r++)t===e[r-1]&&(i=n.push(r));while(i--)e.splice(n[i],1)}return e};function ut(e,t){var n=t&&e,r=n&&(~t.sourceIndex||j)-(~e.sourceIndex||j);if(r)return r;if(n)while(n=n.nextSibling)if(n===t)return-1;return e?1:-1}function lt(e){return function(t){var n=t.nodeName.toLowerCase();return\"input\"===n&&t.type===e}}function ct(e){return function(t){var n=t.nodeName.toLowerCase();return(\"input\"===n||\"button\"===n)&&t.type===e}}function pt(e){return ot(function(t){return t=+t,ot(function(n,r){var i,o=e([],n.length,t),a=o.length;while(a--)n[i=o[a]]&&(n[i]=!(r[i]=n[i]))})})}o=st.getText=function(e){var t,n=\"\",r=0,i=e.nodeType;if(i){if(1===i||9===i||11===i){if(\"string\"==typeof e.textContent)return e.textContent;for(e=e.firstChild;e;e=e.nextSibling)n+=o(e)}else if(3===i||4===i)return e.nodeValue}else for(;t=e[r];r++)n+=o(t);return n},i=st.selectors={cacheLength:50,createPseudo:ot,match:U,find:{},relative:{\">\":{dir:\"parentNode\",first:!0},\" \":{dir:\"parentNode\"},\"+\":{dir:\"previousSibling\",first:!0},\"~\":{dir:\"previousSibling\"}},preFilter:{ATTR:function(e){return e[1]=e[1].replace(et,tt),e[3]=(e[4]||e[5]||\"\").replace(et,tt),\"~=\"===e[2]&&(e[3]=\" \"+e[3]+\" \"),e.slice(0,4)},CHILD:function(e){return e[1]=e[1].toLowerCase(),\"nth\"===e[1].slice(0,3)?(e[3]||st.error(e[0]),e[4]=+(e[4]?e[5]+(e[6]||1):2*(\"even\"===e[3]||\"odd\"===e[3])),e[5]=+(e[7]+e[8]||\"odd\"===e[3])):e[3]&&st.error(e[0]),e},PSEUDO:function(e){var t,n=!e[5]&&e[2];return U.CHILD.test(e[0])?null:(e[4]?e[2]=e[4]:n&&z.test(n)&&(t=ft(n,!0))&&(t=n.indexOf(\")\",n.length-t)-n.length)&&(e[0]=e[0].slice(0,t),e[2]=n.slice(0,t)),e.slice(0,3))}},filter:{TAG:function(e){return\"*\"===e?function(){return!0}:(e=e.replace(et,tt).toLowerCase(),function(t){return t.nodeName&&t.nodeName.toLowerCase()===e})},CLASS:function(e){var t=k[e+\" \"];return t||(t=RegExp(\"(^|\"+_+\")\"+e+\"(\"+_+\"|$)\"))&&k(e,function(e){return t.test(e.className||typeof e.getAttribute!==A&&e.getAttribute(\"class\")||\"\")})},ATTR:function(e,t,n){return function(r){var i=st.attr(r,e);return null==i?\"!=\"===t:t?(i+=\"\",\"=\"===t?i===n:\"!=\"===t?i!==n:\"^=\"===t?n&&0===i.indexOf(n):\"*=\"===t?n&&i.indexOf(n)>-1:\"$=\"===t?n&&i.slice(-n.length)===n:\"~=\"===t?(\" \"+i+\" \").indexOf(n)>-1:\"|=\"===t?i===n||i.slice(0,n.length+1)===n+\"-\":!1):!0}},CHILD:function(e,t,n,r,i){var o=\"nth\"!==e.slice(0,3),a=\"last\"!==e.slice(-4),s=\"of-type\"===t;return 1===r&&0===i?function(e){return!!e.parentNode}:function(t,n,u){var l,c,p,f,d,h,g=o!==a?\"nextSibling\":\"previousSibling\",m=t.parentNode,y=s&&t.nodeName.toLowerCase(),v=!u&&!s;if(m){if(o){while(g){p=t;while(p=p[g])if(s?p.nodeName.toLowerCase()===y:1===p.nodeType)return!1;h=g=\"only\"===e&&!h&&\"nextSibling\"}return!0}if(h=[a?m.firstChild:m.lastChild],a&&v){c=m[x]||(m[x]={}),l=c[e]||[],d=l[0]===N&&l[1],f=l[0]===N&&l[2],p=d&&m.childNodes[d];while(p=++d&&p&&p[g]||(f=d=0)||h.pop())if(1===p.nodeType&&++f&&p===t){c[e]=[N,d,f];break}}else if(v&&(l=(t[x]||(t[x]={}))[e])&&l[0]===N)f=l[1];else while(p=++d&&p&&p[g]||(f=d=0)||h.pop())if((s?p.nodeName.toLowerCase()===y:1===p.nodeType)&&++f&&(v&&((p[x]||(p[x]={}))[e]=[N,f]),p===t))break;return f-=i,f===r||0===f%r&&f/r>=0}}},PSEUDO:function(e,t){var n,r=i.pseudos[e]||i.setFilters[e.toLowerCase()]||st.error(\"unsupported pseudo: \"+e);return r[x]?r(t):r.length>1?(n=[e,e,\"\",t],i.setFilters.hasOwnProperty(e.toLowerCase())?ot(function(e,n){var i,o=r(e,t),a=o.length;while(a--)i=M.call(e,o[a]),e[i]=!(n[i]=o[a])}):function(e){return r(e,0,n)}):r}},pseudos:{not:ot(function(e){var t=[],n=[],r=s(e.replace(W,\"$1\"));return r[x]?ot(function(e,t,n,i){var o,a=r(e,null,i,[]),s=e.length;while(s--)(o=a[s])&&(e[s]=!(t[s]=o))}):function(e,i,o){return t[0]=e,r(t,null,o,n),!n.pop()}}),has:ot(function(e){return function(t){return st(e,t).length>0}}),contains:ot(function(e){return function(t){return(t.textContent||t.innerText||o(t)).indexOf(e)>-1}}),lang:ot(function(e){return X.test(e||\"\")||st.error(\"unsupported lang: \"+e),e=e.replace(et,tt).toLowerCase(),function(t){var n;do if(n=d?t.getAttribute(\"xml:lang\")||t.getAttribute(\"lang\"):t.lang)return n=n.toLowerCase(),n===e||0===n.indexOf(e+\"-\");while((t=t.parentNode)&&1===t.nodeType);return!1}}),target:function(t){var n=e.location&&e.location.hash;return n&&n.slice(1)===t.id},root:function(e){return e===f},focus:function(e){return e===p.activeElement&&(!p.hasFocus||p.hasFocus())&&!!(e.type||e.href||~e.tabIndex)},enabled:function(e){return e.disabled===!1},disabled:function(e){return e.disabled===!0},checked:function(e){var t=e.nodeName.toLowerCase();return\"input\"===t&&!!e.checked||\"option\"===t&&!!e.selected},selected:function(e){return e.parentNode&&e.parentNode.selectedIndex,e.selected===!0},empty:function(e){for(e=e.firstChild;e;e=e.nextSibling)if(e.nodeName>\"@\"||3===e.nodeType||4===e.nodeType)return!1;return!0},parent:function(e){return!i.pseudos.empty(e)},header:function(e){return Q.test(e.nodeName)},input:function(e){return G.test(e.nodeName)},button:function(e){var t=e.nodeName.toLowerCase();return\"input\"===t&&\"button\"===e.type||\"button\"===t},text:function(e){var t;return\"input\"===e.nodeName.toLowerCase()&&\"text\"===e.type&&(null==(t=e.getAttribute(\"type\"))||t.toLowerCase()===e.type)},first:pt(function(){return[0]}),last:pt(function(e,t){return[t-1]}),eq:pt(function(e,t,n){return[0>n?n+t:n]}),even:pt(function(e,t){var n=0;for(;t>n;n+=2)e.push(n);return e}),odd:pt(function(e,t){var n=1;for(;t>n;n+=2)e.push(n);return e}),lt:pt(function(e,t,n){var r=0>n?n+t:n;for(;--r>=0;)e.push(r);return e}),gt:pt(function(e,t,n){var r=0>n?n+t:n;for(;t>++r;)e.push(r);return e})}};for(n in{radio:!0,checkbox:!0,file:!0,password:!0,image:!0})i.pseudos[n]=lt(n);for(n in{submit:!0,reset:!0})i.pseudos[n]=ct(n);function ft(e,t){var n,r,o,a,s,u,l,c=E[e+\" \"];if(c)return t?0:c.slice(0);s=e,u=[],l=i.preFilter;while(s){(!n||(r=$.exec(s)))&&(r&&(s=s.slice(r[0].length)||s),u.push(o=[])),n=!1,(r=I.exec(s))&&(n=r.shift(),o.push({value:n,type:r[0].replace(W,\" \")}),s=s.slice(n.length));for(a in i.filter)!(r=U[a].exec(s))||l[a]&&!(r=l[a](r))||(n=r.shift(),o.push({value:n,type:a,matches:r}),s=s.slice(n.length));if(!n)break}return t?s.length:s?st.error(e):E(e,u).slice(0)}function dt(e){var t=0,n=e.length,r=\"\";for(;n>t;t++)r+=e[t].value;return r}function ht(e,t,n){var i=t.dir,o=n&&\"parentNode\"===i,a=C++;return t.first?function(t,n,r){while(t=t[i])if(1===t.nodeType||o)return e(t,n,r)}:function(t,n,s){var u,l,c,p=N+\" \"+a;if(s){while(t=t[i])if((1===t.nodeType||o)&&e(t,n,s))return!0}else while(t=t[i])if(1===t.nodeType||o)if(c=t[x]||(t[x]={}),(l=c[i])&&l[0]===p){if((u=l[1])===!0||u===r)return u===!0}else if(l=c[i]=[p],l[1]=e(t,n,s)||r,l[1]===!0)return!0}}function gt(e){return e.length>1?function(t,n,r){var i=e.length;while(i--)if(!e[i](t,n,r))return!1;return!0}:e[0]}function mt(e,t,n,r,i){var o,a=[],s=0,u=e.length,l=null!=t;for(;u>s;s++)(o=e[s])&&(!n||n(o,r,i))&&(a.push(o),l&&t.push(s));return a}function yt(e,t,n,r,i,o){return r&&!r[x]&&(r=yt(r)),i&&!i[x]&&(i=yt(i,o)),ot(function(o,a,s,u){var l,c,p,f=[],d=[],h=a.length,g=o||xt(t||\"*\",s.nodeType?[s]:s,[]),m=!e||!o&&t?g:mt(g,f,e,s,u),y=n?i||(o?e:h||r)?[]:a:m;if(n&&n(m,y,s,u),r){l=mt(y,d),r(l,[],s,u),c=l.length;while(c--)(p=l[c])&&(y[d[c]]=!(m[d[c]]=p))}if(o){if(i||e){if(i){l=[],c=y.length;while(c--)(p=y[c])&&l.push(m[c]=p);i(null,y=[],l,u)}c=y.length;while(c--)(p=y[c])&&(l=i?M.call(o,p):f[c])>-1&&(o[l]=!(a[l]=p))}}else y=mt(y===a?y.splice(h,y.length):y),i?i(null,a,y,u):H.apply(a,y)})}function vt(e){var t,n,r,o=e.length,a=i.relative[e[0].type],s=a||i.relative[\" \"],u=a?1:0,c=ht(function(e){return e===t},s,!0),p=ht(function(e){return M.call(t,e)>-1},s,!0),f=[function(e,n,r){return!a&&(r||n!==l)||((t=n).nodeType?c(e,n,r):p(e,n,r))}];for(;o>u;u++)if(n=i.relative[e[u].type])f=[ht(gt(f),n)];else{if(n=i.filter[e[u].type].apply(null,e[u].matches),n[x]){for(r=++u;o>r;r++)if(i.relative[e[r].type])break;return yt(u>1&&gt(f),u>1&&dt(e.slice(0,u-1)).replace(W,\"$1\"),n,r>u&&vt(e.slice(u,r)),o>r&&vt(e=e.slice(r)),o>r&&dt(e))}f.push(n)}return gt(f)}function bt(e,t){var n=0,o=t.length>0,a=e.length>0,s=function(s,u,c,f,d){var h,g,m,y=[],v=0,b=\"0\",x=s&&[],w=null!=d,T=l,C=s||a&&i.find.TAG(\"*\",d&&u.parentNode||u),k=N+=null==T?1:Math.random()||.1;for(w&&(l=u!==p&&u,r=n);null!=(h=C[b]);b++){if(a&&h){g=0;while(m=e[g++])if(m(h,u,c)){f.push(h);break}w&&(N=k,r=++n)}o&&((h=!m&&h)&&v--,s&&x.push(h))}if(v+=b,o&&b!==v){g=0;while(m=t[g++])m(x,y,u,c);if(s){if(v>0)while(b--)x[b]||y[b]||(y[b]=L.call(f));y=mt(y)}H.apply(f,y),w&&!s&&y.length>0&&v+t.length>1&&st.uniqueSort(f)}return w&&(N=k,l=T),x};return o?ot(s):s}s=st.compile=function(e,t){var n,r=[],i=[],o=S[e+\" \"];if(!o){t||(t=ft(e)),n=t.length;while(n--)o=vt(t[n]),o[x]?r.push(o):i.push(o);o=S(e,bt(i,r))}return o};function xt(e,t,n){var r=0,i=t.length;for(;i>r;r++)st(e,t[r],n);return n}function wt(e,t,n,r){var o,a,u,l,c,p=ft(e);if(!r&&1===p.length){if(a=p[0]=p[0].slice(0),a.length>2&&\"ID\"===(u=a[0]).type&&9===t.nodeType&&!d&&i.relative[a[1].type]){if(t=i.find.ID(u.matches[0].replace(et,tt),t)[0],!t)return n;e=e.slice(a.shift().value.length)}o=U.needsContext.test(e)?0:a.length;while(o--){if(u=a[o],i.relative[l=u.type])break;if((c=i.find[l])&&(r=c(u.matches[0].replace(et,tt),V.test(a[0].type)&&t.parentNode||t))){if(a.splice(o,1),e=r.length&&dt(a),!e)return H.apply(n,q.call(r,0)),n;break}}}return s(e,p)(r,t,d,n,V.test(e)),n}i.pseudos.nth=i.pseudos.eq;function Tt(){}i.filters=Tt.prototype=i.pseudos,i.setFilters=new Tt,c(),st.attr=b.attr,b.find=st,b.expr=st.selectors,b.expr[\":\"]=b.expr.pseudos,b.unique=st.uniqueSort,b.text=st.getText,b.isXMLDoc=st.isXML,b.contains=st.contains}(e);var at=/Until$/,st=/^(?:parents|prev(?:Until|All))/,ut=/^.[^:#\\[\\.,]*$/,lt=b.expr.match.needsContext,ct={children:!0,contents:!0,next:!0,prev:!0};b.fn.extend({find:function(e){var t,n,r,i=this.length;if(\"string\"!=typeof e)return r=this,this.pushStack(b(e).filter(function(){for(t=0;i>t;t++)if(b.contains(r[t],this))return!0}));for(n=[],t=0;i>t;t++)b.find(e,this[t],n);return n=this.pushStack(i>1?b.unique(n):n),n.selector=(this.selector?this.selector+\" \":\"\")+e,n},has:function(e){var t,n=b(e,this),r=n.length;return this.filter(function(){for(t=0;r>t;t++)if(b.contains(this,n[t]))return!0})},not:function(e){return this.pushStack(ft(this,e,!1))},filter:function(e){return this.pushStack(ft(this,e,!0))},is:function(e){return!!e&&(\"string\"==typeof e?lt.test(e)?b(e,this.context).index(this[0])>=0:b.filter(e,this).length>0:this.filter(e).length>0)},closest:function(e,t){var n,r=0,i=this.length,o=[],a=lt.test(e)||\"string\"!=typeof e?b(e,t||this.context):0;for(;i>r;r++){n=this[r];while(n&&n.ownerDocument&&n!==t&&11!==n.nodeType){if(a?a.index(n)>-1:b.find.matchesSelector(n,e)){o.push(n);break}n=n.parentNode}}return this.pushStack(o.length>1?b.unique(o):o)},index:function(e){return e?\"string\"==typeof e?b.inArray(this[0],b(e)):b.inArray(e.jquery?e[0]:e,this):this[0]&&this[0].parentNode?this.first().prevAll().length:-1},add:function(e,t){var n=\"string\"==typeof e?b(e,t):b.makeArray(e&&e.nodeType?[e]:e),r=b.merge(this.get(),n);return this.pushStack(b.unique(r))},addBack:function(e){return this.add(null==e?this.prevObject:this.prevObject.filter(e))}}),b.fn.andSelf=b.fn.addBack;function pt(e,t){do e=e[t];while(e&&1!==e.nodeType);return e}b.each({parent:function(e){var t=e.parentNode;return t&&11!==t.nodeType?t:null},parents:function(e){return b.dir(e,\"parentNode\")},parentsUntil:function(e,t,n){return b.dir(e,\"parentNode\",n)},next:function(e){return pt(e,\"nextSibling\")},prev:function(e){return pt(e,\"previousSibling\")},nextAll:function(e){return b.dir(e,\"nextSibling\")},prevAll:function(e){return b.dir(e,\"previousSibling\")},nextUntil:function(e,t,n){return b.dir(e,\"nextSibling\",n)},prevUntil:function(e,t,n){return b.dir(e,\"previousSibling\",n)},siblings:function(e){return b.sibling((e.parentNode||{}).firstChild,e)},children:function(e){return b.sibling(e.firstChild)},contents:function(e){return b.nodeName(e,\"iframe\")?e.contentDocument||e.contentWindow.document:b.merge([],e.childNodes)}},function(e,t){b.fn[e]=function(n,r){var i=b.map(this,t,n);return at.test(e)||(r=n),r&&\"string\"==typeof r&&(i=b.filter(r,i)),i=this.length>1&&!ct[e]?b.unique(i):i,this.length>1&&st.test(e)&&(i=i.reverse()),this.pushStack(i)}}),b.extend({filter:function(e,t,n){return n&&(e=\":not(\"+e+\")\"),1===t.length?b.find.matchesSelector(t[0],e)?[t[0]]:[]:b.find.matches(e,t)},dir:function(e,n,r){var i=[],o=e[n];while(o&&9!==o.nodeType&&(r===t||1!==o.nodeType||!b(o).is(r)))1===o.nodeType&&i.push(o),o=o[n];return i},sibling:function(e,t){var n=[];for(;e;e=e.nextSibling)1===e.nodeType&&e!==t&&n.push(e);return n}});function ft(e,t,n){if(t=t||0,b.isFunction(t))return b.grep(e,function(e,r){var i=!!t.call(e,r,e);return i===n});if(t.nodeType)return b.grep(e,function(e){return e===t===n});if(\"string\"==typeof t){var r=b.grep(e,function(e){return 1===e.nodeType});if(ut.test(t))return b.filter(t,r,!n);t=b.filter(t,r)}return b.grep(e,function(e){return b.inArray(e,t)>=0===n})}function dt(e){var t=ht.split(\"|\"),n=e.createDocumentFragment();if(n.createElement)while(t.length)n.createElement(t.pop());return n}var ht=\"abbr|article|aside|audio|bdi|canvas|data|datalist|details|figcaption|figure|footer|header|hgroup|mark|meter|nav|output|progress|section|summary|time|video\",gt=/ jQuery\\d+=\"(?:null|\\d+)\"/g,mt=RegExp(\"<(?:\"+ht+\")[\\\\s/>]\",\"i\"),yt=/^\\s+/,vt=/<(?!area|br|col|embed|hr|img|input|link|meta|param)(([\\w:]+)[^>]*)\\/>/gi,bt=/<([\\w:]+)/,xt=/<tbody/i,wt=/<|&#?\\w+;/,Tt=/<(?:script|style|link)/i,Nt=/^(?:checkbox|radio)$/i,Ct=/checked\\s*(?:[^=]|=\\s*.checked.)/i,kt=/^$|\\/(?:java|ecma)script/i,Et=/^true\\/(.*)/,St=/^\\s*<!(?:\\[CDATA\\[|--)|(?:\\]\\]|--)>\\s*$/g,At={option:[1,\"<select multiple='multiple'>\",\"</select>\"],legend:[1,\"<fieldset>\",\"</fieldset>\"],area:[1,\"<map>\",\"</map>\"],param:[1,\"<object>\",\"</object>\"],thead:[1,\"<table>\",\"</table>\"],tr:[2,\"<table><tbody>\",\"</tbody></table>\"],col:[2,\"<table><tbody></tbody><colgroup>\",\"</colgroup></table>\"],td:[3,\"<table><tbody><tr>\",\"</tr></tbody></table>\"],_default:b.support.htmlSerialize?[0,\"\",\"\"]:[1,\"X<div>\",\"</div>\"]},jt=dt(o),Dt=jt.appendChild(o.createElement(\"div\"));At.optgroup=At.option,At.tbody=At.tfoot=At.colgroup=At.caption=At.thead,At.th=At.td,b.fn.extend({text:function(e){return b.access(this,function(e){return e===t?b.text(this):this.empty().append((this[0]&&this[0].ownerDocument||o).createTextNode(e))},null,e,arguments.length)},wrapAll:function(e){if(b.isFunction(e))return this.each(function(t){b(this).wrapAll(e.call(this,t))});if(this[0]){var t=b(e,this[0].ownerDocument).eq(0).clone(!0);this[0].parentNode&&t.insertBefore(this[0]),t.map(function(){var e=this;while(e.firstChild&&1===e.firstChild.nodeType)e=e.firstChild;return e}).append(this)}return this},wrapInner:function(e){return b.isFunction(e)?this.each(function(t){b(this).wrapInner(e.call(this,t))}):this.each(function(){var t=b(this),n=t.contents();n.length?n.wrapAll(e):t.append(e)})},wrap:function(e){var t=b.isFunction(e);return this.each(function(n){b(this).wrapAll(t?e.call(this,n):e)})},unwrap:function(){return this.parent().each(function(){b.nodeName(this,\"body\")||b(this).replaceWith(this.childNodes)}).end()},append:function(){return this.domManip(arguments,!0,function(e){(1===this.nodeType||11===this.nodeType||9===this.nodeType)&&this.appendChild(e)})},prepend:function(){return this.domManip(arguments,!0,function(e){(1===this.nodeType||11===this.nodeType||9===this.nodeType)&&this.insertBefore(e,this.firstChild)})},before:function(){return this.domManip(arguments,!1,function(e){this.parentNode&&this.parentNode.insertBefore(e,this)})},after:function(){return this.domManip(arguments,!1,function(e){this.parentNode&&this.parentNode.insertBefore(e,this.nextSibling)})},remove:function(e,t){var n,r=0;for(;null!=(n=this[r]);r++)(!e||b.filter(e,[n]).length>0)&&(t||1!==n.nodeType||b.cleanData(Ot(n)),n.parentNode&&(t&&b.contains(n.ownerDocument,n)&&Mt(Ot(n,\"script\")),n.parentNode.removeChild(n)));return this},empty:function(){var e,t=0;for(;null!=(e=this[t]);t++){1===e.nodeType&&b.cleanData(Ot(e,!1));while(e.firstChild)e.removeChild(e.firstChild);e.options&&b.nodeName(e,\"select\")&&(e.options.length=0)}return this},clone:function(e,t){return e=null==e?!1:e,t=null==t?e:t,this.map(function(){return b.clone(this,e,t)})},html:function(e){return b.access(this,function(e){var n=this[0]||{},r=0,i=this.length;if(e===t)return 1===n.nodeType?n.innerHTML.replace(gt,\"\"):t;if(!(\"string\"!=typeof e||Tt.test(e)||!b.support.htmlSerialize&&mt.test(e)||!b.support.leadingWhitespace&&yt.test(e)||At[(bt.exec(e)||[\"\",\"\"])[1].toLowerCase()])){e=e.replace(vt,\"<$1></$2>\");try{for(;i>r;r++)n=this[r]||{},1===n.nodeType&&(b.cleanData(Ot(n,!1)),n.innerHTML=e);n=0}catch(o){}}n&&this.empty().append(e)},null,e,arguments.length)},replaceWith:function(e){var t=b.isFunction(e);return t||\"string\"==typeof e||(e=b(e).not(this).detach()),this.domManip([e],!0,function(e){var t=this.nextSibling,n=this.parentNode;n&&(b(this).remove(),n.insertBefore(e,t))})},detach:function(e){return this.remove(e,!0)},domManip:function(e,n,r){e=f.apply([],e);var i,o,a,s,u,l,c=0,p=this.length,d=this,h=p-1,g=e[0],m=b.isFunction(g);if(m||!(1>=p||\"string\"!=typeof g||b.support.checkClone)&&Ct.test(g))return this.each(function(i){var o=d.eq(i);m&&(e[0]=g.call(this,i,n?o.html():t)),o.domManip(e,n,r)});if(p&&(l=b.buildFragment(e,this[0].ownerDocument,!1,this),i=l.firstChild,1===l.childNodes.length&&(l=i),i)){for(n=n&&b.nodeName(i,\"tr\"),s=b.map(Ot(l,\"script\"),Ht),a=s.length;p>c;c++)o=l,c!==h&&(o=b.clone(o,!0,!0),a&&b.merge(s,Ot(o,\"script\"))),r.call(n&&b.nodeName(this[c],\"table\")?Lt(this[c],\"tbody\"):this[c],o,c);if(a)for(u=s[s.length-1].ownerDocument,b.map(s,qt),c=0;a>c;c++)o=s[c],kt.test(o.type||\"\")&&!b._data(o,\"globalEval\")&&b.contains(u,o)&&(o.src?b.ajax({url:o.src,type:\"GET\",dataType:\"script\",async:!1,global:!1,\"throws\":!0}):b.globalEval((o.text||o.textContent||o.innerHTML||\"\").replace(St,\"\")));l=i=null}return this}});function Lt(e,t){return e.getElementsByTagName(t)[0]||e.appendChild(e.ownerDocument.createElement(t))}function Ht(e){var t=e.getAttributeNode(\"type\");return e.type=(t&&t.specified)+\"/\"+e.type,e}function qt(e){var t=Et.exec(e.type);return t?e.type=t[1]:e.removeAttribute(\"type\"),e}function Mt(e,t){var n,r=0;for(;null!=(n=e[r]);r++)b._data(n,\"globalEval\",!t||b._data(t[r],\"globalEval\"))}function _t(e,t){if(1===t.nodeType&&b.hasData(e)){var n,r,i,o=b._data(e),a=b._data(t,o),s=o.events;if(s){delete a.handle,a.events={};for(n in s)for(r=0,i=s[n].length;i>r;r++)b.event.add(t,n,s[n][r])}a.data&&(a.data=b.extend({},a.data))}}function Ft(e,t){var n,r,i;if(1===t.nodeType){if(n=t.nodeName.toLowerCase(),!b.support.noCloneEvent&&t[b.expando]){i=b._data(t);for(r in i.events)b.removeEvent(t,r,i.handle);t.removeAttribute(b.expando)}\"script\"===n&&t.text!==e.text?(Ht(t).text=e.text,qt(t)):\"object\"===n?(t.parentNode&&(t.outerHTML=e.outerHTML),b.support.html5Clone&&e.innerHTML&&!b.trim(t.innerHTML)&&(t.innerHTML=e.innerHTML)):\"input\"===n&&Nt.test(e.type)?(t.defaultChecked=t.checked=e.checked,t.value!==e.value&&(t.value=e.value)):\"option\"===n?t.defaultSelected=t.selected=e.defaultSelected:(\"input\"===n||\"textarea\"===n)&&(t.defaultValue=e.defaultValue)}}b.each({appendTo:\"append\",prependTo:\"prepend\",insertBefore:\"before\",insertAfter:\"after\",replaceAll:\"replaceWith\"},function(e,t){b.fn[e]=function(e){var n,r=0,i=[],o=b(e),a=o.length-1;for(;a>=r;r++)n=r===a?this:this.clone(!0),b(o[r])[t](n),d.apply(i,n.get());return this.pushStack(i)}});function Ot(e,n){var r,o,a=0,s=typeof e.getElementsByTagName!==i?e.getElementsByTagName(n||\"*\"):typeof e.querySelectorAll!==i?e.querySelectorAll(n||\"*\"):t;if(!s)for(s=[],r=e.childNodes||e;null!=(o=r[a]);a++)!n||b.nodeName(o,n)?s.push(o):b.merge(s,Ot(o,n));return n===t||n&&b.nodeName(e,n)?b.merge([e],s):s}function Bt(e){Nt.test(e.type)&&(e.defaultChecked=e.checked)}b.extend({clone:function(e,t,n){var r,i,o,a,s,u=b.contains(e.ownerDocument,e);if(b.support.html5Clone||b.isXMLDoc(e)||!mt.test(\"<\"+e.nodeName+\">\")?o=e.cloneNode(!0):(Dt.innerHTML=e.outerHTML,Dt.removeChild(o=Dt.firstChild)),!(b.support.noCloneEvent&&b.support.noCloneChecked||1!==e.nodeType&&11!==e.nodeType||b.isXMLDoc(e)))for(r=Ot(o),s=Ot(e),a=0;null!=(i=s[a]);++a)r[a]&&Ft(i,r[a]);if(t)if(n)for(s=s||Ot(e),r=r||Ot(o),a=0;null!=(i=s[a]);a++)_t(i,r[a]);else _t(e,o);return r=Ot(o,\"script\"),r.length>0&&Mt(r,!u&&Ot(e,\"script\")),r=s=i=null,o},buildFragment:function(e,t,n,r){var i,o,a,s,u,l,c,p=e.length,f=dt(t),d=[],h=0;for(;p>h;h++)if(o=e[h],o||0===o)if(\"object\"===b.type(o))b.merge(d,o.nodeType?[o]:o);else if(wt.test(o)){s=s||f.appendChild(t.createElement(\"div\")),u=(bt.exec(o)||[\"\",\"\"])[1].toLowerCase(),c=At[u]||At._default,s.innerHTML=c[1]+o.replace(vt,\"<$1></$2>\")+c[2],i=c[0];while(i--)s=s.lastChild;if(!b.support.leadingWhitespace&&yt.test(o)&&d.push(t.createTextNode(yt.exec(o)[0])),!b.support.tbody){o=\"table\"!==u||xt.test(o)?\"<table>\"!==c[1]||xt.test(o)?0:s:s.firstChild,i=o&&o.childNodes.length;while(i--)b.nodeName(l=o.childNodes[i],\"tbody\")&&!l.childNodes.length&&o.removeChild(l)\n"
"}b.merge(d,s.childNodes),s.textContent=\"\";while(s.firstChild)s.removeChild(s.firstChild);s=f.lastChild}else d.push(t.createTextNode(o));s&&f.removeChild(s),b.support.appendChecked||b.grep(Ot(d,\"input\"),Bt),h=0;while(o=d[h++])if((!r||-1===b.inArray(o,r))&&(a=b.contains(o.ownerDocument,o),s=Ot(f.appendChild(o),\"script\"),a&&Mt(s),n)){i=0;while(o=s[i++])kt.test(o.type||\"\")&&n.push(o)}return s=null,f},cleanData:function(e,t){var n,r,o,a,s=0,u=b.expando,l=b.cache,p=b.support.deleteExpando,f=b.event.special;for(;null!=(n=e[s]);s++)if((t||b.acceptData(n))&&(o=n[u],a=o&&l[o])){if(a.events)for(r in a.events)f[r]?b.event.remove(n,r):b.removeEvent(n,r,a.handle);l[o]&&(delete l[o],p?delete n[u]:typeof n.removeAttribute!==i?n.removeAttribute(u):n[u]=null,c.push(o))}}});var Pt,Rt,Wt,$t=/alpha\\([^)]*\\)/i,It=/opacity\\s*=\\s*([^)]*)/,zt=/^(top|right|bottom|left)$/,Xt=/^(none|table(?!-c[ea]).+)/,Ut=/^margin/,Vt=RegExp(\"^(\"+x+\")(.*)$\",\"i\"),Yt=RegExp(\"^(\"+x+\")(?!px)[a-z%]+$\",\"i\"),Jt=RegExp(\"^([+-])=(\"+x+\")\",\"i\"),Gt={BODY:\"block\"},Qt={position:\"absolute\",visibility:\"hidden\",display:\"block\"},Kt={letterSpacing:0,fontWeight:400},Zt=[\"Top\",\"Right\",\"Bottom\",\"Left\"],en=[\"Webkit\",\"O\",\"Moz\",\"ms\"];function tn(e,t){if(t in e)return t;var n=t.charAt(0).toUpperCase()+t.slice(1),r=t,i=en.length;while(i--)if(t=en[i]+n,t in e)return t;return r}function nn(e,t){return e=t||e,\"none\"===b.css(e,\"display\")||!b.contains(e.ownerDocument,e)}function rn(e,t){var n,r,i,o=[],a=0,s=e.length;for(;s>a;a++)r=e[a],r.style&&(o[a]=b._data(r,\"olddisplay\"),n=r.style.display,t?(o[a]||\"none\"!==n||(r.style.display=\"\"),\"\"===r.style.display&&nn(r)&&(o[a]=b._data(r,\"olddisplay\",un(r.nodeName)))):o[a]||(i=nn(r),(n&&\"none\"!==n||!i)&&b._data(r,\"olddisplay\",i?n:b.css(r,\"display\"))));for(a=0;s>a;a++)r=e[a],r.style&&(t&&\"none\"!==r.style.display&&\"\"!==r.style.display||(r.style.display=t?o[a]||\"\":\"none\"));return e}b.fn.extend({css:function(e,n){return b.access(this,function(e,n,r){var i,o,a={},s=0;if(b.isArray(n)){for(o=Rt(e),i=n.length;i>s;s++)a[n[s]]=b.css(e,n[s],!1,o);return a}return r!==t?b.style(e,n,r):b.css(e,n)},e,n,arguments.length>1)},show:function(){return rn(this,!0)},hide:function(){return rn(this)},toggle:function(e){var t=\"boolean\"==typeof e;return this.each(function(){(t?e:nn(this))?b(this).show():b(this).hide()})}}),b.extend({cssHooks:{opacity:{get:function(e,t){if(t){var n=Wt(e,\"opacity\");return\"\"===n?\"1\":n}}}},cssNumber:{columnCount:!0,fillOpacity:!0,fontWeight:!0,lineHeight:!0,opacity:!0,orphans:!0,widows:!0,zIndex:!0,zoom:!0},cssProps:{\"float\":b.support.cssFloat?\"cssFloat\":\"styleFloat\"},style:function(e,n,r,i){if(e&&3!==e.nodeType&&8!==e.nodeType&&e.style){var o,a,s,u=b.camelCase(n),l=e.style;if(n=b.cssProps[u]||(b.cssProps[u]=tn(l,u)),s=b.cssHooks[n]||b.cssHooks[u],r===t)return s&&\"get\"in s&&(o=s.get(e,!1,i))!==t?o:l[n];if(a=typeof r,\"string\"===a&&(o=Jt.exec(r))&&(r=(o[1]+1)*o[2]+parseFloat(b.css(e,n)),a=\"number\"),!(null==r||\"number\"===a&&isNaN(r)||(\"number\"!==a||b.cssNumber[u]||(r+=\"px\"),b.support.clearCloneStyle||\"\"!==r||0!==n.indexOf(\"background\")||(l[n]=\"inherit\"),s&&\"set\"in s&&(r=s.set(e,r,i))===t)))try{l[n]=r}catch(c){}}},css:function(e,n,r,i){var o,a,s,u=b.camelCase(n);return n=b.cssProps[u]||(b.cssProps[u]=tn(e.style,u)),s=b.cssHooks[n]||b.cssHooks[u],s&&\"get\"in s&&(a=s.get(e,!0,r)),a===t&&(a=Wt(e,n,i)),\"normal\"===a&&n in Kt&&(a=Kt[n]),\"\"===r||r?(o=parseFloat(a),r===!0||b.isNumeric(o)?o||0:a):a},swap:function(e,t,n,r){var i,o,a={};for(o in t)a[o]=e.style[o],e.style[o]=t[o];i=n.apply(e,r||[]);for(o in t)e.style[o]=a[o];return i}}),e.getComputedStyle?(Rt=function(t){return e.getComputedStyle(t,null)},Wt=function(e,n,r){var i,o,a,s=r||Rt(e),u=s?s.getPropertyValue(n)||s[n]:t,l=e.style;return s&&(\"\"!==u||b.contains(e.ownerDocument,e)||(u=b.style(e,n)),Yt.test(u)&&Ut.test(n)&&(i=l.width,o=l.minWidth,a=l.maxWidth,l.minWidth=l.maxWidth=l.width=u,u=s.width,l.width=i,l.minWidth=o,l.maxWidth=a)),u}):o.documentElement.currentStyle&&(Rt=function(e){return e.currentStyle},Wt=function(e,n,r){var i,o,a,s=r||Rt(e),u=s?s[n]:t,l=e.style;return null==u&&l&&l[n]&&(u=l[n]),Yt.test(u)&&!zt.test(n)&&(i=l.left,o=e.runtimeStyle,a=o&&o.left,a&&(o.left=e.currentStyle.left),l.left=\"fontSize\"===n?\"1em\":u,u=l.pixelLeft+\"px\",l.left=i,a&&(o.left=a)),\"\"===u?\"auto\":u});function on(e,t,n){var r=Vt.exec(t);return r?Math.max(0,r[1]-(n||0))+(r[2]||\"px\"):t}function an(e,t,n,r,i){var o=n===(r?\"border\":\"content\")?4:\"width\"===t?1:0,a=0;for(;4>o;o+=2)\"margin\"===n&&(a+=b.css(e,n+Zt[o],!0,i)),r?(\"content\"===n&&(a-=b.css(e,\"padding\"+Zt[o],!0,i)),\"margin\"!==n&&(a-=b.css(e,\"border\"+Zt[o]+\"Width\",!0,i))):(a+=b.css(e,\"padding\"+Zt[o],!0,i),\"padding\"!==n&&(a+=b.css(e,\"border\"+Zt[o]+\"Width\",!0,i)));return a}function sn(e,t,n){var r=!0,i=\"width\"===t?e.offsetWidth:e.offsetHeight,o=Rt(e),a=b.support.boxSizing&&\"border-box\"===b.css(e,\"boxSizing\",!1,o);if(0>=i||null==i){if(i=Wt(e,t,o),(0>i||null==i)&&(i=e.style[t]),Yt.test(i))return i;r=a&&(b.support.boxSizingReliable||i===e.style[t]),i=parseFloat(i)||0}return i+an(e,t,n||(a?\"border\":\"content\"),r,o)+\"px\"}function un(e){var t=o,n=Gt[e];return n||(n=ln(e,t),\"none\"!==n&&n||(Pt=(Pt||b(\"<iframe frameborder='0' width='0' height='0'/>\").css(\"cssText\",\"display:block !important\")).appendTo(t.documentElement),t=(Pt[0].contentWindow||Pt[0].contentDocument).document,t.write(\"<!doctype html><html><body>\"),t.close(),n=ln(e,t),Pt.detach()),Gt[e]=n),n}function ln(e,t){var n=b(t.createElement(e)).appendTo(t.body),r=b.css(n[0],\"display\");return n.remove(),r}b.each([\"height\",\"width\"],function(e,n){b.cssHooks[n]={get:function(e,r,i){return r?0===e.offsetWidth&&Xt.test(b.css(e,\"display\"))?b.swap(e,Qt,function(){return sn(e,n,i)}):sn(e,n,i):t},set:function(e,t,r){var i=r&&Rt(e);return on(e,t,r?an(e,n,r,b.support.boxSizing&&\"border-box\"===b.css(e,\"boxSizing\",!1,i),i):0)}}}),b.support.opacity||(b.cssHooks.opacity={get:function(e,t){return It.test((t&&e.currentStyle?e.currentStyle.filter:e.style.filter)||\"\")?.01*parseFloat(RegExp.$1)+\"\":t?\"1\":\"\"},set:function(e,t){var n=e.style,r=e.currentStyle,i=b.isNumeric(t)?\"alpha(opacity=\"+100*t+\")\":\"\",o=r&&r.filter||n.filter||\"\";n.zoom=1,(t>=1||\"\"===t)&&\"\"===b.trim(o.replace($t,\"\"))&&n.removeAttribute&&(n.removeAttribute(\"filter\"),\"\"===t||r&&!r.filter)||(n.filter=$t.test(o)?o.replace($t,i):o+\" \"+i)}}),b(function(){b.support.reliableMarginRight||(b.cssHooks.marginRight={get:function(e,n){return n?b.swap(e,{display:\"inline-block\"},Wt,[e,\"marginRight\"]):t}}),!b.support.pixelPosition&&b.fn.position&&b.each([\"top\",\"left\"],function(e,n){b.cssHooks[n]={get:function(e,r){return r?(r=Wt(e,n),Yt.test(r)?b(e).position()[n]+\"px\":r):t}}})}),b.expr&&b.expr.filters&&(b.expr.filters.hidden=function(e){return 0>=e.offsetWidth&&0>=e.offsetHeight||!b.support.reliableHiddenOffsets&&\"none\"===(e.style&&e.style.display||b.css(e,\"display\"))},b.expr.filters.visible=function(e){return!b.expr.filters.hidden(e)}),b.each({margin:\"\",padding:\"\",border:\"Width\"},function(e,t){b.cssHooks[e+t]={expand:function(n){var r=0,i={},o=\"string\"==typeof n?n.split(\" \"):[n];for(;4>r;r++)i[e+Zt[r]+t]=o[r]||o[r-2]||o[0];return i}},Ut.test(e)||(b.cssHooks[e+t].set=on)});var cn=/%20/g,pn=/\\[\\]$/,fn=/\\r?\\n/g,dn=/^(?:submit|button|image|reset|file)$/i,hn=/^(?:input|select|textarea|keygen)/i;b.fn.extend({serialize:function(){return b.param(this.serializeArray())},serializeArray:function(){return this.map(function(){var e=b.prop(this,\"elements\");return e?b.makeArray(e):this}).filter(function(){var e=this.type;return this.name&&!b(this).is(\":disabled\")&&hn.test(this.nodeName)&&!dn.test(e)&&(this.checked||!Nt.test(e))}).map(function(e,t){var n=b(this).val();return null==n?null:b.isArray(n)?b.map(n,function(e){return{name:t.name,value:e.replace(fn,\"\\r\\n\")}}):{name:t.name,value:n.replace(fn,\"\\r\\n\")}}).get()}}),b.param=function(e,n){var r,i=[],o=function(e,t){t=b.isFunction(t)?t():null==t?\"\":t,i[i.length]=encodeURIComponent(e)+\"=\"+encodeURIComponent(t)};if(n===t&&(n=b.ajaxSettings&&b.ajaxSettings.traditional),b.isArray(e)||e.jquery&&!b.isPlainObject(e))b.each(e,function(){o(this.name,this.value)});else for(r in e)gn(r,e[r],n,o);return i.join(\"&\").replace(cn,\"+\")};function gn(e,t,n,r){var i;if(b.isArray(t))b.each(t,function(t,i){n||pn.test(e)?r(e,i):gn(e+\"[\"+(\"object\"==typeof i?t:\"\")+\"]\",i,n,r)});else if(n||\"object\"!==b.type(t))r(e,t);else for(i in t)gn(e+\"[\"+i+\"]\",t[i],n,r)}b.each(\"blur focus focusin focusout load resize scroll unload click dblclick mousedown mouseup mousemove mouseover mouseout mouseenter mouseleave change select submit keydown keypress keyup error contextmenu\".split(\" \"),function(e,t){b.fn[t]=function(e,n){return arguments.length>0?this.on(t,null,e,n):this.trigger(t)}}),b.fn.hover=function(e,t){return this.mouseenter(e).mouseleave(t||e)};var mn,yn,vn=b.now(),bn=/\\?/,xn=/#.*$/,wn=/([?&])_=[^&]*/,Tn=/^(.*?):[ \\t]*([^\\r\\n]*)\\r?$/gm,Nn=/^(?:about|app|app-storage|.+-extension|file|res|widget):$/,Cn=/^(?:GET|HEAD)$/,kn=/^\\/\\//,En=/^([\\w.+-]+:)(?:\\/\\/([^\\/?#:]*)(?::(\\d+)|)|)/,Sn=b.fn.load,An={},jn={},Dn=\"*/\".concat(\"*\");try{yn=a.href}catch(Ln){yn=o.createElement(\"a\"),yn.href=\"\",yn=yn.href}mn=En.exec(yn.toLowerCase())||[];function Hn(e){return function(t,n){\"string\"!=typeof t&&(n=t,t=\"*\");var r,i=0,o=t.toLowerCase().match(w)||[];if(b.isFunction(n))while(r=o[i++])\"+\"===r[0]?(r=r.slice(1)||\"*\",(e[r]=e[r]||[]).unshift(n)):(e[r]=e[r]||[]).push(n)}}function qn(e,n,r,i){var o={},a=e===jn;function s(u){var l;return o[u]=!0,b.each(e[u]||[],function(e,u){var c=u(n,r,i);return\"string\"!=typeof c||a||o[c]?a?!(l=c):t:(n.dataTypes.unshift(c),s(c),!1)}),l}return s(n.dataTypes[0])||!o[\"*\"]&&s(\"*\")}function Mn(e,n){var r,i,o=b.ajaxSettings.flatOptions||{};for(i in n)n[i]!==t&&((o[i]?e:r||(r={}))[i]=n[i]);return r&&b.extend(!0,e,r),e}b.fn.load=function(e,n,r){if(\"string\"!=typeof e&&Sn)return Sn.apply(this,arguments);var i,o,a,s=this,u=e.indexOf(\" \");return u>=0&&(i=e.slice(u,e.length),e=e.slice(0,u)),b.isFunction(n)?(r=n,n=t):n&&\"object\"==typeof n&&(a=\"POST\"),s.length>0&&b.ajax({url:e,type:a,dataType:\"html\",data:n}).done(function(e){o=arguments,s.html(i?b(\"<div>\").append(b.parseHTML(e)).find(i):e)}).complete(r&&function(e,t){s.each(r,o||[e.responseText,t,e])}),this},b.each([\"ajaxStart\",\"ajaxStop\",\"ajaxComplete\",\"ajaxError\",\"ajaxSuccess\",\"ajaxSend\"],function(e,t){b.fn[t]=function(e){return this.on(t,e)}}),b.each([\"get\",\"post\"],function(e,n){b[n]=function(e,r,i,o){return b.isFunction(r)&&(o=o||i,i=r,r=t),b.ajax({url:e,type:n,dataType:o,data:r,success:i})}}),b.extend({active:0,lastModified:{},etag:{},ajaxSettings:{url:yn,type:\"GET\",isLocal:Nn.test(mn[1]),global:!0,processData:!0,async:!0,contentType:\"application/x-www-form-urlencoded; charset=UTF-8\",accepts:{\"*\":Dn,text:\"text/plain\",html:\"text/html\",xml:\"application/xml, text/xml\",json:\"application/json, text/javascript\"},contents:{xml:/xml/,html:/html/,json:/json/},responseFields:{xml:\"responseXML\",text:\"responseText\"},converters:{\"* text\":e.String,\"text html\":!0,\"text json\":b.parseJSON,\"text xml\":b.parseXML},flatOptions:{url:!0,context:!0}},ajaxSetup:function(e,t){return t?Mn(Mn(e,b.ajaxSettings),t):Mn(b.ajaxSettings,e)},ajaxPrefilter:Hn(An),ajaxTransport:Hn(jn),ajax:function(e,n){\"object\"==typeof e&&(n=e,e=t),n=n||{};var r,i,o,a,s,u,l,c,p=b.ajaxSetup({},n),f=p.context||p,d=p.context&&(f.nodeType||f.jquery)?b(f):b.event,h=b.Deferred(),g=b.Callbacks(\"once memory\"),m=p.statusCode||{},y={},v={},x=0,T=\"canceled\",N={readyState:0,getResponseHeader:function(e){var t;if(2===x){if(!c){c={};while(t=Tn.exec(a))c[t[1].toLowerCase()]=t[2]}t=c[e.toLowerCase()]}return null==t?null:t},getAllResponseHeaders:function(){return 2===x?a:null},setRequestHeader:function(e,t){var n=e.toLowerCase();return x||(e=v[n]=v[n]||e,y[e]=t),this},overrideMimeType:function(e){return x||(p.mimeType=e),this},statusCode:function(e){var t;if(e)if(2>x)for(t in e)m[t]=[m[t],e[t]];else N.always(e[N.status]);return this},abort:function(e){var t=e||T;return l&&l.abort(t),k(0,t),this}};if(h.promise(N).complete=g.add,N.success=N.done,N.error=N.fail,p.url=((e||p.url||yn)+\"\").replace(xn,\"\").replace(kn,mn[1]+\"//\"),p.type=n.method||n.type||p.method||p.type,p.dataTypes=b.trim(p.dataType||\"*\").toLowerCase().match(w)||[\"\"],null==p.crossDomain&&(r=En.exec(p.url.toLowerCase()),p.crossDomain=!(!r||r[1]===mn[1]&&r[2]===mn[2]&&(r[3]||(\"http:\"===r[1]?80:443))==(mn[3]||(\"http:\"===mn[1]?80:443)))),p.data&&p.processData&&\"string\"!=typeof p.data&&(p.data=b.param(p.data,p.traditional)),qn(An,p,n,N),2===x)return N;u=p.global,u&&0===b.active++&&b.event.trigger(\"ajaxStart\"),p.type=p.type.toUpperCase(),p.hasContent=!Cn.test(p.type),o=p.url,p.hasContent||(p.data&&(o=p.url+=(bn.test(o)?\"&\":\"?\")+p.data,delete p.data),p.cache===!1&&(p.url=wn.test(o)?o.replace(wn,\"$1_=\"+vn++):o+(bn.test(o)?\"&\":\"?\")+\"_=\"+vn++)),p.ifModified&&(b.lastModified[o]&&N.setRequestHeader(\"If-Modified-Since\",b.lastModified[o]),b.etag[o]&&N.setRequestHeader(\"If-None-Match\",b.etag[o])),(p.data&&p.hasContent&&p.contentType!==!1||n.contentType)&&N.setRequestHeader(\"Content-Type\",p.contentType),N.setRequestHeader(\"Accept\",p.dataTypes[0]&&p.accepts[p.dataTypes[0]]?p.accepts[p.dataTypes[0]]+(\"*\"!==p.dataTypes[0]?\", \"+Dn+\"; q=0.01\":\"\"):p.accepts[\"*\"]);for(i in p.headers)N.setRequestHeader(i,p.headers[i]);if(p.beforeSend&&(p.beforeSend.call(f,N,p)===!1||2===x))return N.abort();T=\"abort\";for(i in{success:1,error:1,complete:1})N[i](p[i]);if(l=qn(jn,p,n,N)){N.readyState=1,u&&d.trigger(\"ajaxSend\",[N,p]),p.async&&p.timeout>0&&(s=setTimeout(function(){N.abort(\"timeout\")},p.timeout));try{x=1,l.send(y,k)}catch(C){if(!(2>x))throw C;k(-1,C)}}else k(-1,\"No Transport\");function k(e,n,r,i){var c,y,v,w,T,C=n;2!==x&&(x=2,s&&clearTimeout(s),l=t,a=i||\"\",N.readyState=e>0?4:0,r&&(w=_n(p,N,r)),e>=200&&300>e||304===e?(p.ifModified&&(T=N.getResponseHeader(\"Last-Modified\"),T&&(b.lastModified[o]=T),T=N.getResponseHeader(\"etag\"),T&&(b.etag[o]=T)),204===e?(c=!0,C=\"nocontent\"):304===e?(c=!0,C=\"notmodified\"):(c=Fn(p,w),C=c.state,y=c.data,v=c.error,c=!v)):(v=C,(e||!C)&&(C=\"error\",0>e&&(e=0))),N.status=e,N.statusText=(n||C)+\"\",c?h.resolveWith(f,[y,C,N]):h.rejectWith(f,[N,C,v]),N.statusCode(m),m=t,u&&d.trigger(c?\"ajaxSuccess\":\"ajaxError\",[N,p,c?y:v]),g.fireWith(f,[N,C]),u&&(d.trigger(\"ajaxComplete\",[N,p]),--b.active||b.event.trigger(\"ajaxStop\")))}return N},getScript:function(e,n){return b.get(e,t,n,\"script\")},getJSON:function(e,t,n){return b.get(e,t,n,\"json\")}});function _n(e,n,r){var i,o,a,s,u=e.contents,l=e.dataTypes,c=e.responseFields;for(s in c)s in r&&(n[c[s]]=r[s]);while(\"*\"===l[0])l.shift(),o===t&&(o=e.mimeType||n.getResponseHeader(\"Content-Type\"));if(o)for(s in u)if(u[s]&&u[s].test(o)){l.unshift(s);break}if(l[0]in r)a=l[0];else{for(s in r){if(!l[0]||e.converters[s+\" \"+l[0]]){a=s;break}i||(i=s)}a=a||i}return a?(a!==l[0]&&l.unshift(a),r[a]):t}function Fn(e,t){var n,r,i,o,a={},s=0,u=e.dataTypes.slice(),l=u[0];if(e.dataFilter&&(t=e.dataFilter(t,e.dataType)),u[1])for(i in e.converters)a[i.toLowerCase()]=e.converters[i];for(;r=u[++s];)if(\"*\"!==r){if(\"*\"!==l&&l!==r){if(i=a[l+\" \"+r]||a[\"* \"+r],!i)for(n in a)if(o=n.split(\" \"),o[1]===r&&(i=a[l+\" \"+o[0]]||a[\"* \"+o[0]])){i===!0?i=a[n]:a[n]!==!0&&(r=o[0],u.splice(s--,0,r));break}if(i!==!0)if(i&&e[\"throws\"])t=i(t);else try{t=i(t)}catch(c){return{state:\"parsererror\",error:i?c:\"No conversion from \"+l+\" to \"+r}}}l=r}return{state:\"success\",data:t}}b.ajaxSetup({accepts:{script:\"text/javascript, application/javascript, application/ecmascript, application/x-ecmascript\"},contents:{script:/(?:java|ecma)script/},converters:{\"text script\":function(e){return b.globalEval(e),e}}}),b.ajaxPrefilter(\"script\",function(e){e.cache===t&&(e.cache=!1),e.crossDomain&&(e.type=\"GET\",e.global=!1)}),b.ajaxTransport(\"script\",function(e){if(e.crossDomain){var n,r=o.head||b(\"head\")[0]||o.documentElement;return{send:function(t,i){n=o.createElement(\"script\"),n.async=!0,e.scriptCharset&&(n.charset=e.scriptCharset),n.src=e.url,n.onload=n.onreadystatechange=function(e,t){(t||!n.readyState||/loaded|complete/.test(n.readyState))&&(n.onload=n.onreadystatechange=null,n.parentNode&&n.parentNode.removeChild(n),n=null,t||i(200,\"success\"))},r.insertBefore(n,r.firstChild)},abort:function(){n&&n.onload(t,!0)}}}});var On=[],Bn=/(=)\\?(?=&|$)|\\?\\?/;b.ajaxSetup({jsonp:\"callback\",jsonpCallback:function(){var e=On.pop()||b.expando+\"_\"+vn++;return this[e]=!0,e}}),b.ajaxPrefilter(\"json jsonp\",function(n,r,i){var o,a,s,u=n.jsonp!==!1&&(Bn.test(n.url)?\"url\":\"string\"==typeof n.data&&!(n.contentType||\"\").indexOf(\"application/x-www-form-urlencoded\")&&Bn.test(n.data)&&\"data\");return u||\"jsonp\"===n.dataTypes[0]?(o=n.jsonpCallback=b.isFunction(n.jsonpCallback)?n.jsonpCallback():n.jsonpCallback,u?n[u]=n[u].replace(Bn,\"$1\"+o):n.jsonp!==!1&&(n.url+=(bn.test(n.url)?\"&\":\"?\")+n.jsonp+\"=\"+o),n.converters[\"script json\"]=function(){return s||b.error(o+\" was not called\"),s[0]},n.dataTypes[0]=\"json\",a=e[o],e[o]=function(){s=arguments},i.always(function(){e[o]=a,n[o]&&(n.jsonpCallback=r.jsonpCallback,On.push(o)),s&&b.isFunction(a)&&a(s[0]),s=a=t}),\"script\"):t});var Pn,Rn,Wn=0,$n=e.ActiveXObject&&function(){var e;for(e in Pn)Pn[e](t,!0)};function In(){try{return new e.XMLHttpRequest}catch(t){}}function zn(){try{return new e.ActiveXObject(\"Microsoft.XMLHTTP\")}catch(t){}}b.ajaxSettings.xhr=e.ActiveXObject?function(){return!this.isLocal&&In()||zn()}:In,Rn=b.ajaxSettings.xhr(),b.support.cors=!!Rn&&\"withCredentials\"in Rn,Rn=b.support.ajax=!!Rn,Rn&&b.ajaxTransport(function(n){if(!n.crossDomain||b.support.cors){var r;return{send:function(i,o){var a,s,u=n.xhr();if(n.username?u.open(n.type,n.url,n.async,n.username,n.password):u.open(n.type,n.url,n.async),n.xhrFields)for(s in n.xhrFields)u[s]=n.xhrFields[s];n.mimeType&&u.overrideMimeType&&u.overrideMimeType(n.mimeType),n.crossDomain||i[\"X-Requested-With\"]||(i[\"X-Requested-With\"]=\"XMLHttpRequest\");try{for(s in i)u.setRequestHeader(s,i[s])}catch(l){}u.send(n.hasContent&&n.data||null),r=function(e,i){var s,l,c,p;try{if(r&&(i||4===u.readyState))if(r=t,a&&(u.onreadystatechange=b.noop,$n&&delete Pn[a]),i)4!==u.readyState&&u.abort();else{p={},s=u.status,l=u.getAllResponseHeaders(),\"string\"==typeof u.responseText&&(p.text=u.responseText);try{c=u.statusText}catch(f){c=\"\"}s||!n.isLocal||n.crossDomain?1223===s&&(s=204):s=p.text?200:404}}catch(d){i||o(-1,d)}p&&o(s,c,p,l)},n.async?4===u.readyState?setTimeout(r):(a=++Wn,$n&&(Pn||(Pn={},b(e).unload($n)),Pn[a]=r),u.onreadystatechange=r):r()},abort:function(){r&&r(t,!0)}}}});var Xn,Un,Vn=/^(?:toggle|show|hide)$/,Yn=RegExp(\"^(?:([+-])=|)(\"+x+\")([a-z%]*)$\",\"i\"),Jn=/queueHooks$/,Gn=[nr],Qn={\"*\":[function(e,t){var n,r,i=this.createTween(e,t),o=Yn.exec(t),a=i.cur(),s=+a||0,u=1,l=20;if(o){if(n=+o[2],r=o[3]||(b.cssNumber[e]?\"\":\"px\"),\"px\"!==r&&s){s=b.css(i.elem,e,!0)||n||1;do u=u||\".5\",s/=u,b.style(i.elem,e,s+r);while(u!==(u=i.cur()/a)&&1!==u&&--l)}i.unit=r,i.start=s,i.end=o[1]?s+(o[1]+1)*n:n}return i}]};function Kn(){return setTimeout(function(){Xn=t}),Xn=b.now()}function Zn(e,t){b.each(t,function(t,n){var r=(Qn[t]||[]).concat(Qn[\"*\"]),i=0,o=r.length;for(;o>i;i++)if(r[i].call(e,t,n))return})}function er(e,t,n){var r,i,o=0,a=Gn.length,s=b.Deferred().always(function(){delete u.elem}),u=function(){if(i)return!1;var t=Xn||Kn(),n=Math.max(0,l.startTime+l.duration-t),r=n/l.duration||0,o=1-r,a=0,u=l.tweens.length;for(;u>a;a++)l.tweens[a].run(o);return s.notifyWith(e,[l,o,n]),1>o&&u?n:(s.resolveWith(e,[l]),!1)},l=s.promise({elem:e,props:b.extend({},t),opts:b.extend(!0,{specialEasing:{}},n),originalProperties:t,originalOptions:n,startTime:Xn||Kn(),duration:n.duration,tweens:[],createTween:function(t,n){var r=b.Tween(e,l.opts,t,n,l.opts.specialEasing[t]||l.opts.easing);return l.tweens.push(r),r},stop:function(t){var n=0,r=t?l.tweens.length:0;if(i)return this;for(i=!0;r>n;n++)l.tweens[n].run(1);return t?s.resolveWith(e,[l,t]):s.rejectWith(e,[l,t]),this}}),c=l.props;for(tr(c,l.opts.specialEasing);a>o;o++)if(r=Gn[o].call(l,e,c,l.opts))return r;return Zn(l,c),b.isFunction(l.opts.start)&&l.opts.start.call(e,l),b.fx.timer(b.extend(u,{elem:e,anim:l,queue:l.opts.queue})),l.progress(l.opts.progress).done(l.opts.done,l.opts.complete).fail(l.opts.fail).always(l.opts.always)}function tr(e,t){var n,r,i,o,a;for(i in e)if(r=b.camelCase(i),o=t[r],n=e[i],b.isArray(n)&&(o=n[1],n=e[i]=n[0]),i!==r&&(e[r]=n,delete e[i]),a=b.cssHooks[r],a&&\"expand\"in a){n=a.expand(n),delete e[r];for(i in n)i in e||(e[i]=n[i],t[i]=o)}else t[r]=o}b.Animation=b.extend(er,{tweener:function(e,t){b.isFunction(e)?(t=e,e=[\"*\"]):e=e.split(\" \");var n,r=0,i=e.length;for(;i>r;r++)n=e[r],Qn[n]=Qn[n]||[],Qn[n].unshift(t)},prefilter:function(e,t){t?Gn.unshift(e):Gn.push(e)}});function nr(e,t,n){var r,i,o,a,s,u,l,c,p,f=this,d=e.style,h={},g=[],m=e.nodeType&&nn(e);n.queue||(c=b._queueHooks(e,\"fx\"),null==c.unqueued&&(c.unqueued=0,p=c.empty.fire,c.empty.fire=function(){c.unqueued||p()}),c.unqueued++,f.always(function(){f.always(function(){c.unqueued--,b.queue(e,\"fx\").length||c.empty.fire()})})),1===e.nodeType&&(\"height\"in t||\"width\"in t)&&(n.overflow=[d.overflow,d.overflowX,d.overflowY],\"inline\"===b.css(e,\"display\")&&\"none\"===b.css(e,\"float\")&&(b.support.inlineBlockNeedsLayout&&\"inline\"!==un(e.nodeName)?d.zoom=1:d.display=\"inline-block\")),n.overflow&&(d.overflow=\"hidden\",b.support.shrinkWrapBlocks||f.always(function(){d.overflow=n.overflow[0],d.overflowX=n.overflow[1],d.overflowY=n.overflow[2]}));for(i in t)if(a=t[i],Vn.exec(a)){if(delete t[i],u=u||\"toggle\"===a,a===(m?\"hide\":\"show\"))continue;g.push(i)}if(o=g.length){s=b._data(e,\"fxshow\")||b._data(e,\"fxshow\",{}),\"hidden\"in s&&(m=s.hidden),u&&(s.hidden=!m),m?b(e).show():f.done(function(){b(e).hide()}),f.done(function(){var t;b._removeData(e,\"fxshow\");for(t in h)b.style(e,t,h[t])});for(i=0;o>i;i++)r=g[i],l=f.createTween(r,m?s[r]:0),h[r]=s[r]||b.style(e,r),r in s||(s[r]=l.start,m&&(l.end=l.start,l.start=\"width\"===r||\"height\"===r?1:0))}}function rr(e,t,n,r,i){return new rr.prototype.init(e,t,n,r,i)}b.Tween=rr,rr.prototype={constructor:rr,init:function(e,t,n,r,i,o){this.elem=e,this.prop=n,this.easing=i||\"swing\",this.options=t,this.start=this.now=this.cur(),this.end=r,this.unit=o||(b.cssNumber[n]?\"\":\"px\")},cur:function(){var e=rr.propHooks[this.prop];return e&&e.get?e.get(this):rr.propHooks._default.get(this)},run:function(e){var t,n=rr.propHooks[this.prop];return this.pos=t=this.options.duration?b.easing[this.easing](e,this.options.duration*e,0,1,this.options.duration):e,this.now=(this.end-this.start)*t+this.start,this.options.step&&this.options.step.call(this.elem,this.now,this),n&&n.set?n.set(this):rr.propHooks._default.set(this),this}},rr.prototype.init.prototype=rr.prototype,rr.propHooks={_default:{get:function(e){var t;return null==e.elem[e.prop]||e.elem.style&&null!=e.elem.style[e.prop]?(t=b.css(e.elem,e.prop,\"\"),t&&\"auto\"!==t?t:0):e.elem[e.prop]},set:function(e){b.fx.step[e.prop]?b.fx.step[e.prop](e):e.elem.style&&(null!=e.elem.style[b.cssProps[e.prop]]||b.cssHooks[e.prop])?b.style(e.elem,e.prop,e.now+e.unit):e.elem[e.prop]=e.now}}},rr.propHooks.scrollTop=rr.propHooks.scrollLeft={set:function(e){e.elem.nodeType&&e.elem.parentNode&&(e.elem[e.prop]=e.now)}},b.each([\"toggle\",\"show\",\"hide\"],function(e,t){var n=b.fn[t];b.fn[t]=function(e,r,i){return null==e||\"boolean\"==typeof e?n.apply(this,arguments):this.animate(ir(t,!0),e,r,i)}}),b.fn.extend({fadeTo:function(e,t,n,r){return this.filter(nn).css(\"opacity\",0).show().end().animate({opacity:t},e,n,r)},animate:function(e,t,n,r){var i=b.isEmptyObject(e),o=b.speed(t,n,r),a=function(){var t=er(this,b.extend({},e),o);a.finish=function(){t.stop(!0)},(i||b._data(this,\"finish\"))&&t.stop(!0)};return a.finish=a,i||o.queue===!1?this.each(a):this.queue(o.queue,a)},stop:function(e,n,r){var i=function(e){var t=e.stop;delete e.stop,t(r)};return\"string\"!=typeof e&&(r=n,n=e,e=t),n&&e!==!1&&this.queue(e||\"fx\",[]),this.each(function(){var t=!0,n=null!=e&&e+\"queueHooks\",o=b.timers,a=b._data(this);if(n)a[n]&&a[n].stop&&i(a[n]);else for(n in a)a[n]&&a[n].stop&&Jn.test(n)&&i(a[n]);for(n=o.length;n--;)o[n].elem!==this||null!=e&&o[n].queue!==e||(o[n].anim.stop(r),t=!1,o.splice(n,1));(t||!r)&&b.dequeue(this,e)})},finish:function(e){return e!==!1&&(e=e||\"fx\"),this.each(function(){var t,n=b._data(this),r=n[e+\"queue\"],i=n[e+\"queueHooks\"],o=b.timers,a=r?r.length:0;for(n.finish=!0,b.queue(this,e,[]),i&&i.cur&&i.cur.finish&&i.cur.finish.call(this),t=o.length;t--;)o[t].elem===this&&o[t].queue===e&&(o[t].anim.stop(!0),o.splice(t,1));for(t=0;a>t;t++)r[t]&&r[t].finish&&r[t].finish.call(this);delete n.finish})}});function ir(e,t){var n,r={height:e},i=0;for(t=t?1:0;4>i;i+=2-t)n=Zt[i],r[\"margin\"+n]=r[\"padding\"+n]=e;return t&&(r.opacity=r.width=e),r}b.each({slideDown:ir(\"show\"),slideUp:ir(\"hide\"),slideToggle:ir(\"toggle\"),fadeIn:{opacity:\"show\"},fadeOut:{opacity:\"hide\"},fadeToggle:{opacity:\"toggle\"}},function(e,t){b.fn[e]=function(e,n,r){return this.animate(t,e,n,r)}}),b.speed=function(e,t,n){var r=e&&\"object\"==typeof e?b.extend({},e):{complete:n||!n&&t||b.isFunction(e)&&e,duration:e,easing:n&&t||t&&!b.isFunction(t)&&t};return r.duration=b.fx.off?0:\"number\"==typeof r.duration?r.duration:r.duration in b.fx.speeds?b.fx.speeds[r.duration]:b.fx.speeds._default,(null==r.queue||r.queue===!0)&&(r.queue=\"fx\"),r.old=r.complete,r.complete=function(){b.isFunction(r.old)&&r.old.call(this),r.queue&&b.dequeue(this,r.queue)},r},b.easing={linear:function(e){return e},swing:function(e){return.5-Math.cos(e*Math.PI)/2}},b.timers=[],b.fx=rr.prototype.init,b.fx.tick=function(){var e,n=b.timers,r=0;for(Xn=b.now();n.length>r;r++)e=n[r],e()||n[r]!==e||n.splice(r--,1);n.length||b.fx.stop(),Xn=t},b.fx.timer=function(e){e()&&b.timers.push(e)&&b.fx.start()},b.fx.interval=13,b.fx.start=function(){Un||(Un=setInterval(b.fx.tick,b.fx.interval))},b.fx.stop=function(){clearInterval(Un),Un=null},b.fx.speeds={slow:600,fast:200,_default:400},b.fx.step={},b.expr&&b.expr.filters&&(b.expr.filters.animated=function(e){return b.grep(b.timers,function(t){return e===t.elem}).length}),b.fn.offset=function(e){if(arguments.length)return e===t?this:this.each(function(t){b.offset.setOffset(this,e,t)});var n,r,o={top:0,left:0},a=this[0],s=a&&a.ownerDocument;if(s)return n=s.documentElement,b.contains(n,a)?(typeof a.getBoundingClientRect!==i&&(o=a.getBoundingClientRect()),r=or(s),{top:o.top+(r.pageYOffset||n.scrollTop)-(n.clientTop||0),left:o.left+(r.pageXOffset||n.scrollLeft)-(n.clientLeft||0)}):o},b.offset={setOffset:function(e,t,n){var r=b.css(e,\"position\");\"static\"===r&&(e.style.position=\"relative\");var i=b(e),o=i.offset(),a=b.css(e,\"top\"),s=b.css(e,\"left\"),u=(\"absolute\"===r||\"fixed\"===r)&&b.inArray(\"auto\",[a,s])>-1,l={},c={},p,f;u?(c=i.position(),p=c.top,f=c.left):(p=parseFloat(a)||0,f=parseFloat(s)||0),b.isFunction(t)&&(t=t.call(e,n,o)),null!=t.top&&(l.top=t.top-o.top+p),null!=t.left&&(l.left=t.left-o.left+f),\"using\"in t?t.using.call(e,l):i.css(l)}},b.fn.extend({position:function(){if(this[0]){var e,t,n={top:0,left:0},r=this[0];return\"fixed\"===b.css(r,\"position\")?t=r.getBoundingClientRect():(e=this.offsetParent(),t=this.offset(),b.nodeName(e[0],\"html\")||(n=e.offset()),n.top+=b.css(e[0],\"borderTopWidth\",!0),n.left+=b.css(e[0],\"borderLeftWidth\",!0)),{top:t.top-n.top-b.css(r,\"marginTop\",!0),left:t.left-n.left-b.css(r,\"marginLeft\",!0)}}},offsetParent:function(){return this.map(function(){var e=this.offsetParent||o.documentElement;while(e&&!b.nodeName(e,\"html\")&&\"static\"===b.css(e,\"position\"))e=e.offsetParent;return e||o.documentElement})}}),b.each({scrollLeft:\"pageXOffset\",scrollTop:\"pageYOffset\"},function(e,n){var r=/Y/.test(n);b.fn[e]=function(i){return b.access(this,function(e,i,o){var a=or(e);return o===t?a?n in a?a[n]:a.document.documentElement[i]:e[i]:(a?a.scrollTo(r?b(a).scrollLeft():o,r?o:b(a).scrollTop()):e[i]=o,t)},e,i,arguments.length,null)}});function or(e){return b.isWindow(e)?e:9===e.nodeType?e.defaultView||e.parentWindow:!1}b.each({Height:\"height\",Width:\"width\"},function(e,n){b.each({padding:\"inner\"+e,content:n,\"\":\"outer\"+e},function(r,i){b.fn[i]=function(i,o){var a=arguments.length&&(r||\"boolean\"!=typeof i),s=r||(i===!0||o===!0?\"margin\":\"border\");return b.access(this,function(n,r,i){var o;return b.isWindow(n)?n.document.documentElement[\"client\"+e]:9===n.nodeType?(o=n.documentElement,Math.max(n.body[\"scroll\"+e],o[\"scroll\"+e],n.body[\"offset\"+e],o[\"offset\"+e],o[\"client\"+e])):i===t?b.css(n,r,s):b.style(n,r,i,s)},n,a?i:t,a,null)}})}),e.jQuery=e.$=b,\"function\"==typeof define&&define.amd&&define.amd.jQuery&&define(\"jquery\",[],function(){return b})})(window);\n";

const char *datatables = 
"(function(la,s,p){(function(i){if(typeof define===\"function\"&&define.amd)define([\"jquery\"],i);else jQuery&&!jQuery.fn.dataTable&&i(jQuery)})(function(i){var l=function(h){function n(a,b){var c=l.defaults.columns,d=a.aoColumns.length;b=i.extend({},l.models.oColumn,c,{sSortingClass:a.oClasses.sSortable,sSortingClassJUI:a.oClasses.sSortJUI,nTh:b?b:s.createElement(\"th\"),sTitle:c.sTitle?c.sTitle:b?b.innerHTML:\"\",aDataSort:c.aDataSort?c.aDataSort:[d],mData:c.mData?c.oDefaults:d});a.aoColumns.push(b);if(a.aoPreSearchCols[d]===\n"
"p||a.aoPreSearchCols[d]===null)a.aoPreSearchCols[d]=i.extend({},l.models.oSearch);else{b=a.aoPreSearchCols[d];if(b.bRegex===p)b.bRegex=true;if(b.bSmart===p)b.bSmart=true;if(b.bCaseInsensitive===p)b.bCaseInsensitive=true}q(a,d,null)}function q(a,b,c){var d=a.aoColumns[b];if(c!==p&&c!==null){if(c.mDataProp&&!c.mData)c.mData=c.mDataProp;if(c.sType!==p){d.sType=c.sType;d._bAutoType=false}i.extend(d,c);r(d,c,\"sWidth\",\"sWidthOrig\");if(c.iDataSort!==p)d.aDataSort=[c.iDataSort];r(d,c,\"aDataSort\")}var e=d.mRender?\n"
"ca(d.mRender):null,f=ca(d.mData);d.fnGetData=function(g,j){var k=f(g,j);if(d.mRender&&j&&j!==\"\")return e(k,j,g);return k};d.fnSetData=Ja(d.mData);if(!a.oFeatures.bSort)d.bSortable=false;if(!d.bSortable||i.inArray(\"asc\",d.asSorting)==-1&&i.inArray(\"desc\",d.asSorting)==-1){d.sSortingClass=a.oClasses.sSortableNone;d.sSortingClassJUI=\"\"}else if(i.inArray(\"asc\",d.asSorting)==-1&&i.inArray(\"desc\",d.asSorting)==-1){d.sSortingClass=a.oClasses.sSortable;d.sSortingClassJUI=a.oClasses.sSortJUI}else if(i.inArray(\"asc\",\n"
"d.asSorting)!=-1&&i.inArray(\"desc\",d.asSorting)==-1){d.sSortingClass=a.oClasses.sSortableAsc;d.sSortingClassJUI=a.oClasses.sSortJUIAscAllowed}else if(i.inArray(\"asc\",d.asSorting)==-1&&i.inArray(\"desc\",d.asSorting)!=-1){d.sSortingClass=a.oClasses.sSortableDesc;d.sSortingClassJUI=a.oClasses.sSortJUIDescAllowed}}function o(a){if(a.oFeatures.bAutoWidth===false)return false;ta(a);for(var b=0,c=a.aoColumns.length;b<c;b++)a.aoColumns[b].nTh.style.width=a.aoColumns[b].sWidth}function v(a,b){a=A(a,\"bVisible\");\n"
"return typeof a[b]===\"number\"?a[b]:null}function w(a,b){a=A(a,\"bVisible\");b=i.inArray(b,a);return b!==-1?b:null}function D(a){return A(a,\"bVisible\").length}function A(a,b){var c=[];i.map(a.aoColumns,function(d,e){d[b]&&c.push(e)});return c}function G(a){for(var b=l.ext.aTypes,c=b.length,d=0;d<c;d++){var e=b[d](a);if(e!==null)return e}return\"string\"}function E(a,b){b=b.split(\",\");for(var c=[],d=0,e=a.aoColumns.length;d<e;d++)for(var f=0;f<e;f++)if(a.aoColumns[d].sName==b[f]){c.push(f);break}return c}\n"
"function Y(a){for(var b=\"\",c=0,d=a.aoColumns.length;c<d;c++)b+=a.aoColumns[c].sName+\",\";if(b.length==d)return\"\";return b.slice(0,-1)}function ma(a,b,c,d){var e,f,g,j,k;if(b)for(e=b.length-1;e>=0;e--){var m=b[e].aTargets;i.isArray(m)||O(a,1,\"aTargets must be an array of targets, not a \"+typeof m);f=0;for(g=m.length;f<g;f++)if(typeof m[f]===\"number\"&&m[f]>=0){for(;a.aoColumns.length<=m[f];)n(a);d(m[f],b[e])}else if(typeof m[f]===\"number\"&&m[f]<0)d(a.aoColumns.length+m[f],b[e]);else if(typeof m[f]===\n"
"\"string\"){j=0;for(k=a.aoColumns.length;j<k;j++)if(m[f]==\"_all\"||i(a.aoColumns[j].nTh).hasClass(m[f]))d(j,b[e])}}if(c){e=0;for(a=c.length;e<a;e++)d(e,c[e])}}function R(a,b){var c;c=i.isArray(b)?b.slice():i.extend(true,{},b);b=a.aoData.length;var d=i.extend(true,{},l.models.oRow);d._aData=c;a.aoData.push(d);var e;d=0;for(var f=a.aoColumns.length;d<f;d++){c=a.aoColumns[d];typeof c.fnRender===\"function\"&&c.bUseRendered&&c.mData!==null?S(a,b,d,da(a,b,d)):S(a,b,d,F(a,b,d));if(c._bAutoType&&c.sType!=\"string\"){e=\n"
"F(a,b,d,\"type\");if(e!==null&&e!==\"\"){e=G(e);if(c.sType===null)c.sType=e;else if(c.sType!=e&&c.sType!=\"html\")c.sType=\"string\"}}}a.aiDisplayMaster.push(b);a.oFeatures.bDeferRender||ua(a,b);return b}function ea(a){var b,c,d,e,f,g,j;if(a.bDeferLoading||a.sAjaxSource===null)for(b=a.nTBody.firstChild;b;){if(b.nodeName.toUpperCase()==\"TR\"){c=a.aoData.length;b._DT_RowIndex=c;a.aoData.push(i.extend(true,{},l.models.oRow,{nTr:b}));a.aiDisplayMaster.push(c);f=b.firstChild;for(d=0;f;){g=f.nodeName.toUpperCase();\n"
"if(g==\"TD\"||g==\"TH\"){S(a,c,d,i.trim(f.innerHTML));d++}f=f.nextSibling}}b=b.nextSibling}e=fa(a);d=[];b=0;for(c=e.length;b<c;b++)for(f=e[b].firstChild;f;){g=f.nodeName.toUpperCase();if(g==\"TD\"||g==\"TH\")d.push(f);f=f.nextSibling}c=0;for(e=a.aoColumns.length;c<e;c++){j=a.aoColumns[c];if(j.sTitle===null)j.sTitle=j.nTh.innerHTML;var k=j._bAutoType,m=typeof j.fnRender===\"function\",u=j.sClass!==null,x=j.bVisible,y,B;if(k||m||u||!x){g=0;for(b=a.aoData.length;g<b;g++){f=a.aoData[g];y=d[g*e+c];if(k&&j.sType!=\n"
"\"string\"){B=F(a,g,c,\"type\");if(B!==\"\"){B=G(B);if(j.sType===null)j.sType=B;else if(j.sType!=B&&j.sType!=\"html\")j.sType=\"string\"}}if(j.mRender)y.innerHTML=F(a,g,c,\"display\");else if(j.mData!==c)y.innerHTML=F(a,g,c,\"display\");if(m){B=da(a,g,c);y.innerHTML=B;j.bUseRendered&&S(a,g,c,B)}if(u)y.className+=\" \"+j.sClass;if(x)f._anHidden[c]=null;else{f._anHidden[c]=y;y.parentNode.removeChild(y)}j.fnCreatedCell&&j.fnCreatedCell.call(a.oInstance,y,F(a,g,c,\"display\"),f._aData,g,c)}}}if(a.aoRowCreatedCallback.length!==\n"
"0){b=0;for(c=a.aoData.length;b<c;b++){f=a.aoData[b];K(a,\"aoRowCreatedCallback\",null,[f.nTr,f._aData,b])}}}function V(a,b){return b._DT_RowIndex!==p?b._DT_RowIndex:null}function va(a,b,c){b=W(a,b);var d=0;for(a=a.aoColumns.length;d<a;d++)if(b[d]===c)return d;return-1}function na(a,b,c,d){for(var e=[],f=0,g=d.length;f<g;f++)e.push(F(a,b,d[f],c));return e}function F(a,b,c,d){var e=a.aoColumns[c];if((c=e.fnGetData(a.aoData[b]._aData,d))===p){if(a.iDrawError!=a.iDraw&&e.sDefaultContent===null){O(a,0,\"Requested unknown parameter \"+\n"
"(typeof e.mData==\"function\"?\"{mData function}\":\"'\"+e.mData+\"'\")+\" from the data source for row \"+b);a.iDrawError=a.iDraw}return e.sDefaultContent}if(c===null&&e.sDefaultContent!==null)c=e.sDefaultContent;else if(typeof c===\"function\")return c();if(d==\"display\"&&c===null)return\"\";return c}function S(a,b,c,d){a.aoColumns[c].fnSetData(a.aoData[b]._aData,d)}function ca(a){if(a===null)return function(){return null};else if(typeof a===\"function\")return function(c,d,e){return a(c,d,e)};else if(typeof a===\n"
"\"string\"&&(a.indexOf(\".\")!==-1||a.indexOf(\"[\")!==-1)){var b=function(c,d,e){var f=e.split(\".\"),g;if(e!==\"\"){var j=0;for(g=f.length;j<g;j++){if(e=f[j].match(ga)){f[j]=f[j].replace(ga,\"\");if(f[j]!==\"\")c=c[f[j]];g=[];f.splice(0,j+1);f=f.join(\".\");j=0;for(var k=c.length;j<k;j++)g.push(b(c[j],d,f));c=e[0].substring(1,e[0].length-1);c=c===\"\"?g:g.join(c);break}if(c===null||c[f[j]]===p)return p;c=c[f[j]]}}return c};return function(c,d){return b(c,d,a)}}else return function(c){return c[a]}}function Ja(a){if(a===\n"
"null)return function(){};else if(typeof a===\"function\")return function(c,d){a(c,\"set\",d)};else if(typeof a===\"string\"&&(a.indexOf(\".\")!==-1||a.indexOf(\"[\")!==-1)){var b=function(c,d,e){e=e.split(\".\");var f,g,j=0;for(g=e.length-1;j<g;j++){if(f=e[j].match(ga)){e[j]=e[j].replace(ga,\"\");c[e[j]]=[];f=e.slice();f.splice(0,j+1);g=f.join(\".\");for(var k=0,m=d.length;k<m;k++){f={};b(f,d[k],g);c[e[j]].push(f)}return}if(c[e[j]]===null||c[e[j]]===p)c[e[j]]={};c=c[e[j]]}c[e[e.length-1].replace(ga,\"\")]=d};return function(c,\n"
"d){return b(c,d,a)}}else return function(c,d){c[a]=d}}function oa(a){for(var b=[],c=a.aoData.length,d=0;d<c;d++)b.push(a.aoData[d]._aData);return b}function wa(a){a.aoData.splice(0,a.aoData.length);a.aiDisplayMaster.splice(0,a.aiDisplayMaster.length);a.aiDisplay.splice(0,a.aiDisplay.length);I(a)}function xa(a,b){for(var c=-1,d=0,e=a.length;d<e;d++)if(a[d]==b)c=d;else a[d]>b&&a[d]--;c!=-1&&a.splice(c,1)}function da(a,b,c){var d=a.aoColumns[c];return d.fnRender({iDataRow:b,iDataColumn:c,oSettings:a,\n"
"aData:a.aoData[b]._aData,mDataProp:d.mData},F(a,b,c,\"display\"))}function ua(a,b){var c=a.aoData[b],d;if(c.nTr===null){c.nTr=s.createElement(\"tr\");c.nTr._DT_RowIndex=b;if(c._aData.DT_RowId)c.nTr.id=c._aData.DT_RowId;if(c._aData.DT_RowClass)c.nTr.className=c._aData.DT_RowClass;for(var e=0,f=a.aoColumns.length;e<f;e++){var g=a.aoColumns[e];d=s.createElement(g.sCellType);d.innerHTML=typeof g.fnRender===\"function\"&&(!g.bUseRendered||g.mData===null)?da(a,b,e):F(a,b,e,\"display\");if(g.sClass!==null)d.className=\n"
"g.sClass;if(g.bVisible){c.nTr.appendChild(d);c._anHidden[e]=null}else c._anHidden[e]=d;g.fnCreatedCell&&g.fnCreatedCell.call(a.oInstance,d,F(a,b,e,\"display\"),c._aData,b,e)}K(a,\"aoRowCreatedCallback\",null,[c.nTr,c._aData,b])}}function Ka(a){var b,c,d;if(i(\"th, td\",a.nTHead).length!==0){b=0;for(d=a.aoColumns.length;b<d;b++){c=a.aoColumns[b].nTh;c.setAttribute(\"role\",\"columnheader\");if(a.aoColumns[b].bSortable){c.setAttribute(\"tabindex\",a.iTabIndex);c.setAttribute(\"aria-controls\",a.sTableId)}a.aoColumns[b].sClass!==\n"
"null&&i(c).addClass(a.aoColumns[b].sClass);if(a.aoColumns[b].sTitle!=c.innerHTML)c.innerHTML=a.aoColumns[b].sTitle}}else{var e=s.createElement(\"tr\");b=0;for(d=a.aoColumns.length;b<d;b++){c=a.aoColumns[b].nTh;c.innerHTML=a.aoColumns[b].sTitle;c.setAttribute(\"tabindex\",\"0\");a.aoColumns[b].sClass!==null&&i(c).addClass(a.aoColumns[b].sClass);e.appendChild(c)}i(a.nTHead).html(\"\")[0].appendChild(e);ha(a.aoHeader,a.nTHead)}i(a.nTHead).children(\"tr\").attr(\"role\",\"row\");if(a.bJUI){b=0;for(d=a.aoColumns.length;b<\n"
"d;b++){c=a.aoColumns[b].nTh;e=s.createElement(\"div\");e.className=a.oClasses.sSortJUIWrapper;i(c).contents().appendTo(e);var f=s.createElement(\"span\");f.className=a.oClasses.sSortIcon;e.appendChild(f);c.appendChild(e)}}if(a.oFeatures.bSort)for(b=0;b<a.aoColumns.length;b++)a.aoColumns[b].bSortable!==false?ya(a,a.aoColumns[b].nTh,b):i(a.aoColumns[b].nTh).addClass(a.oClasses.sSortableNone);a.oClasses.sFooterTH!==\"\"&&i(a.nTFoot).children(\"tr\").children(\"th\").addClass(a.oClasses.sFooterTH);if(a.nTFoot!==\n"
"null){c=Z(a,null,a.aoFooter);b=0;for(d=a.aoColumns.length;b<d;b++)if(c[b]){a.aoColumns[b].nTf=c[b];a.aoColumns[b].sClass&&i(c[b]).addClass(a.aoColumns[b].sClass)}}}function ia(a,b,c){var d,e,f,g=[],j=[],k=a.aoColumns.length,m;if(c===p)c=false;d=0;for(e=b.length;d<e;d++){g[d]=b[d].slice();g[d].nTr=b[d].nTr;for(f=k-1;f>=0;f--)!a.aoColumns[f].bVisible&&!c&&g[d].splice(f,1);j.push([])}d=0;for(e=g.length;d<e;d++){if(a=g[d].nTr)for(;f=a.firstChild;)a.removeChild(f);f=0;for(b=g[d].length;f<b;f++){m=k=1;\n"
"if(j[d][f]===p){a.appendChild(g[d][f].cell);for(j[d][f]=1;g[d+k]!==p&&g[d][f].cell==g[d+k][f].cell;){j[d+k][f]=1;k++}for(;g[d][f+m]!==p&&g[d][f].cell==g[d][f+m].cell;){for(c=0;c<k;c++)j[d+c][f+m]=1;m++}g[d][f].cell.rowSpan=k;g[d][f].cell.colSpan=m}}}}function H(a){var b=K(a,\"aoPreDrawCallback\",\"preDraw\",[a]);if(i.inArray(false,b)!==-1)P(a,false);else{var c,d;b=[];var e=0,f=a.asStripeClasses.length;c=a.aoOpenRows.length;a.bDrawing=true;if(a.iInitDisplayStart!==p&&a.iInitDisplayStart!=-1){a._iDisplayStart=\n"
"a.oFeatures.bServerSide?a.iInitDisplayStart:a.iInitDisplayStart>=a.fnRecordsDisplay()?0:a.iInitDisplayStart;a.iInitDisplayStart=-1;I(a)}if(a.bDeferLoading){a.bDeferLoading=false;a.iDraw++}else if(a.oFeatures.bServerSide){if(!a.bDestroying&&!La(a))return}else a.iDraw++;if(a.aiDisplay.length!==0){var g=a._iDisplayStart;d=a._iDisplayEnd;if(a.oFeatures.bServerSide){g=0;d=a.aoData.length}for(g=g;g<d;g++){var j=a.aoData[a.aiDisplay[g]];j.nTr===null&&ua(a,a.aiDisplay[g]);var k=j.nTr;if(f!==0){var m=a.asStripeClasses[e%\n"
"f];if(j._sRowStripe!=m){i(k).removeClass(j._sRowStripe).addClass(m);j._sRowStripe=m}}K(a,\"aoRowCallback\",null,[k,a.aoData[a.aiDisplay[g]]._aData,e,g]);b.push(k);e++;if(c!==0)for(j=0;j<c;j++)if(k==a.aoOpenRows[j].nParent){b.push(a.aoOpenRows[j].nTr);break}}}else{b[0]=s.createElement(\"tr\");if(a.asStripeClasses[0])b[0].className=a.asStripeClasses[0];c=a.oLanguage;f=c.sZeroRecords;if(a.iDraw==1&&a.sAjaxSource!==null&&!a.oFeatures.bServerSide)f=c.sLoadingRecords;else if(c.sEmptyTable&&a.fnRecordsTotal()===\n"
"0)f=c.sEmptyTable;c=s.createElement(\"td\");c.setAttribute(\"valign\",\"top\");c.colSpan=D(a);c.className=a.oClasses.sRowEmpty;c.innerHTML=za(a,f);b[e].appendChild(c)}K(a,\"aoHeaderCallback\",\"header\",[i(a.nTHead).children(\"tr\")[0],oa(a),a._iDisplayStart,a.fnDisplayEnd(),a.aiDisplay]);K(a,\"aoFooterCallback\",\"footer\",[i(a.nTFoot).children(\"tr\")[0],oa(a),a._iDisplayStart,a.fnDisplayEnd(),a.aiDisplay]);e=s.createDocumentFragment();c=s.createDocumentFragment();if(a.nTBody){f=a.nTBody.parentNode;c.appendChild(a.nTBody);\n"
"if(!a.oScroll.bInfinite||!a._bInitComplete||a.bSorted||a.bFiltered)for(;c=a.nTBody.firstChild;)a.nTBody.removeChild(c);c=0;for(d=b.length;c<d;c++)e.appendChild(b[c]);a.nTBody.appendChild(e);f!==null&&f.appendChild(a.nTBody)}K(a,\"aoDrawCallback\",\"draw\",[a]);a.bSorted=false;a.bFiltered=false;a.bDrawing=false;if(a.oFeatures.bServerSide){P(a,false);a._bInitComplete||pa(a)}}}function qa(a){if(a.oFeatures.bSort)$(a,a.oPreviousSearch);else if(a.oFeatures.bFilter)X(a,a.oPreviousSearch);else{I(a);H(a)}}function Ma(a){var b=\n"
"i(\"<div></div>\")[0];a.nTable.parentNode.insertBefore(b,a.nTable);a.nTableWrapper=i('<div id=\"'+a.sTableId+'_wrapper\" class=\"'+a.oClasses.sWrapper+'\" role=\"grid\"></div>')[0];a.nTableReinsertBefore=a.nTable.nextSibling;for(var c=a.nTableWrapper,d=a.sDom.split(\"\"),e,f,g,j,k,m,u,x=0;x<d.length;x++){f=0;g=d[x];if(g==\"<\"){j=i(\"<div></div>\")[0];k=d[x+1];if(k==\"'\"||k=='\"'){m=\"\";for(u=2;d[x+u]!=k;){m+=d[x+u];u++}if(m==\"H\")m=a.oClasses.sJUIHeader;else if(m==\"F\")m=a.oClasses.sJUIFooter;if(m.indexOf(\".\")!=-1){k=\n"
"m.split(\".\");j.id=k[0].substr(1,k[0].length-1);j.className=k[1]}else if(m.charAt(0)==\"#\")j.id=m.substr(1,m.length-1);else j.className=m;x+=u}c.appendChild(j);c=j}else if(g==\">\")c=c.parentNode;else if(g==\"l\"&&a.oFeatures.bPaginate&&a.oFeatures.bLengthChange){e=Na(a);f=1}else if(g==\"f\"&&a.oFeatures.bFilter){e=Oa(a);f=1}else if(g==\"r\"&&a.oFeatures.bProcessing){e=Pa(a);f=1}else if(g==\"t\"){e=Qa(a);f=1}else if(g==\"i\"&&a.oFeatures.bInfo){e=Ra(a);f=1}else if(g==\"p\"&&a.oFeatures.bPaginate){e=Sa(a);f=1}else if(l.ext.aoFeatures.length!==\n"
"0){j=l.ext.aoFeatures;u=0;for(k=j.length;u<k;u++)if(g==j[u].cFeature){if(e=j[u].fnInit(a))f=1;break}}if(f==1&&e!==null){if(typeof a.aanFeatures[g]!==\"object\")a.aanFeatures[g]=[];a.aanFeatures[g].push(e);c.appendChild(e)}}b.parentNode.replaceChild(a.nTableWrapper,b)}function ha(a,b){b=i(b).children(\"tr\");var c,d,e,f,g,j,k,m,u,x,y=function(B,T,M){for(B=B[T];B[M];)M++;return M};a.splice(0,a.length);e=0;for(j=b.length;e<j;e++)a.push([]);e=0;for(j=b.length;e<j;e++){c=b[e];for(d=c.firstChild;d;){if(d.nodeName.toUpperCase()==\n"
"\"TD\"||d.nodeName.toUpperCase()==\"TH\"){m=d.getAttribute(\"colspan\")*1;u=d.getAttribute(\"rowspan\")*1;m=!m||m===0||m===1?1:m;u=!u||u===0||u===1?1:u;k=y(a,e,0);x=m===1?true:false;for(g=0;g<m;g++)for(f=0;f<u;f++){a[e+f][k+g]={cell:d,unique:x};a[e+f].nTr=c}}d=d.nextSibling}}}function Z(a,b,c){var d=[];if(!c){c=a.aoHeader;if(b){c=[];ha(c,b)}}b=0;for(var e=c.length;b<e;b++)for(var f=0,g=c[b].length;f<g;f++)if(c[b][f].unique&&(!d[f]||!a.bSortCellsTop))d[f]=c[b][f].cell;return d}function La(a){if(a.bAjaxDataGet){a.iDraw++;\n"
"P(a,true);var b=Ta(a);Aa(a,b);a.fnServerData.call(a.oInstance,a.sAjaxSource,b,function(c){Ua(a,c)},a);return false}else return true}function Ta(a){var b=a.aoColumns.length,c=[],d,e,f,g;c.push({name:\"sEcho\",value:a.iDraw});c.push({name:\"iColumns\",value:b});c.push({name:\"sColumns\",value:Y(a)});c.push({name:\"iDisplayStart\",value:a._iDisplayStart});c.push({name:\"iDisplayLength\",value:a.oFeatures.bPaginate!==false?a._iDisplayLength:-1});for(f=0;f<b;f++){d=a.aoColumns[f].mData;c.push({name:\"mDataProp_\"+\n"
"f,value:typeof d===\"function\"?\"function\":d})}if(a.oFeatures.bFilter!==false){c.push({name:\"sSearch\",value:a.oPreviousSearch.sSearch});c.push({name:\"bRegex\",value:a.oPreviousSearch.bRegex});for(f=0;f<b;f++){c.push({name:\"sSearch_\"+f,value:a.aoPreSearchCols[f].sSearch});c.push({name:\"bRegex_\"+f,value:a.aoPreSearchCols[f].bRegex});c.push({name:\"bSearchable_\"+f,value:a.aoColumns[f].bSearchable})}}if(a.oFeatures.bSort!==false){var j=0;d=a.aaSortingFixed!==null?a.aaSortingFixed.concat(a.aaSorting):a.aaSorting.slice();\n"
"for(f=0;f<d.length;f++){e=a.aoColumns[d[f][0]].aDataSort;for(g=0;g<e.length;g++){c.push({name:\"iSortCol_\"+j,value:e[g]});c.push({name:\"sSortDir_\"+j,value:d[f][1]});j++}}c.push({name:\"iSortingCols\",value:j});for(f=0;f<b;f++)c.push({name:\"bSortable_\"+f,value:a.aoColumns[f].bSortable})}return c}function Aa(a,b){K(a,\"aoServerParams\",\"serverParams\",[b])}function Ua(a,b){if(b.sEcho!==p)if(b.sEcho*1<a.iDraw)return;else a.iDraw=b.sEcho*1;if(!a.oScroll.bInfinite||a.oScroll.bInfinite&&(a.bSorted||a.bFiltered))wa(a);\n"
"a._iRecordsTotal=parseInt(b.iTotalRecords,10);a._iRecordsDisplay=parseInt(b.iTotalDisplayRecords,10);var c=Y(a);c=b.sColumns!==p&&c!==\"\"&&b.sColumns!=c;var d;if(c)d=E(a,b.sColumns);b=ca(a.sAjaxDataProp)(b);for(var e=0,f=b.length;e<f;e++)if(c){for(var g=[],j=0,k=a.aoColumns.length;j<k;j++)g.push(b[e][d[j]]);R(a,g)}else R(a,b[e]);a.aiDisplay=a.aiDisplayMaster.slice();a.bAjaxDataGet=false;H(a);a.bAjaxDataGet=true;P(a,false)}function Oa(a){var b=a.oPreviousSearch,c=a.oLanguage.sSearch;c=c.indexOf(\"_INPUT_\")!==\n"
"-1?c.replace(\"_INPUT_\",'<input type=\"text\" />'):c===\"\"?'<input type=\"text\" />':c+' <input type=\"text\" />';var d=s.createElement(\"div\");d.className=a.oClasses.sFilter;d.innerHTML=\"<label>\"+c+\"</label>\";if(!a.aanFeatures.f)d.id=a.sTableId+\"_filter\";c=i('input[type=\"text\"]',d);d._DT_Input=c[0];c.val(b.sSearch.replace('\"',\"&quot;\"));c.bind(\"keyup.DT\",function(){for(var e=a.aanFeatures.f,f=this.value===\"\"?\"\":this.value,g=0,j=e.length;g<j;g++)e[g]!=i(this).parents(\"div.dataTables_filter\")[0]&&i(e[g]._DT_Input).val(f);\n"
"f!=b.sSearch&&X(a,{sSearch:f,bRegex:b.bRegex,bSmart:b.bSmart,bCaseInsensitive:b.bCaseInsensitive})});c.attr(\"aria-controls\",a.sTableId).bind(\"keypress.DT\",function(e){if(e.keyCode==13)return false});return d}function X(a,b,c){var d=a.oPreviousSearch,e=a.aoPreSearchCols,f=function(g){d.sSearch=g.sSearch;d.bRegex=g.bRegex;d.bSmart=g.bSmart;d.bCaseInsensitive=g.bCaseInsensitive};if(a.oFeatures.bServerSide)f(b);else{Va(a,b.sSearch,c,b.bRegex,b.bSmart,b.bCaseInsensitive);f(b);for(b=0;b<a.aoPreSearchCols.length;b++)Wa(a,\n"
"e[b].sSearch,b,e[b].bRegex,e[b].bSmart,e[b].bCaseInsensitive);Xa(a)}a.bFiltered=true;i(a.oInstance).trigger(\"filter\",a);a._iDisplayStart=0;I(a);H(a);Ba(a,0)}function Xa(a){for(var b=l.ext.afnFiltering,c=A(a,\"bSearchable\"),d=0,e=b.length;d<e;d++)for(var f=0,g=0,j=a.aiDisplay.length;g<j;g++){var k=a.aiDisplay[g-f];if(!b[d](a,na(a,k,\"filter\",c),k)){a.aiDisplay.splice(g-f,1);f++}}}function Wa(a,b,c,d,e,f){if(b!==\"\"){var g=0;b=Ca(b,d,e,f);for(d=a.aiDisplay.length-1;d>=0;d--){e=Ya(F(a,a.aiDisplay[d],c,\n"
"\"filter\"),a.aoColumns[c].sType);if(!b.test(e)){a.aiDisplay.splice(d,1);g++}}}}function Va(a,b,c,d,e,f){d=Ca(b,d,e,f);e=a.oPreviousSearch;c||(c=0);if(l.ext.afnFiltering.length!==0)c=1;if(b.length<=0){a.aiDisplay.splice(0,a.aiDisplay.length);a.aiDisplay=a.aiDisplayMaster.slice()}else if(a.aiDisplay.length==a.aiDisplayMaster.length||e.sSearch.length>b.length||c==1||b.indexOf(e.sSearch)!==0){a.aiDisplay.splice(0,a.aiDisplay.length);Ba(a,1);for(b=0;b<a.aiDisplayMaster.length;b++)d.test(a.asDataSearch[b])&&\n"
"a.aiDisplay.push(a.aiDisplayMaster[b])}else for(b=c=0;b<a.asDataSearch.length;b++)if(!d.test(a.asDataSearch[b])){a.aiDisplay.splice(b-c,1);c++}}function Ba(a,b){if(!a.oFeatures.bServerSide){a.asDataSearch=[];var c=A(a,\"bSearchable\");b=b===1?a.aiDisplayMaster:a.aiDisplay;for(var d=0,e=b.length;d<e;d++)a.asDataSearch[d]=Da(a,na(a,b[d],\"filter\",c))}}function Da(a,b){a=b.join(\"  \");if(a.indexOf(\"&\")!==-1)a=i(\"<div>\").html(a).text();return a.replace(/[\\n\\r]/g,\" \")}function Ca(a,b,c,d){if(c){a=b?a.split(\" \"):\n"
"Ea(a).split(\" \");a=\"^(?=.*?\"+a.join(\")(?=.*?\")+\").*$\";return new RegExp(a,d?\"i\":\"\")}else{a=b?a:Ea(a);return new RegExp(a,d?\"i\":\"\")}}function Ya(a,b){if(typeof l.ext.ofnSearch[b]===\"function\")return l.ext.ofnSearch[b](a);else if(a===null)return\"\";else if(b==\"html\")return a.replace(/[\\r\\n]/g,\" \").replace(/<.*?>/g,\"\");else if(typeof a===\"string\")return a.replace(/[\\r\\n]/g,\" \");return a}function Ea(a){return a.replace(new RegExp(\"(\\\\/|\\\\.|\\\\*|\\\\+|\\\\?|\\\\||\\\\(|\\\\)|\\\\[|\\\\]|\\\\{|\\\\}|\\\\\\\\|\\\\$|\\\\^|\\\\-)\",\"g\"),\n"
"\"\\\\$1\")}function Ra(a){var b=s.createElement(\"div\");b.className=a.oClasses.sInfo;if(!a.aanFeatures.i){a.aoDrawCallback.push({fn:Za,sName:\"information\"});b.id=a.sTableId+\"_info\"}a.nTable.setAttribute(\"aria-describedby\",a.sTableId+\"_info\");return b}function Za(a){if(!(!a.oFeatures.bInfo||a.aanFeatures.i.length===0)){var b=a.oLanguage,c=a._iDisplayStart+1,d=a.fnDisplayEnd(),e=a.fnRecordsTotal(),f=a.fnRecordsDisplay(),g;g=f===0?b.sInfoEmpty:b.sInfo;if(f!=e)g+=\" \"+b.sInfoFiltered;g+=b.sInfoPostFix;g=za(a,\n"
"g);if(b.fnInfoCallback!==null)g=b.fnInfoCallback.call(a.oInstance,a,c,d,e,f,g);a=a.aanFeatures.i;b=0;for(c=a.length;b<c;b++)i(a[b]).html(g)}}function za(a,b){var c=a.fnFormatNumber(a._iDisplayStart+1),d=a.fnDisplayEnd();d=a.fnFormatNumber(d);var e=a.fnRecordsDisplay();e=a.fnFormatNumber(e);var f=a.fnRecordsTotal();f=a.fnFormatNumber(f);if(a.oScroll.bInfinite)c=a.fnFormatNumber(1);return b.replace(/_START_/g,c).replace(/_END_/g,d).replace(/_TOTAL_/g,e).replace(/_MAX_/g,f)}function ra(a){var b,c,d=\n"
"a.iInitDisplayStart;if(a.bInitialised===false)setTimeout(function(){ra(a)},200);else{Ma(a);Ka(a);ia(a,a.aoHeader);a.nTFoot&&ia(a,a.aoFooter);P(a,true);a.oFeatures.bAutoWidth&&ta(a);b=0;for(c=a.aoColumns.length;b<c;b++)if(a.aoColumns[b].sWidth!==null)a.aoColumns[b].nTh.style.width=t(a.aoColumns[b].sWidth);if(a.oFeatures.bSort)$(a);else if(a.oFeatures.bFilter)X(a,a.oPreviousSearch);else{a.aiDisplay=a.aiDisplayMaster.slice();I(a);H(a)}if(a.sAjaxSource!==null&&!a.oFeatures.bServerSide){c=[];Aa(a,c);a.fnServerData.call(a.oInstance,\n"
"a.sAjaxSource,c,function(e){var f=a.sAjaxDataProp!==\"\"?ca(a.sAjaxDataProp)(e):e;for(b=0;b<f.length;b++)R(a,f[b]);a.iInitDisplayStart=d;if(a.oFeatures.bSort)$(a);else{a.aiDisplay=a.aiDisplayMaster.slice();I(a);H(a)}P(a,false);pa(a,e)},a)}else if(!a.oFeatures.bServerSide){P(a,false);pa(a)}}}function pa(a,b){a._bInitComplete=true;K(a,\"aoInitComplete\",\"init\",[a,b])}function Fa(a){var b=l.defaults.oLanguage;!a.sEmptyTable&&a.sZeroRecords&&b.sEmptyTable===\"No data available in table\"&&r(a,a,\"sZeroRecords\",\n"
"\"sEmptyTable\");!a.sLoadingRecords&&a.sZeroRecords&&b.sLoadingRecords===\"Loading...\"&&r(a,a,\"sZeroRecords\",\"sLoadingRecords\")}function Na(a){if(a.oScroll.bInfinite)return null;var b='<select size=\"1\" '+('name=\"'+a.sTableId+'_length\"')+\">\",c,d,e=a.aLengthMenu;if(e.length==2&&typeof e[0]===\"object\"&&typeof e[1]===\"object\"){c=0;for(d=e[0].length;c<d;c++)b+='<option value=\"'+e[0][c]+'\">'+e[1][c]+\"</option>\"}else{c=0;for(d=e.length;c<d;c++)b+='<option value=\"'+e[c]+'\">'+e[c]+\"</option>\"}b+=\"</select>\";\n"
"e=s.createElement(\"div\");if(!a.aanFeatures.l)e.id=a.sTableId+\"_length\";e.className=a.oClasses.sLength;e.innerHTML=\"<label>\"+a.oLanguage.sLengthMenu.replace(\"_MENU_\",b)+\"</label>\";i('select option[value=\"'+a._iDisplayLength+'\"]',e).attr(\"selected\",true);i(\"select\",e).bind(\"change.DT\",function(){var f=i(this).val(),g=a.aanFeatures.l;c=0;for(d=g.length;c<d;c++)g[c]!=this.parentNode&&i(\"select\",g[c]).val(f);a._iDisplayLength=parseInt(f,10);I(a);if(a.fnDisplayEnd()==a.fnRecordsDisplay()){a._iDisplayStart=\n"
"a.fnDisplayEnd()-a._iDisplayLength;if(a._iDisplayStart<0)a._iDisplayStart=0}if(a._iDisplayLength==-1)a._iDisplayStart=0;H(a)});i(\"select\",e).attr(\"aria-controls\",a.sTableId);return e}function I(a){a._iDisplayEnd=a.oFeatures.bPaginate===false?a.aiDisplay.length:a._iDisplayStart+a._iDisplayLength>a.aiDisplay.length||a._iDisplayLength==-1?a.aiDisplay.length:a._iDisplayStart+a._iDisplayLength}function Sa(a){if(a.oScroll.bInfinite)return null;var b=s.createElement(\"div\");b.className=a.oClasses.sPaging+\n"
"a.sPaginationType;l.ext.oPagination[a.sPaginationType].fnInit(a,b,function(c){I(c);H(c)});a.aanFeatures.p||a.aoDrawCallback.push({fn:function(c){l.ext.oPagination[c.sPaginationType].fnUpdate(c,function(d){I(d);H(d)})},sName:\"pagination\"});return b}function Ga(a,b){var c=a._iDisplayStart;if(typeof b===\"number\"){a._iDisplayStart=b*a._iDisplayLength;if(a._iDisplayStart>a.fnRecordsDisplay())a._iDisplayStart=0}else if(b==\"first\")a._iDisplayStart=0;else if(b==\"previous\"){a._iDisplayStart=a._iDisplayLength>=\n"
"0?a._iDisplayStart-a._iDisplayLength:0;if(a._iDisplayStart<0)a._iDisplayStart=0}else if(b==\"next\")if(a._iDisplayLength>=0){if(a._iDisplayStart+a._iDisplayLength<a.fnRecordsDisplay())a._iDisplayStart+=a._iDisplayLength}else a._iDisplayStart=0;else if(b==\"last\")if(a._iDisplayLength>=0){b=parseInt((a.fnRecordsDisplay()-1)/a._iDisplayLength,10)+1;a._iDisplayStart=(b-1)*a._iDisplayLength}else a._iDisplayStart=0;else O(a,0,\"Unknown paging action: \"+b);i(a.oInstance).trigger(\"page\",a);return c!=a._iDisplayStart}\n"
"function Pa(a){var b=s.createElement(\"div\");if(!a.aanFeatures.r)b.id=a.sTableId+\"_processing\";b.innerHTML=a.oLanguage.sProcessing;b.className=a.oClasses.sProcessing;a.nTable.parentNode.insertBefore(b,a.nTable);return b}function P(a,b){if(a.oFeatures.bProcessing)for(var c=a.aanFeatures.r,d=0,e=c.length;d<e;d++)c[d].style.visibility=b?\"visible\":\"hidden\";i(a.oInstance).trigger(\"processing\",[a,b])}function Qa(a){if(a.oScroll.sX===\"\"&&a.oScroll.sY===\"\")return a.nTable;var b=s.createElement(\"div\"),c=s.createElement(\"div\"),\n"
"d=s.createElement(\"div\"),e=s.createElement(\"div\"),f=s.createElement(\"div\"),g=s.createElement(\"div\"),j=a.nTable.cloneNode(false),k=a.nTable.cloneNode(false),m=a.nTable.getElementsByTagName(\"thead\")[0],u=a.nTable.getElementsByTagName(\"tfoot\").length===0?null:a.nTable.getElementsByTagName(\"tfoot\")[0],x=a.oClasses;c.appendChild(d);f.appendChild(g);e.appendChild(a.nTable);b.appendChild(c);b.appendChild(e);d.appendChild(j);j.appendChild(m);if(u!==null){b.appendChild(f);g.appendChild(k);k.appendChild(u)}b.className=\n"
"x.sScrollWrapper;c.className=x.sScrollHead;d.className=x.sScrollHeadInner;e.className=x.sScrollBody;f.className=x.sScrollFoot;g.className=x.sScrollFootInner;if(a.oScroll.bAutoCss){c.style.overflow=\"hidden\";c.style.position=\"relative\";f.style.overflow=\"hidden\";e.style.overflow=\"auto\"}c.style.border=\"0\";c.style.width=\"100%\";f.style.border=\"0\";d.style.width=a.oScroll.sXInner!==\"\"?a.oScroll.sXInner:\"100%\";j.removeAttribute(\"id\");j.style.marginLeft=\"0\";a.nTable.style.marginLeft=\"0\";if(u!==null){k.removeAttribute(\"id\");\n"
"k.style.marginLeft=\"0\"}d=i(a.nTable).children(\"caption\");if(d.length>0){d=d[0];if(d._captionSide===\"top\")j.appendChild(d);else d._captionSide===\"bottom\"&&u&&k.appendChild(d)}if(a.oScroll.sX!==\"\"){c.style.width=t(a.oScroll.sX);e.style.width=t(a.oScroll.sX);if(u!==null)f.style.width=t(a.oScroll.sX);i(e).scroll(function(){c.scrollLeft=this.scrollLeft;if(u!==null)f.scrollLeft=this.scrollLeft})}if(a.oScroll.sY!==\"\")e.style.height=t(a.oScroll.sY);a.aoDrawCallback.push({fn:$a,sName:\"scrolling\"});a.oScroll.bInfinite&&\n"
"i(e).scroll(function(){if(!a.bDrawing&&i(this).scrollTop()!==0)if(i(this).scrollTop()+i(this).height()>i(a.nTable).height()-a.oScroll.iLoadGap)if(a.fnDisplayEnd()<a.fnRecordsDisplay()){Ga(a,\"next\");I(a);H(a)}});a.nScrollHead=c;a.nScrollFoot=f;return b}function $a(a){var b=a.nScrollHead.getElementsByTagName(\"div\")[0],c=b.getElementsByTagName(\"table\")[0],d=a.nTable.parentNode,e,f,g,j,k,m,u,x,y=[],B=[],T=a.nTFoot!==null?a.nScrollFoot.getElementsByTagName(\"div\")[0]:null,M=a.nTFoot!==null?T.getElementsByTagName(\"table\")[0]:\n"
"null,L=a.oBrowser.bScrollOversize,ja=function(z){u=z.style;u.paddingTop=\"0\";u.paddingBottom=\"0\";u.borderTopWidth=\"0\";u.borderBottomWidth=\"0\";u.height=0};i(a.nTable).children(\"thead, tfoot\").remove();e=i(a.nTHead).clone()[0];a.nTable.insertBefore(e,a.nTable.childNodes[0]);g=a.nTHead.getElementsByTagName(\"tr\");j=e.getElementsByTagName(\"tr\");if(a.nTFoot!==null){k=i(a.nTFoot).clone()[0];a.nTable.insertBefore(k,a.nTable.childNodes[1]);m=a.nTFoot.getElementsByTagName(\"tr\");k=k.getElementsByTagName(\"tr\")}if(a.oScroll.sX===\n"
"\"\"){d.style.width=\"100%\";b.parentNode.style.width=\"100%\"}var U=Z(a,e);e=0;for(f=U.length;e<f;e++){x=v(a,e);U[e].style.width=a.aoColumns[x].sWidth}a.nTFoot!==null&&N(function(z){z.style.width=\"\"},k);if(a.oScroll.bCollapse&&a.oScroll.sY!==\"\")d.style.height=d.offsetHeight+a.nTHead.offsetHeight+\"px\";e=i(a.nTable).outerWidth();if(a.oScroll.sX===\"\"){a.nTable.style.width=\"100%\";if(L&&(i(\"tbody\",d).height()>d.offsetHeight||i(d).css(\"overflow-y\")==\"scroll\"))a.nTable.style.width=t(i(a.nTable).outerWidth()-\n"
"a.oScroll.iBarWidth)}else if(a.oScroll.sXInner!==\"\")a.nTable.style.width=t(a.oScroll.sXInner);else if(e==i(d).width()&&i(d).height()<i(a.nTable).height()){a.nTable.style.width=t(e-a.oScroll.iBarWidth);if(i(a.nTable).outerWidth()>e-a.oScroll.iBarWidth)a.nTable.style.width=t(e)}else a.nTable.style.width=t(e);e=i(a.nTable).outerWidth();N(ja,j);N(function(z){y.push(t(i(z).width()))},j);N(function(z,Q){z.style.width=y[Q]},g);i(j).height(0);if(a.nTFoot!==null){N(ja,k);N(function(z){B.push(t(i(z).width()))},\n"
"k);N(function(z,Q){z.style.width=B[Q]},m);i(k).height(0)}N(function(z,Q){z.innerHTML=\"\";z.style.width=y[Q]},j);a.nTFoot!==null&&N(function(z,Q){z.innerHTML=\"\";z.style.width=B[Q]},k);if(i(a.nTable).outerWidth()<e){g=d.scrollHeight>d.offsetHeight||i(d).css(\"overflow-y\")==\"scroll\"?e+a.oScroll.iBarWidth:e;if(L&&(d.scrollHeight>d.offsetHeight||i(d).css(\"overflow-y\")==\"scroll\"))a.nTable.style.width=t(g-a.oScroll.iBarWidth);d.style.width=t(g);a.nScrollHead.style.width=t(g);if(a.nTFoot!==null)a.nScrollFoot.style.width=\n"
"t(g);if(a.oScroll.sX===\"\")O(a,1,\"The table cannot fit into the current element which will cause column misalignment. The table has been drawn at its minimum possible width.\");else a.oScroll.sXInner!==\"\"&&O(a,1,\"The table cannot fit into the current element which will cause column misalignment. Increase the sScrollXInner value or remove it to allow automatic calculation\")}else{d.style.width=t(\"100%\");a.nScrollHead.style.width=t(\"100%\");if(a.nTFoot!==null)a.nScrollFoot.style.width=t(\"100%\")}if(a.oScroll.sY===\n"
"\"\")if(L)d.style.height=t(a.nTable.offsetHeight+a.oScroll.iBarWidth);if(a.oScroll.sY!==\"\"&&a.oScroll.bCollapse){d.style.height=t(a.oScroll.sY);L=a.oScroll.sX!==\"\"&&a.nTable.offsetWidth>d.offsetWidth?a.oScroll.iBarWidth:0;if(a.nTable.offsetHeight<d.offsetHeight)d.style.height=t(a.nTable.offsetHeight+L)}L=i(a.nTable).outerWidth();c.style.width=t(L);b.style.width=t(L);c=i(a.nTable).height()>d.clientHeight||i(d).css(\"overflow-y\")==\"scroll\";b.style.paddingRight=c?a.oScroll.iBarWidth+\"px\":\"0px\";if(a.nTFoot!==\n"
"null){M.style.width=t(L);T.style.width=t(L);T.style.paddingRight=c?a.oScroll.iBarWidth+\"px\":\"0px\"}i(d).scroll();if(a.bSorted||a.bFiltered)d.scrollTop=0}function N(a,b,c){for(var d=0,e=0,f=b.length,g,j;e<f;){g=b[e].firstChild;for(j=c?c[e].firstChild:null;g;){if(g.nodeType===1){c?a(g,j,d):a(g,d);d++}g=g.nextSibling;j=c?j.nextSibling:null}e++}}function ab(a,b){if(!a||a===null||a===\"\")return 0;if(!b)b=s.body;var c=s.createElement(\"div\");c.style.width=t(a);b.appendChild(c);a=c.offsetWidth;b.removeChild(c);\n"
"return a}function ta(a){var b=0,c,d=0,e=a.aoColumns.length,f,g,j=i(\"th\",a.nTHead),k=a.nTable.getAttribute(\"width\");g=a.nTable.parentNode;for(f=0;f<e;f++)if(a.aoColumns[f].bVisible){d++;if(a.aoColumns[f].sWidth!==null){c=ab(a.aoColumns[f].sWidthOrig,g);if(c!==null)a.aoColumns[f].sWidth=t(c);b++}}if(e==j.length&&b===0&&d==e&&a.oScroll.sX===\"\"&&a.oScroll.sY===\"\")for(f=0;f<a.aoColumns.length;f++){c=i(j[f]).width();if(c!==null)a.aoColumns[f].sWidth=t(c)}else{b=a.nTable.cloneNode(false);f=a.nTHead.cloneNode(true);\n"
"d=s.createElement(\"tbody\");c=s.createElement(\"tr\");b.removeAttribute(\"id\");b.appendChild(f);if(a.nTFoot!==null){b.appendChild(a.nTFoot.cloneNode(true));N(function(u){u.style.width=\"\"},b.getElementsByTagName(\"tr\"))}b.appendChild(d);d.appendChild(c);d=i(\"thead th\",b);if(d.length===0)d=i(\"tbody tr:eq(0)>td\",b);j=Z(a,f);for(f=d=0;f<e;f++){var m=a.aoColumns[f];if(m.bVisible&&m.sWidthOrig!==null&&m.sWidthOrig!==\"\")j[f-d].style.width=t(m.sWidthOrig);else if(m.bVisible)j[f-d].style.width=\"\";else d++}for(f=\n"
"0;f<e;f++)if(a.aoColumns[f].bVisible){d=bb(a,f);if(d!==null){d=d.cloneNode(true);if(a.aoColumns[f].sContentPadding!==\"\")d.innerHTML+=a.aoColumns[f].sContentPadding;c.appendChild(d)}}g.appendChild(b);if(a.oScroll.sX!==\"\"&&a.oScroll.sXInner!==\"\")b.style.width=t(a.oScroll.sXInner);else if(a.oScroll.sX!==\"\"){b.style.width=\"\";if(i(b).width()<g.offsetWidth)b.style.width=t(g.offsetWidth)}else if(a.oScroll.sY!==\"\")b.style.width=t(g.offsetWidth);else if(k)b.style.width=t(k);b.style.visibility=\"hidden\";cb(a,\n"
"b);e=i(\"tbody tr:eq(0)\",b).children();if(e.length===0)e=Z(a,i(\"thead\",b)[0]);if(a.oScroll.sX!==\"\"){for(f=d=g=0;f<a.aoColumns.length;f++)if(a.aoColumns[f].bVisible){g+=a.aoColumns[f].sWidthOrig===null?i(e[d]).outerWidth():parseInt(a.aoColumns[f].sWidth.replace(\"px\",\"\"),10)+(i(e[d]).outerWidth()-i(e[d]).width());d++}b.style.width=t(g);a.nTable.style.width=t(g)}for(f=d=0;f<a.aoColumns.length;f++)if(a.aoColumns[f].bVisible){g=i(e[d]).width();if(g!==null&&g>0)a.aoColumns[f].sWidth=t(g);d++}e=i(b).css(\"width\");\n"
"a.nTable.style.width=e.indexOf(\"%\")!==-1?e:t(i(b).outerWidth());b.parentNode.removeChild(b)}if(k)a.nTable.style.width=t(k)}function cb(a,b){if(a.oScroll.sX===\"\"&&a.oScroll.sY!==\"\"){i(b).width();b.style.width=t(i(b).outerWidth()-a.oScroll.iBarWidth)}else if(a.oScroll.sX!==\"\")b.style.width=t(i(b).outerWidth())}function bb(a,b){var c=db(a,b);if(c<0)return null;if(a.aoData[c].nTr===null){var d=s.createElement(\"td\");d.innerHTML=F(a,c,b,\"\");return d}return W(a,c)[b]}function db(a,b){for(var c=-1,d=-1,e=\n"
"0;e<a.aoData.length;e++){var f=F(a,e,b,\"display\")+\"\";f=f.replace(/<.*?>/g,\"\");if(f.length>c){c=f.length;d=e}}return d}function t(a){if(a===null)return\"0px\";if(typeof a==\"number\"){if(a<0)return\"0px\";return a+\"px\"}var b=a.charCodeAt(a.length-1);if(b<48||b>57)return a;return a+\"px\"}function eb(){var a=s.createElement(\"p\"),b=a.style;b.width=\"100%\";b.height=\"200px\";b.padding=\"0px\";var c=s.createElement(\"div\");b=c.style;b.position=\"absolute\";b.top=\"0px\";b.left=\"0px\";b.visibility=\"hidden\";b.width=\"200px\";\n"
"b.height=\"150px\";b.padding=\"0px\";b.overflow=\"hidden\";c.appendChild(a);s.body.appendChild(c);b=a.offsetWidth;c.style.overflow=\"scroll\";a=a.offsetWidth;if(b==a)a=c.clientWidth;s.body.removeChild(c);return b-a}function $(a,b){var c,d,e,f,g,j,k=[],m=[],u=l.ext.oSort,x=a.aoData,y=a.aoColumns,B=a.oLanguage.oAria;if(!a.oFeatures.bServerSide&&(a.aaSorting.length!==0||a.aaSortingFixed!==null)){k=a.aaSortingFixed!==null?a.aaSortingFixed.concat(a.aaSorting):a.aaSorting.slice();for(c=0;c<k.length;c++){d=k[c][0];\n"
"e=w(a,d);f=a.aoColumns[d].sSortDataType;if(l.ext.afnSortData[f]){g=l.ext.afnSortData[f].call(a.oInstance,a,d,e);if(g.length===x.length){e=0;for(f=x.length;e<f;e++)S(a,e,d,g[e])}else O(a,0,\"Returned data sort array (col \"+d+\") is the wrong length\")}}c=0;for(d=a.aiDisplayMaster.length;c<d;c++)m[a.aiDisplayMaster[c]]=c;var T=k.length,M;c=0;for(d=x.length;c<d;c++)for(e=0;e<T;e++){M=y[k[e][0]].aDataSort;g=0;for(j=M.length;g<j;g++){f=y[M[g]].sType;f=u[(f?f:\"string\")+\"-pre\"];x[c]._aSortData[M[g]]=f?f(F(a,\n"
"c,M[g],\"sort\")):F(a,c,M[g],\"sort\")}}a.aiDisplayMaster.sort(function(L,ja){var U,z,Q,aa,ka;for(U=0;U<T;U++){ka=y[k[U][0]].aDataSort;z=0;for(Q=ka.length;z<Q;z++){aa=y[ka[z]].sType;aa=u[(aa?aa:\"string\")+\"-\"+k[U][1]](x[L]._aSortData[ka[z]],x[ja]._aSortData[ka[z]]);if(aa!==0)return aa}}return u[\"numeric-asc\"](m[L],m[ja])})}if((b===p||b)&&!a.oFeatures.bDeferRender)ba(a);c=0;for(d=a.aoColumns.length;c<d;c++){e=y[c].sTitle.replace(/<.*?>/g,\"\");b=y[c].nTh;b.removeAttribute(\"aria-sort\");b.removeAttribute(\"aria-label\");\n"
"if(y[c].bSortable)if(k.length>0&&k[0][0]==c){b.setAttribute(\"aria-sort\",k[0][1]==\"asc\"?\"ascending\":\"descending\");b.setAttribute(\"aria-label\",e+((y[c].asSorting[k[0][2]+1]?y[c].asSorting[k[0][2]+1]:y[c].asSorting[0])==\"asc\"?B.sSortAscending:B.sSortDescending))}else b.setAttribute(\"aria-label\",e+(y[c].asSorting[0]==\"asc\"?B.sSortAscending:B.sSortDescending));else b.setAttribute(\"aria-label\",e)}a.bSorted=true;i(a.oInstance).trigger(\"sort\",a);if(a.oFeatures.bFilter)X(a,a.oPreviousSearch,1);else{a.aiDisplay=\n"
"a.aiDisplayMaster.slice();a._iDisplayStart=0;I(a);H(a)}}function ya(a,b,c,d){fb(b,{},function(e){if(a.aoColumns[c].bSortable!==false){var f=function(){var g,j;if(e.shiftKey){for(var k=false,m=0;m<a.aaSorting.length;m++)if(a.aaSorting[m][0]==c){k=true;g=a.aaSorting[m][0];j=a.aaSorting[m][2]+1;if(a.aoColumns[g].asSorting[j]){a.aaSorting[m][1]=a.aoColumns[g].asSorting[j];a.aaSorting[m][2]=j}else a.aaSorting.splice(m,1);break}k===false&&a.aaSorting.push([c,a.aoColumns[c].asSorting[0],0])}else if(a.aaSorting.length==\n"
"1&&a.aaSorting[0][0]==c){g=a.aaSorting[0][0];j=a.aaSorting[0][2]+1;a.aoColumns[g].asSorting[j]||(j=0);a.aaSorting[0][1]=a.aoColumns[g].asSorting[j];a.aaSorting[0][2]=j}else{a.aaSorting.splice(0,a.aaSorting.length);a.aaSorting.push([c,a.aoColumns[c].asSorting[0],0])}$(a)};if(a.oFeatures.bProcessing){P(a,true);setTimeout(function(){f();a.oFeatures.bServerSide||P(a,false)},0)}else f();typeof d==\"function\"&&d(a)}})}function ba(a){var b,c,d,e,f,g=a.aoColumns.length,j=a.oClasses;for(b=0;b<g;b++)a.aoColumns[b].bSortable&&\n"
"i(a.aoColumns[b].nTh).removeClass(j.sSortAsc+\" \"+j.sSortDesc+\" \"+a.aoColumns[b].sSortingClass);c=a.aaSortingFixed!==null?a.aaSortingFixed.concat(a.aaSorting):a.aaSorting.slice();for(b=0;b<a.aoColumns.length;b++)if(a.aoColumns[b].bSortable){f=a.aoColumns[b].sSortingClass;e=-1;for(d=0;d<c.length;d++)if(c[d][0]==b){f=c[d][1]==\"asc\"?j.sSortAsc:j.sSortDesc;e=d;break}i(a.aoColumns[b].nTh).addClass(f);if(a.bJUI){f=i(\"span.\"+j.sSortIcon,a.aoColumns[b].nTh);f.removeClass(j.sSortJUIAsc+\" \"+j.sSortJUIDesc+\" \"+\n"
"j.sSortJUI+\" \"+j.sSortJUIAscAllowed+\" \"+j.sSortJUIDescAllowed);f.addClass(e==-1?a.aoColumns[b].sSortingClassJUI:c[e][1]==\"asc\"?j.sSortJUIAsc:j.sSortJUIDesc)}}else i(a.aoColumns[b].nTh).addClass(a.aoColumns[b].sSortingClass);f=j.sSortColumn;if(a.oFeatures.bSort&&a.oFeatures.bSortClasses){a=W(a);e=[];for(b=0;b<g;b++)e.push(\"\");b=0;for(d=1;b<c.length;b++){j=parseInt(c[b][0],10);e[j]=f+d;d<3&&d++}f=new RegExp(f+\"[123]\");var k;b=0;for(c=a.length;b<c;b++){j=b%g;d=a[b].className;k=e[j];j=d.replace(f,k);\n"
"if(j!=d)a[b].className=i.trim(j);else if(k.length>0&&d.indexOf(k)==-1)a[b].className=d+\" \"+k}}}function Ha(a){if(!(!a.oFeatures.bStateSave||a.bDestroying)){var b,c;b=a.oScroll.bInfinite;var d={iCreate:(new Date).getTime(),iStart:b?0:a._iDisplayStart,iEnd:b?a._iDisplayLength:a._iDisplayEnd,iLength:a._iDisplayLength,aaSorting:i.extend(true,[],a.aaSorting),oSearch:i.extend(true,{},a.oPreviousSearch),aoSearchCols:i.extend(true,[],a.aoPreSearchCols),abVisCols:[]};b=0;for(c=a.aoColumns.length;b<c;b++)d.abVisCols.push(a.aoColumns[b].bVisible);\n"
"K(a,\"aoStateSaveParams\",\"stateSaveParams\",[a,d]);a.fnStateSave.call(a.oInstance,a,d)}}function gb(a,b){if(a.oFeatures.bStateSave){var c=a.fnStateLoad.call(a.oInstance,a);if(c){var d=K(a,\"aoStateLoadParams\",\"stateLoadParams\",[a,c]);if(i.inArray(false,d)===-1){a.oLoadedState=i.extend(true,{},c);a._iDisplayStart=c.iStart;a.iInitDisplayStart=c.iStart;a._iDisplayEnd=c.iEnd;a._iDisplayLength=c.iLength;a.aaSorting=c.aaSorting.slice();a.saved_aaSorting=c.aaSorting.slice();i.extend(a.oPreviousSearch,c.oSearch);\n"
"i.extend(true,a.aoPreSearchCols,c.aoSearchCols);b.saved_aoColumns=[];for(d=0;d<c.abVisCols.length;d++){b.saved_aoColumns[d]={};b.saved_aoColumns[d].bVisible=c.abVisCols[d]}K(a,\"aoStateLoaded\",\"stateLoaded\",[a,c])}}}}function lb(a,b,c,d,e){var f=new Date;f.setTime(f.getTime()+c*1E3);c=la.location.pathname.split(\"/\");a=a+\"_\"+c.pop().replace(/[\\/:]/g,\"\").toLowerCase();var g;if(e!==null){g=typeof i.parseJSON===\"function\"?i.parseJSON(b):eval(\"(\"+b+\")\");b=e(a,g,f.toGMTString(),c.join(\"/\")+\"/\")}else b=a+\n"
"\"=\"+encodeURIComponent(b)+\"; expires=\"+f.toGMTString()+\"; path=\"+c.join(\"/\")+\"/\";a=s.cookie.split(\";\");e=b.split(\";\")[0].length;f=[];if(e+s.cookie.length+10>4096){for(var j=0,k=a.length;j<k;j++)if(a[j].indexOf(d)!=-1){var m=a[j].split(\"=\");try{(g=eval(\"(\"+decodeURIComponent(m[1])+\")\"))&&g.iCreate&&f.push({name:m[0],time:g.iCreate})}catch(u){}}for(f.sort(function(x,y){return y.time-x.time});e+s.cookie.length+10>4096;){if(f.length===0)return;d=f.pop();s.cookie=d.name+\"=; expires=Thu, 01-Jan-1970 00:00:01 GMT; path=\"+\n"
"c.join(\"/\")+\"/\"}}s.cookie=b}function mb(a){var b=la.location.pathname.split(\"/\");a=a+\"_\"+b[b.length-1].replace(/[\\/:]/g,\"\").toLowerCase()+\"=\";b=s.cookie.split(\";\");for(var c=0;c<b.length;c++){for(var d=b[c];d.charAt(0)==\" \";)d=d.substring(1,d.length);if(d.indexOf(a)===0)return decodeURIComponent(d.substring(a.length,d.length))}return null}function C(a){for(var b=0;b<l.settings.length;b++)if(l.settings[b].nTable===a)return l.settings[b];return null}function fa(a){var b=[];a=a.aoData;for(var c=0,d=\n"
"a.length;c<d;c++)a[c].nTr!==null&&b.push(a[c].nTr);return b}function W(a,b){var c=[],d,e,f,g,j;e=0;var k=a.aoData.length;if(b!==p){e=b;k=b+1}for(e=e;e<k;e++){j=a.aoData[e];if(j.nTr!==null){b=[];for(d=j.nTr.firstChild;d;){f=d.nodeName.toLowerCase();if(f==\"td\"||f==\"th\")b.push(d);d=d.nextSibling}f=d=0;for(g=a.aoColumns.length;f<g;f++)if(a.aoColumns[f].bVisible)c.push(b[f-d]);else{c.push(j._anHidden[f]);d++}}}return c}function O(a,b,c){a=a===null?\"DataTables warning: \"+c:\"DataTables warning (table id = '\"+\n"
"a.sTableId+\"'): \"+c;if(b===0)if(l.ext.sErrMode==\"alert\")alert(a);else throw new Error(a);else la.console&&console.log&&console.log(a)}function r(a,b,c,d){if(d===p)d=c;if(b[c]!==p)a[d]=b[c]}function hb(a,b){var c;for(var d in b)if(b.hasOwnProperty(d)){c=b[d];if(typeof h[d]===\"object\"&&c!==null&&i.isArray(c)===false)i.extend(true,a[d],c);else a[d]=c}return a}function fb(a,b,c){i(a).bind(\"click.DT\",b,function(d){a.blur();c(d)}).bind(\"keypress.DT\",b,function(d){d.which===13&&c(d)}).bind(\"selectstart.DT\",\n"
"function(){return false})}function J(a,b,c,d){c&&a[b].push({fn:c,sName:d})}function K(a,b,c,d){b=a[b];for(var e=[],f=b.length-1;f>=0;f--)e.push(b[f].fn.apply(a.oInstance,d));c!==null&&i(a.oInstance).trigger(c,d);return e}function ib(a){var b=i('<div style=\"position:absolute; top:0; left:0; height:1px; width:1px; overflow:hidden\"><div style=\"position:absolute; top:1px; left:1px; width:100px; overflow:scroll;\"><div id=\"DT_BrowserTest\" style=\"width:100%; height:10px;\"></div></div></div>')[0];s.body.appendChild(b);\n"
"a.oBrowser.bScrollOversize=i(\"#DT_BrowserTest\",b)[0].offsetWidth===100?true:false;s.body.removeChild(b)}function jb(a){return function(){var b=[C(this[l.ext.iApiIndex])].concat(Array.prototype.slice.call(arguments));return l.ext.oApi[a].apply(this,b)}}var ga=/\\[.*?\\]$/,kb=la.JSON?JSON.stringify:function(a){var b=typeof a;if(b!==\"object\"||a===null){if(b===\"string\")a='\"'+a+'\"';return a+\"\"}var c,d,e=[],f=i.isArray(a);for(c in a){d=a[c];b=typeof d;if(b===\"string\")d='\"'+d+'\"';else if(b===\"object\"&&d!==\n"
"null)d=kb(d);e.push((f?\"\":'\"'+c+'\":')+d)}return(f?\"[\":\"{\")+e+(f?\"]\":\"}\")};this.$=function(a,b){var c,d=[],e;c=C(this[l.ext.iApiIndex]);var f=c.aoData,g=c.aiDisplay,j=c.aiDisplayMaster;b||(b={});b=i.extend({},{filter:\"none\",order:\"current\",page:\"all\"},b);if(b.page==\"current\"){b=c._iDisplayStart;for(c=c.fnDisplayEnd();b<c;b++)(e=f[g[b]].nTr)&&d.push(e)}else if(b.order==\"current\"&&b.filter==\"none\"){b=0;for(c=j.length;b<c;b++)(e=f[j[b]].nTr)&&d.push(e)}else if(b.order==\"current\"&&b.filter==\"applied\"){b=\n"
"0;for(c=g.length;b<c;b++)(e=f[g[b]].nTr)&&d.push(e)}else if(b.order==\"original\"&&b.filter==\"none\"){b=0;for(c=f.length;b<c;b++)(e=f[b].nTr)&&d.push(e)}else if(b.order==\"original\"&&b.filter==\"applied\"){b=0;for(c=f.length;b<c;b++){e=f[b].nTr;i.inArray(b,g)!==-1&&e&&d.push(e)}}else O(c,1,\"Unknown selection options\");f=i(d);d=f.filter(a);a=f.find(a);return i([].concat(i.makeArray(d),i.makeArray(a)))};this._=function(a,b){var c=[],d=this.$(a,b);a=0;for(b=d.length;a<b;a++)c.push(this.fnGetData(d[a]));return c};\n"
"this.fnAddData=function(a,b){if(a.length===0)return[];var c=[],d,e=C(this[l.ext.iApiIndex]);if(typeof a[0]===\"object\"&&a[0]!==null)for(var f=0;f<a.length;f++){d=R(e,a[f]);if(d==-1)return c;c.push(d)}else{d=R(e,a);if(d==-1)return c;c.push(d)}e.aiDisplay=e.aiDisplayMaster.slice();if(b===p||b)qa(e);return c};this.fnAdjustColumnSizing=function(a){var b=C(this[l.ext.iApiIndex]);o(b);if(a===p||a)this.fnDraw(false);else if(b.oScroll.sX!==\"\"||b.oScroll.sY!==\"\")this.oApi._fnScrollDraw(b)};this.fnClearTable=\n"
"function(a){var b=C(this[l.ext.iApiIndex]);wa(b);if(a===p||a)H(b)};this.fnClose=function(a){for(var b=C(this[l.ext.iApiIndex]),c=0;c<b.aoOpenRows.length;c++)if(b.aoOpenRows[c].nParent==a){(a=b.aoOpenRows[c].nTr.parentNode)&&a.removeChild(b.aoOpenRows[c].nTr);b.aoOpenRows.splice(c,1);return 0}return 1};this.fnDeleteRow=function(a,b,c){var d=C(this[l.ext.iApiIndex]),e,f;a=typeof a===\"object\"?V(d,a):a;var g=d.aoData.splice(a,1);e=0;for(f=d.aoData.length;e<f;e++)if(d.aoData[e].nTr!==null)d.aoData[e].nTr._DT_RowIndex=\n"
"e;e=i.inArray(a,d.aiDisplay);d.asDataSearch.splice(e,1);xa(d.aiDisplayMaster,a);xa(d.aiDisplay,a);typeof b===\"function\"&&b.call(this,d,g);if(d._iDisplayStart>=d.fnRecordsDisplay()){d._iDisplayStart-=d._iDisplayLength;if(d._iDisplayStart<0)d._iDisplayStart=0}if(c===p||c){I(d);H(d)}return g};this.fnDestroy=function(a){var b=C(this[l.ext.iApiIndex]),c=b.nTableWrapper.parentNode,d=b.nTBody,e,f;a=a===p?false:a;b.bDestroying=true;K(b,\"aoDestroyCallback\",\"destroy\",[b]);if(!a){e=0;for(f=b.aoColumns.length;e<\n"
"f;e++)b.aoColumns[e].bVisible===false&&this.fnSetColumnVis(e,true)}i(b.nTableWrapper).find(\"*\").andSelf().unbind(\".DT\");i(\"tbody>tr>td.\"+b.oClasses.sRowEmpty,b.nTable).parent().remove();if(b.nTable!=b.nTHead.parentNode){i(b.nTable).children(\"thead\").remove();b.nTable.appendChild(b.nTHead)}if(b.nTFoot&&b.nTable!=b.nTFoot.parentNode){i(b.nTable).children(\"tfoot\").remove();b.nTable.appendChild(b.nTFoot)}b.nTable.parentNode.removeChild(b.nTable);i(b.nTableWrapper).remove();b.aaSorting=[];b.aaSortingFixed=\n"
"[];ba(b);i(fa(b)).removeClass(b.asStripeClasses.join(\" \"));i(\"th, td\",b.nTHead).removeClass([b.oClasses.sSortable,b.oClasses.sSortableAsc,b.oClasses.sSortableDesc,b.oClasses.sSortableNone].join(\" \"));if(b.bJUI){i(\"th span.\"+b.oClasses.sSortIcon+\", td span.\"+b.oClasses.sSortIcon,b.nTHead).remove();i(\"th, td\",b.nTHead).each(function(){var g=i(\"div.\"+b.oClasses.sSortJUIWrapper,this),j=g.contents();i(this).append(j);g.remove()})}if(!a&&b.nTableReinsertBefore)c.insertBefore(b.nTable,b.nTableReinsertBefore);\n"
"else a||c.appendChild(b.nTable);e=0;for(f=b.aoData.length;e<f;e++)b.aoData[e].nTr!==null&&d.appendChild(b.aoData[e].nTr);if(b.oFeatures.bAutoWidth===true)b.nTable.style.width=t(b.sDestroyWidth);if(f=b.asDestroyStripes.length){a=i(d).children(\"tr\");for(e=0;e<f;e++)a.filter(\":nth-child(\"+f+\"n + \"+e+\")\").addClass(b.asDestroyStripes[e])}e=0;for(f=l.settings.length;e<f;e++)l.settings[e]==b&&l.settings.splice(e,1);h=b=null};this.fnDraw=function(a){var b=C(this[l.ext.iApiIndex]);if(a===false){I(b);H(b)}else qa(b)};\n"
"this.fnFilter=function(a,b,c,d,e,f){var g=C(this[l.ext.iApiIndex]);if(g.oFeatures.bFilter){if(c===p||c===null)c=false;if(d===p||d===null)d=true;if(e===p||e===null)e=true;if(f===p||f===null)f=true;if(b===p||b===null){X(g,{sSearch:a+\"\",bRegex:c,bSmart:d,bCaseInsensitive:f},1);if(e&&g.aanFeatures.f){b=g.aanFeatures.f;c=0;for(d=b.length;c<d;c++)try{b[c]._DT_Input!=s.activeElement&&i(b[c]._DT_Input).val(a)}catch(j){i(b[c]._DT_Input).val(a)}}}else{i.extend(g.aoPreSearchCols[b],{sSearch:a+\"\",bRegex:c,bSmart:d,\n"
"bCaseInsensitive:f});X(g,g.oPreviousSearch,1)}}};this.fnGetData=function(a,b){var c=C(this[l.ext.iApiIndex]);if(a!==p){var d=a;if(typeof a===\"object\"){var e=a.nodeName.toLowerCase();if(e===\"tr\")d=V(c,a);else if(e===\"td\"){d=V(c,a.parentNode);b=va(c,d,a)}}if(b!==p)return F(c,d,b,\"\");return c.aoData[d]!==p?c.aoData[d]._aData:null}return oa(c)};this.fnGetNodes=function(a){var b=C(this[l.ext.iApiIndex]);if(a!==p)return b.aoData[a]!==p?b.aoData[a].nTr:null;return fa(b)};this.fnGetPosition=function(a){var b=\n"
"C(this[l.ext.iApiIndex]),c=a.nodeName.toUpperCase();if(c==\"TR\")return V(b,a);else if(c==\"TD\"||c==\"TH\"){c=V(b,a.parentNode);a=va(b,c,a);return[c,w(b,a),a]}return null};this.fnIsOpen=function(a){for(var b=C(this[l.ext.iApiIndex]),c=0;c<b.aoOpenRows.length;c++)if(b.aoOpenRows[c].nParent==a)return true;return false};this.fnOpen=function(a,b,c){var d=C(this[l.ext.iApiIndex]),e=fa(d);if(i.inArray(a,e)!==-1){this.fnClose(a);e=s.createElement(\"tr\");var f=s.createElement(\"td\");e.appendChild(f);f.className=\n"
"c;f.colSpan=D(d);if(typeof b===\"string\")f.innerHTML=b;else i(f).html(b);b=i(\"tr\",d.nTBody);i.inArray(a,b)!=-1&&i(e).insertAfter(a);d.aoOpenRows.push({nTr:e,nParent:a});return e}};this.fnPageChange=function(a,b){var c=C(this[l.ext.iApiIndex]);Ga(c,a);I(c);if(b===p||b)H(c)};this.fnSetColumnVis=function(a,b,c){var d=C(this[l.ext.iApiIndex]),e,f,g=d.aoColumns,j=d.aoData,k,m;if(g[a].bVisible!=b){if(b){for(e=f=0;e<a;e++)g[e].bVisible&&f++;m=f>=D(d);if(!m)for(e=a;e<g.length;e++)if(g[e].bVisible){k=e;break}e=\n"
"0;for(f=j.length;e<f;e++)if(j[e].nTr!==null)m?j[e].nTr.appendChild(j[e]._anHidden[a]):j[e].nTr.insertBefore(j[e]._anHidden[a],W(d,e)[k])}else{e=0;for(f=j.length;e<f;e++)if(j[e].nTr!==null){k=W(d,e)[a];j[e]._anHidden[a]=k;k.parentNode.removeChild(k)}}g[a].bVisible=b;ia(d,d.aoHeader);d.nTFoot&&ia(d,d.aoFooter);e=0;for(f=d.aoOpenRows.length;e<f;e++)d.aoOpenRows[e].nTr.colSpan=D(d);if(c===p||c){o(d);H(d)}Ha(d)}};this.fnSettings=function(){return C(this[l.ext.iApiIndex])};this.fnSort=function(a){var b=\n"
"C(this[l.ext.iApiIndex]);b.aaSorting=a;$(b)};this.fnSortListener=function(a,b,c){ya(C(this[l.ext.iApiIndex]),a,b,c)};this.fnUpdate=function(a,b,c,d,e){var f=C(this[l.ext.iApiIndex]);b=typeof b===\"object\"?V(f,b):b;if(i.isArray(a)&&c===p){f.aoData[b]._aData=a.slice();for(c=0;c<f.aoColumns.length;c++)this.fnUpdate(F(f,b,c),b,c,false,false)}else if(i.isPlainObject(a)&&c===p){f.aoData[b]._aData=i.extend(true,{},a);for(c=0;c<f.aoColumns.length;c++)this.fnUpdate(F(f,b,c),b,c,false,false)}else{S(f,b,c,a);\n"
"a=F(f,b,c,\"display\");var g=f.aoColumns[c];if(g.fnRender!==null){a=da(f,b,c);g.bUseRendered&&S(f,b,c,a)}if(f.aoData[b].nTr!==null)W(f,b)[c].innerHTML=a}c=i.inArray(b,f.aiDisplay);f.asDataSearch[c]=Da(f,na(f,b,\"filter\",A(f,\"bSearchable\")));if(e===p||e)o(f);if(d===p||d)qa(f);return 0};this.fnVersionCheck=l.ext.fnVersionCheck;this.oApi={_fnExternApiFunc:jb,_fnInitialise:ra,_fnInitComplete:pa,_fnLanguageCompat:Fa,_fnAddColumn:n,_fnColumnOptions:q,_fnAddData:R,_fnCreateTr:ua,_fnGatherData:ea,_fnBuildHead:Ka,\n"
"_fnDrawHead:ia,_fnDraw:H,_fnReDraw:qa,_fnAjaxUpdate:La,_fnAjaxParameters:Ta,_fnAjaxUpdateDraw:Ua,_fnServerParams:Aa,_fnAddOptionsHtml:Ma,_fnFeatureHtmlTable:Qa,_fnScrollDraw:$a,_fnAdjustColumnSizing:o,_fnFeatureHtmlFilter:Oa,_fnFilterComplete:X,_fnFilterCustom:Xa,_fnFilterColumn:Wa,_fnFilter:Va,_fnBuildSearchArray:Ba,_fnBuildSearchRow:Da,_fnFilterCreateSearch:Ca,_fnDataToSearch:Ya,_fnSort:$,_fnSortAttachListener:ya,_fnSortingClasses:ba,_fnFeatureHtmlPaginate:Sa,_fnPageChange:Ga,_fnFeatureHtmlInfo:Ra,\n"
"_fnUpdateInfo:Za,_fnFeatureHtmlLength:Na,_fnFeatureHtmlProcessing:Pa,_fnProcessingDisplay:P,_fnVisibleToColumnIndex:v,_fnColumnIndexToVisible:w,_fnNodeToDataIndex:V,_fnVisbleColumns:D,_fnCalculateEnd:I,_fnConvertToWidth:ab,_fnCalculateColumnWidths:ta,_fnScrollingWidthAdjust:cb,_fnGetWidestNode:bb,_fnGetMaxLenString:db,_fnStringToCss:t,_fnDetectType:G,_fnSettingsFromNode:C,_fnGetDataMaster:oa,_fnGetTrNodes:fa,_fnGetTdNodes:W,_fnEscapeRegex:Ea,_fnDeleteIndex:xa,_fnReOrderIndex:E,_fnColumnOrdering:Y,\n"
"_fnLog:O,_fnClearTable:wa,_fnSaveState:Ha,_fnLoadState:gb,_fnCreateCookie:lb,_fnReadCookie:mb,_fnDetectHeader:ha,_fnGetUniqueThs:Z,_fnScrollBarWidth:eb,_fnApplyToChildren:N,_fnMap:r,_fnGetRowData:na,_fnGetCellData:F,_fnSetCellData:S,_fnGetObjectDataFn:ca,_fnSetObjectDataFn:Ja,_fnApplyColumnDefs:ma,_fnBindAction:fb,_fnExtend:hb,_fnCallbackReg:J,_fnCallbackFire:K,_fnJsonString:kb,_fnRender:da,_fnNodeToColumnIndex:va,_fnInfoMacros:za,_fnBrowserDetect:ib,_fnGetColumns:A};i.extend(l.ext.oApi,this.oApi);\n"
"for(var Ia in l.ext.oApi)if(Ia)this[Ia]=jb(Ia);var sa=this;this.each(function(){var a=0,b,c,d;c=this.getAttribute(\"id\");var e=false,f=false;if(this.nodeName.toLowerCase()!=\"table\")O(null,0,\"Attempted to initialise DataTables on a node which is not a table: \"+this.nodeName);else{a=0;for(b=l.settings.length;a<b;a++){if(l.settings[a].nTable==this)if(h===p||h.bRetrieve)return l.settings[a].oInstance;else if(h.bDestroy){l.settings[a].oInstance.fnDestroy();break}else{O(l.settings[a],0,\"Cannot reinitialise DataTable.\\n\\nTo retrieve the DataTables object for this table, pass no arguments or see the docs for bRetrieve and bDestroy\");\n"
"return}if(l.settings[a].sTableId==this.id){l.settings.splice(a,1);break}}if(c===null||c===\"\")this.id=c=\"DataTables_Table_\"+l.ext._oExternConfig.iNextUnique++;var g=i.extend(true,{},l.models.oSettings,{nTable:this,oApi:sa.oApi,oInit:h,sDestroyWidth:i(this).width(),sInstance:c,sTableId:c});l.settings.push(g);g.oInstance=sa.length===1?sa:i(this).dataTable();h||(h={});h.oLanguage&&Fa(h.oLanguage);h=hb(i.extend(true,{},l.defaults),h);r(g.oFeatures,h,\"bPaginate\");r(g.oFeatures,h,\"bLengthChange\");r(g.oFeatures,\n"
"h,\"bFilter\");r(g.oFeatures,h,\"bSort\");r(g.oFeatures,h,\"bInfo\");r(g.oFeatures,h,\"bProcessing\");r(g.oFeatures,h,\"bAutoWidth\");r(g.oFeatures,h,\"bSortClasses\");r(g.oFeatures,h,\"bServerSide\");r(g.oFeatures,h,\"bDeferRender\");r(g.oScroll,h,\"sScrollX\",\"sX\");r(g.oScroll,h,\"sScrollXInner\",\"sXInner\");r(g.oScroll,h,\"sScrollY\",\"sY\");r(g.oScroll,h,\"bScrollCollapse\",\"bCollapse\");r(g.oScroll,h,\"bScrollInfinite\",\"bInfinite\");r(g.oScroll,h,\"iScrollLoadGap\",\"iLoadGap\");r(g.oScroll,h,\"bScrollAutoCss\",\"bAutoCss\");r(g,\n"
"h,\"asStripeClasses\");r(g,h,\"asStripClasses\",\"asStripeClasses\");r(g,h,\"fnServerData\");r(g,h,\"fnFormatNumber\");r(g,h,\"sServerMethod\");r(g,h,\"aaSorting\");r(g,h,\"aaSortingFixed\");r(g,h,\"aLengthMenu\");r(g,h,\"sPaginationType\");r(g,h,\"sAjaxSource\");r(g,h,\"sAjaxDataProp\");r(g,h,\"iCookieDuration\");r(g,h,\"sCookiePrefix\");r(g,h,\"sDom\");r(g,h,\"bSortCellsTop\");r(g,h,\"iTabIndex\");r(g,h,\"oSearch\",\"oPreviousSearch\");r(g,h,\"aoSearchCols\",\"aoPreSearchCols\");r(g,h,\"iDisplayLength\",\"_iDisplayLength\");r(g,h,\"bJQueryUI\",\n"
"\"bJUI\");r(g,h,\"fnCookieCallback\");r(g,h,\"fnStateLoad\");r(g,h,\"fnStateSave\");r(g.oLanguage,h,\"fnInfoCallback\");J(g,\"aoDrawCallback\",h.fnDrawCallback,\"user\");J(g,\"aoServerParams\",h.fnServerParams,\"user\");J(g,\"aoStateSaveParams\",h.fnStateSaveParams,\"user\");J(g,\"aoStateLoadParams\",h.fnStateLoadParams,\"user\");J(g,\"aoStateLoaded\",h.fnStateLoaded,\"user\");J(g,\"aoRowCallback\",h.fnRowCallback,\"user\");J(g,\"aoRowCreatedCallback\",h.fnCreatedRow,\"user\");J(g,\"aoHeaderCallback\",h.fnHeaderCallback,\"user\");J(g,\"aoFooterCallback\",\n"
"h.fnFooterCallback,\"user\");J(g,\"aoInitComplete\",h.fnInitComplete,\"user\");J(g,\"aoPreDrawCallback\",h.fnPreDrawCallback,\"user\");if(g.oFeatures.bServerSide&&g.oFeatures.bSort&&g.oFeatures.bSortClasses)J(g,\"aoDrawCallback\",ba,\"server_side_sort_classes\");else g.oFeatures.bDeferRender&&J(g,\"aoDrawCallback\",ba,\"defer_sort_classes\");if(h.bJQueryUI){i.extend(g.oClasses,l.ext.oJUIClasses);if(h.sDom===l.defaults.sDom&&l.defaults.sDom===\"lfrtip\")g.sDom='<\"H\"lfr>t<\"F\"ip>'}else i.extend(g.oClasses,l.ext.oStdClasses);\n"
"i(this).addClass(g.oClasses.sTable);if(g.oScroll.sX!==\"\"||g.oScroll.sY!==\"\")g.oScroll.iBarWidth=eb();if(g.iInitDisplayStart===p){g.iInitDisplayStart=h.iDisplayStart;g._iDisplayStart=h.iDisplayStart}if(h.bStateSave){g.oFeatures.bStateSave=true;gb(g,h);J(g,\"aoDrawCallback\",Ha,\"state_save\")}if(h.iDeferLoading!==null){g.bDeferLoading=true;a=i.isArray(h.iDeferLoading);g._iRecordsDisplay=a?h.iDeferLoading[0]:h.iDeferLoading;g._iRecordsTotal=a?h.iDeferLoading[1]:h.iDeferLoading}if(h.aaData!==null)f=true;\n"
"if(h.oLanguage.sUrl!==\"\"){g.oLanguage.sUrl=h.oLanguage.sUrl;i.getJSON(g.oLanguage.sUrl,null,function(k){Fa(k);i.extend(true,g.oLanguage,h.oLanguage,k);ra(g)});e=true}else i.extend(true,g.oLanguage,h.oLanguage);if(h.asStripeClasses===null)g.asStripeClasses=[g.oClasses.sStripeOdd,g.oClasses.sStripeEven];b=g.asStripeClasses.length;g.asDestroyStripes=[];if(b){c=false;d=i(this).children(\"tbody\").children(\"tr:lt(\"+b+\")\");for(a=0;a<b;a++)if(d.hasClass(g.asStripeClasses[a])){c=true;g.asDestroyStripes.push(g.asStripeClasses[a])}c&&\n"
"d.removeClass(g.asStripeClasses.join(\" \"))}c=[];a=this.getElementsByTagName(\"thead\");if(a.length!==0){ha(g.aoHeader,a[0]);c=Z(g)}if(h.aoColumns===null){d=[];a=0;for(b=c.length;a<b;a++)d.push(null)}else d=h.aoColumns;a=0;for(b=d.length;a<b;a++){if(h.saved_aoColumns!==p&&h.saved_aoColumns.length==b){if(d[a]===null)d[a]={};d[a].bVisible=h.saved_aoColumns[a].bVisible}n(g,c?c[a]:null)}ma(g,h.aoColumnDefs,d,function(k,m){q(g,k,m)});a=0;for(b=g.aaSorting.length;a<b;a++){if(g.aaSorting[a][0]>=g.aoColumns.length)g.aaSorting[a][0]=\n"
"0;var j=g.aoColumns[g.aaSorting[a][0]];if(g.aaSorting[a][2]===p)g.aaSorting[a][2]=0;if(h.aaSorting===p&&g.saved_aaSorting===p)g.aaSorting[a][1]=j.asSorting[0];c=0;for(d=j.asSorting.length;c<d;c++)if(g.aaSorting[a][1]==j.asSorting[c]){g.aaSorting[a][2]=c;break}}ba(g);ib(g);a=i(this).children(\"caption\").each(function(){this._captionSide=i(this).css(\"caption-side\")});b=i(this).children(\"thead\");if(b.length===0){b=[s.createElement(\"thead\")];this.appendChild(b[0])}g.nTHead=b[0];b=i(this).children(\"tbody\");\n"
"if(b.length===0){b=[s.createElement(\"tbody\")];this.appendChild(b[0])}g.nTBody=b[0];g.nTBody.setAttribute(\"role\",\"alert\");g.nTBody.setAttribute(\"aria-live\",\"polite\");g.nTBody.setAttribute(\"aria-relevant\",\"all\");b=i(this).children(\"tfoot\");if(b.length===0&&a.length>0&&(g.oScroll.sX!==\"\"||g.oScroll.sY!==\"\")){b=[s.createElement(\"tfoot\")];this.appendChild(b[0])}if(b.length>0){g.nTFoot=b[0];ha(g.aoFooter,g.nTFoot)}if(f)for(a=0;a<h.aaData.length;a++)R(g,h.aaData[a]);else ea(g);g.aiDisplay=g.aiDisplayMaster.slice();\n"
"g.bInitialised=true;e===false&&ra(g)}});sa=null;return this};l.fnVersionCheck=function(h){var n=function(A,G){for(;A.length<G;)A+=\"0\";return A},q=l.ext.sVersion.split(\".\");h=h.split(\".\");for(var o=\"\",v=\"\",w=0,D=h.length;w<D;w++){o+=n(q[w],3);v+=n(h[w],3)}return parseInt(o,10)>=parseInt(v,10)};l.fnIsDataTable=function(h){for(var n=l.settings,q=0;q<n.length;q++)if(n[q].nTable===h||n[q].nScrollHead===h||n[q].nScrollFoot===h)return true;return false};l.fnTables=function(h){var n=[];jQuery.each(l.settings,\n"
"function(q,o){if(!h||h===true&&i(o.nTable).is(\":visible\"))n.push(o.nTable)});return n};l.version=\"1.9.4\";l.settings=[];l.models={};l.models.ext={afnFiltering:[],afnSortData:[],aoFeatures:[],aTypes:[],fnVersionCheck:l.fnVersionCheck,iApiIndex:0,ofnSearch:{},oApi:{},oStdClasses:{},oJUIClasses:{},oPagination:{},oSort:{},sVersion:l.version,sErrMode:\"alert\",_oExternConfig:{iNextUnique:0}};l.models.oSearch={bCaseInsensitive:true,sSearch:\"\",bRegex:false,bSmart:true};l.models.oRow={nTr:null,_aData:[],_aSortData:[],\n"
"_anHidden:[],_sRowStripe:\"\"};l.models.oColumn={aDataSort:null,asSorting:null,bSearchable:null,bSortable:null,bUseRendered:null,bVisible:null,_bAutoType:true,fnCreatedCell:null,fnGetData:null,fnRender:null,fnSetData:null,mData:null,mRender:null,nTh:null,nTf:null,sClass:null,sContentPadding:null,sDefaultContent:null,sName:null,sSortDataType:\"std\",sSortingClass:null,sSortingClassJUI:null,sTitle:null,sType:null,sWidth:null,sWidthOrig:null};l.defaults={aaData:null,aaSorting:[[0,\"asc\"]],aaSortingFixed:null,\n"
"aLengthMenu:[10,25,50,100],aoColumns:null,aoColumnDefs:null,aoSearchCols:[],asStripeClasses:null,bAutoWidth:true,bDeferRender:false,bDestroy:false,bFilter:true,bInfo:true,bJQueryUI:false,bLengthChange:true,bPaginate:true,bProcessing:false,bRetrieve:false,bScrollAutoCss:true,bScrollCollapse:false,bScrollInfinite:false,bServerSide:false,bSort:true,bSortCellsTop:false,bSortClasses:true,bStateSave:false,fnCookieCallback:null,fnCreatedRow:null,fnDrawCallback:null,fnFooterCallback:null,fnFormatNumber:function(h){if(h<\n"
"1E3)return h;var n=h+\"\";h=n.split(\"\");var q=\"\";n=n.length;for(var o=0;o<n;o++){if(o%3===0&&o!==0)q=this.oLanguage.sInfoThousands+q;q=h[n-o-1]+q}return q},fnHeaderCallback:null,fnInfoCallback:null,fnInitComplete:null,fnPreDrawCallback:null,fnRowCallback:null,fnServerData:function(h,n,q,o){o.jqXHR=i.ajax({url:h,data:n,success:function(v){v.sError&&o.oApi._fnLog(o,0,v.sError);i(o.oInstance).trigger(\"xhr\",[o,v]);q(v)},dataType:\"json\",cache:false,type:o.sServerMethod,error:function(v,w){w==\"parsererror\"&&\n"
"o.oApi._fnLog(o,0,\"DataTables warning: JSON data from server could not be parsed. This is caused by a JSON formatting error.\")}})},fnServerParams:null,fnStateLoad:function(h){h=this.oApi._fnReadCookie(h.sCookiePrefix+h.sInstance);var n;try{n=typeof i.parseJSON===\"function\"?i.parseJSON(h):eval(\"(\"+h+\")\")}catch(q){n=null}return n},fnStateLoadParams:null,fnStateLoaded:null,fnStateSave:function(h,n){this.oApi._fnCreateCookie(h.sCookiePrefix+h.sInstance,this.oApi._fnJsonString(n),h.iCookieDuration,h.sCookiePrefix,\n"
"h.fnCookieCallback)},fnStateSaveParams:null,iCookieDuration:7200,iDeferLoading:null,iDisplayLength:10,iDisplayStart:0,iScrollLoadGap:100,iTabIndex:0,oLanguage:{oAria:{sSortAscending:\": activate to sort column ascending\",sSortDescending:\": activate to sort column descending\"},oPaginate:{sFirst:\"First\",sLast:\"Last\",sNext:\"Next\",sPrevious:\"Previous\"},sEmptyTable:\"No data available in table\",sInfo:\"Showing _START_ to _END_ of _TOTAL_ entries\",sInfoEmpty:\"Showing 0 to 0 of 0 entries\",sInfoFiltered:\"(filtered from _MAX_ total entries)\",\n"
"sInfoPostFix:\"\",sInfoThousands:\",\",sLengthMenu:\"Show _MENU_ entries\",sLoadingRecords:\"Loading...\",sProcessing:\"Processing...\",sSearch:\"Search:\",sUrl:\"\",sZeroRecords:\"No matching records found\"},oSearch:i.extend({},l.models.oSearch),sAjaxDataProp:\"aaData\",sAjaxSource:null,sCookiePrefix:\"SpryMedia_DataTables_\",sDom:\"lfrtip\",sPaginationType:\"two_button\",sScrollX:\"\",sScrollXInner:\"\",sScrollY:\"\",sServerMethod:\"GET\"};l.defaults.columns={aDataSort:null,asSorting:[\"asc\",\"desc\"],bSearchable:true,bSortable:true,\n"
"bUseRendered:true,bVisible:true,fnCreatedCell:null,fnRender:null,iDataSort:-1,mData:null,mRender:null,sCellType:\"td\",sClass:\"\",sContentPadding:\"\",sDefaultContent:null,sName:\"\",sSortDataType:\"std\",sTitle:null,sType:null,sWidth:null};l.models.oSettings={oFeatures:{bAutoWidth:null,bDeferRender:null,bFilter:null,bInfo:null,bLengthChange:null,bPaginate:null,bProcessing:null,bServerSide:null,bSort:null,bSortClasses:null,bStateSave:null},oScroll:{bAutoCss:null,bCollapse:null,bInfinite:null,iBarWidth:0,iLoadGap:null,\n"
"sX:null,sXInner:null,sY:null},oLanguage:{fnInfoCallback:null},oBrowser:{bScrollOversize:false},aanFeatures:[],aoData:[],aiDisplay:[],aiDisplayMaster:[],aoColumns:[],aoHeader:[],aoFooter:[],asDataSearch:[],oPreviousSearch:{},aoPreSearchCols:[],aaSorting:null,aaSortingFixed:null,asStripeClasses:null,asDestroyStripes:[],sDestroyWidth:0,aoRowCallback:[],aoHeaderCallback:[],aoFooterCallback:[],aoDrawCallback:[],aoRowCreatedCallback:[],aoPreDrawCallback:[],aoInitComplete:[],aoStateSaveParams:[],aoStateLoadParams:[],\n"
"aoStateLoaded:[],sTableId:\"\",nTable:null,nTHead:null,nTFoot:null,nTBody:null,nTableWrapper:null,bDeferLoading:false,bInitialised:false,aoOpenRows:[],sDom:null,sPaginationType:\"two_button\",iCookieDuration:0,sCookiePrefix:\"\",fnCookieCallback:null,aoStateSave:[],aoStateLoad:[],oLoadedState:null,sAjaxSource:null,sAjaxDataProp:null,bAjaxDataGet:true,jqXHR:null,fnServerData:null,aoServerParams:[],sServerMethod:null,fnFormatNumber:null,aLengthMenu:null,iDraw:0,bDrawing:false,iDrawError:-1,_iDisplayLength:10,\n"
"_iDisplayStart:0,_iDisplayEnd:10,_iRecordsTotal:0,_iRecordsDisplay:0,bJUI:null,oClasses:{},bFiltered:false,bSorted:false,bSortCellsTop:null,oInit:null,aoDestroyCallback:[],fnRecordsTotal:function(){return this.oFeatures.bServerSide?parseInt(this._iRecordsTotal,10):this.aiDisplayMaster.length},fnRecordsDisplay:function(){return this.oFeatures.bServerSide?parseInt(this._iRecordsDisplay,10):this.aiDisplay.length},fnDisplayEnd:function(){return this.oFeatures.bServerSide?this.oFeatures.bPaginate===false||\n"
"this._iDisplayLength==-1?this._iDisplayStart+this.aiDisplay.length:Math.min(this._iDisplayStart+this._iDisplayLength,this._iRecordsDisplay):this._iDisplayEnd},oInstance:null,sInstance:null,iTabIndex:0,nScrollHead:null,nScrollFoot:null};l.ext=i.extend(true,{},l.models.ext);i.extend(l.ext.oStdClasses,{sTable:\"dataTable\",sPagePrevEnabled:\"paginate_enabled_previous\",sPagePrevDisabled:\"paginate_disabled_previous\",sPageNextEnabled:\"paginate_enabled_next\",sPageNextDisabled:\"paginate_disabled_next\",sPageJUINext:\"\",\n"
"sPageJUIPrev:\"\",sPageButton:\"paginate_button\",sPageButtonActive:\"paginate_active\",sPageButtonStaticDisabled:\"paginate_button paginate_button_disabled\",sPageFirst:\"first\",sPagePrevious:\"previous\",sPageNext:\"next\",sPageLast:\"last\",sStripeOdd:\"odd\",sStripeEven:\"even\",sRowEmpty:\"dataTables_empty\",sWrapper:\"dataTables_wrapper\",sFilter:\"dataTables_filter\",sInfo:\"dataTables_info\",sPaging:\"dataTables_paginate paging_\",sLength:\"dataTables_length\",sProcessing:\"dataTables_processing\",sSortAsc:\"sorting_asc\",\n"
"sSortDesc:\"sorting_desc\",sSortable:\"sorting\",sSortableAsc:\"sorting_asc_disabled\",sSortableDesc:\"sorting_desc_disabled\",sSortableNone:\"sorting_disabled\",sSortColumn:\"sorting_\",sSortJUIAsc:\"\",sSortJUIDesc:\"\",sSortJUI:\"\",sSortJUIAscAllowed:\"\",sSortJUIDescAllowed:\"\",sSortJUIWrapper:\"\",sSortIcon:\"\",sScrollWrapper:\"dataTables_scroll\",sScrollHead:\"dataTables_scrollHead\",sScrollHeadInner:\"dataTables_scrollHeadInner\",sScrollBody:\"dataTables_scrollBody\",sScrollFoot:\"dataTables_scrollFoot\",sScrollFootInner:\"dataTables_scrollFootInner\",\n"
"sFooterTH:\"\",sJUIHeader:\"\",sJUIFooter:\"\"});i.extend(l.ext.oJUIClasses,l.ext.oStdClasses,{sPagePrevEnabled:\"fg-button ui-button ui-state-default ui-corner-left\",sPagePrevDisabled:\"fg-button ui-button ui-state-default ui-corner-left ui-state-disabled\",sPageNextEnabled:\"fg-button ui-button ui-state-default ui-corner-right\",sPageNextDisabled:\"fg-button ui-button ui-state-default ui-corner-right ui-state-disabled\",sPageJUINext:\"ui-icon ui-icon-circle-arrow-e\",sPageJUIPrev:\"ui-icon ui-icon-circle-arrow-w\",\n"
"sPageButton:\"fg-button ui-button ui-state-default\",sPageButtonActive:\"fg-button ui-button ui-state-default ui-state-disabled\",sPageButtonStaticDisabled:\"fg-button ui-button ui-state-default ui-state-disabled\",sPageFirst:\"first ui-corner-tl ui-corner-bl\",sPageLast:\"last ui-corner-tr ui-corner-br\",sPaging:\"dataTables_paginate fg-buttonset ui-buttonset fg-buttonset-multi ui-buttonset-multi paging_\",sSortAsc:\"ui-state-default\",sSortDesc:\"ui-state-default\",sSortable:\"ui-state-default\",sSortableAsc:\"ui-state-default\",\n"
"sSortableDesc:\"ui-state-default\",sSortableNone:\"ui-state-default\",sSortJUIAsc:\"css_right ui-icon ui-icon-triangle-1-n\",sSortJUIDesc:\"css_right ui-icon ui-icon-triangle-1-s\",sSortJUI:\"css_right ui-icon ui-icon-carat-2-n-s\",sSortJUIAscAllowed:\"css_right ui-icon ui-icon-carat-1-n\",sSortJUIDescAllowed:\"css_right ui-icon ui-icon-carat-1-s\",sSortJUIWrapper:\"DataTables_sort_wrapper\",sSortIcon:\"DataTables_sort_icon\",sScrollHead:\"dataTables_scrollHead ui-state-default\",sScrollFoot:\"dataTables_scrollFoot ui-state-default\",\n"
"sFooterTH:\"ui-state-default\",sJUIHeader:\"fg-toolbar ui-toolbar ui-widget-header ui-corner-tl ui-corner-tr ui-helper-clearfix\",sJUIFooter:\"fg-toolbar ui-toolbar ui-widget-header ui-corner-bl ui-corner-br ui-helper-clearfix\"});i.extend(l.ext.oPagination,{two_button:{fnInit:function(h,n,q){var o=h.oLanguage.oPaginate,v=function(D){h.oApi._fnPageChange(h,D.data.action)&&q(h)};o=!h.bJUI?'<a class=\"'+h.oClasses.sPagePrevDisabled+'\" tabindex=\"'+h.iTabIndex+'\" role=\"button\">'+o.sPrevious+'</a><a class=\"'+\n"
"h.oClasses.sPageNextDisabled+'\" tabindex=\"'+h.iTabIndex+'\" role=\"button\">'+o.sNext+\"</a>\":'<a class=\"'+h.oClasses.sPagePrevDisabled+'\" tabindex=\"'+h.iTabIndex+'\" role=\"button\"><span class=\"'+h.oClasses.sPageJUIPrev+'\"></span></a><a class=\"'+h.oClasses.sPageNextDisabled+'\" tabindex=\"'+h.iTabIndex+'\" role=\"button\"><span class=\"'+h.oClasses.sPageJUINext+'\"></span></a>';i(n).append(o);var w=i(\"a\",n);o=w[0];w=w[1];h.oApi._fnBindAction(o,{action:\"previous\"},v);h.oApi._fnBindAction(w,{action:\"next\"},v);\n"
"if(!h.aanFeatures.p){n.id=h.sTableId+\"_paginate\";o.id=h.sTableId+\"_previous\";w.id=h.sTableId+\"_next\";o.setAttribute(\"aria-controls\",h.sTableId);w.setAttribute(\"aria-controls\",h.sTableId)}},fnUpdate:function(h){if(h.aanFeatures.p)for(var n=h.oClasses,q=h.aanFeatures.p,o,v=0,w=q.length;v<w;v++)if(o=q[v].firstChild){o.className=h._iDisplayStart===0?n.sPagePrevDisabled:n.sPagePrevEnabled;o=o.nextSibling;o.className=h.fnDisplayEnd()==h.fnRecordsDisplay()?n.sPageNextDisabled:n.sPageNextEnabled}}},iFullNumbersShowPages:5,\n"
"full_numbers:{fnInit:function(h,n,q){var o=h.oLanguage.oPaginate,v=h.oClasses,w=function(G){h.oApi._fnPageChange(h,G.data.action)&&q(h)};i(n).append('<a  tabindex=\"'+h.iTabIndex+'\" class=\"'+v.sPageButton+\" \"+v.sPageFirst+'\">'+o.sFirst+'</a><a  tabindex=\"'+h.iTabIndex+'\" class=\"'+v.sPageButton+\" \"+v.sPagePrevious+'\">'+o.sPrevious+'</a><span></span><a tabindex=\"'+h.iTabIndex+'\" class=\"'+v.sPageButton+\" \"+v.sPageNext+'\">'+o.sNext+'</a><a tabindex=\"'+h.iTabIndex+'\" class=\"'+v.sPageButton+\" \"+v.sPageLast+\n"
"'\">'+o.sLast+\"</a>\");var D=i(\"a\",n);o=D[0];v=D[1];var A=D[2];D=D[3];h.oApi._fnBindAction(o,{action:\"first\"},w);h.oApi._fnBindAction(v,{action:\"previous\"},w);h.oApi._fnBindAction(A,{action:\"next\"},w);h.oApi._fnBindAction(D,{action:\"last\"},w);if(!h.aanFeatures.p){n.id=h.sTableId+\"_paginate\";o.id=h.sTableId+\"_first\";v.id=h.sTableId+\"_previous\";A.id=h.sTableId+\"_next\";D.id=h.sTableId+\"_last\"}},fnUpdate:function(h,n){if(h.aanFeatures.p){var q=l.ext.oPagination.iFullNumbersShowPages,o=Math.floor(q/2),v=\n"
"Math.ceil(h.fnRecordsDisplay()/h._iDisplayLength),w=Math.ceil(h._iDisplayStart/h._iDisplayLength)+1,D=\"\",A,G=h.oClasses,E,Y=h.aanFeatures.p,ma=function(R){h.oApi._fnBindAction(this,{page:R+A-1},function(ea){h.oApi._fnPageChange(h,ea.data.page);n(h);ea.preventDefault()})};if(h._iDisplayLength===-1)w=o=A=1;else if(v<q){A=1;o=v}else if(w<=o){A=1;o=q}else if(w>=v-o){A=v-q+1;o=v}else{A=w-Math.ceil(q/2)+1;o=A+q-1}for(q=A;q<=o;q++)D+=w!==q?'<a tabindex=\"'+h.iTabIndex+'\" class=\"'+G.sPageButton+'\">'+h.fnFormatNumber(q)+\n"
"\"</a>\":'<a tabindex=\"'+h.iTabIndex+'\" class=\"'+G.sPageButtonActive+'\">'+h.fnFormatNumber(q)+\"</a>\";q=0;for(o=Y.length;q<o;q++){E=Y[q];if(E.hasChildNodes()){i(\"span:eq(0)\",E).html(D).children(\"a\").each(ma);E=E.getElementsByTagName(\"a\");E=[E[0],E[1],E[E.length-2],E[E.length-1]];i(E).removeClass(G.sPageButton+\" \"+G.sPageButtonActive+\" \"+G.sPageButtonStaticDisabled);i([E[0],E[1]]).addClass(w==1?G.sPageButtonStaticDisabled:G.sPageButton);i([E[2],E[3]]).addClass(v===0||w===v||h._iDisplayLength===-1?G.sPageButtonStaticDisabled:\n"
"G.sPageButton)}}}}}});i.extend(l.ext.oSort,{\"string-pre\":function(h){if(typeof h!=\"string\")h=h!==null&&h.toString?h.toString():\"\";return h.toLowerCase()},\"string-asc\":function(h,n){return h<n?-1:h>n?1:0},\"string-desc\":function(h,n){return h<n?1:h>n?-1:0},\"html-pre\":function(h){return h.replace(/<.*?>/g,\"\").toLowerCase()},\"html-asc\":function(h,n){return h<n?-1:h>n?1:0},\"html-desc\":function(h,n){return h<n?1:h>n?-1:0},\"date-pre\":function(h){h=Date.parse(h);if(isNaN(h)||h===\"\")h=Date.parse(\"01/01/1970 00:00:00\");\n"
"return h},\"date-asc\":function(h,n){return h-n},\"date-desc\":function(h,n){return n-h},\"numeric-pre\":function(h){return h==\"-\"||h===\"\"?0:h*1},\"numeric-asc\":function(h,n){return h-n},\"numeric-desc\":function(h,n){return n-h}});i.extend(l.ext.aTypes,[function(h){if(typeof h===\"number\")return\"numeric\";else if(typeof h!==\"string\")return null;var n,q=false;n=h.charAt(0);if(\"0123456789-\".indexOf(n)==-1)return null;for(var o=1;o<h.length;o++){n=h.charAt(o);if(\"0123456789.\".indexOf(n)==-1)return null;if(n==\n"
"\".\"){if(q)return null;q=true}}return\"numeric\"},function(h){var n=Date.parse(h);if(n!==null&&!isNaN(n)||typeof h===\"string\"&&h.length===0)return\"date\";return null},function(h){if(typeof h===\"string\"&&h.indexOf(\"<\")!=-1&&h.indexOf(\">\")!=-1)return\"html\";return null}]);i.fn.DataTable=l;i.fn.dataTable=l;i.fn.dataTableSettings=l.settings;i.fn.dataTableExt=l.ext})})(window,document);\n";

const char *widgetsScript = 
"function refresh (api_name, prefix, id){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../' + api_name,true);\n"
  "hr.onreadystatechange = function(){\n"
    "if (hr.readyState != 4 || hr.status != 200) { return; }\n"
    "var sta = JSON.parse(hr.responseText);\n"
    "var el = document.getElementById(id);\n"
    "var pl = prefix.length;\n"
    "var childs = el.getElementsByTagName('*');\n"
    "for (di in childs){\n"
      "var ce = childs[di];\n"
      "try{\n"
        "if (ce.id.substr(0,pl) == prefix){\n"
          "var name = ce.id.substr(pl);\n"
	  "ce.innerHTML = sta[name];\n"
        "}\n"
      "} catch(e) {}\n"
    "}\n"
  "}\n"
  "hr.send(null);\n"
"}\n";

/**
 *
 * setCall (device, variable, value)
 *
 * Sets variable with device call. Please be aware that device cache will
 * not be refreshed after call, so most probably it will contain old values.
 *
 * setGetCallFunction (device, variable, value, func)
 *
 * Call setCall function with device, variable and value as parameters. Then
 * calls info function to refresh device variables, and when it sucessfully finished, calls
 * func. Pass (new) value of variable to function as single parameter.
 */

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

"function setCallFunction (device, variable, value, func){\n"
  "jsonCall ('../api/set?d=' + device + '&n=' + variable + '&v=' + value, func);\n"
"}\n"

"function setCall (device, variable, value){\n"
  "setCallFunction (device, variable, value, function(ret) {});\n"
"}\n"

"function setCallRaDec (device, variable, ra, dec, func){\n"
  "setCallFunction (device, variable, ra + ' ' + dec, func);\n"
"}\n"

"function getCallFunction (device, variable, func){\n"
  "jsonCall ('../api/get?d=' + device, function (ret) { func(ret.d[variable]); });\n"
"}\n"

"function getCall (device, variable, input){\n"
  "getCallFunction (device, variable, function(v) { input.value = v;});\n"
"}\n"

"function getCallTime (device, variable, input, show_diff){\n"
  "getCallFunction (device, variable, function(v) {"
    "var now = new Date();\n"
    "var d = new Date(v * 1000);\n"
    "var s = d.toString();\n"
    "if (show_diff){ s += ' ' + (now.valueOf() - d.valueOf()) / 1000.0;}\n"
    "input.value = s;\n"
  "});\n"
"}\n"

"function setGetCall (device, variable, input){\n"
  "setCall (device, variable, input.value);\n"
  "jsonCall ('../api/cmd?c=info&d=' + device, function (ret) { input.value = ret.d[variable][1]; });\n"
"}\n"

"function setGetCallFunction (device, variable, value, func){\n"
  "setCall (device, variable, value);\n"
  "jsonCall ('../api/cmd?c=info&d=' + device, function (ret) { func(ret.d[variable][1]); });\n"
"}\n"

"function queueTarget (device, variable, value, func){\n"
  "jsonCall ('../api/cmd?c=queue ' + value +'&d=' + device, function (ret) { func(ret.d[variable][1]);} );\n"
"}\n"

"function queueTargetAt (device, variable, value, start, end, func){\n"
  "jsonCall ('../api/cmd?c=queue_at ' + value + '%20' + start + '%20' + end + '&d=' + device, function (ret) { func(ret.d[variable][1]);} );\n"
"}\n"

"function refreshDeviceTable (device){\n"
  "func = function(ret){\n"
    "for (v in ret.d) {\n"
      "document.getElementById(device + '_' + v).innerHTML = ret.d[v];\n"
    "}\n"  
  "}\n"
  "jsonCall('../api/get?d=' + device + '&e=1', func);\n"
"}\n"

"function deviceGetAll(device, func){\n"
  "jsonCall('../api/get?d=' + device, func);\n"
"}\n"

"function getTargetInfo(id, func){\n"
  "jsonCall('../api/tbyid?e=1&id=' + id, func);\n"
"}\n"

"function getTaltitudes(id, func){\n"
  "jsonCall('../api/taltitudes?id=' + id, func);\n"
"}\n"

"function getTargetCamScript(id, cameraName, func){\n"
  "jsonCall('../api/script?id=' + id + '&cn=' + cameraName, func);\n"
"}\n"
;

const char *labels =
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

"function getLabels(func){\n"
  "jsonCall('../api/labellist', func);\n"
"}\n"

"function setLabel(id,label_str,ltype,func){\n"
  "jsonCall('../api/tlabs_add?id=' + id + '&ltext=' + label_str + '&ltype=' + ltype, func);\n"
"}\n"
;

void LibJavaScript::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	const char *reply = NULL;
	
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	if (vals.size () == 2)
	{
		if (vals[0] == "vrml")
			processVrml (vals[1], response_type, response, response_length);
		else
			throw rts2core::Error ("File not found");
		return;
	}

	cacheMaxAge (CACHE_MAX_STATIC);

	if (vals.size () != 1)
		throw rts2core::Error ("File not found");

	if (vals[0] == "equ.js")
		reply = equScript;
	else if (vals[0] == "date.js")
		reply = dateScript;
	else if (vals[0] == "targetedit.js")
	  	reply = targetEdit;
	else if (vals[0] == "table.js")
		reply = tableScript;
	else if (vals[0] == "widgets.js")
	  	reply = widgetsScript;
	else if (vals[0] == "setgetapi.js")
	  	reply = setGetApi;
        else if (vals[0] == "rts2centrald.js")
                reply = rts2Central;
	else if (vals[0] == "labels.js")
	        reply = labels;
	else if (vals[0] == "jquery.js")
		reply = jquery;
	else if (vals[0] == "datatables.js")
		reply = datatables;
	else
		throw rts2core::Error ("JavaScript not found");

	response_length = strlen (reply);
	response = new char[response_length];
	response_type = "text/javascript";
	memcpy (response, reply, response_length);
}

void LibJavaScript::processVrml (std::string file, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	rts2core::connections_t::iterator iter = getServer ()->getConnections ()->begin ();
	getServer ()->getOpenConnectionType (DEVICE_TYPE_MOUNT, iter);
	if (iter == getServer ()->getConnections ()->end ())
	  	throw XmlRpc::XmlRpcException ("Telescope is not connected");
	
	_os << "function initialize() {\n";

	if (file == "RA_script.js")
	{
		rts2core::ValueDouble *telHa = (rts2core::ValueDouble *) ((*iter)->getValueType ("HA", RTS2_VALUE_DOUBLE));
		rts2core::ValueInteger *telFlip = (rts2core::ValueInteger *) ((*iter)->getValueType ("MNT_FLIP", RTS2_VALUE_INTEGER));
		double ha = telHa->getValueDouble ();
		if (telFlip->getValueInteger () == 1)
			ha -= 180;
		if (ha > 180)
			ha -= 360;
		ha -= 90;
	 	_os << "RA = " << ln_deg_to_rad (ha) << ";  // Rektascenze v radianech (-1.57 - 1.57)\n"
			"RA_rot = new SFRotation(0, 1, 0, RA);\n"
			"set_state = RA_rot;\n";
	}
	else if (file == "DEC_script.js")
	{
		rts2core::ValueRaDec *telRaDec = (rts2core::ValueRaDec *) ((*iter)->getValueType ("TEL", RTS2_VALUE_RADEC));
		rts2core::ValueInteger *telFlip = (rts2core::ValueInteger *) ((*iter)->getValueType ("MNT_FLIP", RTS2_VALUE_INTEGER));
		double dec = telRaDec->getDec ();
		if (telFlip->getValueInteger () == 1)
			dec -= 2*(90-fabs(dec));
		_os << "DEC = " << ln_deg_to_rad (dec) << ";  // Deklinace v radianech (-1.57 - 1.57)\n"
		    	"DEC_rot = new SFRotation(0, 1, 0, DEC);\n"
			"set_state = DEC_rot;\n";
	}
	else if (file == "roof_script.js")
	{
		rts2core::connections_t::iterator iter_dome = getServer ()->getConnections ()->begin ();
		getServer ()->getOpenConnectionType (DEVICE_TYPE_DOME, iter_dome);
		if (iter_dome == getServer ()->getConnections ()->end ())
		  	throw XmlRpc::XmlRpcException ("Dome is not connected");
	    	_os << "set_state = " << (((*iter_dome)->getState () & DOME_OPENED) ? "opened" : "closed") << "; // closed OR opened\n";
	}
	else
	{
		throw XmlRpc::XmlRpcException ("");
	}

	_os << "}";

	response_type = "text/javascript";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

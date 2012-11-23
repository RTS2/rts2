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
#include "xmlrpcd.h"
#include "utilsfunc.h"
#include "error.h"

using namespace rts2xmlrpc;

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
	  "if (a.suffix !== undefined) a.href += a.suffix;\n"
	  "a.href += '/';\n"
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
"function setCall (device, variable, value){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/set?d=' + device + '&n=' + variable + '&v=' + value,true);\n"
  "hr.onreadystatechange = function(){\n"
    "if (hr.readyState != 4 || hr.status != 200) { return; }\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function setCallFunction (device, variable, value, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/set?d=' + device + '&n=' + variable + '&v=' + value,true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (hr.readyState != 4 || hr.status != 200) { return; }\n"
  "this.func(hr.status);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"


"function setCallRaDec (device, variable, ra, dec, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/set?d=' + device + '&n=' + variable + '&v=' + ra + ' ' + dec,true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (hr.readyState != 4 || hr.status != 200) { return; }\n"
  "this.func(hr.status);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function getCallFunction (device, variable, func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/get?d=' + device, true);\n"
  "hr.variable = variable;\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "func (t.d[this.variable]);\n"
  "}\n"
  "hr.send(null);\n"
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
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/cmd?c=info&d=' + device, true);\n"
  "hr.variable = variable;\n"
  "hr.input = input;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.input.value = t.d[this.variable][1];\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function setGetCallFunction (device, variable, value, func){\n"
  "setCall (device, variable, value);\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/cmd?c=info&d=' + device, true);\n"
  "hr.variable = variable;\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.d[this.variable][1]);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function queueTarget (device, variable, value, func){\n"
  "setCall (device, variable, value);\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/cmd?c=queue ' + value +'&d=' + device, true);\n"
  "hr.variable = variable;\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.d[this.variable][1]);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"


"function queueTargetAt (device, variable, value, start, end, func){\n"
  "//setCall (device, variable, value);\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/cmd?c=queue_at ' + value + '%20' + start + '%20' + end + '&d=' + device, true);\n"
  "hr.variable = variable;\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t.d[this.variable][1]);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"


"function refreshDeviceTable (device){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/get?d=' + device + '&e=1', true);\n"
  "hr.device = device;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "for (v in t.d) {\n"
      "document.getElementById(this.device + '_' + v).innerHTML = t.d[v];\n"
    "}\n"  
  "}\n"
  "hr.send(null);\n"
"}\n"


"function deviceGetAll(device, func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET','../api/get?d=' + device, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
 "hr.send(null);\n"
 "}\n"

"function getTargetInfo(id, func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET','../api/tbyid?e=1&id=' + id, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
 "hr.send(null);\n"
 "}\n"



"function getTaltitudes(id, func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET','../api/taltitudes?id=' + id, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
 "hr.send(null);\n"
 "}\n"

"function getTargetCamScript(id, cameraName, func){\n"
 "var hr = new XMLHttpRequest();\n"
 "hr.open('GET','../api/script?id=' + id + '&cn=' + cameraName, true);\n"
 "hr.func = func;\n"
 "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
 "hr.send(null);\n"
 "}\n"

;

const char *labels =
"function getLabels(func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/labellist', true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
  "hr.send(null);\n"
"}\n"

"function setLabel(id,label_str,ltype,func){\n"
  "var hr = new XMLHttpRequest();\n"
  "hr.open('GET','../api/tlabs_add?id=' + id + '&ltext=' + label_str + '&ltype=' + ltype, true);\n"
  "hr.func = func;\n"
  "hr.onreadystatechange = function(){\n"
    "if (this.readyState != 4 || this.status != 200) { return; }\n"
    "var t = JSON.parse(this.responseText);\n"
    "this.func(t);\n"
  "}\n"
  "hr.send(null);\n"
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

	XmlRpcd *master = (XmlRpcd *) getMasterApp ();
	connections_t::iterator iter = master->getConnections ()->begin ();
	master->getOpenConnectionType (DEVICE_TYPE_MOUNT, iter);
	if (iter == master->getConnections ()->end ())
	  	throw XmlRpcException ("Telescope is not connected");
	
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
		connections_t::iterator iter_dome = master->getConnections ()->begin ();
		master->getOpenConnectionType (DEVICE_TYPE_DOME, iter_dome);
		if (iter_dome == master->getConnections ()->end ())
		  	throw XmlRpcException ("Dome is not connected");
	    	_os << "set_state = " << (((*iter_dome)->getState () & DOME_OPENED) ? "opened" : "closed") << "; // closed OR opened\n";
	}
	else
	{
		throw XmlRpcException ("");
	}

	_os << "}";

	response_type = "text/javascript";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

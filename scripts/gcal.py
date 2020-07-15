#!/usr/bin/env python
#
# Copyright (C) 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


__author__ = 'api.rboyd@gmail.com (Ryan Boyd)'


try:
  from xml.etree import ElementTree
except ImportError:
  from elementtree import ElementTree
import gdata.calendar.service
import gdata.service
import atom.service
import gdata.calendar
import atom
import getopt
import sys
import string
import time
import os


class CalendarExample:

  def __init__(self, email, password):
    """Creates a CalendarService and provides ClientLogin auth details to it.
    The email and password are required arguments for ClientLogin.  The 
    CalendarService automatically sets the service to be 'cl', as is 
    appropriate for calendar.  The 'source' defined below is an arbitrary 
    string, but should be used to reference your name or the name of your
    organization, the app name and version, with '-' between each of the three
    values.  The account_type is specified to authenticate either 
    Google Accounts or Google Apps accounts.  See gdata.service or 
    http://code.google.com/apis/accounts/AuthForInstalledApps.html for more
    info on ClientLogin.  NOTE: ClientLogin should only be used for installed 
    applications and not for multi-user web applications."""

    self.cal_client = gdata.calendar.service.CalendarService()
    self.cal_client.email = email
    self.cal_client.password = password
    self.cal_client.source = 'RTS2-0.8'
    self.cal_client.ProgrammaticLogin()

  def _InsertEvent(self, title='Tennis with Beth', 
      content='Meet for a quick lesson', where='On the courts',
      start_time=None, end_time=None, recurrence_data=None):
    """Inserts a basic event using either start_time/end_time definitions
    or gd:recurrence RFC2445 icalendar syntax.  Specifying both types of
    dates is not valid.  Note how some members of the CalendarEventEntry
    class use arrays and others do not.  Members which are allowed to occur
    more than once in the calendar or GData "kinds" specifications are stored
    as arrays.  Even for these elements, Google Calendar may limit the number
    stored to 1.  The general motto to use when working with the Calendar data
    API is that functionality not available through the GUI will not be 
    available through the API.  Please see the GData Event "kind" document:
    http://code.google.com/apis/gdata/elements.html#gdEventKind
    for more information"""
    
    event = gdata.calendar.CalendarEventEntry()
    event.title = atom.Title(text=title)
    event.content = atom.Content(text=content)
    event.where.append(gdata.calendar.Where(value_string=where))

    if recurrence_data is not None:
      # Set a recurring event
      event.recurrence = gdata.calendar.Recurrence(text=recurrence_data)
    else:
      if start_time is None:
        # Use current time for the start_time and have the event last 1 hour
        start_time = time.strftime('%Y-%m-%dT%H:%M:%S.000Z', time.gmtime())
        end_time = time.strftime('%Y-%m-%dT%H:%M:%S.000Z', 
            time.gmtime(time.time() + 3600))
      event.when.append(gdata.calendar.When(start_time=start_time, 
          end_time=end_time))
    
    new_event = self.cal_client.InsertEvent(event, 
        '/calendar/feeds/default/private/full')
    
    return new_event
   
  def _InsertSingleEvent(self, title='One-time Tennis with Beth',
      content='Meet for a quick lesson', where='On the courts',
      start_time=None, end_time=None):
    """Uses the _InsertEvent helper method to insert a single event which
    does not have any recurrence syntax specified."""

    new_event = self._InsertEvent(title, content, where, start_time, end_time, 
        recurrence_data=None)

    print 'New single event inserted: %s' % (new_event.id.text,)
    print '\tEvent edit URL: %s' % (new_event.GetEditLink().href,)
    print '\tEvent HTML URL: %s' % (new_event.GetHtmlLink().href,)

    return new_event

  def _InsertSimpleWebContentEvent(self):
    """Creates a WebContent object and embeds it in a WebContentLink.
    The WebContentLink is appended to the existing list of links in the event
    entry.  Finally, the calendar client inserts the event."""

    # Create a WebContent object
    url = 'http://www.google.com/logos/worldcup06.gif'
    web_content = gdata.calendar.WebContent(url=url, width='276', height='120')
    
    # Create a WebContentLink object that contains the WebContent object
    title = 'World Cup'
    href = 'http://www.google.com/calendar/images/google-holiday.gif'
    type = 'image/gif'
    web_content_link = gdata.calendar.WebContentLink(title=title, href=href, 
        link_type=type, web_content=web_content)
        
    # Create an event that contains this web content
    event = gdata.calendar.CalendarEventEntry()
    event.link.append(web_content_link)

    print 'Inserting Simple Web Content Event'
    new_event = self.cal_client.InsertEvent(event, 
        '/calendar/feeds/default/private/full')
    return new_event

  def _InsertWebContentGadgetEvent(self):
    """Creates a WebContent object and embeds it in a WebContentLink.
    The WebContentLink is appended to the existing list of links in the event
    entry.  Finally, the calendar client inserts the event.  Web content
    gadget events display Calendar Gadgets inside Google Calendar."""

    # Create a WebContent object
    url = 'http://google.com/ig/modules/datetime.xml'
    web_content = gdata.calendar.WebContent(url=url, width='300', height='136')
    web_content.gadget_pref.append(
        gdata.calendar.WebContentGadgetPref(name='color', value='green'))

    # Create a WebContentLink object that contains the WebContent object
    title = 'Date and Time Gadget'
    href = 'http://gdata.ops.demo.googlepages.com/birthdayicon.gif'
    type = 'application/x-google-gadgets+xml'
    web_content_link = gdata.calendar.WebContentLink(title=title, href=href,
        link_type=type, web_content=web_content)

    # Create an event that contains this web content
    event = gdata.calendar.CalendarEventEntry()
    event.link.append(web_content_link)

    print 'Inserting Web Content Gadget Event'
    new_event = self.cal_client.InsertEvent(event,
        '/calendar/feeds/default/private/full')
    return new_event

def main():
  """Runs the CalendarExample application with the provided username and
  and password values.  Authentication credentials are required.  
  NOTE: It is recommended that you run this sample using a test account."""

  # parse command line options
  try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["user=", "pw=", "title=", "event=", "place=", "end="])
  except getopt.error, msg:
    print ('python calendarExample.py --user [username] --pw [password] ' + 
        '--title [title] --event [event] --place [place] --end [end date]')
    sys.exit(2)

  user = ''
  pw = ''

  title = ''
  event = ''
  place = ''

  start = time.gmtime()
  end = ''

  # Process options
  for o, a in opts:
    if o == "--user":
      user = a
    elif o == "--pw":
      pw = a
    elif o == "--title":
      title = a
    elif o == "--event":
      event = a
    elif o == "--place":
      place = a
    elif o == "--end":
      end = time.gmtime(time.time() + int(a))

  if user == '' or pw == '' or title == '' or event == '' or place == '' or end == '':
    print ('python calendarExample.py --user [username] --pw [password] ' + 
        '--title [title] --event [event] --place [place] -- end [end date]')
    sys.exit(2)

  sample = CalendarExample(user, pw)
  see = sample._InsertSingleEvent(title, event, place, time.strftime('%Y-%m-%dT%H:%M:%S.000Z', start), time.strftime('%Y-%m-%dT%H:%M:%S.000Z', end))

if __name__ == '__main__':
  main()

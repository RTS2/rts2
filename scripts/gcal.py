#!/usr/bin/python
#
# Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

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

class Rts2GoogleCalendar

  def __init__(self, email, password):
    """Creates a CalendarService and provides ClientLogin auth details to it."""
    self.cal_client = gdata.calendar.service.CalendarService()
    self.cal_client.email = email
    self.cal_client.password = password
    self.cal_client.source = 'RTS2-0.8'
    self.cal_client.ProgrammaticLogin()

  def _Rts2Event(self, title, content, where, start_time, end_time):
    """Inserts new RTS2 event. Returns event reference, which can be used
    for modification of the event."""
    event = gdata.calendar.CalendarEventEntry()
    event.title = atom.Title(text=title)
    event.content = atom.Content(text=content)
    event.where.append(gdata.calendar.Where(value_string=where))
    event.when.append(gdata.calendar.When(start_time=start_time, end_time=end_time))
    new_event = self.cal_client.InsertEvent(event, '/calendar/feeds/default/private/full')
    return new_event
   
  def _Rts2Observation(self, target_name, target_id, observation_id, start_time):
    """Insert new observation with link to observation preview."""
    
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

  def _UpdateTitle(self, event, new_title='Updated event title'):
    """Updates the title of the specified event with the specified new_title.
    Note that the UpdateEvent method (like InsertEvent) returns the 
    CalendarEventEntry object based upon the data returned from the server
    after the event is inserted.  This represents the 'official' state of
    the event on the server.  The 'edit' link returned in this event can
    be used for future updates.  Due to the use of the 'optimistic concurrency'
    method of version control, most GData services do not allow you to send 
    multiple update requests using the same edit URL.  Please see the docs:
    http://code.google.com/apis/gdata/reference.html#Optimistic-concurrency
    """

    previous_title = event.title.text
    event.title.text = new_title
    print 'Updating title of event from:\'%s\' to:\'%s\'' % (
        previous_title, event.title.text,) 
    return self.cal_client.UpdateEvent(event.GetEditLink().href, event)


def main():
  """Runs the CalendarExample application with the provided username and
  and password values.  Authentication credentials are required.  
  NOTE: It is recommended that you run this sample using a test account."""

  # parse command line options
  try:
    opts, args = getopt.getopt(sys.argv[1:], "", ["user=", "pw=", "delete="])
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
      end = a

  if user == '' or pw == '' or tilte == '' or event == '' or place == '' or end == '':
    print ('python calendarExample.py --user [username] --pw [password] ' + 
        '--title [title] --event [event] --place [place] -- end [end date]')
    sys.exit(2)

  sample = CalendarExample(user, pw)
  see = self._InsertSingleEvent(title, event, place, start, end)

if __name__ == '__main__':
  main()

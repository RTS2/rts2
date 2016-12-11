# (C) 2016, Markus Wildi, wildi.markus@bluewin.ch
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

'''

Classes for watchdog

'''

__author__ = 'wildi.markus@bluewin.ch'

#ToDo
#from watchdog.events import LoggingEventHandler
from watchdog.events import FileSystemEventHandler

class ImageEventHandler(FileSystemEventHandler):
  def __init__(self,lg=None,acq_queue=None):
    self.lg=lg
    self.acq_queue=acq_queue
    
  def on_modified(self, event):
    if 'fit' in event.src_path.lower():
        self.lg.info('RTS2 external CCD image: {}'.format(event.src_path))
    acq=acq_queue.put(event.src_path)

# watchdog has not all events
import pyinotify
class EventHandler(pyinotify.ProcessEvent):
  def __init__(self,lg=None,fn=None):
    self.lg=lg
    self.fn=fn
    self.not_writable=False
    
  def process_IN_ACCESS(self, event):
    if self.fn in event.pathname:
      #print( "ACCESS event:", event.pathname)
      self.not_writable=True

  #def process_IN_ATTRIB(self, event):
  #  print( "ATTRIB event:", event.pathname)

  def process_IN_CLOSE_NOWRITE(self, event):
    if self.fn in event.pathname:
      #print( "CLOSE_NOWRITE event:", event.pathname)
      self.not_writable=False

  def process_IN_CLOSE_WRITE(self, event):
    if self.fn in event.pathname:
      #print( "CLOSE_WRITE event:", event.pathname)
      self.not_writable=False

  def process_IN_CREATE(self, event):
    if self.fn in event.pathname:
      #print( "CREATE event:", event.pathname)
      self.not_writable=True
  
  #def process_IN_DELETE(self, event):
  #  print( "DELETE event:", event.pathname)

  def process_IN_MODIFY(self, event):
    if self.fn in event.pathname:
      #print( "MODIFY event:", event.pathname)
      self.not_writable=False

  def process_IN_OPEN(self, event):
    if self.fn in event.pathname:
      #print( "OPEN event:", event.pathname)
      self.not_writable=False

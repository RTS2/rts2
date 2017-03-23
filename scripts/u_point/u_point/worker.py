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

Multiprocesser Worker

'''

__author__ = 'wildi.markus@bluewin.ch'

import numpy as np
from multiprocessing import Process,Event,current_process,Queue


class Worker(Process):
  def __init__(self, work_queue=None,cmd_queue=None,next_queue=None,lock=None, dbg=None, lg=None, anl=None):
    Process.__init__(self)
    self.exit=Event()
    self.work_queue=work_queue
    self.cmd_queue=cmd_queue
    self.next_queue=next_queue
    self.anl=anl
    self.lock=lock
    self.dbg=dbg
    self.lg=lg

  def append_position(self,sky=None):
    self.lock.acquire()
    self.anl.append_position(sky=sky,analyzed=True)
    self.lock.release()

  def run(self):
    sky=acq_image_fn=None
    while not self.exit.is_set():
      if self.cmd_queue is not None:
        try:
          cmd=self.cmd_queue.get()
        # ToDO Empty is not correct
        except Queue.Empty:
          self.lg.info('{}: cmd queue empty, returning'.format(current_process().name))
          return
        #
        if isinstance(cmd, str):
          if 'dl' in cmd:
            self.lg.info('{}: got \'Delete\', delete last: {}'.format(current_process().name,sky.image_fn))
            acq_image_fn=sky.image_fn
            sky=None
          elif 'q' in cmd:
            self.append_position(sky=sky)
            self.lg.error('{}: got {}, call shutdown'.format(current_process().name, cmd))
            self.shutdown()
            return
          else:
            self.lg.warn('{}: got {}, continue'.format(current_process().name, cmd))
        if sky:
          self.append_position(sky=sky)
        elif acq_image_fn is not None:
          self.lg.info('{}: not storing {}'.format(current_process().name, acq_image_fn))
          acq_image_fn=None
      sky=None
      try:
        sky=self.work_queue.get()
      except Queue.Empty:
        self.lg.info('{}: queue empty, returning'.format(current_process().name))
        return
      except Exception as  e:
        self.lg.error('{}: queue error: {}, returning'.format(current_process().name, e))
        return
      # 'STOP'
      if isinstance(sky, str):
        self.lg.error('{}: got {}, call shutdown_on_STOP'.format(current_process().name, sky))
        self.shutdown_on_STOP()
        return
      
      # analysis part
      self.anl.catalog_to_apparent(sky=sky,pcn=current_process().name)
      x,y=self.anl.sextract(sky=sky,pcn=current_process().name)
      if x is not None and y is not None:
        self.anl.xy2lonlat_apparent(px=x,py=y,sky=sky,pcn=current_process().name)

      self.anl.astrometry(sky=sky,pcn=current_process().name)
      # ToDo why this condition
      if self.cmd_queue is None:
        self.append_position(sky=sky)
      else:
        self.next_queue.put('c')
      # end analysis part

    self.lg.info('{}: got shutdown event, exiting'.format(current_process().name))

  def shutdown(self):
    if self.next_queue is not None:
      self.next_queue.put('c')
    self.lg.info('{}: shutdown event, initiate exit'.format(current_process().name))
    self.exit.set()
    return

  def shutdown_on_STOP(self):
    if self.next_queue is not None:
      self.next_queue.put('c')
    self.lg.info('{}: shutdown on STOP, initiate exit'.format(current_process().name))
    self.exit.set()
    return

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
from multiprocessing import Process,Event,current_process

class Worker(Process):
  def __init__(self, work_queue=None,ds9_queue=None,next_queue=None,lock=None, dbg=None, lg=None, anl=None):
    Process.__init__(self)
    self.exit=Event()
    self.work_queue=work_queue
    self.ds9_queue=ds9_queue
    self.next_queue=next_queue
    self.anl=anl
    self.lock=lock
    self.dbg=dbg
    self.lg=lg

  def append_position(self,pos=None):
    self.lock.acquire()
    self.anl.append_position(pos=pos,analyzed=True)
    self.lock.release()

  def run(self):
    pos=acq_image_fn=None
    while not self.exit.is_set():
      if self.ds9_queue is not None:
        try:
          cmd=self.ds9_queue.get()
        except Queue.Empty:
          self.lg.info('{}: ds9 queue empty, returning'.format(current_process().name))
          return
        # 'q'
        if isinstance(cmd, str):
          if 'dl' in cmd:
            self.lg.info('{}: got {}, delete last: {}'.format(current_process().name, cmd,pos.image_fn))
            acq_image_fn=pos.image_fn
            acq=None
          elif 'q' in cmd:
            self.append_position(pos=pos)
            self.lg.error('{}: got {}, call shutdown'.format(current_process().name, cmd))
            self.shutdown()
            return
          else:
            self.lg.error('{}: got {}, continue'.format(current_process().name, cmd))
        if pos:
          self.append_position(pos=pos)
        elif acq_image_fn is not None:
          self.lg.info('{}: not storing {}'.format(current_process().name, acq_image_fn))
          acq_image_fn=None
      pos=None
      try:
        pos=self.work_queue.get()
      except Queue.Empty:
        self.lg.info('{}: queue empty, returning'.format(current_process().name))
        return
      except Exception as  e:
        self.lg.error('{}: queue error: {}, returning'.format(current_process().name, e))
        return
      # 'STOP'
      if isinstance(pos, str):
        self.lg.error('{}: got {}, call shutdown_on_STOP'.format(current_process().name, pos))
        self.shutdown_on_STOP()
        return
      # analysis part
      x,y=self.anl.sextract(pos=pos,pcn=current_process().name)
      if x is not None and y is not None:
        self.anl.xy2lonlat_appr(px=x,py=y,pos=pos,pcn=current_process().name)

      self.anl.astrometry(pos=pos,pcn=current_process().name)
      # ToDo why this condition
      if self.ds9_queue is None:
        self.append_position(pos=pos)
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

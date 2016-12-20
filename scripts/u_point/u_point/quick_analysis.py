#!/usr/bin/env python3
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

Make decision about exposure


'''

__author__ = 'wildi.markus@bluewin.ch'

import re
import numpy as np
from astropy.io import fits

import u_point.sextractor_3 as sextractor_3
#import sextractor_3 as sextractor_3
#  Background: 4085.92    RMS: 241.384    / Threshold: 651.737 
pat = re.compile('.*?Background:[ ]+([0-9.]+)[ ]+RMS:[ ]+([0-9.]+)[ /]+Threshold:[ ]+([0-9.]+).*?')

class UserControl(object):
  def __init__(self,lg=None,base_path=None):
    self.lg=lg
    self.base_path=base_path
  def create_socket(self, port=9999):
    # acquire runs as a subprocess of rts2-script-exec and has no
    # stdin available from controlling TTY.
    sckt=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # use telnet 127.0.0.1 9999 to connect
    for p in range(port,port-10,-1):
      try:
        sckt.bind(('0.0.0.0',p)) # () tupple!
        self.lg.info('create_socket port to connect for user input: {0}, use telnet 127.0.0.1 {0}'.format(p))
        break
      except Exception as e :
        self.lg.debug('create_socket: can not bind socket {}'.format(e))
        continue
      
    sckt.listen(1)
    (user_input, address) = sckt.accept()
    return user_input

  def wait_for_user(self,user_input=None, last_exposure=None,):
    menu=bytearray('mount synchronized\n',encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('current exposure: {0:.1f}\n'.format(last_exposure),encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('r [exp]: redo current position, [exposure [sec]]\n',encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('[exp]:  <RETURN> current exposure, [exposure [sec]],\n',encoding='UTF-8')
    user_input.sendall(menu)
    menu=bytearray('q: quit\n\n',encoding='UTF-8')
    user_input.sendall(menu)
    cmd=None
    while True:
      menu=bytearray('your choice: ',encoding='UTF-8')
      user_input.sendall(menu)
      uir=user_input.recv(512)
      user_input.sendall(uir)
      ui=uir.decode('utf-8').replace('\r\n','')
      self.lg.debug('user input:>>{}<<, len: {}'.format(ui,len(ui)))

      if 'q' in ui or 'Q' in ui: 
        user_input.close()
        self.lg.info('wait_for_user: quit, exiting')
        sys.exit(1)

      if len(ui)==0: #<ENTER> 
        exp=last_exposure
        cmd='e'
        break
      
      if len(ui)==1 and 'r' in ui: # 'r' alone
        exp=last_exposure
        cmd='r'
        break
      
      try:
        cmd,exps=ui.split()
        exp=float(exps)
        break
      except Exception as e:
        self.lg.debug('exception: {} '.format(e))

      try:
        cmd='e'
        exp=float(ui)
        break
      except:
        # ignore bad input
        menu=bytearray('no acceptable input: >>{}<<,try again\n'.format(ui),encoding='UTF-8')
        user_input.sendall(menu)
        continue
    return cmd,exp

  def save_code(self):
      user_input=self.create_socket()

      if self.mode_user:
        while True:
          if not_first:
                self.expose(nml_id=nml.nml_id,cat_no=cat_no,cat_ic=cat_ic,exp=exp)
          else:
            not_first=True
              
          self.lg.info('acquire: mount synchronized, waiting for user input')
          cmd,exp=self.wait_for_user(user_input=user_input,last_exposure=last_exposure)
          last_exposure=exp
          self.lg.debug('acquire: user input received: {}, {}'.format(cmd,exp))
          if 'r' not in cmd:
            break
          else:
            self.lg.debug('acquire: redoing same position')

class QuickAnalysis(object):
  def __init__(self,lg=None,ds9_display=None,background_max=None,peak_max=None,objects_min=None,exposure_interval=None,ratio_interval=None):
    self.lg=lg
    self.ds9_display=ds9_display
    self.background_max=background_max
    self.peak_max=peak_max
    self.objects_min=objects_min
    self.exposure_interval=exposure_interval
    self.ratio_interval=ratio_interval
    self.display=None
    
  def analyze(self,nml_id=None,exposure_last=None,ptfn=None):

    sx = sextractor_3.Sextractor(['EXT_NUMBER','X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE','FLUX_MAX','BACKGROUND','THRESHOLD'],sexpath='/usr/bin/sextractor',sexconfig='/usr/share/sextractor/default.sex',starnnw='/usr/share/sextractor/default.nnw',verbose=True)
    
    stdo=stde=None
    try:
      stdo,stde=sx.runSExtractor(filename=ptfn)
    except Exception as e:
      self.lg.error('exception: {}, stde: {}'.format(e,stde))
      return None,None

    if len(sx.objects)==0:
      self.lg.warn('analyze: no sextracted objects: {}'.format(ptfn))
      return None,None
  
    stde_str=stde.decode('utf-8').replace('\n','')
    #  Background: 4085.92    RMS: 241.384    / Threshold: 651.737 
    m=pat.match(stde_str)     
    if m is not None:
      background=float(m.group(1))
      background_rms=float(m.group(2))
      treshold=float(m.group(3))
    else:
      self.lg.warn('sextract: no background found for file: {}'.format(ptfn))
      return None,None
  
    background_f=True
    if background > self.background_max:
      background_f=False
      self.lg.info('sextract: background {0:6.1f} > {1:6.1f}'.format(background,self.background_max))
    
    sx.sortObjects(sx.get_field('MAG_BEST'))
    i_f = sx.get_field('FLAGS')
    try:
        sxobjs=[x for x in sx.objects if x[i_f]==0] # only the good ones
        sxobjs_len=len(sxobjs)
        self.lg.debug('analyze:id: {0}, number of sextract objects: {1}, fn: {2} '.format(nml_id,sxobjs_len,ptfn))
    except:
      self.lg.warn('analyze:id: {0}, no sextract result for: {1} '.format(nml_id,ptfn))
      return None,None
  
    if sxobjs_len < self.objects_min:
      self.lg.debug('sextract: found objects {0:6.1f} < {1:6.1f}'.format(len(sxobjs),self.objects_min))

    i_x = sx.get_field('X_IMAGE')
    i_y = sx.get_field('Y_IMAGE')
    i_m = sx.get_field('FLUX_MAX')
    i_h = sx.get_field('FWHM_IMAGE')
    # fetch from brightest object the brightest pixel value
    image_data = fits.getdata(ptfn)
    # x=y=0  : lower left
    # x=y=max : upper right
    # matplotlib x=y=0 up, max lower right
    pixel_values=list()
    x_l=list()
    y_l=list()
    for x in range(0,image_data.shape[0]):
      dx2=(x-sxobjs[0][i_y])**2
      for y in range(0,image_data.shape[1]):
        dy2=(y-sxobjs[0][i_x])**2
        if dx2 + dy2 < 10. * sxobjs[0][i_h]:
          # matplotlib!
          x_l.append(y)
          y_l.append(x)
          pixel_values.append(image_data[x][y])
          
    peak=max(pixel_values)
    peak_f=True
    if peak > self.peak_max:
      peak_f=False
      self.lg.info('sextract: peak {0:6.1f} > {1:6.1f}'.format(peak,self.peak_max))
    
    ratio= (max(pixel_values)-background)/(self.peak_max-background)
    if self.ratio_interval[0] < ratio < self.ratio_interval[0]:
      redo=False
      exposure_current=exposure_last
      self.lg.debug('analyze: exposure unchanged, not redoing')
    else:
      redo=True
      exposure_current=exposure_last / ratio
      if exposure_current > self.exposure_interval[1]:
        exposure_current=self.exposure_interval[1]
        self.lg.warn('analyze: exposure exceeds maximum, limiting')
      elif exposure_current < self.exposure_interval[0]:
        exposure_current=self.exposure_interval[0]
        self.lg.warn('analyze: exposure exceeds minimum, limiting')
      self.lg.info('analyze: exposure last: {0:6.3f}, current: {1:6.3f}, min: {2:6.3f}, max: {3:6.3f}, peak image: {4:6.1f}, value max: {5:6.1f}, ratio: {6:6.4f}'.format(exposure_last, exposure_current,self.exposure_interval[0],self.exposure_interval[1],peak,self.peak_max,ratio))

    if False:  # for debugging only
      import matplotlib.pyplot as plt
      plt.imshow(image_data, cmap='gray')
      plt.scatter(x_l,y_l, c='yellow')
      plt.show()
    
    if self.ds9_display:
      # absolute
      self.display_fits(fn=ptfn, sxobjs=sxobjs,i_x=i_x,i_y=i_y)

    return exposure_current,redo


  def display_fits(self,fn=None,sxobjs=None,i_x=None,i_y=None):
    if self.display is None:
      from pyds9 import DS9
      import time
      try:
        self.display = DS9()
        time.sleep(10.)
      except ValueError as e:
        self.lg.info('display_fits: ds9 died, retrying, error: {}'.format(e))
        return
    
    try:
      self.display.set('file {0}'.format(fn))
      self.display.set('scale zscale')
    except ValueError as e:
      self.display=None
      self.lg.info('display_fits: ds9 died, retrying, error: {}'.format(e))
      return

    for x in sxobjs:
      self.display.set('regions', 'image; circle {0} {1} 10'.format(x[i_x],x[i_y]))
      break
                                      

if __name__ == "__main__":

  import argparse
  import logging,sys
  parser= argparse.ArgumentParser(prog=sys.argv[0], description='Check exposure analysis')
  parser.add_argument('--level', dest='level', default='DEBUG', help=': %(default)s, debug level')
  parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
  args=parser.parse_args()

  filename='/tmp/{}.log'.format(sys.argv[0].replace('.py','')) # ToDo datetime, name of the script
  logformat= '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
  logging.basicConfig(filename=filename, level=args.level.upper(), format= logformat)
  logger=logging.getLogger()
    
  if args.toconsole:
    # http://www.mglerner.com/blog/?p=8
    soh=logging.StreamHandler(sys.stdout)
    soh.setLevel(args.level)
    logger.addHandler(soh)

  qa=QuickAnalysis(lg=logger,base_path='/tmp/u_point',ds9_display=True,background_max=45000.,peak_max=50000.,objects_min=100.,exposure_interval=[1.,60.])

  
  qa.analyze(nml_id=-1,image_fn='dss_16_889203_m70_085265.fits',exp=10.)

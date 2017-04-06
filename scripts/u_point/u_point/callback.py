# (C) http://scipy-cookbook.readthedocs.io/items/Matplotlib_Interactive_Plotting.html
# Thanks to scipy
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

Callbacks for matplotlib

'''

__author__ = 'wildi.markus@bluewin.ch'
import numpy as np
from operator import itemgetter

class AnnotatedPlot(object):
  def __init__(self,xx=None,nml_id=None,lon=None,lat=None,annotes=None):
    self.xx=xx # see ax....
    self.nml_id=nml_id
    self.lon=lon
    self.lat=lat
    self.annotes=annotes

class AnnoteFinder(object):
  
  drawnAnnotations = {}
  def __init__(self,ax=None, aps=None, xtol=None, ytol=None,ds9_display=None,lg=None, annotate_fn=False,analyzed=None,delete_one=None):
    self.xtol = xtol
    self.ytol = ytol
    self.ax = ax # 'leading' plot
    self.aps=aps # depending plots
    self.ds9_display=ds9_display
    self.lg=lg
    self.annotate_fn=annotate_fn
    self.analyzed=analyzed
    self.delete_one=delete_one
    self.nml_id_delete=None
    
  def distance(self, x1=None, x2=None, y1=None, y2=None):# lon,lat
    return np.sqrt((x2-x1)**2+(y2-y1)**2)
  
  def mouse_event(self, event):
    if event.inaxes:
      #self.lg.debug('mouse_event: {}'.format(event.inaxes))
      if (self.ax is None) or (self.ax is event.inaxes):
        for i,ap in enumerate(self.aps):
          if event.inaxes is ap.xx:
            ap_ax=ap
            break
        else:
          return

      clickX = event.xdata 
      clickY = event.ydata
      (c_x,c_y)=ap_ax.xx.transData.transform((clickX,clickY))
      points=list()
      for i,nid in enumerate(ap_ax.nml_id):
        # plot coordinates
        (d_x,d_y)=ap_ax.xx.transData.transform((ap_ax.lon[i],ap_ax.lat[i]))
        dst=self.distance(x1=d_x,x2=c_x,y1=d_y,y2=c_y)
        points.append([dst,nid])
            
      if points:
        # ToDo
        p_s=sorted(points, key=itemgetter(0))
        distance,nml_id=p_s[0]#sorted(points, key=itemgetter(0))[0]
        i_ap_ax_nml_id=ap_ax.nml_id.index(nml_id)
        self.nml_id_delete=nml_id
        self.lg.debug('callback::mouse_event: nml_id: {}'.format(nml_id))
  
        self.drawAnnote(nml_id,i_ap_ax_nml_id)

  def drawAnnote(self,nml_id,i_ap_ax_nml_id):
    if self.aps is None:
      return
    # i_ap_ax_nml_id fn:  1:1
    self.lg.debug('callback::drawAnnote: nml_id: {}, {}'.format(nml_id,self.aps[0].annotes[i_ap_ax_nml_id]))
    
    fn=self.aps[0].annotes[i_ap_ax_nml_id].split()[1]
    if self.ds9_display:
      self.display_fits(fn=fn)
      
    stop=False
    for ap in self.aps:          
      # attention: ax.clear() clears annotations too
      if (ap.lon[i_ap_ax_nml_id], ap.lat[i_ap_ax_nml_id]) in AnnoteFinder.drawnAnnotations:
        an = AnnoteFinder.drawnAnnotations[(ap.lon[i_ap_ax_nml_id], ap.lat[i_ap_ax_nml_id])]
        an.set_visible(not an.get_visible())
        ap.xx.figure.canvas.draw()
        # update all plots first
        stop=True
        
    if stop:
      return
    
    for ap in self.aps:
      if self.annotate_fn:
        an=ap.xx.text(ap.lon[i_ap_ax_nml_id], ap.lat[i_ap_ax_nml_id], "%s,%s\n%s" % (int(nml_id),ap.annotes[i_ap_ax_nml_id].split()[0],fn),)
      else:
        an= ap.xx.text(ap.lon[i_ap_ax_nml_id], ap.lat[i_ap_ax_nml_id], "%s,%s" % (int(nml_id),ap.annotes[i_ap_ax_nml_id].split()[0]),)

      AnnoteFinder.drawnAnnotations[(ap.lon[i_ap_ax_nml_id], ap.lat[i_ap_ax_nml_id])] = an
      ap.xx.figure.canvas.draw()
  
  def keyboard_event(self,event):
    self.lg.debug('callback::keyboard_event: key board pressed: {}'.format(event.key))

    if event.key == 'delete':
      if self.nml_id_delete is not None:
        self.lg.debug('callback::keyboard_event: delete nml_id: {}'.format(self.nml_id_delete))
        if self.nml_id_delete is not None:
          self.delete_one(nml_id=self.nml_id_delete,analyzed=False)
          if self.analyzed:
            self.delete_one(nml_id=self.nml_id_delete,analyzed=self.analyzed)#ToDo nml_id are float
            
        self.nml_id_delete=None
    # attention: ax.clear() clears annotations too
    elif event.key == 'c':
      for k,an in AnnoteFinder.drawnAnnotations.items():
        an.set_visible(False)
        
      for ap in self.aps:
        #ap.xx.figure.canvas.draw()
        ap.xx.figure.canvas.show()
        
      AnnoteFinder.drawnAnnotations = {}
      
    # ToDO not yet decided
    #ax.plot(self.randoms, 'o', picker=5)
    #fig.canvas.draw()
  # ToDo generalize
  def display_fits(self,fn=None,sxobjs=None,i_x=None,i_y=None):
    from pyds9 import DS9
    #ds9=ds9region.Ds9DisplayThread(debug=True,logger=self.lg)
    #ds9.run(fn)
    # Todo: ugly
    self.display = DS9()
    self.display.set('file {0}'.format(fn))
    self.display.set('scale zscale')

    if sxobjs is not None:
      for x in sxobjs:
        self.display.set('regions', 'image; circle {0} {1} 10'.format(x[i_x],x[i_y]))


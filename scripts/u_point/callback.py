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

import math
import ds9region
from operator import itemgetter
class AnnotatedPlot(object):
  def __init__(self,xx=None,nml_id=None,x=None,y=None,annotes=None):
    self.xx=xx # sie ax....
    self.nml_id=nml_id
    self.x=x
    self.y=y
    self.annotes=annotes

class AnnoteFinder(object):

  def __init__(self, nml_id=None,a_lon=None,a_lat=None, annotes=None, ax=None, aps=None, xtol=None, ytol=None,ds9_display=None,lg=None, annotate_fn=False,analyzed=None,delete_one=None):
    self.nml_id=nml_id
    self.data = list(zip(nml_id,a_lon,a_lat,annotes))
    self.a_lon=a_lon
    self.xtol = xtol
    self.ytol = ytol
    self.ax = ax # 'leading' plot
    self.aps=aps # depending plots
    self.drawnAnnotations = {}
    self.links = []
    self.ds9_display=ds9_display
    self.lg=lg
    self.annotate_fn=annotate_fn
    self.analyzed=analyzed
    self.delete_one=delete_one
    self.point_annotation=None
    self.nml_id_delete=None
    
  def distance(self, x1, x2, y1, y2):# lon,lat
    #a = sin²(Δφ/2) + cos φ1 ⋅ cos φ2 ⋅ sin²(Δλ/2)
    #c = 2 ⋅ atan2( √a, √(1−a) )
    #xx1= x1 /180.*math.pi
    #xx2= x2/180.*math.pi
    #yy1= y1/180.*math.pi
    #yy2= y2/180.*math.pi
    #a=pow(math.sin((yy2-yy1)/2.),2) + math.cos(yy1) * math.cos(yy2) * pow(math.sin(xx2-xx1),2)
    #c= 2. * math.atan2(math.sqrt(a),math.sqrt(1.-a))
    #return abs(c * 180. /math.pi)
    # better
    return(math.sqrt((x1 - x2)**2 + (y1 - y2)**2))
  
  def mouse_event(self, event):
    if event.inaxes:
      #self.lg.debug('mouse_event: {}'.format(event.inaxes))
      clickX = event.xdata 
      clickY = event.ydata
      if (self.ax is None) or (self.ax is event.inaxes):

        annotes = []
        ##print('candidates: {}'.format(annotes))
        for i,(nml_id, x, y, a) in enumerate(self.data):
        #  print('event  x, y: {},{}'.format(clickX, clickY))
        #  print('event dx,dy: {},{}'.format(abs(clickX-x),abs(clickY-y)))
        #  print('event xt,yt: {},{}'.format(self.xtol,self.ytol))
        #  if ((clickX-self.xtol < x < clickX+self.xtol) and (clickY-self.ytol < y < clickY+self.ytol)):
          annotes.append((self.distance(x,clickX,y,clickY),nml_id,x, y, a,i))
            
        #print('candidates: {}'.format(annotes))
        if annotes:
          # ToDo not yet good
          a_s=sorted(annotes, key=itemgetter(0))
          #annotes.sort()
          distance,nml_id,x, y,annote,i_q=a_s[0]
          #print('dist : {}, {}'.format(distance,a_s[1][0]))
          #print('click: {}, {}'.format(clickX,clickY))
          #print('sel  : {}, {}'.format(x,y))
          self.point_annotation=a_s[0]
          self.nml_id_delete=nml_id
          #self.drawAnnote(event.inaxes, x, y, annote,i)
          
          self.drawAnnote(x,y,annote,i_q,nml_id)
          self.lg.debug(annote)
          # ToDo understand
          #for l in self.links:
          #  l.drawSpecificAnnote(annote)

  def keyboard_event(self,event):
    self.lg.debug('key board pressed: {}'.format(event.key))

    if event.key == 'delete':
      if self.nml_id_delete is not None:
        self.lg.debug('keyboard_event: delete nml_id: {}, {}'.format(self.nml_id_delete,self.point_annotation))
        if self.nml_id_delete is not None:
          self.delete_one(nml_id=self.nml_id_delete,analyzed=self.analyzed)#ToDo nml_id are float
        self.nml_id_delete=None
        self.point_annotation=None
        
    # ToDO not yet decided
    #ax.plot(self.randoms, 'o', picker=5)
    #fig.canvas.draw()

  def display_fits(self,fn=None, x=None,y=None,color=None):
    ds9=ds9region.Ds9DisplayThread(debug=True,logger=self.lg)
    # Todo: ugly
    ds9.run(fn,x=x,y=y,color=color)

  def drawAnnote(self, x, y, annote,i_q,nml_id):
    """
    Draw the annotation on the plot
    """
    print(annote)
    fn=annote.split()[1]
    if self.ds9_display:
      self.display_fits(fn=fn, x=0,y=0,color='red')
          
    if (x, y) in self.drawnAnnotations:
      markers = self.drawnAnnotations[(x, y)]
      for m in markers:
        #m.set_visible(not m.get_visible())
        if self.aps is not None:
          for ap in self.aps:
            ap.xx.figure.canvas.draw_idle()       
    else:
      if self.annotate_fn:
        if self.aps is not None:
          for i,ap in enumerate(self.aps):
            t=ap.xx.text(ap.x[i_q], ap.y[i_q], "%s,%s\n%s" % (nml_id,annote.split()[0],fn),)
          
      else:
        # ToDoS t,m
        if self.aps is not None:
          for i,ap in enumerate(self.aps):
            t = ap.xx.text(ap.x[i_q], ap.y[i_q], "%s,%s" % (nml_id,annote.split()[0]),)
        
      if self.aps is not None:
        for i,ap in enumerate(self.aps):
          m=ap.xx.scatter([ap.x[i_q]], [ap.y[i_q]], marker='d', c='r', zorder=100)
        
      self.drawnAnnotations[(x, y)] = (t, m)
      if self.aps is not None:
        for i,ap in enumerate(self.aps):
          self.drawnAnnotations[(ap.x[i_q], ap.y[i_q])] = (t, m)
          try:
            ap.xx.figure.canvas.draw_idle()
          #except TclError:
          except:
            pass
            
      if self.aps is not None:
        for i,ap in enumerate(self.aps):
          try:
            ap.xx.figure.canvas.draw_idle()
          #except TclError:
          except:
            self.lg.warn('callback: drawAnnote: something went wrong, most likely plot window closed by user')

  # ToDo understand
  # def drawSpecificAnnote(self, annote):
  #  annotesToDraw = [(x, y, a) for x, y, a in self.data if a == annote]
  #  for x, y, a in annotesToDraw:
  #    self.drawAnnote(self.ax, x, y, a)



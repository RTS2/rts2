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

Acquire sky positions for a pointing model

'''

__author__ = 'wildi.markus@bluewin.ch'
import math
import ds9region
class SimpleAnnoteFinder(object):

  def __init__(self, xdata, ydata, annotes, ax=None, xtol=None, ytol=None,ds9_display=None,lg=None, annotate_fn=False):
    self.data = list(zip(xdata, ydata, annotes))
    self.xtol = xtol
    self.ytol = ytol
    if ax is None:
      self.ax = plt.gca()
    else:
      self.ax = ax
    self.drawnAnnotations = {}
    self.links = []
    self.ds9_display=ds9_display
    self.lg=lg
    self.annotate_fn=annotate_fn
  def distance(self, x1, x2, y1, y2):# lon,lat
      #a = sin²(Δφ/2) + cos φ1 ⋅ cos φ2 ⋅ sin²(Δλ/2)
      #c = 2 ⋅ atan2( √a, √(1−a) )
      xx1= x1 /180.*math.pi
      xx2= x2/180.*math.pi
      yy1= y1/180.*math.pi
      yy2= y2/180.*math.pi
      a=pow(math.sin((yy2-yy1)/2.),2) + math.cos(yy1) * math.cos(yy2) * pow(math.sin(xx2-xx1),2)
      c= 2. * math.atan2(math.sqrt(a),math.sqrt(1.-a))
      return abs(c * 180. /math.pi)
  
  def __call__(self, event):

    if event.inaxes:
      clickX = event.xdata #*180./math.pi
      clickY = event.ydata
      if (self.ax is None) or (self.ax is event.inaxes):
        annotes = []
        ##print('candidates: {}'.format(annotes))
        ##print('event x,y: {},{}'.format(clickX, clickY))
        for x, y, a in self.data:
          if ((clickX-self.xtol < x < clickX+self.xtol) and (clickY-self.ytol < y < clickY+self.ytol)):
                      
            annotes.append((self.distance(x, clickX, y, clickY), x, y, a))
        ##print('candidates: {}'.format(annotes))
        if annotes:
          annotes.sort()
          distance, x, y, annote = annotes[0]
          self.drawAnnote(event.inaxes, x, y, annote)
          for l in self.links:
            l.drawSpecificAnnote(annote)
    
  def display_fits(self,fn=None, x=None,y=None,color=None):
    ds9=ds9region.Ds9DisplayThread(debug=True,logger=self.lg)
    # Todo: ugly
    ds9.run(fn,x=x,y=y,color=color)

  def drawAnnote(self, ax, x, y, annote):
    """
    Draw the annotation on the plot
    """
    
    fn=annote.split()[1]
    if self.ds9_display:
      self.display_fits(fn=fn, x=0,y=0,color='red')
          
    if (x, y) in self.drawnAnnotations:
      markers = self.drawnAnnotations[(x, y)]
      for m in markers:
        m.set_visible(not m.get_visible())
        self.ax.figure.canvas.draw_idle()
    else:
      if self.annotate_fn:
        t = ax.text(x, y, "%s\n%s" % (annote.split()[0],fn),)
      else:
        t = ax.text(x, y, "%s" % (annote.split()[0]),)
        
      m = ax.scatter([x], [y], marker='d', c='r', zorder=100)
      self.drawnAnnotations[(x, y)] = (t, m)
      ##ax.set_rmax(90.0)
      self.ax.figure.canvas.draw_idle()

  def drawSpecificAnnote(self, annote):
    annotesToDraw = [(x, y, a) for x, y, a in self.data if a == annote]
    for x, y, a in annotesToDraw:
      self.drawAnnote(self.ax, x, y, a)






class AnnotatedPlot(object):
  def __init__(self,xx=None,x=None,y=None,annotes=None):
    self.xx=xx # sie ax....
    self.x=x
    self.y=y
    self.annotes=annotes

class AnnoteFinder(object):

  def __init__(self, a_lon=None,a_lat=None, annotes=None, ax=None, aps=None, xtol=None, ytol=None,ds9_display=None,lg=None, annotate_fn=False):
    self.data = list(zip(a_lon, a_lat, annotes))
    self.xtol = xtol
    self.ytol = ytol
    self.ax = ax # 'leading' plot
    self.aps=aps # depending plots
    self.drawnAnnotations = {}
    self.links = []
    self.ds9_display=ds9_display
    self.lg=lg
    self.annotate_fn=annotate_fn
    
  def distance(self, x1, x2, y1, y2):# lon,lat
      #a = sin²(Δφ/2) + cos φ1 ⋅ cos φ2 ⋅ sin²(Δλ/2)
      #c = 2 ⋅ atan2( √a, √(1−a) )
      xx1= x1 /180.*math.pi
      xx2= x2/180.*math.pi
      yy1= y1/180.*math.pi
      yy2= y2/180.*math.pi
      a=pow(math.sin((yy2-yy1)/2.),2) + math.cos(yy1) * math.cos(yy2) * pow(math.sin(xx2-xx1),2)
      c= 2. * math.atan2(math.sqrt(a),math.sqrt(1.-a))
      return abs(c * 180. /math.pi)
  
  def __call__(self, event):
    if event.inaxes:
      clickX = event.xdata 
      clickY = event.ydata
      if (self.ax is None) or (self.ax is event.inaxes):
        annotes = []
        ##print('candidates: {}'.format(annotes))
        ##print('event x,y: {},{}'.format(clickX, clickY))
        for i,(x, y, a) in enumerate(self.data):
          if ((clickX-self.xtol < x < clickX+self.xtol) and (clickY-self.ytol < y < clickY+self.ytol)):
                      
            annotes.append((self.distance(x, clickX, y, clickY), x, y, a,i))
        ##print('candidates: {}'.format(annotes))
        if annotes:
          annotes.sort()
          distance, x, y, annote,i = annotes[0]
          self.drawAnnote(event.inaxes, x, y, annote,i)
          # ToDo understand
          #for l in self.links:
          #  l.drawSpecificAnnote(annote)
  
  def display_fits(self,fn=None, x=None,y=None,color=None):
    ds9=ds9region.Ds9DisplayThread(debug=True,logger=self.lg)
    # Todo: ugly
    ds9.run(fn,x=x,y=y,color=color)

  def drawAnnote(self, ax, x, y, annote,i):
    """
    Draw the annotation on the plot
    """
    
    fn=annote.split()[1]
    if self.ds9_display:
      self.display_fits(fn=fn, x=0,y=0,color='red')
          
    if (x, y) in self.drawnAnnotations:
      markers = self.drawnAnnotations[(x, y)]
      for m in markers:
        m.set_visible(not m.get_visible())
        self.ax.figure.canvas.draw_idle()
        for ap in self.aps:
          ap.xx.figure.canvas.draw_idle()       
    else:
      if self.annotate_fn:
        t = ax.text(x, y, "%s\n%s" % (annote.split()[0],fn),)
        for ap in self.aps:
          t=ap.xx.text(ap.x[i], ap.y[i], "%s\n%s" % (annote.split()[0],fn),)
          
      else:
        t = ax.text(x, y, "%s" % (annote.split()[0]),)
        for ap in self.aps:
          t = ap.xx.text(ap.x[i], ap.y[i], "%s" % (annote.split()[0]),)
        
      m = ax.scatter([x], [y], marker='d', c='r', zorder=100)
      for ap in self.aps:
        m=ap.xx.scatter([ap.x[i]], [ap.y[i]], marker='d', c='r', zorder=100)
        
      self.drawnAnnotations[(x, y)] = (t, m)
      for ap in self.aps:
        self.drawnAnnotations[(ap.x[i], ap.y[i])] = (t, m)

      self.ax.figure.canvas.draw_idle()
      for ap in self.aps:
        ap.xx.figure.canvas.draw_idle()
  # ToDo understand
  # def drawSpecificAnnote(self, annote):
  #  annotesToDraw = [(x, y, a) for x, y, a in self.data if a == annote]
  #  for x, y, a in annotesToDraw:
  #    self.drawAnnote(self.ax, x, y, a)



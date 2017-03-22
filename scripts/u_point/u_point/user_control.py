#!/usr/bin/env python3
# (C) 2017, Markus Wildi, wildi.markus@bluewin.ch
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

class UserControl(object):
  def __init__(self,lg=None,base_path=None):
    self.lg=lg
    self.base_path=base_path
    
  def create_socket(self, port=9999):
    # acquire runs as a subprocess of rts2-scriptexec and has no
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

#!/usr/bin/python
# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
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

__author__ = 'markus.wildi@bluewin.ch'

import logging
import sys

class Logger(object):
    """Define the logger for rts2saf

    :var debug: enable more debug output with --debug and --level
    :var logformat: format string
    :var args: command line arguments or their defaults


    """
    def __init__(self, 
                 debug=False, 
                 logformat='%(asctime)s:%(name)s:%(levelname)s:%(message)s', 
                 args=None):

        self.debug=debug
        self.logformat=logformat
        self.args=args

        ok= True
        try:
            logging.basicConfig(filename='{0}/{1}'.format(self.args.toPath, self.args.logfile), level=self.args.level.upper(), format=self.logformat)
        except Exception, e:
            ok=False

        if not ok:
            #/rts2saf_focus.py.log
            lgFn='/tmp/{0}'.format(self.args.logfile.split('/')[-1])
            logging.basicConfig(filename=lgFn, level=self.args.level.upper(), format=self.logformat)
            
        self.logger = logging.getLogger()

        if args.level in 'DEBUG':
            toconsole=True
        else:
            toconsole=args.toconsole

        if toconsole:
            # http://www.mglerner.com/blog/?p=8
            soh = logging.StreamHandler(sys.stdout)
            soh.setLevel(args.level)
            self.logger.addHandler(soh)

        if not ok:
            self.logger.warn('logging to: {0} instead of {1}, error:\n{2}'.format(lgFn, self.args.logfile, e))


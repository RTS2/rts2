# (C) 2014, Markus Wildi, markus.wildi@bluewin.ch
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

import unittest
from rts2saf.log import Logger

import logging
logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

class Args(object):
    def __init__(self):
        pass

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestLogging('test_accessVar'))
    suite.addTest(TestLogging('test_accessTmp'))
    suite.addTest(TestLogging('test_accessRoot'))

    return suite

#@unittest.skip('class not yet implemented')
class TestLogging(unittest.TestCase):

    #@unittest.skip('feature not yet implemented')
    def test_accessVar(self):
        logger.info('== {} =='.format(self._testMethodName))
        args=Args()
        args.toPath='/var/log'
        args.logfile= 'rts2-debug'
        args.level='INFO'
        args.toconsole=False
        lg=Logger(debug=False,args=args)
        # it is a weak assertion since Logger is not yet really unittestable (for developemnt purposes only)
        self.assertIsInstance(obj=lg, cls=Logger, msg='returned object: {} is not of type Logger'.format(type(lg)))

    #@unittest.skip('feature not yet implemented')
    def test_accessTmp(self):
        logger.info('== {} =='.format(self._testMethodName))
        args=Args()
        args.toPath='/tmp/rts2saf_log'
        args.logfile= 'rts2-debug'
        args.level='INFO'
        args.toconsole=False
        lg=Logger(debug=False,args=args)
        # it is a weak assertion since Logger is not yet really unittestable (for developemnt purposes only)
        self.assertIsInstance(obj=lg, cls=Logger, msg='returned object: {} is not of type Logger'.format(type(lg)))


    #@unittest.skip('feature not yet implemented')
    def test_accessRoot(self):
        logger.info('== {} =='.format(self._testMethodName))
        args=Args()
        args.toPath='/'
        args.logfile= 'rts2-debug'
        args.level='INFO'
        args.toconsole=False
        lg=Logger(debug=False,args=args)
        # it is a weak assertion since Logger is not yet really unittestable (for developemnt purposes only)
        self.assertIsInstance(obj=lg, cls=Logger, msg='returned object: {} is not of type Logger'.format(type(lg)))



if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

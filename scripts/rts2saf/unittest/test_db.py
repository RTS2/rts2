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
import os
import re
import pwd

import psycopg2

from rts2saf.config import Configuration 

import logging
if not os.path.isdir('/tmp/rts2saf_log'):
    os.mkdir('/tmp/rts2saf_log')


logging.basicConfig(filename='/tmp/rts2saf_log/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()


comment = re.compile('^;.*')
dbname= re.compile('^[ \t]*name[ \t]*=[ \t]*(\w+)')
dbusername= re.compile('^[ \t]*username[ \t]*=[ \t]*(\w+)')
dbpasswd= re.compile('^[ \t]*password[ \t]*=[ \t]*(\w+)')

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestDatabase('test_readRts2Configuration'))
    suite.addTest(TestDatabase('test_dbReadAccessTarget'))
    suite.addTest(TestDatabase('test_dbReadAccessCCDScript'))

    return suite

#@unittest.skip('class not yet implemented')
class TestDatabase(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        # ToDo: ugly
        self.dbName = None
        self.dbUser = None
        self.dbPasswd = None
        self.openedRts2Ini=False
        try:
            with open('/etc/rts2/rts2.ini') as ini:
                self.openedRts2Ini=True
                
                for ln in ini:
                    mc = comment.match(ln)
                    if mc:
                        continue

                    mn = dbname.match(ln)
                    if mn:
                        self.dbName= mn.group(1)
                        continue

                    mu = dbusername.match(ln)
                    if mu:
                        self.dbUser= mu.group(1)
                        continue

                    mp = dbpasswd.match(ln)
                    if mp:
                        self.dbPasswd= mp.group(1)
                        continue

                        
        except Exception, e:
            print 'error: {}'.format(e)

        # last resort
        if not self.dbName:
            self.dbName ='stars'
        if not self.dbUser:
            self.dbUser = pwd.getpwuid(os.getuid()).pw_name
        if not self.dbPasswd:
            self.dbPasswd =''

    def test_readRts2Configuration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.openedRts2Ini,'return value: {},  coud not read rts2.ini'.format(self.openedRts2Ini))

    def test_dbReadAccessTarget(self):
        logger.info('== {} =='.format(self._testMethodName))
        entry=(5, 'o', 'OnTargetFocus', None, None, 'this target does not change the RA/DEC values', True, 1, None, None, None, None, True, None, None, None)
        conn = psycopg2.connect('dbname={} user={} password={}'.format(self.dbName, self.dbUser, self.dbPasswd))
        crsr = conn.cursor()
        crsr.execute('SELECT *  FROM targets WHERE tar_id=5 ;')
        result=crsr.fetchone()
        crsr.close()
        conn.close()
        self.assertEqual(entry, result, 'return value:{}'.format(result))


    def test_dbReadAccessCCDScript(self):
        logger.info('== {} =='.format(self._testMethodName))
        entry=(' exe /usr/local/bin/rts2saf_focus.py E 1 ')

        conn = psycopg2.connect('dbname={} user={} password={}'.format(self.dbName, self.dbUser, self.dbPasswd))
        crsr = conn.cursor()
        crsr.execute('SELECT *  FROM scripts WHERE tar_id=5 ;')
        result=crsr.fetchone()
        crsr.close()
        conn.close()
        self.assertEqual(entry, result[2], 'return value:>>{}<<'.format(result[2]))



if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

# (C) 2017, Markus Wildi, markus.wildi@bluewin.ch
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

basepath='/tmp/u_point_unittest'
os.makedirs(basepath, exist_ok=True)
import logging
logging.basicConfig(filename=os.path.join(basepath,'unittest.log'), level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()


comment = re.compile('^;.*')
dbname= re.compile('^[ \t]*name[ \t]*=[ \t]*(\w+)')
dbusername= re.compile('^[ \t]*username[ \t]*=[ \t]*(\w+)')
dbpasswd= re.compile('^[ \t]*password[ \t]*=[ \t]*(\w+)')

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestDatabase('test_read_rts2_configuration'))
    suite.addTest(TestDatabase('test_db_read_access_users'))
    suite.addTest(TestDatabase('test_db_write_access_users'))

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

                        
        except Exception as e:
            logger.error('error: {}'.format(e))

        # last resort
        if not self.dbName:
            self.dbName ='stars'
        if not self.dbUser:
            self.dbUser = pwd.getpwuid(os.getuid()).pw_name
        if not self.dbPasswd:
            self.dbPasswd =''
    #@unittest.skip('feature not yet implemented')
    def test_read_rts2_configuration(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.assertTrue(self.openedRts2Ini,'return value: {},  coud not read rts2.ini'.format(self.openedRts2Ini))
    #@unittest.skip('feature not yet implemented')
    def test_db_read_access_users(self):
        logger.info('== {} =='.format(self._testMethodName))  
        entry=('pghttpd', None, 'abc@cde.ch', 3, True, '$6$WyQQAQr3$PKw3sF.mJSuh73PPzgkYuCmOSpGwj8mGrBn3yx2pBaeuM.tSCmtGETEQLQGweyQCtzsI2YjApeCpEkXeLfwqq0', 'W0 C0 F0 W0 T0 HTTPD')
        conn = psycopg2.connect('dbname={} user={} password={}'.format(self.dbName, self.dbUser, self.dbPasswd))
        crsr = conn.cursor()
        crsr.execute('SELECT *  FROM users WHERE usr_login=\'pghttpd\';')
        result=crsr.fetchone()
        crsr.close()
        conn.close()
        self.assertEqual(entry, result, 'return value:{}'.format(result))
    #@unittest.skip('feature not yet implemented')
    def test_db_write_access_users(self):
        #  UPDATE users SET usr_execute_permission='t', allowed_devices = 'W0 C0 F0 W0 T0 HTTPD' WHERE usr_login='pghttpd' ;
        logger.info('== {} =='.format(self._testMethodName))  
        conn = psycopg2.connect('dbname={} user={} password={}'.format(self.dbName, self.dbUser, self.dbPasswd))
        crsr = conn.cursor()
        crsr.execute('UPDATE users SET usr_execute_permission=\'t\', allowed_devices = \'W0 C0 F0 W0 T0 HTTPD\' WHERE usr_login=\'pghttpd\' ;')
        rc=crsr.rowcount
        self.assertEqual(1, rc, 'return value:{}'.format(rc))
        
if __name__ == '__main__':
    
    unittest.TextTestRunner(verbosity=0).run(suite())

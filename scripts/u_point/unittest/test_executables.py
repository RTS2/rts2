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
import subprocess
from pathlib import Path
import pwd

import os
import re
basepath='/tmp/u_point_unittest'
os.makedirs(basepath, exist_ok=True)
import logging
logging.basicConfig(filename=os.path.join(basepath,'unittest.log'), level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

from rts2_environment import RTS2Environment

# sequence matters
def suite_with_connection():
    suite = unittest.TestSuite()
    #suite.addTest(TestAcquisition('test_u_acquire'))
    #suite.addTest(TestAcquisition('test_u_acquire_dss'))
    #suite.addTest(TestAcquisition('test_u_acquire_rts2_dummy_dss'))
    #suite.addTest(TestAcquisition('test_u_acquire_rts2_dummy_httpd_dss'))
    suite.addTest(TestAcquisition('test_u_acquire_rts2_dummy_httpd_dss_bright_stars'))
    #suite.addTest(TestAcquisition('test_acquisition'))
    return suite

def suite_no_connection():
    suite = unittest.TestSuite()
    suite.addTest(TestAnalysisModel('test_u_analyze_u_model'))
    #suite.addTest(TestAnalysisModel('test_analysis_model'))
    return suite

#@unittest.skip('class not yet implemented')
class TestAnalysisModel(unittest.TestCase):
    def tearDown(self):
        pass

    def setUp(self):
        pass
    
    def exec_cmd(self,cmd=None,using_shell=False):
        logger.info('== setUp: {} ==='.format(cmd[0]))
        logger.info('executing: {}'.format(cmd))
        proc  = subprocess.Popen( cmd, shell=using_shell, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdo, stde = proc.communicate()
        stdo_l = stdo.decode("utf-8").split('\n')
        for ln  in stdo_l:
            logger.info('TestAcquisition::setUp:stdo:{}: {}'.format(cmd[0],ln))
        
        stde_l = stde.decode("utf-8").split('\n')
        for ln  in stde_l:
            logger.info('TestAcquisition::setUp:stde:{}: {}'.format(cmd[0],ln))

        return stdo_l,stde_l
    
    #@unittest.skip('feature not yet implemented')
    def test_u_analyze_u_model(self):
        logger.info('== {} =='.format(self._testMethodName))
        cmd=['../u_analyze.py','--toc','--base-path', basepath, '--level', 'DEBUG']
        stdo_l,stde_l=self.exec_cmd(cmd=cmd)
        # ToDo only quick, add meaningful asserts
        
        cmd=['../u_model.py','--toc','--base-path', basepath, '--level', 'DEBUG']
        stdo_l,stde_l=self.exec_cmd(cmd=cmd)
        # ToDo only quick, add meaningful asserts
        #ToDo somting wrong, float: ([ -+]?\d*\.\d+|\d+)
        m=re.compile('MA : polar axis left-right alignment[ ]?:([-+]?\d*.*?)\[arcsec\]')
        val=None
        for o_l in stdo_l:
            print(o_l)
            v = m.match(o_l)
            if v:
                print('match')
                val = abs(float(v.group(1)))
                break
        val_max=3.
        if val is not None:
            self.assertLess(val,val_max, msg='return value: {}, instead of max: {}'.format(val, val_max))
        else:
            self.assertEqual(1.,val_max, msg='return value: None, instead of max: {}'.format(val_max))
            
    @unittest.skip('feature not yet implemented')
    def test_analysis_model(self):
        logger.info('== {} =='.format(self._testMethodName))


#@unittest.skip('class not yet implemented')
class TestAcquisition(RTS2Environment):

    #setUp, tearDown see base class
    def setUpCmds(self):
        fn=Path(os.path.join(basepath,'observable.cat'))
        if not fn.is_file():
            # ./u_select.py  --toc
            cmd=[ '../u_select.py', '--base-path', basepath ]
            self.exec_cmd(cmd=cmd)
        
        fn=Path(os.path.join(basepath,'nominal_positions.nml'))
        if not fn.is_file():
            # ./u_acquire.py  --create --toc --eq-mount
            cmd=[ '../u_acquire.py', '--create','--toc', '--eq-mount', '--base-path', basepath, '--force-overwrite', '--lon-step', '40', '--lat-step', '20']
            self.exec_cmd(cmd=cmd)
            
        self.exec_cmd(cmd=cmd)
        
    def exec_cmd(self,cmd=None,using_shell=False):
        logger.info('== setUp: {} ==='.format(cmd[0]))
        logger.info('executing: {}'.format(cmd))
        proc  = subprocess.Popen( cmd, shell=using_shell, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdo, stde = proc.communicate()
        stdo_l = stdo.decode("utf-8").split('\n')
        for ln  in stdo_l:
            logger.info('TestAcquisition::setUp:stdo:{}: {}'.format(cmd[0],ln))
        
        stde_l = stde.decode("utf-8").split('\n')
        for ln  in stde_l:
            logger.info('TestAcquisition::setUp:stde:{}: {}'.format(cmd[0],ln))

        return stdo_l,stde_l
    
    @unittest.skip('feature not yet implemented')
    def test_u_acquire(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setUpCmds()
        cmd=['../u_acquire.py','--toc','--device-class','DeviceDss','--base-path', basepath]
        stdo_l,stde_l=self.exec_cmd(cmd=cmd)

    @unittest.skip('feature not yet implemented')
    def test_u_acquire_dss(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setUpCmds()
        cmd=['../u_acquire.py','--toc','--device-class','DeviceDss','--fetch-dss','--base-path', basepath]
        stdo_l,stde_l=self.exec_cmd(cmd=cmd)

    @unittest.skip('feature not yet implemented')
    def test_u_acquire_rts2_dummy_dss(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setUpCmds()
        # ToDo ugly
        uid=self.uid=pwd.getpwuid(os.getuid())[0]
        acq_script='/home/{}/rts2/scripts/u_point/unittest/u_acquire_fetch_dss_continuous.sh'.format(uid)
        cmd=['rts2-scriptexec', '--port', '1617','-d','C0','-s',' exe {} '.format(acq_script)]
        stdo_l,stde_l=self.exec_cmd(cmd=cmd, using_shell=False)
        
    @unittest.skip('feature not yet implemented')
    def test_u_acquire_rts2_dummy_httpd_dss(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setUpCmds()
        cmd=['../u_acquire.py','--toc','--device-class','DeviceRts2Httpd','--fetch-dss','--base-path', basepath, '--level', 'DEBUG']
        stdo_l,stde_l=self.exec_cmd(cmd=cmd)

    #@unittest.skip('feature not yet implemented')
    def test_u_acquire_rts2_dummy_httpd_dss_bright_stars(self):
        logger.info('== {} =='.format(self._testMethodName))
        self.setUpCmds()
        cmd=['../u_acquire.py','--toc','--device-class','DeviceRts2Httpd','--fetch-dss','--use-bright-stars', '--base-path', basepath, '--level', 'DEBUG']
        stdo_l,stde_l=self.exec_cmd(cmd=cmd)

    @unittest.skip('feature not yet implemented')
    def test_acquisition(self):
        logger.info('== {} =='.format(self._testMethodName))
        pass
    
if __name__ == '__main__':
    
    suiteNoConnection = suite_no_connection()
    suiteWithConnection = suite_with_connection()
    # a list is a list: breaking unittest independency, ok it's Python
    alltests = unittest.TestSuite([suiteWithConnection,suiteNoConnection])
    #unittest.TextTestRunner(verbosity=0).run(alltests)
    unittest.TextTestRunner(verbosity=0).run(suiteNoConnection)

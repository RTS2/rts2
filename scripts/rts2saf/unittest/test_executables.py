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

import unittest
import datetime
import subprocess
import re

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()


from test_focus import TestFocus

# sequence matters
def suite_no_connection():
    suite = unittest.TestSuite()
    suite.addTest(TestExecutables('test_rts2saf_fwhm'))
    suite.addTest(TestExecutables('test_rts2saf_imgp'))
    suite.addTest(TestExecutables('test_rts2saf_analyze'))
    suite.addTest(TestExecutables('test_rts2saf_analyze_assoc'))

    return suite

def suite_with_connection():
    suite = unittest.TestSuite()
    suite.addTest(TestRts2safFocus('test_rts2saf_focus'))

    return suite


#@unittest.skip('class not yet implemented')
class TestExecutables(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        pass

    #@unittest.skip('feature not yet implemented')
    def test_rts2saf_fwhm(self):
        # ../rts2saf_fwhm.py  --fitsFn ../samples/20071205025911-725-RA.fits --toc
        # sextract: no FILTA name information found, ../samples/20071205025911-725-RA.fits
        # sextract: no FILTB name information found, ../samples/20071205025911-725-RA.fits
        # sextract: no FILTC name information found, ../samples/20071205025911-725-RA.fits
        # rts2af_fwhm: no focus run  queued, fwhm:  2.77 < 35.00 (thershold)
        # rts2af_fwhm: DONE

        m = re.compile('.*?(no focus run  queued, fwhm:)  (2.77)')
        cmd=[ '../rts2saf_fwhm.py',  '--fitsFn', '../samples/20071205025911-725-RA.fits', '--conf', './rts2saf-bootes-2-autonomous.cfg', '--toc' ]
        proc  = subprocess.Popen( cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout_value, stderr_value = proc.communicate()
        lines = stdout_value.split('\n')
        val=0.
        for ln  in lines:
            v = m.match(ln)
            if v:
                val = float(v.group(2))
                break

        self.assertEqual(val, 2.77, 'return value: {}'.format(repr(stdout_value)))
        self.assertEqual(stderr_value, '', 'return value: {}'.format(repr(stderr_value)))


    #@unittest.skip('feature not yet implemented')
    def test_rts2saf_imgp(self):
        # rts2saf_imgp.py: starting
        # rts2saf_imgp.py, rts2-astrometry-std-fits.net: corrwerr 1 0.3624045465 39.3839441225 -0.0149071686 -0.0009854536 0.0115640672
        # corrwerr 1 0.3624045465 39.3839441225 -0.0149071686 -0.0009854536 0.0115640672
        # ...
        m = re.compile('.*?(corrwerr).+? ([0-9.]+)')
        cmd=[ '../rts2saf_imgp.py',   '../imgp/20131011054939-621-RA.fits', '--conf', './rts2saf-bootes-2-autonomous.cfg', '--toc' ]
        proc  = subprocess.Popen( cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout_value, stderr_value = proc.communicate()
        lines = stdout_value.split('\n')
        val=0.
        for ln  in lines:
            v = m.match(ln)
            if v:
                val = float(v.group(2))
                break
        # if this test fails due to no match, it is likely that astrometry could not solve field.
        self.assertAlmostEqual(val, 0.3624045465, places=1, msg='return value: {}'.format(val))
        self.assertEqual(stderr_value, '', 'return value: {}'.format(repr(stderr_value)))
        
    #@unittest.skip('feature not yet implemented')
    def test_rts2saf_analyze(self):
        # ...
        # analyze: storing plot file: ../samples/UNK-2013-11-23T09:24:58.png
        # analyze: FWHM FOC_DEF:  5437 : fitted minimum position,  2.2px FWHM, NoTemp ambient temperature
        # ResultMeans: FOC_DEF:  5363 : weighted mean derived from sextracted objects
        # ResultMeans: FOC_DEF:  5377 : weighted mean derived from FWHM
        # ResultMeans: FOC_DEF:  5367 : weighted mean derived from std(FWHM)
        # ResultMeans: FOC_DEF:  5382 : weighted mean derived from CombinedFWHM
        # analyzeRuns: ('NODATE', 'NOFTW') :: NOFT 14 
        # rts2saf_analyze: no ambient temperature available in FITS files, no model fitted
        m = re.compile('.*?(FOC_DEF:)  ([0-9]{4,4}).+?([0-9\.]{3,3})px')
        cmd=[ '../rts2saf_analyze.py',   '--basepath', '../samples', '--conf', './rts2saf-bootes-2-autonomous.cfg', '--toc' ]
        proc  = subprocess.Popen( cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout_value, stderr_value = proc.communicate()
        lines = stdout_value.split('\n')
        pos=0
        val=0.
        for ln  in lines:
            v = m.match(ln)
            if v:
                
                pos = int(v.group(2))
                val = float(v.group(3))
                break

        self.assertEqual(pos, 5437, 'return value: {}'.format(pos))
        self.assertEqual(val, 2.2, 'return value: {}'.format(val))
        self.assertEqual(stderr_value, '', 'return value: {}'.format(repr(stderr_value)))


    #@unittest.skip('feature not yet implemented')
    def test_rts2saf_analyze_assoc(self):
        m = re.compile('.*?(FOC_DEF:)  ([0-9]{4,4}).+?([0-9\.]{3,3})px')
        cmd=[ '../rts2saf_analyze.py',   '--associate', '--basepath', '../samples', '--conf', './rts2saf-bootes-2-autonomous.cfg', '--toc' ]
        proc  = subprocess.Popen( cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout_value, stderr_value = proc.communicate()
        lines = stdout_value.split('\n')
        pos=0
        val=0.
        for ln  in lines:
            v = m.match(ln)
            if v:
                
                pos = int(v.group(2))
                val = float(v.group(3))
                break

        self.assertEqual(pos, 5435, 'return value: {}'.format(pos))
        self.assertEqual(val, 2.1, 'return value: {}'.format(val))
        self.assertEqual(stderr_value, '', 'return value: {}'.format(repr(stderr_value)))


#@unittest.skip('class not yet implemented')
class TestRts2safFocus(TestFocus):

    def test_rts2saf_focus(self):
        # analyze: FWHM FOC_DEF:  5426 : fitted minimum position,  2.2px FWHM, NoTemp ambient temperature
        m = re.compile('.*?(FOC_DEF:)  ([0-9]{4,4}).+? ([0-9\.]{3,3})px')
        cmd=[ '../rts2saf_focus.py',   '--dryfitsfiles', '../samples', '--conf', './rts2saf-bootes-2-autonomous.cfg', '--toc' ]
 
        proc  = subprocess.Popen( cmd, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout_value, stderr_value = proc.communicate()
        lines = stdout_value.split('\n')
        pos=0
        val=float('nan')
        for ln  in lines:
            print ln
            v = m.match(ln)
            if v:
                
                pos = int(v.group(2))
                val = float(v.group(3))
                break

        self.assertEqual(pos, 5426, 'return value: {}'.format(pos))
        self.assertEqual(val, 2.2, 'return value: {}'.format(val))
        self.assertEqual(stderr_value, '', 'return value: {}'.format(repr(stderr_value)))

if __name__ == '__main__':
    
    suiteNoConnection = suite_no_connection()
    suiteWithConnection = suite_with_connection()
    alltests = unittest.TestSuite([suiteNoConnection, suiteWithConnection])
    unittest.TextTestRunner(verbosity=0).run(alltests)

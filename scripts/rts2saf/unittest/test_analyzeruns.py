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
import glob
import sys 


from rts2saf.config import Configuration 
from rts2saf.environ import Environment
from rts2saf.analyzeruns import AnalyzeRuns
from rts2saf.data import ResultFit

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestAnalyzeRuns('test_analyze_runs'))
    suite.addTest(TestAnalyzeRuns('test_analyze_runs_assoc'))

    return suite

class Args(object):
    def __init__(self):
        pass


#@unittest.skip('class not yet implemented')
class TestAnalyzeRuns(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='./rts2saf-flux.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)
        self.ev=Environment(debug=False, rt=self.rt,logger=logger)
        self.args=Args()
        self.args.catalogAnalysis = True
        self.args.Ds9Display = False
        self.args.FitDisplay = False
        self.args.criteria = 'rts2saf.criteria_radius'
        self.args.basePath = '../samples'
        self.args.filterNames = None
        self.args.sxDebug=False
        self.args.flux=True
        self.args.fractObjs=0.5

    #@unittest.skip('feature not yet implemented')
    def test_analyze_runs(self):
        self.args.associate = False
        aRs = AnalyzeRuns(basePath = self.args.basePath, args = self.args, rt = self.rt, ev = self.ev, logger = logger)
        aRs.aggregateRuns()
        rFf = aRs.analyzeRuns()
        self.assertEqual(type(rFf[0]), ResultFit, 'return value: {}'.format(type(rFf[0])))

    #@unittest.skip('feature not yet implemented')
    def test_analyze_runs_assoc(self):
        self.args.associate = True
        aRs = AnalyzeRuns(basePath = self.args.basePath, args = self.args, rt = self.rt, ev = self.ev, logger = logger)
        aRs.aggregateRuns()
        rFf = aRs.analyzeRuns()
        
        self.assertEqual(type(rFf[0]), ResultFit, 'return value: {}'.format(type(rFf[0])))


if __name__ == '__main__':
    
    suite=suite()
    alltests = unittest.TestSuite([suite])
    unittest.TextTestRunner(verbosity=0).run(alltests)

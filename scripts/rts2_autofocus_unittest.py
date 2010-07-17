#!/usr/bin/python
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#   unit tests for 
#   rts_autofocus.py 
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
# required packages

__author__ = 'markus.wildi@one-arcsec.org'


import unittest
from rts2af import Configuration


class testConfiguration(unittest.TestCase):

    # ToDo def setUp(self):
    # ToDo  def tearDown():

    def test_configuration(self):

        dc= Configuration()
        values={}

        for (section, identifier), value in dc.configIdentifiers():
            values[identifier]= value
#            print 'value ', section,  ' ', identifier, ' ', value, ' ', values[identifier]
            self.assertEqual( value, values[identifier])


    def test_basic(self):

        dc= Configuration()
        values={}
        for (section, identifier), value in dc.configIdentifiers():
            values[identifier]= value

        # int
        i= 1 + values['NUMBER_OF_AUTO_FOCUS_IMAGES']
        self.assertEqual( i, 11)
        # float
        f= 1. + values['ELLIPTICITY']
        self.assertEqual( f, 1.1)
        # bool
        self.assertTrue( values['TAKE_DATA'])
        self.assertEqual( dc.defaultsValue('SEXPRG'), 'sex 2>/dev/null') 

        for filter in dc.filtersInUse:
# ToDo find an assert
            print 'filters--------' + filter

suite = unittest.TestLoader().loadTestsFromTestCase(testConfiguration)
unittest.TextTestRunner(verbosity=2).run(suite)


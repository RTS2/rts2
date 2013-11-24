#!/bin/bash
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


if ! [ -e '/usr/local/bin/sex' ]
then
    echo 'SExtractor as /usr/local/bin/sex not found'
    echo 'Either create a link or change SEXPATH in all ./unittest/rts2saf-*cfg'
    exit
fi


if [ $1 ] 
then
 dir=$1
else
 dir=./unittest
fi
 cd $dir


python -m unittest discover -v -s .

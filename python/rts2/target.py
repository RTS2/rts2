# Library for RTS2 JSON calls.
# (C) 2012 Petr Kubanek, Institute of Physics
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA

from . import rtsapi


class Target:

    def __init__(self, id, name=None):
        self.id = id
        self.name = name

    def reload(self):
        """Load target data from JSON interface."""
        if self.id is None:
            name = None
            return
        try:
            data = rtsapi.getProxy().loadJson(
                '/api/tbyid', {'id': self.id})['d'][0]
            self.name = data[1]
        except Exception as ex:
            self.name = None


def get(name):
    """Return array with targets matching given name or target ID"""
    try:
        return rtsapi.getProxy().loadJson('/api/tbyid', {'id': int(name)})['d']
    except ValueError:
        return rtsapi.getProxy().loadJson('/api/tbyname', {'n': name})['d']


def create(name, ra, dec):
    return rtsapi.getProxy().loadJson('/api/create_target', {'tn': name, 'ra': ra, 'dec': dec})['id']


def list():
    return rtsapi.getProxy().loadJson('/api/tlist')


def print_list(ls):
    """Converts JSON target list to string for printing."""
    def print_target(t):
        if t[2] is None or t[3] is None:
            return "{0:>5d} {1:20s} {2} {3} {4:30s}".format(t[0], t[1], t[2], t[3], t[4])
        return "{0:>5d} {1:20s} {2:.2f} {3:.2f} {4:30s}".format(t[0], t[1], t[2], t[3], t[4])
    return '\n'.join([print_target(t) for t in ls['d']])

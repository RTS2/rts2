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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

from future.standard_library import install_aliases

install_aliases()

try:
    import base64
    import json as sysjson
    import http.client
    from urllib.parse import urlencode
    import re
    import string
    import os
    import socket
    import threading
except:
    raise


DEVICE_TYPE_SERVERD = 1
DEVICE_TYPE_MOUNT = 2
DEVICE_TYPE_CCD = 3
DEVICE_TYPE_DOME = 4
DEVICE_TYPE_WEATHER = 5
DEVICE_TYPE_ROTATOR = 6
DEVICE_TYPE_PHOT = 7
DEVICE_TYPE_PLAN = 8
DEVICE_TYPE_GRB = 9
DEVICE_TYPE_FOCUS = 10
DEVICE_TYPE_MIRROR = 11
DEVICE_TYPE_CUPOLA = 12
DEVICE_TYPE_FW = 13
DEVICE_TYPE_AUGERSH = 14
DEVICE_TYPE_SENSOR = 15

DEVICE_TYPE_EXECUTOR = 20
DEVICE_TYPE_IMGPROC = 21
DEVICE_TYPE_SELECTOR = 22
DEVICE_TYPE_XMLRPC = 23
DEVICE_TYPE_INDI = 24
DEVICE_TYPE_LOGD = 25
DEVICE_TYPE_SCRIPTOR = 26

# value types
RTS2_VALUE_WRITABLE = 0x02000000

RTS2_VALUE_BASETYPE = 0x0000000f

RTS2_VALUE_STRING = 0x00000001
RTS2_VALUE_INTEGER = 0x00000002
RTS2_VALUE_TIME = 0x00000003
RTS2_VALUE_DOUBLE = 0x00000004
RTS2_VALUE_FLOAT = 0x00000005
RTS2_VALUE_BOOL = 0x00000006
RTS2_VALUE_SELECTION = 0x00000007
RTS2_VALUE_LONGINT = 0x00000008
RTS2_VALUE_RADEC = 0x00000009
RTS2_VALUE_ALTAZ = 0x0000000A

RTS2_VALUE_STAT = 0x00000010
RTS2_VALUE_MMAX = 0x00000020

RTS2_VALUE_STAT = 0x10

# Python < 2.7 does not provide _MAXLINE
try:
    http.client._MAXLINE
except AttributeError:
    http.client._MAXLINE = 2000


class ChunkResponse(http.client.HTTPResponse):

    read_by_chunks = False

    def __init__(
        self, sock, debuglevel=0, strict=0,
        method=None, buffering=False
    ):
        # Python 2.6 does not have buffering
        try:
            http.client.HTTPResponse.__init__(
                self, sock, debuglevel=debuglevel,
                strict=strict, method=method, buffering=buffering
            )
        except TypeError as te:
            http.client.HTTPResponse.__init__(
                self, sock, debuglevel=debuglevel,
                strict=strict, method=method
            )

    def _read_chunked(self, amt):
        if not(self.read_by_chunks):
            return http.client.HTTPResponse._read_chunked(self, amt)

        assert self.chunked != http.client._UNKNOWN
        line = self.fp.readline(http.client._MAXLINE + 1)
        if len(line) > http.client._MAXLINE:
            raise http.client.LineTooLong("chunk size")
        i = line.find(';')
        if i >= 0:
            line = line[:i]
        try:
            chunk_left = int(line, 16)
        except ValueError:
            self.close()
            raise http.client.IncompleteRead('')
        if chunk_left == 0:
            return ''
        ret = self._safe_read(chunk_left)
        self._safe_read(2)
        return ret

# Handles db stuff. Internal class, visible through Rts2JSON.db


class _Rts2JSON__db:

    def __init__(self, json):
        self.json = json

    def createTarget(self, name, ra, dec, info='', comment=''):
        return self.json.loadJson(
            '/api/create_target',
            {
                'tn': name, 'ra': ra, 'dec': dec,
                'info': info, 'comment': comment
            }
        )

    def createTLETarget(self, name, tle1, tle2, comment=''):
        return self.json.loadJson(
            '/api/create_tle_target',
            {
                'tn': name, 'tle1': tle1, 'tle2': tle2, 'comment': comment
            }
        )


class Rts2JSON:

    def __init__(
        self, url='http://localhost:8889', username=None, password=None,
        verbose=False, http_proxy=None
    ):
        use_proxy = False
        prefix = ''
        # use proxy only if not connecting to localhost
        if url.find('localhost') == -1 and (
            'http_proxy' in os.environ or http_proxy
        ):
            if not(re.match('^http', url)):
                prefix = 'http://' + url
            if http_proxy:
                prefix = url
            url = os.environ['http_proxy']
            use_proxy = True

        url = re.sub('^http://', '', url)
        slash = url.find('/')
        if slash >= 0:
            if not(use_proxy):
                prefix = url[slash:]
            self.host = url[:slash]
        else:
            self.host = url

        a = self.host.split(':')
        self.port = 80
        if len(a) > 2:
            raise Exception('too much : separating port and server')
        elif len(a) == 2:
            self.host = a[0]
            self.port = int(a[1])

        self.db = __db(self)

        self.headers = {'Authorization': 'Basic' + string.strip(
            base64.encodestring('{0}:{1}'.format(username, password)))}
        self.prefix = prefix
        self.verbose = verbose
        self.hlib_lock = threading.Lock()
        self.hlib = self.newConnection()

    def newConnection(self):
        r = http.client.HTTPConnection(self.host, self.port)
        r.response_class = ChunkResponse
        if self.verbose:
            r.set_debuglevel(5)
        return r

    def getResponse(self, path, args=None, hlib=None):
        url = self.prefix + path
        if args:
            url += '?' + urlencode(args)
        if self.verbose:
            print('retrieving {0}'.format(url))
        try:
            th = hlib
            if hlib is None:
                self.hlib_lock.acquire()
                th = self.hlib
            th.request('GET', url, None, self.headers)

            r = None
            try:
                r = th.getresponse()
            except http.client.BadStatusLine as ex:
                # try to reload
                th = self.newConnection()
                if hlib is None:
                    self.hlib = th
                th.request('GET', url, None, self.headers)
                r = th.getresponse()
            except http.client.ResponseNotReady as ex:
                # try to reload
                th = self.newConnection()
                if hlib is None:
                    self.hlib = th
                th.request('GET', url, None, self.headers)
                r = th.getresponse()

            if not(r.status == http.client.OK):
                jr = sysjson.load(r)
                raise Exception(jr['error'])
            return r
        except Exception as ec:
            if self.verbose:
                import traceback
                traceback.print_exc()
                print('Cannot parse', url, ':', ec)
            raise ec
        finally:
            if hlib is None:
                self.hlib_lock.release()

    def loadData(self, path, args={}, hlib=None):
        return self.getResponse(path, args, hlib).read()

    def loadJson(self, path, args=None):
        d = self.loadData(path, args)
        if self.verbose:
            print(d)
        return sysjson.loads(d)

    def chunkJson(self, r):
        r.read_by_chunks = True
        d = r.read()
        if self.verbose:
            print(d)
        r.read_by_chunks = False
        return sysjson.loads(d)


class JSONProxy(Rts2JSON):

    """Connection with managed cache of variables."""

    def __init__(
        self, url='http://localhost:8889', username=None, password=None,
        verbose=False, http_proxy=None
    ):
        Rts2JSON.__init__(self, url, username, password, verbose, http_proxy)
        self.devices = {}
        self.selection_cache = {}

    def refresh(self, device=None):
        if device is None:
            dall = self.loadJson('/api/getall', {'e': 1})
            self.devices = dict([(x, dall[x]['d']) for x in dall])

        else:
            self.devices[device] = self.loadJson(
                '/api/get', {'d': device, 'e': 1})['d']

    def isIdle(self, device):
        return self.loadJson('/api/get', {'d': device, 'e': 1})['idle']

    def getState(self, device):
        return self.loadJson('/api/get', {'d': device})['state']

    def getDeviceInfo(self, device):
        return self.loadJson('/api/deviceinfo', {'d': device})

    def getDevice(self, device, refresh_not_found=False):
        try:
            return self.devices[device]
        except KeyError as ke:
            if refresh_not_found is False:
                raise ke
            self.refresh(device)
            return self.devices[device]

    def getVariable(self, device, value, refresh_not_found=False):
        try:
            return self.devices[device][value]
        except KeyError as ke:
            if refresh_not_found is False:
                raise ke
            self.refresh(device)
            return self.devices[device][value]

    def getValue(self, device, value, refresh_not_found=False):
        """Return full (extended) format of the value. You probably better
           use getSingleValue."""
        return self.getVariable(device, value, refresh_not_found)[1]

    def getValueError(self, device, value, refresh_not_found=False):
        return self.getFlags(device, value, refresh_not_found) & 0x20000000

    def getValueWarning(self, device, value, refresh_not_found=False):
        return self.getFlags(device, value, refresh_not_found) & 0x10000000

    def getFlags(self, device, value, refresh_not_found=False):
        """Return value flags. Please see value.h for meaning of individual
           bits."""
        return self.getVariable(device, value, refresh_not_found)[0]

    def getSingleValue(self, device, value, refresh_not_found=False):
        """Return single value - e.g. for stat values, return only value, not
           statics array."""
        var = self.getVariable(device, value, refresh_not_found)
        f = var[0]
        v = var[1]
        if f & RTS2_VALUE_STAT:
            return v[1]
        return v

    def getSelection(self, device, name):
        try:
            return self.selection_cache[device][name]
        except KeyError as k:
            rep = self.loadJson('/api/selval', {'d': device, 'n': name})
            try:
                self.selection_cache[device][name] = rep
            except KeyError as k2:
                self.selection_cache[device] = {}
                self.selection_cache[device][name] = rep
            return rep

    def setValue(self, device, name, value, async=None):
        values = {'d': device, 'n': name, 'v': value, 'async': async}
        if async:
            values['async'] = async
        self.loadJson('/api/set', values)

    def incValue(self, device, name, value):
        return self.loadJson('/api/inc', {'d': device, 'n': name, 'v': value})

    def setValues(self, values, device=None, async=None):
        if device:
            values = dict([('{0}.{1}'.format(device, x[0]), x[1])
                          for x in list(values.items())])
        if async:
            values['async'] = async
        self.loadJson('/api/mset', values)

    def executeCommand(self, device, command, async=False):
        ret = self.loadJson(
            '/api/cmd',
            {
                'd': device, 'c': command, 'e': 1, 'async': 1 if async else 0
            }
        )
        if async:
            return 0
        self.devices[device] = ret['d']
        return ret['ret']

    def getDevicesByType(self, device_type):
        return self.loadJson('/api/devbytype', {'t': device_type})

    def lastImage(self, device):
        """Returns image from the last exposure."""
        return self.getResponse('/api/lastimage', {'ccd': device}).read()

    def takeImage(self, camera):
        """Returns image from camera as array. Exposure parameters should be
           set before starting exposure with setValue calls."""
        r = self.getResponse('/api/exposedata', {'ccd': camera})
        return self.getImage(r)

    def getImage(self, r):
        import struct
        import numpy

        if not(r.getheader('Content-type') == 'binary/data'):
            raise Exception(
                'wrong header, expecting binary/data, received:',
                r.getheader('Content-type'), r.read()
            )

        tb = int(r.getheader('Content-length'))

        imh = r.read(44)

        (
            data_type, naxes, w, h, a3, a4, a5, bv, bh, b3, b4, b5,
            shutter, filt, x, y, chan
        ) = struct.unpack('!hhiiiiihhhhhhhhhH', imh)

        # map data_typ to numpy type
        rts2_2_numpy = {
            8: numpy.uint8, 16: numpy.int16, 32: numpy.int32,
            64: numpy.int64,
            -32: numpy.float, -64: numpy.double,
            10: numpy.uint8, 20: numpy.uint16, 40: numpy.uint32
        }
        dt = rts2_2_numpy[data_type]

        a = numpy.empty((h, w), dt)

        for row in range(h):
            line = r.read(a.itemsize * w)
            a[row] = numpy.fromstring(line, dtype=dt)

        return a


def getProxy():
    global __jsonProxy
    return __jsonProxy


def set_proxy(jsonProxy):
    global __jsonProxy
    __jsonProxy = jsonProxy


def createProxy(
    url, username=None, password=None, verbose=False, http_proxy=None
):
    global __jsonProxy
    __jsonProxy = JSONProxy(
        url=url, username=username, password=password, verbose=verbose,
        http_proxy=http_proxy
    )
    return __jsonProxy

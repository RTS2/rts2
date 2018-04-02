#!/usr/bin/python
#
# Basic libnova calculatons
# (C) 2011 - 2016, Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

from numpy import cos, sin, arctan2, tan, sqrt, radians, degrees, arcsin, arccos, arctan, seterr, divide


def angular_separation(ra1, dec1, ra2, dec2):
    a1 = radians(ra1)
    d1 = radians(dec1)
    a2 = radians(ra2)
    d2 = radians(dec2)

    return degrees(angular_separation_rad(a1, d1, a2, d2))


def angular_separation_rad(a1, d1, a2, d2):
    x = (cos(d1) * sin(d2)) - (sin(d1) * cos(d2) * cos(a2 - a1))
    y = cos(d2) * sin(a2 - a1)
    z = (sin(d1) * sin(d2)) + (cos(d1) * cos(d2) * cos(a2 - a1))

    x = x * x
    y = y * y
    return arctan2(sqrt(x + y), z)


def equ_to_hrz(ra, dec, lst, latitude):
    """ Transform RA-DEC (in degrees) vector to ALT-AZ (in degrees) vector"""
    ha = radians(lst - ra)
    dec = radians(dec)
    latitude_r = radians(latitude)
    A = sin(latitude_r) * sin(dec) + cos(latitude_r) * cos(dec) * cos(ha)
    alt = arcsin(A)

    Z = arccos(A)
    Zs = sin(Z)

    old_err_state = seterr(divide='ignore', invalid='ignore')
    As = divide(cos(dec) * sin(ha), Zs)
    Ac = divide(sin(latitude_r) * cos(dec) *
                cos(ha) - cos(latitude_r) * sin(dec), Zs)
    seterr(**old_err_state)
    Aa = arctan2(As, Ac)

    return degrees(alt), (degrees(Aa) + 360.0) % 360.0


def hrz_to_equ(az, alt, latitude):
    """ Transform AZ-ALT (in degrees) vector to HA-DEC (in degrees) vector"""
    latitude_r = radians(latitude)
    az = radians(az)
    alt = radians(alt)

    ha = arctan2(sin(az), cos(az) * sin(
        latitude_r) + tan(alt) * cos(latitude_r))
    dec = sin(latitude_r) * sin(alt) - cos(latitude_r) * cos(alt) * cos(az)
    dec = arcsin(dec)

    return (degrees(ha) + 360.0) % 360.0, degrees(dec)


def parallactic_angle(latitude, ha, dec):
    latitude_r = radians(latitude)
    ha_r = radians(ha)
    dec_r = radians(dec)
    div = sin(latitude_r) * cos(dec_r) - \
        cos(latitude_r) * sin(dec_r) * cos(ha_r)
    return degrees(parallactic_angle_rad(latitude_r, ha_r, dec_r))


def parallactic_angle_rad(latitude_r, ha_r, dec_r):
    div = sin(latitude_r) * cos(dec_r) - \
        cos(latitude_r) * sin(dec_r) * cos(ha_r)
    return arctan2(cos(latitude_r) * sin(ha_r), div)


def hrz_to_vect(az, el):
    """Transform az/alt pair to XYZ vector.
    :param az: azimuth (radians)
    :param el: elevation (radians)
    :return [x,y,z]: unit vector representing
    """
    return [cos(az) * cos(el), sin(az) * cos(el), sin(el)]


def vect_to_hrz(x, y, z):
    r = sqrt(x**2 + y**2 + z**2)
    el = arcsin(z / r)
    az = arctan2(y,  x)
    return [az, el]

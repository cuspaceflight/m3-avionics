"""Conversion between lat/lon and ECEF.
   Simon Crane 2017

Attributes:
    a (float): equatorial radius of earth
    b (float): radius of earth at poles
    f (float): something about flatness
    c (float): speed of light

Example usage:
    home = (52, 10.1, 300)
    e = convert_polar_to_ECEF ( home )
    p = convert_ECEF_to_polar( e )
"""
from math import *

a = 63781370.0 # equatorial radius of Earth
b = 63567523.0 # radius of Earth at poles
f = 1 - 1/298.257224 # something about flatness

c = 299792458.0 #speed of light


def earth_radius(lat):
    R = ( ( (a**2 * cos ( lat * pi/180)) ** 2 + (b**2 * sin( lat * pi/180))**2 ) / ( (a * cos (lat * pi/180) )**2 + (b * sin (lat * pi/180) ) **2 ) ) ** 0.5
    return R

def convert_polar_to_ECEF( p_coords ):
    r = earth_radius( p_coords[0] ) + p_coords[2]
    #r = a / ( (cos (p_coords[0] * pi/180 ) ) **2 + f*2 * ( sin ( p_coords[0] * pi/180 ) )**2 ) + p_coords[2]
    x = r * cos ( p_coords[0] * pi/180 ) * cos( p_coords[1] * pi/180 )
    y = r * cos ( p_coords[0] * pi/180 ) * sin( p_coords[1] * pi/180 )
    z = r * sin ( p_coords[0] * pi/180 )
    return (x, y, z)

def convert_ECEF_to_polar( ec_coords ):
    r = (ec_coords[0]**2 + ec_coords[1]**2 + ec_coords[2]**2) ** 0.5
    lat = asin( ec_coords[2] / r) * 180/pi
    long = atan( ec_coords[1] / ec_coords[0])* 180/pi
    alt = r - earth_radius( lat )
    return (lat, long, alt)

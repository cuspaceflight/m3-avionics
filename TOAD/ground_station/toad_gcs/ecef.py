"""Conversion between lat/lon and ECEF.
   Simon Crane, Gregory Brooks 2017

Attributes:
    a (float):   equatorial radius of earth
    b (float):   radius of earth at poles
    e (float):   first eccentricity
    e_2 (float): second eccentricity

Example usage:
    home = (52, 10.1, 300)
    e = convert_llh_to_ECEF ( home )
    p = convert_ECEF_to_llh( e )
"""
from math import *

a = 6378137.0 # equatorial radius of Earth
b = 6356752.314245179 # radius of Earth at poles
e = (1-(b/a)**2)**0.5  # first eccentricity
e_2 = ((a/b)**2 - 1)**0.5  # second eccentricity


#f = 1 - 1/298.257224 # something about flatness

#c = 299792458.0 #speed of light


def N(lat):
    #R = ( ( (a**2 * cos ( lat * pi/180)) ** 2 + (b**2 * sin( lat * pi/180))**2 ) / ( (a * cos (lat * pi/180) )**2 + (b * sin (lat * pi/180) ) **2 ) ) ** 0.5
    phi = lat*pi/180
    R = a**2 / ( (a*cos(phi))**2 + (b*sin(phi))**2 )**0.5
    return R

def convert_llh_to_ECEF( p_coords ):
    r = N( p_coords[0] ) + p_coords[2]
    #r = a / ( (cos (p_coords[0] * pi/180 ) ) **2 + f*2 * ( sin ( p_coords[0] * pi/180 ) )**2 ) + p_coords[2]
    x = r * cos ( p_coords[0] * pi/180 ) * cos( p_coords[1] * pi/180 )
    y = r * cos ( p_coords[0] * pi/180 ) * sin( p_coords[1] * pi/180 )
    z = ( (b/a)**2 * N( p_coords[0] ) + p_coords[2]) * sin(p_coords[0] * pi/180)
    return (x, y, z)

def convert_ECEF_to_llh( ec_coords ):
    # r = (ec_coords[0]**2 + ec_coords[1]**2 + ec_coords[2]**2) ** 0.5
    # lat = asin( ec_coords[2] / r) * 180/pi
    # lon = atan( ec_coords[1] / ec_coords[0])* 180/pi
    # alt = r - earth_radius( lat )

    # Taken from Wikipedia (https://en.wikipedia.org/wiki/Geographic_coordinate_conversion#From_geodetic_to_ECEF_coordinates)
    X = ec_coords[0]
    Y = ec_coords[1]
    Z = ec_coords[2]

    r = (X**2 + Y**2)**0.5
    E_2 = a**2 - b**2
    F = 54*(b**2)*Z**2
    G = r**2 + (1 - e**2)*Z**2 - e**2*E_2
    C = e**4 * F * r**2 / G**3
    S = (1 + C + (C**2 + 2*C)**0.5)**(1/3)
    P = F/(3*(S + 1/S + 1)**2 * G**2)
    Q = (1 + 2*e**4 *P)**0.5
    r_0 = -P*e**2*r/(1+Q)
    r_0 += (0.5*a**2*(1+1/Q) - (P*(1-e**2)*Z**2)/(Q*(1+Q)) - 0.5*P*r**2)**0.5
    U = ( (r - e**2 * r_0)**2 + Z**2 )**0.5
    V = ( (r - e**2 * r_0)**2 + (1 - e**2 )*Z**2 )**0.5
    Z_0 = (b**2 * Z) / (a*V)

    alt = U*(1 - b**2/(a*V))

    lat = atan( (Z + e_2**2 * Z_0)/r )
    lat = lat*180/pi

    lon = atan2(Y,X)
    lon = lon*180/pi

    return (lat, lon, alt)

def test():
    res = convert_llh_to_ECEF([40.9,-119.1,1191])  # Black Rock Desert
    print("ECEF: ",res)
    print("LLH: ",convert_ECEF_to_llh(res))

if __name__ == "__main__":
    test()

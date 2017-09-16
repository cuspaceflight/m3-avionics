"""Conversion between lat/lon, ECEF and ENU.
   Simon Crane, Gregory Brooks 2017

Example usage:
    home = (52, 10.1, 300)
    e = convert_llh_to_ECEF ( home )
    p = convert_ECEF_to_llh( e )
"""
from math import *
import numpy as np

a = 6378137.0 # equatorial radius of Earth
b = 6356752.314245179 # radius of Earth at poles
e = (1-(b/a)**2)**0.5  # first eccentricity
e_2 = ((a/b)**2 - 1)**0.5  # second eccentricity

enu_ref_llh = []  # Origin of ENU coordinate system, llh
enu_ref_ecef = []
enu_ref_mat = np.zeros((3,3))  # Rotation matrix
enu_inv_mat = np.zeros((3,3))

#f = 1 - 1/298.257224 # something about flatness

#c = 299792458.0 #speed of light

def N(lat):
    #R = ( ( (a**2 * cos ( lat * pi/180)) ** 2 + (b**2 * sin( lat * pi/180))**2 ) / ( (a * cos (lat * pi/180) )**2 + (b * sin (lat * pi/180) ) **2 ) ) ** 0.5
    phi = radians(lat)
    R = a**2 / ( (a*cos(phi))**2 + (b*sin(phi))**2 )**0.5
    return R

def convert_llh_to_ECEF( p_coords ):
    r = N( p_coords[0] ) + p_coords[2]
    #r = a / ( (cos (p_coords[0] * pi/180 ) ) **2 + f*2 * ( sin ( p_coords[0] * pi/180 ) )**2 ) + p_coords[2]
    x = r * cos ( radians(p_coords[0]) ) * cos( radians(p_coords[1]) )
    y = r * cos ( radians(p_coords[0]) ) * sin( radians(p_coords[1]) )
    z = ( (b/a)**2 * N( p_coords[0] ) + p_coords[2]) * sin(radians(p_coords[0]))
    return (x, y, z)

def convert_ECEF_to_llh( ec_coords ):
    # r = (ec_coords[0]**2 + ec_coords[1]**2 + ec_coords[2]**2) ** 0.5
    # lat = asin( ec_coords[2] / r) * 180/pi
    # lon = atan( ec_coords[1] / ec_coords[0])* 180/pi
    # alt = r - earth_radius( lat )

    # Taken from Wikipedia (https://en.wikipedia.org/wiki/Geographic_coordinate_conversion#From_geodetic_to_ECEF_coordinates)
    # Ferrari's solution:
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

    lat = degrees( atan( (Z + e_2**2 * Z_0)/r ) )

    lon = degrees( atan2(Y,X) )

    return (lat, lon, alt)

def convert_ECEF_to_ENU( ec_coords ):
    ecef_vector = np.array([ec_coords[0] - enu_ref_ecef[0],
                            ec_coords[1] - enu_ref_ecef[1],
                            ec_coords[2] - enu_ref_ecef[2]])
    xyz = np.dot(enu_ref_mat, ecef_vector)
    return(xyz[0],xyz[1],xyz[2])

def convert_ENU_to_ECEF( enu_coords ):
    xyz = np.array([enu_coords[0],enu_coords[1],enu_coords[2]])
    XYZ = np.dot(enu_inv_mat, xyz)
    XYZ += np.array([enu_ref_ecef[0],enu_ref_ecef[1],enu_ref_ecef[2]])
    return(XYZ[0],XYZ[1],XYZ[2])

def convert_llh_to_ENU( p_coords ):
    return convert_ECEF_to_ENU(convert_llh_to_ECEF(p_coords))

def convert_ENU_to_llh( enu_coords ):
    return convert_ECEF_to_llh(convert_ENU_to_ECEF(enu_coords))

def set_enu_ref(llh = [40.883683,-119.0781,1191]):  # Black Rock Desert
    global enu_ref_llh
    global enu_ref_ecef
    global enu_ref_mat
    global enu_inv_mat
    enu_ref_llh = llh
    lat_r = radians(enu_ref_llh[0])
    lon_r = radians(enu_ref_llh[1])
    h_r = enu_ref_llh[2]
    enu_ref_ecef = convert_llh_to_ECEF(llh)

    enu_ref_mat = np.array([(-sin(lon_r)           , cos(lon_r)            , 0         ),
                            (-sin(lat_r)*cos(lon_r), -sin(lat_r)*sin(lon_r), cos(lat_r)),
                            (cos(lat_r)*cos(lon_r) , cos(lat_r)*sin(lon_r) , sin(lat_r)) ])

    enu_inv_mat = np.linalg.inv(enu_ref_mat)


def test():
    set_enu_ref()
    llh = (40.9,-119.0,1190)
    print("LLH:     ", llh)
    res = convert_llh_to_ECEF(llh)
    print("ECEF:    ",res)
    res2 = convert_ECEF_to_ENU(res)
    print("ENU:     ", res2)
    res3 = convert_ENU_to_ECEF(res2)
    print("ECEF(2): ", res3)
    print("LLH(2):  ",convert_ECEF_to_llh(res3))

if __name__ == "__main__":
    test()

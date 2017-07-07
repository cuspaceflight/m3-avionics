#!/usr/bin/env python3

"""
Speedy trilateration algorithm.

USAGE NOTES HERE.
"""

# Derivation: Y. Zhou, "An Efficient Least-Squares Trilateration Algorithm for Mobile Robot Localization".
# Matlab implementations in TOAD documentation.

import sys
import math
import numpy as np

def speedy_trilat(p_i, r_i, n=3):
    """
    Returns position estimate from ground station locations and distances between them and tracked objects.
    
    Args:
        p_i: nxN numpy array containing position vectors of reference points
        r_i: N element row vector (numpy array) containing distances from each reference point
          n: number of dimensions, 2 or 3 (defaults to 3)
    
    Returns:
        Position of tracked object as an n element column vector (numpy array)
    
    Raises:
        n/a
    """   
    #Step 1: Calculate useful variables
    N = p_i.shape[1]
    pt_p = p_i * p_i
    pt_ps = np.sum(pt_p, axis=0)
    a = p_i * pt_ps[np.newaxis,:] - p_i * (r_i*r_i)[np.newaxis,:]
    a = np.sum(a, axis=1)
    
    B = -2 * np.dot(p_i, np.transpose(p_i)) + np.sum(r_i*r_i - pt_ps) * np.identity(n)
    
    c = np.sum(p_i, axis=1)
    
    a = a/N
    B = B/N
    c = c/N
    print("a:")
    print(a)
    print("")
    print("sizeof a=")
    print(a.shape)
    print("")
    print("B:")
    print(B)
    print("")
    print("c:")
    print(c)
    print("")
    a = a[:,np.newaxis]      # Vector
    c_mat = c[:,np.newaxis]  # Single dimension matrix
    print(c)
    f = a + np.dot(B,c_mat) + 2*np.dot( np.dot( c_mat, np.transpose(c_mat) ) , c_mat )
    print("sizeof c = ")
    print(c.shape)
    print("")

    print("f:")
    print(f)

    print("")
    H = ( np.dot(p_i, np.transpose(p_i)) + 2*N*np.dot(c_mat, np.transpose(c_mat)) ) * (-2/N)
    
    
    print("H:")
    print(H)
    print("")
    
    fprime = f - f[n-1]
    fprime = np.delete(fprime, (n-1), axis=0)
    Hprime = H - H[n-1,:][np.newaxis,:]
    Hprime = np.delete(Hprime, (n-1), axis=0)


    print("fprime:")
    print(fprime)
    print("")
    print("Hprime")
    print(Hprime)
    print("")


    Q,U = np.linalg.qr(Hprime)
    
    
    pos=Q,U
    return pos

print(speedy_trilat(np.array([
    [7,3,3,4],
    [5,9,9,9],
    [9,10,111,112]]),
    np.array([9,8,7,6]),3))

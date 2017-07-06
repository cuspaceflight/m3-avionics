#!/usr/bin/env python3

"""
Speedy trilateration algorithm.

USAGE NOTES HERE.
"""

# Derivation: Y. Zhou, "An Efficient Least-Squares Trilateration Algorithm for Mobile Robot Localization".
# Matlab implementations in TOAD documentation.

import sys
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
    pos=a
    return pos

print(speedy_trilat(np.array([
    [1,2,3,4],
    [5,6,7,8],
    [9,10,11,12]]),
    np.array([1,2,3,4])))

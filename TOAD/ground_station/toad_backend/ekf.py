"""Extended Kalman Filter for estimating rocket trajectory based on noisy TOAD data.

Attributes:
    #kalman filter parameters/coefficients

Todo:
    Everything
"""

import numpy as np
from pyquaternion import Quaternion

class State:
    """Creates object to describe state of tracked object.

    Attributes:
        time (float): time of state estimate in seconds
        position (3x1 numpy array): position vector
        velocity (3x1 numpy array): 3x1 velocity vector
        acceleration (3x1 numpy array): 3x1 acceleration vector
        angular_position (pyquaternion Quaternion): orientation quaternion
        angular_velocity (pyquaternion Quaternion): angular velocity quaternion
    """
    def __init__(self,t,pos,vel,acc,ang_p,ang_v):
        # Check vector dimensions
        assert(np.shape(position) == (3,1))
        assert(np.shape(velocity) == (3,1))
        assert(np.shape(acceleration) == (3,1))

        self.time = t
        self.position = pos
        self.velocity = vel
        self.acceleration = acc
        self.angular_position = ang_p
        self.angular_velocity =

    #Methods for packing/unpacking to/from struct?

def update(state,dt):
    """Update state (advance by one timestep)
    Args:
        state: the previous state (to update)
        dt: timestep duration in seconds
        ###measurement data###

    Returns:
        The new state (State object)

    Raises:
    """

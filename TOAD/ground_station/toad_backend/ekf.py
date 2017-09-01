"""Extended Kalman Filter for estimating rocket trajectory based on noisy TOAD data.

Attributes:
    #kalman filter parameters/coefficients, accessed via getter functions so that values depend on stage
        #^^^Move this up to state estimator? so ekf module is independent of prev states

Todo:
    Everything
"""

import numpy as np
#from pyquaternion import Quaternion
from enum import Enum

class Stage(Enum):
    # Modify ekf behaviour depending on flight stage
    PAD = 0
    LIFTOFF = 1
    POWERED_ASCENT = 2
    FREE_ASCENT = 3
    APOGEE = 4
    DROGUE_DESCENT = 5
    MAIN_DESCENT = 6
    LANDED = 7
    DYNAMIC_EVENT = 8  # Increase process noise variance in response
                       # to rapid changes e.g. motor ignition

class State:
    """Creates object to describe state of tracked object.

    Attributes:
        time (float): time of state estimate in seconds
        thrust (float): scalar thrust from rocket motor
        mass (float): mass of tracked object
        position (3x1 numpy array): position vector
        velocity (3x1 numpy array): 3x1 velocity vector
        acceleration (3x1 numpy array): 3x1 acceleration vector
        stage (enum Stage): flight stage
    """
    def __init__(self,ti,th,m,pos,vel,acc,st):
        # Check vector dimensions
        assert(np.shape(position) == (3,1))
        assert(np.shape(velocity) == (3,1))
        assert(np.shape(acceleration) == (3,1))

        self.time = ti
        self.thrust = th
        self.mass = m
        self.position = pos
        self.velocity = vel
        self.acceleration = acc
        self.stage = st

    def forwards(self):
        """Return unit vector in the direction rocket is facing.
        """
        # Assume rocket is pointing in direction of motion
        return self.velocity/np.linalg.norm(self.velocity)

    #Methods for packing/unpacking to/from struct?

def update(state,dt):
    """Update state (advance by one timestep)
    Args:
        state: the previous state (to update)
        dt: timestep duration in seconds
        ###measurement data incl. new stage ###

    Returns:
        The new state (State object)

    Raises:
    """

"""
Extended Kalman Filter for estimating rocket trajectory based on noisy TOAD data.
"""

import numpy as np
from pyquaternion import Quaternion
from functools import singledispatch

#Global state variable, use getter/setter functions to access
_state = {'time':0.0, 'position':np.zeros((3,1)), 'velocity':np.zeros((3,1)),
            'acceleration':np.zeros((3,1)), 'angular_position':Quaternion(),
            'angular_velocity':Quaternion() }


def update(dt):
    """
    Update the state dictionary.
    Args:
        dt: timestep duration in seconds
        ###measurement data###

    Returns:
        n/a

    Raises:
    """


def get_state():
    """
    Return the current state estimate (getter function).

    Args:
        None

    Returns:
        Dict containing state of tracked object
    """
    return _state


def set_state_dict(newdict):
    """
    Enter a new current state as a dictionary, ensuring dictionary keys match
    (setter function).

    Args:
        newdict: dictionary containing state to be set

    Returns:
        n/a

    Raises:
        KeyError if argument and state dictionary keys do not match.
    """
    if _state.keys() == newdict.keys():
        _state = newdict
    else:
        raise KeyError("Argument keys do not match state dictionary")


def set_state(time, position, velocity, acceleration, angular_position,
                angular_velocity):
    """
    Enter a new current state by specifying each variable as a separate
    argument (setter function).

    Args:
        time: time of state estimate in seconds
        position: 3x1 position vector
        velocity: 3x1 velocity vector
        acceleration: 3x1 acceleration vector
        angular_position: quaternion describing orientation
        angular_velocity: quaternion describing angular velocity

    Returns:
        n/a

    Raises:
        AssertionError if vector dimensions are not 3x1.
    """

    # Check vector dimensions
    assert(np.shape(position) == (3,1))
    assert(np.shape(velocity) == (3,1))
    assert(np.shape(acceleration) == (3,1))

    # Assign to state
    _state = {'time':time, 'position':position, 'velocity':velocity,
                'acceleration':acceleration, 'angular_position':angular_position,
                'angular_velocity':angular_velocity }

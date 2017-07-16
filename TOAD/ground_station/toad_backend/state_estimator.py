"""
Top level state estimation process, updates state with each incoming TOAD packet.

Attributes:
    global_state (ekf.State object): state of tracked object. Shouldn't need to
        be accessed from outside the state_estimator thread


Todo:
    - Main loop
    - Utilise ECEF conversion
    - Find a way to obtain timetagged imu data from m3gcs
    - Transfer states to command processor e.g. in a pipe
    - Log states to file
"""
import numpy as np
#from pyquaternion import Quaternion
from . import ekf
from multiprocessing import Pipe
from .trilateration import *
from .ekf import State,Stage, update

def run(toad_in, cmd_pipe):
    """
    """
    #read toad data from pipe
    #perform trilateration and state estimation
    #log states and pipe them to command processor

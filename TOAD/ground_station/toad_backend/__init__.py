import multiprocessing
import numpy as np
from pyquaternion import Quaternion

def run():
    #Initialise parallel processes (ekf, logging etc.)
    # Advance global state upon receipt of each new message
    # Respond to commands from frontend

    shared_mgr = multiprocessing.Manager()
    global_state = shared_mgr.dict(time=0.0,
        position=np.zeros((3,1)), velocity=np.zeros((3,1)), acceleration=np.zeros((3,1)),
        angular_position=Quaternion(), angular_velocity=np.zeros((3,1)) )

import multiprocessing
import numpy as np
from pyquaternion import Quaternion

def run():
    #Initialise parallel processes (ekf, logging etc.)
    # Advance global state upon receipt of each new message
    # Respond to commands from frontend

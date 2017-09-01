#import numpy as np
#from pyquaternion import Quaternion
import multiprocessing

from . import logging
from . import usb
from . import state_estimator
from . import command_processor

def run(frontend_pipe):
    """Initialise and run the TOAD backend.

    Args:
        frontend_pipe (multiprocessing.Pipe end): pipe used to communicate with frontend
                note that this may be replaced depending on technology used for frontend

    Returns:

    Raises:
    """
    # Create communication links between processes:

    # Pipe TOAD data from USB to state estimator
    estimator_parsed_pipe,usb_parsed_pipe = multiprocessing.Pipe(duplex=False)

    # Pipe between state estimator and command processor
    cm_st_pipe, st_cm_pipe = multiprocessing.Pipe(True)

    # Command pipe between USB process and command processor
    usb_cm_pipe, cm_usb_pipe = multiprocessing.Pipe(True)



    # Start usb parsing process
    usb_process = multiprocessing.Process(target=usb.run, args=(usb_parsed_pipe,usb_cm_pipe))
    usb_process.start()

    # Start state_estimation process
    state_est_process = multiprocessing.Process(target=state_estimator.run, args=(estimator_parsed_pipe,st_cm_pipe))
    state_est_process.start()

    # Start command processor
    commands_process = multiprocessing.Process(target=command_processor.run, args=(cm_st_pipe,cm_usb_pipe,frontend_pipe))
    commands_process.start()

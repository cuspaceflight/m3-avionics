"""GUI Process
Gregory Brooks 2017

Attributes:

Todo:
    Everything
"""

from multiprocessing import Pipe
from .ekf import Stage
# define command classes here

def run(state_est_pipe,usb_pipe,frontend_pipe):
    while True:
        # read commands from frontend
        # get flight stage from m3gcs (e.g. POWERED_ASCENT)
        # send instructions to backend processes

import glob
import os.path
import argparse
import multiprocessing

shared_mgr = multiprocessing.Manager()
global_state = shared_mgr.dict()

def update_state(parent, name, logstring):
    global shared_mgr, global_state
    key = "{}-{}".format(parent, name)
    global_state[key] = logstring
    
from . import command_processor
from . import webapp

from . import m3pyro

parser = argparse.ArgumentParser()
parser.add_argument("--port", help="Path to serial port on m3debug or radio",
                    default="/dev/serial/by-id/*m3debug*-if02")
args = parser.parse_args()

def run():
    if "*" in args.port:
        port = glob.glob(args.port)
        if len(port) == 0:
            raise RuntimeError("No port matching glob found")
        args.port = os.path.realpath(port[0])

    command_processor.run(port=args.port, state=global_state)
    webapp.run(state=global_state)

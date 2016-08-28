import glob
import os.path
import argparse
import multiprocessing
    
from . import command_processor
from . import webapp
from .packets import registered_packets

from . import m3pyro, m3fc, m3psu, m3radio


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="Path to serial port on m3debug or "
                                       "radio",
                        default="/dev/serial/by-id/*m3debug*-if02")
    args = parser.parse_args()

    if "*" in args.port:
        port = glob.glob(args.port)
        if len(port) == 0:
            raise RuntimeError("No port matching glob found")
        args.port = os.path.realpath(port[0])
    
    shared_mgr = multiprocessing.Manager()
    global_state = shared_mgr.dict({c:{"data":{}, "time":{}} for c in registered_packets.keys()})

    command_processor.run(port=args.port, state=global_state)
    webapp.run(state=global_state)

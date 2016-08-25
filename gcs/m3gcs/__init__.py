import glob
import os.path
import argparse
import multiprocessing

from . import command_processor
from . import webapp

parser = argparse.ArgumentParser()
parser.add_argument("--port", help="Path to serial port on m3debug or radio",
                    default="/dev/serial/by-id/*m3debug*-if02")
args = parser.parse_args()


def run():
    if "*" in args.port:
        port = glob.unglob(args.port)
        if len(port) == 0:
            raise RuntimeError("No port matching glob found")
        args.port = os.path.realpath(port[0])

    shared_mgr = multiprocessing.Manager

    command_processor.run(port=args.port)
    webapp.app.run(debug=True)

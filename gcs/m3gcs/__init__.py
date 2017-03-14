import glob
import os.path
import argparse
import multiprocessing
import asyncio
    
from . import command_processor
from . import webapp
from .packets import registered_packets

from . import m3pyro, m3fc, m3psu, m3radio, m3dl, m3imu


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="Path to serial port on m3debug or "
                                       "radio",
                        default="/dev/serial/by-id/*m3debug*-if02")

    parser.add_argument("--logfile", help="Path to logfile to replay",
                        default=None)

    args = parser.parse_args()

    shared_mgr = multiprocessing.Manager()
    packet_queue = shared_mgr.Queue()

    if args.logfile:
        command_processor.run(logfile=args.logfile, queue=packet_queue)
    else:
        if "*" in args.port:
            port = glob.glob(args.port)
            if len(port) == 0:
                print("No port matching glob found, running with no data")
            else:
                args.port = os.path.realpath(port[0])
        command_processor.run(port=args.port, queue=packet_queue)

    webapp.setup_server(queue=packet_queue)

    loop = asyncio.get_event_loop()
    # Run the main loop
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    finally:
        webapp.shutdown()
    loop.close()

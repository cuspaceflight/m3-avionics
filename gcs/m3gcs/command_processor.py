import atexit
from datetime import datetime
import multiprocessing

from . import usbcan
from .packets import registered_packets, registered_commands
from . import logreplay

txq = multiprocessing.Queue()

def find_processor(sid):
    for name,parent in registered_packets.items():
        if sid in parent:
            return name, parent[sid]

def process(parent, name, arg):
    can_id, data = registered_commands[parent][name][0](arg)
    txq.put(usbcan.CANFrame(can_id, False, len(data), data))

errors = []

@atexit.register
def report_errors():
    if errors:
        print("Errors seen during execution:")
        print("--------------------")
        for e in errors:
            print(e)
            print("--------------------")

def run(queue, port=None, logfile=None):
    if logfile:
        logreplay_runner = multiprocessing.Process(target=logreplay.run, args=(logfile, queue))
        logreplay_runner.start()
    
    else:
        usbcan_runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, queue))
        usbcan_runner.start()

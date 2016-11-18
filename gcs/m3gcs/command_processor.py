import atexit
from datetime import datetime
import json
import multiprocessing
from queue import Queue, Empty

from . import usbcan
from .packets import registered_packets, registered_commands
from . import logreplay

txq = multiprocessing.Queue()
rxq = multiprocessing.Queue()

def update_state(state, sid, parent, name, logstring):
    tmp = state[parent]
    tmp['data'][name] = logstring
    tmp['time'][sid] = datetime.now().timestamp()
    state[parent] = tmp

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

def packet_process(rxq, state):
    global errors
    try:
        while True:
            try:
                frame = rxq.get_nowait()
                res = find_processor(frame.sid)
                if res is not None:
                    parent, processor = res
                    try:
                        update_state(state, frame.sid, parent, processor[0], processor[1](frame.data))
                    except Exception as e:
                        errors.append(e)
                        print(e)
                else:
                    print("No handler found for SID: {:b}".format(frame.sid))
                    #pass
            except Empty:
                pass
    except KeyboardInterrupt:
        return

def run(state, port=None, logfile=None):
        
    if logfile:
        logreplay_runner = multiprocessing.Process(target=logreplay.run, args=(logfile, rxq))
        logreplay_runner.start()
    
    else:
        usbcan_runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
        usbcan_runner.start()
        
    
    
    packet_processor = multiprocessing.Process(target=packet_process, args=(rxq, state))
    packet_processor.start()


import json
import multiprocessing
from queue import Queue, Empty

from . import usbcan, update_state
from .packets import registered_packets, registered_commands

txq = multiprocessing.Queue()
rxq = multiprocessing.Queue()

def find_processor(sid):
    for name,parent in registered_packets.items():
        if sid in parent:
            return name, parent[sid]

def process(parent, name, arg):
    can_id, data = registered_commands[parent][name][0](arg)
    txq.put(usbcan.CANFrame(can_id, False, len(data), data))

def packet_process(rxq):
    while True:
        try:
            frame = rxq.get_nowait()
            res = find_processor(frame.sid)
            if res is not None:
                parent, processor = res
                print(processor[1](frame.data))
                update_state(parent, processor[0], processor[1](frame.data))
            else:
                print("No handler found for SID: {:b}".format(frame.sid))
        except Empty:
            pass

def run(port, state):
    global global_state
    global_state = state
    
    usbcan_runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
    usbcan_runner.start()
    
    packet_processor = multiprocessing.Process(target=packet_process, args=(rxq,))
    packet_processor.start()


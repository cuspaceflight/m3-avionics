import json
import multiprocessing
from queue import Queue, Empty

from . import usbcan
from .packets import registered_packets, registered_commands
from . import state

def find_processor(sid):
    for name,parent in registered_packets.items():
        if sid in parent:
            return name, parent[sid]

def process(command_json):
    print(json.parse(command_json))
    #TODO something with registered_commands

def packet_process(rxq):
    while True:
        try:
            frame = rxq.get_nowait()
            parent, processor = find_procesor(frame.sid)
            if parent is not None:
                state.update(parent, processor[0], processor[1](frame.data))
            else:
                print("No handler found for SID: {:b}".format(sid))
        except Empty:
            pass

def run(port):
    txq = Queue()
    rxq = Queue()
    
    usbcan_runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
    usbcan_runner.start()
    
    packet_processor = multiprocessing.Process(target=packet_process, args=(rxq,))
    packet_processor.start()


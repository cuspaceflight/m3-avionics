import sys
import multiprocessing
from queue import Empty

import usbcan

port = sys.argv[1]
txq = multiprocessing.Queue()
rxq = multiprocessing.Queue()
runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
runner.start()


batt = []


def process_frame(frame):
    global batt, baro
    pid = frame.sid & 0x3f
    if pid == 1:
        batt = [ (float(b*2)/100.0) for b in frame.data ]
    else:
        print("Unknown SID")

while True:
    try:
        frame = rxq.get_nowait()
        process_frame(frame)
        print("Batt: 1={:02f} 2={:02f}\t"
              #"Baro: Temp={:04d} Pressure={:06d}"
              .format(batt[0], batt[1],
              #        accel[2], baro[0], baro[1]
              ),
              end="\r")
    except Empty:
        continue

import sys
import multiprocessing
from queue import Empty

import usbcan

port = sys.argv[1]
txq = multiprocessing.Queue()
rxq = multiprocessing.Queue()
runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
runner.start()


accel = 0, 0, 0
baro = 0, 0


def process_frame(frame):
    global accel, baro
    if frame.sid == 1:
        accel = frame.as_int16()
    elif frame.sid == 2:
        baro = frame.as_int32()
    else:
        print("Unknown SID")

while True:
    try:
        frame = rxq.get_nowait()
        process_frame(frame)
        print("Accel: X={:+05d} Y={:+05d} Z={:+05d}\t"
              "Baro: Temp={:04d} Pressure={:06d}"
              .format(accel[0], accel[1],
                      accel[2], baro[0], baro[1]),
              end="\r")
    except Empty:
        continue

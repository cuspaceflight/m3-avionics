import os
import glob
import struct
import argparse
import multiprocessing
from queue import Empty

from usbcan import CANFrame, run


m3radio_id = 4
msg_id = lambda x: x << 5
m3radio_msg_pkt1 = m3radio_id | msg_id(60)
m3radio_msg_pkt2 = m3radio_id | msg_id(61)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-port", help="path to serial port on m3debug",
                        default="/dev/serial/by-id/*m3debug*-if02")
    args = parser.parse_args()
    unglob = glob.glob(args.serial_port)
    if len(unglob) == 0:
        raise RuntimeError("No serial ports matching glob found")
    port = unglob[0]
    port = os.path.realpath(port)

    txq = multiprocessing.Queue()
    rxq = multiprocessing.Queue()

    runner = multiprocessing.Process(target=run, args=(port, txq, rxq))
    runner.start()

    num = 0
    last_num = None
    first_half = ""
    second_half = ""
    while True:
        frame = rxq.get()
        if frame.sid == m3radio_msg_pkt1:
            first_half = "".join(chr(x) for x in frame.data)
        elif frame.sid == m3radio_msg_pkt2:
            second_half = "".join(chr(x) for x in frame.data[:-1])
            num = int(first_half + second_half)
            if last_num is not None and num != last_num + 1:
                for _ in range(num - last_num):
                    print()
            print("{:015d}".format(num))
            last_num = num


if __name__ == "__main__":
    main()

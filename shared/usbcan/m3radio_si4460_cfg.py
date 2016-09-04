import os
import glob
import struct
import argparse
import multiprocessing
from queue import Empty

from usbcan import CANFrame, run


m3radio_id = 4
msg_id = lambda x: x << 5
m3radio_msg_si4460_cfg = m3radio_id | msg_id(52)


def read_ezradiopro(filename):
    groups = {}
    groupnames = {}
    props = {}
    getting_groups = False
    getting_numbers = False
    with open(filename) as f:
        for line in f:
            if "Property Groups" in line:
                getting_groups = True
                getting_numbers = False
                continue
            if "Property Numbers" in line:
                getting_numbers = True
                getting_groups = False
                continue
            if line.startswith("#define EZRP_PROP"):
                if line[-5:-3] == "0x":
                    val = int(line[-3:-1], 16)
                    name = line[18:-5].strip()
                    if getting_groups:
                        groups[val] = name
                        groupnames[name] = val
                    elif getting_numbers:
                        for group in groups.values():
                            if name.startswith(group):
                                groupnum = groupnames[group]
                                if groupnum not in props:
                                    props[groupnum] = {}
                                props[groupnum][val] = name
                                break
    return groups, props


def read_config(rxq, groups, props):
    while True:
        frame = rxq.get()
        if frame.sid == m3radio_msg_si4460_cfg:
            group, prop, val = frame.data[0:3]
            propname = props.get(group, {}).get(prop, "UNKNOWN")
            print("{}: 0x{:X}".format(propname, val))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-port", help="path to serial port on m3debug",
                        default="/dev/serial/by-id/*m3debug*-if02")
    parser.add_argument("--ezradiopro", help="path to ezradiopro.h",
                        required=True)
    args = parser.parse_args()
    unglob = glob.glob(args.serial_port)
    if len(unglob) == 0:
        raise RuntimeError("No serial ports matching glob found")
    port = unglob[0]
    port = os.path.realpath(port)

    cmds, props = read_ezradiopro(args.ezradiopro)

    txq = multiprocessing.Queue()
    rxq = multiprocessing.Queue()

    runner = multiprocessing.Process(target=run, args=(port, txq, rxq))
    runner.start()

    read_config(rxq, cmds, props)

if __name__ == "__main__":
    main()

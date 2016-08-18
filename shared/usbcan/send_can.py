import os
import glob
import struct
import serial
import binascii
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('id', help='message id in binary')
parser.add_argument('data', help='data to send, hex', nargs="?", default="0")
parser.add_argument('--rtr', action='store_true',
                    help='remote transmission request')
parser.add_argument('--serial-port', help="path to serial port on m3debug",
                    default="/dev/serial/by-id/usb-Black_Sphere_Technologies"
                    "_Black_Magic_Probe__m3debug_*-if02")
args = parser.parse_args()


def resolve_port(port):
    unglob = glob.glob(port)
    if len(unglob) == 0:
        raise RuntimeError("No ports matching glob found")
    port = unglob[0]
    real = os.path.realpath(port)
    return real


def check_args():
    if len(args.id) > 11:
        raise ValueError("ID is too long")
    int(args.id, 2)
    if len(args.data) > 16:
        raise ValueError("Data is too long")
    int(args.data, 16)


def main():
    check_args()
    port = resolve_port(args.serial_port)
    print("Using serial port", port)
    ser = serial.Serial(port)
    sid = int(args.id, 2)
    rtr = int(args.rtr)
    length = (len(args.data)+1)//2
    data = int(args.data, 16)
    frame = struct.pack("<HBB", sid, rtr, length)
    frame += struct.pack(">Q", data)
    buf = [0x7E]
    for byte in frame:
        if byte == 0x7E:
            buf.append(0x7D)
            buf.append(0x5E)
        elif byte == 0x7D:
            buf.append(0x7D)
            buf.append(0x5D)
        else:
            buf.append(byte)
    buf = struct.pack("{}B".format(len(buf)), *buf)
    print(binascii.hexlify(buf))
    ser.write(buf)

if __name__ == "__main__":
    main()

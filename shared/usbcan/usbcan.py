import os
import glob
import time
import struct
import serial
import binascii
import argparse
import multiprocessing
from queue import Empty


class CANFrame:
    @classmethod
    def from_buf(cls, buf):
        sid = buf[0] | (buf[1] << 8)
        rtr = buf[2]
        dlc = buf[3]
        data = buf[4:4+dlc]
        return cls(sid, rtr, dlc, data)

    def __init__(self, sid=None, rtr=None, dlc=None, data=None):
        self.sid = sid
        self.rtr = rtr
        self.dlc = dlc
        self.data = data

    def to_bytes(self):
        return (
            struct.pack("<HBB", self.sid, self.rtr, self.dlc) +
            struct.pack("{}B".format(self.dlc), *self.data))

    def __str__(self):
        return "ID={} RTR={} DLC={} DATA={}".format(
            bin(self.sid)[2:], self.rtr, self.dlc,
            " ".join("{:02X}".format(b) for b in self.data))

    def as_int16(self):
        n = self.dlc//2
        return struct.unpack("<{}h".format(n), bytes(self.data[:self.dlc]))

    def as_int32(self):
        n = self.dlc//4
        return struct.unpack("<{}i".format(n), bytes(self.data[:self.dlc]))


class CANRX:
    def __init__(self):
        self.outbuf = []
        self.in_frame = False

    def process(self, buf):
        it = iter(buf)
        for byte in it:
            if not self.in_frame:
                if byte == 0x7E:
                    self.in_frame = True
                    self.outbuf = []
            else:
                if byte == 0x7D:
                    byte = next(it)
                    self.outbuf.append(byte ^ 0x20)
                else:
                    self.outbuf.append(byte)
                if len(self.outbuf) == 12:
                    self.in_frame = False
                    yield CANFrame.from_buf(self.outbuf)


def ppp_pad(buf):
    out = [0x7E]
    for byte in buf:
        if byte == 0x7E:
            out.append(0x7D)
            out.append(0x5E)
        elif byte == 0x7D:
            out.append(0x7D)
            out.append(0x5D)
        else:
            out.append(byte)
    return struct.pack("{}B".format(len(out)), *out)


def run(port, txq, rxq):
    ser = serial.Serial(port, timeout=0.1)
    rx = CANRX()

    while True:
        try:
            frame = txq.get_nowait()
            ser.write(ppp_pad(frame.to_bytes()))
        except Empty:
            pass

        buf = ser.read(4096)
        for frame in rx.process(buf):
            rxq.put(frame)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-port", help="path to serial port on m3debug",
                        default="/dev/serial/by-id/*m3debug*-if02")
    parser.add_argument("--rx", help="RX only", action="store_true")
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

    if not args.rx:
        print("Press enter to view received packets, or type a packet to send")
        print("in the following format (DLC is inferred from DATA):")
        print("ID (binary)     RTR (0/1)    DATA (hex)")
        print("Example: 11001100110 0 CAFEC0FFEECAFE00")
        print("Ctrl-C to quit")
        print()

    def dump_packets():
        while True:
            try:
                frame = rxq.get_nowait()
                print(str(frame))
            except Empty:
                return

    def send_packet(cmd):
        try:
            sid, rtr, data = cmd.split()
            sid = int(sid, 2)
            rtr = int(rtr)
            data = list(binascii.a2b_hex(data))
            dlc = len(data)
        except ValueError as e:
            print("Error parsing packet:", e)
            return
        assert 0 <= dlc <= 8, "0 <= DLC <= 8"
        assert 0 <= rtr <= 1, "0 <= RTR <= 1"
        assert 0 <= sid <= 2047, "0 <= SID <= 2047"
        packet = CANFrame(sid, rtr, dlc, data)
        print("Sending packet: {}".format(packet))
        txq.put(packet)

    while True:
        if args.rx:
            dump_packets()
            time.sleep(0.1)
            continue

        try:
            cmd = input("> ")
        except KeyboardInterrupt:
            return

        if len(cmd) == 0:
            dump_packets()
        else:
            send_packet(cmd)
            dump_packets()

if __name__ == "__main__":
    main()

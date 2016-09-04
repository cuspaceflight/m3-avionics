import os
import glob
import time
import struct
import serial
import binascii
import argparse
import multiprocessing
import os
from queue import Empty


class CANFrame:
    @classmethod
    def from_buf(cls, buf):
        sid = buf[0] | (buf[1] << 8)
        rtr = buf[2]
        dlc = buf[3]
        data = buf[4:]
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


def logfilename():
    idx = 0
    while os.path.exists("logfile_{}.txt".format(idx)):
        idx += 1
    return "logfile_{}.txt".format(idx)


def log(f, frame):
    f.write(str(frame) + "\n")


def run(port, txq, rxq):
    ser = serial.Serial(port, timeout=0.1)
    rx = CANRX()

    logname = logfilename()

    print("Logging to {}".format(logname))

    with open(logname, "w") as f:
        try:
            while True:
                try:
                    frame = txq.get_nowait()
                    ser.write(ppp_pad(frame.to_bytes()))
                except Empty:
                    pass

                buf = ser.read(64)
                for frame in rx.process(buf):
                    log(f, frame)
                    rxq.put(frame)
        except KeyboardInterrupt:
            ser.close()


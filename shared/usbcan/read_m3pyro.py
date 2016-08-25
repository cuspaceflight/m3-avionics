import os
import glob
import time
import struct
import serial
import argparse
import multiprocessing
from queue import Empty


m3pyro_id = 3
msg_id = lambda x: x << 5
m3pyro_msg_fire_command = m3pyro_id | msg_id(1)
m3pyro_msg_arm_command = m3pyro_id | msg_id(2)
m3pyro_msg_fire_status = m3pyro_id | msg_id(16)
m3pyro_msg_arm_status = m3pyro_id | msg_id(17)
m3pyro_msg_continuity = m3pyro_id | msg_id(48)
m3pyro_msg_supply_status = m3pyro_id | msg_id(49)


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
        print("Press enter to view received packets, or type a command")
        print("to send:")
        print("arm|disarm")
        print("fire <channel> <mode: 0=off 1=on 2=pulsed>")
        print("Ctrl-C to quit")
        print()

    def dump_packets():
        while True:
            try:
                frame = rxq.get_nowait()
                if frame.sid == m3pyro_msg_fire_status:
                    status_map = {0: "Off", 1: "Firing", 2: "Pulsed Firing"}
                    statuses = [status_map[d] for d in frame.data[:4]]
                    print("Fire Status: {} {} {} {}".format(*statuses))
                elif frame.sid == m3pyro_msg_arm_status:
                    status_map = {0: "Disarmed", 1: "Armed"}
                    print("Arm Status: {}".format(status_map[frame.data[0]]))
                elif frame.sid == m3pyro_msg_continuity:
                    r = ["{:.1f}R".format(d/10.0) if d != 255 else "HI" for d in frame.data[:4]]
                    print("Continuity: {} {} {} {}".format(*r))
                elif frame.sid == m3pyro_msg_supply_status:
                    print("Supply: {:.1f}".format(frame.data[0]/10.0))
            except Empty:
                return

    def send_packet(cmd):
        try:
            command = cmd.split()[0].strip()
            if command in ("arm", "disarm"):
                sid = m3pyro_msg_arm_command
                data = [1] if command == "arm" else [0]
                packet = CANFrame(sid, False, len(data), data)
                print("Sending packet: {}".format(packet))
                txq.put(packet)
            elif command == "fire":
                sid = m3pyro_msg_fire_command
                _, channel, mode = cmd.split()
                channel = int(channel.strip()) - 1
                mode = int(mode.strip())
                data = [0] * channel + [mode] + [0] * (4-channel)
                packet = CANFrame(sid, False, len(data), data)
                print("Sending packet: {}".format(packet))
                txq.put(packet)
        except ValueError as e:
            print("Error parsing packet:", e)
            return

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

import os
import glob
import struct
import argparse
import multiprocessing
from queue import Empty

from usbcan import CANFrame, run


m3fc_id = 1
msg_id = lambda x: x << 5
m3fc_msg_cfg_profile = m3fc_id | msg_id(54)
m3fc_msg_cfg_pyros = m3fc_id | msg_id(55)
m3fc_msg_set_cfg_profile = m3fc_id | msg_id(1)
m3fc_msg_set_cfg_pyros = m3fc_id | msg_id(2)
m3fc_msg_save_cfg = m3fc_id | msg_id(4)


class M3FCConfigPyros:
    use_map = {0: "Unused", 1: "Drogue", 2: "Main", 3: "Dart Separation",
               4: "Booster Separation"}
    type_map = {0: "None", 1: "EMatch", 2: "Talon", 3: "Metron"}

    def __init__(self, p1use, p2use, p3use, p4use,
                 p1type, p2type, p3type, p4type):
        self.p1use = p1use
        self.p2use = p2use
        self.p3use = p3use
        self.p4use = p4use
        self.p1type = p1type
        self.p2type = p2type
        self.p3type = p3type
        self.p4type = p4type

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_pyros
        return cls(*packet.data)

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_pyros, rtr=False, dlc=8,
                        data=[self.p1use, self.p2use,
                              self.p3use, self.p4use,
                              self.p1type, self.p2type,
                              self.p3type, self.p4type])

    def __str__(self):
        out = []
        out.append("M3FC Config Pyros:")
        out.append("Pyro 1: {}/{}".format(self.use_map[self.p1use],
                                          self.type_map[self.p1type]))
        out.append("Pyro 2: {}/{}".format(self.use_map[self.p2use],
                                          self.type_map[self.p2type]))
        out.append("Pyro 3: {}/{}".format(self.use_map[self.p3use],
                                          self.type_map[self.p3type]))
        out.append("Pyro 4: {}/{}".format(self.use_map[self.p4use],
                                          self.type_map[self.p4type]))
        out.append("")
        return "\n".join(out)


class M3FCConfigProfile:
    m3fc_position_map = {0: "UNSET", 1: "Dart", 2: "Core"}
    accel_axis_map = {0: "UNSET", 1: "X", 2: "-X", 3: "Y", 4: "-Y",
                      5: "Z", 6: "-Z"}

    def __init__(self, m3fc_position, accel_axis, ignition_accel,
                 burnout_timeout, apogee_timeout, main_altitude,
                 main_timeout, land_timeout):
        self.m3fc_position = m3fc_position
        self.accel_axis = accel_axis
        self.ignition_accel = ignition_accel
        self.burnout_timeout = burnout_timeout
        self.apogee_timeout = apogee_timeout
        self.main_altitude = main_altitude
        self.main_timeout = main_timeout
        self.land_timeout = land_timeout

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_profile
        return cls(*packet.data)

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_profile, rtr=False, dlc=8,
                        data=[self.m3fc_position, self.accel_axis,
                              self.ignition_accel, self.burnout_timeout,
                              self.apogee_timeout, self.main_altitude,
                              self.main_timeout, self.land_timeout])

    def __str__(self):
        out = []
        out.append("M3FC Config Profile")
        out.append("M3FC Position: {}".format(
                   self.m3fc_position_map[self.m3fc_position]))
        out.append("Accelerometer Up Axis: {}".format(
                   self.accel_axis_map[self.accel_axis]))
        out.append("Ignition Detection Threshold: {}m/s/s".format(
                   self.ignition_accel))
        out.append("Burnout Detection Timeout: {:.1f}s after launch".format(
                   self.burnout_timeout/10.0))
        out.append("Apogee Detection Timeout: {}s after launch".format(
                   self.apogee_timeout))
        out.append("Main Chute Release Altitude: {}m above launch".format(
                   self.main_altitude*10))
        out.append("Main Chute Release Timeout: {}s after apogee".format(
                   self.main_timeout))
        out.append("Landing Detection Timeout: {}s after launch".format(
                   self.land_timeout*10))
        out.append("")
        return "\n".join(out)


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


def read_config(rxq):
    cfg_profile = None
    cfg_pyros = None

    print("Waiting to receive current profile...")

    while cfg_profile is None or cfg_pyros is None:
        frame = rxq.get()
        if frame.sid == m3fc_msg_cfg_profile:
            cfg_profile = M3FCConfigProfile.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_pyros:
            cfg_pyros = M3FCConfigPyros.from_can(frame)

    print(cfg_profile)
    print(cfg_pyros)


def get_new_config():
    m3fc_position = input("M3FC Position (dart/core): ")
    accel_axis = input("Accel Up Axis (X/-X/Y/-Y/Z/-Z): ")
    ignition_accel = int(input("Ignition Accel (m/s/s): "))
    burnout_timeout = int(input("Burnout Timeout (0.1s): "))
    apogee_timeout = int(input("Apogee Timeout (s): "))
    main_altitude = int(input("Main Chute Altitude "
                              "(10m above launch altitude): "))
    main_timeout = int(input("Main Chute Timeout (s after apogee): "))
    land_timeout = int(input("Landing Timeout (10s after launch): "))
    if m3fc_position.lower() == "dart":
        m3fc_position = 1
    elif m3fc_position.lower() == "core":
        m3fc_position = 2
    else:
        raise ValueError("Invalid M3FC Position")
    if accel_axis.lower() == "x":
        accel_axis = 1
    elif accel_axis.lower() == "-x":
        accel_axis = 2
    elif accel_axis.lower() == "y":
        accel_axis = 3
    elif accel_axis.lower() == "-y":
        accel_axis = 4
    elif accel_axis.lower() == "z":
        accel_axis = 5
    elif accel_axis.loweR() == "-z":
        accel_axis = 6
    else:
        raise ValueError("Invalid accel_axis")
    cfg_profile = M3FCConfigProfile(m3fc_position, accel_axis, ignition_accel,
                                    burnout_timeout, apogee_timeout,
                                    main_altitude, main_timeout, land_timeout)

    pyro_use_map = {"none": 0, "drogue": 1, "main": 2,
                    "dartsep": 3, "boostersep": 4}
    pyro_type_map = {"ematch": 1, "talon": 2, "metron": 3}
    pyro_uses = {}
    pyro_types = {}
    for n in range(1, 4+1):
        pyro_use = input("Pyro {} use (none/drogue/main/dartsep/boostersep): "
                         .format(n))
        pyro_use = pyro_use_map[pyro_use.lower()]
        if pyro_use == 0:
            pyro_type = 0
        else:
            pyro_type = input("Pyro {} type (ematch/talon/metron): ".format(n))
            pyro_type = pyro_type_map[pyro_type]
        pyro_uses[n] = pyro_use
        pyro_types[n] = pyro_type
    cfg_pyros = M3FCConfigPyros(
        pyro_uses[1], pyro_uses[2], pyro_uses[3], pyro_uses[4],
        pyro_types[1], pyro_types[2], pyro_types[3], pyro_types[4])

    return cfg_profile, cfg_pyros


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial-port", help="path to serial port on m3debug",
                        default="/dev/serial/by-id/*m3debug*-if02")
    parser.add_argument("--init", help="first flash, don't try to read first",
                        action="store_true")
    parser.add_argument("--flash", help="immediately flash config",
                        action="store_true")
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

    if not args.init:
        read_config(rxq)

    try:
        cmd = input("Update config? (y/N): ")
    except KeyboardInterrupt:
        print("Cancelling")
        return

    if cmd.lower() != "y":
        print("Not updating config")
        return

    cfg_profile, cfg_pyros = get_new_config()

    print("Desired new config:")
    print(cfg_profile)
    print(cfg_pyros)

    accept = input("Set new config? (y/N): ")
    if accept.lower() != "y":
        print("Not writing new config")
        return

    print("Sending new config")
    txq.put(cfg_profile.to_can())
    txq.put(cfg_pyros.to_can())

    print("Waiting to read back new config")

    # clear current q
    while not rxq.empty():
        try:
            rxq.get_nowait()
        except Empty:
            break

    read_config(rxq)

    save = input("Save new config to flash? (y/N): ")

    if save.lower() != "y":
        print("Not saving to flash")
        return

    print("Saving new config to flash")
    txq.put(CANFrame(sid=m3fc_msg_save_cfg, rtr=False, dlc=0, data=[]))


if __name__ == "__main__":
    main()

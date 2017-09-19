import os
import glob
import yaml
import time
import struct
import argparse
import multiprocessing
import crcmod.predefined
from queue import Empty

from usbcan import CANFrame, run


m3fc_id = 1
msg_id = lambda x: x << 5

m3fc_msg_cfg_profile = m3fc_id | msg_id(54)
m3fc_msg_cfg_pyros = m3fc_id | msg_id(55)
m3fc_msg_cfg_accel_cal_x = m3fc_id | msg_id(56)
m3fc_msg_cfg_accel_cal_y = m3fc_id | msg_id(57)
m3fc_msg_cfg_accel_cal_z = m3fc_id | msg_id(58)
m3fc_msg_cfg_radio_freq = m3fc_id | msg_id(59)
m3fc_msg_cfg_crc = m3fc_id | msg_id(60)

m3fc_msg_set_cfg_profile = m3fc_id | msg_id(1)
m3fc_msg_set_cfg_pyros = m3fc_id | msg_id(2)
m3fc_msg_set_cfg_accel_x = m3fc_id | msg_id(10)
m3fc_msg_set_cfg_accel_y = m3fc_id | msg_id(11)
m3fc_msg_set_cfg_accel_z = m3fc_id | msg_id(12)
m3fc_msg_set_cfg_radio_freq = m3fc_id | msg_id(13)
m3fc_msg_set_cfg_crc = m3fc_id | msg_id(14)

m3fc_msg_save_cfg = m3fc_id | msg_id(4)


class M3FCConfigPyros:
    use_map = {0x00: "Unused",
               0x10: "Drogue", 0x20: "Main", 0x30: "Dart Separation"}
    use_invmap = {"unused": 0, "drogue": 0x10, "main": 0x20, "dartsep": 0x30}
    use_mask = 0xf0
    type_map = {0: "None", 1: "EMatch", 2: "Talon", 3: "Metron"}
    type_invmap = {"none": 0, "ematch": 1, "talon": 2, "metron": 3}
    type_mask = 0x03
    current_map = {0: "None", 0x04: "1A", 0x08: "3A"}
    current_invmap = {"none": 0, "1A": 0x04, "3A": 0x08}
    current_mask = 0x0c

    def __init__(self, p1, p2, p3, p4, p5, p6, p7, p8):
        self.pyro1 = p1
        self.pyro2 = p2
        self.pyro3 = p3
        self.pyro4 = p4
        self.pyro5 = p5
        self.pyro6 = p6
        self.pyro7 = p7
        self.pyro8 = p8

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_pyros
        return cls(*packet.data)

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_pyros, rtr=False, dlc=8,
                        data=[self.pyro1, self.pyro2, self.pyro3, self.pyro4,
                              self.pyro5, self.pyro6, self.pyro7, self.pyro8])

    @classmethod
    def from_dict(cls, d):
        def from_pyro_dict(d):
            if d == "none":
                return 0
            assert set(d.keys()) == {"type", "usage", "current"}
            return (
                cls.use_invmap.get(d['usage'], 0) |
                cls.type_invmap.get(d['type'], 0) |
                cls.current_invmap.get(d['current'], 0))

        assert set(d.keys()) == {"pyro1", "pyro2", "pyro3", "pyro4",
                                 "pyro5", "pyro6", "pyro7", "pyro8"}
        return cls(from_pyro_dict(d['pyro1']),
                   from_pyro_dict(d['pyro2']),
                   from_pyro_dict(d['pyro3']),
                   from_pyro_dict(d['pyro4']),
                   from_pyro_dict(d['pyro5']),
                   from_pyro_dict(d['pyro6']),
                   from_pyro_dict(d['pyro7']),
                   from_pyro_dict(d['pyro8']))

    def __str__(self):
        def pyro2str(pyro):
            usage = self.use_map.get(pyro & 0xf0, "?")
            current = self.current_map.get(pyro & 0x0c, "?")
            type_ = self.type_map.get(pyro & 0x03, "?")
            return "{}/{}/{}".format(usage, type_, current)
        out = []
        out.append("M3FC Config Pyros:")
        out.append("Pyro 1: " + pyro2str(self.pyro1))
        out.append("Pyro 2: " + pyro2str(self.pyro2))
        out.append("Pyro 3: " + pyro2str(self.pyro3))
        out.append("Pyro 4: " + pyro2str(self.pyro4))
        out.append("Pyro 5: " + pyro2str(self.pyro5))
        out.append("Pyro 6: " + pyro2str(self.pyro6))
        out.append("Pyro 7: " + pyro2str(self.pyro7))
        out.append("Pyro 8: " + pyro2str(self.pyro8))
        out.append("")
        return "\n".join(out)


class M3FCConfigProfile:
    m3fc_position_map = {1: "Dart", 2: "Core"}
    m3fc_position_invmap = {"dart": 1, "core": 2}
    accel_axis_map = {1: "X", 2: "-X", 3: "Y", 4: "-Y", 5: "Z", 6: "-Z"}
    accel_axis_invmap = {"x": 1, "-x": 2, "y": 3, "-y": 4, "z": 5, "-z": 6}

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

    @classmethod
    def from_dict(cls, d):
        assert set(d.keys()) == {"m3fc_position", "accel_axis",
                                 "ignition_accel", "burnout_timeout",
                                 "apogee_timeout", "main_altitude",
                                 "main_timeout", "land_timeout"}
        return cls(
            cls.m3fc_position_invmap.get(d['m3fc_position'].lower(), 0),
            cls.accel_axis_invmap.get(d['accel_axis'].lower(), 0),
            int(d['ignition_accel']),
            int(d['burnout_timeout'] * 10),
            int(d['apogee_timeout']),
            int(d['main_altitude'] // 10),
            int(d['main_timeout']),
            int(d['land_timeout'] // 10))

    def __str__(self):
        out = []
        out.append("M3FC Config Profile:")
        out.append("M3FC Position: {}".format(
                   self.m3fc_position_map.get(self.m3fc_position, "Unknown")))
        out.append("Accelerometer Up Axis: {}".format(
                   self.accel_axis_map.get(self.accel_axis, "Unknown")))
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


class M3FCConfigAccelCalX:
    def __init__(self, scale, offset):
        self.scale = scale
        self.offset = offset

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_accel_cal_x
        return cls(*struct.unpack("<ff", packet.data_bytes()))

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_accel_x, rtr=False, dlc=8,
                        data=struct.pack("<ff", self.scale, self.offset))

    @classmethod
    def from_dict(cls, d):
        assert set(d.keys()) == {"scale", "offset"}
        return cls(d['scale'], d['offset'])

    def __str__(self):
        out = "Accel Cal X: Scale={:.6f} Offset={:.3f}"
        return out.format(self.scale, self.offset)


class M3FCConfigAccelCalY:
    def __init__(self, scale, offset):
        self.scale = scale
        self.offset = offset

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_accel_cal_y
        return cls(*struct.unpack("<ff", packet.data_bytes()))

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_accel_y, rtr=False, dlc=8,
                        data=struct.pack("<ff", self.scale, self.offset))

    @classmethod
    def from_dict(cls, d):
        assert set(d.keys()) == {"scale", "offset"}
        return cls(d['scale'], d['offset'])

    def __str__(self):
        out = "Accel Cal Y: Scale={:.6f} Offset={:.3f}"
        return out.format(self.scale, self.offset)


class M3FCConfigAccelCalZ:
    def __init__(self, scale, offset):
        self.scale = scale
        self.offset = offset

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_accel_cal_z
        return cls(*struct.unpack("<ff", packet.data_bytes()))

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_accel_z, rtr=False, dlc=8,
                        data=struct.pack("<ff", self.scale, self.offset))

    @classmethod
    def from_dict(cls, d):
        assert set(d.keys()) == {"scale", "offset"}
        return cls(d['scale'], d['offset'])

    def __str__(self):
        out = "Accel Cal Z: Scale={:.6f} Offset={:.3f}"
        return out.format(self.scale, self.offset)


class M3FCConfigRadioFreq:
    def __init__(self, freq):
        self.freq = freq

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_radio_freq
        return cls(struct.unpack("<I", packet.data_bytes())[0])

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_radio_freq, rtr=False, dlc=4,
                        data=struct.pack("<I", self.freq))

    @classmethod
    def from_dict(cls, d):
        return cls(d)

    def __str__(self):
        return "Radio Freq: {}".format(self.freq)


class M3FCConfigCRC:
    def __init__(self, crc):
        self.crc = crc

    @classmethod
    def from_can(cls, packet):
        assert packet.sid == m3fc_msg_cfg_crc
        return cls(struct.unpack("<I", packet.data_bytes())[0])

    def to_can(self):
        return CANFrame(sid=m3fc_msg_set_cfg_crc, rtr=False, dlc=4,
                        data=struct.pack("<I", self.crc))

    def __str__(self):
        return "CRC: {}".format(hex(self.crc))


class M3FCConfig:
    def __init__(self, profile, pyros, accel_cal_x, accel_cal_y, accel_cal_z,
                 radio_freq, crc):
        self.profile = profile
        self.pyros = pyros
        self.accel_cal_x = accel_cal_x
        self.accel_cal_y = accel_cal_y
        self.accel_cal_z = accel_cal_z
        self.radio_freq = radio_freq
        self.crc = crc

    @classmethod
    def from_dict(cls, d):
        assert set(d.keys()) == {"profile", "pyros", "accel_cal_x",
                                 "accel_cal_y", "accel_cal_z", "radio_freq"}
        cfg = cls(
            M3FCConfigProfile.from_dict(d['profile']),
            M3FCConfigPyros.from_dict(d['pyros']),
            M3FCConfigAccelCalX.from_dict(d['accel_cal_x']),
            M3FCConfigAccelCalY.from_dict(d['accel_cal_y']),
            M3FCConfigAccelCalZ.from_dict(d['accel_cal_z']),
            M3FCConfigRadioFreq.from_dict(d['radio_freq']),
            None)
        cfg.crc = cfg.compute_crc()
        return cfg

    def compute_crc(self):
        profile = self.profile
        pyros = self.pyros
        accel_cal_x = self.accel_cal_x
        accel_cal_y = self.accel_cal_y
        accel_cal_z = self.accel_cal_z
        radio_freq = self.radio_freq
        raw = struct.pack(
            "<16B6fI",
            profile.m3fc_position, profile.accel_axis, profile.ignition_accel,
            profile.burnout_timeout, profile.apogee_timeout,
            profile.main_altitude, profile.main_timeout, profile.land_timeout,
            pyros.pyro1, pyros.pyro2, pyros.pyro3, pyros.pyro4,
            pyros.pyro5, pyros.pyro6, pyros.pyro7, pyros.pyro8,
            accel_cal_x.scale, accel_cal_x.offset, accel_cal_y.scale,
            accel_cal_y.offset, accel_cal_z.scale, accel_cal_z.offset,
            radio_freq.freq)
        # convert to 32 bit words and then reverse the byte ordering
        u32 = struct.unpack(">11I", raw)
        raw = struct.pack("<11I", *u32)
        crc32_func = crcmod.predefined.mkCrcFun('crc-32-mpeg')
        return M3FCConfigCRC(crc32_func(raw))

    def parts(self):
        return [self.profile, self.pyros, self.accel_cal_x, self.accel_cal_y,
                self.accel_cal_z, self.radio_freq, self.crc]

    def loaded(self):
        return all(x is not None for x in self.parts())

    def to_can(self):
        return [x.to_can() for x in self.parts()]

    def update_from_can(self, frame):
        if frame.sid == m3fc_msg_cfg_profile:
            self.profile = M3FCConfigProfile.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_pyros:
            self.pyros = M3FCConfigPyros.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_accel_cal_x:
            self.accel_cal_x = M3FCConfigAccelCalX.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_accel_cal_y:
            self.accel_cal_y = M3FCConfigAccelCalY.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_accel_cal_z:
            self.accel_cal_z = M3FCConfigAccelCalZ.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_radio_freq:
            self.radio_freq = M3FCConfigRadioFreq.from_can(frame)
        elif frame.sid == m3fc_msg_cfg_crc:
            self.crc = M3FCConfigCRC.from_can(frame)

    def __str__(self):
        return "\n".join(str(x) for x in self.parts())


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


def read_config_from_can(rxq):
    cfg = M3FCConfig(None, None, None, None, None, None, None)

    while not cfg.loaded():
        frame = rxq.get()
        cfg.update_from_can(frame)

    print(cfg)


def config_from_file(path):
    yaml_cfg = yaml.load(open(path))
    return M3FCConfig.from_dict(yaml_cfg)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="path to serial port on m3debug",
                        default="/dev/serial/by-id/*m3debug*-if02")
    parser.add_argument("--file", help="path to config yaml file")
    parser.add_argument("--flash", help="save new config to flash",
                        action="store_true")
    parser.add_argument("--crc", help="just compute crc on a file",
                        action="store_true")
    parser.add_argument("--slow", help="work slowly over rf links",
                        action="store_true")
    args = parser.parse_args()

    if args.crc:
        if not args.file:
            print("must specify --file with --crc")
            return
        cfg = config_from_file(args.file)
        print(cfg)
        return

    unglob = glob.glob(args.port)
    if len(unglob) == 0:
        raise RuntimeError("No serial ports matching glob found")
    port = unglob[0]
    port = os.path.realpath(port)

    txq = multiprocessing.Queue()
    rxq = multiprocessing.Queue()

    runner = multiprocessing.Process(target=run, args=(port, txq, rxq))
    runner.start()

    if args.file:
        cfg = config_from_file(args.file)
        print("Loaded config:")
        print(cfg)
        accept = input("Set new config? (y/N): ")
        if accept.lower() == "y":
            print("Setting new config")
            if args.slow:
                for i in range(5):
                    print("Attempt {}".format(i))
                    for idx, frame in enumerate(cfg.to_can()):
                        print("{}, ", end='', flush=True)
                        txq.put(frame)
                        time.sleep(1)
            else:
                for frame in cfg.to_can():
                    txq.put(frame)
        else:
            print("Not setting new config")

    if args.flash:
        print("Saving current config to flash")
        txq.put(CANFrame(sid=m3fc_msg_save_cfg, rtr=False, dlc=0, data=[]))

    # clear current q
    while not rxq.empty():
        try:
            rxq.get_nowait()
        except Empty:
            break

    print("Reading current config...\n")
    read_config_from_can(rxq)

    runner.terminate()

if __name__ == "__main__":
    main()

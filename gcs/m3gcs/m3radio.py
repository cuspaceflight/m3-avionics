from .packets import register_packet, register_command
import struct
from math import sqrt


def msg_id(x):
    return x << 5


CAN_ID_M3RADIO = 4
CAN_MSG_ID_M3RADIO_GPS_LATLNG = CAN_ID_M3RADIO | msg_id(48)
CAN_MSG_ID_M3RADIO_GPS_ALT = CAN_ID_M3RADIO | msg_id(49)
CAN_MSG_ID_M3RADIO_GPS_TIME = CAN_ID_M3RADIO | msg_id(50)
CAN_MSG_ID_M3RADIO_GPS_STATUS = CAN_ID_M3RADIO | msg_id(51)

@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_LATLNG, "GPS Lat/Long")
def latlong(data):
    lat, lon = struct.unpack("ii", bytes(data))
    return "{: 10.6f} {: 10.6f}".format(lat/1e7, lon/1e7)

@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_ALT, "GPS Alt")
def alt(data):
    height, h_msl = struct.unpack("ii", bytes(data))
    return "{: 5d}m above ellipsoid, {: 5d}m ASL".format(int(height/1000),
        int(h_msl/1000))

@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_TIME, "GPS Time")
def gpstime(data):
    year, month, day, hour, minute, second, valid = struct.unpack(
        "HBBBBBB", bytes(data))
    string = "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d} ".format(
        year, month, day, hour, minute, second)
    string += "(valid)" if valid else "(invalid)"
    return string

@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_STATUS, "GPS Status")
def gpsstatus(data):
    fix_type, flags, num_sv = struct.unpack("BBB", bytes(data[:3]))
    fix_types = {0:"No fix", 2:"2d fix", 3:"3d fix"}
    return "{}, {} satellites, flags: {:08b}".format(fix_types[fix_type],
        num_sv, flags)


from .packets import register_packet, register_command
import struct


def msg_id(x):
    return x << 5


CAN_ID_M3RADIO = 4
CAN_ID_GROUND = 7
CAN_MSG_ID_M3RADIO_STATUS = CAN_ID_M3RADIO | msg_id(0)
CAN_MSG_ID_M3RADIO_GPS_LATLNG = CAN_ID_M3RADIO | msg_id(48)
CAN_MSG_ID_M3RADIO_GPS_ALT = CAN_ID_M3RADIO | msg_id(49)
CAN_MSG_ID_M3RADIO_GPS_TIME = CAN_ID_M3RADIO | msg_id(50)
CAN_MSG_ID_M3RADIO_GPS_STATUS = CAN_ID_M3RADIO | msg_id(51)
CAN_MSG_ID_M3RADIO_PACKET_COUNT = CAN_ID_M3RADIO | msg_id(53)
CAN_MSG_ID_M3RADIO_PACKET_STATS = CAN_ID_M3RADIO | msg_id(54)
CAN_MSG_ID_M3RADIO_PACKET_PING = CAN_ID_M3RADIO | msg_id(55)
CAN_MSG_ID_GROUND_PACKET_COUNT = CAN_ID_GROUND | msg_id(53)
CAN_MSG_ID_GROUND_PACKET_STATS = CAN_ID_GROUND | msg_id(54)

components = {
    1: "uBlox",
    2: "Si4460",
    3: "GPS Antenna",
    4: "Packet Processor",
}

component_errors = {
    0: "No Error",
    1: "uBlox Checksum", 2: "uBlox Timeout", 3: "uBlox UART",
    4: "uBlox Config", 5: "uBlox Decode", 6: "uBlox Flight Mode",
    7: "uBlox NAK",
    8: "Si4460 Config"
}

compstatus = {k: {"state": 0, "reason": "Unknown"} for k in components}


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_STATUS, "Status")
def status(data):
    global compstatus

    # The string must start with 'OK:' 'INIT:' or 'ERROR:' as the web
    # interface watches for these and applies special treatment (colours)
    statuses = {0: "OK", 1: "INIT", 2: "ERROR"}
    overall, comp, comp_state, comp_error = struct.unpack(
        "BBBB", bytes(data[:4]))

    # Display the state (and error) of the component that sent the message
    string = "{}: ({} {}".format(statuses.get(overall, "Unknown"),
                                 components.get(comp, "Unknown"),
                                 statuses.get(comp_state, "Unknown"))
    if comp_error != 0:
        string += " {})".format(component_errors.get(comp_error, "Unknown"))
    else:
        string += ")"

    # Update our perception of the overall state
    if comp in compstatus:
        compstatus[comp]['state'] = comp_state
        compstatus[comp]['reason'] = component_errors[comp_error]

    # List all components we believe to be in error
    errors = ""
    for k in components:
        if compstatus[k]['state'] == 2:
            errors += "\n{}: {}".format(components[k], compstatus[k]['reason'])
    if errors:
        string += "\nErrors:" + errors
    return string


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_LATLNG, "GPS Lat/Long")
def latlong(data):
    lat, lon = struct.unpack("ii", bytes(data))
    return "{: 10.6f} {: 10.6f}".format(lat/1e7, lon/1e7)


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_ALT, "GPS Alt")
def alt(data):
    height, h_msl = struct.unpack("ii", bytes(data))
    return "{: 5d}m above ellipsoid, {: 5d}m ASL".format(
        int(height/1000), int(h_msl/1000))


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_TIME, "GPS Time")
def gpstime(data):
    year, month, day, hour, minute, second, valid = struct.unpack(
        "HBBBBBB", bytes(data))
    string = "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d} ".format(
        year, month, day, hour, minute, second)
    string += "(valid)" if (valid & 0b111 == 0b111) else "(invalid)"
    return string


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_GPS_STATUS, "GPS Status")
def gpsstatus(data):
    fix_type, flags, num_sv = struct.unpack("BBB", bytes(data[:3]))
    fix_types = {0: "No fix", 2: "2d fix", 3: "3d fix"}
    return "{}, {} satellites, flags: {:08b}".format(
        fix_types[fix_type], num_sv, flags)


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_PACKET_COUNT, "Packet Count")
@register_packet("ground", CAN_MSG_ID_GROUND_PACKET_COUNT, "Packet Count")
def packet_count(data):
    txcount, rxcount = struct.unpack("II", bytes(data))
    return "TX {}, RX {}".format(txcount, rxcount)


@register_packet("m3radio", CAN_MSG_ID_M3RADIO_PACKET_STATS, "Packet Stats")
@register_packet("ground", CAN_MSG_ID_GROUND_PACKET_STATS, "Packet Stats")
def packet_stats(data):
    rssi, freqoff, biterrs, iters = struct.unpack("hhHH", bytes(data))
    return "RSSI {}dBm, Freq Offset {}Hz, Bit Errs {}, LDPC Iters {}".format(
        rssi, freqoff, biterrs, iters)


@register_command("ground", "Ping", ["Ping"])
def ping_cmd(data):
    return CAN_MSG_ID_M3RADIO_PACKET_PING, []

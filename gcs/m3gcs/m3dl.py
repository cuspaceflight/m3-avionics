from .packets import register_packet, register_command
import struct


def msg_id(x):
    return x << 5

CAN_ID_M3DL = 6
CAN_MSG_ID_M3DL_STATUS = CAN_ID_M3DL | msg_id(0)

@register_packet("m3dl", CAN_MSG_ID_M3DL_STATUS, "Status")
def status(data):
    statuses = {0: "OK", 1: "INIT", 2: "ERROR"}
    overall, comp, comp_state, comp_error = struct.unpack(
        "BBBB", bytes(data[:4]))
    return "{}:".format(statuses.get(overall, "Unknown"))

from .packets import register_packet

def msg_id(x):
    return x << 5

CAN_ID_M3FC = 1
CAN_ID_M3PSU = 2
CAN_ID_M3PYRO = 3
CAN_ID_M3RADIO = 4
CAN_ID_M3IMU = 5
CAN_ID_M3DL = 6

CAN_MSG_ID_M3FC_VERSION = CAN_ID_M3FC | msg_id(63)
CAN_MSG_ID_M3PSU_VERSION = CAN_ID_M3PSU | msg_id(63)
CAN_MSG_ID_M3PYRO_VERSION = CAN_ID_M3PYRO | msg_id(63)
CAN_MSG_ID_M3RADIO_VERSION = CAN_ID_M3RADIO | msg_id(63)
CAN_MSG_ID_M3IMU_VERSION = CAN_ID_M3IMU | msg_id(63)
CAN_MSG_ID_M3DL_VERSION = CAN_ID_M3DL | msg_id(63)


@register_packet("Version", CAN_MSG_ID_M3FC_VERSION, "M3FC")
@register_packet("Version", CAN_MSG_ID_M3PSU_VERSION, "M3PSU")
@register_packet("Version", CAN_MSG_ID_M3PYRO_VERSION, "M3PYRO")
@register_packet("Version", CAN_MSG_ID_M3RADIO_VERSION, "M3RADIO")
@register_packet("Version", CAN_MSG_ID_M3IMU_VERSION, "M3IMU")
@register_packet("Version", CAN_MSG_ID_M3DL_VERSION, "M3DL")
def version(data):
    return "".join(chr(x) for x in data)

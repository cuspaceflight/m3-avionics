from .packets import register_packet, register_command
import struct


def msg_id(x):
    return x << 5

CAN_ID_M3DL = 6
CAN_MSG_ID_M3DL_STATUS = CAN_ID_M3DL | msg_id(0)
CAN_MSG_ID_M3DL_FREE_SPACE = CAN_ID_M3DL | msg_id(32)
CAN_MSG_ID_M3DL_RATE = CAN_ID_M3DL | msg_id(33)
CAN_MSG_ID_M3DL_TEMP_1_2 = CAN_ID_M3DL | msg_id(48)
CAN_MSG_ID_M3DL_TEMP_3_4 = CAN_ID_M3DL | msg_id(49)
CAN_MSG_ID_M3DL_TEMP_5_6 = CAN_ID_M3DL | msg_id(50)
CAN_MSG_ID_M3DL_TEMP_7_8 = CAN_ID_M3DL | msg_id(51)
CAN_MSG_ID_M3DL_TEMP_9 = CAN_ID_M3DL | msg_id(52)
CAN_MSG_ID_M3DL_PRESSURE = CAN_ID_M3DL | msg_id(53)

@register_packet("m3dl", CAN_MSG_ID_M3DL_STATUS, "Status")
def status(data):
    statuses = {0: "OK", 1: "INIT", 2: "ERROR"}
    overall, comp, comp_state, comp_error = struct.unpack(
        "BBBB", bytes(data[:4]))
    return "{}:".format(statuses.get(overall, "Unknown"))
    
@register_packet("m3dl", CAN_MSG_ID_M3DL_FREE_SPACE, "Storage")
def free_space(data):
    # 4 byte integer - number of empty 16kB clusters
    free_clusters, = struct.unpack("I", bytes(data[:4]))
    free_space = ((free_clusters * 16) / (1024 * 1024))
    return "Empty Space: {: 2.2f}GB".format(free_space)
    
@register_packet("m3dl", CAN_MSG_ID_M3DL_RATE, "Logging")
def packet_rate(data):
    # 4 byte integer - packet rate
    pkt_rate, = struct.unpack("I", bytes(data[:4]))
    return "Packet Rate: {: 3d}/s".format(pkt_rate)
    
@register_packet("m3dl", CAN_MSG_ID_M3DL_PRESSURE, "Pressure")
def pressure(data):
    pressure1, pressure2, pressure3, pressure4 = struct.unpack("HHHH", bytes(data))
    return "P1 = {: 3.1f}kPa &nbsp;&nbsp;&nbsp; P2 = {: 3.1f}kPa &nbsp;&nbsp;&nbsp; P3 = {: 3.1f}kPa &nbsp;&nbsp;&nbsp; P4 = {: 3.1f}kPa".format(pressure1 * 1.25, pressure2 * 1.25, pressure3 * 1.25, pressure4 * 1.25)

@register_packet("m3dl", CAN_MSG_ID_M3DL_TEMP_1_2, "T1 T2")
def temp_1_2(data):
    temp1 = (data[1] << 16) | (data[2] << 8) | (data[3])
    temp2 = (data[5] << 16) | (data[6] << 8) | (data[7])
    temp1 = temp1 if temp1 < (1<<23) else temp1 - (1<<24)
    temp2 = temp2 if temp2 < (1<<23) else temp2 - (1<<24)
    return "T1 = {: 3.2f}'C, T2 = {: 3.2f}'C".format(temp1/1024, temp2/1024)

@register_packet("m3dl", CAN_MSG_ID_M3DL_TEMP_3_4, "T3 T4")
def temp_3_4(data):
    temp3 = (data[1] << 16) | (data[2] << 8) | (data[3])
    temp4 = (data[5] << 16) | (data[6] << 8) | (data[7])
    temp3 = temp3 if temp3 < (1<<23) else temp3 - (1<<24)
    temp4 = temp4 if temp4 < (1<<23) else temp4 - (1<<24)
    return "T3 = {: 3.2f}'C, T4 = {: 3.2f}'C".format(temp3/1024, temp4/1024)

@register_packet("m3dl", CAN_MSG_ID_M3DL_TEMP_5_6, "T5 T6")
def temp_5_6(data):
    temp5 = (data[1] << 16) | (data[2] << 8) | (data[3])
    temp6 = (data[5] << 16) | (data[6] << 8) | (data[7])
    temp5 = temp5 if temp5 < (1<<23) else temp5 - (1<<24)
    temp6 = temp6 if temp6 < (1<<23) else temp6 - (1<<24)
    return "T5 = {: 3.2f}'C, T6 = {: 3.2f}'C".format(temp5/1024, temp6/1024)

@register_packet("m3dl", CAN_MSG_ID_M3DL_TEMP_7_8, "T7 T8")
def temp_7_8(data):
    temp7 = (data[1] << 16) | (data[2] << 8) | (data[3])
    temp8 = (data[5] << 16) | (data[6] << 8) | (data[7])
    temp7 = temp7 if temp7 < (1<<23) else temp7 - (1<<24)
    temp8 = temp8 if temp8 < (1<<23) else temp8 - (1<<24)
    return "T7 = {: 3.2f}'C, T8 = {: 3.2f}'C".format(temp7/1024, temp8/1024)

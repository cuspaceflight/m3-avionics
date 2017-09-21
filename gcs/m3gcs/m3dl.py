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

components = {
    1: "Temperature",
    2: "SD Card",
    3: "Pressure"
}

component_errors = {
    0: "No Error",
    1: "LTC2983 TX Overflow", 2: "LTC2983 Setup",
    3: "SD Card Connection", 4: "SD Card Mounting",
    5: "SD Card File Open", 6: "SD Card Inc File Open",
    7: "SD Card Write", 8: "Logging Cache Flush", 9: "SD Card FULL", 
    16: "T1 Invalid", 17: "T2 Invalid", 18: "T3 Invalid",
    19: "T4 Invalid", 20: "T5 Invalid", 21: "T6 Invalid",
    22: "T4 Invalid", 23: "T5 Invalid", 24: "T9 Invalid",
    25: "CRC Failure", 32: "Pressure Timeout"
}

compstatus = {k: {"state": 0, "reason": "Unknown"} for k in components}

@register_packet("m3dl", CAN_MSG_ID_M3DL_STATUS, "Status")
def status(data):
    global compstatus
    # The string must start with 'OK:' 'INIT:' or 'ERROR:' as the web
    # interface watches for these and applies special treatment (colours)
    statuses = {0: "OK", 1: "INIT", 2: "ERROR"}
    overall, comp, comp_state = data[:3]
    if len(data) == 4:
        comp_error = data[3]
    else:
        comp_error = 0

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

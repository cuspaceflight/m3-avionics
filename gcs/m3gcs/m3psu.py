from .packets import register_packet, register_command
import struct


def msg_id(x):
    return x << 5


CAN_ID_M3PSU = 2
CAN_MSG_ID_M3PSU_BATT_VOLTAGES = CAN_ID_M3PSU | msg_id(56)
CAN_MSG_ID_M3PSU_TOGGLE_PYROS = CAN_ID_M3PSU | msg_id(16)
CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12 = CAN_ID_M3PSU | msg_id(49)
CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34 = CAN_ID_M3PSU | msg_id(50)
CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56 = CAN_ID_M3PSU | msg_id(51)
CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78 = CAN_ID_M3PSU | msg_id(52)
CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910 = CAN_ID_M3PSU | msg_id(53)
CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112= CAN_ID_M3PSU | msg_id(54)
CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL = CAN_ID_M3PSU | msg_id(17)
CAN_MSG_ID_M3PSU_PYRO_STATUS = CAN_ID_M3PSU | msg_id(48)
CAN_MSG_ID_M3PSU_CHARGER_STATUS = CAN_ID_M3PSU | msg_id(55)
CAN_MSG_ID_M3PSU_TOGGLE_CHARGER = CAN_ID_M3PSU | msg_id(18)
CAN_MSG_ID_M3PSU_TOGGLE_LOWPOWER = CAN_ID_M3PSU | msg_id(19)
CAN_MSG_ID_M3PSU_TOGGLE_BATTLESHORT = CAN_ID_M3PSU | msg_id(20)
CAN_MSG_ID_M3PSU_CAPACITY = CAN_ID_M3PSU | msg_id(57)


@register_packet("m3psu", CAN_MSG_ID_M3PSU_BATT_VOLTAGES,
    "Battery Voltages")
def batt_volts(data):
    # 3 bytes
    cell1, cell2, batt = struct.unpack("HHH", bytes(data[:6]))
    cell1 *= 0.01
    cell2 *= 0.01
    batt *= 0.01

    return "{: 4.2f}V, {: 4.2f}V, Batt: {: 4.2f}V".format(cell1, cell2, batt)


@register_packet("m3psu", CAN_MSG_ID_M3PSU_TOGGLE_PYROS, "Toggle Pyros")
def toggle_pyros(data):
    if data[0] == 0:
        return "Disable pyros"
    elif data[0] == 1:
        return "Enable pyros"
    else:
        return "Invalid packet"


@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12,
    "Channel 1,2 status")
@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34,
    "Channel 3,4 status")
@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56,
    "Channel 5,6 status")
@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78,
    "Channel 7,8 status")
@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910,
    "Channel 9,10 status")
@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112,
    "Channel 11,12 status")
def channel_status(data):
    # 8 bytes: voltage/0.03V, current/0.003A, power/0.02W, blank,
    # voltage/0.03V, current/0.003A, power/0.02W, blank
    voltage, current, power = struct.unpack("BBBx", bytes(data[:4]))
    string = "{: 5.3f}V {: 6.3f}A {: 5.2f}W".format(voltage * 0.03,
        current * 0.003, power * 0.02)
    voltage, current, power = struct.unpack("BBBx", bytes(data[4:]))
    string += ", {: 5.3f}V {: 6.3f}A {: 5.2f}W".format(voltage * 0.03,
        current * 0.003, power * 0.02)
    return string


@register_packet("m3psu", CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL,
    "Toggle Channel")
def toggle_channel(data):
    if data[0] == 0:
        return "Disable channel {: 2d}".format(data[1])
    elif data[1] == 1:
        return "Enable channel {: 2d}".format(data[1])
    else:
        return "Invalid packet"


@register_packet("m3psu", CAN_MSG_ID_M3PSU_PYRO_STATUS, "Pyro Status")
def pyro_status(data):
    # 7 bytes: 16bit voltage (mV), 16bit current (uA), 16bit power (0.1mW)
    # 1bit pyro enable line measurement
    voltage, current, power, pyro = struct.unpack("HHHB", bytes(data[:7]))
    string = "{: 5.3f}V {: 6.3f}A {: 7.3f}W".format(voltage/1000.0,
        current/1000000.0, power/100000.0)
    if pyro == 1:
        string += ", pyro enabled"
    elif pyro == 0:
        string += ", pyro disabled"
    else:
        string += ", invalid packet!"
    return string

@register_packet("m3psu", CAN_MSG_ID_M3PSU_CHARGER_STATUS, "Charger Status")
def charger_status(data):
    # 5 bytes. First two are charge current in mA, 3rd is status bits, 4-5th are temperature in cK
    current, state, tempcK = struct.unpack("=hBH", bytes(data[:5]))
    charger_enabled = bool(state & 1)
    is_charging = bool(state & 2)
    charge_inhibit = bool(state & 4)
    battleshort = bool(state & 32)
    voltage_mode = (state >> 3) & 0x3;
    tempC = (tempcK/10) - 273.2

    string = "{: 4d}mA, {: 3.1f}degC".format(current, tempC)
    if charger_enabled:
        string += ", charger enabled"
    else:
        string += ", charger disabled"
    if is_charging:
        string += ", charging"
    if charge_inhibit:
        string += ", inhibited"
    if battleshort:
        string += ", WAR MODE"
    else:
        string += ", peace mode"
    string += ", {} mode".format(["PCV", "LV", "MV", "HV", "INVAL"][voltage_mode])
    return string

@register_packet("m3psu", CAN_MSG_ID_M3PSU_TOGGLE_CHARGER, "Toggle Charger")
def toggle_charger(data):
    if data[0] == 0:
        return "Disable charger"
    elif data[0] == 1:
        return "Enable charger"
    else:
        return "Invalid packet"

@register_packet("m3psu", CAN_MSG_ID_M3PSU_CAPACITY, "Capacity")
def capacity(data):
    mins, = struct.unpack("h", bytes(data[:2]))
    if mins == -1:
        mins = "Inf"
    percent = data[2]
    return "Capacity: {}%, Time left: {}".format(percent, mins)


@register_command("m3psu", "Pyro supply", ("Off", "On"))
def toggle_pyros_cmd(data):
    data = [{"Off":0, "On":1}[data]]
    return CAN_MSG_ID_M3PSU_TOGGLE_PYROS, data

@register_command("m3psu", "Charger", ("Off", "On"))
def toggle_charger_cmd(data):
    data = [{"Off":0, "On":1}[data]]
    return CAN_MSG_ID_M3PSU_TOGGLE_CHARGER, data

@register_command("m3psu", "Lowpower", ("Off", "On"))
def toggle_lowpower_cmd(data):
    data = [{"Off":0, "On":1}[data]]
    return CAN_MSG_ID_M3PSU_TOGGLE_LOWPOWER, data

@register_command("m3psu", "Battleshort", ("Peace", "War"))
def toggle_battleshort(data):
    data = [{"Peace":0, "War":1}[data]]
    return CAN_MSG_ID_M3PSU_TOGGLE_BATTLESHORT, data

@register_command("m3psu", "5V IMU", ("1 Off", "1 On"))
@register_command("m3psu", "5V AUX 2", ("2 Off", "2 On"))
@register_command("m3psu", "3V3 FC", ("3 Off", "3 On"))
@register_command("m3psu", "3V3 IMU", ("4 Off", "4 On"))
@register_command("m3psu", "5V Radio", ("5 Off", "5 On"))
@register_command("m3psu", "5V AUX 1", ("6 Off", "6 On"))
@register_command("m3psu", "3V3 Pyro", ("7 Off", "7 On"))
@register_command("m3psu", "3V3 Radio", ("8 Off", "8 On"))
@register_command("m3psu", "5V Cameras", ("9 Off", "9 On"))
@register_command("m3psu", "3V3 AUX 1", ("10 Off", "10 On"))
@register_command("m3psu", "3V3 DL", ("11 Off", "11 On"))
@register_command("m3psu", "5V CAN", ("12 Off", "12 On"))
def toggle_channel_cmd(data):
    [channel, operation] = data.split(" ")
    data = [{"Off":0, "On":1}[operation], int(channel)-1]
    return CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL, data


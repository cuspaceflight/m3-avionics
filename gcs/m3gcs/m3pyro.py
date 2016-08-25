from .packets import register_packet, register_command


def msg_id(x):
    return x << 5


CAN_ID_M3PYRO = 2
CAN_MSG_ID_M3PYRO_FIRE_COMMAND = (CAN_ID_M3PYRO | msg_id(1))
CAN_MSG_ID_M3PYRO_ARM_COMMAND = (CAN_ID_M3PYRO | msg_id(2))
CAN_MSG_ID_M3PYRO_FIRE_STATUS = (CAN_ID_M3PYRO | msg_id(16))
CAN_MSG_ID_M3PYRO_ARM_STATUS = (CAN_ID_M3PYRO | msg_id(17))
CAN_MSG_ID_M3PYRO_CONTINUITY = (CAN_ID_M3PYRO | msg_id(48))
CAN_MSG_ID_M3PYRO_SUPPLY_STATUS = (CAN_ID_M3PYRO | msg_id(49))


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_FIRE_STATUS, "Fire Status")
def fire_status(data):
    status_map = {0: "Off", 1: "Firing", 2: "Pulsed Firing"}
    return "Ch1: {}, Ch2: {}, Ch3: {}, Ch4: {}".format(
        *[status_map.get(x, "Unknown") for x in data[:4]])


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_ARM_STATUS, "Arm Status")
def arm_status(data):
    status_map = {0: "Disarmed", 1: "Armed"}
    return status_map.get(data[0], "Unknown")


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_CONTINUITY, "Continuity")
def continuity(data):
    resistances = ["{:.1f}Ω".format(float(d)/10) if d != 255 else "HI"
                   for d in data[:4]]
    return "Ch1: {}, Ch2: {}, Ch3: {}, Ch4: {}".format(*resistances)


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, "Supply Status")
def supply_status(data):
    return "{:.1f}V".format(float(data[0])/10)


@register_command("m3pyro", "Fire Ch1", ("Off", "Fire", "Pulsed Fire"))
def fire_ch1(data):
    command_map = {"Off": 0, "Fire": 1, "Pulsed Fire": 2}
    data = [command_map.get(data, 0), 0, 0, 0]
    return CAN_MSG_ID_M3PYRO_FIRE_COMMAND, data


@register_command("m3pyro", "Fire Ch2", ("Off", "Fire", "Pulsed Fire"))
def fire_ch2(data):
    command_map = {"Off": 0, "Fire": 1, "Pulsed Fire": 2}
    data = [0, command_map.get(data, 0), 0, 0]
    return CAN_MSG_ID_M3PYRO_FIRE_COMMAND, data


@register_command("m3pyro", "Fire Ch3", ("Off", "Fire", "Pulsed Fire"))
def fire_ch3(data):
    command_map = {"Off": 0, "Fire": 1, "Pulsed Fire": 2}
    data = [0, 0, command_map.get(data, 0), 0]
    return CAN_MSG_ID_M3PYRO_FIRE_COMMAND, data


@register_command("m3pyro", "Fire Ch4", ("Off", "Fire", "Pulsed Fire"))
def fire_ch4(data):
    command_map = {"Off": 0, "Fire": 1, "Pulsed Fire": 2}
    data = [0, 0, 0, command_map.get(data, 0)]
    return CAN_MSG_ID_M3PYRO_FIRE_COMMAND, data


@register_command("m3pyro", "Arm", ("Disarm", "Arm"))
def arm_command(data):
    command_map = {"Disarm": 0, "Arm": 1}
    data = [command_map.get(data, 0)]
    return CAN_MSG_ID_M3PYRO_ARM_COMMAND, data

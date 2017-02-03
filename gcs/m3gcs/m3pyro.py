from .packets import register_packet, register_command


def msg_id(x):
    return x << 5


CAN_ID_M3PYRO = 3
CAN_MSG_ID_M3PYRO_STATUS = (CAN_ID_M3PYRO | msg_id(0))
CAN_MSG_ID_M3PYRO_FIRE_COMMAND = (CAN_ID_M3PYRO | msg_id(1))
CAN_MSG_ID_M3PYRO_ARM_COMMAND = (CAN_ID_M3PYRO | msg_id(2))
CAN_MSG_ID_M3PYRO_FIRE_STATUS = (CAN_ID_M3PYRO | msg_id(16))
CAN_MSG_ID_M3PYRO_ARM_STATUS = (CAN_ID_M3PYRO | msg_id(17))
CAN_MSG_ID_M3PYRO_CONTINUITY = (CAN_ID_M3PYRO | msg_id(48))
CAN_MSG_ID_M3PYRO_SUPPLY_STATUS = (CAN_ID_M3PYRO | msg_id(49))


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_STATUS, "Status")
def status(data):
    status_map = {0: "OK", 1: "Init", 2: "Error"}
    return "{}".format(
        status_map.get(data[0], "Unknown"))


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_FIRE_STATUS, "Fire Status")
def fire_status(data):
    status_map = {0: "Off", 1: "EMatch", 2: "Talon", 3: "Metron"}
    return "Ch1: {}, Ch2: {}, Ch3: {}, Ch4: {}".format(
        *[status_map.get(x, "Unknown") for x in data[:4]])


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_ARM_STATUS, "Arm Status")
def arm_status(data):
    status_map = {0: "Disarmed", 1: "Armed"}
    return status_map.get(data[0], "Unknown")


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_CONTINUITY, "Continuity")
def continuity(data):
    resistances = ["{:.1f}Ω".format(float(d)*2) if d != 255 else "HI"
                   for d in data[:4]]
    # Disabled reading of 2 byte raw ADC values
    # resistances = ["{:d}".format(d)
    #                for d in struct.unpack("HHHH", bytes(data))]
    return "Ch1: {}, Ch2: {}, Ch3: {}, Ch4: {}".format(*resistances)


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, "Supply Status")
def supply_status(data):
    return "{:.1f}V".format(float(data[0])/10)


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_FIRE_COMMAND, "Fire")
def fire(data):
    command_map = {0: "Off", 1: "EMatch", 2: "Talon", 3: "Metron"}
    string = ""
    for ch, op in enumerate(data):
        if command_map[op] == "Off":
            continue
        else:
            string += "Channel {} type {}, ".format(ch+1, command_map[op])
    if string == "":
        string = "No channels firing"
    return string


@register_command("m3pyro", "Fire Ch1",
                  ("1 Off", "1 EMatch", "1 Talon", "1 Metron"))
@register_command("m3pyro", "Fire Ch2",
                  ("2 Off", "2 EMatch", "2 Talon", "2 Metron"))
@register_command("m3pyro", "Fire Ch3",
                  ("3 Off", "3 EMatch", "3 Talon", "3 Metron"))
@register_command("m3pyro", "Fire Ch4",
                  ("4 Off", "4 EMatch", "4 Talon", "4 Metron"))
def fire_ch_cmd(data):
    [channel, operation] = data.split(" ")
    command_map = {"Off": 0, "EMatch": 1, "Talon": 2, "Metron": 3}
    data = [0, 0, 0, 0]
    data[int(channel)-1] = int(command_map[operation])
    return CAN_MSG_ID_M3PYRO_FIRE_COMMAND, data


@register_command("m3pyro", "Arm", ("Disarm", "Arm"))
def arm_command(data):
    command_map = {"Disarm": 0, "Arm": 1}
    data = [command_map.get(data, 0)]
    return CAN_MSG_ID_M3PYRO_ARM_COMMAND, data

from .packets import register_packet, register_command
import struct
import datetime


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

components = {
    1: "HAL",
    2: "Self Test",
    3: "Continuity",
    4: "Firing",
}

component_errors = {
    0: "No Error",
    1: "ADC", 2: "Bad Channel", 3: "Discharge", 4: "Continuity", 5: "1A",
    6: "3A", 7: "Supply", 8: "EStop", 9: "Fire Type Unknown",
    10: "Fire Supply Unknown", 11: "Fire Supply Fault", 12: "Fire Bad Msg"
}

compstatus = {k: {"state": 0, "reason": "Unknown"} for k in components}

@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_STATUS, "Status")
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
                   for d in data[:8]]
    return """
    <table style='font-family: mono;'>
        <tr>
            <td style="padding: 3px"><strong>Ch1:</strong><br>{}</td>
            <td style="padding: 3px"><strong>Ch2:</strong><br>{}</td>
            <td style="padding: 3px"><strong>Ch3:</strong><br>{}</td>
            <td style="padding: 3px"><strong>Ch4:</strong><br>{}</td>
        </tr>
        <tr>
            <td style="padding: 3px"><strong>Ch5:</strong><br>{}</td>
            <td style="padding: 3px"><strong>Ch6:</strong><br>{}</td>
            <td style="padding: 3px"><strong>Ch7:</strong><br>{}</td>
            <td style="padding: 3px"><strong>Ch8:</strong><br>{}</td>
        </tr>
    </table>""".format(*resistances).replace("\n","")


@register_packet("m3pyro", CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, "Supply Status")
def supply_status(data):
    return "Supply {:.1f}V, Bus {:.1f}V".format(float(data[0])/10,
            float(data[1])/10)


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
                  ("1 Off", "1 EMatch 1A", "1 Talon 1A", "1 Metron 1A",
                            "1 EMatch 3A", "1 Talon 3A", "1 Metron 3A"))
@register_command("m3pyro", "Fire Ch2",
                  ("2 Off", "2 EMatch 1A", "2 Talon 1A", "2 Metron 1A",
                            "2 EMatch 3A", "2 Talon 3A", "2 Metron 3A"))
@register_command("m3pyro", "Fire Ch3",
                  ("3 Off", "3 EMatch 1A", "3 Talon 1A", "3 Metron 1A",
                            "3 EMatch 3A", "3 Talon 3A", "3 Metron 3A"))
@register_command("m3pyro", "Fire Ch4",
                  ("4 Off", "4 EMatch 1A", "4 Talon 1A", "4 Metron 1A",
                            "4 EMatch 3A", "4 Talon 3A", "4 Metron 3A"))
@register_command("m3pyro", "Fire Ch5",
                  ("5 Off", "5 EMatch 1A", "5 Talon 1A", "5 Metron 1A",
                            "5 EMatch 3A", "5 Talon 3A", "5 Metron 3A"))
@register_command("m3pyro", "Fire Ch6",
                  ("6 Off", "6 EMatch 1A", "6 Talon 1A", "6 Metron 1A",
                            "6 EMatch 3A", "6 Talon 3A", "6 Metron 3A"))
@register_command("m3pyro", "Fire Ch7",
                  ("7 Off", "7 EMatch 1A", "7 Talon 1A", "7 Metron 1A",
                            "7 EMatch 3A", "7 Talon 3A", "7 Metron 3A"))
@register_command("m3pyro", "Fire Ch8",
                  ("8 Off", "8 EMatch 1A", "8 Talon 1A", "8 Metron 1A",
                            "8 EMatch 3A", "8 Talon 3A", "8 Metron 3A"))
def fire_ch_cmd(data):
    [channel, operation] = data.split(" ", 1)
    command_map = {"Off": 0,
                   "EMatch 1A": 0x01, "Talon 1A": 0x02, "Metron 1A": 0x03,
                   "EMatch 3A": 0x11, "Talon 3A": 0x12, "Metron 3A": 0x13}
    data = [0, 0, 0, 0, 0, 0, 0, 0]
    data[int(channel)-1] = int(command_map[operation])
    return CAN_MSG_ID_M3PYRO_FIRE_COMMAND, data


@register_command("m3pyro", "Arm", ("Disarm", "Arm"))
def arm_command(data):
    command_map = {"Disarm": 0, "Arm": 1}
    data = [command_map.get(data, 0)]
    return CAN_MSG_ID_M3PYRO_ARM_COMMAND, data

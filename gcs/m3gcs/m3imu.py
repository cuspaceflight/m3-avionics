from .packets import register_packet
import struct


def msg_id(x):
    return x << 5


CAN_ID_M3IMU = 5
CAN_MSG_ID_M3IMU_STATUS = CAN_ID_M3IMU | msg_id(0)

components = {
    1: "Mission Control",
    2: "State Estimation",
    3: "Configuration",
    4: "Beeper",
    5: "LEDs",
    6: "Accelerometer",
    7: "Barometer",
    8: "Flash",
    9: "Pyros",
    10: "Mock"
}

components = {
    0: "messaging",
    1: "telemetry_allocator",
    2: "adis16405",
    3: "mpu9250",
    4: "ms5611",
    5: "can_telemetry",
    6: "world_mag_model",
    7: "ublox",
    8: "sd_card",
    9: "file_telemetry_output",
    10: "state_board_config",
}

compstatus = {k: {"state": 0, "reason": "Unknown"} for k in components}


@register_packet("m3imu", CAN_MSG_ID_M3IMU_STATUS, "Status")
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
        string += " {})".format(comp_error)
    else:
        string += ")"

    # Update our perception of the overall state
    if comp in compstatus:
        compstatus[comp]['state'] = comp_state
        compstatus[comp]['reason'] = comp_error

    # List all components we believe to be in error
    errors = ""
    for k in components:
        if compstatus[k]['state'] == 2:
            errors += "\n{}: {}".format(components[k], compstatus[k]['reason'])
    if errors:
        string += "\nErrors:" + errors
    return string

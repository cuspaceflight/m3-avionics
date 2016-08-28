from .packets import register_packet, register_command
import struct
from math import sqrt


def msg_id(x):
    return x << 5


CAN_ID_M3FC = 1
CAN_MSG_ID_M3FC_STATUS = CAN_ID_M3FC | msg_id(0)
CAN_MSG_ID_M3FC_MISSION_STATE = CAN_ID_M3FC | msg_id(32)
CAN_MSG_ID_M3FC_ACCEL = CAN_ID_M3FC | msg_id(48)
CAN_MSG_ID_M3FC_BARO = CAN_ID_M3FC | msg_id(49)
CAN_MSG_ID_M3FC_SE_T_H = CAN_ID_M3FC | msg_id(50)
CAN_MSG_ID_M3FC_SE_V_A = CAN_ID_M3FC | msg_id(51)
CAN_MSG_ID_M3FC_SE_VAR_H = CAN_ID_M3FC | msg_id(52)
CAN_MSG_ID_M3FC_SE_VAR_V_A = CAN_ID_M3FC | msg_id(53)
CAN_MSG_ID_M3FC_CFG_PROFILE = CAN_ID_M3FC | msg_id(54)
CAN_MSG_ID_M3FC_CFG_PYROS = CAN_ID_M3FC | msg_id(55)
CAN_MSG_ID_M3FC_SET_CFG_PROFILE = CAN_ID_M3FC | msg_id(1)
CAN_MSG_ID_M3FC_SET_CFG_PYROS = CAN_ID_M3FC | msg_id(2)
CAN_MSG_ID_M3FC_LOAD_CFG = CAN_ID_M3FC | msg_id(3)
CAN_MSG_ID_M3FC_SAVE_CFG = CAN_ID_M3FC | msg_id(4)

components = ["Unknown", "Mission Control", "State estimation", "Config",
              "Beeper", "LEDs", "Accelerometer", "Barometer", "Flash",
              "Pyros", "Mock"]
component_errors = ["Unknown",
                    "CRC", "Write", # Flash errors
                    "Read", # Config errors
                    "Pyro1", "Pyro2", "Pyro3", "Pyro4", # Pyro errors
                    "Profile", "Pyros", # More config errors
                    "Bad ID", "Selftest", "Timeout", "Axis", # Accel err
                    "Pressure", # State estimator error
                    "Pyro Arm", # Microcontroller error
                    "Mock Enabled"] # Mock enabled

compstatus = {k:{"state":0, "reason":"Unknown"} for k in components}

@register_packet("m3fc", CAN_MSG_ID_M3FC_STATUS, "Status")
def status(data):
    global compstatus
    # The string must start with 'OK:' 'INIT:' or 'ERROR:' as the web
    # interface watches for these and applies special treatment (colours)
    statuses = ["OK", "INIT", "ERROR"]
    
    overall, comp, comp_state, comp_error = struct.unpack("BBBB",
        bytes(data[:4]))
    string = "{}: ".format(statuses[overall])
    compstatus[components[comp]]['state'] = comp_state
    compstatus[components[comp]]['reason'] = component_errors[comp_error]
    for k in components[1:]:
        if compstatus[k]['state'] == 2: # Component is in error
            string += "\n{}: {}".format(k, compstatus[k]['reason'])
    return string


@register_packet("m3fc", CAN_MSG_ID_M3FC_MISSION_STATE, "Mission State")
def mission_state(data):
    # 5 bytes total. 4 bytes met, 1 byte can_state
    met, can_state = struct.unpack("IB", bytes(data[:5]))
    states = ["Init", "Pad", "Ignition", "Powered Ascent", "Burnout",
              "Free Ascent", "Apogee", "Drogue Ascent",
              "Release Main", "Main Descent", "Land", "Landed"]
    return "MET: {: 9.3f} s, State: {}".format(met/1000.0, states[can_state])


@register_packet("m3fc", CAN_MSG_ID_M3FC_ACCEL, "Acceleration")
def accel(data):
    # 6 bytes, 3 int16_ts for 3 accelerations
    # 3.9 MSB per milli-g
    factor = 3.9 / 1000.0 * 9.80665
    accel1, accel2, accel3 = struct.unpack("hhh", bytes(data[0:6]))
    accel1, accel2, accel3 = accel1*factor, accel2*factor, accel3*factor
    return "{: 3.1f} m/s/s {: 3.1f} m/s/s {: 3.1f} m/s/s".format(accel1,
        accel2, accel3)
    

@register_packet("m3fc", CAN_MSG_ID_M3FC_BARO, "Barometer")
def baro(data):
    # 8 bytes: 4 bytes of temperature in centidegrees celcius,
    # 4 bytes of pressure in Pascals
    temperature, pressure = struct.unpack("ii", bytes(data))
    return "Temperature: {: 4.1f}'C, Pressure: {: 6.0f} Pa".format(
        temperature / 100.0, pressure)


@register_packet("m3fc", CAN_MSG_ID_M3FC_SE_T_H, "State Estimate T,H")
def se_t_h(data):
    # 8 bytes, 2 float32s
    dt, h = struct.unpack("ff", bytes(data))
    return "dt: {: 6.4f} s, altitude: {: 5.0f} m".format(dt, h)

@register_packet("m3fc", CAN_MSG_ID_M3FC_SE_V_A, "State Estimate V,A")
def se_v_a(data):
    v, a = struct.unpack("ff", bytes(data))
    return "velocity: {: 6.1f} m/s, acceleration: {: 5.1f} m/s/s".format(v, a)


@register_packet("m3fc", CAN_MSG_ID_M3FC_SE_VAR_H, "State Estimate var(H)")
def se_var_h(data):
    (var_h,) = struct.unpack("f", bytes(data[0:4]))
    return "SD(altitude): {: 7.3f} m^2".format(sqrt(var_h))


@register_packet("m3fc", CAN_MSG_ID_M3FC_SE_VAR_V_A,
    "State Estimate var(V),var(A)")
def se_var_v_var_a(data):
    var_v, var_a = struct.unpack("ff", bytes(data))
    return "SD(velocity): {: 6.3f} m^2, SD_acceleration: {: 5.3f} m^2".format(
        sqrt(var_v), sqrt(var_a))


@register_packet("m3fc", CAN_MSG_ID_M3FC_CFG_PROFILE, "Profile config")
@register_packet("m3fc", CAN_MSG_ID_M3FC_SET_CFG_PROFILE, "Set profile config")
def cfg_profile(data):
    position = {1: "dart", 2: "core"}.get(data[0], "unset")
    accel_axis = {1:"X", 2:"-X", 3:"Y", 4:"-Y", 5:"Z", 6:"-Z"}.get(data[1],
        "unset")
    ignition_accel, burnout_timeout, apogee_timeout = data[2:5]
    main_altitude, main_timeout, land_timeout = data[5:8]
    return ("Position: {}, ".format(position) +
            "Accelerometer axis: {}, ".format(accel_axis) +
            "Ignition acceleration: {: 4.1f} m/s/s, ".format(ignition_accel) +
            "Burnout timeout: {: 4.1f} s, ".format(burnout_timeout/10.0) +
            "Apogee timeout: {: 3d} s, ".format(apogee_timeout) +
            "Main altitude: {: 4d} m, ".format(main_altitude*10) +
            "Main timeout: {: 3d} s, ".format(main_timeout) +
            "Land timeout: {: 4d} s".format(land_timeout*10))
            

@register_packet("m3fc", CAN_MSG_ID_M3FC_CFG_PYROS, "Pyros config")
@register_packet("m3fc", CAN_MSG_ID_M3FC_SET_CFG_PYROS, "Set pyro config")
def cfg_pyros(data):
    usages = {0:"None", 1:"Drogue", 2:"Main", 3:"Dart Separation"}
    types = {0:"None", 1:"E-match", 2:"Talon", 3:"Metron"}
    pyro_1_usage, pyro_2_usage, pyro_3_usage, pyro_4_usage = [
        usages.get(x, "Unset") for x in data[:4]]
    pyro_1_type, pyro_2_type, pyro_3_type, pyro_4_type = [
        types.get(x, "Unset") for x in data[4:]]

    return ("(usage/type) Pyro1: {}/{}, Pyro2: {}/{}, Pyro3: {}/{}, "
          "Pyro4: {}/{}".format(pyro_1_usage, pyro_1_type,
                                pyro_2_usage, pyro_2_type,
                                pyro_3_usage, pyro_3_type,
                                pyro_4_usage, pyro_4_type))


@register_packet("m3fc", CAN_MSG_ID_M3FC_LOAD_CFG, "Load config")
def load_config(data):
    return "Load config from flash"
@register_packet("m3fc", CAN_MSG_ID_M3FC_SAVE_CFG, "Save config")
def save_config(data):
    return "Save config to flash"


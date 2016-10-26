import ctypes

# Link against our fake mission library
lib = ctypes.CDLL("./libm3fc_mission.so")


# Get a reference to an object inside the library.
trust_baro = ctypes.c_bool.in_dll(lib, "m3fc_state_estimation_trust_barometer")
print("trust_baro =", trust_baro.value)


# Our mock functions can just call a Python function via a callback,
# which we can provide like this:
# First, make the "type" of the callback
py_can_send_t = ctypes.CFUNCTYPE(
    None, ctypes.c_uint16, ctypes.c_bool,
    ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8)
# Make our Python implementation
def can_send(msg_id, can_rtr, data_p, datalen):
    data = data_p[0:datalen]
    print("can_send: ", msg_id, can_rtr, data, datalen)
# Finally, set the global function pointer in the mock library to our function
ctypes.c_void_p.in_dll(lib, "py_can_send").value = \
    ctypes.cast(py_can_send_t(can_send), ctypes.c_void_p).value


# Call m3fc_mission_init, which will kick off the mission code
lib.m3fc_mission_init()

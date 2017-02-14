import sys
import struct
import matplotlib.pyplot as plt

SID_M3FC_MISSION_STATE = (32 << 5) | 1
SID_M3FC_ACCEL = (48 << 5) | 1
SID_M3FC_BARO = (49 << 5) | 1
SID_M3FC_SE_T_H = (50 << 5) | 1
SID_M3FC_SE_V_A = (51 << 5) | 1
SID_M3PYRO_SUPPLY = (49 << 5) | 3

accel_times = []
accel_vals = []
pressure_times = []
pressure_vals = []
se_a_times = []
se_a_vals = []
se_v_times = []
se_v_vals = []
se_h_times = []
se_h_vals = []
state_times = []
state_vals = []
supply_times = []
supply_vals = []

states = ["init", "pad", "ignition", "powered ascent", "burnout",
          "free ascent", "apogee", "drogue descent", "release main",
          "main descent", "land", "landed"]

prev_state = 0
with open(sys.argv[1], "rb") as f:
    packet = f.read(16)
    while packet:
        sid, rtr, dlc, data, ts = struct.unpack("<HBB8sI", packet)
        ts /= 1e4
        if sid == SID_M3FC_MISSION_STATE:
            _, state = struct.unpack("<IB", data[:5])
            if state != prev_state:
                state_times.append(ts)
                state_vals.append(state)
                prev_state = state
        elif sid == SID_M3FC_ACCEL:
            _, _, z = struct.unpack("<hhh", data[:6])
            accel_times.append(ts)
            accel_vals.append(z*3.9e-3*9.81)
        elif sid == SID_M3FC_BARO:
            _, pressure = struct.unpack("<ii", data)
            pressure_times.append(ts)
            pressure_vals.append(pressure)
        elif sid == SID_M3FC_SE_V_A:
            se_v, se_a = struct.unpack("<ff", data)
            se_a_times.append(ts)
            se_a_vals.append(se_a)
            se_v_times.append(ts)
            se_v_vals.append(se_v)
        elif sid == SID_M3FC_SE_T_H:
            _, se_h = struct.unpack("<ff", data)
            se_h_times.append(ts)
            se_h_vals.append(se_h)
        elif sid == SID_M3PYRO_SUPPLY:
            supply = struct.unpack("<B", data[:1])
            supply_times.append(ts)
            supply_vals.append(supply)
        packet = f.read(16)

n_accels = len(accel_vals)
t_accels = accel_times[-1] - accel_times[0]
n_baros = len(pressure_vals)
t_baros = pressure_times[-1] - pressure_times[0]

print("{} accels over {}s".format(n_accels, t_accels))
print("{} baros over {}s".format(n_baros, t_baros))

ax1 = plt.gca()
#ax1.step(accel_times, accel_vals, 'b-', label="Accelerometer Reading")
#ax3 = plt.twinx()
#ax3.step(pressure_times, pressure_vals, 'g-')
#ax2.step(supply_times, supply_vals, 'g-')
ax1.step(se_a_times, se_a_vals, 'g-', label="Accel")
ax2 = plt.twinx()
ax2.step(se_v_times, se_v_vals, 'b-', label="Vel")
ax2.step(se_h_times, se_h_vals, 'r-', label="Height")
plt.xlabel("Time (s)")
plt.legend()

for t, s in zip(state_times, state_vals):
    plt.axvline(t)
    ydisp = plt.gca().transAxes.transform((0, 1-s/len(states)))[1]
    ydata = plt.gca().transData.inverted().transform((0, ydisp))[1]
    plt.text(t, ydata, states[s])

plt.grid()
plt.show()

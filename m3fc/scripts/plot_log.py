import sys
import math
import struct
import numpy as np
import matplotlib.pyplot as plt

SID_M3FC_MISSION_STATE = (32 << 5) | 1
SID_M3FC_ACCEL = (48 << 5) | 1
SID_M3FC_BARO = (49 << 5) | 1
SID_M3FC_SE_T_H = (50 << 5) | 1
SID_M3FC_SE_V_A = (51 << 5) | 1
SID_M3FC_SE_VAR_H = (52 << 5) | 1
SID_M3FC_SE_VAR_V_A = (53 << 5) | 1
SID_M3PYRO_SUPPLY = (49 << 5) | 3
SID_M3RADIO_GPS_ALT = (49 << 5) | 4
SID_M3DL_PRESSURE = (53 << 5) | 6

accel_times = []
accel_vals = []
pressure_times = []
pressure_vals = []
baro_alt_times = []
baro_alt_vals = []
se_a_times = []
se_a_vals = []
se_v_times = []
se_v_vals = []
se_h_times = []
se_h_vals = []
se_var_h_times = []
se_var_h_vals = []
se_var_v_times = []
se_var_v_vals = []
se_var_a_times = []
se_var_a_vals = []
state_times = []
state_vals = []
supply_times = []
supply_vals = []
gps_alt_times = []
gps_alt_vals = []
dl_pressure_times = []
dl_pressure_vals = []

states = ["init", "pad", "ignition", "powered ascent", "burnout",
          "free ascent", "apogee", "drogue descent", "release main",
          "main descent", "land", "landed"]

Rs = 8.31432
g0 = 9.80665
M = 0.0289644
Lb = [-0.0065, 0.0, 0.001, 0.0028, 0.0, -0.0028, -0.002]
Pb = [101325.0, 22632.10, 5474.89, 868.02, 110.91, 66.94, 3.96]
Tb = [288.15, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65]
Hb = [0.0, 11000.0, 20000.0, 32000.0, 47000.0, 51000.0, 71000.0]


def p2a(p):
    if p > Pb[0]:
        return p2a_nzl(p, 0)
    for b in range(7):
        if p <= Pb[b] and p > Pb[b+1]:
            if Lb[b] == 0.0:
                return p2a_zl(p, b)
            else:
                return p2a_nzl(p, b)
    return -9999.0


def p2a_zl(p, b):
    hb = Hb[b]
    pb = Pb[b]
    tb = Tb[b]
    return hb + (Rs * tb)/(g0 * M) * (math.log(p) - math.log(pb))


def p2a_nzl(p, b):
    lb = Lb[b]
    hb = Hb[b]
    pb = Pb[b]
    tb = Tb[b]
    return hb + tb/lb * (math.pow(p/pb, (-Rs*lb)/(g0*M)) - 1)


prev_state = 0
with open(sys.argv[1], "rb") as f:
    f.seek(62561392)  # dart
    #f.seek(64273600)  # booster
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
            if pressure < 1e8:
                pressure_times.append(ts)
                pressure_vals.append(pressure)
                baro_alt_times.append(ts)
                baro_alt_vals.append(p2a(pressure))
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
        elif sid == SID_M3FC_SE_VAR_H:
            se_var_h = struct.unpack("<f", data[:4])
            se_var_h_times.append(ts)
            se_var_h_vals.append(se_h)
        elif sid == SID_M3FC_SE_VAR_V_A:
            se_var_v, se_var_a = struct.unpack("<ff", data)
            se_var_v_times.append(ts)
            se_var_v_vals.append(se_var_v)
            se_var_a_times.append(ts)
            se_var_a_vals.append(se_var_a)
        elif sid == SID_M3PYRO_SUPPLY:
            supply = struct.unpack("<B", data[:1])
            supply_times.append(ts)
            supply_vals.append(supply)
        elif sid == SID_M3RADIO_GPS_ALT:
            _, gps_alt = struct.unpack("<ii", data)
            gps_alt_times.append(ts)
            gps_alt_vals.append(gps_alt/1e3)
        elif sid == SID_M3DL_PRESSURE:
            p1, p2, p3, p4 = struct.unpack("<HHHH", data)
            dl_pressure_times.append(ts)
            dl_pressure_vals.append((p1+p2+p3+p4)/4)
        packet = f.read(16)

n_accels = len(accel_vals)
t_accels = accel_times[-1] - accel_times[0]
n_baros = len(pressure_vals)
t_baros = pressure_times[-1] - pressure_times[0]

print("{} accels over {}s, rate={}/s".format(
    n_accels, t_accels, n_accels/t_accels))
print("{} baros over {}s, rate={}/s".format(
    n_baros, t_baros, n_baros/t_baros))

baro_vel = np.diff(baro_alt_vals) / np.diff(baro_alt_times)

se_h = np.array(se_h_vals)
se_var_h = np.array(se_var_h_vals)
se_std_h = np.sqrt(se_var_h)
#se_range_h_top = se_h + se_std_h
#se_range_h_btm = se_h - se_std_h

se_a = np.array(se_a_vals)
se_var_a = np.array(se_var_a_vals)
se_std_a = np.sqrt(se_var_a)
#se_range_a_top = se_a + se_std_a
#se_range_a_btm = se_a - se_std_a

plt.grid()
ax1 = plt.gca()
#ax2.step(supply_times, supply_vals, 'g-')
ax1.step(accel_times, accel_vals, 'b-', label="Accelerometer")
#ax1.fill_between(se_var_a_times, se_range_a_top, se_range_a_btm, color='c', alpha=0.3)
ax1.step(se_a_times, se_a_vals, 'c-', label="SE Acceleration")
plt.ylim(-200, 200)
plt.ylabel("Acceleration (m/s/s)")
plt.legend(loc='upper left')
ax2 = plt.twinx()
#ax2.step(se_v_times, se_v_vals, 'b-', label="Vel")
#ax2.fill_between(se_var_h_times, se_range_h_top, se_range_h_btm, color='r', alpha=0.3)
#ax2.step(se_var_h_times, se_range_h_top, (1, .8, .8))
#ax2.step(se_var_h_times, se_range_h_btm, (1, .8, .8))
ax2.step(se_h_times, se_h_vals, 'r-', label="SE Altitude")
ax2.step(baro_alt_times, baro_alt_vals, 'g-', label="Barometric")
ax2.step(gps_alt_times, gps_alt_vals, 'm-', label="GPS Alt")
plt.xlabel("Time (s)")
plt.ylabel("Altitude (m)")
plt.legend(loc='upper right')
plt.ylim(-1000, 10000)

ax3 = plt.twinx()
ax3.step(se_v_times, se_v_vals, label="SE Vel (m/s)", color='k', lw=3)
plt.ylabel("Velocity (m/s)")

for t, s in zip(state_times, state_vals):
    plt.axvline(t, color='k')
    ydisp = plt.gca().transAxes.transform((0, 1-s/len(states)))[1]
    ydata = plt.gca().transData.inverted().transform((0, ydisp))[1]
    if s >= len(states):
        s = "Unknown {}".format(s)
    else:
        s = states[s]
    plt.text(t, ydata, s, bbox=dict(facecolor='white'))

plt.show()

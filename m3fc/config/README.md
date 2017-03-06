# M3FC Config Files

Each file contains the following top level keys:
* profile
* pyros
* accel_cal_x
* accel_cal_y
* accel_cal_z
* radio_freq

Profile contains the following keys:
* m3fc_position: either dart or core
* accel_axis: one of x, -x, y, -y, z, -z
* ignition_accel: ignition detection accel threshold in m/s/s
* burnout_timeout: burnout detection timeout in seconds, 0 to 25.5
* apogee_timeout: apogee detection timeout in seconds, 0 to 255
* main_altitude: main chute deployment altitude in metres above launch, 0 to 2550
* main_timeout: main chute deployment timeout in seconds since apogee, 0 to 255
* land_timeout: landing detection timeout in seconds since launch, 0 to 2550

Pyros contains the following keys:
* pyro_1_use: one of unused, drogue, main, dartsep, boostersep
* pyro_1_type: one of none, ematch, talon, metron
* etc for pyro_2, pyro_3, and pyro_4

Each accel_cal contains the following keys:
* scale, the axis scale in g/LSB
* offset, the 0g offset in LSB

Radio freq is in Hz

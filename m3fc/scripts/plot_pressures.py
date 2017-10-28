import numpy as np
import matplotlib.pyplot as plt

with np.load("dart_pressure.npz") as data:
    dart_t = data['t']
    dart_p = data['p']

with np.load("booster_pressure.npz") as data:
    booster_t = data['t']
    booster_p = data['p']

plt.plot(dart_t, dart_p, label="Dart")
plt.plot(booster_t+40.31, booster_p, label="Booster")
plt.xlabel("Time (s)")
plt.ylabel("Pressure (Pa)")
plt.grid()
plt.legend()
plt.show()

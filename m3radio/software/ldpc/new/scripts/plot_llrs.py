import numpy as np
import matplotlib.pyplot as plt

llrs = np.loadtxt("/tmp/ldpc.txt")
plt.plot(llrs)
plt.grid()
plt.xlabel("Iteration")
plt.ylabel("LLR APP")
plt.show()

import matplotlib.pyplot as plt

from results.throughput_highsnr import (
    ebn0_db, kbps_128_64, kbps_256_128, kbps_512_256,
    kbps_1280_1024, kbps_1536_1024, kbps_2048_1024)

plt.figure(figsize=(12, 8))

plt.plot(ebn0_db, kbps_128_64, '-x', label="(128, 64)")
plt.plot(ebn0_db, kbps_256_128, '-x', label="(256, 128)")
plt.plot(ebn0_db, kbps_512_256, '-x', label="(512, 256)")
plt.plot(ebn0_db, kbps_1280_1024, '-x', label="(1280, 1024)")
plt.plot(ebn0_db, kbps_1536_1024, '-x', label="(1536, 1024)")
plt.plot(ebn0_db, kbps_2048_1024, '-x', label="(2048, 1024)")

plt.legend(loc='upper left')

plt.xlabel("Eb/N0 (dB)")
plt.ylabel("Throughput (kbps)")

plt.title("LDPC Throughput Test")
plt.grid()

plt.savefig("ldpc_throughput_highsnr.pdf")

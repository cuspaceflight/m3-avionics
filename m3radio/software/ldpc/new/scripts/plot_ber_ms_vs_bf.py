import numpy as np
import matplotlib.pyplot as plt

from results.ber_256_128_minsum import (
    ebn0_db,
    soft_ber, soft_cer,
    hard_mp_ber, hard_mp_cer,
    hard_bf_ber, hard_bf_cer,
    uncoded_ber, uncoded_cer)

plt.figure(figsize=(12, 8))

plt.plot(ebn0_db, 1e-5 + np.array(soft_ber), 'g-x', label="MinSum, Soft Info (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_mp_ber), 'b-x', label="MinSum, Hard Info (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_bf_ber), 'r-x', label="BitFlipping, Hard Info (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(uncoded_ber), 'k-x', label="Uncoded (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(soft_cer), 'g--x', label="MinSum, Soft Info (CER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_mp_cer), 'b--x', label="MinSum, Hard Info (CER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_bf_cer), 'r--x', label="BitFlipping, Hard Info (CER)")
plt.plot(ebn0_db, 1e-5 + np.array(uncoded_cer), 'k--x', label="Uncoded (CER)")

plt.legend(loc='lower left')

plt.semilogy()

plt.xlabel("Eb/N0 (dB)")

plt.title("LDPC (256, 128) Benchmark")
plt.grid()

plt.ylim(1e-5, 1e0)

plt.savefig("ldpc_ms_vs_bf.pdf")

import numpy as np
import matplotlib.pyplot as plt

from results.ldpc_1280_1024_minsum_corrected import (
    ebn0_db, soft_ber, hard_ber_ber, hard_mp_ber, hard_bf_ber, uncoded_ber,
    soft_cer, hard_ber_cer, hard_mp_cer, hard_bf_cer, uncoded_cer, soft_ucer,
    hard_ber_ucer, hard_mp_ucer, hard_bf_ucer)

from results.ber_256_128_minsum import ebn0_db, soft_ber as soft_ber_ms, soft_cer as soft_cer_ms
from results.ber_256_128_bp import soft_ber as soft_ber_bp, soft_cer as soft_cer_bp

plt.figure(figsize=(12, 8))

plt.plot(ebn0_db, np.array(soft_ber_ms), 'g-x', label="Min-Sum (BER)")
plt.plot(ebn0_db, np.array(soft_ber_bp), 'b-x', label="Belief Propagation (BER)")

plt.plot(ebn0_db, np.array(soft_cer_ms), 'g--x', label="Min-Sum (CER)")
plt.plot(ebn0_db, np.array(soft_cer_bp), 'b--x', label="Belief Propagation (CER)")

plt.legend(loc='lower left')

plt.semilogy()

plt.xlabel("Eb/N0 (dB)")

plt.title("LDPC (256, 128) Benchmark")
plt.grid()

plt.ylim(1e-5, 1e0)

plt.savefig("ldpc_bp_vs_ms.pdf")

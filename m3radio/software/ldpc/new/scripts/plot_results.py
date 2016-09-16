import numpy as np
import matplotlib.pyplot as plt

from results.ldpc_1280_1024_minsum_corrected import (
    ebn0_db, soft_ber, hard_ber_ber, hard_mp_ber, hard_bf_ber, uncoded_ber,
    soft_cer, hard_ber_cer, hard_mp_cer, hard_bf_cer, uncoded_cer, soft_ucer,
    hard_ber_ucer, hard_mp_ucer, hard_bf_ucer)

plt.figure(figsize=(12, 8))

plt.plot(ebn0_db, 1e-5 + np.array(soft_ber), 'g-x', label="Soft MP (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_ber_ber), 'b-x', label="Hard MP w/ SNR (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_mp_ber), 'c-x', label="Hard MP (BER)")
#plt.plot(ebn0_db, 1e-5 + np.array(hard_bf_ber), 'r-x', label="Hard BF (BER)")
plt.plot(ebn0_db, 1e-5 + np.array(uncoded_ber), 'k-x', label="Uncoded (BER)")

plt.plot(ebn0_db, 1e-5 + np.array(soft_cer), 'g--o', label="Soft MP (CER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_ber_cer), 'b--o', label="Hard MP w/ SNR (CER)")
plt.plot(ebn0_db, 1e-5 + np.array(hard_mp_cer), 'c--o', label="Hard MP (CER)")
#plt.plot(ebn0_db, 1e-5 + np.array(hard_bf_cer), 'r--o', label="Hard BF (CER)")
plt.plot(ebn0_db, 1e-5 + np.array(uncoded_cer), 'k--o', label="Uncoded (CER)")

#plt.plot(ebn0_db, 1e-5 + np.array(soft_ucer), 'g-.s', label="Soft MP (UCER)")
#plt.plot(ebn0_db, 1e-5 + np.array(hard_ber_ucer), 'b-.s', label="Hard MP w/ SNR (UCER)")
#plt.plot(ebn0_db, 1e-5 + np.array(hard_mp_ucer), 'c-.s', label="Hard MP (UCER)")
#plt.plot(ebn0_db, 1e-5 + np.array(hard_bf_ucer), 'r-.s', label="Hard BF (UCER)")

plt.legend(loc='lower left')

plt.semilogy()

plt.xlabel("Eb/N0 (dB)")

plt.title("LDPC (1280, 1024) Benchmark")
plt.grid()

plt.ylim(1e-5, 1e0)

plt.savefig("ldpc_1280_1024_minsum_corrected.pdf")

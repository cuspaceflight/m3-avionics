import numpy as np
import sys
import multiprocessing
from queue import Empty
import usbcan

def main():
    port = sys.argv[1]
    txq = multiprocessing.Queue()
    rxq = multiprocessing.Queue()
    runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
    runner.start()

    print("Place m3fc flat and upright and press enter to continue")
    input()
    print("Sampling...")
    while not rxq.empty():
        try:
            rxq.get_nowait()
        except Empty:
            break
    samples_upright = []
    while len(samples_upright) < 5000:
        try:
            frame = rxq.get_nowait()
        except Empty:
            continue
        if frame.sid == 1 + (48 << 5):
            samples_upright.append(frame.as_int16()[:3])
            if len(samples_upright) % 1000 == 0:
                print("Got {}/5000 samples".format(len(samples_upright)))
    print("Place m3fc upside down and press enter to continue")
    input()
    print("Sampling...")
    while not rxq.empty():
        try:
            rxq.get_nowait()
        except Empty:
            break
    samples_upside = []
    while len(samples_upside) < 5000:
        try:
            frame = rxq.get_nowait()
        except Empty:
            continue
        if frame.sid == 1 + (48 << 5):
            samples_upside.append(frame.as_int16()[:3])
            if len(samples_upside) % 1000 == 0:
                print("Got {}/5000 samples".format(len(samples_upside)))
    runner.terminate()
    samples_upright = np.array(samples_upright)
    samples_upside = np.array(samples_upside)
    mean_upright = np.mean(samples_upright, axis=0)
    mean_upside = np.mean(samples_upside, axis=0)
    print("Means: Upright: {}, Upside down: {}".format(mean_upright, mean_upside))

    a = 0.5 * (mean_upright[2] - mean_upside[2])
    b = 0.5 * (mean_upright[2] + mean_upside[2])

    print("Z axis: sens={} offset={}".format(a, b))

if __name__ == "__main__":
    main()

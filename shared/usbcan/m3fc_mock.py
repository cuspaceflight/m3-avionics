import os
import glob
import time
import struct
import argparse
import multiprocessing

from usbcan import CANFrame, run

m3fc_id = 1
msg_id = lambda x: x << 5
m3fc_msg_mock_enable = m3fc_id | msg_id(5)
m3fc_msg_mock_accel = m3fc_id | msg_id(6)
m3fc_msg_mock_baro = m3fc_id | msg_id(7)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("accel_file")
    parser.add_argument("baro_file")
    parser.add_argument("--serial-port", help="path to serial port on m3debug",
                        default="/dev/serial/by-id/*m3debug*-if02")
    args = parser.parse_args()
    unglob = glob.glob(args.serial_port)
    if len(unglob) == 0:
        raise RuntimeError("No serial ports matching glob found")
    port = unglob[0]
    port = os.path.realpath(port)

    txq = multiprocessing.Queue()
    rxq = multiprocessing.Queue()

    runner = multiprocessing.Process(target=run, args=(port, txq, rxq))
    runner.start()

    accel_log = []
    with open(args.accel_file) as f:
        for line in f:
            ts, x, y, z = line.split(",")
            z = z.strip()
            ts = float(ts)
            x, y, z = int(x), int(y), int(z)
            accel_log.append((ts, x, y, z))

    baro_log = []
    with open(args.baro_file) as f:
        for line in f:
            ts, pressure, temperature = line.split(",")
            ts = float(ts)
            pressure = int(pressure)
            temperature = int(temperature.strip())
            baro_log.append((ts, pressure, temperature))

    print("Entering mock mode...")
    txq.put(CANFrame(sid=m3fc_msg_mock_enable, rtr=False, dlc=0, data=[]))

    accel_idx = baro_idx = 0
    real_t0 = time.time()
    log_t0 = min(accel_log[0][0], baro_log[0][0])

    while True:
        if accel_log[accel_idx][0] < baro_log[baro_idx][0]:
            accel = accel_log[accel_idx]
            while (time.time() - real_t0) < (accel[0] - log_t0):
                pass
            txq.put(CANFrame(sid=m3fc_msg_mock_accel, rtr=False, dlc=6,
                    data=[accel[1] & 0xFF, (accel[1] >> 8) & 0xFF,
                          accel[2] & 0xFF, (accel[2] >> 8) & 0xFF,
                          accel[3] & 0xFF, (accel[3] >> 8) & 0xFF]))
            print("{:08.3f}\r".format(accel[0]), end='')
            accel_idx += 1
        else:
            baro = baro_log[baro_idx]
            while (time.time() - real_t0) < (baro[0] - log_t0):
                pass
            txq.put(CANFrame(sid=m3fc_msg_mock_baro, rtr=False, dlc=8,
                    data=[(baro[1]) & 0xFF, (baro[1] >> 8) & 0xFF,
                          (baro[1] >> 16) & 0xFF, (baro[1] >> 24) & 0xFF,
                          (baro[2]) & 0xFF, (baro[2] >> 8) & 0xFF,
                          (baro[2] >> 16) & 0xFF, (baro[2] >> 24) & 0xFF]))
            print("{:08.3f}\r".format(baro[0]), end='')
            baro_idx += 1



if __name__ == "__main__":
    main()

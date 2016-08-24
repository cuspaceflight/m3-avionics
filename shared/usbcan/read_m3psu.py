import sys
import multiprocessing
from queue import Empty

import usbcan


m3psu_did = 0b00010
msg_id = lambda x: x << 5
m3psu_msg_toggle_pyros = m3psu_did | msg_id(16)
m3psu_msg_toggle_channel = m3psu_did | msg_id(17)
m3psu_msg_toggle_charger = m3psu_did | msg_id(18)
m3psu_msg_toggle_balance = m3psu_did | msg_id(19)
m3psu_msg_pyro_status = m3psu_did | msg_id(48)
m3psu_msg_channel_status_1_2 = m3psu_did | msg_id(49)
m3psu_msg_channel_status_3_4 = m3psu_did | msg_id(50)
m3psu_msg_channel_status_5_6 = m3psu_did | msg_id(51)
m3psu_msg_channel_status_7_8 = m3psu_did | msg_id(52)
m3psu_msg_channel_status_9_10 = m3psu_did | msg_id(53)
m3psu_msg_channel_status_11_12 = m3psu_did | msg_id(54)
m3psu_msg_charger_status = m3psu_did | msg_id(55)
m3psu_msg_battery_voltages = m3psu_did | msg_id(56)


port = sys.argv[1]
txq = multiprocessing.Queue()
rxq = multiprocessing.Queue()
runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
runner.start()


batt = [0, 0, False, False, False]
pyro = [0, 0, 0, "Disabled"]
channels = [[0,0,0] for i in range(12)]
charger = [0, False, False, False]

def channels_format():
    return "".join(["C %d:({:.2f}V {:.3f}A {:.2f}W)%s" % (i, "\n" if (i+1)%3==0 else "\t") for i in range(len(channels))])

def unpack_channels():
    return [j for c in channels for j in c]
    
def process_frame(frame):
    global batt, pyro, channels, charger
    did = frame.sid & 0b11111
    if did != m3psu_did:
        return
    if frame.sid == m3psu_msg_battery_voltages:
        batt[0] = (float(frame.data[0]*2)/100.0)
        batt[1] = (float(frame.data[1]*2)/100.0)
        batt[2] = (frame.data[2] & (1<<2)) != 0
        batt[3] = (frame.data[2] & (1<<1)) != 0
        batt[4] = (frame.data[2] & (1<<0)) != 0
    elif frame.sid == m3psu_msg_pyro_status:
        ints = frame.as_int16()
        pyro[0:2] = [ (float(b)/1000.0) for b in ints[0:2] ]
        pyro[2] = float(ints[2]) / 10.0
        pyro[3] = "Disabled" if ints[3] == 0 else "Enabled"
    elif (frame.sid >= m3psu_msg_channel_status_1_2
            and frame.sid <= m3psu_msg_channel_status_11_12):
        floats = list(map(float, frame.data))
        first_channel_id = (frame.sid >> 5) - (m3psu_msg_channel_status_1_2 >> 5)
        channels[first_channel_id] = [floats[0]*0.03, floats[1]*0.003, floats[2]*0.02]
        channels[first_channel_id + 1] = [floats[4]*0.03, floats[5]*0.003, floats[6]*0.02]
    elif frame.sid == m3psu_msg_charger_status:
        charger[0] = (frame.data[1] << 8) | frame.data[0]
        charger[1] = (frame.data[2] & (1<<2)) != 0
        charger[2] = (frame.data[2] & (1<<1)) != 0
        charger[3] = (frame.data[2] & (1<<0)) != 0
    else:
        print("Unknown SID %d" % frame.sid)

def inputtrigger(txq):
    while True:
        line = input("> ").strip().split(" ")
        cmd = line[0]
        if len(line) > 1:
            if cmd == "on":
                try:
                    chan = int(line[1])
                except:
                    continue
                packet = usbcan.CANFrame(m3psu_msg_toggle_channel, 0, 2, [1, chan])
                txq.put(packet)
            elif cmd == "off":
                try:
                    chan = int(line[1])
                except:
                    continue
                packet = usbcan.CANFrame(m3psu_msg_toggle_channel, 0, 2, [0, chan])
                txq.put(packet)
            elif cmd == "pyro":
                if line[1] == "enable":
                    packet = usbcan.CANFrame(m3psu_msg_toggle_pyros, 0, 1, [1])
                    txq.put(packet)
                elif line[1] == "disable":
                    packet = usbcan.CANFrame(m3psu_msg_toggle_pyros, 0, 1, [0])
                    txq.put(packet)
            elif cmd == "balance":
                if line[1] == "enable":
                    packet = usbcan.CANFrame(m3psu_msg_toggle_balance, 0, 1, [1])
                    txq.put(packet)
                elif line[1] == "disable":
                    packet = usbcan.CANFrame(m3psu_msg_toggle_balance, 0, 1, [0])
                    txq.put(packet)
            elif cmd == "charger":
                if line[1] == "enable":
                    packet = usbcan.CANFrame(m3psu_msg_toggle_charger, 0, 1, [1])
                    txq.put(packet)
                elif line[1] == "disable":
                    packet = usbcan.CANFrame(m3psu_msg_toggle_charger, 0, 1, [0])
                    txq.put(packet)

def mainloop(txq, rxq):
    count = 0
    while True:
        try:
            frame = rxq.get_nowait()
            process_frame(frame)
            
            count += 1
            if (count == 9):
                count = 0
                print("Batt: 1={:.2f}V 2={:.2f}V B?={:b} B1={:b} B2={:b}\t".format(*batt) +
                  "Pyro [{:s}]: V={:.4f} mA={:.3f} mW={:.1f}".format(pyro[3], pyro[0], pyro[1], pyro[2]) +
                  "\n" + channels_format().format(*unpack_channels()) +
                  "Charger: mA={:d} AC={:b} OC={:b} SC={:b}".format(*charger)
                  )
        except Empty:
            pass

runner = multiprocessing.Process(target=mainloop, args=(txq,rxq))
runner.start()

inputtrigger(txq)



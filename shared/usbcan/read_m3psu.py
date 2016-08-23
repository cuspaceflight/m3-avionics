import sys
import multiprocessing
from queue import Empty

import usbcan

port = sys.argv[1]
txq = multiprocessing.Queue()
rxq = multiprocessing.Queue()
runner = multiprocessing.Process(target=usbcan.run, args=(port, txq, rxq))
runner.start()


batt = [0, 0]
pyro = [0, 0, 0, "Disabled"]
channels = [[0,0,0] for i in range(12)]
charger = [0,False,False]

def channels_format():
    return "".join(["C %d:({:.2f}V {:.3f}A {:.2f}W)%s" % (i, "\n" if (i+1)%3==0 else "\t") for i in range(len(channels))])

def unpack_channels():
    return [j for c in channels for j in c]
    
def process_frame(frame):
    global batt, pyro, channels, charger
    pid = frame.sid & 0x3f
    did = frame.sid >> 6
    if did != 2:
        return
    if pid == 1:
        batt = [ (float(b*2)/100.0) for b in frame.data ]
    elif pid == 10:
        ints = frame.as_int16()
        pyro[0:2] = [ (float(b)/1000.0) for b in ints[0:2] ]
        pyro[2] = float(ints[2]) / 10.0
        pyro[3] = "Disabled" if ints[3] == 0 else "Enabled"
    elif pid >= 3 and pid <= 8:
        floats = list(map(float, frame.data))
        channels[(pid-3)*2] = [floats[0]*0.03, floats[1]*0.003, floats[2]*0.02]
        channels[((pid-3)*2) + 1] = [floats[4]*0.03, floats[5]*0.003, floats[6]*0.02]
    elif pid == 11:
        charger[0] = (frame.data[1] << 8) | frame.data[0]
        charger[1] = frame.data[2] == 1
        charger[2] = frame.data[3] == 1
    else:
        print("Unknown SID %d" % pid)

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
                packet = usbcan.CANFrame((2<<6)|9, 0, 2, [1, chan])
                txq.put(packet)
            elif cmd == "off":
                try:
                    chan = int(line[1])
                except:
                    continue
                packet = usbcan.CANFrame((2<<6)|9, 0, 2, [0, chan])
                txq.put(packet)
            elif cmd == "pyro":
                if line[1] == "enable":
                    packet = usbcan.CANFrame((2<<6)|2, 0, 1, [1])
                    txq.put(packet)
                elif line[1] == "disable":
                    packet = usbcan.CANFrame((2<<6)|2, 0, 1, [0])
                    txq.put(packet)

def mainloop(txq, rxq):
    #pyro_toggle = True
    count = 0
    while True:
        try:
            frame = rxq.get_nowait()
            process_frame(frame)
            
            count += 1
            if (count == 9):
            #    packet = usbcan.CANFrame((2<<6)|9, 0, 2, [1 if pyro_toggle else 0, 5])
            #    txq.put(packet)
            #    pyro_toggle = not pyro_toggle
                count = 0
                print("Batt: 1={:.2f}V 2={:.2f}V\t".format(batt[0], batt[1]) +
                  "Pyro [{:s}]: V={:.4f} mA={:.3f} mW={:.1f}".format(pyro[3], pyro[0], pyro[1], pyro[2]) +
                  "\n" + channels_format().format(*unpack_channels()) +
                  "Charger: mA={:d} AC={:b} OC={:b}".format(charger[0], charger[1], charger[2])
                  )
        except Empty:
            pass

runner = multiprocessing.Process(target=mainloop, args=(txq,rxq))
runner.start()

inputtrigger(txq)



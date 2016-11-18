from .usbcan import CANFrame
import time

def run(logfile, rxq):
    with open(logfile, "r") as f:
        for line in f:
            line = line.strip()
            parts = line.split(" ")
            parts[3:] = [" ".join(parts[3:])]
            parts = [x.split("=")[1] for x in parts]
          
          
            sid = int(parts[0], 2)
            rtr = int(parts[1])
            dlc = int(parts[2])
            data = [int(x, 16) for x in parts[3].split(" ")]
            
            
            packet = CANFrame(sid,rtr,dlc,data)
            
            rxq.put(packet)
            time.sleep(0.01)
            

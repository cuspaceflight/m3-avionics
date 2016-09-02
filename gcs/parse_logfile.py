import sys
from m3gcs.command_processor import find_processor

if len(sys.argv) != 2:
    print("Usage: {} <logfile.bin>".format(sys.argv[0]))
    sys.exit(1)

with open(sys.argv[1], 'rb') as f:
    while True:
        packet = f.read(16)
        if len(packet) != 16:
            break

        sid = packet[0] | (packet[1] << 8) # Sensor ID
        rtr = packet[2] # Data send or request
        dlc = packet[3] # Data length
        data = packet[4:12]
        timestamp = (packet[12] | (packet[13] << 8) | (packet[14] << 16) |
                    (packet[15] << 24)) # ten-thousandths of sec since bootup
        timestamp /= 10000.0 # seconds since bootup
        
        # See if we have a processor for this type of packet
        result = find_processor(sid)
        if result is None:
            print("No handler found for SID {:04x}".format(sid))
            continue
        
        parent, processor = result
        print("[{:010.4f} {}:{}] {}".format(timestamp, parent, processor[0],
                                            processor[1](data)))


#!/usr/bin/env python3

import sys
from m3gcs.usbcan import CANFrame
from m3gcs.command_processor import find_processor

if len(sys.argv) != 2:
    print("Usage: {} <logfile.bin>".format(sys.argv[0]))
    sys.exit(1)

with open(sys.argv[1], 'rb') as f:
    while True:
        packet = f.read(16)
        if len(packet) != 16:
            break

        frame = CANFrame.from_buf(packet[:12])

        # Systicks since datalogger startup, 1/10000 s
        timestamp = (packet[12] | (packet[13] << 8) | (packet[14] << 16) |
                     (packet[15] << 24))
        timestamp /= 10000.0

        # See if we have a processor for this type of packet
        result = find_processor(frame.sid)
        if result is None:
            message = "No handler for frame: " + str(frame)
            source = ""
        else:
            parent, (processor_name, processor_func) = result
            message = processor_func(frame.data)
            source = " {}: {}".format(parent, processor_name)

        string = "[{:010.4f}{}] {}".format(timestamp, source, message)
        print(string.replace("\n", " "))

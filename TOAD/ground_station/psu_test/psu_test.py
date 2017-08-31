#!/usr/bin/env python3

import sys
import struct

# Useage
if len(sys.argv) != 2:
    print("Usage: {} <logfile.bin>".format(sys.argv[0]))
    sys.exit(1)

# Open log file
with open(sys.argv[1], 'rb') as log:

    # Read in a log file
    data = log.read()
    
    # File pointer
    i = 0
    num_bytes = log.tell()
    
    print("TIME (S), VOLTAGE (V), CURRENT (mA)")
    time = 0
    
    # Loop until EOF
    while i in range(num_bytes):
        
        # Seek to next log
        log.seek(i)
        
        # Read log type
        type = log.read(1)            
                   
        # Handle PSU Message
        if type == b'\x02':
            data = log.read(7)
            psu = struct.unpack('<BHHBB', data)
            print(time, ",",(psu[2]/1000), ",", psu[1])           
            time += 5
                         
        # Increment file pointer
        i += 128
        

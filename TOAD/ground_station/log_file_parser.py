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
    print(data)
    
    # File pointer
    i = 0
    num_bytes = log.tell()
    
    # Loop until EOF
    while i in range(num_bytes):
        
        # Seek to next log
        log.seek(i)
        
        # Read log type
        type = log.read(1)    
        
        # Handle PVT Message
        if type == b'\x01':
            data = log.read(93)
            pvt = struct.unpack('<BIHBBBBBBIiBBBBiiiiIIiiiiiIIHHIiI', data)
            print(pvt, '\n')
            print("TOAD ID = ", pvt[0])
            print("i_tow = ", pvt[1])
            print("year = ", pvt[2])
            print("month = ", pvt[3])
            print("day = ", pvt[4])
            print("hour = ", pvt[5])
            print("minute = ", pvt[6])
            print("second = ", pvt[7])
            print("valid = ", pvt[8])
            print("t_acc = ", pvt[9])
            print("nano = ", pvt[10])
            print("fix_type = ", pvt[11])
            print("flags = ", pvt[12])
            print("num_sv = ", pvt[14])
            print("lon = ", (pvt[15]/10000000))
            print("lat = ", (pvt[16]/10000000))
            print("height = ", pvt[17])
            print("h_msl = ", pvt[18])
            print("h_acc = ", pvt[19])
            print("v_acc = ", pvt[20])
            print("velN = ", pvt[21])
            print("velE = ", pvt[22])
            print("velD = ", pvt[23])
            print("gspeed = ", pvt[24])
            print("head_mot = ", pvt[25])
            print("s_acc = ", pvt[26])
            print("head_acc = ", pvt[27])
            print("p_dop = ", pvt[28])
            print("head_veh = ", pvt[31])
            print('\n\n')
            
        # Handle PSU Message
        if type == b'\x02':
            data = log.read(7)
            psu = struct.unpack('<BHHBB', data)
            print(psu, '\n')
            print("TOAD ID = ", psu[0])
            print("battery voltage = ", psu[2])
            print("charging = ", psu[4])
            print("charge current = ", psu[1])
            print("charge temperature = ", psu[3])
            print('\n\n')
            
        # Handle Ranging Packet
        if type == b'\x04':
            data = log.read(12)
            ranging = struct.unpack('<BBIIH', data)
            print(ranging, '\n')
            print("TOAD ID = ", ranging[0])
            print("time of flight = ", ranging[2])
            print("i_tow = ", ranging[3])
            print ("battery voltage = ", ranging[4])
            print('\n\n')
            
        # Handle Position Packet
        if type == b'\x08':
            data = log.read(17)
            pos = struct.unpack('<BBiiiBH', data)
            print(pos, '\n')
            print("TOAD ID = ", pos[0])
            print("lon = ", (pos[2]/10000000))
            print("lat = ", (pos[3]/10000000))
            print("height = ", pos[4])  
            print("num sat = ", pos[5])
            print("battery voltage = ", pos[6])
            print('\n\n')
                              
        # Increment file pointer
        i += 128

#!/usr/bin/env python3

import sys
import struct

# Useage
if len(sys.argv) != 2:
    print("Usage: {} <logfile.bin>".format(sys.argv[0]))
    sys.exit(1)

# Message Type Definitions
MESSAGE_PVT         = 1
MESSAGE_PSU         = 2         
MESSAGE_RANGING     = 4     
MESSAGE_POSITION    = 8    
MESSAGE_TELEM_1     = 16     
MESSAGE_TELEM_2     = 32     
MESSAGE_PVT_CAPTURE = 64 
MESSAGE_LAB_STATS   = 128  
MESSAGE_BH_RANGE    =  17  
MESSAGE_BH_POS      = 34    

# Open log file
with open(sys.argv[1], 'rb') as log:

    # Read File
    log.read();

    # File pointer
    i = 0
    num_bytes = log.tell()
    
    # Loop until EOF
    while i in range(num_bytes):
        
        # Seek to next log
        log.seek(i)
        
        # Read Metadata
        header = log.read(6)    
        
        # Get Message Metadata
        meta_data = struct.unpack('<BBI', header)
        log_type = meta_data[0]
        toad_id = meta_data[1]
        systick = meta_data[2]
        systick /= 10000.0
        
        # Handle PVT Message
        if (log_type == MESSAGE_PVT):
                       
            payload = log.read(92)
            pvt = struct.unpack('<IHBBBBBBIiBBBBiiiiIIiiiiiIIHHIiI', payload)
            print("PVT MESSAGE:")
            print("TOAD ID = ", toad_id)
            print("Timestamp = ", systick, " s")
            print("Log Type = ", log_type)
            print("i_tow = ", pvt[0])
            print("year = ", pvt[1])
            print("month = ", pvt[2])
            print("day = ", pvt[3])
            print("hour = ", pvt[4])
            print("minute = ", pvt[5])
            print("second = ", pvt[6])
            print("valid = ", pvt[7])
            print("t_acc = ", pvt[8])
            print("nano = ", pvt[9])
            print("fix_type = ", pvt[10])
            print("flags = ", pvt[11])
            print("num_sv = ", pvt[13])
            print("lon = ", (pvt[14]/10000000), "degrees")
            print("lat = ", (pvt[15]/10000000), "degrees")
            print("height = ", (pvt[16]/1000), "m")
            print("h_msl = ", (pvt[17]/1000), "m")
            print("h_acc = ", pvt[18])
            print("v_acc = ", pvt[19])
            print("velN = ", pvt[20])
            print("velE = ", pvt[21])
            print("velD = ", pvt[22])
            print("gspeed = ", pvt[23])
            print("head_mot = ", pvt[24])
            print("s_acc = ", pvt[25])
            print("head_acc = ", pvt[26])
            print("p_dop = ", pvt[27])
            print("head_veh = ", pvt[30])
            print('\n\n')
        
        # Handle PSU Message
        if (log_type == MESSAGE_PSU):
            payload = log.read(7)
            psu = struct.unpack('<HHBBB', payload)
            print("PSU MESSAGE:")
            print("TOAD ID = ", toad_id)
            print("Timestamp = ", systick, " s")
            print("Log Type = ", log_type)
            print("battery voltage = ", (psu[1]/1000), "V")
            print("stm32 temp = ", psu[4], "degrees C")
            print("charging = ", psu[3])
            print("charge current = ", psu[0], "mA")
            print("charge temperature = ", psu[2], "degrees C")
            print('\n\n')
            
        # Handle Ranging Packet
        if (log_type == MESSAGE_RANGING):
            payload = log.read(11)
            ranging = struct.unpack('<BIIBB', payload)
            print("RANGING PACKET:")
            print("TOAD ID = ", toad_id)
            print("Timestamp = ", systick, " s")
            print("Log Type = ", log_type)
            print("time of flight = ", ranging[1])
            print("i_tow = ", ranging[2])
            print ("battery voltage = ", (ranging[3]/10), "V")
            print("stm32 temp = ", ranging[4], "degrees C")
            print('\n\n')
            
        # Handle Position Packet
        if (log_type == MESSAGE_POSITION):
            payload = log.read(16)
            pos = struct.unpack('<BiiiBBB', payload)
            print("POSITION PACKET:")
            print("TOAD ID = ", toad_id)
            print("Timestamp = ", systick, " s")
            print("Log Type = ", log_type)
            print("lon = ", (pos[1]/10000000), "degrees")
            print("lat = ", (pos[2]/10000000), "degrees")
            print("height = ", (pos[3]/1000), "m")  
            print("num sat = ", pos[4])
            print("battery voltage = ", (pos[5]/10), "V")
            print("stm32 temp = ", pos[6], "degrees C")
            print('\n\n')
                              
        # Increment file pointer
        i += 128

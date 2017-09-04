"""Handle incoming serial data (logging and parsing).
Gregory Brooks, Matt Coates 2017
"""
import serial
from multiprocessing import Pipe
import sys
import struct
from toad_packets import *

# Message Type Definitions
MESSAGE_PVT         = 1
MESSAGE_PSU         = 2
MESSAGE_RANGING     = 4
MESSAGE_POSITION    = 8
MESSAGE_RX_PACKET   = 51

# Packet Types
RANGE_PACKET = 64
POSITION_PACKET = 128


def run(gui_pipe, log_pipe):
    """
    Args:

    Returns:

    Raises:
    """
    # Open Serial Port
    ser = serial.Serial('/dev/ttyACM0')
    while True:

        # Read in a Log
        data = ser.read(128)

        # Get Message Log Type
        log_type = struct.unpack('<B', data[0])

        # Handle PVT Message
        if (log_type == MESSAGE_PVT):
            pvt_message = Pvt_packet(data)
            gui_pipe.send(pvt_message)

        # Handle PSU Message
        if (log_type == MESSAGE_PSU):
            psu_message = Psu_packet(data)
            gui_pipe.send(psu_message)

        # Handle Ranging Packet
        if (log_type == MESSAGE_RANGING):
            ranging_message = Ranging_packet(data)
            gui_pipe.send(ranging_message)

        # Handle Position Packet
        if (log_type == MESSAGE_POSITION):
            pos_message = Position_packet(data)
            gui_pipe.send(pos_message)

        # Handle SR Traffic - RX Packet Logged
        if (log_type == MESSAGE_RX_PACKET):
            ##### Uncomment to print things  #####
            # Get Message Metadata
            # meta_data = struct.unpack('<BI', data[1:6])
            #toad_id = meta_data[1]
            # systick = meta_data[2]
            #systick /= 10000.0

            #print("SR TRAFFIC [RX Packet]:")
            #print("TOAD ID = ", toad_id)
            #print("Timestamp = ", systick, " s")
            #######################################

            buf = b''
            buf += data[6]     # rx_type
            buf += data[1]     # will be replaced with id value from rx_type
            buf += data[2:5]   # systicks


            # Handle Packet Types
            if ((rx_type & RANGE_PACKET) == RANGE_PACKET):
                buf += data[7:17]  # payload
                sr_ranging_message = Ranging_packet(buf)
                gui_pipe.send(sr_ranging_message)

            if ((rx_type & POSITION_PACKET) == POSITION_PACKET):
                buf += data[7:22]  # payload
                sr_pos_message = Position_packet(buf)
                gui_pipe_send(sr_pos_message)

        #log raw serial data to file

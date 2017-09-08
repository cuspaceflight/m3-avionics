"""Place position packets into bins depending on itow value"""

from toad_packets import *
from math import floor

class Ref_point:
    def __init(self):
        self.latitude = 0  # degrees
        self.longitude = 0  # degrees
        self.height = 0  # height
        self.distance = 0  # distance

class Position_measurement:
    def __init__(self,itow_s):
        self.itow_s = itow_s
        self.flags = 0
        self.full = False
        self.toads = []
        for count in range (0,NUM_TOADS):
            toad = Ref_point()
            self.toads.append(toad)  # List of toads, containing position and tof

    def set_dist(self,id,dist):
        self.toads[id].distance = dist
        # Set flags
        self.flags = self.flags | 2**(id)
        self.check_full()

    def set_pos(self,id,lat,lon,h):
        self.toads[id].latitude = lat
        self.toads[id].longitude = lon
        self.toads[id].height = h
        # Set flags
        self.flags = self.flags | 2**(id+NUM_TOADS)
        self.check_full()

    def check_full(self):
        if self.flags == 0b111111111111:
            self.full = True

measurement_list = [] # newer bins -> higher index
MAX_BINS = 100  # Delete oldest unfilled bins once measurement_list gets too large

def add_packet(packet):
    # Delete any full bins
    no_full_bins = [x for x in measurement_list if not x.full]
    measurement_list = no_full_bins

    # If measurement_list is too large, delete oldest unfilled bin
    end = len(measurement_list)
    if end > MAX_BINS:
        measurement_list = measurement_list[end-100:end]

    # Process new packet
    itow_s = floor(packet.i_tow/1000)  # Second of the week the packet is timestamped with

    if packet.toad_id == TOAD_1:
        id = 0
    elif packet.toad_id == TOAD_2:
        id = 1
    elif packet.toad_id == TOAD_3:
        id = 2
    elif packet.toad_id == TOAD_4:
        id = 3
    elif packet.toad_id == TOAD_5:
        id = 4
    elif packet.toad_id == TOAD_6:
        id = 5

    for index,bin in enumerate(measurement_list):
        if bin.itow_s == itow_s:
            # Place new packet into existing bin
            found_bin = index
            break
        else:
            # Need to create new bin
            found_bin = MAX_BINS  # i.e. not indexes 0 to NUM_TOADS-1

    if found_bin == MAX_BINS:
        # Create new bin
        new_bin = Position_measurement(itow_s)
        measurement_list.append(new_bin)

    if packet.log_type == MESSAGE_POSITION:
        # Put position into found bin
        measurement_list[found_bin].set_pos(id,packet.lat,packet.lon,packet.height)

    elif packet.log_type == MESSAGE_RANGING:
        # Put distance into found bin
        measurement_list[found_bin].set_dist(id,packet.dist())

    # Check for full bin to return
    for index,bin in enumerate(measurement_list):
        if bin.full:
            full_bin = index
            break
        else:
            full_bin = 0

    if full_bin != 0:
        return measurement_list[full_bin]

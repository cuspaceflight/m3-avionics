"""Packet Definitions
Gregory Brooks, Matt Coates 2017"""

import struct

# Message Type Definitions
MESSAGE_PVT         = 1
MESSAGE_PSU         = 2
MESSAGE_RANGING     = 4
MESSAGE_POSITION    = 8
MESSAGE_RX_PACKET   = 51

# Packet Types
RANGE_PACKET = 64
POSITION_PACKET = 128

# TOAD ID Definitions
TOAD_1 = 1
TOAD_2 = 2
TOAD_3 = 4
TOAD_4 = 8
TOAD_5 = 16
TOAD_6 = 32

class Packet(object):
    """Base class"""
    def __init__(self, input_struct=bytes(128)):
        self.data_struct = input_struct

        # Get Message Metadata
        meta_data = struct.unpack('<BBI', self.data_struct[0:6])
        self.log_type = meta_data[0]
        self.toad_id = meta_data[1]
        self.systick = meta_data[2]  # systicks
        self.systick_freq = 10000  # Hz
        self.timestamp = self.systick / self.systick_freq  # s




class Pvt_packet(Packet):

    def __init__(self, input_struct=bytes(128)):

        Packet.__init__(self, input_struct)
        payload = self.data_struct[6:98]
        pvt = struct.unpack('<IHBBBBBBIiBBBBiiiiIIiiiiiIIHHIiI', payload)
        self.i_tow = pvt[0]
        self.year = pvt[1]
        self.month = pvt[2]
        self.day = pvt[3]
        self.hour = pvt[4]
        self.minute = pvt[5]
        self.second = pvt[6]
        self.valid = pvt[7]
        self.t_acc = pvt[8]
        self.nano = pvt[9]
        self.fix_type = pvt[10]
        self.flags = pvt[11]
        self.num_sv = pvt[13]
        self.lon = pvt[14]/10000000  # degrees
        self.lat = pvt[15]/10000000  # degrees
        self.height = pvt[16]/1000  # m
        self.h_msl = pvt[17]/1000  # m
        self.h_acc = pvt[18]
        self.v_acc = pvt[19]
        self.velN = pvt[20]
        self.velE = pvt[21]
        self.velD = pvt[22]
        self.gspeed = pvt[23]
        self.head_mot = pvt[24]
        self.s_acc = pvt[25]
        self.head_acc = pvt[26]
        self.p_dop = pvt[27]
        self.head_veh = pvt[30]

    def validString():
        if self.valid == 1:
            return 'validDate'
        elif self.valid == 2:
            return 'validTime'
        elif self.valid == 4:
            return 'fullyResolved'
        elif self.valid == 8:
            return 'validMag'
        else:
            return 'Error! Invalid value'

    def fixString():
        list = ['no_fix', 'dead_rkn_only', '2D', '3D', 'GNSS+dead_rkn', 'time_only']
        return list[self.fix_type]

    def printout(self):
        print("PVT MESSAGE:")
        print("TOAD ID = ", self.toad_id)
        print("Timestamp = ", self.timestamp, " s")
        print("Log Type = ", self.log_type)
        print("i_tow = ", self.i_tow)
        print("year = ", self.year)
        print("month = ", self.month)
        print("day = ", self.day)
        print("hour = ", self.hour)
        print("minute = ", self.minute)
        print("second = ", self.second)
        print("valid = ", self.valid)
        print("t_acc = ", self.t_acc)
        print("nano = ", self.nano)
        print("fix_type = ", self.fix_type)
        print("flags = ", self.flags)
        print("num_sv = ", self.num_sv)
        print("lon = ", self.lon, "degrees")
        print("lat = ", self.lat, "degrees")
        print("height = ", self.height, "m")
        print("h_msl = ", self.h_msl, "m")
        print("h_acc = ", self.h_acc)
        print("v_acc = ",self.v_acc)
        print("velN = ", self.velN)
        print("velE = ", self.velE)
        print("velD = ", self.velD)
        print("gspeed = ", self.gspeed)
        print("head_mot = ", self.head_mot)
        print("s_acc = ", self.s_acc)
        print("head_acc = ", self.head_acc)
        print("p_dop = ", self.p_dop)
        print("head_veh = ", self.head_veh)
        print('\n\n')

class Psu_packet(Packet):
    def __init__(self, input_struct=bytes(128)):
        Packet.__init__(self, input_struct)
        payload = self.data_struct[6:13]
        psu = struct.unpack('<HHBBB', payload)
        self.batt_v = psu[1]/1000  # V
        self.mcu_temp = psu[4]  # Celsius
        self.charging = psu[3]
        self.charge_current = psu[0]  # mA
        self.charge_temperature = psu[2]  # Celsius

    def printout(self):
        print("PSU MESSAGE:")
        print("TOAD ID = ", self.toad_id)
        print("Timestamp = ", self.timestamp, " s")
        print("Log Type = ", self.log_type)
        print("battery voltage = ", self.batt_v, "V")
        print("stm32 temp = ", self.mcu_temp, "degrees C")
        print("charging = ", self.charging)
        print("charge current = ", self.charge_current, "mA")
        print("charge temperature = ", self.charge_temperature, "degrees C")
        print('\n\n')

class Ranging_packet(Packet):
    def __init__(self, input_struct=bytes(128)):
        Packet.__init__(self, input_struct)
        payload = self.data_struct[6:17]
        ranging = struct.unpack('<BIIBB', payload)

        self.tof = ranging[1]
        self.i_tow = ranging[2]
        self.batt_v = ranging[3]/10  # V
        self.mcu_temp = ranging[4]  # Celsius
    def printout(self):
        print("RANGING PACKET:")
        print("TOAD ID = ", self.toad_id)
        print("Timestamp = ", self.timestamp, " s")
        print("Log Type = ", self.log_type)
        print("time of flight = ", self.tof)
        print("i_tow = ", self.i_tow)
        print ("battery voltage = ", self.batt_v, "V")
        print("stm32 temp = ", self.mcu_temp, "degrees C")
        print('\n\n')

class Position_packet(Packet):
    def __init__(self, input_struct=bytes(128)):
        Packet.__init__(self, input_struct)
        payload = self.data_struct[6:22]
        pos = struct.unpack('<BiiiBBB', payload)

        self.lon = pos[1]/10000000  # degrees
        self.lat = pos[2]/10000000  # degrees
        self.height = pos[3]/1000  # m
        self.num_sat = pos[4]
        self.batt_v = pos[5]/10  # V
        self.mcu_temp = pos[6]  # Celsius

    def printout(self):
        print("POSITION PACKET:")
        print("TOAD ID = ", self.toad_id)
        print("Timestamp = ", self.timestamp, " s")
        print("Log Type = ", self.log_type)
        print("lon = ", self.lon, "degrees")
        print("lat = ", self.lat, "degrees")
        print("height = ", self.height, "m")
        print("num sat = ",self.num_sat)
        print("battery voltage = ", self.batt_v, "V")
        print("stm32 temp = ", self.mcu_temp, "degrees C")
        print('\n\n')

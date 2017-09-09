"""Packet Definitions
Gregory Brooks, Matt Coates 2017"""

import struct
from PyQt4 import QtCore,QtGui

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

NUM_TOADS=6

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

    def printout(self,textbox):
        textbox.moveCursor(QtGui.QTextCursor.End)
        textbox.ensureCursorVisible()
        textbox.insertPlainText("Log type: {}\n".format(self.log_type))
        textbox.insertPlainText("Toad ID: {}\n".format(self.toad_id))
        textbox.insertPlainText("Systicks: {}\n".format(self.systick))



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

    def validString(self):
        if self.valid == 1:
            return 'validDate'
        elif self.valid == 2:
            return 'validTime'
        elif self.valid == 4:
            return 'fullyResolved'
        elif self.valid == 8:
            return 'validMag'
        else:
            return 'Invalid!'

    def fixString(self):
        list = ['no_fix', 'dead_rkn_only', '2D', '3D', 'GNSS+dead_rkn', 'time_only']
        if self.fix_type > 5 or self.fix_type < 0:
            return('Invalid!')
        else:
            return list[self.fix_type]

    def printout(self,textbox):
        textbox.moveCursor(QtGui.QTextCursor.End)
        textbox.ensureCursorVisible()
        textbox.insertPlainText("PVT MESSAGE:\n")
        textbox.insertPlainText("TOAD ID = {}\n".format(self.toad_id))
        textbox.insertPlainText("Timestamp = {} s\n".format(self.timestamp))
        textbox.insertPlainText("Log type = {}\n".format(self.log_type))
        textbox.insertPlainText("i_tow = {} ms\n".format(self.i_tow))
        textbox.insertPlainText("year = {}\n".format(self.year))
        textbox.insertPlainText("month = {}\n".format(self.month))
        textbox.insertPlainText("day = {}\n".format(self.day))
        textbox.insertPlainText("hour = {}\n".format(self.hour))
        textbox.insertPlainText("minute = {}\n".format(self.minute))
        textbox.insertPlainText("second = {}\n".format(self.second))
        textbox.insertPlainText("valid = {}\n".format(self.valid))
        textbox.insertPlainText("t_acc = {} ns\n".format(self.t_acc))
        textbox.insertPlainText("nano = {} ns\n".format(self.nano))
        textbox.insertPlainText("fix_type = {}\n".format(self.fix_type))
        textbox.insertPlainText("flags = {}\n".format(self.flags))
        textbox.insertPlainText("num_sv = {}\n".format(self.num_sv))
        textbox.insertPlainText("lat = {} degrees\n".format(self.lat))
        textbox.insertPlainText("lon = {} degrees\n".format(self.lon))
        textbox.insertPlainText("height = {} m\n".format(self.height))
        textbox.insertPlainText("h_msl = {} m\n".format(self.h_msl))
        textbox.insertPlainText("h_acc = {} mm\n".format(self.h_acc))
        textbox.insertPlainText("v_acc = {} mm\n".format(self.v_acc))
        textbox.insertPlainText("velN = {} mm/s\n".format(self.velN))
        textbox.insertPlainText("velE = {} mm/s\n".format(self.velE))
        textbox.insertPlainText("velD = {} mm/s\n".format(self.velD))
        textbox.insertPlainText("gspeed = {} mm/s\n".format(self.gspeed))
        textbox.insertPlainText("head_mot = {} degrees\n".format(self.head_mot))
        textbox.insertPlainText("s_acc = {} mm/s\n".format(self.s_acc))
        textbox.insertPlainText("head_acc = {} degrees\n".format(self.head_acc))
        textbox.insertPlainText("p_dop = {}\n".format(self.p_dop))
        textbox.insertPlainText("head_veh = {} degrees\n".format(self.head_veh))
        textbox.insertPlainText("\n\n")

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
        textbox.moveCursor(QtGui.QTextCursor.End)
        textbox.ensureCursorVisible()
        textbox.insertPlainText("PSU MESSAGE:\n")
        textbox.insertPlainText("TOAD ID = {}\n".format(self.toad_id))
        textbox.insertPlainText("Timestamp = {} s\n".format(self.timestamp))
        textbox.insertPlainText("Log type = {}\n".format(self.log_type))
        textbox.insertPlainText("battery voltage = {} V\n".format(self.batt_v))
        textbox.insertPlainText("stm32 temp = {} °C\n".format(self.mcu_temp))
        textbox.insertPlainText("charging = {}\n".format(self.charging))
        textbox.insertPlainText("charge current = {} mA\n".format(self.charge_current))
        textbox.insertPlainText("charge temperature = {} °C\n".format(self.charge_temperature))
        textbox.insertPlainText("\n\n")

class Ranging_packet(Packet):
    def __init__(self, input_struct=bytes(128)):
        Packet.__init__(self, input_struct)
        payload = self.data_struct[6:17]
        ranging = struct.unpack('<BIIBB', payload)

        self.tof = ranging[1]
        self.i_tow = ranging[2]
        self.batt_v = ranging[3]/10  # V
        self.mcu_temp = ranging[4]  # Celsius
    def dist(self,freq=84000000):
        return(299792458*self.tof/freq)  # speed*time
    def printout(self):
        textbox.moveCursor(QtGui.QTextCursor.End)
        textbox.ensureCursorVisible()
        textbox.insertPlainText("RANGING PACKET:\n")
        textbox.insertPlainText("TOAD ID = {}\n".format(self.toad_id))
        textbox.insertPlainText("Timestamp = {} s\n".format(self.timestamp))
        textbox.insertPlainText("Log Type = {}\n".format(self.log_type))
        textbox.insertPlainText("time of flight = {} (timer register)\n".format(self.tof))
        textbox.insertPlainText("i_tow = {} ms\n".format(self.i_tow))
        textbox.insertPlainText("battery voltage = {} V\n".format(self.batt_v))
        textbox.insertPlainText("stm32 temp = {} °C\n".format(self.mcu_temp))
        textbox.insertPlainText("\n\n")

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
        textbox.moveCursor(QtGui.QTextCursor.End)
        textbox.ensureCursorVisible()
        textbox.insertPlainText("POSITION PACKET:\n")
        textbox.insertPlainText("TOAD ID = {}\n".format(self.toad_id))
        textbox.insertPlainText("Timestamp = {} s\n".format(self.timestamp))
        textbox.insertPlainText("Log Type = {}\n".format(self.log_type))
        textbox.insertPlainText("lat = {} degrees\n".format(self.lat))
        textbox.insertPlainText("lon = {} degrees\n".format(self.lon))
        textbox.insertPlainText("height = {} m\n".format(self.height))
        textbox.insertPlainText("num sat = {}\n".format(self.num_sat))
        textbox.insertPlainText("battery voltage = {} V\n".format(self.batt_v))
        textbox.insertPlainText("stm32 temp = {} °C\n".format(self.mcu_temp))
        textbox.insertPlainText("\n\n")


### Internal to ground station ###
class Usb_command:
    def __init__(self,conn):
        self.conn = conn

class Position_fix:
    # Result from trilat algorithm
    def __init__(self,e,n,u,itow_s):
        self.e_coord = e
        self.n_coord = n
        self.u_coord = u
        self.itow_s = itow_s
# class Trilat_point:
#     def __init__(self,lat,lon,height,tof):
#         self.lat = lat
#         self.lon = lon
#         self.height = height
#         self.tof = tof

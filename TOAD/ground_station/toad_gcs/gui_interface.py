"""GUI Process
Gregory Brooks 2017
"""

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import QThread, SIGNAL
from multiprocessing import Pipe, Process
#from .toad_frontend import *
from .toad_frontend.toad_gui import Ui_toad_main_window
from .toad_packets import *
import sys

class MainThd(QThread):
    def __init__(self,trilat_pipe, usb_pipe):
        QThread.__init__(self)
        self.trilat_pipe = trilat_pipe
        self.usb_pipe = usb_pipe

    def __del__(self):
        self.wait()

    def run(self):
        while True:
            new_packet = self.usb_pipe.recv()
            self.emit(SIGNAL('new_packet(PyQt_PyObject)'),new_packet)


class gcs_main_window(QtGui.QMainWindow, Ui_toad_main_window):
    """Inherit main window generated in QT4 Designer"""
    def __init__(self, trilat_pipe, usb_pipe, parent=None):
        self.trilat_pipe = trilat_pipe
        self.usb_pipe = usb_pipe

        super().__init__(parent)
        self.setupUi(self)

        # Add slots and signals manually

        # Start update thread
        self.update_thread = MainThd(self.trilat_pipe, self.usb_pipe)
        self.connect(self.update_thread, SIGNAL("new_packet(PyQt_PyObject)"),self.new_packet)
        self.update_thread.start()

    def set_usb_id(self,frame,packet):
        if packet.toad_id == TOAD_1:
            id = "TOAD 1"
        elif packet.toad_id == TOAD_2:
            id = "TOAD 2"
        elif packet.toad_id == TOAD_3:
            id = "TOAD 3"
        elif packet.toad_id == TOAD_4:
            id = "TOAD 4"
        elif packet.toad_id == TOAD_5:
            id = "TOAD 5"
        elif packet.toad_id == TOAD_6:
            id = "TOAD 6"
        else:
            id = "M3Ground"

        frame.lineEdit_id.setText(id)

    def fill_fields_pvt(self,frame,packet):
        self.set_usb_id(frame,packet)
        frame.lineEdit_timestamp_pvt.setText(str(packet.timestamp))
        frame.lineEdit_timestamp_pvt.home(False)

        frame.LineEdit_itow_pvt.setText(str(packet.i_tow))
        frame.LineEdit_itow_pvt.home(False)

        frame.LineEdit_systicks_pvt.setText(str(packet.systick))
        frame.LineEdit_systicks_pvt.home(False)

        frame.LineEdit_fixtype.setText("{} ({})".format(packet.fixString(),packet.fix_type))
        frame.LineEdit_fixtype.home(False)

        frame.LineEdit_validity.setText("{} ({})".format(packet.validString(),packet.valid))
        frame.LineEdit_validity.home(False)

        #Set UTC time
        date_time_obj = QtCore.QDateTime(packet.year, packet.month, packet.day,
                                         packet.hour,packet.minute,packet.second)
        frame.DateTimeEdit.setDateTime(date_time_obj)

        frame.LineEdit_num_sv.setText(str(packet.num_sv))
        frame.LineEdit_num_sv.home(False)

    def fill_fields_psu(self,frame,packet):
        self.set_usb_id(frame,packet)
        frame.lineEdit_timestamp_psu.setText(str(packet.timestamp))
        frame.lineEdit_timestamp_psu.home(False)

        frame.lineEdit_systicks_psu.setText(str(packet.systick))
        frame.lineEdit_systicks_psu.home(False)

        frame.LineEdit_charging.setText(str(packet.charging))
        frame.LineEdit_charging.home(False)

        frame.LineEdit_charge_current.setText(str(packet.charge_current))
        frame.LineEdit_charge_current.home(False)

        frame.LineEdit_charge_temp.setText(str(packet.charge_temperature))
        frame.LineEdit_charge_temp.home(False)

    def fill_fields_ranging(self,frame, packet):
        frame.frame.lineEdit_timestamp_s_ranging.setText(str(packet.timestamp))
        frame.frame.lineEdit_timestamp_s_ranging.home(False)

        frame.frame.lineEdit_timestamp_itow_ranging.setText(str(packet.i_tow))
        frame.frame.lineEdit_timestamp_itow_ranging.home(False)

        frame.frame.lineEdit_timestamp_systicks_ranging.setText(str(packet.systick))
        frame.frame.lineEdit_timestamp_systicks_ranging.home(False)

        frame.frame.lineEdit_tof.setText(str(packet.tof))
        frame.frame.lineEdit_tof.home(False)

        freq = frame.frame.spinBox_timerfreq.value()
        dist = 299792458*packet.tof/freq  # speed*time
        frame.frame.lineEdit_distance.setText(str(dist))
        frame.frame.lineEdit_distance.home(False)

    def fill_fields_pos(self,frame,packet):
        frame.frame.lineEdit_timestamp_s_pos.setText(str(packet.timestamp))
        frame.frame.lineEdit_timestamp_s_pos.home(False)

        frame.frame.lineEdit_timestamp_systicks_pos.setText(str(packet.systick))
        frame.frame.lineEdit_timestamp_systicks_pos.home(False)

        frame.frame.lineEdit_lat.setText(str(packet.lat))
        frame.frame.lineEdit_lat.home(False)

        frame.frame.lineEdit_lon.setText(str(packet.lon))
        frame.frame.lineEdit_lon.home(False)

        frame.frame.lineEdit_height.setText(str(packet.height))
        frame.frame.lineEdit_height.home(False)

        frame.frame.lineEdit_num_sat.setText(str(packet.num_sat))
        frame.frame.lineEdit_num_sat.home(False)

        frame.frame.lineEdit_batt_v.setText(str(packet.batt_v))
        frame.frame.lineEdit_batt_v.home(False)

        frame.frame.lineEdit_mcu_temp.setText(str(packet.mcu_temp))
        frame.frame.lineEdit_mcu_temp.home(False)

    def new_packet(self, packet):
        if packet.toad_id == TOAD_1:
            toad_frame_x = self.frame_toad_1
        elif packet.toad_id == TOAD_2:
            toad_frame_x = self.frame_toad_2
        elif packet.toad_id == TOAD_3:
            toad_frame_x = self.frame_toad_3
        elif packet.toad_id == TOAD_4:
            toad_frame_x = self.frame_toad_4
        elif packet.toad_id == TOAD_5:
            toad_frame_x = self.frame_toad_5
        elif packet.toad_id == TOAD_6:
            toad_frame_x = self.frame_toad_6
        else:
            toad_frame_x = self.frame_toad_master

        if packet.log_type == MESSAGE_PVT:
            self.fill_fields_pvt(self.frame_toad_master, packet)
        elif packet.log_type == MESSAGE_PSU:
            self.fill_fields_psu(self.frame_toad_master, packet)
        elif packet.log_type == MESSAGE_RANGING:
            self.fill_fields_ranging(toad_frame_x, packet)
        elif packet.log_type == MESSAGE_POSITION:
            self.fill_fields_pos(toad_frame_x, packet)

def run(trilat_pipe, usb_pipe, gui_exit):
    app = QtGui.QApplication(sys.argv)
    toad_main_window = gcs_main_window(trilat_pipe, usb_pipe)
    toad_main_window.show()
    #sys.exit(app.exec_())
    app.exec_()
    gui_exit.set()

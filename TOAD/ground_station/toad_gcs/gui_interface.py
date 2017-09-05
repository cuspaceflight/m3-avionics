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

    def fill_fields_pvt(frame,packet):
        frame.lineEdit_timestamp_pvt.setText(str(packet.timestamp))
        frame.LineEdit_itow_pvt.setText(str(packet.itow))
        frame.LineEdit_systicks_pvt.setText(str(packet.systick))
        frame.LineEdit_fixtype.setText("{} ({})".format(packet.fixString(),packet.fix_type))

        #Set UTC time
        date_time_obj = QtCore.QDateTime(packet.Year, packet.Month, packet.Day,
                                         packet.hour,packet.minute,packet.second)
        frame.DateTimeEdit.setDateTime(date_time_obj)

        frame.LineEdit_num_sv.setText(str(packet.num_sv))

    def fill_fields_psu(frame,packet):
        frame.lineEdit_timestamp_psu.setText(str(packet.timestamp))
        frame.lineEdit_systicks_psu.setText(str(packet.systick))
        frame.lineEdit_charging.setText(str(packet.charging))
        frame.LineEdit_charge_current.setText(str(packet.charge_current))
        frame.LineEdit_charge_temp.setText(str(packet.charge_temperature))

    def fill_fields_ranging(frame, packet):
        frame.widget.lineEdit_timestamp_s_ranging.setText(str(packet.timestamp))
        frame.widget.lineEdit_timestamp_itow_ranging.setText(str(packet.i_tow))
        frame.widget.lineEdit_timestamp_systicks_ranging.setText(str(packet.systick))
        frame.widget.lineEdit_tof.setText(str(packet.tof))
        freq = frame.widget.spinBox_timerfreq.value()
        dist = 299792458*packet.tof/freq  # speed*time
        frame.widget.lineEdit_distance.setText(str(dist))

    def fill_fields_pos(frame,packet):
        frame.widget.lineEdit_timestamp_s_pos.setText(str(packet.timestamp))
        #frame.widget.lineEdit_timestamp_itow_pos.setText(str(packet.))

    def new_packet(self, packet):
        if new_packet.toad_id == TOAD_1:
            toad_frame_x = self.frame_toad_1
        elif new_packet.toad_id == TOAD_2:
            toad_frame_x = self.frame_toad_2
        elif new_packet.toad_id == TOAD_3:
            toad_frame_x = self.frame_toad_3
        elif new_packet.toad_id == TOAD_4:
            toad_frame_x = self.frame_toad_4
        elif new_packet.toad_id == TOAD_5:
            toad_frame_x = self.frame_toad_5
        else:
            toad_frame_x = self.frame_toad_master

        if new_packet.log_type == MESSAGE_PVT:
            fill_fields_pvt(toad_frame_x, new_packet)
        elif new_packet.log_type == MESSAGE_PSU:
            fill_fields_psu(toad_frame_x, new_packet)
        elif new.packet.log_type == MESSAGE_RANGING:
            fill_fields_ran(toad_frame_x, new_packet)
        elif new.packet.log_type == MESSAGE_POSITION:
            fill_fields_pos(toad_frame_x, new_packet)

def run(trilat_pipe, usb_pipe, gui_exit):
    app = QtGui.QApplication(sys.argv)
    toad_main_window = gcs_main_window(trilat_pipe, usb_pipe)
    toad_main_window.show()
    #sys.exit(app.exec_())
    app.exec_()
    gui_exit.set()

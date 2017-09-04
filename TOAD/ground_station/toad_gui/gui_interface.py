"""GUI Process
Gregory Brooks 2017
"""

from PyQt4 import QtCore, QtGui, QThread
from multiprocessing import Pipe, Process
from toad_frontend import *
from
import sys

class MainThd(QThread):
    def __init__(trilat_pipe, usb_pipe):
        QThread.__init__(self)
        self.trilat_pipe = trilat_pipe
        self.usb_pipe = usb_pipe

    def __del__(self):
        self.wait()

    def fill_fields_ranging(frame, packet):#needs args (which toad panel)
        #e.g. gcs_toad_main_window.frame_toad_1.widget.(field)
        frame.widget.dateTimeEdit_timestamp_ranging

    def run(self):
        while True:
            new_packet = usb_pipe.recv()
            if new_packet


class gcs_main_window(Ui_toad_main_window):
    """Inherit main window generated in QT4 Designer"""
    def __init__(trilat_pipe, usb_pipe, toad_main_window):
        self.trilat_pipe = trilat_pipe
        self.usb_pipe = usb_pipe
        super().setupUI(toad_main_window)

        # Add slots and signals manually


        QtCore.QMetaObject.connectSlotsByName(toad_main_window)


def run(trilat_pipe, usb_pipe):
    app = QtGui.QApplication(sys.argv)
    toad_main_window = QtGui.QMainWindow()
    ui = gui_mainwindow(trilat_pipe, usb_pipe, toad_main_window)
    #ui.setupUi(toad_main_window)
    toad_main_window.show()
    sys.exit(app.exec_())

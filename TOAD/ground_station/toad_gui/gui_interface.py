"""GUI Process
Gregory Brooks 2017
"""

from multiprocessing import Pipe, Process
from toad_frontend import *
import sys

def open_gui():
    # app = QtGui.QApplication(sys.argv)
    # MainWindow = QtGui.QMainWindow()
    # ui = Ui_MainWindow()
    # ui.setupUi(MainWindow)
    # MainWindow.show()
    # sys.exit(app.exec_())

usb_process = multiprocessing.Process(target=usb.run, args=(usb_gui_pipe, usb_log_pipe))
usb_process.start()

def run(trilat_pipe, usb_pipe):

    while True:
        # read commands from frontend
        # get flight stage from m3gcs (e.g. POWERED_ASCENT)
        # send instructions to backend processes

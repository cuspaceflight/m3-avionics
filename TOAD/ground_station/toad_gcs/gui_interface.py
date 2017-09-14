"""GUI Process
Gregory Brooks 2017
"""

from PyQt4 import QtCore, QtGui, QtWebKit
from PyQt4.QtCore import QThread, SIGNAL
from multiprocessing import Pipe, Process
#from .toad_frontend import *
from .toad_frontend.toad_gui import Ui_toad_main_window
from .toad_packets import *
from .coords import convert_ENU_to_llh
import sys
import os

script_dir = os.path.dirname(__file__)

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

class MainThd(QThread):
    def __init__(self, window_pipe, trilat_pipe, usb_pipe):
        QThread.__init__(self)
        self.window_pipe = window_pipe
        self.trilat_pipe = trilat_pipe
        self.usb_pipe = usb_pipe

    def __del__(self):
        self.wait()

    def run(self):
        while True:
            if self.usb_pipe.poll(0.02):
                new_packet_usb = self.usb_pipe.recv()
                if new_packet_usb.log_type == MESSAGE_RANGING or new_packet_usb.log_type == MESSAGE_POSITION:
                    # Send to trilateration thread
                    self.trilat_pipe.send(new_packet_usb)
                self.emit(SIGNAL('new_packet(PyQt_PyObject)'),new_packet_usb)

            if self.window_pipe.poll(0.02):
                new_packet_window = self.window_pipe.recv()

                if isinstance(new_packet_window,Usb_command):
                    self.usb_pipe.send(new_packet_window)


            if self.trilat_pipe.poll(0.02):
                trilat_rx_packet = self.trilat_pipe.recv()
                self.emit(SIGNAL('new_fix(PyQt_PyObject)'),trilat_rx_packet)


class gcs_main_window(QtGui.QMainWindow, Ui_toad_main_window):
    """Inherit main window generated in QT4 Designer"""
    def __init__(self, trilat_pipe, usb_pipe, parent=None):
        # Origin for ENU coordinates displayed on front panel
        self.origin = [0,0,0]  # e,n,u
        self.apogee = 0  # Record of apogee in m

        super().__init__(parent)
        self.setupUi(self)

        # Add map
        self.map_view = QtWebKit.QWebView()
        self.map_view.setObjectName(_fromUtf8("map_view"))

        self.map_view.page().mainFrame().addToJavaScriptWindowObject("TOAD Map", self)
        rel_path = 'leaflet_map/map.html'
        abs_file_path = os.path.abspath(os.path.join(script_dir,rel_path))
        self.map_view.load(QtCore.QUrl.fromLocalFile(abs_file_path))
        #self.map_view.loadFinished.connect(self.on_map_load)
        self.tabWidget.addTab(self.map_view,'Map')

        # Add slots and signals manually
        self.frame_toad_master.pushButton_conn.clicked.connect(self.toggle_con)
        self.pushButton_zero.clicked.connect(self.confirm_zero)
        self.actionSave_Terminal.triggered.connect(self.terminal_save)
        self.actionRefresh.triggered.connect(self.refresh_map)

        # Start update thread
        thread_end,self.gui_end = Pipe(duplex=False)  # So that QThread and gui don't use same pipe end at same time
        self.update_thread = MainThd(thread_end, trilat_pipe, usb_pipe)
        self.connect(self.update_thread, SIGNAL("new_packet(PyQt_PyObject)"),self.new_packet)
        self.connect(self.update_thread, SIGNAL("new_fix(PyQt_PyObject)"),self.trilat_rx)
        self.update_thread.start()

    def refresh_map(self):
        self.map_view.load(QtCore.QUrl.fromLocalFile(os.path.abspath(os.path.join(script_dir,'leaflet_map/map.html'))))

    def toggle_con(self):
        if self.frame_toad_master.pushButton_conn.isChecked():
            # Connect
            self.frame_toad_master.pushButton_conn.setText("Disconnect")
            self.gui_end.send(Usb_command(True))
        else:
            # Disconnect
            self.frame_toad_master.pushButton_conn.setText("Connect")
            self.gui_end.send(Usb_command(False))

    def confirm_zero(self):
        confirmation_box = QtGui.QMessageBox.question(self,'Confirm Origin Reset',
                                                        "Are you sure?\n\nConfirm the rocket is on the pad:",
                                                        QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
        if confirmation_box == QtGui.QMessageBox.Yes:
            self.set_new_origin()

    def set_new_origin(self, enu = None):
        if enu == None:
            self.origin[0] += float(self.dxLineEdit.text())
            self.origin[1] += float(self.dyLineEdit.text())
            self.origin[2] += float(self.dzLineEdit.text())
        else:
            self.origin = enu

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
        self.set_text(packet.timestamp,frame.lineEdit_timestamp_pvt)

        self.set_text(packet.i_tow,frame.LineEdit_itow_pvt)

        self.set_text(packet.systick,frame.LineEdit_systicks_pvt)

        self.set_text("{} ({})".format(packet.fixString(),packet.fix_type),frame.LineEdit_fixtype)

        self.set_text("{} ({})".format(packet.validString(),packet.valid),frame.LineEdit_validity)

        #Set UTC time
        date_time_obj = QtCore.QDateTime(packet.year, packet.month, packet.day,
                                         packet.hour,packet.minute,packet.second)
        frame.DateTimeEdit.setDateTime(date_time_obj)

        self.set_Text(packet.num_sv,frame.LineEdit_num_sv)

    def fill_fields_psu(self,frame,packet):
        self.set_usb_id(frame,packet)
        self.set_text(packet.timestamp,frame.lineEdit_timestamp_psu)

        self.set_text(packet.systick,frame.lineEdit_systicks_psu)

        self.set_text(packet.charging,frame.LineEdit_charging)

        self.set_text(packet.charge_current,frame.LineEdit_charge_current)

        self.set_text(packet.charge_temperature,frame.LineEdit_charge_temp)

    def fill_fields_ranging(self,frame, packet):
        self.set_text(packet.timestamp, frame.frame.lineEdit_timestamp_s_ranging)

        self.set_text(packet.i_tow,frame.frame.lineEdit_timestamp_itow_ranging)

        self.set_text(packet.systick,frame.frame.lineEdit_timestamp_systicks_ranging)

        self.set_text(packet.tof, frame.frame.lineEdit_tof)

        freq = frame.frame.spinBox_timerfreq.value()
        dist = packet.dist(freq)
        self.set_text(dist,frame.frame.lineEdit_distance)

    def fill_fields_pos(self,frame,packet):
        self.set_text(packet.timestamp, frame.frame.lineEdit_timestamp_s_pos)

        self.set_text(packet.systick,frame.frame.lineEdit_timestamp_systicks_pos)

        self.set_text(packet.lat,frame.frame.lineEdit_lat)

        self.set_text(packet.lon, frame.frame.lineEdit_lon)

        self.set_text(packet.height,frame.frame.lineEdit_height)

        self.set_text(packet.num_sat, frame.frame.lineEdit_num_sat)

        self.set_text(packet.batt_v, frame.frame.lineEdit_batt_v)

        self.set_text(packet.mcu_temp, frame.frame.lineEdit_mcu_temp)

    def new_packet(self, packet):
        # Print to terminal tab
        packet.printout(self.textBrowser_terminal)

        if packet.toad_id == TOAD_1:
            toad_frame_x = self.frame_toad_1
            id_no = 1
        elif packet.toad_id == TOAD_2:
            toad_frame_x = self.frame_toad_2
            id_no = 2
        elif packet.toad_id == TOAD_3:
            toad_frame_x = self.frame_toad_3
            id_no = 3
        elif packet.toad_id == TOAD_4:
            toad_frame_x = self.frame_toad_4
            id_no = 4
        elif packet.toad_id == TOAD_5:
            toad_frame_x = self.frame_toad_5
            id_no = 5
        elif packet.toad_id == TOAD_6:
            toad_frame_x = self.frame_toad_6
            id_no = 6
        else:
            toad_frame_x = self.frame_toad_master
            id_no = 0

        if packet.log_type == MESSAGE_PVT:
            self.fill_fields_pvt(self.frame_toad_master, packet)
        elif packet.log_type == MESSAGE_PSU:
            self.fill_fields_psu(self.frame_toad_master, packet)
        elif packet.log_type == MESSAGE_RANGING and id_no !=0:
            self.fill_fields_ranging(toad_frame_x, packet)
        elif packet.log_type == MESSAGE_POSITION:
            self.fill_fields_pos(toad_frame_x, packet)
            # Update map marker
            self.map_view.page().mainFrame().evaluateJavaScript("toad_marker_{}.setLatLng([{},{}]).update()".format(id_no,packet.lat,packet.lon))

    def trilat_rx(self, packet):
        if isinstance(packet,Position_fix):
            e_disp = packet.e_coord - self.origin[0]
            n_disp = packet.n_coord - self.origin[1]
            u_disp = packet.u_coord - self.origin[2]  # Altitude - pad altitude
            alt_disp = int(round(u_disp))
            self.altimeter.display(alt_disp)

            self.set_text(e_disp,self.dxLineEdit)
            self.set_text(n_disp,self.dyLineEdit)
            self.set_text(u_disp,self.dzLineEdit)

            llh = convert_ENU_to_llh([packet.e_coord, packet.n_coord, packet.u_coord])
            self.set_text(llh[0],self.latitudeLineEdit)
            self.set_text(llh[1],self.longitudeLineEdit)

            dx = packet.e_coord - self.trilat_rx_prev_packet.e_coord  # m
            dy = packet.n_coord - self.trilat_rx_prev_packet.n_coord  # m
            dz = packet.u_coord - self.trilat_rx_prev_packet.n_coord  # m
            dt = packet.itow_s - self.trilat_rx_prev_packet.itow_s    # s

            self.set_text(dx/dt,self.dxdtLineEdit)
            self.set_text(dy/dt,self.dydtLineEdit)
            self.set_text(dz/dt,self.dzdtLineEdit)

            speed = ( (dx/dt)**2 + (dy/dt)**2 + (dz/dt)**2 )**0.5
            self.set_text(speed,self.LineEdit_speed)

            if packet.u_coord > self.apogee:
                # New max alt
                self.apogee = packet.u_coord
                self.max_alt_value.display(int(round(self.apogee - self.origin[2])))

            self.trilat_rx_prev_packet = packet

            # Update map marker including u_disp in the bubble
            self.map_view.page().mainFrame().evaluateJavaScript("marker_dart.setLatLng([{},{}]).update()".format(llh[0],llh[1]))
            self.map_view.page().mainFrame().evaluateJavaScript("marker_dart.bindPopup(\"TOAD Dart (height above pad: {} m)\").openPopup();".format(u_disp))

    def set_text(self,text,lineedit):
        lineedit.setText(str(text))
        lineedit.home(False)  # Return cursor to start so most significant digits displayed

    def terminal_save(self):
        name = QtGui.QFileDialog.getSaveFileName(self,'Save File')
        file = open(name,'w')
        text = self.textBrowser_terminal.toPlainText()
        file.write(text)
        file.close()

def run(trilat_pipe, usb_pipe, gui_exit):
    app = QtGui.QApplication(sys.argv)
    toad_main_window = gcs_main_window(trilat_pipe, usb_pipe)
    toad_main_window.show()
    #sys.exit(app.exec_())
    app.exec_()
    gui_exit.set()

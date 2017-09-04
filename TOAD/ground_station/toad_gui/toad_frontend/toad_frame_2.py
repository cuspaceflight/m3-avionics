# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ui/toad_frame_2.ui'
#
# Created by: PyQt4 UI code generator 4.11.4
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_toad_frame_2(object):
    def setupUi(self, toad_frame_2):
        toad_frame_2.setObjectName(_fromUtf8("toad_frame_2"))
        toad_frame_2.resize(400, 300)
        toad_frame_2.setStyleSheet(_fromUtf8("background-color: rgb(65, 212, 102);"))
        toad_frame_2.setFrameShape(QtGui.QFrame.StyledPanel)
        toad_frame_2.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(toad_frame_2)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.label = QtGui.QLabel(toad_frame_2)
        font = QtGui.QFont()
        font.setPointSize(14)
        font.setBold(True)
        font.setWeight(75)
        self.label.setFont(font)
        self.label.setObjectName(_fromUtf8("label"))
        self.verticalLayout.addWidget(self.label)
        self.widget = toad_frame(toad_frame_2)
        self.widget.setAutoFillBackground(False)
        self.widget.setStyleSheet(_fromUtf8("background-color: rgb(255, 255, 255);"))
        self.widget.setObjectName(_fromUtf8("widget"))
        self.verticalLayout.addWidget(self.widget)

        self.retranslateUi(toad_frame_2)
        QtCore.QMetaObject.connectSlotsByName(toad_frame_2)

    def retranslateUi(self, toad_frame_2):
        toad_frame_2.setWindowTitle(_translate("toad_frame_2", "Frame", None))
        self.label.setText(_translate("toad_frame_2", "TOAD 2", None))

from toad_frame import toad_frame

class toad_frame_2(QtGui.QFrame, Ui_toad_frame_2):
    def __init__(self, parent=None, f=QtCore.Qt.WindowFlags()):
        QtGui.QFrame.__init__(self, parent, f)

        self.setupUi(self)


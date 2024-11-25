from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QCheckBox, QRadioButton, QFileDialog
from SimosBIN import SimosBIN
from Return import ReturnType
import sys


class TABCal(QWidget):
    def __init__(self, parent: QWidget):
        super().__init__(parent)
        self.parent = parent

        layoutBoxMode = QHBoxLayout()
        self.radioButtonImport = QRadioButton("Import")
        self.radioButtonExport = QRadioButton("Export")
        layoutBoxMode.addWidget(self.radioButtonImport)
        layoutBoxMode.addWidget(self.radioButtonExport)
        self.radioButtonImport.setChecked(True)

        self.checkBoxmatchBoxCode = QCheckBox("Require matching boxcode")
        layoutBoxMode.addWidget(self.checkBoxmatchBoxCode)
        self.checkBoxmatchBoxCode.setCheckState(Qt.CheckState.Checked)
        
        layoutBoxButton = QHBoxLayout()
        pushButtonCal = QPushButton("CAL")
        pushButtonCal.setFixedHeight(100)
        pushButtonCal.pressed.connect(self.CALButtonClick)
        layoutBoxButton.addWidget(pushButtonCal)

        layoutBoxMain = QVBoxLayout()
        layoutBoxMain.addLayout(layoutBoxMode)
        layoutBoxMain.addLayout(layoutBoxButton)
        self.setLayout(layoutBoxMain)


    def CALButtonClick(self):
        if self.radioButtonImport.isChecked():
            self.CALImport()
        else:
            self.CALExport()


    def CALImport(self):
        self.parent.addLogEntry("------------------------")
        self.parent.addLogEntry("- Starting CAL import  -")
        self.parent.addLogEntry("------------------------")

        fullFileName = QFileDialog.getOpenFileName(self, "Open FULL bin", "", "Bins (*.bin)", )
        if fullFileName[0] == "":
            self.parent.addLogEntry("User did not select a full bin")
            return

        self.parent.addLogEntry("Full BIN [" + fullFileName[0] + "]")
        fullBin = SimosBIN()
        fullBin.load(fullFileName[0])
        fullHWKey, fullHWValue = fullBin.hardwareType()

        calFileName = QFileDialog.getOpenFileName(self, "Open CALL bin", "", "Bins (*.bin)", )
        if calFileName[0] == "":
            self.parent.addLogEntry("User did not select a cal bin")
            return

        if fullHWKey is None:
            self.parent.addLogEntry("Full BIN - invalid hardware type")
            return

        self.parent.addLogEntry("Cal BIN [" + calFileName[0] + "]")
        calBin = SimosBIN()
        calBin.load(calFileName[0])
        calHWKey, calHWValue = calBin.hardwareType()
        if calHWValue is None:
            self.parent.addLogEntry("Cal BIN - invalid hardware type")
            return

        if calHWKey != fullHWKey:
            self.parent.addLogEntry("Hardware types do not match  [" + calHWKey + " : " + fullHWKey + "]")
            return

        if self.checkBoxmatchBoxCode.checkState() == Qt.CheckState.Checked and calBin.boxCode() != fullBin.boxCode():
            self.parent.addLogEntry("Boxcodes do not match  [" + calBin.boxCode() + " : " + fullBin.boxCode() + "]")
            return

        outputFileName = QFileDialog.getSaveFileName(self, "Save OUT bin", "", "Bins (*.bin)", )
        if outputFileName[0] == "":
            self.parent.addLogEntry("User did not select an output bin")
            return

        self.parent.addLogEntry("Output BIN [" + outputFileName[0] + "]")
        outputBin = SimosBIN()
        outputBin.copy(fullBin)
        ret = outputBin.swapBlock(calBin, 4)
        if ret != ReturnType.OK:
            self.parent.addLogEntry("Error importing cal [" + ret.string() + "]")
            return

        outputBin.save(outputFileName[0])
        self.parent.addLogEntry("Cal swap successfully wrote [" + outputFileName[0] + "]")


    def CALExport(self):
        self.parent.addLogEntry("------------------------")
        self.parent.addLogEntry("- Starting CAL export  -")
        self.parent.addLogEntry("------------------------")

        fullFileName = QFileDialog.getOpenFileName(self, "Open FULL bin", "", "Bins (*.bin)", )
        if fullFileName[0] == "":
            self.parent.addLogEntry("User did not select a full bin")
            return

        self.parent.addLogEntry("Full BIN [" + fullFileName[0] + "]")
        fullBin = SimosBIN()
        fullBin.load(fullFileName[0])
        fullHWKey, fullHWValue = fullBin.hardwareType()

        outputFileName = QFileDialog.getSaveFileName(self, "Save OUT bin", "", "Bins (*.bin)", )
        if outputFileName[0] == "":
            self.parent.addLogEntry("User did not select an output bin")
            return

        self.parent.addLogEntry("Output BIN [" + outputFileName[0] + "]")
        outputBin = SimosBIN()
        outputBin.copy(fullBin, 4)

        outputBin.save(outputFileName[0])
        self.parent.addLogEntry("Cal export successfully wrote [" + outputFileName[0] + "]")
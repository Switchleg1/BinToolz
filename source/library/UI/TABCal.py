from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QCheckBox, QRadioButton, QFileDialog
from library.Functions import calImport, calExport


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


    # Gather the bins via dialogs, then hand off to the universal function with
    # addLogEntry as the log sink. All processing lives in library.Functions.
    def CALImport(self):
        log = self.parent.addLogEntry

        fullFileName = QFileDialog.getOpenFileName(self, "Open FULL bin", "", "Bins (*.bin)", )
        if fullFileName[0] == "":
            log("User did not select a full bin")
            return

        calFileName = QFileDialog.getOpenFileName(self, "Open CALL bin", "", "Bins (*.bin)", )
        if calFileName[0] == "":
            log("User did not select a cal bin")
            return

        outputFileName = QFileDialog.getSaveFileName(self, "Save OUT bin", "", "Bins (*.bin)", )
        if outputFileName[0] == "":
            log("User did not select an output bin")
            return

        matchBoxCode = self.checkBoxmatchBoxCode.checkState() == Qt.CheckState.Checked
        calImport(fullFileName[0], calFileName[0], outputFileName[0], matchBoxCode, log)


    def CALExport(self):
        log = self.parent.addLogEntry

        fullFileName = QFileDialog.getOpenFileName(self, "Open FULL bin", "", "Bins (*.bin)", )
        if fullFileName[0] == "":
            log("User did not select a full bin")
            return

        outputFileName = QFileDialog.getSaveFileName(self, "Save OUT bin", "", "Bins (*.bin)", )
        if outputFileName[0] == "":
            log("User did not select an output bin")
            return

        calExport(fullFileName[0], outputFileName[0], log)

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QGroupBox, QRadioButton, QFileDialog
from library.Functions import FunctionType, DataMode, patchApply, patchCreate


class TABPatch(QWidget):
    def __init__(self, parent: QWidget):
        super().__init__(parent)
        self.parent = parent

        layoutBoxMode = QHBoxLayout()
        self.radioButtonModeAdd = QRadioButton("Add")
        self.radioButtonModeRemove = QRadioButton("Remove")
        self.radioButtonModeCreate = QRadioButton("Create")
        self.radioButtonModeCheck = QRadioButton("Check")
        layoutBoxMode.addWidget(self.radioButtonModeAdd)
        layoutBoxMode.addWidget(self.radioButtonModeRemove)
        layoutBoxMode.addWidget(self.radioButtonModeCreate)
        layoutBoxMode.addWidget(self.radioButtonModeCheck)
        self.radioButtonModeAdd.setChecked(True)

        layoutBoxData = QHBoxLayout()
        self.radioButtonDataNormal = QRadioButton("Normal")
        self.radioButtonDataIgnore = QRadioButton("Ignore")
        self.radioButtonDataForce = QRadioButton("Force")
        layoutBoxData.addWidget(self.radioButtonDataNormal)
        layoutBoxData.addWidget(self.radioButtonDataIgnore)
        layoutBoxData.addWidget(self.radioButtonDataForce)
        self.radioButtonDataNormal.setChecked(True)
        groupBoxData = QGroupBox("How to treat Cal data")
        groupBoxData.setLayout(layoutBoxData)
        layoutBoxMode.addWidget(groupBoxData)

        layoutBoxButton = QHBoxLayout()
        pushButtonPatch = QPushButton("PATCH")
        pushButtonPatch.setFixedHeight(100)
        pushButtonPatch.pressed.connect(self.PATCHButtonClick)
        layoutBoxButton.addWidget(pushButtonPatch)

        layoutBoxMain = QVBoxLayout()
        layoutBoxMain.addLayout(layoutBoxMode)
        layoutBoxMain.addLayout(layoutBoxButton)

        self.setLayout(layoutBoxMain)


    # translate the cal-data radio buttons into a DataMode for the functions
    def dataMode(self):
        if self.radioButtonDataIgnore.isChecked():
            return DataMode.IGNORE
        if self.radioButtonDataForce.isChecked():
            return DataMode.FORCE
        return DataMode.NORMAL


    def PATCHButtonClick(self):
        if self.radioButtonModeAdd.isChecked():
            self.PATCHApply(FunctionType.FUNC_ADD)

        elif self.radioButtonModeRemove.isChecked():
            self.PATCHApply(FunctionType.FUNC_REMOVE)

        elif self.radioButtonModeCheck.isChecked():
            self.PATCHApply(FunctionType.FUNC_CHECK)

        elif self.radioButtonModeCreate.isChecked():
            self.PATCHCreate()


    # Collect the input bin, patch list, and (for add/remove) the output bin via
    # dialogs, then delegate add/remove/check to the universal patchApply.
    def PATCHApply(self, type):
        log = self.parent.addLogEntry

        binFileName = QFileDialog.getOpenFileName(self, "Open bin", "", "Bins (*.bin)", )
        if binFileName[0] == "":
            log("User did not select an input bin")
            return

        patchFileName = QFileDialog.getOpenFileNames(self, "Open Patch", "", "BinToolz Patch (*.btp)", )
        if len(patchFileName[0]) == 0:
            log("User did not select a patch")
            return

        outputFileName = ""
        if type != FunctionType.FUNC_CHECK:
            saveFileName = QFileDialog.getSaveFileName(self, "Save OUT bin", "", "Bins (*.bin)", )
            if saveFileName[0] == "":
                log("User did not select an output bin")
                return
            outputFileName = saveFileName[0]

        patchApply(type, binFileName[0], patchFileName[0], outputFileName, self.dataMode(), log)


    def PATCHCreate(self):
        log = self.parent.addLogEntry

        originalFileName = QFileDialog.getOpenFileName(self, "Open STARTING bin", "", "Bins (*.bin)", )
        if originalFileName[0] == "":
            log("User did not select an original bin")
            return

        modifiedFileName = QFileDialog.getOpenFileName(self, "Open MODIFIED bin", "", "Bins (*.bin)", )
        if modifiedFileName[0] == "":
            log("User did not select a modified bin")
            return

        outputFileName = QFileDialog.getSaveFileName(self, "Save BTP", "", "BinToolz Patch (*.btp)", )
        if outputFileName[0] == "":
            log("User did not select an output btp file")
            return

        patchCreate(originalFileName[0], modifiedFileName[0], outputFileName[0], self.dataMode(), log)

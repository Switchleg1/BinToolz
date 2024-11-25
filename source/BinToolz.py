from PyQt6.QtWidgets import QMainWindow, QApplication, QWidget, QVBoxLayout, QListWidget, QTabWidget
from TABCal import TABCal
from TABPatch import TABPatch
import sys


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("BinToolz v2.0.0")

        #tabs
        tabs = QTabWidget()
        tabs.addTab(TABCal(self), "CAL")
        tabs.addTab(TABPatch(self), "Patches")

        #log box
        self.listViewLog = QListWidget()
        layoutLog = QVBoxLayout()
        layoutLog.addWidget(self.listViewLog)

        layoutBoxAll = QVBoxLayout()
        layoutBoxAll.addWidget(tabs)
        layoutBoxAll.addLayout(layoutLog)

        widget = QWidget()
        widget.setLayout(layoutBoxAll)
        self.setGeometry(300, 300, 650, 600)
        self.setCentralWidget(widget)
        self.show()


    def addLogEntry(self, entry):
        self.listViewLog.addItem(entry)
        self.listViewLog.scrollToBottom()


# Main
app = QApplication(sys.argv)
w = MainWindow()
app.exec()
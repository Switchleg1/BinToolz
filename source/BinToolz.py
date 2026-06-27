from PyQt6.QtWidgets import QMainWindow, QApplication, QWidget, QVBoxLayout, QListWidget, QTabWidget
from library.UI import TABCal, TABPatch
from library.Functions import FunctionType, DataMode, calImport, calExport, patchApply, patchCreate
from Return import ReturnType
import sys


# ---------------------------------------------------------------------------
# CLI
#
# Each handler receives the argument list that follows the command keyword and
# a `log` callback (print, when invoked from the command line). They share the
# exact same library.Functions calls the GUI uses, so behaviour is identical.
# ---------------------------------------------------------------------------

# pull data-mode flags out of an argument list, returning (DataMode, remaining)
def cliDataMode(args):
    mode = DataMode.NORMAL
    rest = []
    for a in args:
        low = a.lower()
        if low in ("--normal", "-n"):
            mode = DataMode.NORMAL
        elif low in ("--ignore", "-i"):
            mode = DataMode.IGNORE
        elif low in ("--force", "-f"):
            mode = DataMode.FORCE
        else:
            rest.append(a)
    return mode, rest


def cliImport(args, log):
    matchBoxCode = True
    files = []
    for a in args:
        if a.lower() in ("--no-boxcode", "--nobox"):
            matchBoxCode = False
        else:
            files.append(a)

    if len(files) != 3:
        log("Usage: IMPORT <full.bin> <cal.bin> <output.bin> [--no-boxcode]")
        return ReturnType.INVALID_PARAM

    return calImport(files[0], files[1], files[2], matchBoxCode, log)


def cliExport(args, log):
    if len(args) != 2:
        log("Usage: EXPORT <full.bin> <output.bin>")
        return ReturnType.INVALID_PARAM

    return calExport(args[0], args[1], log)


def cliCreate(args, log):
    mode, files = cliDataMode(args)
    if len(files) != 3:
        log("Usage: CREATE <original.bin> <modified.bin> <output.btp> [--ignore|--force]")
        return ReturnType.INVALID_PARAM

    return patchCreate(files[0], files[1], files[2], mode, log)


# shared by ADD and REMOVE: <input.bin> <output.bin> <patch.btp> [more patches...]
def cliPatchApply(type, verb, args, log):
    mode, files = cliDataMode(args)
    if len(files) < 3:
        log("Usage: " + verb + " <input.bin> <output.bin> <patch.btp> [patch2.btp ...] [--ignore|--force]")
        return ReturnType.INVALID_PARAM

    return patchApply(type, files[0], files[2:], files[1], mode, log)


def cliAdd(args, log):
    return cliPatchApply(FunctionType.FUNC_ADD, "ADD", args, log)


def cliRemove(args, log):
    return cliPatchApply(FunctionType.FUNC_REMOVE, "REMOVE", args, log)


def cliCheck(args, log):
    mode, files = cliDataMode(args)
    if len(files) < 2:
        log("Usage: CHECK <input.bin> <patch.btp> [patch2.btp ...] [--ignore|--force]")
        return ReturnType.INVALID_PARAM

    return patchApply(FunctionType.FUNC_CHECK, files[0], files[1:], "", mode, log)


CLI_COMMAND = {
    'ADD'    : cliAdd,
    'REMOVE' : cliRemove,
    'CREATE' : cliCreate,
    'CHECK'  : cliCheck,
    'IMPORT' : cliImport,
    'EXPORT' : cliExport,
}


def do_cli(argv):
    command = argv[1].upper()
    handler = CLI_COMMAND.get(command)
    if handler is None:
        print("Unknown command [" + argv[1] + "]")
        print("Commands: " + ", ".join(CLI_COMMAND.keys()))
        return 1

    ret = handler(argv[2:], print)
    return 0 if ret == ReturnType.OK else ret.int()


# ---------------------------------------------------------------------------
# GUI
# ---------------------------------------------------------------------------

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("BinToolz v2.1.0")

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


# Main: a command keyword runs headless; no arguments launches the GUI.
if __name__ == "__main__":
    if len(sys.argv) > 1:
        sys.exit(do_cli(sys.argv))
    else:
        app = QApplication(sys.argv)
        w = MainWindow()
        app.exec()

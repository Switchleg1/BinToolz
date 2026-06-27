from PyQt6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QGroupBox, QRadioButton, QFileDialog
from library.SimosBIN import SimosBIN
from Return import ReturnType
from library.BTP import BTP, BTP_VERSION
from enum import Enum


class FunctionType(Enum):
    FUNC_CHECK = 1
    FUNC_ADD = 2
    FUNC_REMOVE = 3


def PATCHCreate(self, log, original_filename, modified_filename, ):
    log("------------------------")
    log("- Starting Patch Create-")
    log("------------------------")

    originalFileName = QFileDialog.getOpenFileName(self, "Open STARTING bin", "", "Bins (*.bin)", )
    if originalFileName[0] == "":
        self.parent.addLogEntry("User did not select an original bin")
        return

    self.parent.addLogEntry("Original BIN [" + originalFileName[0] + "]")
    originalBin = SimosBIN()
    originalBin.load(originalFileName[0])
    originalHWKey, originalHWValue = originalBin.hardwareType()

    if originalHWValue is None:
        self.parent.addLogEntry("Original BIN - invalid hardware type")
        return

    modifiedFileName = QFileDialog.getOpenFileName(self, "Open MODIFIED bin", "", "Bins (*.bin)", )
    if modifiedFileName[0] == "":
        self.parent.addLogEntry("User did not select a modified bin")
        return

    self.parent.addLogEntry("Modified BIN [" + modifiedFileName[0] + "]")
    modifiedBin = SimosBIN()
    modifiedBin.load(modifiedFileName[0])
    modifiedHWKey, modifiedHWValue = modifiedBin.hardwareType()
    if modifiedHWValue is None:
        self.parent.addLogEntry("Modified BIN - invalid hardware type")
        return

    if modifiedHWKey != originalHWKey:
        self.parent.addLogEntry("Hardware types do not match  [" + modifiedHWKey + " : " + originalHWKey + "]")
        return

    patch = BTP()
    ret = patch.createPatch(originalBin, modifiedBin, self.radioButtonDataIgnore.isChecked() == False)
    if ret != ReturnType.OK:
        self.parent.addLogEntry("Unable to create patch [" + ret.string() + "]")
        return

    outputFileName = QFileDialog.getSaveFileName(self, "Save BTP", "", "BinToolz Patch (*.btp)", )
    if outputFileName[0] == "":
        self.parent.addLogEntry("User did not select an output btp file")
        return

    ret = patch.save(outputFileName[0])
    if ret != ReturnType.OK:
        self.parent.addLogEntry("Unable to write patch [" + ret.string() + "]")
        return

    self.parent.addLogEntry("Output BTP [" + outputFileName[0] + "]")


def PatchMainFunction(log, type, ):
    binFileName = QFileDialog.getOpenFileName(self, "Open bin", "", "Bins (*.bin)", )
    if binFileName[0] == "":
        self.parent.addLogEntry("User did not select an input bin")
        return

    self.parent.addLogEntry("Input BIN [" + binFileName[0] + "]")
    patchBin = SimosBIN()
    patchBin.load(binFileName[0])
    hwKey, hwValue = patchBin.hardwareType()

    if hwValue is None:
        self.parent.addLogEntry("Input BIN - invalid hardware type")
        return

    self.parent.addLogEntry("Hardware code [" + hwKey + "]")

    softwareCode = patchBin.softwareCode()
    if softwareCode is not None:
        self.parent.addLogEntry("Software code [" + softwareCode + "]")

    patchFileName = QFileDialog.getOpenFileNames(self, "Open Patch", "", "BinToolz Patch (*.btp)", )
    if patchFileName[0] == "":
        self.parent.addLogEntry("User did not select a patch")
        return

    if type != FunctionType.FUNC_CHECK:
        outputFileName = QFileDialog.getSaveFileName(self, "Save OUT bin", "", "Bins (*.bin)", )
        if outputFileName[0] == "":
            self.parent.addLogEntry("User did not select an output bin")
            return

        self.parent.addLogEntry("Output BIN [" + outputFileName[0] + "]")

    #build logging strings
    if type == FunctionType.FUNC_ADD:
        functionVerb = "Adding"
        functionAdj = "added"

    elif type == FunctionType.FUNC_REMOVE:
        functionVerb = "Removing"
        functionAdj = "removed"

    else:
        functionVerb = "Checking"
        functionAdj = "checked"

    #add all selected patches to input bin
    operationsSuccess = 0;
    operationsFailed = 0;
    for currentPatchFileName in patchFileName[0]:
        self.parent.addLogEntry(functionVerb + " patch [" + currentPatchFileName + "]")
        ret = self.PatchSubFunction(type, currentPatchFileName, patchBin)
        if type == FunctionType.FUNC_CHECK:
                self.parent.addLogEntry("  " + ret.string())

        else:
            if ret == ReturnType.OK:
                operationsSuccess += 1
                self.parent.addLogEntry("  Successful")

            else:
                operationsFailed += 1
                self.parent.addLogEntry("  Failed [" + ret.string() + "]")

    #display info and save bin
    if type != FunctionType.FUNC_CHECK:
        self.parent.addLogEntry("* Patches successfully " + functionAdj + " [" + str(operationsSuccess) + "] *")
        if operationsFailed > 0:
            self.parent.addLogEntry("* Patches failed to be " + functionAdj + " [" + str(operationsFailed) + "] *")

        #save bin
        patchBin.save(outputFileName[0])
        self.parent.addLogEntry("Successfully wrote bin [" + outputFileName[0] + "]")


#ran once per patch (check/add/remove)
def PatchSubFunction(log, type, patchFileName, bin):
    #open patch and checksum
    patch = BTP()
    ret = patch.load(patchFileName)
    if ret != ReturnType.OK:
        if ret == ReturnType.INVALID_VERSION:
            log("Patch file version mismatch [" + BTP_VERSION + ":" + patch.header.version + "]")

        elif ret == ReturnType.INVALID_CHECKSUM:
            log("Patch file version mismatch [" + hex(patch.header.blockChecksum) + ":" + hex(patch.checkSum) + "]")

        else:
            log("Unkown error")

        return ret

    #compare softwarecodes
    if patch.header.softCode.find(bin.softwareCode()) != 0:
        log("Software code mismatch [" + bin.softwareCode() + ":" + patch.header.softCode + "]")
        return ReturnType.INVALID_PARAM

    #compare file size
    if patch.header.fileSize != len(bin.data):
        log("Filesize mismatch [" + str(len(bin.data)) + ":" + str(patch.header.fileSize) + "]")

    #do function
    if type == FunctionType.FUNC_CHECK:
        ret = PatchFunctionCheck(patch, bin, True)
        if ret != ReturnType.OK:
            ret = PatchFunctionCheck(patch, bin, False)
            if ret != ReturnType.OK:
                return ReturnType.NOT_READY_TO_ACCEPT

            else: 
                return ReturnType.READY_TO_ACCEPT

        else:
            return ReturnType.PATCH_FOUND

    elif type == FunctionType.FUNC_ADD:
        return PatchFunctionChange(patch, bin, False)

    elif type == FunctionType.FUNC_REMOVE:
        return PatchFunctionChange(patch, bin, True)
    
    return ReturnType.OK


#check bin to see if it is ready to accept patch
def PatchFunctionCheck(patch, bin, remove, ignore):
    ret = patch.checkBin(bin, remove)
    if ret != ReturnType.OK:
        if ret == ReturnType.MODIFIED_CAL:
            if ignore == False:
                return ReturnType.MODIFIED_CAL

        else:
            return ret

    return ReturnType.OK


#physically changes the bytes in bin (add/remove)
def PatchFunctionChange(patch, bin, remove, ignore):
    ret = PatchFunctionCheck(patch, bin, remove)
    if ret != ReturnType.OK:
        return ret

    return patch.changeBin(bin, remove, ignore == False)
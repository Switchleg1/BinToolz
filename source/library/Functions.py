from library.SimosBIN import SimosBIN
from library.BTP import BTP, BTP_VERSION
from Return import ReturnType
from enum import Enum


# Patch operation requested by the caller (GUI radio button or CLI verb).
class FunctionType(Enum):
    FUNC_CHECK  = 1
    FUNC_ADD    = 2
    FUNC_REMOVE = 3


# How calibration data should be treated when checking/applying a patch.
#   NORMAL - refuse to touch a bin whose cal has been modified
#   IGNORE - leave the cal block untouched, only patch ASW
#   FORCE  - apply over a modified cal anyway
class DataMode(Enum):
    NORMAL = 1
    IGNORE = 2
    FORCE  = 3


# Every function below is GUI/CLI agnostic: it takes the file paths and options
# it needs plus a `log` callback (one string argument). The GUI passes
# MainWindow.addLogEntry; the CLI passes the builtin print. They return a
# ReturnType so the caller can react (the CLI maps it to an exit code).


def calImport(fullFileName, calFileName, outputFileName, matchBoxCode, log):
    log("------------------------")
    log("- Starting CAL import  -")
    log("------------------------")

    log("Full BIN [" + fullFileName + "]")
    fullBin = SimosBIN()
    ret = fullBin.load(fullFileName)
    if ret != ReturnType.OK:
        log("Unable to open full bin [" + ret.string() + "]")
        return ret
    fullHWKey, fullHWValue = fullBin.hardwareType()
    if fullHWValue is None:
        log("Full BIN - invalid hardware type")
        return ReturnType.UNKNOWN_HW

    log("Cal BIN [" + calFileName + "]")
    calBin = SimosBIN()
    ret = calBin.load(calFileName)
    if ret != ReturnType.OK:
        log("Unable to open cal bin [" + ret.string() + "]")
        return ret
    calHWKey, calHWValue = calBin.hardwareType()
    if calHWValue is None:
        log("Cal BIN - invalid hardware type")
        return ReturnType.UNKNOWN_HW

    if calHWKey != fullHWKey:
        log("Hardware types do not match  [" + calHWKey + " : " + fullHWKey + "]")
        return ReturnType.HW_DOES_NOT_MATCH

    if matchBoxCode and calBin.boxCode() != fullBin.boxCode():
        log("Boxcodes do not match  [" + calBin.boxCode() + " : " + fullBin.boxCode() + "]")
        return ReturnType.BOX_DOES_NOT_MATCH

    log("Output BIN [" + outputFileName + "]")
    outputBin = SimosBIN()
    outputBin.copy(fullBin)
    ret = outputBin.swapBlock(calBin, 4)
    if ret != ReturnType.OK:
        log("Error importing cal [" + ret.string() + "]")
        return ret

    ret = outputBin.save(outputFileName)
    if ret != ReturnType.OK:
        log("Unable to write bin [" + ret.string() + "]")
        return ret

    log("Cal swap successfully wrote [" + outputFileName + "]")
    return ReturnType.OK


def calExport(fullFileName, outputFileName, log):
    log("------------------------")
    log("- Starting CAL export  -")
    log("------------------------")

    log("Full BIN [" + fullFileName + "]")
    fullBin = SimosBIN()
    ret = fullBin.load(fullFileName)
    if ret != ReturnType.OK:
        log("Unable to open full bin [" + ret.string() + "]")
        return ret
    fullHWKey, fullHWValue = fullBin.hardwareType()
    if fullHWValue is None:
        log("Full BIN - invalid hardware type")
        return ReturnType.UNKNOWN_HW

    log("Output BIN [" + outputFileName + "]")
    outputBin = SimosBIN()
    ret = outputBin.copy(fullBin, 4)
    if ret != ReturnType.OK:
        log("Error exporting cal [" + ret.string() + "]")
        return ret

    ret = outputBin.save(outputFileName)
    if ret != ReturnType.OK:
        log("Unable to write bin [" + ret.string() + "]")
        return ret

    log("Cal export successfully wrote [" + outputFileName + "]")
    return ReturnType.OK


def patchCreate(originalFileName, modifiedFileName, outputFileName, dataMode, log):
    log("------------------------")
    log("- Starting Patch Create-")
    log("------------------------")

    log("Original BIN [" + originalFileName + "]")
    originalBin = SimosBIN()
    ret = originalBin.load(originalFileName)
    if ret != ReturnType.OK:
        log("Unable to open original bin [" + ret.string() + "]")
        return ret
    originalHWKey, originalHWValue = originalBin.hardwareType()
    if originalHWValue is None:
        log("Original BIN - invalid hardware type")
        return ReturnType.UNKNOWN_HW

    log("Modified BIN [" + modifiedFileName + "]")
    modifiedBin = SimosBIN()
    ret = modifiedBin.load(modifiedFileName)
    if ret != ReturnType.OK:
        log("Unable to open modified bin [" + ret.string() + "]")
        return ret
    modifiedHWKey, modifiedHWValue = modifiedBin.hardwareType()
    if modifiedHWValue is None:
        log("Modified BIN - invalid hardware type")
        return ReturnType.UNKNOWN_HW

    if modifiedHWKey != originalHWKey:
        log("Hardware types do not match  [" + modifiedHWKey + " : " + originalHWKey + "]")
        return ReturnType.HW_DOES_NOT_MATCH

    patch = BTP()
    ret = patch.createPatch(originalBin, modifiedBin, dataMode != DataMode.IGNORE)
    if ret != ReturnType.OK:
        log("Unable to create patch [" + ret.string() + "]")
        return ret

    ret = patch.save(outputFileName)
    if ret != ReturnType.OK:
        log("Unable to write patch [" + ret.string() + "]")
        return ret

    log("Output BTP [" + outputFileName + "]")
    return ReturnType.OK


def patchApply(type, binFileName, patchFileNames, outputFileName, dataMode, log):
    if type == FunctionType.FUNC_ADD:
        banner = "- Starting Patch Add   -"
        functionVerb = "Adding"
        functionAdj = "added"
    elif type == FunctionType.FUNC_REMOVE:
        banner = "- Starting Patch Remove-"
        functionVerb = "Removing"
        functionAdj = "removed"
    else:
        banner = "- Starting Patch Check -"
        functionVerb = "Checking"
        functionAdj = "checked"

    log("------------------------")
    log(banner)
    log("------------------------")

    log("Input BIN [" + binFileName + "]")
    patchBin = SimosBIN()
    ret = patchBin.load(binFileName)
    if ret != ReturnType.OK:
        log("Unable to open input bin [" + ret.string() + "]")
        return ret
    hwKey, hwValue = patchBin.hardwareType()
    if hwValue is None:
        log("Input BIN - invalid hardware type")
        return ReturnType.UNKNOWN_HW

    log("Hardware code [" + hwKey + "]")

    softwareCode = patchBin.softwareCode()
    if softwareCode is not None:
        log("Software code [" + softwareCode + "]")

    if type != FunctionType.FUNC_CHECK:
        log("Output BIN [" + outputFileName + "]")

    # apply each selected patch to the input bin
    operationsSuccess = 0
    operationsFailed = 0
    for currentPatchFileName in patchFileNames:
        log(functionVerb + " patch [" + currentPatchFileName + "]")
        # never let one malformed patch abort the whole batch (or crash the GUI)
        try:
            ret = _patchSubFunction(type, currentPatchFileName, patchBin, dataMode, log)
        except Exception as e:
            log("  Error processing patch [" + str(e) + "]")
            ret = ReturnType.CANNOT_READ
        if type == FunctionType.FUNC_CHECK:
            log("  " + ret.string())
        else:
            if ret == ReturnType.OK:
                operationsSuccess += 1
                log("  Successful")
            else:
                operationsFailed += 1
                log("  Failed [" + ret.string() + "]")

    # check mode is informational only - nothing is written
    if type == FunctionType.FUNC_CHECK:
        return ReturnType.OK

    # display info and save bin
    log("* Patches successfully " + functionAdj + " [" + str(operationsSuccess) + "] *")
    if operationsFailed > 0:
        log("* Patches failed to be " + functionAdj + " [" + str(operationsFailed) + "] *")

    ret = patchBin.save(outputFileName)
    if ret != ReturnType.OK:
        log("Unable to write bin [" + ret.string() + "]")
        return ret

    log("Successfully wrote bin [" + outputFileName + "]")
    return ReturnType.OK


# ran once per patch (check/add/remove)
def _patchSubFunction(type, patchFileName, bin, dataMode, log):
    # open patch and checksum
    patch = BTP()
    ret = patch.load(patchFileName)
    if ret != ReturnType.OK:
        if ret == ReturnType.INVALID_VERSION:
            log("Patch file version mismatch [" + BTP_VERSION + ":" + patch.header.version + "]")
        elif ret == ReturnType.INVALID_CHECKSUM:
            log("Patch file checksum mismatch [" + hex(patch.header.blockChecksum) + ":" + hex(patch.checksum) + "]")
        else:
            log("Unkown error")
        return ret

    # compare softwarecodes
    if patch.header.softCode.find(bin.softwareCode()) != 0:
        log("Software code mismatch [" + bin.softwareCode() + ":" + patch.header.softCode + "]")
        return ReturnType.INVALID_PARAM

    # compare file size
    if patch.header.fileSize != len(bin.data):
        log("Filesize mismatch [" + str(len(bin.data)) + ":" + str(patch.header.fileSize) + "]")

    # do function
    if type == FunctionType.FUNC_CHECK:
        ret = _patchFunctionCheck(patch, bin, True, dataMode)
        if ret != ReturnType.OK:
            ret = _patchFunctionCheck(patch, bin, False, dataMode)
            if ret != ReturnType.OK:
                return ReturnType.NOT_READY_TO_ACCEPT
            else:
                return ReturnType.READY_TO_ACCEPT
        else:
            return ReturnType.PATCH_FOUND

    elif type == FunctionType.FUNC_ADD:
        return _patchFunctionChange(patch, bin, False, dataMode)

    elif type == FunctionType.FUNC_REMOVE:
        return _patchFunctionChange(patch, bin, True, dataMode)

    return ReturnType.OK


# check bin to see if it is ready to accept patch
def _patchFunctionCheck(patch, bin, remove, dataMode):
    ret = patch.checkBin(bin, remove)
    if ret != ReturnType.OK:
        if ret == ReturnType.MODIFIED_CAL:
            if dataMode == DataMode.NORMAL:
                return ReturnType.MODIFIED_CAL
        else:
            return ret

    return ReturnType.OK


# physically changes the bytes in bin (add/remove)
def _patchFunctionChange(patch, bin, remove, dataMode):
    ret = _patchFunctionCheck(patch, bin, remove, dataMode)
    if ret != ReturnType.OK:
        return ret

    return patch.changeBin(bin, remove, dataMode != DataMode.IGNORE)

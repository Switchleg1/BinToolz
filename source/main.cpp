//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <time.h>
#include "main.h"
#include "Constants.h"
#include "LogFile.h"
#include "Simos.h"
#include "PatchBlock.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm	*MainForm;
//---------------------------------------------------------------------------
AnsiString IntToHex(uint32_t in)
{
	AnsiString ret;
	while(in) {
		int8_t current = in % 16;

		if(current < 10) {
			current += '0';
		} else {
			current += 'A' - 10;
		}

		ret = AnsiString(current) + ret;
		in = in >> 4;
	}

	if(!ret.Length())
		ret = "0";

	ret = AnsiString("0x") + ret;
	return ret;
}
//---------------------------------------------------------------------------
AnsiString DataToHex(uint8_t *in, uint32_t len)
{
	AnsiString ret;
	for(uint32_t i=0; i<len; i++) {
		uint8_t in2 = in[i];
		AnsiString byteString;
		for(uint32_t d=0; d<2; d++) {
			int8_t current = in2 % 16;

			if(current < 10) {
				current += '0';
			} else {
				current += 'A' - 10;
			}

			byteString = AnsiString(current) + byteString;
			in2 = in2 >> 4;
		}
		ret += byteString;
	}

	if(!ret.Length())
		ret = "00";

	return ret;
}
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
	: TForm(Owner)
{

}
__fastcall TMainForm::~TMainForm()
{

}
//---------------------------------------------------------------------------
void __fastcall TMainForm::addToLog(AnsiString line)
{
	time_t timer;
    char buffer[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	AnsiString logLine = AnsiString("[")+AnsiString(buffer)+"] "+line;
	LogListBox->Items->Add(logLine);
	LogListBox->ItemIndex = LogListBox->Count-1;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::fileResult(int8_t result, AnsiString fileName)
{
	switch(result) {
	case -1:
		addToLog("File already open ["+fileName+"]");
		break;
	case -2:
		addToLog("Unable to open file ["+fileName+"]");
		break;
	case -3:
		addToLog("File too large ["+fileName+"]");
		break;
	case -4:
		addToLog("Unable to allocate memory for file ["+fileName+"]");
		break;
	case -5:
		addToLog("Unable to read file data ["+fileName+"]");
		break;
	case -6:
		addToLog("Unable to save file ["+fileName+"]");
		break;
	case -7:
		addToLog("Unknown hardware type ["+fileName+"]");
		break;
	case -8:
		addToLog("Unable to write data ["+fileName+"]");
		break;
	case -9:
		addToLog("File already created ["+fileName+"]");
		break;
	case -10:
		addToLog("Unable to allocate memory for file ["+fileName+"]");
		break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ConvertButtonClick(TObject *Sender)
{
	addToLog("");
	addToLog("Converting Ghidra export to full bin.");
	addToLog("-------------------------------------");

	OpenDialog->Options >> ofAllowMultiSelect;
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open Ghidra export";
	if(!OpenDialog->Execute()) {
		addToLog("User did not select Ghidra export.");
		return;
	}
	AnsiString	inFileName = OpenDialog->FileName;
	OpenDialog->Title = "Open CAL replacement";

	if(CALCheckBox->Checked && !BlockRadioButton->Checked && !OpenDialog->Execute()) {
		addToLog("User did not select CAL file.");
		return;
	}
	AnsiString	fileCALName = OpenDialog->FileName;

	SaveDialog->Title = "Save output Bin";
	SaveDialog->DefaultExt = ".bin";
	SaveDialog->Filter = "Bin|*.bin";
	if(!SaveDialog->Execute()) {
		addToLog("User did not select output file.");
		return;
	}
	AnsiString	fileOutName = SaveDialog->FileName;

	int32_t iResult;
	SimosBIN *dInputBin = NULL, *dCALBin = NULL, *dOutBin = NULL;
	hardware_t* inHardware = NULL;

	//open input bin
	dInputBin = new SimosBIN();
	iResult = dInputBin->load((char*)inFileName.c_str());
	if(iResult) {
		fileResult(iResult, inFileName);
		goto cleanup;
	}
	addToLog("Opened input bin ["+inFileName+"] Size ["+IntToStr((int32_t)dInputBin->size())+"]");

	//check simos hw version
	inHardware = dInputBin->hardwareType();
	if(inHardware == NULL) {
		addToLog("Invalid bin length ["+IntToStr((int)dInputBin->size())+"]");
		goto cleanup;
	}

	//creating a full bin?
	if(FullRadioButton->Checked) {
		//write full bin
		dOutBin = new SimosBIN();
		dOutBin->create(inHardware->bin_size);
		for(int i=0; i<5; i++) {
			memcpy(&dOutBin->data()[inHardware->blocks[i].bin_pos], &dInputBin->data()[inHardware->blocks[i].ghidra_pos], inHardware->blocks[i].length);
			addToLog("Copied block ["+IntToStr(i+1)+"]");
		}

		//replace cal
		if(CALCheckBox->Checked) {
			//open input bin
			dCALBin = new SimosBIN();
			iResult = dCALBin->load((char*)fileCALName.c_str());
			if(iResult) {
				fileResult(iResult, fileCALName);
				goto cleanup;
			}
			addToLog("Opened CAL bin ["+fileCALName+"]");

			if(dCALBin->size() != inHardware->bin_size && dCALBin->size() != inHardware->blocks[4].length) {
				addToLog("Invalid CAL bin type ["+fileCALName+"]");
				goto cleanup;
			}

			int32_t boxCode = dOutBin->verifyBoxCode(dCALBin);
			if(boxCode == -2 && ConvertCheckBox->Checked) {
				addToLog("Boxcodes do not match");
				goto cleanup;
			}

			//swap block
			iResult = dOutBin->swapBlock(dCALBin, 4);
			switch(iResult) {
				case -1:
					addToLog("Invalid CAL bin type ["+fileCALName+"]");
					goto cleanup;
				case -2: {
						AnsiString calHWName = dCALBin->hardwareType() ? AnsiString(dCALBin->hardwareType()->name) : AnsiString("None");
						addToLog("BIN hardware types do not match ["+AnsiString(dInputBin->hardwareType()->name)+":"+calHWName+"]");
						goto cleanup;
					}
					goto cleanup;
				case -3:
					addToLog("Invalid input bin ["+inFileName+"]");
					goto cleanup;
			}
			addToLog("Swapped CAL block");
		}

		//write output bin
		iResult = dOutBin->save((char*)fileOutName.c_str());
		if(iResult) {
			fileResult(iResult, fileOutName);
			goto cleanup;
		}
	} else {
		//separate block bins
		for(int i=0; i<5; i++) {
			//write output bin
			iResult = dOutBin->save((char*)(fileOutName+IntToStr(i)).c_str(), i);
			if(iResult) {
				fileResult(iResult, fileOutName+IntToStr(i));
				goto cleanup;
			}
		}
	}

	addToLog("Wrote "+AnsiString(inHardware->name)+" bin ["+fileOutName+"]");

cleanup:
	if(dInputBin)
		free(dInputBin);
	if(dCALBin)
		free(dCALBin);
	if(dOutBin)
		free(dOutBin);
}
//---------------------------------------------------------------------------
int __fastcall TMainForm::PatchFunction(AnsiString inFileName, AnsiString patchFileName, AnsiString outFileName)
{
	int32_t iResult;
	SimosBIN *dInputBin = NULL, *dPatchBin = NULL;

	//open input bin
	dInputBin = new SimosBIN();
	iResult = dInputBin->load((char*)inFileName.c_str());
	if(iResult) {
		fileResult(iResult, inFileName);
		goto cleanup;
	}
	if(!dInputBin->softwareCode()) {
		addToLog("Software code not found ["+inFileName+"] Size ["+IntToStr((int32_t)dInputBin->size())+"]");
		goto cleanup;
	}
	char sw_code[12];
	memset(sw_code, 0, 12);
	memcpy(sw_code, dInputBin->softwareCode(), 11);
	addToLog("Hardware code ["+AnsiString(dInputBin->hardwareType()->name)+"]");
	addToLog("Software code ["+AnsiString(sw_code)+"]");
	addToLog("Opened input bin ["+inFileName+"] Size ["+IntToStr((int32_t)dInputBin->size())+"]");

	//open patch bin
	dPatchBin = new SimosBIN();
	iResult = dPatchBin->load((char*)patchFileName.c_str());
	if(iResult) {
		fileResult(iResult, patchFileName);
		goto cleanup;
	}
	addToLog("Opened patch bin ["+patchFileName+"] Size ["+IntToStr((int32_t)dPatchBin->size())+"]");

	if(CreateRadioButton->Checked) {
		//error check size of bins
		if(dPatchBin->size() != dInputBin->size()) {
			addToLog("File sizes must match");
			goto cleanup;
		}

		int gapFill;
		try {
			gapFill = StrToInt(GapFillEdit->Text);
		} catch (EConvertError &E) {
			gapFill = 0;
		}

		//create block data
		uint32_t	inSize		= dInputBin->size();
		uint8_t*	inData	  	= dInputBin->data();
		uint8_t*	patchData	= dPatchBin->data();
		uint8_t*	blockData	= NULL;
		uint32_t 	blockSize	= 0;
		int32_t 	blockCount	= 0;
		uint32_t 	patchSize	= 0;
		int32_t		changeStart	= -1;
		for(uint32_t i=0; i<inSize; i++) {
			if(changeStart > -1) {
				if(inData[i] == patchData[i]) {
					BYTE n = 0;
					BOOL found_next = false;
					while(n++ < gapFill) {
						if(inData[i+n] != patchData[i+n])
							found_next = true;
					}

					if(!found_next) {
						uint32_t newPatchSize = i-changeStart;
						blockData = addPatchBlock(newPatchSize, changeStart, inData, patchData, blockData, &blockSize);
						patchSize += newPatchSize;
						blockCount++;
						addToLog("Adding block ["+IntToStr(blockCount)+"] Size ["+IntToStr((int32_t)newPatchSize)+"] Offset ["+IntToHex(changeStart)+"]");
						uint32_t hexSize = 16;
						if(hexSize > newPatchSize)
							hexSize = newPatchSize;
						addToLog("    ["+DataToHex(&inData[changeStart], hexSize)+"] : ["+DataToHex(&patchData[changeStart], hexSize)+"]");
						changeStart = -1;
					}
				}
			} else {
				if(inData[i] != patchData[i])
					changeStart = i;
			}
		}

		//create output header
		free(dPatchBin);
		uint32_t iSaveSize = blockSize+sizeof(patch_header_t);
		dPatchBin = new SimosBIN();
		dPatchBin->create(iSaveSize);
		memset(dPatchBin->data(), 0, iSaveSize);
		patch_header_t *header = (patch_header_t*)dPatchBin->data();
		strcpy(header->version, BTP_VERSION);
		memcpy(header->soft_code, dInputBin->softwareCode(), 8);
		header->block_checksum	= getChecksum(blockSize, blockData, 0xFFFFFFFF);
		header->block_count		= blockCount;
		header->file_size		= dInputBin->size();
		memcpy(&dPatchBin->data()[sizeof(patch_header_t)], blockData, blockSize);
		free(blockData);

		//write output bin
		iResult = dPatchBin->save((char*)outFileName.c_str());
		if(iResult) {
			fileResult(iResult, outFileName);
			goto cleanup;
		}

		//update statusbar
		addToLog("Created Patch: "+IntToStr((int32_t)patchSize)+"bytes "+IntToStr(blockCount)+"blocks ["+outFileName+"]");
	} else {
		patch_header_t *header 		= (patch_header_t*)dPatchBin->data();
		AnsiString btp_version 		= header->version;
		AnsiString soft_code_patch 	= AnsiString(header->soft_code).SetLength(8);
		AnsiString soft_code_bin 	= AnsiString(dInputBin->softwareCode()).SetLength(8);

		//check file version
		if(btp_version.Trim() != BTP_VERSION) {
			addToLog("Patch file version mismatch ["+btp_version+"]");
			goto cleanup;
		}

		//check file size
		if(dInputBin->size() != header->file_size) {
			addToLog("Incorrect input bin size ["+IntToStr((int32_t)dInputBin->size())+":"+IntToStr((int32_t)header->file_size)+"]");
			goto cleanup;
		}

		//check software code
		if(soft_code_patch.Trim() != soft_code_bin.Trim()) {
			addToLog("Patch and bin software codes do not match ["+soft_code_patch.Trim()+":"+soft_code_bin.Trim()+"]");
			goto cleanup;
		}

		//check patch checksum
		uint8_t*	block_data = dPatchBin->data() + sizeof(patch_header_t);
		uint32_t	block_checksum = getChecksum(dPatchBin->size()-sizeof(patch_header_t), block_data, 0xFFFFFFFF);
		if(block_checksum != header->block_checksum) {
			addToLog("Invalid patch checksum");
			goto cleanup;
		}

		bool remove_patch = false;
		AnsiString status_str = "Added Patch: ";
		AnsiString bin_str = "Bin is not ready to accept patch";
		if(RemoveRadioButton->Checked) {
			remove_patch = true;
			status_str = "Removed Patch: ";
			bin_str = "Bin does not contain patch";
		} else if(CheckRadioButton->Checked) {
			remove_patch = true;
			status_str = "Bin contains patch";
			bin_str = "Bin does not contain patch";
		}

		//check bin is ready to accept patch
		unsigned char *block_data_tmp = block_data;
		bool modifiedCAL = false;
		for(uint32_t i=0; i<header->block_count; i++) {
			patch_block_t *block_header = (patch_block_t*)block_data_tmp;
			hardware_t *hwType = dInputBin->hardwareType();
			bool isCAL = false;

			if((IgnoreCheckBox->Checked || CheckRadioButton->Checked) &&
				block_header->offset >= hwType->blocks[4].bin_pos &&
				block_header->offset + block_header->length <= hwType->blocks[4].bin_pos + hwType->blocks[4].length) {
				isCAL = true;
			}

			if(!checkPatchBlock(dInputBin->size(), dInputBin->data(), block_data_tmp, remove_patch)) {
				if(!isCAL) {
					addToLog(bin_str);
					iResult = -1;
					goto cleanup;
				} else {
					modifiedCAL = true;
				}
			}

			block_data_tmp += sizeof(patch_block_t) + block_header->length * 2;
		}

		//check patch succeeded
		if(CheckRadioButton->Checked) {
			if(modifiedCAL) {
				status_str += ", CAL has been modified";
			}

			addToLog(status_str);
			goto cleanup;
		}

		//patch bin data
		int patchSize = 0;
		block_data_tmp = block_data;
		for(uint32_t i=0; i<header->block_count; i++) {
			patch_block_t *block_header = (patch_block_t*)block_data_tmp;
			hardware_t *hwType = dInputBin->hardwareType();
			bool isCAL = false;

			if(IgnoreCheckBox->Checked &&
				block_header->offset >= hwType->blocks[4].bin_pos &&
				block_header->offset + block_header->length <= hwType->blocks[4].bin_pos + hwType->blocks[4].length) {
				isCAL = true;
			}

			if(!(IgnoreCheckBox->Checked && isCAL) && !setPatchBlock(dInputBin->size(), dInputBin->data(), block_data_tmp, !remove_patch)) {
				addToLog("Patch block is invalid");
				goto cleanup;
			}

			patchSize += block_header->length;
			block_data_tmp += sizeof(patch_block_t) + block_header->length * 2;
		}

		//write output bin
		iResult = dInputBin->save((char*)outFileName.c_str());
		if(iResult) {
			fileResult(iResult, outFileName);
			goto cleanup;
		}

		//update statusbar
		addToLog(status_str+IntToStr(patchSize)+"bytes "+IntToStr((int32_t)header->block_count)+"blocks ["+outFileName+"]");
	}

	iResult = 0;

cleanup:
	if(dInputBin)
		free(dInputBin);
	if(dPatchBin)
		free(dPatchBin);

	return iResult;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::PatchButtonClick(TObject *Sender)
{
	addToLog("");
	addToLog("One click patch.");
	addToLog("----------------");

	OpenDialog->Options >> ofAllowMultiSelect;
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	if(CreateRadioButton->Checked) OpenDialog->Title = "Open Original Bin";
	else OpenDialog->Title = "Open bin";
	if(!OpenDialog->Execute()) {
		addToLog("User did not select original file.");
		return;
	}
	AnsiString inFileName = OpenDialog->FileName;

	AnsiString fileType = "modified";
	if(CreateRadioButton->Checked) {
		OpenDialog->Title = "Open modified bin";

		SaveDialog->Title = "Save BinToolz patch";
		SaveDialog->DefaultExt = ".btp";
		SaveDialog->Filter = "BinToolz Patch|*.btp";
	} else {
		if(AddRadioButton->Checked) {
			OpenDialog->Title = "Add patch";
		} else if(RemoveRadioButton->Checked) {
			OpenDialog->Title = "Remove patch";
		} else {
			OpenDialog->Title = "Check patch";
		}
		OpenDialog->Options << ofAllowMultiSelect;
		OpenDialog->DefaultExt = ".btp";
		OpenDialog->Filter = "BinToolz Patch|*.btp";
		fileType = "patch";

		SaveDialog->Title = "Save output bin";
		SaveDialog->DefaultExt = ".bin";
		SaveDialog->Filter = "Bin|*.bin";
	}
	if(!OpenDialog->Execute()) {
		addToLog("User did not select "+fileType+" file.");
		return;
	}


	//get output bin name
	if(!CheckRadioButton->Checked) {
		SaveDialog->FileName = inFileName;
		if(!SaveDialog->Execute()) {
			addToLog("User did not select output file.");
			return;
		}
	}
	AnsiString outFileName = SaveDialog->FileName;

	uint8_t operationsSuccess = 0;
	uint8_t operationsFailed = 0;
	for(int i=0; i<OpenDialog->Files->Count; i++) {
		AnsiString patchFileName = OpenDialog->Files->Strings[i];
		if(PatchFunction(inFileName, patchFileName, outFileName)) {
			operationsFailed++;
		} else {
			operationsSuccess++;
		}
		inFileName = outFileName;
	}

	if(OpenDialog->Files->Count > 1) {
		addToLog("***Functions Passed ["+IntToStr(operationsSuccess)+"] Failed ["+IntToStr(operationsFailed)+"]***");
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::CALButtonClick(TObject *Sender)
{
	AnsiString funcType = "Import";
	if(ExportRadioButton->Checked) funcType = "Export";

	addToLog("");
	addToLog(funcType+" CAL block.");
	addToLog("-----------------");

	OpenDialog->Options >> ofAllowMultiSelect;
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open FULL bin";
	if(!OpenDialog->Execute()) {
		addToLog("User did not select FULL bin.");
		return;
	}
	AnsiString	inFileName = OpenDialog->FileName;

	if(ImportRadioButton->Checked) {
		OpenDialog->Title = "Open CAL bin";
		if(!OpenDialog->Execute()) {
			addToLog("User did not select CAL bin.");
			return;
		}
	}
	AnsiString	fileCALName = OpenDialog->FileName;

	SaveDialog->Title = "Save output bin";
	SaveDialog->DefaultExt = ".bin";
	SaveDialog->Filter = "Bin|*.bin";
	if(!SaveDialog->Execute()) {
		addToLog("User did not select output bin.");
		return;
	}
	AnsiString fileOutName = SaveDialog->FileName;

	int32_t 	iResult;
	SimosBIN 	*dInputBin = NULL, *dCALBin = NULL;
	hardware_t* inHardware;
	char 		boxCode;

	//open input bin
	dInputBin = new SimosBIN();
	iResult = dInputBin->load((char*)inFileName.c_str());
	if(iResult) {
		fileResult(iResult, inFileName);
		goto cleanup;
	}
	addToLog("Opened FULL bin ["+inFileName+"] Size ["+IntToStr((int32_t)dInputBin->size())+"]");

	//get input bin hardware type
	inHardware = dInputBin->hardwareType();
	if(!inHardware) {
		addToLog("Invalid input bin ["+inFileName+"]");
		goto cleanup;
	}

	//do specified function
	if(ImportRadioButton->Checked) { //Import CAL
		//open CAL bin
		dCALBin = new SimosBIN();
		iResult = dCALBin->load((char*)fileCALName.c_str());
		if(iResult) {
			fileResult(iResult, fileCALName);
			goto cleanup;
		}
		addToLog("Opened CAL bin ["+fileCALName+"] Size ["+IntToStr((int32_t)dCALBin->size())+"]");

		//check box codes
		boxCode = dInputBin->verifyBoxCode(dCALBin);
		if(boxCode == -2 && SwapCheckBox->Checked) {
			addToLog("Boxcodes do not match");
			goto cleanup;
		}

		//swap block
		iResult = dInputBin->swapBlock(dCALBin, 4);
		switch(iResult) {
			case -1:
				addToLog("Invalid CAL bin type ["+fileCALName+"]");
				goto cleanup;
			case -2: {
					AnsiString calHWName = dCALBin->hardwareType() ? AnsiString(dCALBin->hardwareType()->name) : AnsiString("None");
					addToLog("BIN hardware types do not match ["+AnsiString(dInputBin->hardwareType()->name)+":"+calHWName+"]");
					goto cleanup;
				}
			case -3:
				addToLog("Invalid input bin ["+inFileName+"]");
				goto cleanup;
		}
		addToLog("Swapped CAL block ["+IntToStr((int32_t)dInputBin->hardwareType()->blocks[4].length)+"]");

		//write output bin
		iResult = dInputBin->save((char*)fileOutName.c_str());
		if(iResult) {
			fileResult(iResult, fileOutName);
			goto cleanup;
		}

		addToLog("Wrote "+AnsiString(dInputBin->hardwareType()->name)+" bin ["+fileOutName+"]");
	} else { //Export CAL
		//write output bin
		iResult = dInputBin->save((char*)fileOutName.c_str(), 4);
		if(iResult) {
			fileResult(iResult, fileOutName);
			goto cleanup;
		}

		addToLog("Wrote "+AnsiString(inHardware->name)+" bin ["+fileOutName+"]");
	}

cleanup:
	if(dInputBin)
		free(dInputBin);
	if(dCALBin)
		free(dCALBin);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::LogListBoxDblClick(TObject *Sender)
{
	SaveDialog->Title = "Save log file";
	SaveDialog->DefaultExt = ".txt";
	SaveDialog->Filter = "Log|*.txt";
	if(!SaveDialog->Execute()) {
		addToLog("User did not select log name");
		return;
	}
	AnsiString fileName = SaveDialog->FileName;
	LogFile *logFile = new LogFile();
	logFile->open((char*)fileName.c_str());
	for(int32_t i=0; i<LogListBox->Count; i++) {
		AnsiString entry = LogListBox->Items->Strings[i]+"\n";
		logFile->add((char*)entry.c_str());
	}
	logFile->close();
	free(logFile);

	addToLog("Log file saved ["+fileName+"]");
}
//---------------------------------------------------------------------------

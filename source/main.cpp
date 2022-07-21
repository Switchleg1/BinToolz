//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include "main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;
//---------------------------------------------------------------------------
#define HW_COUNT		2
#define BTP_VERSION		"BinToolz Patch v1.1"
#define MAX_BINSIZE     4194304

typedef unsigned int	uint32_t;
typedef unsigned short	uint16_t;
typedef unsigned char	uint8_t;
typedef int				int32_t;
typedef short			int16_t;
typedef char			int8_t;

typedef struct {
	uint32_t	ghidra_pos;
	uint32_t	bin_pos;
	uint32_t	length;
} block_t;

typedef struct {
	int8_t		name[15];
	uint32_t	box_code_start;
	uint32_t	box_code_end;
	uint32_t	soft_code_start;
	uint32_t	soft_code_end;
	uint32_t	bin_size;
	uint32_t	ghidra_size;
	block_t		blocks[5];
} hardware_t;

typedef struct {
	int8_t		version[20];
	int8_t		soft_code[8];
	uint32_t	block_count;
	uint32_t	block_checksum;
	uint32_t	file_size;
	uint8_t		space[60];
} patch_header_t;

typedef struct {
	uint32_t	offset;
	uint32_t	length;
} patch_block_t;

//---------------------------------------------------------------------------
hardware_t  simosHW[HW_COUNT] = {
	{
		"Simos 18.1",
		0x00200060,
		0x0020006b,
		0x00200023,
		0x0020002B,
		4194304,
		0x002E2E00,
		{
			{0x0, 			0x0001c000,	0x00023E00},
			{0x00023E00, 	0x00040000,	0x000FFC00},
			{0x00123A00, 	0x00140000,	0x000BFC00},
			{0x001E3600, 	0x00280000,	0x0007FC00},
			{0x00263200, 	0x00200000,	0x0007FC00}
		}
	},
	{
		"Simos 18.10",
		0x00220060,
		0x0022006b,
		0x00220023,
		0x0022002B,
		4194304,
		0x003DEE00,
		{
			{0x001df800, 	0x00200000,	0x0001FE00},
			{0x0, 			0x00020000,	0x000DFC00},
			{0x000dfc00, 	0x00100000,	0x000FFC00},
			{0x001ff600, 	0x002c0000,	0x0013FC00},
			{0x0033f200, 	0x00220000,	0x0009FC00}
		}
	}
};

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
hardware_t* getHardwareType(uint32_t iSize)
{
	//determine simos version
	for(uint32_t i=0; i<HW_COUNT; i++) {
		if(iSize == simosHW[i].ghidra_size) {
			return &simosHW[i];
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
hardware_t* getHardwareType(uint8_t* fData, uint32_t iSize)
{
	if(iSize == MAX_BINSIZE) {
		//determine simos version
		for(uint32_t i=0; i<HW_COUNT; i++) {
			int8_t box_code[12];
			memset(box_code, 0, 12);
			memcpy(box_code, &fData[simosHW[i].box_code_start], 11);
			if(AnsiString(box_code).Length() != 0)
				return &simosHW[i];
		}
	} else {
		for(uint32_t i=0; i<HW_COUNT; i++) {
			if(iSize == simosHW[i].blocks[4].length) {
				return &simosHW[i];
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
void getSoftcode(hardware_t* hw, uint8_t* fData, uint8_t* code)
{
	memset(code, 0, 9);

	if(hw) {
		memcpy(code, &fData[hw->soft_code_start], 8);
	}
}
//---------------------------------------------------------------------------
void getBoxcode(hardware_t* hw, uint8_t* fData, uint8_t* code)
{
	memset(code, 0, 12);

	if(hw) {
		memcpy(code, &fData[hw->box_code_start], 11);
	}
}
//---------------------------------------------------------------------------
int8_t verifyBoxCodes(uint8_t* fData1, uint8_t* fData2, uint32_t iSize2)
{
	hardware_t* hw = getHardwareType(fData1, MAX_BINSIZE);

	if(hw != getHardwareType(fData2, iSize2)) {
		return -1;
	}

	uint32_t iOffset2 = 0;
	if(iSize2 == hw->blocks[4].length) {
		iOffset2 = hw->blocks[4].bin_pos;
	}

	uint8_t box_code1[12];
	memset(box_code1, 0, 12);
	memcpy(box_code1, &fData1[hw->box_code_start], 11);

	uint8_t box_code2[12];
	memset(box_code2, 0, 12);
	memcpy(box_code2, &fData2[hw->box_code_start-iOffset2], 11);

	if(strcmp(box_code1, box_code2))
		return -2;

	return 1;
}
//---------------------------------------------------------------------------
uint32_t getChecksum(uint32_t data_len, uint8_t* data_in, uint32_t crc)
{
	for(uint32_t i=0;i<data_len;i++) {
		uint8_t ch=data_in[i];
		for(uint32_t j=0;j<8;j++) {
			uint32_t b=(ch^crc)&1;
			crc>>=1;
			if(b) crc=crc^0xEDB88320;
			ch>>=1;
		}
	}

	return ~crc;
}
//---------------------------------------------------------------------------
bool checkPatchBlock(uint32_t data_len, uint8_t* data_in, uint8_t* data_block, bool removing)
{
	patch_block_t *header = (patch_block_t*)data_block;
	if(header->offset+header->length > data_len)
		return false;
	data_block += sizeof(patch_block_t) + (removing*header->length);
	data_in += header->offset;
	for(uint32_t i=0; i<header->length; i++) {
		if(data_in[i] != data_block[i])
			return false;
	}
	return true;
}
//---------------------------------------------------------------------------
bool setPatchBlock(uint32_t data_len, uint8_t* data_in, uint8_t* data_block, bool adding)
{
	patch_block_t *header = (patch_block_t*)data_block;
	if(header->offset+header->length > data_len)
		return false;
	data_block += sizeof(patch_block_t) + (adding*header->length);
	data_in += header->offset;
	for(int i=0; i<header->length; i++) {
		data_in[i] = data_block[i];
	}
	return true;
}
//---------------------------------------------------------------------------
uint8_t* loadFile(char* name, uint32_t max_size, uint32_t* size, int8_t* result)
{
	FILE* file = fopen(name, "rb");
	if(file == NULL) {
		*result = -1;
		return NULL;
	}
	fseek(file, 0L, SEEK_END);
	*size = ftell(file);
	if(*size > max_size) {
		*result = -2;
		fclose(file);
		return NULL;
	}
	rewind(file);
	uint8_t* data = (uint8_t*)malloc(*size);
	if(data == NULL) {
		*result = -3;
		fclose(file);
		return NULL;
	}
	if(fread(data, sizeof(uint8_t), *size, file) != *size) {
		*result = -4;
		fclose(file);
		free(data);
		return NULL;
	}
	*result = 0;
	fclose(file);

	return data;
}
//---------------------------------------------------------------------------
void saveFile(char* name, uint8_t *data, uint32_t* size, int8_t* result)
{
	FILE* file = fopen(name, "wb");
	if(file == NULL) {
		*result = -5;
		return;
	}
	uint32_t actual_size = fwrite(data, sizeof(uint8_t), *size, file);
	if(actual_size != *size) {
		*size = actual_size;
		*result = -6;
		fclose(file);
		return;
	}
	fclose(file);

	*result = 0;
}
//---------------------------------------------------------------------------
void setFileResultStatus(TStatusBar* status, int8_t result, AnsiString fileName)
{
	switch(result) {
	case -1:
		status->Panels->Items[0]->Text = "Unable to open file ["+fileName+"]";
		break;
	case -2:
		status->Panels->Items[0]->Text = "File too large ["+fileName+"]";
		break;
	case -3:
		status->Panels->Items[0]->Text = "Unable to allocate memory for file ["+fileName+"]";
		break;
	case -4:
		status->Panels->Items[0]->Text = "Unable to read file data ["+fileName+"]";
		break;
	case -5:
		status->Panels->Items[0]->Text = "Unable to save file ["+fileName+"]";
		break;
	case -6:
		status->Panels->Items[0]->Text = "Unable to write data ["+fileName+"]";
		break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ConvertButtonClick(TObject *Sender)
{
	StatusBar->Panels->Items[0]->Text = "";
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open Ghidra export";
	if(!OpenDialog->Execute()) {
		return;
	}
	AnsiString	inFileName = OpenDialog->FileName;
	OpenDialog->Title = "Open CAL replacement";

	if(CALCheckBox->Checked && !BlockRadioButton->Checked && !OpenDialog->Execute()) {
		return;
	}
	AnsiString	fileCALName = OpenDialog->FileName;
	SaveDialog->DefaultExt = ".bin";
	SaveDialog->Filter = "Bin|*.bin";
	if(!SaveDialog->Execute()) {
		return;
	}
	AnsiString	fileOutName = SaveDialog->FileName;

	int8_t iResult;
	uint32_t iInputSize, iCALSize;
	uint8_t *dInputBin = NULL, *dCALBin = NULL, *dOutBin = NULL;
	hardware_t*	inHardware;

	//open input bin
	dInputBin = loadFile((char*)inFileName.c_str(), MAX_BINSIZE, &iInputSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, inFileName);
		goto cleanup;
	}

	//check simos hw version
	inHardware = getHardwareType(iInputSize);
	if(inHardware == NULL) {
		StatusBar->Panels->Items[0]->Text = "Invalid bin length ["+IntToStr((int)iInputSize)+"]";
		goto cleanup;
	}

	//creating a full bin?
	if(FullRadioButton->Checked) {
		//write full bin
		dOutBin = (BYTE*)malloc(inHardware->bin_size);
		memset(dOutBin, 0, inHardware->bin_size);
		for(int i=0; i<5; i++) {
			memcpy(&dOutBin[inHardware->blocks[i].bin_pos], &dInputBin[inHardware->blocks[i].ghidra_pos], inHardware->blocks[i].length);
		}

		//replace cal
		if(CALCheckBox->Checked) {
			//open input bin
			uint32_t iCALOffset = 0;
			dCALBin = loadFile((char*)fileCALName.c_str(), MAX_BINSIZE, &iCALSize, &iResult);
			if(iResult) {
				setFileResultStatus(StatusBar, iResult, fileCALName);
				goto cleanup;
			}

			bool validBin = false;
			if(iCALSize == inHardware->bin_size) {
				validBin = true;
				iCALOffset = inHardware->blocks[4].bin_pos;
			} else if(iCALSize == inHardware->blocks[4].length) {
				validBin = true;
			}

			if(!validBin) {
				StatusBar->Panels->Items[0]->Text = "Invalid CAL bin type ["+IntToStr((int)iInputSize)+"]";
				goto cleanup;
			}

			if(inHardware != getHardwareType(dCALBin, iCALSize)) {
				StatusBar->Panels->Items[0]->Text = "Hardware types do not match";
				goto cleanup;
			}

			char boxCode = verifyBoxCodes(dOutBin, dCALBin, iCALSize);
			if(boxCode == -2 && ConvertCheckBox->Checked) {
				StatusBar->Panels->Items[0]->Text = "Boxcodes do not match";
				goto cleanup;
			}

			memcpy(&dOutBin[inHardware->blocks[4].bin_pos], &dCALBin[iCALOffset], inHardware->blocks[4].length);
		}

		//write output bin
		uint32_t iSize = inHardware->bin_size;
		int8_t iResult = 0;
		saveFile((char*)fileOutName.c_str(), dOutBin, &iSize, &iResult);
		if(iResult) {
			setFileResultStatus(StatusBar, iResult, fileOutName);
			goto cleanup;
		}
	} else {
		//separate block bins
		for(int i=0; i<5; i++) {
			//write output bin
			uint32_t iSize = inHardware->blocks[i].length;
			int8_t iResult = 0;
			saveFile((char*)(fileOutName+IntToStr(i)).c_str(), &dOutBin[inHardware->blocks[i].ghidra_pos], &iSize, &iResult);
			if(iResult) {
				setFileResultStatus(StatusBar, iResult, fileOutName+IntToStr(i));
				goto cleanup;
			}
		}
	}

	StatusBar->Panels->Items[0]->Text = "Wrote "+AnsiString(inHardware->name)+" bin ["+fileOutName+"]";

cleanup:
	if(dInputBin)
		free(dInputBin);
	if(dCALBin)
		free(dCALBin);
	if(dOutBin)
		free(dOutBin);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::PatchButtonClick(TObject *Sender)
{
	StatusBar->Panels->Items[0]->Text = "";
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	if(CreateRadioButton->Checked) OpenDialog->Title = "Open Original Bin";
	else OpenDialog->Title = "Open Bin";
	if(!OpenDialog->Execute()) {
		return;
	}
	AnsiString inFileName = OpenDialog->FileName;
	if(CreateRadioButton->Checked) {
		OpenDialog->Title = "Open Modified Bin";
	} else {
		OpenDialog->Title = "Open Patch";
		OpenDialog->DefaultExt = ".btp";
		OpenDialog->Filter = "BinToolz Patch|*.btp";
	}
	if(!OpenDialog->Execute()) {
		return;
	}
	AnsiString patchFileName = OpenDialog->FileName;
	if(CreateRadioButton->Checked) {
		SaveDialog->Title = "Save BinToolz Patch";
		SaveDialog->DefaultExt = ".btp";
		SaveDialog->Filter = "BinToolz Patch|*.btp";
	} else {
		SaveDialog->Title = "Save Output Bin";
		SaveDialog->DefaultExt = ".bin";
		SaveDialog->Filter = "Bin|*.bin";
		SaveDialog->FileName = inFileName;
	}

	int8_t iResult;
	uint32_t iInputSize, iPatchSize, iSaveSize;
	uint8_t *dInputBin = NULL, *dPatchBin = NULL;

	//open input bin
	dInputBin = loadFile((char*)inFileName.c_str(), MAX_BINSIZE, &iInputSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, inFileName);
		goto cleanup;
	}

	//open patch bin
	dPatchBin = loadFile((char*)patchFileName.c_str(), MAX_BINSIZE, &iPatchSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, patchFileName);
		goto cleanup;
	}

	hardware_t*	inHardware;
	char		inSoftcode[9];
	inHardware = getHardwareType(dInputBin, iInputSize);
	getSoftcode(inHardware, dInputBin, inSoftcode);

	if(CreateRadioButton->Checked) {
		//error check size of bins
		if(iPatchSize != iInputSize) {
			StatusBar->Panels->Items[0]->Text = "File sizes must match";
			goto cleanup;
		}

		int gapFill;
		try {
			gapFill = StrToInt(GapFillEdit->Text);
		} catch (EConvertError &E) {
			gapFill = 0;
		}

		//create block data
		BYTE*		blockData	= NULL;
		uint32_t 	blockSize	= 0;
		int32_t 	blockCount	= 0;
		int32_t 	patchSize	= 0;
		int32_t		changeStart	= -1;
		for(uint32_t i=0; i<iInputSize; i++) {
			if(changeStart > -1) {
				uint32_t iSize = i - changeStart;
				if(dInputBin[i] == dPatchBin[i]) {
					BYTE n = 0;
					BOOL found_next = false;
					while(n++ < gapFill) {
						if(dInputBin[i+n] != dPatchBin[i+n])
							found_next = true;
					}

					if(!found_next) {
						uint32_t newPatchSize = i-changeStart;
						uint32_t newBlockSize = newPatchSize*2+sizeof(patch_block_t);
						patch_block_t header = {
							changeStart,
							newPatchSize
						};

						uint8_t* newBlockData = (uint8_t*)malloc(blockSize+newBlockSize);
						if(blockData) memcpy(newBlockData, blockData, blockSize);
						memcpy(newBlockData+blockSize, &header, sizeof(patch_block_t));
						blockSize += sizeof(patch_block_t);
						memcpy(newBlockData+blockSize, &dInputBin[changeStart], newPatchSize);
						blockSize += newPatchSize;
						memcpy(newBlockData+blockSize, &dPatchBin[changeStart], newPatchSize);
						blockSize += newPatchSize;
						if(blockData) free(blockData);
						blockData = newBlockData;
						patchSize += newPatchSize;
						blockCount++;
						changeStart = -1;
					}
				}
			} else {
				if(dInputBin[i] != dPatchBin[i])
					changeStart = i;
			}
		}

		//create output header
		free(dPatchBin);
		iSaveSize = blockSize+sizeof(patch_header_t);
		dPatchBin = (uint8_t*)malloc(iSaveSize);
		memset(dPatchBin, 0, iSaveSize);
		patch_header_t *header = (patch_header_t*)dPatchBin;
		strcpy(header->version, BTP_VERSION);
		strcpy(header->soft_code, inSoftcode);
		header->block_checksum	= getChecksum(blockSize, blockData, 0xFFFFFFFF);
		header->block_count		= blockCount;
		header->file_size		= iInputSize;
		memcpy(&dPatchBin[sizeof(patch_header_t)], blockData, blockSize);
		free(blockData);

		//write output patch
		if(!SaveDialog->Execute()) {
			goto cleanup;
		}
		AnsiString outFileName = SaveDialog->FileName;

		//write output bin
		iResult = 0;
		saveFile((char*)outFileName.c_str(), dPatchBin, &iSaveSize, &iResult);
		if(iResult) {
			setFileResultStatus(StatusBar, iResult, outFileName);
			goto cleanup;
		}

		//update statusbar
		StatusBar->Panels->Items[0]->Text = "Created Patch: "+IntToStr(patchSize)+"bytes "+IntToStr(blockCount)+"blocks ["+outFileName+"]";
	} else {
		patch_header_t *header 		= (patch_header_t*)dPatchBin;
		AnsiString btp_version 		= header->version;
		AnsiString soft_code_patch 	= header->soft_code;
		AnsiString soft_code_bin 	= inSoftcode;

		//check file version
		if(btp_version.Trim() != BTP_VERSION) {
			StatusBar->Panels->Items[0]->Text = "Patch file version mismatch ["+btp_version+"]";
			goto cleanup;
		}

		//check file size
		if(iInputSize != header->file_size) {
			StatusBar->Panels->Items[0]->Text = "Incorrect input bin size ["+IntToStr((int32_t)iInputSize)+":"+IntToStr((int32_t)header->file_size)+"]";
			goto cleanup;
		}

		//check software code
		if(soft_code_patch.Trim() != soft_code_bin.Trim()) {
			StatusBar->Panels->Items[0]->Text = "Patch and bin software codes do not match ["+soft_code_patch.Trim()+":"+soft_code_bin.Trim()+"]";
			goto cleanup;
		}

		//check patch checksum
		uint8_t*	block_data = dPatchBin + sizeof(patch_header_t);
		uint32_t	block_checksum = getChecksum(iPatchSize-sizeof(patch_header_t), block_data, 0xFFFFFFFF);
		if(block_checksum != header->block_checksum) {
			StatusBar->Panels->Items[0]->Text = "Invalid patch checksum";
			goto cleanup;
		}

		bool remove_patch = false;
		AnsiString status_str = "Added Patch: ";
		AnsiString bin_str = "Bin is not ready to accept patch";
		if(RemoveRadioButton->Checked) {
			remove_patch = true;
			status_str = "Removed Patch: ";
			AnsiString bin_str = "Bin does not contain patch";
		}

		//check bin is ready to accept patch
		unsigned char *block_data_tmp = block_data;
		for(int i=0; i<header->block_count; i++) {
			patch_block_t *block_header = (patch_block_t*)block_data_tmp;

			if(!checkPatchBlock(iInputSize, dInputBin, block_data_tmp, remove_patch)) {
				StatusBar->Panels->Items[0]->Text = bin_str;
				goto cleanup;
			}

			block_data_tmp += sizeof(patch_block_t) + block_header->length * 2;
		}

		//patch bin data
		int patchSize = 0;
		block_data_tmp = block_data;
		for(int i=0; i<header->block_count; i++) {
			patch_block_t *block_header = (patch_block_t*)block_data_tmp;

			if(!setPatchBlock(iInputSize, dInputBin, block_data_tmp, !remove_patch)) {
				StatusBar->Panels->Items[0]->Text = "Patch block is invalid";
				goto cleanup;
			}

			patchSize += block_header->length;
			block_data_tmp += sizeof(patch_block_t) + block_header->length * 2;
		}

		//write output bin
		if(!SaveDialog->Execute()) {
			return;
		}
		AnsiString outFileName = SaveDialog->FileName;

		//write output bin
		iSaveSize = inHardware->bin_size;
		iResult = 0;
		saveFile((char*)outFileName.c_str(), dInputBin, &iSaveSize, &iResult);
		if(iResult) {
			setFileResultStatus(StatusBar, iResult, outFileName);
			goto cleanup;
		}

		//update statusbar
		StatusBar->Panels->Items[0]->Text = status_str+IntToStr(patchSize)+"bytes "+IntToStr((int32_t)header->block_count)+"blocks ["+outFileName+"]";
	}

cleanup:
	if(dInputBin)
		free(dInputBin);
	if(dPatchBin)
		free(dPatchBin);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ImportButtonClick(TObject *Sender)
{
	StatusBar->Panels->Items[0]->Text = "";
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open ASW Bin";
	if(!OpenDialog->Execute()) {
		return;
	}
	AnsiString	inFileName = OpenDialog->FileName;
	OpenDialog->Title = "Open CAL Bin";
	if(!OpenDialog->Execute()) {
		return;
	}
	AnsiString	fileCALName = OpenDialog->FileName;
	SaveDialog->DefaultExt = ".bin";
	SaveDialog->Filter = "Bin|*.bin";
	if(!SaveDialog->Execute()) {
		return;
	}
	AnsiString fileOutName = SaveDialog->FileName;

	int8_t 		iResult;
	uint32_t 	iInputSize, iCALSize, iSaveSize;
	uint8_t 	*dInputBin = NULL, *dCALBin = NULL;
	FILE* 		fOut;
	uint32_t 	iCALOffset = 0;
	bool 		validBin = false;
	char 		boxCode;

	//open input bin
	dInputBin = loadFile((char*)inFileName.c_str(), MAX_BINSIZE, &iInputSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, inFileName);
		goto cleanup;
	}

	//open CAL bin
	dCALBin = loadFile((char*)fileCALName.c_str(), MAX_BINSIZE, &iCALSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, fileCALName);
		goto cleanup;
	}

	//get input bin hardware type
	hardware_t* inHardware;
	if(iInputSize != simosHW[0].bin_size || !(inHardware = getHardwareType(dInputBin, iInputSize))) {
		StatusBar->Panels->Items[0]->Text = "Invalid input bin type ["+IntToStr((int)iInputSize)+"]";
		goto cleanup;
	}

	//validate bin and get type
	if(iCALSize == inHardware->bin_size) {
		validBin = true;
		iCALOffset = inHardware->blocks[4].bin_pos;
	} else if(iCALSize == inHardware->blocks[4].length) {
		validBin = true;
	}

	if(!validBin) {
		StatusBar->Panels->Items[0]->Text = "Invalid CAL bin type";
		goto cleanup;
	}

	if(inHardware != getHardwareType(dCALBin, iCALSize)) {
		StatusBar->Panels->Items[0]->Text = "Hardware types do not match";
		goto cleanup;
	}

	//check box codes
	boxCode = verifyBoxCodes(dInputBin, dCALBin, iCALSize);
	if(boxCode == -2 && SwapCheckBox->Checked) {
		StatusBar->Panels->Items[0]->Text = "Boxcodes do not match";
		goto cleanup;
	}

	//swap CAL block
	memcpy(&dInputBin[inHardware->blocks[4].bin_pos], &dCALBin[iCALOffset], inHardware->blocks[4].length);

	//write output bin
	iSaveSize = inHardware->bin_size;
	iResult = 0;
	saveFile((char*)fileOutName.c_str(), dInputBin, &iSaveSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, fileOutName);
		goto cleanup;
	}

	StatusBar->Panels->Items[0]->Text = "Wrote "+AnsiString(inHardware->name)+" bin ["+fileOutName+"]";

cleanup:
	if(dInputBin)
		free(dInputBin);
	if(dCALBin)
		free(dCALBin);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ExportButtonClick(TObject *Sender)
{
	StatusBar->Panels->Items[0]->Text = "";
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open FULL Bin";
	if(!OpenDialog->Execute()) {
		return;
	}
	AnsiString	inFileName = OpenDialog->FileName;
	SaveDialog->DefaultExt = ".bin";
	SaveDialog->Filter = "Bin|*.bin";
	if(!SaveDialog->Execute()) {
		return;
	}
	AnsiString fileOutName = SaveDialog->FileName;

	int8_t 		iResult;
	uint32_t 	iInputSize, iSaveSize;
	uint8_t 	*dInputBin = NULL;
	FILE* 		fOut;
	char 		boxCode;

	//open input bin
	dInputBin = loadFile((char*)inFileName.c_str(), MAX_BINSIZE, &iInputSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, inFileName);
		goto cleanup;
	}

	//get input bin hardware type
	hardware_t* inHardware;
	if(iInputSize != simosHW[0].bin_size || !(inHardware = getHardwareType(dInputBin, iInputSize))) {
		StatusBar->Panels->Items[0]->Text = "Invalid input bin type ["+IntToStr((int)iInputSize)+"]";
		goto cleanup;
	}

	//write output bin
	iSaveSize = inHardware->blocks[4].length;
	iResult = 0;
	saveFile((char*)fileOutName.c_str(), &dInputBin[inHardware->blocks[4].bin_pos], &iSaveSize, &iResult);
	if(iResult) {
		setFileResultStatus(StatusBar, iResult, fileOutName);
		goto cleanup;
	}

	StatusBar->Panels->Items[0]->Text = "Wrote "+AnsiString(inHardware->name)+" bin ["+fileOutName+"]";

cleanup:
	if(dInputBin)
		free(dInputBin);
}
//---------------------------------------------------------------------------


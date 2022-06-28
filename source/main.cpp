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
hardware_t* getHardwareType(uint32_t fSize)
{
	//determine simos version
	for(uint32_t i=0; i<HW_COUNT; i++) {
		if(fSize == simosHW[i].ghidra_size) {
			return &simosHW[i];
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
hardware_t* getHardwareType(uint8_t* fData)
{
	//determine simos version
	for(uint32_t i=0; i<HW_COUNT; i++) {
		int8_t box_code[12];
		memset(box_code, 0, 12);
		memcpy(box_code, &fData[simosHW[i].box_code_start], 11);
		if(AnsiString(box_code).Length() != 0)
			return &simosHW[i];
	}

	return NULL;
}
//---------------------------------------------------------------------------
void getSoftcode(hardware_t* hw, uint8_t* fData, uint8_t* code)
{
	memset(code, 0, 9);
	memcpy(code, &fData[hw->soft_code_start], 8);
}
//---------------------------------------------------------------------------
void getBoxcode(hardware_t* hw, uint8_t* fData, uint8_t* code)
{
	memset(code, 0, 12);
	memcpy(code, &fData[hw->box_code_start], 11);
}
//---------------------------------------------------------------------------
int8_t verifyBoxCodes(uint8_t* fData1, uint8_t* fData2)
{
	hardware_t* hw = getHardwareType(fData1);

	if(hw != getHardwareType(fData2))
		return -1;

	uint8_t box_code1[12];
	memset(box_code1, 0, 12);
	memcpy(box_code1, &fData1[hw->box_code_start], 11);

	uint8_t box_code2[12];
	memset(box_code2, 0, 12);
	memcpy(box_code2, &fData2[hw->box_code_start], 11);

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
void __fastcall TMainForm::ConvertButtonClick(TObject *Sender)
{
	StatusBar->Panels->Items[0]->Text = "";
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open Ghidra export";
	if(OpenDialog->Execute()) {
		AnsiString	fileInName = OpenDialog->FileName;
		OpenDialog->Title = "Open CAL replacement";
		if(!CALCheckBox->Checked || BlockRadioButton->Checked || OpenDialog->Execute()) {
			AnsiString	fileCALName = OpenDialog->FileName;
			SaveDialog->DefaultExt = ".bin";
			SaveDialog->Filter = "Bin|*.bin";
			if(SaveDialog->Execute()) {
				//open input bin
				FILE* fIn = fopen((char*)fileInName.c_str(), "rb");
				if(fIn == NULL) {
					StatusBar->Panels->Items[0]->Text = "Unable to open file ["+fileInName+"]";
					return;
				}

				//read input bin
				BYTE* dataIn = (BYTE*)malloc(simosHW[0].bin_size);
				uint32_t inSize = fread(dataIn, sizeof(BYTE), simosHW[0].bin_size, fIn);
				fclose(fIn);

				hardware_t* currentHardware = getHardwareType(inSize);
				if(currentHardware == NULL) {
					StatusBar->Panels->Items[0]->Text = "Invalid bin length ["+IntToStr((int)inSize)+"]";
					free(dataIn);
					return;
				}

				AnsiString	fileOutName = SaveDialog->FileName;
				if(FullRadioButton->Checked) {
					//write full bin
					BYTE* dataOut = (BYTE*)malloc(currentHardware->bin_size);
					memset(dataOut, 0, currentHardware->bin_size);
					for(int i=0; i<5; i++) {
						memcpy(&dataOut[currentHardware->blocks[i].bin_pos], &dataIn[currentHardware->blocks[i].ghidra_pos], currentHardware->blocks[i].length);
					}
					free(dataIn);

					//replace cal
					if(CALCheckBox->Checked) {
						//open cal bin
						FILE* fCAL = fopen((char*)fileCALName.c_str(), "rb");
						if(fIn == NULL) {
							StatusBar->Panels->Items[0]->Text = "Unable to open file ["+fileInName+"]";
							free(dataOut);
							return;
						}

						//read cal bin
						BYTE* dataCAL = (BYTE*)malloc(simosHW[0].bin_size);
						inSize = fread(dataCAL, sizeof(BYTE), simosHW[0].bin_size, fCAL);
						fclose(fCAL);

						bool validBin = false;
						if(inSize == currentHardware->bin_size) {
							validBin = true;
						} else if(inSize == currentHardware->blocks[4].length) {
							memcpy(&dataCAL[currentHardware->blocks[4].bin_pos], dataCAL, currentHardware->blocks[4].length);
							validBin = true;
						}

						if(!validBin || currentHardware != getHardwareType(dataCAL)) {
							StatusBar->Panels->Items[0]->Text = "Invalid CAL bin type ["+IntToStr((int)inSize)+"]";
							free(dataOut);
							free(dataCAL);
							return;
						}

						char boxCode = verifyBoxCodes(dataCAL, dataOut);
						if(boxCode == -2 && ConvertCheckBox->Checked) {
							StatusBar->Panels->Items[0]->Text = "Boxcodes do not match ["+IntToStr((int)inSize)+"]";
							free(dataOut);
							free(dataCAL);
							return;
						}

						if(boxCode == -1) {
							StatusBar->Panels->Items[0]->Text = "Hardware types do not match ["+IntToStr((int)inSize)+"]";
							free(dataOut);
							free(dataCAL);
							return;
						}

						memcpy(&dataOut[currentHardware->blocks[4].bin_pos], &dataCAL[currentHardware->blocks[4].bin_pos], currentHardware->blocks[4].length);
						free(dataCAL);
					}

					//write output bin
					FILE* fOut = fopen((char*)fileOutName.c_str(), "wb");
					if(fOut == NULL) {
						StatusBar->Panels->Items[0]->Text = "Unable to save file ["+fileOutName+"]";
						free(dataOut);
						return;
					}
					fwrite(dataOut, 1, currentHardware->bin_size, fOut);
					fclose(fOut);
					free(dataOut);
				} else {
					//separate block bins
					for(int i=0; i<5; i++) {
						FILE* fOut = fopen((char*)(fileOutName+IntToStr(i)).c_str(), "wb");
						if(fOut == NULL) {
							StatusBar->Panels->Items[0]->Text = "Unable to save file ["+fileOutName+"]";
							free(dataIn);
							return;
						}
						fwrite(&dataIn[currentHardware->blocks[i].ghidra_pos], 1, currentHardware->blocks[i].length, fOut);
						fclose(fOut);
					}
					free(dataIn);
				}

				StatusBar->Panels->Items[0]->Text = "Wrote "+AnsiString(currentHardware->name)+" bin ["+fileOutName+"]";
			}
		}
	}
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
	if(!SaveDialog->Execute()) {
		return;
	}
	AnsiString outFileName = SaveDialog->FileName;

	//open input bin
	FILE* fInputBin = fopen((char*)inFileName.c_str(), "rb");
	if(fInputBin == NULL) {
		StatusBar->Panels->Items[0]->Text = "Unable to open file ["+inFileName+"]";
		return;
	}

	//open patch bin
	FILE* fPatchBin = fopen((char*)patchFileName.c_str(), "rb");
	if(fPatchBin == NULL) {
		StatusBar->Panels->Items[0]->Text = "Unable to open file ["+patchFileName+"]";
		fclose(fInputBin);
		return;
	}

	//check size
	fseek(fInputBin, 0L, SEEK_END);
	int iInputSize = ftell(fInputBin);
	rewind(fInputBin);
	fseek(fPatchBin, 0L, SEEK_END);
	int iPatchSize = ftell(fPatchBin);
	rewind(fPatchBin);

	//error check size of bins
	if(CreateRadioButton->Checked) {
		if(iPatchSize != iInputSize) {
			StatusBar->Panels->Items[0]->Text = "File sizes must match";
			fclose(fInputBin);
			fclose(fPatchBin);
			return;
		}
	}

	//read bin data
	BYTE* dInputBin = (BYTE*)malloc(iInputSize);
	fread(dInputBin, sizeof(BYTE), iInputSize, fInputBin);
	fclose(fInputBin);
	BYTE* dPatchBin = (BYTE*)malloc(iPatchSize);
	fread(dPatchBin, sizeof(BYTE), iPatchSize, fPatchBin);
	fclose(fPatchBin);

	hardware_t*	inHardware;
	char		inSoftcode[9];
	inHardware = getHardwareType(dInputBin);
	getSoftcode(inHardware, dInputBin, inSoftcode);

	if(CreateRadioButton->Checked) {
		int gapFill;
		try {
			gapFill = StrToInt(GapFillEdit->Text);
		} catch (EConvertError &E) {
			gapFill = 0;
		}

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

		//write output patch
		FILE* fOutPatch = fopen((char*)outFileName.c_str(), "wb");
		if(fOutPatch == NULL) {
			StatusBar->Panels->Items[0]->Text = "Unable to save file ["+outFileName+"]";
			free(blockData);
			free(dInputBin);
			free(dPatchBin);
			return;
		}
		patch_header_t header = {
			BTP_VERSION,
			"",
			0,
			0
		};
		strcpy(header.soft_code, inSoftcode);
		header.block_checksum	= getChecksum(blockSize, blockData, 0xFFFFFFFF);
		header.block_count		= blockCount;
		header.file_size		= iInputSize;
		fwrite(&header, 1, sizeof(patch_header_t), fOutPatch);
		fwrite(blockData, 1, blockSize, fOutPatch);
        fclose(fOutPatch);

		//cleanup
		free(blockData);
		free(dInputBin);
		free(dPatchBin);

		//update statusbar
		StatusBar->Panels->Items[0]->Text = "Created Patch: "+IntToStr(patchSize)+"bytes "+IntToStr(blockCount)+"blocks ["+outFileName+"]";
	} else {
		patch_header_t *header = (patch_header_t*)dPatchBin;
		AnsiString btp_version = header->version;
		AnsiString soft_code_patch = header->soft_code;
		AnsiString soft_code_bin = inSoftcode;

		//check file version
		if(btp_version.Trim() != BTP_VERSION) {
			free(dInputBin);
			free(dPatchBin);
			StatusBar->Panels->Items[0]->Text = "Patch file version mismatch ["+btp_version+"]";
			return;
		}

		//check file size
		if(iInputSize != header->file_size) {
			free(dInputBin);
			free(dPatchBin);
			StatusBar->Panels->Items[0]->Text = "Incorrect input bin size ["+IntToStr(iInputSize)+":"+IntToStr((int32_t)header->file_size)+"]";
			return;
		}

		//check software code
		if(soft_code_patch.Trim() != soft_code_bin.Trim()) {
			free(dInputBin);
			free(dPatchBin);
			StatusBar->Panels->Items[0]->Text = "Patch and bin software codes do not match ["+AnsiString(inSoftcode)+":"+AnsiString(header->soft_code)+"]";
			return;
		}

		//check patch checksum
		uint8_t*	block_data = dPatchBin + sizeof(patch_header_t);
		uint32_t	block_checksum = getChecksum(iPatchSize-sizeof(patch_header_t), block_data, 0xFFFFFFFF);
		if(block_checksum != header->block_checksum) {
			free(dInputBin);
			free(dPatchBin);
			StatusBar->Panels->Items[0]->Text = "Invalid patch checksum";
			return;
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
				free(dInputBin);
				free(dPatchBin);

				StatusBar->Panels->Items[0]->Text = bin_str;
				return;
			}

			block_data_tmp += sizeof(patch_block_t) + block_header->length * 2;
		}

		//patch bin data
		int patchSize = 0;
		block_data_tmp = block_data;
		for(int i=0; i<header->block_count; i++) {
			patch_block_t *block_header = (patch_block_t*)block_data_tmp;

			if(!setPatchBlock(iInputSize, dInputBin, block_data_tmp, !remove_patch)) {
				free(dInputBin);
				free(dPatchBin);
				StatusBar->Panels->Items[0]->Text = "Patch block is invalid";
				return;
			}

			patchSize += block_header->length;
			block_data_tmp += sizeof(patch_block_t) + block_header->length * 2;
		}

		//write output bin
		FILE* fOutBin = fopen((char*)outFileName.c_str(), "wb");
		if(fOutBin == NULL) {
			StatusBar->Panels->Items[0]->Text = "Unable to save file ["+outFileName+"]";
			free(dInputBin);
			free(dPatchBin);
			return;
		}
		fwrite(dInputBin, sizeof(unsigned char), iInputSize, fOutBin);
		fclose(fOutBin);

		//cleen up
		free(dInputBin);
		free(dPatchBin);

		//update statusbar
		StatusBar->Panels->Items[0]->Text = status_str+IntToStr(patchSize)+"bytes "+IntToStr((int32_t)header->block_count)+"blocks ["+outFileName+"]";
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::SwapButtonClick(TObject *Sender)
{
	StatusBar->Panels->Items[0]->Text = "";
	OpenDialog->DefaultExt = ".bin";
	OpenDialog->Filter = "Bin|*.bin";
	OpenDialog->Title = "Open ASW Bin";
	if(OpenDialog->Execute()) {
		AnsiString	fileInName = OpenDialog->FileName;
		OpenDialog->Title = "Open CAL Bin";
		if(OpenDialog->Execute()) {
			AnsiString	fileCALName = OpenDialog->FileName;
			SaveDialog->DefaultExt = ".bin";
			SaveDialog->Filter = "Bin|*.bin";
			if(SaveDialog->Execute()) {
				//open input bin
				FILE* fIn = fopen((char*)fileInName.c_str(), "rb");
				if(fIn == NULL) {
					StatusBar->Panels->Items[0]->Text = "Unable to open file ["+fileInName+"]";
					return;
				}

				//read input bin
				BYTE* dataIn = (BYTE*)malloc(simosHW[0].bin_size);
				uint32_t inSize = fread(dataIn, sizeof(BYTE), simosHW[0].bin_size, fIn);
				fclose(fIn);

				hardware_t* currentHardware;
				if(inSize != simosHW[0].bin_size || !(currentHardware = getHardwareType(dataIn))) {
					StatusBar->Panels->Items[0]->Text = "Invalid input bin type ["+IntToStr((int)inSize)+"]";
					free(dataIn);
					return;
				}

				//open cal bin
				FILE* fCAL = fopen((char*)fileCALName.c_str(), "rb");
				if(fIn == NULL) {
					StatusBar->Panels->Items[0]->Text = "Unable to open file ["+fileInName+"]";
					free(dataIn);
					return;
				}

				//read cal bin
				BYTE* dataCAL = (BYTE*)malloc(simosHW[0].bin_size);
				inSize = fread(dataCAL, sizeof(BYTE), simosHW[0].bin_size, fCAL);
				fclose(fCAL);

				bool validBin = false;
				if(inSize == currentHardware->bin_size) {
					validBin = true;
				} else if(inSize == currentHardware->blocks[4].length) {
					memcpy(&dataCAL[currentHardware->blocks[4].bin_pos], dataCAL, currentHardware->blocks[4].length);
					validBin = true;
				}

				if(!validBin || currentHardware != getHardwareType(dataCAL)) {
					StatusBar->Panels->Items[0]->Text = "Invalid CAL bin type ["+IntToStr((int)inSize)+"]";
					free(dataIn);
					free(dataCAL);
					return;
				}

				char boxCode = verifyBoxCodes(dataCAL, dataIn);
				if(boxCode == -2 && SwapCheckBox->Checked) {
					StatusBar->Panels->Items[0]->Text = "Boxcodes do not match ["+IntToStr((int)inSize)+"]";
					free(dataIn);
					free(dataCAL);
					return;
				}

				if(boxCode == -1) {
					StatusBar->Panels->Items[0]->Text = "Hardware types do not match ["+IntToStr((int)inSize)+"]";
					free(dataIn);
					free(dataCAL);
					return;
				}

				memcpy(&dataIn[currentHardware->blocks[4].bin_pos], &dataCAL[currentHardware->blocks[4].bin_pos], currentHardware->blocks[4].length);

				//write output bin
				AnsiString fileOutName = SaveDialog->FileName;
				FILE* fOut = fopen((char*)fileOutName.c_str(), "wb");
				if(fOut == NULL) {
					StatusBar->Panels->Items[0]->Text = "Unable to save file ["+fileOutName+"]";
					free(dataIn);
					free(dataCAL);
					return;
				}
				fwrite(dataIn, 1, currentHardware->bin_size, fOut);

				fclose(fOut);
				free(dataIn);
				free(dataCAL);

				StatusBar->Panels->Items[0]->Text = "Wrote "+AnsiString(currentHardware->name)+" bin ["+fileOutName+"]";
			}
		}
	}
}
//---------------------------------------------------------------------------


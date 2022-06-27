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
#define HW_COUNT	2

typedef struct {
	unsigned int ghidra_pos;
	unsigned int bin_pos;
	unsigned int length;
} block_t;

typedef struct {
	char			name[15];
	unsigned int	box_code_start;
	unsigned int	box_code_end;
	unsigned int	soft_code_start;
	unsigned int	soft_code_end;
	unsigned int	bin_size;
	unsigned int	ghidra_size;
	block_t			blocks[5];
} hardware_t;

typedef struct {
	char			version[20];
	char			soft_code[8];
	unsigned int	block_count;
	unsigned int	block_checksum;
} patch_header_t;

typedef struct {
	unsigned int	offset;
	unsigned int	length;
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
hardware_t* getHardwareType(unsigned int fSize)
{
	//determine simos version
	for(int i=0; i<HW_COUNT; i++) {
		if(fSize == simosHW[i].ghidra_size) {
			return &simosHW[i];
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
hardware_t* getHardwareType(unsigned char* fData)
{
	//determine simos version
	for(int i=0; i<HW_COUNT; i++) {
		char box_code[12];
		memset(box_code, 0, 12);
		memcpy(box_code, &fData[simosHW[i].box_code_start], 11);
		if(AnsiString(box_code).Length() != 0)
			return &simosHW[i];
	}

	return NULL;
}
//---------------------------------------------------------------------------
void getSoftcode(hardware_t* hw, unsigned char* fData, unsigned char* code)
{
	memset(code, 0, 9);
	memcpy(code, &fData[hw->soft_code_start], 8);
}
//---------------------------------------------------------------------------
void getBoxcode(hardware_t* hw, unsigned char* fData, unsigned char* code)
{
	memset(code, 0, 12);
	memcpy(code, &fData[hw->box_code_start], 11);
}
//---------------------------------------------------------------------------
char verifyBoxCodes(unsigned char* fData1, unsigned char* fData2)
{
	hardware_t* hw = getHardwareType(fData1);

	if(hw != getHardwareType(fData2))
		return -1;

	char box_code1[12];
	memset(box_code1, 0, 12);
	memcpy(box_code1, &fData1[hw->box_code_start], 11);

	char box_code2[12];
	memset(box_code2, 0, 12);
	memcpy(box_code2, &fData2[hw->box_code_start], 11);

	if(strcmp(box_code1, box_code2))
		return -2;

	return 1;
}
//---------------------------------------------------------------------------
void writePatchBlock(unsigned int start, unsigned int end, FILE* file, unsigned char* data_in, unsigned char* data_patch, unsigned int *checksum)
{
	unsigned int patch_count = end-start;
	unsigned int patch_size = patch_count*2+sizeof(patch_block_t);
	patch_block_t header = {
		start,
		patch_count
	};
	BYTE* pData = (BYTE*)malloc(patch_size);
	memset(pData, 0, patch_size);
	memcpy(pData, &header, sizeof(patch_block_t));
	memcpy(pData+sizeof(patch_block_t), &data_in[start], patch_count);
	memcpy(pData+sizeof(patch_block_t)+patch_count, &data_patch[start], patch_count);
	fwrite(pData, 1, patch_size, file);

	//update checksum
	for(int i=0; i<patch_size; i++) {
		*checksum += pData[i];
	}

	free(pData);
}
//---------------------------------------------------------------------------
bool checkPatchBlock(unsigned int data_len, unsigned char* data_in, unsigned char *data_block, bool removing)
{
	patch_block_t *header = (patch_block_t*)data_block;
	if(header->offset+header->length > data_len)
		return false;
	data_block += sizeof(patch_block_t) + (removing*header->length);
	data_in += header->offset;
	for(int i=0; i<header->length; i++) {
		if(data_in[i] != data_block[i])
			return false;
	}
	return true;
}
//---------------------------------------------------------------------------
bool setPatchBlock(unsigned int data_len, unsigned char* data_in, unsigned char *data_block, bool adding)
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
unsigned int getChecksum(unsigned int data_len, unsigned char* data_in)
{
	unsigned int checksum = 0;
	for(int i=0; i<data_len; i++) {
		checksum += data_in[i];
	}
	return checksum;
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
				unsigned int inSize = fread(dataIn, sizeof(BYTE), simosHW[0].bin_size, fIn);
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
		//write output patch header
		FILE* fOutPatch = fopen((char*)outFileName.c_str(), "wb");
		if(fOutPatch == NULL) {
			StatusBar->Panels->Items[0]->Text = "Unable to save file ["+outFileName+"]";
			free(dInputBin);
			free(dPatchBin);
			return;
		}
		patch_header_t header = {
			"BinToolz Patch v1.0",
			"",
			0,
			0
		};
		strcpy(header.soft_code, inSoftcode);
		fwrite(&header, 1, sizeof(patch_header_t), fOutPatch);

		int gapFill;
		try {
			gapFill = StrToInt(GapFillEdit->Text);
		} catch (EConvertError &E) {
			gapFill = 0;
		}
		int blockCount = 0;
		int patchSize = 0;
		int changeStart = -1;
		unsigned int blockChecksum = 0;
		for(unsigned int i=0; i<iInputSize; i++) {
			if(changeStart > -1) {
				unsigned int iSize = i - changeStart;
				if(dInputBin[i] == dPatchBin[i]) {
					BYTE n = 0;
					BOOL found_next = false;
					while(n++ < gapFill) {
						if(dInputBin[i+n] != dPatchBin[i+n])
							found_next = true;
					}

					if(!found_next) {
						writePatchBlock(changeStart, i, fOutPatch, dInputBin, dPatchBin, &blockChecksum);
						patchSize += iSize;
						blockCount++;
						changeStart = -1;
					}
				} else if(iSize >= 1024) {
					writePatchBlock(changeStart, i, fOutPatch, dInputBin, dPatchBin, &blockChecksum);
					patchSize += iSize;
					blockCount++;
					changeStart = i;
				}
			} else {
				if(dInputBin[i] != dPatchBin[i])
					changeStart = i;
			}
		}

		//write header
		rewind(fOutPatch);
		header.block_count = blockCount;
		header.block_checksum = blockChecksum;
		fwrite(&header, 1, sizeof(patch_header_t), fOutPatch);

		//close patch and delete bin data
		fclose(fOutPatch);
		free(dInputBin);
		free(dPatchBin);

		StatusBar->Panels->Items[0]->Text = "Created Patch: "+IntToStr(patchSize)+"bytes ["+outFileName+"]";
	} else {
		patch_header_t *header = (patch_header_t*)dPatchBin;
		AnsiString soft_code_patch = header->soft_code;
		AnsiString soft_code_bin = inSoftcode;

		//check software code
		if(soft_code_patch.Trim() != soft_code_bin.Trim()) {
			free(dInputBin);
			free(dPatchBin);
			StatusBar->Panels->Items[0]->Text = "Patch and bin software codes do not match ["+AnsiString(header->soft_code)+":"+AnsiString(inSoftcode)+"]";
			return;
		}

		//check patch checksum
		unsigned char *block_data = dPatchBin + sizeof(patch_header_t);
		unsigned int  block_checksum = getChecksum(iPatchSize-sizeof(patch_header_t), block_data);
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

		//close patch and delete bin data
		fclose(fOutBin);
		free(dInputBin);
		free(dPatchBin);

		StatusBar->Panels->Items[0]->Text = status_str+IntToStr(patchSize)+"bytes ["+outFileName+"]";
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
				unsigned int inSize = fread(dataIn, sizeof(BYTE), simosHW[0].bin_size, fIn);
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


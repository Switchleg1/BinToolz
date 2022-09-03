//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "Simos.h"
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
	},
	{
		"DQ250",
		0x4FFB0,
		0x4FFBB,
		0x4FFE0,
		0x4FFE4,
		1572864,
		0x0,
		{
			{0x0,			0x0,		0x0},
			{0x0,			0x0,		0x0},
			{0x0,			0x0,		0x00080E},
			{0x0,			0x50000,	0x130000},
			{0x0,			0x30000,	0x020000}
		}
	},
	{
		"DQ250",
		0x3FFB0,
		0x3FFBB,
		0x3FFE0,
		0x3FFE4,
		1572864,
		0x0,
		{
			{0x0,			0x0,		0x0},
			{0x0,			0x0,		0x0},
			{0x0,			0x0,		0x00080E},
			{0x0,			0x40000,	0x130000},
			{0x0,			0x20000,	0x020000}
		}
	}
};
//---------------------------------------------------------------------------
SimosBIN::SimosBIN()
{
	pData = NULL;
	iSize = 0;
}
//---------------------------------------------------------------------------
SimosBIN::~SimosBIN()
{
	clear();
}
//---------------------------------------------------------------------------
int32_t SimosBIN::load(int8_t* fileName)
{
	if(pData) {
		return -1;
	}

	FILE* file = fopen(fileName, "rb");
	if(file == NULL) {
		return -2;
	}
	fseek(file, 0L, SEEK_END);
	uint32_t size = ftell(file);
	if(size > MAX_BINSIZE) {
		fclose(file);
		return -3;
	}
	rewind(file);
	uint8_t* data = (uint8_t*)malloc(size);
	if(data == NULL) {
		fclose(file);
		return -4;
	}
	if(fread(data, sizeof(uint8_t), size, file) != size) {
		fclose(file);
		free(data);
		return -5;
	}
	fclose(file);

	pData = data;
	iSize = size;

	return 0;
}
//---------------------------------------------------------------------------
int32_t SimosBIN::save(int8_t* fileName)
{
	return save(fileName, 0xFF);
}
//---------------------------------------------------------------------------
int32_t SimosBIN::save(int8_t* fileName, uint8_t block)
{
	FILE* file = fopen(fileName, "wb");
	if(file == NULL) {
		return -6;
	}

	uint32_t start = 0, length = iSize;
	if(block < 5) {
		hardware_t* hw = hardwareType();
		if(hw == NULL) {
			return -7;
		}

		start = hw->blocks[block].bin_pos;
		length = hw->blocks[block].length;
	}

	uint32_t size = fwrite(&pData[start], sizeof(uint8_t), length, file);
	if(size != length) {
		fclose(file);
		return -8;
	}
	fclose(file);

	return 0;
}
//---------------------------------------------------------------------------
int32_t SimosBIN::create(uint32_t size)
{
	if(pData) {
		return -9;
	}

	uint8_t* data = (uint8_t*)malloc(size);
	if(data == NULL) {
		return -10;
	}
	memset(data, 0, size);

	pData = data;
	iSize = size;

	return 0;
}
//---------------------------------------------------------------------------
int32_t SimosBIN::clear()
{
	if(pData) {
		free(pData);
		pData	= NULL;
		iSize	= 0;
		return 0;
	}

	return -1;
}
//---------------------------------------------------------------------------
hardware_t* SimosBIN::hardwareType()
{
	//determine simos version
	for(uint32_t i=0; i<HW_COUNT; i++) {
		if(iSize == simosHW[i].bin_size) {
			int8_t box_code[12];
			memset(box_code, 0, 12);
			memcpy(box_code, &pData[simosHW[i].box_code_start], 11);
			if(strlen(box_code) != 0)
				return &simosHW[i];
		} else if(iSize == simosHW[i].blocks[4].length) {
        	int8_t box_code[12];
			memset(box_code, 0, 12);
			memcpy(box_code, &pData[simosHW[i].box_code_start-simosHW[i].blocks[4].length], 11);
			if(strlen(box_code) != 0)
				return &simosHW[i];
		} else if(iSize == simosHW[i].ghidra_size) {
			for(uint32_t i=0; i<HW_COUNT; i++) {
				if(iSize == simosHW[i].blocks[4].length) {
					return &simosHW[i];
				} else if(iSize == simosHW[i].ghidra_size) {
					return &simosHW[i];
				}
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
int8_t* SimosBIN::softwareCode()
{
	if(pData) {
		hardware_t* hw = hardwareType();
		if(hw) {
			return &pData[hw->soft_code_start];
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
int8_t* SimosBIN::boxCode()
{
	if(pData) {
		hardware_t* hw = hardwareType();
		if(hw) {
			uint32_t offset = 0;
			if(iSize == hw->blocks[4].length) {
				offset = hw->blocks[4].bin_pos;
			}

			return &pData[hw->box_code_start-offset];
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------
int32_t SimosBIN::verifyBoxCode(SimosBIN* bin)
{
	hardware_t* hw = hardwareType();
	if(hw != bin->hardwareType()) {
		return -1;
	}

	try {
		uint8_t box_code1[12];
		memset(box_code1, 0, 12);
		memcpy(box_code1, boxCode(), 11);

		uint8_t box_code2[12];
		memset(box_code2, 0, 12);
		memcpy(box_code2, bin->boxCode(), 11);

		if(strcmp(box_code1, box_code2))
			return -2;
	} catch (char* E) {
		return -3;
	}

	return 1;
}
//---------------------------------------------------------------------------
int32_t SimosBIN::swapBlock(SimosBIN* bin, uint8_t block)
{
	if(block < 5) {
		hardware_t* hw = hardwareType();
		if(hw && iSize == hw->bin_size) {
			if(bin->hardwareType() && strstr(hw->name, bin->hardwareType()->name)) {
				if(bin->data(block)) {
					memcpy(data(block), bin->data(block), hw->blocks[block].length);
					return 0;
				}
				return -1;
			}
			return -2;
		}
	}
	return -3;
}
//---------------------------------------------------------------------------
uint8_t* SimosBIN::data()
{
	return pData;
}
//---------------------------------------------------------------------------
uint8_t* SimosBIN::data(uint8_t block)
{
	if(block < 5) {
		hardware_t* hw = hardwareType();
		if(hw && iSize == hw->bin_size) {
			return &pData[hw->blocks[block].bin_pos];
		}
	}

	return pData;
}
//---------------------------------------------------------------------------
uint32_t SimosBIN::size()
{
	return iSize;
}
//---------------------------------------------------------------------------

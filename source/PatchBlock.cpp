//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PatchBlock.h"
//---------------------------------------------------------------------------
uint32_t getChecksum(uint32_t data_len, uint8_t* data_in, uint32_t crc)
{
	uint32_t i,j;
	for(i=0;i<data_len;i++) {
		uint8_t ch=data_in[i];
		for(j=0;j<8;j++) {
			uint32_t b=(ch^crc)&1;
			crc>>=1;
			if(b) crc=crc^0xEDB88320;
			ch>>=1;
		}
	}

	return ~crc;
}
//---------------------------------------------------------------------------
uint8_t checkPatchBlock(uint32_t data_len, uint8_t* data_in, uint8_t* data_block, uint8_t removing)
{
	uint32_t i;
	patch_block_t *header = (patch_block_t*)data_block;
	if(header->offset+header->length > data_len)
		return FALSE;
	data_block += sizeof(patch_block_t) + (removing*header->length);
	data_in += header->offset;
	for(i=0; i<header->length; i++) {
		if(data_in[i] != data_block[i])
			return FALSE;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
uint8_t setPatchBlock(uint32_t data_len, uint8_t* data_in, uint8_t* data_block, uint8_t adding)
{
	uint32_t i;
	patch_block_t *header = (patch_block_t*)data_block;
	if(header->offset+header->length > data_len)
		return FALSE;
	data_block += sizeof(patch_block_t) + (adding*header->length);
	data_in += header->offset;
	for(i=0; i<header->length; i++) {
		data_in[i] = data_block[i];
	}
	return TRUE;
}
//---------------------------------------------------------------------------
uint8_t* addPatchBlock(uint32_t new_len, uint32_t new_offset, uint8_t* data_in, uint8_t* data_patch, uint8_t* data_block, uint32_t* block_len)
{
	uint32_t new_size = new_len*2+sizeof(patch_block_t);
	patch_block_t header = {
		new_offset,
		new_len
	};

	uint8_t* new_data = (uint8_t*)malloc(*block_len+new_size);
	if(data_block) memcpy(new_data, data_block, *block_len);
	memcpy(new_data+*block_len, &header, sizeof(patch_block_t));
	*block_len += sizeof(patch_block_t);
	memcpy(new_data+*block_len, &data_in[new_offset], new_len);
	*block_len += new_len;
	memcpy(new_data+*block_len, &data_patch[new_offset], new_len);
	*block_len += new_len;
	if(data_block) free(data_block);

	return new_data;
}
//---------------------------------------------------------------------------

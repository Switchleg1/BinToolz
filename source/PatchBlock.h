#ifndef PATCHBLOCK_H
#define PATCHBLOCK_H
//---------------------------------------------------------------------------
#include "Constants.h"
//---------------------------------------------------------------------------
typedef struct {
	int8_t		version[20];
	int8_t		soft_code[8];
	uint32_t	block_count;
	uint32_t	block_checksum;
	uint32_t	file_size;
	uint8_t		space[60];
} patch_header_t;
//---------------------------------------------------------------------------
typedef struct {
	uint32_t	offset;
	uint32_t	length;
} patch_block_t;
//---------------------------------------------------------------------------
uint32_t	getChecksum(uint32_t data_len, uint8_t* data_in, uint32_t crc);
uint8_t		checkPatchBlock(uint32_t data_len, uint8_t* data_in, uint8_t* data_block, uint8_t removing);
uint8_t		setPatchBlock(uint32_t data_len, uint8_t* data_in, uint8_t* data_block, uint8_t adding);
uint8_t*	addPatchBlock(uint32_t new_len, uint32_t new_offset, uint8_t* data_in, uint8_t* data_patch, uint8_t* data_block, uint32_t* block_len);
//---------------------------------------------------------------------------

#endif

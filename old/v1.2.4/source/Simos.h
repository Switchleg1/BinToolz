#ifndef SIMOS_H
#define SIMOS_H
//---------------------------------------------------------------------------
#include "Constants.h"
//---------------------------------------------------------------------------
typedef struct {
	uint32_t	ghidra_pos;
	uint32_t	bin_pos;
	uint32_t	length;
} block_t;
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
extern hardware_t  simosHW[HW_COUNT];
//---------------------------------------------------------------------------
class SimosBIN {
public:
	SimosBIN();
	~SimosBIN();

	int32_t load(int8_t* fileName);
	int32_t save(int8_t* fileName);
	int32_t save(int8_t* fileName, uint8_t block);
	int32_t create(uint32_t size);
	int32_t clear();

	hardware_t* hardwareType();
	int8_t*		softwareCode();
	int8_t*		boxCode();
	int32_t		verifyBoxCode(SimosBIN* bin);
	int32_t		swapBlock(SimosBIN* bin, uint8_t block);
	uint8_t*	data();
	uint8_t*	data(uint8_t block);
	uint32_t	size();

private:
	uint8_t*	pData;
	uint32_t	iSize;
};
//---------------------------------------------------------------------------
#endif

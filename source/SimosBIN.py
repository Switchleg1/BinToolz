from Return import ReturnType
import sys


class SimosBlock:
    def __init__(self, ghidraPosition, binPosition, length):
        self.ghidraPosition	= ghidraPosition
        self.binPosition	= binPosition
        self.length			= length

class SimosHardware:
    def __init__(self, boxCodeStart, boxCodeEnd, softCodeStart, softCodeEnd, binSize, ghidraSize, blocks):
        self.boxCodeStart		= boxCodeStart
        self.boxCodeEnd			= boxCodeEnd
        self.softCodeStart		= softCodeStart
        self.softCodeEnd		= softCodeEnd
        self.binSize			= binSize
        self.ghidraSize			= ghidraSize
        self.blocks				= blocks

simosHW = {
    "Simos 18.1": SimosHardware(
        0x00200060,
		0x0020006b,
		0x00200023,
		0x0020002B,
		4194304,
		0x002E2E00,
		[
            SimosBlock(0x0,			0x0001c000,	    0x00023E00),
		    SimosBlock(0x00023E00, 	0x00040000,	    0x000FFC00),
		    SimosBlock(0x00123A00, 	0x00140000,	    0x000BFC00),
		    SimosBlock(0x001E3600, 	0x00280000,	    0x0007FC00),
		    SimosBlock(0x00263200, 	0x00200000,	    0x0007FC00)
        ]
    ),
    "Simos 18.10": SimosHardware(
		0x00220060,
		0x0022006b,
		0x00220023,
		0x0022002B,
		4194304,
		0x003DEE00,
		[
			SimosBlock(0x001df800, 	0x00200000,		0x0001FE00),
			SimosBlock(0x0,			0x00020000,		0x000DFC00),
			SimosBlock(0x000dfc00, 	0x00100000,		0x000FFC00),
			SimosBlock(0x001ff600, 	0x002c0000,		0x0013FC00),
			SimosBlock(0x0033f200, 	0x00220000,		0x0009FC00)
		]
	),
	"DQ250": SimosHardware(
		0x3FFB0,
		0x3FFBB,
		0x3FFE0,
		0x3FFE4,
		1572864,
		0x0,
		[
			SimosBlock(0x0,			0x0,			0x0),
			SimosBlock(0x0,			0x0,			0x0),
			SimosBlock(0x0,			0x0,			0x00080E),
			SimosBlock(0x0,			0x40000,		0x130000),
			SimosBlock(0x0,			0x20000,		0x020000)
		]
	),
	"DQ250": SimosHardware(
		0x4FFB0,
		0x4FFBB,
		0x4FFE0,
		0x4FFE4,
		1572864,
		0x0,
		[
			SimosBlock(0x0,			0x0,			0x0),
			SimosBlock(0x0,			0x0,			0x0),
			SimosBlock(0x0,			0x0,			0x00080E),
			SimosBlock(0x0,			0x50000,		0x130000),
			SimosBlock(0x0,			0x30000,		0x020000)
		]
	),
}

class SimosBIN:
	def __init__(self):
		self.fileName = ""
		self.data = None


	def clear(self):
		self.data = None

		return ReturnType.OK


	def copy(self, bin, block = -1):
		if isinstance(bin, SimosBIN) == False and block < 5:
			return ReturnType.INVALID_PARAM

		self.fileName = bin.fileName

		if block < 0:
			self.data = bin.data

		else:
			if bin.data is None:
				return ReturnType.FILE_NOT_OPEN

			binKey, binValue = bin.hardwareType()
			if binValue is None:
				return ReturnType.UNKNOWN_HW

			self.data = bin.data[binValue.blocks[block].binPosition : binValue.blocks[block].binPosition + binValue.blocks[block].length]

		return ReturnType.OK

	def swapBlock(self, bin, block):
		if isinstance(block, int) == False or block > 4 or isinstance(bin, SimosBIN) == False:
			return ReturnType.INVALID_PARAM

		if self.data is None or bin.data is None:
			return ReturnType.FILE_NOT_OPEN

		hwKey, hwValue = self.hardwareType()
		binKey, binValue = bin.hardwareType()
		if hwValue is None or hwKey != binKey:
			return ReturnType.UNKNOWN_HW

		self.data = self.data[ : hwValue.blocks[block].binPosition] + \
					bin.data[hwValue.blocks[block].binPosition : hwValue.blocks[block].binPosition + hwValue.blocks[block].length] + \
					self.data[hwValue.blocks[block].binPosition + hwValue.blocks[block].length : ]
		
		return ReturnType.OK


	def load(self, fileName):
		#make sure we do not already have an open bin
		if self.data is not None:
			return ReturnType.FILE_OPEN

		#open file and read data
		self.fileName = fileName
		try:
			loadFile = open(fileName, mode='rb')
		except:
			return ReturnType.CANNOT_OPEN_FILE

		self.data = loadFile.read()
		loadFile.close()

		if self.data is None:
			return ReturnType.CANNOT_READ

		return ReturnType.OK


	def save(self, fileName):
		#make sure we already have an open bin
		if self.data is None:
			return ReturnType.FILE_NOT_OPEN

		#open file and read data
		self.fileName = fileName
		try:
			saveFile = open(fileName, mode='wb')
		except:
			return ReturnType.CANNOT_OPEN_FILE

		ret = saveFile.write(self.data)
		saveFile.close()

		if ret == 0:
			return ReturnType.CANNOT_SAVE

		return ReturnType.OK


	def hardwareType(self):
		#make sure we already have an open bin
		if self.data is None:
			return "", None

		for key, value in simosHW.items():
			data_size = len(self.data)
			if data_size == value.binSize:
				box_code = self.data[value.boxCodeStart : value.boxCodeStart + 11].strip()

				if box_code[0] != 0x00:
					return key, value

			elif data_size == value.blocks[4].length:
				box_code_pos = value.boxCodeStart - value.blocks[4].binPosition
				box_code = self.data[box_code_pos : box_code_pos + 11].strip()

				if box_code[0] != 0x00:
					return key, value

			elif data_size == value.ghidraSize:
				return key, value

		return "", None


	def boxCode(self):
		key, hw = self.hardwareType()
		if hw is not None:
			offset = 0
			if len(self.data) == hw.blocks[4].length:
				offset = hw.blocks[4].binPosition

			box_code_pos = hw.boxCodeStart - offset
			return self.data[box_code_pos : box_code_pos + 11].decode("latin1")

		return None


	def softwareCode(self):
		key, hw = self.hardwareType()
		if hw is not None and len(self.data) == hw.binSize:
			return self.data[hw.softCodeStart : hw.softCodeEnd].decode("latin1")

		return None
from Return import ReturnType
import sys

#constants
BTP_VERSION				= "BinToolz Patch v1.1"
BTP_HEADER_SIZE			= 100
BTP_PATCH_BLOCK_SIZE	= 8
BTP_GAP_FILL			= 8


class PatchHeaderType:
	def __init__(self, version, softCode, blockCount, blockChecksum, fileSize):
		self.version		= version
		self.softCode		= softCode
		self.blockCount		= blockCount
		self.blockChecksum	= blockChecksum
		self.fileSize		= fileSize


	@classmethod
	def fromBytes(cls, data):
		version			= data[0 : 20].decode("latin1")
		softCode		= data[20 : 28].decode("latin1")
		blockCount		= int.from_bytes(data[28 : 32], "little")
		blockChecksum	= int.from_bytes(data[32 : 36], "little")
		fileSize		= int.from_bytes(data[36 : 40], "little")

		return cls(version, softCode, blockCount, blockChecksum, fileSize)


	def toBytes(self):
		retData = self.version.encode(encoding="latin1")
		if len(retData) < 20:
			retData += bytearray(20 - len(retData))
		else:
			retData = retData[:20]

		retData += self.softCode.encode(encoding="latin1")
		if len(retData) < 28:
			retData += bytearray(28 - len(retData))
		else:
			retData = retData[:28]

		retData += (self.blockCount & 0xFFFFFFFF).to_bytes(4, "little")
		retData += (self.blockChecksum & 0xFFFFFFFF).to_bytes(4, "little")

		retData += self.fileSize.to_bytes(4, "little")
		if len(retData) < 100:
			retData += bytearray(100 - len(retData))
		else:
			retData = retData[:100]

		return retData


class PatchBlockType:
	def __init__(self, offset, length):
		self.offset = offset
		self.length = length


	@classmethod
	def fromBytes(cls, data):
		offset = int.from_bytes(data[0 : 4], "little")
		length = int.from_bytes(data[4 : 8], "little")

		return cls(offset, length)


	def toBytes(self):
		return self.offset.to_bytes(4, "little") + self.length.to_bytes(4, "little")


class BTP:
	def __init__(self):
		self.data = None
		self.header = None
		self.checkSum = 0


	def __readHeader(self):
		if self.data is None or len(self.data) < BTP_HEADER_SIZE:
			return ReturnType.FILE_NOT_OPEN

		self.header = PatchHeaderType.fromBytes(self.data[0 : BTP_HEADER_SIZE])

		if self.header.version.find(BTP_VERSION) != 0:
			return ReturnType.INVALID_VERSION

		return self.checkChecksum()


	def copy(self, btp):
		self.data = btp.data
		self.header = btp.header
		self.checkSum = btp.checksum


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

		#load header data
		return self.__readHeader()


	def save(self, fileName):
		#load header data
		ret = self.__readHeader()
		if ret != ReturnType.OK:
			return ret

		#open file and write data
		self.fileName = fileName
		try:
			saveFile = open(fileName, mode='wb')
		except:
			return ReturnType.CANNOT_OPEN_FILE

		saveFile.write(self.data)
		saveFile.close()

		return ReturnType.OK


	def checkChecksum(self):
		if self.data is None:
			return ReturnType.FILE_NOT_OPEN

		crc = 0xFFFFFFFF
		for i in range(BTP_HEADER_SIZE, len(self.data)):
			ch = self.data[i]
			for j in range(8):
				b = (ch ^ crc) & 1
				crc = (crc >> 1) & 0xFFFFFFFF
				if b == 1:
					crc = crc ^ 0xEDB88320

				ch = ch >> 1

		self.checksum = ~(crc & 0xFFFFFFFF)

		if self.header.blockChecksum != self.checksum:
			ReturnType.INVALID_CHECKSUM

		return ReturnType.OK


	def checkBin(self, bin, remove = False):
		modifiedCal = False
		modifiedASW = False

		hwKey, hwValue = bin.hardwareType()
		currentData = self.data[BTP_HEADER_SIZE:]
		#print("Block count [" + str(self.header.blockCount) + "]")

		#check all blocks
		try:
			for i in range(self.header.blockCount):
				blockHeader = PatchBlockType.fromBytes(currentData[0:8])
				currentData = currentData[BTP_PATCH_BLOCK_SIZE:]

				#print("Block offset [" + hex(blockHeader.offset) + "] length [" + hex(blockHeader.length) + "]")

				if remove:
					currentData = currentData[blockHeader.length:]

				for d in range(blockHeader.length):
					if bin.data[blockHeader.offset + d] != currentData[d]:
						if blockHeader.offset + d >= hwValue.blocks[4].binPosition and blockHeader.offset + d <= hwValue.blocks[4].binPosition + hwValue.blocks[4].length:
							modifiedCal = True
						else:
							modifiedASW = True

						#print("Wrong byte at location [" + hex(blockHeader.offset + d) + "] [" + hex(bin.data[blockHeader.offset + d]) + ":" + hex(currentData[d]) + "]")

				if remove == False:
					currentData = currentData[blockHeader.length:]

				currentData = currentData[blockHeader.length:]

		except:
			return ReturnType.CANNOT_READ
			
		#return state
		if modifiedCal == False:
			if modifiedASW == False:
				return ReturnType.OK

			return ReturnType.MODIFIED_ASW

		else:
			if modifiedASW:
				return ReturnType.MODIFIED_BOTH

			return ReturnType.MODIFIED_CAL


	def changeBin(self, bin, remove, doCalBlock):
		hwKey, hwValue = bin.hardwareType()
		currentData = self.data[BTP_HEADER_SIZE:]
		binData = bytearray(bin.data)
		for i in range(self.header.blockCount):
			blockHeader = PatchBlockType.fromBytes(currentData[0:8])
			currentData = currentData[BTP_PATCH_BLOCK_SIZE:]

			if remove == False:
				currentData = currentData[blockHeader.length:]

			#check for if we are in cal block and only modify if flag is set
			if doCalBlock or (blockHeader.offset >= hwValue.blocks[4].binPosition and blockHeader.offset + blockHeader.length <= hwValue.blocks[4].binPosition + hwValue.blocks[4].length) == False:
				for d in range(blockHeader.length):
					binData[blockHeader.offset + d] = currentData[d]
				
			if remove:
				currentData = currentData[blockHeader.length:]

			currentData = currentData[blockHeader.length:]

		bin.data = bytes(binData)

		return ReturnType.OK


	def createPatch(self, originalBin, modifiedBin, doCalBlock):
		if originalBin is None or modifiedBin is None or originalBin.data is None or modifiedBin.data is None or len(originalBin.data) != len(modifiedBin.data):
			return ReturnType.INVALID_PARAM

		originalhwKey, originalhwValue = originalBin.hardwareType()
		modifiedhwKey, modifiedhwValue = modifiedBin.hardwareType()

		if originalhwKey == "" or originalhwValue != modifiedhwValue:
			return ReturnType.HW_DOES_NOT_MATCH

		if originalBin.softwareCode() != originalBin.softwareCode():
			return ReturnType.SOFT_DOES_NOT_MATCH

		self.data = bytearray(BTP_HEADER_SIZE)
		inSize		= len(originalBin.data)
		blockSize	= 0
		blockCount	= 0
		patchSize	= 0
		blockOffset	= -1
		for i in range(inSize):
			if blockOffset > -1:
				if originalBin.data[i] == modifiedBin.data[i]:
					n = 0
					foundNext = False
					while n < BTP_GAP_FILL:
						if originalBin.data[i + n] != modifiedBin.data[i + n]:
							foundNext = True

						n += 1

					if foundNext == False:
						blockLength = i - blockOffset
						header = PatchBlockType(blockOffset, blockLength)

						self.data += header.toBytes()
						self.data += originalBin.data[blockOffset : blockOffset + blockLength]
						self.data += modifiedBin.data[blockOffset : blockOffset + blockLength]
						blockCount += 1

						#print("Adding block [" + str(blockCount) + "] Size [" + hex(blockLength) + "] Offset [" + hex(blockOffset) + "]")

						hexSize = 16
						if hexSize > blockLength:
							hexSize = blockLength

						blockOffset = -1

			else:
				if originalBin.data[i] != modifiedBin.data[i]:
					blockOffset = i

		
		self.__readHeader()
		self.checkChecksum()
		header = PatchHeaderType(BTP_VERSION, originalBin.softwareCode(), blockCount, self.checksum, inSize)
		self.data = header.toBytes() + self.data[BTP_HEADER_SIZE:]
		self.__readHeader()

		return ReturnType.OK
from enum import Enum
import sys


class ReturnType(Enum):
	OK					= 0, "Ok"
	FILE_OPEN			= 1, "File already open"
	FILE_NOT_OPEN		= 2, "File not open"
	CANNOT_OPEN_FILE	= 3, "Unable to open file"
	FILE_TOO_LARGE		= 4, "File too large"
	ALLOC_ERROR			= 5, "Unable to allocate memory for file"
	CANNOT_READ			= 6, "Unable to read file data"
	CANNOT_SAVE			= 7, "Unable to save file"
	UNKNOWN_HW			= 8, "Unknown hardware type"
	UNABLE_TO_WRITE		= 9, "Unable to write data"
	FILE_EXISTS			= 10, "File already created"
	FILE_DOESNT_EXIST	= 11, "File does not exist"
	INVALID_PARAM		= 12, "Invalid parameter"
	INVALID_VERSION		= 13, "Invalid version"
	INVALID_CHECKSUM	= 14, "Invalid checksum"
	MODIFIED_CAL		= 15, "Cal has been modified"
	MODIFIED_ASW		= 16, "ASW has been modified"
	MODIFIED_BOTH		= 17, "ASW and Cal have been modified"
	NOT_READY_TO_ACCEPT	= 18, "Bin is not ready to accept patch"
	READY_TO_ACCEPT		= 19, "Bin is ready to accept patch"
	PATCH_FOUND			= 20, "Bin contains patch"
	HW_DOES_NOT_MATCH	= 21, "Hardware types do not match"
	BOX_DOES_NOT_MATCH	= 22, "Boxcodes do not match"
	SOFT_DOES_NOT_MATCH	= 23, "Software code does not match"

	def string(self):
		s =  str(self.value)
		s = s[1 + s.find('\'') : s.rfind('\'')]

		return s

	def int(self):
		s =  str(self.value)
		s = s[1 : s.find(',')]
		try:
			ret = int(s)

		except:
			ret = -1

		return ret
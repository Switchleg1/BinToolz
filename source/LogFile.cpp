//---------------------------------------------------------------------------
#include "LogFile.h"
#include <string.h>
//---------------------------------------------------------------------------
LogFile::LogFile()
{
	logFile = NULL;
}
//---------------------------------------------------------------------------
LogFile::LogFile(int8_t* fileName)
{
	open(fileName);
}
//---------------------------------------------------------------------------
LogFile::~LogFile()
{
	close();
}
//---------------------------------------------------------------------------
int32_t LogFile::add(int8_t* line)
{
	if(logFile) {
		int length = strlen(line);
		return fwrite(line, sizeof(int8_t), length, logFile);
	}

	return -1;
}
//---------------------------------------------------------------------------
int32_t LogFile::open(int8_t* fileName)
{
	if(!logFile) {
		logFile = fopen(fileName, "wb");
		if(logFile) return 0;
		else return -1;
	}

	return -2;
}
//---------------------------------------------------------------------------
int32_t LogFile::close()
{
	if(logFile) {
		fclose(logFile);
		logFile = NULL;
		return 0;
	}

	return -1;
}

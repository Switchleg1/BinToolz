#ifndef LOGFILE_H
#define LOGFILE_H

#include <stdio.h>
#include <stdlib.h>
#include "Constants.h"

class LogFile {
public:
	LogFile();
	LogFile(int8_t* fileName);
	~LogFile();

	int32_t add(int8_t* line);
	int32_t open(int8_t* fileName);
	int32_t close();
private:
	FILE* logFile;
};

#endif

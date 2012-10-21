#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdio.h>

void log(FILE*, const char*, const char*);
void logInfo(FILE*, const char*);
void logError(FILE*, const char*);
void logWarning(FILE*, const char*);

#endif

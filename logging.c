#include "logging.h"

void log(FILE *fd, const char *header, const char *msg) {
	fprintf(fd, "%s%s", header, msg);
}

void logInfo (FILE *fd, const char *msg) {
	log(fd, "[INFO] :: ", msg);
}

void logError(FILE *fd, const char *msg) {
	log(fd, "[ ERR] :: ", msg);
}

void logWarning(FILE *fd, const char *msg) {
	log(fd, "[WARN] :: ", msg);
}

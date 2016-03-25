/*
 * tools.c
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include "tools.h"

char trace_level = TL_INFO;
char gTimeBuffer[50];

const char *tl_name[] = {
        "ERROR",
        "WARNING",
        "INFO",
        "DEBUG"
};

void log(char pLevel, const char *fmt, ...) {
    char buffer[2048];
    // if I ever send debug string longer than 2048 bytes,
    // I will never find this error why application crashes :-)
    if (pLevel > trace_level)
        return;
    va_list ap;
    va_start(ap, fmt);
    getTimeStamp();
    sprintf(buffer, "%s %-9s: ", gTimeBuffer, tl_name[(int) pLevel]);
    vsprintf(buffer + 23 + 11, fmt, ap);
    va_end(ap);
    fprintf(stderr, "# %s\n", buffer);

#ifdef FILE_LOG
    FILE *file;
    file = fopen("/tmp/my.log", "a+");
    fprintf(file, "%s\n", buffer);
    fclose(file);
#endif
}

void setTraceLevel(char value) {
    trace_level = value;
}

char *getTimeStamp() {
    struct timeval tv;
    struct tm *ptm;
    long milliseconds;

    /* Obtain the time of day, and convert it to a tm struct. */
    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);
    /* Format the date and time, down to a single second. */
    strftime(gTimeBuffer, sizeof(gTimeBuffer), "%d.%m.%Y %H:%M:%S", ptm);
    /* Compute milliseconds from microseconds. */
    milliseconds = tv.tv_usec / 1000;
    /* Print the formatted time, in seconds, followed by a decimal point
       and the milliseconds. */
    sprintf(gTimeBuffer, "%s.%03ld", gTimeBuffer, milliseconds);
    return gTimeBuffer;
}

int isTraceLevel(char value) {
    if (trace_level >= value)
        return 1;
    else
        return 0;
}

char *rtrim(char *value) {
	int i;
	for (i = strlen(value) - 1; i >= 0; i--) {
		if (value[i] == '\n' || value[i] == '\r' || value[i] == '\t' || value[i] == ' ') {
			value[i] = 0;
		} else {
			break;
		}
	}
	return value;
}

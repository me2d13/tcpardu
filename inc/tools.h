/*
 * tools.h
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#ifndef INC_TOOLS_H_
#define INC_TOOLS_H_

#define TL_ERROR    0
#define TL_WARNING  1
#define TL_INFO     2
#define TL_DEBUG    3

#define RETURN_ERROR 1
#define RETURN_OK 0
#define RETURN_STOP 3

#define FALSE 0
#define TRUE 1

void debugLog(char pLevel, const char *fmt, ...);

void setTraceLevel(char value);

char *getTimeStamp();

int isTraceLevel(char value);

#endif /* INC_TOOLS_H_ */

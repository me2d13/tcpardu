#ifndef TCPARDU_TCPARDU_H
#define TCPARDU_TCPARDU_H

#define TL_ERROR    0
#define TL_WARNING  1
#define TL_INFO     2
#define TL_DEBUG    3

void debugLog(char pLevel, const char *fmt, ...);
void shutDown();
void handle_ctrl_c(int pVal);
void handle_kill(int pVal);
void print_version(FILE *stream);
void print_help(FILE *stream, char *exec);
int processCommandLineArguments(int argc, char *argv[]);
int validateConfiguration();
void executionLoop();
void prepareSelectSets(fd_set *pRS, fd_set *pWS, int *pMaxFD);

#endif //TCPARDU

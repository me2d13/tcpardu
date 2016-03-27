/*
 * tcp.h
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#ifndef INC_TCP_H_
#define INC_TCP_H_

#define MAX_REQUEST_LENGTH 1024

void startTcpServer(int port);
int isServerStarted();
int isClientConnected();
void tcpShutdown();
void prepareTcpSelectSets(fd_set *pRS, int *pMaxFD);
void handleTcpRead(fd_set *ptr);
void sendToTcpClientIfConnected(char *data);
int processReceivedLine(char *value);
void setHttpMode();
void setTelnetMode();
short isTelnetMode();
short isHttpMode();
void processHttpRequest(char *request);
short translateHttpRequest(char *request, char *messageBuffer);


#endif /* INC_TCP_H_ */

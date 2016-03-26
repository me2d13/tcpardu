/*
 * tcp.h
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#ifndef INC_TCP_H_
#define INC_TCP_H_

void startTcpServer(int port);
int isServerStarted();
int isClientConnected();
void tcpShutdown();
void prepareTcpSelectSets(fd_set *pRS, int *pMaxFD);
void handleTcpRead(fd_set *ptr);
void sendToTcpClientIfConnected(char *data);
int processReceivedLine(char *value);


#endif /* INC_TCP_H_ */

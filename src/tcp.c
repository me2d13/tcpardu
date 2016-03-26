/*
 * tcp.c
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */


#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "tcp.h"
#include "tools.h"
#include <arpa/inet.h>
#include "serial.h"

int serverFd = 0;
int clientFd = 0;
struct sockaddr_in server , client;

void startTcpServer(int pPort) {
    log(TL_DEBUG, "TCP: Starting server at port %d", pPort);

    //Create socket
    serverFd = socket(AF_INET , SOCK_STREAM , 0);
    if (serverFd == -1)
    {
        log(TL_ERROR, "TCP: Could not create socket");
        return;
    }
    log(TL_DEBUG, "TCP: Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( pPort );

    //Bind
    if( bind(serverFd,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        log(TL_ERROR, "TCP: bind failed. Error");
        return;
    }
    log(TL_DEBUG, "TCP: bind done");

    //Listen - turn socket into listening mode
    listen(serverFd, 3);

}

int isServerStarted() {
    return serverFd;
}

void tcpShutdown() {
    if (serverFd) {
        close(serverFd);
    }
    if (clientFd) {
        close(clientFd);
    }
}

void prepareTcpSelectSets(fd_set *pRS, int *pMaxFD) {
    FD_SET(serverFd, pRS);
    if (serverFd > *pMaxFD) {
        *pMaxFD = serverFd;
    }
    if (clientFd > 0) {
        FD_SET(clientFd, pRS);
        if (clientFd > *pMaxFD) {
            *pMaxFD = clientFd;
        }
    }
}

void handleTcpRead(fd_set *readfds) {
    char *message = "tcpardu here\n";
    if (FD_ISSET(serverFd , readfds)) {
        if (clientFd > 0) {
            log(TL_WARNING, "TCP: New client login, old client disconnected");
            close(clientFd);
        }
        if ((clientFd = accept(serverFd, (struct sockaddr *)&client, (socklen_t*)&client))<0)
        {
            log(TL_ERROR, "TCP: Cannot accept new connection");
            tcpShutdown();
            return;
        }

        //inform user of socket number - used in send and receive commands
        log(TL_DEBUG, "TCP: New connection , socket fd is %d , ip is : %s , port : %d \n" , clientFd , inet_ntoa(client.sin_addr) , ntohs(client.sin_port));

        //send new connection greeting message
        if( send(clientFd, message, strlen(message), 0) != strlen(message) )
        {
            log(TL_ERROR, "TCP: send welcome message");
        }

        log(TL_DEBUG, "TCP: Welcome message sent successfully");
    }
    if (clientFd > 0 && FD_ISSET(clientFd , readfds)) {
        //Check if it was for closing , and also read the incoming message
        ssize_t valread;
        char buffer[1025];  //data buffer of 1K
        if ((valread = read(clientFd , buffer, 1024)) == 0)
        {
            //Somebody disconnected , get his details and print
            log(TL_WARNING, "TCP: Host disconnected , clearing client FD");

            //Close the socket and mark as 0 in list for reuse
            close( clientFd );
            clientFd = 0;
        } else {
            //set the string terminating NULL byte on the end of the data read
            buffer[valread] = '\0';
            processReceivedLine(buffer);
        }
    }
}

int isClientConnected() {
    return clientFd;
}

void sendToTcpClientIfConnected(char *data) {
    if (clientFd > 0) {
        if (send(clientFd, data, strlen(data), 0) != strlen(data)) {
            log(TL_ERROR, "TCP: send data");
        }
        log(TL_DEBUG, "TCP: Data message sent successfully");
    } else {
        log(TL_ERROR, "TCP: cannot send data to client as it is not connected");
    }
}

int processReceivedLine(char *value) {
	return dispatchMessageForSerialDevice(value);
}

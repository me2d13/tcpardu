/*
 * serial.h
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#include <termios.h>

#ifndef INC_SERIAL_H_
#define INC_SERIAL_H_

#define MAX_DEVICE_FILENAME_LENGTH 400
#define WAIT_TEN_SEC_BEFORE_REDETECTING_SERIAL_DEVICES 3
#define BAUDRATE B9600
#define READ_BUFFER_LENGTH 1024
#define MAX_MESSAGE_VALUES 20
#define MAX_UNITS_PER_DEVICE 10
#define MAX_STATUS_REQUESTS_PER_DEVICE 10
#define MAX_UNIT_NAME_LENGTH 30

typedef char NamesArray[MAX_UNITS_PER_DEVICE][MAX_UNIT_NAME_LENGTH];

typedef struct
{
    char deviceFileName[MAX_DEVICE_FILENAME_LENGTH];
    int index;
    int fd;
    struct termios oldtio;
    NamesArray units;
    NamesArray statusRequests;
    short unitsCount;
    short statusRequestsCount;
} DeviceInfo;

typedef struct
{
    short isStatus;
    short isCommand;
    char *from;
    char *to;
    char *command;
    char *values[MAX_MESSAGE_VALUES];
    int numberOfValues;
} Message;

void listSerialDevices();
int findSerialDevices(DeviceInfo *resultTable);
void setConfigDeviceRoot(char *deviceRoot);
void setConfigDeviceFilter(char *deviceRoot);
void detectSerialDevices();
void removeDeviceWithIndex(int index);
void addDevice(DeviceInfo* deviceInfo);
void prepareSerialSelectSets(fd_set *pRS, fd_set *pWS, int *pMaxFD);
void handleSerialRead(fd_set *readfds);
int openDevice(DeviceInfo* device);
void cleanupSerial();
void processCommandFromSerial(char *command, DeviceInfo *deviceInfo);
void processHelloCommand(DeviceInfo *deviceInfo);
void processDebugCommand(char *command, DeviceInfo *deviceInfo);
void sendString(char *value, DeviceInfo *deviceInfo);
void processCommand(char *command, DeviceInfo *deviceInfo);
void processStatus(char *command, DeviceInfo *deviceInfo);
short parseMessage(char *message, Message *result);
void processOrderCommandsFor(Message *message, DeviceInfo *deviceInfo);
int dispatchMessageForSerialDevice(char *value);
short strArrayContains(char *value, NamesArray names, int arraySize);
void setDevicePath(char *value);
void setDeviceFilter(char *value);
void setDevicePathDefaults();

#endif /* INC_SERIAL_H_ */

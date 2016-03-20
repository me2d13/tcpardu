/*
 * serial.h
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#include <termios.h>

#ifndef INC_SERIAL_H_
#define INC_SERIAL_H_

#define SERIAL_MAX_DEVCE_FILENAME_LENGTH 400
#define SERIAL_MAX_DEVICES_NUMBER 10

#define WAIT_TEN_SEC_BEFORE_REDETECTING_SERIAL_DEVICES 3

#define BAUDRATE B9600

#define READ_BUFFER_LENGTH 1024

typedef struct
{
    char deviceFileName[SERIAL_MAX_DEVCE_FILENAME_LENGTH];
    int fd;
    struct termios oldtio;
} DeviceInfo;

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

#endif /* INC_SERIAL_H_ */

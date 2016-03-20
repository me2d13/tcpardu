/*
 * serial.h
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */

#ifndef INC_SERIAL_H_
#define INC_SERIAL_H_

#define SERIAL_MAX_DEVCE_FILENAME_LENGTH 400
#define SERIAL_MAX_DEVICES_NUMBER 10

#define WAIT_SEC_BEFORE_REDETECTING_SERIAL_DEVICES 10

typedef struct
{
    char deviceFileName[SERIAL_MAX_DEVCE_FILENAME_LENGTH];
} DeviceInfo;

void listSerialDevices();
int findSerialDevices(DeviceInfo *resultTable);
void setConfigDeviceRoot(char *deviceRoot);
void setConfigDeviceFilter(char *deviceRoot);
void detectSerialDevices();
void removeDeviceWithIndex(int index);
void addDevice(DeviceInfo* deviceInfo);

#endif /* INC_SERIAL_H_ */

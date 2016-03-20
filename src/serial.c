/*
 * serial.c
 *
 *  Created on: Mar 20, 2016
 *      Author: petr
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "tools.h"

DeviceInfo gSerialDevices[SERIAL_MAX_DEVICES_NUMBER];
int gDeviceCount = 0;

char *gDeviceRoot = "/dev/serial/by-id";
char *gDeviceFilter = "arduino";

void detectSerialDevices() {
	debugLog(TL_DEBUG, "SERIAL: Detecting devices, currently %d known", gDeviceCount);
	DeviceInfo lSerialDevices[SERIAL_MAX_DEVICES_NUMBER];
	int count = findSerialDevices(lSerialDevices);
	// go through detected and check if it is known already
	int i, j;
	int existingFound[gDeviceCount+1];
	int isNew[count+1];
	for (j = 0; j < gDeviceCount; j++) {
		existingFound[j] = FALSE;
	}
	for (i = 0; i < count; i++) {
		isNew[i] = FALSE;
	}
	for (i = 0; i < count; i++) {
		isNew[i] = TRUE;
		for (j = 0; j < gDeviceCount; j++) {
			if (strcmp(lSerialDevices[i].deviceFileName, gSerialDevices[j].deviceFileName) == 0) {
				isNew[i] = FALSE;
				existingFound[j] = TRUE;
				break;
			}
		}
	}
	// remove obsolete
	j = 0;
	while (j < gDeviceCount) {
		if (!existingFound[j]) {
			removeDeviceWithIndex(j);
		} else {
			j++;
		}
	}
	// add new
	for (i = 0; i < count; i++) {
		if (isNew[i]) {
			addDevice(lSerialDevices+i);
		}
	}
}

int findSerialDevices(DeviceInfo *resultTable) {
	int serialDeviceFileNamefilter(const struct dirent *entry)
	{
		if (gDeviceFilter == NULL) {
			return 1;
		} else {
			return strcasestr(entry->d_name, gDeviceFilter) != NULL;
		}
	}

	struct dirent **entry_list;
	int count;
	int i;

	count = scandir(gDeviceRoot, &entry_list, serialDeviceFileNamefilter, alphasort);
	if (count < 0) {
		perror("scandir");
		return 0;
	}

	for (i = 0; i < count; i++) {
		struct dirent *entry;

		entry = entry_list[i];
		if (gDeviceRoot[(strlen(gDeviceRoot)-1)] == '/') {
			sprintf(resultTable[i].deviceFileName, "%s%s", gDeviceRoot, entry->d_name);
		} else {
			sprintf(resultTable[i].deviceFileName, "%s/%s", gDeviceRoot, entry->d_name);
		}
		free(entry);
	}
	free(entry_list);
	return count;
}


void listSerialDevices() {
	DeviceInfo lSerialDevices[SERIAL_MAX_DEVICES_NUMBER];
	int count = findSerialDevices(lSerialDevices);
	int i;
	for (i = 0; i < count; i++) {
		printf("%s\n", lSerialDevices[i].deviceFileName);
	}
}

void removeDeviceWithIndex(int index) {
	int i;
	debugLog(TL_INFO, "SERIAL: Removing device %s with index %d", gSerialDevices[index].deviceFileName, index);
	for (i = index; i < gDeviceCount - 1; i++) {
		memcpy(gSerialDevices+i, gSerialDevices+i+1, sizeof(DeviceInfo));
	}
	gSerialDevices--;
}

void addDevice(DeviceInfo* deviceInfo) {
	if (gDeviceCount == SERIAL_MAX_DEVICES_NUMBER) {
		debugLog(TL_ERROR, "SERIAL: Cannot add more than %d devices. %s ignored.", SERIAL_MAX_DEVICES_NUMBER, deviceInfo->deviceFileName);
		return;
	}
	debugLog(TL_INFO, "SERIAL: Adding device %s with index %d", deviceInfo->deviceFileName, gDeviceCount);
	memcpy(gSerialDevices+gDeviceCount, deviceInfo, sizeof(DeviceInfo));
	gDeviceCount++;
}

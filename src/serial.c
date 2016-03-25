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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "serial.h"
#include "tools.h"

DeviceInfo gSerialDevices[MAX_UNITS_PER_DEVICE];
int gDeviceCount = 0;

char *gDeviceRoot = "/dev/serial/by-id";
char *gDeviceFilter = "arduino";

void detectSerialDevices() {
	log(TL_DEBUG, "SERIAL: Detecting devices, currently %d known", gDeviceCount);
	DeviceInfo lSerialDevices[MAX_UNITS_PER_DEVICE];
	int count = findSerialDevices(lSerialDevices);
	// go through detected and check if it is known already
	int i, j;
	int existingFound[gDeviceCount + 1];
	int isNew[count + 1];
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
			addDevice(lSerialDevices + i);
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
		if (gDeviceRoot[(strlen(gDeviceRoot) - 1)] == '/') {
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
	DeviceInfo lSerialDevices[MAX_UNITS_PER_DEVICE];
	int count = findSerialDevices(lSerialDevices);
	int i;
	for (i = 0; i < count; i++) {
		printf("%s\n", lSerialDevices[i].deviceFileName);
	}
}

void removeDeviceWithIndex(int index) {
	int i;
	log(TL_INFO, "SERIAL: Removing device %s with index %d", gSerialDevices[index].deviceFileName, index);

	/*
	 restore the old port settings
	 tcsetattr(fd, TCSANOW, &oldtio);
	 */

	for (i = index; i < gDeviceCount - 1; i++) {
		memcpy(gSerialDevices + i, gSerialDevices + i + 1, sizeof(DeviceInfo));
		gSerialDevices[i].index = i;
	}
	gDeviceCount--;
}

void addDevice(DeviceInfo* deviceInfo) {
	if (gDeviceCount == MAX_UNITS_PER_DEVICE) {
		log(TL_ERROR, "SERIAL: Cannot add more than %d devices. %s ignored.", MAX_UNITS_PER_DEVICE,
				deviceInfo->deviceFileName);
		return;
	}
	log(TL_INFO, "SERIAL: Adding device %s with index %d", deviceInfo->deviceFileName, gDeviceCount);
	memcpy(gSerialDevices + gDeviceCount, deviceInfo, sizeof(DeviceInfo));
	gSerialDevices[gDeviceCount].index = gDeviceCount;
	gDeviceCount++;
	openDevice(gSerialDevices + gDeviceCount - 1);
}

void prepareSerialSelectSets(fd_set *pRS, fd_set *pWS, int *pMaxFD) {
	int i;
	for (i = 0; i < gDeviceCount; i++) {
		if (gSerialDevices[i].fd <= 0) {
			log(TL_WARNING, "Serial device %s not opened, cannot read.", gSerialDevices[i].deviceFileName);
			continue;
		}
		FD_SET(gSerialDevices[i].fd, pRS);
		if (gSerialDevices[i].fd > *pMaxFD) {
			*pMaxFD = gSerialDevices[i].fd;
		}
	}
}

void handleSerialRead(fd_set *readfds) {
	int i;
	for (i = 0; i < gDeviceCount; i++) {
		if (gSerialDevices[i].fd > 0 && FD_ISSET(gSerialDevices[i].fd, readfds)) {
			char buf[READ_BUFFER_LENGTH];
			int res = read(gSerialDevices[i].fd, buf, READ_BUFFER_LENGTH);
			buf[res] = 0; // set end of string, so we can printf
			log(TL_DEBUG, "SERIAL[%d]: got %d bytes: %s", i, res, buf);
			rtrim(buf);
			if (buf[0]) {
				processCommandFromSerial(buf, gSerialDevices + i);
			}
		}
	}
}

int openDevice(DeviceInfo* device) {
	struct termios newtio;
	device->fd = open(device->deviceFileName, O_RDWR | O_NOCTTY);
	if (device->fd < 0) {
		perror(device->deviceFileName);
		return RETURN_ERROR;
	}

	tcgetattr(device->fd, &(device->oldtio)); /* save current serial port settings */
	bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

	/*
	 BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
	 CRTSCTS : output hardware flow control (only used if the cable has
	 all necessary lines. See sect. 7 of Serial-HOWTO)
	 CS8     : 8n1 (8bit,no parity,1 stopbit)
	 CLOCAL  : local connection, no modem contol
	 CREAD   : enable receiving characters
	 */
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;

	/*
	 IGNPAR  : ignore bytes with parity errors
	 ICRNL   : map CR to NL (otherwise a CR input on the other computer
	 will not terminate input)
	 otherwise make device raw (no other input processing)
	 */
	newtio.c_iflag = IGNPAR | ICRNL;

	/*
	 Raw output.
	 */
	newtio.c_oflag = 0;

	/*
	 ICANON  : enable canonical input
	 disable all echo functionality, and don't send signals to calling program
	 */
	newtio.c_lflag = ICANON;

	/*
	 initialize all control characters
	 default values can be found in /usr/include/termios.h, and are given
	 in the comments, but we don't need them here
	 */
	newtio.c_cc[VINTR] = 0; /* Ctrl-c */
	newtio.c_cc[VQUIT] = 0; /* Ctrl-\ */
	newtio.c_cc[VERASE] = 0; /* del */
	newtio.c_cc[VKILL] = 0; /* @ */
	newtio.c_cc[VEOF] = 4; /* Ctrl-d */
	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 1; /* blocking read until 1 character arrives */
	newtio.c_cc[VSWTC] = 0; /* '\0' */
	newtio.c_cc[VSTART] = 0; /* Ctrl-q */
	newtio.c_cc[VSTOP] = 0; /* Ctrl-s */
	newtio.c_cc[VSUSP] = 0; /* Ctrl-z */
	newtio.c_cc[VEOL] = 0; /* '\0' */
	newtio.c_cc[VREPRINT] = 0; /* Ctrl-r */
	newtio.c_cc[VDISCARD] = 0; /* Ctrl-u */
	newtio.c_cc[VWERASE] = 0; /* Ctrl-w */
	newtio.c_cc[VLNEXT] = 0; /* Ctrl-v */
	newtio.c_cc[VEOL2] = 0; /* '\0' */

	/*
	 now clean the modem line and activate the settings for the port
	 */
	tcflush(device->fd, TCIFLUSH);
	tcsetattr(device->fd, TCSANOW, &newtio);

	return RETURN_OK;
}

void cleanupSerial() {
	int i;
	for (i = 0; i < gDeviceCount; i++) {
		if (gSerialDevices[i].fd > 0) {
			tcsetattr(gSerialDevices[i].fd, TCSANOW, &(gSerialDevices[i].oldtio));
		}
	}
}

void processCommandFromSerial(char *command, DeviceInfo *deviceInfo) {
	if (strcasecmp("HELLO", command) == 0) {
		processHelloCommand(deviceInfo);
	} else if (strncasecmp("DEBUG:", command, 6) == 0) {
		processDebugCommand(command, deviceInfo);
	} else if (strncasecmp("CMD:", command, 4) == 0) {
		processCommand(command, deviceInfo);
	} else if (strncasecmp("STS:", command, 4) == 0) {
		processStatus(command, deviceInfo);
	} else {
		log(TL_WARNING, "SERIAL: Unknown message %s ignored.", command);
	}
}

void processHelloCommand(DeviceInfo *deviceInfo) {
	sendString("OLLEH", deviceInfo);
}

void processDebugCommand(char *command, DeviceInfo *deviceInfo) {
	log(TL_DEBUG, "SERIAL[%d]: %s", deviceInfo->index, command);
}

void sendString(char *value, DeviceInfo *deviceInfo) {
	log(TL_DEBUG, "SERIAL[%d]: sent '%s'", deviceInfo->index, value);
	write(deviceInfo->fd, value, strlen(value));
}

void processCommand(char *command, DeviceInfo *deviceInfo) {
	Message message;
	if (parseMessage(command, &message) && message.isCommand) {
		if (strcasecmp("ORDER_COMMAND_FOR", message.command) == 0) {
			processOrderCommandsFor(&message, deviceInfo);
		} else {
			log(TL_ERROR, "SERIAL[%d]: Unsupported command '%s'", deviceInfo->index, command);
		}
	} else {
		log(TL_ERROR, "SERIAL[%d]: Error parsing message '%s'", deviceInfo->index, command);
	}

}

void processOrderCommandsFor(Message *message, DeviceInfo *deviceInfo) {
	log(TL_DEBUG, "SERIAL[%d]: Registering %d units to receive commands", message->numberOfValues);
	memset(deviceInfo->units, 0, sizeof(deviceInfo->units));
	int i;
	for (i = 0; i < message->numberOfValues; i++) {
		if (i == MAX_UNITS_PER_DEVICE) {
			log(TL_ERROR, "Maximum units per device reached. Cannot allocate more command requests.");
			break;
		}
		strncpy(deviceInfo->units[i], message->values[i], MAX_UNIT_NAME_LENGTH);
		log(TL_DEBUG, "Registering command request for %s", deviceInfo->units[i]);
	}
	deviceInfo->unitsCount = i;
}

void processStatus(char *command, DeviceInfo *deviceInfo) {
	log(TL_DEBUG, "SERIAL[%d]: Status %s", deviceInfo->index, command);
}

short parseMessage(char *message, Message *result) {
	int item = 0;
	int pos = 0;
	result->numberOfValues = 0;
	while (message[pos]) {
		if (message[pos] == ':') {
			message[pos] = 0;
			switch (item) {
				case 0:
					if (strcasecmp("CMD", message) == 0) {
						result->isCommand = TRUE;
						result->isStatus = FALSE;
					} else if (strcasecmp("STS", message) == 0) {
						result->isCommand = FALSE;
						result->isStatus = TRUE;
					} else {
						log(TL_ERROR, "Unknown message type '%s'", message);
						return FALSE;
					}
					pos++;
					result->from = message+pos;
					break;
				case 1:
					pos++;
					result->to = message+pos;
					break;
				case 2:
					pos++;
					if (result->isCommand) {
						result->command = message+pos;
					} else {
						result->values[0] = message+pos;
						result->numberOfValues++;
					}
					break;
				default:
					pos++;
					if (result->numberOfValues == MAX_MESSAGE_VALUES) {
						log(TL_ERROR, "Message '%s' has more values then supported (%d)", message, MAX_MESSAGE_VALUES);
						return FALSE;
					}
					result->values[result->numberOfValues] = message+pos;
					result->numberOfValues++;
					break;
			}
			item++;
		} else {
			pos++;
		}
	}
	return TRUE;
}

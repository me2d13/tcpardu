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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "serial.h"
#include "tools.h"
#include "tcp.h"
#include "mqtt.h"

DeviceInfo gSerialDevices[MAX_UNITS_PER_DEVICE];
int gDeviceCount = 0;


char gDeviceRoot[MAX_DEVICE_FILENAME_LENGTH];
char gDeviceFilter[MAX_DEVICE_FILENAME_LENGTH];

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
	memcpy(gSerialDevices + gDeviceCount, deviceInfo, sizeof(DeviceInfo));
	gSerialDevices[gDeviceCount].index = gDeviceCount;
	openDevice(gSerialDevices + gDeviceCount);
	log(TL_INFO, "SERIAL: Added device %s with index %d, fd %d", deviceInfo->deviceFileName,
			gDeviceCount, gSerialDevices[gDeviceCount].fd);
	gDeviceCount++;
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
			//log(TL_DEBUG, "Max fd for select is now %d", *pMaxFD);
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
			log(TL_DEBUG, "fromSERIAL[%d]: got %d bytes from fd %d, bytes: %s", i, res, gSerialDevices[i].fd, buf);
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
	} else if (strncasecmp("MP:", command, 3) == 0) {
		processMqttPublish(command, deviceInfo);
	} else if (strncasecmp("MS:", command, 3) == 0) {
		processMqttSubscribe(command, deviceInfo);
	} else {
		log(TL_WARNING, "SERIAL: Unknown message %s ignored.", command);
	}
}

void processHelloCommand(DeviceInfo *deviceInfo) {
	sendString("OLLEH", deviceInfo);
}

void processDebugCommand(char *command, DeviceInfo *deviceInfo) {
	log(TL_DEBUG, "fromSERIAL[%d]: %s", deviceInfo->index, command);
}

void sendString(char *value, DeviceInfo *deviceInfo) {
	int valueLength = strlen(value);
	char *valueCopy = (char*)malloc(valueLength+2);
	if (valueCopy == NULL) {
		log(TL_ERROR, "Cannot allocate %d bytes for value copy", valueLength+2);
		return;
	}
	strcpy(valueCopy, value);
	valueCopy[valueLength] = MESSAGE_SEPARATOR;
	valueCopy[valueLength+1] = 0;
	log(TL_DEBUG, "toSERIAL[%d]: sent '%s' to fd %d", deviceInfo->index, valueCopy, deviceInfo->fd);
	write(deviceInfo->fd, valueCopy, valueLength+1);
	free(valueCopy);
}

void sendMqttMessage(char *topic, char *payload, int payloadLength, DeviceInfo *deviceInfo) {
	int topicLength = strlen(topic);
	int totalSize = 2+1+topicLength+1+payloadLength+1+1;
	// format: MM:topic:payload#0
	char *outBuffer = (char*)malloc(totalSize);
	if (outBuffer == NULL) {
		log(TL_ERROR, "Cannot allocate %d bytes for value copy", totalSize);
		return;
	}
	outBuffer[0] = 'M';
	outBuffer[1] = 'M';
	outBuffer[2] = ':';
	int topicStart = 3;
	strncpy(outBuffer+topicStart, topic, topicLength);
	outBuffer[topicStart + topicLength] = ':';
	int payloadStart = topicStart+1+topicLength;
	strncpy(outBuffer+payloadStart, payload, payloadLength);
	outBuffer[payloadStart+payloadLength] = MESSAGE_SEPARATOR;
	outBuffer[payloadStart+payloadLength+1] = 0;
	log(TL_DEBUG, "toSERIAL[%d]: sent '%s' to fd %d", deviceInfo->index, outBuffer, deviceInfo->fd);
	write(deviceInfo->fd, outBuffer, totalSize-1);
	// -1 means: do not send string terminator \0. Anything after message separator
	// would be considered as part of next message which would be empty string
	// but keep it there for debug output purpose, just send shorter string to serial
	free(outBuffer);
}

void processCommand(char *command, DeviceInfo *deviceInfo) {
	Message message;
	if (parseMessage(command, &message) && message.isCommand) {
		if (strcasecmp("ORDER_COMMAND_FOR", message.command) == 0) {
			processOrderCommandsFor(&message, deviceInfo);
		} else {
			//log(TL_ERROR, "SERIAL[%d]: Unsupported command '%s'", deviceInfo->index, command);
			log(TL_DEBUG, "SERIAL[%d]: Sending command '%s' to tcp client.", deviceInfo->index, command);
			sendToTcpClientIfConnected(command);
		}
	} else {
		log(TL_ERROR, "SERIAL[%d]: Error parsing message '%s'", deviceInfo->index, command);
	}
}

void processMqttPublish(char *command, DeviceInfo *deviceInfo) {
	char topic[MAX_TOPIC_LENGTH+1];
	int i = 0;
	while (command[i+3] != ':') {
		topic[i] = command[i+3];
		if (command[i+3] == 0 || i == MAX_TOPIC_LENGTH+1) {
			break;
		}
		i++;
	}
	topic[i] = 0;
	if (i == 0) {
		log(TL_ERROR, "SERIAL[%d]: No topic parsed in MQTT publish command '%s'", deviceInfo->index, command);
		return;
	}
	if (i == MAX_TOPIC_LENGTH+1 && topic[i] != 0) {
		log(TL_ERROR, "SERIAL[%d]: Topic too long (maximum %d) in MQTT publish command '%s'", deviceInfo->index, MAX_TOPIC_LENGTH, command);
		return;
	}
	if (command[i+3] != 0) {
		mqttPublish(topic, command + i + 4);
	} else {
		mqttPublish(topic, NULL);
	}
}

void processMqttSubscribe(char *command, DeviceInfo *deviceInfo) {
	mqttSubscribe(command + 3);
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

void processStatus(char *status, DeviceInfo *deviceInfo) {
	//log(TL_DEBUG, "SERIAL[%d]: Status %s", deviceInfo->index, status);
	// forward to other serial devices if requested
	char *statusCopy = (char*)malloc(strlen(status)+1);
	if (statusCopy == NULL) {
		log(TL_ERROR, "Cannot allocate %d bytes for message copy", strlen(status)+1);
		return;
	}
	strcpy(statusCopy, status);
	dispatchMessageForSerialDevice(statusCopy);
	Message message;
	if (parseMessage(statusCopy, &message) && message.isStatus) {
		log(TL_DEBUG, "SERIAL[%d]: Sending status '%s' to tcp client.", deviceInfo->index, status);
		sendToTcpClientIfConnected(status);
	} else {
		log(TL_ERROR, "SERIAL[%d]: Error parsing message '%s'", deviceInfo->index, status);
	}
	free(statusCopy);
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
				result->from = message + pos;
				break;
			case 1:
				pos++;
				result->to = message + pos;
				break;
			case 2:
				pos++;
				if (result->isCommand) {
					result->command = message + pos;
				} else {
					result->values[0] = message + pos;
					result->numberOfValues++;
				}
				break;
			default:
				pos++;
				if (result->numberOfValues == MAX_MESSAGE_VALUES) {
					log(TL_ERROR, "Message '%s' has more values then supported (%d)", message, MAX_MESSAGE_VALUES);
					return FALSE;
				}
				result->values[result->numberOfValues] = message + pos;
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

int dispatchMessageForSerialDevice(char *value) {
	Message message;
	int messagesSent = 0;
	char *valueCopy = (char*)malloc(strlen(value)+1);
	if (valueCopy == NULL) {
		log(TL_ERROR, "Cannot allocate %d bytes for message copy", strlen(value)+1);
		return 0;
	}
	strcpy(valueCopy, value);
	if (parseMessage(valueCopy, &message)) {
		int i;
		for (i = 0; i < gDeviceCount; i++) {
			if (message.isCommand) {
				if (strArrayContains(message.to, gSerialDevices[i].units, gSerialDevices[i].unitsCount)) {
					sendString(value, gSerialDevices + i);
					messagesSent++;
				}
			}
			if (message.isStatus) {
				if (strArrayContains(message.from, gSerialDevices[i].statusRequests, gSerialDevices[i].statusRequestsCount)) {
					sendString(value, gSerialDevices + i);
					messagesSent++;
				}
			}
		}
		log(TL_WARNING, "SERIAL: Message sent %d times.", messagesSent);
	} else {
		log(TL_WARNING, "Cannot parse message '%s' - wrong format.", value);
	}
	free(valueCopy);
	return messagesSent;
}

short strArrayContains(char *value, NamesArray names, int arraySize) {
	int i;
	for (i = 0; i < arraySize; i++) {
		if (strncasecmp(value, names[i], MAX_UNIT_NAME_LENGTH) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

void setDevicePath(char *value) {
	strncpy(gDeviceRoot, value, MAX_DEVICE_FILENAME_LENGTH);
}

void setDeviceFilter(char *value) {
	strncpy(gDeviceFilter, value, MAX_DEVICE_FILENAME_LENGTH);
}

void setDevicePathDefaults() {
	setDevicePath("/dev/serial/by-id");
	setDeviceFilter("arduino");
	//char *gDeviceFilter = "if00";
}

void sendMqttMessageToAllSerialDevices(char *topic, void *payload, int payloadLength) {
	int i;
	for (i = 0; i < gDeviceCount; i++) {
		sendMqttMessage(topic, payload, payloadLength, gSerialDevices + i);
	}
}

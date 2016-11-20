#include <string.h>
#include <MQTTClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "mqtt.h"
#include "tools.h"
#include "serial.h"

char gMqttServerHost[MAX_HOSTNAME_LENGTH];

char gSubscriptions[MAX_SUBSCRIPTIONS][MAX_TOPIC_LENGTH];
int gSubscriptionsCount = 0;

// 0 = not connected
// 1 = connected
// >2 means reconnecting phases (different timeouts, retry number)
int gConnectionPhase = 0;
long gLastConnectAttemptInSec = 0;

int reconnectionIntervalsInSec[] = {10, 10, 10, 20, 20, 30, 30, 30, 60, 60, 120, 120, 300};
int reconnectionIntervalsLength = 13;

volatile MQTTClient_deliveryToken deliveredtoken;
MQTTClient client;

long getCurrentTimeInSec() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

void setMqttBrokerHost(char *value) {
	strncpy(gMqttServerHost, value, MAX_HOSTNAME_LENGTH);
}

char *getMqttBrokerHost() {
	return gMqttServerHost[0] == 0 ? NULL : gMqttServerHost;
}


void delivered(void *context, MQTTClient_deliveryToken dt) {
	log(TL_DEBUG, "MQTT: Message with token value %d delivery confirmed", dt);
	deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen,
		MQTTClient_message *message) {
	log(TL_DEBUG, "MQTT: Message with topic %s and length %d arrived.",
			topicName, message->payloadlen);

	//TODO: for now send message to all arduinos. Later implement topic parsing + remember subsribed topics for each serial line
	sendMqttMessageToAllSerialDevices(topicName, message->payload, message->payloadlen);

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}

void connlost(void *context, char *cause) {
	log(TL_ERROR, "MQTT: Connection lost, cause: %s", cause);
	gConnectionPhase = 2; // disconnected - first retry
}

void connectMqtt() {
	if (gConnectionPhase == 1) {
		log(TL_WARNING, "MQTT: Already connected, disconnect first");
		return;
	}
	int rc;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

	MQTTClient_create(&client, getMqttBrokerHost(), CLIENTID,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		gLastConnectAttemptInSec = getCurrentTimeInSec();
		if (gConnectionPhase <= 1) {
			gConnectionPhase = 2; // disconnected - first retry
		} else {
			gConnectionPhase++;
		}
		log(TL_ERROR, "MQTT: Failed to connect, return code %d. Current connection phase is %d.", rc, gConnectionPhase);
		return;
	}
	gConnectionPhase = 1; // connected
	// add requested subscriptions
	int i;
	for (i = 0; i < gSubscriptionsCount; i++) {
		mqttSubscribe(gSubscriptions[i]);
	}
}

void disconnectMqtt() {
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	gConnectionPhase = 0; // disconnected
}

void mqttPublish(char *topic, char *payload) {
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	pubmsg.payload = payload;
	pubmsg.payloadlen = strlen(payload);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
	log(TL_DEBUG, "MQTT: Published message '%s' on topis %s.", payload, topic);
}

void mqttSubscribe(char *topic) {
	//TODO: build subscription list that can be reprocessed after reconnecting
	int rc;
	if ((rc = MQTTClient_subscribe(client, topic, QOS)) != MQTTCLIENT_SUCCESS) {
		log(TL_ERROR, "Failed to subsribe to %s, return code %d", topic, rc);
		return;
	}
	int i;
	for (i = 0; i < gSubscriptionsCount; i++) {
		if (strncmp(topic, gSubscriptions[i], MAX_TOPIC_LENGTH) == 0) {
			log(TL_DEBUG, "MQTT: Subscription %s already known, not adding to list.", topic);
			return;
		}
	}
	strncpy(gSubscriptions[gSubscriptionsCount++], topic, MAX_TOPIC_LENGTH);
	log(TL_DEBUG, "MQTT: Subscription %s added to list making it %d big.", topic, gSubscriptionsCount);
}

void mqttConnectionCheckTick() {
	if (gConnectionPhase == 1) {
		return;
	}
	if (gConnectionPhase == 0) {
		if (getMqttBrokerHost() != NULL) {
			connectMqtt();
		}
		return;
	}
	int currentInterval = (gConnectionPhase-2 < reconnectionIntervalsLength) ? reconnectionIntervalsInSec[gConnectionPhase-2] :
			reconnectionIntervalsInSec[reconnectionIntervalsLength-1];
	if (gLastConnectAttemptInSec + currentInterval < getCurrentTimeInSec()) {
		connectMqtt();
	}
}

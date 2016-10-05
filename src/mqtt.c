#include <string.h>
#include <MQTTClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mqtt.h"
#include "tools.h"
#include "serial.h"

char gMqttServerHost[MAX_HOSTNAME_LENGTH];

volatile MQTTClient_deliveryToken deliveredtoken;
MQTTClient client;

void setMqttBrokerHost(char *value) {
	strncpy(gMqttServerHost, value, MAX_HOSTNAME_LENGTH);
}

char *getMqttBrokerHost() {
	return gMqttServerHost[0] == 0 ? NULL : gMqttServerHost;
}
;

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
}

void connectMqtt() {
	int rc;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

	MQTTClient_create(&client, getMqttBrokerHost(), CLIENTID,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		log(TL_ERROR, "MQTT: Failed to connect, return code %d", rc);
		return;
	}
}

void disconnectMqtt() {
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
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
	int rc;
	if ((rc = MQTTClient_subscribe(client, topic, QOS)) != MQTTCLIENT_SUCCESS) {
		log(TL_ERROR, "Failed to subsribe to %s, return code %d", topic, rc);
		return;
	}
}

#include "mqtt.h"
#include <string.h>
#include <MQTTClient.h>

#define CLIENTID    "Tcpardu"
#define TOPIC       "/garage/#"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

char gMqttServerHost[MAX_HOSTNAME_LENGTH];

volatile MQTTClient_deliveryToken deliveredtoken;
MQTTClient client;

void setMqttBrokerHost(char *value) {
	strncpy(gMqttServerHost, value, MAX_HOSTNAME_LENGTH);
}

char *getMqttBrokerHost() {
	return gMqttServerHost[0] == 0 ? NULL : gMqttServerHost; 
};

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
void connectMqtt() {
	int rc;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
 
    MQTTClient_create(&client, getMqttBrokerHost(), CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        return;       
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    if ((rc = MQTTClient_subscribe(client, TOPIC, QOS)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to subsribe, return code %d\n", rc);
        return;       
    }
}

void disconnectMqtt() {
	MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}
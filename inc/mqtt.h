/*
 * mqtt.h
 *
 *      Author: petr
 */

#ifndef INC_MQTT_H_
#define INC_MQTT_H_

#define MAX_HOSTNAME_LENGTH 400
#define MAX_TOPIC_LENGTH 500
#define MAX_SUBSCRIPTIONS 20

#define CLIENTID    "Tcpardu"
#define QOS         1
#define TIMEOUT     10000L


void setMqttBrokerHost(char *value);
char *getMqttBrokerHost();
void connectMqtt();
void disconnectMqtt();
void mqttPublish(char *topic, char *payload);
void mqttSubscribe(char *topic);
void mqttConnectionCheckTick();


#endif /* INC_MQTT_H_ */

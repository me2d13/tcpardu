/*
 * mqtt.h
 *
 *      Author: petr
 */

#ifndef INC_MQTT_H_
#define INC_MQTT_H_

#define MAX_HOSTNAME_LENGTH 400


void setMqttBrokerHost(char *value);
char *getMqttBrokerHost();
void connectMqtt();
void disconnectMqtt();


#endif /* INC_MQTT_H_ */

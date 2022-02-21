#ifndef _COMMON_H__
#define _COMMON_H__

#include <Arduino.h>


#define DEBUG 1
//#define DEBUG_MQTT 1
#include "debug_print.h"


#include <TFT_eSPI.h>

#define MQTT_DEV_TYPE "printme"

// mqtt topics
enum topics_t {
	       MQTT_TOPIC_SETCONFIG = 0,
	       MQTT_TOPIC_STATE,
	       MQTT_TOPIC_VERSION,
	       MQTT_TOPIC_IPADDR,
	       MQTT_TOPIC_RST,
	       MQTT_TOPIC_SPEED,
	       MQTT_TOPIC_INCLINE,
	       MQTT_NUM_TOPICS
};


void setupMqttTopic(const String &id);
const char* getTopic(topics_t topic);
const char* getRstReason(esp_reset_reason_t r);

int setupWifi(TFT_eSPI* tft = NULL);
bool mqttConnect(TFT_eSPI* tft = NULL);
void initAsyncWebserver();


extern const char* VERSION;
extern String MQTTDEVICEID;
extern String mqtt_topics[];


extern TFT_eSPI tft;
extern esp_reset_reason_t rr;


#endif

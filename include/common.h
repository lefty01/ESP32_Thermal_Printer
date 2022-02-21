#ifndef _COMMON_H__
#define _COMMON_H__

#include <Arduino.h>


#define DEBUG 1
//#define DEBUG_MQTT 1
#include "debug_print.h"


#include <TFT_eSPI.h>

#define MQTT_DEV_TYPE "ESP32printer"

// mqtt topics
enum topics_t {
	       MQTT_TOPIC_SETCONFIG = 0,
	       MQTT_TOPIC_STATE,
	       MQTT_TOPIC_VERSION,
	       MQTT_TOPIC_IPADDR,
	       MQTT_TOPIC_RST,
	       MQTT_TOPIC_MSG,
	       MQTT_NUM_TOPICS
};


void setupMqttTopic(const String &id);
const char* getTopic(topics_t topic);
const char* getRstReason(esp_reset_reason_t r);
bool mqttPublish(topics_t topic, const char* msg);

bool setupWifi(TFT_eSPI* tft = NULL);
bool mqttConnect(TFT_eSPI* tft = NULL);
void initAsyncWebserver();
void checkWifi(bool &wifiAvail, bool &mqttAvail);
const String getWifiIpAddr();



extern const char* VERSION;
extern String MQTTDEVICEID;
extern String mqtt_topics[];


extern TFT_eSPI tft;
extern esp_reset_reason_t rr;


#endif

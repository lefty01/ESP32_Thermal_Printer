#include "common.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "wifi_mqtt_creds.h"

WiFiClientSecure wifiClient;
PubSubClient mqttClient;
AsyncWebServer httpServer(80);

String ipAddr;
String dnsAddr;
const unsigned maxWifiWaitSeconds = 60;


String mqtt_topics[] {
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/setconfig",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/state",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/version",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/ipaddr",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/rst",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/speed",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/incline",
};


void setupMqttTopic(const String &id)
{
  for (unsigned i = 0; i < MQTT_NUM_TOPICS; ++i) {
    mqtt_topics[i].replace("%MQTTDEVICEID%", id);
  }
}

const char* getTopic(topics_t topic) {
  return mqtt_topics[topic].c_str();
}


String getWifiIpAddr() {
  return ipAddr;
}

// note: delays mainly to keep tft text shortly readable
int setupWifi(TFT_eSPI* tft /*= NULL*/) {
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Connecting to wifi");

  if (tft != NULL) {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE);
    tft->setTextFont(2);
    tft->setCursor(20, 40);
    tft->println("Connecting to WiFi");
  }
  unsigned retry_counter = 0;
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
    if (tft != NULL) tft->print(".");
    retry_counter++;
    if (retry_counter > maxWifiWaitSeconds) {
      DEBUG_PRINTLN(" TIMEOUT!");
      if (tft != NULL) {
	  tft->setTextFont(4);
	  tft->fillScreen(TFT_BLACK);
	  tft->setTextColor(TFT_RED);
	  tft->setCursor(20, 60);
	  tft->println("Wifi TIMEOUT");
      }
      delay(2000);
      return 1;
    }
  }
  ipAddr  = WiFi.localIP().toString();
  dnsAddr = WiFi.dnsIP().toString();

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(ipAddr);
  DEBUG_PRINTLN("DNS address: ");
  DEBUG_PRINTLN(dnsAddr);

  if (tft != NULL) {
      tft->fillScreen(TFT_BLACK);
      tft->setTextColor(TFT_GREEN);
      tft->setCursor(20, 40);
      tft->println("Wifi CONNECTED");
      tft->print("IP Addr: "); tft->println(ipAddr);
  }
  delay(2000);
  return 0;
}


bool mqttConnect(TFT_eSPI* tft /*= NULL*/) {
  bool rc;
  DEBUG_PRINT("Attempting MQTT connection...");
  if (tft != NULL) {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE);
    tft->setTextFont(2);
    tft->setCursor(20, 40);
    tft->print("Connecting to MQTT server: ");
    tft->println(mqtt_host);
  }
  // Attempt to connect
#ifdef MQTT_USE_SSL
  wifiClient.setCACert(server_crt_str);
  wifiClient.setCertificate(client_crt_str);
  wifiClient.setPrivateKey(client_key_str);
#endif

  mqttClient.setServer(mqtt_host, mqtt_port);
  if (mqttClient.connect(MQTTDEVICEID.c_str(), mqtt_user, mqtt_pass,
		     getTopic(MQTT_TOPIC_STATE), 1, 1, "OFFLINE")) {
    DEBUG_PRINTLN("connected");
    // Once connected, publish an announcement...
    if (tft != NULL) tft->println("publish connected...");
    rc = mqttClient.publish(getTopic(MQTT_TOPIC_STATE),  "CONNECTED", true);
    if (tft != NULL) {
      if (rc) tft->println("OK");
      else    tft->println("ERROR");
    }
    delay(1500);

    if (tft != NULL) tft->println("publish version & IP");
    rc |= mqttClient.publish(getTopic(MQTT_TOPIC_RST), getRstReason(rr), true);
    rc |= mqttClient.publish(getTopic(MQTT_TOPIC_VERSION), VERSION, true);
    rc |= mqttClient.publish(getTopic(MQTT_TOPIC_IPADDR), ipAddr.c_str(), true);
    if (tft != NULL) {
      if (rc) tft->println("OK");
      else    tft->println("ERROR");
    }
    delay(1500);
    if (tft != NULL) {
      tft->fillScreen(TFT_BLACK);
      tft->setTextColor(TFT_GREEN);
      tft->setCursor(20, 60);
      tft->println("MQTT CONNECTED");
    }
    return true;
  } else {
    DEBUG_PRINT("failed, rc=");
    DEBUG_PRINTLN(mqttClient.state());
    if (tft != NULL) {
      tft->fillScreen(TFT_BLACK);
      tft->setTextColor(TFT_RED);
      tft->setCursor(20, 60);
      tft->println("MQTT FAILED");
    }
  }
  return false;
}


void initAsyncWebserver()
{
  AsyncElegantOTA.begin(&httpServer);
}

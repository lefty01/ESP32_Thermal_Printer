#include "common.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "wifi_mqtt_creds.h"

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
AsyncWebServer httpServer(80);

String ipAddr;
String dnsAddr;
const unsigned maxWifiWaitSeconds = 60;
unsigned long wifi_reconnect_timer   = 0;
unsigned long wifi_reconnect_counter = 0;

#define EVERY_SECOND 1000
#define WIFI_CHECK   60 * EVERY_SECOND

// must match topics_t order
String mqtt_topics[] {
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/setconfig",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/state",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/version",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/ipaddr",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/rst",
  "home/" MQTT_DEV_TYPE "/%MQTTDEVICEID%/msg"
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


const String getWifiIpAddr() {
  return ipAddr;
}


void checkWifi(bool &wifiAvail, bool &mqttAvail) {
  // re-connect to wifi
  if ((WiFi.status() != WL_CONNECTED) && ((millis() - wifi_reconnect_timer) > WIFI_CHECK)) {
    wifi_reconnect_timer = millis();
    wifiAvail = false;
    DEBUG_PRINTLN("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
  if (! wifiAvail && (WiFi.status() == WL_CONNECTED)) {
    // connection was lost and now got reconnected ...
    wifiAvail = true;
    wifi_reconnect_counter++;
    //show_WIFI(wifi_reconnect_counter, getWifiIpAddr());
  }
  if (! mqttAvail && wifiAvail)
    mqttAvail = mqttConnect(&tft);
}


// note: delays mainly to keep tft text shortly readable
bool setupWifi(TFT_eSPI* tft /*= NULL*/) {
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
      return false;
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
  return true;
}


bool mqttConnect(TFT_eSPI* tft /*= NULL*/) {
  bool rc;
  DEBUG_PRINTLN("Attempting MQTT connection...");
  DEBUG_PRINT("HOST: "); DEBUG_PRINTLN(mqtt_host);
  if (tft != NULL) {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE);
    tft->setTextFont(2);
    tft->setCursor(20, 40);
    tft->print("Connecting to MQTT server: ");
    tft->println(mqtt_host);
    delay(1000);
  }
  // Attempt to connect
#ifdef MQTT_USE_SSL
  DEBUG_PRINTLN("set SSL cert & key");
  wifiClient.setCACert(server_crt_str);
  wifiClient.setCertificate(client_crt_str);
  wifiClient.setPrivateKey(client_key_str);
#endif
  DEBUG_PRINTLN("setServer ...");
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

bool mqttPublish(topics_t t, const char* msg)
{
  return mqttClient.publish(getTopic(t), msg);
}

void initAsyncWebserver()
{
  AsyncElegantOTA.begin(&httpServer);
  httpServer.begin();
}

#include "common.h"

const char* VERSION = "0.0.2";

#include "JoD.h"  // Joke of the Day library (german jokes)

#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#define TX_PIN 17 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 13 // Arduino receive   GREEN WIRE   labeled TX on printer
SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor

TFT_eSPI tft;

String MQTTDEVICEID = "ESP32_Print_";

esp_reset_reason_t rr;
uint8_t mac_addr[6];
bool isWifiAvailable = false;
bool isMqttAvailable = false;


const char* getRstReason(esp_reset_reason_t r) {
  switch(r) {
  case ESP_RST_UNKNOWN:    return "ESP_RST_UNKNOWN";   //!< Reset reason can not be determined
  case ESP_RST_POWERON:    return "ESP_RST_POWERON";   //!< Reset due to power-on event
  case ESP_RST_EXT:        return "ESP_RST_EXT";       //!< Reset by external pin (not applicable for ESP32)
  case ESP_RST_SW:         return "ESP_RST_SW";        //!< Software reset via esp_restart
  case ESP_RST_PANIC:      return "ESP_RST_PANIC";     //!< Software reset due to exception/panic
  case ESP_RST_INT_WDT:    return "ESP_RST_INT_WDT";   //!< Reset (software or hardware) due to interrupt watchdog
  case ESP_RST_TASK_WDT:   return "ESP_RST_TASK_WDT";  //!< Reset due to task watchdog
  case ESP_RST_WDT:        return "ESP_RST_WDT";       //!< Reset due to other watchdogs
  case ESP_RST_DEEPSLEEP:  return "ESP_RST_DEEPSLEEP"; //!< Reset after exiting deep sleep mode
  case ESP_RST_BROWNOUT:   return "ESP_RST_BROWNOUT";  //!< Brownout reset (software or hardware)
  case ESP_RST_SDIO:       return "ESP_RST_SDIO";      //!< Reset over SDIO
  }
  return "INVALID";
}



// void Adafruit_Thermal::setCodePage(uint8_t val) {
//   if(val > 47) val = 47;
//   writeBytes(ASCII_FS, '.');  // Cancel Kanji character mode
//   writeBytes(ASCII_ESC, 't', val);
// }



// This is to hide non-test related source code.
// https://docs.platformio.org/en/latest/plus/unit-testing.html
#ifndef UNIT_TEST
void setup() {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("setup started");
  rr = esp_reset_reason();

  // fixme check return code
  esp_efuse_mac_get_default(mac_addr);

  MQTTDEVICEID += String(mac_addr[4], HEX);
  MQTTDEVICEID += String(mac_addr[5], HEX);
  setupMqttTopic(MQTTDEVICEID);
  DEBUG_PRINT("MQTTDEVICEID: ");
  DEBUG_PRINTLN(MQTTDEVICEID);

//  buttonInit();
  tft.init(); // vs begin??
  tft.setRotation(1); // 3
#ifdef TFT_ROTATE
  tft.setRotation(TFT_ROTATE);
#endif
#ifdef TFT_BL
  if (TFT_BL > 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }
#endif
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Started");

  isWifiAvailable = setupWifi(&tft);

  if (isWifiAvailable) {
    DEBUG_PRINTLN("Init OTA Webserver");
    tft.println("Init OTA Webserver");
    initAsyncWebserver();
    delay(2000);
  }
  //else show offline msg, halt or reboot?!

  if (isWifiAvailable) {
    isMqttAvailable = mqttConnect(&tft);
    delay(2000);
  }


  tft.println("Setup Printer");

  mySerial.begin(9600);  // Initialize SoftwareSerial
  printer.begin();       // Init printer (same regardless of serial type)

  // printer.setFont('B');
  // printer.println("FontB");
  // printer.println("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  //printer.setFont('A');
  // printer.println("FontA (default)");
  // printer.println("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  //printer.println("");
  printer.justify('C');
  printer.boldOn();
  printer.println(F("Witz des Tages"));
  printer.boldOff();

  //printer.setCharset(CHARSET_GERMANY);
  //printer.writeBytes(27, 't', CODEPAGE_ISO_8859_1);
  printer.setCodePage(CODEPAGE_ISO_8859_1);
  printer.justify('L');

  printer.println(F("Der erste Tag für die neue Sekretärin. Das Telefon klingelt mehrmals, aber sie geht nicht an den Apparat. \"Frau Meyer-Schmidthuber, wollen Sie nicht vielleicht mal ans Telefon gehen?\" fragt der Chef argwöhnisch. \"Och nee, wissense, ich hab niemand von meinen neuen Job erzählt, also muss es für Sie sein!\""));

  //printer.justify('R');
  //printer.println(F("Right justified"));


  printer.feed(2);
  printer.setDefault(); // Restore printer to defaults

  tft.println("Setup Done");
  delay(5000L);
}

void loop() {

  checkWifi(isWifiAvailable, isMqttAvailable);

  if (isMqttAvailable) mqttPublish(MQTT_TOPIC_MSG, "msg: in the loop");

  DEBUG_PRINTF("rotation: %d\n", tft.getRotation());
  tft.setRotation(tft.getRotation() + 1 % 3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("LOOP Started");
  tft.println(getWifiIpAddr());

  delay(5000);
}

#endif

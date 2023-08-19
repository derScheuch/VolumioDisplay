#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <StreamUtils.h>




#ifndef DISPLAYMODEL_H
#define DISPLAYMODEL_H

#define MODE_IDLE 0
#define MODE_RELOAD_PICTURE 1
#define MODE_REPRINT_PICTURE 2
#define MODE_PRINT_TITLE 3
#define MODE_WAIT_FOR_WIFI_SETUP 4



class Configuration {
  uint8_t *buf;
public:
  String ssid;
  String password;
  String proxyBaseUrl;
  String volumioBaseUrl;
  String buttonPressCommand_1;
  String buttonPressCommand_2;
  String startCommand;
  int textScrollWait;
  int textDisplayWait;
  Configuration(uint8_t *buf);
  void save();
  void load();
};

class DisplayModel {
public:
  int loopingMode;
  Configuration *configuration;
  int currentBrightness;
  uint8_t pictureBuf[64 * 64 * 3 + 100] = { 0 };
  void saveConfiguration();
  void loadConfiguration();
  DisplayModel();
};



#endif
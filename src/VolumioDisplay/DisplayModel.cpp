#include "DisplayModel.h"

//
// CONFIGURATION
//
// Its alos possible to make this later via web configuration but you can do it
// int the code, too...
//
// enter here the URL where you installed the proxy.php for reskaling the JPEGs
const String _proxyBaseUrl = "http://proxy.local/proxy.php?url=";

// enter here the url where you can access the REST API for your volumio
// will be in most cases something like http://volumio.local
//
const String _volumioBaseUrl = "http://volumio.local";
// enter the ssid of your network
const String _ssid = "";
// and the password
const String _password = "";
const String _startCommand = "http://volumio.local/api/v1/commands/?cmd=playplaylist&name=auto";
;
const String _buttonPressCommand_1 = "http://volumio.local/api/v1/commands/?cmd=toggle";


DisplayModel::DisplayModel() {
  this->configuration = new Configuration(this->pictureBuf);
  this->configuration->load();
}


Configuration::Configuration(uint8_t *_buf) {
  this->buf = _buf;
}

void Configuration::load() {
  Serial.println("Loading Config");
  EEPROM.begin(2048);
  delay(10);
  int i = 0;
  for (int i = 0; i < 600; ++i) {
    buf[i] = EEPROM.read(i);
  }
  DynamicJsonDocument doc(300);
  DeserializationError error = deserializeJson(doc, buf);
  if (error) {
    //loading a preset configurqtion
    Serial.println("Deserialization of config falied");
    this->password = _password;
    this->ssid = _ssid;
    if (this->volumioBaseUrl.length() == 0) {
      this->volumioBaseUrl = _volumioBaseUrl;
      this->proxyBaseUrl = _proxyBaseUrl;
      this->startCommand = _startCommand;
      this->buttonPressCommand_1 = _buttonPressCommand_1;
      this->textDisplayWait = 30;
      this->textScrollWait = 80;
    }
  } else {
    Serial.println("Deserialization of this->SUCCESS");
    this->ssid = doc["ssid"].as<String>();
    this->password = doc["password"].as<String>();
    this->volumioBaseUrl = doc["vbu"].as<String>();
    this->proxyBaseUrl = doc["pbu"].as<String>();
    this->buttonPressCommand_1 = doc["b1c"].as<String>();
    this->buttonPressCommand_2 = doc["b2c"].as<String>();
    this->startCommand = doc["stc"].as<String>();
    this->textDisplayWait = doc["tdw"];
    this->textScrollWait = doc["tsw"];
  }
#if defined(DEBUG)
  Serial.println(this->ssid);
  Serial.println(this->password);
  Serial.println(this->volumioBaseUrl);
  Serial.println(this->proxyBaseUrl);
  Serial.println(this->buttonPressCommand_1);
  Serial.println(this->buttonPressCommand_2);
  Serial.println(this->startCommand);
  Serial.println(this->textDisplayWait);
  Serial.println(this->textScrollWait);
#endif
  EEPROM.end();
  doc.clear();
}

void Configuration::save() {
  Serial.println("saving config");
  DynamicJsonDocument doc(300);
  doc["ssid"] = this->ssid;
  doc["password"] = this->password;
  doc["vbu"] = this->volumioBaseUrl;
  doc["pbu"] = this->proxyBaseUrl;
  doc["tdw"] = this->textDisplayWait;
  doc["tsw"] = this->textScrollWait;
  doc["b1c"] = this->buttonPressCommand_1;
  doc["b2c"] = this->buttonPressCommand_2;
  doc["stc"] = this->startCommand;
  EEPROM.begin(2048);
  for (int L = 0; L < 300; ++L) EEPROM.write(L, 0);
  EepromStream eepromStream(0, 2048);
  serializeJson(doc, eepromStream);
  EEPROM.commit();
  EEPROM.end();
  doc.clear();
  Serial.println("this->saved");
}


#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>
#include <WebServer.h>
#include <StreamUtils.h>
#include <EEPROM.h>

// Define the address in EEPROM where the configuration will be stored
#define DEBUG true



const char CONFIG_PAGE[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Display Configuration</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f0;
      margin: 0;
      padding: 20px;
    }

    .container {
      background-color: #ffffff;
      border-radius: 8px;
      box-shadow: 0px 2px 4px rgba(0, 0, 0, 0.1);
      padding: 20px;
    }

    label {
      font-weight: bold;
      margin-bottom: 5px;
      display: block;
    }

    input, select {
      width: 100%;
      padding: 8px;
      margin-bottom: 15px;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
    }

    button {
      background-color: #4CAF50;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }

    button:hover {
      background-color: #45a049;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>ESP32 Configuration</h2>
    <form action="/setup" method="post">
      <label for="proxyBaseUrl">Proxy Base URL:</label>
      <input type="text" id="proxyBaseUrl" name="proxyBaseUrl" value="%PROXY_BASE_URL%" required>

      <label for="volumioBaseUrl">Volumio Base URL:</label>
      <input type="text" id="volumioBaseUrl" name="volumioBaseUrl" value="%VOLUMIO_BASE_URL%" required>

      <label for="buttonCommand_1">Button 1 Command:</label>
      <input type="text" id="buttonCommand_1" name="buttonCommand_1" value="%BUTTON_COMMAND%_1">

      <label for="buttonCommand_2">Button 2 Command:</label>
      <input type="text" id="buttonCommand_2" name="buttonCommand_2" value="%BUTTON_COMMAND%_2">

      <label for="startCommand">Start Command:</label>
      <input type="text" id="startCommand" name="startCommand" value="%START_COMMAND%">

      <label for="scrollWait">Scroll Wait 5-200(ms):</label>
      <input type="number" id="scrollWait" name="scrollWait" value="%SCROLL_WAIT%" required>

      <label for="textDisplayWait">Text Display Wait 0 - 200(s):</label>
      <input type="number" id="textDisplayWait" name="textDisplayWait" value="%TEXT_DISPLAY_WAIT%" required>

      <label for="brightness">Text Display Wait (ms):</label>
      <input type="number" id="brightness" name="brightness" value="%BRIGHTNESS%" required>
      <button type="submit">Save Configuration</button>
    </form>
  </div>
  <script>
    // JavaScript hier einfügen, um %PROXY_BASE_URL%, %VOLUMIO_BASE_URL%, etc. zu ersetzen
  </script>
</body>
</html>
)=====";

const char NETWORK_PAGE[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Network Configuration</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f0;
      margin: 0;
      padding: 20px;
    }

    .container {
      background-color: #ffffff;
      border-radius: 8px;
      box-shadow: 0px 2px 4px rgba(0, 0, 0, 0.1);
      padding: 20px;
    }

    label {
      font-weight: bold;
      margin-bottom: 5px;
      display: block;
    }

    input, select {
      width: 100%;
      margin-bottom: 15px;
      border: 1px solid #ccc;
      padding: 8px;
      border-radius: 4px;
      box-sizing: border-box;
    }

    button {
      background-color: #4CAF50;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }

    button:hover {
      background-color: #45a049;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Network Configuration</h2>
    <form action="/saveNetwork" method="post">
      <label for="ssid">Network Name (SSID):</label>
      <input type="text" id="ssid" name="ssid" required>

      <label for="password">Password:</label>
      <input type="password" id="password" name="password" required>

      <label for="deviceName">Device Name:</label>
      <input type="text" id="deviceName" name="deviceName" required>

      <button type="submit">Save Network Configuration</button>
    </form>
  </div>
</body>
</html>
)=====";

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
const String _startCommand = "http://volumio.local/api/v1/commands/?cmd=playplaylist&name=auto";;
const String _buttonPressCommand_1 = "http://volumio.local/api/v1/commands/?cmd=toggle";
    


// dont change these unless volumio api changes
const String stateUrl = "/api/v1/getState";
const String pushNotificationUrl = "/api/v1/pushNotificationUrls";
const String volumeApiUrl = "/api/v1/commands/?cmd=volume&volume="; 


struct Configuration {
  String ssid;
  String password;
  String proxyBaseUrl;
  String volumioBaseUrl;
  String buttonPressCommand_1;
  String buttonPressCommand_2;
  String startCommand;
  int textScrollWait;
  int textDisplayWait;
};

Configuration configuration;


#define MODE_IDLE 0
#define MODE_RELOAD_PICTURE 1
#define MODE_REPRINT_PICTURE 2
#define MODE_PRINT_TITLE 3
#define MODE_WAIT_FOR_WIFI_SETUP 4

uint8_t loopingMode;

WebServer webServer(80);




#define PANEL_RES_X 64  // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 64  // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1   // Total number of panels chained one to another

//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;



// The picture Buffer (64 +x 64 pixel, each 3 bytes)
uint8_t pictureBuf[64 * 64 * 3 + 100] = { 0 };


char currentTitle[300];
float currentMaxDivideRatio = 3.0;  // always between 1.2 and 3.0
uint16_t currentTextColor;
int currentBrightness;
int currentVolume;
int textScrollingOffset = 0;
long milliSteps = 0;
int lastStateButton1 = 0;
int lastStateButton2 = 0;

int lastAnalogRead1 = 0;
int lastAnalogRead2 = 0;
long lastAnalogCahngedMillis = 0;
#define BUTTON_1 18
#define BUTTON_2 19

#define ANALOG_READ_1 34
#define ANALOG_READ_2 35

void setup() {
  Serial.begin(115200);
  loadConfiguration(configuration);
  configureDisplay();
  pinMode(BUTTON_1, INPUT_PULLDOWN);
  pinMode(BUTTON_2, INPUT_PULLDOWN);
  if (connectToWiFi()) {
    loopingMode = MODE_RELOAD_PICTURE;
  } else {
    loopingMode = MODE_WAIT_FOR_WIFI_SETUP;
  }
  lastAnalogRead1 = analogRead(ANALOG_READ_1) / 256;
  lastAnalogRead2 = analogRead(ANALOG_READ_2) / 256;
}


void loop() {
  webServer.handleClient();
  if (loopingMode == MODE_RELOAD_PICTURE) {
    if (getVolumioStatus() == 1) {
      loopingMode = MODE_REPRINT_PICTURE;
    } else {
      delay(1000);
    }
  }
  if (loopingMode == MODE_REPRINT_PICTURE) {
    printPicture();
    calculateCurrentTextColor();
    loopingMode = MODE_PRINT_TITLE;
    textScrollingOffset = 0;
  }
  if (loopingMode == MODE_PRINT_TITLE) {
    if (millis() > milliSteps + configuration.textScrollWait) {
      textScrollingOffset = setText(textScrollingOffset);
      milliSteps = millis();
      if (textScrollingOffset < 0) {
        loopingMode = MODE_IDLE;
      }
    }
  }
  if (loopingMode == MODE_WAIT_FOR_WIFI_SETUP) {
    // just wait...
  }
  if (loopingMode == MODE_IDLE) {
    if (millis() > milliSteps + configuration.textDisplayWait * 1000) {
      milliSteps = millis();
      loopingMode = MODE_PRINT_TITLE;
      textScrollingOffset = 0;
    }
  }
  handleInputs();
  delay(2);
}

void handleInputs() {

  int buttonState = digitalRead(BUTTON_1);

  if (buttonState != lastStateButton1) {
    Serial.print("ButtonState1 ");
    Serial.println(buttonState);
    lastStateButton1 = buttonState;
    if (buttonState == 0) {
      makeSimpleGetCall(configuration.buttonPressCommand_1);
    }
  }/*
  buttonState = digitalRead(BUTTON_2);


  if (buttonState != lastStateButton2) {
    Serial.print("ButtonState2 ");
    Serial.println(buttonState);
    lastStateButton2 = buttonState;
    if (buttonState == 0) {
      makeSimpleGetCall(configuration.buttonPressCommand_2);
    }
  }
  */
  int _analogRead = analogRead(ANALOG_READ_1) / 256;
  if (_analogRead != lastAnalogRead1) {
    Serial.print("analogRead1 ");
    Serial.println(_analogRead);
    lastAnalogCahngedMillis = millis();
    lastAnalogRead1 = _analogRead;

  } else if (_analogRead == lastAnalogRead1 && lastAnalogCahngedMillis > 0) {
    if (millis() - lastAnalogCahngedMillis > 100) {
      lastAnalogCahngedMillis = 0;
      //setBrightness(_analogRead * 16);
    }
  }
  _analogRead = analogRead(ANALOG_READ_2) / 256;
  if (_analogRead != lastAnalogRead2) {
    Serial.print("analogRead2 ");
    Serial.println(_analogRead);
    lastAnalogCahngedMillis = millis();
    lastAnalogRead2 = _analogRead;

  } else if (_analogRead == lastAnalogRead2 && lastAnalogCahngedMillis > 0) {
    if (millis() - lastAnalogCahngedMillis  > 100) {
      lastAnalogCahngedMillis = 0;
      //setVolume((_analogRead * 100) / 16);
    }
  }
}
void setVolume(int vol) {
  if (vol >= 0 && vol <= 100) {
    makeSimpleGetCall(configuration.volumioBaseUrl + volumeApiUrl + vol);
  } else {
    Serial.print("VOLUNE CANT BE SET TO ");
    Serial.println(vol);
  }
}
int getVolumioStatus() {
  HTTPClient httpClient;
  httpClient.begin(configuration.volumioBaseUrl + stateUrl);
  int httpCode = httpClient.GET();
  int pictureLoaded = 0;
  if (httpCode == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, httpClient.getStream());
    httpClient.end();
    String artist = doc["artist"];
    String album = doc["album"];
    String service = doc["service"];
    String title2 = doc["title"];
    currentVolume = doc["volume"];
    String albumart = "";
    if (service == "webradio") {
      pictureLoaded = getPictureFromTitle(title2);
    }
    if (pictureLoaded == 0l) {
      String a = doc["albumart"];
      albumart = a;
      pictureLoaded = getPicture(configuration.proxyBaseUrl + urlEncode(albumart));
    }

    title2 = replaceSpecialCharacters(title2);
    title2.toCharArray(currentTitle, title2.length() > 299 ? 299 : title2.length() + 1);

    doc.clear();
  }
  return pictureLoaded;
}

int getPictureFromTitle(const String _title) {
  String title = _title;
  title.replace(" ", "_");
  title.toLowerCase();
  int pictureLoaded = 0;
  // we want to show the artist of the title
  // most webradios provide sometzhing like "$artist - $title" in their title tag
  int pos = title.indexOf("_-");
  if (pos < 0) {
    pos = title.indexOf("_/");  //sometimes it will be divided by a slash
  }
  if (pos > 0) {
    // this is just a blind guess.., jazz comes often with artists like "Duke Ellington & John Coltrane"
    // but that normally leads to no result...
    int pos2 = title.indexOf("_&");

    if (pos2 < 0) {
      pos2 = title.indexOf(",_");
    }
    if ((pos2 > 0) && pos2 < pos) {
      pictureLoaded = tryLoadAndCheckPicture("/tinyart/" + title.substring(0, pos2) + "/large");
    }
    if (!pictureLoaded) {
      pictureLoaded = tryLoadAndCheckPicture("/tinyart/" + title.substring(0, pos) + "/large");
    }
  }
  return pictureLoaded;
}

int tryLoadAndCheckPicture(String albumArt) {
  Serial.print("tryLoadAndCheckPicture: ");
  Serial.println(albumArt);
  int pictureLoaded = getPicture(configuration.proxyBaseUrl + urlEncode(albumArt));
  if (pictureLoaded) {
    // check if it is a valid picture and not a dummy one.
    // very simple guessing mechanism... checks if all pixels in the beginning are all black
    int a = 0;
    for (int i = 0; i < 400; ++i) {
      a += pictureBuf[i];
    }
    if (a == 0) {
      Serial.println("picture from tinyaert starts completely black... fallback");
      pictureLoaded = 0;
    }
  }
  return pictureLoaded;
}


String replaceSpecialCharacters(String input) {
  input.replace("ä", "ae");
  input.replace("ö", "oe");
  input.replace("ü", "ue");
  input.replace("ß", "ss");
  input.replace("à", "a");
  input.replace("â", "a");
  input.replace("ç", "c");
  input.replace("è", "e");
  input.replace("é", "e");
  input.replace("ê", "e");
  input.replace("î", "i");
  input.replace("ï", "i");
  input.replace("ô", "o");
  input.replace("ù", "u");
  input.replace("û", "u");
  String filteredInput = "";
  for (size_t i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (c >= 32 && c <= 126) {  // Zeichen im ASCII-Bereich
      filteredInput += c;
    }
  }
  return filteredInput;
}

int getPicture(const String imageUrl) {
  Serial.println("getPicture");
  HTTPClient httpClient;
  int returnCode = 0;
  httpClient.begin(imageUrl);
  httpClient.useHTTP10(true);
  int httpCode = httpClient.GET();
  if (httpCode == 200) {
    WiFiClient *stream = httpClient.getStreamPtr();
    int index = 0;
    while (httpClient.connected()) {
      size_t size = stream->available();
      if (size) {
        int c = stream->readBytes(pictureBuf + index, size);
        index += c;
      } else {
        break;
      }
    }
    returnCode = (index == 64 * 64 * 3);
  }
  httpClient.end();
  return returnCode;
}

void printPicture() {
  printPicture(0, 64, 1.0);
}

void calculateCurrentTextColor() {
  Serial.println("calculateCurrentTextColor");
  int index = 0;
  int highesIndex = 0;
  long highesValue = 0;
  long middle = 0;
  // calculate the hightes pixel and the medium level of brightness
  for (int height = 0; height < 8; ++height) {
    for (int width = 0; width < 64; ++width) {
      long value = pictureBuf[index] * pictureBuf[index] + pictureBuf[index] * pictureBuf[index + 2] + pictureBuf[index + 2] * pictureBuf[index + 2];
      if (value > highesValue) {
        highesIndex = index;
        highesValue = value;
      }
      middle += pictureBuf[index];
      middle += pictureBuf[index + 1];
      middle += pictureBuf[index + 2];
      index += 3;
    }
  }
  middle /= (64 * 8 * 3);
  Serial.println(middle);
  currentMaxDivideRatio = 1.2 + ((1.8 * (float)middle) / 255.0);
  Serial.print("currentMaxDivideRatio");
  Serial.println(currentMaxDivideRatio);
  
  int r = pictureBuf[highesIndex];
  int g = pictureBuf[highesIndex + 1];
  int b = pictureBuf[highesIndex + 2];

  int level = (r + g + b) / 3;

  if (level < 20) {
    currentTextColor = dma_display->color565(127, 127, 127);
    return;
  }
  r = level + ((r - level) * 3) / 4;
  b = level + ((b - level) * 3) / 4;
  g = level + ((g - level) * 3) / 4;

  if (level < middle + 20) {
    r = (r * (middle + 20) )/ level;
    b = (b * (middle + 20) )/ level;
    b = (b * (middle + 20) )/ level;
  }

  if (r > 255) {
    g = g * 255 / r;
    b = b * 255 / r;
    r = 255;
  }
  if (b > 255) {
    r = r * 255 / b;
    g = g * 255 / b;
    b = 255;
  }
  if (g > 255) {
    r = r * 255 / g;
    b = b * 255 / g;
    g = 255;
  }
  currentTextColor = dma_display->color565(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}

void printPicture(int fromLine, int toLine, float divide) {
  int index = fromLine * 64 * 3;
  for (int height = fromLine; height < toLine; ++height) {
    for (int width = 0; width < 64; ++width) {
      if (divide == 1.0) {
        dma_display->drawPixelRGB888(width, height, pictureBuf[index], pictureBuf[index + 1], pictureBuf[index + 2]);
      } else if (divide > 1 && divide <= 3) {
        dma_display->drawPixelRGB888(width, height, (int)((float)pictureBuf[index] / divide), (int)((float)pictureBuf[index + 1] / divide), (int)((float)pictureBuf[index + 2] / divide));
      } else {
        Serial.println("WTF?");
        Serial.println(divide);
      }
      index += 3;
    }
  }
}

int setText(int textScrollingOffset) {
  long printTime = millis();
  int divideSteps = 64;
  float maxDivide = currentMaxDivideRatio;
  float stepDivide = (maxDivide - 1.0) / divideSteps;
  int maxOffset = strlen(currentTitle) * 6 + 70;

  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);
  float divide = maxDivide;
  if (textScrollingOffset < divideSteps) {
    // fade out background
    divide = 1.0 + textScrollingOffset * stepDivide;
  } else if (textScrollingOffset > maxOffset - divideSteps) {
    // fade id background
    divide = 1.0 + (maxOffset - (textScrollingOffset - 1)) * stepDivide;
  }
  printPicture(0, 7, divide);
  printPicture(7, 8, 1.0 + 3 * (divide - 1) / 4);
  printPicture(8, 9, 1.0 + (divide - 1) / 2);
  printPicture(9, 10, 1.0 + (divide - 1) / 5);
  dma_display->setCursor(63 - textScrollingOffset, 0);
  dma_display->setTextColor(currentTextColor);
  dma_display->print(currentTitle);
  if (textScrollingOffset > maxOffset) {
    // end scrolling
    textScrollingOffset = -10;  // 10000;
  }

  printTime -= millis();
  return textScrollingOffset + 1;
}


int connectToWiFi() {
  Serial.println("Connecting to WiFi...");

  if (configuration.password.length() == 0 || configuration.ssid.length() == 0) {
    Serial.println("no configuration found... opening own AP");
    startOwnAP();
    return 0;
  }
  WiFi.begin(configuration.ssid, configuration.password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 300) {
    Serial.println("Connecting to WiFi...");
    dma_display->drawPixelRGB888(tries / 64, tries % 64, 127, 127, 127);
    ++tries;
    delay(200);
  }
  if (tries >= 300) {
    Serial.println("Could not coneect to WiFi... opening own AP");
    startOwnAP();
    return 0;
  }
  Serial.println("Connected to WiFi!");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  setupWebServer();
  startupCalls();
  return 1;
}

void startOwnAP() {
  Serial.println("startOwnAp");
  WiFi.softAP("ESP32", "Hello");

  IPAddress apIP(192, 168, 1, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  Serial.println("Own WiFi network started. IP Address: " + WiFi.softAPIP().toString());
  setupWebServer();
}

void configureDisplay() {
  Serial.println("configureDisplay");
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,  // module width
    PANEL_RES_Y,  // module height
    PANEL_CHAIN   // Chain length
  );

  mxconfig.gpio.e = 32;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  setBrightness(120);
  dma_display->clearScreen();
}

void setupWebServer() {
  Serial.println("setupWebServer");
  webServer.on("/push", webServerPush);
  webServer.on("/setup", HTTP_GET, handleGETRequestSetup);
  webServer.on("/setup", HTTP_POST, handlePOSTRequestSetup);
  webServer.on("/saveWifi", HTTP_POST, handleSaveWifi);
  webServer.on("/", HTTP_GET, handleSetupWifi);
  webServer.onNotFound(webServerNotFound);
  webServer.begin();
  Serial.println("HTTP server started");
}

void handleGETRequestSetup() {
  String configPage = CONFIG_PAGE;
  configPage.replace("%PROXY_BASE_URL%", configuration.proxyBaseUrl);
  configPage.replace("%VOLUMIO_BASE_URL%", configuration.volumioBaseUrl);
  configPage.replace("%BUTTON_COMMAND%_1", configuration.buttonPressCommand_1);
  configPage.replace("%BUTTON_COMMAND%_2", configuration.buttonPressCommand_2);
  configPage.replace("%START_COMMAND%", configuration.startCommand);

  configPage.replace("%SCROLL_WAIT%", String(configuration.textScrollWait));
  configPage.replace("%TEXT_DISPLAY_WAIT%", String(configuration.textDisplayWait));
  configPage.replace("%BRIGHTNESS%", String(currentBrightness));
  webServer.send(200, "text/html", configPage);
}

void handlePOSTRequestSetup() {
  String proxyBaseUrl = webServer.arg("proxyBaseUrl");
  String volumioBaseUrl = webServer.arg("volumioBaseUrl");
  String buttonCommand_1 = webServer.arg("buttonCommand_1");
  String buttonCommand_2 = webServer.arg("buttonCommand_2");
  String startCommand = webServer.arg("startCommand");
  int scrollWait = constrain(webServer.arg("scrollWait").toInt(), 0, 201);
  int textDisplayWait = constrain(webServer.arg("textDisplayWait").toInt(), 0, 201);
  int brightness = constrain(webServer.arg("brightness").toInt(), 0, 255);

  //if (strlen(proxyBaseUrl) > 0)
  configuration.proxyBaseUrl = proxyBaseUrl;
  //if (strlen(volumioBaseUrl) > 0)
  configuration.volumioBaseUrl = volumioBaseUrl;

  configuration.buttonPressCommand_1 = buttonCommand_1;
  configuration.buttonPressCommand_2 = buttonCommand_2;
  configuration.startCommand = startCommand;

  if (scrollWait != 201)
    configuration.textScrollWait = scrollWait;
  if (textDisplayWait != 601)
    configuration.textDisplayWait = textDisplayWait;
  saveConfiguration(configuration);
  setBrightness(brightness);
  handleGETRequestSetup();
}

void setBrightness(int brightness) {
  if (brightness >= 0 && brightness < 180) {
    currentBrightness = brightness;
    dma_display->setBrightness8(brightness);
  }
}


void handleSetupWifi() {
  webServer.send(200, "text/html", NETWORK_PAGE);
}

void handleSaveWifi() {
  String newSSID = webServer.arg("ssid");
  String newPassword = webServer.arg("password");
  configuration.password = newPassword;
  configuration.ssid = newSSID;
  saveConfiguration(configuration);
  webServer.send(200, "text/plain", "Data saved and connected to WiFi.");
  connectToWiFi();
}


void webServerNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
}

void webServerPush() {
  webServer.send(200, "text/plain", "getting state and reloading picture");
  loopingMode = MODE_RELOAD_PICTURE;
}

void startupCalls() {
  notifyVolumio();
  makeSimpleGetCall(configuration.startCommand);
}

int makeSimpleGetCall(String request) {
#if defined(DEBUG)
  Serial.print("makesimpleGetCall: ");
  Serial.println(request);
#endif
  if (request.length() > 0) {
    HTTPClient httpClient;
    WiFiClient wifiClient;
    httpClient.begin(wifiClient, request);
    int responseCode = httpClient.GET();
    httpClient.end();
    return responseCode;
  } else {
    return 1;
  }
}

int notifyVolumio() {
  WiFiClient client;
  HTTPClient http;
  String ip = WiFi.localIP().toString();
  http.begin(client, configuration.volumioBaseUrl + pushNotificationUrl);
  http.addHeader("Content-Type", "application/json");  // Data to send with HTTP POST
  String httpRequestData = "{\"url\": \"http://" + ip + "/push\"}";
  int httpResponseCode = http.POST(httpRequestData);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  http.end();
  return httpResponseCode;
}


// Function to save the configuration to EEPROM
void saveConfiguration(const Configuration &config) {
  Serial.println("saving config");
  DynamicJsonDocument doc(300);
  doc["ssid"] = config.ssid;
  doc["password"] = config.password;
  doc["vbu"] = config.volumioBaseUrl;
  doc["pbu"] = config.proxyBaseUrl;
  doc["tdw"] = config.textDisplayWait;
  doc["tsw"] = config.textScrollWait;
  doc["b1c"] = config.buttonPressCommand_1;
  doc["b2c"] = config.buttonPressCommand_2;
  doc["stc"] = config.startCommand;
  EEPROM.begin(2048);
  for (int L = 0; L < 300; ++L) EEPROM.write(L, 0);
  EepromStream eepromStream(0, 2048);
  serializeJson(doc, eepromStream);
  EEPROM.commit();
  EEPROM.end();
  doc.clear();
  Serial.println("config saved");
}

// Function to load the configuration from EEPROM
void loadConfiguration(Configuration &config) {
  Serial.println("Loading Config");
  EEPROM.begin(2048);
  delay(10);
  int i = 0;
  for (int i = 0; i < 600; ++i) {
    pictureBuf[i] = EEPROM.read(i);
  }
  DynamicJsonDocument doc(300);
  DeserializationError error = deserializeJson(doc, pictureBuf);
  if (error) {
    //loading a preset configurqtion
    Serial.println("Deserialization of config falied");
    config.password = _password;
    config.ssid = _ssid;
    if (config.volumioBaseUrl.length() == 0) {
      config.volumioBaseUrl = _volumioBaseUrl;
      config.proxyBaseUrl = _proxyBaseUrl;
      config.startCommand = _startCommand; 
      config.buttonPressCommand_1 = _buttonPressCommand_1;
      config.textDisplayWait = 30;
      config.textScrollWait = 80;
    }
  } else {
    Serial.println("Deserialization of config SUCCESS");
    config.ssid = doc["ssid"].as<String>();
    config.password = doc["password"].as<String>();
    config.volumioBaseUrl = doc["vbu"].as<String>();
    config.proxyBaseUrl = doc["pbu"].as<String>();
    config.buttonPressCommand_1 = doc["b1c"].as<String>();
    config.buttonPressCommand_2 = doc["b2c"].as<String>();
    config.startCommand = doc["stc"].as<String>();
    config.textDisplayWait = doc["tdw"];
    config.textScrollWait = doc["tsw"];
  }
#if defined(DEBUG)
  Serial.println(config.ssid);
  Serial.println(config.password);
  Serial.println(config.volumioBaseUrl);
  Serial.println(config.proxyBaseUrl);
  Serial.println(config.buttonPressCommand_1);
  Serial.println(config.buttonPressCommand_2);
  Serial.println(config.startCommand);
  Serial.println(config.textDisplayWait);
  Serial.println(config.textScrollWait);
#endif
  EEPROM.end();
  doc.clear();
}

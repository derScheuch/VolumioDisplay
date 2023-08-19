
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>


#include "DisplayWebserver.h"
#include "DisplayModel.h"

// Define the address in EEPROM where the configuration will be stored
#define DEBUG true





// dont change these unless volumio api changes
const String stateUrl = "/api/v1/getState";
const String pushNotificationUrl = "/api/v1/pushNotificationUrls";
const String volumeApiUrl = "/api/v1/commands/?cmd=volume&volume="; 















#define PANEL_RES_X 64  // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 64  // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1   // Total number of panels chained one to another

//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;



// The picture Buffer (64 +x 64 pixel, each 3 bytes)



char currentTitle[300];
float currentMaxDivideRatio = 3.0;  // always between 1.2 and 3.0
uint16_t currentTextColor;

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

DisplayModel *model;
DisplayWebServer *webServer;

void setup() {
  Serial.begin(115200);
  model = new DisplayModel();
  webServer = new DisplayWebServer(model);
  
  //loadConfiguration(model->configuration);
  configureDisplay();
  pinMode(BUTTON_1, INPUT_PULLDOWN);
  pinMode(BUTTON_2, INPUT_PULLDOWN);
  if (connectToWiFi()) {
    model->loopingMode = MODE_RELOAD_PICTURE;
  } else {
    model->loopingMode = MODE_WAIT_FOR_WIFI_SETUP;
  }
  lastAnalogRead1 = analogRead(ANALOG_READ_1) / 256;
  lastAnalogRead2 = analogRead(ANALOG_READ_2) / 256;
}


void loop() {
    webServer->handleCalls();
  if (model->loopingMode == MODE_RELOAD_PICTURE) {
    if (getVolumioStatus() == 1) {
      model->loopingMode = MODE_REPRINT_PICTURE;
    } else {
      delay(1000);
    }
  }
  if (model->loopingMode == MODE_REPRINT_PICTURE) {
    printPicture();
    calculateCurrentTextColor();
    model->loopingMode = MODE_PRINT_TITLE;
    textScrollingOffset = 0;
  }
  if (model->loopingMode == MODE_PRINT_TITLE) {
    if (millis() > milliSteps + model->configuration->textScrollWait) {
      textScrollingOffset = setText(textScrollingOffset);
      milliSteps = millis();
      if (textScrollingOffset < 0) {
        model->loopingMode = MODE_IDLE;
      }
    }
  }
  if (model->loopingMode == MODE_WAIT_FOR_WIFI_SETUP) {
    // just wait...
  }
  if (model->loopingMode == MODE_IDLE) {
    if (millis() > milliSteps + model->configuration->textDisplayWait * 1000) {
      milliSteps = millis();
      model->loopingMode = MODE_PRINT_TITLE;
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
      makeSimpleGetCall(model->configuration->buttonPressCommand_1);
    }
  }/*
  buttonState = digitalRead(BUTTON_2);


  if (buttonState != lastStateButton2) {
    Serial.print("ButtonState2 ");
    Serial.println(buttonState);
    lastStateButton2 = buttonState;
    if (buttonState == 0) {
      makeSimpleGetCall(configuration->buttonPressCommand_2);
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
    makeSimpleGetCall(model->configuration->volumioBaseUrl + volumeApiUrl + vol);
  } else {
    Serial.print("VOLUNE CANT BE SET TO ");
    Serial.println(vol);
  }
}
int getVolumioStatus() {
  HTTPClient httpClient;
  httpClient.begin(model->configuration->volumioBaseUrl + stateUrl);
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
      pictureLoaded = getPicture(model->configuration->proxyBaseUrl + urlEncode(albumart));
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
  int pictureLoaded = getPicture(model->configuration->proxyBaseUrl + urlEncode(albumArt));
  if (pictureLoaded) {
    // check if it is a valid picture and not a dummy one.
    // very simple guessing mechanism... checks if all pixels in the beginning are all black
    int a = 0;
    for (int i = 0; i < 400; ++i) {
      a += model->pictureBuf[i];
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
        int c = stream->readBytes(model->pictureBuf + index, size);
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
      long value = model->pictureBuf[index] * model->pictureBuf[index] + model->pictureBuf[index] * model->pictureBuf[index + 2] + model->pictureBuf[index + 2] * model->pictureBuf[index + 2];
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

  if (model->configuration->password.length() == 0 || model->configuration->ssid.length() == 0) {
    Serial.println("no configuration found... opening own AP");
    startOwnAP();
    return 0;
  }
  WiFi.begin(model->configuration->ssid, model->configuration->password);
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




void setBrightness(int brightness) {
  if (brightness >= 0 && brightness < 180) {
    model->currentBrightness = brightness;
    dma_display->setBrightness8(brightness);
  }
}



void startupCalls() {
  notifyVolumio();
  makeSimpleGetCall(model->configuration->startCommand);
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
  http.begin(client, model->configuration->volumioBaseUrl + pushNotificationUrl);
  http.addHeader("Content-Type", "application/json");  // Data to send with HTTP POST
  String httpRequestData = "{\"url\": \"http://" + ip + "/push\"}";
  int httpResponseCode = http.POST(httpRequestData);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  http.end();
  return httpResponseCode;
}



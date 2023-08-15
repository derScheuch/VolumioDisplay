
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>
#include <WebServer.h>

// enter yozr SSID for your WiFi
//
const char *ssid = "";

// enter your password for your WiFi
//
const char *password = "";

// enter here the URL where you installed the proxy.php for reskaling the JPEGs
//
const String proxyBaseUrl = "http://proxy.local/index2.php?url=";

// enter here the url where you can access the REST API for your volumio
// will be in most cases something like http://volumio.local
//
const String volumioBaseUrl = "http://volumio.local";

const String stateUrl = volumioBaseUrl + "/api/v1/getState";
const String pushNotificationUrl = volumioBaseUrl + "/api/v1/pushNotificationUrls";

#define MODE_IDLE 0
#define MODE_RELOAD_PICTURE 1
#define MODE_REPRINT_PICTURE 2
#define MODE_PRINT_TITLE 3
uint8_t mode;

#define TEXT_SCROLL_WAIT 100
#define TEXT_DISPLAY_WAIT 30000l

WebServer webServer(80);

uint16_t textColor;



#define PANEL_RES_X 64  // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 64  // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1   // Total number of panels chained one to another

//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;



// The picture Buffer (64 +x 64 pixel, each 3 bytes)
uint8_t pictureBuf[64 * 64 * 3 + 100] = { 0 };
char currentTitle[300];

void setup() {
  Serial.begin(115200);
  configureDisplay();
  connectToWiFi();
  setupWebServer();
  notifyVolumio();
  mode = MODE_RELOAD_PICTURE;
}

int offset = 0;
long milliSteps = 0;

void loop() {
  webServer.handleClient();
  if (mode == MODE_RELOAD_PICTURE) {
    if (getVolumioStatus() == 1) {
      printPicture();
      offset = 0;
      calcTextColor();
      mode = MODE_PRINT_TITLE;
    } else {
      delay(1000);
    }
  } else if (mode == MODE_REPRINT_PICTURE) {
    printPicture();
    mode = MODE_PRINT_TITLE;
    offset = 0;
  } else if (mode == MODE_PRINT_TITLE) {
    if (millis() > milliSteps + TEXT_SCROLL_WAIT) {
      offset = setText(offset);
      milliSteps = millis();
      if (offset < 0) {
        mode = MODE_IDLE;
      }
    }
  } else {
    if (millis() > milliSteps + TEXT_DISPLAY_WAIT) {
      milliSteps = millis();
      mode = MODE_PRINT_TITLE;
      offset = 0;
    }
  }
  delay(2);
}

int getVolumioStatus() {
  HTTPClient httpClient;
  httpClient.begin(stateUrl);
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
    String albumart = "";
    if (service == "webradio") {
      pictureLoaded = getPictureFromTitle(title2);
    }
    if (pictureLoaded == 0l) {
      String a = doc["albumart"];
      albumart = a;
      pictureLoaded = getPicture(proxyBaseUrl + urlEncode(albumart));
    }

    title2 = replaceSpecialCharacters(title2);
    title2.toCharArray(currentTitle, title2.length() > 299 ? 299 : title2.length() + 1);

    doc.clear();
  }
  return pictureLoaded;
}

int getPictureFromTitle(const String _title) {
  Serial.println(_title);
  String title = _title;
  Serial.println(title);
   title.replace(" ", "_");
   Serial.println(title);
   title.toLowerCase();
   Serial.println(title);
  int pictureLoaded = 0;
  // we want to show the artist of the title
  // most webradios provide sometzhing like "$artist - $title" in their title tag
  int pos = title.indexOf("_-");
   Serial.println(pos);
  if (pos < 0) {
    pos = title.indexOf("_/");  //sometimes it will be divided by a slash
  }
  Serial.println(title);
  Serial.println(pos);
    if (pos > 0) {
    // this is just a blind guess.., jazz comes often with artists like "Duke Ellington & John Coltrane"
    // but that normally leads to no result...
    int pos2 = title.indexOf("_&");
    
    if (pos2 < 0) {
      pos2 = title.indexOf("_,");
    }
    Serial.println(pos2);
    if ( (pos2 > 0) && pos2 < pos) {
       pictureLoaded = tryLoadAndCheckPicture("/tinyart/" + title.substring(0, pos2) + "/large");
    }
    if (!pictureLoaded) {
      pictureLoaded = tryLoadAndCheckPicture("/tinyart/" + title.substring(0, pos) + "/large");
    }
  }
  return pictureLoaded;
}

int tryLoadAndCheckPicture(String albumArt) {
  Serial.println(albumArt);
  int pictureLoaded = getPicture(proxyBaseUrl + urlEncode(albumArt));
  if (pictureLoaded) {
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
  // Weitere Ersetzungen hier hinzufügen

  // Entferne alle Zeichen, die nicht im normalen ASCII-Bereich liegen
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

void calcTextColor() {
  int index = 0;
  int highesIndex = 0;
  long highesValue = 0;
  long middle = 0;
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
  int r, g, b;

  r = pictureBuf[highesIndex];
  g = pictureBuf[highesIndex + 1];
  b = pictureBuf[highesIndex + 2];

  int level = (r + g + b) / 3;

  if (level < 20) {
    textColor = dma_display->color565(127, 127, 127);
    return;
  }
  r = level + (r - level) * 3 / 4;
  b = level + (b - level) * 3 / 4;
  g = level + (g - level) * 3 / 4;

  if (level < middle + 20) {
    r = r * (middle + 20) / level;
    b = b * (middle + 20) / level;
    b = b * (middle + 20) / level;
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
  textColor = dma_display->color565(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}

void printPicture(int fromLine, int toLine, float divide) {
  int index = fromLine * 64 * 3;
  for (int height = fromLine; height < toLine; ++height) {
    for (int width = 0; width < 64; ++width) {
      if (divide == 1.0) {
        dma_display->drawPixelRGB888(width, height, pictureBuf[index], pictureBuf[index + 1], pictureBuf[index + 2]);
      } else {
        dma_display->drawPixelRGB888(width, height, (int)((float)pictureBuf[index] / divide), (int)((float)pictureBuf[index + 1] / divide), (int)((float)pictureBuf[index + 2] / divide));
      }
      index += 3;
    }
  }
}

int setText(int offset) {
  long printTime = millis();
  int divideSteps = 64;
  float maxDivide = 3.0;
  float stepDivide = (maxDivide - 1.0) / divideSteps;
  int maxOffset = strlen(currentTitle) * 6 + 70;


  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);
  float divide = maxDivide;
  if (offset < divideSteps) {
    divide = 1.0 + offset * stepDivide;
  } else if (offset > maxOffset - divideSteps) {
    divide = 1 + 0 + (maxOffset - (offset - 1)) * stepDivide;
  }
  printPicture(0, 7, divide);
  printPicture(7, 8, 1.0 + 3 * (divide - 1) / 4);
  printPicture(8, 9, 1.0 + (divide - 1) / 2);
  printPicture(9, 10, 1.0 + (divide - 1) / 5);
  dma_display->setCursor(63 - offset, 0);
  dma_display->setTextColor(textColor);
  dma_display->print(currentTitle);
  if (offset > maxOffset) {
    offset = -10;  // 10000;
  }

  //Serial.print("millis for print:");
  printTime -= millis();
  //Serial.println(printTime);
  return offset + 1;
}


void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    dma_display->drawPixelRGB888(tries / 64, tries % 64, 127, 127, 127);
    ++tries;
    delay(100);
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
}

void configureDisplay() {
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
  dma_display->setBrightness8(120);  //0-255
  dma_display->clearScreen();
}

void setupWebServer() {
  webServer.on("/push", webServerPush);
  webServer.on("/setup", HTTP_GET, handleSetup);
  webServer.on("/saveSetup", HTTP_POST, handleSaveSetup);
  webServer.on("/saveWifi", HTTP_POST, handleSaveWifi);
  webServer.onNotFound(webServerNotFound);
  webServer.begin();
  Serial.println("HTTP server started");
}

void handleSetup() {
  String html = "<html><body>";
  html += "<form action='/saveSetup' method='POST'>";
  html += "brightness: <input type='text' name='brightness'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";
  html += "</body></html>";
  webServer.send(200, "text/html", html);
}

void handleSaveSetup() {
  char *endPtr;
  char brightness[20];
  String str = webServer.arg("brightness");
  str.toCharArray(brightness, 20);
  int nuumber = strtol(brightness, &endPtr, 10);
  if (endPtr != brightness && *endPtr == '\0') {
    if (nuumber > 0 && nuumber < 255) {
      dma_display->setBrightness8(nuumber);  //0-255
    }
  }


  webServer.send(200, "text/plain", "Data saved");
}

void handleRoot() {
  String html = "<html><body>";
  html += "<form action='/saveWifi' method='POST'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";
  html += "</body></html>";
  webServer.send(200, "text/html", html);
}

void handleSaveWifi() {
  String newSSID = webServer.arg("ssid");
  String newPassword = webServer.arg("password");

  // Save new SSID and password to EEPROM
  //saveCredentials(newSSID, newPassword);

  // Connect to the new WiFi network
  //connectToWiFi();

  webServer.send(200, "text/plain", "Data saved and connected to WiFi.");
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
  mode = MODE_RELOAD_PICTURE;
}


void notifyVolumio() {
  WiFiClient client;
  HTTPClient http;
  String ip = WiFi.localIP().toString();
  http.begin(client, pushNotificationUrl);
  http.addHeader("Content-Type", "application/json");  // Data to send with HTTP POST
  String httpRequestData = "{\"url\": \"http://" + ip + "/push\"}";
  int httpResponseCode = http.POST(httpRequestData);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  http.end();
}

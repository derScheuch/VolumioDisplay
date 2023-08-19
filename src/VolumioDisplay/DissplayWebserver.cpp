
#include "DisplayModel.h"
#include "DisplayWebserver.h"

DisplayWebServer *server;

void _webServerPush() {
  server->push();
}  
void _handleGETRequestSetup() {
  server-> handleGETRequestSetup();
}
void _handlePOSTRequestSetup() {
  server->handlePOSTRequestSetup();
}


void _handleSetupWifi() {
  server->handleSetupWifi();
}

void _handleSaveWifi() {
  server->handleSaveWifi();

}
void _handleNotFound () {
  server->handleNotFound();
}

DisplayWebServer::DisplayWebServer(DisplayModel *displayModel) {
  server = this;
  this->model = model;
  this->webServer = new WebServer(80);
   Serial.println("setupWebServer");
  webServer->on("/push", _webServerPush);
  webServer->on("/setup", HTTP_GET, _handleGETRequestSetup);
  webServer->on("/setup", HTTP_POST, _handlePOSTRequestSetup);
  webServer->on("/saveWifi", HTTP_POST, _handleSaveWifi);
  webServer->on("/", HTTP_GET, _handleSetupWifi);
  webServer->onNotFound(_handleNotFound);
  webServer->begin();
  Serial.println("HTTP server started");
 
}

void DisplayWebServer::handleCalls() {
   this->webServer->handleClient();
}
void DisplayWebServer::push() {
  webServer->send(200, "text/plain", "getting state and reloading picture");
  this->model->loopingMode = MODE_RELOAD_PICTURE;
}

void DisplayWebServer::handleGETRequestSetup() {
  String configPage = CONFIG_PAGE;
  configPage.replace("%PROXY_BASE_URL%", model->configuration->proxyBaseUrl);
  configPage.replace("%VOLUMIO_BASE_URL%", model->configuration->volumioBaseUrl);
  configPage.replace("%BUTTON_COMMAND%_1", model->configuration->buttonPressCommand_1);
  configPage.replace("%BUTTON_COMMAND%_2", model->configuration->buttonPressCommand_2);
  configPage.replace("%START_COMMAND%", model->configuration->startCommand);

  configPage.replace("%SCROLL_WAIT%", String(model->configuration->textScrollWait));
  configPage.replace("%TEXT_DISPLAY_WAIT%", String(model->configuration->textDisplayWait));
  configPage.replace("%BRIGHTNESS%", String(model->currentBrightness));
  webServer->send(200, "text/html", configPage);
}

void DisplayWebServer::handlePOSTRequestSetup() {
  String proxyBaseUrl = webServer->arg("proxyBaseUrl");
  String volumioBaseUrl = webServer->arg("volumioBaseUrl");
  String buttonCommand_1 = webServer->arg("buttonCommand_1");
  String buttonCommand_2 = webServer->arg("buttonCommand_2");
  String startCommand = webServer->arg("startCommand");
  int scrollWait = constrain(webServer->arg("scrollWait").toInt(), 0, 201);
  int textDisplayWait = constrain(webServer->arg("textDisplayWait").toInt(), 0, 201);
  int brightness = constrain(webServer->arg("brightness").toInt(), 0, 255);

  //if (strlen(proxyBaseUrl) > 0)
  model->configuration->proxyBaseUrl = proxyBaseUrl;
  //if (strlen(volumioBaseUrl) > 0)
  model->configuration->volumioBaseUrl = volumioBaseUrl;

  model->configuration->buttonPressCommand_1 = buttonCommand_1;
  model->configuration->buttonPressCommand_2 = buttonCommand_2;
  model->configuration->startCommand = startCommand;

  if (scrollWait != 201)
    model->configuration->textScrollWait = scrollWait;
  if (textDisplayWait != 601)
  model->configuration->textDisplayWait = textDisplayWait;
  model->configuration->save(); //saveConfiguration(model.configuration);
  //setBrightness(brightness);
  this->handleGETRequestSetup();
}

void DisplayWebServer::handleSetupWifi() {
  webServer->send(200, "text/html", NETWORK_PAGE);
}

void DisplayWebServer::handleSaveWifi() {
  String newSSID = webServer->arg("ssid");
  String newPassword = webServer->arg("password");
  model->configuration->password = newPassword;
  model->configuration->ssid = newSSID;
  model->configuration->save(); //saveConfiguration(model.configuration);
  webServer->send(200, "text/plain", "Data saved and connected to WiFi.");
  // TODO connectToWiFi();
}


void DisplayWebServer::handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer->uri();
  message += "\nMethod: ";
  message += (webServer->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer->args();
  message += "\n";
  for (uint8_t i = 0; i < webServer->args(); i++) {
    message += " " + webServer->argName(i) + ": " + webServer->arg(i) + "\n";
  }
  webServer->send(404, "text/plain", message);
}








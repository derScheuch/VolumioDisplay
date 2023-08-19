#include "DisplayModel.h"
#include <WebServer.h>

#ifndef DISPLAYWEbSERVER
#define DISPLAYWEbSERVER

class DisplayWebServer {
private:
  DisplayModel *model;
  WebServer *webServer;
  
public:
  DisplayWebServer(DisplayModel *displayModel);
  void handleCalls();
  void handleGETRequestSetup();
   void handlePOSTRequestSetup();
   void handleSetupWifi();
   void handleSaveWifi();
   void handleNotFound();
  void push();
};



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
#endif
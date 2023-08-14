# Volumio LED Display

Display album covers or similar visuals of the music playing on a Volumio device on an LED display. This project utilizes an ESP32 controller to drive a HUB75 LED display panel. It also requires an additional server on the network to scale JPEG files.

## Files

### VolumioDisplay.ino

This file contains the implementation of the graphics logic and the control of the Volumio player. It handles the process of displaying album covers or visuals on the LED display in sync with the music being played.

### proxy.php

`proxy.php` serves as a simple proxy script. It receives URLs pointing to JPEG images, scales them down to 64x64 pixels, and converts them into a format that is easily processed by the HUB75 LED display connected to the ESP32 controller. This script helps in efficient handling and display of album cover images on the LED display.

## Implementation

To set up this project, follow these steps:

1. Upload the `VolumioDisplay.ino` sketch to your ESP32 controller.
2. Set up the `proxy.php` script on a server within your network. This script will be responsible for receiving image URLs and scaling them appropriately for the LED display.
3. Configure the ESP32 to communicate with the `proxy.php` script so that it can retrieve scaled album cover images.
4. Connect the ESP32 to the HUB75 LED display panel according to the appropriate wiring scheme.

With this setup, your LED display will showcase album covers or visuals corresponding to the music playing on your Volumio device. The ESP32 will seamlessly interact with the proxy server to fetch and display the scaled images on the LED display.

Feel free to customize and enhance the implementation as per your requirements.


# ESP32-CYD-Shelly-Monitor
Power monitor with ESP32 Cheap Yellow Display 2432S028R and Shelly EM

Shelly EM is an IoT device that connects to the home WiFi network and allows for real-time monitoring of power through two current clamps. The collected data is sent to the Shelly cloud, where it can be accessed via a dedicated app. Additionally, this device also provides a local API interface that can be accessed directly. Therefore, I decided to implement a project using an Arduino board with ESP32 Cheap Yellow Display (ESP32-2432S028R), a TFT Touchscreen display LCD with an ESP32 development board included. 

This board will periodically query Shelly EM to retrieve power measurements from both the photovoltaic system and the meter, displaying them on LCD screen.

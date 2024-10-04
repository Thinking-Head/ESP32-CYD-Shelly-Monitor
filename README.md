# ESP32-CYD-Shelly-Monitor
Power monitor with ESP32 Cheap Yellow Display 2432S028R and Shelly EM

Shelly EM is an IoT device that connects to the home WiFi network and allows for real-time monitoring of power through two current clamps. The collected data is sent to the Shelly cloud, where it can be accessed via a dedicated app. Additionally, this device also provides a local API interface that can be accessed directly. Therefore, I decided to implement a project using an Arduino board with ESP32 Cheap Yellow Display (ESP32-2432S028R), a TFT Touchscreen display LCD with an ESP32 development board included. 

This board will periodically query Shelly EM to retrieve power measurements from both the photovoltaic system and the meter, displaying them on LCD screen.


![alt text](https://github.com/Thinking-Head/ESP32-CYD-Shelly-Monitor/blob/main/pics/IMG_4521-min.jpg?raw=true)

<img src="https://github.com/user-attachments/assets/302a1624-cace-4747-adf0-6aba08cff229" width=50% height=50%>



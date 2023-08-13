| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# Wifi Sniffer for Esp32


## How to use the sniffer
use menu config to config the board to either be an AP or join one 


## Example of for wireshark connection 
the board defaults to 192.168.19.1 in AP mode join the ssid you've configured in menu config 
your wifi board should get a IP from the DHCP 

to connect wireshark to it use 
"C:\Program Files\Wireshark\Wireshark.exe" -i TCP@192.168.19.1 -k
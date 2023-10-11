# WOOL
Wake On Lan with ESP32

Monitor remote server command, e.g. via https connection to https://your_cute_server.com/switch.html 

If GET "On", send WOL over WIFI to wake up some PC

Then monitor with ping to ensure the PC is on.

Finally go to sleep until next WOL.

References: (examples  from arduino/ESP32 libraries)

- BasicHttpsClient
- Blinker
- HTTPSRequest
- SimpleWiFiServer
- test_ping
- WakeOnLan-ESP32

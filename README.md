# Work in Progress

This code is sitting on GitHub so I can work on it from a few different computers, but is in no way ready for consumption by anyone else.
Feel free to take a look at the approach I'm taking, and hack around on it if you want, but at this point I'm not offering any sort of support.


# Eventual goals

This project is intended to serve as a bridge between a Linak DPG controller (BLE) and both a web interface and/or a MQTT broker. The code is
intended to be run on an Espressif ESP32, and makes heavy use of the ESP32's BLE library. It could probably be ported to work on any other MCU
that has both BLE and WiFi radios, but I have no plans to do that port (since I don't have any other hardware with those radios)

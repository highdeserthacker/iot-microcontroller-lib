# README.md
Microcontroller C++ Library
Shared code for microcontroller projects.
https://github.com/highdeserthacker/iot-microcontroller-lib
Language: C++

# Key Features
QCore: base/common functionality for starting up all devices, including wifi connection,
connection to mqtt broker. Can specify the services needed.

QWifi: establishes and maintains a reliable connection. Detects lost connection and reconnects
automatically. A robust state machine to manage the most common wifi connection related problems.

QMqtt: establish connection to mqtt broker, methods for publishing, subscribing to topics.

QMqtt_Entity: classes to manage various sensors and devices that communicate via mqtt. Abstraction 
layer for managing and reporting the state of sensors, switches, binary sensors. 
Can manage single pin devices, devices connected to a
shift register. Reporting compatible with Home Assistant.

QTrace: provides debug log (aka trace) support. Can output to serial and/or mqtt. Support for
logging levels including Error, Warning, Informational, Verbose, etc. Support for trace switches
to vary trace level across different functionality within a program. ASSERT() support.


QTime: class to manage time related information from NTPClient.

A variety of support for timers, sensors, shift register, and other devices.


# Setup Instructions

## Install the following Arduino/ESP8266 libaries

PubSubClient
ArduinoJson
NTPClient
OneWire

## PubSubClient

Make sure the maximum mqtt payload is a minimum of 256 bytes. I run it at 320.
In PubSubClient.h, set the following:
~~~
// MQTT_MAX_PACKET_SIZE : Maximum packet size
#define MQTT_MAX_PACKET_SIZE 256
~~~


## Install this library
This library is typically installed in a sub-directory of your sketch libraries, e.g. sketches/libraries/MyLib.

Requires the file env.json connectivity info. Do one of:
a. Create this file on the ESP8266 ahead of time.
b. Or, in AllApps.h, modify the wifi and mqtt info to your environment and if the file does not yet exist on the device, it will be created.


///////////////////////////////////////////////////////////////////////////////
/*  AllApps.h - general settings that span all Arduino / Microcontroller related apps.
   Settings here allow the libraries to remain generic and not device
   specific.                  */
///////////////////////////////////////////////////////////////////////////////
#ifndef AllApps_h
#define AllApps_h

#include <inttypes.h>      // data types, e.g. uint16_t
#include <limits.h>

/**************************************************************************************/
// Environment Settings File
/**************************************************************************************/
/* Device Credentials - Set these for your environment */
#define  WIFI_SSID                     "enter-your-wifi-ssid"
#define  WIFI_PASSWORD                 "enter-your-wifi-password"
#define  MQTT_ADDRESS                  "enter-your-mqtt-address"     // e.g. 192.168.0.23
#define  UTC_OFFSET_HOURS              (-7)

#define  ENV_SETTINGS_FILENAME         "/env.json"  

#define  JSON_PROP_VERSION             "version"
#define  JSON_PROP_WIFI_SSID           "wifi_ssid"
#define  JSON_PROP_WIFI_PASSWORD       "wifi_password"
#define  JSON_PROP_MQTT_ADDRESS        "mqtt_address"
#define  JSON_PROP_TIMEZONE_OFFSET     "tz_offset_hours"

/* MQTT Device & Topic Names  */
#define  TOPIC_PREFIX_DEVICE           "device"          // Prefix for device related topics

/**************************************************************************************/
// App Settings File
/**************************************************************************************/
#define  APP_SETTINGS_FILENAME         "/app.json"

#define  JSON_PROP_DEVICE_NAME         "device_name"


#endif

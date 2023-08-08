/***************************************************************************************
// :mode=c:
   QCore.cpp - Common setup() and loop() functions.

***************************************************************************************/
#include <ArduinoJson.h>
#include "QCore.h"
#include "QFile.h"

/**************************************************************************************/
// Static Member Initialization - all static data *must* be initialized.
/**************************************************************************************/
bool                    QCore::_Initialized=       false;
bool                    QCore::_EnvFileFound;
const char              QCore::_BuildDate[]=       __DATE__;     // Of the form: "Aug  3 2023". alt- with time: __DATE__ " " __TIME__;

bool                    QCore::_RebootNotify=      false;
QCore::ServiceSettingT  QCore::_ServiceSetting=    (QCore::ServiceSettingT) 0;
ESP8266WebServer *      QCore::_pWebServer=        NULL;
ESP8266HTTPUpdateServer *  QCore::_pUpdateServer=  NULL;
QTimer *                QCore::_pOTAStsTimer=      NULL;
QCore::OTACallback      QCore::_pOTACallback=      NULL;
QTimer *                QCore::_pTraceTimer=       NULL;

// Environment Settings file
float                   QCore::_SettingsVersion=   0.0;
int                     QCore::_UtcOffsetHours=    0;
char                    QCore::_Wifi_SSID[31+1];
char                    QCore::_Wifi_Password[31+1];
char                    QCore::_MQTT_Address[31+1];

// Trace Settings
bool                    QCore::_Trace_Connection_Status= true;

/**************************************************************************************/
/* QCore                                                                              */
/**************************************************************************************/
QCore::QCore(const char * pDeviceIdentifier, ServiceSettingT Services)
/* 
*/
{
   Serial.begin(115200);

   /* Power up safety delay */
   delay(/*sec*/2*/*msec*/1000);          

   Serial.printf("\nQCore(): entry, booting.");   

   if (!QCore::_Initialized)
   {
      /* Pin Setup */
      pinMode(_PinOnBoardLED, OUTPUT);
      digitalWrite(_PinOnBoardLED, HIGH);    // turn the on board LED off

      /* Set static data. */
      //_ServiceSetting= ServiceSettingT::SST_Default;
      _ServiceSetting= Services;
      _pOTAStsTimer= new QTimer(/*msec*/2000,/*Repeat*/true,/*Start*/true); 
      _pTraceTimer= new QTimer(/*min*/60/*sec*/*60/*msec*/*1000,/*Repeat*/true,/*Start*/false,/*Done*/true);

      /* Create Environment settings file. Create it only if it does not yet exist. */
      WriteEnvSettings();

      /* Read settings file if present.   */
      ReadSettings();

      /* Wifi setup */
      new QWifi(_Wifi_SSID, _Wifi_Password);             // Creates the Master object

      /* OTA Setup */
      _pWebServer= new ESP8266WebServer(/*Port*/80);
      _pUpdateServer= new ESP8266HTTPUpdateServer();
      _pUpdateServer->setup(_pWebServer);                // Tell it which webserver to work with. Set up the "update" page
      _pWebServer->begin();                              // Start the web server

      /* Trace setup */
      bool TraceOn= _ServiceSetting & ServiceSettingT::SST_Trace;
      QTraceSwitch TraceSwitch= (TraceOn)?(QTrace::_TraceSwitchesLevel_Verbose):(QTrace::_TraceSwitchesLevel_Warning);
      _Trace.SetTraceSwitch(TraceSwitch);

      QCore::_Initialized= true;
   }

   Init();

   // TBD - READ APP SETTINGS

   /* MQTT setup - server, publish and subscribe topics. */
   new QMQTT(_MQTT_Address, pDeviceIdentifier);       // Creates the Master object

   /* Trace setup for directing output of trace to mqtt. */
   sprintf(_pTraceTopic, "%s/%s/%s", TOPIC_PREFIX_DEVICE, pDeviceIdentifier, QMQTT::_pTraceSubTopic);
   QMQTT::Master()->SetTraceTopic(_pTraceTopic);            
   _Trace.SetCallback(QMQTT::TraceCallback);

   Serial.printf("\nQCore::QCore(): exit\n");   

} // QCore
/**************************************************************************************/
void QCore::Init()
{
   //_TraceSetting= TraceSettingT::TST_Default;

} // Init   
/**************************************************************************************/
void QCore::SetOTACallback(void (* pCallback)(bool Before))
{
   _pOTACallback= pCallback;
   
} // SetOTACallback
/**************************************************************************************/
void QCore::DoService()
{
   /* Housekeeping I */
   /* Wifi library needs to be called to maintain connection.
      Needs 2x calls before it establishes initial connection.
      Note that it self-throttles.                             */
   QWifi::Master()->DoService();                                                                 

   /* Housekeeping II: Stuff we do when wifi is connected. */
   if (QWifi::Master()->IsConnected())
   {  // Wifi connection is ok

      // Webserver handler - process client web connection, incl OTA update (/update)
      if (_pOTAStsTimer->IsDone())
      {
         if (_pOTACallback != NULL)
            _pOTACallback(/*Before*/true);            // Allow parent to disable interrupts.

         _pWebServer->handleClient();                 // Process any client web connection
         if (_pOTACallback != NULL)
            _pOTACallback(/*Before*/false);
      }

      // MQTT handler
      QMQTT::Master()->DoService();

      /* Activities performed on reboot upon connection to network. */
      if (!_RebootNotify)
      {  /* Initial wifi connection after reboot. Note- resets flag once mqtt is connected. */

         // Set up NTP now that we're connected to the network
         if ((_ServiceSetting & ServiceSettingT::SST_NTP) && (QTime::Master() == NULL))
            new QTime(/*sec*/60*/*min*/60*/*tz*/_UtcOffsetHours); // Note- does not handle DST, -7 summer, -8 winter

         // Issue notice of reboot once mqtt is connected
         if (QMQTT::Master()->IsConnected())
         {  /* Boot/Reboot occurred. */
            char BuildDateFormatted[15+1];
            QTime::FormatDateStr(_BuildDate, BuildDateFormatted);
            //_Trace.printf(TS_SERVICES, TLT_Event, "Rebooted. Build %s", BuildDateFormatted); // Issue event notice
            _Trace.printf(TS_SERVICES, TLT_Event, "Rebooted. Build %s, EnvFile: %s", BuildDateFormatted, (_EnvFileFound)?("Yes"):("No")); // Issue event notice
            _RebootNotify= true;
         }
      } 

      /* NTP Handler */
      if (_ServiceSetting & ServiceSettingT::SST_NTP)
         QTime::Master()->DoService();

      if (_Trace_Connection_Status)
      {
         /* Periodically dump status.  */
         if (_pTraceTimer->IsDone())
         {
            //float UptimeDays= millis() / 1000.0 / (60*60*24);
            float UptimeDays= QTimestamp::GetUptimeDays();
            _Trace.printf(TS_SERVICES, TLT_Info, "Uptime (days): %.1f, %s", 
               UptimeDays,            
               QWifi::Master()->Dump()); // Mainly to see if there are disconnects occurring.
            _Trace.printf(TS_SERVICES, TLT_Verbose, "%s", QMQTT::Master()->Dump());
            if (_ServiceSetting & ServiceSettingT::SST_NTP)
               _Trace.printf(TS_SERVICES, TLT_Max, "%s", QTime::Master()->Dump());
         }
      }
   }
   #ifdef DEBUG_REBOOT
   else
   {
      if (QWifi::Master()->ConnectionFailure())  
      {  /* Connection has been lost beyond the max allowed time. Reboot the device.
            Trigger hardware watchdog by disabling software one.  */
         ESP.wdtDisable();
         while (1) {}
      }
   }
   #endif

} // DoService
/**************************************************************************************/
/* Reads environment settings file into the variables.   */
void QCore::ReadSettings()
{
   /* Load configuration data from the configuration file. */
   /* Set defaults in the event that no cfg file yet. */
   QFile * pEnvSettingsFile= new QFile(ENV_SETTINGS_FILENAME);

   if (pEnvSettingsFile->Exists())
   {
      int ConfigBfrSize= 256;
      char ConfigBfr[ConfigBfrSize];
      int Cnt= pEnvSettingsFile->ReadStr(ConfigBfr, ConfigBfrSize);      // Read all the settings
   
      /* Parse the json configuration data */
      StaticJsonBuffer<JSON_OBJECT_SIZE(32)> jsonBuffer; // allocates buffer on stack
      JsonObject& JsonRoot= jsonBuffer.parseObject(ConfigBfr);

      if (JsonRoot.success())
      {  // Successful parse
         //JsonRoot.printTo(_SettingsBfr);              // https://arduinojson.org/v5/api/jsonobject/printto/
         _SettingsVersion= JsonRoot[JSON_PROP_VERSION];
         strlcpy(_Wifi_SSID, JsonRoot[JSON_PROP_WIFI_SSID]         | "MyWifiSSID", sizeof(_Wifi_SSID));
         strlcpy(_Wifi_Password, JsonRoot[JSON_PROP_WIFI_PASSWORD] | "MyWifiPassword", sizeof(_Wifi_Password));
         strlcpy(_MQTT_Address, JsonRoot[JSON_PROP_MQTT_ADDRESS]   | "MyMqttAddress", sizeof(_MQTT_Address));
         _UtcOffsetHours= JsonRoot[JSON_PROP_TIMEZONE_OFFSET];
      }
   }
   else
   {  /* Settings file not found, critical failure. */
      Serial.printf("\nQCore::ReadSettings(): env settings file not found.");  

      #ifdef OLDSTUFF
      strlcpy(_Wifi_SSID, WIFI_SSID, sizeof(_Wifi_SSID));
      strlcpy(_Wifi_Password, WIFI_PASSWORD, sizeof(_Wifi_Password));
      strlcpy(_MQTT_Address, MQTT_URL, sizeof(_MQTT_Address));
      #endif

   }
   
} // ReadSettings
/**************************************************************************************/
/* Writes the environment settings file if not already present.
   Creates file /env.json using LittleFS file system.
   Of the form:
   {
      "version":"1.0",
      "wifi_ssid":"blah",
      "wifi_password":"secret",
      "mqtt_address":"192.168.0.10"
   } 

   If using a separate program to initialize the settings file, then this can be removed.  
*/
void QCore::WriteEnvSettings()
{
   QFile * pSettingsFile= new QFile(ENV_SETTINGS_FILENAME);
   _EnvFileFound= pSettingsFile->Exists();
   if (!_EnvFileFound)
   {
      const int JsonBuffserSize= JSON_OBJECT_SIZE(16);
      StaticJsonBuffer<JsonBuffserSize> jsonBuffer;
      JsonObject& JsonRoot= jsonBuffer.createObject();

      JsonRoot[JSON_PROP_VERSION]=           (float) 1.0;      // Use a float form of version for ease of comparison between versions
      JsonRoot[JSON_PROP_WIFI_SSID]=         WIFI_SSID;
      JsonRoot[JSON_PROP_WIFI_PASSWORD]=     WIFI_PASSWORD;
      JsonRoot[JSON_PROP_MQTT_ADDRESS]=      MQTT_ADDRESS;
      JsonRoot[JSON_PROP_TIMEZONE_OFFSET]=   (int) -7;

      int Cnt= JsonRoot.measureLength() + 1;
      char JsonText[Cnt];
      Cnt= JsonRoot.printTo(JsonText, sizeof(JsonText));
      pSettingsFile->WriteStr(JsonText);
      Serial.printf("\nWriteEnvSettings(): created file.\n"); 
   }  
} // WriteEnvSettings






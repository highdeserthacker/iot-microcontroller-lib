///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*                                               */
///////////////////////////////////////////////////////////////////////////////
#ifndef QCore_h
#define QCore_h

#include "AllApps.h"
#include "QWifi.h"
#include "QMQTT.h"
#include "QTrace.h"
#include "QTime.h"                                    // NTP
#include <ESP8266HTTPUpdateServer.h>

/**************************************************************************************/
/* QCore - Common setup() and loop() functionality. Use this as a library to avoid
   replicating setup code in every program.
   - wifi connection setup & mgt
   - mqtt connection setup & mgt
   - OTA programming setup & mgt
   - trace setup. Outputs to serial and mqtt topic based on device/DeviceIdentifier/trace
   - reboot notification
   - callback before and after OTA update check.
   
   Multiple instances can be created if in the off-chance a single micro needs to manage
   multiple mqtt devices and associated separate topics.

   Settings common to all instances managed in static data.
*/   
/**************************************************************************************/
class QCore
{
   public:
   /* Bitmask for enabling functional areas for trace debug. */
   typedef enum TraceSettingT
   {
      TST_Default=         0
   };

   /* Bitmask for enabling services, e.g. NTP. */
   typedef enum ServiceSettingT
   {
      SST_Default=         0x03,                      // Trace+MQTT
      SST_Trace=           0x01,                      // Trace ouput
      SST_MQTT=            0x02,                      // MQTT, incl trace output
      SST_NTP=             0x04,                      // Time

      SST_All=             0xFFFF
   };

   /* Signature for optional callback to read sensor. 
      pointer to a function that has input int and returns an int.       */
   typedef void (* OTACallback)(bool BeforeUpdate);


   ///////////////////////////////////////////////////////////
   // Static Data
   ///////////////////////////////////////////////////////////
   public:
   static bool             _EnvFileFound;
   static const char       _BuildDate[];

   //char                    _SettingsBfr[255+1];       // the entire parsed json object (root)
   static bool             _Trace_Connection_Status;  // Periodic trace dump of uptime, wifi, and mqtt status

   protected:
   static bool             _Initialized;

   /* Version of settings file.  */
   static float            _SettingsVersion;

   static const int        _PinOnBoardLED=    LED_BUILTIN;       // on board led GPIO 2 (Silkscreen D4). Must be high at boot.

   /* Sets which of the core services are to be enabled. */
   static ServiceSettingT  _ServiceSetting;

   /* Controls issue of one-time trace notification upon reboot. */  
   static bool             _RebootNotify;             

   //////// OTA ////////
   static ESP8266WebServer *  _pWebServer;
   static ESP8266HTTPUpdateServer *  _pUpdateServer;

   /* Controls freq of ota update check.   */
   static QTimer *         _pOTAStsTimer;

   /* Optional pointer to callback function to call before and after OTA check.
      Used by parent to disable interrupt handling prior to OTA, else OTA doesn't work (times out).   
      Callback is required if your program uses interrupts. */
   static OTACallback      _pOTACallback;

   //////// QWifi ////////
   //static constexpr char * _pWifi_SSID=      WIFI_SSID;
   //static constexpr char * _pWifi_Password=  WIFI_PASSWORD;
   //static char *           _pWifi_SSID= WIFI_SSID;
   //static char *           _pWifi_Password= WIFI_PASSWORD;
   static char             _Wifi_SSID[];
   static char             _Wifi_Password[];

   static QTimer *         _pTraceTimer;

   //////// QMQTT ////////
   //static constexpr char * _pMQTT_IPAddress=  MQTT_URL;
   static char             _MQTT_Address[];


   //////// NTP ////////
   static int              _UtcOffsetHours;


   ///////////////////////////////////////////////////////////
   // Instance Data
   ///////////////////////////////////////////////////////////
   char                    _pTraceTopic[MQTT_TOPIC_LEN+1];              // the trace topic, e.g. device/this-device


   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QCore(const char * pDeviceIdentifier, ServiceSettingT Services);
   void                    SetOTACallback(void (* pCallback)(bool Before));

   void                    DoService();
   ESP8266WebServer *      GetWebServer(){return _pWebServer;}

   protected:
   void                    Init();
   void                    ReadSettings();

   void                    WriteEnvSettings();   

}; // QCore


#endif

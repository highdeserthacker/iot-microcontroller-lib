///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/* Wifi.h - 
      Based on input from https://github.com/esp8266/Arduino/issues/4352#issuecomment-458881563
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef QWifi_h
#define QWifi_h

#include <ESP8266WiFi.h>
#include "AllApps.h"
#include "QTimer.h"

#define  WIFI_STS_BUFFER_LEN        95
#define  ENH_WIFI_CONNECTION_DROP_DETECT

enum
{
   WFS_SETUP=                        0,               // 0 - Wifi not set up                         
   WFS_CONNECT,                                       // 1
   WFS_CONNECT_WAIT,                                  // 2
   WFS_CONNECTED,                                     // 3 - Connected to AP
   WFS_CONNECTION_TIMEOUT,                            // 4
   WFS_CONNECTION_TIMEOUT_WAIT,                       // 5 - Radio off, waiting

   WFS_STATE_COUNT
};

/**************************************************************************************/
/* QWifi - Manages a wifi connection. Handles login, reconnect on lost connection.
   Must call DoService() to retain connection.
*/   
/**************************************************************************************/
class QWifi
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   static bool             _Trace_Wifi_Status;        // Periodic trace dump of wifi state

   protected:
   static QWifi *          _pMasterObject;   

   public:
   #ifdef _DEBUG_WIFI_DISABLED
   // WiFi STATUS STRINGS
   static constexpr char * WiFiStatusStrArr[]= {"WL_IDLE_STATUS", "WL_NO_SSID_AVAIL", "WL_SCAN_COMPLETED",
   "WL_CONNECTED", "WL_CONNECT_FAILED", "WL_CONNECTION_LOST", "WL_DISCONNECTED"};
   #endif   
   
   protected:
   const char *            _pSSID;
   const char *            _pPassword;

   /* Connection State Machine */
   QTimer *                _pConnMgrStateTimer;       // state machine period
   int                     _ConnectionState;
   QTimer *                _pConnectionTimer;         // time it took to connect

   unsigned long           _ConnectStartTimeMsec;

   /* Connection Statistics */
   /* _ConnectionCount - # of times we've established wifi connection successfully.
      Gets incremented everytime there is a reconnect, exclusive of any timeouts, i.e. any connection drops 
      and reconnects. */
   unsigned long           _ConnectionCount; 

   /* _ConnectionAttemptTimeouts - Provides an indication of difficulties in establishing a connection. 
      Cumulative. */         
   unsigned long           _ConnectionAttemptTimeouts;       

   /* _ConnectionLostTimeMsec - the time connection was lost. 0 if ok. 
      tbd- parent to utilize this to determine whether to reboot.       */
   unsigned long           _ConnectionLostTimeMsec;


   /* Reconnection settings
      Throttle attempts to reconnect as it bogs down the main program. */
   int                     _RetryDelaySec;            // Progressive delay, up to _RetryDelayMaxSec. Reset on connection success.
   QTimer *                _pRetryTimer;
  
   char                    _StsBfr[WIFI_STS_BUFFER_LEN+1];

   WiFiClient              _espClient;

   /* Controls frequency of reporting connection failure status via the led. */
   QTimer *                _pLedStsTimer;         


   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   static QWifi *          Master(){return _pMasterObject;}

   public:
                           QWifi(const char * pSSID, const char * pPassword);
   WiFiClient *            GetWifiClient();
   void                    Setup();
   const char *            Dump();

   bool                    IsConnected();

   /* ConnectionFailure() - true if not connected and exceeded maximum elapsed time without successfully 
      connecting. */
   bool                    ConnectionFailure();
   
   /* Get state machine status. */
   int                     GetConnectionState(){return _ConnectionState;}


   void                    DoService();

   protected:
   void                    Init();

   /* Returns connection status. This is the direct status from the global Wifi object. */
   int                     GetConnectionStatus();
   void                    ConnectionMgr();
   int                     Connect();
   
}; // QWifi

#endif
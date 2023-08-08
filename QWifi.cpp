///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QWifi.cpp
   Resources
      https://www.arduino.cc/en/Reference/WiFi

   To do
      - get mac address. helpful for new devices. https://techtutorialsx.com/2017/06/29/esp32-arduino-getting-started-with-wifi/

*/
///////////////////////////////////////////////////////////////////////////////
#include "QWifi.h"
#include "QTrace.h"
#include "QIndicator.h"


#define WIFI_CONNECT_WAIT_MSEC                  (/*sec*/15/*msec*/*1000)    // How long we wait for connection to be established
#define  WIFI_RECONNECT_RETRY_DELAY_START_SEC   (4)
#define  WIFI_RECONNECT_RETRY_DELAY_MAX_SEC     (64)

#define  WIFI_CONNECTION_FAILED_TIME_SEC        (/*hr*/2 */*min*/60 */*sec*/60)  


/**************************************************************************************/
// QWifi - Static Member Initialization
/**************************************************************************************/
bool           QWifi::_Trace_Wifi_Status= false;
QWifi *        QWifi::_pMasterObject=     NULL;

// TBD - move to static
const char * gWiFiStatusStrArr[]= {"WL_IDLE_STATUS", "WL_NO_SSID_AVAIL", "WL_SCAN_COMPLETED",
   "WL_CONNECTED", "WL_CONNECT_FAILED", "WL_CONNECTION_LOST", "WL_DISCONNECTED"};

/**************************************************************************************/
QWifi::QWifi(const char * pSSID, const char * pPassword)
/*  */
{
   Init();
   _pSSID= pSSID;
   _pPassword= pPassword;
   
} // QWifi
/**************************************************************************************/
void QWifi::Init()
/*  */
{
   if (QWifi::_pMasterObject == NULL)
      QWifi::_pMasterObject= this;

   _pSSID= _pPassword= ""; 

   _pConnMgrStateTimer= new QTimer(/*Msec*/100,/*Repeat*/true,/*Start*/true);
   _ConnectionState= 0;
   _pConnectionTimer= new QTimer(/*Msec*/WIFI_CONNECT_WAIT_MSEC,/*Repeat*/true,/*Start*/false);
 
   /* Create the retry timer, but don't start it. */
   _RetryDelaySec= WIFI_RECONNECT_RETRY_DELAY_START_SEC;
   //_pRetryTimer= new QTimer(/*Msec*/WIFI_RECONNECT_WAIT_MSEC,/*Repeat*/true,/*Start*/false);
   _pRetryTimer= new QTimer(/*Sec*/_RetryDelaySec */*Msec*/1000,/*Repeat*/true,/*Start*/false);
   _ConnectionCount= 0;
   _ConnectionAttemptTimeouts= 0;
   _ConnectionLostTimeMsec= 0;
   
   int WifiSts= 0;
   //Serial.printf("%s\n", gWiFiStatusStrArr[WifiSts]);

   if (_Trace_Wifi_Status)
      _pLedStsTimer= new QTimer(/*sec*/30 */*Msec*/1000,/*Repeat*/true,/*Start*/true);
   
} // Init
/**************************************************************************************/
const char * QWifi::Dump()
{
   snprintf(_StsBfr, WIFI_STS_BUFFER_LEN, "QWifi: Sts:%s, Connections:%lu, Timeouts:%lu, RSSI:%ld", 
      gWiFiStatusStrArr[WiFi.status()], 
      _ConnectionCount, _ConnectionAttemptTimeouts,
      WiFi.RSSI()                                     // in dbm 0 (higher) .. -100 (lower)
      );
   return _StsBfr;
   
} // Dump
/**************************************************************************************/
WiFiClient * QWifi::GetWifiClient()
/*  */
{
   return &_espClient;
   
} // GetWifiClient
/**************************************************************************************/
int QWifi::GetConnectionStatus()
{
   return WiFi.status();
} // ConnectionStatus
/**************************************************************************************/
bool QWifi::IsConnected()
{
   int Result= false;

   /* Changed this to fix problems with parent sensing connected state before state machine. */
   if (     (_ConnectionState == WFS_CONNECTED)
         && (GetConnectionStatus() == WL_CONNECTED)   // still connected (account for state machine lag)
      )
      Result= true;

   return Result;

} // IsConnected
/**************************************************************************************/
bool QWifi::ConnectionFailure()
{
   int Result= false;

   /* Changed this to fix problems with parent sensing connected state before state machine. */
   if ((!IsConnected()) && (_ConnectionLostTimeMsec > 0))
   {  // Connection dropped, has it been a long time?
      unsigned long ElapsedTimeMsec= /*now*/millis() - _ConnectionLostTimeMsec;
      unsigned long MaxElapsedTimeMsec= WIFI_CONNECTION_FAILED_TIME_SEC * /*msec*/1000;
      if (ElapsedTimeMsec > MaxElapsedTimeMsec)
         Result= true;
   }   

   return Result;

} // ConnectionFailure
/**************************************************************************************/
void QWifi::DoService()
/* Called periodically to manage the wifi connection.
*/
{
   ConnectionMgr();

   #ifdef _DEBUG_WIFI_DISABLED
   /* Troubleshoot connection failures. This will indicate state machine state. */
   if (ConnectionFailure() && _pLedStsTimer->IsDone())
   {
      int BlinkCount= _ConnectionState;
      QIndicator::BlinkLed(/*Pin*/LED_BUILTIN, /*Count*/BlinkCount, /*Period (msec)*/BLINK_PERIOD_FAST_MSEC);
   }
   #endif

} // DoService
/**************************************************************************************/
void QWifi::ConnectionMgr()
/* Called periodically to manage the wifi connection. Takes care of all the steps from
   establishing initial connection, to re-connecting as required.

   Refer to https://github.com/esp8266/Arduino/issues/4352#issuecomment-458881563 for good example
     of the odd problems with wifi state and how best to set this up.
*/
{
   if (_pConnMgrStateTimer->IsDone())
   {
      switch (_ConnectionState)
      {
         case WFS_SETUP:
         {  /* Set up the connection */
            Setup();
            _ConnectionState= WFS_CONNECT;
            break;
         }
         case WFS_CONNECT:
         {  /* Try and connect */
            Connect();                                // login to wifi AP
            //_pConnectionTimer->Start(/*sec*/15/*msec*/*1000);
            _pConnectionTimer->Start();
            _ConnectionState= WFS_CONNECT_WAIT;
            break;
         }
         case WFS_CONNECT_WAIT:
         {  /* Connection attempt is underway. We give it ~15 seconds to connect. */
            int WifiSts= WiFi.status();
            if (WifiSts == WL_CONNECTED)
            {  // State change to Connected
               _ConnectionCount++; 
               _ConnectionState= WFS_CONNECTED;
               _RetryDelaySec= WIFI_RECONNECT_RETRY_DELAY_START_SEC;    // Reset the retry delay
               _ConnectionLostTimeMsec= 0;
               if (_Trace_Wifi_Status)
               {
                  unsigned long ConnectTimeMsec= millis() - _ConnectStartTimeMsec;
                  // IPAddress type print is pita. See https://forum.arduino.cc/index.php?topic=228884.msg3102705#msg3102705
                  _Trace.printf(TRACE_SWITCH_WIFI, TLT_Verbose, "QWifi::ConnectionMgr(): Connected in %lu msec, IP:%s", ConnectTimeMsec, WiFi.localIP().toString().c_str());
               }
            }
            else if (_pConnectionTimer->IsDone())
            {  // Connection timer timed out.
               _ConnectionState= WFS_CONNECTION_TIMEOUT;          // Connect() failed
               if (_Trace_Wifi_Status)
                  _Trace.printf(TRACE_SWITCH_WIFI, TLT_Verbose, "QWifi::ConnectionMgr(): timeout, Sts:%s", gWiFiStatusStrArr[WifiSts]);
            }
            // else - waiting, no timeout yet

            break;
         }
         case WFS_CONNECTED:
         {  /* We are connected.  */
            int WifiSts= WiFi.status();
            #ifdef ENH_WIFI_CONNECTION_DROP_DETECT
            /* Workaround for esp8266 bug in which it thinks it is connected, but it is not. */
            IPAddress MyIP= WiFi.localIP();
            bool NoIP= (MyIP[0] == 0);                // Check if the first byte is 0: NN.xx.xx.xx. It lost IP address.
            if ((WifiSts != WL_CONNECTED) || NoIP)
            {  /* Lost connection - just attempt to connect again. If it times out, it will get more aggressive
                  and fall into the WFS_CONNECTION_TIMEOUT state. */
               _ConnectionState= WFS_SETUP;
               _ConnectionLostTimeMsec= millis();
               if (_Trace_Wifi_Status)
                  _Trace.printf(TRACE_SWITCH_WIFI, TLT_Verbose, "QWifi::ConnectionMgr(): lost connection, Sts:%s", gWiFiStatusStrArr[WifiSts]);
            }
            #else
            if (WifiSts != WL_CONNECTED)
            {  /* Lost connection - just attempt to connect again. If it times out, it will get more aggressive
                  and fall into the WFS_CONNECTION_TIMEOUT state. */
               _ConnectionState= WFS_SETUP;
               _ConnectionLostTimeMsec= millis();
               if (_Trace_Wifi_Status)
                  _Trace.printf(TRACE_SWITCH_WIFI, TLT_Verbose, "QWifi::ConnectionMgr(): lost connection, Sts:%s", gWiFiStatusStrArr[WifiSts]);
            }
            #endif
            break;
         }
         case WFS_CONNECTION_TIMEOUT:
         {  /* Connect() failed - timeout waiting for connection.            */
            _ConnectionAttemptTimeouts++;
            WiFi.disconnect();                              // Disconnect any existing connection
            WiFi.mode(WIFI_OFF);                            // turn the wifi radio off
            delay(/*msec*/1);
            //_pRetryTimer->Start(/*sec*/15/*msec*/*1000);             // Leave radio off for several seconds
            _pRetryTimer->Start(/*sec*/_RetryDelaySec */*msec*/1000);   // Leave radio off for several seconds
            _RetryDelaySec= (_RetryDelaySec < WIFI_RECONNECT_RETRY_DELAY_MAX_SEC)?(_RetryDelaySec << 1):(_RetryDelaySec);   
            _ConnectionState= WFS_CONNECTION_TIMEOUT_WAIT;
            break;
         }
         case WFS_CONNECTION_TIMEOUT_WAIT:
         {  /* We wait here with the radio off for awhile. */
            if (_pRetryTimer->IsDone())
            {  // Radio has been off long enough, turn it back on and attempt re-connect
               WiFi.mode(WIFI_STA);
               delay(/*msec*/1);
               wifi_station_connect();                // sdk level call. sdk/include/user_interface.h
               delay(/*msec*/1);
               _ConnectionState= WFS_SETUP;
               if (_Trace_Wifi_Status)
                  _Trace.printf(TRACE_SWITCH_WIFI, TLT_Verbose, "QWifi::ConnectionMgr(): begin retry, Sts:%s", gWiFiStatusStrArr[WiFi.status()]);
            }
            // else - keep waiting
 
            break;
         }
         default:
            break;
      }
   }

} // ConnectionMgr
/**************************************************************************************/
void QWifi::Setup()
/* This sets up the wifi settings, but doesn't initiate connection.  */
{
   int WifiSts;
   WiFi.persistent(false);                            // Don't save WiFi configuration in flash - optional
   /* true: Removes any credentials stored in NVM. */
   WiFi.disconnect(true);                             // To avoid problems where we were previously connected (e.g. ota update)
   //delay(/*msec*/3000);                               // Allow time for radio off.

   /* Wifi settings */   
   WiFi.mode(WIFI_STA);
   WiFi.setAutoReconnect(false);

   // At this stage, status should be WL_IDLE_STATUS
   if (_Trace_Wifi_Status)
   {
      WifiSts= WiFi.status();
      //Serial.printf("QWifi::Setup(): Wifi connection status: %s\n", gWiFiStatusStrArr[WifiSts] );
      _Trace.printf(TRACE_SWITCH_WIFI, TLT_Verbose, "QWifi::Setup(): Setup done, connection status: %s", gWiFiStatusStrArr[WifiSts]);
   }
     
} // Setup
/**************************************************************************************/
int QWifi::Connect()
/*
   Note- this doesn't do anything but begin() now.  
*/
{
   /* Initiate the wifi connection. */
   int WifiSts= WiFi.begin(_pSSID, _pPassword);  // returns wifi status. Normally should come back with WL_DISCONNECTED
   /* Note that "Wifi" is a global extern.
      Wifi status:
      https://links2004.github.io/Arduino/dc/da7/class_wi_fi_client.html
      https://www.arduino.cc/en/Reference/WiFiStatus
      0: not connected
      // 0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
                or when not connected to a network, but powered on.
      // 1 : WL_NO_SSID_AVAIL in case configured SSID cannot be reached
         2: WL_SCAN_COMPLETED
      // 3 : WL_CONNECTED after successful connection is established
      // 4 : WL_CONNECT_FAILED if password is incorrect
         5 : WL_CONNECTION_LOST
      // 6 : WL_DISCONNECTED if module is not configured in station mode

      Users say if wifi connection dropped, status can be either idle or disconnected.

      [br] OTA issue - when browser upload of new code occurs, we get stuck waiting for WL_CONNECTED. 
         When it's stuck, status= 5 prior to begin(), and 6 on timeout.
            Also 0, then 6
            Also hanging in begin()??
         Reset success: 1, then ok
         On power up, status= 0

         12/28/19: seems to stay connected when I have _DEBUG_WIFI active. 
            Immedly after OTA it gets screwed up. Hard reset and then it's ok. something about ota and wifi state.


   */
   _ConnectStartTimeMsec= millis();
  
   return WifiSts;
   
} // Connect






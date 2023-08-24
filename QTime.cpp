///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTime.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "QTime.h"

/**************************************************************************************/
// QTime
/**************************************************************************************/
/* Unfortunately, gnu C++ compiler requires out of class initialization of static variables.
   Unless they are const, they need to be instantiated here.
   https://stackoverflow.com/questions/185844/how-to-initialize-private-static-members-in-c
   https://stackoverflow.com/questions/9656941/why-cant-i-initialize-non-const-static-member-or-static-array-in-class
*/
QTime *        QTime::_pMasterObject= NULL;
bool           QTime::_NTPInitialized= false;
bool           QTime::_NTP_UpdateOk= false;
QTimer *       QTime::_pOfflineTimer;

WiFiUDP        QTime::_ntpUDP;
NTPClient *    QTime::_pNTPClient;
//QTimer *       QTime::_pWaitTimer;  
const char *   QTime::_pDayNames[]= {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; 

uint8_t        QTime::_DayOfWeekOffline= 0;
int32_t        QTime:: _TimeOfDayOffsetOfflineSec= 0;

/**************************************************************************************/
/*static*/ void QTime::FormatDateStr(const char * pInputStr, char * pOutputStr)
{ 
    int month, day, year;
    static const char month_names[]= "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(pInputStr, "%s %d %d", pOutputStr, &day, &year);
    month= (strstr(month_names, pOutputStr)-month_names)/3+1;
    sprintf(pOutputStr, "%d-%02d-%02d", year, month, day);
} // FormatDateStr

/**************************************************************************************/
// Instance Methods
/**************************************************************************************/
QTime::QTime(long utcOffsetSeconds)
{
   _utcOffsetSeconds= utcOffsetSeconds;
   Init();

} // QTime
/**************************************************************************************/
void QTime::Init()
{
   if (QTime::_pMasterObject == NULL)
   {  // First instance initialization of static items
      _pMasterObject= this;
      //Initialize();
      _pNTPClient= new NTPClient(_ntpUDP, "pool.ntp.org", /*TZ Offset (sec)*/_utcOffsetSeconds, /*update interval (msec)*/_NTPUpdateIntervalSec*1000);
      //_pWaitTimer= new QTimer(/*Sec*/60*/*Msec*/1000,/*Repeat*/false,/*Start*/true);
      _pOfflineTimer= new QTimer(/*Sec*/_NTPUpdateIntervalSec/*Msec*/*1000,/*Repeat*/false,/*Start*/false);
   
      #ifdef QTIME_OFFLINE
      _DayOfWeekOffline= random(/*min*/0, /*below max*/8); 
      #endif  
   }

} // Init
/**************************************************************************************/
const char * QTime::Dump()
{
   snprintf(_StatusBfr, _StatusBfrSz, "Time: %s %02d:%02d:%02d %s",  
      _pDayNames[_pNTPClient->getDay()], 
      _pNTPClient->getHours(), _pNTPClient->getMinutes(), _pNTPClient->getSeconds(),
      (_NTP_UpdateOk)?(""):("NTP Update Failed")
      );
   return _StatusBfr;
   
} // Dump
/**************************************************************************************/
/* Initializes the NTPClient if not yet done. Tries to wait for wifi connection, else
   starts it up regardless so that default time values can be used.     */
void QTime::CheckInitialized()
{
   // TBD - GET A NEW _pNTPClient every few days to ensure THAT PULLING A NEW ONE FROM NTP POOL.
   if (!_NTPInitialized)
   {
      if (QWifi::Master()->IsConnected())
      {  // Wifi connection is ok
         /* Starts the udp listener socket. */
         _pNTPClient->begin();
         _NTPInitialized= true;
      }
   }

} // CheckInitialized
/**************************************************************************************/
bool QTime::IsReady()
{
   CheckInitialized();
   return _NTPInitialized;

} // IsReady
#ifdef QTIME_OFFLINE
/**************************************************************************************/
bool QTime::IsReady()
{
   /* Handle NTP offline
      1) If it's initialized and valid readings
      2) If it's been way longer than the 5 minute update interval, consider it offline
   */
   bool Result= false;
   CheckInitialized();                                // this sets _NTPInitialized
   if (_NTPInitialized)
   {
      if (!_NTP_UpdateOk)
      {  /* update() calls to NTP are failing. i.e. past due on the update interval.
            Start the offline timer. If it times out, we consider time values unusable.  */
         if (!_pOfflineTimer->IsEnabled())
         {  // Initial offline detections
            _pOfflineTimer->Start();
            Result= true;
         }
         else if (!_pOfflineTimer->IsDone())
         {  // Still in the grace period
            Result= true;
         }
         else
         {  // Offline
            Result= false;
         }
      }
      else
         Result= true;                                // ok
   }

   return Result;

} // IsReady
#endif
/**************************************************************************************/
/* CheckStatus(): Initialize if required. 
      If connection was lost and is now restored, update the offline settings. 
*/
void QTime::CheckStatus()
{
   CheckInitialized();

   if (_NTPInitialized)
   {
      bool Status;
      bool BackOnline= false;

      /* Call periodically to keep NTP time up to date. Returns false if time is stale (default is
         60 sec and fetch of ntp update failed.   */
      Status= _pNTPClient->update();                  
      if (Status && !_NTP_UpdateOk)
         BackOnline= true;                            // Change in state from offline to online

      _NTP_UpdateOk= Status;

      #ifdef QTIME_OFFLINE
      if (BackOnline)
      {
         /* Sync the backup clock when we're online to get reasonable values to use for time and day
            should we go offline. Determines _DayOfWeekOffline & _TimeOfDayOffsetMsecOffline
            based relative to how long we've been running since reboot.          */
         _DayOfWeekOffline= _pNTPClient->getDay();
      
         const int32_t SecPerDay= 86400L;
         uint32_t      NowSec= millis() / 1000;
         int32_t NowTimeOfDaySec= NowSec % SecPerDay; // Modulo 1 day. 0..86400-1
         int32_t NTPTimeOfDaySec=
              (_pNTPClient->getHours() * 60 * 60)
            + (_pNTPClient->getMinutes() * 60)
            + (_pNTPClient->getSeconds());
      
         if (NTPTimeOfDaySec >= NowTimeOfDaySec) 
            _TimeOfDayOffsetOfflineSec= NTPTimeOfDaySec - NowTimeOfDaySec;
         else  // Negative offset
            _TimeOfDayOffsetOfflineSec= -(NTPTimeOfDaySec + (SecPerDay - NowTimeOfDaySec));
      }
      #endif
   }

} // CheckStatus
/**************************************************************************************/
void QTime::DoService()
{
   CheckStatus();
} // DoService
/**************************************************************************************/
int32_t QTime::GetTimeOfDaySec()
/* Note that NTPClient will provide values relative to millis() if connection is lost.
   If a wifi connection or ntp connection was never established, then readings will 
   still be relative to millis() and still somewhat usable (just wrong). */
{
   uint32_t Result= 0;
   CheckStatus();
   if (_NTPInitialized)
   {
      Result=  _pNTPClient->getHours() * 60 * 60;
      Result+= (_pNTPClient->getMinutes() * 60);
      Result+= _pNTPClient->getSeconds();
   }

   return Result;

} // GetTimeOfDaySec
/**************************************************************************************/
int QTime::GetDayOfWeek()
{
   int Result= 0;
   CheckStatus();
   if (_NTPInitialized)
      Result= _pNTPClient->getDay();

   return Result; 

} // GetDayOfWeek





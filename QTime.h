///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTime.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QTime_h
#define QTime_h


#include <NTPClient.h>
#include <WiFiUdp.h>             // For WiFiUDP
#include "QWifi.h"
#include "QTimer.h"

// Turns out not to be necessary
//#define QTIME_OFFLINE          // (not implemented) handle time when offline

/**************************************************************************************/
/* QTime - uses NTP to obtain and maintain time.
   Utilizes NTPClient library. This library handles:
      - obtains current time in this timezone (sans DST).
      - only reads ntp server every updateInterval msec.
      It should be noted that time is initialized to zero on startup. Until a successful
      ntp read occurs, all day & time information returned will be relative to Jan. 1, 1970.
      If it never initializes (e.g. due to lack of internet), then results will rollover
      based on millis() limits.
      Unfortunately, library doesn't provide means to know if it got initialized successfully.

   This class allows sharing of the info and provides necessary wifi connectivity
   check.
*/   
/**************************************************************************************/
class QTime
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   //////// Static Data        
   static QTime *          _pMasterObject; 
   static const int        _StatusBfrSz= 128;
   static const int        _NTPUpdateIntervalSec= 5*60;  
   static bool             _NTPInitialized;

   /* If NTP update() returns false as a result of underlying call to forceUpdate(), this
      indicates that time is not getting updated (or never was). We use this to determine if we're
      offline or not. */
   static bool             _NTP_UpdateOk;

   /* Used to decide if we've been offline from NTP server long enough to consider the time values too old to use. */
   static QTimer *         _pOfflineTimer;

   static WiFiUDP          _ntpUDP;
   static NTPClient *      _pNTPClient;

   /* Controls max time wait to initialize the NTPClient. We wait on startup for internet
      connectivity so the NTPClient can be used. If it times out, proceed with initialization
      and rely on (incorrect) default values from NTPClient. */
   //static QTimer *         _pWaitTimer;
 
   static const char *     _pDayNames[]; 

   /* Alternate time, used when NTP not available.
      On power up, default now to be midnight, and randomly generate a day of the week. 
   */
   static uint8_t          _DayOfWeekOffline;

   /* Offset between millis() reading and NTP time of day, in msec.
      Can be positive or negative, +- 0..(86400-1).
         >0:   NTP > millis()
         <0:   NTP < millis
         ==0:  NTP == millis. ie. if reboot happened to occur at midnight.
      This value is added to millis() reading when offline. 
      On power up, this is defaulted to zero and adjusted anytime we get an actual NTP reading.      */
   static int32_t          _TimeOfDayOffsetOfflineSec;

   //////// Instance Data
   long                    _utcOffsetSeconds;

   /* Used for Dump()               */
   char                    _StatusBfr[_StatusBfrSz];


   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   static QTime *          Master(){return _pMasterObject;}
   static void             FormatDateStr(const char * pInputStr, char * pOutputStr);

   private:
   //void                    Initialize();

   public:
   /* QTime() - create after wifi connection established. */
                           QTime(long utcOffsetSeconds);

   const char *            Dump();

   /* IsReady() - indicates if NTP initialized successfully. */
   bool                    IsReady();

   /* Get the time of day in seconds since Midnight. 0 if NTP client not functioning. */
   int32_t                 GetTimeOfDaySec();

   /* Get the day of week. 0= Sunday..6= Saturday. 0 if NTP client not functioning. */
   int                     GetDayOfWeek();


   /* Call this periodically to ensure things are kept up-to-date. */
   void                    DoService();
   
   protected:
   void                    Init();

   /* Initializes the NTP Client as necessary. */
   void                    CheckInitialized();
   void                    CheckStatus();
   
}; // QTime

#endif

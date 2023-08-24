///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTimer.h                                                                 */
///////////////////////////////////////////////////////////////////////////////
#ifndef QTimer_h
#define QTimer_h
#include "Arduino.h"             // e.g. DigitalRead(), sprintf()

//#define  _DEBUG_QTIMER
//#define  _DEBUG_QTIMESTAMP

/**************************************************************************************/
/* QTimestamp - Helpers for time measurements from based on microcontroller timestamps,
   e.g. millis().
*/   
/**************************************************************************************/
class QTimestamp
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:
   typedef uint32_t        TimestampType;

   protected:
   static const TimestampType _MaxTimestampMsec= 0xFFFFFFFFUL;
   static const TimestampType _MsecPerDay= /*hrs*/24 * /*min*/60 */*sec*/60 * /*msec*/1000;
   static TimestampType       _PrevTimestamp;
   static int              _RolloverCnt;

   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////

   public:
   /* Gets the current timestamp. For ESP8266, this is based on millis(). */
   static TimestampType    GetNowTimeMsec();

   /* Compares the Timestamp to the Reference time. Values are integer times in any units of interest
      e.g. millis() readings. Handles rollover of timestamps.
      Relies on assumption that max difference is 0x7FFFFFFF.
      Returns: <0       Timestamp earlier/older than Reference
               >0       Timestamp later than Reference
   */
   static int              Compare(uint32_t Timestamp, uint32_t Reference);

   static TimestampType    Difference(uint32_t Timestamp, uint32_t Reference);

   /* Age of timestamp in seconds, relative to Now. */
   static uint32_t         GetAgeSec(TimestampType Timestamp);

   /* Time in days since reboot. */
   static float            GetUptimeDays();

   #ifdef _DEBUG_QTIMESTAMP
   static int              _RolledOver;
   static void             Test();
   #endif

}; // QTimestamp


/**************************************************************************************/
/* QTimer - a general purpose countdown timer. Various controls available for:
   - automatically repeating the countdown once Done.
   - Initial state of Done or !Done.
   - Pause & Resume.

   Future enhancements:
   - Initial start delay in msec.

*/   
/**************************************************************************************/
class QTimer
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   /* Timer states */
   typedef enum TimerStateT
   {
      TST_Disabled=        0,
      TST_Enabled,
      TST_Paused,
      TST_Done,
      TST_Count
   };

   TimerStateT             _State;
   bool                    _Repeat;
   unsigned long           _StartTimeMsec;
   unsigned long           _MasterDurationMsec;
   unsigned long           _CurDurationMsec;
   unsigned long           _RemainingTimeMsec;
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   /* General Usage
         - To create a timer that starts immediately, create with StartTimer= true;
         - Repeat: if true, timer starts again afer IsDone(). If false, stays in IsDone() state until
                   started (becomes Enabled and not done) again, or Stopped (becomes Disabled and not done).
    
   */
   /* Countdown Timer. Defaults to 1 sec countdown, does not start.
      Call Start() with or without timer value to begin the timer. */
                           QTimer();

   /* Full control constructor
         Done - if true, and not repeat, initial state is Done.
         Repeat
           If repeat=true, then timer will start once IsDone() is checked.
           If repeat=false, then timer stays in the Done state until Start.

         - To create a timer that starts immediately, create with StartTimer= true; 
         - To create a  timer that starts off in the Done state, create with StartTimer= false, and Done=true.
   */
                           QTimer(unsigned long DurationMsec, bool Repeat, bool StartTimer, bool Done);
   /* Countdown Timer.
         This timer is always initialized to !Done state.
   */
                           QTimer(unsigned long DurationMsec, bool Repeat, bool StartTimer);

   /* Countdown timer, non-repetitive. */
                           QTimer(unsigned long DurationMsec, bool StartTimer);

   void                    Set(unsigned long DurationMsec);
   void                    Start();
   void                    Start(unsigned long DurationMsec);
   void                    Restart(){Start();}
   void                    Pause();

   /* Resume the timer. Note that can resume a timer that was never started (starts from beginning). */
   void                    Resume();

   /* Cancel() - Finishes the timer immediately, placing it in the Done state. */
   void                    Cancel();

   /* Stop() - Stops the Timer, placing it in Disabled state, resetting it, and state is not Done. */ 
   void                    Stop();

   void                    SetRepeat(bool Flag);
   unsigned long           ElapsedTimeMsec();
   unsigned long           RemainingTimeMsec();
   unsigned long           RemainingTimeSec(){return (RemainingTimeMsec()/1000);}

   bool                    IsDone();                  // returns true if not yet started
   bool                    IsEnabled(){return (_State == TimerStateT::TST_Enabled);}
   
   protected:
   void                    Set(unsigned long DurationMsec, bool Repeat, bool StartTimer);
   void                    Init();
   void                    SetDuration(unsigned long DurationMsec);
   void                    Disable();
};



#endif
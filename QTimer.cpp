///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTimer.cpp
*/
///////////////////////////////////////////////////////////////////////////////
#include "Arduino.h"             // e.g. DigitalRead()
#include "QTimer.h"
#include "QTrace.h" 

/**************************************************************************************/
/* QTimestamp                                                                         */
/**************************************************************************************/
uint32_t                   QTimestamp::_PrevTimestamp= 0;
int                        QTimestamp::_RolloverCnt= 0;
#ifdef _DEBUG_QTIMESTAMP
int                        QTimestamp::_RolledOver= -2;

/**************************************************************************************/
/*static*/ void QTimestamp::Test()
{
   ASSERT_MSG(QTimestamp::Compare(0xFFFFFFF0UL,/*Reference*/0x00000008UL) < 0, "QTimestamp failure 1"); 
   ASSERT_MSG(QTimestamp::Compare(0x00000008UL,/*Reference*/0xFFFFFFF0UL) > 0, "QTimestamp failure 2"); 
}
#endif
/**************************************************************************************/
/*static */ uint32_t QTimestamp::GetNowTimeMsec()
{
   unsigned long Result= millis();

   #ifdef _DEBUG_QTIMESTAMP
   /* Rollover testing - start millis off with a high offset, N minutes from rollover. */
   unsigned long OffsetMsec= (/*min*/10 */*sec*/60 * /*msec*/1000);
   unsigned long MaxTimeMsec= 0xFFFFFFFFUL;
   unsigned long StartTimeMsec= (MaxTimeMsec - OffsetMsec);
   Result= StartTimeMsec + millis();

   if ((_RolledOver < 0) && (Result < StartTimeMsec))
   {  // Rollover
      _Trace.printf(TLT_Verbose, "QTimestamp::GetNowTimeMsec(): Rollover %08lx", Result);
      _RolledOver++;
   }
   else if ((_RolledOver < -1) && (millis() > (/*min*/1 */*sec*/60 * /*msec*/1000)))
   {  // Rollover
      Test();
      _Trace.printf(TLT_Verbose, "QTimestamp::GetNowTimeMsec(): Result %08lx", Result);
      _RolledOver++;
   }
   #endif

   if (Result < _PrevTimestamp)
   {  // Rollover
      _RolloverCnt++;
   }
   _PrevTimestamp= Result;

   return Result;

} // GetNowTimeMsec
/**************************************************************************************/
/*static*/ int QTimestamp::Compare(uint32_t Timestamp, uint32_t Reference)
{
   int Result= 0;

   if (Timestamp != Reference)
   {
      /*  32 bits, rolls over after ~49.7 days. See http://playground.arduino.cc/Code/TimingRollover  */
      if ((long) (Timestamp - Reference) >= 0)
         Result= 1;
      else
         Result= -1;
   }

   return Result;
} // Compare
/**************************************************************************************/
/*static*/ uint32_t QTimestamp::Difference(uint32_t Timestamp, uint32_t Reference)
{
   uint32_t Result= Timestamp - Reference;
   return Result;

} // Difference
/**************************************************************************************/
/*static*/ float QTimestamp::GetUptimeDays()
{
   float UptimeDays= _RolloverCnt *  (((float) _MaxTimestampMsec) / (float) _MsecPerDay);
   UptimeDays+= ((float) GetNowTimeMsec()) / (float) _MsecPerDay;
   return UptimeDays;

} // GetUptimeDays


/**************************************************************************************/
/* QTimer                                                                             */
/**************************************************************************************/
QTimer::QTimer()
{
   Init();
   
} // QTimer
/**************************************************************************************/
QTimer::QTimer(unsigned long DurationMsec, bool Repeat, bool StartTimer)
{
   Init();
   Set(DurationMsec, Repeat, StartTimer);   
} // QTimer
/**************************************************************************************/
QTimer::QTimer(unsigned long DurationMsec, bool Repeat, bool StartTimer, bool Done)
{
   Init();
   Set(DurationMsec, Repeat, StartTimer);
   if ((!StartTimer) && (Done))
      _State= TimerStateT::TST_Done;
   
} // QTimer
/**************************************************************************************/
QTimer::QTimer(unsigned long DurationMsec, bool StartTimer)
{
   Init();
   Set(DurationMsec, /*Repeat*/false, StartTimer);   
   
} // QTimer
/**************************************************************************************/
void QTimer::Init()
{
   _State= TimerStateT::TST_Disabled;
   _StartTimeMsec= 0;
   _Repeat= false;
   _RemainingTimeMsec= 0;
   SetDuration(/*msec*/1000);

} // Init 
/**************************************************************************************/
void QTimer::SetDuration(unsigned long DurationMsec)
{
   _MasterDurationMsec= _CurDurationMsec= DurationMsec;

} // SetDuration 
/**************************************************************************************/
void QTimer::Set(unsigned long DurationMsec)
{
   SetDuration(DurationMsec);
} // Set 
/**************************************************************************************/
void QTimer::Set(unsigned long DurationMsec, bool Repeat, bool StartTimer)
{
   _Repeat= Repeat;
   Set(DurationMsec);
   if (StartTimer)
      Start();

} // Set
/**************************************************************************************/
void QTimer::Disable()
{
   _State= TimerStateT::TST_Disabled;
} // Disable 
/**************************************************************************************/
void QTimer::Start()
/* Starts/restarts the timer with the value existing in _CurDurationMsec. */
{
   _StartTimeMsec= QTimestamp::GetNowTimeMsec();
   _State= TimerStateT::TST_Enabled;
} // Start 
/**************************************************************************************/
void QTimer::Start(unsigned long DurationMsec)
{
   SetDuration(DurationMsec);
   Start();

} // Start 
/**************************************************************************************/
void QTimer::Pause()
{
   /* Note the remaining time left for later resume. */
   _RemainingTimeMsec= RemainingTimeMsec();
   _State= TimerStateT::TST_Paused;
} // Pause 
/**************************************************************************************/
void QTimer::Resume()
{
   if (_State == TimerStateT::TST_Paused)
   {
      _CurDurationMsec= _RemainingTimeMsec;
      Start();
   }
   else if (_State == TimerStateT::TST_Disabled)
   {  // Timer was never paused, starting from Stopped state.
      _CurDurationMsec= _MasterDurationMsec;
      Start();
   }
   // else - enabled, not Paused, ignore Resume

} // Resume 
/**************************************************************************************/
void QTimer::Cancel()
{
   if (  (_State == TimerStateT::TST_Enabled)
      || (_State == TimerStateT::TST_Paused))
   {  // Force it to Done state
      _State= TimerStateT::TST_Done;
   }

} // Cancel 
/**************************************************************************************/
void QTimer::Stop()
{
   Disable();
   SetDuration(_MasterDurationMsec);           
} // Stop 
/**************************************************************************************/
void QTimer::SetRepeat(bool Flag)
{
   _Repeat= Flag;
} // SetRepeat 
/**************************************************************************************/
unsigned long QTimer::ElapsedTimeMsec()
{
   unsigned long Result= 0;
   if (_State == TimerStateT::TST_Enabled)
      Result= QTimestamp::GetNowTimeMsec() - _StartTimeMsec;
   else if (_State == TimerStateT::TST_Paused)
   {
      Result= _MasterDurationMsec - _RemainingTimeMsec;
   }

   return Result;
   
} // ElapsedTimeMsec 
/**************************************************************************************/
unsigned long QTimer::RemainingTimeMsec()
{
   unsigned long Result= 0;
   if (_State == TimerStateT::TST_Enabled)
   {
      unsigned long EndTimeMsec= _StartTimeMsec + _CurDurationMsec;  // may rollover
      Result= EndTimeMsec - QTimestamp::GetNowTimeMsec();
   }
   else if (_State == TimerStateT::TST_Paused)
      Result= _RemainingTimeMsec;

   return Result;
   
} // RemainingTimeMsec 
/**************************************************************************************/
bool QTimer::IsDone()
{
   bool Done= false; 
   if (_State == TimerStateT::TST_Enabled)
   {
      if (_StartTimeMsec == 0)
         Done= true;                               // Initial timer activation  LOSE THIS
      else
      {
         unsigned long EndTimeMsec= _StartTimeMsec + _CurDurationMsec;  // may rollover
         if (QTimestamp::Compare(QTimestamp::GetNowTimeMsec(), /*Reference*/EndTimeMsec) >= 0)
            Done= true;
      }

      if (Done)
      {  // Timer is done, restart it if in repeat mode, else disable.
         if (_Repeat)
            Start(_MasterDurationMsec);
         else
           _State= TimerStateT::TST_Done;             // Non-repetitive timers move to a done state 
      }
   }
   else if (_State == TimerStateT::TST_Done)
   {  /* Timer is one of: initialized in Done state, non-repetitive and Done, or Cancelled. */
      Done= true;
      if (_Repeat)
         Start(_MasterDurationMsec);
   }
   return Done;
   
} // IsDone









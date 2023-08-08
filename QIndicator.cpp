///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QIndicator.cpp - indicator library, including led indicator classes.
*/
///////////////////////////////////////////////////////////////////////////////
#include "QIndicator.h"     

/**************************************************************************************/
/*static*/ void QIndicator::BlinkLed(int pin, int BlinkCount, int BlinkPeriodMsec)
{
    for (int i= 0; i < BlinkCount; i++) 
    {
      digitalWrite(pin, LOW);
      delay(BlinkPeriodMsec >> 1);
      digitalWrite(pin, HIGH);
      //if (i + 1 < BlinkCount)
      if (BlinkCount > 1)           // Don't hold up processing for the 1 blink case
         delay(BlinkPeriodMsec >> 1);
    }
} // BlinkLed


/**************************************************************************************/
QIndicator::QIndicator(int Pin, unsigned long PeriodMsec, unsigned long OnPeriodMsec, int BlinkCount)
/* Allows full control of the setup, including duty cycle. 
*/
{
   Init();
   _IndicatorPin= Pin;
   Set(PeriodMsec, OnPeriodMsec, BlinkCount);
   
} // QIndicator
/**************************************************************************************/
void QIndicator::Init()
{
   //_Mode= 0;
   _Enabled= false;
   _ActiveLow= true;
   _nBlinks= 0;
   _State= false;
   _pStateTimer= new QTimer(/*Msec*/_BlinkPeriodFastMsec,/*Repeat*/false,/*Start*/false);

   _nTicks= 0;

} // Init   
/**************************************************************************************/
void QIndicator::Set(unsigned long PeriodMsec, unsigned long OnPeriodMsec, int BlinkCount)
/* Set the indicator settings. Does not activate the indicator.
   Inputs:  BlinkCnt    <0 indicates blink forever
*/
{
   _Enabled= false;
   if (BlinkCount <= 0)
      BlinkCount= _BlinkForeverCnt;
      
   _MasterBlinkCnt= _BlinkCnt= BlinkCount;

   _PeriodMsec= PeriodMsec;
   if (OnPeriodMsec > PeriodMsec)
      OnPeriodMsec= PeriodMsec;

   _OnPeriodMsec= OnPeriodMsec;
   
   _PeriodTicks= _PeriodMsec / _TickPeriodMsec;
   _OnPeriodTicks= _OnPeriodMsec / _TickPeriodMsec;
   
} // Set
/**************************************************************************************/
void QIndicator::SetState(bool State)
{
   /* Restart the timer for the appropriate on or off period. */
   unsigned long PeriodMsec= (_State == true)?(_PeriodMsec - _OnPeriodMsec):(_OnPeriodMsec);         
   _pStateTimer->Start(PeriodMsec); 

   _State= State;
   int OutputLevel;
   if (_ActiveLow)
      OutputLevel= (State)?(LOW):(HIGH);
   else
      OutputLevel= (State)?(HIGH):(LOW);
   
   digitalWrite(_IndicatorPin, OutputLevel);

} // SetState
/**************************************************************************************/
void QIndicator::Activate()
/* Activates the Indicator, based on the settings. */
{
   _BlinkCnt= _MasterBlinkCnt;
   _nBlinks= 0;
   _nTicks= 0;

   SetState(true);
   _Enabled= true;
} // Activate   
/**************************************************************************************/
void QIndicator::Deactivate()
/* Deactivates the Indicator. If it is currently on, it is turned off. */
{
   _Enabled= false;
   SetState(false);
} // Deactivate   
/**************************************************************************************/
bool QIndicator::IsActive()
/* Indicates whether the Indicator is enabled and currently active (e.g. still completing
   a blink sequence).
*/   
{
   return (_Enabled && (_nBlinks < _BlinkCnt));
} // IsActive
/**************************************************************************************/
void QIndicator::DoService()
/* Called by parent in polling mode. */   
{
   if (!_Enabled)
   {  /* Make sure Indicator is off if it's been disabled. */
      //SetState(false);
   }
   else if (IsActive())
   {  /* Blinking forever or N count blink sequence still in progress. */ 
      /* If in the On state, wait until timer is done. */
      if (_pStateTimer->IsDone())
      {  // Timer is done, toggle state.
         if (_State == false)
         {  // end of a blink period
            if (_BlinkCnt < _BlinkForeverCnt)
               _nBlinks++;
         }

         SetState(!_State);
      }
   }

} // DoService



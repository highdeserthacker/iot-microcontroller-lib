///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QIndicator.h - indicator library, including led indicator classes.
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef QIndicator_h
#define QIndicator_h

#include "Arduino.h"
#include "QTimer.h"

//#define BLINK_PERIOD_FAST_MSEC   (250)                // Intended to be the shortest period for visually counting pulses

/**************************************************************************************/
/* Manages an led indicator, including state, blinking frequency.
   Can be run in a blocking mode or with Interrupt to control blinking in a non-blocking
   manner.

   To run in polling mode: TBD
   To run in interrupt mode: TBD

   TBD:
   - parameter for active low or high output
   - Handle color leds

*/   
/**************************************************************************************/
class QIndicator
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:
   static const int        _BlinkPeriodFastMsec= 50;   //    
   static const int        _BlinkPeriodMinMsec= 50;   //    
   protected:
   static const int        _TickPeriodMsec= 20;       // Interrupt Timer 1 Period
   static const int        _BlinkForeverCnt= 0x7FFF; 
   
   //int                     _Mode;                     // 0=polling, 1=interrupt
   int                     _IndicatorPin;
   bool                    _ActiveLow;
   bool                    _Enabled;

   /* Indicator Waveform
        ____              ____
   ____|    |____________|    |____________
   
   */
   unsigned long           _PeriodMsec;               // Total period
   unsigned long           _OnPeriodMsec;             // Amount of time within period that Indicator is on. This is duty cycle setting.
   
   /* Blink Settings                  */
   int                     _MasterBlinkCnt;
   int                     _BlinkCnt;                 // <0 = blink forever

   /* State Control                 */
   int                     _nBlinks;

   /* Polling Based Control - based on millis() readings. */
   bool                    _State;                    // true=on, false=off
   QTimer *                _pStateTimer;       
   

   /* Interrupt Based Control - based on fixed 20 msec tick period. */
   unsigned long           _PeriodTicks;
   unsigned long           _OnPeriodTicks;
   
   int                     _nTicks;

   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   /* Blinks the led, using Standalone non-interrupt method (delay), so note that this is
      inline blocking.
      Led must be configured as active low. e.g. on board led.
      Inputs:
               BlinkPeriodMsec     
   */
   static void             BlinkLed(int pin, int BlinkCount, int BlinkPeriodMsec);

   public:

   /* Create an indicator with specified blink duty cycle.
      BlinkCount: <=0: forever, >0 indicates a fixed number of blinks before it stops.  
   */
                           QIndicator(int Pin, unsigned long PeriodMsec, unsigned long OnPeriodMsec, int BlinkCount);

   /* To change the settings on an existing object. */
   void                    Set(unsigned long PeriodMsec, unsigned long OnPeriodMsec, int BlinkCount);                           

   /* Activates the Indicator, based on the settings. */
   void                    Activate();

   /* Deactivates the Indicator. If it is currently on, it is turned off. */
   void                    Deactivate();

   //void                    Blink();                   // Deprecated
   //void                    Blink(int BlinkCnt);

   void                    DoService();

   /* Call this from your interrupt service routine on every interrupt.
      This handles the Indicator state updates. 
      This assumes a timer interrupt period of 20 msec.
   */ 
   //void                    Tick();
   
   protected:
   void                    Init();
   void                    SetState(bool State);
   bool                    IsActive();
};

#endif
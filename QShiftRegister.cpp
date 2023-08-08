///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QShiftRegister.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "QShiftRegister.h"
#include "Arduino.h"

/**************************************************************************************/
// QShiftRegister
/**************************************************************************************/
/**************************************************************************************/
/* Sets the size of the shift register.
   Note that pins are not assigned yet, so register is not initialized. */
QShiftRegister::QShiftRegister(uint8_t BitCount)
{
   Init();
   _BitCount= BitCount;
   
   Write();                                           // Fill with all zero's

} // QShiftRegister
/**************************************************************************************/
void QShiftRegister::Init()
{
   _BitCount= 8;
   _DataPin= _ClkPin= _LatchPin= _OEPin= 0;
   _Data= 0;

} // Init
/**************************************************************************************/
void QShiftRegister::SetPins(uint8_t DataPin, uint8_t ClkPin, uint8_t LatchPin, uint8_t OEPin)
/* Assigns the shift register control pins and initializes the shift register. */
{
   _DataPin=   DataPin;
   _ClkPin=    ClkPin;
   _LatchPin=  LatchPin;
   _OEPin=     OEPin;
   
   digitalWrite(_OEPin, HIGH);                        // Disable shift register outputs (tri-state) 
   pinMode(_OEPin, OUTPUT);
   digitalWrite(_OEPin, HIGH);                        // Disable shift register outputs (tri-state) 

   pinMode(_DataPin, OUTPUT);
   pinMode(_ClkPin, OUTPUT);

   #ifdef ENH_SHIFT_REGISTER
   digitalWrite(_ClkPin, LOW);                        // Clock is normally in low state. Shift out will toggle it hi then low.
   #else
   digitalWrite(_ClkPin, HIGH);                       // Clock=hi
   #endif

   pinMode(_LatchPin, OUTPUT);
   digitalWrite(_LatchPin, HIGH);                     // Latch=hi

   _Data= 0;
   Write();                                           // Fill with all zero's

} // SetPins
/**************************************************************************************/
/* Enable/Disable the output (OE)*/
void QShiftRegister::Enable(bool Flag)
{
   int State= (Flag)?(LOW):(HIGH);
   digitalWrite(_OEPin, State);    

} // Enable
/**************************************************************************************/
/* Writes the current data in _Data. */
void QShiftRegister::Write()
{
   uint8_t nBytes= ((_BitCount - 1) >> 3) + 1;     // # bytes determines how many daisy chained register ICs there are

   digitalWrite(_LatchPin, LOW);

   /* Shift data in serially, in byte groups.
      Serial output of highest byte to lowest byte, msb first. */
   /* ZZZ - SOMEHOW BIT SETTINGS AREN'T TAKING, BUT OTHERS REMAIN OK. */
   for (int i= nBytes - 1 ; i >= 0 ; i--)
   {
      int ShiftCnt= i * 8;
      uint8_t byt= (_Data >> ShiftCnt) & 0xFF;
      /* ZZZ- SHOULD I WRITE MY OWN SHIFT?
         */
      // https://www.arduino.cc/reference/en/language/functions/advanced-io/shiftout/
      shiftOut(_DataPin, _ClkPin, MSBFIRST, byt);     // Shift a byte's worth
   }

   /* Rising edge of Latch pin to write to the output register. */
   digitalWrite(_LatchPin, HIGH);
   
} // Write
/**************************************************************************************/
void QShiftRegister::Set(uint32_t Data)
{
   _Data= Data;
   Write();

} // Set





///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  ShiftRegister.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QShiftRegister_h
#define QShiftRegister_h

#include "AllApps.h"

#define ENH_SHIFT_REGISTER          // 11/22: for testing out clock change

/**************************************************************************************/
/* Manages Shift Register based I/O, including the following devices:
   - 74xx595
   - MCP23008
   - MCP23017

   Manages the collection of bits in the set of shift registers.
   Supports up to 2 bytes / 16 bits.
   Assumes shift registers are cascaded from lowest byte first to highest byte. Bits are
   shifted out MSB first (most significant bit to least significant bit).
*/   
/**************************************************************************************/
class QShiftRegister
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   uint8_t                    _BitCount;              // # bits of shift register data
   uint8_t                    _DataPin;
   uint8_t                    _ClkPin;
   uint8_t                    _LatchPin;

   /* Controls Output Enable (OE). Active low. If undefined or zero, is not used. */
   uint8_t                    _OEPin;

   /* The shift register contents. */
   uint32_t                   _Data;
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   /* Note- Output Enable (OE) is inactive upon construction to allow setting initial register value.
      Enable() must be called separately, after construction and specifying pin assignments with SetPins().
   */
                           QShiftRegister(uint8_t BitCount);

   void                    SetPins(uint8_t DataPin, uint8_t ClkPin, uint8_t LatchPin, uint8_t OEPin);

   /* Flag= true to activate OE. */                        
   void                    Enable(bool Flag);

   /* Writes data to shift register and latches to output registers.
      Note that it does not modify OE setting, so can write when registers are tri-state. */
   void                    Set(uint32_t Data);

   // TBD - bit level write

   uint32_t                Get(){return _Data;};
   // TBD - bit level get, 0|1.
   int                     GetBitCount(){return _BitCount;};

   // SetBit()
   
   protected:
   void                    Init();
   void                    Write();
   
};

#endif

///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QMath.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QMath_h
#define QMath_h
#include "AllApps.h"


#ifndef min
//define min(a,b) ((a)<(b)?(a):(b))
#endif

/**************************************************************************************/
/* QMath - Math related functions.      */   
/**************************************************************************************/
class QMath
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:

   protected:

   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   static bool                  IsBitSet(uint16_t Target, uint16_t Mask)
    {
      if ((Target & Mask) != 0)
          return true;
      else
          return false;
    };

  /*  Sets all the bits specified in the mask.    */
   static uint16_t           SetBits(uint16_t Val, uint16_t Mask)
    {   // Hack to allow Or'g of enums without the compiler bitching
      Val|= Mask;
      return Val;
    };

  /*  Resets all the bits specified in the mask. */
   static uint16_t           ResetBits(uint16_t Val, uint16_t Mask)
    {   // Hack to allow And'g of enums without the compiler bitching
      Val&= ~Mask;
      return Val;
    };

   /* Sets or resets the bit(s) specified. */
   static uint16_t           SetBits(uint16_t Val, uint16_t Bits, bool Set)
    {
      return (Set)?(SetBits(Val,Bits)):(ResetBits(Val,Bits));
    }

   /* Sets the state for a single bit. */
   static uint16_t              SetBitState(uint16_t Word, int8_t Position, int8_t BitValue)
   {
      uint16_t NewWord= ResetBits(Word, 1 << Position);
      if (BitValue == 1)
         NewWord= SetBits(NewWord, 1 << Position);

      return NewWord;
   }


   protected:
   
}; // QMath


#endif

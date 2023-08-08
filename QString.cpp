///////////////////////////////////////////////////////////////////////////////
// :mode=c:
///////////////////////////////////////////////////////////////////////////////
#include "QString.h"
#include <Arduino.h>

/**************************************************************************************/
int FindCh(const char * pStr, char ch)
{
   int Result= -1;
   for (int i= 0 ; i < strlen(pStr) ; i++)
   {
      if (pStr[i] == ch)
      {
         Result= i;
         break;
      }
   }

   return Result;

} // FindCh
/**************************************************************************************/
void ToStr(const uint16_t * pArr, int nItems, char * pStr, int Size)
/* Writes the array contents to a string, delimiting each element with comma.
   Data is output in decimal format. 
*/
{
   int Cnt= 0;
   for (int i= 0 ; i < nItems ; i++)
   {
      uint16_t Item= *(pArr+i);
      int RemainingSize= Size - Cnt;
      /* Refer to http://www.cplusplus.com/reference/cstdio/snprintf/
         Returns: # written, up to max bfr size. */
      int nWritten= snprintf(pStr+Cnt, RemainingSize, "%d%c", Item, (i < nItems - 1)?(','):(' '));
      if (nWritten >= 0 && nWritten < RemainingSize)
         Cnt+= nWritten;
      else
         break;                                          // Buffer is full
   }

} // ToStr
/**************************************************************************************/
void ToHexStr(const unsigned char * Bytes, int nBytes, char * Str)
/* Convert the array of bytes to Hex string. Output is start of array to end,
   hi nibble, low nibble order.  */
{
   unsigned int Nibble;
   char NibbleCh;
   int j= 0;
   
   for (int i= 0 ; i < nBytes ; i++)
   {
      Nibble= Bytes[i] >> 4;
      NibbleCh= ((Nibble < 10)?('0'):('A' - 10)) + Nibble;
      Str[j++]= NibbleCh;

      Nibble= Bytes[i] & 0x0F;  
      NibbleCh= ((Nibble < 10)?('0'):('A' - 10)) + Nibble;
      Str[j++]= NibbleCh;

      if (i < nBytes - 1)
         Str[j++]= ',';
   } 
   Str[j]= 0;  

} // ToHexStr
/**************************************************************************************/
void FloatToStr(float f, char * pStr, int Precision)
/* Workaround for the horrid Arduino non-support an aberrant behavior with sprintf() on
   floats. sprintf() has bizarre problems with %f, supposed to show '?', which it does
   not, and it will even blow away the buffer such that it never prints anything.  */
{
   const char * pSign= (f < 0)?("-"):("");
   float fUnsigned= (f < 0) ? -f : f;

   int IntegerPortion= fUnsigned;                  // Get the unsigned integer (678).
   float tmpFrac= fUnsigned - IntegerPortion;      // Get unsigned fraction (0.0123).
   //int FractionInt= trunc(tmpFrac * 10000);        // Turn into integer (123).
   int FractionInt= tmpFrac * 100;

   // Print as parts, note that you need 0-padding for fractional bit.
   sprintf(pStr, "%s%d.%d", pSign, IntegerPortion, FractionInt);

} // FloatToStr


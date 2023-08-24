///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/* String functions and helpers                                              */
///////////////////////////////////////////////////////////////////////////////
#ifndef QString_h
#define QString_h

#include "AllApps.h"

const char * GetTrueFalseStr(bool Flag);

int FindCh(const char * pStr, char ch);

void ToStr(const uint16_t * pArr, int nItems, char * pStr, int Size);
void ToHexStr(const unsigned char * Bytes, int nBytes, char * Str);
void FloatToStr(float f, char * pStr, int Precision);



#endif

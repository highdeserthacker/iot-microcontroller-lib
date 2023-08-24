///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QBfr.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include <inttypes.h>      // data types, e.g. uint16_t
#include "QBfr.h"
#include "QTrace.h"

/**************************************************************************************/
// QBfr
/**************************************************************************************/
template<typename T> QBfr<T>::QBfr(int Size)
{
   Init();
   _Size= Size;

   _Bfr= new T[_Size];
   for (int i= 0 ; i < _Size ; i++)
   {
      _Bfr[i]= 0;
   }
   
} // QBfr
/**************************************************************************************/
template<typename T> void QBfr<T>::Init()
{
   _Index= 0;
   _Wraparound= false;
} // Init
/**************************************************************************************/
template<typename T> void QBfr<T>::Clear()
{
   _Index= 0;
   _Wraparound= false;
   for (int i= 0 ; i < _Size ; i++)
      _Bfr[i]= 0;

} // Clear
/**************************************************************************************/
template<typename T> int QBfr<T>::Count()
{
   int Result= (_Wraparound)?(_Size):(_Index);
   return Result;
} // Count
/**************************************************************************************/
template<typename T> void QBfr<T>::Set(T Element)
{
   _Bfr[_Index]= Element;

} // Set
/**************************************************************************************/
template<typename T> void QBfr<T>::Next()
{
   if (_Index == (_Size-1))
   {
      _Wraparound= true;
      _Index= 0;
   }
   else
      _Index++;

   Set(0);                                            // Initialize head

} // Next
/**************************************************************************************/
template<typename T> void QBfr<T>::Put(T Element)
{
   Set (Element);
   Next();

} // Put
/**************************************************************************************/
template<typename T> T QBfr<T>::Get(int LookbackOffset)
{
   ASSERT(LookbackOffset < _Size);
   LookbackOffset= _min(LookbackOffset, _Size-1);
   int i= _Index - LookbackOffset;
   if (i < 0)
      i+= _Size;

   T Result= _Bfr[i];
   return Result;

} // Get

/**************************************************************************************/
/* Explicit instantiation of all the supported types. Must be done here, after the 
   implementation code above, to avoid putting all implementation code in header files.
*/   
/**************************************************************************************/
template class QBfr<float>;
template class QBfr<uint8_t>;
template class QBfr<uint16_t>;
template class QBfr<uint32_t>;



/**************************************************************************************/
// QQueue
/**************************************************************************************/
template<typename T> QQueue<T>::QQueue(int Size)
{
   Init();
   _Size= Size;

   _Bfr= new T[_Size];
   for (int i= 0 ; i < _Size ; i++)
      _Bfr[i]= 0;
   
} // QQueue
/**************************************************************************************/
template<typename T> void QQueue<T>::Init()
{
   _HeadIndex= _TailIndex= 0;
   _Enabled= true;
} // Init
/**************************************************************************************/
template<typename T> void QQueue<T>::Clear()
{
   _HeadIndex= _TailIndex= 0;
   for (int i= 0 ; i < _Size ; i++)
      _Bfr[i]= 0;

} // Clear
/**************************************************************************************/
template<typename T> int QQueue<T>::Count()
{
   int Cnt;
   if (_TailIndex >= _HeadIndex)
      Cnt= _TailIndex - _HeadIndex;
   else
      Cnt= (_Size - _HeadIndex) + _TailIndex;

   return Cnt;

} // Count
/**************************************************************************************/
template<typename T> bool QQueue<T>::IsFull()
{
   return (Count() >= (_Size - 1));
} // IsFull
/**************************************************************************************/
template<typename T> bool QQueue<T>::IsEmpty()
{
   return (Count() == 0);
} // IsEmpty
/**************************************************************************************/
template<typename T> int QQueue<T>::Put(T Element)
{
   int Result= 0;
   if (_Enabled && !IsFull())
   {
      _Bfr[_TailIndex]= Element;
      _TailIndex= (_TailIndex + 1) % _Size;
   }
   else
      Result= -1;

   return Result;   

} // Put
/**************************************************************************************/
template<typename T> T QQueue<T>::Peek()
{
   T Result= 0;
   ASSERT(Count() > 0);
   if (Count() > 0)
   {
      Result= _Bfr[_HeadIndex];
   }

   return Result;

} // Peek
/**************************************************************************************/
template<typename T> T QQueue<T>::Get()
{
   T Result= 0;
   ASSERT(Count() > 0);
   if (Count() > 0)
   {
      Result= _Bfr[_HeadIndex];
      _HeadIndex= (_HeadIndex + 1) % _Size;
   }

   return Result;

} // Get


/**************************************************************************************/
/* Explicit instantiation of all the supported types. Must be done here, after the 
   implementation code above, to avoid putting all implementation code in header files.
*/   
/**************************************************************************************/
template class QQueue<float>;
template class QQueue<uint8_t>;
template class QQueue<uint16_t>;
template class QQueue<uint32_t>;





///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QMyClass.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QBfr_h
#define QBfr_h

/**************************************************************************************/
/* Classes for general buffer handling, wraparound buffers, queues, stacks.
*/   
/**************************************************************************************/

/**************************************************************************************/
/* QBfr - a wrap-around buffer. Used to keep the last Size data elements, e.g. for
   measurement recording. It only has the concept of a head (no tail, hence no overrun).
   Elements are added to the tail, and it can wrap around. 
   Examining the contents is a lookback relative to the head (most recent).
   Once it wraps around, count is always equal to Size.        */   
/**************************************************************************************/
template <class T> class QBfr
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   int                     _Size;

   /* Only used for Count(). */
   bool                    _Wraparound;

   /* Points to the next available location. */
   int                     _Index;
   T *                     _Bfr;
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QBfr(int Size);
   int                     GetIndex(){return _Index;}
   void                    Clear();
   int                     Count();                        

   /* Set the value at head of buffer. */
   void                    Set(T Element);

   void                    Next();

   /* Set the value at head of buffer and move head to the next location.
      Initializes the head location to zero. */
   void                    Put(T Element);

   /* Get the value at the offset relative to the head of the buffer. 
      0: head, 1: head-1, etc.                                    */
   T                       Get(int LookbackOffset);
   
   protected:
   void                    Init();
   
}; // QBfr

/**************************************************************************************/
/* QQueue - implements a FIFO.
*/   
/**************************************************************************************/
template <class T> class QQueue
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   /* Set to false to disable entry of elements into the queue. */
   bool                    _Enabled;

   int                     _Size;

   /* Head points to the next available data. */
   uint16_t                _HeadIndex;
   /* Tail points to the next empty spot. */
   uint16_t                _TailIndex;

   T *                     _Bfr;
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QQueue(int Size);
   void                    Clear();
   int                     Count(); 
   bool                    IsFull();                       
   bool                    IsEmpty();                       

   /* Set the value at head of buffer and move to next location. */
   int                     Put(T Element);

   T                       Peek();

   /* Get the value at the offset relative to the head of the buffer. 
      0: head, 1: head-1, etc.                                    */
   T                       Get();

   
   protected:
   void                    Init();
   
}; // QQueue

#endif

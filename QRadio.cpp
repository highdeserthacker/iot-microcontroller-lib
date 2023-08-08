///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QRadio.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "Arduino.h"             // e.g. DigitalRead()
#include "QRadio.h"
#include "QTrace.h"
#include "QString.h"

#define  TRACE_SWITCH_RADIO      TS_SERVICES          // Allows remapping of the trace switch

/**************************************************************************************/
// QRadioRcvr
/**************************************************************************************/
QRadioRcvr::QRadioRcvr(int PulseBfrSize)
{
   Init();

   _PulseBfrSize= PulseBfrSize;
   _PulseBfr= new uint16_t[_PulseBfrSize];
   
} // QMyClass
/**************************************************************************************/
void QRadioRcvr::Init()
{
   _Enabled= false;
   _HeadIndex= _TailIndex= 0;
   _PulseBfrSize= 0;
   _MaxSyncPulseCount= 10;
   _LastEntryTime_usec= 0;
   _MaxMsgSizeBytes= 0;
   _MinMsgSizeBytes= 1;  _MaxMsgSizeBytes= 16;
   _MsgCntBytes= 0;

   _State= State_Sync_Seek;

   #ifdef _RADIO_DEBUG
   _MsgAttemptCnt= _MsgErrorCnt= 0;
   #endif

   #ifdef _RADIO_INDICATOR
   _PinMessageReceived= (D6);
   _pIndicatorTimer= new QTimer(/*Msec*/1000,/*Repeat*/false,/*Start*/false);
   #endif

} // Init
#ifdef _RADIO_DEBUG
/**************************************************************************************/
const char * QRadioRcvr::Dump()
{
   //sprintf(_TraceBfr, "State:%d, Queue:%d, H:%03d, T:%03d, MsgRdy:%d", _State, Count(), _HeadIndex, _TailIndex, _MsgCntBytes);
   //sprintf(_TraceBfr, "State:%d, ErrRate:%.1f, Queue:%d, MsgRdy:%d", _State, ErrorRate, Count(), _MsgCntBytes);
   const char * pErrStats= GetStats();
   char TraceBfr2[64];
   sprintf(TraceBfr2, ", State:%d, Queue:%d, MsgRdy:%d", _State, Count(), _MsgCntBytes);
   strcat(_TraceBfr, TraceBfr2);
   return _TraceBfr;
   
} // Dump
#endif
/**************************************************************************************/
/*
   tbd - add avg data rate in, overruns
*/
const char * QRadioRcvr::GetStats()
{
   float ErrorRate= ((_MsgAttemptCnt)?((float) _MsgErrorCnt / (float) _MsgAttemptCnt):(0)) * 100.0;
   sprintf(_TraceBfr, "ErrRate:%.1f (%lu of %lu)", ErrorRate, _MsgErrorCnt, _MsgAttemptCnt);
   return _TraceBfr;
   
} // GetStats
/**************************************************************************************/
void QRadioRcvr::SetRadioType(uint8_t RadioType)
{
   _RadioType= RadioType;
   if ((_RadioType == Radio_Type_RXB6_433) || (_RadioType == Radio_Type_Default))
   {
      /* RXB6 Timing.                   */
      _SyncPulseWidth_usec= 600;       // measured actual of 537..670
      _PulseTolerance_usec= 99;        // 99 is max without overlap
   }

} // SetRadioType
/**************************************************************************************/
void QRadioRcvr::SetDeviceType(uint8_t DeviceType)
/* Other protocols:  PT2262        */
{
   _DeviceType= DeviceType;
   if ((_DeviceType == Device_Type_Accurite) || (_DeviceType == Radio_Type_Default))
   {
      /* Accurite Family Protocol.                   */
      SetRadioType(Radio_Type_RXB6_433);
 
      /* Note- pulse widths can't overlap within tolerance bands. */
      _MinSyncPulseCount= 8;
      _MaxSyncPulseCount= 10;
      _SyncPulseWidth_usec= 600;       // measured actual of 537..670
      _DataPulseLong_usec= 400;        // measured actual of 367..488. Avg ~450
      _DataPulseShort_usec= 200;       // measured actual of 84..245. Avg ~180. Subject to most noise.
      _TerminationPulseLong_usec= 1400;// measured 1400..30K+
      _PulseTolerance_usec= 99;        // 99 is max without overlap

      _MinMsgSizeBytes= 7;
      _MaxMsgSizeBytes= 9;
   }

   /* Set up message buffers */
   _MsgBfr1= new uint8_t[_MaxMsgSizeBytes];
   _MsgBfr2= new uint8_t[_MaxMsgSizeBytes];
   _pMsgBfrReady= NULL;
   _pMsgBfrPending= _MsgBfr1;

   _LastEntryTime_usec= micros();         // Capture time right before this gets turned on
   _Enabled= true;

} // SetDeviceType
/**************************************************************************************/
void QRadioRcvr::Enable(bool Flag)
{
   _Enabled= Flag;

} // Enable
/**************************************************************************************/
/* # items in queue.  */
int QRadioRcvr::Count()
{
   int Cnt;
   if (_TailIndex >= _HeadIndex)
      Cnt= _TailIndex - _HeadIndex;
   else
      Cnt= (_PulseBfrSize - _HeadIndex) + _TailIndex;
   return Cnt;

} // Count
/**************************************************************************************/
bool QRadioRcvr::IsFull()
/* Max fill is BfrSize-1. */   
{
   return (Count() >= (_PulseBfrSize - 1));
} // IsFull
/**************************************************************************************/
bool QRadioRcvr::IsEmpty()
{
   return (Count() == 0);
} // IsEmpty
/**************************************************************************************/
/* Add a data element to the fifo. Intended to be called by Interrupt Service Routine
   (ISR), so this code needs to be execution time efficient.
*/
ICACHE_RAM_ATTR int QRadioRcvr::AddPulse()
{
   int Result= 0;

   /* number of microseconds since the Arduino board began. Rolls over every ~70 minutes. */
   unsigned long NowTime_usec= micros(); 
   unsigned long DeltaTime_usec;
   if (NowTime_usec < _LastEntryTime_usec) 
      DeltaTime_usec= (ULONG_MAX - _LastEntryTime_usec) + 1 + NowTime_usec; // rollover case
   else 
      DeltaTime_usec= NowTime_usec - _LastEntryTime_usec;

   /* Cap it to 32K-1 */
   uint16_t Data= DeltaTime_usec & 0x7FFF;

   if (_Enabled && !IsFull())
   {
      _LastEntryTime_usec= NowTime_usec;
      _PulseBfr[_TailIndex]= Data;
      _TailIndex= (_TailIndex + 1) % _PulseBfrSize;
   }
   else
      Result= -1;

   return Result;   

} // AddPulse
#ifdef OLDSTUFF
/**************************************************************************************/
/* Remove a data element from the fifo.  */
uint16_t QRadioRcvr::RemovePulse()
{
   uint16_t Result= 0;
   if (Count() > 0)
   {
      Result= _PulseBfr[_HeadIndex];
      _HeadIndex= (_HeadIndex + 1) % _PulseBfrSize;
   }

   return Result;
} // RemovePulse
#endif
/**************************************************************************************/
/* Copies the message to the caller's buffer
   Returns: # bytes - <= 0 if no message                                      */
int QRadioRcvr::GetMessage(uint8_t * pMsgOutArr, int MsgBfrSize)
{
   int Result= _MsgCntBytes;
   if (_MsgCntBytes > 0)
   {
      for (int i= 0 ; i < _MsgCntBytes ; i++)
      {
         *(pMsgOutArr + i)= *(_pMsgBfrReady + i);   
      }
      _MsgCntBytes= 0;             

      #ifdef _RADIO_DEBUG
      char TraceStr[_MaxMsgSizeBytes * 3];
      ToHexStr(pMsgOutArr, Result, TraceStr);
      _Trace.printf(TRACE_SWITCH_RADIO, TLT_Verbose, "QRadioRcvr::GetMessage(): Message retrieved, Sz:%d [%s], %s", Result, TraceStr, GetStats());
      #endif
   }

   return Result;
} // GetMessage
/**************************************************************************************/
// Processing of Pulse Data
/**************************************************************************************/
/* Determines the type of pulse. 
   Returns
   0 =   Short data pulse
   1 =   Long data pulse
   2 =   Sync pulse 
   3 =   Extra Long, e.g. Termination pulse
   4 =   Invalid Pulse

   Note! This does not identify bit values, only pulse lengths.
*/   
int QRadioRcvr::GetPulseType(uint16_t PulseWidth_usec)
{
   int Result;
   uint16_t PulseMin;
   uint16_t PulseMax;

   do
   {
      Result= PT_Data_Short;
      PulseMin= _DataPulseShort_usec - _PulseTolerance_usec;
      PulseMax= _DataPulseShort_usec + _PulseTolerance_usec;
      if ((PulseWidth_usec >= PulseMin) && (PulseWidth_usec <= PulseMax))
         break;                              // Data - Short
      
      Result++;
      PulseMin= _DataPulseLong_usec - _PulseTolerance_usec;
      PulseMax= _DataPulseLong_usec + _PulseTolerance_usec;
      if ((PulseWidth_usec >= PulseMin) && (PulseWidth_usec <= PulseMax))
         break;                              // Data - long

      Result++;
      PulseMin= _SyncPulseWidth_usec - _PulseTolerance_usec;
      PulseMax= _SyncPulseWidth_usec + _PulseTolerance_usec;
      if ((PulseWidth_usec >= PulseMin) && (PulseWidth_usec <= PulseMax))
         break;                              // Sync

      Result++;
      PulseMin= _TerminationPulseLong_usec;
      if (PulseWidth_usec >= PulseMin)
         break;                              // Termination

      Result++;                              // Invalid (extra short)
      
   } while (0);

   return Result;

} // GetPulseType
/**************************************************************************************/
/* Starting at the head of the queue, determines if there are a minimum number of
   sync entries in a row. Tosses each sync up until a data bit is found, so leaves
   with data bit at head of queue.
   On entry, there is supposed to be a sync pulse at head of queue
   Returns: true  if sync pulse train found 
*/   
bool QRadioRcvr::IsSync()
{
   bool Result= false;
   int   PulseCount= 0;
   #ifdef _RADIO_DEBUG2   
   _SyncPulseMin= 1024; _SyncPulseMax= 0;
   #endif

   if (Count() >= _MaxSyncPulseCount + (_MinMsgSizeBytes * 8))
   {  // Sufficient data in queue to search for pulse train
      // tbd - Assert(sync pulse at head of queue)
      while (Count())
      {
         uint16_t PulseWidth= _PulseBfr[_HeadIndex];
         int PulseType= GetPulseType(PulseWidth);
         if (PulseType != PT_Sync)
         {  /* Not sync. */
            /* Finished our search. Possible conditions:
               - sufficient number of sync pulses found, data at head of queue
               - sufficient number of sync pulses found, invalid pulse at head of queue
               - Insufficient number of sync pulses found
            */
            //if ((PulseCount > _MinSyncPulseCount) && (PulseType >= 0))
            if (PulseCount > _MinSyncPulseCount)
               Result= true;           // Min pulse count for sync and current one not invalid

            break;
         }
         else
         {
            PulseCount++;
            #ifdef _RADIO_DEBUG2 
            _SyncPulseMin= (PulseWidth < _SyncPulseMin)?(PulseWidth):(_SyncPulseMin);  
            _SyncPulseMax= (PulseWidth > _SyncPulseMax)?(PulseWidth):(_SyncPulseMax); 
            #endif

            IncrHead(1);
         } 
      }
   }
   return Result;

} // IsSync
/**************************************************************************************/
/* Processes item at head of queue. If a data bit, removes it from queue.
   Returns:
   0:    0 bit
   1:    1 bit
   PulseType:  Other (not data), e.g. Termination 
*/   
int QRadioRcvr::GetBit()
{
   int Result= PT_Invalid;

   if (Count() >= 2)
   {
      uint16_t Pulse1_Width_usec= _PulseBfr[_HeadIndex]; 
      uint16_t Pulse2_Width_usec= _PulseBfr[(_HeadIndex + 1) % _PulseBfrSize];

      int Pulse1Type= GetPulseType(Pulse1_Width_usec);
      int Pulse2Type= GetPulseType(Pulse2_Width_usec);

      if ((Pulse1Type == PT_Data_Long) && (Pulse2Type == PT_Data_Short))
         Result= 1;
      else if ((Pulse1Type == PT_Data_Short) && (Pulse2Type == PT_Data_Long))
         Result= 0;

      #ifdef _RADIO_DECODE_METHOD2
      else
      {  /* Alt method - based on ratio of the 2 pulses. This occurs when the pair
            is shortened, leaving the short pulse too short for above method.
            If short pulse is close to standard 1/2 of long (1/3 of total), try again.      */
         uint16_t Total_Width_usec= Pulse1_Width_usec + Pulse2_Width_usec; 
         if (  (Total_Width_usec >= (_SyncPulseWidth_usec - _PulseTolerance_usec))
            && (Total_Width_usec <= (_SyncPulseWidth_usec + _PulseTolerance_usec)))
         //if (  (Total_Width_usec >= (_SyncPulseWidth_usec - _PulseTolerance_usec))
         //   && (Total_Width_usec <= (_SyncPulseWidth_usec)))
         {  // Sum of the 2 pulses is within range of a valid data pulse. So looks like a data pulse pair.
            uint16_t Short_Pulse_Width_usec= (Pulse1_Width_usec <= Pulse2_Width_usec)?(Pulse1_Width_usec):(Pulse2_Width_usec);
            uint16_t Long_Pulse_Width_usec=  (Pulse1_Width_usec > Pulse2_Width_usec)?(Pulse1_Width_usec):(Pulse2_Width_usec);
            
            /* Std tolerance is +- 25% of long pulse. Apply this percentage to these.
               No. Let's set nominal short as 1/3 of the total. */
            uint16_t Tolerance_usec= Long_Pulse_Width_usec >> 2;
            //uint16_t Nominal_Short_Pulse_Width_usec= Long_Pulse_Width_usec >> 1;
            uint16_t Nominal_Short_Pulse_Width_usec= Total_Width_usec / 3;
            if (  (Short_Pulse_Width_usec >= (Nominal_Short_Pulse_Width_usec - Tolerance_usec))
               && (Short_Pulse_Width_usec <= (Nominal_Short_Pulse_Width_usec + Tolerance_usec)))
            {  // Valid data pulse pair
               Result= (Pulse1_Width_usec <= Pulse2_Width_usec)?(0):(1);
               #ifdef _RADIO_DEBUG2
               _Trace.printf(TRACE_SWITCH_RADIO, TLT_Warning, "QRadioRcvr: Pulse Ratio Pass. %d/%d=%d",
                  Pulse1_Width_usec, Pulse2_Width_usec, Result);
               #endif
            }
   
            // Other method- allow to be +-X % of total
            //float Ratio= (float) Short_Pulse_Width_usec / (float) Total_Width_usec;
         }
      }
      #endif

      if (Result <= 1)
         IncrHead(2);
      else
      {  // Not a data bit. Back to seek
         if ((Pulse1Type == PT_Data_Short) && (Pulse2Type == PT_Termination))
            Result= PT_Termination;
         else
            Result= PT_Invalid;
      }
   }
   return Result;

} // GetBit
/**************************************************************************************/
/* Cycle the state machine. Requires at least 3 calls to post data.  */
void QRadioRcvr::DoStateMachine()
{
   /* Note- designed to allow re-entrance to the same state for completion,
      e.g. if waiting on fifo data. */
   switch (_State)
   {  
      default:
      case State_Sync_Seek:
      {  /* In this state, we process the queue until a single sync pulse is found.
            This takes care of purging all pulses until start of a valid sync.  */
         digitalWrite(LED_BUILTIN, HIGH);                 // 0/Low=On. Set high as we start seek. Falling edge can trigger scope
         while (Count() > 0)
         {
            if (GetPulseType(_PulseBfr[_HeadIndex]) == 2)
            {  /* Head of queue is a sync pulse. */
               _State= State_Sync_Pulse_Train;
               break;
            }
            else
               _HeadIndex= (_HeadIndex + 1) % _PulseBfrSize;   // toss it 
         }

         #ifdef _RADIO_INDICATOR
         if (_pIndicatorTimer->IsDone())
            digitalWrite(_PinMessageReceived, HIGH);   
         #endif
         break;
      }
      case State_Sync_Pulse_Train:
      {  /* Head of queue has a sync pulse in it. See if we have a sufficient sync
            pulse train. In this state, we are searching for a string of sync pulses. The number 
            of pulses can vary. e.g. for an RXB6 that is typically 8-9. Stop seeking
            once the trailing item is a data bit.         */
         _BitIndex= 0;
         /* Clear the message buffer. */
         for (int i= 0 ; i < _MaxMsgSizeBytes ; i++)
            *(_pMsgBfrPending + i)= 0;   

         if (Count() >= _MaxSyncPulseCount + (_MinMsgSizeBytes * 8))
         {  // Enough to see if there is a valid pulse train starting at head of queue.
            // tbd- assert(head is sync pulse)
            if (IsSync())
            {  // Found a valid sync pulse train. Head of queue is 1st data bit.
               digitalWrite(LED_BUILTIN, LOW);                 // 0/Low=On.
               #ifdef _RADIO_DEBUG2
               _Trace.printf(TRACE_SWITCH_RADIO, TLT_Verbose, "QRadioRcvr:Sync found, min/max:%d/%d, %s", _SyncPulseMin, _SyncPulseMax, Dump());
               #endif
               _State= State_Accumulate_Wait;                  // found N sync pulses in a row
               break;
            }
            else // Failed to find valid sync pulse train. Search again.
               _State= State_Sync_Seek;                        
         }

         break;
      }
      case State_Accumulate_Wait:
      {  // Have processed sync, next pulses should all be data bits. Wait until enough data pulses in queue
         int MinBits= _MinMsgSizeBytes * /*bits/byte*/ 8 * /*pulses/bit*/ 2;
         //int MinBits= 2 * 8;
         //int MinBits= /*bytes*/ 8 * /*bits/byte*/ 8 * /*pulses/bit*/ 2;
         if (Count() >= MinBits)
         {
            #ifdef _RADIO_DEBUG2
            // Dump the data pulses
            int TraceBfrSz= 128;
            if (_HeadIndex < _PulseBfrSize - MinBits)
            {
               char TraceBfr[TraceBfrSz];
               ToStr(_PulseBfr + _HeadIndex, /*#Items*/MinBits, TraceBfr, TraceBfrSz);
               _Trace.printf(TRACE_SWITCH_RADIO, TLT_Verbose, "QRadioRcvr: %s [%s]", Dump(), TraceBfr);
            }
            #endif

            _State= State_Accumulate_Data;
         }
         break;
      }
      case State_Accumulate_Data:
      {  // Accumulate data bits until hit non-data item or reach max message length
         while (Count() > 2)
         {
            int Bit= GetBit();
            if (Bit <= 1)
            {  // Pack the bit into the byte array. Bits are msb to lsb in pulse array.
               int MsgBfrIndex= _BitIndex >> 3;
               if (MsgBfrIndex >= _MaxMsgSizeBytes)
               {  // Message has hit max size. For now, keep it. tbd- whether to abandon and return to seek.
                  _Trace.printf(TRACE_SWITCH_RADIO, TLT_Warning, "QRadioRcvr: message size exceeds max (%d of %d), truncating.", MsgBfrIndex, _MaxMsgSizeBytes);
                  _State= State_Post_Data;
                  break;
               }
               else
               {
                  _pMsgBfrPending[MsgBfrIndex]|= (Bit << ((7-(_BitIndex % 8)))); 
                  _BitIndex++;
               }
            }
            else
            {  /* Not a data bit. Note, item is still in queue and will be processed by Seek.
                  If we got disabled in middle of this, start over. */
               _State= (_Enabled)?(State_Post_Data):(State_Sync_Seek);
               #ifdef _RADIO_DEBUG2
               if (Bit != PT_Termination)
                  _Trace.printf(TRACE_SWITCH_RADIO, TLT_Verbose, "QRadioRcvr: data stream terminated: BitIndex:%d, Pulse:%d,%d.",
                     _BitIndex, _PulseBfr[_HeadIndex], _PulseBfr[(_HeadIndex +1) % _PulseBfrSize]);
               #endif
               break;                                       // Done accumulating data
            }
         }
         break;
      }
      case State_Post_Data:
      {  /* Completed data accumulation. Post what we have if it meets criteria.
            If at least min # full bytes received and no partial bytes. */
         digitalWrite(LED_BUILTIN, HIGH);                 // 0/Low=On. Set high as we start seek. Rising edge identifies end of accumulation
         _MsgAttemptCnt++;
         int ByteCnt= _BitIndex >> 3;
         if ((_BitIndex == 0) || (_BitIndex % 8 != 0))
         {  // Partially complete byte, error
            #ifdef _RADIO_DEBUG2
            _MsgErrorCnt++;
            _Trace.printf(TRACE_SWITCH_RADIO, TLT_Verbose, "QRadioRcvr: incomplete data byte received, dropping message. BitIndex:%d, %s", _BitIndex, GetStats());
            #endif
         }
         else if (ByteCnt < _MinMsgSizeBytes)
         {  // Message too short, error
            #ifdef _RADIO_DEBUG2
            _MsgErrorCnt++;
            _Trace.printf(TRACE_SWITCH_RADIO, TLT_Warning, "QRadioRcvr: message size below min (%d of %d).", ByteCnt, _MinMsgSizeBytes);
            #endif
         }
         else if (_MsgCntBytes > 0)
         {  // Prior message not read yet, dump this one.
            #ifdef _RADIO_DEBUG2
            _Trace.printf(TRACE_SWITCH_RADIO, TLT_Warning, "QRadioRcvr: message overrun.");
            #endif
         }
         else
         {  // Successful receive. Set the active buffer and mark as message ready
            #ifdef _RADIO_INDICATOR
            digitalWrite(_PinMessageReceived, LOW);
            _pIndicatorTimer->Start();
            #endif
   
            _pMsgBfrReady= _pMsgBfrPending;
            _pMsgBfrPending= (_pMsgBfrPending == _MsgBfr1)?(_MsgBfr2):(_MsgBfr1);   // ping pong the buffers
            _MsgCntBytes= ByteCnt;                          // signals to host that data is available
         }
         _State= State_Sync_Seek;
         break;   
      }
   }
} // DoStateMachine






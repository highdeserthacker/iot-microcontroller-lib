///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  Radio.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QRadio_h
#define QRadio_h

#include "AllApps.h"
#include "QTimer.h"
#define _RADIO_DEBUG
//#define _RADIO_DEBUG2
//#define  _RADIO_INDICATOR               // Blinks led on D6 when packet received. Note- can conflict with parent indicator
#define _RADIO_DECODE_METHOD2

/**************************************************************************************/
/* Objects for handling digital radio, such as RXB6.
   - ISR for capturing pulse timing to a queue
   - sync detection
   - bit decoding
*/   
/**************************************************************************************/
class QRadioRcvr
{
   protected:
   typedef enum RadioType
   {
      Radio_Type_Default=        0,
      Radio_Type_RXB6_315,
      Radio_Type_RXB6_433,
      Radio_Type_Count
   };

   public:
   typedef enum DeviceType
   {
      Device_Type_Default=        0,
      Device_Type_Accurite,
      Device_Type_Count
   };

   typedef enum PulseType
   {
      PT_Data_Short=             0,
      PT_Data_Long,
      PT_Sync,
      PT_Termination,
      PT_Invalid,
      PT_Count
   };

   protected:
   enum
   {
      State_Sync_Seek=        0,
      State_Sync_Pulse_Train,
      State_Accumulate_Wait,
      State_Accumulate_Data,
      State_Post_Data,
      State_Count
   };
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   /* Set to false to stop ISR from processing pulses. */
   bool                    _Enabled;

   //////// Radio Settings ////////
   /* Defaults to RXB6 433 MHz Radio. */
   uint8_t                 _RadioType;
   uint8_t                 _DeviceType;

   /* Sync pulse width in usec. */
   uint16_t                _SyncPulseWidth_usec;

   /* Number of pulses for a valid sync sequence. */
   uint16_t                _MinSyncPulseCount;
   uint16_t                _MaxSyncPulseCount;

   /* Data pulse settings. */
   uint16_t                _DataPulseLong_usec;
   uint16_t                _DataPulseShort_usec;

   /* Termination pulse. */
   uint16_t                _TerminationPulseLong_usec;

   /* Measurement of pulse width within +- this value is acceptable. */
   uint16_t                _PulseTolerance_usec;

   //////// Pulse Queue ////////
   /* Contains time in usec since previous pulse edge (delta entries).
      Valid range from 0..32K-1. Bit 15 reserved. */
   uint16_t                _PulseBfrSize;
   uint16_t *              _PulseBfr;

   /* Keeps track of time since last pulse entered into queue. */
   unsigned long           _LastEntryTime_usec;

   /* Head points to the next available data. */
   uint16_t                _HeadIndex;
   /* Tail points to the next empty spot. */
   uint16_t                _TailIndex;

   //////// State Machine ////////
   int                     _State;
   int                     _BitIndex;

   //////// Messages ////////
   /* 2 ping ponged message buffers. */
   int                     _MinMsgSizeBytes;
   int                     _MaxMsgSizeBytes;
   uint8_t *               _MsgBfr1;
   uint8_t *               _MsgBfr2;
   int                     _MsgCntBytes;        // >0 message is waiting in Ready buffer
   uint8_t *               _pMsgBfrReady;       // Points to completed message buffer
   uint8_t *               _pMsgBfrPending;     // Points to message buffer being filled

   #ifdef _RADIO_INDICATOR
   QTimer *                _pIndicatorTimer;   
   int                     _PinMessageReceived;
   #endif

   #ifdef _RADIO_DEBUG
   char                    _TraceBfr[127+1];
   uint16_t                _SyncPulseMin;
   uint16_t                _SyncPulseMax;

   /* Message Error Rate Calculation. */
   unsigned long           _MsgAttemptCnt;
   //unsigned long           _MsgSuccessCnt;
   unsigned long           _MsgErrorCnt;

   public:
   const char *            Dump();
   const char *            GetStats();
   #endif
   
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QRadioRcvr(int PulseBfrSize);

   /* Sets the device type/family. This defines the radio protocol that is to be used:
         - sync pulse lengths and count
         - data bit protocol
         - min/max message size
      Note that data capture is only enabled after this call.           
   */
   void                    SetDeviceType(uint8_t DeviceType);

   /* Radio (and interrupts) should be disabled on OTA updates. On disable->enable transition, radio
      starts in the seek state. */
   void                    Enable(bool Flag);

   //////// Queue ////////
   int                     Count();
   bool                    IsFull();
   bool                    IsEmpty();

   /* If enabled, adds the data element (pulse timestamp) to the fifo. 
      Intended to be called by Interrupt Service Routine (ISR), so this code is cached in ram.
      Returns: 0: added to queue, <0: not added- disabled or queue full.
   */
   int                     AddPulse();

   void                    DoStateMachine();
   int                     GetMessage(uint8_t * pMsgOutArr, int MsgBfrSize);

   protected:
   void                    Init();
   void                    SetRadioType(uint8_t RadioType);
   void                    IncrHead(int k){_HeadIndex= (_HeadIndex + k) % _PulseBfrSize;};
   int                     GetPulseType(uint16_t PulseWidth_usec);
   bool                    IsSync();
   int                     GetBit();
   
};

#endif

///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTrace.h - for display of debug information on configured output, e.g. serial

*/
///////////////////////////////////////////////////////////////////////////////
#ifndef QTrace_h
#define QTrace_h

#include "Arduino.h"
#include <inttypes.h>
#include <assert.h>


/**************************************************************************************/
#define TRACE_MAX_STRING      511

enum TraceLevelType
{
   TLT_Off=       0,
   TLT_Event,
   TLT_Error,
   TLT_Warning,
   TLT_Info,
   TLT_Verbose,
   TLT_Reserved1,                                     // reserved
   TLT_Max
};

/* Trace switches - Internal & Libraries. Switches 0..3 reserved for applications. */
enum
{
   TRACE_SWITCH_DFLT=                  0,
   TS_MAIN=                            TRACE_SWITCH_DFLT,
   TS_DFLT=                            TRACE_SWITCH_DFLT,

   TRACE_SWITCH_INTERNAL_START=        4,
   TRACE_SWITCH_WIFI=                  TRACE_SWITCH_INTERNAL_START,
   TRACE_SWITCH_MQTT,
   TRACE_SWITCH_LIB,
   TS_SERVICES=                        TRACE_SWITCH_LIB
};

typedef uint32_t  QTraceSwitch;

/**************************************************************************************/
/* ASSERT   */
/**************************************************************************************/
//extern void __assert(const char * __file, int __lineno, const char * __func, const char * __sexp);

#ifdef NDEBUG                                         /* required by ANSI standard */
# define ASSERT(__e) ((void)0)
# define ASSERT_MSG(__e, pMsg) ((void)0)
#else
# define ASSERT(__e) ((__e) ? (void)0 : QTrace::Assert (PSTR(__FILE__), __LINE__, __ASSERT_FUNC, PSTR(#__e), NULL))
# define ASSERT_MSG(__e, pMsg) ((__e) ? (void)0 : QTrace::Assert (PSTR(__FILE__), __LINE__, __ASSERT_FUNC, PSTR(#__e), pMsg))
#endif


/**************************************************************************************/
/* Handles debug trace logs.
   Calls can use the global _Trace.xx() object or QTrace::Master()->xx(), both are equivalent.
   Can output to:
      Serial
      Generic callbacks - e.g. to MQTT
   
   Note that calls to this class can be issued even when everything is disabled. Therefore,
   trace statements can always be active in code (not surrounded by #ifdef statements).
*/   
/**************************************************************************************/
class QTrace
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   /* Static Data          */
   static QTrace *         _pMasterObject;   

   public:
   static const uint32_t   _TraceSwitchesLevel_Warning=  0x33333333;
   static const uint32_t   _TraceSwitchesLevel_Verbose=  0x55555555;
   static const uint32_t   _TraceSwitchesLevel_Max=      0x77777777;
   
   ///////////////////////////////////////////////////////////
   protected:
   /* Is a nibble field, containing 8 4 bit nibbles. Each nibble contains the trace level threshold. These can be assigned for various sections of an 
      application to enable tracing in desired sections. It takes the caller's given position (0..7) and level, and checks it against the trace level 
      specified here.
      The level specified in the switch indicates the max reporting level. The caller's level must be less than or equal to this to be reported.
   */
   uint32_t                _TraceSwitches;
   
   /* Output destinations. Note that more than one can be active at a time. */
   bool                    _EnbSerial;

   /* Optional pointer to callback function. e.g. to trace to mqtt.
      Called by print().      */
   void                    (* _pCallback)(const char *);
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   static QTrace *         Master(){return _pMasterObject;}
   static void             Assert(const char * __file, int __lineno, const char * __func, const char * __sexp, const char * pMsg);

   /* Assert callback. */
   //static void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp);

   ///////////////////////////////////////////////////////////
   public:
                           QTrace();
   void                    SetTraceToSerial(bool On){_EnbSerial= On;} 

   /* Sets an optional callback to an external function for hooking into trace output.
      This allows output of trace information to other channels, e.g. mqtt, local display, etc.
      Note that callback function is responsible for immediately flushing the string, it is not guaranteed
      to be persistent.    */                           
   void                    SetCallback(void (* pCallback)(const char *));

   /* Sets all of the trace switches to the specified level. */
   void                    SetTraceSwitch(uint32_t TraceSwitches);

   /* Trace print output methods.
      TraceSwitchId: 0..7
   */      
   void                    print(uint8_t TraceSwitchId, TraceLevelType TraceLevel, char pStr[]);
   void                    print(char * pStr);

   void                    printf(uint8_t TraceSwitchId, TraceLevelType TraceLevel, char pStr[], ...);
   void                    printf(TraceLevelType TraceLevel, char pStr[], ...);  // uses default switch
   
   protected:
   void                    Init();
   uint8_t                 GetTraceSwitch(uint8_t TraceSwitchId);
   void                    PrintIt(const char * pStr);
   
}; // QTrace

extern QTrace _Trace;

#endif

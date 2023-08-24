///////////////////////////////////////////////////////////////////////////////
/*  QTrace.cpp       */
///////////////////////////////////////////////////////////////////////////////
#ifdef ASSERT
#error "ASSERT is already defined." 
#endif
#include "QTrace.h"

#ifdef NDEBUG
#error "NDEBUG is defined. assert() is disabled." 
#endif


/**************************************************************************************/
// QTrace - Static Member Initialization
/**************************************************************************************/
/* The global trace object. */
QTrace *         QTrace::_pMasterObject= NULL;
QTrace           _Trace= QTrace();

/**************************************************************************************/
/*static*/ void QTrace::Assert(const char * __file, int __lineno, const char * __func, const char * __sexp, const char * pMsg)
{
   if (QTrace::Master() != NULL)
   {
      if (pMsg == NULL)
         QTrace::Master()->printf(TS_DFLT, TLT_Event,"ASSERT: at %s#%d:%s expr:%s", __file, __lineno, __func, __sexp);
      else  
         QTrace::Master()->printf(TS_DFLT, TLT_Event,"ASSERT: [%s] at %s#%d:%s expr:%s", pMsg, __file, __lineno, __func, __sexp);  
   }

} // Assert


/**************************************************************************************/
QTrace::QTrace()
{
   Init(); 
} // QTrace
/**************************************************************************************/
void QTrace::Init()
{
   if (QTrace::_pMasterObject == NULL)
      _pMasterObject= this;

   _TraceSwitches= _TraceSwitchesLevel_Warning;
   _EnbSerial= true;
   _pCallback= NULL;

} // Init 
/**************************************************************************************/
void QTrace::SetCallback(void (* pCallback)(const char *))
{
   _pCallback= pCallback;
   
} // SetCallback
/**************************************************************************************/
uint8_t QTrace::GetTraceSwitch(uint8_t TraceSwitchId)
{
   int Switch= (_TraceSwitches >> (TraceSwitchId*4)) & 0x0F;
   return Switch;
   
} // GetTraceSwitch
/**************************************************************************************/
void QTrace::SetTraceSwitch(uint32_t TraceSwitches)
{
   _TraceSwitches= TraceSwitches;
   
} // SetTraceSwitch
/**************************************************************************************/
void QTrace::PrintIt(const char * pStr)
/* This method is for private use. Determined at this point that trace will be outputted. */
{
   if (_pCallback != NULL)
      _pCallback(pStr);
   
   if (_EnbSerial)
   {
      Serial.print(pStr);
      Serial.print("\n");
   }

} // PrintIt
/**************************************************************************************/
// Trace Switch & Level Prints
/**************************************************************************************/
void QTrace::print(uint8_t TraceSwitchId, TraceLevelType TraceLevel, char pStr[])
/*
   Inputs:  TraceSwitchId  0..7
            TraceLevel
            Str
*/
{
   /* Get the trace threshold for this switch. */
   int Switch= GetTraceSwitch(TraceSwitchId);
   
   /* Does the given trace level meet the threshold specified in the switch? 
      If this trace entry is more verbose than setting, we ignore it. */
   if ((int) TraceLevel <= Switch)
   {
      PrintIt(pStr);     
   }
   
} // print
/**************************************************************************************/
void QTrace::print(char * pStr)
/* Defaults to print to TraceSwitch zero. Useful for simple apps.
   TraceSwitchId  defaults to 0
   TraceLevel     defaults to Verbose
*/
{
   print(/*TraceSwitchId*/0, TLT_Verbose, pStr);
   
} // print
/**************************************************************************************/
void QTrace::printf(uint8_t TraceSwitchId, TraceLevelType TraceLevel, char pStr[], ...)
/*
   Inputs:  TraceSwitchId  0..7
            TraceLevel
            Str
*/
{ 
   char PrnBfr[_TraceBfrSize];
   /* Get the trace threshold for this switch. */
   //int Switch= (_TraceSwitches >> (TraceSwitchId*4)) & 0x0F;
   int Switch= GetTraceSwitch(TraceSwitchId);
   
   /* Does the given trace level meet the threshold specified in the switch? 
      If this trace entry is more verbose than setting, we ignore it. */
   if ((int) TraceLevel <= Switch)
   {
      va_list arg_list;
      va_start(arg_list, pStr);
      //vsprintf(PrnBfr, pStr, arg_list);
      vsnprintf(PrnBfr, sizeof(PrnBfr), pStr, arg_list);
      va_end (arg_list);
   
      PrintIt(PrnBfr);
   }
   
} // printf
/**************************************************************************************/
void QTrace::printf(TraceLevelType TraceLevel, char pStr[], ...)
/* Output to default trace switch.
   Inputs:  TraceLevel
            Str
*/
{
   char PrnBfr[_TraceBfrSize];
   /* Get the trace threshold for this switch. */
   //int Switch= (_TraceSwitches >> (TraceSwitchId*4)) & 0x0F;
   int Switch= GetTraceSwitch(/*TraceSwitchId*/ TS_DFLT);
   
   /* Does the given trace level meet the threshold specified in the switch? 
      If this trace entry is more verbose than setting, we ignore it. */
   if ((int) TraceLevel <= Switch)
   {
      va_list arg_list;
      va_start(arg_list, pStr);
      //vsprintf(PrnBfr, pStr, arg_list);
      vsnprintf(PrnBfr, sizeof(PrnBfr), pStr, arg_list);
      va_end (arg_list);
   
      PrintIt(PrnBfr);
   }
   
} // printf





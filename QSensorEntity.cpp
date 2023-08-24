///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QSensorEntity.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "Arduino.h"             // e.g. DigitalRead(), sprintf()
#include "QSensorEntity.h"
#include "QTrace.h"

/**************************************************************************************/
// QSensorEntity
/**************************************************************************************/
#ifdef _DEBUG_QSENSORENTITY
char                       QSensorEntity::_TraceBfr[_DumpBfrLen+1];
#endif

/**************************************************************************************/
QSensorEntity::QSensorEntity(const char * pName, SensorEntityType SensorType, int StaleTimeSec)
{
   Init(StaleTimeSec);
   _pName= pName;
   _SensorType= SensorType;
   
} // QSensorEntity
/**************************************************************************************/
void QSensorEntity::Init(int StaleTimeSec)
{
   _SensorType= SensorEntityType::ST_None;
   _ValueFloat= 0;
   _StaleTimeSec= StaleTimeSec;
   _pStaleTimer= new QTimer(/*sec*/_StaleTimeSec * /*msec*/1000,/*Repeat*/false,/*Start*/false,/*Done*/true);

} // Init
#ifdef _DEBUG_QSENSORENTITY
/**************************************************************************************/
const char * QSensorEntity::Dump()
{
   snprintf(_TraceBfr, _DumpBfrLen, "QSensorEntity: %s:%.1f, Stale:%s", 
      _pName, _ValueFloat, this->IsStale()?("true"):("false"));
   return _TraceBfr;
   
} // Dump
#endif
/**************************************************************************************/
void QSensorEntity::Set(float Value)
{
   _ValueFloat= Value;
   _pStaleTimer->Start();                             // Reset the stale timer.
   ASSERT_MSG(!IsStale(), "QSensorEntity::Set(): stale failure"); 
   #ifdef _DEBUG_QSENSORENTITY
   _Trace.printf(TS_SERVICES, TLT_Max, "QSensorEntity::Set(): %s=%.2f", _pName, _ValueFloat);
   #endif

} // Set
/**************************************************************************************/
float QSensorEntity::Get()
{
   return _ValueFloat;

} // Get
/**************************************************************************************/
bool QSensorEntity::IsStale()
{
   // Until timers in !Repeat mode stay in Done state.
   //return (!_pStaleTimer->IsEnabled()) || _pStaleTimer->IsDone();
   return _pStaleTimer->IsDone();

} // IsStale






///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QSensorEntity.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QSensorEntity_h
#define QSensorEntity_h

#include "QTimer.h"
#include "AllApps.h"

#define  _DEBUG_QSENSORENTITY
/**************************************************************************************/
/* QSensorEntity - Abstraction layer to hold sensor readings from external devices.
   e.g. readings from mqtt based entities.
*/   
/**************************************************************************************/
class QSensorEntity
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:
   static const int        _DumpBfrLen= 95;

   public:
   typedef enum SensorEntityType
   {
      ST_None=          0,
      ST_Float,
      ST_Count
   };

   protected:
   const char *            _pName;

   SensorEntityType        _SensorType;

   /* Sensor readings. */
   float                   _ValueFloat;

   /* Controls how long a sensor value remains fresh.
      Defaults to 5 minutes.     */
   int                     _StaleTimeSec;
   QTimer *                _pStaleTimer;

   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QSensorEntity(const char * pName, SensorEntityType SensorType, int StaleTimeSec);
   void                    Set(float Value);
   float                   Get();
   bool                    IsStale();

   #ifdef _DEBUG_QSENSORENTITY
   public:
   static char             _TraceBfr[_DumpBfrLen+1];
   const char *            Dump();
   #endif
   
   protected:
   void                    Init(int StaleTimeSec);
   
};


#endif

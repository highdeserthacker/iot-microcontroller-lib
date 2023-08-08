///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTemperature.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QTemperature_h
#define QTemperature_h

// I2C Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

//#define  _DEBUG_TEMPERATURE                           // Enables trace dump

/**************************************************************************************/
/* QTemperature - handles temperature sensors, including:
      TMP36
      I2C
*/   
/**************************************************************************************/
class QTemperature
{
   public:
   typedef enum TemperatureSensorT
   {
      TST_TMP36=             0,     // TMP36 analog sensor
      TST_I2C,                      // xx18B20 I2c temperature sensor
      TST_Count
   };

   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:
   static const int        _MinValidTemperatureF= -190;

   protected:
   int                     _PinNumber;
   uint8_t                 _SensorType;

   // TMP36 Sensor
   // Compensate for ADC voltage offset. This varies from device to device.
   float                   _VoltageOffset;

   // I2C Sensor
   OneWire *               _pOneWire;
   DallasTemperature *     _pDallasTemperature;

   float                   _TemperatureF;
   float                   _TemperatureC;
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QTemperature(TemperatureSensorT SensorType, int Pin);

   /* If I2C, indicates if device is found/connected. */
   bool                    IsActive();

   /* Returns < _MinValidTemperatureF if device is not responding. */
   float                   GetTemperatureF();
   
   protected:
   void                    Init();
   void                    ReadTemperature();
   
}; // QTemperature

#endif

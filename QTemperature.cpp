///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QTemperature.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "QTemperature.h"
#include "QTrace.h"
//#include <wiring.h>                                   // analogRead()
#include "Arduino.h"                                 // analogRead()

/**************************************************************************************/
// QTemperature
/**************************************************************************************/
QTemperature::QTemperature(TemperatureSensorT SensorType, int Pin)
{
   Init();
   _PinNumber= Pin;
   _SensorType= SensorType;

   if (_SensorType == TemperatureSensorT::TST_I2C)
   {
      _pOneWire= new OneWire(_PinNumber);    // TBD- onewire s/b initialized 1 time somewhere else.
      _pDallasTemperature= new DallasTemperature(_pOneWire);
   }
   
} // QTemperature
/**************************************************************************************/
void QTemperature::Init()
{
   _PinNumber= -1;
   _SensorType= 0;
   _VoltageOffset= 0.050;
   _pOneWire= NULL;
   _pDallasTemperature= NULL;

} // Init
/**************************************************************************************/
bool QTemperature::IsActive()
// NOT IMPLEMENTED
{
   bool Result= true;
   if (_SensorType == TemperatureSensorT::TST_I2C)
   {  // Check that I2C sensor is connected
      //Result= _pDallasTemperature->isConnected();
      //int DeviceCnt= _pDallasTemperature->isConnected();
   }

   return Result;

} // IsActive
/**************************************************************************************/
void QTemperature::ReadTemperature()
{
   if (_SensorType == TemperatureSensorT::TST_TMP36)
   {  // Analog Sensor
      /* Read the temperature.
         TMP36 output is 10 mV/�C Scale Factor. It is calibrated for 0.5v at 0 degrees C
         Example: 0.715 mV= (0.715-0.5) / 10 mV/�C = 21.5 �C = 70.7 F
         3.3 reference voltage is critical to A/D reading.
         Note that voltage ladder inside esp8266 references to 0..3.2v -> 0..1.0 for A/D,
         so we reference the analog reading to 3.2v max.
         A/D is 10 bits (0..1023). Resolution= 3.1mV= ~1/3�C= 0.56 �F      */
      int AnalogReading= analogRead(_PinNumber);
      float VoltageReference= 3.2;
      float Voltage= (AnalogReading * VoltageReference) / 1024.0; // 1023= 3.3v, 0= 0v.
      // Compensate for ADC voltage offset. This varies from device to device.
      //float VoltageOffset= 0.050; // 0.015; 
      Voltage-=   _VoltageOffset;                       
      _TemperatureC= (Voltage - 0.5) * 100.0;
      _TemperatureF= ((_TemperatureC * 9.0) / 5.0) + 32.0; 
      
      //if ((_TemperatureF >= 0) && (_TemperatureF <= 140)) 
      {
         #ifdef _DEBUG_TEMPERATURE
         _Trace.printf(TRACE_SWITCH_LIB, TLT_Verbose, "Temperature (TMP36): %.1fC, %.1fF, Voltage:%.3f, ADC:%d", 
            _TemperatureC, _TemperatureF, Voltage, AnalogReading);
         #endif
      }
   }
   else if (_SensorType == TemperatureSensorT::TST_I2C)
   {  // I2C Digital Sensor
      _pDallasTemperature->requestTemperatures();
      // Returns DEVICE_DISCONNECTED_F (-196.6) if device disconnected.
      _TemperatureF= _pDallasTemperature->getTempFByIndex(/*DeviceIndex*/0);
      _TemperatureC= _pDallasTemperature->getTempCByIndex(/*DeviceIndex*/0);

      #ifdef _DEBUG_TEMPERATURE
      //_Trace.printf(TRACE_SWITCH_LIB, TLT_Verbose, "Temperature (I2C): %.1fC, %.1fF", _TemperatureC, _TemperatureF);
      if (_TemperatureF <= DEVICE_DISCONNECTED_F)
      {
         int I2CDeviceCnt= _pDallasTemperature->getDeviceCount();
         int TempSensorCnt= _pDallasTemperature->getDS18Count();

         // Causes wire->reset_search(). Always returns false, even when it's working!         
         DeviceAddress deviceAddress;
         bool GetAddressResult= _pDallasTemperature->getAddress(deviceAddress, /*deviceIndex*/0);

         /*
         _Trace.printf(TRACE_SWITCH_LIB, TLT_Verbose, "#Devices: %d, GetAddress:%s",
            I2CDeviceCnt,
            (GetAddressResult == true)?("Ok"):("Not Ok")
            );*/
         _Trace.printf(TRACE_SWITCH_LIB, TLT_Verbose, "#Devices: %d, #TempSensors: %d",
            I2CDeviceCnt,
            TempSensorCnt
            );

         if (I2CDeviceCnt == 0)
         {
            _Trace.printf(TRACE_SWITCH_LIB, TLT_Verbose, "QTemperature::ReadTemperature(): call begin()");
            _pDallasTemperature->begin();
            delay(/*msec*/2000);
         }          
      }
      else
         _Trace.printf(TRACE_SWITCH_LIB, TLT_Verbose, "Temperature (I2C): %.1fF", _TemperatureF);
      #endif
   }

} // ReadTemperature
/**************************************************************************************/
float QTemperature::GetTemperatureF()
{
   ReadTemperature();
   return _TemperatureF;
} // GetTemperature




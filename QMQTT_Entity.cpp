///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QMQTT_Entity.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "Arduino.h"             // e.g. DigitalRead(), sprintf()
#include <ArduinoJson.h>
#include "QMQTT_Entity.h"
#include "QTrace.h"
#include "QIndicator.h"

/**************************************************************************************/
// QMQTT_Entity - Statics
/**************************************************************************************/
/* Define all non-const static variables.
   Unfortunately, Arduino compiler requires out of class initialization for non-const
   static variables.
   https://stackoverflow.com/questions/185844/how-to-initialize-private-static-members-in-c
*/
int               QMQTT_Entity::_EntityCount= -1;
QMQTT_Entity *    QMQTT_Entity::_EntityInstanceArr[MAX_ENTITY_INSTANCES];
QWifi *           QMQTT_Entity::_pWifi= NULL;
QTimer *          QMQTT_Entity::_pServiceTimer= NULL;

QMQTT *           QMQTT_Entity::_pMQTT= NULL;         // tbd- replace with QMQTT::Master()
char              QMQTT_Entity::_pEntitiesTopic[MQTT_TOPIC_LEN+1];
char              QMQTT_Entity::_pSubscribeTopic[MQTT_TOPIC_LEN+1];
QTimer *          QMQTT_Entity::_pAvailabilityTimer;

char              QMQTT_Entity::_pJsonPayloadStr[MAX_JSON_PAYLOAD_STR+1];

QShiftRegister *  QMQTT_Entity::_pShiftRegister= NULL;
#ifdef _MQTT_ENTITY_DEBUG
char              QMQTT_Entity::_TraceBfr[127+1];
#endif


/**************************************************************************************/
/*static*/ void QMQTT_Entity::Initialize()
{
   if (QMQTT_Entity::_EntityCount < 0)
   {
      QMQTT_Entity::_EntityCount= 0;
      QMQTT_Entity::_pWifi= QWifi::Master();
      QMQTT_Entity::_pMQTT= QMQTT::Master();
      QMQTT_Entity::_pServiceTimer= new QTimer(/*Msec*/1000,/*Repeat*/true,/*Start*/true);
      QMQTT_Entity::_pAvailabilityTimer= new QTimer(_AvailabilityReportingSec */*msec*/1000,/*Repeat*/true,/*Start*/false, /*Done*/true);

      /* Generate the path to entities top, under which all entities will hang.
         Of the form "device/DeviceId/#", e.g. device/lighting_back     */
      sprintf(_pEntitiesTopic, "%s/%s", TOPIC_PREFIX_DEVICE, _pMQTT->GetIdentifier());
   
      /* Set up the callback for subscribing to this device's topic.
         Note that Trace dumps will also generate callbacks here, so any Trace in these routines is overload. */
      sprintf(_pSubscribeTopic, "%s/#", _pEntitiesTopic);
      _pMQTT->Subscribe(/*Topic*/_pSubscribeTopic, /*Callback*/QMQTT_Entity::MQTT_Callback);
   
      _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity::Initialize(): SubscribeTo:[%s]", _pSubscribeTopic);
   }
} // Initialize
/**************************************************************************************/
/*static*/ void QMQTT_Entity::Initialize(QShiftRegister * pShiftRegister)
{
   Initialize();
   QMQTT_Entity::_pShiftRegister= pShiftRegister;
} // Initialize
/**************************************************************************************/
/*static*/ void QMQTT_Entity::MQTT_Callback(char * pTopic, byte * pPayload, unsigned int PayloadLength)
/* Callback from mqtt server on command channel. This method serves as a dispatcher.
   Implemented here (vs QMQTT) as we need to redirect to appropriate QMQTT_Entity (or subclassed) object.
   Note that we can also arrive here from trace callbacks. So do *not* perform any trace statements within here.
   Inputs:  pTopic         - e.g. device/lighting_back/pathway/set
            pPayload       - raw byte format. For this app it is json.
            PayloadLength

   Note - Trace does not work inside of callbacks and there are intermittent failures that occur.
*/
{
   #ifdef _MQTT_ENTITY_DEBUG_DISABLED
   Serial.printf("QMQTT_Entity::MQTT_Callback(): Topic:[%s]\n", pTopic);
   #endif
   
   /* Messages are json text, so convert from bytes. */
   char Message[PayloadLength + 1];
   for (int i= 0 ; i < PayloadLength ; i++)
   {
      Message[i]= (char) pPayload[i];
   }
   Message[PayloadLength]= '\0';

   /* Review the entity list and see which if any should receive this callback message.
      Topic can be a subtopic of root.
      Right now, this just checks for the command topic.        */
   int BfrSz= MQTT_TOPIC_LEN+1;
   char pEntityTopicCommand[BfrSz];
   for (int i= 0 ; i < _EntityCount ; i++)
   {
      QMQTT_Entity * pEntity= _EntityInstanceArr[i];
      /* Generate the full path to this entity's command topic.
         If entity topic is too long, it will be ignored completely.      */
      int FullLength= snprintf(pEntityTopicCommand, BfrSz, "%s/%s/%s", 
         _pEntitiesTopic,                             // e.g. devices/mydevicename
         pEntity->_pSubTopicEntity,                   // e.g. "myswitch"
         pEntity->_pSubTopicCommand);

      if ((FullLength < BfrSz) && strcmp(pTopic, pEntityTopicCommand) == 0)
      {  /* The topic for this callback matches this entity instance. Do not trace in here. */
         /* Call DoCommand() for this instance */
         pEntity->DoCommand(Message);
         break;
      }
   }

} // MQTT_Callback
/**************************************************************************************/
/* Publishes the payload to the appropriate mqtt state topic expected by HomeAssistant.
   Inputs:  pText    payload- text, json.
*/
/*static*/void QMQTT_Entity::ReportAvailability()
{
   /* Generate the topic path for availability publishing. */
   char pTopic[MQTT_TOPIC_LEN+1];
   sprintf(pTopic, "%s/%s", _pEntitiesTopic, _pAvailabilitySubTopic);
   bool Result= QMQTT_Entity::_pMQTT->Publish(/*Topic*/pTopic, /*Payload*/_pAvailability_Online);

} // ReportAvailability
/**************************************************************************************/
/*static*/ void QMQTT_Entity::DoService()
{
   if ((_EntityCount > 0) && _pAvailabilityTimer->IsDone())
   {
      ReportAvailability();                           // Periodic reporting of availability
   }

   if (_pServiceTimer->IsDone())
   {
      /* Service each entity - for entities that need periodic checks/calls, e.g. to read state
         sensor value, etc. */
      for (int i= 0 ; i < _EntityCount ; i++)
      {
         QMQTT_Entity * pEntity= _EntityInstanceArr[i];
         pEntity->CheckEntity();
      }
   }

} // DoService
/**************************************************************************************/
// Helpers
/**************************************************************************************/
/* Generates json string payload for single key/value case.
   Note - utilizes static buffer, so not thread safe and resultant string must be
   processed before next call.            
*/
/*static*/ char * QMQTT_Entity::GetJsonStr(const char * pKey, const char * pValue)
{
   /* Prep json buffer. */
   StaticJsonBuffer<JSON_OBJECT_SIZE(8)> jsonBuffer;
   JsonObject& JsonRoot= jsonBuffer.createObject();
   JsonRoot[pKey]= pValue;

   /* Generate text string payload. Refer to https://arduinojson.org/v5/api/jsonobject/printto/  */
   int Cnt= JsonRoot.measureLength() + 1;
   int nBytes= JsonRoot.printTo(_pJsonPayloadStr, sizeof(_pJsonPayloadStr));
   return _pJsonPayloadStr;

} // GetJsonStr


/**************************************************************************************/
// QMQTT_Entity
/**************************************************************************************/
QMQTT_Entity::QMQTT_Entity(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow)
{
   Init(IOType, Address, ActiveLow, pSubTopicEntity);
   _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity(): Created entity %d, type:%d, Addr:%d, topic:[%s]", 
      _EntityCount, _IOType, _Address, _pSubTopicEntity);

} // QMQTT_Entity
/**************************************************************************************/
QMQTT_Entity::QMQTT_Entity(const char * pSubTopicEntity)
{
   Init(EIOT::IOT_SHIFT_REGISTER, /*Address*/-1, /*ActiveLow*/false, pSubTopicEntity);
   _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity(): Created instance %d, topic:[%s]", _EntityCount, _pSubTopicEntity);

} // QMQTT_Entity
/**************************************************************************************/
void QMQTT_Entity::Init(EIOT IOType, int Address, bool ActiveLow, const char * pSubTopicEntity)
{
   /* If global initialization not performed yet, do it. */
   if (_EntityCount < 0)
      Initialize();

   _Id= _EntityCount;
   _IOType= IOType;
   _Address= Address;
   _ActiveLow= ActiveLow;
   _State= 0;
   _pSubTopicCommand= _pSubTopicState= _pSubTopicAnnounce= _pSubTopicStatus= NULL;
   _Enabled= true;
   _pSubTopicEntity= pSubTopicEntity;                 // e.g. "myswitch"
   _pSubTopicCommand= QMQTT_Entity::_pSetSubTopic;    // e.g. "set"

   _pStateReportingTimer= new QTimer(_StateReportingSec */*msec*/1000,/*Repeat*/true,/*Start*/false, /*Done*/true);
   _pStateCheckTimer= NULL;

   /* Add the new object to the list of objects. Used for dispatching callbacks from mqtt to the appropriate entity. */
   if (_EntityCount < MAX_ENTITY_INSTANCES)
   {
      _EntityInstanceArr[_EntityCount]= this;
      _EntityCount++;
   }

} // Init
#ifdef _MQTT_ENTITY_DEBUG
/**************************************************************************************/
const char * QMQTT_Entity::Dump()
{
   sprintf(_TraceBfr, "QMQTT_Entity: %s", _pSubTopicEntity);
   return _TraceBfr;
   
} // Dump
#endif
/**************************************************************************************/
void QMQTT_Entity::Enable(bool Flag)
{
   _Enabled= Flag;

} // Enable
/**************************************************************************************/
/* Publishes the payload to the mqtt state topic.
   Used by sensors, e.g. to publish sensor reading.
   Inputs:  pText    payload- json string.
*/
void QMQTT_Entity::ReportJsonStr(char * pText)
{
   /* Generate the topic path for state publishing. Of the form EntityPath/ThisEntityName */
   char pStateTopic[MQTT_TOPIC_LEN+1];
   sprintf(pStateTopic, "%s/%s", _pEntitiesTopic, _pSubTopicEntity);

   bool Result= QMQTT_Entity::_pMQTT->Publish(/*Topic*/pStateTopic, /*Payload*/pText);

} // ReportJsonStr
/**************************************************************************************/
/* Reports state to mqtt in the form of on|off. This is to the state topic expected by 
   HomeAssistant in response to a command, i.e. the ack.
   Payload:
   {
     "state":"on",
   }
   state: on|off
*/
void QMQTT_Entity::ReportStateOnOff(bool State)
{
   char * pJsonTextPayload= QMQTT_Entity::GetJsonStr(_pStateCommand, (State)?("on"):("off"));

   ReportJsonStr(pJsonTextPayload);

} // ReportStateOnOff


/**************************************************************************************/
// QMQTT_Entity_Switch
/**************************************************************************************/
QMQTT_Entity_Switch::QMQTT_Entity_Switch(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow) : QMQTT_Entity(pSubTopicEntity, IOType, Address, ActiveLow)
{
   Init();

   QMQTT_Entity::ReportStateOnOff(/*State*/false);    // Notify of initial state of 'off'

} // QMQTT_Entity_Switch
/**************************************************************************************/
void QMQTT_Entity_Switch::Init()
{
   _MaxOnTimeSec= QMQTT_Entity::_MaxOnTimeSecDflt;
   _pMaxOnTimer= new QTimer(_MaxOnTimeSec */*msec*/1000,/*Repeat*/false,/*Start*/false);
} // Init
/**************************************************************************************/
void QMQTT_Entity_Switch::DoCommand(char * pMessage)
/* Callback from mqtt server on command channel.

   Inputs:  pMessage       json payload   e.g. {"state":"off"}
*/
{
   _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity_Switch::DoCommand(): %s, Received:[%s]", _pSubTopicEntity, pMessage);

   /*  Process the mqtt message. TBD - JUST MOVE THE CODE IN ParseMessage here. */
   if (!ParseMessage(pMessage))
   {
      #ifdef _MQTT_ENTITY_DEBUG
      _Trace.printf(TS_SERVICES, TLT_Error, "Callback(): json error [%s]", pMessage);
      #endif
   }
  
} // DoCommand
/**************************************************************************************/
bool QMQTT_Entity_Switch::ParseMessage(char * pMessage)
/* Process json message received via MQTT.
   Commands: On | Off
      On - turns on specified switch (e.g. light)
      Off - turns off specified switch
      Test
   Station Number: 1..8    Corresponds to relay location
   Run Time:   Run time in seconds
   Inputs
      pMessage  raw json. 
         {"state":"on"}

   Returns: true if message format is ok.

   Called by: MQTT_Subscribe_Callback()
*/
{
   bool Result= true;
   StaticJsonBuffer<JSON_OBJECT_SIZE(32)> jsonBuffer;
  
   // Parse the raw json. Creates key/value pairs.
   JsonObject& JsonRoot= jsonBuffer.parseObject(pMessage);
   
   if (!JsonRoot.success())
   {
      #ifdef _MQTT_ENTITY_DEBUG
      _Trace.printf(TS_SERVICES, TLT_Error, "QMQTT_Entity_Switch::ParseMessage(): json error [%s]", pMessage);
      #endif
      Result= false;
   }
   else
   {  // Process the message
      if (JsonRoot.containsKey(QMQTT_Entity::_pStateCommand))
      {  // Command
         const char * pCmd= JsonRoot[QMQTT_Entity::_pStateCommand];
         if (strcmp(JsonRoot[QMQTT_Entity::_pStateCommand], QMQTT_Entity::_pStateOn) == 0)
         {  /* Received "on" command. */
            SetState(true);
         }
         else if (strcmp(JsonRoot[QMQTT_Entity::_pStateCommand], QMQTT_Entity::_pStateOff) == 0)
         {  /* Received "off" command. */
            SetState(false);
         }
         #ifdef DISABLED
         else if (strcmp(JsonRoot[pJsonCommandHdr], pJsonCommandTest) == 0)
         {  /* Received "Test" command. */
            int RunTimeSec=   JsonRoot[pJsonRunTimeSec];
            _pQStationMgr->Test(/*RunTime (sec)*/RunTimeSec);
         }
         #endif
         else
         {  // unknown command
            #ifdef _MQTT_ENTITY_DEBUG
            _Trace.printf(TS_SERVICES, TLT_Error, "QMQTT_Entity_Switch::ParseMessage(): unknown cmd [%s]", pCmd);
            #endif
            Result= false;
         }
      }
   }
   
   return Result;

} // ParseMessage
/**************************************************************************************/
void QMQTT_Entity_Switch::CheckEntity()
/* Called by master QMQTT_Entity::DoService(). */
{
   /* Check Max On Timer. Note that it doesn't matter what the current state is. */
   if (_pMaxOnTimer->IsDone())
      SetState(false);

   if (_pStateReportingTimer->IsDone())
      Report();

} // CheckEntity
/**************************************************************************************/
bool QMQTT_Entity_Switch::GetState()
{
   if (_IOType == IOT_SHIFT_REGISTER)
   {
      uint32_t StationBits= _pShiftRegister->Get();
      int StationId= _Address;
      uint32_t StationBit= StationBits & (1 << StationId);

      // Convert it to logical value
      bool StationState;
      if (_ActiveLow)
         StationState= (StationBit == 0);
      else
         StationState= (StationBit != 0);

      bool SavedState= (_State == 0)?(false):(true);
      ASSERT_MSG(StationState == SavedState, "QMQTT_Entity_Switch::GetState(): shift register bit state inconsistent."); 

      _State= (StationState)?(1):(0);                 // Update it to what we see in shift register.
   }

   return _State;

} // GetState
/**************************************************************************************/
void QMQTT_Entity_Switch::SetState(bool State)
/* Sets the state for the switch.
   Sets the shift register output directly based on this entity's position in the register (_Address)
   and accounting for ActiveLow or not.
   Note! Does not change the state of any of the other shift register outputs, they
   are retained.
   Creates the bit array required for the shift register output, reading the current state
   of the register.
   Reports updated state to the configured mqtt state channel.
   
   Inputs:  State    The logical value. true= on, false= off. 
*/
{
   #ifdef _MQTT_ENTITY_DEBUG_DISABLED
   _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity_Switch::Entry(): addr:%d, state:%d", _Address, (State)?(1):(0)); 
   #endif

   _State= (State)?(1):(0);                        // State retained is logical value (exclusive of ActiveLow)

   uint32_t StateBit;
   if (_ActiveLow)
      StateBit= (State)?(0):(1);
   else
      StateBit= (State)?(1):(0);

   if (_IOType == IOT_SHIFT_REGISTER)
   {
      uint32_t CurStationBits= _pShiftRegister->Get();
      int StationId= _Address;
      uint32_t Mask= 0xFFFFFFFF ^ (1 << StationId);               // Set bit=zero at the station position
      uint32_t NewStationBits= CurStationBits & Mask;             // Clear this station bit (zero, regardless of active low or not)
      NewStationBits= NewStationBits | (StateBit << StationId);   // Set the bit for this station
      #ifdef _MQTT_ENTITY_DEBUG_DISABLED
      _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity_Switch::SetState(): Cur:%04x, Mask:%04x, New:%04x", 
         CurStationBits, Mask, NewStationBits);
      #endif

      QMQTT_Entity::_pShiftRegister->Set(/*bits*/NewStationBits);
   }
   else if (_IOType == IOT_GPIO)
   {
      //_State= (State)?(1):(0);                        // State retained is logical value (exclusive of ActiveLow)
      digitalWrite(_Address, (StateBit == 1)?(HIGH):(LOW));
      #ifdef _MQTT_ENTITY_DEBUG
      _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity_Switch::SetState(): %d", StateBit); 
      #endif
   }

   /* Ack state info to state topic. Do this regardless of whether state changed in case of
      multiple masters or master is out of sync. */
   ReportStateOnOff(State);                               
   _pStateReportingTimer->Restart();

   /* Update the watchdog timer. */
   if (State)
      _pMaxOnTimer->Start();
   else
      _pMaxOnTimer->Stop();                           // Kill the timer since we've turned the switch off

} // SetState
/**************************************************************************************/
void QMQTT_Entity_Switch::Report()
{
   ReportStateOnOff(GetState());                               

} // Report


/**************************************************************************************/
// QMQTT_Entity_Sensor
/**************************************************************************************/
QMQTT_Entity_Sensor::QMQTT_Entity_Sensor(const char * pSubTopicEntity) : QMQTT_Entity(pSubTopicEntity)
{
   Init();

} // QMQTT_Entity_Sensor
/**************************************************************************************/
QMQTT_Entity_Sensor::QMQTT_Entity_Sensor(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow)  : QMQTT_Entity(pSubTopicEntity, IOType, Address, ActiveLow)
{
   Init();

} // QMQTT_Entity_Sensor
/**************************************************************************************/
void QMQTT_Entity_Sensor::Init()
{
} // Init
/**************************************************************************************/
void QMQTT_Entity_Sensor::CheckEntity()
{
   // Check whether it's time to read the sensor.
   if ((_pStateCheckTimer != NULL) && _pStateCheckTimer->IsDone())
   {  // A separate state check timer is defined for this sensor, use it.
      ReadSensor();
   }

   // Check whether it's time to report the sensor value.
   if (_pStateReportingTimer->IsDone())
   {  // Time to report state
      if (_pStateCheckTimer == NULL)
         ReadSensor();

      Report();
   }

} // CheckEntity
/**************************************************************************************/
void QMQTT_Entity_Sensor::DoCommand(char * pMessage)
/* Callback from mqtt server on command channel. We ignore any command input for sensors. */
{
} // DoCommand
/**************************************************************************************/
/*virtual*/ void QMQTT_Entity_Sensor::Report()
{
   ReportStateOnOff();                               
   _pStateReportingTimer->Restart();
} // Report

/**************************************************************************************/
// QMQTT_Entity_Temperature_Sensor
/**************************************************************************************/
QMQTT_Entity_Temperature_Sensor::QMQTT_Entity_Temperature_Sensor(const char * pSubTopicEntity, QTemperature::TemperatureSensorT SensorType, int Address) : QMQTT_Entity_Sensor(pSubTopicEntity)
{
   Init();
   _pTemperatureSensor= new QTemperature(SensorType, Address);

} // QMQTT_Entity_Temperature_Sensor
/**************************************************************************************/
void QMQTT_Entity_Temperature_Sensor::Init()
{
   _TemperatureF= QTemperature::_MinValidTemperatureF - 1;
} // Init
/**************************************************************************************/
void QMQTT_Entity_Temperature_Sensor::ReadSensor()
/* 
*/
{
   _TemperatureF= _pTemperatureSensor->GetTemperatureF();
   #ifdef _MQTT_ENTITY_DEBUG_DISABLED
   _Trace.printf(TS_SERVICES, TLT_Verbose, "QMQTT_Entity_Temperature_Sensor::ReadSensor(): %.1f", _TemperatureF);
   #endif

} // ReadSensor
/**************************************************************************************/
void QMQTT_Entity_Temperature_Sensor::Report()
{
   if (_TemperatureF >= QTemperature::_MinValidTemperatureF)
   {
      // Publish the payload
      char TemperatureStr[15+1];
      sprintf(TemperatureStr, "%0.1f", _TemperatureF);
      char * pJsonTextPayload= QMQTT_Entity::GetJsonStr("temperature", TemperatureStr);
      ReportJsonStr(pJsonTextPayload);
   }
} // Report


/**************************************************************************************/
// QMQTT_Entity_Binary_Sensor
/**************************************************************************************/
QMQTT_Entity_Binary_Sensor::QMQTT_Entity_Binary_Sensor(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow) : QMQTT_Entity_Sensor(pSubTopicEntity, IOType, Address, ActiveLow)
{
   Init();

} // QMQTT_Entity_Temperature_Sensor
/**************************************************************************************/
void QMQTT_Entity_Binary_Sensor::Init()
{
   _pReadSensorCallback= NULL;
   _pStateCheckTimer= new QTimer(_StateCheckSec */*msec*/1000, /*Repeat*/true, /*Start*/false, /*Done*/true);
} // Init
/**************************************************************************************/
void QMQTT_Entity_Binary_Sensor::SetReadSensorCallback(ReadSensorCallback pReadSensorCallback)
{
   _pReadSensorCallback= pReadSensorCallback;
} // SetReadSensorCallback
/**************************************************************************************/
void QMQTT_Entity_Binary_Sensor::ReadSensor()
/* 
*/
{
   bool State= digitalRead(_Address);                 // HIGH / 1 / true
   if (_ActiveLow)
      State= !State;

   /* Optional callback here to process the result. e.g. LPF, debounce.
      Overrides read of state above.            */
   if (_pReadSensorCallback != NULL)
      State= this->_pReadSensorCallback(_Id, State); 

   /* Check for change in state, in which case we report immediately. */
   bool StateChange= (State != _State);
   _State= State;
   if (StateChange)
      Report();

} // ReadSensor








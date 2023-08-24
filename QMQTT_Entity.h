///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/* QMQTT_Entity.h       
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef QMQTT_Entity_h
#define QMQTT_Entity_h
#include "QShiftRegister.h"
#include "QTimer.h"
#include "QMQTT.h"

#define  _MQTT_ENTITY_DEBUG                           // Enables trace dump
#define  MAX_ENTITY_INSTANCES          8
#define  MAX_JSON_PAYLOAD_STR          127            // Max resultant json payload string

/**************************************************************************************/
/* QMQTT_Entity - Base class for MQTT Entities for use with HomeAssistant.
   These items are connected to end devices and can operate as:
   - actuators: subscribe to a topic to listen for commands.
   - sensors: publish readings to a topic. Controlled by parent. 

   This class manages mqtt topics for announcing device presence, publishing state.

   Actuators:
   Subscribing to the command topic is typically handled by subclassed children.
   This class is used by devices that are controlling items via an external shift register
   or gpio. The end devices are abstracted and can be e.g. relays, triacs, or digital devices.

   Sensors: see QMQTT_Entity_Sensor

   Note that this base class is not instantiated directly, but user creates child switch and
   sensor classes.
*/   
/**************************************************************************************/
class QMQTT_Entity
{
   public:
   /* IO Control Type. Currently supports GPIO, QShiftRegister. */
   typedef enum EIOT
   {
      IOT_GPIO=             0,      // GPIO pin
      IOT_SHIFT_REGISTER,           // Shift Register
      IOT_I2C,                      // I2C Device
      IOT_Count
   };

   typedef int16_t         EntityStateT;

   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:
   /* Terms used in json payloads.  */
   static constexpr char * _pSetSubTopic=       "cmd";
   static constexpr char * _pAnnounceSubTopic=  "config";
   static constexpr char * _pAvailabilitySubTopic= "status";

   static constexpr char * _pStateCommand=      "state";
   static constexpr char * _pStateOn=           "on";
   static constexpr char * _pStateOff=          "off";

   static constexpr char * _pAvailability_Online= "online";


   protected:
   /* Static Data          */
   static int              _EntityCount;

   /* Array of entities.   */
   static QMQTT_Entity *   _EntityInstanceArr[MAX_ENTITY_INSTANCES];

   static QWifi *          _pWifi;           // shared wifi object

   /* Sets frequency of CheckEntity() call on each entity.    */
   static QTimer *         _pServiceTimer;   

   //////// MQTT
   /* MQTT - subscribe, publish control */
   static QMQTT *          _pMQTT;

   /* mqtt subscribe topics must be static/global
      e.g. device/lighting_back/entities/#         */
   static char             _pSubscribeTopic[MQTT_TOPIC_LEN+1];

   /* Availability timer for periodic publish of availability= online|offline. */
   static const int        _AvailabilityReportingSec= /*min*/5 * /*sec*/60;
   static QTimer *         _pAvailabilityTimer;

   /* Frequency of state reporting for sensors, switches. Sets default value of StateReportingTimer. */
   static const int        _StateReportingSec=        /*min*/6 * /*sec*/60;

   /* Frequency of sensor check. This sets the latency detecting a change in sensor value.  */
   static const int        _StateCheckSec=            /*min*/1 * /*sec*/1;

   /* Full path to entities topic.     
      e.g. device/lighting_back         */
   static char             _pEntitiesTopic[MQTT_TOPIC_LEN+1];


   static char             _pJsonPayloadStr[MAX_JSON_PAYLOAD_STR+1];


   static const int        _MaxOnTimeSecDflt=   /*hrs*/6 * /*min*/60 * /*sec*/60;

   static QShiftRegister * _pShiftRegister;
   #ifdef _MQTT_ENTITY_DEBUG
   static char             _TraceBfr[127+1];
   #endif


   /* Instance Data        */
   protected:
   uint8_t                 _Id;
   
   //////// IO Configuration
   /* Common state info shared by binary devices (switches, sensors).   */
   EIOT                    _IOType;

   /* (optional) Address for IO. This may be:
      - Bit position for the I/O controlled by this entity. 0..N-1. 
      - GPIO address, e.g. (D0) 
      - I2C address.                                  */
   int                     _Address;

   /* Applicable to binary devices (switches, binary sensors). */
   bool                    _ActiveLow;

   /* State of entity. Defined as Int16 to support a range of sensor types, including
      binary (on/off).
      Contains the logical value, i.e. does not account for any ActiveLow setting,
      which is a function of the underlying device.      */
   EntityStateT            _State;

   //////// Topic Configuration
   /* Entity subtopic. This is essentially the name of the entity as seen
      under the entities hierarchy. e.g. foo will reside in device/entities/foo   */
   const char *            _pSubTopicEntity;

   /* Command topic where commands are received. If not defined, then defaults to the
      subtopic "set". Note that this is not the full path
      This is the topic that subscribes to the mqtt server. */
   char *                  _pSubTopicCommand;

   /* Optional subtopic to publish state information. If not defined,
      then state is published to root. */
   char *                  _pSubTopicState;

   /* Optional subtopic to publish announcement configuration information for 
      automatic detection by HomeAssistant. If not defined, then no announcement
      is published
      (not implemented)                                     */
   char *                  _pSubTopicAnnounce;

   /* Optional subtopic for publishing a heartbeat. Used by HomeAssistant to determine
      whether entity is online|offline. 
      e.g. "status"                                      */
   char *                  _pSubTopicStatus;

   /* Entity enable/disable control. If disabled, this entity does not respond at all
      and appears offline. */
   bool                    _Enabled;

   /* Availability timer for period publish of availability= online|offline. */
   //QTimer *                _pAvailabilityTimer;

   /* Sets the period for checking the entity state, e.g. how often to read sensor value.
      By default, this is not defined, and state check is done at the time of reporting. Entities
      can override this, typically to check state more frequently, e.g. for averaging, etc.  */
   QTimer *                _pStateCheckTimer;

   /* Sets the period for entity state reporting to mqtt. */
   QTimer *                _pStateReportingTimer;

   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   /* Static Methods */
   public:
   /* Global initialization. Call this prior to creating any entity objects.
      Note that wifi and QMQTT must be set up beforehand.
   */
   static void             Initialize();
   static void             Initialize(QShiftRegister * pShiftRegister);
   static void             ReportAvailability();
   static void             DoService();

   static void             MQTT_Callback(char * pSubTopicEntity, byte * pPayload, unsigned int PayloadLength);
   static char *           GetJsonStr(const char * pKey, const char * pValue);

   /* Instance Methods */
   public:
   /* Note that any use of a QShiftRegister must be created by parent, as it has awareness of control pin assignments.
      Address -   0..n-1
   */
                           QMQTT_Entity(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow); 
                           QMQTT_Entity(const char * pSubTopicEntity);

   #ifdef _MQTT_ENTITY_DEBUG
   public:
   const char *            Dump();
   #endif

   bool                    IsEnabled(){return _Enabled;};
   void                    Enable(bool Flag);

   protected:
   void                    Init(EIOT IOType, int Address, bool ActiveLow, const char * pSubTopicEntity);
   virtual void            DoCommand(char * pMessage)= 0;

   /* Service each entity - for entities that need periodic checks/calls, e.g. to read state
            sensor value, etc. 
      Called by DoService(), with period set by ServiceTimer. 
      ServiceTimer is frequent, and entities decide how frequently to check their own state and
      reporting.                          */
   virtual void            CheckEntity()= 0;

   /* Publishes entity availability state periodically. */
   //virtual void            ReportAvailability();

   /* Report value to mqtt (state, sensor value, etc.).   */
   virtual void            Report()= 0;

   /* Report state helpers that can be used by entities for common needs. */
   void                    ReportStateOnOff(bool State);
   void                    ReportStateOnOff(){ReportStateOnOff(_State);};

   public:
   void                    ReportJsonStr(char * pJsonText);  

}; // QMQTT_Entity

/**************************************************************************************/
/* QMQTT_Entity_Switch - controls a simple on/off switch.
   Subclass of QMQTT_Entity.
   Additional functionality for:
      - setting an on/off switch. e.g. relay, digital output
      - awareness of active high or low signal. Relays are typically active low.
      - watchdog timer to turn off the switch after a maximum amount of time. Useful
        for devices that can fall offline.
*/   
/**************************************************************************************/
class QMQTT_Entity_Switch : public QMQTT_Entity
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   /* Watchdog to ensure that switch on not on for a specified maximum amount of time. */
   int                     _MaxOnTimeSec;
   QTimer *                _pMaxOnTimer;
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   /* Note that pShiftRegister must be initialized by parent, as it has awareness of pin assignments. */ 
                           QMQTT_Entity_Switch(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow);

   void                    ActiveLowControl(bool Flag);
   bool                    GetState();
   void                    SetState(bool State);


   protected:
   void                    Init();
   void                    DoCommand(char * pMessage);
   bool                    ParseMessage(char * pMessage);

   void                    CheckEntity();
   virtual void            Report();

}; // QMQTT_Entity_Switch

/**************************************************************************************/
/* QMQTT_Entity_Sensor - virtual class for sensors, including binary, digital, analog.
   Subclass of QMQTT_Entity.
   This is a virtual class, must be subclassed in order to specify ReadSensor().

   Additional functionality for:
      - periodically reading the sensor value via virtual ReadSensor().
      - an optional reporting callback can be assigned for custom payloads.

*/   
/**************************************************************************************/
class QMQTT_Entity_Sensor : public QMQTT_Entity
{
   ///////////////////////////////////////////////////////////
   // Callback Signatures
   ///////////////////////////////////////////////////////////
   public:
   /* Signature for optional callback to read sensor. 
      pointer to a function that has input int and returns an int.       */
   typedef int (* ReadSensorCallback)(int Id, bool State);

   /* Signature for optional callback for reporting sensor value. Allows override of
      the default reporting method.    */
   typedef void (* ReportSensorCallback)(int Id, bool State);

   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   ReadSensorCallback      _pReadSensorCallback;
   ReportSensorCallback    _pReportSensorCallback;


   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QMQTT_Entity_Sensor(const char * pSubTopicEntity);
                           QMQTT_Entity_Sensor(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow);

   void                    SetReadSensorCallback(ReadSensorCallback pReadSensorCallback);
   void                    SetReportSensorCallback(ReportSensorCallback pReportSensorCallback);

   protected:
   void                    Init();
   void                    DoCommand(char * pMessage);

   void                    CheckEntity();

   /* Read the sensor state/value and report to mqtt. */
   virtual void            ReadSensor()= 0;

   /* Helper for child classes post read of sensor to handle optional callback,
      detection of state change and consequent immediate reporting. */
   void                    ReadSensorHandler(EntityStateT SensorState);


   /* Default reporting method that can be used by subclasses. Assumes entity is binary sensor
      and reports _State as on|off.
      If ReportSensorCallback is specified, this is used instead. */
   virtual void            Report();

}; // QMQTT_Entity_Sensor

/**************************************************************************************/
/* QMQTT_Entity_Temperature_Sensor - entity wrapper for QTemperature.                 */   
/**************************************************************************************/
#include "QTemperature.h"

class QMQTT_Entity_Temperature_Sensor : public QMQTT_Entity_Sensor
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   QTemperature *          _pTemperatureSensor;
   float                   _TemperatureF;

   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   /* Note that pShiftRegister must be initialized by parent, as it has awareness of pin assignments. */ 
                           QMQTT_Entity_Temperature_Sensor(const char * pSubTopicEntity,  QTemperature::TemperatureSensorT SensorType, int Address);

   protected:
   void                    Init();
   void                    ReadSensor();
   void                    Report();  

}; // QMQTT_Entity_Temperature_Sensor


/**************************************************************************************/
/* QMQTT_Entity_Binary_Sensor - entity wrapper for binary sensors.                    */   
/**************************************************************************************/
class QMQTT_Entity_Binary_Sensor : public QMQTT_Entity_Sensor
{
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QMQTT_Entity_Binary_Sensor(const char * pSubTopicEntity, EIOT IOType, int Address, bool ActiveLow);

   protected:
   void                    Init();
   void                    ReadSensor();
   
}; // QMQTT_Entity_Binary_Sensor


#endif

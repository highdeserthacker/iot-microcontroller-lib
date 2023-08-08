///////////////////////////////////////////////////////////////////////////////
// :mode=c:
///////////////////////////////////////////////////////////////////////////////
#ifndef MQTT_h
#define MQTT_h

#include <PubSubClient.h>
#include "QWifi.h"
#include "QTimer.h"

#define  _DEBUG_MQTT                                  // QMQTT - Outputs additional trace info to Serial
#define MQTT_TOPIC_LEN          63

/* Signature of mqtt callback. */
#if defined(ESP8266) || defined(ESP32)
#include <functional>
//#define MQTT_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, unsigned int)> callback
typedef std::function<void(char*, uint8_t*, unsigned int)> pMQTTCallback;
#else
//#define MQTT_CALLBACK_SIGNATURE void (*callback)(char*, uint8_t*, unsigned int)
typedef void (* pMQTTCallback)(char*, uint8_t*, unsigned int);
#endif


/**************************************************************************************/
/* Handles MQTT interface
   Subclassed from PubSubClient library. Enhancements:
   - timer to limit connection checks
   - automatic reconnect and resubscribe to topic.

   It starts off unconnected. Will auto-connect on Publish or CheckMessages().
   Note- Each instance supports only 1 subscribe topic.

   PubSubClient limitations & bugs
      - Cannot have more than 1 callback for subscribes. The callback handles all subscribed 
        messages. So one can subscribe to more than 1 topic but cannot have separate callbacks,
        even with separate instances. 
        Oddly, it calls back only the first one set when using multiple instances.
*/   
/**************************************************************************************/
class QMQTT : public PubSubClient
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   public:
   static constexpr char * _pTraceSubTopic=  "trace"; 

   protected:
   /* Static Data          */
   static QMQTT *          _pMasterObject;

   // TBD - since this is now a subtopic of each entity, it should be non-static
   static const char *     _pTraceTopic;

   // Credential info.
   static const uint16_t   _DfltPort= 1883;

   static const int        _MaxSubscribers= 3;
   static int              _SubscriberCnt;

   /* Can contain wildcard suffix, e.g. "device/mydevice/#" */
   static const char *     _pSubscriberTopics[_MaxSubscribers];

   //static char             _pSubscribeTopic[_MaxSubscribers][MQTT_TOPIC_LEN+1];
   static pMQTTCallback    _pSubscriberCallbacks[_MaxSubscribers];

   static const int        _DumpBfrLen= 95;
   
   //////// Instance Data ////////
   const char *            _pIPAddress;
   uint16_t                _Port;
   const char *            _pUserName;
   const char *            _pPassword;

   QTimer *                _pConnectionStatusTimer;         // controls freq of attempts to re-establish connection 
   QTimer *                _pMessageStatusTimer;            // controls freq of message checking

   /* Name for this client of the mqtt server. e.g. device name.
      Must be unique across all entities. */
   const char *            _pIdentifier;                    

   /* Default publish channel. */
   const char *            _pPublishTopic;

   /* Used for Dump()               */
   char                    _StsBfr[_DumpBfrLen+1];


   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
   static QMQTT *          Master(){return _pMasterObject;}

   /* Topic to publish trace callbacks to. */
   static void             SetTraceTopic(const char * pTraceTopic){_pTraceTopic= pTraceTopic;}
   /* Callback for redirecting Trace output to mqtt channel on the specified topic.
      This allows trace callback to be embedded within QMQTT class instead of declaring
      it in every single app. Useful for apps that only need trace output on a single channel.    */
   static void             TraceCallback(const char * pPayload);
   protected:
   static void             Dispatch_Callback(char * pTopic, byte * pPayload, unsigned int PayloadLength);

   public:
                           QMQTT(const char * pIPAddress, const char * pIdentifier);
                           QMQTT(const char * pIPAddress, const char * pIdentifier, QWifi * pWifi);
                           QMQTT(const char * pIPAddress, const char * pIdentifier, const char * pPublishChannel, QWifi * pWifi);

   void                    SetIdentifier(const char * pIdentifier);
   const char *            GetIdentifier(){return _pIdentifier;};
   const char *            Dump();
   bool                    IsConnected();
   void                    DoService(); 

   /* Subscribe to a channel / topic. 
      pChannelName - Note! must be static! We don't make a copy.

      Note - subscribe after mqtt server is ready (IsConnected()). Any subsequent loss of connection
         and it will subscribe/re-subscribe as needed.   */
   void                    Subscribe(const char * pTopic, MQTT_CALLBACK_SIGNATURE);

   /* Publish related. */
   void                    SetPublishTopic(const char * pTopic);

   /* Deprecated - old terminology. */
   void                    SetPublishChannel(const char * pChannelName){SetPublishTopic(pChannelName);}

   /* Publish a message to an mqtt channel/topic
      pChannelName:  e.g. device/DeviceName
      Note that PubSubClient limits the topic payload to just MQTT_MAX_PACKET_SIZE. 
      This includes the topic/channel name too.
      So ChannelName + Payload + mqtt header (5) + 2 < MQTT_MAX_PACKET_SIZE
      It will toss it if exceeded.       
   */
   bool                    Publish(const char * pTopic, const char * pPayload, bool RetainMsg);
   bool                    Publish(const char * pTopic, const char * pPayload);
   bool                    Publish(const char * pPayload);

   protected:
   void                    Init();
   void                    Connect();
   void                    CheckConnection();
   void                    Resubscribe(); 
   
}; // QMQTT

#endif

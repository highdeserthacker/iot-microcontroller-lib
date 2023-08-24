///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  MQTT.cpp - 
*/
///////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include "QMQTT.h"
#include "QString.h"

/* Library has a poorly conceived default size, modified it. This check is to make sure it doesn't   
   get blown away on an update. */
#if MQTT_MAX_PACKET_SIZE < 256
#error "PubSubClient.h MQTT_MAX_PACKET_SIZE must be overridden to >= 256" 
#endif

/*
#if defined(ESP8266) || defined(ESP32)
#error "PubSubClient.h MQTT_CALLBACK_SIGNATURE unexpected" 
#endif   */

/**************************************************************************************/
// QMQTT - Static Member Initialization
/**************************************************************************************/
QMQTT *           QMQTT::_pMasterObject= NULL;
const char *      QMQTT::_pTraceTopic= NULL;
int               QMQTT::_SubscriberCnt= 0;
const char *      QMQTT::_pSubscriberTopics[QMQTT::_MaxSubscribers];
pMQTTCallback     QMQTT::_pSubscriberCallbacks[QMQTT::_MaxSubscribers];


/**************************************************************************************/
/* Callback from QTrace to publish trace data to mqtt channel.
   Hack. Refer to https://isocpp.org/wiki/faq/pointers-to-members  */
/*static*/ void QMQTT::TraceCallback(const char * pPayload)
{
   if ((_pMasterObject != NULL) && (_pMasterObject->_pTraceTopic != NULL))
   {
      _pMasterObject->Publish(/*Channel*/_pTraceTopic, /*Payload*/pPayload, /*RetainMsg*/false);
   }

} // TraceCallback
/**************************************************************************************/
/*static*/ void QMQTT::Dispatch_Callback(char * pTopic, byte * pPayload, unsigned int PayloadLength)
/* Callback from mqtt server. This method serves as a dispatcher, finding the end
   callback that this topic belngs to.

   Inputs:  pTopic
            pPayload       - raw byte format. For this app it is json.
            PayloadLength

   Note - Trace output can be directed here, so do not use Trace statements, else creates
          re-entrant problems.
*/
{
   #ifdef _DEBUG_MQTT
   Serial.printf("QMQTT::Dispatch_Callback(): Entry, Topic:[%s]\n", pTopic);
   #endif

   /* Filter out Trace callbacks. Of the form .../trace */
   int TraceSuffixIndex= strlen(pTopic) - strlen(QMQTT::_pTraceSubTopic);
   if (strcmp(QMQTT::_pTraceSubTopic, &pTopic[TraceSuffixIndex]) == 0)
   {  /* This topic ends in trace, skip dispatch processing. */
      #ifdef _DEBUG_MQTT
      Serial.printf("QMQTT::Dispatch_Callback(): Filtering trace callback, Topic:[%s]\n", pTopic);
      #endif

      return;
   }

   /* Trace statements should be safe as of here. */
   for (int i= 0 ; i < _SubscriberCnt ; i++)
   {
      /* Find the subscriber callback that this topic belongs to. 
         Note that there can be multiple subscribers to the same topic. */
      const char * pSubscriberTopic= _pSubscriberTopics[i];

      /* If wildcard present in the subscriber, process up to that point in the string. */
      char WildcardCh= '#';
      int FindIndex= FindCh(pSubscriberTopic, WildcardCh);
      if (FindIndex >= 0)
      {
         /* Identify if of the form "...xyz/#" */
         if ( (FindIndex > 0) && (pSubscriberTopic[FindIndex-1] == '/'))
            FindIndex--;

         int Result= strncmp(pTopic, pSubscriberTopic, FindIndex); // also excludes any trailing '/'
         if (Result == 0)
         {
            #ifdef _DEBUG_MQTT
            Serial.printf("QMQTT::Dispatch_Callback(): Wildcard Match, Subscriber:%d, Topic:[%s], SubscriberTopic:[%s]\n",
               i, pTopic, pSubscriberTopic);
            #endif
            _pSubscriberCallbacks[i](pTopic, pPayload, PayloadLength);
         }
      }
      else
      {  /* Exact topic match only. */
         if (strcmp(pTopic, _pSubscriberTopics[i]) == 0)
         {
            #ifdef _DEBUG_MQTT
            Serial.printf("QMQTT::Dispatch_Callback(): Match, Subscriber:%d, Topic:[%s]\n", i, pTopic);
            #endif
            _pSubscriberCallbacks[i](pTopic, pPayload, PayloadLength);
         }
      }
   }

} // Dispatch_Callback


/**************************************************************************************/
// QMQTT
/**************************************************************************************/
QMQTT::QMQTT(const char * pIPAddress, const char * pIdentifier)
{
   Init();
   this->setClient(*QWifi::Master()->GetWifiClient());
   _pIPAddress= pIPAddress;
   SetIdentifier(pIdentifier);
   this->setServer(_pIPAddress, _Port);   
   
} // QMQTT
/**************************************************************************************/
QMQTT::QMQTT(const char * pIPAddress, const char * pIdentifier, QWifi * pWifi) : PubSubClient{ *pWifi->GetWifiClient() }
{
   Init();
   _pIPAddress= pIPAddress;
   SetIdentifier(pIdentifier);
   this->setServer(_pIPAddress, _Port);   
   
} // QMQTT
/**************************************************************************************/
QMQTT::QMQTT(const char * pIPAddress, const char * pIdentifier, const char * pPublishChannel, QWifi * pWifi) : PubSubClient{ *pWifi->GetWifiClient() }
{
   Init();
   _pIPAddress= pIPAddress;
   SetIdentifier(pIdentifier);
   this->setServer(_pIPAddress, _Port);   
   SetPublishChannel(pPublishChannel);
   
} // QMQTT
/**************************************************************************************/
void QMQTT::Init()
{
   if (QMQTT::_pMasterObject == NULL)
      _pMasterObject= this;

   _pIPAddress= NULL;
   _Port= _DfltPort;
   _pUserName= _pPassword= "";                  // Default is empty string (no username or password)
   _pConnectionStatusTimer=   new QTimer(/*sec*/60/*msec*/*1000,/*Repeat*/true,/*Start*/false,/*Done*/true); // Start in Done state   
   _pMessageStatusTimer=      new QTimer(/*msec*/500,/*Repeat*/true,/*Start*/true,/*Done*/false);
   
   _pIdentifier= NULL;
   _pPublishTopic= NULL;
   //_pConnectionStatusTimer->Enable();
   
} // Init
/**************************************************************************************/
const char * QMQTT::Dump()
{
   //sprintf(_StsBfr, "MQTT: Connected:%s", (this->IsConnected()?("true"):("false")));
   /*
   sprintf(_StsBfr, "QMQTT: Connected:%s, SubscribedTo:%s", 
      (this->IsConnected()?("true"):("false")),
      (_pSubscribeTopic != NULL)?(_pSubscribeTopic):("none")
      ); */

   snprintf(_StsBfr, _DumpBfrLen, "QMQTT: %s Connected:%s, SubscriberCnt:%d", 
      _pIdentifier,
      (this->IsConnected()?("true"):("false")),
      _SubscriberCnt
      );

   return _StsBfr;
   
} // Dump
/**************************************************************************************/
void QMQTT::SetIdentifier(const char * pIdentifier)
{
   _pIdentifier= pIdentifier;
   
} // SetIdentifier
/**************************************************************************************/
bool QMQTT::IsConnected()
{
   return connected();
} // IsConnected
/**************************************************************************************/
void QMQTT::DoService()
{
   // TBD - BAIL IF QWIFI IS NOT CONNECTED?
   CheckConnection();

   // PubSub handler.
   if (_pMessageStatusTimer->IsDone())
   {
      // Process MQTT messages, issue Keep Alive
      loop();             // PubSubClient - note that if we're not connected, this returns immediately with false
   }

} // DoService
/**************************************************************************************/
void QMQTT::CheckConnection()
{
   // TBD - SHOULDN'T THIS ONLY BE IF QWIFI IS CONNECTED?
   /* TBD: DETECT DISCONNECTED STATE, INCL AS A RESULT OF BROKER GOING OFFLINE & ONLINE
      Use client.state()      */
   if (!connected())
   {  // Not connected to mqtt server, reconnect
      Connect();
   }

} // CheckConnection
/**************************************************************************************/
void QMQTT::Connect() 
/* Connect/Reconnect to MQTT server. Will try once and return. */
{
   if (_pConnectionStatusTimer->IsDone())
   {
      #ifdef _DEBUG_MQTT
      Serial.printf("Attempting MQTT connection to %s\n", _pIPAddress);   
      #endif
      // Attempt to connect
      if (connect(_pIdentifier, _pUserName, _pPassword))
      {  // Successful connection
         #ifdef _DEBUG_MQTT  
         Serial.println("connected");
         #endif

         /* Subscribe to specified topic.
            Note that PubSub library will not perform subscribe unless we are currently connected.
            Otherwise it ignores the call. */
         Resubscribe();
      } 
      else
      {
         #ifdef _DEBUG_MQTT  
         Serial.print("Connect failed, rc=");
         Serial.print(state());
         Serial.printf("\n");   
         #endif
      }
   }
} // Connect
/**************************************************************************************/
/* Publish */
/**************************************************************************************/
void QMQTT::SetPublishTopic(const char * pTopic)
{
   _pPublishTopic= pTopic;
   
} // SetPublishTopic
/**************************************************************************************/
bool QMQTT::Publish(const char * pTopic, const char * pPayload, bool RetainMsg)
/* Publishes the message to the specified channel.
   Returns: false - if not connected to mqtt server or send error.
*/
{
   bool Result= false;

   if (pTopic != NULL)
   {
      /* The payload limit does not seem to be enforced properly.
         In PubSubClient CHECK_STRING_LENGTH it is capped to  - 
         topic length + payload length + 2 <= MQTT_MAX_PACKET_SIZE
         But it fails without returning false for topic+payload strlen within 4 bytes of MQTT_MAX_PACKET_SIZE.
      */
      if ((strlen(pTopic) + 1 + strlen(pPayload) + 1 + 4) < MQTT_MAX_PACKET_SIZE)
         Result= publish(pTopic, pPayload, RetainMsg);   // returns false on fail (e.g. payload too big)
   }
   return Result;
   
} // Publish
/**************************************************************************************/
bool QMQTT::Publish(const char * pTopic, const char * pPayload)
/* Publishes the message to the specified channel.
   Returns: false - if not connected to mqtt server or send error.
*/
{
   return Publish(pTopic, pPayload, /*RetainMsg*/false);
   
} // Publish
/**************************************************************************************/
bool QMQTT::Publish(const char * pPayload)
/* Publishes the message to the default publish topic. If topic is not defined, returns error.
   Returns: false - if not connected to mqtt server or send error.
*/
{
   return Publish(_pPublishTopic, pPayload, /*RetainMsg*/false);
   
} // Publish

/**************************************************************************************/
/* Subscribe */
/**************************************************************************************/
void QMQTT::Resubscribe()
/* Re/subscribe to our registered topics.
   Must be connected. PubSub library will not perform subscribe unless we are currently connected.
   Otherwise it ignores the call. */ 
{
   for (int i= 0 ; i < _SubscriberCnt ; i++)
      subscribe(_pSubscriberTopics[i], /*QoS*/ 1);               // re-subscribe to the channel
} // Resubscribe
/**************************************************************************************/
void QMQTT::Subscribe(const char * pTopic, MQTT_CALLBACK_SIGNATURE)
/* When the topic we're subscribed to receives a message, the given function is called.
   Of the form: (char * pTopic, byte * pPayload, unsigned int PayloadLength)  */
{
   if (_SubscriberCnt < _MaxSubscribers)
   {
      /* Once we have the first subscriber, set up the dispatcher as the callback. */
      if (_SubscriberCnt == 0)
         this->setCallback(QMQTT::Dispatch_Callback); 

      _pSubscriberTopics[_SubscriberCnt]= pTopic;
      _pSubscriberCallbacks[_SubscriberCnt]= callback;
      _SubscriberCnt++;

      /* If we're connected, perform subscribe. Otherwise, it will take place when we connect. */
      if (IsConnected())
         Resubscribe();
   }
  
} // Subscribe







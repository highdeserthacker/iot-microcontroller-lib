///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QCredentials.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QCredentials_h
#define QCredentials_h

/**************************************************************************************/
/* QCredentials - Manages login credentials needed for devices, e.g. wifi, mqtt.
*/   
/**************************************************************************************/
class QCredentials
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   static const int        _MaxUserName= 15;
   char                    _pLogin[_MaxUserName+1];
   char                    _pPassword[_MaxUserName+1];

   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QCredentials(const char * pLogin, const char * pPassword);
   
   protected:
   void                    Init();
   
}; // QCredentials


#endif

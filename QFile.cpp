///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QFile.cpp  */
///////////////////////////////////////////////////////////////////////////////
#include "QFile.h"
//#include "Arduino.h"
#include "QTrace.h"

/**************************************************************************************/
// QFile
/**************************************************************************************/
QFile *        QFile::_pMasterObject= NULL;

/**************************************************************************************/
QFile::QFile(const char * pFileName)
{
   Init();
   _pFileName= pFileName;
   
} // QFile
/**************************************************************************************/
void QFile::Init()
{
   if (QFile::_pMasterObject == NULL)
   {  // First instance initialization of static items
      _pMasterObject= this;
      // Mount File System
      #ifdef ENB_SPIFFS
      bool Result= SPIFFS.begin();                    // returns true if successful
      #else
      bool Result= LittleFS.begin();
      #endif

      if (!Result)
         _Trace.printf(TS_SERVICES, TLT_Error, "Spiffs: could not mount file system");
   }
} // Init
/**************************************************************************************/
bool QFile::Exists()
{
   #ifdef ENB_SPIFFS
   return (SPIFFS.exists(_pFileName)); 
   #else
   return (LittleFS.exists(_pFileName)); 
   #endif

} // Exists
/**************************************************************************************/
int QFile::ReadStr(char * pStrBfr, int BfrSize)
{
   int Result= 0;
   if (this->Exists())
   {
      #ifdef ENB_SPIFFS
      File f= SPIFFS.open(_pFileName, "r");
      #else
      File f= LittleFS.open(_pFileName, "r");
      #endif      
      //_Trace.printf(TS_SERVICES, TLT_Verbose, "Spiffs File Open for Read:%s", (f != NULL)?("Ok"):("Error"));

      if (f != NULL)
      {
         int i;
         for (i= 0 ; (i < f.size()) && (i < (BfrSize-1)) ; i++)
         {
            pStrBfr[i]= (char) f.read();
         }
         pStrBfr[i]= 0;
         Result= i;
         f.close();
         #ifdef _QFILE_DEBUG
         _Trace.printf(TS_SERVICES, TLT_Verbose, "QFile::ReadStr():%d, %s", Result, pStrBfr);
         #endif
      }
      else
         _Trace.printf(TS_SERVICES, TLT_Error, "QFile::ReadStr(): file read error");
   }
   return Result;

} // ReadStr
/**************************************************************************************/
int QFile::WriteStr(char * pStrBfr)
{
   int Result= 0;
   #ifdef ENB_SPIFFS
   File f= SPIFFS.open(_pFileName, "w");
   #else
   File f= LittleFS.open(_pFileName, "w");
   #endif   
   //_Trace.printf(TS_SERVICES, TLT_Verbose, "QFile::WriteStr:%s", (f != NULL)?("Ok"):("Error"));
   if (f != NULL)
   {
      f.print(pStrBfr);
      f.close();
   }
   else
      _Trace.printf(TS_SERVICES, TLT_Error, "QFile::WriteStr(): file write error");

   return Result;

} // WriteStr






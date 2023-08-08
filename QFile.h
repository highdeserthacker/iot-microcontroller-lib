///////////////////////////////////////////////////////////////////////////////
// :mode=c:
/*  QFile.h */
///////////////////////////////////////////////////////////////////////////////
#ifndef QFile_h
#define QFile_h

#include "AllApps.h"

//#define     ENB_SPIFFS
#ifdef ENB_SPIFFS
#include <FS.h>                                       // Enable SPIFFS
#else
#include "LittleFS.h" 
#endif

#define  _QFILE_DEBUG                                 // Enables trace dump

/**************************************************************************************/
/* Abstracts the file system for non-volatile storage in ESP8266 environments.
   Applications include storing of settings.
   Create an object for each file. 
*/   
/**************************************************************************************/
class QFile
{
   ///////////////////////////////////////////////////////////
   // Data
   ///////////////////////////////////////////////////////////
   protected:
   static QFile *             _pMasterObject;

   protected:
   const char *               _pFileName;             // filenames of up to 31 characters
   
   ///////////////////////////////////////////////////////////
   // Methods
   ///////////////////////////////////////////////////////////
   public:
                           QFile(const char * pFileName);

   bool                    Exists();

   /* Reads the contents of the file and returns as a null terminated string.
      Inputs:  pStrBfr     char buffer to store the file data                 
      Returns: # chars read. 0 if no or empty file.                  */
   int                     ReadStr(char * pStrBfr, int BfrSize);

   /* WriteStr(): writes the given string to the file.
      Returns: # chars written. 0 if write failure.                  */
   int                     WriteStr(char * pStrBfr);
   
   protected:
   void                    Init();
   
};

#endif

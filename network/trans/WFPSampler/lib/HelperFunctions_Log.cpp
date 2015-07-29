////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Log.cpp
//
//   Abstract:
//      This module contains functions which assist in logging.
//
//   Naming Convention:
//
//      <Scope><Module><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                  - Function is likely visible to other modules
//          }
//       <Module>
//          {
//            Hlpr  - Function is from HelperFunctions_* Modules.
//          }
//       <Action>
//          {
//            Log   - Function writes data to the console
//          }
//       <Modifier>
//          {
//            Error - Function pertains to error occurrences
//            Info  - Function pertains to informational mesages
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprLogError(),
//      HlprLogInfo(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

#define MAX_CHARS 1024

/**
 @helper_function="HlprLogError"
 
   Purpose:  Log an error message to the console.                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprLogError(_In_ PCWSTR pMessage,
                  ...)
{
   ASSERT(pMessage);

   UINT32 status                 = NO_ERROR;
   PCWSTR pFormatMessage         = L"ERROR:  %s\n";
   PWSTR  pActualMessage         = 0;
   size_t size                   = 0;
   size_t actualSize             = 11;                 /// Size of format Message and room for NULL Terminator '\0'
   WCHAR  pLogMessage[MAX_CHARS] = {0};

   va_list argumentList;

   va_start(argumentList,
            pMessage);

   status = StringCchVPrintf(pLogMessage,
                             MAX_CHARS,
                             pMessage,
                             argumentList);
   if(status != NO_ERROR)
   {
      wprintf(L"HlprLogError : StringCchVPrintf() [status: %#x]",
              status);

      HLPR_BAIL;
   }

   status = StringCchLength(pLogMessage,
                            STRSAFE_MAX_CCH,
                            &size);
   if(status != NO_ERROR)
   {
      wprintf(L"HlprLogError : StringCchLength() [status: %#x]",
              status);

      HLPR_BAIL;
   }

   actualSize += size;

   HLPR_NEW_ARRAY(pActualMessage,
                  WCHAR,
                  actualSize);
   HLPR_BAIL_ON_ALLOC_FAILURE(pActualMessage,
                              status);

   status = StringCchPrintf(pActualMessage,
                            actualSize,
                            pFormatMessage,
                            pLogMessage);
   if(status != NO_ERROR)
   {
      wprintf(L"HlprLogError : StringCchPrintf() [status: %#x]",
              status);

      HLPR_BAIL;
   }

   wprintf(pActualMessage);

   HLPR_BAIL_LABEL:

   va_end(argumentList);

   HLPR_DELETE_ARRAY(pActualMessage);

   return;
}

/**
 @helper_function="HlprLogInfo"
 
   Purpose:  Log an information message to the console.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
VOID HlprLogInfo(_In_ PCWSTR pMessage,
                 ...)
{
   ASSERT(pMessage);

   UINT32 status                 = NO_ERROR;
   PCWSTR pFormatMessage         = L"INFO:  %s\n";
   PWSTR  pActualMessage         = 0;
   size_t size                   = 0;
   size_t actualSize             = 9;                  /// Size of format Message and room for NULL Terminator '\0'
   WCHAR  pLogMessage[MAX_CHARS] = {0};

   va_list argumentList;

   va_start(argumentList,
            pMessage);

   status = StringCchVPrintf(pLogMessage,
                             MAX_CHARS,
                             pMessage,
                             argumentList);
   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprLogInfo : StringCchVPrintf() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   status = StringCchLength(pLogMessage,
                            STRSAFE_MAX_CCH,
                            &size);
   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprLogInfo : StringCchLength() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   actualSize += size;

   HLPR_NEW_ARRAY(pActualMessage,
                  WCHAR,
                  actualSize);
   HLPR_BAIL_ON_ALLOC_FAILURE(pActualMessage,
                              status);

   status = StringCchPrintf(pActualMessage,
                            actualSize,
                            pFormatMessage,
                            pLogMessage);
   if(status != NO_ERROR)
   {
      HlprLogError(L"HlprLogInfo : StringCchPrintf() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   wprintf(pActualMessage);

   HLPR_BAIL_LABEL:

   va_end(argumentList);

   HLPR_DELETE_ARRAY(pActualMessage);

   return;
}

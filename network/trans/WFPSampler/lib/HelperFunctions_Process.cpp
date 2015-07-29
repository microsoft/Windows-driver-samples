////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Process.cpp
//
//   Abstract:
//      This module contains functions which assist actions pertaining to processes.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                         - Function is likely visible to other modules
//            Prv          - Function is private to this module.
//          }
//       <Module>
//          {
//            Hlpr         - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            Process      - Function pertains to processes.
//          }
//       <Action>
//          {
//            Get          - Function retrieves data.
//          }
//       <Modifier>
//          {
//            State      - Funstion acts on the object's state.
//            String     - Function acts on a null terminated wide character string.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprProcessGetID(),
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

/**
 @helper_function="PrvHlprServiceQueryState"
 
   Purpose: Return the state of the service.                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>

*/

/**
 @helper_function="HlprServiceStop"
 
   Purpose: Issue a stop control to the specified service.                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS682489.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684834.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684836.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684320.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684218.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/MS684221.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprProcessGetID(_In_ PCWSTR pProcessName,
                        _Inout_ UINT32* pPID)
{
   ASSERT(pProcessName);
   ASSERT(pPID);

   UINT32         status                = NO_ERROR;
   HANDLE         processSnapshotHandle = 0;
   PROCESSENTRY32 processEntry          = {0};

   processEntry.dwSize = sizeof(PROCESSENTRY32);

   processSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,
                                                    0);
   if(processSnapshotHandle == INVALID_HANDLE_VALUE)
   {
      status = GetLastError();

      HlprLogError(L"HlprProcessGetID : CreateToolhelp32Snapshot() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   if(!Process32First(processSnapshotHandle,
                      &processEntry))
   {
      status = GetLastError();

      HlprLogError(L"HlprProcessGetID : Process32First() [status: %#x]",
                   status);

      HLPR_BAIL;
   }

   for(;
       *pPID == 0;
       )
   {
      HANDLE        processHandle        = 0;
      HANDLE        moduleSnapshotHandle = 0;
      MODULEENTRY32 moduleEntry          = {0};

      moduleEntry.dwSize = sizeof(MODULEENTRY32);

      processHandle = OpenProcess(PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_READ,
                                  FALSE,
                                  processEntry.th32ProcessID);
      if(processHandle == 0)
      {
         status = GetLastError();

         /// Don't log errors for the Idle, CSRSS, & System processes
         if(status != ERROR_ACCESS_DENIED &&
            status != ERROR_INVALID_PARAMETER)
            HlprLogError(L"HlprProcessGetID : OpenProcess() [status: %#x]",
                         status);

         HLPR_BAIL_2;
      }

      moduleSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
                                                      processEntry.th32ProcessID);
      if(moduleSnapshotHandle == INVALID_HANDLE_VALUE)
      {
         status = GetLastError();

         HlprLogError(L"HlprProcessGetID : CreateToolhelp32Snapshot() [status: %#x]",
                      status);

         HLPR_BAIL_2;
      }

      if(!Module32First(moduleSnapshotHandle,
                        &moduleEntry))
      {
         status = GetLastError();

         HlprLogError(L"HlprProcessGetID : Module32First() [status: %#x]",
                      status);

         HLPR_BAIL_2;
      }

      for(;
          *pPID == 0;
          )
      {
         if(HlprStringsAreEqual(pProcessName,
                                moduleEntry.szModule))
            *pPID = processEntry.th32ProcessID;

         if(!Module32Next(moduleSnapshotHandle,
                          &moduleEntry))
            break;
      }

      HLPR_BAIL_LABEL_2:

      HLPR_CLOSE_HANDLE(processHandle);

      if(!Process32Next(processSnapshotHandle,
                        &processEntry))
         break;
   }

   HLPR_BAIL_LABEL:

   HLPR_CLOSE_HANDLE(processSnapshotHandle);

   ASSERT(*pPID);

   return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmEngine.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to BFE engine objects.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                       - Function is likely visible to other modules.
//          }
//       <Module>
//          {
//            Hlpr       - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpmEngine - Function pertains to BFE engine objects.
//          }
//       <Action>
//          {
//            Close      - Function destroys a handle.
//            Open       - Function creates a handle.
//          }
//       <Modifier>
//          {
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprFwpmEngineClose(),
//      HlprFwpmEngineOpen()
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
 @helper_function="HlprFwpmEngineOpen"
 
   Purpose:  Wrapper for the FwpmEngineClose API.                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364034.aspx                              <br>
*/
_When_(return == NO_ERROR, _At_(*pEngineHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmEngineClose(_Inout_ HANDLE* pEngineHandle)
{
   ASSERT(pEngineHandle);

   UINT32 status = NO_ERROR;

   if(*pEngineHandle)
   {
      status = FwpmEngineClose(*pEngineHandle);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprFwpmEngineClose : FwpmEngineClose() [status: %#x]",
                      status);

         HLPR_BAIL;
      }

      *pEngineHandle = 0;
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprFwpmEngineOpen"
 
   Purpose:  Wrapper for the FwpmEngineOpen API.                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364040.aspx                              <br>
*/
_At_(*pEngineHandle, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pEngineHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEngineHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmEngineOpen(_Out_ HANDLE* pEngineHandle,
                          _In_ const UINT32 sessionFlags) /* FWPM_SESSION_FLAG_NONDYNAMIC */
{
   ASSERT(pEngineHandle);

   UINT32       status  = NO_ERROR;
   FWPM_SESSION session = {0};

   session.displayData.name = L"WFPSampler's User Mode Session";
   session.flags            = sessionFlags;

   status = FwpmEngineOpen(0,
                           RPC_C_AUTHN_WINNT,
                           0,
                           &session,
                           pEngineHandle);
   if(status != NO_ERROR)
      HlprLogError(L"HlprFwpmEngineOpen : FwpmEngineOpen() [status: %#x]",
                   status);

   return status;
}

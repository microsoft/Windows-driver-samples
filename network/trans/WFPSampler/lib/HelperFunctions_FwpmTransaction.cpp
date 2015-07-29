////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmTransaction.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to BFE transactions.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                            - Function is likely visible to other modules.
//          }
//       <Module>
//          {
//            Hlpr            - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpmTransaction - Function pertains to BFE engine objects.
//          }
//       <Action>
//          {
//            Abort           - Function ends a transaction without making changes.
//            Begin           - Function starts a transaction.
//            Commit          - Function ends a transaction successfully.
//          }
//       <Modifier>
//          {
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprFwpmTransactionAbort(),
//      HlprFwpmTransactionBegin(),
//      HlprFwpmTransactionCommit(),
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
 @helper_function="HlprFwpmTransactionAbort"
 
   Purpose:  Wrapper for the FwpmTransactionAbort API.                                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364242.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmTransactionAbort(_In_ HANDLE engineHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      status = FwpmTransactionAbort(engineHandle);
      if(status != NO_ERROR)
         HlprLogError(L"HlprFwpmTransactionAbort() [status: %#x]",
                      status);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmTransactionAbort() [status: %#x][engineHandle: %#p]",
                   status,
                   engineHandle);
   }

   return status;
}

/**
 @helper_function="HlprFwpmTransactionBegin"
 
   Purpose:  Wrapper for the FwpmTransactionBegin API.                                          <br>
                                                                                                <br>
   Notes:    Only need to use if making multiple BFE calls.                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364243.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmTransactionBegin(_In_ HANDLE engineHandle,
                                _In_ UINT32 flags)        /* 0 */
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      status = FwpmTransactionBegin(engineHandle,
                                    flags);
      if(status != NO_ERROR)
         HlprLogError(L"HlprFwpmTransactionBegin() [status: %#x]",
                      status);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmTransactionBegin() [status: %#x][engineHandle: %#p]",
                   status,
                   engineHandle);
   }

   return status;
}

/**
 @helper_function="HlprFwpmTransactionCommit"
 
   Purpose:  Wrapper for the FwpmTransactionCommit API.                                         <br>
                                                                                                <br>
   Notes:    On failure, transaction gets aborted.                                              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364245.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmTransactionCommit(_In_ HANDLE engineHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      status = FwpmTransactionCommit(engineHandle);
      if(status != NO_ERROR)
      {
         HlprLogError(L"HlprFwpmTransactionCommit() [status: %#x]",
                      status);

         HlprFwpmTransactionAbort(engineHandle);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmTransactionCommit() [status: %#x][engineHandle: %#p]",
                   status,
                   engineHandle);
   }

   return status;
}

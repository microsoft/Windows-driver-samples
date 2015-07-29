////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmCallout.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to FWPM_CALLOUT objects.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                        - Function is likely visible to other modules.
//          }
//       <Module>
//          {
//            Hlpr        - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpmCallout - Function pertains to FWPM_CALLOUT objects.
//          }
//       <Action>
//          {
//            Add         - Function adds and object to BFE.
//            Create      - Function allocates memory for an object.
//            Delete      - Function deletes an object from BFE.
//            Destroy     - Function frees memory for an object
//            Enum        - Function returns list of requested objects.
//            Remove      - Function deletes objects from BFE.
//          }
//       <Modifier>
//          {
//            All         - Function acts on all FWPM_CALLOUT objects.
//            ByKey       - Function takes a runtime ID.
//            EnumHandle  - Function acts on the enumeration Handle.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprFwpmCalloutAdd(),
//      HlprFwpmCalloutCreateEnumHandle(),
//      HlprFwpmCalloutDeleteByKey(),
//      HlprFwpmCalloutDestroyEnumHandle(),
//      HlprFwpmCalloutEnum(),
//      HlprFwpmCalloutRemoveAll(),
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
 @helper_function="HlprFwpmCalloutDeleteByKey"
 
   Purpose:  Wrapper for the FwpmCalloutDeleteByKey API                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364016.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutDeleteByKey(_In_ const HANDLE engineHandle,
                                  _In_ const GUID* pCalloutKey)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pCalloutKey)
   {
      status = FwpmCalloutDeleteByKey(engineHandle,
                                      pCalloutKey);
      if(status != NO_ERROR)
      {
         if(status != FWP_E_IN_USE &&
            status != FWP_E_BUILTIN_OBJECT &&
            status != FWP_E_CALLOUT_NOT_FOUND)
            HlprLogError(L"HlprFwpmCalloutDeleteByKey : FwpmCalloutDeleteByKey() [status: %#x]",
                         status);
         else
         {
            HlprLogInfo(L"HlprFwpmCalloutDeleteByKey : FwpmCalloutDeleteByKey() [status: %#x]",
                         status);

            status = NO_ERROR;
         }
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmCalloutDeleteByKey() [status: %#x][engineHandle: %#p][pCalloutKey: %#p]",
                   status,
                   engineHandle,
                   pCalloutKey);
   }

   return status;
}

/**
 @helper_function="HlprFwpmCalloutAdd"
 
   Purpose:  Wrapper for the FwpmCalloutAdd API                                                 <br>
                                                                                                <br>
   Notes:    Callout ID is written to pCallout->calloutId.                                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364010.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutAdd(_In_ const HANDLE engineHandle,
                          _Inout_ FWPM_CALLOUT* pCallout)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pCallout)
   {
      status = FwpmCalloutAdd(engineHandle,
                              pCallout,
                              0,
                              &(pCallout->calloutId));
      if(status != NO_ERROR)
      {
         if(status == FWP_E_ALREADY_EXISTS)
         {
            HlprLogInfo(L"Callout Already Exists");

            status = NO_ERROR;
         }
         else
            HlprLogError(L"HlprFwpmCalloutAdd : FwpmCalloutAdd() [status: %#x]",
                         status);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmCalloutAdd() [status: %#x][engineHandle: %#p][pCallout: %#p]",
                   status,
                   engineHandle,
                   pCallout);
   }

   return status;
}

/**
 @helper_function="HlprFwpmCalloutRemoveAll"
 
   Purpose:  Remove all callouts associated with the specified provider.                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutRemoveAll(_In_opt_ HANDLE* pEngineHandle,
                                _In_ const GUID* pProviderKey)
{
   UINT32  status       = NO_ERROR;
   HANDLE  engineHandle = 0;
   BOOLEAN isLocal      = FALSE;

   if(pEngineHandle &&
      *pEngineHandle)
      engineHandle = *pEngineHandle;
   else
   {
      status = HlprFwpmEngineOpen(&engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      isLocal = TRUE;
   }

   HANDLE                     enumHandle   = 0;
   FWPM_CALLOUT**             ppCallouts   = 0;
   UINT32                     calloutCount = 0;
   FWPM_CALLOUT_ENUM_TEMPLATE enumTemplate;

   enumTemplate.providerKey = (GUID*)pProviderKey;

   status = HlprFwpmCalloutCreateEnumHandle(engineHandle,
                                            &enumTemplate,
                                            &enumHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmCalloutEnum(engineHandle,
                                enumHandle,
                                0xFFFFFFFF,
                                &ppCallouts,
                                &calloutCount);
   if(ppCallouts &&
      calloutCount)
   {
      for(UINT32 calloutIndex = 0;
          calloutIndex < calloutCount;
          calloutIndex++)
      {
         HlprFwpmCalloutDeleteByKey(engineHandle,
                                    &(ppCallouts[calloutIndex]->calloutKey));
      }

      FwpmFreeMemory((VOID**)&ppCallouts);
   }

   HlprFwpmCalloutDestroyEnumHandle(engineHandle,
                                    &enumHandle);

   HLPR_BAIL_LABEL:

   if(engineHandle &&
      isLocal)
      HlprFwpmEngineClose(&engineHandle);

   return status;
}

/**
 @helper_function="HlprFwpmCalloutEnum"
 
   Purpose:  Wrapper for the FwpmCalloutEnum API                                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364020.aspx                              <br>
*/
_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutEnum(_In_ const HANDLE engineHandle,
                           _In_ const HANDLE enumHandle,
                           _In_ const UINT32 numEntriesRequested,
                           _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_CALLOUT*** pppEntries,
                           _Out_ UINT32* pNumEntriesReturned)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      enumHandle &&
      numEntriesRequested &&
      pppEntries &&
      pNumEntriesReturned)
   {
      status = FwpmCalloutEnum(engineHandle,
                               enumHandle,
                               numEntriesRequested,
                               pppEntries,
                               pNumEntriesReturned);
      if(status != NO_ERROR &&
         status != FWP_E_CALLOUT_NOT_FOUND &&
         status != FWP_E_NOT_FOUND)
         HlprLogError(L"HlprFwpmCalloutEnum : FwpmCalloutEnum() [status: %#x]",
                      status);
   }
   else
   {
      if(pppEntries)
         *pppEntries = 0;

      if(pNumEntriesReturned)
         *pNumEntriesReturned = 0;

      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmCalloutEnum() [status: %#x][engineHandle: %#p][enumHandle: %#p][numEntriesRequested: %d][pppEntries: %#p][pNumEntriesReturned: %#p]",
                   status,
                   engineHandle,
                   enumHandle,
                   numEntriesRequested,
                   pppEntries,
                   pNumEntriesReturned);
   }

   return status;
}

/**
 @helper_function="HlprFwpmCalloutDestroyEnumHandle"
 
   Purpose:  Wrapper for the FwpmCalloutDestroyEnumHandle API                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364017.aspx                              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                        _Inout_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      if(*pEnumHandle)
      {
         status = FwpmCalloutDestroyEnumHandle(engineHandle,
                                               *pEnumHandle);
         if(status != NO_ERROR)
         {
            HlprLogError(L"HlprFwpmCalloutDestroyEnumHandle : FwpmCalloutDestroyEnumHandle() [status: %#x]",
                         status);

            HLPR_BAIL;
         }

         *pEnumHandle = 0;
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmCalloutDestroyEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprFwpmCalloutCreateEnumHandle"
 
   Purpose:  Wrapper for the FwpmCalloutCreateEnumHandle API                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364012.aspx                              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmCalloutCreateEnumHandle(_In_ const HANDLE engineHandle,
                                       _In_opt_ const FWPM_CALLOUT_ENUM_TEMPLATE* pEnumTemplate,
                                       _Out_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      status = FwpmCalloutCreateEnumHandle(engineHandle,
                                           pEnumTemplate,
                                           pEnumHandle);
      if(status != NO_ERROR)
      {
         *pEnumHandle = 0;

         HlprLogError(L"HlprFwpmCalloutCreateEnumHandle : FwpmCalloutCreateEnumHandle() [status: %#x]",
                      status);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmCalloutCreateEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   return status;
}

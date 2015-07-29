////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmFilter.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to FWPM_FILTER objects.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                                - Function is likely visible to other modules.
//            Prv                 - Function is private to this module.
//          }
//       <Module>
//          {
//            Hlpr                - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpmProviderContext - Function pertains to FWPM_PROVIDER_CONTEXT objects.
//          }
//       <Action>
//          {
//            Add                 - Function adds an object.
//            Create              - Function allocates memory for an object.
//            Delete              - Function deletes an object.
//            Destroy             - Function frees memory for an object.
//            Enum                - Function returns list of requested objects.
//            Remove              - Function deletes objects.
//          }
//       <Modifier>
//          {
//            All                 - Functions acts on all of WFPSampler's provider contexts.
//            ByKey               - Function acts off of the object's GUID.
//            EnumHandle          - Function acts on the enumeration handle.
//          }
//
//   Private Functions:
//
//   Public Functions:
//      HlprFwpmProviderContextAdd(),
//      HlprFwpmProviderContextCreateEnumHandle(),
//      HlprFwpmProviderContextDeleteByKey(),
//      HlprFwpmProviderContextDestroyEnumHandle(),
//      HlprFwpmProviderContextEnum(),
//      HlprFwpmProviderContextRemoveAll(),
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
 @helper_function="HlprFwpmProviderContextDeleteByKey"
 
   Purpose:  Wrapper for the FwpmProviderContextDeleteByKey API.                                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364184.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextDeleteByKey(_In_ const HANDLE engineHandle,
                                          _In_ const GUID* pProviderContextKey)
{
   ASSERT(engineHandle);
   ASSERT(pProviderContextKey);

   UINT32 status = NO_ERROR;

   status = FwpmProviderContextDeleteByKey(engineHandle,
                                           pProviderContextKey);
   if(status != NO_ERROR)
   {
      if(status != FWP_E_IN_USE &&
         status != FWP_E_BUILTIN_OBJECT &&
         status != FWP_E_PROVIDER_CONTEXT_NOT_FOUND)
         HlprLogError(L"HlprFwpmProviderContextDeleteByKey : FwpmProviderContextDeleteByKey() [status: %#x]",
                      status);
      else
      {
         HlprLogInfo(L"HlprFwpmProviderContextDeleteByKey : FwpmProviderContextDeleteByKey() [status: %#x]",
                     status);

         status = NO_ERROR;
      }
   }

   return status;
}

/**
 @helper_function="HlprFwpmProviderContextAdd"
 
   Purpose:  Wrapper for the FwpmProviderContextAdd API.                                        <br>
                                                                                                <br>
   Notes:    ProviderContext ID is written to pProviderContext->providerContextId.              <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364181.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextAdd(_In_ const HANDLE engineHandle,
                                  _Inout_ FWPM_PROVIDER_CONTEXT* pProviderContext)
{
   ASSERT(engineHandle);
   ASSERT(pProviderContext);

   UINT32 status = NO_ERROR;

   status = FwpmProviderContextAdd(engineHandle,
                                   pProviderContext,
                                   0,
                                   &(pProviderContext->providerContextId));
   if(status != NO_ERROR)
   {
      if(status == FWP_E_ALREADY_EXISTS)
      {
         HlprLogInfo(L"ProviderContext Already Exists");

         status = NO_ERROR;
      }
      else
         HlprLogError(L"HlprFwpmProviderContextAdd : FwpmProviderContextAdd() [status: %#x]",
                      status);
   }

   return status;
}

/**
 @helper_function="HlprFwpmProviderContextEnum"
 
   Purpose:  Wrapper for the FwpmProviderContextEnum API.                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364186.aspx              <br>
*/
_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextEnum(_In_ const HANDLE engineHandle,
                                   _In_ const HANDLE enumHandle,
                                   _In_ const UINT32 numEntriesRequested,
                                   _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_PROVIDER_CONTEXT*** pppEntries,
                                   _Out_ UINT32* pNumEntriesReturned)
{
   ASSERT(engineHandle);
   ASSERT(enumHandle);
   ASSERT(numEntriesRequested);
   ASSERT(pppEntries);
   ASSERT(pNumEntriesReturned);

   UINT32 status = NO_ERROR;

   status = FwpmProviderContextEnum(engineHandle,
                                    enumHandle,
                                    numEntriesRequested,
                                    pppEntries,
                                    pNumEntriesReturned);
   if(status != NO_ERROR &&
      status != FWP_E_PROVIDER_CONTEXT_NOT_FOUND &&
      status != FWP_E_NOT_FOUND)
   {
      *pppEntries = 0;

      *pNumEntriesReturned = 0;

      HlprLogError(L"HlprFwpmProviderContextEnum : FwpmProviderContextEnum() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprFwpmProviderContextDestroyEnumHandle"
 
   Purpose:  Wrapper for the FwpmProviderContextDestroyEnumHandle API.                          <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364185.aspx                              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                                _Inout_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      if(*pEnumHandle)
      {
         status = FwpmProviderContextDestroyEnumHandle(engineHandle,
                                                       *pEnumHandle);
         if(status != NO_ERROR)
         {
            HlprLogError(L"HlprFwpmProviderContextDestroyEnumHandle : FwpmProviderContextDestroyEnumHandle() [status: %#x]",
                         status);

            HLPR_BAIL;
         }

         *pEnumHandle = 0;
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmProviderContextDestroyEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprFwpmProviderContextCreateEnumHandle"
 
   Purpose:  Wrapper for the FwpmProviderContextCreateEnumHandle API.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364182.aspx              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextCreateEnumHandle(_In_ const HANDLE engineHandle,
                                               _In_opt_ const FWPM_PROVIDER_CONTEXT_ENUM_TEMPLATE* pEnumTemplate,
                                               _Out_ HANDLE* pEnumHandle)
{
   ASSERT(engineHandle);
   ASSERT(pEnumHandle);

   UINT32 status = NO_ERROR;

   status = FwpmProviderContextCreateEnumHandle(engineHandle,
                                                pEnumTemplate,
                                                pEnumHandle);
   if(status != NO_ERROR)
      HlprLogError(L"HlprFwpmProviderContextCreateEnumHandle : FwpmProviderContextCreateEnumHandle() [status: %#x]",
                   status);

   return status;
}

/**
 @helper_function="HlprFwpmProviderContextRemoveAll"
 
   Purpose:  Remove all providerContexts associated with the specified provider.                <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderContextRemoveAll(_In_opt_ HANDLE* pEngineHandle,
                                        _In_opt_ const GUID* pProviderContextKey)
{
   UINT32                              status               = NO_ERROR;
   HANDLE                              engineHandle         = 0;
   HANDLE                              enumHandle           = 0;
   BOOLEAN                             isLocal              = FALSE;
   FWPM_PROVIDER_CONTEXT**             ppProviderContexts   = 0;
   UINT32                              providerContextCount = 0;
   FWPM_PROVIDER_CONTEXT_ENUM_TEMPLATE enumTemplate         = {0};

   if(pEngineHandle &&
      *pEngineHandle)
      engineHandle = *pEngineHandle;
   else
   {
      status = HlprFwpmEngineOpen(&engineHandle);
      HLPR_BAIL_ON_FAILURE(status);

      isLocal = TRUE;
   }

   enumTemplate.providerKey = (GUID*)pProviderContextKey;

   status = HlprFwpmProviderContextCreateEnumHandle(engineHandle,
                                                    &enumTemplate,
                                                    &enumHandle);
   HLPR_BAIL_ON_FAILURE(status);

   status = HlprFwpmProviderContextEnum(engineHandle,
                                        enumHandle,
                                        0xFFFFFFFF,
                                        &ppProviderContexts,
                                        &providerContextCount);
   if(ppProviderContexts &&
      providerContextCount)
   {
      for(UINT32 providerContextIndex = 0;
          providerContextIndex < providerContextCount;
          providerContextIndex++)
      {
         HlprFwpmProviderContextDeleteByKey(engineHandle,
                                            &(ppProviderContexts[providerContextIndex]->providerContextKey));
      }

      FwpmFreeMemory((VOID**)&ppProviderContexts);
   }

   HlprFwpmProviderContextDestroyEnumHandle(engineHandle,
                                            &enumHandle);

   HLPR_BAIL_LABEL:

   if(engineHandle &&
      isLocal)
      HlprFwpmEngineClose(&engineHandle);

   return status;
}

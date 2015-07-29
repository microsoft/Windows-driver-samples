////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmProvider.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to FWPM_PROVIDER objects.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                          - Function is likely visible to other modules.
//          }
//       <Module>
//          {
//            Hlpr          - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpmProvider  - Function pertains to FWPM_PROVIDER objects.
//          }
//       <Action>
//          {
//            Add           - Function adds an object.
//            Remove        - Function deletes objects.
//          }
//       <Modifier>
//          {
//            DefaultGlobal - Function acts on the programs constant global object.
//          }
//   Private Functions:
//
//   Public Functions:
//      HlprFwpmProviderAddDefaultGlobal(),
//      HlprFwpmProviderRemoveDefaultGlobal(),
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
 @helper_function="HlprFwpmProviderEnum"
 
   Purpose:  Wrapper for the FwpmProviderEnum API.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364197.aspx              <br>
*/
_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderEnum(_In_ const HANDLE engineHandle,
                            _In_ const HANDLE enumHandle,
                            _In_ const UINT32 numEntriesRequested,
                            _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_PROVIDER*** pppEntries,
                            _Out_ UINT32* pNumEntriesReturned)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      enumHandle &&
      numEntriesRequested &&
      pppEntries &&
      pNumEntriesReturned)
   {
      status = FwpmProviderEnum(engineHandle,
                                enumHandle,
                                numEntriesRequested,
                                pppEntries,
                                pNumEntriesReturned);
      if(status != NO_ERROR &&
         status != FWP_E_PROVIDER_NOT_FOUND &&
         status != FWP_E_NOT_FOUND)
         HlprLogError(L"HlprFwpmProviderEnum : FwpmProviderEnum() [status: %#x]",
                      status);
   }
   else
   {
      if(pppEntries)
         *pppEntries = 0;

      if(pNumEntriesReturned)
         *pNumEntriesReturned = 0;

      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmProviderEnum() [status: %#x][engineHandle: %#p][enumHandle: %#p][numEntriesRequested: %d][pppEntries: %#p][pNumEntriesReturned: %#p]",
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
 @helper_function="HlprFwpmProviderDestroyEnumHandle"
 
   Purpose:  Wrapper for the FwpmProviderDestroyEnumHandle API.                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364196.aspx              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                         _Inout_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      if(*pEnumHandle)
      {
         status = FwpmProviderDestroyEnumHandle(engineHandle,
                                                *pEnumHandle);
         if(status != NO_ERROR)
         {
            HlprLogError(L"HlprFwpmProviderDestroyEnumHandle : FwpmProviderDestroyEnumHandle() [status: %#x]",
                         status);

            HLPR_BAIL;
         }

         *pEnumHandle = 0;
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmProviderDestroyEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprFwpmProviderCreateEnumHandle"
 
   Purpose:  Wrapper for the FwpmProviderCreateEnumHandle API.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364194.aspx              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderCreateEnumHandle(_In_ const HANDLE engineHandle,
                                        _In_opt_ const FWPM_PROVIDER_ENUM_TEMPLATE* pEnumTemplate,
                                        _Out_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      status = FwpmProviderCreateEnumHandle(engineHandle,
                                            pEnumTemplate,
                                            pEnumHandle);
      if(status != NO_ERROR)
         HlprLogError(L"HlprFwpmProviderCreateEnumHandle : FwpmProviderCreateEnumHandle() [status: %#x]",
                      status);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmProviderCreateEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   return status;
}

/**
 @helper_function="HlprFwpmProviderDeleteByKey"
 
   Purpose:  Wrapper for the FwpmProviderDeleteByKey API.                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364195.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderDeleteByKey(_In_ const HANDLE engineHandle,
                                   _In_ const GUID* pProviderKey)
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      if(pProviderKey)
      {
         status = FwpmProviderDeleteByKey(engineHandle,
                                          pProviderKey);
         if(status != NO_ERROR)
         {
            if(status != FWP_E_IN_USE &&
               status != FWP_E_BUILTIN_OBJECT &&
               status != FWP_E_PROVIDER_NOT_FOUND)
               HlprLogError(L"HlprFwpmProviderDeleteByKey : FwpmProviderDeleteByKey() [status: %#x]",
                            status);
            else
            {
               HlprLogInfo(L"HlprFwpmProviderDeleteByKey : FwpmProviderDeleteByKey() [status: %#x]",
                            status);

               status = NO_ERROR;
            }
         }
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmProviderDeleteByKey() [status: %#x][engineHandle: %#p]",
                   status,
                   engineHandle);
   }

   return status;
}

/**
 @helper_function="HlprFwpmProviderDelete"
 
   Purpose:  Remove the provider that was associated with all of this program's WFP objects.    <br>
                                                                                                <br>
   Notes:    Function will fail if any of this program's WFP objects are still present and 
             associated with this provider.                                                     <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364195.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderDelete(_In_opt_ HANDLE* pEngineHandle,
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

   status = FwpmProviderDeleteByKey(engineHandle,
                                    pProviderKey);
   if(status != NO_ERROR)
   {
      if(status != FWP_E_PROVIDER_NOT_FOUND &&
         status != FWP_E_IN_USE)
         HlprLogError(L"HlprFwpmProviderDelete : FwpmProviderDeleteByKey() [status: %#x]",
                      status);
      else
      {
         HlprLogInfo(L"HlprFwpmProviderDelete : FwpmProviderDeleteByKey() [status: %#x]",
                     status);

         status = NO_ERROR;
      }
   }

   HLPR_BAIL_LABEL:

   if(isLocal &&
      engineHandle)
      HlprFwpmEngineClose(&engineHandle);

   return status;
}

/**
 @helper_function="HlprFwpmProviderAdd"
 
   Purpose:  Add a default provider to associate with all of this program's WFP objects.        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA364180.aspx              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmProviderAdd(_In_ HANDLE engineHandle,
                           _In_ const GUID* pProviderKey,
                           _In_ PCWSTR pCompanyName,
                           _In_ PCWSTR pBinaryDescription,
                           _In_ PCWSTR pServiceName,
                           _In_ UINT32 flags)
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      FWPM_PROVIDER provider = {0};   

      provider.providerKey             = *pProviderKey;
      provider.displayData.name        = (PWSTR)pCompanyName;
      provider.displayData.description = (PWSTR)pBinaryDescription;
      provider.flags                   = flags;
      provider.serviceName             = (PWSTR)pServiceName;

      status = FwpmProviderAdd(engineHandle,
                               &provider,
                               0);
      if(status != NO_ERROR)
      {
         if(status == FWP_E_ALREADY_EXISTS)
         {
            HlprLogInfo(L"Provider Already Exists");

            status = NO_ERROR;
         }
         else
            HlprLogError(L"HlprFwpmProviderAdd : FwpmProviderAdd() [status: %#x]",
                         status);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmProviderAdd() [status: %#x]",
                   status);
   }

   return status;
}

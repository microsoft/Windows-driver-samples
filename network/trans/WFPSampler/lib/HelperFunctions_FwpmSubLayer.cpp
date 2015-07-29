////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmSubLayer.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to FWPM_SUBLAYER objects.
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
//            FwpmSubLayer  - Function pertains to FWPM_SUBLAYER objects.
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
//      HlprFwpmSubLayerAddDefaultGlobal(),
//      HlprFwpmSubLayerRemoveDefaultGlobal(),
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
 @helper_function="HlprFwpmSubLayerEnum"
 
   Purpose:  Wrapper for the FwpmSubLayerEnum API.                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364211.aspx                              <br>
*/
_At_(*pppEntries, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppEntries, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerEnum(_In_ const HANDLE engineHandle,
                            _In_ const HANDLE enumHandle,
                            _In_ const UINT32 numEntriesRequested,
                            _Outptr_result_buffer_maybenull_(*pNumEntriesReturned) FWPM_SUBLAYER*** pppEntries,
                            _Out_ UINT32* pNumEntriesReturned)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      enumHandle &&
      numEntriesRequested &&
      pppEntries &&
      pNumEntriesReturned)
   {
      status = FwpmSubLayerEnum(engineHandle,
                                enumHandle,
                                numEntriesRequested,
                                pppEntries,
                                pNumEntriesReturned);
      if(status != NO_ERROR &&
         status != FWP_E_SUBLAYER_NOT_FOUND &&
         status != FWP_E_NOT_FOUND)
         HlprLogError(L"HlprFwpmSubLayerEnum : FwpmSubLayerEnum() [status: %#x]",
                      status);
   }
   else
   {
      if(pppEntries)
         *pppEntries = 0;

      if(pNumEntriesReturned)
         *pNumEntriesReturned = 0;

      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmSubLayerEnum() [status: %#x][engineHandle: %#p][enumHandle: %#p][numEntriesRequested: %d][pppEntries: %#p][pNumEntriesReturned: %#p]",
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
 @helper_function="HlprFwpmSubLayerDestroyEnumHandle"
 
   Purpose:  Wrapper for the FwpmSubLayerDestroyEnumHandle API.                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364210.aspx                              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerDestroyEnumHandle(_In_ const HANDLE engineHandle,
                                         _Inout_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      if(*pEnumHandle)
      {
         status = FwpmSubLayerDestroyEnumHandle(engineHandle,
                                                *pEnumHandle);
         if(status != NO_ERROR)
         {
            HlprLogError(L"HlprFwpmSubLayerDestroyEnumHandle : FwpmSubLayerDestroyEnumHandle() [status: %#x]",
                         status);

            HLPR_BAIL;
         }

         *pEnumHandle = 0;
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmSubLayerDestroyEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   HLPR_BAIL_LABEL:

   return status;
}

/**
 @helper_function="HlprFwpmSubLayerCreateEnumHandle"
 
   Purpose:  Wrapper for the FwpmSubLayerCreateEnumHandle API.                                  <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364208.aspx                              <br>
*/
_When_(return != NO_ERROR, _At_(*pEnumHandle, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pEnumHandle, _Post_ _Notnull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerCreateEnumHandle(_In_ const HANDLE engineHandle,
                                        _In_opt_ const FWPM_SUBLAYER_ENUM_TEMPLATE* pEnumTemplate,
                                        _Out_ HANDLE* pEnumHandle)
{
   UINT32 status = NO_ERROR;

   if(engineHandle &&
      pEnumHandle)
   {
      status = FwpmSubLayerCreateEnumHandle(engineHandle,
                                            pEnumTemplate,
                                            pEnumHandle);
      if(status != NO_ERROR)
         HlprLogError(L"HlprFwpmSubLayerCreateEnumHandle : FwpmSubLayerCreateEnumHandle() [status: %#x]",
                      status);
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmSubLayerCreateEnumHandle() [status: %#x][engineHandle: %#p][pEnumHandle: %#p]",
                   status,
                   engineHandle,
                   pEnumHandle);
   }

   return status;
}

/**
 @helper_function="HlprFwpmSubLayerDeleteByKey"
 
   Purpose:  Wrapper for the FwpmSubLayerDeleteByKey API.                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364209.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerDeleteByKey(_In_ const HANDLE engineHandle,
                                   _In_ const GUID* pSubLayerKey)
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      if(pSubLayerKey)
      {
         status = FwpmSubLayerDeleteByKey(engineHandle,
                                          pSubLayerKey);
         if(status != NO_ERROR)
         {
            if(status != FWP_E_IN_USE &&
               status != FWP_E_BUILTIN_OBJECT &&
               status != FWP_E_SUBLAYER_NOT_FOUND)
               HlprLogError(L"HlprFwpmSubLayerDeleteByKey : FwpmSubLayerDeleteByKey() [status: %#x]",
                            status);
            else
            {
               HlprLogInfo(L"HlprFwpmSubLayerDeleteByKey : FwpmSubLayerDeleteByKey() [status: %#x]",
                            status);

               status = NO_ERROR;
            }
         }
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmSubLayerDeleteByKey() [status: %#x][engineHandle: %#p]",
                   status,
                   engineHandle);
   }

   return status;
}

/**
 @helper_function="HlprFwpmSubLayerAdd"
 
   Purpose:  Add a sublayer to associate with all of this program's filters.                    <br>
                                                                                                <br>
   Notes:    By default sublayer is weighted just below the weight of FWPM_SUBLAYER_UNIVERSAL 
             so IPsec decryption will occur prior to invocation of any of WFPSampler's callout 
             routines.                                                                          <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364207.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerAdd(_In_ HANDLE engineHandle,
                           _In_ const GUID* pSubLayerKey,
                           _In_ PCWSTR pSubLayerName,
                           _In_ const GUID* pProviderKey,
                           _In_ UINT16 weight,            /* 0x7FFF */
                           _In_ UINT32 flags)             /* FWPM_SUBLAYER_FLAG_PERSISTENT */
{
   UINT32 status = NO_ERROR;

   if(engineHandle)
   {
      FWPM_SUBLAYER subLayer = {0};

      subLayer.subLayerKey      = *pSubLayerKey;
      subLayer.displayData.name = (PWSTR)pSubLayerName;
      subLayer.flags            = flags;
      subLayer.providerKey      = (GUID*)pProviderKey;
      subLayer.weight           = weight;

      status = FwpmSubLayerAdd(engineHandle,
                               &subLayer,
                               0);
      if(status != NO_ERROR)
      {
         if(status == FWP_E_ALREADY_EXISTS)
         {
            HlprLogInfo(L"SubLayer Already Exists");

            status = NO_ERROR;
         }
         else
            HlprLogError(L"HlprFwpmSubLayerAdd : FwpmSubLayerAdd() [status: %#x]",
                         status);
      }
   }
   else
   {
      status = ERROR_INVALID_PARAMETER;

      HlprLogError(L"HlprFwpmSubLayerAdd() [status: %#x]",
                   status);
   }

   return status;
}

/**
 @helper_function="HlprFwpmSubLayerDelete"
 
   Purpose:  Remove the sublayer that was associated with all of WFPSampler's filters.          <br>
                                                                                                <br>
   Notes:    Function will fail if any of WFPSampler's filters are still present and associated 
             with this sublayer.                                                                <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/AA364209.aspx                              <br>
*/
_Success_(return == NO_ERROR)
UINT32 HlprFwpmSubLayerDelete(_In_opt_ HANDLE* pEngineHandle,
                              _In_ const GUID* pSubLayerKey)
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

   status = FwpmSubLayerDeleteByKey(engineHandle,
                                    pSubLayerKey);
   if(status != NO_ERROR)
   {
      if(status != FWP_E_SUBLAYER_NOT_FOUND &&
         status != FWP_E_IN_USE)
         HlprLogError(L"HlprFwpmSubLayerDelete : FwpmSubLayerDeleteByKey() [status: %#x]",
                      status);
      else
      {
         HlprLogInfo(L"HlprFwpmSubLayerDelete : FwpmSubLayerDeleteByKey() [status: %#x]",
                     status);

         status = NO_ERROR;
      }
   }

   HLPR_BAIL_LABEL:

   if(engineHandle &&
      isLocal)
      HlprFwpmEngineClose(&engineHandle);

   return status;
}

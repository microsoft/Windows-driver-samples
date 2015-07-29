////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_RedirectData.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with REDIRECT_DATA.
//
//   Naming Convention:
//
//      <Module><Object><Action>
//  
//      i.e.
//
//       KrnlHlprWorkItemQueue
//
//       <Module>
//          KrnlHlpr     -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          RedirectData -       Function pertains to REDIRECT_DATA objects.
//       <Action>
//          {
//            Create     -       Function allocates and fills memory.
//            Destroy    -       Function cleans up and frees memory.
//            Populate   -       Function fills memory with values
//            Purge      -       Function cleans up values
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprRedirectDataDataCreate(),
//      KrnlHlprRedirectDataDataDestroy(),
//      KrnlHlprRedirectDataDataPopulate(),
//      KrnlHlprRedirectDataDataPurge(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance support for ALE redirection.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"           /// .
#include "HelperFunctions_RedirectData.tmh"    /// $(OBJ_PATH)\$(O)\ 

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @kernel_helper_function="KrnlHlprRedirectDataPurge"
 
   Purpose:  Cleanup a REDIRECT_DATA object.                                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF551137.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF551150.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF551208.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprRedirectDataPurge(_Inout_ REDIRECT_DATA* pRedirectData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprRedirectDataPurge()\n");

#endif /// DBG
   
   NT_ASSERT(pRedirectData);

   if(pRedirectData->pWritableLayerData)
   {
      FwpsApplyModifiedLayerData(pRedirectData->classifyHandle,
                                 pRedirectData->pWritableLayerData,
                                 FWPS_CLASSIFY_FLAG_REAUTHORIZE_IF_MODIFIED_BY_OTHERS);

      pRedirectData->pWritableLayerData = 0;
   }

   if(pRedirectData->classifyHandle)
   {
      if(pRedirectData->isPended)
      {
         FwpsCompleteClassify(pRedirectData->classifyHandle,
                              0,
                              pRedirectData->pClassifyOut);

         pRedirectData->isPended = FALSE;
      }

      FwpsReleaseClassifyHandle(pRedirectData->classifyHandle);

      pRedirectData->classifyHandle = 0;
   }

   RtlZeroMemory(pRedirectData,
                 sizeof(REDIRECT_DATA));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprRedirectDataPurge()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprRedirectDataDestroy"
 
   Purpose:  Cleanup and free a REDIRECT_DATA object.                                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppRedirectData, _Pre_ _Notnull_)
_At_(*ppRedirectData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppRedirectData == 0)
inline VOID KrnlHlprRedirectDataDestroy(_Inout_ REDIRECT_DATA** ppRedirectData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprRedirectDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppRedirectData);

   if(*ppRedirectData)
   {
      KrnlHlprRedirectDataPurge(*ppRedirectData);

      HLPR_DELETE(*ppRedirectData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprRedirectDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprRedirectDataPopulate"
 
   Purpose:  Populates a REDIRECT_DATA object with the classifyHandle and the writable layer 
             data pointer.                                                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF550085.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF550087.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprRedirectDataPopulate(_Inout_ REDIRECT_DATA* pRedirectData,
                                      _In_ const VOID* pClassifyContext,
                                      _In_ const FWPS_FILTER* pFilter,
                                      _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                      _In_opt_ HANDLE redirectHandle)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprRedirectDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pRedirectData);
   NT_ASSERT(pClassifyContext);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);
   NT_ASSERT(pFilter->providerContext);
   NT_ASSERT(pFilter->providerContext->type == FWPM_GENERAL_CONTEXT);
   NT_ASSERT(pFilter->providerContext->dataBuffer);
   NT_ASSERT(pFilter->providerContext->dataBuffer->size == sizeof(PC_PROXY_DATA));
   NT_ASSERT(pFilter->providerContext->dataBuffer->data);

   NTSTATUS status = STATUS_SUCCESS;

   status = FwpsAcquireClassifyHandle((void*)pClassifyContext,
                                      0,
                                      &(pRedirectData->classifyHandle));
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprRedirectDataPopulate : FwpsAcquireClassifyHandle() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   status = FwpsAcquireWritableLayerDataPointer(pRedirectData->classifyHandle,
                                                pFilter->filterId,
                                                0,
                                                &(pRedirectData->pWritableLayerData),
                                                pClassifyOut);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprRedirectDataPopulate : FwpsAcquireWritableLayerDataPointer() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

#if(NTDDI_VERSION >= NTDDI_WIN8)

   pRedirectData->redirectHandle = redirectHandle;

#else

   UNREFERENCED_PARAMETER(redirectHandle);

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)


   pRedirectData->pProxyData = (PC_PROXY_DATA*)pFilter->providerContext->dataBuffer->data;

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprRedirectDataPurge(pRedirectData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprRedirectDataPopulate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprRedirectDataCreate"
 
   Purpose:  Allocates and populates a REDIRECT_DATA object with the classifyHandle and the 
             writable layer data pointer.                                                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppRedirectData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppRedirectData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppRedirectData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprRedirectDataCreate(_Outptr_ REDIRECT_DATA** ppRedirectData,
                                    _In_ const VOID* pClassifyContext,
                                    _In_ const FWPS_FILTER* pFilter,
                                    _In_ FWPS_CLASSIFY_OUT* pClassifyOut,
                                    _In_opt_ HANDLE redirectHandle)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprRedirectDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppRedirectData);
   NT_ASSERT(pClassifyContext);
   NT_ASSERT(pFilter);
   NT_ASSERT(pClassifyOut);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppRedirectData,
            REDIRECT_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppRedirectData,
                              status);

   status = KrnlHlprRedirectDataPopulate(*ppRedirectData,
                                         pClassifyContext,
                                         pFilter,
                                         pClassifyOut,
                                         redirectHandle);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppRedirectData initialized with call to HLPR_NEW & KrnlHlprRedirectDataPopulate 

   if(status != STATUS_SUCCESS &&
      *ppRedirectData)
      KrnlHlprRedirectDataDestroy(ppRedirectData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprRedirectDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

#endif // (NTDDI_VERSION >= NTDDI_WIN7)

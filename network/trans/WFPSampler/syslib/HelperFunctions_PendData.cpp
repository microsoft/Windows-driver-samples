////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_PendData.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with PEND_DATA.
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
//          KrnlHlpr   -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          PendData   -       Function pertains to PEND_DATA objects.
//       <Action>
//          {
//            Create   -       Function allocates and fills memory.
//            Destroy  -       Function cleans up and frees memory.
//            Populate -       Function fills memory with values
//            Purge    -       Function cleans up values
//          }
//
//   Private Functions:
//
//   Public Functions:
//      KrnlHlprPendDataDataCreate(),
//      KrnlHlprPendDataDataDestroy(),
//      KrnlHlprPendDataDataPopulate(),
//      KrnlHlprPendDataDataPurge(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for pending at 
//                                              FWPM_LAYER_ALE_ENDPOINT_CLOSURE
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h"    /// .
#include "HelperFunctions_PendData.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @kernel_helper_function="KrnlHlprPendDataPurge"
 
   Purpose:  Cleanup a PEND_DATA object.                                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF551199.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
inline VOID KrnlHlprPendDataPurge(_Inout_ PEND_DATA* pPendData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprPendDataPurge()\n");

#endif /// DBG
   
   NT_ASSERT(pPendData);

#if(NTDDI_VERSION >= NTDDI_WIN7)

   if(pPendData->layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
      pPendData->layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
   {
      if(pPendData->classifyHandle &&
         pPendData->isPended)
      {
         FwpsCompleteClassify(pPendData->classifyHandle,
                              0,
                              &(pPendData->classifyOut));


         FwpsReleaseClassifyHandle(pPendData->classifyHandle);

         pPendData->classifyHandle = 0;
         pPendData->pPCPendData    = 0;
         pPendData->isPended       = FALSE;
      }
   }
   else

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   {
      if(pPendData->completionContext &&
         pPendData->isPended)
      {
         FwpsCompleteOperation(pPendData->completionContext,
                               pPendData->pNBL);

         pPendData->completionContext      = 0;
         pPendData->pNBL                   = 0;
         pPendData->pPendAuthorizationData = 0;
         pPendData->isPended               = FALSE;
      }
   }

   RtlZeroMemory(pPendData,
                 sizeof(PEND_DATA));

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprPendDataPurge()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprPendDataDestroy"
 
   Purpose:  Cleanup and free a PEND_DATA object.                                               <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(ppPendData, _Pre_ _Notnull_)
_At_(*ppPendData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppPendData == 0)
inline VOID KrnlHlprPendDataDestroy(_Inout_ PEND_DATA** ppPendData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprPendDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppPendData);

   if(*ppPendData)
   {
      KrnlHlprPendDataPurge(*ppPendData);

      HLPR_DELETE(*ppPendData,
                  WFPSAMPLER_SYSLIB_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprPendDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @kernel_helper_function="KrnlHlprPendDataPopulate"
 
   Purpose:  Populates a PEND_DATA object with the completionContext.                           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/ff551199.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprPendDataPopulate(_Inout_ PEND_DATA* pPendData,
                                  _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                  _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                  _In_opt_ NET_BUFFER_LIST* pNBL,
                                  _In_ const FWPS_FILTER* pFilter,
                                  _In_opt_ VOID* pClassifyContext,                         /* 0 */
                                  _In_opt_ FWPS_CLASSIFY_OUT* pClassifyOut)                /* 0 */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprPendDataPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pPendData);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);

   NTSTATUS status = STATUS_SUCCESS;

   pPendData->layerID = pClassifyValues->layerId;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   if(pPendData->layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
      pPendData->layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
   {
      NT_ASSERT(pClassifyContext);
      NT_ASSERT(pClassifyOut);

      if(pClassifyContext &&
         pClassifyOut)
      {
         status = FwpsAcquireClassifyHandle(pClassifyContext,
                                            0,
                                            &(pPendData->classifyHandle));
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprPendDataPopulate : FwpsAcquireClassifyHandle() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         status = FwpsPendClassify(pPendData->classifyHandle,
                                   pFilter->filterId,
                                   0,
                                   pClassifyOut);
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprPendDataPopulate : FwpsPendClassify() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         RtlCopyMemory(&(pPendData->classifyOut),
                       pClassifyOut,
                       sizeof(FWPS_CLASSIFY_OUT));

         pPendData->pPCPendData = pFilter->providerContext->dataBuffer->data;
         pPendData->isPended    = TRUE;
      }
      else
      {
         status = STATUS_INVALID_PARAMETER;

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprPendDataPopulate : [status: %#x][pClassifyContext: %#p][pClassifyOut: %#p]\n",
                    status,
                    pClassifyContext,
                    pClassifyOut);

         HLPR_BAIL;
      }
   }
   else

#else

   UNREFERENCED_PARAMETER(pClassifyContext);
   UNREFERENCED_PARAMETER(pClassifyOut);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   {


      if(FWPS_IS_METADATA_FIELD_PRESENT(pMetadata,
                                        FWPS_METADATA_FIELD_COMPLETION_HANDLE))
      {
         status = FwpsPendOperation(pMetadata->completionHandle,
                                    &(pPendData->completionContext));
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprPendDataPopulate : FwpsPendOperation() [status: %#x]\n",
                       status);

            HLPR_BAIL;
         }

         pPendData->pNBL        = pNBL;
         pPendData->pPCPendData = pFilter->providerContext->dataBuffer->data;
         pPendData->isPended    = TRUE;
      }
      else
      {
         status = STATUS_INVALID_HANDLE;

         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprPendDataPopulate() [status: %#x]\n",
                    status);
      }
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprPendDataPurge(pPendData);


#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprPendDataPopulate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

/**
 @kernel_helper_function="KrnlHlprPendDataCreate"
 
   Purpose:  Allocates and populates a PEND_DATA object with the completionContext.             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppPendData, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppPendData, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppPendData, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprPendDataCreate(_Outptr_ PEND_DATA** ppPendData,
                                _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                _In_opt_ NET_BUFFER_LIST* pNBL,
                                _In_ const FWPS_FILTER* pFilter,
                                 _In_opt_ VOID* pClassifyContext,                         /* 0 */
                                 _In_opt_ FWPS_CLASSIFY_OUT* pClassifyOut)                /* 0 */


{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprPendDataCreate()\n");

#endif /// DBG
   
   NT_ASSERT(ppPendData);
   NT_ASSERT(pClassifyValues);
   NT_ASSERT(pMetadata);
   NT_ASSERT(pFilter);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppPendData,
            PEND_DATA,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppPendData,
                              status);

   status = KrnlHlprPendDataPopulate(*ppPendData,
                                     pClassifyValues,
                                     pMetadata,
                                     pNBL,
                                     pFilter,
                                     pClassifyContext,
                                     pClassifyOut);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppPendData initialized with calls to HLPR_NEW & KrnlHlprPendDataPopulate 

   if(status != STATUS_SUCCESS &&
      *ppPendData)
      KrnlHlprPendDataDestroy(ppPendData);

#pragma warning(pop)

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprPendDataCreate() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

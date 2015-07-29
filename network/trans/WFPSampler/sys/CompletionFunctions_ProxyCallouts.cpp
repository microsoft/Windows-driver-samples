////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_ProxyCallouts.cpp
//
//   Abstract:
//      This module contains WFP Completion functions for proxied connections using the injection 
//         clone / drop / modify /inject model.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       CompleteProxyInjection
//
//       <Module>
//          Complete                     - Function is an FWPS_INJECT_COMPLETE function.
//       <Scenario>
//          ProxyInjection               - Function demonstrates the clone / block / modify / inject 
//                                            model for proxying.
//
//      <Object><Action>
//
//      i.e.
//       ProxyCompletionDataDestroy
//
//       <Object>
//        {
//          ProxyCompletionData          - pertains to PROXY_COMPLETION_DATA.
//        }
//       <Action>
//          Destroy                      - Cleans up and frees all memory in the object.
//
//   Private Functions:
//      ProxyInjectionCompletionDataDestroy(),
//
//   Public Functions:
//      CompleteProxyInjection(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense and 
//                                              enhance traces.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"   /// . 
#include "CompletionFunctions_ProxyCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_function="ProxyCompletionDataDestroy"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_At_(*ppCompletionData, _Pre_ _Notnull_)
_At_(*ppCompletionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppCompletionData == 0)
VOID ProxyCompletionDataDestroy(_Inout_ PROXY_COMPLETION_DATA** ppCompletionData)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ProxyCompletionDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppCompletionData);
   NT_ASSERT(*ppCompletionData);

   PROXY_COMPLETION_DATA* pCompletionData = *ppCompletionData;


   if(pCompletionData->pClassifyData)
   {
      if(!pCompletionData->performedInline)
         KrnlHlprClassifyDataDestroyLocalCopy(&(pCompletionData->pClassifyData));
      else
      {
         HLPR_DELETE(pCompletionData->pClassifyData,
                     WFPSAMPLER_CALLOUT_DRIVER_TAG);
      }
   }

   if(pCompletionData->pInjectionData)
      KrnlHlprInjectionDataDestroy(&(pCompletionData->pInjectionData));

   if(pCompletionData->pSendParams)
   {
      HLPR_DELETE_ARRAY(pCompletionData->pSendParams->remoteAddress,
                        WFPSAMPLER_CALLOUT_DRIVER_TAG);

      HLPR_DELETE(pCompletionData->pSendParams,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   HLPR_DELETE(pCompletionData,
               WFPSAMPLER_CALLOUT_DRIVER_TAG);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ProxyCompletionDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @completion_function="CompleteProxyInjection"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Hardware/FF545018.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteProxyInjection(_In_ VOID* pContext,
                                  _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                  _In_ BOOLEAN dispatchLevel)
{
   NT_ASSERT(pContext);
   NT_ASSERT(((PROXY_COMPLETION_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((PROXY_COMPLETION_DATA*)pContext)->pClassifyData->pClassifyValues);
   NT_ASSERT(((PROXY_COMPLETION_DATA*)pContext)->pClassifyData->pClassifyOut);
   NT_ASSERT(((PROXY_COMPLETION_DATA*)pContext)->pClassifyData->pFilter);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(NT_SUCCESS(pNetBufferList->Status));

   UNREFERENCED_PARAMETER(dispatchLevel);

   NTSTATUS               status          = pNetBufferList->Status;
   PROXY_COMPLETION_DATA* pCompletionData = (PROXY_COMPLETION_DATA*)pContext;
   UINT32                 layerID         = pCompletionData->pClassifyData->pClassifyValues->layerId;
   UINT64                 filterID        = pCompletionData->pClassifyData->pFilter->filterId;

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> CompleteProxyInjection() [Layer: %s][FilterID: %#I64x][NBL->Status: %#x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              filterID,
              status);

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! CompleteProxyInjection() [status: %#x]\n",
                 status);

   FwpsFreeCloneNetBufferList(pNetBufferList,
                              0);

   ProxyCompletionDataDestroy(&pCompletionData);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- CompleteProxyInjection() [Layer: %s][FilterID: %#I64x][NBL->Status: %#x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              filterID,
              status);

   return;
}

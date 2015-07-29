////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_BasicPacketInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Completion functions for injecting packets back into the data path 
//         using the clone / block / inject method for modified NBLs.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       CompleteBasicPacketModification
//
//       <Module>
//          Complete                           - Function is an FWPS_INJECT_COMPLETE function.
//       <Scenario>
//          BasicPacketModification            - Function demonstrates the clone / block / inject 
//                                                model for modified NBLs.
//
//      <Object><Action>
//
//      i.e.
//       BasicPacketModificationCompletionDataDestroy
//
//       <Object>
//        {
//          BasicPacketModificationCompletionData - pertains to 
//                                                     BASIC_PACKET_MODIFICATION_COMPLETION_DATA.
//        }
//       <Action>
//          Destroy                               - Cleans up and frees all memory in the object.
//
//   Private Functions:
//      BasicPacketModificationCompletionDataDestroy(),
//
//   Public Functions:
//      CompleteBasicPacketModification(),
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

#include "Framework_WFPSamplerCalloutDriver.h"                     /// . 
#include "CompletionFunctions_BasicPacketModificationCallouts.tmh" /// $(OBJ_PATH)\$(O)\ 

/**
 @private_function="BasicPacketModificationCompletionDataDestroy"
 
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
VOID BasicPacketModificationCompletionDataDestroy(_Inout_ BASIC_PACKET_MODIFICATION_COMPLETION_DATA** ppCompletionData,
                                                  _In_ BOOLEAN override)                                                /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicPacketModificationCompletionDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppCompletionData);
   NT_ASSERT(*ppCompletionData);

   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = *ppCompletionData;
   KIRQL                                      originalIRQL    = PASSIVE_LEVEL;
   INT32                                      refCount        = -1;

   refCount = InterlockedDecrement((LONG*)&(pCompletionData->refCount));
   if(refCount == 0 ||
      override)
   {
      KeAcquireSpinLock(&(pCompletionData->spinLock),
                        &originalIRQL);

      pCompletionData->refCount = -1;

      if(pCompletionData->pClassifyData)
      {
         if(!(pCompletionData->performedInline))
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

      KeReleaseSpinLock(&(pCompletionData->spinLock),
                        originalIRQL);

      HLPR_DELETE(*ppCompletionData,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicPacketModificationCompletionDataDestroy()\n");

#endif /// DBG
   
   return;
}

/**
 @completion_function="CompleteBasicPacketModification"
 
   Purpose:                                                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Hardware/FF545018.aspx             <br>
             HTTP://MSDN.Microsoft.com/En-Us/Library/Windows/Hardware/FF551170.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteBasicPacketModification(_In_ VOID* pContext,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ BOOLEAN dispatchLevel)
{
   NT_ASSERT(pContext);
   NT_ASSERT(((BASIC_PACKET_MODIFICATION_COMPLETION_DATA*)pContext)->pClassifyData);
   NT_ASSERT(((BASIC_PACKET_MODIFICATION_COMPLETION_DATA*)pContext)->pClassifyData->pClassifyValues);
   NT_ASSERT(((BASIC_PACKET_MODIFICATION_COMPLETION_DATA*)pContext)->pClassifyData->pClassifyOut);
   NT_ASSERT(((BASIC_PACKET_MODIFICATION_COMPLETION_DATA*)pContext)->pClassifyData->pFilter);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(NT_SUCCESS(pNetBufferList->Status));

   UNREFERENCED_PARAMETER(dispatchLevel);

   NTSTATUS                                   status          = pNetBufferList->Status;
   BASIC_PACKET_MODIFICATION_COMPLETION_DATA* pCompletionData = (BASIC_PACKET_MODIFICATION_COMPLETION_DATA*)pContext;
   UINT32                                     layerID         = pCompletionData->pClassifyData->pClassifyValues->layerId;
   UINT64                                     filterID        = pCompletionData->pClassifyData->pFilter->filterId;

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> CompleteBasicPacketModification() [Layer: %s][FilterID: %#I64x][NBL->Status: %#x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              filterID,
              status);

   BasicPacketModificationCompletionDataDestroy(&pCompletionData);

   if(status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! CompleteBasicPacketModification() [status: %#x]\n",
                 pNetBufferList->Status);

   FwpsFreeCloneNetBufferList(pNetBufferList,
                              0);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- CompleteBasicPacketModification() [Layer: %s][FilterID: %#I64x][NBL->Status: %#x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              filterID,
              status);

   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      CompletionFunctions_BasicStreamInjectionCallouts.cpp
//
//   Abstract:
//      This module contains WFP Completion functions for injecting packets back into the stream 
//         using the clone / block / inject method.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//       CompleteBasicStreamInjection
//
//       <Module>
//          Complete                           - Function is an FWPS_INJECT_COMPLETE function.
//       <Scenario>
//          BasicStreamInjection               - Function demonstrates the clone / block / inject 
//                                               model.
//
//      <Object><Action>
//
//      i.e.
//       BasicPacketInjectionsCompletionDataDestroy
//
//       <Object>
//        {
//          BasicStreamInjectionCompletionData - pertains to BASIC_STREAM_INJECTION_COMPLETION_DATA.
//        }
//       <Action>
//          Destroy                            - Cleans up and frees all memory in the object.
//
//   Private Functions:
//      BasicStreamInjectionCompletionDataDestroy(),
//
//   Public Functions:
//      CompleteBasicStreamInjection(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense 
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h"                  /// .
#include "CompletionFunctions_BasicStreamInjectionCallouts.tmh" /// $(OBJ_PATH)\$(O)\

_At_(*ppCompletionData, _Pre_ _Notnull_)
_At_(*ppCompletionData, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(*ppCompletionData == 0)
VOID BasicStreamInjectionCompletionDataDestroy(_Inout_ BASIC_STREAM_INJECTION_COMPLETION_DATA** ppCompletionData,
                                               _In_ BOOLEAN override)                                             /* FALSE */
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> BasicStreamInjectionCompletionDataDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppCompletionData);
   NT_ASSERT(*ppCompletionData);

   BASIC_STREAM_INJECTION_COMPLETION_DATA* pCompletionData = *ppCompletionData;
   KIRQL                                   originalIRQL    = PASSIVE_LEVEL;

   KeAcquireSpinLock(&(pCompletionData->spinLock),
                     &originalIRQL);

   pCompletionData->refCount--;

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

   KeReleaseSpinLock(&(pCompletionData->spinLock),
                     originalIRQL);

   /// Stream indicated each individual NBL from a chain, so wait until the last NBL is indicated 
   /// before removing the completionData.
   if(pCompletionData->refCount == 0 ||
      override)
   {
      HLPR_DELETE(*ppCompletionData,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- BasicStreamInjectionCompletionDataDestroy()\n");

#endif /// DBG
   
   return;
}

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI CompleteBasicStreamInjection(_In_ VOID* pContext,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ BOOLEAN dispatchLevel)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> CompleteBasicStreamInjection()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(dispatchLevel);

   NT_ASSERT(pContext);
   NT_ASSERT(pNetBufferList);
   NT_ASSERT(NT_SUCCESS(pNetBufferList->Status) ||
             pNetBufferList->Status == STATUS_CONNECTION_ABORTED);

   if(pNetBufferList->Status != STATUS_SUCCESS)
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! CompleteBasicStreamInjection() [status: %#x]\n",
                 pNetBufferList->Status);

   FwpsFreeCloneNetBufferList(pNetBufferList,
                              0);

   BasicStreamInjectionCompletionDataDestroy((BASIC_STREAM_INJECTION_COMPLETION_DATA**)&pContext);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- CompleteBasicStreamInjection()\n");

#endif /// DBG
   
   return;
}

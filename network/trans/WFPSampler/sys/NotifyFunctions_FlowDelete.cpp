////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_FlowDelete.cpp
//
//   Abstract:
//      This module contains WFP Flow Delete Notification functions.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       FlowDeleteNotification
//
//       <Module>
//          FlowDelete        -       Function is an FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN
//       <Scenario>
//          Notification      -       Function demonstates use of the simple notifications
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      December  13,   2013  -     1.1   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h" /// .
#include "NotifyFunctions_FlowDelete.tmh"      /// $(OBJ_PATH)\$(O)\ 

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID NTAPI NotifyFlowDeleteNotification(_In_ UINT16 layerID,
                                        _In_ UINT32 calloutID,
                                        _In_ UINT64 flowContext)
{
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> NotifyFlowDeleteNotification() [Layer: %s][CalloutID: %#I64x][FlowContext: %#I64x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              calloutID,
              flowContext);

   UNREFERENCED_PARAMETER(calloutID);

   if(flowContext)
   {
      FLOW_CONTEXT* pFlowContext = (FLOW_CONTEXT*)flowContext;

      if(layerID == FWPS_LAYER_STREAM_V4 ||
         layerID == FWPS_LAYER_STREAM_V6)
      {



      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      else if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
              layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
      {
         PEND_DATA* pPendData = (PEND_DATA*)pFlowContext->pALEEndpointClosureContext->pPendData;
         KIRQL      irql      = PASSIVE_LEVEL;

         KeAcquireSpinLock(&(pFlowContext->pALEEndpointClosureContext->spinLock),
                           &irql);
         
         KrnlHlprPendDataDestroy(&pPendData);

         KeReleaseSpinLock(&(pFlowContext->pALEEndpointClosureContext->spinLock),
                           irql);
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      KrnlHlprFlowContextDestroy(&pFlowContext);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- NotifyFlowDeleteNotification() [Layer: %s][CalloutID: %#I64x][FlowContext: %#I64x]",
              KrnlHlprFwpsLayerIDToString(layerID),
              calloutID,
              flowContext);

   return;
}

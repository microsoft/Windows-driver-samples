////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FlowContext.cpp
//
//   Abstract:
//      This module contains kernel helper functions that assist with FLOW_CONTEXT.
//
//   Naming Convention:
//
//      <Module><Object><Action><Modifier>
//  
//      i.e.
//
//       KrnlHlprFlowContextCreate
//
//       <Module>
//          KrnlHlpr     -       Function is located in syslib\ and applies to kernel mode.
//       <Object>
//          FlowContext  -       Function pertains to FLOW_CONTEXT objects.
//       <Action>
//          {
//            Create     -       Function allocates and fills memory.
//            Destroy    -       Function cleans up and frees memory.
//          }
//       <Modifer>
//
//   Private Functions:
//
//   Public Functions:
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

#include "HelperFunctions_Include.h"        /// .
#include "HelperFunctions_FlowContext.tmh"  /// $(OBJ_PATH)\$(O)\ 

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
inline NTSTATUS KrnlHlprFlowContextPurge(_Inout_ FLOW_CONTEXT* pFlowContext)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFlowContextPurge()\n");

#endif /// DBG
   
   NT_ASSERT(pFlowContext);

   NTSTATUS status = STATUS_SUCCESS;   

   status = FwpsFlowRemoveContext(pFlowContext->flowID,
                                  pFlowContext->layerID,
                                  pFlowContext->calloutID);
   if(status != STATUS_SUCCESS)
   {
      if(status != STATUS_UNSUCCESSFUL)
      {
         DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                    DPFLTR_ERROR_LEVEL,
                    " !!!! KrnlHlprFlowContextPurge : FwpsFlowRemoveContext() [status: %#x]\n",
                    status);

          HLPR_BAIL;
      }

      /// no context currently associated with the flow, so it should be safe to delete
   }

   HLPR_DELETE(pFlowContext->pContext,
               WFPSAMPLER_SYSLIB_TAG);

   RtlZeroMemory(pFlowContext,
                 sizeof(FLOW_CONTEXT));

   HLPR_BAIL_LABEL:

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFlowContextPurge()\n");

#endif /// DBG

   return status;
}

_At_(*ppFlowContext, _Pre_ _Notnull_)
_At_(*ppFlowContext, _Post_ _Null_ __drv_freesMem(Pool))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS && *ppFlowContext == 0)
inline NTSTATUS KrnlHlprFlowContextDestroy(_Inout_ FLOW_CONTEXT** ppFlowContext)
{
#if DBG
      
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFlowContextDestroy()\n");

#endif /// DBG
   
   NT_ASSERT(ppFlowContext);

   NTSTATUS status = STATUS_SUCCESS;

   if(*ppFlowContext)
   {
      status = KrnlHlprFlowContextPurge(*ppFlowContext);
      HLPR_BAIL_ON_FAILURE(status);

      HLPR_DELETE(*ppFlowContext,
                  WFPSAMPLER_SYSLIB_TAG);
   }

   HLPR_BAIL_LABEL:

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFlowContextDestroy()\n");

#endif /// DBG

   return status;
}

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFlowContextPopulate(_Inout_ FLOW_CONTEXT* pFlowContext,
                                     _In_ UINT64 flowHandle,
                                     _In_ UINT16 layerID,
                                     _In_ UINT32 calloutID,
                                     _In_ UINT8 contextType,                /* CONTEXT_TYPE_DEFAULT */
                                     _In_opt_ const VOID* pProviderContext) /* 0 */

{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFlowContextPopulate()\n");

#endif /// DBG
   
   NT_ASSERT(pFlowContext);

   NTSTATUS status = STATUS_SUCCESS;

   pFlowContext->flowID      = flowHandle;
   pFlowContext->layerID     = layerID;
   pFlowContext->calloutID   = calloutID;
   pFlowContext->contextType = contextType;

   if(pFlowContext->contextType == CONTEXT_TYPE_DEFAULT ||
      pFlowContext->contextType >= CONTEXT_TYPE_MAX)
   {
      if(layerID == FWPS_LAYER_STREAM_V4 ||
         layerID == FWPS_LAYER_STREAM_V6)
         pFlowContext->contextType = CONTEXT_TYPE_STREAM;

#if(NTDDI_VERSION >= NTDDI_WIN7)

      else if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
              layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
         pFlowContext->contextType = CONTEXT_TYPE_ALE_ENDPOINT_CLOSURE;

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

      else
      {
         status = STATUS_INVALID_PARAMETER;

         HLPR_BAIL;
      }
   }

   switch(pFlowContext->contextType)
   {
      case CONTEXT_TYPE_STREAM:
      {
         HLPR_NEW(pFlowContext->pStreamContext,
                  STREAM_CONTEXT,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pFlowContext->pStreamContext,
                                    status);
      
         KeInitializeSpinLock(&(pFlowContext->pStreamContext->serializationList.spinLock));

         status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES,
                                    &(pFlowContext->pStreamContext->serializationList.waitLock));
         if(status != STATUS_SUCCESS)
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! KrnlHlprFlowContextCreate : WdfWaitLockCreate() [status: %#x]\n",
                       status);
         
            HLPR_BAIL;
         }

         InitializeListHead(&(pFlowContext->pStreamContext->serializationList.listHead));

         pFlowContext->pStreamContext->processorID = KeGetCurrentProcessorNumber();

#if(NTDDI_VERSION >= NTDDI_WIN7)

         if(pProviderContext)
         {
            PC_FLOW_ASSOCIATION_DATA* pFlowAssociationData = (PC_FLOW_ASSOCIATION_DATA*)pProviderContext;

            for(UINT32 aecIndex = 0;
                aecIndex < pFlowAssociationData->itemCount;
                aecIndex++)
            {
               if(pFlowAssociationData->pLayerIDs[aecIndex] == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
                  pFlowAssociationData->pLayerIDs[aecIndex] == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
               {
                  pFlowContext->aecLayerID   = pFlowAssociationData->pLayerIDs[aecIndex];
                  pFlowContext->aecCalloutID = pFlowAssociationData->pCalloutIDs[aecIndex];

                  break;
               }
            }
         }

#else

         UNREFERENCED_PARAMETER(pProviderContext);

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

         break;
      }

#if(NTDDI_VERSION >= NTDDI_WIN7)

      case CONTEXT_TYPE_ALE_ENDPOINT_CLOSURE:
      {
         HLPR_NEW(pFlowContext->pALEEndpointClosureContext,
                  ALE_ENDPOINT_CLOSURE_CONTEXT,
                  WFPSAMPLER_SYSLIB_TAG);
         HLPR_BAIL_ON_ALLOC_FAILURE(pFlowContext->pALEEndpointClosureContext,
                                    status);

         KeInitializeSpinLock(&(pFlowContext->pALEEndpointClosureContext->spinLock));

         if(pProviderContext)
         {
            PC_FLOW_ASSOCIATION_DATA* pFlowAssociationData = (PC_FLOW_ASSOCIATION_DATA*)pProviderContext;

            for(UINT32 injectionIndex = 0;
                injectionIndex < pFlowAssociationData->itemCount;
                injectionIndex++)
            {
               if(pFlowAssociationData->pLayerIDs[injectionIndex] == FWPS_LAYER_STREAM_V4 ||
                  pFlowAssociationData->pLayerIDs[injectionIndex] == FWPS_LAYER_STREAM_V6)
               {
                  pFlowContext->injectionLayerID   = pFlowAssociationData->pLayerIDs[injectionIndex];
                  pFlowContext->injectionCalloutID = pFlowAssociationData->pCalloutIDs[injectionIndex];

                  break;
               }
            }
         }

         break;
      }

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

   }

   status = FwpsFlowAssociateContext(pFlowContext->flowID,
                                     pFlowContext->layerID,
                                     pFlowContext->calloutID,
                                     (UINT64)pFlowContext);
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! KrnlHlprFlowContextCreate : FwpsFlowAssociateContext() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
      KrnlHlprFlowContextPurge(pFlowContext);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFlowContextPopulate() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

_At_(*ppFlowContext, _Pre_ _Null_)
_When_(return != STATUS_SUCCESS, _At_(*ppFlowContext, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppFlowContext, _Post_ _Notnull_ __drv_allocatesMem(Pool)))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprFlowContextCreate(_Outptr_ FLOW_CONTEXT** ppFlowContext,
                                   _In_ UINT64 flowHandle,
                                   _In_ UINT16 layerID,
                                   _In_ UINT32 calloutID,
                                   _In_ UINT8 contextType,                /* CONTEXT_TYPE_DEFAULT */
                                   _In_opt_ const VOID* pProviderContext) /* 0 */
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> KrnlHlprFlowContextCreate()\n");

#endif /// DBG

   NT_ASSERT(ppFlowContext);

   NTSTATUS status = STATUS_SUCCESS;

   HLPR_NEW(*ppFlowContext,
            FLOW_CONTEXT,
            WFPSAMPLER_SYSLIB_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(*ppFlowContext,
                              status);

   status = KrnlHlprFlowContextPopulate(*ppFlowContext,
                                        flowHandle,
                                        layerID,
                                        calloutID,
                                        contextType,
                                        pProviderContext);

   HLPR_BAIL_LABEL:

#pragma warning(push)
#pragma warning(disable: 6001) /// *ppFlowContext initialized with call to HLPR_NEW & KrnlHlprFlowContextPopulate

   if(status != STATUS_SUCCESS &&
      *ppFlowContext)
      KrnlHlprFlowContextDestroy(ppFlowContext);

#pragma warning(pop)

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- KrnlHlprFlowContextCreate() [status: %#x]\n",
              status);

#endif /// DBG

   return status;
}

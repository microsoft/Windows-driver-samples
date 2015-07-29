////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      NotifyFunctions_ProxyCallouts.h
//
//   Abstract:
//      This module contains WFP Notify functions for the proxy callouts using WFP's REDIRECT layers.
//
//   Naming Convention:
//
//      <Module><Scenario>
//  
//      i.e.
//
//       NotifyProxyByALERedirectNotification
//
//       <Module>
//          Notify                            -       Function is an FWPS_CALLOUT_NOTIFY_FN
//       <Scenario>
//          ProxyByALERedirectNotification    -       Function demonstates use of notifications for 
//                                                       callouts using WFP's REDIRECT layers
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance function declaration for IntelliSense and 
//                                              improve traces
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h" /// . 
#include "NotifyFunctions_ProxyCallouts.tmh"   /// $(OBJ_PATH)\$(O)\ 

#if(NTDDI_VERSION >= NTDDI_WIN7)

/**
 @private_function="PrvProxyByALERedirectNotificationWorkItemRoutine"
 
   Purpose:  Traces the appropriate notification event.                                         <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID PrvProxyByALERedirectNotificationWorkItemRoutine(_In_ PDEVICE_OBJECT pDeviceObject,
                                                      _Inout_opt_ PVOID pContext)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PrvProxyByALERedirectNotificationWorkItemRoutine()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pDeviceObject);

   NT_ASSERT(pContext);
   NT_ASSERT(((WORKITEM_DATA*)pContext)->pNotifyData);

   WORKITEM_DATA* pWorkItemData = (WORKITEM_DATA*)pContext;

   if(pWorkItemData)
   {
      NTSTATUS      status       = STATUS_SUCCESS;
      FWPM_CALLOUT* pCallout     = 0;
      PWSTR         pCalloutName = L"";

      status = FwpmCalloutGetById(g_EngineHandle,
                                  pWorkItemData->pNotifyData->calloutID,
                                  &pCallout);
      if(status != STATUS_SUCCESS)
      {
         if(status != STATUS_FWP_CALLOUT_NOT_FOUND)
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! [FilterID: %#I64x][CalloutID: %#x] PrvProxyByALERedirectNotificationWorkItemRoutine : FwpmCalloutGetById() [status: %#x]\n",
                       pWorkItemData->pNotifyData->filterID,
                       pWorkItemData->pNotifyData->calloutID,
                       status);
      }
      else
      {
         pCalloutName = pCallout->displayData.name;

         if(pCallout->applicableLayer != FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 &&
            pCallout->applicableLayer != FWPM_LAYER_ALE_CONNECT_REDIRECT_V6 &&
            pCallout->applicableLayer != FWPM_LAYER_ALE_BIND_REDIRECT_V4 &&
            pCallout->applicableLayer != FWPM_LAYER_ALE_BIND_REDIRECT_V6)
         {
            status = STATUS_FWP_INCOMPATIBLE_LAYER;

            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_ERROR_LEVEL,
                       " !!!! [FilterID: %#I64x][CalloutID: %#x] PrvProxyByALERedirectNotificationWorkItemRoutine() [status: %#x]\n",
                       pWorkItemData->pNotifyData->filterID,
                       pWorkItemData->pNotifyData->calloutID,
                       status);
         }
      }

      switch(pWorkItemData->pNotifyData->notificationType)
      {
         case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "   -- [FilterID: %#I64x][CalloutID: %#x] A filter referencing %S callout was added\n",
                       pWorkItemData->pNotifyData->filterID,
                       pWorkItemData->pNotifyData->calloutID,
                       pCalloutName ? pCalloutName : L"");

            break;
         }
         case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "   -- [FilterID: %#I64x][CalloutID: %#x] A filter referencing %S callout was deleted\n",
                       pWorkItemData->pNotifyData->filterID,
                       pWorkItemData->pNotifyData->calloutID,
                       pCalloutName ? pCalloutName : L"");

            break;
         }
         case FWPS_CALLOUT_NOTIFY_ADD_FILTER_POST_COMMIT:
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "   -- [FilterID: %#I64x][CalloutID: %#x] A filter referencing %S callout was committed\n",
                       pWorkItemData->pNotifyData->filterID,
                       pWorkItemData->pNotifyData->calloutID,
                       pCalloutName ? pCalloutName : L"");

            break;
         }
         default:
         {
            DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                       DPFLTR_INFO_LEVEL,
                       "   -- [FilterID: %#I64x][CalloutID: %#x] Invalid Notification Type.  Please Contact WFP@Microsoft.com\n",
                       pWorkItemData->pNotifyData->filterID,
                       pWorkItemData->pNotifyData->calloutID);

            break;
         }
      }

      FwpmFreeMemory((VOID**)&pCallout);

      HLPR_DELETE(pWorkItemData->pNotifyData,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);

      KrnlHlprWorkItemDataDestroy(&pWorkItemData);
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PrvProxyByALERedirectNotificationWorkItemRoutine()\n");

#endif /// DBG
   
   return;
}

/**
 @notify_function="NotifyProxyByALERedirectNotification"
 
   Purpose:  Traces the notification event.                                                     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF568803.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF568804.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS NTAPI NotifyProxyByALERedirectNotification(_In_ FWPS_CALLOUT_NOTIFY_TYPE notificationType,
                                                    _In_ const GUID* pFilterKey,
                                                    _Inout_ FWPS_FILTER* pFilter)
{
   NT_ASSERT(pFilter);

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> NotifyProxyByALERedirectNotification() [FilterID: %#I64x][CalloutID: %#x]\n",
              pFilter->filterId,
              pFilter->action.calloutId);

   NTSTATUS     status      = STATUS_SUCCESS;
   NOTIFY_DATA* pNotifyData = 0;

#pragma warning(push)
#pragma warning(disable: 6014) /// pNotifyData is expected to be cleaned up by caller using PrvProxyByALERedirectNotificationWorkItemRoutine

   HLPR_NEW(pNotifyData,
            NOTIFY_DATA,
            WFPSAMPLER_CALLOUT_DRIVER_TAG);
   HLPR_BAIL_ON_ALLOC_FAILURE(pNotifyData,
                              status);

#pragma warning(pop)

   pNotifyData->notificationType = notificationType;
   pNotifyData->calloutID        = pFilter ? pFilter->action.calloutId : 0;
   pNotifyData->filterID         = pFilter ? pFilter->filterId : 0;
   pNotifyData->pFilterKey       = pFilterKey;

   status = KrnlHlprWorkItemQueue(g_pWDMDevice,
                                  PrvProxyByALERedirectNotificationWorkItemRoutine,
                                  pNotifyData);

   HLPR_BAIL_LABEL:

   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! NotifyProxyByALERedirectNotification() [status: %#x]\n",
                 status);

      HLPR_DELETE(pNotifyData,
                  WFPSAMPLER_CALLOUT_DRIVER_TAG);
   }

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- NotifyProxyByALERedirectNotification() [FilterID: %#I64x][CalloutID: %#x]\n",
              pFilter->filterId,
              pFilter->action.calloutId);

   return status;
}

#endif // (NTDDI_VERSION >= NTDDI_WIN7)

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_PowerStates.cpp
//
//   Abstract:
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

#include "Framework_WFPSamplerCalloutDriver.h" /// .
#include "Framework_PowerStates.tmh"           /// $(OBJ_PATH)\$(O)\

KGUARDED_MUTEX guardedMutex = {0};

/**
 @framework_function="ActOnPowerStateEnter"

   Purpose:  Passive function to handle entering a power managed state.  Prior to dropping into a power managed   <br>
             state, unregister the callouts.  This prevents the driver from potentially taking any more           <br>
             references on NBLs, and instead causes the filters to return BLOCK.  This should also allow adequate <br>
             time to drain currently queued NBLs.                                                                 <br>
                                                                                                                  <br>
   Notes:                                                                                                         <br>    
                                                                                                                  <br>
   MSDN_Ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nc-wdm-io_workitem_routine <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID ActOnPowerStateEnter(_In_ PDEVICE_OBJECT pDeviceObject,
                          _Inout_opt_ PVOID pContext)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ActOnPowerStateEnter()\n");

#endif /// DBG

   UNREFERENCED_PARAMETER(pDeviceObject);
   UNREFERENCED_PARAMETER(pContext);

   KeAcquireGuardedMutex(&guardedMutex);

   if(g_calloutsRegistered == TRUE)
   {
      NTSTATUS status = STATUS_SUCCESS;

      status = KrnlHlprExposedCalloutsUnregister();
      HLPR_BAIL_ON_FAILURE(status);

      g_calloutsRegistered = FALSE;
   }

   HLPR_BAIL_LABEL:

   KeReleaseGuardedMutex(&guardedMutex);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ActOnPowerStateEnter()\n");

#endif /// DBG

   return;
}

/**
 @framework_function="ActOnPowerStateExit"

   Purpose:  Passive function to handle exiting a power managed state.  When coming out of a power managed state, <br>
             register the callouts so normal processing of NBLs will commence.                                    <br>
                                                                                                                  <br>
   Notes:                                                                                                         <br>    
                                                                                                                  <br>
   MSDN_Ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nc-wdm-io_workitem_routine <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(IO_WORKITEM_ROUTINE)
VOID ActOnPowerStateExit(_In_ PDEVICE_OBJECT pDeviceObject,
                         _Inout_opt_ PVOID pContext)
{
#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> ActOnPowerStateExit()\n");

#endif /// DBG

   UNREFERENCED_PARAMETER(pDeviceObject);
   UNREFERENCED_PARAMETER(pContext);

   KeAcquireGuardedMutex(&guardedMutex);

   if(g_calloutsRegistered == FALSE)
   {
      NTSTATUS status = STATUS_SUCCESS;

      status = KrnlHlprExposedCalloutsRegister();
      HLPR_BAIL_ON_FAILURE(status);

      g_calloutsRegistered = TRUE;
   }

   HLPR_BAIL_LABEL:

   KeReleaseGuardedMutex(&guardedMutex);

#if DBG

   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- ActOnPowerStateExit()\n");

#endif /// DBG

   return;
}

/**
 @framework_function="PowerStateCallback"
 
   Purpose:  Callback which handles various power state transitions for the callout driver.     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
_IRQL_requires_same_
_Function_class_(CALLBACK_FUNCTION)
VOID PowerStateCallback(_In_ VOID* pCallbackContext,
                        _In_ VOID* pPowerStateEvent,
                        _In_ VOID* pEventSpecifics)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> PowerStateCallback()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(pCallbackContext);

   if(pPowerStateEvent == (VOID*)PO_CB_SYSTEM_STATE_LOCK)
   {
      if(pEventSpecifics)
      {
         /// entering the ON state (S0), so return operation to normal
         IoQueueWorkItem(g_pPowerStateExitIOWorkItem,
                         ActOnPowerStateExit,
                         DelayedWorkQueue,
                         0);
      }
      else
      {
         IoQueueWorkItem(g_pPowerStateEnterIOWorkItem,
                         ActOnPowerStateEnter,
                         DelayedWorkQueue,
                         0);
      }
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- PowerStateCallback()\n");

#endif /// DBG
   
   return;
}

/**
 @framework_function="UnregisterPowerStateChangeCallback"
 
   Purpose:  Unregister a callback that handled notifications of power state changes.           <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF545649.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF547724.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
VOID UnregisterPowerStateChangeCallback(_Inout_ DEVICE_EXTENSION* pDeviceExtension)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> UnregisterPowerStateChangeCallback()\n");

#endif /// DBG
   
   NT_ASSERT(pDeviceExtension);

   if(pDeviceExtension->pRegistration)
   {
      ExUnregisterCallback(pDeviceExtension->pRegistration);

      pDeviceExtension->pRegistration = 0;
   }

   if(pDeviceExtension->pCallbackObject)
   {
      ObDereferenceObject(pDeviceExtension->pCallbackObject);

      pDeviceExtension->pCallbackObject = 0;
   }

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- UnregisterPowerStateChangeCallback()\n");

#endif /// DBG
   
   return;
}

/**
 @framework_function="RegisterPowerStateChangeCallback"
 
   Purpose:  Create and register a callback to handle notifications of power state changes.     <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/FF544560.aspx                              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/FF545534.aspx                              <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS RegisterPowerStateChangeCallback(_Inout_ DEVICE_EXTENSION* pDeviceExtension)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> RegisterPowerStateChangeCallback()\n");

#endif /// DBG
   
   NT_ASSERT(pDeviceExtension);

   NTSTATUS          status           = STATUS_SUCCESS;
   OBJECT_ATTRIBUTES objectAttributes = {0};
   UNICODE_STRING    unicodeString    = {0};

   KeInitializeGuardedMutex(&guardedMutex);

   RtlInitUnicodeString(&unicodeString,
                        L"\\Callback\\PowerState");

   InitializeObjectAttributes(&objectAttributes,
                              &unicodeString,
                              OBJ_CASE_INSENSITIVE,
                              0,
                              0);

   status = ExCreateCallback(&(pDeviceExtension->pCallbackObject),
                             &objectAttributes,
                             FALSE,                                  /// Do not create as the system should already have done this
                             TRUE);                                  /// Allow multiple callbacks
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! RegisterPowerStateChangeCallback : ExCreateCallback() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pDeviceExtension->pRegistration = ExRegisterCallback(pDeviceExtension->pCallbackObject,
                                                        PowerStateCallback,
                                                        pDeviceExtension);

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- RegisterPowerStateChangeCallback() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}

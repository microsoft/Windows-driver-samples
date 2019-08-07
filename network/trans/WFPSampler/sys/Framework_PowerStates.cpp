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
 @framework_function="ActOnPowerStateTransition"

   Purpose:  Passive function to handle power state transition behaviors.  If going into a power state, the  <br>
             callouts are unregistered.  This prevents the driver from potentially taking any more           <br>
             references on NBLs, and instead causes the filters to return BLOCK.  This should also allow     <br>
             adequate time to drain currently queued NBLs.  If coming out a a power state, the callouts are  <br>
             reregistered and normal processing of NBLs will commence.                                       <br>
                                                                                                             <br>
   Notes:                                                                                                    <br>    
                                                                                                             <br>
   MSDN_Ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nc-wdm-kstart_routine <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID ActOnPowerStateTransition(_In_ PVOID pOperationNormal)  /// BOOLEAN*
{
#if DBG

    DbgPrintEx(DPFLTR_IHVNETWORK_ID,
        DPFLTR_INFO_LEVEL,
        " ---> ActOnPowerStateTransition()\n");

#endif /// DBG

   NTSTATUS status = STATUS_SUCCESS;

   KeAcquireGuardedMutex(&guardedMutex);

   if (*((BOOLEAN*)pOperationNormal) == TRUE)
   {
      if (g_calloutsRegistered == FALSE)
      {
          status = KrnlHlprExposedCalloutsRegister();
          HLPR_BAIL_ON_FAILURE(status);

         g_calloutsRegistered = TRUE;
      }
   }
   else
   {
      if (g_calloutsRegistered == TRUE)
      {
          status = KrnlHlprExposedCalloutsUnregister();
          HLPR_BAIL_ON_FAILURE(status);

          g_calloutsRegistered = FALSE;
      }
   }

   HLPR_BAIL_LABEL:

   KeReleaseGuardedMutex(&guardedMutex);

#if DBG

       DbgPrintEx(DPFLTR_IHVNETWORK_ID,
           DPFLTR_INFO_LEVEL,
           " <--- PreventFurtherCalloutProcessing()\n");

#endif /// DBG

   PsTerminateSystemThread(status);

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
      NTSTATUS status          = STATUS_SUCCESS;
      HANDLE   sysThreadHandle = 0;
      BOOLEAN  normalOperation = TRUE;

      if(pEventSpecifics)
      {
         /// entering the ON state (S0), so return operation to normal
      }
      else
      {
         /// leaving the ON state (S0) to sleep (S1/S2/S3) or hibernate (S4)
         /// Need to stop callouts from taking further references on NBLs, and 
         /// drain what is in the current queues
         normalOperation = FALSE;
      }

      status = PsCreateSystemThread(&sysThreadHandle,
                                    THREAD_ALL_ACCESS,
                                    0,
                                    0,
                                    0,
                                    ActOnPowerStateTransition,
                                    &normalOperation);
      HLPR_BAIL_ON_FAILURE(status);

      ZwClose(sysThreadHandle);
   }

   HLPR_BAIL_LABEL:

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

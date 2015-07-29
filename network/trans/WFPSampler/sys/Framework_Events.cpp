////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_Events.cpp
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
//      December  13,   2013  -     1.1   -  Add support for multiple injectors and redirectors
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework_WFPSamplerCalloutDriver.h" /// .
#include "Framework_Include.h"                 /// .
#include "Framework_Events.tmh"                /// $(OBJ_PATH)\$(O)\ 

/**
 @framework_function="EventDriverUnload"
 
   Purpose:  Callback function responding to Driver Unload Events.                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF541694.aspx             <br>
*/
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_DRIVER_UNLOAD)
VOID EventDriverUnload(_In_ WDFDRIVER wdfDriver)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> EventDriverUnload()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(wdfDriver);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- EventDriverUnload()\n");

#endif /// DBG
   
   WPP_CLEANUP(wdfDriver);

   return;
}

/**
 @framework_function="EventCleanupDriverObject"
 
   Purpose:  Callback function responding to Driver Cleanup Events.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF540840.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
VOID EventCleanupDriverObject(_In_ WDFOBJECT driverObject)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> EventCleanupDriverObject()\n");


#endif /// DBG
   
   UNREFERENCED_PARAMETER(driverObject);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- EventCleanupDriverObject()\n");

#endif /// DBG
   
   return;
}

/**
 @framework_function="EventCleanupDeviceObject"
 
   Purpose:  Callback function responding to Object Cleanup Events.                             <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF540840.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
VOID EventCleanupDeviceObject(_In_ WDFOBJECT deviceObject)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> EventCleanupDeviceObject()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(deviceObject);

   KrnlHlprExposedCalloutsUnregister();

   for(UINT32 index = 0;
       index <= UNIVERSAL_INDEX;
       index++)
   {

#if(NTDDI_VERSION >= NTDDI_WIN8)

      if(g_WFPSamplerDeviceData.ppRedirectionHandles[index])
         KrnlHlprFwpsRedirectHandleDestroy(g_WFPSamplerDeviceData.ppRedirectionHandles[index]);

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

      if(g_WFPSamplerDeviceData.ppOutboundInjectionHandles[index])
         KrnlHlprInjectionHandleDataDestroy(&(g_WFPSamplerDeviceData.ppOutboundInjectionHandles[index]));

      if(g_WFPSamplerDeviceData.ppInboundInjectionHandles[index])
         KrnlHlprInjectionHandleDataDestroy(&(g_WFPSamplerDeviceData.ppInboundInjectionHandles[index]));

      if(g_WFPSamplerDeviceData.ppIPv6OutboundInjectionHandles[index])
         KrnlHlprInjectionHandleDataDestroy(&(g_WFPSamplerDeviceData.ppIPv6OutboundInjectionHandles[index]));

      if(g_WFPSamplerDeviceData.ppIPv6InboundInjectionHandles[index])
         KrnlHlprInjectionHandleDataDestroy(&(g_WFPSamplerDeviceData.ppIPv6InboundInjectionHandles[index]));

      if(g_WFPSamplerDeviceData.ppIPv4OutboundInjectionHandles[index])
         KrnlHlprInjectionHandleDataDestroy(&(g_WFPSamplerDeviceData.ppIPv4OutboundInjectionHandles[index]));

      if(g_WFPSamplerDeviceData.ppIPv4InboundInjectionHandles[index])
         KrnlHlprInjectionHandleDataDestroy(&(g_WFPSamplerDeviceData.ppIPv4InboundInjectionHandles[index]));
   }

   if(g_WFPSamplerDeviceData.pEngineHandle)
      KrnlHlprFwpmSessionDestroyEngineHandle(&(g_WFPSamplerDeviceData.pEngineHandle));

   FwpmBfeStateUnsubscribeChanges(g_bfeSubscriptionHandle);

   UnregisterPowerStateChangeCallback(&g_deviceExtension);

   if(g_pNDISPoolData)
      KrnlHlprNDISPoolDataDestroy(&g_pNDISPoolData);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- EventCleanupDeviceObject()\n");

#endif /// DBG
   
   return;
}

/**
 @framework_function="EventIODeviceControl"
 
   Purpose:  Callback function responding to IO Control Events.                                 <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF541758.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL)
VOID EventIODeviceControl(_In_ WDFQUEUE wdfQueue,
                          _In_ WDFREQUEST wdfRequest,
                          _In_ size_t outputBufferLength,
                          _In_ size_t inputBufferLength,
                          _In_ ULONG ioControlCode)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> EventIODeviceControl()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(wdfQueue);
   UNREFERENCED_PARAMETER(wdfRequest);
   UNREFERENCED_PARAMETER(outputBufferLength);
   UNREFERENCED_PARAMETER(inputBufferLength);
   UNREFERENCED_PARAMETER(ioControlCode);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- EventIODeviceControl()\n");

#endif /// DBG
   
   return;
}

/**
 @framework_function="EventIORead"
 
   Purpose:  Callback function responding to IO Read Events.                                    <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF541776.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_IO_QUEUE_IO_READ)
VOID EventIORead(_In_ WDFQUEUE wdfQueue,
                 _In_ WDFREQUEST wdfRequest,
                 _In_ size_t length)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> EventIORead()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(wdfQueue);
   UNREFERENCED_PARAMETER(wdfRequest);
   UNREFERENCED_PARAMETER(length);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- EventIORead()\n");

#endif /// DBG
   
   return;
}

/**
 @framework_function="EventIOWrite"
 
   Purpose:  Callback function responding to IO Write Events.                                   <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Hardware/FF541813.aspx             <br>
*/
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_IO_QUEUE_IO_WRITE)
VOID EventIOWrite(_In_ WDFQUEUE wdfQueue,
                  _In_ WDFREQUEST wdfRequest,
                  _In_ size_t length)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> EventIOWrite()\n");

#endif /// DBG
   
   UNREFERENCED_PARAMETER(wdfQueue);
   UNREFERENCED_PARAMETER(wdfRequest);
   UNREFERENCED_PARAMETER(length);

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- EventIOWrite()\n");

#endif /// DBG
   
   return;
}

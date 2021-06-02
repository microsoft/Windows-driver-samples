/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   Transport Inspect Proxy Callout Driver Sample.

   This sample callout driver intercepts all transport layer traffic (e.g. 
   TCP, UDP, and non-error ICMP) sent to or receive from a (configurable) 
   remote peer and queue them to a threaded DPC for out-of-band processing. 
   The sample performs inspection of inbound and outbound connections as 
   well as all packets belong to those connections.  In addition the sample 
   demonstrates special considerations required to be compatible with Windows 
   Vista and Windows Server 2008�s IpSec implementation.

   Inspection parameters are configurable via the following registry 
   values --

   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Inspect\Parameters
      
    o  BlockTraffic (REG_DWORD) : 0 (permit, default); 1 (block)
    o  RemoteAddressToInspect (REG_SZ) : literal IPv4/IPv6 string 
                                                (e.g. �10.0.0.1�)
   The sample is IP version agnostic. It performs inspection for 
   both IPv4 and IPv6 traffic.

Environment:

    Kernel mode

--*/

#include <ntddk.h>
#include <wdf.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include <fwpmk.h>

#include <ws2ipdef.h>
#include <in6addr.h>
#include <ip2string.h>

#include "inspect.h"

#define INITGUID
#include <guiddef.h>
#include <traceloggingprovider.h>

// {93b77253-d19c-41df-8c18-05960dab6bc3}
TRACELOGGING_DEFINE_PROVIDER(
   gTlgHandle,
   "Microsoft.Windows.Networking.Inspect",
   (0x93b77253, 0xd19c, 0x41df, 0x8c, 0x18, 0x05, 0x96, 0x0d, 0xab, 0x6b, 0xc3));

// 
// Configurable parameters (addresses and ports are in host order)
//

BOOLEAN gIsTrafficPermitted;
UINT8*   configInspectRemoteAddrV4 = NULL;
UINT8*   configInspectRemoteAddrV6 = NULL;

IN_ADDR  remoteAddrStorageV4;
IN6_ADDR remoteAddrStorageV6;

// 
// Callout and sublayer GUIDs
//

// 76b743d4-1249-4614-a632-6f9c4d08d25a
DEFINE_GUID(
    TL_INSPECT_ALE_CONNECT_CALLOUT_V4,
    0x76b743d4,
    0x1249,
    0x4614,
    0xa6, 0x32, 0x6f, 0x9c, 0x4d, 0x08, 0xd2, 0x5a
);

// ac80683a-5b84-43c3-8ae9-eddb5c0d23c2
DEFINE_GUID(
    TL_INSPECT_ALE_CONNECT_CALLOUT_V6,
    0xac80683a,
    0x5b84,
    0x43c3,
    0x8a, 0xe9, 0xed, 0xdb, 0x5c, 0x0d, 0x23, 0xc2
);

// 2e207682-d95f-4525-b966-969f26587f03
DEFINE_GUID(
    TL_INSPECT_SUBLAYER,
    0x2e207682,
    0xd95f,
    0x4525,
    0xb9, 0x66, 0x96, 0x9f, 0x26, 0x58, 0x7f, 0x03
);

// 
// Callout driver global variables
//

DEVICE_OBJECT* gWdmDevice;
WDFKEY gParametersKey;

HANDLE gEngineHandle;
UINT32 gAleConnectCalloutIdV4;
UINT32 gAleConnectCalloutIdV6;
HANDLE gInjectionHandle;
BOOLEAN gDriverUnloading = FALSE;
TL_INSPECT_PROCESSOR_QUEUE* gProcessorQueue = NULL;

// 
// Callout driver implementation
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD TLInspectEvtDriverUnload;

NTSTATUS
TLInspectLoadConfig(
   _In_ const WDFKEY key
   )
{
   NTSTATUS status;
   DECLARE_CONST_UNICODE_STRING(valueName, L"RemoteAddressToInspect");
   DECLARE_UNICODE_STRING_SIZE(value, INET6_ADDRSTRLEN);
   DECLARE_CONST_UNICODE_STRING(blockTrafficValueName, L"BlockTraffic");
   ULONG result;

   status = WdfRegistryQueryUnicodeString(key, &valueName, NULL, &value);

   if (NT_SUCCESS(status))
   {
      PWSTR terminator;
      // Defensively null-terminate the string
      value.Length = min(value.Length, value.MaximumLength - sizeof(WCHAR));
      value.Buffer[value.Length/sizeof(WCHAR)] = UNICODE_NULL;

      status = RtlIpv4StringToAddressW(
                  value.Buffer,
                  TRUE,
                  &terminator,
                  &remoteAddrStorageV4
                  );

      if (NT_SUCCESS(status))
      {
         remoteAddrStorageV4.S_un.S_addr = 
            RtlUlongByteSwap(remoteAddrStorageV4.S_un.S_addr);
         configInspectRemoteAddrV4 = &remoteAddrStorageV4.S_un.S_un_b.s_b1;
      }
      else
      {
         status = RtlIpv6StringToAddressW(
                     value.Buffer,
                     &terminator,
                     &remoteAddrStorageV6
                     );

         if (NT_SUCCESS(status))
         {
            configInspectRemoteAddrV6 = (UINT8*)(&remoteAddrStorageV6.u.Byte[0]);
         }
      }
   }

   status = WdfRegistryQueryULong(
               gParametersKey,
               &blockTrafficValueName,
               &result
               );

   if (NT_SUCCESS(status))
   {
      // blockTraffic == 1 means block, 0 means permit
      gIsTrafficPermitted = !result;
   }

   return status;
}

NTSTATUS
TLInspectAddFilter(
   _In_ const wchar_t* filterName,
   _In_ const wchar_t* filterDesc,
   _In_reads_opt_(16) const UINT8* remoteAddr,
   _In_ UINT64 context,
   _In_ const GUID* layerKey,
   _In_ const GUID* calloutKey
   )
{
   NTSTATUS status = STATUS_SUCCESS;

   FWPM_FILTER filter = {0};
   FWPM_FILTER_CONDITION filterConditions[3] = {0}; 
   UINT conditionIndex;

   filter.layerKey = *layerKey;
   filter.displayData.name = (wchar_t*)filterName;
   filter.displayData.description = (wchar_t*)filterDesc;

   filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
   filter.action.calloutKey = *calloutKey;
   filter.filterCondition = filterConditions;
   filter.subLayerKey = TL_INSPECT_SUBLAYER;
   filter.weight.type = FWP_EMPTY; // auto-weight.
   filter.rawContext = context;

   conditionIndex = 0;

   if (remoteAddr != NULL)
   {
      filterConditions[conditionIndex].fieldKey = 
         FWPM_CONDITION_IP_REMOTE_ADDRESS;
      filterConditions[conditionIndex].matchType = FWP_MATCH_EQUAL;

      if (IsEqualGUID(layerKey, &FWPM_LAYER_ALE_AUTH_CONNECT_V4) ||
          IsEqualGUID(layerKey, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4) ||
          IsEqualGUID(layerKey, &FWPM_LAYER_INBOUND_TRANSPORT_V4) ||
          IsEqualGUID(layerKey, &FWPM_LAYER_OUTBOUND_TRANSPORT_V4))
      {
         filterConditions[conditionIndex].conditionValue.type = FWP_UINT32;
         filterConditions[conditionIndex].conditionValue.uint32 = 
            *(UINT32*)remoteAddr;
      }
      else
      {
         filterConditions[conditionIndex].conditionValue.type = 
            FWP_BYTE_ARRAY16_TYPE;
         filterConditions[conditionIndex].conditionValue.byteArray16 = 
            (FWP_BYTE_ARRAY16*)remoteAddr;
      }

      conditionIndex++;
   }

   filter.numFilterConditions = conditionIndex;

   status = FwpmFilterAdd(
               gEngineHandle,
               &filter,
               NULL,
               NULL);

   return status;
}

NTSTATUS
TLInspectRegisterALEClassifyCallouts(
   _In_ const GUID* layerKey,
   _In_ const GUID* calloutKey,
   _Inout_ void* deviceObject,
   _Out_ UINT32* calloutId
   )
/* ++

   This function registers callouts and filters at the following layers 
   to intercept outbound connect attempts.
   
      FWPM_LAYER_ALE_AUTH_CONNECT_V4
      FWPM_LAYER_ALE_AUTH_CONNECT_V6

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   FWPS_CALLOUT sCallout = {0};
   FWPM_CALLOUT mCallout = {0};

   FWPM_DISPLAY_DATA displayData = {0};

   BOOLEAN calloutRegistered = FALSE;

   sCallout.calloutKey = *calloutKey;

   if (IsEqualGUID(layerKey, &FWPM_LAYER_ALE_AUTH_CONNECT_V4) ||
       IsEqualGUID(layerKey, &FWPM_LAYER_ALE_AUTH_CONNECT_V6))
   {
      sCallout.classifyFn = TLInspectALEConnectClassify;
      sCallout.notifyFn = TLInspectALEConnectNotify;
   }
   else
   {
      // This sample only uses CONNECT layer callouts
      NT_ASSERT(FALSE);
   }

   status = FwpsCalloutRegister(
               deviceObject,
               &sCallout,
               calloutId
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }
   calloutRegistered = TRUE;

   displayData.name = L"Transport Inspect ALE Classify Callout";
   displayData.description = 
      L"Intercepts outbound connect attempts";

   mCallout.calloutKey = *calloutKey;
   mCallout.displayData = displayData;
   mCallout.applicableLayer = *layerKey;

   status = FwpmCalloutAdd(
               gEngineHandle,
               &mCallout,
               NULL,
               NULL
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = TLInspectAddFilter(
               L"Transport Inspect ALE Classify",
               L"Intercepts outbound connect attempts",
               IsEqualGUID(layerKey, &FWPM_LAYER_ALE_AUTH_CONNECT_V4) ? 
                  configInspectRemoteAddrV4 : configInspectRemoteAddrV6,
               0,
               layerKey,
               calloutKey
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

Exit:

   if (!NT_SUCCESS(status))
   {
      if (calloutRegistered)
      {
         FwpsCalloutUnregisterById(*calloutId);
         *calloutId = 0;
      }
   }

   return status;
}

NTSTATUS
TLInspectRegisterCallouts(
   _Inout_ void* deviceObject
   )
/* ++

   This function registers dynamic callouts and filters that intercept 
   transport traffic at ALE AUTH_CONNECT/AUTH_RECV_ACCEPT and 
   INBOUND/OUTBOUND transport layers.

   Callouts and filters will be removed during DriverUnload.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;
   FWPM_SUBLAYER TLInspectSubLayer;

   BOOLEAN engineOpened = FALSE;
   BOOLEAN inTransaction = FALSE;

   FWPM_SESSION session = {0};

   session.flags = FWPM_SESSION_FLAG_DYNAMIC;

   status = FwpmEngineOpen(
                NULL,
                RPC_C_AUTHN_WINNT,
                NULL,
                &session,
                &gEngineHandle
                );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }
   engineOpened = TRUE;

   status = FwpmTransactionBegin(gEngineHandle, 0);
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }
   inTransaction = TRUE;

   RtlZeroMemory(&TLInspectSubLayer, sizeof(FWPM_SUBLAYER)); 

   TLInspectSubLayer.subLayerKey = TL_INSPECT_SUBLAYER;
   TLInspectSubLayer.displayData.name = L"Transport Inspect Sub-Layer";
   TLInspectSubLayer.displayData.description = 
      L"Sub-Layer for use by Transport Inspect callouts";
   TLInspectSubLayer.flags = 0;
   TLInspectSubLayer.weight = 0; // must be less than the weight of 
                                 // FWPM_SUBLAYER_UNIVERSAL to be
                                 // compatible with Vista's IpSec
                                 // implementation.

   status = FwpmSubLayerAdd(gEngineHandle, &TLInspectSubLayer, NULL);
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   if (configInspectRemoteAddrV4 != NULL)
   {
      status = TLInspectRegisterALEClassifyCallouts(
                  &FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                  &TL_INSPECT_ALE_CONNECT_CALLOUT_V4,
                  deviceObject,
                  &gAleConnectCalloutIdV4
                  );
      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }
   }

   if (configInspectRemoteAddrV6 != NULL)
   {
      status = TLInspectRegisterALEClassifyCallouts(
                  &FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                  &TL_INSPECT_ALE_CONNECT_CALLOUT_V6,
                  deviceObject,
                  &gAleConnectCalloutIdV6
                  );
      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }
   }

   status = FwpmTransactionCommit(gEngineHandle);
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }
   inTransaction = FALSE;

Exit:

   if (!NT_SUCCESS(status))
   {
      if (inTransaction)
      {
         FwpmTransactionAbort(gEngineHandle);
         _Analysis_assume_lock_not_held_(gEngineHandle); // Potential leak if "FwpmTransactionAbort" fails
      }
      if (engineOpened)
      {
         FwpmEngineClose(gEngineHandle);
         gEngineHandle = NULL;
      }
   }

   return status;
}

void
TLInspectUnregisterCallouts(void)
{
   FwpmEngineClose(gEngineHandle);
   gEngineHandle = NULL;

   FwpsCalloutUnregisterById(gAleConnectCalloutIdV6);
   FwpsCalloutUnregisterById(gAleConnectCalloutIdV4);
}

void
InitializeWorkerQueues()
{
   ULONG numProcessors = KeQueryMaximumProcessorCount();

   gProcessorQueue = ExAllocatePool2(
                        POOL_FLAG_NON_PAGED,
                        sizeof(TL_INSPECT_PROCESSOR_QUEUE) * numProcessors,
                        TL_INSPECT_SETUP_POOL_TAG
                        );

   //
   // Create a per-processor work queue.
   //
   for (ULONG i = 0; i < numProcessors; ++i)
   {
      InitializeListHead(&gProcessorQueue[i].connList);
      KeInitializeSpinLock(&gProcessorQueue[i].connListLock);
      InitializeListHead(&gProcessorQueue[i].classifiedConnList);
      KeInitializeSpinLock(&gProcessorQueue[i].classifiedConnListLock);
      gProcessorQueue[i].isDpcQueued = FALSE;

      KeInitializeThreadedDpc(
         &gProcessorQueue[i].kdpc,
         TLInspectWorker,
         NULL
         );
   }
}

void
UninitializeWorkerQueues()
{
   KLOCK_QUEUE_HANDLE connListLockHandle;
   BOOLEAN isDpcRunning = TRUE;
   ULONG numProcessors = KeQueryMaximumProcessorCount();

   if (gProcessorQueue != NULL)
   {
     // Wait for all DPCs to complete before exiting
     while (isDpcRunning)
     {
       isDpcRunning = FALSE;
       for (ULONG i = 0; i < numProcessors; ++i)
       {
          KeAcquireInStackQueuedSpinLock(
             &gProcessorQueue[i].connListLock,
             &connListLockHandle
             );
          isDpcRunning |= gProcessorQueue[i].isDpcQueued;
          KeReleaseInStackQueuedSpinLock(&connListLockHandle);
        }
     }

   ExFreePoolWithTag(gProcessorQueue, TL_INSPECT_SETUP_POOL_TAG);

   }
}

_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void
TLInspectEvtDriverUnload(
   _In_ WDFDRIVER driverObject
   )
{
   UNREFERENCED_PARAMETER(driverObject);

   gDriverUnloading = TRUE;

   TLInspectUnregisterCallouts();

   UninitializeWorkerQueues();

   FwpsInjectionHandleDestroy(gInjectionHandle);

   TraceLoggingUnregister(gTlgHandle);
}

NTSTATUS
TLInspectInitDriverObjects(
   _Inout_ DRIVER_OBJECT* driverObject,
   _In_ const UNICODE_STRING* registryPath,
   _Out_ WDFDRIVER* pDriver,
   _Out_ WDFDEVICE* pDevice
   )
{
   NTSTATUS status;
   WDF_DRIVER_CONFIG config;
   PWDFDEVICE_INIT pInit = NULL;

   WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

   config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
   config.EvtDriverUnload = TLInspectEvtDriverUnload;

   status = WdfDriverCreate(
               driverObject,
               registryPath,
               WDF_NO_OBJECT_ATTRIBUTES,
               &config,
               pDriver
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   pInit = WdfControlDeviceInitAllocate(*pDriver, &SDDL_DEVOBJ_KERNEL_ONLY);

   if (!pInit)
   {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto Exit;
   }

   WdfDeviceInitSetDeviceType(pInit, FILE_DEVICE_NETWORK);
   WdfDeviceInitSetCharacteristics(pInit, FILE_DEVICE_SECURE_OPEN, FALSE);
   WdfDeviceInitSetCharacteristics(pInit, FILE_AUTOGENERATED_DEVICE_NAME, TRUE);

   status = WdfDeviceCreate(&pInit, WDF_NO_OBJECT_ATTRIBUTES, pDevice);
   if (!NT_SUCCESS(status))
   {
      WdfDeviceInitFree(pInit);
      goto Exit;
   }

   TraceLoggingRegister(gTlgHandle);

   WdfControlFinishInitializing(*pDevice);

Exit:
   return status;
}

NTSTATUS
DriverEntry(
   DRIVER_OBJECT* driverObject,
   UNICODE_STRING* registryPath
   )
{
   NTSTATUS status;
   WDFDRIVER driver;
   WDFDEVICE device;

   // Request NX Non-Paged Pool when available
   ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

   status = TLInspectInitDriverObjects(
               driverObject,
               registryPath,
               &driver,
               &device
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = WdfDriverOpenParametersRegistryKey(
               driver,
               KEY_READ,
               WDF_NO_OBJECT_ATTRIBUTES,
               &gParametersKey
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = TLInspectLoadConfig(gParametersKey);

   if (!NT_SUCCESS(status))
   {
      status = STATUS_DEVICE_CONFIGURATION_ERROR;
      goto Exit;
   }

   if ((configInspectRemoteAddrV4 == NULL) && 
       (configInspectRemoteAddrV6 == NULL))
   {
      status = STATUS_DEVICE_CONFIGURATION_ERROR;
      goto Exit;
   }

   status = FwpsInjectionHandleCreate(
               AF_UNSPEC,
               FWPS_INJECTION_TYPE_TRANSPORT,
               &gInjectionHandle
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   InitializeWorkerQueues();

   gWdmDevice = WdfDeviceWdmGetDeviceObject(device);

   status = TLInspectRegisterCallouts(gWdmDevice);

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

Exit:
   
   if (!NT_SUCCESS(status))
   {
      if (gEngineHandle != NULL)
      {
         TLInspectUnregisterCallouts();
      }
      if (gInjectionHandle != NULL)
      {
         FwpsInjectionHandleDestroy(gInjectionHandle);
      }
      UninitializeWorkerQueues();
   }

   return status;
};


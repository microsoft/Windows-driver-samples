/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   Datagram-Data Transparent Proxy Callout Driver Sample.

   This sample callout driver intercepts UDP and non-error ICMP traffic 
   of interest and proxies them to a new destination address and/or port 
   (for UDP); response traffic will be proxied back to have the original 
   tuple values. The proxying is transparent to the application. 

   Inspection parameters and proxy settings are configurable via the 
   following registry values --

   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ddproxy\Parameters
      
    o  InspectUdp (REG_DWORD) : 0 (ICMP); 1 (UDP, default)
    o  DestinationAddressToIntercept (REG_SZ) : literal IPv4/IPv6 string 
                                                (e.g. “10.0.0.1”)
    o  DestinationPortToIntercept (REG_DWORD) : applicable if InspectUdp is 1
    o  NewDestinationAddress(REG_SZ) : literal IPv4/IPv6 string
    o  NewDestinationPort(REG_DWORD)

   The sample is IP version agnostic. It performs proxying for both IPv4 
   and IPv6 traffic.

Environment:

    Kernel mode

--*/

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
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

#include "DD_proxy.h"

#define INITGUID
#include <guiddef.h>

// 
// Configurable parameters (addresses and ports are in host order)
//

BOOLEAN configInspectUdp = TRUE;

UINT16   configInspectDestPort = 5001;
UINT8*   configInspectDestAddrV4 = NULL;
UINT8*   configInspectDestAddrV6 = NULL;

UINT16   configNewDestPort = 5001;
UINT8*   configNewDestAddrV4 = NULL;
UINT8*   configNewDestAddrV6 = NULL;

SOCKADDR_STORAGE destAddr, newDestAddr;

// 
// Callout and sublayer GUIDs
//

// b16b0a6e-2b2a-41a3-8b39-bd3ffc855ff8
DEFINE_GUID(
    DD_PROXY_CALLOUT_V4,
    0xb16b0a6e,
    0x2b2a,
    0x41a3,
    0x8b, 0x39, 0xbd, 0x3f, 0xfc, 0x85, 0x5f, 0xf8
);
// 2cebde39-1f59-48d1-a5d9-3e2458351476
DEFINE_GUID(
    DD_PROXY_CALLOUT_V6,
    0x2cebde39,
    0x1f59,
    0x48d1,
    0xa5, 0xd9, 0x3e, 0x24, 0x58, 0x35, 0x14, 0x76
);
// ee93719d-ad5d-48c9-ae46-7270367d205d
DEFINE_GUID(
    DD_PROXY_FLOW_ESTABLISHED_CALLOUT_V4,
    0xee93719d,
    0xad5d,
    0x48c9,
    0xae, 0x46, 0x72, 0x70, 0x36, 0x7d, 0x20, 0x5d
);

// 1e3d3d13-0588-4167-82a3-14f68c98de86
DEFINE_GUID(
    DD_PROXY_FLOW_ESTABLISHED_CALLOUT_V6,
    0x1e3d3d13,
    0x0588,
    0x4167,
    0x82, 0xa3, 0x14, 0xf6, 0x8c, 0x98, 0xde, 0x86
);

// 0104fd7e-c825-414e-94c9-f0d525bbc169
DEFINE_GUID(
    DD_PROXY_SUBLAYER,
    0x0104fd7e,
    0xc825,
    0x414e,
    0x94, 0xc9, 0xf0, 0xd5, 0x25, 0xbb, 0xc1, 0x69
);

// 
// Callout driver global variables
//

DEVICE_OBJECT* gWdmDevice;

HANDLE gEngineHandle;
UINT32 gFlowEstablishedCalloutIdV4, gCalloutIdV4;
UINT32 gFlowEstablishedCalloutIdV6, gCalloutIdV6;

HANDLE gInjectionHandle;

LIST_ENTRY gFlowList;
KSPIN_LOCK gFlowListLock;

LIST_ENTRY gPacketQueue;
KSPIN_LOCK gPacketQueueLock;
KEVENT gPacketQueueEvent;

BOOLEAN gDriverUnloading = FALSE;
void* gThreadObj;

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

// 
// Callout driver implementation
//

NTSTATUS
DDProxyLoadIPAddress(
   _In_ const WDFKEY key,
   _In_ const UNICODE_STRING* valueName,
   _Out_ SOCKADDR_STORAGE* result
   )
{
   NTSTATUS status;
   PWSTR terminator;
   DECLARE_UNICODE_STRING_SIZE(value, INET6_ADDRSTRLEN);
   IN_ADDR *resultV4 = &((SOCKADDR_IN*)result)->sin_addr;
   IN6_ADDR *resultV6 = &((SOCKADDR_IN6*)result)->sin6_addr;
   
   status = WdfRegistryQueryUnicodeString(key, valueName, NULL, &value);
   result->ss_family = AF_UNSPEC;

   if (NT_SUCCESS(status))
   {
      // The Registry API does not guarantee that the string will be
      // null-terminated.
      // Defensively null-terminate the string.
      value.Length = min(value.Length, value.MaximumLength - sizeof(WCHAR));
      value.Buffer[value.Length/sizeof(WCHAR)] = UNICODE_NULL;

      status = RtlIpv4StringToAddressW(
                  value.Buffer,
                  TRUE,
                  &terminator,
                  resultV4
                  );

      if (NT_SUCCESS(status))
      {
         resultV4->S_un.S_addr = RtlUlongByteSwap(resultV4->S_un.S_addr);
         result->ss_family = AF_INET;
      }
      else
      {
         status = RtlIpv6StringToAddressW(
                     value.Buffer,
                     &terminator,
                     resultV6
                     );

         if (NT_SUCCESS(status))
         {
            result->ss_family = AF_INET6;
         }
      }
   }

   return status;
}

NTSTATUS
DDProxyLoadConfig(
   _In_ const WDFKEY key
   )
{
   NTSTATUS status = STATUS_SUCCESS;
   DECLARE_CONST_UNICODE_STRING(inspectUdpValueName, L"InspectUdp");
   DECLARE_CONST_UNICODE_STRING(destAddrValueName, L"DestinationAddressToIntercept");
   DECLARE_CONST_UNICODE_STRING(destPortValueName, L"DestinationPortToIntercept");
   DECLARE_CONST_UNICODE_STRING(newDestAddrValueName, L"NewDestinationAddress");
   DECLARE_CONST_UNICODE_STRING(newDestPortValueName, L"NewDestinationPort");

   ULONG ulongValue;

   if (NT_SUCCESS(WdfRegistryQueryULong(
                     key,
                     &inspectUdpValueName,
                     &ulongValue
                     )))
   {
      configInspectUdp = (ulongValue != 0);
   }


   if (NT_SUCCESS(DDProxyLoadIPAddress(
                     key,
                     &destAddrValueName,
                     &destAddr
                     )))
   {
      if (destAddr.ss_family == AF_INET)
      {
         configInspectDestAddrV4 = &((SOCKADDR_IN*)&destAddr)->sin_addr.S_un.S_un_b.s_b1;
      }
      else if (destAddr.ss_family == AF_INET6)
      {
         configInspectDestAddrV6 = (UINT8*)(&((SOCKADDR_IN6*)&destAddr)->sin6_addr.u.Byte[0]);
      }
   }

   if (NT_SUCCESS(WdfRegistryQueryULong(
                     key,
                     &destPortValueName,
                     &ulongValue
                     )))
   {
      configInspectDestPort = (USHORT) ulongValue;
   }

   if (NT_SUCCESS(DDProxyLoadIPAddress(
                     key,
                     &newDestAddrValueName,
                     &newDestAddr
                     )))
   {
      if (destAddr.ss_family == AF_INET)
      {
         configNewDestAddrV4 = &((SOCKADDR_IN*)&newDestAddr)->sin_addr.S_un.S_un_b.s_b1;
      }
      else if (destAddr.ss_family == AF_INET6)
      {
         configNewDestAddrV6 = (UINT8*)(&((SOCKADDR_IN6*)&newDestAddr)->sin6_addr.u.Byte[0]);
      }
   }

   if (NT_SUCCESS(WdfRegistryQueryULong(
                     key,
                     &newDestPortValueName,
                     &ulongValue
                     )))
   {
      configNewDestPort = (USHORT) ulongValue;
   }

   return status;
}

NTSTATUS
DDProxyAddFilter(
   _In_ const PWSTR filterName,
   _In_ const PWSTR filterDesc,
   _In_reads_(16) const UINT8* remoteAddr,
   _In_ USHORT remotePort,
   _In_ FWP_DIRECTION direction,
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
   filter.subLayerKey = DD_PROXY_SUBLAYER;
   filter.weight.type = FWP_EMPTY; // auto-weight.
   filter.rawContext = context;

   conditionIndex = 0;

   if (remoteAddr != NULL)
   {
      filterConditions[conditionIndex].fieldKey = 
         FWPM_CONDITION_IP_REMOTE_ADDRESS;
      filterConditions[conditionIndex].matchType = FWP_MATCH_EQUAL;

      if (IsEqualGUID(layerKey, &FWPM_LAYER_DATAGRAM_DATA_V4) ||
          IsEqualGUID(layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4))
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

   filterConditions[conditionIndex].fieldKey = FWPM_CONDITION_DIRECTION;
   filterConditions[conditionIndex].matchType = FWP_MATCH_EQUAL;
   filterConditions[conditionIndex].conditionValue.type = FWP_UINT32;
   filterConditions[conditionIndex].conditionValue.uint32 = direction;

   conditionIndex++;

   if (configInspectUdp)
   {
      filterConditions[conditionIndex].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
      filterConditions[conditionIndex].matchType = FWP_MATCH_EQUAL;
      filterConditions[conditionIndex].conditionValue.type = FWP_UINT16;
      filterConditions[conditionIndex].conditionValue.uint16 = remotePort;
      
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
DDProxyRegisterFlowEstablishedCallouts(
   _In_ const GUID* layerKey,
   _In_ const GUID* calloutKey,
   _Inout_ void* deviceObject,
   _Out_ UINT32* calloutId
   )
/* ++

   This function registers callouts and filters at the following layers 
   to intercept flow creations for the original and the proxy flows.
   
      FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4
      FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   FWPS_CALLOUT sCallout = {0};
   FWPM_CALLOUT mCallout = {0};

   FWPM_DISPLAY_DATA displayData = {0};

   BOOLEAN calloutRegistered = FALSE;

   sCallout.calloutKey = *calloutKey;
   sCallout.classifyFn = DDProxyFlowEstablishedClassify;
   sCallout.notifyFn = DDProxyFlowEstablishedNotify;

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

   displayData.name = L"Datagram-Data Proxy Flow-Established Callout";
   displayData.description = 
      L"Intercepts flow creations for the original and the proxy flows";

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

   status = DDProxyAddFilter(
               L"Datagram-Data Proxy Flow-Established Filter (Original Flow)",
               L"Intercepts flow creations for the original flow",
               IsEqualGUID(layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4) ? 
                  configInspectDestAddrV4 : configInspectDestAddrV6,
               configInspectDestPort,
               FWP_DIRECTION_OUTBOUND,
               DD_PROXY_FLOW_ORIGINAL,
               layerKey,
               calloutKey
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyAddFilter(
               L"Datagram-Data Proxy Flow-Established Filter (Proxy Flow)",
               L"Intercepts flow creations for the proxy flow",
               IsEqualGUID(layerKey, &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4) ? 
                  configNewDestAddrV4 : configNewDestAddrV6,
               configNewDestPort,
               FWP_DIRECTION_OUTBOUND,
               DD_PROXY_FLOW_PROXY,
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
DDProxyRegisterDatagramDataCallouts(
   _In_ const GUID* layerKey,
   _In_ const GUID* calloutKey,
   _Inout_ void* deviceObject,
   _Out_ UINT32* calloutId
   )
/* ++

   This function registers callouts and filters that intercept TCP traffic at 
   WFP FWPM_LAYER_DATAGRAM_DATA_V4 or FWPM_LAYER_DATAGRAM_DATA_V6 layer.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   FWPS_CALLOUT sCallout = {0};
   FWPM_CALLOUT mCallout = {0};

   FWPM_DISPLAY_DATA displayData = {0};

   BOOLEAN calloutRegistered = FALSE;

   sCallout.calloutKey = *calloutKey;
   sCallout.classifyFn = DDProxyClassify;
   sCallout.notifyFn = DDProxyNotify;
   sCallout.flowDeleteFn = DDProxyFlowDelete;
   sCallout.flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;

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

   displayData.name = L"Datagram-Data Proxy Callout";
   displayData.description = L"Proxies destination address/port for UDP/ICMP";

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

   status = DDProxyAddFilter(
               L"Datagram-Data Proxy Filter (Outbound)",
               L"Proxies destination address/port for UDP/ICMP",
               IsEqualGUID(layerKey, &FWPM_LAYER_DATAGRAM_DATA_V4) ? 
                  configInspectDestAddrV4 : configInspectDestAddrV6,
               configInspectDestPort,
               FWP_DIRECTION_OUTBOUND,
               0,
               layerKey,
               calloutKey
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyAddFilter(
               L"Datagram-Data Proxy Filter (Inbound)",
               L"Proxies destination address/port for UDP/ICMP",
               IsEqualGUID(layerKey, &FWPM_LAYER_DATAGRAM_DATA_V4) ? 
                  configNewDestAddrV4 : configNewDestAddrV6,
               configNewDestPort,
               FWP_DIRECTION_INBOUND,
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
DDProxyRegisterCallouts(
   _Inout_ void* deviceObject
   )
/* ++

   This function registers dynamic callouts and filters that intercept UDP or
   non-error ICMP traffic at WFP FWPM_LAYER_DATAGRAM_DATA_V{4|6} and 
   FWPM_LAYER_ALE_FLOW_ESTABLISHED_V{4|6} layers.

   Callouts and filters will be removed during DriverUnload.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;
   FWPM_SUBLAYER DDProxySubLayer;

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

   RtlZeroMemory(&DDProxySubLayer, sizeof(FWPM_SUBLAYER)); 

   DDProxySubLayer.subLayerKey = DD_PROXY_SUBLAYER;
   DDProxySubLayer.displayData.name = L"Datagram-Data Proxy Sub-Layer";
   DDProxySubLayer.displayData.description = 
      L"Sub-Layer for use by Datagram-Data Proxy callouts";
   DDProxySubLayer.flags = 0;
   DDProxySubLayer.weight = FWP_EMPTY; // auto-weight.;

   status = FwpmSubLayerAdd(gEngineHandle, &DDProxySubLayer, NULL);
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyRegisterFlowEstablishedCallouts(
               &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
               &DD_PROXY_FLOW_ESTABLISHED_CALLOUT_V4,
               deviceObject,
               &gFlowEstablishedCalloutIdV4
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyRegisterFlowEstablishedCallouts(
               &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
               &DD_PROXY_FLOW_ESTABLISHED_CALLOUT_V6,
               deviceObject,
               &gFlowEstablishedCalloutIdV6
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyRegisterDatagramDataCallouts(
               &FWPM_LAYER_DATAGRAM_DATA_V4,
               &DD_PROXY_CALLOUT_V4,
               deviceObject,
               &gCalloutIdV4
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyRegisterDatagramDataCallouts(
               &FWPM_LAYER_DATAGRAM_DATA_V6,
               &DD_PROXY_CALLOUT_V6,
               deviceObject,
               &gCalloutIdV6
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
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
DDProxyUnregisterCallouts(void)
{
   FwpmEngineClose(gEngineHandle);
   gEngineHandle = NULL;

   FwpsCalloutUnregisterById(gCalloutIdV6);
   FwpsCalloutUnregisterById(gCalloutIdV4);

   FwpsCalloutUnregisterById(gFlowEstablishedCalloutIdV6);
   FwpsCalloutUnregisterById(gFlowEstablishedCalloutIdV4);
}

void
DDProxyRemoveFlows(void)
{
   while (!IsListEmpty(&gFlowList))
   {
      KLOCK_QUEUE_HANDLE flowListLockHandle;
      LIST_ENTRY* listEntry = NULL;
      DD_PROXY_FLOW_CONTEXT* flowContext;

      KeAcquireInStackQueuedSpinLock(
         &gFlowListLock,
         &flowListLockHandle
         );

      if (!IsListEmpty(&gFlowList))
      {
         listEntry = RemoveHeadList(&gFlowList);
      }

      //
      // Releasing the lock here since removing the flow context 
      // will invoke the callout's flowDeleteFn synchronously 
      // if there are no active classifications in progress.
      //
      KeReleaseInStackQueuedSpinLock(&flowListLockHandle);

      if (listEntry != NULL)
      {
         flowContext = CONTAINING_RECORD(
                           listEntry,
                           DD_PROXY_FLOW_CONTEXT,
                           listEntry
                           );

         flowContext->deleted = TRUE;

         FwpsFlowRemoveContext(
            flowContext->flowId,
            flowContext->layerId,
            flowContext->calloutId
            );
      }
   }
}

_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void
EvtDriverUnload(
   _In_ WDFDRIVER driverObject
   )
{
   KLOCK_QUEUE_HANDLE packetQueueLockHandle;
   KLOCK_QUEUE_HANDLE flowListLockHandle;

   UNREFERENCED_PARAMETER(driverObject);

   KeAcquireInStackQueuedSpinLock(
      &gPacketQueueLock,
      &packetQueueLockHandle
      );

   KeAcquireInStackQueuedSpinLock(
      &gFlowListLock,
      &flowListLockHandle
      );

   gDriverUnloading = TRUE;

   KeReleaseInStackQueuedSpinLock(&flowListLockHandle);

   //
   // Any associated flow contexts must be removed before
   // a callout can be successfully unregistered.
   //
   DDProxyRemoveFlows();

   if (IsListEmpty(&gPacketQueue))
   {
      KeSetEvent(
         &gPacketQueueEvent,
         IO_NO_INCREMENT, 
         FALSE
         );
   }

   KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);

   NT_ASSERT(gThreadObj != NULL);

   KeWaitForSingleObject(
      gThreadObj,
      Executive,
      KernelMode,
      FALSE,
      NULL
      );

   ObDereferenceObject(gThreadObj);

   DDProxyUnregisterCallouts();

   FwpsInjectionHandleDestroy(gInjectionHandle);
}

//
// Create the minimal WDF Driver and Device objects required for a WFP callout
// driver.
//
NTSTATUS
DDProxyInitDriverObjects(
   _Inout_ DRIVER_OBJECT* driverObject,
   _In_ const UNICODE_STRING* registryPath,
   _Out_ WDFDRIVER* pDriver,
   _Out_ WDFDEVICE* pDevice
   )
{
   NTSTATUS status;
   WDF_DRIVER_CONFIG config;
   PWDFDEVICE_INIT pInit = NULL;

   WDF_DRIVER_CONFIG_INIT(
      &config,
      WDF_NO_EVENT_CALLBACK
      );

   config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
   config.EvtDriverUnload = EvtDriverUnload;

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

   pInit = WdfControlDeviceInitAllocate(
               *pDriver,
               &SDDL_DEVOBJ_KERNEL_ONLY
               );
               
   if (!pInit)
   {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto Exit;
   }

   WdfDeviceInitSetDeviceType(
      pInit,
      FILE_DEVICE_NETWORK
      );

   WdfDeviceInitSetCharacteristics(
      pInit,
      FILE_DEVICE_SECURE_OPEN,
      FALSE
      );

   WdfDeviceInitSetCharacteristics(
      pInit,
      FILE_AUTOGENERATED_DEVICE_NAME,
      TRUE
      );

   status = WdfDeviceCreate(
               &pInit,
               WDF_NO_OBJECT_ATTRIBUTES,
               pDevice
               );

   if (!NT_SUCCESS(status))
   {
      WdfDeviceInitFree(pInit);
      goto Exit;
   }

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
   WDFKEY configKey;
   HANDLE threadHandle;

   // Request NX Non-Paged Pool when available
   ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

   status = DDProxyInitDriverObjects(
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
               &configKey
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = DDProxyLoadConfig(configKey);

   if (!NT_SUCCESS(status))
   {
      status = STATUS_DEVICE_CONFIGURATION_ERROR;
      goto Exit;
   }

   //
   // To proxy UDP traffic, a new destination port or a pair of inspect and
   // proxy ip address need to be pre-configured. To proxy UDP traffic, a
   // pair of inspect and proxy ip addresses must be pre-configured.
   //
   if (configInspectUdp)
   {
      if ((configInspectDestPort == configNewDestPort) &&
          (((configInspectDestAddrV4 == NULL) || 
            (configNewDestAddrV4 == NULL)) && 
          ((configInspectDestAddrV6 == NULL) || 
           (configNewDestAddrV6 == NULL))))
      {
         status = STATUS_DEVICE_CONFIGURATION_ERROR;
         goto Exit;
      }
   }
   else
   {
      if (((configInspectDestAddrV4 == NULL) || 
           (configNewDestAddrV4 == NULL)) && 
          ((configInspectDestAddrV6 == NULL) || 
           (configNewDestAddrV6 == NULL)))
      {
         status = STATUS_DEVICE_CONFIGURATION_ERROR;
         goto Exit;
      }
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

   InitializeListHead(&gFlowList);
   KeInitializeSpinLock(&gFlowListLock);   

   InitializeListHead(&gPacketQueue);
   KeInitializeSpinLock(&gPacketQueueLock);   
   KeInitializeEvent(
      &gPacketQueueEvent,
      NotificationEvent,
      FALSE
      );

   gWdmDevice = WdfDeviceWdmGetDeviceObject(device);
   
   status = DDProxyRegisterCallouts(gWdmDevice);

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = PsCreateSystemThread(
               &threadHandle,
               THREAD_ALL_ACCESS,
               NULL,
               NULL,
               NULL,
               DDProxyWorker,
               NULL
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = ObReferenceObjectByHandle(
               threadHandle,
               0,
               NULL,
               KernelMode,
               &gThreadObj,
               NULL
               );
   NT_ASSERT(NT_SUCCESS(status));

   ZwClose(threadHandle);

Exit:
   
   if (!NT_SUCCESS(status))
   {
      if (gEngineHandle != NULL)
      {
         DDProxyUnregisterCallouts();
      }
      if (gInjectionHandle != NULL)
      {
         FwpsInjectionHandleDestroy(gInjectionHandle);
      }
   }

   return status;
}

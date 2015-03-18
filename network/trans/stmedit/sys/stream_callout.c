/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   Stream Edit Callout Driver Sample.

   This sample demonstrates finding and replacing a string pattern from a
   live TCP stream via the WFP stream API.

   The driver can function in one of the two modes --

      o  Inline Editing where all modification is carried out within the
         WFP ClassifyFn callout function.

      o  Out-of-band (OOB) Editing where all modification is done by a 
         worker thread. (this is the default)

   The mode setting, along with other inspection parameters are configurable
   via the following registry values

  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\stmedit\Parameters
      
      o  StringToFind (REG_SZ, default = "rainy")
      o  StringToReplace (REG_SZ, default = "sunny")
      o  InspectionPort (REG_DWORD, default = 5001)
      o  InspectOutbound (REG_DWORD, default = 0)
      o  EditInline (REG_DWORD, default = 0)

   The sample is IP version agnostic. It performs inspection on both IPv4 and
   IPv6 data streams.

   Before experimenting with the sample, please be sure to add an exception for
   the InspectionPort configured to the firewall. 

Environment:

    Kernel mode

--*/

#include <ntifs.h>
#include <wdf.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include <fwpmk.h>

#include "inline_edit.h"
#include "oob_edit.h"
#include "stream_callout.h"

#define INITGUID
#include <guiddef.h>

// 
// Configurable parameters
//

USHORT  configInspectionPort = 5001;
BOOLEAN configInspectionOutbound = FALSE; 
BOOLEAN configEditInline = FALSE;

CHAR configStringToFind[128] = "rainy";
CHAR configStringToReplace[128] = "sunny";
   
// 
// Callout driver keys
//

// e6011cdc-440b-4a6f-8499-6fdb55fb1f92
DEFINE_GUID(
    STREAM_EDITOR_STREAM_CALLOUT_V4,
    0xe6011cdc,
    0x440b,
    0x4a6f,
    0x84, 0x99, 0x6f, 0xdb, 0x55, 0xfb, 0x1f, 0x92
);
// c0bc07b4-aaf6-4242-a3dc-3ef341ffde5d
DEFINE_GUID(
    STREAM_EDITOR_STREAM_CALLOUT_V6,
    0xc0bc07b4,
    0xaaf6,
    0x4242,
    0xa3, 0xdc, 0x3e, 0xf3, 0x41, 0xff, 0xde, 0x5d
);

// 
// Callout driver global variables
//

MDL* gStringToReplaceMdl;

STREAM_EDITOR gStreamEditor;

HANDLE gEngineHandle;
UINT32 gCalloutIdV4;
UINT32 gCalloutIdV6;

DEVICE_OBJECT* gWdmDevice;

HANDLE gInjectionHandle;

NDIS_GENERIC_OBJECT* gNdisGenericObj;
NDIS_HANDLE gNetBufferListPool;

#define STREAM_EDITOR_NDIS_OBJ_TAG 'oneS'
#define STREAM_EDITOR_NBL_POOL_TAG 'pneS'
#define STREAM_EDITOR_FLAT_BUFFER_TAG 'bfeS'


DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD StreamEditEvtDriverUnload;

NTSTATUS
StreamEditNotify(
   FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   const GUID* filterKey,
   const FWPS_FILTER* filter
   )
{
   UNREFERENCED_PARAMETER(notifyType);
   UNREFERENCED_PARAMETER(filterKey);
   UNREFERENCED_PARAMETER(filter);

   return STATUS_SUCCESS;
}

NTSTATUS
RegisterCalloutForLayer(
   const GUID* layerKey,
   const GUID* calloutKey,
   _Inout_ void* deviceObject,
   _Out_ UINT32* calloutId
   )
/* ++

   This function registers callouts and filters that intercept TCP
   traffic at WFP FWPM_LAYER_STREAM_V4 or FWPM_LAYER_STREAM_V6 layer.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   FWPS_CALLOUT sCallout = {0};

   FWPM_FILTER filter = {0};
   FWPM_FILTER_CONDITION filterConditions[1] = {0}; 

   FWPM_CALLOUT mCallout = {0};
   FWPM_DISPLAY_DATA displayData = {0};

   BOOLEAN calloutRegistered = FALSE;

   sCallout.calloutKey = *calloutKey;
   sCallout.classifyFn = (configEditInline ? StreamInlineEditClassify :
                                             StreamOobEditClassify);
   sCallout.notifyFn = StreamEditNotify;

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

   displayData.name = L"Stream Edit Callout";
   displayData.description = L"Callout that finds and replaces a token from a TCP stream";

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

   filter.layerKey = *layerKey;
   filter.displayData.name = L"Stream Edit Filter";
   filter.displayData.description = L"Filter that finds and replaces a token from a TCP stream";

   filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
   filter.action.calloutKey = *calloutKey;
   filter.filterCondition = filterConditions;
   filter.numFilterConditions = 1;
   filter.subLayerKey = FWPM_SUBLAYER_UNIVERSAL;
   filter.weight.type = FWP_EMPTY; // auto-weight.

   filterConditions[0].fieldKey = (configInspectionOutbound ? FWPM_CONDITION_IP_REMOTE_PORT :
                                                              FWPM_CONDITION_IP_LOCAL_PORT);
   filterConditions[0].matchType = FWP_MATCH_EQUAL;
   filterConditions[0].conditionValue.type = FWP_UINT16;
   filterConditions[0].conditionValue.uint16 = configInspectionPort;

   status = FwpmFilterAdd(
               gEngineHandle,
               &filter,
               NULL,
               NULL);
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
      }
   }

   return status;
}

NTSTATUS
StreamEditRegisterCallout(
   const STREAM_EDITOR* streamEditor,
   _Inout_ void* deviceObject
   )
/* ++

   This function registers dynamic callouts and filters that intercept
   TCP traffic at WFP FWPM_LAYER_STREAM_V4 and FWPM_LAYER_STREAM_V6 
   layer.

   Callouts and filters will be removed during DriverUnload.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   BOOLEAN engineOpened = FALSE;
   BOOLEAN inTransaction = FALSE;

   FWPM_SESSION session = {0};

   UNREFERENCED_PARAMETER(streamEditor);

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

   status = RegisterCalloutForLayer(
               &FWPM_LAYER_STREAM_V4,
               &STREAM_EDITOR_STREAM_CALLOUT_V4,
               deviceObject,
               &gCalloutIdV4
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = RegisterCalloutForLayer(
               &FWPM_LAYER_STREAM_V6,
               &STREAM_EDITOR_STREAM_CALLOUT_V6,
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
         NTSTATUS abortStatus;
         abortStatus = FwpmTransactionAbort(gEngineHandle);
         _Analysis_assume_(NT_SUCCESS(abortStatus));
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
StreamEditUnregisterCallout(void)
{
   FwpmEngineClose(gEngineHandle);
   gEngineHandle = NULL;

   FwpsCalloutUnregisterById(gCalloutIdV6);
   FwpsCalloutUnregisterById(gCalloutIdV4);
}

_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void
StreamEditEvtDriverUnload(
   _In_ WDFDRIVER driverObject
   )
{

   UNREFERENCED_PARAMETER(driverObject);

   if (!configEditInline)
   {
      OobEditShutdown(&gStreamEditor);
   }

   if (gStreamEditor.scratchBuffer != NULL)
   {
      ExFreePoolWithTag(
         gStreamEditor.scratchBuffer,
         STREAM_EDITOR_FLAT_BUFFER_TAG
         );

   }

   StreamEditUnregisterCallout();

   FwpsInjectionHandleDestroy(gInjectionHandle);

   NdisFreeNetBufferListPool(gNetBufferListPool);
   NdisFreeGenericObject(gNdisGenericObj);

   IoFreeMdl(gStringToReplaceMdl);
}

NTSTATUS
StreamEditLoadConfig(
   const WDFKEY key
   )
{
   NTSTATUS status = STATUS_SUCCESS;
   DECLARE_CONST_UNICODE_STRING(stringToFindKey, L"StringToFind");
   DECLARE_CONST_UNICODE_STRING(stringToReplaceKey, L"StringToReplace");
   DECLARE_CONST_UNICODE_STRING(inspectionPortKey, L"InspectionPort");
   DECLARE_CONST_UNICODE_STRING(editInlineKey, L"EditInline");
   DECLARE_CONST_UNICODE_STRING(inspectOutboundKey, L"InspectOutbound");

   UNICODE_STRING stringValue;
   WCHAR buffer[128];
   USHORT requiredSize;
   ULONG valueSize;
   ULONG ulongValue;

   stringValue.Buffer = buffer;
   stringValue.Length = 0;
   stringValue.MaximumLength = sizeof(buffer) - sizeof(buffer[0]);

   if (NT_SUCCESS(
         WdfRegistryQueryUnicodeString(
            key,
            &stringToFindKey,
            &requiredSize,
            &stringValue
            )))
   {
      stringValue.Buffer[stringValue.Length/sizeof(stringValue.Buffer[0])] = 
         UNICODE_NULL;

      status = RtlUnicodeToMultiByteN(
                  configStringToFind,
                  sizeof(configStringToFind) - 1,
                  &valueSize,
                  stringValue.Buffer,
                  (ULONG)requiredSize
                  );

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }

      configStringToFind[valueSize] = '\0';
   }

   if (NT_SUCCESS(
         WdfRegistryQueryUnicodeString(
            key,
            &stringToReplaceKey,
            &requiredSize,
            &stringValue
            )))
   {
      status = RtlUnicodeToMultiByteN(
                  configStringToReplace,
                  sizeof(configStringToReplace) - 1,
                  &valueSize,
                  stringValue.Buffer,
                  (ULONG)requiredSize
                  );

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }

      configStringToReplace[valueSize] = '\0';
   }

   if (NT_SUCCESS(
         WdfRegistryQueryULong(
            key,
            &inspectionPortKey,
            &ulongValue
            )))
   {
      configInspectionPort = (USHORT) ulongValue;
   }

   if (NT_SUCCESS(
         WdfRegistryQueryULong(
            key,
            &editInlineKey,
            &ulongValue
            )))
   {
      configEditInline = (ulongValue != 0);
   }

   if (NT_SUCCESS(
         WdfRegistryQueryULong(
            key,
            &inspectOutboundKey,
            &ulongValue
            )))
   {
      configInspectionOutbound = (ulongValue != 0);
   }

Exit:
   return status;
}

NTSTATUS
StreamEditInitDriverObjects(
   _Inout_ DRIVER_OBJECT* driverObject,
   const UNICODE_STRING* registryPath,
   _Out_ WDFDRIVER* pDriver,
   _Out_ WDFDEVICE* pDevice
   )
{
   NTSTATUS status;
   WDF_DRIVER_CONFIG config;
   PWDFDEVICE_INIT pInit = NULL;

   WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

   config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
   config.EvtDriverUnload = StreamEditEvtDriverUnload;

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

   WdfDeviceInitSetCharacteristics(pInit, FILE_AUTOGENERATED_DEVICE_NAME, TRUE);
   WdfDeviceInitSetDeviceType(pInit, FILE_DEVICE_NETWORK);
   WdfDeviceInitSetCharacteristics(pInit, FILE_DEVICE_SECURE_OPEN, TRUE);
   status = WdfDeviceCreate(&pInit, WDF_NO_OBJECT_ATTRIBUTES, pDevice);

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
   WDFDEVICE device;
   WDFDRIVER driver;
   WDFKEY configKey;
   NET_BUFFER_LIST_POOL_PARAMETERS nblPoolParams = {0};

   // Request NX Non-Paged Pool when available
   ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

   status = StreamEditInitDriverObjects(
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

   status = StreamEditLoadConfig(configKey);

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   gStringToReplaceMdl = IoAllocateMdl(
                           configStringToReplace,
                           (ULONG) strlen(configStringToReplace),
                           FALSE,
                           FALSE,
                           NULL
                           );
   if (gStringToReplaceMdl == NULL)
   {
      status = STATUS_NO_MEMORY;
      goto Exit;
   }

   MmBuildMdlForNonPagedPool(gStringToReplaceMdl);

   gNdisGenericObj = NdisAllocateGenericObject(
                        driverObject, 
                        STREAM_EDITOR_NDIS_OBJ_TAG, 
                        0
                        );

   if (gNdisGenericObj == NULL)
   {
      status = STATUS_NO_MEMORY;
      goto Exit;
   }

   nblPoolParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
   nblPoolParams.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
   nblPoolParams.Header.Size = sizeof(nblPoolParams);

   nblPoolParams.fAllocateNetBuffer = TRUE;
   nblPoolParams.DataSize = 0;

   nblPoolParams.PoolTag = STREAM_EDITOR_NBL_POOL_TAG;

   gNetBufferListPool = NdisAllocateNetBufferListPool(
                           gNdisGenericObj,
                           &nblPoolParams
                           );

   if (gNetBufferListPool == NULL)
   {
      status = STATUS_NO_MEMORY;
      goto Exit;
   }

   status = FwpsInjectionHandleCreate(
               AF_UNSPEC,
               FWPS_INJECTION_TYPE_STREAM,
               &gInjectionHandle
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   gWdmDevice = WdfDeviceWdmGetDeviceObject(device);

   status = StreamEditRegisterCallout(
               &gStreamEditor,
               gWdmDevice
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   if (configEditInline)
   {
      InlineEditInit(&gStreamEditor);
   }
   else
   {

      status = OobEditInit(&gStreamEditor);

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }
   }

Exit:
   
   if (!NT_SUCCESS(status))
   {
      if (gEngineHandle != NULL)
      {
         StreamEditUnregisterCallout();
      }
      if (gInjectionHandle != NULL)
      {
         FwpsInjectionHandleDestroy(gInjectionHandle);
      }
      if (gNetBufferListPool != NULL)
      {
         NdisFreeNetBufferListPool(gNetBufferListPool);
      }
      if (gNdisGenericObj != NULL)
      {
         NdisFreeGenericObject(gNdisGenericObj);
      }
      if (gStringToReplaceMdl != NULL)
      {
         IoFreeMdl(gStringToReplaceMdl);
      }
   }

   return status;
}

BOOLEAN
StreamCopyDataForInspection(
   _Inout_ STREAM_EDITOR* streamEditor,
   const FWPS_STREAM_DATA* streamData
   )
/* ++

   This function copies stream data described by the FWPS_STREAM_DATA
   structure into a flat buffer.

-- */

{
   SIZE_T bytesCopied;

   size_t existingDataLength = streamEditor->dataLength;
   NT_ASSERT(streamEditor->dataOffset == 0);
   if (streamEditor->bufferSize - existingDataLength < streamData->dataLength)
   {
      size_t newBufferSize = (streamData->dataLength + existingDataLength) * 2;
      void* newBuffer = ExAllocatePoolWithTag(
                           NonPagedPool,
                           newBufferSize,
                           STREAM_EDITOR_FLAT_BUFFER_TAG
                           );

      if (newBuffer != NULL)
      {
         if (existingDataLength > 0)
         {
            NT_ASSERT(streamEditor->scratchBuffer != NULL);
            RtlCopyMemory(
               newBuffer, 
               streamEditor->scratchBuffer, 
               existingDataLength
               );
         }
      }

      if (streamEditor->scratchBuffer != NULL)
      {
         ExFreePoolWithTag(
            streamEditor->scratchBuffer, 
            STREAM_EDITOR_FLAT_BUFFER_TAG
            );

         streamEditor->scratchBuffer = NULL;
         streamEditor->bufferSize = 0;
         streamEditor->dataLength = 0;
      }

      if (newBuffer != NULL)
      {
         streamEditor->scratchBuffer = newBuffer;
         streamEditor->bufferSize = newBufferSize;
         streamEditor->dataLength = existingDataLength;
      }
      else
      {
         return FALSE;
      }
   }

   FwpsCopyStreamDataToBuffer(
      streamData,
      (BYTE*)streamEditor->scratchBuffer + streamEditor->dataLength,
      streamData->dataLength,
      &bytesCopied
      );

   NT_ASSERT(bytesCopied == streamData->dataLength);

   streamEditor->dataLength += bytesCopied;

   return TRUE;
}

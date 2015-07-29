/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    WMI.C

Abstract:

    This module handles all the WMI Irps.

Environment:

    Kernel mode

--*/

#include "busenum.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,Bus_WmiRegistration)
#pragma alloc_text(PAGE,Bus_EvtStdDataSetItem)
#pragma alloc_text(PAGE,Bus_EvtStdDataSetInstance)
#pragma alloc_text(PAGE,Bus_EvtStdDataQueryInstance)
#endif

NTSTATUS
Bus_WmiRegistration(
    WDFDEVICE      Device
    )
/*++
Routine Description

    Registers with WMI as a data provider for this
    instance of the device

--*/
{
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;
    PFDO_DEVICE_DATA deviceData;
    NTSTATUS status;
    DECLARE_CONST_UNICODE_STRING(busRsrcName, BUSRESOURCENAME);

    PAGED_CODE();

    deviceData = FdoGetData(Device);

    //
    // Register WMI classes.
    // First specify the resource name which contain the binary mof resource.
    //
    status = WdfDeviceAssignMofResourceName(Device, &busRsrcName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &ToasterBusInformation_GUID);
    providerConfig.MinInstanceBufferSize = sizeof(TOASTER_BUS_WMI_STD_DATA);

    //
    // You would want to create a WDFWMIPROVIDER handle separately if you are
    // going to dynamically create instances on the provider.  Since we are
    // statically creating one instance, there is no need to create the provider
    // handle.
    //
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

    //
    // By setting Regsiter to TRUE, we tell the framework to create a provider
    // as part of the Instance creation call. This eliminates the need to
    // call WdfWmiProviderRegister.
    //
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = Bus_EvtStdDataQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance = Bus_EvtStdDataSetInstance;
    instanceConfig.EvtWmiInstanceSetItem = Bus_EvtStdDataSetItem;

    status = WdfWmiInstanceCreate(
        Device,
        &instanceConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
        );

    if (NT_SUCCESS(status)) {
        deviceData->StdToasterBusData.ErrorCount = 0;
    }

    return status;
}

//
// WMI System Call back functions
//
NTSTATUS
Bus_EvtStdDataSetItem(
    IN  WDFWMIINSTANCE WmiInstance,
    IN  ULONG DataItemId,
    IN  ULONG InBufferSize,
    IN  PVOID InBuffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    an instance.

Arguments:

    WmiInstance is the instance being set

    DataItemId has the id of the data item being set

    InBufferSize has the size of the data item passed

    InBuffer has the new values for the data item

Return Value:

    status

--*/
{
    PFDO_DEVICE_DATA    fdoData;

    PAGED_CODE();

    fdoData = FdoGetData(WdfWmiInstanceGetDevice(WmiInstance));
    //
    // TODO: Use generated header's #defines for constants and sizes
    // (for the remainder of the file)
    //
    if (DataItemId == 2) {
        if (InBufferSize < sizeof(ULONG)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        BusEnumDebugLevel = fdoData->StdToasterBusData.DebugPrintLevel =
            *((PULONG)InBuffer);

        return STATUS_SUCCESS;
    }

    //
    // All other fields are read only
    //
    return STATUS_WMI_READ_ONLY;
}

NTSTATUS
Bus_EvtStdDataSetInstance(
    IN  WDFWMIINSTANCE WmiInstance,
    IN  ULONG InBufferSize,
    IN  PVOID InBuffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    an instance.

Arguments:

    WmiInstance is the instance being set

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block

Return Value:

    status

--*/
{
    PFDO_DEVICE_DATA   fdoData;

    UNREFERENCED_PARAMETER(InBufferSize);

    PAGED_CODE();

    fdoData = FdoGetData(WdfWmiInstanceGetDevice(WmiInstance));

    //
    // We will update only writable elements.
    //
    BusEnumDebugLevel = fdoData->StdToasterBusData.DebugPrintLevel =
        ((PTOASTER_BUS_WMI_STD_DATA)InBuffer)->DebugPrintLevel;

    return STATUS_SUCCESS;
}

NTSTATUS
Bus_EvtStdDataQueryInstance(
    _In_  WDFWMIINSTANCE WmiInstance,
    _In_  ULONG OutBufferSize,
    _Out_writes_bytes_to_(OutBufferSize, *BufferUsed) PVOID OutBuffer,
    _Out_ PULONG BufferUsed
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a wmi instance

Arguments:

    WmiInstance is the instance being set

    OutBufferSize on has the maximum size available to write the data
        block.

    OutBuffer on return is filled with the returned data block

    BufferUsed pointer containing how many bytes are required (upon failure) or
        how many bytes were used (upon success)

Return Value:

    status

--*/
{
    PFDO_DEVICE_DATA fdoData;

    UNREFERENCED_PARAMETER(OutBufferSize);

    PAGED_CODE();

    fdoData = FdoGetData(WdfWmiInstanceGetDevice(WmiInstance));

    *BufferUsed = sizeof (fdoData->StdToasterBusData);
    if (OutBufferSize < sizeof(fdoData->StdToasterBusData)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    * (PTOASTER_BUS_WMI_STD_DATA) OutBuffer = fdoData->StdToasterBusData;

    return STATUS_SUCCESS;
}


/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    BusPdo.c

Abstract:

    This module handles plug & play calls for the child device (PDO).

Environment:

    kernel mode only

--*/

#include "busenum.h"

ULONG BusEnumDebugLevel;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Bus_CreatePdo)
#pragma alloc_text(PAGE, Bus_EvtDeviceListCreatePdo)
#endif

NTSTATUS
Bus_EvtChildListIdentificationDescriptionDuplicate(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SourceIdentificationDescription,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER DestinationIdentificationDescription
    )
/*++

Routine Description:

    It is called when the framework needs to make a copy of a description.
    This happens when a request is made to create a new child device by
    calling WdfChildListAddOrUpdateChildDescriptionAsPresent.
    If this function is left unspecified, RtlCopyMemory will be used to copy the
    source description to destination. Memory for the description is managed by the
    framework.

    NOTE:   Callback is invoked with an internal lock held.  So do not call out
    to any WDF function which will require this lock
    (basically any other WDFCHILDLIST api)

Arguments:

    DeviceList - Handle to the default WDFCHILDLIST created by the framework.
    SourceIdentificationDescription - Description of the child being created -memory in
                            the calling thread stack.
    DestinationIdentificationDescription - Created by the framework in nonpaged pool.

Return Value:

    NT Status code.

--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION src, dst;
    size_t safeMultResult;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceList);

    src = CONTAINING_RECORD(SourceIdentificationDescription,
                            PDO_IDENTIFICATION_DESCRIPTION,
                            Header);
    dst = CONTAINING_RECORD(DestinationIdentificationDescription,
                            PDO_IDENTIFICATION_DESCRIPTION,
                            Header);

    dst->SerialNo = src->SerialNo;
    dst->CchHardwareIds = src->CchHardwareIds;
    status = RtlSizeTMult(dst->CchHardwareIds,
                                  sizeof(WCHAR),
                                  &safeMultResult
                                  );
    if(!NT_SUCCESS(status)){
        return status;
    }

    dst->HardwareIds = (PWCHAR) ExAllocatePoolWithTag(
        NonPagedPool,
        safeMultResult,
        BUS_TAG);

    if (dst->HardwareIds == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(dst->HardwareIds,
                  src->HardwareIds,
                  dst->CchHardwareIds * sizeof(WCHAR));

    return STATUS_SUCCESS;
}

BOOLEAN
Bus_EvtChildListIdentificationDescriptionCompare(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription
    )
/*++

Routine Description:

    It is called when the framework needs to compare one description with another.
    Typically this happens whenever a request is made to add a new child device.
    If this function is left unspecified, RtlCompareMemory will be used to compare the
    descriptions.

    NOTE:   Callback is invoked with an internal lock held.  So do not call out
    to any WDF function which will require this lock
    (basically any other WDFCHILDLIST api)

Arguments:

    DeviceList - Handle to the default WDFCHILDLIST created by the framework.

Return Value:

   TRUE or FALSE.

--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION lhs, rhs;

    UNREFERENCED_PARAMETER(DeviceList);

    lhs = CONTAINING_RECORD(FirstIdentificationDescription,
                            PDO_IDENTIFICATION_DESCRIPTION,
                            Header);
    rhs = CONTAINING_RECORD(SecondIdentificationDescription,
                            PDO_IDENTIFICATION_DESCRIPTION,
                            Header);

    return (lhs->SerialNo == rhs->SerialNo) ? TRUE : FALSE;
}

#pragma prefast(push)
#pragma prefast(disable:6101, "No need to assign IdentificationDescription")

VOID
Bus_EvtChildListIdentificationDescriptionCleanup(
    _In_ WDFCHILDLIST DeviceList,
    _Out_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
/*++

Routine Description:

    It is called to free up any memory resources allocated as part of the description.
    This happens when a child device is unplugged or ejected from the bus.
    Memory for the description itself will be freed by the framework.

Arguments:

    DeviceList - Handle to the default WDFCHILDLIST created by the framework.

    IdentificationDescription - Description of the child being deleted

Return Value:


--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION pDesc;


    UNREFERENCED_PARAMETER(DeviceList);

    pDesc = CONTAINING_RECORD(IdentificationDescription,
                              PDO_IDENTIFICATION_DESCRIPTION,
                              Header);

    if (pDesc->HardwareIds != NULL) {
        ExFreePool(pDesc->HardwareIds);
        pDesc->HardwareIds = NULL;
    }
}

#pragma prefast(pop) // disable:6101

NTSTATUS
Bus_EvtDeviceListCreatePdo(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    PWDFDEVICE_INIT ChildInit
    )
/*++

Routine Description:

    Called by the framework in response to Query-Device relation when
    a new PDO for a child device needs to be created.

Arguments:

    DeviceList - Handle to the default WDFCHILDLIST created by the framework as part
                        of FDO.

    IdentificationDescription - Decription of the new child device.

    ChildInit - It's a opaque structure used in collecting device settings
                    and passed in as a parameter to CreateDevice.

Return Value:

    NT Status code.

--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION pDesc;

    PAGED_CODE();

    pDesc = CONTAINING_RECORD(IdentificationDescription,
                              PDO_IDENTIFICATION_DESCRIPTION,
                              Header);

    return Bus_CreatePdo(WdfChildListGetDevice(DeviceList),
                         ChildInit,
                         pDesc->HardwareIds,
                         pDesc->SerialNo);
}


NTSTATUS
Bus_CreatePdo(
    _In_ WDFDEVICE       Device,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_reads_(MAX_INSTANCE_ID_LEN) PCWSTR HardwareIds,
    _In_ ULONG           SerialNo
    )
/*++

Routine Description:

    This routine creates and initialize a PDO.

Arguments:

Return Value:

    NT Status code.

--*/
{
    NTSTATUS                    status;
    PPDO_DEVICE_DATA            pdoData = NULL;
    WDFDEVICE                   hChild = NULL;
    WDF_QUERY_INTERFACE_CONFIG  qiConfig;
    WDF_OBJECT_ATTRIBUTES       pdoAttributes;
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES powerCaps;
    TOASTER_INTERFACE_STANDARD  ToasterInterface;
    DECLARE_CONST_UNICODE_STRING(compatId, BUSENUM_COMPATIBLE_IDS);
    DECLARE_CONST_UNICODE_STRING(deviceLocation, L"Toaster Bus 0");
    DECLARE_UNICODE_STRING_SIZE(buffer, MAX_INSTANCE_ID_LEN);
    DECLARE_UNICODE_STRING_SIZE(deviceId, MAX_INSTANCE_ID_LEN);

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);

    KdPrint(("Entered Bus_CreatePdo\n"));

    //
    // Set DeviceType
    //
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RtlInitUnicodeString(&deviceId, HardwareIds);

    status = WdfPdoInitAssignDeviceID(DeviceInit, &deviceId);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // NOTE: same string  is used to initialize hardware id too
    //
    status = WdfPdoInitAddHardwareID(DeviceInit, &deviceId);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status =  RtlUnicodeStringPrintf(&buffer, L"%02d", SerialNo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfPdoInitAssignInstanceID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Provide a description about the device. This text is usually read from
    // the device. In the case of USB device, this text comes from the string
    // descriptor. This text is displayed momentarily by the PnP manager while
    // it's looking for a matching INF. If it finds one, it uses the Device
    // Description from the INF file or the friendly name created by
    // coinstallers to display in the device manager. FriendlyName takes
    // precedence over the DeviceDesc from the INF file.
    //
    status = RtlUnicodeStringPrintf( &buffer,
                                     L"Microsoft_Eliyas_Toaster_%02d",
                                     SerialNo );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    status = WdfPdoInitAddDeviceText(DeviceInit,
                                    &buffer,
                                    &deviceLocation,
                                     0x409 );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WdfPdoInitSetDefaultLocale(DeviceInit, 0x409);

    //
    // Initialize the attributes to specify the size of PDO device extension.
    // All the state information private to the PDO will be tracked here.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, PDO_DEVICE_DATA);

    status = WdfDeviceCreate(&DeviceInit, &pdoAttributes, &hChild);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the device context.
    //
    pdoData = PdoGetData(hChild);

    pdoData->SerialNo = SerialNo;

    //
    // Set some properties for the child device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.Removable         = WdfTrue;
    pnpCaps.EjectSupported    = WdfTrue;
    pnpCaps.SurpriseRemovalOK = WdfTrue;

    pnpCaps.Address  = SerialNo;
    pnpCaps.UINumber = SerialNo;

    WdfDeviceSetPnpCapabilities(hChild, &pnpCaps);

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);

    powerCaps.DeviceD1 = WdfTrue;
    powerCaps.WakeFromD1 = WdfTrue;
    powerCaps.DeviceWake = PowerDeviceD1;

    powerCaps.DeviceState[PowerSystemWorking]   = PowerDeviceD1;
    powerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
    powerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
    powerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD2;
    powerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    powerCaps.DeviceState[PowerSystemShutdown]  = PowerDeviceD3;

    WdfDeviceSetPowerCapabilities(hChild, &powerCaps);

    //
    // Create a custom interface so that other drivers can
    // query (IRP_MN_QUERY_INTERFACE) and use our callbacks directly.
    //
    RtlZeroMemory(&ToasterInterface, sizeof(ToasterInterface));

    ToasterInterface.InterfaceHeader.Size = sizeof(ToasterInterface);
    ToasterInterface.InterfaceHeader.Version = 1;
    ToasterInterface.InterfaceHeader.Context = (PVOID) hChild;

    //
    // Let the framework handle reference counting.
    //
    ToasterInterface.InterfaceHeader.InterfaceReference =
        WdfDeviceInterfaceReferenceNoOp;
    ToasterInterface.InterfaceHeader.InterfaceDereference =
        WdfDeviceInterfaceDereferenceNoOp;

    ToasterInterface.GetCrispinessLevel  = Bus_GetCrispinessLevel;
    ToasterInterface.SetCrispinessLevel  = Bus_SetCrispinessLevel;
    ToasterInterface.IsSafetyLockEnabled = Bus_IsSafetyLockEnabled;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
                                    (PINTERFACE) &ToasterInterface,
                                    &GUID_TOASTER_INTERFACE_STANDARD,
                                    NULL);
    //
    // If you have multiple interfaces, you can call WdfDeviceAddQueryInterface
    // multiple times to add additional interfaces.
    //
    status = WdfDeviceAddQueryInterface(hChild, &qiConfig);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}


BOOLEAN
Bus_GetCrispinessLevel(
    IN   WDFDEVICE ChildDevice,
    OUT  PUCHAR Level
    )
/*++

Routine Description:

    This routine gets the current crispiness level of the toaster.

Arguments:

    Context        pointer to  PDO device extension
    Level          crispiness level of the device

Return Value:

    TRUE or FALSE

--*/
{
    UNREFERENCED_PARAMETER(ChildDevice);

    //
    // Validate the context to see if it's really a pointer
    // to PDO's device extension. You can store some kind
    // of signature in the PDO for this purpose
    //

    KdPrint(("GetCrispnessLevel\n"));

    *Level = 10;
    return TRUE;
}

BOOLEAN
Bus_SetCrispinessLevel(
    IN   WDFDEVICE ChildDevice,
    IN   UCHAR Level
    )
/*++

Routine Description:

    This routine sets the current crispiness level of the toaster.

Arguments:

    Context        pointer to  PDO device extension
    Level          crispiness level of the device

Return Value:

    TRUE or FALSE

--*/
{
    UNREFERENCED_PARAMETER(ChildDevice);
    UNREFERENCED_PARAMETER(Level);

    KdPrint(("SetCrispnessLevel\n"));

    return TRUE;
}

BOOLEAN
Bus_IsSafetyLockEnabled(
    IN   WDFDEVICE ChildDevice
    )
/*++

Routine Description:

    Routine to check whether safety lock is enabled

Arguments:

    Context        pointer to  PDO device extension

Return Value:

    TRUE or FALSE

--*/
{
    UNREFERENCED_PARAMETER(ChildDevice);

    KdPrint(("IsSafetyLockEnabled\n"));

    return TRUE;
}


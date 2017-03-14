/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Rawpdo.c

Abstract:

    This modules has routines to enumerate dip switches on the board
    as child devices. We use dynamic enumeration interfaces to manage
    child devices.

Environment:

    Kernel mode

--*/

#include <osrusbfx2.h>
#include "rawpdo.h"

#include "rawpdo.tmh"

VOID
OsrFxInitChildList(
    IN PWDFDEVICE_INIT  DeviceInit
    )
/*++

Routine Description:

    This routine is called from the EvtDeviceAdd routine to initialize
    the default child list.

Arguments:


Return Value:

    NT status value

--*/
{
    WDF_CHILD_LIST_CONFIG      config;

    //
    // Init the default child list so that we can enumerate a raw PDO
    //
    WDF_CHILD_LIST_CONFIG_INIT(&config,
                                sizeof(PDO_IDENTIFICATION_DESCRIPTION),
                                OsrEvtDeviceListCreatePdo // callback to create a child device.
                                );
    //
    // Tell the framework to use the built-in devicelist to track the state
    // of the device based on the configuration we just created.
    //
    WdfFdoInitSetDefaultChildListConfig(DeviceInit,
                                         &config,
                                         WDF_NO_OBJECT_ATTRIBUTES);

    return;

}


NTSTATUS
OsrEvtDeviceListCreatePdo(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    PWDFDEVICE_INIT ChildInit
    )
/*++

Routine Description:

    Called by the framework in response to Query-Device relation when
    a new PDO for a child device needs to be created.

Arguments:

    DeviceList - Handle to the default WDFCHILDLIST created by the
                        framework as part of FDO.

    IdentificationDescription - Decription of the new child device.

    ChildInit - It's a opaque structure used in collecting device settings
                    and passed in as a parameter to CreateDevice.

Return Value:

    NT Status code.

--*/
{
    NTSTATUS                        status;
    WDFDEVICE                       hChild = NULL;
    PPDO_IDENTIFICATION_DESCRIPTION pDesc;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    DECLARE_CONST_UNICODE_STRING(deviceId, OSRUSBFX2_SWITCH_DEVICE_ID );
    DECLARE_CONST_UNICODE_STRING(hardwareId, OSRUSBFX2_SWITCH_DEVICE_ID );
    DECLARE_CONST_UNICODE_STRING(deviceLocation, L"OSR USB-FX2 Learning Kit" );
    DECLARE_UNICODE_STRING_SIZE(buffer, DEVICE_DESC_LENGTH);

    UNREFERENCED_PARAMETER(DeviceList);

    pDesc = CONTAINING_RECORD(IdentificationDescription,
                              PDO_IDENTIFICATION_DESCRIPTION,
                              Header);

    //
    // Mark the device RAW so that the child device can be started
    // and accessed without requiring a function driver. Since we are
    // creating a RAW PDO, we must provide a class guid.
    //
    status = WdfPdoInitAssignRawDevice(ChildInit, &GUID_DEVCLASS_OSRUSBFX2);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    //
    // Since our devices for switches can trigger nuclear explosion,
    // we must protect them from random users sending I/Os.
    //
    status = WdfDeviceInitAssignSDDLString(ChildInit,
                                           &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = WdfPdoInitAssignDeviceID(ChildInit, &deviceId);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    //
    // On XP and later, there is no need to provide following IDs for raw pdos.
    // BusQueryHardwareIDs but on On Win2K, we must provide a HWID and a NULL
    // section in the INF to get the device installed without any problem.
    //
    status = WdfPdoInitAddHardwareID(ChildInit, &hardwareId);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    //
    // Since we are enumerating more than one children, we must
    // provide a BusQueryInstanceID. If we don't, system will throw
    // CA bugcheck.
    //
    status =  RtlUnicodeStringPrintf(&buffer, L"%02u", pDesc->SwitchNumber);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfPdoInitAssignInstanceID(ChildInit, &buffer);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Provide a description about the device. This text is usually read from
    // the device. In the case of USB device, this text comes from the string
    // descriptor. This text is displayed momentarily by the PnP manager while
    // it's looking for a matching INF. If it finds one, it uses the Device
    // Description from the INF file to display in the device manager.
    // Since our device is raw device and we don't provide any hardware ID
    // to match with an INF, this text will be displayed in the device manager.
    //
    status = RtlUnicodeStringPrintf(&buffer,
                                    L"OsrUsbFX2 RawPdo For Switch %02u",
                                    pDesc->SwitchNumber);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    status = WdfPdoInitAddDeviceText(ChildInit,
                                        &buffer,
                                        &deviceLocation,
                                        0x409);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    WdfPdoInitSetDefaultLocale(ChildInit, 0x409);

    status = WdfDeviceCreate(&ChildInit, WDF_NO_OBJECT_ATTRIBUTES, &hChild);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;

    }

    //
    // Set idle-time out on the child device. This is required to allow
    // the parent device to idle-out when there are no active I/O.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = 1000; // 1-sec

    status = WdfDeviceAssignS0IdleSettings(hChild, &idleSettings);
    if ( !NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                "WdfDeviceSetPowerPolicyS0IdlePolicy failed %x\n", status);
        return status;
    }

    return status;

Cleanup:

    TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,"CreatePdo failed %x\n", status);

    //
    // On error, framework will cleanup all the resources when it deletes
    // the device. So there is nothing to do.
    //

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
OsrFxEnumerateChildren(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine configures a continuous reader on the
    interrupt endpoint.

Arguments:


Return Value:

    NT status value

--*/
{
    WDFCHILDLIST    list;
    UCHAR           i;
    NTSTATUS        status;
    PDEVICE_CONTEXT pDeviceContext;

    pDeviceContext = GetDeviceContext(Device);

    list = WdfFdoGetDefaultChildList(Device);

    WdfChildListBeginScan(list);

    //
    // A call to WdfChildListBeginScan indicates to the framework that the
    // driver is about to scan for dynamic children. If the driver doesn't
    // call either WdfChildListUpdateChildDescriptionAsPresent  or
    // WdfChildListMarkAllChildDescriptionsPresent before WdfChildListEndScan is,
    // called, all the previously reported children will be reported as missing
    // to the PnP subsystem.
    //
    for(i=0; i< RTL_BITS_OF(UCHAR); i++) {

        //
        // Report every set bit in the switchstate as a child device.
        //
        if(pDeviceContext->CurrentSwitchState & (1<<i)) {

            PDO_IDENTIFICATION_DESCRIPTION description;

            //
            // Initialize the description with the information about the newly
            // plugged in device.
            //
            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
                &description.Header,
                sizeof(description)
                );

            //
            // Since switches are marked in the wrong order on the board,
            // we will fix it here so that the DM display matches with the
            // board.
            //
            description.SwitchNumber = RTL_BITS_OF(UCHAR)-i;

            TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
                     "Switch %d is ON\n", description.SwitchNumber);

            //
            // Call the framework to add this child to the devicelist. This call
            // will internaly call our DescriptionCompare callback to check
            // whether this device is a new device or existing device. If
            // it's a new device, the framework will call DescriptionDuplicate to create
            // a copy of this description in nonpaged pool.
            // The actual creation of the child device will happen when the framework
            // receives QUERY_DEVICE_RELATION request from the PNP manager in
            // response to InvalidateDevice relation call made as part of adding
            // a new child.
            //
            status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
                            list,
                            &description.Header,
                            NULL); // AddressDescription

            if (status == STATUS_OBJECT_NAME_EXISTS) {
            }

        }
    }


    WdfChildListEndScan(list);

    return;
}


/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    acpispecific.c

Abstract:

    This module defines global variables that are used by the common library
    for the platform extension.


Environment:

    Kernel mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include "pch.h"
#include "acpispecific.h"

#if defined(EVENT_TRACING)
#include "acpispecific.tmh"
#endif

//
// To add new devices that will be accpeted by the platform extension,
// follow to the example below.
//

//
// Step 1: Define a unique device type for each new device.
// (See acpispecific.h for details)
//

//
// Step 2: Define a name for each new device.
// (See acpispecific.h for details)
//

//
//--------------------------------------------------------------------- Globals
//

//
// The common library will consume the PepDeviceMatchArray to determine
// whether a device should be accpeted by the platform extension.
// And then the common library will consult PepDeviceDefinitionArray for
// the native methods and device-specific notification handlers of the accpeted
// devices.
//

//
// Step 3: Edit the PepDeviceMatchArray to tell the common library which device
// should be accepted by the platform extension. For each accepted device,
// specify which kind of notification (ACPI/DPM/PPM) the platform extension
// will handle and also specify the way platform extension should match its
// name (partial match or full match) when identifying the device.
// In this example, the sample platform extension only handles ACPI
// notifications for all devices and all devices should be matched by their
// full names.
//

PEP_DEVICE_MATCH PepDeviceMatchArray[] = {

    {PEP_DEVICE_TYPE_ROOT,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_ROOT_WCHAR,
     PepDeviceIdMatchFull},

    {PEP_DEVICE_TYPE_BUSD,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_BUSD_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull},

     {PEP_DEVICE_TYPE_GPIO,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_GPIO_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull},

     {PEP_DEVICE_TYPE_SPBD,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_SPBD_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull},

     {PEP_DEVICE_TYPE_TST1,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_TST1_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull},

     {PEP_DEVICE_TYPE_TST2,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_TST2_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull},

     {PEP_DEVICE_TYPE_TST3,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_TST3_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull},

     {PEP_DEVICE_TYPE_PWRR,
     PEP_NOTIFICATION_CLASS_ACPI,
     SAMPLE_PWRR_ACPI_NAME_WCHAR,
     PepDeviceIdMatchFull}
};

ULONG PepDeviceMatchArraySize = ARRAYSIZE(PepDeviceMatchArray);

//
// Step 4: Define the list of native methods under each device.
// In this example, the platform extension natively handles:
//     1). _HID and _CID methods for \_SB.BUSD;
//     2). _HID method for \_SB.GPIO and \_SB.SPBD;
//     3). _ADR and _CRS methods for \_SB.BUSD.TST1;
//     4). _ADR, _CRS and _DEP methods for \_SB.BUSD.TST2;
//     5). _HID and _CRS methods for \_SB.TST3;
//     5). _HID, _STA, _ON, and _OFF methods for \_SB.PWRR.
//


PEP_OBJECT_INFORMATION RootNativeMethods[] =
{
    //
    // _Child Devices
    //

    {(ULONG)SAMPLE_BUSD_REAL_NAME, 0, 1, PepAcpiObjectTypeDevice},
    {(ULONG)SAMPLE_GPIO_REAL_NAME, 0, 1, PepAcpiObjectTypeDevice},
    {(ULONG)SAMPLE_SPBD_REAL_NAME, 0, 1, PepAcpiObjectTypeDevice},
    {(ULONG)SAMPLE_TST3_REAL_NAME, 0, 1, PepAcpiObjectTypeDevice}
};

PEP_OBJECT_INFORMATION BusdNativeMethods[] =
{
    // _HID
    {ACPI_OBJECT_NAME_HID, 0, 1, PepAcpiObjectTypeMethod},

    // _CID
    {ACPI_OBJECT_NAME_CID, 0, 1, PepAcpiObjectTypeMethod},

    //
    // _Child Devices
    //

    {(ULONG)SAMPLE_TST1_REAL_NAME, 0, 1, PepAcpiObjectTypeDevice},
    {(ULONG)SAMPLE_TST2_REAL_NAME, 0, 1, PepAcpiObjectTypeDevice},
};

PEP_OBJECT_INFORMATION GpioAndSpbdNativeMethods[] =
{
    // _HID
    {ACPI_OBJECT_NAME_HID, 0, 1, PepAcpiObjectTypeMethod},

    // _CID
    {ACPI_OBJECT_NAME_CID, 0, 1, PepAcpiObjectTypeMethod}
};

PEP_OBJECT_INFORMATION Tst1NativeMethods[] =
{
    // _ADR
    {ACPI_OBJECT_NAME_ADR, 0, 1, PepAcpiObjectTypeMethod},

    // _CRS
    {ACPI_OBJECT_NAME_CRS, 0, 1, PepAcpiObjectTypeMethod},

    // _DEP
    {ACPI_OBJECT_NAME_DEP, 0, 1, PepAcpiObjectTypeMethod}
};

PEP_OBJECT_INFORMATION Tst2NativeMethods[] =
{
    // _ADR
    {ACPI_OBJECT_NAME_ADR, 0, 1, PepAcpiObjectTypeMethod},

    // _CRS
    {ACPI_OBJECT_NAME_CRS, 0, 1, PepAcpiObjectTypeMethod}
};

PEP_OBJECT_INFORMATION Tst3NativeMethods[] =
{
    // _HID
    {ACPI_OBJECT_NAME_HID, 0, 1, PepAcpiObjectTypeMethod},

    // _CID
    {ACPI_OBJECT_NAME_CID, 0, 1, PepAcpiObjectTypeMethod},
};

PEP_OBJECT_INFORMATION PwrrNativeMethods[] =
{
    // _HID
    {ACPI_OBJECT_NAME_HID, 0, 1, PepAcpiObjectTypeMethod},

    // _STA
    {ACPI_OBJECT_NAME_STA, 0, 1, PepAcpiObjectTypeMethod},

    // _ON
    {ACPI_OBJECT_NAME_ON, 0, 1, PepAcpiObjectTypeMethod},

    // _OFF
    {ACPI_OBJECT_NAME_OFF, 0, 1, PepAcpiObjectTypeMethod},
};

//
// Step 5: Define the list of device-specific notification handlers
// for each device.
//
// In this example:
//     1) all devices define PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD;
//     2) for \_SB.BUSD, its _HID method will be handled synchronously
//         while its _CID method will be handled asynchronously;
//     3) for \_SB.GPIO and \_SB.SPBD, their _HID method will both be
//         handled synchronously;
//     4) for \_SB.BUSD.TST1 and \_SB.BUSD.TST2, their _ADR and _DEP
//         methods will be handled synchronously while _CRS method
//         will be handled asynchronously.
//     5) \_SB.BUSD.TST1 also synchronously handles
//         PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES.
//     6) \_SB.TST3 synchronously handles all
//         PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD except for _CID.
//         The processing for _CID is done in a self-managed manner (i.e. it is
//         processed asynchronously but does not rely on common library worker).
//         Hence there is no async worker registered.
//     7) \_SB.PWRR synchronously handles all
//         PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD.
//

PEP_DEVICE_NOTIFICATION_HANDLER RootNotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     RootSyncEvaluateControlMethod,
     NULL}
};

PEP_DEVICE_NOTIFICATION_HANDLER BusdNotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     BusdSyncEvaluateControlMethod,
     BusdWorkerCallbackEvaluateControlMethod}
};

PEP_DEVICE_NOTIFICATION_HANDLER GpioNotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     GpioSyncEvaluateControlMethod,
     NULL}
};

PEP_DEVICE_NOTIFICATION_HANDLER SpbdNotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     SpbdSyncEvaluateControlMethod,
     NULL}
};

PEP_DEVICE_NOTIFICATION_HANDLER Tst1NotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     Tst1SyncEvaluateControlMethod,
     Tst1WorkerCallbackEvaluateControlMethod},

     {PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES,
     Tst1SyncQueryControlResources,
     NULL}
};

PEP_DEVICE_NOTIFICATION_HANDLER Tst2NotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     Tst2SyncEvaluateControlMethod,
     Tst2WorkerCallbackEvaluateControlMethod}
};

PEP_DEVICE_NOTIFICATION_HANDLER Tst3NotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     Tst3SyncEvaluateControlMethod,
     NULL}
};

PEP_DEVICE_NOTIFICATION_HANDLER PwrrNotificationHandler[] = {
    {PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
     PwrrSyncEvaluateControlMethod,
     NULL}
};

//
// Step 6: Edit the PepDeviceDefinitionArray to register any native methods
// and device-specific notification handlers.
//

PEP_DEVICE_DEFINITION PepDeviceDefinitionArray[] = {

    // \_SB
    {PEP_DEVICE_TYPE_ROOT,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(RootNativeMethods),
    RootNativeMethods,
    ARRAYSIZE(RootNotificationHandler),
    RootNotificationHandler,
    0,
    NULL},

    // \_SB.BUSD
    {PEP_DEVICE_TYPE_BUSD,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(BusdNativeMethods),
    BusdNativeMethods,
    ARRAYSIZE(BusdNotificationHandler),
    BusdNotificationHandler,
    0,
    NULL},

    // \_SB.GPIO
    {PEP_DEVICE_TYPE_GPIO,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(GpioAndSpbdNativeMethods),
    GpioAndSpbdNativeMethods,
    ARRAYSIZE(GpioNotificationHandler),
    GpioNotificationHandler,
    0,
    NULL},

    // \_SB.SPBD
    {PEP_DEVICE_TYPE_SPBD,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(GpioAndSpbdNativeMethods),
    GpioAndSpbdNativeMethods,
    ARRAYSIZE(SpbdNotificationHandler),
    SpbdNotificationHandler,
    0,
    NULL},

    // \_SB.BUSD.TST1
    {PEP_DEVICE_TYPE_TST1,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(Tst1NativeMethods),
    Tst1NativeMethods,
    ARRAYSIZE(Tst1NotificationHandler),
    Tst1NotificationHandler,
    0,
    NULL},

    // \_SB.BUSD.TST2
    {PEP_DEVICE_TYPE_TST2,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(Tst2NativeMethods),
    Tst2NativeMethods,
    ARRAYSIZE(Tst2NotificationHandler),
    Tst2NotificationHandler,
    0,
    NULL},

    // \_SB.TST3
    {PEP_DEVICE_TYPE_TST3,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(Tst3NativeMethods),
    Tst3NativeMethods,
    ARRAYSIZE(Tst3NotificationHandler),
    Tst3NotificationHandler,
    0,
    NULL},

    // \_SB.PWRR
    {PEP_DEVICE_TYPE_PWRR,
    sizeof(PEP_ACPI_DEVICE),
    NULL,
    ARRAYSIZE(PwrrNativeMethods),
    PwrrNativeMethods,
    ARRAYSIZE(PwrrNotificationHandler),
    PwrrNotificationHandler,
    0,
    NULL},
};

ULONG PepDeviceDefinitionArraySize = ARRAYSIZE(PepDeviceDefinitionArray);

//
// Step 7: Implement the device-specific notification handlers.
// (See testdevice.c for details)
//

//
// Debug
//
volatile BOOLEAN _SAMPLE_PEP_DEBUG_ENABLED = TRUE;


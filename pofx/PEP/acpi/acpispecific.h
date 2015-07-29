/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    acpispecific.h

Abstract:

    This module defines constants for the sample platform extension.


Environment:

    Kernel mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include "pch.h"

//
//----------------------------------------------------------------- Definitions
//

//
// To add new devices that will be accpeted by the platform extension,
// follow to the example below.
//

//
// Step 1: Define a unique device type for each new device.
// In this example, the sample platform extension accepts the
// following devices:
//     0). \_SB: Acpi Root
//     1). \_SB.BUSD: Bus device;
//     2). \_SB.GPIO: GPIO controller;
//     3). \_SB.SPBD: SPB controller;
//     4). \_SB.BUSD.TST1: Test device 1;
//     5). \_SB.BUSD.TST2: Test device 2;
//     6). \_SB.TST3: Test device 3;
//     7). \_SB.PWRR: Power rail device.
//

#define PEP_DEVICE_TYPE_ROOT \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x0)

#define PEP_DEVICE_TYPE_BUSD \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x1)

#define PEP_DEVICE_TYPE_GPIO \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x2)

#define PEP_DEVICE_TYPE_SPBD \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x3)

#define PEP_DEVICE_TYPE_TST1 \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x4)

#define PEP_DEVICE_TYPE_TST2 \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x5)

#define PEP_DEVICE_TYPE_TST3 \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x6)

#define PEP_DEVICE_TYPE_PWRR \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeAcpi, \
                         PepAcpiMinorTypeDevice, \
                         0x7)

//
// Step 2: Define a name for each new device.
//

#define SAMPLE_ROOT_ANSI "\\_SB"
#define SAMPLE_ROOT_WCHAR L"\\_SB"
#define SAMPLE_BUSD_ACPI_NAME_ANSI "\\_SB.BUSD"
#define SAMPLE_BUSD_ACPI_NAME_WCHAR L"\\_SB.BUSD"
#define SAMPLE_GPIO_ACPI_NAME_ANSI "\\_SB.GPIO"
#define SAMPLE_GPIO_ACPI_NAME_WCHAR L"\\_SB.GPIO"
#define SAMPLE_SPBD_ACPI_NAME_ANSI "\\_SB.SPBD"
#define SAMPLE_SPBD_ACPI_NAME_WCHAR L"\\_SB.SPBD"
#define SAMPLE_TST1_ACPI_NAME_ANSI "\\_SB.BUSD.TST1"
#define SAMPLE_TST1_ACPI_NAME_WCHAR L"\\_SB.BUSD.TST1"
#define SAMPLE_TST2_ACPI_NAME_ANSI "\\_SB.BUSD.TST2"
#define SAMPLE_TST2_ACPI_NAME_WCHAR L"\\_SB.BUSD.TST2"
#define SAMPLE_TST3_ACPI_NAME_ANSI "\\_SB.TST3"
#define SAMPLE_TST3_ACPI_NAME_WCHAR L"\\_SB.TST3"
#define SAMPLE_PWRR_ACPI_NAME_ANSI "\\_SB.PWRR"
#define SAMPLE_PWRR_ACPI_NAME_WCHAR L"\\_SB.PWRR"

#define SAMPLE_BUSD_REAL_NAME 'DSUB'
#define SAMPLE_GPIO_REAL_NAME 'OIPG'
#define SAMPLE_SPBD_REAL_NAME 'DBPS'
#define SAMPLE_TST1_REAL_NAME '1TST'
#define SAMPLE_TST2_REAL_NAME '2TST'
#define SAMPLE_TST3_REAL_NAME '3TST'
#define SAMPLE_PWRR_REAL_NAME 'PWRR'

//
// Step 3: Edit the PepDeviceMatchArray to tell the common library which device
// should be accepted by the platform extension.
// (See acpispecific.c for details)
//

//
// Step 4: Define the list of native methods under each device.
// (See acpispecific.c for details)
//

//
// Step 5: Define the list of device-specific notification handlers
// for each device.
// (See acpispecific.c for details)
//

//
// Step 6: Edit the PepDeviceDefinitionArray to register any native methods
// and device-specific notification handlers.
// (See acpispecific.c for details)
//

//
// Step 7: Implement the device-specific notification handlers.
// (See testdevice.c for details)
//

//
// Define the _HID, _CID, _ADR's for the test devices.
//

//
// Define the _HID of the bus device.
//

#define BUSD_HID "BUSD0001"

//
// Define the _CID of the bus device.
//

#define BUSD_CID "root\\dynambus"

//
// Define the _HID for the GPIO controller.
//

#define GPIO_HID "GPIO0001"

//
// Define the _HID for the SPBD controller.
//

#define SPBD_HID "SPBD0001"

//
// Define the _CID for the GPIO and SPBD.
//

#define GPIO_SPBD_CID "{b85b7c50-6a01-11d2-b841-00c04fad5171}\\MsToaster"

//
// Define the _ADR for the test devices.
//

#define TST1_ADR 0x1
#define TST2_ADR 0x2

//
// Define the _HID and _CID for the test device 3.
//

#define TST3_HID "TSTDV3"
#define TST3_CID "{b85b7c50-6a01-11d2-b841-00c04fad5171}\\MsToaster"

//
// Define the UUIDs for the _DSM of test device 3.
//

// {9BEC5298-A0A0-4725-916F-06AAF0CF69F4}
static const GUID TST3_DSM_UUID1 =
{ 0x9bec5298, 0xa0a0, 0x4725, { 0x91, 0x6f, 0x6, 0xaa, 0xf0, 0xcf, 0x69, 0xf4 } };

// {74DB3E61-06FA-4268-9A32-1101338FD38E}
static const GUID TST3_DSM_UUID2 =
{ 0x74db3e61, 0x6fa, 0x4268, { 0x9a, 0x32, 0x11, 0x1, 0x33, 0x8f, 0xd3, 0x8e } };

//
// Define the _HID for the power rail.
//

#define PWRR_HID "PWRR0001"

//
// Debugging
//

extern volatile BOOLEAN _SAMPLE_PEP_DEBUG_ENABLED;

#define DEBUG_BREAK() \
    {if (_SAMPLE_PEP_DEBUG_ENABLED) { \
        DbgBreakPoint(); \
    }} \

//
//------------------------------------------------------------------ Prototypes
//

PEP_NOTIFICATION_HANDLER_RESULT
RootSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
BusdSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
BusdWorkerCallbackEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
GpioSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
SpbdSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
Tst1SyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
Tst1WorkerCallbackEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
Tst1SyncQueryControlResources (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
Tst2SyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
Tst2WorkerCallbackEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
Tst3SyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

PEP_NOTIFICATION_HANDLER_RESULT
PwrrSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );


/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        hid.h

    Abstract:

        HID related function declarations, types, and defines

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

//
// Global Data Declarations
//
extern const PWSTR gpwstrManufacturerID;
extern const PWSTR gpwstrProductID;
extern const PWSTR gpwstrSerialNumber;
extern const USHORT gOEMVendorID;
extern const USHORT gOEMProductID;
extern const USHORT gOEMVersionID;

//
// Function prototypes
//

NTSTATUS
TchGetDeviceAttributes(
    IN WDFREQUEST Request
    );

NTSTATUS 
TchGetFeatureReport(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
TchGetHidDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
TchGetReportDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS 
TchGetString(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
TchProcessIdleRequest(
    IN  WDFDEVICE Device,
    IN  WDFREQUEST Request,
    OUT BOOLEAN *Pending
    );

NTSTATUS 
TchSetFeatureReport(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );
    
NTSTATUS 
TchReadReport(
    IN  WDFDEVICE Device,
    IN  WDFREQUEST Request,
    OUT BOOLEAN *Pending
    );


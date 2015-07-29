/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        driver.h

    Abstract:

        Contains WDF driver-specific function declarations

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once


DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DEVICE_CONTEXT_CLEANUP OnContextCleanup;

EVT_WDF_DRIVER_DEVICE_ADD OnDeviceAdd;


/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        device.h

    Abstract:

        Declarations of WDF device specific entry points

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once


EVT_WDF_DEVICE_D0_ENTRY OnD0Entry;

EVT_WDF_DEVICE_D0_EXIT OnD0Exit;

EVT_WDF_INTERRUPT_ISR OnInterruptIsr;

EVT_WDF_DEVICE_PREPARE_HARDWARE OnPrepareHardware;

EVT_WDF_DEVICE_RELEASE_HARDWARE OnReleaseHardware;
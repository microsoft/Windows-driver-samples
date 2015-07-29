/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    wmi.h

Abstract:

    This module contains the Windows Driver Framework WMI
    handlers for the firefly filter driver.

Environment:

    Kernel mode

--*/

//
// Where they are described.
//
#define MOFRESOURCENAME L"FireflyWMI"

//
// Initialize the FireFly drivers WMI support
//
NTSTATUS
WmiInitialize(
    WDFDEVICE       Device,
    PDEVICE_CONTEXT DeviceContext
    );

EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceSetInstance;
EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceSetItem;


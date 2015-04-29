/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:

    Driver object related declarations for bthecho client device

Environment:

    Kernel mode only


--*/

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD BthEchoCliEvtDriverDeviceAdd;

EVT_WDF_OBJECT_CONTEXT_CLEANUP BthEchoCliEvtDriverCleanup;


/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Driver.h

Abstract:

    Driver object callbacks, functions, and types.

Environment:

    Kernel-mode.

--*/

#pragma once


EXTERN_C DRIVER_INITIALIZE INIT_CODE_SEG DriverEntry;

namespace UcmUcsiAcpiClient
{

#define TEST_BIT(value, bitNumber) ((value) & (1<<(bitNumber))) ? true : false

class Driver
{
public:

    static
    DRIVER_INITIALIZE
    INIT_CODE_SEG
    CreateAndInitialize;

    static const ULONG c_PoolTag = 'ISCU';

    Driver () = delete;

    ~Driver () = delete;

private:

    static
    EVT_WDF_DRIVER_DEVICE_ADD
    PAGED_CODE_SEG
    EvtDriverDeviceAddThunk;

    static
    EVT_WDF_DRIVER_UNLOAD
    PAGED_CODE_SEG
    EvtDriverUnloadThunk;
};

}

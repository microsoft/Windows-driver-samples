/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    DeviceControl.cpp

Abstract:

    Implements the functions to control the OSR USB FX2 device.

Environment:

    User mode

--*/

#include "stdafx.h"

//
// Keep track of where the OSRFX2 device's bar graph currently is.
//
INT             CurrentBar;
BAR_GRAPH_STATE BarGraphState;

/*++

Routine Description:

    Initialize global variables for OSRFX2 device.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
OsrFx2InitializeDevice(
    VOID
    )
{
    CurrentBar = 0;
}


/*++

Routine Description:

    Turns off all of the bar graph lights on the OSR USB FX2 device.

Arguments:

    Context - The callback context

Return Value:

    VOID

--*/
DWORD
OsrFx2ClearAllBars(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    ULONG BytesReturned;

    BarGraphState.BarsAsUChar = 0;

    if (!DeviceIoControl(Context->DeviceHandle,
                         IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                         &BarGraphState,          // Pointer to InBuffer
                         sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                         NULL,                    // Pointer to OutBuffer
                         0,                       // Length of OutBuffer
                         &BytesReturned,          // BytesReturned
                         0))                      // Pointer to Overlapped structure
    {
        Err = GetLastError();
        goto cleanup;
    }

cleanup:

    return Err;
}


/*++

Routine Description:

    Lights the next bar on the OSR USB FX2 device.

Arguments:

    Context - The callback context

Return Value:

    VOID

--*/
DWORD
OsrFx2LightNextBar(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    ULONG BytesReturned;

    //
    // Normalize to 0-7
    //
    CurrentBar += 1;

    if (CurrentBar > 7)
    {
        CurrentBar = 0;
    }

    BarGraphState.BarsAsUChar = 1 << (UCHAR)CurrentBar;

    if (!DeviceIoControl(Context->DeviceHandle,
                         IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                         &BarGraphState,          // Pointer to InBuffer
                         sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                         NULL,                    // Pointer to OutBuffer
                         0,                       // Length of OutBuffer
                         &BytesReturned,          // BytesReturned
                         0))                      // Pointer to Overlapped structure
    {
        Err = GetLastError();
        goto cleanup;
    }

cleanup:

    return Err;
}

/*++

Routine Description:

    Lights the next bar on the OSRFX2 device.

Arguments:

    Context - The device context

Return Value:

    A Win32 error code.

--*/
DWORD
OsrFx2ControlDevice(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    Err = OsrFx2ClearAllBars(Context);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    Err = OsrFx2LightNextBar(Context);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

cleanup:

    return Err;
}

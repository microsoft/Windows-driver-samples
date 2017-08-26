/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Utils.cpp

Abstract:

    Provides utility function to SampleApp.cpp.

Environment:

    User mode

--*/

#include "Utils.h"

//
// Service trace event provider
// {54a25c42-cd91-4210-98cc-c3447b56e447}
//
EXTERN_C __declspec(selectany) const GUID SERVICE_PROVIDER_GUID = { 0x54a25c42, 0xcd91, 0x4210,{ 0x98, 0xcc, 0xc3, 0x44, 0x7b, 0x56, 0xe4, 0x47 } };

REGHANDLE m_etwRegHandle;


/*++

Routine Description:

    Sets up logging.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
SetupEvents()
{
    NTSTATUS status = EventRegister(&SERVICE_PROVIDER_GUID,
                                    nullptr,
                                    nullptr,
                                    &m_etwRegHandle);

    if (status != ERROR_SUCCESS)
    {
        wprintf(L"Provider not registered.  EventRegister failed with error: 0x%08X\n", status);
    }
}


/*++

Routine Description:

    Destroys logging.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
DestroyEvents()
{
    if (m_etwRegHandle != NULL)
    {
        EventUnregister(m_etwRegHandle);
    }
}


/*++

Routine Description:

    Log a message.

Arguments:

    Message     - The string message to be logged

    Level       - The type of event to be logged.  This parameter can
                  be one of the following values:

                  TRACE_LEVEL_CRITICAL
                  TRACE_LEVEL_ERROR
                  TRACE_LEVEL_WARNING
                  TRACE_LEVEL_INFORMATION
                  TRACE_LEVEL_VERBOSE

Return Value:

    VOID

--*/
VOID
WriteToEventLog(
    PWSTR Message,
    BYTE  Level
    )
{
    if (m_etwRegHandle != NULL)
    {
        EventWriteString(m_etwRegHandle, Level, 0, Message);
    }
}


/*++

Routine Description:

    Log an error message.

Arguments:

    Function - The function that gives the error

    Error    - The error code

Return Value:

    VOID

--*/
VOID
WriteToErrorLog(
    PWSTR Function,
    DWORD Error
    )
{
    WCHAR Message[260];

    StringCchPrintf(Message, ARRAYSIZE(Message),
                    L"%ws failed with error: 0x%08x", Function, Error);

    WriteToEventLog(Message, TRACE_LEVEL_ERROR);
}
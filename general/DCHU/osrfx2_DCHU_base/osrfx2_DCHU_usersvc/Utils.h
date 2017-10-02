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

#pragma once

#include <Windows.h>
#include <strsafe.h>
#include <evntprov.h>
#include <evntrace.h>

/*++

Routine Description:

    Sets up logging.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID SetupEvents();


/*++

Routine Description:

    Destroys logging.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID DestroyEvents();


/*++

Routine Description:

    Log a message.

Arguments:

    Message - The string message to be logged

    Level   - The type of event to be logged.  This parameter can
              be one of the following values:

              TRACE_LEVEL_CRITICAL
              TRACE_LEVEL_ERROR
              TRACE_LEVEL_WARNING
              TRACE_LEVEL_INFORMATION
              TRACE_LEVEL_VERBOSE

Return Value:

    VOID

--*/
VOID WriteToEventLog(PWSTR Message, BYTE Level);


/*++

Routine Description:

    Log an error message.

Arguments:

    Function - The function that gives the error

    Error    - The error code

Return Value:

    VOID

--*/
VOID WriteToErrorLog(PWSTR Function, DWORD Error);
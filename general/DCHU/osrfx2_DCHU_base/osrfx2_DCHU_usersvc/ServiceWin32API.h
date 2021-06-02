/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

--*/

#pragma once

VOID
WINAPI
ServiceMain(
    DWORD Argc,
    PWSTR *Argv
    );

VOID
WINAPI
ServiceCtrlHandler(
    DWORD Ctrl
    );

BOOL
UpdateServiceStatus(
    __in_opt SERVICE_STATUS_HANDLE hSvcHandle,
    __in     DWORD                 dwCurrentState,
    __in     DWORD                 dwWin32ExitCode
    );

VOID
CALLBACK
ServiceStopCallback(
    _In_ PVOID   lpParameter,
    _In_ BOOLEAN TimerOrWaitFired
    );

VOID
ServiceStop(
    _In_ DWORD ExitCode
    );

DWORD
WINAPI
ServiceRunningWorkerThread(
    _In_ PVOID lpThreadParameter
    );

DWORD
WINAPI
ServiceStopWorkerThread(
    _In_ PVOID lpThreadParameter
    );

/*++

Copyright (c) Microsoft Corporation. All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    SystemTraceControl.cpp

Abstract:

    This sample demonstrates how to collect events from SystemTraceProvider 
    on Windows 8.

Environment:

    User mode only.

--*/

#define INITGUID
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <strsafe.h>
#include <evntrace.h>

#define MAXIMUM_SESSION_NAME 1024

//
// Guid definitions from "NT Kernel Logger Constants" section on MSDN.
//

DEFINE_GUID ( /* 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c */
    ProcessGuid,
    0x3d6fa8d0,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

DEFINE_GUID ( /* 2cb15d1d-5fc1-11d2-abe1-00a0c911f518 */
    ImageLoadGuid,
    0x2cb15d1d,
    0x5fc1,
    0x11d2,
    0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18
  );

PEVENT_TRACE_PROPERTIES
AllocateTraceProperties (
    _In_opt_ PWSTR LoggerName,
    _In_opt_ PWSTR LogFileName
    )
{
    PEVENT_TRACE_PROPERTIES TraceProperties = NULL;
    ULONG BufferSize;

    BufferSize = sizeof(EVENT_TRACE_PROPERTIES) + 
        (MAXIMUM_SESSION_NAME + MAX_PATH) * sizeof(WCHAR);

    TraceProperties = (PEVENT_TRACE_PROPERTIES)malloc(BufferSize);  
    if (TraceProperties == NULL) {
        wprintf(L"Unable to allocate %d bytes for properties structure.\n", BufferSize);
        goto Exit;
    }

    //
    // Set the session properties.
    //

    ZeroMemory(TraceProperties, BufferSize);
    TraceProperties->Wnode.BufferSize = BufferSize;
    TraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    TraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    TraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + 
        (MAXIMUM_SESSION_NAME * sizeof(WCHAR)); 

    if (LoggerName != NULL) {
        StringCchCopyW((LPWSTR)((PCHAR)TraceProperties + TraceProperties->LoggerNameOffset), 
                      MAXIMUM_SESSION_NAME,
                      LoggerName);
    }

    if (LogFileName != NULL) {
        StringCchCopyW((LPWSTR)((PCHAR)TraceProperties + TraceProperties->LogFileNameOffset), 
                      MAX_PATH, 
                      LogFileName);
    }

Exit:
    return TraceProperties;
}

VOID
FreeTraceProperties (
    _In_ PEVENT_TRACE_PROPERTIES TraceProperties
    )
{
    free(TraceProperties);
    return;
}

int
__cdecl
wmain()
{
    CLASSIC_EVENT_ID EventId[2];
    ULONG Status = ERROR_SUCCESS;
    TRACEHANDLE SessionHandle = 0;
    PEVENT_TRACE_PROPERTIES TraceProperties;
    ULONG SystemTraceFlags[8];
    PWSTR LoggerName = L"MyTrace";

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    //
    // Allocate EVENT_TRACE_PROPERTIES structure and perform some
    // basic initialization. 
    //
    // N.B. LoggerName will be populated during StartTrace call.
    //

    TraceProperties = AllocateTraceProperties(NULL, L"SystemTrace.etl");
    if (TraceProperties == NULL) {
        Status = ERROR_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Configure additinal trace settings.
    //

    TraceProperties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_SYSTEM_LOGGER_MODE;
    TraceProperties->Wnode.ClientContext = 1; // Use QueryPerformanceCounter for time stamps
    TraceProperties->MaximumFileSize = 100; // Limit file size to 100MB max
    TraceProperties->BufferSize = 512; // Use 512KB trace buffers
    TraceProperties->MinimumBuffers = 64;
    TraceProperties->MaximumBuffers = 128;
    
    //
    // Start trace session which can receive events from SystemTraceProvider.
    //
    
    Status = StartTraceW(&SessionHandle, LoggerName, TraceProperties);
    if (Status != ERROR_SUCCESS) {
        wprintf(L"StartTrace() failed with %lu\n", Status);
        goto Exit;
    }

    //
    // Configure stack walking. In this example stack traces will be collected on
    // ImageLoad and ProcessCreate events.
    //
    // N.B. Stack tracing is configured before enabling event collection.
    //    
    
    ZeroMemory(EventId, sizeof(EventId));
    EventId[0].EventGuid = ImageLoadGuid;
    EventId[0].Type =  EVENT_TRACE_TYPE_LOAD;
    EventId[1].EventGuid = ProcessGuid;
    EventId[1].Type = EVENT_TRACE_TYPE_START;
    
    Status = TraceSetInformation(SessionHandle,
                                 TraceStackTracingInfo,
                                 EventId,
                                 sizeof(EventId));

    if (Status != ERROR_SUCCESS) {
        wprintf(L"TraceSetInformation(StackTracing) failed with %lu\n", Status);
        goto Exit;
    }
    
    //
    // Enable system events for Process, Thread and Loader groups.
    //

    ZeroMemory(SystemTraceFlags, sizeof(SystemTraceFlags));
    SystemTraceFlags[0] = (EVENT_TRACE_FLAG_PROCESS | 
                           EVENT_TRACE_FLAG_THREAD |
                           EVENT_TRACE_FLAG_IMAGE_LOAD);
    
    Status = TraceSetInformation(SessionHandle, 
                                 TraceSystemTraceEnableFlagsInfo,
                                 SystemTraceFlags,
                                 sizeof(SystemTraceFlags));
    
    if (Status != ERROR_SUCCESS) {
        wprintf(L"TraceSetInformation(EnableFlags) failed with %lu\n", Status);
        goto Exit;
    }
    
    //
    // Collect trace for 30 seconds.
    //

    Sleep(30 * 1000);

Exit:

    //
    // Stop tracing.
    //

    if (SessionHandle != 0) {
        Status = ControlTrace(SessionHandle, NULL, TraceProperties, EVENT_TRACE_CONTROL_STOP);  
        if (Status != ERROR_SUCCESS) {
            wprintf(L"StopTrace() failed with %lu\n", Status);
        }
    }

    if (TraceProperties != NULL) {
        FreeTraceProperties(TraceProperties);
    }

    return Status;
}

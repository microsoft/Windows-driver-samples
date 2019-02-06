/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fixsession.cpp

Abstract:

    This file contains the Fix sessions and contains logic to provide position.

Environment:

    Windows User-Mode Driver Framework

--*/

#include "precomp.h"
#include "Trace.h"
#include "Defaults.h"
#include "FixSession.h"
#include "FixHandler.h"

#include "FixSession.tmh"


static void CALLBACK
s_FixThreadStub(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK Work
);

CFixSession::CFixSession() :
    _Id(INVALID_SESSION_ID),
    _State(STOPPED),
    _HasCachedFix(false),
    _StopFixEvent(nullptr),
    _ThreadpoolWork(nullptr)
{
    InitializeCriticalSection(&_Lock);
    _GnssPosition = g_DefaultGnssFixData;
}

CFixSession::~CFixSession()
{
    if (_StopFixEvent != nullptr && _ThreadpoolWork != nullptr)
    {
        SetEvent(_StopFixEvent);
        WaitForThreadpoolWorkCallbacks(_ThreadpoolWork, TRUE);
    }

    if (_StopFixEvent != nullptr)
    {
        CloseHandle(_StopFixEvent);
        _StopFixEvent = nullptr;
    }

    if (_ThreadpoolWork != nullptr)
    {
        CloseThreadpoolWork(_ThreadpoolWork);
        _ThreadpoolWork = nullptr;
    }

    DeleteCriticalSection(&_Lock);
}

NTSTATUS
CFixSession::Initialize(
    _In_ WDFQUEUE Queue
)
{
    EnterCriticalSection(&_Lock);

    NTSTATUS status = STATUS_SUCCESS;

    // set the state
    _State = STOPPED;

    // Create the TP work
    _ThreadpoolWork = CreateThreadpoolWork(
        (PTP_WORK_CALLBACK)s_FixThreadStub,
        (PVOID)this,
        nullptr
    );

    if (_ThreadpoolWork == nullptr)
    {
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Create the stop fix Event
    _StopFixEvent = CreateEvent(/* lpEventAttributes */ nullptr, /* bManualReset */ FALSE, /* bInitialState */ FALSE, /* lpName */ nullptr);
    if (_StopFixEvent == nullptr)
    {
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Create the manual dispatch queue
    WDF_IO_QUEUE_CONFIG queueConfiguration;
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfiguration, WdfIoQueueDispatchManual);

    // Initialize the attributes of the queue. Set the execution level to
    // passive to force all IRPs to be sent to the driver at passive IRQL.
    WDF_OBJECT_ATTRIBUTES ioQueueAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&ioQueueAttributes);
    ioQueueAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;
    ioQueueAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    status = WdfIoQueueCreate(WdfIoQueueGetDevice(Queue),
                              &queueConfiguration,
                              &ioQueueAttributes,
                              &_FixAcquisitionRequestQueue);

Exit:
    LeaveCriticalSection(&_Lock);
    return status;
}

NTSTATUS
CFixSession::StartAcquiringFix(
    _In_ PGNSS_FIXSESSION_PARAM FixSessionParameters
)
{
    NTSTATUS status = STATUS_SUCCESS;

    EnterCriticalSection(&_Lock);

    // Check the state of the thread.
    // If already running we should not get this request
    // as multiplexing is not supported for Singleshot
    if (_State == ACQUIRING)
    {
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Create PTP_WORK only when first start fix is called.
    if (_ThreadpoolWork == nullptr)
    {
        // Not initialized. Error out
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    _Id = FixSessionParameters->FixSessionID;
    _State = ACQUIRING;
    _FixParameter = *FixSessionParameters;

    ResetEvent(_StopFixEvent);

    // Post a work object to the thread
    SubmitThreadpoolWork(_ThreadpoolWork);

    // Initialize the GetFix Queue
    WdfIoQueueStart(_FixAcquisitionRequestQueue);

Exit:
    LeaveCriticalSection(&_Lock);

    return status;
}

NTSTATUS
CFixSession::StopAcquiringFix(
    _In_ PGNSS_STOPFIXSESSION_PARAM StopFixSessionParam
)
{
    NTSTATUS status = STATUS_SUCCESS;

    {
        EnterCriticalSection(&_Lock);

        // If Ids don't Match return Error
        if (StopFixSessionParam->FixSessionID != _Id)
        {
            status = STATUS_UNSUCCESSFUL;
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "Id mismatch in Stop Fix Session call");
            goto Exit;
        }

        // Check the state of the session
        if (_State != ACQUIRING)
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "Can't stop an inactive session.");
            goto Exit;
        }

        // Stop the current processing of the fix session
        if (_StopFixEvent != nullptr)
        {
            SetEvent(_StopFixEvent);
        }

        LeaveCriticalSection(&_Lock);
    }

    // Wait for any threadpool work to finish.
    if (_ThreadpoolWork != nullptr)
    {
        WaitForThreadpoolWorkCallbacks(_ThreadpoolWork, TRUE);
    }

    EnterCriticalSection(&_Lock);
    _State = STOPPED;

    // Drain the GetFix Queue
    WdfIoQueuePurgeSynchronously(_FixAcquisitionRequestQueue);

Exit:
    LeaveCriticalSection(&_Lock);

    return status;
}

NTSTATUS
CFixSession::ModifyAcquiringFix(
    _In_ PGNSS_FIXSESSION_PARAM ModifyFixSessionParam
)
{
    NTSTATUS status = STATUS_SUCCESS;

    EnterCriticalSection(&_Lock);

    memcpy_s(&_FixParameter, sizeof(GNSS_FIXSESSION_PARAM), ModifyFixSessionParam, sizeof(*ModifyFixSessionParam));

    LeaveCriticalSection(&_Lock);

    return status;
}

NTSTATUS
CFixSession::ReportGnssFix(
    _In_ PGNSS_FIXDATA GnssFix
)
{
    EnterCriticalSection(&_Lock);

    NTSTATUS status = STATUS_SUCCESS;
    WDFREQUEST Request;
    WDF_IO_QUEUE_STATE QueueStatus;
    ULONG uRequest;

    QueueStatus = WdfIoQueueGetState(_FixAcquisitionRequestQueue, &uRequest, nullptr);
    if (!WDF_IO_QUEUE_READY(QueueStatus) || (uRequest == 0))
    {
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (STATUS_SUCCESS == WdfIoQueueRetrieveNextRequest(_FixAcquisitionRequestQueue, &Request))
    {
        status = CompleteRequestWithGnssFix(Request, GnssFix);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Could not respond with fix data with %!STATUS!", status);
            goto Exit;
        }
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Couldn't retrieve the next request with %!STATUS!", status);
        goto Exit;
    }

Exit:
    LeaveCriticalSection(&_Lock);

    return status;
}

NTSTATUS
CFixSession::CompleteRequestWithGnssFix(
    _In_ WDFREQUEST Request,
    PGNSS_FIXDATA GnssFix
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID outputBuffer;
    size_t outputBufferLength;
    PGNSS_EVENT gnssEvent;
    size_t size = FIELD_OFFSET(GNSS_EVENT, FixData) + sizeof(GNSS_FIXDATA);

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            size,
                                            &outputBuffer,
                                            &outputBufferLength);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "WdfRequestRetrieveOutputBuffer() failed with %!STATUS!", status);
        goto Exit;
    }

    gnssEvent = ((GNSS_EVENT *)outputBuffer);
    memset(outputBuffer, 0, size);
    gnssEvent->Version = GNSS_DRIVER_VERSION_2;
    gnssEvent->EventType = GNSS_Event_FixAvailable;
    gnssEvent->EventDataSize = sizeof(GNSS_FIXDATA);
    gnssEvent->Size = (ULONG)size;
    memcpy_s(&gnssEvent->FixData, sizeof(GNSS_FIXDATA), GnssFix, sizeof(GNSS_FIXDATA));
    gnssEvent->FixData.FixSessionID = _Id;

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, outputBufferLength);
    status = STATUS_PENDING;
Exit:
    return status;
}

NTSTATUS
CFixSession::UpdateCurrentPosition(
    _In_ PGNSS_FIXDATA GnssFix
)
{
    NTSTATUS status = STATUS_SUCCESS;

    EnterCriticalSection(&_Lock);

    memcpy_s(&_GnssPosition, sizeof(GNSS_FIXDATA), GnssFix, sizeof(*GnssFix));

    LeaveCriticalSection(&_Lock);

    return status;
}

NTSTATUS
CFixSession::GetFix(
    _In_ WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sessionId = INVALID_SESSION_ID;
    PGNSS_EVENT outputEvent = nullptr;
    ULONG *pSessionId = nullptr;

    EnterCriticalSection(&_Lock);

    // Verify the size of where output goes eventually, when this IRP is serviced
    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(GNSS_EVENT), (void**)&outputEvent, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "Could not retrieve output buffer for request with %!STATUS!", status);
        goto Exit;
    }

    // Get the session ID this get fix is for
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(sessionId), (void**)&pSessionId, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "Could not retrieve input sessionID for request");
        goto Exit;
    }

    sessionId = *pSessionId;

    // Check if the Ids match
    if (_Id != sessionId)
    {
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }
    // If we have a cached fix around because we tried to respond while there
    // weren't any getfixes around, answer immediately with it
    if (_HasCachedFix)
    {
        status = CompleteRequestWithGnssFix(Request, &_GnssPosition);

        _HasCachedFix = !(NT_SUCCESS(status));
    }
    // Otherwise, we push the request into a queue
    else
    {
        status = WdfRequestForwardToIoQueue(Request, _FixAcquisitionRequestQueue);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "WdfRequestForwardToIoQueue failed with %!STATUS!", status);
            goto Exit;
        }

        status = STATUS_PENDING;
    }

Exit:
    LeaveCriticalSection(&_Lock);

    return status;
}

static void CALLBACK
s_FixThreadStub(
    _Inout_ PTP_CALLBACK_INSTANCE /*Instance*/,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK /*Work*/
)
{
    CFixSession *pFixSession = static_cast<CFixSession *>(Context);

    pFixSession->AcquireSingleFix();
}

void
CFixSession::AcquireSingleFix()
{
    NTSTATUS status = STATUS_SUCCESS;

    while (true)
    {
        // Wait stop fix event for 1 second and then retrieve position
        DWORD WaitCode = WaitForSingleObject(_StopFixEvent, DEFAULT_FIX_INTERVAL_SECONDS * 1000);
        if (WaitCode == WAIT_TIMEOUT)
        {
            status = RetrievePosition(&_GnssPosition);
            if (!NT_SUCCESS(status))
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "Fail to retrieve fix data from position source");
            }

            // Send the default fix to the session
            status = ReportGnssFix(&_GnssPosition);
            if (!NT_SUCCESS(status))
            {
                // We were not able to send the fix so cache it
                _HasCachedFix = true;
            }
            else
            {
                if (_GnssPosition.IsFinalFix != FALSE)
                {
                    break;
                }
            }
        }
        else if (WaitCode == WAIT_OBJECT_0)
        {
            // Stop session signalled
            break;
        }
    }

    EnterCriticalSection(&_Lock);

    // Reset the StopFix Event if it is set
    ResetEvent(_StopFixEvent);

    // Update the state of the session
    _State = STOPPED;

    LeaveCriticalSection(&_Lock);
}


NTSTATUS
CFixSession::RetrievePosition(
    _Inout_ PGNSS_FIXDATA GnssFixData
)
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // FIX ME:
    // You can retrieve a position from a predefined position or GPS HW.
    // This sample code calls RetrievePositionFromPredefinedValue to give fake location.
    // To give a real location from GPS HW, replace this function with new function that defines
    // (1) establishing and enumerating HW interface such as Serial or Bluetooth, and
    // (2) reading and parsing NMEA messages from the interface.
    // You can also define a custom IOCTL instead, updating _GnssPosition directly in CQueue and skip the call below as no-op.
    //
    status = RetrievePositionFromPredefinedValue(GnssFixData);

    return status;
}

NTSTATUS
CFixSession::RetrievePositionFromPredefinedValue(
    _Out_ PGNSS_FIXDATA GnssFixData
)
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // FIX ME:
    // Specify user-defined GNSS_FIXDATA output based off the DDI format. The following is just an example.
    //
    *GnssFixData =
    {
        sizeof(GNSS_FIXDATA),
        GNSS_DRIVER_DDK_VERSION,
        INVALID_SESSION_ID,
        { 0xCD700000, 0x1D48908 }, // an example date/time, Dec/01/2018
        TRUE,
        STATUS_SUCCESS,
        GNSS_FIXDETAIL_BASIC | GNSS_FIXDETAIL_ACCURACY,
        {
            sizeof(GNSS_FIXDATA_BASIC),
            GNSS_DRIVER_DDK_VERSION,
            // Location: Mount Rainier National Park
            46.852273,
            -121.757468,
            0.0,
            0.0,
            0.0
        },
        {
            sizeof(GNSS_FIXDATA_ACCURACY),
            GNSS_DRIVER_DDK_VERSION,
            10 // Meters accuracy
        }
    };
    return status;
}

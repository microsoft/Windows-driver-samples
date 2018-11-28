/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fixhandler.cpp

Abstract:

    This file contains the Fix handler and contains logic to provide position.

Environment:

    Windows User-Mode Driver Framework

--*/

#include "precomp.h"
#include "Trace.h"
#include "Defaults.h"
#include "NmeaProcessor.h"
#include "FixSession.h"
#include "FixHandler.h"

#include "FixHandler.tmh" 


NTSTATUS
CFixHandler::Initialize(
    _In_ WDFQUEUE Queue
)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = _SingleShotSession.Initialize(Queue);

    return status;
}

NTSTATUS
CFixHandler::StartFix(
    _In_ WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PGNSS_FIXSESSION_PARAM fixSessionParameters = nullptr;

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(GNSS_FIXSESSION_PARAM), (void**)&fixSessionParameters, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "WdfRequestRetrieveInputBuffer failed with %!STATUS!", status);
        goto Exit;
    }

    if (fixSessionParameters->SessionType != GNSS_FixSession_SingleShot)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Session type is not GNSS_FixSession_SingleShot");
        goto Exit;
    }

    _CurrentSinglsShotSessionId = fixSessionParameters->FixSessionID;

    status = _SingleShotSession.StartAcquiringFix(fixSessionParameters);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "StartFix failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS
CFixHandler::StopFix(
    _In_ WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PGNSS_STOPFIXSESSION_PARAM StopFixSessionParam = nullptr;

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(GNSS_STOPFIXSESSION_PARAM), (void**)&StopFixSessionParam, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "WdfRequestRetrieveInputBuffer failed with %!STATUS!", status);
        goto Exit;
    }

    // Send the session parameters to the right session based on the ID
    if (StopFixSessionParam->FixSessionID != _CurrentSinglsShotSessionId)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Session type is not GNSS_FixSession_SingleShot");
        goto Exit;
    }

    _CurrentSinglsShotSessionId = INVALID_SESSION_ID;

    status = _SingleShotSession.StopAcquiringFix(StopFixSessionParam);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "StopFix failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS
CFixHandler::ModifyFix(
    _In_ WDFREQUEST /*Request*/
)
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // TODO: Currently not doing anything yet
    //

    return status;
}

NTSTATUS
CFixHandler::GetFixRequest(
    _In_ WDFREQUEST Request
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sessionId = INVALID_SESSION_ID;
    ULONG *pSessionId = nullptr;

    // Get the session ID this get fix is for
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(sessionId), (void**)&pSessionId, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Could not retrieve input sessionID for request");
        goto Exit;
    }

    // Should do this check at the FixHandler Level.
    sessionId = *pSessionId;

    status = _SingleShotSession.GetFix(Request);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "SinglShotSession.GetFix failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS
CFixHandler::SetCurrentPosition(
    _In_ PGNSS_FIXDATA GnssFixData
)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Update the current sessions.
    status = _SingleShotSession.UpdateCurrentPosition(GnssFixData);

    return status;
}

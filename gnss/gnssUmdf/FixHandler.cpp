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
    _In_ WDFREQUEST Request,
    _In_ BOOL LocationServiceEnabled
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PGNSS_FIXSESSION_PARAM fixSessionParameters = nullptr;

    // For privacy reasons, the location stack notifies the driver when user toggles the location device controls.
    // When set to off, the driver must not entertain any new location sessions.
    if (!LocationServiceEnabled)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Location master switch is off, we do not start a fix session");
        goto Exit;
    }

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(GNSS_FIXSESSION_PARAM), (void**)&fixSessionParameters, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "WdfRequestRetrieveInputBuffer failed with %!STATUS!", status);
        goto Exit;
    }

    //
    // FIX ME:
    // This sample currently supports only single shot session fix.
    // If you support another session type, e.g., distance tracking, update the logic below accordingly.
    //
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
    PGNSS_STOPFIXSESSION_PARAM stopFixSessionParam = nullptr;

    status = WdfRequestRetrieveInputBuffer(Request, sizeof(GNSS_STOPFIXSESSION_PARAM), (void**)&stopFixSessionParam, nullptr);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "WdfRequestRetrieveInputBuffer failed with %!STATUS!", status);
        goto Exit;
    }

    // Send the session parameters to the right session based on the ID
    if (stopFixSessionParam->FixSessionID != _CurrentSinglsShotSessionId)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Session type is not GNSS_FixSession_SingleShot");
        goto Exit;
    }

    _CurrentSinglsShotSessionId = INVALID_SESSION_ID;

    status = _SingleShotSession.StopAcquiringFix(stopFixSessionParam);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "StopFix failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS
CFixHandler::StopExistingSinglsShotFix()
{
    NTSTATUS status = STATUS_SUCCESS;
    PGNSS_STOPFIXSESSION_PARAM stopFixSessionParam = nullptr;
    
    if (_CurrentSinglsShotSessionId == INVALID_SESSION_ID)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FIX, "There is no single shot session to stop");
        goto Exit;
    }

    _CurrentSinglsShotSessionId = INVALID_SESSION_ID;
    
    INIT_POD_GNSS_STRUCT(stopFixSessionParam);
    status = _SingleShotSession.StopAcquiringFix(stopFixSessionParam);
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

    //
    // FIX ME:
    // We currently support only single shot session fix.
    // If you support another session type, e.g., distance tracking, update the logic below accordingly.
    //
    if (fixSessionParameters->SessionType != GNSS_FixSession_SingleShot)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "Session type is not GNSS_FixSession_SingleShot");
        goto Exit;
    }
    
    //
    // FIX ME:
    // Since only single session is supported, use _SingleShotSession instance.
    // If multiple sessions, get the right session pointer searched by the session ID.
    //
    _SingleShotSession.ModifyAcquiringFix(fixSessionParameters);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_FIX, "ModifyFix failed with %!STATUS!", status);
        goto Exit;
    }

Exit:
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

    //
    // FIX ME:
    // Single session is only option now, but if multiple sessions, this sample should do this check at the FixHandler Level.
    //
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

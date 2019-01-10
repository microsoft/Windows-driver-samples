/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Fixsession.h

Abstract:

    This module contains the type definitions of the fix logic of the Umdf Gnss Driver.

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

//
// This class implements the Fix sessions and contains logic to provide position within GNSS driver.
//
class CFixSession
{
public:
    CFixSession();
    ~CFixSession();

    void AcquireSingleFix();
    NTSTATUS Initialize(_In_ WDFQUEUE Queue);
    NTSTATUS StartAcquiringFix(_In_ PGNSS_FIXSESSION_PARAM FixSessionParameters);
    NTSTATUS StopAcquiringFix(_In_ PGNSS_STOPFIXSESSION_PARAM StopFixSessionParam);
    NTSTATUS ModifyAcquiringFix(_In_ PGNSS_FIXSESSION_PARAM ModifyFixSessionParam);
    NTSTATUS GetFix(_In_ WDFREQUEST Request);
    NTSTATUS UpdateCurrentPosition(_In_ PGNSS_FIXDATA GnssFix);

    NTSTATUS RetrievePosition(_Inout_ PGNSS_FIXDATA GnssFixData);
    NTSTATUS RetrievePositionFromPredefinedValue(_Out_ PGNSS_FIXDATA GnssFixData);

private:
    enum STATE
    {
        ACQUIRING = 0,
        STOPPED
    };

    ULONG _Id = INVALID_SESSION_ID;
    STATE _State = STOPPED;
    GNSS_FIXSESSION_PARAM _FixParameter = {};
    WDFQUEUE _FixAcquisitionRequestQueue = nullptr;
    bool _HasCachedFix = false;
    GNSS_FIXDATA _GnssPosition = {};
    HANDLE _StopFixEvent = nullptr;
    PTP_WORK _ThreadpoolWork = nullptr;
    CRITICAL_SECTION _Lock;

    NTSTATUS CompleteRequestWithGnssFix(_In_ WDFREQUEST Request, PGNSS_FIXDATA GnssFix);
    NTSTATUS ReportGnssFix(_In_ PGNSS_FIXDATA GnssFix);
};

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Fixhandler.h

Abstract:

    This module contains the type definitions of the fix logic of the Umdf Gnss Driver.

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

class CFixSession;

class CFixHandler
{
public:
    NTSTATUS Initialize(_In_ WDFQUEUE Queue);

    NTSTATUS StartFix(_In_ WDFREQUEST Request);
    NTSTATUS StopFix(_In_ WDFREQUEST Request);
    NTSTATUS GetFixRequest(_In_ WDFREQUEST Request);
    NTSTATUS ModifyFix(_In_ WDFREQUEST Request);

    NTSTATUS SetCurrentPosition(_In_ PGNSS_FIXDATA GnssFixData);
    
private:
    // Session objects
    CFixSession _SingleShotSession;
    ULONG _CurrentSinglsShotSessionId = INVALID_SESSION_ID;
};

/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    hbastat.h

Abstract:

    

Notes:

Revision History:

--*/

#pragma once

BOOLEAN
AhciAdapterReset( 
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

VOID
AhciCOMRESET(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_PORT Px
    );

BOOLEAN
P_NotRunning(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_PORT Px
    );

VOID
AhciAdapterRunAllPorts(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    );

VOID
RunNextPort(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    );

VOID
P_Running_StartAttempt(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    );

BOOLEAN
P_Running(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

HW_TIMER_EX P_Running_Callback;

VOID
P_Running_WaitOnDET(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitWhileDET1(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitOnDET3(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitOnFRE(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_WaitOnBSYDRQ(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

VOID
P_Running_StartFailed(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    );

BOOLEAN
AhciPortReset (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN CompleteAllRequests
    );

VOID
AhciPortErrorRecovery(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );


/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Server.h

Abstract:

    Contains declarations for Bluetooth server functionality.

Environment:

    Kernel mode only


--*/

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvInitialize(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvRegisterPSM(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoSrvUnregisterPSM(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvRegisterL2CAPServer(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoSrvUnregisterL2CAPServer(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoSrvPublishSdpRecord(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx,
    _In_ PUCHAR SdpRecord,
    _In_ ULONG  SdpRecordLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
BthEchoSrvRemoveSdpRecord(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
BthEchoSrvSendConnectResponse(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT DevCtx,
    _In_ PINDICATION_PARAMETERS ConnectParams
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvDisconnectConnection(
    _In_ PBTHECHO_CONNECTION Connection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
BthEchoSrvDisconnectConnectionsOnRemove(
    _In_ PBTHECHOSAMPLE_SERVER_CONTEXT devCtx
    );

EVT_WDF_REQUEST_COMPLETION_ROUTINE BthEchoSrvRemoteConnectResponseCompletion;


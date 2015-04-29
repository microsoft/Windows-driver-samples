/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    client.h

Abstract:

    Contains declarations for Bluetooth client functionality.
    
Environment:

    Kernel mode only


--*/

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoCliBthQueryInterfaces(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoCliRetrieveServerBthAddress(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BthEchoCliRetrieveServerSdpRecord(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx,
    _Out_ PBTH_SDP_STREAM_RESPONSE * ServerSdpRecord
    );

NTSTATUS
BthEchoCliRetrievePsmFromSdpRecord(
    _In_ PBTHDDI_SDP_PARSE_INTERFACE sdpParseInterface,
    _In_ PBTH_SDP_STREAM_RESPONSE ServerSdpRecord,
    _Out_ USHORT * Psm
    );

_IRQL_requires_same_
NTSTATUS
BthEchoCliOpenRemoteConnection(
    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx,
    _In_ WDFFILEOBJECT FileObject,
    _In_ WDFREQUEST Request
    );


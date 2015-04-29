/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Sdp.h

Abstract:

    Contains declarations for Sdp record related functions
    
Environment:

    Kernel mode only


--*/

NTSTATUS
CreateSdpRecord(
    _In_ PBTHDDI_SDP_NODE_INTERFACE SdpNodeInterface,
    _In_ PBTHDDI_SDP_PARSE_INTERFACE SdpParseInterface,
    _In_ const GUID * ClassId,
    _In_ LPWSTR Name,
    _In_ USHORT Psm,
    _Out_ PUCHAR * Stream,
    _Out_ ULONG * Size
    );

VOID
FreeSdpRecord(
    PUCHAR SdpRecord
    );


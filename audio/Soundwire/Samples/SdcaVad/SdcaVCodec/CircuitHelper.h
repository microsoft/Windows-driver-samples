/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CircuitHelper.h

Abstract:

   This module contains helper functions for device.cpp and render.cpp files.

Environment:

    Kernel mode

--*/

PAGED_CODE_SEG
NTSTATUS CreateRenderCircuit(
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ UNICODE_STRING                     CircuitName,
    _In_ WDFDEVICE                          Device,
    _Out_ ACXCIRCUIT*                       Circuit
);

PAGED_CODE_SEG
NTSTATUS ConnectRenderCircuitElements(
    _In_ ULONG                                   ElementCount,
    _In_reads_(ElementCount) ACXELEMENT*         Elements,
    _In_ ACXCIRCUIT                              Circuit
);

PAGED_CODE_SEG
NTSTATUS ObjBagAddBlob(
    _In_ ACXOBJECTBAG                         ObjBag,
    _In_z_ const char*                        Blob
);

PAGED_CODE_SEG
NTSTATUS ObjBagAddEndpointId(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UINT                       Value
);

PAGED_CODE_SEG
NTSTATUS ObjBagAddDataPortNumber(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UINT                       Value
);

PAGED_CODE_SEG
NTSTATUS ObjBagAddTestUI4(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UINT                       Value
);

PAGED_CODE_SEG
NTSTATUS ObjBagAddCircuitId(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ GUID                       Guid
);

PAGED_CODE_SEG
NTSTATUS ObjBagAddUnicodeStrings(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UNICODE_STRING             FriendlyNameStr,
    _In_ UNICODE_STRING             NameStr
);

PAGED_CODE_SEG
NTSTATUS AddJack(
    _In_ WDF_OBJECT_ATTRIBUTES       Attributes,
    _In_ ACXPIN                      Pin,
    _In_ ULONG                       ChannelMapping,
    _In_ ULONG                       Color,
    _In_ ACX_JACK_CONNECTION_TYPE    ConnectionType,
    _In_ ACX_JACK_GEO_LOCATION       GeoLocation,
    _In_ ACX_JACK_GEN_LOCATION       GenLocation,
    _In_ ACX_JACK_PORT_CONNECTION    PortConnection
);

/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CircuitHelper.h

Abstract:

   This module contains helper functions for render.cpp and capture.cpp files.

Environment:

    Kernel mode

--*/

#include "AcpiReader.h"

using namespace ACPIREADER;

PAGED_CODE_SEG
NTSTATUS CreateCaptureCircuit(
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ UNICODE_STRING                     CircuitName,
    _In_ WDFDEVICE                          Device,
    _Out_ ACXCIRCUIT*                       Circuit
);

PAGED_CODE_SEG
NTSTATUS CreateRenderCircuit(
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ UNICODE_STRING                     CircuitName,
    _In_ WDFDEVICE                          Device,
    _Out_ ACXCIRCUIT*                       Circuit
);

PAGED_CODE_SEG
NTSTATUS AllocateFormat(
    _In_ KSDATAFORMAT_WAVEFORMATEXTENSIBLE      WaveFormat,
    _In_ ACXCIRCUIT                             Circuit,
    _In_ WDFDEVICE                              Device,
    _Out_ ACXDATAFORMAT*                        Format
);

PAGED_CODE_SEG
NTSTATUS CreatePin(
    _In_ ACX_PIN_TYPE                           PinType,
    _In_ ACXCIRCUIT                             Circuit,
    _In_ ACX_PIN_COMMUNICATION                  Communication,
    _In_ const GUID*                            Category,
    _In_ ACX_PIN_CALLBACKS*                     PinCallbacks,
    _In_ ULONG                                  PinStreamCount,
    _In_ bool                                   Mic,
    _Out_ ACXPIN*                               Pin
);

PAGED_CODE_SEG
NTSTATUS RetrieveProperties(
    _In_ PACX_FACTORY_CIRCUIT_ADD_CIRCUIT       CircuitConfig,
    _In_ PULONG                                 EndpointID
);

PAGED_CODE_SEG
NTSTATUS
DetermineSpecialStreamDetailsFromVendorProperties(
    _In_        ACXCIRCUIT      Circuit,
    _In_        AcpiReader *    Acpi,
    _In_        HANDLE          CircuitPropertiesHandle
);

PAGED_CODE_SEG
NTSTATUS CreateStreamBridge(
    _In_ ACX_STREAM_BRIDGE_CONFIG                   StreamCfg,
    _In_ ACXCIRCUIT                                 Circuit,
    _In_ ACXPIN                                     Pin,
    _In_ DSP_PIN_CONTEXT*                           PinCtx,
    _In_ ULONG                                      BridgeDataPortNumber,
    _In_ ULONG                                      BridgeEndpointId,
    _In_opt_ PSDCA_PATH_DESCRIPTORS2                PathDescriptors,
    _In_ BOOL                                       Render
);

PAGED_CODE_SEG
NTSTATUS ConnectCaptureCircuitElements(
    _In_ ULONG                                   ElementCount,
    _In_reads_(ElementCount) ACXELEMENT*         Elements,
    _In_ ACXCIRCUIT                              Circuit
);

PAGED_CODE_SEG
NTSTATUS ConnectRenderCircuitElements(
    _In_ ACXAUDIOENGINE                          AudioEngineElement,
    _In_ ACXCIRCUIT                              Circuit
);

PAGED_CODE_SEG
NTSTATUS CreateAudioEngine(
    _In_ ACXCIRCUIT                             Circuit,
    _In_reads_(DspPinType_Count) ACXPIN*        Pins,
    _Out_ ACXAUDIOENGINE*                       AudioEngineElement
);

#pragma code_seg()
VOID CircuitRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
);

PAGED_CODE_SEG
NTSTATUS FindDownstreamVolumeMute(
    _In_    ACXCIRCUIT          Circuit,
    _In_    ACXTARGETCIRCUIT    TargetCircuit
);

PAGED_CODE_SEG
NTSTATUS
ReplicateFormatsForAudioEngine(
    _In_ ACXAUDIOENGINE     AudioEngine,
    _In_ ACXTARGETCIRCUIT   TargetCircuit,
    _In_ ULONG              TargetPinId
);

PAGED_CODE_SEG
NTSTATUS
ReplicateFormatsForPin(
    _In_ ACXPIN             Pin,
    _In_ ACXTARGETCIRCUIT   TargetCircuit,
    _In_ ULONG              TargetPinId
);

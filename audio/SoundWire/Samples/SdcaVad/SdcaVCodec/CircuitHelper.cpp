/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CircuitHelper.cpp

Abstract:

   This module contains helper functions for device.cpp and render.cpp files.

Environment:

    Kernel mode

--*/

#include "private.h"
#include "CircuitHelper.h"

#ifndef __INTELLISENSE__
#include "CircuitHelper.tmh"
#endif

PAGED_CODE_SEG
NTSTATUS CreateRenderCircuit(
    _In_ PACXCIRCUIT_INIT                   CircuitInit,
    _In_ UNICODE_STRING                     CircuitName,
    _In_ WDFDEVICE                          Device,
    _Out_ ACXCIRCUIT*                       Circuit
)
{

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_RENDER_CIRCUIT_CONTEXT);

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignName(CircuitInit, &CircuitName));

    //
    // Add circuit type.
    //
    AcxCircuitInitSetCircuitType(CircuitInit, AcxCircuitTypeRender);

    //
    // Assign the circuit's pnp-power callbacks.
    //
    {
        ACX_CIRCUIT_PNPPOWER_CALLBACKS  powerCallbacks;
        ACX_CIRCUIT_PNPPOWER_CALLBACKS_INIT(&powerCallbacks);
        powerCallbacks.EvtAcxCircuitPowerUp = CodecR_EvtCircuitPowerUp;
        powerCallbacks.EvtAcxCircuitPowerDown = CodecR_EvtCircuitPowerDown;
        AcxCircuitInitSetAcxCircuitPnpPowerCallbacks(CircuitInit, &powerCallbacks);
    }

    //
    // Assign the circuit's composite callbacks.
    //
    {
        ACX_CIRCUIT_COMPOSITE_CALLBACKS compositeCallbacks;
        ACX_CIRCUIT_COMPOSITE_CALLBACKS_INIT(&compositeCallbacks);
        compositeCallbacks.EvtAcxCircuitCompositeCircuitInitialize = CodecR_EvtCircuitCompositeCircuitInitialize;
        compositeCallbacks.EvtAcxCircuitCompositeInitialize = CodecR_EvtCircuitCompositeInitialize;
        AcxCircuitInitSetAcxCircuitCompositeCallbacks(CircuitInit, &compositeCallbacks);
    }

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
        CircuitInit,
        CodecR_EvtCircuitRequestPreprocess,
        (ACXCONTEXT)AcxRequestTypeAny, // dbg only
        AcxRequestTypeAny,
        NULL,
        AcxItemIdNone));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxCreateStreamCallback(
        CircuitInit,
        CodecR_EvtCircuitCreateStream));

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CODEC_RENDER_CIRCUIT_CONTEXT);
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitCreate(Device, &attributes, &CircuitInit, Circuit));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ConnectRenderCircuitElements(
    _In_ ULONG                                   ElementCount,
    _In_reads_(ElementCount) ACXELEMENT*         Elements,
    _In_ ACXCIRCUIT                              Circuit
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Explicitly connect the circuit/elements. Note that driver doens't 
    // need to perform this step when circuit/elements are connected in the 
    // same order as they were added to the circuit. By default ACX connects
    // the elements starting from the sink circuit pin and ending with the 
    // source circuit pin for both render and capture devices.
    //
    // circuit.pin[default_sink]    -> 1st element.pin[default_in]
    // 1st element.pin[default_out] -> 2nd element.pin[default_in]
    // 2nd element.pin[default_out] -> circuit.pin[default_source]
    //
    const int numElements = 2;
    const int numConnections = numElements + 1;

    ACX_CONNECTION connections[numConnections];
    ACX_CONNECTION_INIT(&connections[0], Circuit, Elements[ElementCount - 2]);
    ACX_CONNECTION_INIT(&connections[1], Elements[ElementCount - 2], Elements[ElementCount - 1]);
    ACX_CONNECTION_INIT(&connections[2], Elements[ElementCount - 1], Circuit);

    //
    // Add the connections linking circuit to elements.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddConnections(Circuit, connections, SIZEOF_ARRAY(connections)));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ObjBagAddBlob(
    _In_ ACXOBJECTBAG                         ObjBag,
    _In_z_ const char*                        Blob
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(VendorPropertiesBlock);
    STRING vendorBlob;
    RtlInitString(&vendorBlob, Blob);
    WDFMEMORY vendorBlobMem;
    RETURN_NTSTATUS_IF_FAILED(WdfMemoryCreatePreallocated(NULL, vendorBlob.Buffer, vendorBlob.MaximumLength, &vendorBlobMem));
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddBlob(ObjBag, &VendorPropertiesBlock, vendorBlobMem));
    WdfObjectDelete(vendorBlobMem);

    return status;
}

PAGED_CODE_SEG
NTSTATUS ObjBagAddEndpointId(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UINT                       Value
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(EndpointId);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(ObjBag, &EndpointId, Value));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ObjBagAddDataPortNumber(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UINT                       Value
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_SOUNDWIRE_PROPERTY_NAME(DataPortNumber);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(ObjBag, &DataPortNumber, Value));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ObjBagAddTestUI4(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UINT                       Value
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, TestUI4);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUI4(ObjBag, &TestUI4, Value));

    return status;
}


PAGED_CODE_SEG
NTSTATUS ObjBagAddCircuitId(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ GUID                       Guid
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_DRIVER_PROPERTY_NAME(msft, CircuitId);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddGuid(ObjBag, &CircuitId, Guid));

    return status;
}

PAGED_CODE_SEG
NTSTATUS ObjBagAddUnicodeStrings(
    _In_ ACXOBJECTBAG               ObjBag,
    _In_ UNICODE_STRING             FriendlyNameStr,
    _In_ UNICODE_STRING             NameStr
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(FriendlyName);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUnicodeString(ObjBag, &FriendlyName, &FriendlyNameStr));

    DECLARE_CONST_ACXOBJECTBAG_SYSTEM_PROPERTY_NAME(Name);
    RETURN_NTSTATUS_IF_FAILED(AcxObjectBagAddUnicodeString(ObjBag, &Name, &NameStr));

    return status;
}

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
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    ACX_JACK_CONFIG jackCfg;
    ACX_JACK_CONFIG_INIT(&jackCfg);
    jackCfg.Description.ChannelMapping = ChannelMapping;
    jackCfg.Description.Color = Color;
    jackCfg.Description.ConnectionType = ConnectionType;
    jackCfg.Description.GeoLocation = GeoLocation;
    jackCfg.Description.GenLocation = GenLocation;
    jackCfg.Description.PortConnection = PortConnection;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, CODEC_JACK_CONTEXT);
    Attributes.ParentObject = Pin;

    ACXJACK jack;
    RETURN_NTSTATUS_IF_FAILED(AcxJackCreate(Pin, &Attributes, &jackCfg, &jack));

    ASSERT(jack != NULL);

    PCODEC_JACK_CONTEXT jackCtx;
    jackCtx = GetCodecJackContext(jack);
    ASSERT(jackCtx);
    jackCtx->Dummy = 0;

    RETURN_NTSTATUS_IF_FAILED(AcxPinAddJacks(Pin, &jack, 1));

    return status;
}



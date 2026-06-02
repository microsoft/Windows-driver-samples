/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    circuitstream.cpp

Abstract:

    Circuit Stream callbacks

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>
#include "streamengine.h"

#pragma code_seg()
VOID
Dsp_EvtStreamContextDestroy(
    _In_ WDFOBJECT Object
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    ctx = GetDspStreamContext((ACXSTREAM)Object);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;
    ctx->StreamEngine = NULL;
    delete streamEngine;
}

PAGED_CODE_SEG
VOID
Dsp_EvtStreamContextCleanup(
    _In_ WDFOBJECT Object
)
{
    PDSP_STREAM_CONTEXT streamCtx = GetDspStreamContext((ACXSTREAM)Object);

    PAGED_CODE();

    if (streamCtx->Pin != NULL)
    {
#ifdef ACX_WORKAROUND_ACXPIN_01

        PDSP_PIN_CONTEXT pinCtx = GetDspPinContext(streamCtx->Pin);

        if (streamCtx->StreamIsCounted)
        {
            ASSERT(pinCtx->CurrentStreamsCount > 0);
            InterlockedDecrement(PLONG(&pinCtx->CurrentStreamsCount));
            streamCtx->StreamIsCounted = FALSE;
        }
#endif  // ACX_WORKAROUND_ACXPIN_01

        WdfObjectDereferenceWithTag(streamCtx->Pin, (PVOID)DRIVER_TAG);
        streamCtx->Pin = NULL;
    }

    if (streamCtx->SpecialStreamTargetCircuit)
    {
        WdfObjectDereferenceWithTag(streamCtx->SpecialStreamTargetCircuit, (PVOID)DRIVER_TAG);
        streamCtx->SpecialStreamTargetCircuit = nullptr;
    }
}

#ifdef ACX_WORKAROUND_ACXPIN_01
PAGED_CODE_SEG
VOID
Dsp_EvtStreamGetStreamCountRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
)
/*++

Routine Description:

    This function is a preprocess routine.

--*/
{
    NTSTATUS                    status      = STATUS_NOT_SUPPORTED;
    ACXCIRCUIT                  circuit     = (ACXCIRCUIT)Object;
    ULONG_PTR                   outDataCb   = 0;
    ACX_REQUEST_PARAMETERS      params;

    UNREFERENCED_PARAMETER(DriverContext);

    PAGED_CODE();

    ACX_REQUEST_PARAMETERS_INIT(&params);
    AcxRequestGetParameters(Request, &params);

    //
    // Make sure this is a pin property request.
    //
    if ((params.Type != AcxRequestTypeProperty) || 
        (params.Parameters.Property.ItemType != AcxItemTypePin))
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    //
    // Handle only the 'get' verb.
    //
    if (params.Parameters.Property.Verb == AcxPropertyVerbGet)
    {
        ACXPIN              pin         = NULL;
        KSPIN_CINSTANCES *  value       = NULL;
        ULONG               valueCb     = 0;
        ULONG               minSize = sizeof(KSPIN_CINSTANCES);

        value = (KSPIN_CINSTANCES*)params.Parameters.Property.Value;
        valueCb = params.Parameters.Property.ValueCb;

        //
        // Get the associated pin object.
        //
        pin = AcxCircuitGetPinById(circuit, params.Parameters.Property.ItemId);
        if (pin == NULL)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            goto exit;
        }

        if (valueCb == 0)
        {
            outDataCb = minSize;
            status = STATUS_BUFFER_OVERFLOW;
            goto exit;
        }
        else if (valueCb < minSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto exit;
        }
        else 
        {
            PDSP_PIN_CONTEXT pinCtx = GetDspPinContext(pin);
            value->PossibleCount = pinCtx->MaxStreams;
            value->CurrentCount = pinCtx->CurrentStreamsCount; // Aligned dword reads are atomic.
            outDataCb = minSize;
        }
    }
    else
    {
        //
        // Just give it back to ACX. After this call the request is gone.
        //
        (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
        Request = NULL;
        goto exit;
    }

    status = STATUS_SUCCESS;

exit:
    if (Request != NULL)
    {
        WdfRequestCompleteWithInformation(Request, status, outDataCb);
    }
}
#endif  // ACX_WORKAROUND_ACXPIN_01

// See description in private.h
#ifdef ACX_WORKAROUND_ACXPIN_02
PAGED_CODE_SEG
VOID
Dsp_EvtStreamProposeDataFormatRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
)
/*++

Routine Description:

    This function is a preprocess routine.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverContext);

    {
        ACXCIRCUIT              circuit     = (ACXCIRCUIT)Object;
        ACXPIN                  pin         = NULL;
        PDSP_PIN_CONTEXT        pinCtx      = NULL;

        ACX_REQUEST_PARAMETERS  params;

        ACX_REQUEST_PARAMETERS_INIT(&params);
        AcxRequestGetParameters(Request, &params);

        if ((params.Type != AcxRequestTypeProperty) ||
            (params.Parameters.Property.ItemType != AcxItemTypePin) ||
            (params.Parameters.Property.Verb != AcxPropertyVerbSet))
        {
            goto forward_request;
        }

        //
        // Get the associated pin object.
        //
        pin = AcxCircuitGetPinById(circuit, params.Parameters.Property.ItemId);
        if (pin == NULL)
        {
            goto forward_request;
        }

        //
        // Check if this is the offload pin.
        //
        pinCtx = GetDspPinContext(pin);
        if (!pinCtx || (pinCtx->PinType != DspPinTypeOffload))
        {
            goto forward_request;
        }

        //
        // This is an offload pin, check # of streams.
        //
        if (pinCtx->CurrentStreamsCount >= pinCtx->MaxStreams)
        {
            // Cannot create any more streams, error out.
            WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
    }

    //
    // Just give it back to ACX. After this call the request is gone.
    //
forward_request:
    (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
}
#endif  // ACX_WORKAROUND_ACXPIN_02

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamGetHwLatency(
    _In_ ACXSTREAM Stream,
    _Out_ ULONG * FifoSize,
    _Out_ ULONG * Delay
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->GetHWLatency(FifoSize, Delay);
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamAllocateRtPackets(
    _In_ ACXSTREAM Stream,
    _In_ ULONG PacketCount,
    _In_ ULONG PacketSize,
    _Out_ PACX_RTPACKET *Packets
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->AllocateRtPackets(PacketCount, PacketSize, Packets);
}

PAGED_CODE_SEG
VOID
Dsp_EvtStreamFreeRtPackets(
    _In_ ACXSTREAM Stream,
    _In_ PACX_RTPACKET Packets,
    _In_ ULONG PacketCount
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->FreeRtPackets(Packets, PacketCount);
}

PAGED_CODE_SEG
NTSTATUS
Dsp_PrepareSpecialStreamForStream(
    _In_ ACXSTREAM Stream,
    _In_ SDCA_SPECIALSTREAM_TYPE SpecialStreamType,
    _In_ ULONG FunctionBitMask
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    SDCA_PATH               specialStreamPath = SdcaPathFromSpecialStreamType(SpecialStreamType);
    PDSP_STREAM_CONTEXT     ctx = GetDspStreamContext(Stream);
    ACXCIRCUIT              circuit = AcxPinGetCircuit(ctx->Pin);
    PDSP_CIRCUIT_CONTEXT    circuitCtx = GetDspCircuitContext(circuit);
    PSDCA_PATH_DESCRIPTORS  pathDescriptors = nullptr;
    BOOLEAN                 activeStreamCountIncremented = FALSE;

    PAGED_CODE();

    if (circuitCtx->SpecialStreamAvailablePaths & specialStreamPath &&
        !ctx->SpecialStreamInUse[SpecialStreamType])
    {
        ULONG streamCount = InterlockedIncrement(PLONG(&(circuitCtx->SpecialStreamActive[SpecialStreamType])));
        activeStreamCountIncremented = TRUE;

        if (1 == streamCount)
        {
            // TODO: The above ensures that the global special stream usage counts are protected, however if a special stream
            // were destroyed at near the same time as another one created, then there could be a timing issue between the
            // timing of this call to create the path and the timing of the ReleaseHardware call destroying the path.
            // i.e. ReleaseHardware performs an interlocked decrement to 0 and then a context switch. PrepareHardware runs and does an interlocked increment
            // back to 1, and performs the CreatePath call as it appears to be the first and the path is not created.
            // Then, ReleaseHardware resumes and calls DestroyPath.
            // So, can a PrepareHardware and a ReleaseHardware for two different streams on the same pin, happen at the same time?

            // Sample driver uses audio composition data to determine if it is going to use pathdescriptor2 or pathdescriptor
            // to prepare special stream. Audio composition will also provide the entire pathdescriptor2 to be used.
            if (circuitCtx->SpecialStreamPathDescriptors2[SpecialStreamType])
            {
                status = DSP_SendPropertyTo(
                    AcxCircuitGetWdfDevice(circuit),
                    ctx->SpecialStreamTargetCircuit,
                    KSPROPERTYSETID_Sdca,
                    KSPROPERTY_SDCA_CREATE_PATH2,
                    AcxPropertyVerbSet,
                    nullptr, 0,
                    circuitCtx->SpecialStreamPathDescriptors2[SpecialStreamType],
                    circuitCtx->SpecialStreamPathDescriptors2[SpecialStreamType]->Size,
                    nullptr);
                if (!NT_SUCCESS(status))
                {
                    goto exit;
                }
            }
            else
            {
                // for simplicity, we take a copy of the entire path descriptors returned from downstream, and then adjust that copy
                // to have the requested data port & format, leaving the remaining, if there are any, unused.
                pathDescriptors = (PSDCA_PATH_DESCRIPTORS)ExAllocatePool2(POOL_FLAG_NON_PAGED, circuitCtx->SpecialStreamPathDescriptors[SpecialStreamType]->Size, DRIVER_TAG);
                if (pathDescriptors == nullptr)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
                RtlCopyMemory(pathDescriptors, circuitCtx->SpecialStreamPathDescriptors[SpecialStreamType], circuitCtx->SpecialStreamPathDescriptors[SpecialStreamType]->Size);

                // walk the descriptors and adjust each entry to have 1 format, preferred one if available,
                // and a single data port. The remaining entries beyond the first are there and accounted for
                // by the size, but are unused.
                PSDCA_PATH_DESCRIPTOR currentDescriptor = (PSDCA_PATH_DESCRIPTOR)(pathDescriptors + 1);
                // In some cases we will only use some of the functions; targetDescriptor will receive the next descriptor if we skip any
                PSDCA_PATH_DESCRIPTOR targetDescriptor = currentDescriptor;
                ULONG descriptorCount = 0;
                for (ULONG j = 0; j < pathDescriptors->DescriptorCount; j++)
                {
                    PSDCA_PATH_DESCRIPTOR nextDescriptor = (PSDCA_PATH_DESCRIPTOR)(((BYTE*)currentDescriptor) + currentDescriptor->Size);

                    if (((1 << currentDescriptor->FunctionInformationId) & FunctionBitMask) == 0)
                    {
                        // This audio function isn't included in what should be started
                        currentDescriptor = nextDescriptor;
                        continue;
                    }

                    if (currentDescriptor != targetDescriptor)
                    {
                        RtlCopyMemory(targetDescriptor, currentDescriptor, currentDescriptor->Size);
                    }

                    PSDCA_PATH_DESCRIPTOR nextTargetDescriptor = (PSDCA_PATH_DESCRIPTOR)(((BYTE*)targetDescriptor) + targetDescriptor->Size);

                    for (ULONG i = 0; i < targetDescriptor->FormatCount; i++)
                    {
                        if (48000 == targetDescriptor->Formats[i].Format.nSamplesPerSec)
                        {
                            RtlCopyMemory(&(targetDescriptor->Formats[0]), &(targetDescriptor->Formats[i]), sizeof(targetDescriptor->Formats[0]));
                            break;
                        }
                    }

                    targetDescriptor->FormatCount = min(targetDescriptor->FormatCount, 1);
                    targetDescriptor->DataPortCount = min(targetDescriptor->DataPortCount, 1);

                    targetDescriptor = nextTargetDescriptor;
                    currentDescriptor = nextDescriptor;

                    ++descriptorCount;
                }

                if (descriptorCount == 0)
                {
                    // No target functions were chosen.
                    status = STATUS_INVALID_PARAMETER;
                    goto exit;
                }

                pathDescriptors->DescriptorCount = descriptorCount;

                // A real DSP driver would choose a suitable EndpointID. This is a placeholder.
                pathDescriptors->EndpointId = 0xaa;

                status = DSP_SendPropertyTo(
                    AcxCircuitGetWdfDevice(circuit),
                    ctx->SpecialStreamTargetCircuit,
                    KSPROPERTYSETID_Sdca,
                    KSPROPERTY_SDCA_CREATE_PATH,
                    AcxPropertyVerbSet,
                    nullptr, 0,
                    pathDescriptors, pathDescriptors->Size,
                    nullptr);
                if (!NT_SUCCESS(status))
                {
                    goto exit;
                }
            }
        }

        // If we succeeded in creating the path, set the tracking variable for the stream type
        ctx->SpecialStreamInUse[SpecialStreamType] = TRUE;
    }

exit:
    if (!NT_SUCCESS(status) && activeStreamCountIncremented)
    {
        // if we failed to create it, this call is going to fail, undo the circuit context tracking
        InterlockedDecrement(PLONG(&(circuitCtx->SpecialStreamActive[SpecialStreamType])));
    }

    if (pathDescriptors)
    {
        ExFreePool(pathDescriptors);
        pathDescriptors = nullptr;
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_ReleaseSpecialStreamsForStream(
    _In_ ACXSTREAM Stream
)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT     ctx = GetDspStreamContext(Stream);
    ACXCIRCUIT              circuit = AcxPinGetCircuit(ctx->Pin);
    PDSP_CIRCUIT_CONTEXT    circuitCtx = GetDspCircuitContext(circuit);

    PAGED_CODE();

    for (ULONG streamType = 0; streamType < ARRAYSIZE(ctx->SpecialStreamInUse); ++streamType)
    {
        if (!ctx->SpecialStreamInUse[streamType])
        {
            continue;
        }

        // As the special stream hardware is potentially shared across multiple streams,
        // special stream state is tracked in the circuit context.
        // Decrement shared circuit context tracking to indicate that this stream is no longer using this special stream path
        ULONG streamCount = InterlockedDecrement(PLONG(&(circuitCtx->SpecialStreamActive[streamType])));

        // if this was the last user of it, destroy the special stream path
        if (0 == streamCount)
        {
            SDCA_PATH   path = SdcaPathFromSpecialStreamType((SDCA_SPECIALSTREAM_TYPE)streamType);
            NTSTATUS    sendStatus;

            sendStatus = DSP_SendPropertyTo(
                AcxCircuitGetWdfDevice(circuit),
                ctx->SpecialStreamTargetCircuit,
                KSPROPERTYSETID_Sdca,
                KSPROPERTY_SDCA_DESTROY_PATH,
                AcxPropertyVerbSet,
                nullptr, 0,
                &path, sizeof(path),
                nullptr);

            status = !NT_SUCCESS(status) ? status : sendStatus;
        }

        // if the path has been destroyed, it also cannot be running,
        // so update the special stream state for both.
        ctx->SpecialStreamInUse[streamType] = FALSE;
        ctx->SpecialStreamRunning[streamType] = FALSE;
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamPrepareHardware(
    _In_ ACXSTREAM Stream
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine *     streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    // prepare the stream engine hardware
    status = streamEngine->PrepareHardware();

    // For a host or offload pin, start the Sense stream
    // if it isn't already running
    if (NT_SUCCESS(status) &&
        (DspPinTypeHost == ctx->PinType || DspPinTypeOffload == ctx->PinType))
    {
        status = Dsp_PrepareSpecialStreamForStream(Stream, SpecialStreamTypeIvSense);
    }

    // If this is loopback, we may be able to use reference
    // stream hardware, check
    if (NT_SUCCESS(status) &&
        DspPinTypeLoopback == ctx->PinType)
    {
        // for sample purposes, we're using the same stream engine for loopback with
        // reference stream as without. The only difference is whether the special stream
        // properties are being used to create, destroy, start, and stop the reference stream
        // hardware when the loopback stream is used.

        status = Dsp_PrepareSpecialStreamForStream(Stream, SpecialStreamTypeReferenceStream);
    }

    if (!NT_SUCCESS(status))
    {
        (void)Dsp_ReleaseSpecialStreamsForStream(Stream);
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamReleaseHardware(
    _In_ ACXSTREAM Stream
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine *     streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    status = Dsp_ReleaseSpecialStreamsForStream(Stream);

    NTSTATUS engineStatus = streamEngine->ReleaseHardware();

    return NT_SUCCESS(engineStatus)?status:engineStatus;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_StopSpecialStreamsForStream(
    _In_ ACXSTREAM  Stream
)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT     ctx = GetDspStreamContext(Stream);
    ACXCIRCUIT              circuit = AcxPinGetCircuit(ctx->Pin);
    PDSP_CIRCUIT_CONTEXT    circuitCtx = GetDspCircuitContext(circuit);

    PAGED_CODE();

    for (ULONG streamType = 0; streamType < ARRAYSIZE(ctx->SpecialStreamInUse); ++streamType)
    {
        if (ctx->SpecialStreamRunning[streamType])
        {
            // As the special stream hardware is potentially shared across multiple streams,
            // special stream state is tracked in the circuit context.
            // Decrement shared circuit context tracking to indicate that this stream is no longer running
            ULONG streamCount = InterlockedDecrement(PLONG(&(circuitCtx->SpecialStreamRunning[streamType])));

            // if this was the last stream using it, stop the path
            if (0 == streamCount)
            {
                SDCA_PATH   path = SdcaPathFromSpecialStreamType((SDCA_SPECIALSTREAM_TYPE)streamType);
                NTSTATUS    sendStatus;

                sendStatus = DSP_SendPropertyTo(
                    AcxCircuitGetWdfDevice(circuit),
                    ctx->SpecialStreamTargetCircuit,
                    KSPROPERTYSETID_Sdca,
                    KSPROPERTY_SDCA_STOP_PATH,
                    AcxPropertyVerbSet,
                    nullptr, 0,
                    &path, sizeof(path),
                    nullptr);

                status = !NT_SUCCESS(status) ? status : sendStatus;
            }

            // update special stream state
            ctx->SpecialStreamRunning[streamType] = FALSE;
        }
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_StartSpecialStreamsForStream(
    _In_ ACXSTREAM  Stream
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT     ctx = GetDspStreamContext(Stream);
    ACXCIRCUIT              circuit = AcxPinGetCircuit(ctx->Pin);
    PDSP_CIRCUIT_CONTEXT    circuitCtx = GetDspCircuitContext(circuit);

    PAGED_CODE();

    for (ULONG streamType = 0; streamType < ARRAYSIZE(ctx->SpecialStreamInUse); ++streamType)
    {
        if (ctx->SpecialStreamInUse[streamType] &&
            !ctx->SpecialStreamRunning[streamType])
        {
            // As the special stream hardware is potentially shared across multiple streams,
            // special stream state is tracked in the circuit context.
            // Increment shared circuit context tracking to indicate that this stream is running
            ULONG streamCount = InterlockedIncrement(PLONG(&(circuitCtx->SpecialStreamRunning[streamType])));

            // if we are the first to use it, start the path
            if (1 == streamCount)
            {
                SDCA_PATH path = SdcaPathFromSpecialStreamType((SDCA_SPECIALSTREAM_TYPE)streamType);
                status = DSP_SendPropertyTo(
                    AcxCircuitGetWdfDevice(circuit),
                    ctx->SpecialStreamTargetCircuit,
                    KSPROPERTYSETID_Sdca,
                    KSPROPERTY_SDCA_START_PATH,
                    AcxPropertyVerbSet,
                    nullptr, 0,
                    &path, sizeof(path),
                    nullptr);
            }

            // if we succeeded in starting the path, update set our tracking
            if (NT_SUCCESS(status))
            {
                ctx->SpecialStreamRunning[streamType] = TRUE;
            }
            else
            {
                // if we failed to set the state, so clear the state tracking
                InterlockedDecrement(PLONG(&(circuitCtx->SpecialStreamRunning[streamType])));

                // If we failed, exit early so we can clean up
                break;
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        // If this stream has more than one special stream, it's possible we failed after starting
        // one or more special streams. As such, make sure all streams are stopped.
        (void)Dsp_StopSpecialStreamsForStream(Stream);
    }
    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamRun(
    _In_ ACXSTREAM Stream
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine *     streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    status = streamEngine->Run();

    // if we're using reference stream and aren't already running,
    // set our state to running.
    if (NT_SUCCESS(status))
    {
        status = Dsp_StartSpecialStreamsForStream(Stream);
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamPause(
    _In_ ACXSTREAM Stream
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    // if any special streams are running (which can only happen if they're being used)
    // and we're pausing, update tracking
    status = Dsp_StopSpecialStreamsForStream(Stream);

    NTSTATUS engineStatus = streamEngine->Pause();

    return NT_SUCCESS(engineStatus)?status:engineStatus;
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamAssignDrmContentId(
    _In_ ACXSTREAM      Stream,
    _In_ ULONG          DrmContentId,
    _In_ PACXDRMRIGHTS  DrmRights
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = (CStreamEngine*)ctx->StreamEngine;

    return streamEngine->AssignDrmContentId(DrmContentId, DrmRights);
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamGetCurrentPacket(
    _In_ ACXSTREAM          Stream,
    _Out_ PULONG            CurrentPacket
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = static_cast<CStreamEngine*>(ctx->StreamEngine);

    return streamEngine->GetCurrentPacket(CurrentPacket);
}

PAGED_CODE_SEG
NTSTATUS
Dsp_EvtStreamGetPresentationPosition(
    _In_ ACXSTREAM          Stream,
    _Out_ PULONGLONG        PositionInBlocks,
    _Out_ PULONGLONG        QPCPosition
)
{
    PDSP_STREAM_CONTEXT ctx;
    CStreamEngine * streamEngine = NULL;

    PAGED_CODE();

    ctx = GetDspStreamContext(Stream);

    streamEngine = static_cast<CStreamEngine*>(ctx->StreamEngine);

    return streamEngine->GetPresentationPosition(PositionInBlocks, QPCPosition);
}


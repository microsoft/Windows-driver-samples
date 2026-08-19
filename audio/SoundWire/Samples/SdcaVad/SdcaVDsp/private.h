/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    private.h

Abstract:

    Contains structure definitions and function prototypes private to
    the driver.

Environment:

    Kernel mode

Notes:

    Workarounds:

    ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    enables the logic to workaround an ACX v1.0 issue related to ACXFACTORYCIRCUIT race with Add/Remove child WDFDEVICE.
    this issue has been fixed in ACX v1.1.

    ACX_WORKAROUND_ACXPIN_01
    enables the logic to workaround an ACX v1.0 issue related to ACXPIN's KSPROPERTY_PIN_CINSTANCES to return the pin's
    stream instances vs. returning the total # of streams on a circuit. This # is not the same when circuit supports an
    audio engine node, and client has instantiated streams on several pins (host/loopback/offload).
    this issue has been fixed in ACX v1.1.

    ACX_WORKAROUND_ACXPIN_02
    enables the logic to workaround an ACX v1.1 issue related to ACXPIN's KSPROPERTY_PIN_PROPOSEDATAFORMAT set requests 
    directed to an 'offload' pin of an audio engine. The workaround fails the request if there are no enough resources
    (streams). ACX will be enhanced in the future to automatically check this when the pin is tagged as 'offload' pin.
    ACX_WORKAROUND_ACXPIN_01 must be enabled as well for ACX_WORKAROUND_ACXPIN_02 to work.

--*/

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#include "cpp_utils.h"

#include "stdunk.h"
#include <mmsystem.h>
#include <ks.h>
#include <ksmedia.h>

#include "NewDelete.h"

/* make prototypes usable from C++ */
#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <initguid.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <TestProperties.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#include <wdf.h>
#include <acx.h>

#include "AudioAggregation.h"
#include "soundwirecontroller.h"
#include "sdcastreaming.h"

#include "trace.h"

#define PAGED_CODE_SEG __declspec(code_seg("PAGE"))
#define INIT_CODE_SEG __declspec(code_seg("INIT"))

extern RECORDER_LOG g_SDCAVDspLog;

// Check for workaround dependencies.
#ifdef ACX_WORKAROUND_ACXPIN_02
    #ifndef ACX_WORKAROUND_ACXPIN_01
        #error ACX_WORKAROUND_ACXPIN_02 requires ACX_WORKAROUND_ACXPIN_01.
    #endif
#endif

// Copied from cfgmgr32.h
#if !defined(MAX_DEVICE_ID_LEN)
#define MAX_DEVICE_ID_LEN 200
#endif

// Define a NULL GUID if not already defined.
#if !defined(NULL_GUID)
#define NULL_GUID { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } }
#endif

// SDCA Sample driver

#define DRIVER_TAG (ULONG) 'Dcds'

// Number of millisecs per sec. 
#define MS_PER_SEC 1000

// Number of hundred nanosecs per sec. 
#define HNS_PER_SEC 10000000

// Compatible ID for render/capture
#define ACX_DSP_RENDER_COMPATIBLE_ID   L"{ad164f4d-4149-41ed-82e8-99732ed7371a}"

// Container ID for render/capture
#define ACX_DSP_SYSTEM_CONTAINER_ID    L"{00000000-0000-0000-ffff-ffffffffffff}"


// Compatible ID for render/capture
#define ACX_DSP_TEST_COMPATIBLE_ID   L"{ad164f4d-4149-41ed-82e8-99732ed7371a}"
// Container ID for render/capture
#define ACX_DSP_TEST_CONTAINER_ID    L"{00000000-0000-0000-ffff-ffffffffffff}"

extern const GUID DSP_CIRCUIT_SPEAKER_GUID;
extern const GUID DSP_CIRCUIT_MICROPHONE_GUID;
extern const GUID DSP_CIRCUIT_UNIVERSALJACK_RENDER_GUID;
extern const GUID DSP_CIRCUIT_UNIVERSALJACK_CAPTURE_GUID;

extern const GUID SYSTEM_CONTAINER_GUID;

#undef MIN
#undef MAX
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define REQUEST_TIMEOUT_SECONDS 5

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

//
// Define DSP driver context.
//
typedef struct _DSP_DRIVER_CONTEXT {
    BOOLEAN         Dummy;
} DSP_DRIVER_CONTEXT, *PDSP_DRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_DRIVER_CONTEXT, GetDspDriverContext)

#define ALL_CHANNELS_ID             UINT32_MAX
#define MAX_CHANNELS                2
#define CHANNEL_MASK_INVALID        UINT32_MAX
    
//
// Define DSP device context.
//
typedef struct _DSP_DEVICE_CONTEXT {
    ACXCIRCUIT          Render;
    ACXCIRCUIT          Capture;
    ACXFACTORYCIRCUIT   Factory;
    WDFDEVICE           AudioSensorsDevice;
    WDFCHILDLIST        ChildList;
} DSP_DEVICE_CONTEXT, *PDSP_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_DEVICE_CONTEXT, GetDspDeviceContext)

//
// Define Audio Sensors device context.
//
typedef struct _AUDIO_SENSORS_DEVICE_CONTEXT
{
    WDFDEVICE           Device;
} AUDIO_SENSORS_DEVICE_CONTEXT, *PAUDIO_SENSORS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(AUDIO_SENSORS_DEVICE_CONTEXT, GetAudioSensorsDeviceContext)

//
// Define DSP factory context.
//
typedef struct _DSP_FACTORY_CONTEXT {
    WDFDEVICE           Device;
#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
    WDFWAITLOCK         CacheLock;
    WDFCOLLECTION       Cache;
#endif
} DSP_FACTORY_CONTEXT, *PDSP_FACTORY_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_FACTORY_CONTEXT, GetDspFactoryContext)

#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
//
// Define DSP device ID context.
//
typedef struct _DSP_DEVICEID_CONTEXT {
    ACXFACTORYCIRCUIT   Factory;
    GUID                UniqueID;
} DSP_DEVICEID_CONTEXT, *PDSP_DEVICEID_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_DEVICEID_CONTEXT, GetDspDeviceIdContext)
#endif // ACX_WORKAROUND_ACXFACTORYCIRCUIT_01

//
// Define RENDER device context.
//
typedef struct _DSP_RENDER_DEVICE_CONTEXT {
    ACXCIRCUIT          Circuit;
    BOOLEAN             FirstTimePrepareHardware;
} DSP_RENDER_DEVICE_CONTEXT, *PDSP_RENDER_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_RENDER_DEVICE_CONTEXT, GetRenderDeviceContext)

// Special stream definitions
typedef enum _SDCA_SPECIALSTREAM_TYPE
{
    SpecialStreamTypeNotSupported           = 0,
    SpecialStreamTypeUltrasoundRender       = 1,
    SpecialStreamTypeUltrasoundCapture      = 2,
    SpecialStreamTypeReferenceStream        = 3,
    SpecialStreamTypeIvSense                = 4,
    SpecialStreamType_Count                 = 5,
} SDCA_SPECIALSTREAM_TYPE, *PSDCA_SPECIALSTREAM_TYPE;

inline const SDCA_SPECIALSTREAM_TYPE SpecialStreamTypeFromSdcaPath(SDCA_PATH path)
{
    switch(path)
    {
        case SdcaPathUltrasoundRender:
            return SpecialStreamTypeUltrasoundRender;
        case SdcaPathUltrasoundCapture:
            return SpecialStreamTypeUltrasoundCapture;
        case SdcaPathReferenceStream:
            return SpecialStreamTypeReferenceStream;
        case SdcaPathIvSense:
            return SpecialStreamTypeIvSense;
    }
    return SpecialStreamTypeNotSupported;
}

inline const SDCA_PATH SdcaPathFromSpecialStreamType(SDCA_SPECIALSTREAM_TYPE type)
{
    switch(type)
    {
        case SpecialStreamTypeUltrasoundRender:
            return SdcaPathUltrasoundRender;
        case SpecialStreamTypeUltrasoundCapture:
            return SdcaPathUltrasoundCapture;
        case SpecialStreamTypeReferenceStream:
            return SdcaPathReferenceStream;
        case SpecialStreamTypeIvSense:
            return SdcaPathIvSense;
    }
    return (SDCA_PATH) 0;
}

// Maximum of 8 devices chosen for the purpose of making this sample simpler
#define MAX_AGGREGATED_DEVICES (8)

//
// Define circuit context.
//
typedef struct _DSP_CIRCUIT_CONTEXT {
    ULONG               EndpointId;
    ULONG               DataPortNumber;
    ACXAUDIOENGINE      AudioEngineElement;
    ACXPEAKMETER        PeakMeterElement;
    PVOID               peakMeter;
    ACXKEYWORDSPOTTER   KeywordSpotter;

    // If the VolumeMuteHandler is set, we will forward any
    // Volume/Mute requests for the current circuit to this
    // target circuit. If the target circuit was allocated
    // by this driver, it will also be copied to
    // TargetCircuitToDelete
    ACXTARGETCIRCUIT    TargetCircuitToDelete;
    ACXTARGETCIRCUIT    TargetVolumeMuteCircuit;
    ACXTARGETELEMENT    TargetVolumeHandler;
    ACXTARGETELEMENT    TargetMuteHandler;

    BOOLEAN             IsRenderCircuit;

    // This will contain information on the aggregated devices we're connected to
    BOOLEAN                         Aggregated;
    ULONG                           AggregatedDeviceCount;
    SDCA_AGGREGATION_DEVICE         AggregatedDevices[MAX_AGGREGATED_DEVICES];
    PSDCA_PATH_DESCRIPTORS2         AggregatedPathDescriptors;

    ULONG                           SpecialStreamAvailablePaths;
    PSDCA_PATH_DESCRIPTORS          SpecialStreamPathDescriptors[SpecialStreamType_Count];
    PSDCA_PATH_DESCRIPTORS2         SpecialStreamPathDescriptors2[SpecialStreamType_Count];
    ULONG                           SpecialStreamActive[SpecialStreamType_Count];
    ULONG                           SpecialStreamRunning[SpecialStreamType_Count];
    ACXTARGETCIRCUIT                SpecialStreamTargetCircuit;
    // The ConnectedFunctionInformation will be used with SpecialStream logic and also
    // for determining appropriate data ports to be used with each connected audio function
    PSDCA_FUNCTION_INFORMATION_LIST ConnectedFunctionInformation;

} DSP_CIRCUIT_CONTEXT, * PDSP_CIRCUIT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_CIRCUIT_CONTEXT, GetDspCircuitContext)

//
// Define CAPTURE device context.
//
typedef struct _DSP_CAPTURE_DEVICE_CONTEXT {
    ACXCIRCUIT      Circuit;
    BOOLEAN         FirstTimePrepareHardware;
} DSP_CAPTURE_DEVICE_CONTEXT, *PDSP_CAPTURE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_CAPTURE_DEVICE_CONTEXT, GetCaptureDeviceContext)

//
// Define DSP circuit/stream element context.
//
typedef struct _DSP_ELEMENT_CONTEXT {
    BOOLEAN         Dummy;
} DSP_ELEMENT_CONTEXT, *PDSP_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_ELEMENT_CONTEXT, GetDspElementContext)

//
// Define DSP format context.
//
typedef struct _DSP_FORMAT_CONTEXT {
    BOOLEAN         Dummy;
} DSP_FORMAT_CONTEXT, *PDSP_FORMAT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_FORMAT_CONTEXT, GetDspFormatContext)

typedef enum _DSP_PIN_TYPE {
    DspPinTypeHost,
    DspPinTypeOffload,
    DspPinTypeLoopback,
    DspPinTypeBridge,
    DspPinType_Count
} DSP_PIN_TYPE, * PDSP_PIN_TYPE;

typedef enum _DSP_CAPTURE_PIN_TYPE {
    DspCapturePinTypeHost,
    DspCapturePinTypeKeyword,
    DspCapturePinTypeBridge,
    DspCapturePinType_Count
} DSP_CAPTURE_PIN_TYPE, * PDSP_CAPTURE_PIN_TYPE;

typedef struct _DSP_PIN_CONTEXT {
    ACXTARGETCIRCUIT        TargetCircuit;
    ULONG                   TargetPinId;
    DSP_PIN_TYPE            PinType;
    DSP_CAPTURE_PIN_TYPE    CapturePinType;

    // The stream bridge below will only be valid for the Capture circuit Bridge Pin

    // Host stream bridge will be used to ensure host stream creations are passed
    // to the downlevel circuits. Since the HostStreamBridge won't have InModes set,
    // the ACX framework will not add streams automatically. We will add streams for
    // non KWS pin.
    ACXSTREAMBRIDGE         HostStreamBridge;
    ACXOBJECTBAG            HostStreamObjBag;

#ifdef ACX_WORKAROUND_ACXPIN_01
    ULONG                   MaxStreams;
    ULONG                   CurrentStreamsCount;
#endif
} DSP_PIN_CONTEXT, *PDSP_PIN_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_PIN_CONTEXT, GetDspPinContext)

//
// Define DSP render/capture stream context.
//
typedef struct _DSP_STREAM_CONTEXT {
    PVOID                   StreamEngine;
    DSP_PIN_TYPE            PinType;
    DSP_CAPTURE_PIN_TYPE    CapturePinType;
    ACXPIN                  Pin;                // used by acx workaround, and reference streams

#ifdef ACX_WORKAROUND_ACXPIN_01
    BOOLEAN                 StreamIsCounted;    // TRUE = stream is counted on the pin.
#endif

    ACXTARGETCIRCUIT        SpecialStreamTargetCircuit;
    BOOLEAN                 SpecialStreamInUse[SpecialStreamType_Count];
    BOOLEAN                 SpecialStreamRunning[SpecialStreamType_Count];
} DSP_STREAM_CONTEXT, *PDSP_STREAM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_STREAM_CONTEXT, GetDspStreamContext)

typedef struct _DSP_ENGINE_CONTEXT {
    ACXDATAFORMAT   MixFormat;
    BOOLEAN         GFxEnabled;
} DSP_ENGINE_CONTEXT, * PDSP_ENGINE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_ENGINE_CONTEXT, GetDspEngineContext)

typedef struct _DSP_STREAMAUDIOENGINE_CONTEXT {
    BOOLEAN         LFxEnabled;
} DSP_STREAMAUDIOENGINE_CONTEXT, * PDSP_STREAMAUDIOENGINE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_STREAMAUDIOENGINE_CONTEXT, GetDspStreamAudioEngineContext)

//
// Define DSP keyword spotter context
//
typedef struct _DSP_KEYWORDSPOTTER_CONTEXT {
    ACXPNPEVENT Event;
    PVOID       KeywordDetector;
} DSP_KEYWORDSPOTTER_CONTEXT, *PDSP_KEYWORDSPOTTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_KEYWORDSPOTTER_CONTEXT, GetDspKeywordSpotterContext)

typedef struct _DSP_PNPEVENT_CONTEXT {
    BOOLEAN         Dummy;
} DSP_PNPEVENT_CONTEXT, *PDSP_PNPEVENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_PNPEVENT_CONTEXT, GetDspPnpEventContext)

//
// Define DSP peakmeter element context.
//
typedef struct _DSP_PEAKMETER_ELEMENT_CONTEXT {
    PVOID           peakMeter;
} DSP_PEAKMETER_ELEMENT_CONTEXT, * PDSP_PEAKMETER_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_PEAKMETER_ELEMENT_CONTEXT, GetDspPeakMeterElementContext)

#define PEAKMETER_STEPPING_DELTA    0x1000
#define PEAKMETER_MAXIMUM           LONG_MAX
#define PEAKMETER_MINIMUM           LONG_MIN

//
// Define DSP circuit/stream element context.
//
typedef struct _DSP_MUTE_ELEMENT_CONTEXT {
    BOOL            MuteState[MAX_CHANNELS];
} DSP_MUTE_ELEMENT_CONTEXT, * PDSP_MUTE_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_MUTE_ELEMENT_CONTEXT, GetDspMuteElementContext)

//
// Define DSP circuit/stream element context.
//
typedef struct _DSP_VOLUME_ELEMENT_CONTEXT {
    LONG            VolumeLevel[MAX_CHANNELS];
} DSP_VOLUME_ELEMENT_CONTEXT, * PDSP_VOLUME_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_VOLUME_ELEMENT_CONTEXT, GetDspVolumeElementContext)

#define VOLUME_STEPPING         0x8000
#define VOLUME_LEVEL_MAXIMUM    0x00000000
#define VOLUME_LEVEL_MINIMUM    (-96 * 0x10000)

//
// Driver prototypes.
//
DRIVER_INITIALIZE                   DriverEntry;
EVT_WDF_DRIVER_UNLOAD               Dsp_DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD           Dsp_EvtBusDeviceAdd;
EVT_WDF_CHILD_LIST_CREATE_DEVICE    Dsp_AddAudioSensorsDevice;

// Device callbacks.

EVT_WDF_DEVICE_PREPARE_HARDWARE     Dsp_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     Dsp_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      Dsp_EvtDeviceContextCleanup;

EVT_ACX_FACTORY_CIRCUIT_CREATE_CIRCUITDEVICE Dsp_EvtAcxFactoryCircuitCreateCircuitDevice;
EVT_ACX_FACTORY_CIRCUIT_CREATE_CIRCUIT Dsp_EvtAcxFactoryCircuitCreateCircuit;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      Dsp_EvtFactoryContextCleanup;
EVT_WDF_DEVICE_CONTEXT_DESTROY      Dsp_EvtFactoryContextDestroy;

// Stream callbacks shared between Capture and Render

EVT_WDF_OBJECT_CONTEXT_DESTROY      Dsp_EvtStreamContextDestroy;
EVT_ACX_STREAM_GET_HW_LATENCY       Dsp_EvtStreamGetHwLatency;
EVT_ACX_STREAM_ALLOCATE_RTPACKETS   Dsp_EvtStreamAllocateRtPackets;
EVT_ACX_STREAM_FREE_RTPACKETS       Dsp_EvtStreamFreeRtPackets;
EVT_ACX_STREAM_PREPARE_HARDWARE     Dsp_EvtStreamPrepareHardware;
EVT_ACX_STREAM_RELEASE_HARDWARE     Dsp_EvtStreamReleaseHardware;
EVT_ACX_STREAM_RUN                  Dsp_EvtStreamRun;
EVT_ACX_STREAM_PAUSE                Dsp_EvtStreamPause;
EVT_ACX_STREAM_GET_CURRENT_PACKET   Dsp_EvtStreamGetCurrentPacket;
EVT_ACX_STREAM_ASSIGN_DRM_CONTENT_ID Dsp_EvtStreamAssignDrmContentId;
EVT_ACX_STREAM_GET_PRESENTATION_POSITION Dsp_EvtStreamGetPresentationPosition;
EVT_ACX_OBJECT_PREPROCESS_REQUEST DspC_EvtStreamRequestPreprocess;


// Render callbacks.
EVT_ACX_FACTORY_CIRCUIT_CREATE_CIRCUITDEVICE DspR_EvtAcxFactoryCircuitCreateCircuitDevice;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DspR_EvtAcxFactoryCircuitCreateCircuit(
    _In_
    WDFDEVICE Parent,
    _In_
    WDFDEVICE Device,
    _In_
    ACXFACTORYCIRCUIT Factory,
    _In_
    PACX_FACTORY_CIRCUIT_ADD_CIRCUIT Config,
    _In_
    PACXCIRCUIT_INIT CircuitInit,
    _In_
    ULONG DataPortNumber,
    _In_opt_
    PSDCA_PATH_DESCRIPTORS2        PathDescriptors
);
    
EVT_ACX_CIRCUIT_COMPOSITE_CIRCUIT_INITIALIZE DspR_EvtCircuitCompositeCircuitInitialize;
EVT_ACX_CIRCUIT_COMPOSITE_INITIALIZE DspR_EvtCircuitCompositeInitialize;
    
EVT_WDF_DEVICE_CONTEXT_CLEANUP      DspR_EvtCircuitContextCleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE     DspR_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     DspR_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT DspR_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      DspR_EvtDeviceContextCleanup;
EVT_ACX_OBJECT_PREPROCESS_REQUEST   DspR_EvtCircuitRequestPreprocess;
EVT_ACX_CIRCUIT_CREATE_STREAM       DspR_EvtCircuitCreateStream;
EVT_ACX_CIRCUIT_POWER_UP            DspR_EvtCircuitPowerUp;
EVT_ACX_CIRCUIT_POWER_DOWN          DspR_EvtCircuitPowerDown;
EVT_ACX_STREAM_SET_RENDER_PACKET    DspR_EvtStreamSetRenderPacket;
EVT_ACX_PIN_SET_DATAFORMAT          DspR_EvtAcxPinSetDataFormat;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      DspR_EvtPinContextCleanup;
EVT_ACX_PIN_CONNECTED               DspR_EvtPinConnected;
EVT_ACX_PIN_DISCONNECTED            DspR_EvtPinDisconnected;

//Render Audio Engine
EVT_ACX_MUTE_ASSIGN_STATE                                   DspR_EvtMuteAssignState;
EVT_ACX_MUTE_RETRIEVE_STATE                                 DspR_EvtMuteRetrieveState;
EVT_ACX_VOLUME_ASSIGN_LEVEL                                 DspR_EvtVolumeAssignLevel;
EVT_ACX_VOLUME_RETRIEVE_LEVEL                               DspR_EvtVolumeRetrieveLevel;
EVT_ACX_PEAKMETER_RETRIEVE_LEVEL                            DspR_EvtPeakMeterRetrieveLevelCallback;
EVT_ACX_RAMPED_VOLUME_ASSIGN_LEVEL                          DspR_EvtRampedVolumeAssignLevel;
EVT_ACX_AUDIOENGINE_RETRIEVE_BUFFER_SIZE_LIMITS             DspR_EvtAcxAudioEngineRetrieveBufferSizeLimits;
EVT_ACX_AUDIOENGINE_RETRIEVE_EFFECTS_STATE                  DspR_EvtAcxAudioEngineRetrieveEffectsState;
EVT_ACX_AUDIOENGINE_ASSIGN_EFFECTS_STATE                    DspR_EvtAcxAudioEngineAssignEffectsState;
EVT_ACX_AUDIOENGINE_RETRIEVE_ENGINE_FORMAT                  DspR_EvtAcxAudioEngineRetrieveEngineMixFormat;
EVT_ACX_AUDIOENGINE_ASSIGN_ENGINE_FORMAT                    DspR_EvtAcxAudioEngineAssignEngineDeviceFormat;
EVT_ACX_STREAMAUDIOENGINE_RETRIEVE_EFFECTS_STATE            DspR_EvtAcxStreamAudioEngineRetrieveEffectsState;
EVT_ACX_STREAMAUDIOENGINE_ASSIGN_EFFECTS_STATE              DspR_EvtAcxStreamAudioEngineAssignEffectsState;
EVT_ACX_STREAMAUDIOENGINE_RETRIEVE_PRESENTATION_POSITION    DspR_EvtAcxStreamAudioEngineRetrievePresentationPosition;
EVT_ACX_STREAMAUDIOENGINE_ASSIGN_CURRENT_WRITE_POSITION     DspR_EvtAcxStreamAudioEngineAssignCurrentWritePosition;
EVT_ACX_STREAMAUDIOENGINE_RETRIEVE_LINEAR_BUFFER_POSITION   DspR_EvtAcxStreamAudioEngineRetrieveLinearBufferPosition;
EVT_ACX_STREAMAUDIOENGINE_ASSIGN_LAST_BUFFER_POSITION       DspR_EvtAcxStreamAudioEngineAssignLastBufferPosition;
EVT_ACX_STREAMAUDIOENGINE_ASSIGN_LOOPBACK_PROTECTION        DspR_EvtAcxStreamAudioEngineAssignLoopbackProtection;

// Capture callbacks.
EVT_ACX_FACTORY_CIRCUIT_CREATE_CIRCUITDEVICE DspC_EvtAcxFactoryCircuitCreateCircuitDevice;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DspC_EvtAcxFactoryCircuitCreateCircuit(
    _In_
    WDFDEVICE Parent,
    _In_
    WDFDEVICE Device,
    _In_
    ACXFACTORYCIRCUIT Factory,
    _In_
    PACX_FACTORY_CIRCUIT_ADD_CIRCUIT Config,
    _In_
    PACXCIRCUIT_INIT CircuitInit,
    _In_
    ULONG DataPortNumber,
    _In_
    PSDCA_PATH_DESCRIPTORS2 PathDescriptors
);

EVT_ACX_CIRCUIT_COMPOSITE_CIRCUIT_INITIALIZE DspC_EvtCircuitCompositeCircuitInitialize;
EVT_ACX_CIRCUIT_COMPOSITE_INITIALIZE DspC_EvtCircuitCompositeInitialize;

EVT_WDF_DEVICE_CONTEXT_CLEANUP          DspC_EvtCircuitContextCleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE         DspC_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE         DspC_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT     DspC_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_CONTEXT_CLEANUP          DspC_EvtDeviceContextCleanup;
EVT_ACX_OBJECT_PREPROCESS_REQUEST       DspC_EvtCircuitRequestPreprocess;
EVT_ACX_CIRCUIT_CREATE_STREAM           DspC_EvtCircuitCreateStream;
EVT_ACX_CIRCUIT_POWER_UP                DspC_EvtCircuitPowerUp;
EVT_ACX_CIRCUIT_POWER_DOWN              DspC_EvtCircuitPowerDown;
EVT_ACX_STREAM_GET_CAPTURE_PACKET       DspC_EvtStreamGetCapturePacket;
EVT_ACX_PIN_SET_DATAFORMAT              DspC_EvtAcxPinSetDataFormat;
EVT_WDF_DEVICE_CONTEXT_CLEANUP          DspC_EvtPinContextCleanup;
EVT_ACX_PIN_CONNECTED                   DspC_EvtPinConnected;
EVT_ACX_PIN_DISCONNECTED                DspC_EvtPinDisconnected;
EVT_ACX_KEYWORDSPOTTER_RETRIEVE_ARM     DspC_EvtAcxKeywordSpotterRetrieveArm;
EVT_ACX_KEYWORDSPOTTER_ASSIGN_ARM       DspC_EvtAcxKeywordSpotterAssignArm;
EVT_ACX_KEYWORDSPOTTER_ASSIGN_PATTERNS  DspC_EvtAcxKeywordSpotterAssignPatterns;
EVT_ACX_KEYWORDSPOTTER_ASSIGN_RESET     DspC_EvtAcxKeywordSpotterAssignReset;

// Property testing, todo: remove them.

EVT_ACX_OBJECT_PROCESS_REQUEST      DspR_EvtPinCInstancesCallback;
EVT_ACX_OBJECT_PROCESS_REQUEST      DspR_EvtPinCTypesCallback;
EVT_ACX_OBJECT_PROCESS_REQUEST      DspR_EvtPinDataFlowCallback;
EVT_ACX_OBJECT_PROCESS_REQUEST      DspR_EvtPinDataRangesCallback;
EVT_ACX_OBJECT_PROCESS_REQUEST      DspR_EvtPinDataIntersectionCallback;
EVT_ACX_OBJECT_PROCESS_REQUEST      DspR_EvtPinPhysicalConnectionCallback;


EVT_ACX_OBJECT_PREPROCESS_REQUEST   DspR_EvtStreamRequestPreprocess;
EVT_WDF_OBJECT_CONTEXT_CLEANUP      Dsp_EvtStreamContextCleanup;

#ifdef ACX_WORKAROUND_ACXPIN_01
EVT_ACX_OBJECT_PREPROCESS_REQUEST   Dsp_EvtStreamGetStreamCountRequestPreprocess;
#endif // ACX_WORKAROUND_ACXPIN_01

#ifdef ACX_WORKAROUND_ACXPIN_02
EVT_ACX_OBJECT_PREPROCESS_REQUEST   Dsp_EvtStreamProposeDataFormatRequestPreprocess;
#endif // ACX_WORKAROUND_ACXPIN_02

/* make internal prototypes usable from C++ */
#ifdef __cplusplus
}
#endif

//
// Used to store the registry settings path for the driver
//
extern UNICODE_STRING g_RegistryPath;

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS
CopyRegistrySettingsPath(
    _In_ PUNICODE_STRING RegistryPath
    );

PAGED_CODE_SEG
NTSTATUS
Dsp_CreateChildList(
    _In_ WDFDEVICE Device
);

PAGED_CODE_SEG
NTSTATUS
Dsp_AddFactoryCircuit(
    _In_ WDFDEVICE  Device
);

PAGED_CODE_SEG
VOID
Dsp_RemoveFactoryCircuit(
    _In_ WDFDEVICE  Device
);

#ifdef ACX_WORKAROUND_ACXFACTORYCIRCUIT_01
PAGED_CODE_SEG
NTSTATUS
Dsp_InitializeChildDevicesCache(
    _In_ ACXFACTORYCIRCUIT Factory
    );

PAGED_CODE_SEG
VOID
Dsp_CleanupChildDevicesCache(
    _In_ ACXFACTORYCIRCUIT  Factory
    );

PAGED_CODE_SEG
VOID
Dsp_DeleteChildDevicesCache(
    _In_ ACXFACTORYCIRCUIT  Factory
    );

PAGED_CODE_SEG
bool
Dsp_IsChildDeviceInCacheLocked(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ const GUID *       UniqueId
    );

PAGED_CODE_SEG
NTSTATUS
Dsp_AddChildDeviceToCache(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ const GUID *       UniqueId,
    _In_ WDFDEVICE          Device
    );

PAGED_CODE_SEG
WDFDEVICE
Dsp_RemoveChildDeviceFromCache(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ const GUID *       UniqueId
    );

PAGED_CODE_SEG
VOID
Dsp_PurgeChildDeviceFromCache(
    _In_ ACXFACTORYCIRCUIT  Factory,
    _In_ WDFDEVICE          Device
    );

EVT_ACX_OBJECT_PREPROCESS_REQUEST   Dsp_EvtFactoryRemoveCircuitRequestPreprocess;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      Dsp_EvtDeviceIdContextCleanup;
EVT_ACX_OBJECT_PROCESS_REQUEST      Dsp_EvtFactoryCircuitRemoveCircuitCallback;
#endif  // ACX_WORKAROUND_ACXFACTORYCIRCUIT_01

PAGED_CODE_SEG
NTSTATUS
DspC_CircuitCleanup(
    _In_ ACXCIRCUIT Device
    );

PAGED_CODE_SEG
NTSTATUS
Dsp_SetPowerPolicy(
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
DspR_SetPowerPolicy(
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
DspC_SetPowerPolicy(
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
DSP_SendPropertyTo
(
    _In_ WDFDEVICE Device,
    _In_ ACXTARGETCIRCUIT TargetCircuit,
    _In_ GUID PropertySet,
    _In_ ULONG PropertyId,
    _In_ ACX_PROPERTY_VERB Verb,
    _In_ PVOID Control,
    _In_ ULONG ControlCb,
    _Inout_ PVOID Value,
    _In_ ULONG ValueCb,
    _Out_ ULONG_PTR* Information
);

PAGED_CODE_SEG
NTSTATUS
Dsp_SendTestPropertyTo(
    _In_ WDFDEVICE Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ GUID PropertySet,
    _In_ ULONG PropertyId,
    _In_ ACX_PROPERTY_VERB Verb,
    _In_ PVOID Control,
    _In_ ULONG ControlCb,
    _Inout_ PVOID Value,
    _In_ ULONG ValueCb,
    _Out_ ULONG_PTR* Information
    );

PAGED_CODE_SEG
VOID
Dsp_SendVendorSpecificProperties(
    _In_ WDFDEVICE Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ BOOLEAN SetValue
    );


// Create a single SDCA_PATH for special streaming associated
// with the given ACXSTREAM
PAGED_CODE_SEG
NTSTATUS
Dsp_PrepareSpecialStreamForStream(
    _In_ ACXSTREAM Stream,
    _In_ SDCA_SPECIALSTREAM_TYPE SpecialStreamType,
    _In_ ULONG FunctionBitMask = 0xffffffff
    );

// Destroy all SDCA_PATHs associated with the given ACXSTREAM
PAGED_CODE_SEG
NTSTATUS
Dsp_ReleaseSpecialStreamsForStream(
    _In_ ACXSTREAM Stream
    );

// Start all SDCA_PATHs associated with the given ACXSTREAM
PAGED_CODE_SEG
NTSTATUS
Dsp_StartSpecialStreamsForStream(
    _In_ ACXSTREAM  Stream
    );

// Stop all SDCA_PATHs associated with the given ACXSTREAM
PAGED_CODE_SEG
NTSTATUS
Dsp_StopSpecialStreamsForStream(
    _In_ ACXSTREAM  Stream
    );

#endif // _PRIVATE_H_


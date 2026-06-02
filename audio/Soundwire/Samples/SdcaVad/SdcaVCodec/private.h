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

--*/

#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#include "cpp_utils.h"

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

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#include <wdf.h>
#include <acx.h>

#include "SoundWireController.h"
#include "SdcaXu.h"

#include "trace.h"

#include <TestProperties.h>

#define PAGED_CODE_SEG __declspec(code_seg("PAGE"))
#define INIT_CODE_SEG __declspec(code_seg("INIT"))

extern RECORDER_LOG g_SDCAVCodecLog;

// Simple ACX driver
#define DRIVER_TAG (ULONG) 'Ccds'

// Number of millisecs per sec. 
#define MS_PER_SEC 1000

// Number of hundred nanosecs per sec. 
#define HNS_PER_SEC 10000000

// Compatible ID for render/capture
#define ACX_CODEC_TEST_COMPATIBLE_ID   L"{99a0ee05-7167-4b63-843d-19d6d285942e}"

// Container ID for render/capture
#define ACX_CODEC_TEST_CONTAINER_ID    L"{00000000-0000-0000-ffff-ffffffffffff}"

#define RENDER_CIRCUIT_URI L"test:obj-path:\\SDCAVCODEC\\RENDER"
#define EXT_RENDER_CIRCUIT_URI L"test:obj-path:\\SDCAVCODEC\\RENDER_xu"
#define CAPTURE_CIRCUIT_URI L"test:obj-path:\\SDCAVCODEC\\CAPTURE"
#define EXT_CAPTURE_CIRCUIT_URI L"test:obj-path:\\SDCAVCODEC\\CAPTURE_xu"

#undef MIN
#undef MAX
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

#ifndef RGB
#define RGB(r, g, b) (DWORD)(r << 16 | g << 8 | b)
#endif


//
// Example Acpi blob for Hardware configuration
//
typedef struct _SdcaXuAcpiBlob
{
    // Number of endpoints
    ULONG NumEndpoints;

}SdcaXuAcpiBlob, *PSdcaXuAcpiBlob;

#define ALL_CHANNELS_ID             UINT32_MAX
#define MAX_CHANNELS                2

//
// Ks support.
//
#define KSPROPERTY_TYPE_ALL         KSPROPERTY_TYPE_BASICSUPPORT | \
                                    KSPROPERTY_TYPE_GET | \
                                    KSPROPERTY_TYPE_SET

//
// Define CODEC driver context.
//
typedef struct _CODEC_DRIVER_CONTEXT {
    ULONG reserved;
} CODEC_DRIVER_CONTEXT, *PCODEC_DRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_DRIVER_CONTEXT, GetCodecDriverContext)

//
// Extension Unit specific data
//
typedef struct _SDCAXU_DATA
{
    BOOLEAN bSdcaXu;
    BOOLEAN bExtensionJackOVerride;
    SDCAXU_INTERFACE_V0101 ExtensionInterface;
    ULONG numXUEntities;
    ULONG *XUEntities;
    PSDCAXU_INTERRUPT_INFO InterruptInfo;
}SDCAXU_DATA, *PSDCAXU_DATA;

//
// Define CODEC device context.
//
typedef struct _CODEC_DEVICE_CONTEXT {
    ACXCIRCUIT              Render;
    ACXCIRCUIT              Capture;
    ACXCOMPOSITETEMPLATE    Composite[2];
    ULONG                   refComposite[2];
    SDCAXU_DATA             SdcaXuData;

} CODEC_DEVICE_CONTEXT, *PCODEC_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_DEVICE_CONTEXT, GetCodecDeviceContext)

//
// Define RENDER device context.
//
typedef struct _CODEC_RENDER_DEVICE_CONTEXT {
    ACXCIRCUIT      Circuit;
    BOOLEAN         FirstTimePrepareHardware;
} CODEC_RENDER_DEVICE_CONTEXT, *PCODEC_RENDER_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_RENDER_DEVICE_CONTEXT, GetRenderDeviceContext)
    
//
// Define RENDER circuit context.
//
typedef struct _CODEC_RENDER_CIRCUIT_CONTEXT {
    ACXMUTE         MuteElement;
    ACXVOLUME       VolumeElement;
} CODEC_RENDER_CIRCUIT_CONTEXT, *PCODEC_RENDER_CIRCUIT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_RENDER_CIRCUIT_CONTEXT, GetRenderCircuitContext)

//
// Define CAPTURE device context.
//
typedef struct _CODEC_CAPTURE_DEVICE_CONTEXT {
    ACXCIRCUIT      Circuit;
    BOOLEAN         FirstTimePrepareHardware;
} CODEC_CAPTURE_DEVICE_CONTEXT, *PCODEC_CAPTURE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_CAPTURE_DEVICE_CONTEXT, GetCaptureDeviceContext)
    
//
// Define CAPTURE circuit context.
//
typedef struct _CODEC_CAPTURE_CIRCUIT_CONTEXT {
    BOOLEAN         KwsActiveVadStream;
    KEVENT          KwsSuspendEvent;
    KEVENT          KwsResumeEvent;
    ACXDATAFORMAT   KwsDataFormat;
} CODEC_CAPTURE_CIRCUIT_CONTEXT, *PCODEC_CAPTURE_CIRCUIT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_CAPTURE_CIRCUIT_CONTEXT, GetCaptureCircuitContext)
    
//
// Define CODEC render/capture stream context.
//
typedef struct _CODEC_STREAM_CONTEXT {
    PVOID           StreamEngine;
} CODEC_STREAM_CONTEXT, *PCODEC_STREAM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_STREAM_CONTEXT, GetCodecStreamContext)

//
// Define CODEC circuit/stream element context.
//
typedef struct _CODEC_ELEMENT_CONTEXT {
    BOOLEAN         Dummy;
} CODEC_ELEMENT_CONTEXT, *PCODEC_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_ELEMENT_CONTEXT, GetCodecElementContext)
    
//
// Define CODEC circuit/stream element context.
//
typedef struct _CODEC_MUTE_ELEMENT_CONTEXT {
    BOOL            MuteState[MAX_CHANNELS]; 
    WDFTIMER        Timer; // for testing only.
} CODEC_MUTE_ELEMENT_CONTEXT, *PCODEC_MUTE_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_MUTE_ELEMENT_CONTEXT, GetCodecMuteElementContext)

//
// Define CODEC mute timer context.
//
typedef struct _CODEC_MUTE_TIMER_CONTEXT {
    ACXMUTE      MuteElement;
} CODEC_MUTE_TIMER_CONTEXT, *PCODEC_MUTE_TIMER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_MUTE_TIMER_CONTEXT, GetCodecMuteTimerContext)
    
//
// Define CODEC circuit/stream element context.
//
typedef struct _CODEC_VOLUME_ELEMENT_CONTEXT {
    LONG            VolumeLevel[MAX_CHANNELS];
    WDFTIMER        Timer; // for testing only.
} CODEC_VOLUME_ELEMENT_CONTEXT, *PCODEC_VOLUME_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_VOLUME_ELEMENT_CONTEXT, GetCodecVolumeElementContext)


#define VOLUME_STEPPING         0x8000
#define VOLUME_LEVEL_MAXIMUM    0x00000000
#define VOLUME_LEVEL_MINIMUM    (-96 * 0x10000)

//
// Define CODEC mute timer context.
//
typedef struct _CODEC_VOLUME_TIMER_CONTEXT {
    ACXVOLUME       VolumeElement;
} CODEC_VOLUME_TIMER_CONTEXT, *PCODEC_VOLUME_TIMER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_VOLUME_TIMER_CONTEXT, GetCodecVolumeTimerContext)


//
// Define CODEC format context.
//
typedef struct _CODEC_FORMAT_CONTEXT {
    BOOLEAN         Dummy;
} CODEC_FORMAT_CONTEXT, *PCODEC_FORMAT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_FORMAT_CONTEXT, GetCodecFormatContext)
    
typedef struct _CODEC_PIN_CONTEXT {
    BOOLEAN         Dummy;
} CODEC_PIN_CONTEXT, *PCODEC_PIN_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_PIN_CONTEXT, GetCodecPinContext)

typedef struct _CODEC_JACK_CONTEXT
{
    ULONG Dummy;
} CODEC_JACK_CONTEXT, *PCODEC_JACK_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CODEC_JACK_CONTEXT, GetCodecJackContext)

typedef enum {
    CompositeType_RENDER,
    CompositeType_CAPTURE
}CompositeType;

//
// Driver prototypes.
//
DRIVER_INITIALIZE                   DriverEntry;
EVT_WDF_DRIVER_UNLOAD               Codec_DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD           Codec_EvtBusDeviceAdd;

// Device callbacks.

EVT_WDF_DEVICE_PREPARE_HARDWARE     Codec_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     Codec_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      Codec_EvtDeviceContextCleanup;

// Stream callbacks shared between Capture and Render

EVT_WDF_OBJECT_CONTEXT_DESTROY      Codec_EvtStreamDestroy;
EVT_ACX_STREAM_GET_HW_LATENCY       Codec_EvtStreamGetHwLatency;
EVT_ACX_STREAM_PREPARE_HARDWARE     Codec_EvtStreamPrepareHardware;
EVT_ACX_STREAM_RELEASE_HARDWARE     Codec_EvtStreamReleaseHardware;
EVT_ACX_STREAM_RUN                  Codec_EvtStreamRun;
EVT_ACX_STREAM_PAUSE                Codec_EvtStreamPause;
EVT_ACX_STREAM_ASSIGN_DRM_CONTENT_ID Codec_EvtStreamAssignDrmContentId;

// Render callbacks.

EVT_ACX_OBJECT_PREPROCESS_REQUEST   CodecR_EvtCircuitRequestPreprocess;
EVT_ACX_CIRCUIT_CREATE_STREAM       CodecR_EvtCircuitCreateStream;
EVT_ACX_CIRCUIT_POWER_UP            CodecR_EvtCircuitPowerUp;
EVT_ACX_CIRCUIT_POWER_DOWN          CodecR_EvtCircuitPowerDown;
EVT_ACX_STREAM_SET_RENDER_PACKET    CodecR_EvtStreamSetRenderPacket;
EVT_ACX_PIN_SET_DATAFORMAT          CodecR_EvtAcxPinSetDataFormat;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      CodecR_EvtPinContextCleanup;

EVT_ACX_CIRCUIT_COMPOSITE_CIRCUIT_INITIALIZE CodecR_EvtCircuitCompositeCircuitInitialize;
EVT_ACX_CIRCUIT_COMPOSITE_INITIALIZE CodecR_EvtCircuitCompositeInitialize;

// Capture callbacks.

EVT_ACX_OBJECT_PREPROCESS_REQUEST   CodecC_EvtCircuitRequestPreprocess;
EVT_ACX_CIRCUIT_CREATE_STREAM       CodecC_EvtCircuitCreateStream;
EVT_ACX_CIRCUIT_POWER_UP            CodecC_EvtCircuitPowerUp;
EVT_ACX_CIRCUIT_POWER_DOWN          CodecC_EvtCircuitPowerDown;
EVT_ACX_STREAM_GET_CAPTURE_PACKET   CodecC_EvtStreamGetCapturePacket;
EVT_ACX_PIN_SET_DATAFORMAT          CodecC_EvtAcxPinSetDataFormat;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      CodecC_EvtPinContextCleanup;

EVT_ACX_OBJECT_PREPROCESS_REQUEST   CodecC_EvtStreamRequestPreprocess;

EVT_ACX_OBJECT_PROCESS_REQUEST      CodecC_EvtCircuitDeviceKwsCapability;
EVT_ACX_OBJECT_PROCESS_REQUEST      CodecC_EvtCircuitVadCapability;
EVT_ACX_OBJECT_PROCESS_REQUEST      CodecC_EvtCircuitVadEntities;
EVT_ACX_OBJECT_PROCESS_REQUEST      CodecC_EvtCircuitSetKwsAccessEvents;
EVT_ACX_OBJECT_PROCESS_REQUEST      CodecC_EvtCircuitConfigureVadPort;
EVT_ACX_OBJECT_PROCESS_REQUEST      CodecC_EvtCircuitCleanupVadPort;

EVT_ACX_MUTE_ASSIGN_STATE           CodecR_EvtMuteAssignStateCallback;
EVT_ACX_MUTE_RETRIEVE_STATE         CodecR_EvtMuteRetrieveStateCallback;
EVT_WDF_TIMER                       CodecR_EvtMuteTimerFunc;
EVT_ACX_VOLUME_ASSIGN_LEVEL         CodecR_EvtVolumeAssignLevelCallback;
EVT_ACX_VOLUME_RETRIEVE_LEVEL       CodecR_EvtVolumeRetrieveLevelCallback;
EVT_WDF_TIMER                       CodecR_EvtVolumeTimerFunc;
EVT_ACX_OBJECT_PREPROCESS_REQUEST   CodecR_EvtStreamRequestPreprocess;

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

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS 
Codec_AddComposites(_In_ WDFDEVICE Device, _In_ CompositeType compositeType);

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS 
Codec_AddRenderComposites(_In_ WDFDEVICE Device);

__drv_requiresIRQL(PASSIVE_LEVEL)
PAGED_CODE_SEG
NTSTATUS 
Codec_AddCaptureComposites(_In_ WDFDEVICE Device);

#pragma code_seg()
NTSTATUS 
Codec_RemoveComposites(_In_ WDFDEVICE Device);

PAGED_CODE_SEG
NTSTATUS
Codec_SetPowerPolicy(
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
CodecR_AddRenders(
    _In_ WDFDRIVER      Driver,
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
CodecR_AddStaticRender(
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
CodecR_CreateRenderCircuit(
    _In_     WDFDEVICE      Device,
    _Out_    ACXCIRCUIT *   Circuit
    );

PAGED_CODE_SEG
NTSTATUS
CodecC_AddCaptures(
    _In_ WDFDRIVER      Driver,
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
CodecC_AddStaticCapture(
    _In_ WDFDEVICE      Device
    );

PAGED_CODE_SEG
NTSTATUS
CodecC_CreateCaptureCircuit(
    _In_     WDFDEVICE      Device,
    _Out_    ACXCIRCUIT *   Circuit
    );

//
// Extension Unit
//
PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetJackOverride
(
    _In_ PVOID      Context,    // SDCA Context
    _In_ BOOLEAN    Override    // TRUE:  Override
                                // FALSE: Default SDCA behavior
);

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetJackSelectedMode
(
    _In_ PVOID Context,         // SDCA Context
    _In_ ULONG GroupEntityId,   // SDCA Group Entity ID for Jack(s)
    _In_ ULONG SelectedMode     // Type of jack type overriden by XU
);

#pragma code_seg()
NTSTATUS Codec_SdcaXuPDEPowerReferenceAcquire
(
    _In_ PVOID              Context,
    _In_ ULONG              PowerDomainEntityId,
    _In_ SDCAXU_POWER_STATE RequiredState
);

#pragma code_seg()
NTSTATUS Codec_SdcaXuPDEPowerReferenceRelease
(
    _In_ PVOID              Context,
    _In_ ULONG              PowerDomainEntityId,
    _In_ SDCAXU_POWER_STATE ReleasedState
);

#pragma code_seg()
NTSTATUS Codec_SdcaXuReadDeferredAudioControls
(
    _In_ PVOID                      Context,
    _Inout_ PSDCA_AUDIO_CONTROLS    Controls
);

#pragma code_seg()
NTSTATUS Codec_SdcaXuWriteDeferredAudioControls
(
    _In_ PVOID                      Context,
    _Inout_ PSDCA_AUDIO_CONTROLS    Controls
);

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetXUEntities
(
    _In_        PVOID   Context,
    _In_        ULONG   NumEntities,
    _In_reads_(NumEntities)
                ULONG   EntityIDs[]
);

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuRegisterForInterrupts
(
    _In_    PVOID                   Context,
    _In_    PSDCAXU_INTERRUPT_INFO  InterruptInfo
);

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetRenderEndpointConfig
(
    _In_ WDFDEVICE Device,
    _In_ ACXCIRCUIT Circuit
);

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetCaptureEndpointConfig
(
    _In_ WDFDEVICE Device,
    _In_ ACXCIRCUIT Circuit
);

PAGED_CODE_SEG
NTSTATUS Codec_GetSdcaXu(_In_ WDFDEVICE Device);

PAGED_CODE_SEG
NTSTATUS Codec_SetSdcaXuHwConfig(_In_ WDFDEVICE Device);

#pragma code_seg()

#endif // _PRIVATE_H_

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

#include "stdunk.h"
#include <mmsystem.h>
#include <ks.h>

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

#include "Trace.h"

#define PAGED_CODE_SEG __declspec(code_seg("PAGE"))
#define INIT_CODE_SEG __declspec(code_seg("INIT"))

extern RECORDER_LOG g_SDCAVXuLog;

// Simple ACX driver
#define DRIVER_TAG (ULONG) 'Ecds'

// Number of msec for idle timeout.
#define IDLE_POWER_TIMEOUT 5000

// Number of millisecs per sec. 
#define MS_PER_SEC 1000

// Number of hundred nanosecs per sec. 
#define HNS_PER_SEC 10000000

// Compatible ID for render/capture
#define SDCAVAD_COMPATIBLE_ID L"{2172A9B3-0690-4BB6-88FF-B778E999E04A}"

// Container ID for render/capture
#define SDCAVAD_CONTAINER_ID L"{00000000-0000-0000-ffff-ffffffffffff}"

// Bridge Pin Number for Circuit
const ULONG _BRIDGE_PIN_ID = 1;

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

#define SDCA_INTERRUPT_DEFINE_MASK(Mask, ...)\
    {\
        unsigned char parameters[] = {__VA_ARGS__};\
        for(unsigned char iParameter = 0; iParameter < SIZEOF_ARRAY(parameters); iParameter ++) \
        {\
            Mask = Mask | (0x1<<(unsigned long)parameters[iParameter]); \
        }\
    }\

#ifdef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
#define STATIC_KSPROPERTYSETID_AcxCircuit\
    0x4d12807eL, 0x55db, 0x48b8, 0xa4, 0x66, 0xf1, 0x5a, 0x51, 0x0f, 0x58, 0x17
DEFINE_GUIDSTRUCT("4d12807e-55db-48b8-a466-f15a510f5817", KSPROPERTYSETID_AcxCircuit);
#define KSPROPERTYSETID_AcxCircuit DEFINE_GUIDNAMED(KSPROPERTYSETID_AcxCircuit)

typedef enum {
    KSPROPERTY_ACXCIRCUIT_INFORMATION = 1,    // get
    KSPROPERTY_ACXCIRCUIT_SETNOTIFICATIONDEVICE,
    KSPROPERTY_ACXCIRCUIT_SETNOTIFICATIONID,
    KSPROPERTY_ACXCIRCUIT_SETINSTANCEID,
} KSPROPERTY_ACXCIRCUIT;
#endif

// NULL GUID
static const GUID NULL_GUID = 
{0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

//
// Example Acpi blob for Hardware configuration
//
typedef struct _SdcaXuAcpiBlob
{
    // Number of endpoints
    ULONG NumEndpoints;

}SdcaXuAcpiBlob, *PSdcaXuAcpiBlob;

// Test Structure so the Extension Unit Driver can do the right things.
typedef struct _EXTENSION_UNIT_HW_DATA
{
    UINT8 AvailableXUEntities[8];
    UINT8 AvailableGroupEntities[8];
} EXTENSION_UNIT_HW_DATA, * PEXTENSION_UNIT_HW_DATA;

//
// Define XU driver context.
//
typedef struct _SDCAXU_DRIVER_CONTEXT {
    BOOLEAN         Dummy;
} SDCAXU_DRIVER_CONTEXT, *PSDCAXU_DRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_DRIVER_CONTEXT, GetSdcaXuDriverContext)

typedef struct _SDCA_DEVICE_DATA
{
    BOOLEAN bSDCAInterface;
    PVOID SDCAContext;
    SDCAXU_INTERFACE_V0101 SDCAInterface;

    ULONG NumEndpoints;
    EXTENSION_UNIT_HW_DATA HwData;
}SDCA_DEVICE_DATA, * PSDCA_DEVICE_DATA;

#define MAX_ENDPOINT_COUNT 6
typedef struct _ENDPOINT_DEVICE_PAIR
{
    GUID            CircuitId;
    UNICODE_STRING  CircuitUri;
    WDFDEVICE       CircuitDevice;
}ENDPOINT_DEVICE_PAIR, *PENDPOINT_DEVICE_PAIR;
//
// Define XU device context.
//
typedef struct _SDCAXU_DEVICE_CONTEXT {
    ACXCIRCUIT Render;
    ACXCIRCUIT Capture;
    SDCA_DEVICE_DATA SDCADeviceData;
    WDFDEVICE CircuitDevice;

    ENDPOINT_DEVICE_PAIR EndpointDevices[MAX_ENDPOINT_COUNT];
} SDCAXU_DEVICE_CONTEXT, *PSDCAXU_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_DEVICE_CONTEXT, GetSdcaXuDeviceContext)

//
// Define Circuit Device context.
//
typedef struct _SDCAXU_CIRCUIT_DEVICE_CONTEXT {
    BOOLEAN FirstTimePrepareHardware;
} SDCAXU_CIRCUIT_DEVICE_CONTEXT, *PSDCAXU_CIRCUIT_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_CIRCUIT_DEVICE_CONTEXT, GetCircuitDeviceContext)

//
// Define circuit context for module-hosting circuit.
//
typedef struct _SdcaXuMODULECIRCUIT_CONTEXT {
    BOOLEAN         Dummy;
} SDCAXU_MODULECIRCUIT_CONTEXT, *PSDCAXU_MODULECIRCUIT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_MODULECIRCUIT_CONTEXT, GetModuleCircuitContext)

//
// Define RENDER device context.
//
typedef struct _SDCAXU_RENDER_DEVICE_CONTEXT {
    ACXCIRCUIT Circuit;
    BOOLEAN FirstTimePrepareHardware;
} SDCAXU_RENDER_DEVICE_CONTEXT, *PSDCAXU_RENDER_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_RENDER_DEVICE_CONTEXT, GetRenderDeviceContext)
    
//
// Define RENDER circuit context.
//
typedef struct _SDCAXU_RENDER_CIRCUIT_CONTEXT {
    PSDCAXU_ACX_CIRCUIT_CONFIG CircuitConfig;
    ULONG CommitGroupHandle;
#ifdef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
    WDFDEVICE EndpointDevice;
    WDFIOTARGET WdfIoNotificationTarget;
    GUID PnpNotificationId;
    DWORD InstanceId;
#endif
} SDCAXU_RENDER_CIRCUIT_CONTEXT, *PSDCAXU_RENDER_CIRCUIT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_RENDER_CIRCUIT_CONTEXT, GetRenderCircuitContext)

//
// Define CAPTURE device context.
//
typedef struct _SDCAXU_CAPTURE_DEVICE_CONTEXT {
    ACXCIRCUIT Circuit;
    BOOLEAN FirstTimePrepareHardware;
} SDCAXU_CAPTURE_DEVICE_CONTEXT, *PSDCAXU_CAPTURE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_CAPTURE_DEVICE_CONTEXT, GetCaptureDeviceContext)
    
//
// Define CAPTURE circuit context.
//
typedef struct _SDCAXU_CAPTURE_CIRCUIT_CONTEXT {
    PSDCAXU_ACX_CIRCUIT_CONFIG CircuitConfig;
} SDCAXU_CAPTURE_CIRCUIT_CONTEXT, *PSDCAXU_CAPTURE_CIRCUIT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_CAPTURE_CIRCUIT_CONTEXT, GetCaptureCircuitContext)

//
// Define XU render/capture stream context.
//
typedef struct _SDCAXU_STREAM_CONTEXT {
    PVOID           StreamEngine;
} SDCAXU_STREAM_CONTEXT, *PSDCAXU_STREAM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_STREAM_CONTEXT, GetSdcaXuStreamContext)

//
// Define XU circuit/stream element context.
//
typedef struct _SDCAXU_ELEMENT_CONTEXT {
    BOOLEAN         Dummy;
} SDCAXU_ELEMENT_CONTEXT, *PSDCAXU_ELEMENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_ELEMENT_CONTEXT, GetSdcaXuElementContext)

//
// Define XU format context.
//
typedef struct _SDCAXU_FORMAT_CONTEXT {
    BOOLEAN         Dummy;
} SDCAXU_FORMAT_CONTEXT, *PSDCAXU_FORMAT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_FORMAT_CONTEXT, GetSdcaXuFormatContext)
    
typedef struct _SDCAXU_PIN_CONTEXT {
    BOOLEAN         Dummy;
} SDCAXU_PIN_CONTEXT, *PSDCAXU_PIN_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_PIN_CONTEXT, GetSdcaXuPinContext)

typedef struct _SDCAXU_PNPEVENT_CONTEXT {
    BOOLEAN         Dummy;
} SDCAXU_PNPEVENT_CONTEXT, *PSDCAXU_PNPEVENT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_PNPEVENT_CONTEXT, GetSdcaXuPnpEventContext)


//
// Driver prototypes.
//
DRIVER_INITIALIZE                   DriverEntry;
DRIVER_UNLOAD                       SdcaXu_DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD           SdcaXu_EvtBusDeviceAdd;

// Device callbacks.

EVT_WDF_DEVICE_PREPARE_HARDWARE     SdcaXu_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE     SdcaXu_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_CONTEXT_CLEANUP      SdcaXu_EvtDeviceContextCleanup;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  SdcaXu_EvtIoDeviceControl;

EVT_ACX_PIN_CONNECTED               SdcaXu_EvtPinConnected;

// Stream callbacks shared between Capture and Render

EVT_WDF_OBJECT_CONTEXT_DESTROY          SdcaXu_EvtStreamDestroy;
EVT_ACX_STREAM_GET_HW_LATENCY           SdcaXu_EvtStreamGetHwLatency;
EVT_ACX_STREAM_PREPARE_HARDWARE         SdcaXu_EvtStreamPrepareHardware;
EVT_ACX_STREAM_RELEASE_HARDWARE         SdcaXu_EvtStreamReleaseHardware;
EVT_ACX_STREAM_RUN                      SdcaXu_EvtStreamRun;
EVT_ACX_STREAM_PAUSE                    SdcaXu_EvtStreamPause;
EVT_ACX_STREAM_ASSIGN_DRM_CONTENT_ID    SdcaXu_EvtStreamAssignDrmContentId;

EVT_ACX_FACTORY_CIRCUIT_CREATE_CIRCUITDEVICE    SdcaXu_EvtAcxFactoryCircuitCreateCircuitDevice;
EVT_ACX_FACTORY_CIRCUIT_CREATE_CIRCUIT          SdcaXu_EvtAcxFactoryCircuitCreateCircuit;

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
SdcaXu_SetPowerPolicy(
    _In_ WDFDEVICE      Device
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddRenders(
    _In_ WDFDEVICE Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG CircuitConfig
);

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddCaptures(
    _In_ WDFDEVICE Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG CircuitConfig
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetHwConfig
(
    _In_        PVOID                   Context,
    _In_        SDCAXU_HW_CONFIG_TYPE   HwConfigType,
    _In_opt_    PVOID                   HwConfigData,
    _In_        ULONG                   HwConfigDataSize
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetEndpointConfig
(
    _In_        PVOID                           Context,
    _In_        SDCAXU_ENDPOINT_CONFIG_TYPE     EndpointConfigType,
    _In_opt_    PVOID                           EndpointConfigData,
    _In_        ULONG                           EndpointConfigDataSize
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_SDCAInterruptHandler
(
    _In_ PVOID  Context,
    _In_ ULONG  SDCAInterruptBit
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_SCPInterruptHandler
(
    _In_ PVOID  Context,
    _In_ ULONG  SCPInterruptBit
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_DataPortInterruptHandler
(
    _In_ PVOID  Context,
    _In_ ULONG  DataPort,
    _In_ ULONG  DataPortInterruptBit
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_ChangeNotification
(
    _In_        PVOID                       Context,
    _In_        SDCAXU_NOTIFICATION_TYPE    NotificationType,
    _In_opt_    PVOID                       NotificationData,
    _In_        ULONG                       NotificationDataSize
);

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetXUEntities(_In_ WDFDEVICE Device);

PAGED_CODE_SEG
NTSTATUS SdcaXu_RegisterForInterrupts(_In_ WDFDEVICE Device);

PAGED_CODE_SEG
NTSTATUS SdcaXu_SetJackOverride(_In_ WDFDEVICE Device);

EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST EvtSDCAVXuProcessQueryInterfaceRequest;

PAGED_CODE_SEG
NTSTATUS
SdcaXu_ConfigureTestInterface(
    _In_    WDFDEVICE   Device,
    _In_    PCGUID      Interface
);

#endif // _PRIVATE_H_

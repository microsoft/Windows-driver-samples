/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    Sysvad.h

Abstract:

    Header file for common stuff.

--*/

#ifndef _SYSVAD_H_
#define _SYSVAD_H_

#include <portcls.h>
#include <stdunk.h>
#include <ksdebug.h>
#include <ntintsafe.h>
#include <wdf.h>
#include <wdfminiport.h>
#include <MsApoFxProxy.h>
#include <Ntstrsafe.h>

//=============================================================================
// Defines
//=============================================================================

// Version number. Revision numbers are specified for each sample.
#define SYSVAD_VERSION               1

// Revision number.
#define SYSVAD_REVISION              0

// Product Id
// {5B722BF8-F0AB-47ee-B9C8-8D61D31375A1}
#define STATIC_PID_SYSVAD\
    0x5b722bf8, 0xf0ab, 0x47ee, 0xb9, 0xc8, 0x8d, 0x61, 0xd3, 0x13, 0x75, 0xa1
DEFINE_GUIDSTRUCT("5B722BF8-F0AB-47ee-B9C8-8D61D31375A1", PID_SYSVAD);
#define PID_SYSVAD DEFINE_GUIDNAMED(PID_SYSVAD)

// Pool tag used for SYSVAD allocations
#define SYSVAD_POOLTAG               'DVSM'  

// Debug module name
#define STR_MODULENAME              "SYSVAD: "

// Debug utility macros
#define D_FUNC                      4
#define D_BLAB                      DEBUGLVL_BLAB
#define D_VERBOSE                   DEBUGLVL_VERBOSE        
#define D_TERSE                     DEBUGLVL_TERSE          
#define D_ERROR                     DEBUGLVL_ERROR          
#define DPF                         _DbgPrintF
#define DPF_ENTER(x)                DPF(D_FUNC, x)

// Channel orientation
#define CHAN_LEFT                   0
#define CHAN_RIGHT                  1
#define CHAN_MASTER                 (-1)

// Dma Settings.
#define DMA_BUFFER_SIZE             0x16000

#define KSPROPERTY_TYPE_ALL         KSPROPERTY_TYPE_BASICSUPPORT | \
                                    KSPROPERTY_TYPE_GET | \
                                    KSPROPERTY_TYPE_SET
                                    
// Specific node numbers
#define DEV_SPECIFIC_VT_BOOL        9
#define DEV_SPECIFIC_VT_I4          10
#define DEV_SPECIFIC_VT_UI4         11

#define _100NS_PER_MILLISECOND      10000                       // number of 100ns units per millisecond

// Default volume settings.
#define VOLUME_STEPPING_DELTA       0x8000
#define VOLUME_SIGNED_MAXIMUM       0x00000000
#define VOLUME_SIGNED_MINIMUM       (-96 * 0x10000)

// Default peak meter settings
#define PEAKMETER_STEPPING_DELTA    0x1000
#define PEAKMETER_SIGNED_MAXIMUM    LONG_MAX
#define PEAKMETER_SIGNED_MINIMUM    LONG_MIN

#define VALUE_NORMALIZE_P(v, step) \
    ((((v) + (step)/2) / (step)) * (step))

#define VALUE_NORMALIZE(v, step) \
    ((v) > 0 ? VALUE_NORMALIZE_P((v), (step)) : -(VALUE_NORMALIZE_P(-(v), (step))))
    
#define VALUE_NORMALIZE_IN_RANGE_EX(v, min, max, step) \
    ((v) > (max) ? (max) : \
     (v) < (min) ? (min) : \
     VALUE_NORMALIZE((v), (step)))

// to normalize volume values.
#define VOLUME_NORMALIZE_IN_RANGE(v) \
    VALUE_NORMALIZE_IN_RANGE_EX((v), VOLUME_SIGNED_MINIMUM, VOLUME_SIGNED_MAXIMUM, VOLUME_STEPPING_DELTA)

// to normalize sample peak meter.
#define PEAKMETER_NORMALIZE_IN_RANGE(v) \
    VALUE_NORMALIZE_IN_RANGE_EX((v), PEAKMETER_SIGNED_MINIMUM, PEAKMETER_SIGNED_MAXIMUM, PEAKMETER_STEPPING_DELTA)

#define ALL_CHANNELS_ID             UINT32_MAX

// Flags to identify stream processing mode
typedef enum {
    SIGNALPROCESSINGMODE_NONE           = 0x00,
    SIGNALPROCESSINGMODE_DEFAULT        = 0x01,
    SIGNALPROCESSINGMODE_RAW            = 0x02,
    SIGNALPROCESSINGMODE_COMMUNICATIONS = 0x04,
    SIGNALPROCESSINGMODE_SPEECH         = 0x08,
    SIGNALPROCESSINGMODE_NOTIFICATION   = 0x10,
    SIGNALPROCESSINGMODE_MEDIA          = 0x20,
    SIGNALPROCESSINGMODE_MOVIE          = 0x40
} SIGNALPROCESSINGMODE;

#define MAP_GUID_TO_MODE(guid, mode)                                                  \
    if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_DEFAULT))                        \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_DEFAULT;                                          \
    }                                                                                 \
    else if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_RAW))                       \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_RAW;                                              \
    }                                                                                 \
    else if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_COMMUNICATIONS))            \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_COMMUNICATIONS;                                   \
    }                                                                                 \
    else if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_SPEECH))                    \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_SPEECH;                                           \
    }                                                                                 \
    else if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_NOTIFICATION))              \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_NOTIFICATION;                                     \
    }                                                                                 \
    else if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_MEDIA))                     \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_MEDIA;                                            \
    }                                                                                 \
    else if (IsEqualGUID(guid, AUDIO_SIGNALPROCESSINGMODE_MOVIE))                     \
    {                                                                                 \
        mode = SIGNALPROCESSINGMODE_MOVIE;                                            \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
        ASSERT(FALSE && "Unknown Signal Processing Mode");                            \
        mode = SIGNALPROCESSINGMODE_NONE;                                             \
    }

#define VERIFY_MODE_RESOURCES_AVAILABLE(modes, guid, status)                          \
    {                                                                                 \
        SIGNALPROCESSINGMODE mode = SIGNALPROCESSINGMODE_NONE;                        \
        MAP_GUID_TO_MODE(guid, mode);                                                 \
        if (SIGNALPROCESSINGMODE_NONE != mode)                                        \
        {                                                                             \
            status = (modes & mode) ? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS; \
        }                                                                             \
        else                                                                          \
        {                                                                             \
            status = STATUS_INVALID_PARAMETER;                                        \
        }                                                                             \
    }

#define ALLOCATE_MODE_RESOURCES(modes, guid)                                          \
    {                                                                                 \
        SIGNALPROCESSINGMODE mode = SIGNALPROCESSINGMODE_NONE;                        \
        MAP_GUID_TO_MODE(guid, mode);                                                 \
        modes |= mode;                                                                \
    }

#define FREE_MODE_RESOURCES(modes, guid)                                              \
    {                                                                                 \
        SIGNALPROCESSINGMODE mode = SIGNALPROCESSINGMODE_NONE;                        \
        MAP_GUID_TO_MODE(guid, mode);                                                 \
        modes &= (~mode);                                                             \
    }

// Define the value data type for supported sound detector patterns. Only
// one pattern type is supported in this sample.
typedef struct {
    KSMULTIPLE_ITEM MultipleItem;
    GUID            PatternType[1];
} CONTOSO_SUPPORTEDPATTERNSVALUE;

//=============================================================================
// Typedefs
//=============================================================================

// Flags to identify stream processing mode
typedef enum {
    CONNECTIONTYPE_TOPOLOGY_OUTPUT = 0,
    CONNECTIONTYPE_WAVE_OUTPUT     = 1
} CONNECTIONTYPE;

// Connection table for registering topology/wave bridge connection
typedef struct _PHYSICALCONNECTIONTABLE
{
    ULONG            ulTopology;
    ULONG            ulWave;
    CONNECTIONTYPE   eType;
} PHYSICALCONNECTIONTABLE, *PPHYSICALCONNECTIONTABLE;

//
// This is the structure of the portclass FDO device extension Nt has created
// for us.  We keep the adapter common object here.
//
struct IAdapterCommon;
typedef struct _PortClassDeviceContext              // 32       64      Byte offsets for 32 and 64 bit architectures
{
    ULONG_PTR m_pulReserved1[2];                    // 0-7      0-15    First two pointers are reserved.
    PDEVICE_OBJECT m_DoNotUsePhysicalDeviceObject;  // 8-11     16-23   Reserved pointer to our Physical Device Object (PDO).
    PVOID m_pvReserved2;                            // 12-15    24-31   Reserved pointer to our Start Device function.
    PVOID m_pvReserved3;                            // 16-19    32-39   "Out Memory" according to DDK.  
    IAdapterCommon* m_pCommon;                      // 20-23    40-47   Pointer to our adapter common object.
#ifdef _USE_SingleComponentMultiFxStates
    POHANDLE m_poHandle;                            // 24-27    48-55   PoFxDevice handle.
#else
    PVOID m_pvUnused1;                              // 24-27    48-55   Unused space.
#endif // _USE_SingleComponentMultiFxStates
    PVOID m_pvUnused2;                              // 28-31    56-63   Unused space.

    // Anything after above line should not be used.
    // This actually goes on for (64*sizeof(ULONG_PTR)) but it is all opaque.
} PortClassDeviceContext;

//
// Global settings.
//
extern DWORD g_DoNotCreateDataFiles;
extern DWORD g_DisableBthScoBypass;

//=============================================================================
// Function prototypes
//=============================================================================

// Generic topology handler
NTSTATUS PropertyHandler_Topology
( 
    _In_  PPCPROPERTY_REQUEST PropertyRequest 
);

// Default WaveFilter automation table.
// Handles the GeneralComponentId request.
NTSTATUS PropertyHandler_WaveFilter
(
    _In_ PPCPROPERTY_REQUEST PropertyRequest
);

NTSTATUS PropertyHandler_OffloadPin
(
    _In_ PPCPROPERTY_REQUEST PropertyRequest
);

// common.h uses some of the above definitions.
#include "common.h"
#include "kshelper.h"

#endif



/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2015, Microsoft Corporation.

    File:

        Common.h

    Abstract:

        Common project header.

    History:

        created 02/18/2015

**************************************************************************/

#pragma once

#define _NO_SYS_GUID_OPERATOR_EQ_

#ifdef NTDDI_VERSION
#undef NTDDI_VERSION
#endif //NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_VB

#include <stdio.h>
#include <stdlib.h>

#include <wdm.h>
#include <windef.h>
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <ntstrsafe.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <acpitabl.h>

#include "MetadataInternal.h"
#include "CustomProperties.h"
#include "Dbg.h"
#include "CameraSimulatorCommon.h"

#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_

/*************************************************

    Add definitions that are missing for C++14.

*************************************************/

PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
             "Allocation failures cause a system crash"))
    POOL_TYPE       poolType
);

PVOID operator new
(
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
             "Allocation failures cause a system crash"))
    POOL_TYPE       poolType,
    ULONG           tag
);

/*++

Routine Description:

    Array new() operator for creating objects with a specified allocation tag.

Arguments:

    iSize -
        The size of the entire allocation.

    poolType -
        The type of allocation.  Ex: PagedPool or NonPagedPoolNx

    tag -
        A 4-byte allocation identifier.

Return Value:

    None

--*/
PVOID 
operator new[](
    size_t          iSize,
    _When_((poolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
            "Allocation failures cause a system crash"))
    POOL_TYPE       poolType,
    ULONG           tag
);

/*++

Routine Description:

    Array delete() operator.

Arguments:

    pVoid -
        The memory to free.

Return Value:

    None

--*/
void 
__cdecl 
operator delete[](
    PVOID pVoid
);

/*++

Routine Description:

    Sized delete() operator.

Arguments:

    pVoid -
        The memory to free.

    size -
        The size of the memory to free.

Return Value:

    None

--*/
void __cdecl operator delete
(
    void *pVoid,
    size_t /*size*/
);

/*++

Routine Description:

    Sized delete[]() operator.

Arguments:

    pVoid -
        The memory to free.

    size -
        The size of the memory to free.

Return Value:

    None

--*/
void __cdecl operator delete[]
(
    void *pVoid,
    size_t /*size*/
);

#endif // _NEW_DELETE_OPERATORS_


/*************************************************

    Misc Definitions

*************************************************/
#define STR_MODULENAME "AvsCamera: "

//
// CAPTURE_PIN_DATA_RANGE_COUNT:(formats)
//
// The number of ranges supported on the capture pin.
//
#define IMAGE_CAPTURE_PIN_DATA_RANGE_COUNT 2
#define VIDEO_CAPTURE_PIN_DATA_RANGE_COUNT 32
#define VIDEO_PREVIEW_PIN_DATA_RANGE_COUNT 16

//
// CAPTURE_FILTER_CATEGORIES_COUNT:
//
// The number of categories for the capture filter.
//
#define CAPTURE_FILTER_CATEGORIES_COUNT 3

/*************************************************

    Externed information

*************************************************/
//
// filter.cpp externs:
//
extern
const
KSFILTER_DISPATCH
AvsCameraFilterDispatch;

extern
const
KSFILTER_DESCRIPTOR
AvsCameraFilterDescriptor;

extern
const
KSFILTER_DESCRIPTOR
AvsCameraFilterDescriptorFFC;

extern
const
GUID
AvsCameraFilterCategories [CAPTURE_FILTER_CATEGORIES_COUNT];

//
// capture.cpp externs:
//
extern
const
KSALLOCATOR_FRAMING_EX
VideoCapturePinAllocatorFraming;

extern
const
KSALLOCATOR_FRAMING_EX
ImageCapturePinAllocatorFraming;

extern
const
KSPIN_DISPATCH
VideoCapturePinDispatch;

extern
const
KSPIN_DISPATCH
ImageCapturePinDispatch;

/*************************************************

    Internal Includes

*************************************************/

#include "util.h"
#include "NonCopyable.h"
#include "ref.h"
#include "Mutex.h"
#include "Waitable.h"
#include "WorkItem.h"
#include "Timer.h"
#include "ExtendedProperty.h"
#include "ExtendedVidProcSetting.h"
#include "ExtendedEvComp.h"
#include "ExtendedMetadata.h"
#include "ExtendedPhotoMode.h"
#include "ExtendedMaxVideoFpsForPhotoRes.h"
#include "ExtendedFieldOfView.h"
#include "ExtendedCameraAngleOffset.h"
#include "CameraProfile.h"
#include "Synthesizer.h"
#include "XRGBSynthesizer.h"
#include "RGB24Synthesizer.h"
#include "YUVSynthesizer.h"
#include "YUY2Synthesizer.h"
#include "NV12Synthesizer.h"
#include "hwsim.h"
#include "ImageHwSim.h"
#include "PreviewHwSim.h"
#include "VideoHwSim.h"
#include "device.h"
#include "AvsCameraDevice.h"
#include "Roi.h"
#include "Sensor.h"
#include "SensorSimulation.h"
#include "filter.h"
#include "AvsCameraFilter.h"
#include "capture.h"
#include "videocapture.h"
#include "imagecapture.h"
#include "formats.h"
#include <uuids.h>
#include <devpkey.h>

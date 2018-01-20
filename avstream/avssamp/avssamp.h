/**************************************************************************

    AVStream Filter-Centric Sample

    Copyright (c) 2001, Microsoft Corporation

    File:

        avssamp.h

    Abstract:

        AVStream Filter-Centric Sample header file.  This is the main
        header.

    History:

        created 6/18/01

**************************************************************************/

/*************************************************

    Standard Includes

*************************************************/

extern "C" {
#include <wdm.h>
}

#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntstrsafe.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#pragma warning (disable : 4100 4101 4131 4127 4189 4701 4706)
/*************************************************

    Misc Definitions

*************************************************/

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                \
        ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#define FOURCC_YUV422       mmioFOURCC('U', 'Y', 'V', 'Y')

//
// CAPTURE_PIN_DATA_RANGE_COUNT:
//
// The number of ranges supported on the capture pin.
//
#define CAPTURE_PIN_DATA_RANGE_COUNT 2

//
// CAPTURE_FILTER_PIN_COUNT:
//
// The number of pins on the capture filter.
//
#define CAPTURE_FILTER_PIN_COUNT 1

//
// CAPTURE_FILTER_CATEGORIES_COUNT:
//
// The number of categories for the capture filter.
//
#define CAPTURE_FILTER_CATEGORIES_COUNT 2

#define AVSSMP_POOLTAG 'sSVA'

/*************************************************

    Externed information

*************************************************/

//
// filter.cpp externs:
//
extern
const
KSFILTER_DISPATCH
CaptureFilterDispatch;

extern
const
KSFILTER_DESCRIPTOR
CaptureFilterDescriptor;

extern
const
KSPIN_DESCRIPTOR_EX
CaptureFilterPinDescriptors [CAPTURE_FILTER_PIN_COUNT];

extern
const
GUID
CaptureFilterCategories [CAPTURE_FILTER_CATEGORIES_COUNT];

//
// video.cpp externs:
//
extern
const
KSALLOCATOR_FRAMING_EX
VideoCapturePinAllocatorFraming;

extern
const
KSPIN_DISPATCH
VideoCapturePinDispatch;

extern
const
PKSDATARANGE
VideoCapturePinDataRanges [CAPTURE_PIN_DATA_RANGE_COUNT];

//
// audio.cpp externs:
//
extern
const
KSPIN_DESCRIPTOR_EX
AudioPinDescriptorTemplate;

//
// avssamp.cpp externs:
//
extern
const
KSDEVICE_DESCRIPTOR
CaptureDeviceDescriptor;

#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_

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

    Internal Includes

*************************************************/

#include "image.h"
#include "wave.h"
#include "filter.h"
#include "capture.h"
#include "video.h"
#include "audio.h"


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

#include <stdio.h>
#include <stdlib.h>

#include <wdm.h>
#include <windef.h>
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <kcom.h>
#include <stdunk.h>
#include <ntstrsafe.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <acpitabl.h>

#include "MetadataInternal.h"
#include "CustomProperties.h"
#include "Dbg.h"

/*************************************************

    Add definitions that are missing for C++14.

*************************************************/

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
inline 
PVOID 
operator new[](
	size_t          iSize,
	_When_((poolType & NonPagedPoolMustSucceed) != 0,
		__drv_reportError("Must succeed pool allocations are forbidden. "
			"Allocation failures cause a system crash"))
	POOL_TYPE       poolType,
	ULONG           tag
)
{
	PVOID result = ExAllocatePoolWithTag(poolType, iSize, tag);

	if (result)
	{
		RtlZeroMemory(result, iSize);
	}

	return result;
}

/*++

Routine Description:

    Array delete() operator.

Arguments:

    pVoid -
        The memory to free.

Return Value:

    None

--*/
inline 
void 
__cdecl 
operator delete[](
	PVOID pVoid
)
{
	if (pVoid)
	{
		ExFreePool(pVoid);
	}
}

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
inline void __cdecl operator delete
(
	void *pVoid,
	size_t /*size*/
)
{
	if (pVoid)
	{
		ExFreePool(pVoid);
	}
}

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
inline void __cdecl operator delete[]
(
	void *pVoid,
	size_t /*size*/
)
{
	if (pVoid)
	{
		ExFreePool(pVoid);
	}
}

/*************************************************

    Misc Definitions

*************************************************/
#pragma warning (disable : 4100 4127 4131 4189 4701 4706)
#define STR_MODULENAME "AvsCamera: "

#define IFFAILED_EXIT(status)                   \
    if (!NT_SUCCESS( Status = (status) ))       \
    {                                           \
        goto done;                              \
    }

#define IFNULL_EXIT(exp)                        \
    if ( (exp) == nullptr )                     \
    {                                           \
        Status = STATUS_INSUFFICIENT_RESOURCES; \
        goto done;                              \
    }

//
//  Number of 100ns in one second
//
#define ONESECOND   10000000

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                \
        ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#define FOURCC_YUY2         mmioFOURCC('Y', 'U', 'Y', '2')
#define FOURCC_NV12         mmioFOURCC('N', 'V', '1', '2')

//Number of filters: FFC, RFC
#define CAPTURE_FILTER_COUNT 2

//filter index
#define CAPTURE_FILTER_RFC 0
#define CAPTURE_FILTER_FFC 1

//Arbitrary Maximum faces
#define MAX_FACES       15

//
// CAPTURE_PIN_DATA_RANGE_COUNT:(formats)
//
// The number of ranges supported on the capture pin.
//
#define IMAGE_CAPTURE_PIN_DATA_RANGE_COUNT 2
#define VIDEO_CAPTURE_PIN_DATA_RANGE_COUNT 30
#define VIDEO_PREVIEW_PIN_DATA_RANGE_COUNT 15

//
// CAPTURE_FILTER_PIN_COUNT:
//
// The number of pins on the capture filter.
//
#define CAPTURE_FILTER_VIDEO_PIN_COUNT 2
#define CAPTURE_FILTER_IMAGE_PIN_COUNT 1
#define CAPTURE_FILTER_PIN_COUNT (CAPTURE_FILTER_VIDEO_PIN_COUNT+CAPTURE_FILTER_IMAGE_PIN_COUNT)


#define CAPTURE_FILTER_VIDEO_PIN   0
#define CAPTURE_FILTER_PREVIEW_PIN 1
#define CAPTURE_FILTER_STILL_PIN   2

//
//  Define the number of frames needed in the queue for each pin.
//
#define IMAGE_CAPTURE_PIN_MINIMUM_FRAMES    3
#define VIDEO_CAPTURE_PIN_MINIMUM_FRAMES    4
#define PREVIEW_PIN_MINIMUM_FRAMES          6
#define IMAGE_CAPTURE_PIN_MAXIMUM_FRAMES    20
#define IMAGE_CAPTURE_PIN_MAXIMUM_HISTORY_FRAMES    10

//
// CAPTURE_FILTER_CATEGORIES_COUNT:
//
// The number of categories for the capture filter.
//
#define CAPTURE_FILTER_CATEGORIES_COUNT 3

#define AVSHWS_POOLTAG 'hSVA'

//
//  Focal length definitions
//

//  These are the definitions found in MSDN for the legacy zoom controls.
//
//#define FOCALLENGTH_OCULAR  50
//#define FOCALLENGTH_OPTICAL_MIN 10
//#define FOCALLENGTH_OPTICAL_MAX 600

//#define FOCALLENGTH_OCULAR  50
//#define FOCALLENGTH_OPTICAL_MIN 10
//#define FOCALLENGTH_OPTICAL_MAX 600

//  Note: Despite what MSDN says, they are NOT limits as far as the
//        pipeline is conerned.  You should supply values here that are
//        appropriate to your hardware - whatever that may be.
//
//        Also note that since both optical and ocular focal length are
//        nomally measured in millimeters, and since the optical length is
//        always the numerator and ocular is used as the denominator, the
//        units (millimeters) cancel out.  This means you can stick any set
//        of values you like here that can be used to represent the min and
//        maximum zoom value of your hardware as a simple rational number.
//
//        Again, the rational number is: (Objective length / Ocular length)
//
//  We use the following definitions in this simulation in order to have the
//  values produced and used in the legacy control be the same as the ones
//  in the new extended zoom control.
//
#define FOCALLENGTH_OCULAR      (1<<16)     // Denominator of 2^16 makes this the same as Q16 format in extended.
#define FOCALLENGTH_OPTICAL_MIN (FOCALLENGTH_OCULAR/16) // Allow a wide-field view of 1/16th.
#define FOCALLENGTH_OPTICAL_MAX (FOCALLENGTH_OCULAR*32) // Allow a zoom-in view of 32x.

//  Values for Power Line Frequency
#define POWERLINEFREQ_DISABLED  0
#define POWERLINEFREQ_50HZ      1
#define POWERLINEFREQ_60HZ      2
#define POWERLINEFREQ_DEFAULT   POWERLINEFREQ_DISABLED


//
//  Q-format definitions.
//
#define BASE_Q(n)       (((ULONGLONG)1)<<n)
#define TO_Q31(_x_)     (LONGLONG)( (_x_) * BASE_Q(31) )
#define TO_Q16(_x_)     (LONGLONG)( (_x_) * BASE_Q(16) )
#define FROM_Q16(_x_, _denominator_)   ((LONG)( (LONGLONG)(_x_) / ( TO_Q16(1) / (LONGLONG)(_denominator_)) ))

#define MULT_Q16(_x_, _y_) (LONGLONG)((((LONGLONG)(_x_) * (LONGLONG)(_y_)) + (BASE_Q(15)))>>16)
#define DIV_Q16(_x_, _y_)  (LONGLONG)((((LONGLONG)(_x_)<<16)+(_y_/2))/(LONGLONG)(_y_))

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))

//
//  Copy of histogram channel bitmask definitions from mfapi.h (not available in wdk).
//
#define MF_HISTOGRAM_CHANNEL_Y      0x00000001
#define MF_HISTOGRAM_CHANNEL_R      0x00000002
#define MF_HISTOGRAM_CHANNEL_G      0x00000004
#define MF_HISTOGRAM_CHANNEL_B      0x00000008
#define MF_HISTOGRAM_CHANNEL_Cb     0x00000010
#define MF_HISTOGRAM_CHANNEL_Cr     0x00000020

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
KSPIN_DESCRIPTOR_EX
AvsCameraFilterPinDescriptors [CAPTURE_FILTER_PIN_COUNT];

extern
const
KSPIN_DESCRIPTOR_EX
AvsCameraFilterPinDescriptorsFFC [CAPTURE_FILTER_PIN_COUNT];

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

extern
const
PKSDATARANGE
VideoCapturePinDataRanges [VIDEO_CAPTURE_PIN_DATA_RANGE_COUNT];

extern
const
PKSDATARANGE
VideoPreviewPinDataRanges[VIDEO_PREVIEW_PIN_DATA_RANGE_COUNT];

extern
const
PKSDATARANGE
ImageCapturePinDataRanges[IMAGE_CAPTURE_PIN_DATA_RANGE_COUNT];

/*************************************************

    Enums / Typedefs

*************************************************/

typedef enum _PIN_STATE
{

    PinStopped = 0,
    PinPaused,
    PinRunning

} PIN_STATE, *PPIN_STATE;

typedef enum _PIN_MODE
{

    PinNormalMode = 0,
    PinBurstMode

} PIN_MODE, *PPIN_MODE;


/*************************************************

    Class Definitions

*************************************************/

struct ISP_FRAME_SETTINGS
{
    ULONGLONG   ISOMode;
    ULONG       ISOValue;       // for Manual ISO settings.
    KSCAMERA_EXTENDEDPROP_EVCOMPENSATION EVCompensation;
    ULONGLONG WhiteBalanceMode;
    KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING WhiteBalanceSetting;
    ULONGLONG ExposureMode;
    KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING ExposureSetting;
    ULONGLONG FocusMode;
    KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING FocusSetting;
    ULONGLONG FocusPriority;
    ULONGLONG FlashMode;
    ULONG FlashValue;       //  Adjustable power setting (0-100)
    BOOLEAN bPhotoConfirmation;
} ;

struct SOC_CAP_WITH_STEPPING
{
    KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER    Hdr;
    KSPROPERTY_STEPPING_LONG                    Stepping;
} ;

struct SOC_CAP_WITH_STEPPING_LONGLONG
{
    KSCAMERA_PERFRAMESETTING_CAP_ITEM_HEADER    Hdr;
    KSPROPERTY_STEPPING_LONGLONG                Stepping;
} ;

//
//  ICapturePin:
//
//  This is a capture pin interface.  The device level calls back the
//  CompleteMapping method passing the number of completed mappings for
//  the capture pin.
//
class ICapturePin
{

public:

    virtual
    NTSTATUS
    CompleteMapping(
        _In_ PKSSTREAM_POINTER Clone=nullptr
    ) = 0;

};

class LockDevice
{
public:

    LockDevice(PKSDEVICE device) : m_device(device)
    {
        NT_ASSERT(m_device);
        KsAcquireDevice(m_device);
    }

    ~LockDevice()
    {
        KsReleaseDevice(m_device);
    }
private:
    PKSDEVICE m_device;
};

class LockFilter
{
public:

    LockFilter(PKSFILTER filter) : m_filter(filter)
    {
        NT_ASSERT(m_filter);
        KsFilterAcquireControl(m_filter);
    }

    ~LockFilter()
    {
        KsFilterReleaseControl(m_filter);
    }
private:
    PKSFILTER m_filter;
};

//  The following class definitions and ctors are used solely
//  to simplify initialization of the parent metadata structures.
//  They are not here for any other purpose.

template <class T, class U>
class CMetaRational : public U
{
public:
    CMetaRational( T Num=1, T Denom=1 )
    {
        Set=TRUE;
        Numerator = Num;
        Denominator = Denom;
    }
};

typedef CMetaRational<UINT32, METADATA_RATIONAL> CMetadataRational;
typedef CMetaRational<INT32, METADATA_SRATIONAL> CMetadataSRational;

template <class T, class U>
class CMetaPrimative : public U
{
public:
    CMetaPrimative( T n )
    {
        Set = TRUE;
        Value = n;
    }
};

typedef CMetaPrimative<UINT16, METADATA_UINT16> CMetadataShort;
typedef CMetaPrimative<UINT32, METADATA_UINT32> CMetadataLong;
typedef CMetaPrimative<INT64,  METADATA_INT64>  CMetadataLongLong;
typedef CMetaPrimative<UINT64, METADATA_UINT64> CMetadataULongLong;

class CMetadataShortString : public METADATA_SHORTSTRING
{
public:
    CMetadataShortString(
        _In_z_ const CHAR *str
    )
    {
        Length = (UINT32) min(strlen(str), sizeof(String));
        strncpy_s( String, str, Length );
    }
};

class CMetadataEVCompensation : public METADATA_EVCOMP
{
public:
    CMetadataEVCompensation(
        _In_    UINT64 flags,
        _In_    UINT32 value
    )
    {
        Set = TRUE;
        Flags = flags;
        Value = value;
    }
};

typedef struct
{
    LONG Value;
    ULONG Flags;
    ULONG Capabilities;
} VIDCAP_PROPERTY, *PVIDCAP_PROPERTY;

typedef struct
{
    LONG       lOcularFocalLength;
    LONG       lObjectiveFocalLengthMin;
    LONG       lObjectiveFocalLengthMax;
} FOCAL_LENGTH_PROPERTY, *PFOCAL_LENGTH_PROPERTY;

const ULONG KSCAMERA_EXTENDEDPROP_VERSION = 1;

//
//  Note: I cannot simplify this down to just the FrameIndex.
//  Past frames and non-VPS photos can have a photo confirmation set
//  and those images will not have an index associated with them.
//
class PHOTOCONFIRMATION_INFO
{
public:
    PHOTOCONFIRMATION_INFO(
        _In_    ULONG       FrameIndex,
        _In_    LONGLONG    Time
    )
        : m_Index( FrameIndex )
        , m_Time(Time)
        , m_bRequired(TRUE)
    {}

    PHOTOCONFIRMATION_INFO()
        : m_Index(0)
        , m_Time(0)
        , m_bRequired( FALSE )
    {}

    BOOL
    isRequired()
    {
        return m_bRequired;
    }

    ULONG
    getIndex()
    {
        return m_Index;
    }

    LONGLONG
    getTime()
    {
        return m_Time;
    }

private:
    BOOL        m_bRequired;
    ULONG       m_Index;
    LONGLONG    m_Time;
};

/*************************************************

    Global Functions

*************************************************/

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
#include <uuids.h>
#include <devpkey.h>

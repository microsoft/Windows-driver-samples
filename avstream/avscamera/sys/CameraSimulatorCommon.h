/**************************************************************************

A/V Stream Camera Sample

Copyright (c) 2017, Microsoft Corporation.

File:

CameraSimulatorCommon.h

Abstract:

Common header applicable for all camera simulators.

History:

created 06/05/2017

**************************************************************************/

#pragma once
#pragma warning (disable : 4100 4127 4131 4189 4701 4706)

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

#define AVSCAM_SIZE_VIDEOHEADER2(pbmi) ((pbmi)->bmiHeader.biSize + FIELD_OFFSET(KS_VIDEOINFOHEADER2,bmiHeader))

// RGB24
#define STATIC_KSDATAFORMAT_SUBTYPE_RGB24\
    0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70
DEFINE_GUIDSTRUCT("e436eb7d-524f-11ce-9f53-0020af0ba770", KSDATAFORMAT_SUBTYPE_RGB24);
#define KSDATAFORMAT_SUBTYPE_RGB24 DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_RGB24)

// RGB32
#define STATIC_KSDATAFORMAT_SUBTYPE_RGB32\
    0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70
DEFINE_GUIDSTRUCT("e436eb7e-524f-11ce-9f53-0020af0ba770", KSDATAFORMAT_SUBTYPE_RGB32);
#define KSDATAFORMAT_SUBTYPE_RGB32 DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_RGB32)

// NV12
#define STATIC_KSDATAFORMAT_SUBTYPE_NV12\
    0x3231564E, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("3231564E-0000-0010-8000-00AA00389B71", KSDATAFORMAT_SUBTYPE_NV12);
#define KSDATAFORMAT_SUBTYPE_NV12 DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_NV12)

// YUY2
#define STATIC_KSDATAFORMAT_SUBTYPE_YUY2\
    0x32595559, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("32595559-0000-0010-8000-00AA00389B71", KSDATAFORMAT_SUBTYPE_YUY2);
#define KSDATAFORMAT_SUBTYPE_YUY2 DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_YUY2)

//Arbitrary Maximum faces
#define MAX_FACES       15

//
//  Define the number of frames needed in the queue for image capture.
//
#define IMAGE_CAPTURE_PIN_MINIMUM_FRAMES    3
#define IMAGE_CAPTURE_PIN_MAXIMUM_FRAMES    20
#define IMAGE_CAPTURE_PIN_MAXIMUM_HISTORY_FRAMES    10

#define AVSHWS_POOLTAG 'hSVA'

//
//  Focal length definitions
//
//  These are the definitions found in MSDN for the legacy zoom controls.
//
//#define FOCALLENGTH_OCULAR  50
//#define FOCALLENGTH_OPTICAL_MIN 10
//#define FOCALLENGTH_OPTICAL_MAX 600
//
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

//
//  Values for Power Line Frequency
//
#define POWERLINEFREQ_DISABLED  0
#define POWERLINEFREQ_50HZ      1
#define POWERLINEFREQ_60HZ      2
#define POWERLINEFREQ_AUTO      3
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
#define MAKEDWORD(l, h)     ( ( DWORD ) ( ( ( WORD ) ( l ) )  | ( ( DWORD ) ( ( WORD ) ( h ) ) ) << 16 ) )
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

    Struct / Class Definitions

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
            _In_opt_ PKSSTREAM_POINTER Clone=nullptr
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
        isRequired() const
    {
        return m_bRequired;
    }

    ULONG
        getIndex() const
    {
        return m_Index;
    }

    LONGLONG
        getTime() const
    {
        return m_Time;
    }

private:
    BOOL        m_bRequired;
    ULONG       m_Index;
    LONGLONG    m_Time;
};

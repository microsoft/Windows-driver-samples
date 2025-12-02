/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        MetadataInternal.h

    Abstract:

        Internal structures for private metadata communication between the 
        driver and its MFT0.

    History:

        created 4/29/2013

**************************************************************************/
#pragma once
#if (NTDDI_VERSION >= NTDDI_WINBLUE)

enum MetadataId_Custom
{
    MetadataId_Custom_ImageAggregation = MetadataId_Custom_Start,
    MetadataId_Custom_FaceDetection,
    MetadataId_Custom_PreviewAggregation,
    MetadataId_Custom_Histogram
};

typedef struct
{
    UINT32  Set;
    UINT32  Value;
} METADATA_UINT32, METADATA_LONG;

typedef struct
{
    UINT32  Set;
    UINT16  Value;
    UINT16  Reserved;
} METADATA_UINT16, METADATA_SHORT;

typedef struct
{
    UINT32  Length;     // Use a length of 0 for not Set
    CHAR    String[32];
} METADATA_SHORTSTRING;

typedef struct
{
    UINT32  Set;
    UINT32  Reserved;
    UINT32  Numerator;
    UINT32  Denominator;
} METADATA_RATIONAL;

typedef struct
{
    UINT32  Set;
    UINT32  Reserved;
    INT64   Value;
} METADATA_INT64;

typedef struct
{
    UINT32  Set;
    UINT32  Reserved;
    UINT64  Value;
} METADATA_UINT64;

typedef struct
{
    UINT32  Set;
    UINT32  Reserved;
    INT32   Numerator;
    INT32   Denominator;
} METADATA_SRATIONAL;

typedef struct
{
    UINT32  Set;
    INT32   Value;
    UINT64  Flags;
} METADATA_EVCOMP;

#ifndef _WDM_INCLUDED_
//
//  Time conversion routines
//
typedef short CSHORT;

typedef struct _TIME_FIELDS
{
    CSHORT Year;        // range [1601...]
    CSHORT Month;       // range [1..12]
    CSHORT Day;         // range [1..31]
    CSHORT Hour;        // range [0..23]
    CSHORT Minute;      // range [0..59]
    CSHORT Second;      // range [0..59]
    CSHORT Milliseconds;// range [0..999]
    CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

#endif  //TIME_FIELDS

typedef struct
{
    UINT32      Set;
    UINT32      Reserved;
    TIME_FIELDS Time;
} METADATA_TIMEFIELDS;

typedef struct _METADATA_PREVIEWAGGREGATION
{
    //  Mandatory fields
    METADATA_UINT32                         FocusState;
    METADATA_INT64                          ExposureTime;
    METADATA_EVCOMP                         EVCompensation;
    METADATA_UINT32                         ISOSpeed;
    METADATA_UINT32                         LensPosition;
    METADATA_UINT32                         FlashOn;
    METADATA_UINT32                         WhiteBalanceMode;
    METADATA_SRATIONAL                      IsoAnalogGain;      //  MF_CAPTURE_METADATA_ISO_GAINS
    METADATA_SRATIONAL                      IsoDigitalGain;     //  MF_CAPTURE_METADATA_ISO_GAINS
    METADATA_UINT64                         SensorFrameRate;    //  MF_CAPTURE_METADATA_SENSORFRAMERATE
    METADATA_SRATIONAL                      WhiteBalanceGain_R; //  MF_CAPTURE_METADATA_WHITEBALANCE_GAINS
    METADATA_SRATIONAL                      WhiteBalanceGain_G; //  MF_CAPTURE_METADATA_WHITEBALANCE_GAINS
    METADATA_SRATIONAL                      WhiteBalanceGain_B; //  MF_CAPTURE_METADATA_WHITEBALANCE_GAINS

} METADATA_PREVIEWAGGREGATION, *PMETADATA_PREVIEWAGGREGATION;

typedef struct _METADATA_IMAGEAGGREGATION
{
    //  Mandatory fields
    METADATA_UINT32 FrameId;
    METADATA_INT64  ExposureTime;
    METADATA_UINT32 ISOSpeed;
    METADATA_UINT32 LensPosition;
    METADATA_UINT64 SceneMode;
    METADATA_UINT32 FlashOn;
    METADATA_UINT32 FlashPower;
    METADATA_UINT32 WhiteBalanceMode;
    METADATA_UINT32 ZoomFactor;
    METADATA_UINT32 FocusLocked;
    METADATA_UINT32 WhiteBalanceLocked;
    METADATA_UINT32 ExposureLocked;

    //  Required Exif data
    METADATA_SHORT          Orientation;
    METADATA_TIMEFIELDS     LocalTime;
    METADATA_SHORTSTRING    Make;
    METADATA_SHORTSTRING    Model;
    METADATA_SHORTSTRING    Software;
    METADATA_SHORT          ColorSpace;
    METADATA_RATIONAL       Gamma;
    METADATA_SHORTSTRING    MakerNote;
    METADATA_RATIONAL       FNumber;
    METADATA_SHORT          ExposureProgram;
    METADATA_SRATIONAL      ShutterSpeedValue;
    METADATA_RATIONAL       Aperture;
    METADATA_SRATIONAL      Brightness;
    METADATA_SRATIONAL      ExposureBias;
    METADATA_RATIONAL       SubjectDistance;
    METADATA_SHORT          MeteringMode;
    METADATA_SHORT          LightSource;
    METADATA_SHORT          Flash;
    METADATA_RATIONAL       FocalLength;
    METADATA_RATIONAL       FocalPlaneXResolution;
    METADATA_RATIONAL       FocalPlaneYResolution;
    METADATA_RATIONAL       ExposureIndex;
    METADATA_SHORT          ExposureMode;
    METADATA_SHORT          WhiteBalance;
    METADATA_RATIONAL       DigitalZoomRatio;
    METADATA_SHORT          FocalLengthIn35mmFilm;
    METADATA_SHORT          SceneCaptureType;
    METADATA_RATIONAL       GainControl;
    METADATA_SHORT          Contrast;
    METADATA_SHORT          Saturation;
    METADATA_SHORT          Sharpness;
    METADATA_SHORT          SubjectDistanceRange;

    //  Additional required metadata
    METADATA_EVCOMP         EVCompensation;

    //  Optional
    METADATA_LONG           FocusState;

} METADATA_IMAGEAGGREGATION, *PMETADATA_IMAGEAGGREGATION;

typedef enum
{
    Metadata_Orientation_TopBottomLeftRight = 1,
    Metadata_Orientation_TopBottomRightLeft = 2,
    Metadata_Orientation_BottomTopLeftRight = 3,
    Metadata_Orientation_BottomTopRightLeft = 4,
    Metadata_Orientation_LeftRightTopBottom = 5,    // 90 degree rotation
    Metadata_Orientation_RightLeftTopBottom = 6,
    Metadata_Orientation_LeftRightBottomTop = 7,
    Metadata_Orientation_RightLeftBottomTop = 8

} METADATA_ORIENTATION_ENUM;

typedef struct
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    METADATA_PREVIEWAGGREGATION     Data;
} CAMERA_METADATA_PREVIEWAGGREGATION, *PCAMERA_METADATA_PREVIEWAGGREGATION;

typedef struct
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    METADATA_IMAGEAGGREGATION            Data;
} CAMERA_METADATA_IMAGEAGGREGATION, *PCAMERA_METADATA_IMAGEAGGREGATION;

typedef struct
{
    KSCAMERA_METADATA_ITEMHEADER        Header;
    UINT32                              eFocusState;
    UINT32                              Reserved;
} CAMERA_METADATA_FOCUS_STATE, *PCAMERA_METADATA_FOCUS_STATE;

typedef enum _METADATA_EXPRESSION
{
    EXPRESSION_NONE=0,
    EXPRESSION_SMILE=1
} METADATA_EXPRESSION;

typedef struct _METADATA_FACEDATA
{
    RECT    Region;             // A rectangle describing the face.
    ULONG   confidenceLevel;    // [0, 100].
    ULONG   BlinkScoreLeft;     // [0, 100]. 0 indicates no blink for the left eye. 100 indicates definite blink for the left eye
    ULONG   BlinkScoreRight;    // [0, 100]. 0 indicates no blink for the right eye. 100 indicates definite blink for the right eye
    ULONG   Reserved;           // Pad for 64-bit alignment.
    BOOL    FacialExpression;   // Is the face smiling?
    ULONG   FacialExpressionScore; // [0, 100]. 0 indicates no such facial expression as identified. 100 indicates definite such facial expression as defined
} METADATA_FACEDATA, *PMETADATA_FACEDATA;

typedef struct _CAMERA_METADATA_FACEHEADER
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    UINT32                          Count;  // Number of detected faces.
    UINT64                          Flags;  // If KSCAMERA_EXTENDEDPROP_FACEDETECTION_BLINK or _SMILE bitflags are set, characterization metadata is valid.
    UINT64                          Timestamp;  // Time in 100ns of actual face detection.
} CAMERA_METADATA_FACEHEADER, *PCAMERA_METADATA_FACEHEADER;

typedef struct _METADATA_HISTOGRAM
{
    ULONG   Width;
    ULONG   Height;
    ULONG   ChannelMask;
    ULONG   FourCC;
    ULONG   P0Data[256];        // first primary    (R / Y )
    ULONG   P1Data[256];        // second           (G / Cr)
    ULONG   P2Data[256];        // third            (B / Cb)
} METADATA_HISTOGRAM, *PMETADATA_HISTOGRAM;

typedef struct _CAMERA_METADATA_HISTOGRAM
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    METADATA_HISTOGRAM              Data;
} CAMERA_METADATA_HISTOGRAM, *PCAMERA_METADATA_HISTOGRAM;

typedef struct _CAMERA_METADATA_UVC_HEADER
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    KSSTREAM_UVC_METADATA           Data;
} CAMERA_METADATA_UVC_HEADER, *PCAMERA_METADATA_UVC_HEADER;

////////////////////////////////////////////////////////////////////////////////
// Camera Extrinsics
////////////////////////////////////////////////////////////////////////////////

typedef struct _KS_FLOAT2
{
    FLOAT   x;
    FLOAT   y;
} KS_FLOAT2;

typedef struct _KS_FLOAT3
{
    FLOAT   x;
    FLOAT   y;
    FLOAT   z;
} KS_FLOAT3;

typedef struct _KS_QUATERNION
{
    FLOAT   x;
    FLOAT   y;
    FLOAT   z;
    FLOAT   w;
} KS_QUATERNION;

typedef struct _KS_CAMERA_EXTRINSICS_CALIBRATEDTRANSFORM
{
    GUID            CalibrationId;
    KS_FLOAT3       Position;
    KS_QUATERNION   Orientation;
} KS_CAMERA_EXTRINSICS_CALIBRATEDTRANSFORM, *PKS_CAMERA_EXTRINSICS_CALIBRATEDTRANSFORM;

typedef struct _KS_CAMERA_EXTRINSICS
{
    UINT32 TransformCount;
    KS_CAMERA_EXTRINSICS_CALIBRATEDTRANSFORM CalibratedTransforms[1];
} KS_CAMERA_EXTRINSICS, *PKS_CAMERA_EXTRINSICS;

////////////////////////////////////////////////////////////////////////////////
// Camera Intrinsics
////////////////////////////////////////////////////////////////////////////////

typedef struct _KS_CAMERAINTRINSIC_PINHOLECAMERAMODEL
{
    KS_FLOAT2   FocalLength;
    KS_FLOAT2   PrincipalPoint;
} KS_CAMERAINTRINSIC_PINHOLECAMERAMODEL, *PKS_CAMERAINTRINSIC_PINHOLECAMERAMODEL;

typedef struct _KS_CAMERAINTRINSIC_DISTORTIONMODEL
{
    FLOAT Radial_k1;
    FLOAT Radial_k2;
    FLOAT Radial_k3;
    FLOAT Tangential_p1;
    FLOAT Tangential_p2;
} KS_CAMERAINTRINSIC_DISTORTIONMODEL, *PKS_CAMERAINTRINSIC_DISTORTIONMODEL;

typedef struct _KS_PINHOLECAMERAINTRINSIC_INTRINSICMODEL
{
    UINT32 Width;
    UINT32 Height;
    KS_CAMERAINTRINSIC_PINHOLECAMERAMODEL CameraModel;
    KS_CAMERAINTRINSIC_DISTORTIONMODEL DistortionModel;
} KS_PINHOLECAMERAINTRINSIC_INTRINSICMODEL, *PKS_PINHOLECAMERAINTRINSIC_INTRINSICMODEL;

typedef struct _KS_CAMERA_INTRINSICS
{
    UINT32 IntrinsicModelCount;
    KS_PINHOLECAMERAINTRINSIC_INTRINSICMODEL IntrinsicModels[1];
} KS_CAMERA_INTRINSICS, *PKS_CAMERA_INTRINSICS;

typedef struct _CAMERA_METADATA_EXTRINSICS
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    KS_CAMERA_EXTRINSICS            Data;
} CAMERA_METADATA_EXTRINSICS, *PCAMERA_METADATA_EXTRINSICS;

typedef struct _CAMERA_METADATA_INTRINSICS
{
    KSCAMERA_METADATA_ITEMHEADER    Header;
    KS_CAMERA_INTRINSICS            Data;
} CAMERA_METADATA_INTRINSICS, *PCAMERA_METADATA_INTRINSICS;

#if !defined(STATIC_KSPROPERTYSETID_CameraAttributes)
// Property set implemented by USBVideo.sys and accessed by devproxy
// {2A3C5205-F9C8-4F78-B110-43D534C0CA64}
#define STATIC_KSPROPERTYSETID_CameraAttributes \
     0x2a3c5205, 0xf9c8, 0x4f78, 0xb1, 0x10, 0x43, 0xd5, 0x34, 0xc0, 0xca, 0x64
DEFINE_GUIDSTRUCT("2A3C5205-F9C8-4F78-B110-43D534C0CA64", KSPROPERTYSETID_CameraAttributes);
#define KSPROPERTYSETID_CameraAttributes DEFINE_GUIDNAMED(KSPROPERTYSETID_CameraAttributes)

#define KSPROPERTY_CAMERA_ATTRIBUTES_EXTRINSICS    0
#define KSPROPERTY_CAMERA_ATTRIBUTES_INTRINSICS    1

#pragma message( "Defining KSCAMERA_ATTRIBUTES_HEADER" )

typedef
struct _KSCAMERA_ATTRIBUTES_HEADER
{
    ULONG   Size;           //  Size of the entire payload including this header.
    ULONG   PinId;          //  The Pin this payload affects.
} KSCAMERA_ATTRIBUTES_HEADER, *PKSCAMERA_ATTRIBUTES_HEADER;
#endif
#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)

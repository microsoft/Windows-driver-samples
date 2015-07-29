/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        util.h

    Abstract:

        Contains miscellaneous helper functions such as random number 
        generation and bounds checking functions.

    History:

        created 7/12/2013

**************************************************************************/

//
//  Generate a random number using the standard Crypto library.
//      Fall back to the QPC, if crypto library reports an error.
//
ULONG
GetRandom(
    _In_    ULONG Minimum,
    _In_    ULONG Maximum
);

//
//  Generate a random number using the standard Crypto library.
//      Fall back to the QPC, if crypto library reports an error.
//
LONG
GetRandom(
    _In_    LONG Minimum,
    _In_    LONG Maximum
);

//
//  Convert an EV Compensation flag into the denominator of a fraction.
//
LONG
EVFlags2Denominator(
    _In_    ULONGLONG Flags
);

//
//  Convert a WhiteBalance Extended Property Enum to temperature
//
ULONG
WhiteBalancePreset2Temperature(
    _In_    ULONG WBPreset
);

//
//  Checks if a legacy Exposure setting is in range
//
BOOL
ExposureBinaryLogIsValid(
    _In_ LONG ExposureBL
);

//
//  Converts a legacy Exposure setting into 100 ns
//
LONGLONG
ExposureBinaryLogTo100ns(
    _In_ LONG ExposureBL
);

//
//  Converts a Exposure setting into a Binary log preset
//
LONG
Exposure100nsToBinaryLog(
    _In_ LONGLONG Exposure100ns
);

//
//  Convert ISO preset flags to a preset number
//
ULONG
IsoPreset2Value(
    _In_    ULONGLONG Flags
);

//
//  Convert ISO Mode & Value to legacy presets.
//
//  Note: This function will simply pass legacy presets through.
//
ULONGLONG
IsoModeValue2Preset(
    _In_    ULONGLONG   Mode,
    _In_    ULONG       Value
);

LPCSTR
KSStateToStateName(
    _In_    KSSTATE state
);

BOOL
MultiplyCheckOverflow (
    ULONG a,
    ULONG b,
    ULONG *pab
);

//  Note: I overload this function intentionally.  Just use
//        the correct type as the first parameter and the
//        compiler figures out the function to use.
template<class T>
inline
NTSTATUS
BoundsCheck(
    _In_    T                                               Value,
    _In_    const KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING   &Bounds
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( ((Value - T(Bounds.Min) ) % T(Bounds.Step)) != 0 ||
            Value > T(Bounds.Max) ||
            Value < T(Bounds.Min) )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

inline
NTSTATUS
BoundsCheckSigned(
    _In_    LONGLONG                                        Value,
    _In_    const KSCAMERA_EXTENDEDPROP_VIDEOPROCSETTING   &Bounds
)
{
    return BoundsCheck<LONGLONG>( Value, Bounds );
}

inline
NTSTATUS
BoundsCheckSigned(
    _In_    LONG                            Value,
    _In_    const KSPROPERTY_STEPPING_LONG &Bounds
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( ((Value - Bounds.Bounds.SignedMinimum) % Bounds.SteppingDelta) != 0 ||
            Value > Bounds.Bounds.SignedMaximum ||
            Value < Bounds.Bounds.SignedMinimum )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

inline
NTSTATUS
BoundsCheckSigned(
    _In_    LONGLONG                            Value,
    _In_    const KSPROPERTY_STEPPING_LONGLONG &Bounds
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( ((Value - Bounds.Bounds.SignedMinimum) % Bounds.SteppingDelta) != 0 ||
            Value > Bounds.Bounds.SignedMaximum ||
            Value < Bounds.Bounds.SignedMinimum )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//  Note: I overload this function intentionally.  Just use
//        the correct type as the first parameter and the
//        compiler figures out the function to use.
inline
NTSTATUS
BoundsCheckUnsigned(
    _In_    ULONG                           Value,
    _In_    const KSPROPERTY_STEPPING_LONG &Bounds
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( ((Value - Bounds.Bounds.UnsignedMinimum) % Bounds.SteppingDelta) != 0 ||
            Value > Bounds.Bounds.UnsignedMaximum ||
            Value < Bounds.Bounds.UnsignedMinimum )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

inline
NTSTATUS
BoundsCheckUnsigned(
    _In_    ULONGLONG                           Value,
    _In_    const KSPROPERTY_STEPPING_LONGLONG &Bounds
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( ((Value - Bounds.Bounds.UnsignedMinimum) % Bounds.SteppingDelta) != 0 ||
            Value > Bounds.Bounds.UnsignedMaximum ||
            Value < Bounds.Bounds.UnsignedMinimum )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}

//
//  This function is used internally to translate KSCAMERA_PERFRAMESETTING
//  AUTO & MANUAL flags to KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG AUTO & MANUAL
//  flags.  There is no need to translate them back.
//
inline
ULONGLONG
TranslatePFS2VideoProcFlags(
    ULONGLONG   FromMode
)
{
    //  The PFS settings and VideoProc flags are mixed.
    //  To handle that, we'll first remove any PFS flags
    //  and hang onto the rest.
    ULONGLONG   ToMode=
        FromMode & ~(KSCAMERA_PERFRAMESETTING_AUTO|KSCAMERA_PERFRAMESETTING_MANUAL);

    if( FromMode & KSCAMERA_PERFRAMESETTING_AUTO )
    {
        ToMode  |= KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_AUTO;
    }
    if( FromMode & KSCAMERA_PERFRAMESETTING_MANUAL )
    {
        ToMode  |= KSCAMERA_EXTENDEDPROP_VIDEOPROCFLAG_MANUAL;
    }

    return ToMode;
}

//
//  Debugging Helper
//
PCSTR
FocusStateDbgTxt( 
    _In_    KSCAMERA_EXTENDEDPROP_FOCUSSTATE State 
);

#define SAFE_FREE(_x_)      \
    if( _x_ )               \
    {                       \
        ExFreePool( _x_ );  \
        _x_ = nullptr;      \
    }

#define SAFE_DELETE(_x_)    \
    if( _x_ )               \
    {                       \
        delete  _x_;        \
        _x_ = nullptr;      \
    }

#define SAFE_DELETE_ARRAY(_x_)  \
    if( _x_ )                   \
    {                           \
        delete [] _x_;          \
        _x_ = nullptr;          \
    }

#define SAFE_CLOSE_HANDLE(_x_)          \
    if( (_x_ != NULL) && (_x_ != INVALID_HANDLE_VALUE) )   \
    {                                   \
        ZwClose( _x_ );                 \
        _x_ = INVALID_HANDLE_VALUE;     \
    }


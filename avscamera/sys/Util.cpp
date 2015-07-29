/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        util.cpp

    Abstract:

        Contains miscellaneous helper functions such as random number 
        generation and bounds checking functions.

    History:

        created 7/12/2013

**************************************************************************/

#include "Common.h"
#include <bcrypt.h>

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

//
//  Generate a random number using the standard Crypto library.
//      Fall back to the QPC, if crypto library reports an error.
//
ULONG
GetRandom(
    _In_    ULONG Minimum,
    _In_    ULONG Maximum
)
{
    ULONG   Value=0;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if( KeGetCurrentIrql()==PASSIVE_LEVEL )
    {
        Status=
            BCryptGenRandom( NULL, (PUCHAR) &Value, sizeof(Value), BCRYPT_USE_SYSTEM_PREFERRED_RNG );
    }

    //  Fallback to QPC if BCRYPT isn't available.
    if( !NT_SUCCESS(Status) )
    {
        LARGE_INTEGER QPC = KeQueryPerformanceCounter(NULL);
        Value = QPC.u.LowPart;
    }

    //  Make sure we don't divide by 0.
    if( Minimum < Maximum )
    {
        Value = ((Value - Minimum) % (Maximum - Minimum +1)) + Minimum;
    }
    else
    {
        Value = Minimum ;
    }

    return Value;
}

//
//  Generate a random number using the standard Crypto library.
//      Fall back to the QPC, if crypto library reports an error.
//
LONG
GetRandom(
    _In_    LONG Minimum,
    _In_    LONG Maximum
)
{
    ULONG   Value=0;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if( KeGetCurrentIrql()==PASSIVE_LEVEL )
    {
        Status=
            BCryptGenRandom( NULL, (PUCHAR) &Value, sizeof(Value), BCRYPT_USE_SYSTEM_PREFERRED_RNG );
    }

    //  Fallback to QPC if BCRYPT isn't available.
    if( !NT_SUCCESS(Status) )
    {
        LARGE_INTEGER QPC = KeQueryPerformanceCounter(NULL);
        Value = QPC.u.LowPart;
    }

    //  Make sure we don't divide by 0.
    if( Minimum < Maximum )
    {
        Value = ((Value - Minimum) % (Maximum - Minimum +1)) + Minimum;
    }
    else
    {
        Value = Minimum ;
    }

    return Value;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
//  Convert an EV Compensation flag into the denominator of a fraction.
//
LONG
EVFlags2Denominator(
    _In_    ULONGLONG Flags
)
{
    PAGED_CODE( );

    static
    LONG    val[] = {1,2,3,4,6};

    switch( Flags )
    {
    case KSCAMERA_EXTENDEDPROP_EVCOMP_SIXTHSTEP:
        return 6;
    case KSCAMERA_EXTENDEDPROP_EVCOMP_QUARTERSTEP:
        return 4;
    case KSCAMERA_EXTENDEDPROP_EVCOMP_THIRDSTEP:
        return 3;
    case KSCAMERA_EXTENDEDPROP_EVCOMP_HALFSTEP:
        return 2;
    case KSCAMERA_EXTENDEDPROP_EVCOMP_FULLSTEP:
        return 1;
    case KSCAMERA_PERFRAMESETTING_AUTO:
        return val[GetRandom( (ULONG) 0, (ULONG) SIZEOF_ARRAY(val)-1)];
    }

    return 0;
}

//  Convert white balance presets to a temperature.
ULONG
WhiteBalancePreset2Temperature(
    _In_    ULONG WBPreset
)
{
    PAGED_CODE();

    ULONG temp = 0;

    switch (WBPreset)
    {
    case KSCAMERA_EXTENDEDPROP_WBPRESET_CLOUDY:
        temp = 6000;
        break;
    case KSCAMERA_EXTENDEDPROP_WBPRESET_DAYLIGHT:
        temp = 5200;
        break;
    case KSCAMERA_EXTENDEDPROP_WBPRESET_FLASH:
        temp = 6000;
        break;
    case KSCAMERA_EXTENDEDPROP_WBPRESET_FLUORESCENT:
        temp = 4000;
        break;
    case KSCAMERA_EXTENDEDPROP_WBPRESET_TUNGSTEN:
        temp = 3200;
        break;
    case KSCAMERA_EXTENDEDPROP_WBPRESET_CANDLELIGHT:
        temp = 1000;
        break;
    default:
        temp = 5000;
        break;
    }

    return temp;
}

typedef struct
{
    LONG exposureBinaryLog;
    LONGLONG exposure100ns;
} ExposurePresetStruct;

const ExposurePresetStruct ExposurePresets[] =
{
    { -10, 10000 }, //               1/1024s -> 9765.625         (100 ns) (Min is 1 ms)
    { -9, 19531 }, //               1/512s  -> 19531.25         (100 ns)
    { -8, 39063 }, //               1/256s  -> 39062.5          (100 ns)
    { -7, 78125 }, //               1/128s  -> 78125            (100 ns)
    { -6, 196250 }, //              1/64s   -> 196250           (100 ns)
    { -5, 312500 }, //              1/32s   -> 312500           (100 ns)
    { -4, 625000 }, //              1/16s   -> 625000           (100 ns)
    { -3, 1250000 }, //             1/8s    -> 1250000          (100 ns)
    { -2, 2500000 }, //             1/4s    -> 2500000          (100 ns)
    { -1, 5000000 }, //             1/2s    -> 5000000          (100 ns)
    { 0, 10000000 },//              1s      ->  1 * 10000000    (100 ns)
    { 1, 20000000 },//              2s      ->  2 * 10000000    (100 ns)
    { 2, 40000000 },//              4s      ->  4 * 10000000    (100 ns)
    { 3, 80000000 },//              8s      ->  8 * 10000000    (100 ns)
    { 4, 160000000 },//             16s     -> 16 * 10000000    (100 ns)
    { 5, 320000000 },//             32s     -> 32 * 10000000    (100 ns)
    { 6, 640000000 },//             64s     -> 64 * 10000000    (100 ns)
    { 7, 1280000000 },//           128s    -> 128 * 10000000    (100 ns)
    { 8, 2560000000 },//           256s    -> 256 * 10000000    (100 ns)
    { 9, 3600000000 },//           512s    -> 512 * 10000000    (100 ns) (Max is 360 s)
};

const ExposurePresetStruct ExposureMidpoints[] =
{
    { -10, 13811 }, //              2^(-9.5) -> .001381067
    { -9,  27621 }, //              2^(-8.5) -> .002762136
    { -8, 55243 }, //               2^(-7.5) -> .005524271
    { -7, 110485 }, //              2^(-6.5) -> .011048543
    { -6, 220970 }, //              2^(-5.5) -> .022097086
    { -5, 441941 }, //              2^(-4.5) -> .044194174
    { -4, 883883 }, //              2^(-3.5) -> .0883883476
    { -3, 1767767 }, //             2^(-2.5) -> .1767766953
    { -2, 3535534 }, //             2^(-1.5) -> .3535533905
    { -1, 7071067 }, //             2^(-0.5) -> .7071067811
    { 0, 14142136 },//              2^(0.5) ->  1.414213562
    { 1, 28284271 },//              2^(1.5) ->  2.828427125
    { 2, 56568542 },//              2^(2.5) ->  5.656854250
    { 3, 113137085 },//             2^(3.5) ->  11.31370850
    { 4, 226274170 },//             2^(4.5) ->  22.62741700
    { 5, 452548340 },//             2^(5.5) ->  45.25483400
    { 6, 905096680 },//             2^(6.5) ->  90.50966799
    { 7, 1810193360 },//            2^(7.5) ->  181.0193360
    { 8, 3080000000 },//           Halfway between 8 and Max:  30800
    { 9, 3600000000 },//           Above
};

BOOL
ExposureBinaryLogIsValid(
    _In_ LONG ExposureBL
)
{
    PAGED_CODE();

    if( ExposurePresets[0].exposureBinaryLog > ExposureBL || 
        ExposurePresets[_countof(ExposurePresets) - 1].exposureBinaryLog < ExposureBL )
    {
        return FALSE;
    }

    return TRUE;
}

//  Changes a Legacy Preset value into 100 ns units for Exposure. If the 
//  preset is out of range of what we support, return 0 to indicate failure.
LONGLONG
ExposureBinaryLogTo100ns(
    _In_ LONG ExposureBL
)
{
    PAGED_CODE( );

    for (LONG i = 0; i < _countof(ExposurePresets) - 1; i++)
    {
        if (ExposureBL == ExposurePresets[i].exposureBinaryLog)
        {
            return ExposurePresets[i].exposure100ns;
        }
    }

    return 0;
}

LONG
Exposure100nsToBinaryLog(
    _In_ LONGLONG Exposure100ns
)
{
    PAGED_CODE( );

    for(LONG i = 0; i < _countof(ExposureMidpoints) - 1; i++)
    {
        if(Exposure100ns <= ExposureMidpoints[i].exposure100ns)
        {
            return ExposureMidpoints[i].exposureBinaryLog;
        }
    }

    return ExposureMidpoints[_countof(ExposureMidpoints) - 1].exposureBinaryLog;
}


static
ULONGLONG   PotentialIsoSetting[] =
{
    KSCAMERA_EXTENDEDPROP_ISO_50
    , KSCAMERA_EXTENDEDPROP_ISO_80
    , KSCAMERA_EXTENDEDPROP_ISO_100
    , KSCAMERA_EXTENDEDPROP_ISO_200
    , KSCAMERA_EXTENDEDPROP_ISO_400
    , KSCAMERA_EXTENDEDPROP_ISO_800
    , KSCAMERA_EXTENDEDPROP_ISO_1600
    , KSCAMERA_EXTENDEDPROP_ISO_3200
    , KSCAMERA_EXTENDEDPROP_ISO_6400
    , KSCAMERA_EXTENDEDPROP_ISO_12800
    , KSCAMERA_EXTENDEDPROP_ISO_25600
};

//
//  Convert ISO preset flags to a preset number
//
ULONG
IsoPreset2Value(
    _In_    ULONGLONG Flags
)
{
    PAGED_CODE( );

    static
    ULONG   IsoValue[] =
    {
        50
        , 80
        , 100
        , 200
        , 400
        , 800
        , 1600
        , 3200
        , 6400
        , 12800
        , 25600
    };

    //  Simple scan for min
    for( ULONG i=0; i<_countof(PotentialIsoSetting); i++ )
    {
        if( PotentialIsoSetting[i] & Flags )
        {
            return IsoValue[i];
        }
    }
    return MAXULONG;
}

//
//  Convert ISO Mode & Value to legacy presets.
//
//  Note: This function will simply pass legacy presets through.
//
ULONGLONG
IsoModeValue2Preset(
    _In_    ULONGLONG   Mode,
    _In_    ULONG       Value
)
{
    PAGED_CODE( );

    //  These values are staggerred midpoints along a logrithmic scale.
    //  Using these pre-calculated values simplifies our search to a
    //  simple set of comparisons.
    static
    ULONG   IsoValue[] =
    {
        64      //50        //KSCAMERA_EXTENDEDPROP_ISO_50
        ,90     //, 80      //KSCAMERA_EXTENDEDPROP_ISO_80
        , 142   //, 100     //KSCAMERA_EXTENDEDPROP_ISO_100
        , 283   //, 200     //KSCAMERA_EXTENDEDPROP_ISO_200
        , 566   //, 400     //KSCAMERA_EXTENDEDPROP_ISO_400
        , 1132  //, 800     //KSCAMERA_EXTENDEDPROP_ISO_800
        , 2263  //, 1600    //KSCAMERA_EXTENDEDPROP_ISO_1600
        , 4526  //, 3200    //KSCAMERA_EXTENDEDPROP_ISO_3200
        , 9051  //, 6400    //KSCAMERA_EXTENDEDPROP_ISO_6400
        , 18102 //, 12800   //KSCAMERA_EXTENDEDPROP_ISO_12800
    };

    if( Mode & KSCAMERA_EXTENDEDPROP_ISO_MANUAL )
    {
        //  Simple scan for min
        for( ULONG i=0; i<_countof(PotentialIsoSetting)-1; i++ )
        {
            if( Value < IsoValue[i] )
            {
                return PotentialIsoSetting[i];
            }
        }
        return KSCAMERA_EXTENDEDPROP_ISO_25600;
    }
    return Mode;
}

LPCSTR
KSStateToStateName(
    _In_    KSSTATE state
)
{
    PAGED_CODE( );

    static const CHAR *txt[] =
    {
        "STOP",
        "ACQUIRE",
        "PAUSE",
        "RUN"
    };

    return ((KSSTATE_STOP <= state && KSSTATE_RUN >= state) ? txt[state] : "UNK");
}

//  Debugging helper.
PCSTR
FocusStateDbgTxt( 
    _In_    KSCAMERA_EXTENDEDPROP_FOCUSSTATE State 
)
{
    PAGED_CODE();

    switch( State )
    {
    case KSCAMERA_EXTENDEDPROP_FOCUSSTATE_UNINITIALIZED:
        return "FOCUSSTATE_UNINITIALIZED";
    case KSCAMERA_EXTENDEDPROP_FOCUSSTATE_LOST:
        return "FOCUSSTATE_LOST";
    case KSCAMERA_EXTENDEDPROP_FOCUSSTATE_SEARCHING:
        return "FOCUSSTATE_SEARCHING";
    case KSCAMERA_EXTENDEDPROP_FOCUSSTATE_FOCUSED:
        return "FOCUSSTATE_FOCUSED";
    case KSCAMERA_EXTENDEDPROP_FOCUSSTATE_FAILED:
        return "FOCUSSTATE_FAILED";
    default:
        return "FOCUSSTATE_unknown";
    }
}

BOOL
MultiplyCheckOverflow (
    ULONG a,
    ULONG b,
    ULONG *pab
)

/*++

Routine Description:

    Perform a 32 bit unsigned multiplication and check for arithmetic overflow.

Arguments:

    a -
        First operand

    b -
        Second operand

    pab -
        Result

Return Value:

    TRUE -
        no overflow

    FALSE -
        overflow occurred

--*/

{
    PAGED_CODE();

    *pab = a * b;
    if ((a == 0) || (((*pab) / a) == b))
    {
        return TRUE;
    }
    return FALSE;
}


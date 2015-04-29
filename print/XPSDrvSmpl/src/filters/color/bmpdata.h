/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bmpdata.h

Abstract:

   This file supplies definitions for various look-up tables for use in converting and
   color matching input bitmaps via WCS/ICM and WIC

--*/

#pragma once

//
// Macros to limit maximum buffer size allocations
//
#define MAX_COLDATATYPE_SIZE sizeof(FLOAT)
#define MAX_COLCHANNEL_COUNT 9 // 8 channel plus alpha
#define MAX_PIXELWIDTH_COUNT SIZE_MAX/MAX_COLDATATYPE_SIZE/MAX_COLCHANNEL_COUNT


/*
Enumeration for all WIC pixel formats
*/
enum EWICPixelFormat
{
    kWICPixelFormatDontCare = 0, kWICPixelFormatMin = 0,
    kWICPixelFormat1bppIndexed,
    kWICPixelFormat2bppIndexed,
    kWICPixelFormat4bppIndexed,
    kWICPixelFormat8bppIndexed,
    kWICPixelFormatBlackWhite,
    kWICPixelFormat2bppGray,
    kWICPixelFormat4bppGray,
    kWICPixelFormat8bppGray,
    kWICPixelFormat16bppBGR555,
    kWICPixelFormat16bppBGR565,
    kWICPixelFormat16bppGray,
    kWICPixelFormat24bppBGR,
    kWICPixelFormat24bppRGB,
    kWICPixelFormat32bppBGR,
    kWICPixelFormat32bppBGRA,
    kWICPixelFormat32bppPBGRA,
    kWICPixelFormat32bppGrayFloat,
    kWICPixelFormat48bppRGBFixedPoint,
    kWICPixelFormat16bppGrayFixedPoint,
    kWICPixelFormat32bppBGR101010,
    kWICPixelFormat48bppRGB,
    kWICPixelFormat64bppRGBA,
    kWICPixelFormat64bppPRGBA,
    kWICPixelFormat96bppRGBFixedPoint,
    kWICPixelFormat128bppRGBAFloat,
    kWICPixelFormat128bppPRGBAFloat,
    kWICPixelFormat128bppRGBFloat,
    kWICPixelFormat32bppCMYK,
    kWICPixelFormat64bppRGBAFixedPoint,
    kWICPixelFormat64bppRGBFixedPoint,
    kWICPixelFormat128bppRGBAFixedPoint,
    kWICPixelFormat128bppRGBFixedPoint,
    kWICPixelFormat64bppRGBAHalf,
    kWICPixelFormat64bppRGBHalf,
    kWICPixelFormat48bppRGBHalf,
    kWICPixelFormat32bppRGBE,
    kWICPixelFormat16bppGrayHalf,
    kWICPixelFormat32bppGrayFixedPoint,
    kWICPixelFormat64bppCMYK,
    kWICPixelFormat24bpp3Channels,
    kWICPixelFormat32bpp4Channels,
    kWICPixelFormat40bpp5Channels,
    kWICPixelFormat48bpp6Channels,
    kWICPixelFormat56bpp7Channels,
    kWICPixelFormat64bpp8Channels,
    kWICPixelFormat48bpp3Channels,
    kWICPixelFormat64bpp4Channels,
    kWICPixelFormat80bpp5Channels,
    kWICPixelFormat96bpp6Channels,
    kWICPixelFormat112bpp7Channels,
    kWICPixelFormat128bpp8Channels,
    kWICPixelFormat40bppCMYKAlpha,
    kWICPixelFormat80bppCMYKAlpha,
    kWICPixelFormat32bpp3ChannelsAlpha,
    kWICPixelFormat40bpp4ChannelsAlpha,
    kWICPixelFormat48bpp5ChannelsAlpha,
    kWICPixelFormat56bpp6ChannelsAlpha,
    kWICPixelFormat64bpp7ChannelsAlpha,
    kWICPixelFormat72bpp8ChannelsAlpha,
    kWICPixelFormat64bpp3ChannelsAlpha,
    kWICPixelFormat80bpp4ChannelsAlpha,
    kWICPixelFormat96bpp5ChannelsAlpha,
    kWICPixelFormat112bpp6ChannelsAlpha,
    kWICPixelFormat128bpp7ChannelsAlpha,
    kWICPixelFormat144bpp8ChannelsAlpha,
    kWICPixelFormatMax
};

/*
ICM/WCS BMFORMAT enumeration indexed from 0 for use as a look-up into
a BMFORMAT data structure array
*/
enum EICMPixelFormat
{
    kBM_x555RGB = 0, kICMPixelFormatMin = 0,
    kBM_x555XYZ,
    kBM_x555Yxy,
    kBM_x555Lab,
    kBM_x555G3CH,
    kBM_RGBTRIPLETS,
    kBM_BGRTRIPLETS,
    kBM_XYZTRIPLETS,
    kBM_YxyTRIPLETS,
    kBM_LabTRIPLETS,
    kBM_G3CHTRIPLETS,
    kBM_5CHANNEL,
    kBM_6CHANNEL,
    kBM_7CHANNEL,
    kBM_8CHANNEL,
    kBM_GRAY,
    kBM_xRGBQUADS,
    kBM_xBGRQUADS,
    kBM_xG3CHQUADS,
    kBM_KYMCQUADS,
    kBM_CMYKQUADS,
    kBM_10b_RGB,
    kBM_10b_XYZ,
    kBM_10b_Yxy,
    kBM_10b_Lab,
    kBM_10b_G3CH,
    kBM_NAMED_INDEX,
    kBM_16b_RGB,
    kBM_16b_XYZ,
    kBM_16b_Yxy,
    kBM_16b_Lab,
    kBM_16b_G3CH,
    kBM_16b_GRAY,
    kBM_565RGB,
    kBM_32b_scRGB,
    kBM_32b_scARGB,
    kBM_S2DOT13FIXED_scRGB,
    kBM_S2DOT13FIXED_scARGB,
    kICMPixelFormatMax
};

/*
This structure is used to store information for converting and processing a particular
WIC format. The structure forms the basis of a look-up table between a source WIC pixel
format and the following items:
    The format to convert to before processing -
        This converts to a form consumable by ICM/WCS.
    The corresponding BMFORMAT enumeration -
        This lets us lookup the data type to pass to TranslateBitmapBits.
    Whether we need an intermediate buffer to process alpha data
        The alpha channel in WIC pixel formats do not match ICM/WCS compatible
        formats. Under these circumstances we need to copy the bitmap data, translate
        and apply back to the WIC bitmap.
    The count of channels and the channel width
        This allows us to lookup the offset required when copying intermediate data to
        the WIC bitmap.
    The offset to the alpha channel (we actually store the COLORDATATYPE and lookup the size)
        When copying alpha data we need to know where to retrieve the alpha channel from and
        where to copy it to in a given scanline.

With this data, handling intermediate buffer data follows the following algorithm:

    If no intermediate buffer is required
        The locked WIC pixel data can be converted in situ
    Otherwise
        Color data handling
            From the begining of the WIC buffer and the intermediate buffer
                Copy the intermediate buffer pixel width into the locked WIC scanline
                Move the WIC pixel pointer on by the WIC pixel width
                Move the intermediate buffer pointer on by the BMFORMAT pixel width
                Repeat till no scanline data left to process
        Alpha data handling
            From the begining of the source and destination WIC buffers
                Offset the source to the alpha channel
                Offset the destination to the alpha channel
                Convert the source to format to the destination
                Copy the alpha data to the destination
                Move the source WIC pixel pointer on by the WIC pixel width
                Move the destination WIC pixel pointer on by the WIC pixel width
                Repeat till no alpha data left to process

*/
struct WICToBMFORMAT
{
    EWICPixelFormat m_pixFormTarget;
    EICMPixelFormat m_bmFormTarget;
    BOOL            m_bNeedsScanBuffer;
    UINT            m_cChannels;
    UINT            m_cAlphaOffset;
    COLORDATATYPE   m_colDataType;
};

/*
Structure providing information about BMFORMAT data for use as a lookup against
the local BMFORMAT enumeration
*/
struct BMFormatData
{
    BMFORMAT      m_bmFormat;
    UINT          m_cChannels;
    COLORDATATYPE m_colDataType;
};

extern CONST WICPixelFormatGUID g_lutPixFrmtGuid[kWICPixelFormatMax];
extern CONST WICToBMFORMAT      g_lutWICToBMFormat[kWICPixelFormatMax];
extern CONST size_t             g_lutColorDataSize[];
extern CONST BMFormatData       g_lutBMFormatData[kICMPixelFormatMax];

struct S2DOT13FIXED
{
    WORD val;
};
typedef S2DOT13FIXED* PS2DOT13FIXED;

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts BYTE to BYTE

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ BYTE& dst,
    _In_  BYTE  src
    )
{
    dst = src;
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts WORD to BYTE

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ BYTE& dst,
    _In_  WORD  src
    )
{
    dst = static_cast<BYTE>((static_cast<DWORD>(src) + 0x7F) >> 8);
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts FLOAT to BYTE

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ BYTE& dst,
    _In_  FLOAT src
    )
{
    if (src < 0.0f)
    {
        dst = 0x00;
    }
    else if (src > 1.0f)
    {
        dst = 0xFF;
    }
    else
    {
        dst = static_cast<BYTE>(src * kMaxByteAsFloat);
    }
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts S2DOT13FIXED to BYTE

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ BYTE&        dst,
    _In_  S2DOT13FIXED src
    )
{
    if (src.val & kS2Dot13Neg)
    {
        dst = 0x00;
    }
    else if (src.val > kS2Dot13One)
    {
        dst = 0xFF;
    }
    else
    {
        dst = static_cast<BYTE>(MulDiv(src.val, 0xFF, kS2Dot13One));
    }
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts BYTE to WORD

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ WORD& dst,
    _In_  BYTE  src
    )
{
    dst = (src << 8) | src;
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts WORD to WORD

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ WORD& dst,
    _In_  WORD  src
    )
{
    dst = src;
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts FLOAT to WORD

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ WORD& dst,
    _In_  FLOAT src
    )
{
    if (src < 0.0f)
    {
        dst = 0x0000;
    }
    else if (src > 1.0f)
    {
        dst = 0xFFFF;
    }
    else
    {
        dst = static_cast<WORD>(src * kMaxWordAsFloat);
    }
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts S2DOT13FIXED to WORD

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ WORD&        dst,
    _In_  S2DOT13FIXED src
    )
{
    if (src.val & kS2Dot13Neg)
    {
        dst = 0x0000;
    }
    else if (src.val > kS2Dot13One)
    {
        dst = 0xFFFF;
    }
    else
    {
        dst = static_cast<BYTE>(MulDiv(src.val, 0xFFFF, kS2Dot13One));
    }
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts BYTE to FLOAT

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ FLOAT& dst,
    _In_  BYTE   src
    )
{
    dst = src/kMaxByteAsFloat;
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts WORD to FLOAT

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ FLOAT& dst,
    _In_  WORD   src
    )
{
    dst = src/kMaxWordAsFloat;
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts FLOAT to FLOAT

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ FLOAT& dst,
    _In_  FLOAT  src
    )
{
    dst = src;
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts S2DOT13FIXED to FLOAT

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ FLOAT&       dst,
    _In_  S2DOT13FIXED src
    )
{
    if (src.val & kS2Dot13Neg)
    {
        dst = -static_cast<FLOAT>(src.val^kS2Dot13Neg)/kS2Dot13One;
    }
    else
    {
        dst = static_cast<FLOAT>(src.val)/kS2Dot13One;
    }
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts BYTE to S2DOT13FIXED

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ S2DOT13FIXED& dst,
    _In_  BYTE          src
    )
{
    dst.val = static_cast<WORD>(MulDiv(kS2Dot13One, src, 0xFF));
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts WORD to S2DOT13FIXED

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ S2DOT13FIXED& dst,
    _In_  WORD          src
    )
{
    dst.val = static_cast<WORD>(MulDiv(kS2Dot13One, src, 0xFFFF));
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts FLOAT to S2DOT13FIXED

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ S2DOT13FIXED& dst,
    _In_  FLOAT         src
    )
{
    if (src < -4.0f)
    {
        dst.val = kS2Dot13Min;
    }
    else if (src > 4.0f)
    {
        dst.val = kS2Dot13Max;
    }
    else
    {
        dst.val = static_cast<WORD>(src * kS2Dot13One);
    }
}

/*++

Routine Name:

    ConvertCopy

Routine Description:

    Inline function that converts a src channel data type to a destination
    of another type. This overload converts S2DOT13FIXED to S2DOT13FIXED

Arguments:

    dst - Destination value to be set
    src - Source value

Return Value:

    None

--*/
inline VOID
ConvertCopy(
    _Out_ S2DOT13FIXED& dst,
    _In_  S2DOT13FIXED  src
    )
{
    dst = src;
}


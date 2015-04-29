/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bmpdata.cpp

Abstract:

   This file supplies various look-up tables for use in converting and color matching
   input bitmaps via WCS/ICM and WIC

--*/

#include "precomp.h"
#include "globals.h"
#include "bmpdata.h"

/*
Look up table between WIC pixel format enumeration and WIC pixel format GUID
*/
CONST WICPixelFormatGUID g_lutPixFrmtGuid[kWICPixelFormatMax] = {
    GUID_WICPixelFormatDontCare,
    GUID_WICPixelFormat1bppIndexed,
    GUID_WICPixelFormat2bppIndexed,
    GUID_WICPixelFormat4bppIndexed,
    GUID_WICPixelFormat8bppIndexed,
    GUID_WICPixelFormatBlackWhite,
    GUID_WICPixelFormat2bppGray,
    GUID_WICPixelFormat4bppGray,
    GUID_WICPixelFormat8bppGray,
    GUID_WICPixelFormat16bppBGR555,
    GUID_WICPixelFormat16bppBGR565,
    GUID_WICPixelFormat16bppGray,
    GUID_WICPixelFormat24bppBGR,
    GUID_WICPixelFormat24bppRGB,
    GUID_WICPixelFormat32bppBGR,
    GUID_WICPixelFormat32bppBGRA,
    GUID_WICPixelFormat32bppPBGRA,
    GUID_WICPixelFormat32bppGrayFloat,
    GUID_WICPixelFormat48bppRGBFixedPoint,
    GUID_WICPixelFormat16bppGrayFixedPoint,
    GUID_WICPixelFormat32bppBGR101010,
    GUID_WICPixelFormat48bppRGB,
    GUID_WICPixelFormat64bppRGBA,
    GUID_WICPixelFormat64bppPRGBA,
    GUID_WICPixelFormat96bppRGBFixedPoint,
    GUID_WICPixelFormat128bppRGBAFloat,
    GUID_WICPixelFormat128bppPRGBAFloat,
    GUID_WICPixelFormat128bppRGBFloat,
    GUID_WICPixelFormat32bppCMYK,
    GUID_WICPixelFormat64bppRGBAFixedPoint,
    GUID_WICPixelFormat64bppRGBFixedPoint,
    GUID_WICPixelFormat128bppRGBAFixedPoint,
    GUID_WICPixelFormat128bppRGBFixedPoint,
    GUID_WICPixelFormat64bppRGBAHalf,
    GUID_WICPixelFormat64bppRGBHalf,
    GUID_WICPixelFormat48bppRGBHalf,
    GUID_WICPixelFormat32bppRGBE,
    GUID_WICPixelFormat16bppGrayHalf,
    GUID_WICPixelFormat32bppGrayFixedPoint,
    GUID_WICPixelFormat64bppCMYK,
    GUID_WICPixelFormat24bpp3Channels,
    GUID_WICPixelFormat32bpp4Channels,
    GUID_WICPixelFormat40bpp5Channels,
    GUID_WICPixelFormat48bpp6Channels,
    GUID_WICPixelFormat56bpp7Channels,
    GUID_WICPixelFormat64bpp8Channels,
    GUID_WICPixelFormat48bpp3Channels,
    GUID_WICPixelFormat64bpp4Channels,
    GUID_WICPixelFormat80bpp5Channels,
    GUID_WICPixelFormat96bpp6Channels,
    GUID_WICPixelFormat112bpp7Channels,
    GUID_WICPixelFormat128bpp8Channels,
    GUID_WICPixelFormat40bppCMYKAlpha,
    GUID_WICPixelFormat80bppCMYKAlpha,
    GUID_WICPixelFormat32bpp3ChannelsAlpha,
    GUID_WICPixelFormat40bpp4ChannelsAlpha,
    GUID_WICPixelFormat48bpp5ChannelsAlpha,
    GUID_WICPixelFormat56bpp6ChannelsAlpha,
    GUID_WICPixelFormat64bpp7ChannelsAlpha,
    GUID_WICPixelFormat72bpp8ChannelsAlpha,
    GUID_WICPixelFormat64bpp3ChannelsAlpha,
    GUID_WICPixelFormat80bpp4ChannelsAlpha,
    GUID_WICPixelFormat96bpp5ChannelsAlpha,
    GUID_WICPixelFormat112bpp6ChannelsAlpha,
    GUID_WICPixelFormat128bpp7ChannelsAlpha,
    GUID_WICPixelFormat144bpp8ChannelsAlpha
};

/*
Look-up table between WICPixelFormat enumeration and conversion information

The aim of this table is to assist us in transforming the input WCS data into a format acceptable
to WCS/ICM TranslateBitmapBits call. The sort of problems this aims to resolve are:

    1. The lack of S7DOT24FIXED format support in WCS/ICM.
    2. The lack of half format support in WCS/ICM.
    3. The lack of support for > 8bpc in WCS/ICM for channel counts > 4

The table is used in the following way:

    1. Look-up the most appropriate WIC pixel format for color conversion by indexing into
       the table using the matching WIC pixel format enumeration then using WIC to convert
       to this format.
    2. Identify the matching BMFORMAT enumeration to process with WCS/ICM
    3. Identify where we need to process alpha data seperately and using an intermediate
       transform buffer to pass to ICM/WCS where necessary

Additionally the table also provides useful look-up information for use when processing
intermediate transform buffers: channel count, alpha channel placement and color data type.

*/
CONST WICToBMFORMAT g_lutWICToBMFormat[kWICPixelFormatMax] = {
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormatDontCare
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat1bppIndexed
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat2bppIndexed
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat4bppIndexed
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat8bppIndexed
    {kWICPixelFormat8bppGray,            kBM_GRAY,                FALSE, 1, 0, COLOR_BYTE},         // kWICPixelFormatBlackWhite
    {kWICPixelFormat8bppGray,            kBM_GRAY,                FALSE, 1, 0, COLOR_BYTE},         // kWICPixelFormat2bppGray
    {kWICPixelFormat8bppGray,            kBM_GRAY,                FALSE, 1, 0, COLOR_BYTE},         // kWICPixelFormat4bppGray
    {kWICPixelFormat8bppGray,            kBM_GRAY,                FALSE, 1, 0, COLOR_BYTE},         // kWICPixelFormat8bppGray
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat16bppBGR555
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat16bppBGR565
    {kWICPixelFormat16bppGray,           kBM_16b_GRAY,            FALSE, 1, 0, COLOR_WORD},         // kWICPixelFormat16bppGray
    {kWICPixelFormat24bppBGR,            kBM_BGRTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat24bppBGR
    {kWICPixelFormat24bppRGB,            kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat24bppRGB
    {kWICPixelFormat24bppBGR,            kBM_BGRTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat32bppBGR
    {kWICPixelFormat32bppBGRA,           kBM_xRGBQUADS,           FALSE, 4, 3, COLOR_BYTE},         // kWICPixelFormat32bppBGRA
    {kWICPixelFormat32bppPBGRA,          kBM_xRGBQUADS,           FALSE, 4, 3, COLOR_BYTE},         // kWICPixelFormat32bppPBGRA
    {kWICPixelFormat16bppGray,           kBM_16b_GRAY,            FALSE, 1, 0, COLOR_WORD},         // kWICPixelFormat32bppGrayFloat
    {kWICPixelFormat48bppRGBFixedPoint,  kBM_S2DOT13FIXED_scRGB,  FALSE, 3, 0, COLOR_S2DOT13FIXED}, // kWICPixelFormat48bppRGBFixedPoint
    {kWICPixelFormat16bppGray,           kBM_16b_GRAY,            FALSE, 1, 0, COLOR_WORD},         // kWICPixelFormat16bppGrayFixedPoint
    {kWICPixelFormat48bppRGB,            kBM_16b_RGB,             FALSE, 3, 0, COLOR_WORD},         // kWICPixelFormat32bppBGR101010
    {kWICPixelFormat48bppRGB,            kBM_16b_RGB,             FALSE, 3, 0, COLOR_WORD},         // kWICPixelFormat48bppRGB
    {kWICPixelFormat64bppRGBA,           kBM_16b_RGB,             TRUE,  4, 3, COLOR_WORD},         // kWICPixelFormat64bppRGBA
    {kWICPixelFormat64bppPRGBA,          kBM_16b_RGB,             TRUE,  4, 3, COLOR_WORD},         // kWICPixelFormat64bppPRGBA
    {kWICPixelFormat128bppRGBFloat,      kBM_32b_scRGB,           TRUE,  4, 0, COLOR_FLOAT},        // kWICPixelFormat96bppRGBFixedPoint
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 3, COLOR_FLOAT},        // kWICPixelFormat128bppRGBAFloat
    {kWICPixelFormat128bppPRGBAFloat,    kBM_32b_scARGB,          FALSE, 4, 3, COLOR_FLOAT},        // kWICPixelFormat128bppPRGBAFloat
    {kWICPixelFormat128bppRGBFloat,      kBM_32b_scARGB,          FALSE, 4, 0, COLOR_FLOAT},        // kWICPixelFormat128bppRGBFloat
    {kWICPixelFormat32bppCMYK,           kBM_CMYKQUADS,           FALSE, 4, 0, COLOR_BYTE},         // kWICPixelFormat32bppCMYK
    {kWICPixelFormat64bppRGBAFixedPoint, kBM_S2DOT13FIXED_scARGB, FALSE, 4, 3, COLOR_S2DOT13FIXED}, // kWICPixelFormat64bppRGBAFixedPoint
    {kWICPixelFormat64bppRGBFixedPoint,  kBM_S2DOT13FIXED_scARGB, FALSE, 4, 0, COLOR_S2DOT13FIXED}, // kWICPixelFormat64bppRGBFixedPoint
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 3, COLOR_FLOAT},        // kWICPixelFormat128bppRGBAFixedPoint
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 0, COLOR_FLOAT},        // kWICPixelFormat128bppRGBFixedPoint
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 3, COLOR_FLOAT},        // kWICPixelFormat64bppRGBAHalf
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 0, COLOR_FLOAT},        // kWICPixelFormat64bppRGBHalf
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 0, COLOR_FLOAT},        // kWICPixelFormat48bppRGBHalf
    {kWICPixelFormat128bppRGBAFloat,     kBM_32b_scARGB,          FALSE, 4, 3, COLOR_FLOAT},        // kWICPixelFormat32bppRGBE
    {kWICPixelFormat16bppGray,           kBM_16b_GRAY,            FALSE, 1, 0, COLOR_WORD},         // kWICPixelFormat16bppGrayHalf
    {kWICPixelFormat16bppGray,           kBM_16b_GRAY,            FALSE, 1, 0, COLOR_WORD},         // kWICPixelFormat32bppGrayFixedPoint
    {kWICPixelFormat32bpp4Channels,      kBM_CMYKQUADS,           FALSE, 4, 0, COLOR_BYTE},         // kWICPixelFormat64bppCMYK
    {kWICPixelFormat24bpp3Channels,      kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat24bpp3Channels
    {kWICPixelFormat32bpp4Channels,      kBM_CMYKQUADS,           FALSE, 4, 0, COLOR_BYTE},         // kWICPixelFormat32bpp4Channels
    {kWICPixelFormat40bpp5Channels,      kBM_5CHANNEL,            FALSE, 5, 0, COLOR_BYTE},         // kWICPixelFormat40bpp5Channels
    {kWICPixelFormat48bpp6Channels,      kBM_6CHANNEL,            FALSE, 6, 0, COLOR_BYTE},         // kWICPixelFormat48bpp6Channels
    {kWICPixelFormat56bpp7Channels,      kBM_7CHANNEL,            FALSE, 7, 0, COLOR_BYTE},         // kWICPixelFormat56bpp7Channels
    {kWICPixelFormat64bpp8Channels,      kBM_8CHANNEL,            FALSE, 8, 0, COLOR_BYTE},         // kWICPixelFormat64bpp8Channels
    {kWICPixelFormat24bpp3Channels,      kBM_RGBTRIPLETS,         FALSE, 3, 0, COLOR_BYTE},         // kWICPixelFormat48bpp3Channels
    {kWICPixelFormat32bpp4Channels,      kBM_CMYKQUADS,           FALSE, 4, 0, COLOR_BYTE},         // kWICPixelFormat64bpp4Channels
    {kWICPixelFormat40bpp5Channels,      kBM_5CHANNEL,            FALSE, 5, 0, COLOR_BYTE},         // kWICPixelFormat80bpp5Channels
    {kWICPixelFormat48bpp6Channels,      kBM_6CHANNEL,            FALSE, 6, 0, COLOR_BYTE},         // kWICPixelFormat96bpp6Channels
    {kWICPixelFormat56bpp7Channels,      kBM_7CHANNEL,            FALSE, 7, 0, COLOR_BYTE},         // kWICPixelFormat112bpp7Channels
    {kWICPixelFormat64bpp8Channels,      kBM_8CHANNEL,            FALSE, 8, 0, COLOR_BYTE},         // kWICPixelFormat128bpp8Channels
    {kWICPixelFormat40bppCMYKAlpha,      kBM_CMYKQUADS,           TRUE,  5, 4, COLOR_BYTE},         // kWICPixelFormat40bppCMYKAlpha
    {kWICPixelFormat40bppCMYKAlpha,      kBM_CMYKQUADS,           TRUE,  5, 4, COLOR_BYTE},         // kWICPixelFormat80bppCMYKAlpha
    {kWICPixelFormat32bpp3ChannelsAlpha, kBM_RGBTRIPLETS,         TRUE,  4, 3, COLOR_BYTE},         // kWICPixelFormat32bpp3ChannelsAlpha
    {kWICPixelFormat40bpp4ChannelsAlpha, kBM_CMYKQUADS,           TRUE,  5, 4, COLOR_BYTE},         // kWICPixelFormat40bpp4ChannelsAlpha
    {kWICPixelFormat48bpp5ChannelsAlpha, kBM_5CHANNEL,            TRUE,  6, 5, COLOR_BYTE},         // kWICPixelFormat48bpp5ChannelsAlpha
    {kWICPixelFormat56bpp6ChannelsAlpha, kBM_6CHANNEL,            TRUE,  7, 6, COLOR_BYTE},         // kWICPixelFormat56bpp6ChannelsAlpha
    {kWICPixelFormat64bpp7ChannelsAlpha, kBM_7CHANNEL,            TRUE,  8, 7, COLOR_BYTE},         // kWICPixelFormat64bpp7ChannelsAlpha
    {kWICPixelFormat72bpp8ChannelsAlpha, kBM_8CHANNEL,            TRUE,  9, 8, COLOR_BYTE},         // kWICPixelFormat72bpp8ChannelsAlpha
    {kWICPixelFormat32bpp3ChannelsAlpha, kBM_RGBTRIPLETS,         TRUE,  4, 3, COLOR_BYTE},         // kWICPixelFormat64bpp3ChannelsAlpha
    {kWICPixelFormat40bpp4ChannelsAlpha, kBM_CMYKQUADS,           TRUE,  5, 4, COLOR_BYTE},         // kWICPixelFormat80bpp4ChannelsAlpha
    {kWICPixelFormat48bpp5ChannelsAlpha, kBM_5CHANNEL,            TRUE,  6, 5, COLOR_BYTE},         // kWICPixelFormat96bpp5ChannelsAlpha
    {kWICPixelFormat56bpp6ChannelsAlpha, kBM_6CHANNEL,            TRUE,  7, 6, COLOR_BYTE},         // kWICPixelFormat112bpp6ChannelsAlpha
    {kWICPixelFormat64bpp7ChannelsAlpha, kBM_7CHANNEL,            TRUE,  8, 7, COLOR_BYTE},         // kWICPixelFormat128bpp7ChannelsAlpha
    {kWICPixelFormat72bpp8ChannelsAlpha, kBM_8CHANNEL,            TRUE,  9, 8, COLOR_BYTE},         // kWICPixelFormat144bpp8ChannelsAlpha
};

/*
Look up table between color data type and data type size
*/
CONST size_t g_lutColorDataSize[] =
{
    0,             // Packing as enumeration starts from 1
    sizeof(BYTE),  // COLOR_BYTE =1
    sizeof(WORD),  // COLOR_WORD
    sizeof(FLOAT), // COLOR_FLOAT
    sizeof(WORD),  // COLOR_S2DOT13FIXED
};

/*
Define a color type that is not valid for WCS - we use this to distinguish
BMFORMATs that do not have a corresponding WCS color type
*/
#define COLOR_INVALID 0

/*
Look up between local BMFORMAT enumeration providing the underlying enumeration
value and the corresponding pixel data size
*/
CONST BMFormatData g_lutBMFormatData[kICMPixelFormatMax] = {
    {BM_x555RGB,             3, static_cast<COLORDATATYPE>(COLOR_INVALID)},
    {BM_x555XYZ,             3, static_cast<COLORDATATYPE>(COLOR_INVALID)},
    {BM_x555Yxy,             3, static_cast<COLORDATATYPE>(COLOR_INVALID)},
    {BM_x555Lab,             3, static_cast<COLORDATATYPE>(COLOR_INVALID)},
    {BM_x555G3CH,            3, static_cast<COLORDATATYPE>(COLOR_INVALID)},
    {BM_RGBTRIPLETS,         3, COLOR_BYTE},
    {BM_BGRTRIPLETS,         3, COLOR_BYTE},
    {BM_XYZTRIPLETS,         3, COLOR_BYTE},
    {BM_YxyTRIPLETS,         3, COLOR_BYTE},
    {BM_LabTRIPLETS,         3, COLOR_BYTE},
    {BM_G3CHTRIPLETS,        3, COLOR_BYTE},
    {BM_5CHANNEL,            5, COLOR_BYTE},
    {BM_6CHANNEL,            6, COLOR_BYTE},
    {BM_7CHANNEL,            7, COLOR_BYTE},
    {BM_8CHANNEL,            8, COLOR_BYTE},
    {BM_GRAY,                1, COLOR_BYTE},
    {BM_xRGBQUADS,           4, COLOR_BYTE},
    {BM_xBGRQUADS,           4, COLOR_BYTE},
    {BM_xG3CHQUADS,          4, COLOR_BYTE},
    {BM_KYMCQUADS,           4, COLOR_BYTE},
    {BM_CMYKQUADS,           4, COLOR_BYTE},
    {BM_10b_RGB,             4, COLOR_BYTE},
    {BM_10b_XYZ,             4, COLOR_BYTE},
    {BM_10b_Yxy,             4, COLOR_BYTE},
    {BM_10b_Lab,             4, COLOR_BYTE},
    {BM_10b_G3CH,            4, COLOR_BYTE},
    {BM_NAMED_INDEX,         4, COLOR_BYTE},
    {BM_16b_RGB,             3, COLOR_WORD},
    {BM_16b_XYZ,             3, COLOR_WORD},
    {BM_16b_Yxy,             3, COLOR_WORD},
    {BM_16b_Lab,             3, COLOR_WORD},
    {BM_16b_G3CH,            3, COLOR_WORD},
    {BM_16b_GRAY,            3, COLOR_WORD},
    {BM_565RGB,              3, COLOR_WORD},
    {BM_32b_scRGB,           3, COLOR_FLOAT},
    {BM_32b_scARGB,          4, COLOR_FLOAT},
    {BM_S2DOT13FIXED_scRGB,  3, COLOR_S2DOT13FIXED},
    {BM_S2DOT13FIXED_scARGB, 4, COLOR_S2DOT13FIXED},
};


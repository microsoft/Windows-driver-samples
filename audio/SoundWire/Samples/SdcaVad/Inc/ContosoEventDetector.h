/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ContosoEventDetector.h

Abstract:

    Sample event detector definitions

Environment:

    Kernel mode

--*/


#pragma once

typedef struct
{
    SOUNDDETECTOR_PATTERNHEADER Header;
    LONGLONG                    ContosoDetectorConfigurationData;
} CONTOSO_KEYWORDCONFIGURATION;

typedef struct
{
    SOUNDDETECTOR_PATTERNHEADER Header;
    LONGLONG                    ContosoDetectorResultData;
    ULONGLONG                   KeywordStartTimestamp;
    ULONGLONG                   KeywordStopTimestamp;
    GUID                        EventId;
} CONTOSO_KEYWORDDETECTIONRESULT;

DEFINE_GUID(CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2,
0x207f3d0c, 0x5c79, 0x496f, 0xa9, 0x4c, 0xd3, 0xd2, 0x93, 0x4d, 0xbf, 0xa9);

// {A537F559-2D67-463B-B10E-BEB750A21F31}
DEFINE_GUID(CONTOSO_KEYWORD1,
0xa537f559, 0x2d67, 0x463b, 0xb1, 0xe, 0xbe, 0xb7, 0x50, 0xa2, 0x1f, 0x31);
// {655E417A-80A5-4A77-B3F1-512EAF67ABCF}
DEFINE_GUID(CONTOSO_KEYWORD2,
0x655e417a, 0x80a5, 0x4a77, 0xb3, 0xf1, 0x51, 0x2e, 0xaf, 0x67, 0xab, 0xcf);


/*
    Copyright (c) Microsoft Corporation All Rights Reserved

    Contoso voice activation driver definitions

    Hardware manufacturers define identifiers and data structures specific to
    their detection technology to pass voice models, keyword data, speaker
    data, or any other data relevant to their technology.

*/

//
// Identifier for Contoso keyword configuration data.
//
// {6F7DBCC1-202E-498D-99C5-61C36C4EB2DC}
DEFINE_GUID(CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER, 0x6f7dbcc1, 0x202e, 0x498d, 0x99, 0xc5, 0x61, 0xc3, 0x6c, 0x4e, 0xb2, 0xdc);

// {207F3D0C-5C79-496F-A94C-D3D2934DBFA9}
DEFINE_GUID(CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2, 0x207f3d0c, 0x5c79, 0x496f, 0xa9, 0x4c, 0xd3, 0xd2, 0x93, 0x4d, 0xbf, 0xa9);

// {A537F559-2D67-463B-B10E-BEB750A21F31}
DEFINE_GUID(CONTOSO_KEYWORD1, 0xa537f559, 0x2d67, 0x463b, 0xb1, 0xe, 0xbe, 0xb7, 0x50, 0xa2, 0x1f, 0x31);
// {655E417A-80A5-4A77-B3F1-512EAF67ABCF}
DEFINE_GUID(CONTOSO_KEYWORD2, 0x655e417a, 0x80a5, 0x4a77, 0xb3, 0xf1, 0x51, 0x2e, 0xaf, 0x67, 0xab, 0xcf);

//
// The format of the Contoso keyword pattern matching data.
//
typedef struct
{
    SOUNDDETECTOR_PATTERNHEADER Header;
    LONGLONG                    ContosoDetectorConfigurationData;
} CONTOSO_KEYWORDCONFIGURATION;

//
// The format of the Contoso match result data.
//
typedef struct
{
    SOUNDDETECTOR_PATTERNHEADER Header;
    LONGLONG                    ContosoDetectorResultData;
    ULONGLONG                   KeywordStartTimestamp;
    ULONGLONG                   KeywordStopTimestamp;
    GUID                        EventId;
} CONTOSO_KEYWORDDETECTIONRESULT;

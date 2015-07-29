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
} CONTOSO_KEYWORDDETECTIONRESULT;

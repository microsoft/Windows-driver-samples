/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xps.h

Abstract:

   Enumerations and definitions used by the stream filter.

--*/

#pragma once

#include "xdstring.h"
#include "ipkfile.h"

enum ERelsType
{
    RelsAnnotations = 0, ERelsTypeMin = 0,
    RelsDigitalSignatureDefinitions,
    RelsDiscardControl,
    RelsDocumentStructure,
    RelsPrintTicket,
    RelsRequiredResource,
    RelsRestrictedFont,
    RelsStartPart,
    RelsStoryFragments,
    RelsCoreProperties,
    RelsDigitalSignature,
    RelsDigitalSignatureCertificate,
    RelsDigitalSignatureOrigin,
    RelsThumbnail,
    RelsUnknown, ERelsTypeMax = RelsUnknown
};

typedef std::pair<CStringXDA, ERelsType>   RelsNameType;
typedef std::vector<RelsNameType>        RelsTypeList;
typedef std::map<CStringXDA, RelsTypeList> RelsMap;

enum EContentType
{
    ContentFixedDocumentSequence = 0, EContentTypeMin = 0,
    ContentFixedDocument,
    ContentFixedPage,
    ContentDiscardControl,
    ContentDocumentStructure,
    ContentFont,
    ContentICCProfile,
    ContentObfuscatedFont,
    ContentPrintTicket,
    ContentRemoteResourceDictionary,
    ContentStoryFragments,
    ContentJPEGImage,
    ContentPNGImage,
    ContentTIFFImage,
    ContentWindowsMediaPhotoImage,
    ContentCoreProperties,
    ContentDigitalSignatureCertificate,
    ContentDigitalSignatureOrigin,
    ContentDigitalSignatureXMLSignature,
    ContentRelationships,
    ContentUnknown, EContentTypeMax = ContentUnknown
};

typedef std::map<CStringXDA, EContentType> ContentMap;

typedef std::vector<CStringXDA> FileList;

typedef std::map<CStringXDA, BOOL> SentList;

typedef std::pair <CONST IPKFile*, BOOL> RecordTracker;
typedef std::vector<RecordTracker>       XPSPartStack;


/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   rescpy.h

Abstract:

   Page resource copy class definition. This class stores resources
   from one page and copies them to a destination. This is required when
   copying page markup into a new page - without doing so any resources
   referenced in the source page are lost.

--*/

#pragma once

class CResourceCopier
{
public:
    CResourceCopier();

    virtual ~CResourceCopier();

    HRESULT
    CopyPageResources(
        _In_    CONST IFixedPage* pFPSrc,
        _Inout_ IFixedPage*       pFPDst
        );
};


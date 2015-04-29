/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmflt.h

Abstract:

   Implementation of the color management filter which handles page parts from the
   XPS document container.

--*/

#pragma once

#include "xdrchflt.h"
#include "ptmanage.h"

typedef map<CStringXDW, BOOL> ResDeleteMap;

class CColorManageFilter : public CXDXpsFilter
{
public:
    CColorManageFilter();

    virtual ~CColorManageFilter();

private:
    virtual HRESULT
    ProcessPart(
        _Inout_ IFixedPage* pFP
        );

private:

    //
    // Create a resource cache object to manage the storage of bitmaps and
    // color profiles in the container
    //
    CFileResourceCache m_resCache;
};


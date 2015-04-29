/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colppg.h

Abstract:

   Definition of the color management property page. This class is
   responsible for initialising and registering the color management
   property page and its controls.

--*/

#pragma once

#include "precomp.h"
#include "docppg.h"

class CColorPropPage : public CDocPropPage
{
public:
    CColorPropPage();

    virtual ~CColorPropPage();

    HRESULT
    InitDlgBox(
        _Out_ LPCTSTR* ppszTemplate,
        _Out_ LPCTSTR* ppszTitle
        );
};


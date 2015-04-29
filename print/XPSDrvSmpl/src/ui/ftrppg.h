/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ftrppg.h

Abstract:

   Definition of the feature property page. This class is
   responsible for initialising and registering the features
   property page and its controls.

--*/

#pragma once

#include "precomp.h"
#include "docppg.h"

class CFeaturePropPage : public CDocPropPage
{
public:
    CFeaturePropPage();

    virtual ~CFeaturePropPage();

    HRESULT
    InitDlgBox(
        _Out_ LPCTSTR* ppszTemplate,
        _Out_ LPCTSTR* ppszTitle
        );
};


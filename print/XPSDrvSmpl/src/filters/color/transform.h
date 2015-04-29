/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   transform.h

Abstract:

   CTransform class definition. This class creates and manages color transforms providing
   limited caching functionality based off the source profile keys.


--*/

#pragma once

#include "profile.h"

typedef vector<CProfile*> ProfileList;

class CTransform
{
public:
    CTransform();

    ~CTransform();

    HRESULT
    CreateTransform(
        _In_ ProfileList* pProfiles,
        _In_ CONST DWORD  intent,
        _In_ CONST DWORD  renderFlags
        );

    HRESULT
    GetTransformHandle(
        _Out_ HTRANSFORM* phTrans
        );


private:
    VOID
    FreeTransform(
        VOID
        );

    HRESULT
    CreateProfileKeysBuffer(
        _In_ CONST UINT& cProfiles
        );

    VOID
    FreeProfileKeysBuffer(
        VOID
        );

    HRESULT
    UpdateProfiles(
        _In_  ProfileList* pProfiles,
        _Out_ BOOL*        pbUpdate
        );

private:
    HTRANSFORM  m_hColorTrans;

    DWORD       m_intents;

    DWORD       m_renderFlags;

    CStringXDW* m_pcstrProfileKeys;

    DWORD       m_cProfiles;
};


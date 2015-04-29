/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   colconv.h

Abstract:

   Color conversion class definition. The CColorConverter class is responsible
   for managing the color conversion process for vector objects and bitmaps.

--*/

#pragma once

#include "colchan.h"
#include "rescache.h"
#include "profman.h"
#include "wcsapiconv.h"
#include "cmflt.h"

class CColorConverter
{
public:
    CColorConverter(
        _In_ IXpsDocumentConsumer* pXpsConsumer,
        _In_ IFixedPage*           pFixedPage,
        _In_ CFileResourceCache*   pResCache,
        _In_ CProfileManager*      pProfManager,
        _In_ ResDeleteMap*         pResDel
        );

    virtual ~CColorConverter();

    virtual HRESULT
    Convert(
        _Inout_ BSTR* pbstr
        ) = 0;

protected:
    CComPtr<IXpsDocumentConsumer> m_pXpsConsumer;

    CComPtr<IFixedPage>           m_pFixedPage;

    CProfileManager*              m_pProfManager;

    CFileResourceCache*           m_pResCache;

    ResDeleteMap*                 m_pResDel;
};

class CBitmapColorConverter : public CColorConverter
{
public:
    CBitmapColorConverter(
        _In_ IXpsDocumentConsumer* pXpsConsumer,
        _In_ IFixedPage*           pFixedPage,
        _In_ CFileResourceCache*   pResCache,
        _In_ CProfileManager*      pProfManager,
        _In_ ResDeleteMap*         pResDel
        );

    virtual ~CBitmapColorConverter();

    HRESULT
    Convert(
        _Inout_ BSTR* pbstrBmpURI
        );
};

class CColorRefConverter : public CColorConverter
{
public:
    CColorRefConverter(
        _In_ IXpsDocumentConsumer* pXpsConsumer,
        _In_ IFixedPage*           pFixedPage,
        _In_ CFileResourceCache*   pResCache,
        _In_ CProfileManager*      pProfManager,
        _In_ ResDeleteMap*         pResDel
        );

    virtual ~CColorRefConverter();

    HRESULT
    Convert(
        _Inout_ BSTR* pbstrColorRef
        );

private:
    HRESULT
    InitDstChannels(
        _In_    CColorChannelData* pChannelDataSrc,
        _Inout_ CColorChannelData* pChannelDataDst
        );

    HRESULT
    GetFloatChannelData(
        _In_    LPCWSTR            szColorRef,
        _Inout_ CColorChannelData* pChannelData
        );

    HRESULT
    SetFloatChannelData(
        _In_    CColorChannelData* pChannelData,
        _Inout_ CStringXDW*        pcstrChannelData
        );

    HRESULT
    ParseColorString(
        _In_    BSTR               bstrColorRef,
        _Inout_ CColorChannelData* pChannelData,
        _Out_   BOOL*              pbIsResourceReference
        );

    HRESULT
    TransformColor(
        _In_    CColorChannelData* pSrcData,
        _Inout_ CColorChannelData* pDstData
        );

    HRESULT
    CreateColorString(
        _In_    CColorChannelData* pDstData,
        _Inout_ BSTR*              pbstrColorRef
        );

private:
    CColorChannelData m_srcData;

    CColorChannelData m_dstData;
};

class CResourceDictionaryConverter : public CColorConverter
{
public:
    CResourceDictionaryConverter(
        _In_ IXpsDocumentConsumer*  pXpsConsumer,
        _In_ IFixedPage*            pFixedPage,
        _In_ CFileResourceCache*    pResCache,
        _In_ CProfileManager*       pProfManager,
        _In_ ResDeleteMap*          pResDel,
        _In_ CBitmapColorConverter* pBmpConv,
        _In_ CColorRefConverter*    pRefConv
        );

    virtual ~CResourceDictionaryConverter();

    HRESULT
    Convert(
        _Inout_ BSTR* pbstrDictionaryURI
        );

private:
    CBitmapColorConverter* m_pBmpConv;

    CColorRefConverter*    m_pRefConv;
};


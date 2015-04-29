/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmbase.h

Abstract:

   Base watermark class implementation. The base watermark class provides
   common functionality required between different watermarks (Text, RasterGraphic
   and VectorGraphic. This includes methods for converting a GDI matrix object
   into the appropriate XPS matrix mark-up and intialising the matrix according to
   the watermark options.

--*/

#pragma once

#include "rescache.h"
#include "wmptprop.h"

class CWatermark
{
public:
    CWatermark(
        _In_ CONST CWMPTProperties& wmProps
        );

    virtual ~CWatermark();

    virtual HRESULT
    CreateXMLElement(
        VOID
        ) = 0;

    virtual HRESULT
    AddParts(
        _In_ IXpsDocumentConsumer* pXpsConsumer,
        _In_ IFixedPage*           pFixedPage,
        _In_ CFileResourceCache*   pResCache

        ) = 0;

    virtual HRESULT
    GetXML(
        _Outptr_ BSTR* pbstrXML
        );

    virtual BOOL
    InsertStart(
        VOID
        );

    virtual BOOL
    InsertEnd(
        VOID
        );

protected:
    HRESULT
    CreateWMTransform(
        _In_        RectF wmBounds,
        _Outptr_ BSTR* pbstrMatrixXForm
        );

    HRESULT
    CreateWMTransform(
        _In_        PointF wmOrigin,
        _Outptr_ BSTR*  pbstrMatrixXForm
        );

private:
    HRESULT
    MatrixToXML(
        _In_        CONST Matrix* pMatrix,
        _Outptr_ BSTR*         pbstrMatrixXForm
        );

protected:
    CComPtr<IXMLDOMDocument2> m_pDOMDoc;

    CComPtr<IXMLDOMElement>   m_pWMElem;

    CWMPTProperties           m_WMProps;
};


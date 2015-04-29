/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   scalepage.h

Abstract:

   Page Scaling class implementation. The Page Scaling class provides
   functionality required to generate the XPS markup to perform page scaling.
   This includes methods for converting a GDI matrix object
   into the appropriate XPS matrix mark-up and intialising the matrix according to
   the Page Scaling options.

--*/

#pragma once

#include "pgscptprop.h"

class CPageScaling
{
public:
    CPageScaling(
        _In_ CONST CPGSCPTProperties& psProps
        );

    virtual ~CPageScaling();

    HRESULT
    GetFixedPageWidth(
        _Outptr_ BSTR* pbstrWidth
        );

    HRESULT
    GetFixedPageHeight(
        _Outptr_ BSTR* pbstrHeight
        );

    HRESULT
    GetOpenTagXML(
        _Outptr_ BSTR* pbstrXML
        );

    HRESULT
    GetCloseTagXML(
        _Outptr_ BSTR* pbstrXML
        );

    HRESULT
    SetPageDimensions(
        _In_ CONST REAL width,
        _In_ CONST REAL height
        );

    HRESULT
    SetBleedBox(
        _In_ RectF* pBleedBox
        );

    HRESULT
    SetContentBox(
        _In_ RectF* pContentBox
        );

private:
    HRESULT
    CreateTransform(
        _Outptr_ BSTR* pbstrMatrixXForm
        );

    HRESULT
    CalculateMatrix(
        _Inout_ Matrix* pMatrix
    );

    HRESULT
    MatrixToXML(
        _In_        CONST Matrix* pMatrix,
        _Outptr_ BSTR*         pbstrMatrixXForm
        );

private:
    CComPtr<IXMLDOMDocument2> m_pDOMDoc;

    CComPtr<IXMLDOMElement>   m_pCanvasElem;

    CPGSCPTProperties         m_PGSCProps;

    RectF                     m_bleedBox;

    RectF                     m_contentBox;

    SizeF                     m_pageDimensions;

    BOOL                      m_bAddCanvas;
};


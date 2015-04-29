/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmpthnlr.h

Abstract:

   PageWatermark PrintTicket handling definition. The watermark PT handler
   is used to extract watermark settings from a PrintTicket and populate
   the watermark properties class with the retrieved settings. The class also
   defines a method for setting the feature in the PrintTicket given the
   data structure.

--*/

#pragma once

#include "pthndlr.h"
#include "wmdata.h"

class CWMPTHandler : public CPTHandler
{
public:
    CWMPTHandler(
        _In_ IXMLDOMDocument2* pPrintTicket
        );

    virtual ~CWMPTHandler();

    HRESULT
    GetData(
        _Out_ XDPrintSchema::PageWatermark::WatermarkData* pWmData
        );

    HRESULT
    SetData(
        _In_ CONST XDPrintSchema::PageWatermark::WatermarkData* pWmData
        );

    HRESULT
    Delete(
        VOID
        );

private:
    HRESULT
    CreateCommonWMElements(
        _In_        CONST XDPrintSchema::PageWatermark::WatermarkData* pWmData,
        _Outptr_ IXMLDOMElement**                                   ppWMDataElem,
        _Outptr_ IXMLDOMElement**                                   ppOptionElem,
        _Out_       PTDOMElementVector*                                pParamInitList
        );

    HRESULT
    CreateCommonVectBmpWMElements(
        _In_  CONST XDPrintSchema::PageWatermark::WatermarkData* pWmData,
        _In_  IXMLDOMElement*                                    pOptionElem,
        _Out_ PTDOMElementVector*                                pParamInitList
        );

    HRESULT
    CreateTextWMElements(
        _In_        CONST XDPrintSchema::PageWatermark::WatermarkData* pWmData,
        _Outptr_ IXMLDOMElement**                                   ppWMDataElem,
        _Out_       PTDOMElementVector*                                pParamInitList
        );

    HRESULT
    CreateBitmapWMElements(
        _In_        CONST       XDPrintSchema::PageWatermark::WatermarkData* pWmData,
        _Outptr_ IXMLDOMElement**                                         ppWMDataElem,
        _Out_       PTDOMElementVector*                                      pParamInitList
        );

    HRESULT
    CreateVectorWMElements(
        _In_        CONST XDPrintSchema::PageWatermark::WatermarkData* pWmData,
        _Outptr_ IXMLDOMElement**                                   ppWMDataElem,
        _Out_       PTDOMElementVector*                                pParamInitList
        );

    HRESULT
    GetCmnPropTypeAndValue(
        _In_        CONST XDPrintSchema::PageWatermark::WatermarkData*        pWmData,
        _In_        CONST XDPrintSchema::PageWatermark::ECommonWatermarkProps cmnProps,
        _Outptr_ BSTR*                                                     pbstrType,
        _Outptr_ BSTR*                                                     pbstrValue
        );

    HRESULT
    GetCmnVectBmpPropTypeAndValue(
        _In_        CONST XDPrintSchema::PageWatermark::WatermarkData*         pWmData,
        _In_        CONST XDPrintSchema::PageWatermark::EVectBmpWatermarkProps cmnProps,
        _Outptr_ BSTR*                                                      pbstrType,
        _Outptr_ BSTR*                                                      pbstrValue
        );

    HRESULT
    GetTxtPropTypeAndValue(
        _In_        CONST XDPrintSchema::PageWatermark::WatermarkData*      pWmData,
        _In_        CONST XDPrintSchema::PageWatermark::ETextWatermarkProps txtProps,
        _Outptr_ BSTR*                                                   pbstrType,
        _Outptr_ BSTR*                                                   pbstrValue
        );
};


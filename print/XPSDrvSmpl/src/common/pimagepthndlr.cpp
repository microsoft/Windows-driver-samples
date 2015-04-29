/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pimagepthndlr.cpp

Abstract:

   This class is responsible for retrieving the PageImageableSize properties from a
   PrintCapabilities document.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "ptquerybld.h"
#include "pimagepthndlr.h"

using XDPrintSchema::PageImageableSize::PageImageableData;
using XDPrintSchema::PageImageableSize::ImageableSizeWidth;
using XDPrintSchema::PageImageableSize::ImageableSizeHeight;
using XDPrintSchema::PageImageableSize::ImageableArea;
using XDPrintSchema::PageImageableSize::EPageImageablePropsMax;
using XDPrintSchema::PageImageableSize::OriginWidth;
using XDPrintSchema::PageImageableSize::OriginHeight;
using XDPrintSchema::PageImageableSize::ExtentWidth;
using XDPrintSchema::PageImageableSize::ExtentHeight;
using XDPrintSchema::PageImageableSize::PAGE_IMAGEABLE_PROPERTY;
using XDPrintSchema::PageImageableSize::PAGE_IMAGEABLE_PROPS;
using XDPrintSchema::PageImageableSize::PAGE_IMAGEABLE_PROPS_AREA;

/*++

Routine Name:

    CPageImageablePCHandler::CPageImageablePCHandler

Routine Description:

    CPageImageablePCHandler class constructor

Arguments:

    pPrintCapabilities - Pointer to the DOM document representation of the PrintCapabilities

Return Value:

    None

--*/
CPageImageablePCHandler::CPageImageablePCHandler(
    _In_ IXMLDOMDocument2* pPrintCapabilities
    ) :
    CPCHandler(pPrintCapabilities)
{
}

/*++

Routine Name:

    CPageImageablePCHandler::~CPageImageablePCHandler

Routine Description:

    CPageImageablePCHandler class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageImageablePCHandler::~CPageImageablePCHandler()
{
}

/*++

Routine Name:

    CPageImageablePCHandler::GetData

Routine Description:

    The routine fills the data structure passed in with page imageable size
    data retrieved from the PrintCapabilities passed to the class constructor.

Arguments:

    pPageImageableData - Pointer to the page imageable size data structure to be filled in

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - Feature not present in PrintCapabilities
    E_*                 - On error

--*/
HRESULT
CPageImageablePCHandler::GetData(
    _Inout_ PageImageableData* pPageImageableData
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPageImageableData, E_POINTER)))
    {
        CComBSTR        bstrQuery;
        CPTQueryBuilder queryImageableArea(m_bstrFrameworkPrefix);

        if (SUCCEEDED(hr = queryImageableArea.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPERTY))) &&
            SUCCEEDED(hr = queryImageableArea.GetQuery(&bstrQuery)) &&
            SUCCEEDED(hr = QueryNode(bstrQuery)))
        {
            CPTQueryBuilder queryImageableSizeWidth(queryImageableArea);
            CPTQueryBuilder queryImageableSizeHeight(queryImageableArea);

            if (SUCCEEDED(hr = queryImageableSizeWidth.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS[ImageableSizeWidth]))) &&
                SUCCEEDED(hr = queryImageableSizeHeight.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS[ImageableSizeHeight]))) &&
                SUCCEEDED(hr = queryImageableSizeWidth.GetQuery(&bstrQuery)) &&
                SUCCEEDED(hr = QueryNodeValue(bstrQuery, &pPageImageableData->imageableSizeWidth)) &&
                SUCCEEDED(hr = queryImageableSizeHeight.GetQuery(&bstrQuery)) &&
                SUCCEEDED(hr = QueryNodeValue(bstrQuery, &pPageImageableData->imageableSizeHeight)))
            {
                CPTQueryBuilder queryOriginWidth(queryImageableArea);
                CPTQueryBuilder queryOriginHeight(queryImageableArea);
                CPTQueryBuilder queryExtentWidth(queryImageableArea);
                CPTQueryBuilder queryExtentHeight(queryImageableArea);

                if (SUCCEEDED(hr = queryOriginWidth.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS[ImageableArea]))) &&
                    SUCCEEDED(hr = queryOriginWidth.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS_AREA[OriginWidth]))) &&
                    SUCCEEDED(hr = queryOriginHeight.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS[ImageableArea]))) &&
                    SUCCEEDED(hr = queryOriginHeight.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS_AREA[OriginHeight]))) &&
                    SUCCEEDED(hr = queryExtentWidth.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS[ImageableArea]))) &&
                    SUCCEEDED(hr = queryExtentWidth.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS_AREA[ExtentWidth]))) &&
                    SUCCEEDED(hr = queryExtentHeight.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS[ImageableArea]))) &&
                    SUCCEEDED(hr = queryExtentHeight.AddProperty(m_bstrKeywordsPrefix, CComBSTR(PAGE_IMAGEABLE_PROPS_AREA[ExtentHeight]))) &&
                    SUCCEEDED(hr = queryOriginWidth.GetQuery(&bstrQuery)) &&
                    SUCCEEDED(hr = QueryNodeValue(bstrQuery, &pPageImageableData->originWidth)) &&
                    SUCCEEDED(hr = queryOriginHeight.GetQuery(&bstrQuery)) &&
                    SUCCEEDED(hr = QueryNodeValue(bstrQuery, &pPageImageableData->originHeight)) &&
                    SUCCEEDED(hr = queryExtentWidth.GetQuery(&bstrQuery)) &&
                    SUCCEEDED(hr = QueryNodeValue(bstrQuery, &pPageImageableData->extentWidth)) &&
                    SUCCEEDED(hr = queryExtentHeight.GetQuery(&bstrQuery)))
                {
                    hr = QueryNodeValue(bstrQuery, &pPageImageableData->extentHeight);
                }
            }
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}


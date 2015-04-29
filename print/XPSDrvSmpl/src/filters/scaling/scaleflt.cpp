/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   scaleflt.cpp

Abstract:

   Page Scaling filter implementation. This class derives from the Xps filter
   class and implements the necessary part handlers to support Page Scaling
   printing. The Page Scaling filter is responsible for modifying the XPS document
   to add the appropriate mark-up onto pages to achieve scaling.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "scaleflt.h"
#include "pagescale.h"
#include "scalesax.h"
#include "psizepthndlr.h"
#include "pimagepthndlr.h"
#include "pgscpthndlr.h"
#include "porientpthndlr.h"

using XDPrintSchema::PageScaling::PageScalingData;
using XDPrintSchema::PageMediaSize::PageMediaSizeData;
using XDPrintSchema::PageImageableSize::PageImageableData;
using XDPrintSchema::PageOrientation::PageOrientationData;

/*++

Routine Name:

    CPageScalingFilter::CPageScalingFilter

Routine Description:

    CPageScalingFilter class constructor

Arguments:

    None

Return Value:

    None

--*/
CPageScalingFilter::CPageScalingFilter()
{
    ASSERTMSG(m_gdiPlus.GetGDIPlusStartStatus() == Ok, "GDI plus is not correctly initialized.\n");
}

/*++

Routine Name:

    CPageScalingFilter::~CPageScalingFilter

Routine Description:

    CPageScalingFilter class destructor

Arguments:

    None

Return Value:

    None

--*/
CPageScalingFilter::~CPageScalingFilter()
{
}

/*++

Routine Name:

    CPageScalingFilter::ProcessFixedPage

Routine Description:

    This method processes the XPS content of a page applying
    any scaling required according to information in the PrintTicket.
    The result is written out to a new XPS document.

Arguments:

    pPrintTicket - Pointer to the XML PrintTicket Document that is associated with the page.
    pPageReadStream - Pointer to the input stream interface for the XPS Page.
    pPageWriteStream - Pointer to the output stream interface for the XPS Page.

Return Value:

    HRESULT
    S_OK                                    - On success
    HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) - When the fixed page is not to be modified
    E_*                                     - On error

--*/
HRESULT
CPageScalingFilter::ProcessFixedPage(
    _In_  IXMLDOMDocument2*  pFPPT,
    _In_  ISequentialStream* pPageReadStream,
    _In_  ISequentialStream* pPageWriteStream
    )
{
    VERBOSE("Processing stream fixed page with page scaling handler\n");

    HRESULT hr = S_OK;

    CPageScaling*          pPageScaling = NULL;
    CComPtr<ISAXXMLReader> pSaxRdr(NULL);
    CComPtr<IXMLDOMDocument2> pPrintCapabilities(NULL);

    if (SUCCEEDED(hr = m_ptManager.GetCapabilities(pFPPT, &pPrintCapabilities)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFPPT, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPageReadStream, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pPageWriteStream, E_POINTER)) &&
        SUCCEEDED(hr = GetPageScaling(pFPPT, pPrintCapabilities, &pPageScaling)) &&
        SUCCEEDED(hr = pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)))
    {
        //
        // Set-up the SAX reader and begin parsing the mark-up
        //
        CScaleSaxHandler scaleSaxHndlr(pPageWriteStream, pPageScaling);

        if (SUCCEEDED(hr = pSaxRdr->putContentHandler(&scaleSaxHndlr)))
        {
            hr = pSaxRdr->parse(CComVariant(pPageReadStream));
        }
    }
    else if (hr == E_ELEMENT_NOT_FOUND)
    {
        //
        // No Page Scaling was found in the PrintTicket - return error not supported so
        // the xps container sends the unmodified data
        //
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    //
    // Clean up the Page Scaling if it was successfully created
    //
    if (pPageScaling != NULL)
    {
        delete pPageScaling;
        pPageScaling = NULL;
    }

    ERR_ON_HR_EXC(hr, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    return hr;
}

/*++

Routine Name:

    CPageScalingFilter::GetPageScaling

Routine Description:

    Creates and initialises an instance of the Page Scaling Interface
    using the information in the PrintTicket provided.

Arguments:

    pPrintTicket       - Pointer to an XML PrintTicket Document.
    pPrintCapabilities - Pointer to an XML PrintCapabilities Document.
    ppPageScaling      - Address of pointer that receives the Page Scaling Interface.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPageScalingFilter::GetPageScaling(
    _In_            IXMLDOMDocument2*  pPrintTicket,
    _In_            IXMLDOMDocument2*  pPrintCapabilities,
    _Outptr_    CPageScaling**     ppPageScaling
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppPageScaling, E_POINTER)))
    {
        *ppPageScaling = NULL;

        try
        {
            CPageScalingPTHandler     pgscPTHandler(pPrintTicket);
            CPageImageablePCHandler   imagePCHandler(pPrintCapabilities);
            CPageSizePTHandler        sizePTHandler(pPrintTicket);
            CPageOrientationPTHandler orientPTHandler(pPrintTicket);

            PageScalingData pgscData;
            PageMediaSizeData sizeData;
            PageImageableData imageData;
            PageOrientationData orientData;

            if (SUCCEEDED(hr = pgscPTHandler.GetData(&pgscData)) &&
                SUCCEEDED(hr = sizePTHandler.GetData(&sizeData)) &&
                SUCCEEDED(hr = orientPTHandler.GetData(&orientData)))
            {
                hr = imagePCHandler.GetData(&imageData);

                //
                // The imageable area property in a PrintTicket is an optional requirement.
                //
                if (hr == E_ELEMENT_NOT_FOUND)
                {
                    hr = S_OK;
                }

                if (SUCCEEDED(hr))
                {
                    CPGSCPTProperties pgscProperties(pgscData, sizeData, imageData, orientData);

                    *ppPageScaling = new(std::nothrow) CPageScaling(pgscProperties);

                    if (*ppPageScaling == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR_EXC(hr, E_ELEMENT_NOT_FOUND);
    return hr;
}


/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmflt.cpp

Abstract:

   Watermark filter implementation. This class derives from the Xps filter
   class and implements the necessary part handlers to support Watermark
   printing. The Watermark filter is responsible for adding resources to
   the XPS document and putting the appropriate mark-up onto pages with a
   watermark.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "wmflt.h"
#include "wmtext.h"
#include "wmrast.h"
#include "wmvect.h"
#include "wmsax.h"
#include "wmpthndlr.h"
#include "wmres.h"

using XDPrintSchema::PageWatermark::WatermarkData;
using XDPrintSchema::PageWatermark::EWatermarkOption;
using XDPrintSchema::PageWatermark::TextWatermark;
using XDPrintSchema::PageWatermark::BitmapWatermark;
using XDPrintSchema::PageWatermark::VectorWatermark;

/*++

Routine Name:

    CWatermarkFilter::CWatermarkFilter

Routine Description:

    Default constructor for the watermark filter which ensures GDI plus is correctly running

Arguments:

    None

Return Value:

    None

--*/
CWatermarkFilter::CWatermarkFilter()
{
    ASSERTMSG(m_gdiPlus.GetGDIPlusStartStatus() == Ok, "GDI plus is not correctly initialized.\n");
}

/*++

Routine Name:

    CWatermarkFilter::~CWatermarkFilter

Routine Description:

    Default destructor for the watermark filter

Arguments:

    None

Return Value:

    None

--*/
CWatermarkFilter::~CWatermarkFilter()
{
}

/*++

Routine Name:

    CWatermarkFilter::ProcessPart

Routine Description:

    Method for processing each fixed page part in a container

Arguments:

    pFP - Pointer to the fixed page to process

Return Value:

    HRESULT
    S_OK    - On success
    S_FALSE - When not enabled in the PT
    E_*     - On error

--*/
HRESULT
CWatermarkFilter::ProcessPart(
    _Inout_ IFixedPage* pFP
    )
{
    VERBOSE("Processing Fixed Page part with watermark handler\n");

    HRESULT hr = S_OK;
    CWatermark* pWatermark = NULL;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)))
    {
        //
        // Retrieve the watermark settings from the PrintTicket
        //
        IXMLDOMDocument2* pPT = NULL;
        if (SUCCEEDED(hr = m_ptManager.SetTicket(pFP)) &&
            SUCCEEDED(hr = m_ptManager.GetTicket(kPTPageScope, &pPT)) &&
            SUCCEEDED(hr = GetWatermark(pPT, &pWatermark)) &&
            SUCCEEDED(hr = CHECK_POINTER(pWatermark, E_FAIL)))
        {
            //
            // Add the resource part
            //
            if (SUCCEEDED(hr = pWatermark->AddParts(m_pXDWriter, pFP, &m_resCache)))
            {
                //
                // Retrieve the writer from the fixed page
                //
                CComPtr<IPrintWriteStream>  pWriter(NULL);

                if (SUCCEEDED(hr = pFP->GetWriteStream(&pWriter)))
                {
                    //
                    // Set-up the SAX reader and begin parsing the mark-up
                    //
                    CComPtr<ISAXXMLReader>    pSaxRdr(NULL);
                    CComPtr<IPrintReadStream> pReader(NULL);

                    try
                    {
                        CWMSaxHandler wmSaxHndlr(pWriter, pWatermark);

                        if (SUCCEEDED(hr = pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)) &&
                            SUCCEEDED(hr = pSaxRdr->putContentHandler(&wmSaxHndlr)) &&
                            SUCCEEDED(hr = pFP->GetStream(&pReader)))
                        {
                            CComPtr<ISequentialStream> pReadStreamToSeq(NULL);

                            pReadStreamToSeq.Attach(new(std::nothrow) pfp::PrintReadStreamToSeqStream(pReader));

                            if (SUCCEEDED(hr = CHECK_POINTER(pReadStreamToSeq, E_OUTOFMEMORY)))
                            {
                                hr = pSaxRdr->parse(CComVariant(static_cast<ISequentialStream*>(pReadStreamToSeq)));
                            }
                        }
                    }
                    catch (CXDException& e)
                    {
                        hr = e;
                    }

                    pWriter->Close();
                }
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                //
                // Could not find resource file - fail gracefully so we continue
                // to process the document
                //
                ERR("Specified resource file does not exist\n");
                hr = S_FALSE;
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                //
                // Insufficient rights to open file - fail gracefully so we continue
                // to process the document
                //
                ERR("Insufficient rights to open resource file\n");
                hr = S_FALSE;
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
            {
                //
                // We do not support this URI as a resource - fail gracefully so we continue
                // to process the document
                //
                ERR("Unsupported URI to resource resource\n");
                hr = S_FALSE;
            }
        }
        else if (hr == E_ELEMENT_NOT_FOUND)
        {
            //
            // No watermark was found in the PrintTicket - do not propogate this error
            //
            hr = S_FALSE;
        }
    }

    //
    // Clean up the watermark if it was successfully created
    //
    if (pWatermark != NULL)
    {
        delete pWatermark;
        pWatermark = NULL;
    }

    if (SUCCEEDED(hr))
    {
        //
        // We can send the fixed page
        //
        hr = m_pXDWriter->SendFixedPage(pFP);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CWatermarkFilter::GetWatermark

Routine Description:

    Method for obtaining the watermark PrintTicket preferences

Arguments:

    pPrintTicket - DOM document containing the PrintTicket
    ppWatermark  - Pointer to object which will contain the values
                   read from the PrintTicket

Return Value:

    HRESULT
    S_OK                - On success
    E_ELEMENT_NOT_FOUND - When feature not present in the PT
    E_*                 - On error

--*/
HRESULT
CWatermarkFilter::GetWatermark(
    _In_            IXMLDOMDocument2*  pPrintTicket,
    _Outptr_result_maybenull_ CWatermark**       ppWatermark
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(ppWatermark, E_POINTER)))
    {
        *ppWatermark = NULL;

        try
        {

            CWMPTHandler  wmPTHandler(pPrintTicket);
            WatermarkData wmData;

            if (SUCCEEDED(hr = wmPTHandler.GetData(&wmData)))
            {
                CWMPTProperties wmProperties(wmData);

                EWatermarkOption wmOption;

                if (SUCCEEDED(hr = wmProperties.GetType(&wmOption)))
                {
                    switch (wmOption)
                    {
                        case TextWatermark:
                        {
                            *ppWatermark = new(std::nothrow) CTextWatermark(wmProperties);

                            if (*ppWatermark == NULL)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        break;

                        case BitmapWatermark:
                        {
                            *ppWatermark = new(std::nothrow) CRasterWatermark(wmProperties, IDR_WM_PNG1);

                            if (*ppWatermark == NULL)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        break;

                        case VectorWatermark:
                        {
                            *ppWatermark = new(std::nothrow) CVectorWatermark(wmProperties, IDR_WM_XPS1);

                            if (*ppWatermark == NULL)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        break;

                        default:
                        {
                            hr = E_ELEMENT_NOT_FOUND;
                        }
                        break;
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


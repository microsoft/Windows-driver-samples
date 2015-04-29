/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmflt.cpp

Abstract:

   Color management filter implementation. This class derives from the Xps filter
   class and implements the necessary part handlers to support color management.
   The color management filter is responsible for adding and removing resources to
   and from the XPS document and putting the appropriate mark-up onto pages when applying
   a color transform.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "pthndlr.h"
#include "cmflt.h"
#include "cmsax.h"
#include "colconv.h"
#include "cmpthndlr.h"
#include "cmintpthndlr.h"
#include "cmprofpthndlr.h"

using XDPrintSchema::PageColorManagement::ColorManagementData;
using XDPrintSchema::PageColorManagement::Driver;

using XDPrintSchema::PageSourceColorProfile::PageSourceColorProfileData;

using XDPrintSchema::PageICMRenderingIntent::PageICMRenderingIntentData;

/*++

Routine Name:

    CColorManageFilter::CColorManageFilter

Routine Description:

    Default constructor for the color management filter which ensures GDI plus is correctly running

Arguments:

    None

Return Value:

    None

--*/
CColorManageFilter::CColorManageFilter()
{
}

/*++

Routine Name:

    CColorManageFilter::~CColorManageFilter

Routine Description:

    Default destructor for the color management filter

Arguments:

    None

Return Value:

    None

--*/
CColorManageFilter::~CColorManageFilter()
{
}

/*++

Routine Name:

    CColorManageFilter::ProcessPart

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
CColorManageFilter::ProcessPart(
    _Inout_ IFixedPage* pFP
    )
{
    VERBOSE("Processing Fixed Page part with color management handler\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pFP, E_POINTER)))
    {
        //
        // Get the PT manager to return the correct ticket. Use a regular pointer
        // to retrieve the ticket and assign to the smart pointer to increment
        // ref count (we don't want our smart pointer to release the PT before
        // the PT manager is finished with it)
        //
        IXMLDOMDocument2* pPT = NULL;
        if (SUCCEEDED(hr = m_ptManager.SetTicket(pFP)) &&
            SUCCEEDED(hr = m_ptManager.GetTicket(kPTPageScope, &pPT)))
        {
            try
            {
                //
                // Get data from PT handler and test if we are color matching
                //
                ColorManagementData cmData;
                CColorManagePTHandler cmPTHandler(pPT);

                PageSourceColorProfileData cmProfData;
                CColorManageProfilePTHandler profPTHandler(pPT);

                PageICMRenderingIntentData cmIntData;
                CColorManageIntentsPTHandler ptIntentsHandler(pPT);

                CComVariant varName;

                if (SUCCEEDED(hr = m_pPrintPropertyBag->GetProperty(XPS_FP_PRINTER_NAME, &varName)) &&
                    SUCCEEDED(hr = cmPTHandler.GetData(&cmData)) &&
                    SUCCEEDED(hr = profPTHandler.GetData(&cmProfData)))
                {
                    hr = ptIntentsHandler.GetData(&cmIntData);
                }

                if (SUCCEEDED(hr) &&
                    cmData.cmOption == Driver &&
                    cmProfData.cmProfileName.Length() > 0)
                {
                    //
                    // Retrieve the writer from the fixed page
                    //
                    CComPtr<IPrintWriteStream>  pWriter(NULL);

                    //
                    // Create a map of the resources that need to be cleaned up after the page
                    // has been processed. These are bitmaps that have been replaced with color
                    // matched equivalents and any icc profiles that have been consumed while
                    // color matching.
                    //
                    ResDeleteMap resDel;

                    if (SUCCEEDED(hr = pFP->GetWriteStream(&pWriter)))
                    {
                        //
                        // Set-up the SAX reader and begin parsing the mark-up
                        //
                        CComPtr<ISAXXMLReader>    pSaxRdr(NULL);
                        CComPtr<IPrintReadStream> pReader(NULL);

                        //
                        // Create a profile manager which handles working with the selected profile
                        //
                        CProfileManager profManager(varName.bstrVal, cmProfData, cmIntData, pFP);

                        //
                        // Create two color converter objects which coordinate color transforms for
                        // bitmaps and color mark-up strings
                        //
                        CBitmapColorConverter        cmBmpConverter(m_pXDWriter, pFP, &m_resCache, &profManager, &resDel);
                        CColorRefConverter           cmRefConverter(m_pXDWriter, pFP, &m_resCache, &profManager, &resDel);
                        CResourceDictionaryConverter cmDictConverter(m_pXDWriter,
                                                                     pFP,
                                                                     &m_resCache,
                                                                     &profManager,
                                                                     &resDel,
                                                                     &cmBmpConverter,
                                                                     &cmRefConverter);

                        //
                        // Create a SAX handler to parse the markup in the fixed page
                        //
                        CCMSaxHandler cmSaxHndlr(pWriter, &cmBmpConverter, &cmRefConverter, &cmDictConverter);

                        if (SUCCEEDED(hr = pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)) &&
                            SUCCEEDED(hr = pSaxRdr->putContentHandler(&cmSaxHndlr)) &&
                            SUCCEEDED(hr = pFP->GetStream(&pReader)))
                        {
                            CComPtr<ISequentialStream> pReadStreamToSeq(NULL);

                            pReadStreamToSeq.Attach(new(std::nothrow) pfp::PrintReadStreamToSeqStream(pReader));

                            if (SUCCEEDED(hr = CHECK_POINTER(pReadStreamToSeq, E_OUTOFMEMORY)))
                            {
                                hr = pSaxRdr->parse(CComVariant(static_cast<ISequentialStream*>(pReadStreamToSeq)));
                            }
                        }

                        pWriter->Close();
                    }

                    if (SUCCEEDED(hr))
                    {
                        //
                        // We have parsed the entire page - it should be safe to delete
                        // all unrequired resources
                        //
                        ResDeleteMap::const_iterator iterRes = resDel.begin();

                        for (;iterRes != resDel.end(); iterRes++)
                        {
                            hr = pFP->DeleteResource(iterRes->first);
                        }
                    }
                }
                else if (hr == E_ELEMENT_NOT_FOUND)
                {
                    hr = S_FALSE;
                }
            }
            catch (CXDException& e)
            {
                hr = e;
            }
            catch (...)
            {
                hr = E_FAIL;
            }
        }
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


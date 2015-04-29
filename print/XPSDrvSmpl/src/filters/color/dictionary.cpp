/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   dict.cpp

Abstract:

   Dictionary class implementation. The CRemoteDictionary class is responsible
   for representing a single dictionay including managing any color
   transforms in that dictionary and writing the dictionary out.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "dictionary.h"
#include "cmsax.h"

/*++

Routine Name:

    CRemoteDictionary::CRemoteDictionary

Routine Description:

    Constructor for the CRemoteDictionary class which registers the URI to the resource
    being represented and pointers to suitable color transformation objects
    for use with color elements within that resource

Arguments:

    bstrResURI    - String containing the URI to the resource to be handled
    pBmpConverter - Pointer to a bitmap color conversion object
    pRefConverter - Pointer to a vector color conversion object

Return Value:

    None
    Throws CXDException(HRESULT) on an error

--*/
CRemoteDictionary::CRemoteDictionary(
    _In_ IFixedPage*            pFixedPage,
    _In_ CBitmapColorConverter* pBmpConverter,
    _In_ CColorRefConverter*    pRefConverter,
    _In_ BSTR                   bstrResURI
    ) :
    m_pFixedPage(pFixedPage),
    m_pBmpConverter(pBmpConverter),
    m_pRefConverter(pRefConverter),
    m_bstrDictionaryURI(bstrResURI)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pFixedPage, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pRefConverter, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(m_pBmpConverter, E_POINTER)))
    {
        if (m_bstrDictionaryURI.Length() == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (FAILED(hr))
    {
        throw CXDException(hr);
    }
}

/*++

Routine Name:

    CRemoteDictionary::~CRemoteDictionary

Routine Description:

    Destructor for the CRemoteDictionary class

Arguments:

    None

Return Value:

    None

--*/
CRemoteDictionary::~CRemoteDictionary()
{
}

/*++

Routine Name:

    CRemoteDictionary::WriteData

Routine Description:

    This method handles the parsing of a new dictionary and
    the writing out the new dictionary

Arguments:

    pWriter - Pointer to a stream to write the resource out to

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRemoteDictionary::WriteData(
    _In_ IPartBase*         pResource,
    _In_ IPrintWriteStream* pWriter
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pResource, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pWriter, E_POINTER)))
    {
        try
        {
            CComPtr<IUnknown>                pRead(NULL);
            CComPtr<IPartResourceDictionary> pResDictPart(NULL);
            CComPtr<IPrintReadStream>        pReader(NULL);

            if (SUCCEEDED(hr = m_pFixedPage->GetPagePart(m_bstrDictionaryURI, &pRead)) &&
                SUCCEEDED(hr = pRead->QueryInterface(&pResDictPart)) &&
                SUCCEEDED(hr = pResDictPart->GetStream(&pReader)))
            {
                //
                // Create a SAX handler to parse the markup in the fixed page
                //
                CCMSaxHandler cmSaxHndlr(pWriter, m_pBmpConverter, m_pRefConverter, NULL);

                //
                // Set-up the SAX reader and begin parsing the mark-up
                //
                CComPtr<ISAXXMLReader> pSaxRdr(NULL);

                if (SUCCEEDED(hr = pSaxRdr.CoCreateInstance(CLSID_SAXXMLReader60)) &&
                    SUCCEEDED(hr = pSaxRdr->putContentHandler(&cmSaxHndlr)))
                {
                    CComPtr<ISequentialStream> pReadStreamToSeq(NULL);

                    pReadStreamToSeq.Attach(new(std::nothrow) pfp::PrintReadStreamToSeqStream(pReader));

                    if (SUCCEEDED(hr = CHECK_POINTER(pReadStreamToSeq, E_OUTOFMEMORY)))
                    {
                        hr = pSaxRdr->parse(CComVariant(static_cast<ISequentialStream*>(pReadStreamToSeq)));
                    }
                }
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


/*++

Routine Name:

    CRemoteDictionary::GetKeyName

Routine Description:

    Method to obtain a unique key for the resource being handled

Arguments:

    pbstrKeyName - Pointer to a string to hold the generated key

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRemoteDictionary::GetKeyName(
    _Outptr_ BSTR* pbstrKeyName
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrKeyName, E_POINTER)))
    {
        if (m_bstrDictionaryURI.Length() > 0)
        {
            *pbstrKeyName = NULL;

            //
            // The full URI to the resource is a suitable key
            //
            if (SUCCEEDED(hr = m_bstrDictionaryURI.CopyTo(pbstrKeyName)) &&
                !*pbstrKeyName)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_PENDING;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CRemoteDictionary::GetResURI

Routine Description:

    Method to obtain the URI of the resource being handled

Arguments:

    pbstrResURI - Pointer to a string to hold the resource URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CRemoteDictionary::GetResURI(
    _Outptr_ BSTR* pbstrResURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrResURI, E_POINTER)))
    {
        *pbstrResURI = NULL;

        try
        {
            //
            // Create a unique name for the dictionary for this print session
            //
            CStringXDW cstrURI;
            cstrURI.Format(L"%s_%u.dict", m_bstrDictionaryURI, GetUniqueNumber());

            *pbstrResURI = cstrURI.AllocSysString();
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}


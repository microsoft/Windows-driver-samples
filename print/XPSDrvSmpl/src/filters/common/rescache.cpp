/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   rescache.cpp

Abstract:

   File resource cache implementation. Filters that need to add resources
   may be required to add the same resource to numerous pages in a job. To
   prevent the same resource being sent repeatedly, the file resource cache
   class implements an interface that filters can use to add resources
   without needing to keep track of whether they have been sent. For specific
   filter functionality, a class should be created that implements a resource
   writer interface. This should be able to write the resource to a stream. The
   filter can then add this class to the the resource cache and it will take
   care of writing the resource via the IResWriter::WriteData method if it has
   not already been written.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "rescache.h"

/*++

Routine Name:

    CFileResourceCache::CFileResourceCache

Routine Description:

    CFileResourceCache class constructor

Arguments:

    None

Return Value:

    None

--*/
CFileResourceCache::CFileResourceCache()
{
}

/*++

Routine Name:

    CFileResourceCache::~CFileResourceCache

Routine Description:

    CFileResourceCache class destructor

Arguments:

    None

Return Value:

    None

--*/
CFileResourceCache::~CFileResourceCache()
{
}

/*++

Routine Name:

    CFileResourceCache::WriteResource

Routine Description:

    This template function writes a resource to a fixed page. The type
    of resource is supplied as an argument to the template. The resource
    data is written via the IResWriter interface passed to the method.

Arguments:

    pXpsConsumer - The XPS consumer to create the new resource part
    pFixedPage   - The FixedPage to send the resource to
    pResWriter   - The resource writer (hides the resource type behind a generic write interface)

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
template <class _T>
HRESULT
CFileResourceCache::WriteResource(
    _In_ IXpsDocumentConsumer*  pXpsConsumer,
    _In_ IFixedPage*            pFixedPage,
    _In_ IResWriter*            pResWriter
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pXpsConsumer, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFixedPage, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pResWriter, E_POINTER)))
    {
        CComBSTR bstrKeyName;
        CComBSTR bstrURI;

        //
        // Check if the resource has already been written
        //
        if (SUCCEEDED(hr = pResWriter->GetKeyName(&bstrKeyName)) &&
            SUCCEEDED(hr = pResWriter->GetResURI(&bstrURI)))
        {
            if (!Cached(bstrKeyName))
            {
                //
                // The resource is not cached:
                //    1. Create the resource part
                //    2. Write data to part
                //    3. Cache URI and new part against the resource name
                //
                CComPtr<_T> pRes(NULL);
                CComPtr<IPrintWriteStream> pWrite(NULL);

                if (SUCCEEDED(hr = pXpsConsumer->GetNewEmptyPart(bstrURI,
                                                                 __uuidof(_T),
                                                                 reinterpret_cast<VOID**>(&pRes),
                                                                 &pWrite)))
                {
                    hr = pResWriter->WriteData(pRes, pWrite);

                    pWrite->Close();

                    try
                    {
                        CComPtr<IPartBase> pPartBase(NULL);
                        if (SUCCEEDED(hr) &&
                            SUCCEEDED(hr = pRes.QueryInterface(&pPartBase)))
                        {
                            m_resMap[CComBSTR(bstrKeyName)].first = bstrURI;
                            m_resMap[CComBSTR(bstrKeyName)].second = pPartBase;
                        }
                    }
                    catch (exception& DBG_ONLY(e))
                    {
                        ERR(e.what());
                        hr = E_FAIL;
                    }
                }
            }

            //
            // Set the resource so relationships are updated
            //
            try
            {
                if (SUCCEEDED(hr))
                {
                    hr = pFixedPage->SetPagePart(m_resMap[CComBSTR(bstrKeyName)].second);
                }
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CFileResourceCache::GetURI

Routine Description:

    This routine retrieves the URI to a given resource

Arguments:

    bstrResNameIn - The name of the resource for which the URI is required
    pbstrURI      - Pointer to a BSTR that recieves the resource URI

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CFileResourceCache::GetURI(
    _In_z_          BSTR bstrResNameIn,
    _Outptr_ BSTR* pbstrURI
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrURI, E_POINTER)))
    {
        if (SysStringLen(bstrResNameIn) == 0)
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            CComBSTR bstrResName(bstrResNameIn);
            CComBSTR uri(m_resMap[bstrResName].first);

            if (uri.Length() > 0)
            {
                if (SUCCEEDED(hr = uri.CopyTo(pbstrURI)) &&
                    !*pbstrURI)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                *pbstrURI = NULL;
                hr = E_FAIL;
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CFileResourceCache::Cached

Routine Description:

    This routine checks if a resource has been cached

Arguments:

    bstrResNameIn - The name (key) of the resource

Return Value:

    BOOL
    TRUE  - The resource is in the cache
    FALSE - The resource is not in the cache

--*/
BOOL
CFileResourceCache::Cached(
    _In_z_ BSTR bstrResNameIn
    )
{
    BOOL bCached = FALSE;

    try
    {
        CComBSTR bstrResName(bstrResNameIn);
        ResCache::const_iterator resMapIter = m_resMap.begin();

        for (;resMapIter != m_resMap.end() && !bCached; resMapIter++)
        {
            if (resMapIter->first == bstrResName)
            {
                bCached = TRUE;
            }
        }
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
    }

    return bCached;
}


//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This source code is intended only as a supplement to Microsoft
//  Development Tools and/or on-line documentation.  See these other
//  materials for detailed information regarding Microsoft code samples.
//
//  THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
//  Abstract:
//     WDK print filter sample.
//     This is the C file for the stream filter sample.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"
#include "main.hxx"
#include "StreamFilter.tmh"
#include "StreamFilter.hxx"

#include "winddiui.h"
#include "compstui.h"
#include "printoem.h"

#include "initguid.h"
#include "prcomoem.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)

//
// 5f5460d2-b313-44ca-82e4-37f83d793999 - generate a guid, do not use this one in your code
//
const GUID StreamFilterGuid = {0x5f5460d2, 0xb313, 0x44ca, {0x82, 0xe4, 0x37, 0xf8, 0x3d, 0x79, 0x39, 0x99}};

const GUID&
StreamFilter::
FilterClsid(
    void
    )
{
    return StreamFilterGuid;
}

StreamFilter::
StreamFilter() :
    m_bShutdown(false),
    m_cRef(1)
{
}

//
// IUnknown methods
//
__override
STDMETHODIMP
StreamFilter::
QueryInterface(
    _In_       REFIID           riid,
    _Out_      void             **ppv
    )
{
    HRESULT hRes = E_POINTER;

    if (ppv)
    {
        hRes = E_NOINTERFACE;

        *ppv = NULL;

        if (riid == IID_IPrintPipelineFilter)
        {
            *ppv = static_cast<IPrintPipelineFilter *>(this);
        }
        else if (riid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown *>(this);
        }

        if (*ppv)
        {
            AddRef();

            hRes = S_OK;
        }
    }

    return hRes;
}

__override
STDMETHODIMP_(ULONG)
StreamFilter::
AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

__override
STDMETHODIMP_(ULONG)
StreamFilter::
Release(
    void
    )
{
    ULONG cRefCount = InterlockedDecrement(&m_cRef);

    if (cRefCount)
    {
        return cRefCount;
    }

    delete this;

    return 0;
}

//
// IPrintPipelineFilter
//
__override
STDMETHODIMP
StreamFilter::
ShutdownOperation(
    void
    )
{
    m_bShutdown = true;

    return S_OK;
}

__override
STDMETHODIMP
StreamFilter::
InitializeFilter(
    _In_    IInterFilterCommunicator         *pIFilterCommunicator,
    _In_    IPrintPipelinePropertyBag        *pIPropertyBag,
    _In_    IPrintPipelineManagerControl     *pIPipelineControl
    )
{
    HRESULT hr = S_OK;
    VARIANT varHelper;
    PCSTR *pFeatures = NULL;
    DWORD dwFeatures = 0;
    Tools::SmartPtr<IPrintCoreHelper> pHelper;

    VariantInit(&varHelper);

    //
    // StreamAccessSequential, IID_IPrintReadStream
    //
    hr = pIFilterCommunicator->RequestReader(reinterpret_cast<void **>(&m_pIRead));

    if (SUCCEEDED(hr))
    {
        //
        // StreamAccessSequential, StreamModify, IID_IPrintWriteStream
        //
        hr = pIFilterCommunicator->RequestWriter(reinterpret_cast<void **>(&m_pIWrite));
    }

    if (SUCCEEDED(hr))
    {
        m_pIPipelineControl = pIPipelineControl;
    }

    //
    // This shows how to use the helper interface to read information
    // from a UnidrvUI based configuration module.
    //

    if (SUCCEEDED(hr))
    {
        hr = pIPropertyBag->GetProperty(L"IPrintCoreHelper", &varHelper);
    }

    if (SUCCEEDED(hr) && V_UNKNOWN(&varHelper))
    {
        hr = (V_UNKNOWN(&varHelper))->QueryInterface(IID_IPrintCoreHelper, (VOID**)(&pHelper));
    }

    //
    // No need to release buffers provided by the helper.  Their
    // lifetime matches that of the helper.
    //
    if (SUCCEEDED(hr) && pHelper)
    {
        hr = pHelper->EnumFeatures(&pFeatures, &dwFeatures);
    }

    if (SUCCEEDED(hr) && dwFeatures > 0)
    {
        for (DWORD i = 0; i < dwFeatures; i++)
        {
            DoTraceMessage(WS_TRACE, L"StreamFilter::InitializeFilter: Feature found: %s", pFeatures[i]);
        }
    }

    VariantClear(&varHelper);

    return hr;
}

__override
STDMETHODIMP
StreamFilter::
StartOperation(
    void
    )
{
    HRESULT                         hr = S_OK;
    Tools::SmartPtr<IImgErrorInfo>  pIErrorInfo;
    DWORD                           cbRead;
    BYTE                            *pReadBuf;
    BOOL                            bEof = FALSE;

    pReadBuf = new BYTE[kBufferSize];

    if (!pReadBuf)
    {
        hr = E_OUTOFMEMORY;
    }

    while (SUCCEEDED(hr) && !bEof && !m_bShutdown)
    {
        hr = m_pIRead->ReadBytes(pReadBuf, kBufferSize, &cbRead, &bEof);

        if (SUCCEEDED(hr) && cbRead)
        {
            ULONG   cbWritten;

            hr = m_pIWrite->WriteBytes(pReadBuf, cbRead, &cbWritten);

            if (SUCCEEDED(hr))
            {
                DoTraceMessage(WS_TRACE, "StreamFilter::StartOperation read and wrote %u bytes", cbRead);
            }
        }
    }

    m_pIWrite->Close();

    if (FAILED(hr))
    {
        m_pIPipelineControl->RequestShutdown(hr, pIErrorInfo);
    }

    if (m_pIPipelineControl)
    {
        m_pIPipelineControl->FilterFinished();
    }

    delete [] pReadBuf;

    return hr;
}



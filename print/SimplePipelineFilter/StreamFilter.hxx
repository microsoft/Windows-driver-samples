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
//     This is the header file for the stream filter sample.
//
//----------------------------------------------------------------------------

#ifndef _STREAM_FILTER_SAMPLE_HXX_
#define _STREAM_FILTER_SAMPLE_HXX_

class StreamFilter :
    public  IPrintPipelineFilter,
    private DllLockManager
{
public:

    StreamFilter();

    //
    // IUnknown methods
    //
    __override
    STDMETHODIMP
    QueryInterface(
        _In_       REFIID           riid,
        _Out_      void             **ppv
        );

    __override
    STDMETHODIMP_(ULONG)
    AddRef(
        void
        );

    __override
    STDMETHODIMP_(ULONG)
    Release(
        void
        );

    //
    // IPrintPipelineFilter
    //
    __override
    STDMETHODIMP
    ShutdownOperation(
        void
        );

    __override
    STDMETHODIMP
    InitializeFilter(
        _In_    IInterFilterCommunicator         *pINegotiation,
        _In_    IPrintPipelinePropertyBag        *pIPropertyBag,
        _In_    IPrintPipelineManagerControl     *pIPipelineControl
        );

    __override
    STDMETHODIMP
    StartOperation(
        void
        );

    //
    // Other methods
    //
    static
    const GUID&
    FilterClsid(
        void
        );

private:

    enum
    {
        kBufferSize     = 0x10000
    };

    Tools::SmartPtr<IPrintReadStream>                m_pIRead;
    Tools::SmartPtr<IPrintWriteStream>               m_pIWrite;
    Tools::SmartPtr<IPrintPipelineManagerControl>    m_pIPipelineControl;
    Tools::SmartPtr<IPrintPipelineProgressReport>    m_pProgressReport;
    bool                                             m_bShutdown;
    LONG                                             m_cRef;
};

#endif // _STREAM_FILTER_SAMPLE_HXX_

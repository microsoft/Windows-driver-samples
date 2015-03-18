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
//     This is the header file for the XPS sample.
//
//----------------------------------------------------------------------------

#ifndef _XPS_FILTER_SAMPLE_HXX_
#define _XPS_FILTER_SAMPLE_HXX_

class XpsFilter :
    public  IPrintPipelineFilter,
    private DllLockManager
{
public:

    XpsFilter();

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

    HRESULT
    ProcessFixedPage(
        _In_    void                    *pVoid
        );

    HRESULT
    ProcessXpsDoc(
        _In_    void                    *pVoid
        );

    HRESULT
    ProcessFixedDocSequence(
        _In_    void                    *pVoid
        );

    HRESULT
    ProcessFixedDoc(
        _In_    void                    *pVoid
        );

    HRESULT
    ProcessPart(
        _In_    IUnknown                *pUnk
        );

    static
    HRESULT
    ProcessImagePart(
        _In_    IPartImage              *pIPartImage
        );

    Tools::SmartPtr<IPrintPipelineManagerControl>   m_pIPipelineControl;
    Tools::SmartPtr<IXpsDocumentProvider>           m_pReachProvider;
    Tools::SmartPtr<IXpsDocumentConsumer>           m_pReachConsumer;
    bool                                            m_bShutdown;
    LONG                                            m_cRef;
    LONG                                            m_cIdCount;
};

#endif // _XPS_FILTER_SAMPLE_HXX_


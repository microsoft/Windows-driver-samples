// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    filtertypes.h
//
// Abstract:
//
//    Smart pointer types. Shared structures and enums.
//

#pragma once

//
// XpsDrv Print Pipeline types
//
typedef CComPtr<IXpsDocument>                               IXpsDocument_t;
typedef CComPtr<IFixedDocumentSequence>                     IFixedDocumentSequence_t;
typedef CComPtr<IFixedDocument>                             IFixedDocument_t;
typedef CComPtr<IFixedPage>                                 IFixedPage_t;
typedef CComPtr<IPrintReadStreamFactory>                    IPrintReadStreamFactory_t;
typedef CComPtr<IPrintReadStream>                           IPrintReadStream_t;
typedef CComPtr<IPrintWriteStream>                          IPrintWriteStream_t;
typedef CComPtr<IInterFilterCommunicator>                   IInterFilterCommunicator_t;
typedef CComPtr<IPrintPipelinePropertyBag>                  IPrintPipelinePropertyBag_t;
typedef CComPtr<IPrintPipelineManagerControl>               IPrintPipelineManagerControl_t;
typedef CComPtr<IXpsDocumentProvider>                       IXpsDocumentProvider_t;
typedef CComPtr<IXpsDocumentConsumer>                       IXpsDocumentConsumer_t;
typedef CComPtr<IXpsPartIterator>                           IXpsPartIterator_t;
typedef CComPtr<IPartBase>                                  IPartBase_t;
typedef CComPtr<IPartFont2>                                 IPartFont2_t;
typedef CComPtr<IPartFont>                                  IPartFont_t;
typedef CComPtr<IPartImage>                                 IPartImage_t;
typedef CComPtr<IPartColorProfile>                          IPartColorProfile_t;
typedef CComPtr<IPartResourceDictionary>                    IPartResourceDictionary_t;
typedef std::vector<CAdapt<IPartResourceDictionary_t>>      ResourceDictionaryList_t;
typedef CComPtr<IPartPrintTicket>                           IPartPrintTicket_t;

//
// Xps Object Model types
//
typedef CComPtr<IXpsOMObjectFactory>                        IXpsOMObjectFactory_t;
typedef CComPtr<IXpsOMPartResources>                        IXpsOMPartResources_t;
typedef CComPtr<IXpsOMFontResourceCollection>               IXpsOMFontResourceCollection_t;
typedef CComPtr<IXpsOMImageResourceCollection>              IXpsOMImageResourceCollection_t;
typedef CComPtr<IXpsOMColorProfileResourceCollection>       IXpsOMColorProfileResourceCollection_t;
typedef CComPtr<IXpsOMRemoteDictionaryResourceCollection>   IXpsOMRemoteDictionaryResourceCollection_t;
typedef CComPtr<IXpsOMFontResource>                         IXpsOMFontResource_t;
typedef CComPtr<IXpsOMImageResource>                        IXpsOMImageResource_t;
typedef CComPtr<IXpsOMColorProfileResource>                 IXpsOMColorProfileResource_t;
typedef CComPtr<IXpsOMRemoteDictionaryResource>             IXpsOMRemoteDictionaryResource_t;
typedef CComPtr<IXpsOMPage>                                 IXpsOMPage_t;

//
// Opc Types
//
typedef CComPtr<IOpcFactory>                                IOpcFactory_t;
typedef CComPtr<IOpcPartUri>                                IOpcPartUri_t;

//
// Common types
//
typedef CComPtr<IStream>                                    IStream_t;
typedef CComPtr<IUnknown>                                   IUnknown_t;
typedef CComBSTR                                            BSTR_t;
typedef CComVariant                                         Variant_t;
typedef CComPtr<IPropertyBag2>                              IPropertyBag2_t;

//
// WIC types
//
typedef CComPtr<IWICImagingFactory>                         IWICImagingFactory_t;
typedef CComPtr<IWICBitmap>                                 IWICBitmap_t;
typedef CComPtr<IWICStream>                                 IWICStream_t;
typedef CComPtr<IWICBitmapEncoder>                          IWICBitmapEncoder_t;
typedef CComPtr<IWICBitmapFrameEncode>                      IWICBitmapFrameEncode_t;

//
// Xps Rasterization Service types
//
typedef CComPtr<IXpsRasterizationFactory>                   IXpsRasterizationFactory_t;
typedef CComPtr<IXpsRasterizer>                             IXpsRasterizer_t;

//
// MSXML DOM types
//
typedef CComPtr<IXMLDOMDocument2>                           IXMLDOMDocument2_t;
typedef CComPtr<IXMLDOMNode>                                IXMLDOMNode_t;
typedef CComPtr<IXMLDOMNodeList>                            IXMLDOMNodeList_t;

namespace xpsrasfilter
{

//
// Supported types of print ticket scaling
//
enum PrintTicketScaling
{
    SCALE_NONE,
    SCALE_BLEEDTOIMAGEABLE,
    SCALE_CONTENTTOIMAGEABLE,
    SCALE_MEDIASIZETOIMAGEABLE,
    SCALE_MEDIASIZETOMEDIASIZE
};

//
// Parameters that can be read from the Print Ticket
// to feed into rasterization calculations.
// 
struct ParametersFromPrintTicket
{
    XPS_SIZE            physicalPageSize;   // in XPS units
    PrintTicketScaling  scaling;            // scaling type
    FLOAT               destDPI;            // target rasterization dpi
    XPS_RECT            imageableArea;      // in XPS units
};

//
// Parameters read from the FixedPage
// to feed into rasterization calculations.
//
struct ParametersFromFixedPage
{
    XPS_SIZE    fixedPageSize;      // in XPS units
    XPS_RECT    bleedBoxRect;       // in XPS units
    XPS_RECT    contentBoxRect;     // in XPS units
};

//
// Forward Declarations
//
class RasterizationInterface;
class PrintTicketHandler;
class TiffStreamBitmapHandler;
class FilterLiveness;

} // namespace xpsrasfilter

//
// This class handles setting and clearing the security
// context, based on a token. This token is retrieved from
// the filter pipeline, and we do not want to free it.
//
class ScopeImpersonation
{
public:
    ScopeImpersonation(
        HANDLE  token
        )
    {
        if (
            !SetThreadToken(
                NULL, // set the current thread's token
                token
                )
            )
        {
            THROW_LAST_ERROR();
        }
    }

    ~ScopeImpersonation()
    {
        if (
            !SetThreadToken(
                NULL,  // set the current thread's token
                NULL   // revert to default security context
                )
            )
        {
            //
            // We couldn't revert the security context. The filter pipeline
            // manager will clean up the thread when operation is complete,
            // when it is determined that the security context was not 
            // reverted. Since there are no security implications with 
            // running this filter in an elevated context, we can log the 
            // error and continue to run.
            //
            DWORD error = ::GetLastError();

            WPP_LOG_ON_FAILED_HRESULT_WITH_TEXT(
                HRESULT_FROM_WIN32(error),
                L"Failed to revert thread security context."
                );
        }
    }

private:
    ScopeImpersonation(ScopeImpersonation const&);
    ScopeImpersonation& operator=(ScopeImpersonation const&);
};

//
// RAII object to make HGLOBAL locks exception-safe. This requires
// unlocking during unwind.
//
class HGlobalLock
{
public:
    HGlobalLock(
        HGLOBAL hG
        ) :
        m_hGlobal(hG)
    {
        m_pAddress = static_cast<BYTE *>(
                        ::GlobalLock(m_hGlobal)
                        );

        if (!m_pAddress)
        {
            THROW_LAST_ERROR();
        }
    }

    ~HGlobalLock()
    {
        if (!::GlobalUnlock(m_hGlobal))
        {
            WPP_LOG_ON_FAILED_HRESULT(
                HRESULT_FROM_WIN32(::GetLastError())
                );
        }
    }

    BYTE*
    GetAddress()
    {
        return m_pAddress;
    }
private:
    HGLOBAL m_hGlobal;
    BYTE *m_pAddress;

    HGlobalLock(HGlobalLock const&);
    HGlobalLock& operator=(HGlobalLock const&);

};

typedef std::auto_ptr<HGlobalLock> HGlobalLock_t;

//
// Safe handle to make HGLOBAL exception-safe. This requires both
// freeing and unlocking during unwind.
//
class SafeHGlobal
{
public:

    SafeHGlobal(
        UINT    flags,
        SIZE_T  size
        )
    {
        m_hGlobal = ::GlobalAlloc(flags, size);

        if (!m_hGlobal)
        {
            THROW_ON_FAILED_HRESULT(E_OUTOFMEMORY);
        }
    }

    virtual
    ~SafeHGlobal()
    {
        if (m_hGlobal)
        {
            //
            // Free the HGLOBAL
            //
            ::GlobalFree(m_hGlobal);
        }
    }

    operator HGLOBAL()
    {
        return m_hGlobal;
    }

    //
    // Passes ownership of the HGLOBAL from this safe handle 
    // to a new IStream.
    //
    IStream_t
    ConvertToIStream()
    {
        IStream_t pStream;

        THROW_ON_FAILED_HRESULT(
        ::CreateStreamOnHGlobal(
            m_hGlobal,
            TRUE, // Free the HGLOBAL on Release of the stream
            &pStream
            )
        );

        m_hGlobal = NULL;

        return pStream;
    }

    HGlobalLock_t
    Lock()
    {
        HGlobalLock_t toReturn(
                            new HGlobalLock(m_hGlobal)
                            );
        return toReturn;
    }

private:

    HGLOBAL m_hGlobal;

    SafeHGlobal(SafeHGlobal const&);
    SafeHGlobal& operator=(SafeHGlobal const&);

};

//
// Safe handle to make HPTPROVIDER exception safe. This
// requires closing the provider during unwind.
//
class SafeHPTProvider
{
public:
    SafeHPTProvider(
        const wchar_t   *printerName,
        HANDLE          userSecurityToken
        ) :
        m_token(userSecurityToken)
    {
        //
        // We impersonate the user while we call PTQuerySchemaVersionSupport 
        // and PTOpenProviderEx.
        //
        ScopeImpersonation impersonate(m_token);

        DWORD   maxVersion,
                tempVersion;

        THROW_ON_FAILED_HRESULT(
            ::PTQuerySchemaVersionSupport(
                printerName,
                &maxVersion
                )
            );

        THROW_ON_FAILED_HRESULT(
            ::PTOpenProviderEx(
                printerName,
                maxVersion, // maximum version
                maxVersion, // preferred version
                &m_hProvider,
                &tempVersion // version used by the provider
                )
            );
    }

    virtual
    ~SafeHPTProvider()
    {
        WPP_LOG_ON_FAILED_HRESULT(
            ::PTCloseProvider(m_hProvider)
            );
    }

    void
    PTMergeAndValidatePrintTicket(
        const IStream_t     &pBasePrintTicket,
        const IStream_t     &pDeltaPrintTicket,
        EPrintTicketScope   scope,
        _Inout_ IStream_t   &pMergedPrintTicket
        )
    {
        //
        // We impersonate the user while we call PTMergeAndValidatePrintTicket.
        //
        ScopeImpersonation impersonate(m_token);

        BSTR_t error;

        HRESULT hr = ::PTMergeAndValidatePrintTicket(
                            m_hProvider,
                            pBasePrintTicket,
                            pDeltaPrintTicket,
                            scope,
                            pMergedPrintTicket,
                            &error
                            );

        WPP_LOG_ON_FAILED_HRESULT_WITH_TEXT(
            hr,
            error
            );

        THROW_ON_FAILED_HRESULT(hr);
    }

private:

    HPTPROVIDER m_hProvider;
    HANDLE      m_token;

    SafeHPTProvider(SafeHPTProvider const&);
    SafeHPTProvider& operator=(SafeHPTProvider const&);
};

//
// CoInitialize/CoUninitialize RAII object
//
// This object ensures that COM is initialized for the duration
// of the XpsRasFilter's lifetime, and then uninitialized after
// all of the COM objects, regardless of how the filter exits
//
class SafeCoInit
{
public:
    SafeCoInit() :
        m_doCoUninitialize(FALSE)
    {
        //
        // Initialize COM
        //
        HRESULT hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);

        if (FAILED(hr) &&
            hr != RPC_E_CHANGED_MODE)
        {
            //
            // RPC_E_CHANGED_MODE indicates that we attempted to change the 
            // threading model. It is safe to ignore since we do not *require* 
            // multi-threading. Throw on any other errors.
            //

            THROW_ON_FAILED_HRESULT(hr);
        }
        else if (SUCCEEDED(hr))
        {
            //
            // It is important that we only call CoUninitialize() if we
            // succeeded in setting the threading model.
            //

            m_doCoUninitialize = TRUE;
        }
    }

    ~SafeCoInit()
    {
        if (m_doCoUninitialize)
        {
            ::CoUninitialize();
        }
    }
private:
    SafeCoInit(SafeCoInit const&);
    SafeCoInit& operator=(SafeCoInit const&);

    BOOL m_doCoUninitialize;
};

//
// RAII object to make VARIANT exception-safe. This requires
// VariantClear during unwind.
//
class SafeVariant : public VARIANT
{
public:
    SafeVariant() 
    {
        ::VariantInit(this);
    }

    ~SafeVariant()
    {
        ::VariantClear(this);
    }
private:
    SafeVariant(SafeVariant const&);
    SafeVariant& operator=(SafeVariant const&);
};

//
// Internal Types
//
typedef std::auto_ptr<xpsrasfilter::RasterizationInterface>     RasterizationInterface_t;
typedef std::auto_ptr<xpsrasfilter::PrintTicketHandler>         PrintTicketHandler_t;
typedef std::auto_ptr<xpsrasfilter::TiffStreamBitmapHandler>    TiffStreamBitmapHandler_t;
typedef std::auto_ptr<SafeHGlobal>                              SafeHGlobal_t;
typedef std::auto_ptr<SafeHPTProvider>                          SafeHPTProvider_t;
typedef CComPtr<xpsrasfilter::FilterLiveness>                   FilterLiveness_t;


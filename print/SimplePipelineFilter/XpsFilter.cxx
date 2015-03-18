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
#include "XpsFilter.tmh"
#include "XpsFilter.hxx"

_Analysis_mode_(_Analysis_code_type_user_driver_)

//
// 8267d291-6ddd-4972-a94e-3ce88149a1fa - generate a guid, do not use this one in your code
//
const GUID ClsidXpsFilter = {0x8267d291, 0x6ddd, 0x4972, {0xa9, 0x4e, 0x3c, 0xe8, 0x81, 0x49, 0xa1, 0xfa}};

const GUID&
XpsFilter::
FilterClsid(
    void
    )
{
    return ClsidXpsFilter;
}

#define COUNTOF(x) (sizeof(x)/sizeof(*x))

HRESULT
GetLastErrorAsHResult(
    void
    )
{
    DWORD dwError = GetLastError();

    return HRESULT_FROM_WIN32(dwError);
}

HRESULT
ProcessTicket(
    _In_    IPartPrintTicket   *pIPrintTicket
    )
{
    Tools::SmartBSTR bstr;

    HRESULT hRes = pIPrintTicket->GetUri(&bstr);

    if (SUCCEEDED(hRes))
    {
        Tools::SmartPtr<IPrintReadStream>   pIStream;

        hRes = pIPrintTicket->GetStream(&pIStream);

        if (SUCCEEDED(hRes))
        {
            ULONG cbRead = 0;
            BOOL  bEof   = FALSE;

            DoTraceMessage(WS_TRACE, "ProcessTicket print ticket follows.");

            do
            {
                BYTE  buf[200] = {0};

                //
                // Save one byte to ensure null termination
                //
                if (SUCCEEDED(hRes = pIStream->ReadBytes(buf, COUNTOF(buf) - 1, &cbRead, &bEof)) && cbRead)
                {
                    //
                    // use PT data
                    //
                    DoTraceMessage(WS_TRACE, "%s", reinterpret_cast<PCSTR>(buf));
                }
            }
            while (!bEof && cbRead && SUCCEEDED(hRes));
        }
    }

    return hRes;
}

HRESULT
AddFontToPage(
    _In_    IPartFont           *pNewFont,
    _In_    IPrintWriteStream   *pFontStream,
    _In_    IFixedPage          *pNewPage
    )
{
    WCHAR      szFont[MAX_PATH];
    HRESULT    hRes = S_OK;

    if (!GetWindowsDirectory(szFont, MAX_PATH))
    {
        hRes = GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRes))
    {
        hRes = StringCchCat(szFont, MAX_PATH, L"\\fonts\\verdana.TTF");
    }

    if (SUCCEEDED(hRes))
    {
        HANDLE hFile = CreateFile(szFont,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            BYTE    buf[400];
            ULONG   cbBuf = 400;
            ULONG   cbRead, cbWritten;

            do
            {
                if (ReadFile(hFile, buf, cbBuf, &cbRead, NULL))
                {
                    hRes = pFontStream->WriteBytes(buf, cbRead, &cbWritten);
                }
                else
                {
                    hRes = GetLastErrorAsHResult();
                }
            }
            while (SUCCEEDED(hRes) && cbRead);

            CloseHandle(hFile);
        }
        else
        {
            hRes = GetLastErrorAsHResult();
        }

        pFontStream->Close();
    }

    if (SUCCEEDED(hRes))
    {
        hRes = pNewPage->SetPagePart(pNewFont);
    }

    return hRes;
}

HRESULT
AddTextToPage(
    _In_    PCWSTR              pszReferencedFont,
    _In_    IPrintWriteStream   *pPageMarkupStream
    )
{
    HRESULT  hRes;
    ULONG    cb;
    CHAR     buf[500];

    //
    // Mark up for page generated on the fly: "hello world"
    //
    CHAR  sz[] = "<FixedPage Width=\"816\" Height=\"1056\" xmlns=\"http://schemas.microsoft.com/xps/2005/06\" xml:lang=\"en-US\">"
                 "<Glyphs Fill=\"#ff000000\" FontUri=\"%ws\" FontRenderingEmSize=\"15\" StyleSimulations=\"None\" OriginX=\"172\""
                 " OriginY=\"129.68\" Indices=\"75;72;79;79;82;3;90;82;85;79;71\" UnicodeString=\"hello world\" />"
                 "</FixedPage>";

    //
    // Insert the URI of the font we added to the document
    //
    hRes = StringCchPrintfA(buf, COUNTOF(buf), sz, pszReferencedFont);

    //
    // Write markup for page
    //
    if (SUCCEEDED(hRes) &&
        SUCCEEDED(hRes = pPageMarkupStream->WriteBytes(buf, static_cast<ULONG>(strlen(buf)), &cb)))
    {
        pPageMarkupStream->Close();
    }

    return hRes;
}

//
// This function reads the page content and writes it back. Can be extended
// to do useful work
//
HRESULT
ModifyContent(
    _In_    IPrintReadStream     *pRead,
    _In_    IPrintWriteStream    *pWrite
    )
{
    CHAR buf[100] = {0};
    HRESULT hRes;
    ULONG cbr, cbw;
    BOOL  bEof = FALSE;

    do
    {
        hRes = pRead->ReadBytes(buf, sizeof(buf), &cbr, &bEof);

        if (SUCCEEDED(hRes) && cbr)
        {
            //
            // do something with buffer
            //
            cbw = cbr;

            hRes = pWrite->WriteBytes(buf, cbr, &cbw);
        }
    }
    while (!bEof && cbr && SUCCEEDED(hRes));

    return hRes;
}

HRESULT
XpsFilter::
ProcessImagePart(
    _In_    IPartImage    *pIPartImage
    )
{
    Tools::SmartBSTR bstr;

    HRESULT hRes = pIPartImage->GetUri(&bstr);

    DoTraceMessage(WS_TRACE, "ProcessImagePart uri %ws", static_cast<PCWSTR>(bstr));

    if (SUCCEEDED(hRes))
    {
        Tools::SmartPtr<IPrintReadStream>  pRead;

        if (SUCCEEDED(hRes = pIPartImage->GetStream(&pRead)))
        {
            //
            // do something with image
            //
        }
    }

    return hRes;
}

//
// Non-static members
//
XpsFilter::
XpsFilter() :
    m_bShutdown(false),
    m_cRef(1),
    m_cIdCount(0)
{
}

HRESULT
XpsFilter::
ProcessFixedPage(
    _In_    void    *pVoid
    )
{
    Tools::SmartPtr<IFixedPage> pIFixedPage;

    //
    // XpsFilter::ProcessPart already incremented the reference count for the object. This
    // function is taking ownership of the object, there is no need for another AddRef call
    // (hence the Attach)
    //
    pIFixedPage.Attach(static_cast<IFixedPage *>(pVoid));

    HRESULT  hRes;
    Tools::SmartBSTR bstr;

    if (SUCCEEDED(pIFixedPage->GetUri(&bstr)))
    {
        DoTraceMessage(WS_TRACE, "ProcessFixedPage uri %ws", static_cast<PCWSTR>(bstr));
    }

    Tools::SmartPtr<IPartPrintTicket> pIPrintTicket;

    hRes = pIFixedPage->GetPrintTicket(&pIPrintTicket);

    if (SUCCEEDED(hRes))
    {
        hRes = ProcessTicket(pIPrintTicket);
    }
    else if (hRes == E_ELEMENT_NOT_FOUND)
    {
        //
        // No ticket. Benign.
        //
        hRes = S_OK;

        DoTraceMessage(WS_WARNING, "ProcessFixedPage uri %ws. Page print ticket not present.", static_cast<PCWSTR>(bstr));
    }

    pIFixedPage->SetPartCompression(Compression_Small);

    //
    // modify content for page, optional
    //
    Tools::SmartPtr<IPrintReadStream>   pRead;
    Tools::SmartPtr<IPrintWriteStream>  pWrite;

    if (SUCCEEDED(hRes = pIFixedPage->GetStream(&pRead)) &&
        SUCCEEDED(hRes = pIFixedPage->GetWriteStream(&pWrite)))
    {
        hRes = ModifyContent(pRead, pWrite);

        pWrite->Close();
    }

    //
    // Send page to next filter
    //
    if (SUCCEEDED(hRes))
    {
        hRes = m_pReachConsumer->SendFixedPage(pIFixedPage);
    }

    //
    // Add a new page
    //
    if (SUCCEEDED(hRes))
    {
        Tools::SmartPtr<IFixedPage>         pNewPage;
        Tools::SmartPtr<IPrintWriteStream>  pNewPageMarkupStream;
        Tools::SmartPtr<IPartFont>          pNewFont;
        Tools::SmartPtr<IPrintWriteStream>  pNewFontStream;

        WCHAR                        szName[MAX_PATH];

        //
        // Generate unique page name
        //
        if (SUCCEEDED(hRes = StringCchPrintf(szName, MAX_PATH, L"/pages/newaddedpage%u.xaml", m_cIdCount++)))
        {
            //
            // Create new fixed page
            //
            hRes = m_pReachConsumer->GetNewEmptyPart(szName,
                                                     IID_IFixedPage,
                                                     reinterpret_cast<void **>(&pNewPage),
                                                     &pNewPageMarkupStream);

            DoTraceMessage(WS_TRACE, "ProcessFixedPage new page %ws", szName);
        }

        //
        // Generate unique font name, this can, of course, be optimized so we don't include
        // the same font multiple times. But this code is supposed to just to show how to
        // add a font
        //
        if (SUCCEEDED(hRes = StringCchPrintf(szName, MAX_PATH, L"/font_%u.ttf", m_cIdCount++)))
        {
            //
            // Create new font resource
            //
            hRes = m_pReachConsumer->GetNewEmptyPart(szName,
                                                     IID_IPartFont,
                                                     reinterpret_cast<void **>(&pNewFont),
                                                     &pNewFontStream);

            DoTraceMessage(WS_TRACE, "ProcessFixedPage new font %ws", szName);
        }

        if (SUCCEEDED(hRes) &&
            SUCCEEDED(hRes = AddFontToPage(pNewFont, pNewFontStream, pNewPage)) &&
            SUCCEEDED(hRes = AddTextToPage(szName, pNewPageMarkupStream)))
        {
            //
            // Send page to next filter
            //
            hRes = m_pReachConsumer->SendFixedPage(pNewPage);
        }
    }

    return hRes;
}



struct Pair_t
{
    typedef HRESULT (XpsFilter::*PFN)(void *);

    Pair_t(REFIID d, PFN n) : iid(d), pfn(n) {}

    REFIID  iid;
    PFN     pfn;

private:

    Pair_t& operator=(const Pair_t&);
};


HRESULT
XpsFilter::
ProcessPart(
    _In_    IUnknown    *pUnk
    )
{
    HRESULT   hRes = S_OK;

    Pair_t pairs[] =
    {
        Pair_t(IID_IXpsDocument,            &XpsFilter::ProcessXpsDoc),
        Pair_t(IID_IFixedDocumentSequence,  &XpsFilter::ProcessFixedDocSequence),
        Pair_t(IID_IFixedDocument,          &XpsFilter::ProcessFixedDoc),
        Pair_t(IID_IFixedPage,              &XpsFilter::ProcessFixedPage),
    };

    //
    // Detect what part we got
    //
    for (ULONG i = 0; i < sizeof(pairs)/sizeof(*pairs); i++)
    {
        void *pvoid = NULL;

        if (SUCCEEDED(hRes = pUnk->QueryInterface(pairs[i].iid, &pvoid)))
        {
            //
            // QueryInterface increments the reference count for the object it returns
            //
            hRes = (this->*pairs[i].pfn)(pvoid);

            break;
        }
        else if (hRes != E_NOINTERFACE)
        {
            break;
        }
    }

    //
    // IPartDiscardControl objects may float through the pipeline. The code above does not check for IPartDiscardControl.
    // So, for maximum compatibility, the filter forwards parts that it doesn't understand.
    //
    if (hRes == E_NOINTERFACE)
    {
        hRes = m_pReachConsumer->SendXpsUnknown(pUnk);
    }

    return hRes;
}

HRESULT
XpsFilter::
ProcessXpsDoc(
    _In_    void    *pVoid
    )
{
    Tools::SmartPtr<IXpsDocument>  pIXpsDoc;

    pIXpsDoc.Attach(static_cast<IXpsDocument *>(pVoid));

    DoTraceMessage(WS_TRACE, L"ProcessXpsDoc");

    return m_pReachConsumer->SendXpsDocument(pIXpsDoc);
}

HRESULT
XpsFilter::
ProcessFixedDocSequence(
    _In_    void    *pVoid
    )
{
    Tools::SmartPtr<IFixedDocumentSequence> pIFixedDocumentSequence;

    //
    // XpsFilter::ProcessPart already incremented the reference count for the object. This
    // function is taking ownership of the object, there is no need for another AddRef call
    // (hence the Attach)
    //
    pIFixedDocumentSequence.Attach(static_cast<IFixedDocumentSequence *>(pVoid));

    Tools::SmartBSTR bstr;

    if (SUCCEEDED(pIFixedDocumentSequence->GetUri(&bstr)))
    {
        DoTraceMessage(WS_TRACE, "ProcessFixedDocSequence uri %ws", static_cast<PCWSTR>(bstr));
    }

    Tools::SmartPtr<IPartPrintTicket> pIPrintTicket;

    HRESULT hRes = pIFixedDocumentSequence->GetPrintTicket(&pIPrintTicket);

    if (SUCCEEDED(hRes))
    {
        hRes = ProcessTicket(pIPrintTicket);
    }
    else if (hRes == E_ELEMENT_NOT_FOUND)
    {
        //
        // No ticket. Benign.
        //
        hRes = S_OK;

        DoTraceMessage(WS_WARNING, "ProcessFixedDocSequence uri %ws. Page print ticket not present.", static_cast<PCWSTR>(bstr));
    }

    return m_pReachConsumer->SendFixedDocumentSequence(pIFixedDocumentSequence);
}

HRESULT
XpsFilter::
ProcessFixedDoc(
    _In_    void    *pVoid
    )
{
    Tools::SmartPtr<IFixedDocument> pIFixedDocument;

    //
    // XpsFilter::ProcessPart already incremented the reference count for the object. This
    // function is taking ownership of the object, there is not need for another AddRef call
    // (hence the Attach)
    //
    pIFixedDocument.Attach(static_cast<IFixedDocument *>(pVoid));

    Tools::SmartBSTR bstr;

    if (SUCCEEDED(pIFixedDocument->GetUri(&bstr)))
    {
        DoTraceMessage(WS_TRACE, "ProcessFixedDoc uri %ws", static_cast<PCWSTR>(bstr));
    }

    Tools::SmartPtr<IPartPrintTicket> pIPrintTicket;

    HRESULT hRes = pIFixedDocument->GetPrintTicket(&pIPrintTicket);

    if (SUCCEEDED(hRes))
    {
        hRes = ProcessTicket(pIPrintTicket);
    }
    else if (hRes == E_ELEMENT_NOT_FOUND)
    {
        //
        // No ticket. Benign.
        //
        hRes = S_OK;

        DoTraceMessage(WS_WARNING, "ProcessFixedDoc uri %ws. Page print ticket not present.", static_cast<PCWSTR>(bstr));
    }

    return m_pReachConsumer->SendFixedDocument(pIFixedDocument);
}

//
// IPrintPipelineFilter
//
__override
STDMETHODIMP
XpsFilter::
ShutdownOperation(
    void
    )
{
    m_bShutdown = true;

    return S_OK;
}

__override
STDMETHODIMP
XpsFilter::
InitializeFilter(
    _In_    IInterFilterCommunicator         *pIfc,
    _In_    IPrintPipelinePropertyBag        *pIPropertyBag,
    _In_    IPrintPipelineManagerControl     *pIPipelineControl
    )
{
    HRESULT hRes = S_OK;

    //
    // IID_IReachPackageProvider must be specified in the config file
    //
    hRes = pIfc->RequestReader(reinterpret_cast<void **>(&m_pReachProvider));

    if (SUCCEEDED(hRes))
    {
        //
        // IID_IReachPackageConsumer must be specified in the config file
        //
        hRes = pIfc->RequestWriter(reinterpret_cast<void **>(&m_pReachConsumer));

        m_pIPipelineControl = pIPipelineControl;
    }

    VARIANT var, var2, var3, var4;

    VariantInit(&var);
    VariantInit(&var2);
    VariantInit(&var3);
    VariantInit(&var4);

    if (SUCCEEDED(hRes) &&
        SUCCEEDED(hRes = pIPropertyBag->GetProperty(XPS_FP_PRINTER_NAME, &var)) &&
        SUCCEEDED(hRes = pIPropertyBag->GetProperty(XPS_FP_PRINTER_HANDLE, &var2)) &&
        SUCCEEDED(hRes = pIPropertyBag->GetProperty(XPS_FP_JOB_ID, &var3)) &&
        SUCCEEDED(hRes = pIPropertyBag->GetProperty(XPS_FP_USER_TOKEN, &var4)))
    {
        PWSTR    pszName            = var.bstrVal;
        HANDLE   hPrinter           = var2.byref;
        ULONG    jobId              = var3.ulVal;
        HANDLE   hUserSecurityToken = var4.byref;

        //
        // Filter can use devmode, hPrinter and pszName. byref value should not be freed/deleted
        // in any way. bstrVal must be deleted by clearing the variant.
        //

        //
        // Do not free any byref resource. Ex: do not call CloseHandle on the security token
        //

        //
        // Remove these if you use the variables. This is in order to build clean with /W4
        //
        UNREFERENCED_PARAMETER(jobId);
        UNREFERENCED_PARAMETER(hPrinter);
        UNREFERENCED_PARAMETER(pszName);

        //
        // Sample code to show how to impersonate in order to access resources on behalf of the
        // user who submitted the job
        //
        if (SetThreadToken(NULL, hUserSecurityToken))
        {
            //
            // now revert back to the original security context
            //
            if (!SetThreadToken(NULL, NULL))
            {
                hRes = GetLastErrorAsHResult();
            }
        }
        else
        {
            hRes = GetLastErrorAsHResult();
        }
    }

    VariantClear(&var);
    VariantClear(&var2);
    VariantClear(&var3);
    VariantClear(&var4);

    //
    // Example of how to add and delete a property from the bag
    //
    var.vt   = VT_I4;
    var.lVal = 5;

    if (SUCCEEDED(hRes) &&
        SUCCEEDED(hRes = pIPropertyBag->AddProperty(L"TestProperty", &var)))
    {
        hRes = pIPropertyBag->DeleteProperty(L"TestProperty") ? S_OK : E_FAIL;
    }


    return hRes;
}

__override
STDMETHODIMP
XpsFilter::
StartOperation(
    void
    )
{
    HRESULT hRes = S_OK;

    while (SUCCEEDED(hRes) && !m_bShutdown)
    {
        Tools::SmartPtr<IUnknown>   pUnk;

        hRes =  m_pReachProvider->GetXpsPart(&pUnk);

        if (SUCCEEDED(hRes))
        {
            if (!pUnk)
            {
                //
                // End of objects
                //
                break;
            }

            hRes = ProcessPart(pUnk);
        }
    }

    m_pReachConsumer->CloseSender();

    if (FAILED(hRes))
    {
#pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_1, "MSDN requires that pReason be NULL.")
        m_pIPipelineControl->RequestShutdown(hRes, NULL);
    }

    m_pIPipelineControl->FilterFinished();

    return hRes;
}

//
// IUnknown
//
STDMETHODIMP_(ULONG)
XpsFilter::
AddRef(
    VOID
    )
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
XpsFilter::
Release(
    VOID
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

STDMETHODIMP
XpsFilter::
QueryInterface(
    _In_      REFIID      riid,
    _Out_     VOID        **ppv
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


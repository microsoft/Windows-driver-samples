//
// File Name:
//
//    RenderFilter.cpp
//
// Abstract:
//
//    XPS Rendering filter implementation.
//

#include "precomp.h"
#include "WppTrace.h"
#include "CustomWppCommands.h"
#include "Exception.h"
#include "filtertypes.h"
#include "UnknownBase.h"
#include "OMConvertor.h"
#include "PThandler.h"
#include "RenderFilter.h"

#include "RenderFilter.tmh"

namespace XpsRenderFilter
{

long RenderFilter::ms_numObjects = 0; // Initialize static object count

//
//Routine Name:
//
//    RenderFilter::RenderFilter
//
//Routine Description:
//
//    XPS Rendering filter default constructor.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    None
//
RenderFilter::RenderFilter()
{
    //
    // Take ownership with no AddRef
    //
    m_pLiveness.Attach(new FilterLiveness());
    
    ::InterlockedIncrement(&ms_numObjects);
}

//
//Routine Name:
//
//    RenderFilter::~RenderFilter
//
//Routine Description:
//
//    XPS Rendering sample filter destructor.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    None
//
RenderFilter::~RenderFilter()
{
    ::InterlockedDecrement(&ms_numObjects);
}

//
//Routine Name:
//
//    RenderFilter::InitializeFilter
//
//Routine Description:
//
//    Exception boundary wrapper for IPrintPipelineFilter initialization.
//
//Arguments:
//
//    pICommunicator    - interface to interfilter communicator
//    pIPropertyBag     - interface to pipeline property bag
//    pIPipelineControl - interface to pipeline control methods
//
//Return Value:
//
//    ULONG
//    New reference count
//
_Check_return_
HRESULT STDMETHODCALLTYPE
RenderFilter::InitializeFilter(
    _In_ IInterFilterCommunicator       *pICommunicator,
    _In_ IPrintPipelinePropertyBag      *pIPropertyBag,
    _In_ IPrintPipelineManagerControl   *pIPipelineControl
    )
{
    DoTraceMessage(RENDERFILTER_TRACE_INFO, L"Initializing Filter");

    if (pICommunicator      == NULL ||
        pIPropertyBag       == NULL ||
        pIPipelineControl   == NULL)
    {
        WPP_LOG_ON_FAILED_HRESULT(E_POINTER);

        return E_POINTER;
    }

    HRESULT hr = S_OK;

    try
    {
        InitializeFilter_throws(
            pICommunicator,
            pIPropertyBag
            );
    }
    CATCH_VARIOUS(hr);

    return hr;
}

//
//Routine Name:
//
//    RenderFilter::InitializeFilter_throws
//
//Routine Description:
//
//    Implements IPrintPipelineFilter initialization. Gets
//    all necessary communication interfaces.
//
//Arguments:
//
//    pICommunicator    - interface to interfilter communicator
//    pIPropertyBag     - interface to pipeline property bag
//
VOID
RenderFilter::InitializeFilter_throws(
    const IInterFilterCommunicator_t   &pICommunicator,
    const IPrintPipelinePropertyBag_t  &pIPropertyBag
    )
{
    //
    // Get the pipeline communication interfaces
    //
    THROW_ON_FAILED_HRESULT(
        pICommunicator->RequestReader(reinterpret_cast<void**>(&m_pReader))
        );
    THROW_ON_FAILED_HRESULT(
        pICommunicator->RequestWriter(reinterpret_cast<void**>(&m_pWriter))
        );

    {
        //
        // Check to ensure that the provided interfaces are as expected.
        // That is, that the GUIDs were correctly listed in the
        // pipeline configuration file
        //
        IXpsDocumentProvider_t pReaderCheck;
        IPrintWriteStream_t pWriterCheck;

        THROW_ON_FAILED_HRESULT(
            m_pReader.QueryInterface(&pReaderCheck)
            );
        THROW_ON_FAILED_HRESULT(
            m_pWriter.QueryInterface(&pWriterCheck)
            );
    }

    //
    // Save a pointer to the Property Bag for further
    // initialization, later.
    //
    m_pIPropertyBag = pIPropertyBag;
}

//
//Routine Name:
//
//    RenderFilter::ShutdownOperation
//
//Routine Description:
//
//    Called asynchronously by the pipeline manager
//    to shutdown filter operation.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    HRESULT
//    S_OK - On success
//
_Check_return_
HRESULT
RenderFilter::ShutdownOperation()
{
    DoTraceMessage(RENDERFILTER_TRACE_INFO, L"Shutting Down Operation");
    m_pLiveness->Cancel();
    return S_OK;
}

//
//Routine Name:
//
//    RenderFilter::StartOperation
//
//Routine Description:
//
//    Called by the pipeline manager to start processing
//    a document. Exception boundary for page processing.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    HRESULT
//    S_OK      - On success
//    Otherwise - Failure
//
_Check_return_
HRESULT
RenderFilter::StartOperation()
{
    HRESULT hr = S_OK;

    DoTraceMessage(RENDERFILTER_TRACE_INFO, L"Starting Operation");

    //
    // Process the Xps Package
    //
    try {
        StartOperation_throws();
    }
    CATCH_VARIOUS(hr);

    m_pWriter->Close();

    return hr;
}

//
//Routine Name:
//
//    RenderFilter::StartOperation_throws
//
//Routine Description:
//
//    Iterates over the 'trunk' parts of the document 
//    and calls appropriate processing methods.
//
//Arguments:
//
//    None
//
void
RenderFilter::StartOperation_throws()
{
    //
    // CoInitialize/CoUninitialize RAII object.
    // COM is inititalized for the lifetime of this method.
    //
    SafeCoInit  coInit;

    IXpsOMObjectFactory_t pOMFactory;

    //
    // Create Xps Object Model Object Factory instance
    //
    THROW_ON_FAILED_HRESULT(
        ::CoCreateInstance(
            __uuidof(XpsOMObjectFactory),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IXpsOMObjectFactory),
            reinterpret_cast<LPVOID*>(&pOMFactory)
            )
        );

    IOpcFactory_t pOpcFactory;

    //
    // Create Opc Object Factory instance
    //
    THROW_ON_FAILED_HRESULT(
        ::CoCreateInstance(
            __uuidof(OpcFactory),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IOpcFactory),
            reinterpret_cast<LPVOID*>(&pOpcFactory)
            )
        );


    //
    // Create the Print Ticket Handler
    //
    PrintTicketHandler_t pPrintTicketHandler = 
                            PrintTicketHandler::CreatePrintTicketHandler(
                                                    m_pIPropertyBag
                                                    );

    IUnknown_t pUnk;

    //
    // Get first part
    //
    THROW_ON_FAILED_HRESULT(m_pReader->GetXpsPart(&pUnk));

    while (m_pLiveness->IsAlive() &&
           pUnk != NULL)
    {
        IXpsDocument_t               pDoc;
        IFixedDocumentSequence_t     pFDS;
        IFixedDocument_t             pFD;
        IFixedPage_t                 pFP;

        if (SUCCEEDED(pUnk.QueryInterface(&pFP)))
        {
            DoTraceMessage(RENDERFILTER_TRACE_VERBOSE, L"Handling a Page");

            pPrintTicketHandler->ProcessPart(pFP);
        }
        else if (SUCCEEDED(pUnk.QueryInterface(&pFD)))
        {
            pPrintTicketHandler->ProcessPart(pFD);
        }
        else if (SUCCEEDED(pUnk.QueryInterface(&pFDS)))
        {
            pPrintTicketHandler->ProcessPart(pFDS);
        }
        else if (SUCCEEDED(pUnk.QueryInterface(&pDoc)))
        {
            //
            // Do nothing with the XML Document part
            //
        }
        else
        {
            DoTraceMessage(RENDERFILTER_TRACE_INFO, L"Unhandled Document Part");
        }

        pUnk.Release();

        //
        // Get Next Part
        //
        THROW_ON_FAILED_HRESULT(m_pReader->GetXpsPart(&pUnk));
    }
}

} // namespace XpsRenderFilter

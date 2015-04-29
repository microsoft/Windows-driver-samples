/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplflt.cpp

Abstract:

   Base filter class for stream and Xps filters. This class implements
   the IPrintPipelineFilter methods common to both Xps and stream filters.

Known Issues:

    Request shutdown does not pass an IImgErrorInfo pointer

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdsmplflt.h"

/*++

Routine Name:

    CXDSmplFilter::CXDSmplFilter

Routine Description:

    CXDSmplFilter class constructor

Arguments:

    None

Return Value:

    None

--*/
CXDSmplFilter::CXDSmplFilter() :
    CUnknown<IPrintPipelineFilter>(IID_IPrintPipelineFilter),
    m_bFilterFinished(FALSE)
{
    VERBOSE("Constructing filter\n");
}


/*++

Routine Name:

    CXDSmplFilter::~CXDSmplFilter

Routine Description:

    CXDSmplFilter class destructor

Arguments:

    None

Return Value:

    None

--*/
CXDSmplFilter::~CXDSmplFilter()
{
    VERBOSE("Destroying filter\n");

    //
    // Make sure FilterFinished() is called
    //
    FilterFinished();
}

/*++

Routine Name:

    CXDSmplFilter::InitializeFilter

Routine Description:

    This is the IPrintPipelineFilter::InitializeFilter implementation used
    by all XPS document interface filters. This is called by the print filter
    pipeline manager before IPrintPipelineFilter::StartOperation.

Arguments:

    pIInterFilterCommunicator - Pointer to the inter filter communicator
    pIPropertyBag             - Pointer to the property bag
    pIPipelineControl         - Pointer to the pipeline control interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplFilter::InitializeFilter(
    _In_ IInterFilterCommunicator*     pIInterFilterCommunicator,
    _In_ IPrintPipelinePropertyBag*    pIPropertyBag,
    _In_ IPrintPipelineManagerControl* pIPipelineControl
    )
{
    VERBOSE("Initializing filter\n");

    HRESULT hr = S_OK;

    if (FAILED(hr = CHECK_POINTER(pIInterFilterCommunicator, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(pIPropertyBag, E_POINTER)) ||
        FAILED(hr = CHECK_POINTER(pIPipelineControl, E_POINTER)))
    {
        RequestShutdown(hr);
    }
    else
    {
        m_pPrintPipeManager = pIPipelineControl;
        m_pInterFltrComm = pIInterFilterCommunicator;
        m_pPrintPropertyBag = pIPropertyBag;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplFilter::ShutdownOperation

Routine Description:

    This is the IPrintPipelineFilter::ShutdownOperation implementation used
    by all XPS document interface filters. This is called by the print filter
    pipeline manager when the filter is complete or an error forces the
    pipeline to be shutdown

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplFilter::ShutdownOperation(
    VOID
    )
{
    VERBOSE("Shutting down filter\n");

    FilterFinished();
    return S_OK;
}

/*++

Routine Name:

    CXDSmplFilter::FilterFinished

Routine Description:

    This routine is called when the filter is finished. Filter finished should
    only be called once so this routine protects against multiple calls from the
    same filter.

Arguments:

    None

Return Value:

    None

--*/
VOID
CXDSmplFilter::FilterFinished(
    VOID
    )
{
    if (!m_bFilterFinished)
    {
        VERBOSE("Finishing filter\n");

        if (m_pPrintPipeManager != NULL)
        {
            m_pPrintPipeManager->FilterFinished();
            m_bFilterFinished = TRUE;
        }
    }
}

/*++

Routine Name:

    CXDSmplFilter::RequestShutdown

Routine Description:

    This routine lets derived classes request the filter to shutdown
    without having to know about the pipeline manager explicitly

Arguments:

    hr - The HRESULT value to be passed to the pipeline managers RequestShutdown method.

Return Value:

    None

--*/
VOID
CXDSmplFilter::RequestShutdown(
    _In_ HRESULT hr
    )
{
    VERBOSE("Requesting shutdown\n");

    if (m_pPrintPipeManager != NULL)
    {
#pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_1, "MSDN requires that pReason be NULL.")
        m_pPrintPipeManager->RequestShutdown(hr, NULL);
    }
}

/*++

Routine Name:

    CXDSmplFilter::InitializePrintTicketManager

Routine Description:

    This routine initializes the PrintTicket manager with the default devmode

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplFilter::InitializePrintTicketManager(
    VOID
    )
{
    ASSERTMSG(m_pPrintPropertyBag != NULL, "NULL property bag pointer\n");

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(m_pPrintPropertyBag, E_PENDING)))
    {
        //
        // Get the printer name and user PrintTicket from the property bag
        //
        CComVariant varName;
        CComVariant varPTReadStreamFactory;

        //
        // Avoid CComVariant if getting the XPS_FP_USER_TOKEN property.
        // Please refer to http://go.microsoft.com/fwlink/?LinkID=255534 for detailed information.
        //
        VARIANT     varUserToken;
        VariantInit(&varUserToken);

        if (SUCCEEDED(hr = m_pPrintPropertyBag->GetProperty(XPS_FP_USER_PRINT_TICKET, &varPTReadStreamFactory)) &&
            SUCCEEDED(hr = m_pPrintPropertyBag->GetProperty(XPS_FP_PRINTER_NAME, &varName)) &&
            SUCCEEDED(hr = m_pPrintPropertyBag->GetProperty(XPS_FP_USER_TOKEN, &varUserToken)))
        {
            //
            // Retrieve the PrintReadStream for the user PrintTicket
            //
            CComPtr<IUnknown>                pUnk(varPTReadStreamFactory.punkVal);
            CComPtr<IPrintReadStreamFactory> pPrintReadStreamFactory(NULL);
            CComPtr<IPrintReadStream>        pPrintReadStream(NULL);
            if (SUCCEEDED(hr = pUnk.QueryInterface(&pPrintReadStreamFactory)) &&
                SUCCEEDED(hr = pPrintReadStreamFactory->GetStream(&pPrintReadStream)))
            {
                //
                // Initialise the PT manager with the user PrintTicket and device name
                //
                hr = m_ptManager.Initialise(pPrintReadStream, varName.bstrVal, varUserToken.byref);
            }
        }

        VariantClear(&varUserToken);
    }

    ERR_ON_HR(hr);
    return hr;
}


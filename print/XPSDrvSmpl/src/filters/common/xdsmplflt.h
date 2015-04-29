/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplflt.h

Abstract:

   Base filter class for stream and Xps filters. This class implements
   the IPrintPipelineFilter methods common to both Xps and stream filters.

--*/

#pragma once

#include "cunknown.h"
#include "ptmanage.h"

class CXDSmplFilter : public CUnknown<IPrintPipelineFilter>
{
public:
    CXDSmplFilter();

    virtual ~CXDSmplFilter();

    //
    // IImgPipelineFilter methods
    //
    virtual HRESULT STDMETHODCALLTYPE
    InitializeFilter(
        _In_ IInterFilterCommunicator*     pINegotiation,
        _In_ IPrintPipelinePropertyBag*    pIPropertyBag,
        _In_ IPrintPipelineManagerControl* pIPipelineControl
        );

    virtual HRESULT STDMETHODCALLTYPE
    ShutdownOperation(
        VOID
        );

protected:
    VOID
    FilterFinished(
        VOID
        );

    VOID
    RequestShutdown(
        _In_ HRESULT hr
        );

    HRESULT
    InitializePrintTicketManager(
        VOID
        );

protected:
    CComPtr<IInterFilterCommunicator>     m_pInterFltrComm;

    CComPtr<IPrintPipelineManagerControl> m_pPrintPipeManager;

    CComPtr<IPrintPipelinePropertyBag>    m_pPrintPropertyBag;

    CPTManager                            m_ptManager;

    BOOL                                  m_bFilterFinished;
};


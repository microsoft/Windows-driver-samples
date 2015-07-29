//**@@@*@@@****************************************************
//
// Microsoft Windows 
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:            TopologyExaminers.cpp
//
// Description:         
//
// --------------------------------------------------------------------------------


#include "stdafx.h"
#include "DeviceTopology.h"
#include "TopologyExaminers.h"

_Analysis_mode_(_Analysis_code_type_user_driver_)

// ----------------------------------------------------------------------------
// Function:
//      CFormatExaminer::CFormatExaminer
//
// Description:
//      CFormatExaminer constructor
// ----------------------------------------------------------------------------
CFormatExaminer::CFormatExaminer
(
    PKSDATAFORMAT   pKsFormat,
    ULONG           cbFormat
)
:  m_bSatisfied(FALSE),
   m_pFormatReq(NULL)
{
    ATLASSERT(pKsFormat);
    ATLASSERT(cbFormat >= sizeof(KSDATAFORMAT));

    // If any of this fails, then Examine will fail
    if (cbFormat >= sizeof(KSDATAFORMAT))
    {
        m_pFormatReq = (PKSDATAFORMAT)(new BYTE[cbFormat]);
        CopyMemory(m_pFormatReq, pKsFormat, cbFormat);
        if (cbFormat < m_pFormatReq->FormatSize)
            m_pFormatReq->FormatSize = cbFormat;
    }
}


// ----------------------------------------------------------------------------
// Function:
//      CFormatExaminer::~CFormatExaminer
//
// Description:
//      CFormatExaminer destructor
// ----------------------------------------------------------------------------
CFormatExaminer::~CFormatExaminer()
{
    SAFE_DELETE_ARRAY(m_pFormatReq);
}


// ----------------------------------------------------------------------
// Function:
//      CFormatExaminer::Examine
//
// Description: 
//      Implementation of IExaminer::Examine.  This method checks the supplied
//      path an IConnector that support the KS streaming format (KSDATAFORMAT)
//      that this examiner was set up to look for.
//
// Parameters:
//      pPath - [in] List of parts to examine
//
// Remarks:
//      This examiner only cares about the first part in the list, which is
//      always a connector or an endpoint.  Therefore it only needs to make
//      sure that there is a zeroth element in the list and examine that.
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------
HRESULT CFormatExaminer::Examine
(
    IPartsList* pPath
)
{
    ATLASSERT(pPath != NULL);

    HRESULT         hr = S_OK;
    UINT            cParts;
    CComPtr<IPart>  spPart;

    // Make sure there is at least 1 part
    hr = pPath->GetCount(&cParts);
    IF_FAILED_JUMP(hr, Exit);

    IF_TRUE_ACTION_JUMP((cParts == 0), hr = E_NOTFOUND, Exit);
    
    // This examiner only cares about Connectors so don't bother walking the
    // path, just look at the last in the list, which should be a connector.
    hr = pPath->GetPart(0, &spPart);
    IF_FAILED_JUMP(hr, Exit);

    hr = ExaminePart(spPart);

Exit:
    return hr;
}


// ----------------------------------------------------------------------
// Function:
//      CFormatExaminer::ExaminePart
//
// Description: 
//      Tests a single IPart against the examination criteria, namely be a
//      connector that supports whatever KS Streaming format (KSDATAFORMAT)
//      this examiner was set up to look for
//
// Parameters:
//      pIPart - [in] Part to examine
//
// Remarks:
//      1) Make sure the connector is of type Software_IO
//      2) Activate IKsFormatSupport on the connector
//      3) Call IKsFormatSupport::IsFormatSupported(m_pFormatReq).
//
// ----------------------------------------------------------------------
HRESULT CFormatExaminer::ExaminePart
(
    IPart* pIPart
)
{
    ATLASSERT(pIPart);

    HRESULT hr = S_OK;

    // 1) Check category and make sure it is a software_IO pin
    ConnectorType               conType;
    CComPtr<IConnector>         spConnector;
    CComPtr<IKsFormatSupport>   spFormatSupport;

    hr = pIPart->QueryInterface(__uuidof(IConnector), (void**)&spConnector);
    IF_FAILED_JUMP(hr, Exit);

    hr = spConnector->GetType(&conType);
    IF_FAILED_JUMP(hr, Exit);

    IF_TRUE_ACTION_JUMP((conType != Software_IO), hr = E_NOTFOUND, Exit);

    // 2) Activate IKsFormatSupport interface
    hr = pIPart->Activate(CLSCTX_INPROC_SERVER, __uuidof(IKsFormatSupport), (void**)&spFormatSupport);
    IF_FAILED_JUMP(hr, Exit);

    // 3) Ask the interface if this format is supported
    hr = spFormatSupport->IsFormatSupported(m_pFormatReq, m_pFormatReq->FormatSize, &m_bSatisfied);

Exit:
    return hr;    
}

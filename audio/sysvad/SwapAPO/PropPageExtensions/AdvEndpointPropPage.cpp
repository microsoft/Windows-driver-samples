//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    AdvEndpointPropPage.cpp
//
// Abstract:    Implementation of CAdvEndpointPropPage
//
// ----------------------------------------------------------------------------


#include "stdafx.h"
#include <mmdeviceapi.h>
#include <DeviceTopology.h>
#include "UIWidgets.h"
#include "Parts.h"
#include "TopologyExaminers.h"
#include "AdvEndpointPropPage.h"
#include <functiondiscoverykeys.h>

_Analysis_mode_(_Analysis_code_type_user_driver_)

#define MAX_SEARCH_DEPTH    5   // when searching for a host pin or interface

static IID s_rgAdvCtrlInterfaces[] =
{
    __uuidof(IAudioLoudness),
    __uuidof(IAudioAutoGainControl),
    __uuidof(IDeviceSpecificProperty)
};


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::CAdvEndpointPropPage
//
// Description:
//      CAdvEndpointPropPage constructor
// ----------------------------------------------------------------------------
CAdvEndpointPropPage::CAdvEndpointPropPage()
:   m_pAudioExtParams(NULL)
{
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::~CAdvEndpointPropPage
//
// Description:
//      CAdvEndpointPropPage destructor
// ----------------------------------------------------------------------------
CAdvEndpointPropPage::~CAdvEndpointPropPage()
{
    CUIWidget* pWidget;

    while (m_lstWidgets.RemoveHead(&pWidget))
    {
        SAFE_DELETE(pWidget);
    }

    SAFE_RELEASE(m_pAudioExtParams->pEndpoint);
    SAFE_RELEASE(m_pAudioExtParams->pPnpInterface);
    SAFE_RELEASE(m_pAudioExtParams->pPnpDevnode);

    SAFE_DELETE(m_pAudioExtParams);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::MakeNewWidget
//
// Description:
//      Adds a widget to the list
//
// Parameters:
//      pPart - [in] Part interface
//      iid - [in] Advanced control IID which controls the kind of widget
//                 that gets created
//      nCtrlId - [in] ID of the control
//
// Return:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CAdvEndpointPropPage::MakeNewWidget
(
    IPart*  pPart,
    REFIID  iid,
    UINT    nCtrlId
)
{
    UNREFERENCED_PARAMETER(nCtrlId);

    HRESULT     hr = S_OK;
    LISTPOS     posChk;
    CUIWidget*  pNewWidget = NULL;

    if ((iid == __uuidof(IAudioLoudness)) ||
        (iid == __uuidof(IAudioAutoGainControl)))
    {
        pNewWidget = new CBooleanWidget(pPart, iid);
        IF_TRUE_ACTION_JUMP((pNewWidget == NULL), hr = E_OUTOFMEMORY, Exit);
    }
    else if (iid == __uuidof(IDeviceSpecificProperty))
    {
        CComPtr<IDeviceSpecificProperty>    spDevSpec;
        VARTYPE                             vt;

        hr = pPart->Activate(CLSCTX_INPROC_SERVER, __uuidof(IDeviceSpecificProperty), (void**)&spDevSpec);
        IF_FAILED_JUMP(hr, Exit);

        hr = spDevSpec->GetType(&vt);
        IF_FAILED_JUMP(hr, Exit);

        if (vt == VT_BOOL)
        {
            pNewWidget = new CBooleanWidget(pPart, iid);
            IF_TRUE_ACTION_JUMP((pNewWidget == NULL), hr = E_OUTOFMEMORY, Exit);
        }
        else if (vt == VT_I4)
        {
            pNewWidget = new CLongWidget<LONG>(pPart);
            IF_TRUE_ACTION_JUMP((pNewWidget == NULL), hr = E_OUTOFMEMORY, Exit);
        }
        else if (vt == VT_UI4)
        {
            pNewWidget = new CLongWidget<ULONG>(pPart);
            IF_TRUE_ACTION_JUMP((pNewWidget == NULL), hr = E_OUTOFMEMORY, Exit);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            goto Exit;
        }
    }

    ATLASSERT(pNewWidget);

    posChk = m_lstWidgets.AddTail(pNewWidget);
    if (posChk == NULL)
    {
        hr = E_OUTOFMEMORY;
        SAFE_DELETE(pNewWidget);
    }

Exit:
    return hr;
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::GetDeviceFriendlyName
//
// Description:
//      Retrieves the endpoint's friendly name
//
// Parameters:
//      ppNameOut - [out] The friendly name of the endpoint
//
// Return values:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CAdvEndpointPropPage::GetDeviceFriendlyName
(
    _Outptr_result_maybenull_ LPWSTR* ppNameOut
)
{
    HRESULT                 hr = S_OK;
    CComPtr<IPropertyStore> spProperties;
    PROPVARIANT             var;

    IF_TRUE_ACTION_JUMP((m_pAudioExtParams == NULL), hr = E_POINTER, Exit);
    IF_TRUE_ACTION_JUMP((ppNameOut == NULL), hr = E_POINTER, Exit);

    *ppNameOut = NULL;

    PropVariantInit(&var);

    if (m_pAudioExtParams->pEndpoint != NULL)
    {
        // Open the PropertyStore for read access
        hr = m_pAudioExtParams->pEndpoint->OpenPropertyStore(STGM_READ, &spProperties);
        IF_FAILED_JUMP(hr, Exit);

        // Retrieve the friendly name of the endpoint
        hr = spProperties->GetValue(PKEY_Device_FriendlyName, &var);
        if (SUCCEEDED(hr) && (var.vt == VT_LPWSTR))
        {
            *ppNameOut = var.pwszVal;
        }
        else
        {
            PropVariantClear(&var);
        }
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::OnInitDialog
//
// Description:
//      Dialog initialization routine
//
// Parameters:
//      hwndDlg - [in] Handle to dialog box
//      wParam - [in] Handle to control to receive the default keyboard focus
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      TRUE to direct the system to set the keyboard focus to the control
//      specified by wParam. Otherwise, it should return FALSE to prevent the
//      system from setting the default keyboard focus.
// ----------------------------------------------------------------------------
BOOL CAdvEndpointPropPage::OnInitDialog
(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam
)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    LPWSTR      pwstrEndpointName = NULL;

    LISTPOS     pos;
    CUIWidget*  pWidget;
    SRECT       rcWidget(34, 144, 0, 0);
    RECT        rcDlg;
    HRESULT     hr = S_OK;
    UINT        id = 1800;

    // Retrieve the endpoint's friendly name
    hr = GetDeviceFriendlyName(&pwstrEndpointName);
    IF_FAILED_JUMP(hr, Exit);

    // Update the property page with retrieved information
    SetWindowText(GetDlgItem(hwndDlg, IDC_EPP_ENDPOINT_NAME), pwstrEndpointName);

    // Initialize and create widgets
    hr = InitializeWidgets();
    if (hr == E_NOTFOUND)
    {
        // If no widgets were found, show a message to depict that
        SetWindowText(GetDlgItem(hwndDlg, IDC_NO_ADV_CONTROLS_FOUND),
                        L"No advanced controls found.");
    }
    else
    {
        // Create widget windows
        ::GetClientRect(hwndDlg, &rcDlg);
        rcWidget.w = rcDlg.right - rcDlg.left - 28;
        rcWidget.h = 14;

        pos = m_lstWidgets.GetHeadPosition();
        while (pos)
        {
            pWidget = NULL;
            m_lstWidgets.GetNext(pos, &pWidget);

            hr = pWidget->Create(hwndDlg, rcWidget, id);
            if (SUCCEEDED(hr))
            {
                id++;

                // offset to next widget
                rcWidget.y = rcWidget.y + rcWidget.h + 12;
            }
        }
    }

Exit:
    SAFE_COTASKMEMFREE(pwstrEndpointName);
    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::OnApply
//
// Description:
//      Handle the pressing of the apply button
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//      wParam - [in] Handle to the control to receive the default keyboard focus
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      TRUE to set keyboard focus on control
// ----------------------------------------------------------------------------
BOOL CAdvEndpointPropPage::OnApply
(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam
)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    CUIWidget*  pWidget;
    LISTPOS     pos;

    pos = m_lstWidgets.GetHeadPosition();

    while (pos)
    {
        pWidget = NULL;
        m_lstWidgets.GetNext(pos, &pWidget);
        pWidget->CommitValue();
    }

    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

    return(TRUE);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::OnCheckBoxClicked
//
// Description:
//      Handle the clicking of the check boxes
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//      wParam - [in] Handle to the control to receive the default keyboard focus
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      FALSE to not set default keyboard focus
// ----------------------------------------------------------------------------
BOOL CAdvEndpointPropPage::OnCheckBoxClicked
(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam
)
{
    UNREFERENCED_PARAMETER(lParam);

    CUIWidget*  pWidget;
    LISTPOS     pos;

    pos = m_lstWidgets.GetHeadPosition();

    while (pos)
    {
        pWidget = NULL;
        m_lstWidgets.GetNext(pos, &pWidget);

        if (pWidget->OwnsCtrlId(LOWORD(wParam)))
        {
            pWidget->OnClick();
        }
    }

    // Enable the Apply button upon user changing the control
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

    return(FALSE);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::DialogProcPage1
//
// Description:
//      Callback for property page
//
// Parameters:
//      hwndDlg - [in] Handle to the dialog box
//      uMsg - [in] Specifies the message
//      wParam - [in] Specifies additional message-specific information
//      lParam - [in] Specifies additional message-specific information
//
// Return values:
//      TRUE if it processed the message, FALSE if not
// ----------------------------------------------------------------------------
INT_PTR CALLBACK CAdvEndpointPropPage::DialogProcPage1
(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    CAdvEndpointPropPage* pthis = (CAdvEndpointPropPage*)(LONG_PTR)GetWindowLongPtr(
                            hwndDlg, GWLP_USERDATA);
    BOOL fRet = FALSE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Extract the context data from PROPSHEETPAGE::lParam
            PROPSHEETPAGE*  pSheetDesc = (PROPSHEETPAGE*)lParam;

            // Create the property page factory class
#pragma warning(push)
#pragma warning(disable: 28197)
            pthis = new CComObject<CAdvEndpointPropPage>();
#pragma warning(pop)
            if (pthis == NULL)
            {
                return(FALSE);
            }

            // Save this object in lParam
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pthis);

            // Keep audio extension parameters passed by the control panel
            pthis->m_pAudioExtParams = (AudioExtensionParams*)pSheetDesc->lParam;

            fRet = pthis->OnInitDialog(hwndDlg, wParam, lParam);
            break;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR FAR*)lParam)->code)
            {
                case PSN_APPLY:
                    if (pthis)
                    {
                        // Apply button pressed
                        fRet = pthis->OnApply(hwndDlg, wParam, lParam);
                    }
                    break;
            }
            break;
        }

        case WM_COMMAND:
        {
            if (pthis)
            {
                // Check box clicked
                fRet = pthis->OnCheckBoxClicked(hwndDlg, wParam, lParam);
            }
            break;
        }

        case WM_DESTROY:
        {
            SAFE_DELETE(pthis);
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, NULL);
            fRet = TRUE;
            break;
        }
    }

    return(fRet);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::PropSheetPageProc
//
// Description:
//      Callback that gets invoked right after page creation or right before
//      before page destruction
//
// Parameters:
//      hwnd - Reserved; must be NULL
//      uMsg - [in] Action flag. PSPCB_ADDREF, PSPCB_CREATE, or PSPCB_RELEASE
//      ppsp - [in, out] Pointer to a PROPSHEETPAGE structure that defines
//             the page being created or destroyed.
//
// Return values:
//      Depends on the value of the uMsg parameter
// ----------------------------------------------------------------------------
UINT CALLBACK CAdvEndpointPropPage::PropSheetPageProc
(
    HWND            hwnd,
    UINT            uMsg,
    LPPROPSHEETPAGE ppsp
)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(ppsp);

    // if (uMsg == PSPCB_CREATE) ...
    return(1);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::Initialize
//
// Description:
//      Implementation of IShellExtInit::Initialize. Initializes a property
//      sheet extension, shortcut menu extension, or drag-and-drop handler.
//
// Parameters:
//      pidlFolder - [in] Address of an ITEMIDLIST structure that uniquely
//                   identifies a folder. For property sheet extensions,
//                   this parameter is NULL.
//      pdtobj - [out] Address of an IDataObject interface object that can be
//               used to retrieve the objects being acted upon.
//      hkeyProgID - [in] Registry key for the file object or folder type.
//
// Return values:
//      Returns NOERROR if successful, or an OLE-defined error value otherwise
// ----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CAdvEndpointPropPage::Initialize
(
    LPCITEMIDLIST   pidlFolder,
    IDataObject*    pdtobj,
    HKEY            hkeyProgID
)
{
    UNREFERENCED_PARAMETER(pidlFolder);
    UNREFERENCED_PARAMETER(pdtobj);
    UNREFERENCED_PARAMETER(hkeyProgID);

    return(S_OK);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::FindHostConnector
//
// Description:
//      This method initiates a search for a software connector that supports
//      the supplied format.  If one is found, it returns a path to that
//      connector.
//
// Parameters:
//      pKsFormat - [in] Requested format
//      cbFormat - [in] Size of format buffer in BYTEs
//      bRejectMixedPaths - [in] If TRUE, stop looking if you find a mix unit
//      ppPath - [out] The path to the Host pin
//
// Remarks:
//      To get the host connector, just take the zeroth element in the
//      resulting path
//
// Return:
//      S_OK on success, E_NOTFOUND if not. Other error code indicates
//      something unexpected.
// ----------------------------------------------------------------------------
HRESULT CAdvEndpointPropPage::FindHostConnector
(
    PKSDATAFORMAT   pKsFormat,
    ULONG           cbFormat,
    BOOL            bRejectMixedPaths,
    IPartsList**    ppPath
)
{
    HRESULT                     hr;
    CPartsList*                 pPathOut = NULL;
    CComPtr<IDeviceTopology>    spTopology;
    UINT                        cConnectors;
    CComPtr<IConnector>         spEndpointConnector;
    CComPtr<IConnector>         spConStart;
    CComQIPtr<IPart>            spPartStart;
    DataFlow                    flow;
    UINT                        cDepth = 0;
    CFormatExaminer*            pExaminer = NULL;

    IF_TRUE_ACTION_JUMP((ppPath == NULL), hr = E_POINTER, Exit);
    *ppPath = NULL;

    ATLASSERT(m_pAudioExtParams->pEndpoint != NULL);

    // Get IDeviceTopology interface for the Endpoint
    hr = m_pAudioExtParams->pEndpoint->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&spTopology);
    IF_FAILED_JUMP(hr, Exit);

    // Get the connector to start searching from.  By definition, an endpoint device can only have 1 connector.
    hr = spTopology->GetConnectorCount(&cConnectors);
    IF_FAILED_JUMP(hr, Exit);

    ATLASSERT(cConnectors == 1);
    IF_TRUE_ACTION_JUMP((cConnectors != 1), hr = E_FAIL, Exit);

    // Since an endpoint device can only have 1 connector, get the connector at index 0
    hr = spTopology->GetConnector(0, &spEndpointConnector);
    IF_FAILED_JUMP(hr, Exit);

    hr = spEndpointConnector->GetConnectedTo(&spConStart);
    IF_FAILED_JUMP(hr, Exit);

    // get the dataflow (required by Search method)
    hr = spConStart->GetDataFlow(&flow);
    IF_FAILED_JUMP(hr, Exit);

    // QI for IPart (required by Search method)
    spPartStart = spConStart;

    // Create an examiner that looks for a pin with the specified format
    pExaminer = new CFormatExaminer(pKsFormat, cbFormat);
    IF_TRUE_ACTION_JUMP((pExaminer == NULL), hr = E_OUTOFMEMORY, Exit);

    // Create a new parts list
#pragma warning(push)
#pragma warning(disable: 28197)
    pPathOut = new CPartsList();
#pragma warning(pop)
    IF_TRUE_ACTION_JUMP((pPathOut == NULL), hr = E_OUTOFMEMORY, Exit);

    // Start looking
    hr = Search(spPartStart, flow, pExaminer, pPathOut, cDepth, bRejectMixedPaths);
    if (bRejectMixedPaths && (hr == E_NOTFOUND))
    {
        // Don't actually reject mixed paths, just avoid them
        hr = Search(spPartStart, flow, pExaminer, pPathOut, cDepth, FALSE);
    }
    IF_FAILED_JUMP(hr, Exit);

    hr = pPathOut->QueryInterface(__uuidof(IPartsList), (void**)ppPath);

Exit:
    SAFE_RELEASE(pPathOut);

    SAFE_DELETE(pExaminer);

    return hr;
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::Search
//
// Description:
//      Searches the device topology for parts that satisfy the supplied
//      examiner.
//
// Parameters:
//      pPartStart - [in] The IPart to start looking at
//      flowStart - [in] The flow of the starting part.  This never changes
//                  so it is passed as an argument.
//      pExaminer - [in] The examiner that tests each IPart against some criteria
//      pPathAggregate - [out] The cumulative path that has been searched
//      cDepth - [in] Stop looking if we reach this recursion depth
//      bRejectMixedPaths - [in] If TRUE, then stop looking if we encounter a
//                          MIX unit
//
// Remarks:
//      The search may span several FunctionInstances connected by DeviceModel
//      IConnections.  The algorithm works as follows:
//
//      1)  Each connector in the model is tested against the following
//          (in order):
//          a) Is the flow opposite that of the starting part?
//             If no then goto next connector.
//          b) Does there exist a path from the starting part to this
//             connector?  If no then goto next connector.
//          c) Is the supplied examiner satisfied by the path?
//             If yes, then return.
//          d) Is this connector connected to another connector?
//             If yes, then add the connector to a list of "deferred
//             connectors" to be checked if no other connectors in this model
//             satisfy (c)
//
//      2)  If the function still hasn't returned, then for each connector in
//          the list generated in step 1d:
//          a) Get the connector that it is connected to
//          b) Recurse into this function with "that" connector as the starting
//             part
//
//      The point of the list of "deferred connectors" is to minimize the
//      length of the paths that we find.  e.g. if two viable paths exist, one
//      (A) which spans 3 models, and another (B) which spans only 2, then B
//      will be found first, regardless of the ordering of connections in the
//      models involved.
//
//      Note that when a path is found, all of the parts involved in that path
//      are aggregated into pPathAggregate IN REVERSE ORDER.  In general, all
//      the parts in pPathAggregate will not belong to a single model, so when
//      using pPathAggregate, you must use IPart::GetTopologyObject to get the
//      model and/or FunctionInstance of the part (i.e. don't cache the value
//      of either).
//
// Return:
//      S_OK if successful
// ----------------------------------------------------------------------------
HRESULT CAdvEndpointPropPage::Search
(
    IPart*      pPartStart,
    DataFlow    flowStart,
    IExaminer*  pExaminer,
    CPartsList* pPathAggregate,
    UINT        cDepth,
    BOOL        bRejectMixedPaths
)
{
    HRESULT             hr = S_OK;
    TList<IConnector>   lstDeferredConnectors;
    BOOL                bHaveViableConnector = FALSE;

    {
        CComPtr<IDeviceTopology>    spTopology;
        UINT                        cConnectors;

        // Bail if we are recursing too deep
        IF_TRUE_ACTION_JUMP((cDepth == MAX_SEARCH_DEPTH), hr = E_NOTFOUND, Exit);

        // Get DeviceModel and DeviceModelUtil interfaces associated with pPartStart.
        // We can't just pass these as params, because each time this function is called
        // recursively, we are talking about a part (pPartStart) on a different device.
        hr = pPartStart->GetTopologyObject(&spTopology);
        IF_FAILED_JUMP(hr, Exit);

        //
        // [Step 1] Examine each connector.
        //
        hr = spTopology->GetConnectorCount(&cConnectors);
        IF_FAILED_JUMP(hr, Exit);

        for (UINT c = 0; (c < cConnectors) && !bHaveViableConnector; c++)
        {
            CComPtr<IConnector> spConTest;
            DataFlow            flowTest;
            CComPtr<IPartsList> spPath;
            CComPtr<IPart>      spPartTest;
            BOOL                bConnectorIsConnected;

            hr = spTopology->GetConnector(c, &spConTest);
            IF_FAILED_JUMP(hr, Exit);

            hr = spConTest->GetDataFlow(&flowTest);
            IF_FAILED_JUMP(hr, Exit);

            // [1a] We only care about connectors whose dataflow is opposite that of pConStart
            // (i.e. on the other side of the topology)
            if (flowTest == flowStart)
                continue;

            // QI for IPart
            spPartTest = spConTest;

            ATLASSERT(spPartTest);
            IF_TRUE_ACTION_JUMP((spPartTest == NULL), hr = E_NOINTERFACE, Exit);

            // [1b] See if there exists a path from one to the other
            hr = spTopology->GetSignalPath(pPartStart, spPartTest, bRejectMixedPaths, &spPath);

            // If no path exists from here to there, then try the next connector
            if (hr != S_OK)
                continue;

            // [1c] Let pExaminer take a look at this path, but don't worry if it fails
            pExaminer->Examine(spPath);

            // See if pExaminer has found what it was looking for yet
            if (pExaminer->IsSatisfied())
            {
                bHaveViableConnector = TRUE;

                // add all of the parts in spPath to pPathAggregate
                hr = pPathAggregate->AddParts(spPath);
                break;
            }

            // [1d] See if the connector is connected.  If so, add to the list of deferred
            // connectors
            hr = spConTest->IsConnected(&bConnectorIsConnected);

            if (SUCCEEDED(hr) && bConnectorIsConnected)
            {
                LISTPOS     posChk;
                IConnector* pConDeferred;

                // Get another interface pointer that is NOT a CComPtr, so it doesn't
                // get released until we want (note that .Detach means that when spConTest
                // goes out of scope it won't release the interface)
                pConDeferred = spConTest.Detach();

                // Add to list of connectors to look more closely at
                posChk = lstDeferredConnectors.AddTail(pConDeferred);

                if (posChk == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    pConDeferred->Release();
                    goto Exit;
                }
            }
        }

        if (!bHaveViableConnector)
        {
            //
            // [Step 2] Jump to connected devices and test them.
            //

            LISTPOS pos = lstDeferredConnectors.GetHeadPosition();

            while (pos != NULL)
            {
                CComPtr<IConnector>     spConConnectedTo;
                IConnector*             pConDeferred = NULL;   // this will be cleaned up at the end of the function
                CComPtr<IPart>          spPartConnectedTo;
                CComPtr<IPartsList>     spPath;
                CComQIPtr<IPart>        spPartDeferred;

                // Get the deferred connector.  RefCount should be 1
                lstDeferredConnectors.GetNext(pos, &pConDeferred);

                // Get the part (a connector) that this connector is connected to
                hr = pConDeferred->GetConnectedTo(&spConConnectedTo);
                IF_FAILED_JUMP(hr, Exit);

                // Search that device model as well.  Note that we can just reuse the dataflow here
                spPartConnectedTo = spConConnectedTo;
                ATLASSERT(spPartConnectedTo != NULL);
                IF_TRUE_ACTION_JUMP((spPartConnectedTo == NULL), hr = E_NOINTERFACE, Exit);

                // Recursion
                hr = Search(spPartConnectedTo, flowStart, pExaminer, pPathAggregate, cDepth + 1, bRejectMixedPaths);
                if (FAILED(hr))
                    continue;

                // We found what we were looking for!
                bHaveViableConnector = TRUE;

                // We need to figure out how we got here again and add that path to pPath
                spPartDeferred = pConDeferred;
                ATLASSERT(spPartDeferred != NULL);

                // Note:  If we fail past here, pPathAggregate will contain some parts.  These will
                // be cleaned up by CPartsList destructor
                IF_TRUE_ACTION_JUMP((spPartDeferred == NULL), hr = E_NOINTERFACE, Exit);

                hr = spTopology->GetSignalPath(pPartStart, spPartDeferred, bRejectMixedPaths, &spPath);
                IF_FAILED_JUMP(hr, Exit);

                // Add all of the parts in spPath to pPathAggregate
                hr = pPathAggregate->AddParts(spPath);
                IF_FAILED_JUMP(hr, Exit);

                break;
            }
        }
    }

Exit:
    // Cleanup
    IConnector* pConDelete;
    while (lstDeferredConnectors.RemoveHead(&pConDelete))
    {
        SAFE_RELEASE(pConDelete);
    }

    // Adjust result.  If there were no errors, but we didn't find what
    // we were searching for, then return "not found"
    if (SUCCEEDED(hr) && (!bHaveViableConnector))
    {
        hr = E_NOTFOUND;
    }

    return hr;
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::InitializeWidgets
//
// Description:
//      Looks for parts in the path that support any of these control
//      interfaces:
//          -- IAudioLoudness
//          -- IAudioAutoGainControl
//          -- IDeviceSpecificProperty
//
// Return:
//      S_OK if at least one control found
//      E_NOTFOUND if no controls found
//      other if unexpected error
//
// ---------------------------------------------------------------------------
HRESULT CAdvEndpointPropPage::InitializeWidgets()
{
    HRESULT                     hr = S_OK;
    HRESULT                     hrWarn = S_OK;
    CComPtr<IPartsList>         spPath;
    UINT                        cParts;
    BOOL                        bFoundSomething = FALSE;
    UINT                        nCtrlId = 900;

    // Note: Can use the endpoint format instead of this ...
    KSDATAFORMAT                format;
    ZeroMemory(&format, sizeof(format));

    format.FormatSize = sizeof(KSDATAFORMAT);
    format.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    format.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    // See if we can find a path to any old PCM host pin by only passing sizeof(KSDATAFORMAT)
    hr = FindHostConnector(&format, sizeof(KSDATAFORMAT), FALSE, &spPath);
    IF_FAILED_JUMP(hr, Exit);

    //
    hr = spPath->GetCount(&cParts);
    IF_FAILED_JUMP(hr, Exit);

    // Look for parts that support any of the interfaces in s_rgAdvCtrlInterfaces
    for (UINT i = 0; i < cParts; i++)
    {
        UINT            cInterfaces;
        CComPtr<IPart>  spPart;

        hr = spPath->GetPart(i, &spPart);
        IF_FAILED_JUMP(hr, Exit);

        hr = spPart->GetControlInterfaceCount(&cInterfaces);
        IF_FAILED_JUMP(hr, Exit);

        for (UINT j = 0; j < cInterfaces; j++)
        {
            CComPtr<IControlInterface>  spInterfaceDesc;
            GUID    iid;

            hr = spPart->GetControlInterface(j, &spInterfaceDesc);
            IF_FAILED_JUMP(hr, Exit);

            hr = spInterfaceDesc->GetIID(&iid);
            IF_FAILED_JUMP(hr, Exit);

            for (int k = 0; k < ARRAYSIZE(s_rgAdvCtrlInterfaces); k++)
            {
                if (iid == s_rgAdvCtrlInterfaces[k])
                {
                    bFoundSomething = TRUE;

                    hrWarn = MakeNewWidget(spPart, iid, nCtrlId++);
                }
            }
        }
    }

    if (!bFoundSomething)
    {
        hr = E_NOTFOUND;
    }

Exit:
    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::AddPages
//
// Description:
//      Implementation of IShellPropSheetExt::AddPages. Adds one or more pages
//      to a property sheet that the Shell displays for a file object.
//
// Parameters:
//      lpfnAddPage - [in] Address of a function that the property sheet
//                    handler calls to add a page to the property sheet. The
//                    function takes a property sheet handle returned by the
//                    CreatePropertySheetPage function and the lParam parameter
//                    passed to the AddPages method.
//      lParam - [in] Parameter to pass to the function specified by the
//               lpfnAddPage method.
//
// Return values:
//      Returns S_OK if successful. If the method fails, an OLE-defined error
//      code is returned
// ----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE CAdvEndpointPropPage::AddPages
(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,    // See PrSht.h
    LPARAM                  lParam          // Used by caller, don't modify
)
{
    HRESULT                 hr = S_OK;
    PROPSHEETPAGE           psp;
    HPROPSHEETPAGE          hPage1 = NULL;
    AudioExtensionParams*   pAudioParams = (AudioExtensionParams*)lParam;
#pragma warning(push)
#pragma warning(disable: 28197)
    AudioExtensionParams*   pAudioParamsCopy = new AudioExtensionParams;
#pragma warning(pop)

    if (pAudioParamsCopy == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Make a copy of the params
    CopyMemory(pAudioParamsCopy, pAudioParams, sizeof(AudioExtensionParams));
    SAFE_ADDREF(pAudioParams->pEndpoint);
    SAFE_ADDREF(pAudioParams->pPnpInterface);
    SAFE_ADDREF(pAudioParams->pPnpDevnode);

    // Initialize property page params and create page
    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT | PSP_USECALLBACK;
    psp.hInstance     = _AtlBaseModule.GetModuleInstance();
    psp.hIcon         = 0;
    psp.pcRefParent   = (UINT*)&m_dwRef;
    psp.lParam        = (LPARAM)pAudioParamsCopy;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_ADV_ENDPOINT_PROP_PAGE);
    psp.pfnDlgProc    = (DLGPROC)DialogProcPage1;
    psp.pfnCallback   = PropSheetPageProc;

    // Create the property sheet page and add the page
    hPage1 = CreatePropertySheetPage(&psp);
    if (hPage1)
    {
        if (!lpfnAddPage(hPage1, pAudioParams->AddPageParam))
        {
            hr = E_FAIL;
            delete pAudioParamsCopy;
            DestroyPropertySheetPage(hPage1);
        }
        else
        {
            // Add ref for page
            this->AddRef();
        }
    }
    else
    {
        delete pAudioParamsCopy;
        hr = E_OUTOFMEMORY;
    }

    return(hr);
}


// ----------------------------------------------------------------------------
// Function:
//      CAdvEndpointPropPage::ReplacePage
//
// Description:
//      Implementation of IShellPropSheetExt::ReplacePage. Replaces a page in
//      a property sheet for a Control Panel object.
//
// Parameters:
//      uPageID - [in] Identifier of the page to replace
//      lpfnReplacePage - [in] Address of a function that the property sheet
//                        handler calls to replace a page to the property
//                        sheet. The function takes a property sheet handle
//                        returned by the CreatePropertySheetPage function and
//                        the lParam parameter passed to the ReplacePage
//                        method.
//      lParam - [in] Parameter to pass to the function specified by the
//               lpfnReplacePage parameter.
//
// Return values:
//      Returns NOERROR if successful, or an OLE-defined error value otherwise
// ----------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE CAdvEndpointPropPage::ReplacePage
(
    UINT                    uPageID,
    LPFNSVADDPROPSHEETPAGE  lpfnReplaceWith,
    LPARAM                  lParam
)
{
    UNREFERENCED_PARAMETER(uPageID);
    UNREFERENCED_PARAMETER(lpfnReplaceWith);
    UNREFERENCED_PARAMETER(lParam);

    return(S_FALSE);
}

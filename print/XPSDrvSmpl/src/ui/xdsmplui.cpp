/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplui.cpp

Abstract:

   Implementation of the UI plugin. This is responsible for initialising and maintaining
   the property pages used in the XPSDrv feature sample UI.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdexcept.h"
#include "xdstring.h"
#include "xdsmplui.h"
#include "colppg.h"
#include "wmppg.h"
#include "ftrppg.h"
#include "UIProperties.h"

/*++

Routine Name:

    CXDSmplUI::CXDSmplUI

Routine Description:

    CXDSmplUI class constructor.
    Creates a handler class object for every property page added in Unidrv UI Plug-in.
    Each of these handlers is stored in a collection.


Arguments:

    None

Return Value:

    None

--*/
CXDSmplUI::CXDSmplUI() :
    CUnknown<IPrintOemUI>(IID_IPrintOemUI),
    m_pDriverUIHelp(NULL),
    m_pOemCUIPParam(NULL),
    m_pUIProperties(NULL)
{
}

/*++

Routine Name:

    CXDSmplUI::~CXDSmplUI

Routine Description:

    CXDSmplUI class destructor.

Arguments:

    None

Return Value:

    None.

--*/
CXDSmplUI::~CXDSmplUI()
{
    DestroyPropPages();
}

/*++

Routine Name:

    CXDSmplUI::PublishDriverInterface

Routine Description:

    The PublishDriverInterface method allows a user interface plug-in to obtain
    the Unidrv driver's IPrintOemDriverUI interface.

Arguments:

    pIUnknown - Caller-supplied pointer to the IUnknown interface of the driver's
                IPrintOemDriverUI COM Interface.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplUI::PublishDriverInterface(
    _In_ IUnknown* pIUnknown
    )
{
    HRESULT hr = E_FAIL;
    PVOID   pInterface = NULL;

    if (m_pDriverUIHelp == NULL &&
        SUCCEEDED(hr = pIUnknown->QueryInterface(IID_IPrintOemDriverUI, &pInterface)))
    {
        m_pDriverUIHelp = reinterpret_cast<IPrintOemDriverUI*>(pInterface);
        hr = S_OK;
    }

    return hr;
}

/*++

Routine Name:

    CXDSmplUI::GetInfo

Routine Description:

    Unidrv will call this routine to obtain identification information.

Arguments:

    dwMode - Supported modes are OEMGI_GETSIGNATURE and OEMGI_GETVERSION.
    pBuffer - Caller-supplied pointer to memory allocated to receive the information specified by dwMode.
    cbSize - Caller-supplied size of the buffer pointed to by pBuffer.
    pcbNeeded - Caller-supplied pointer to a location to receive the number of bytes written into the buffer pointed to by pBuffer.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplUI::GetInfo(
    _In_  DWORD  dwMode,
    _Out_writes_bytes_(cbSize) PVOID  pBuffer,
    _In_  DWORD  cbSize,
    _Out_ PDWORD pcbNeeded
    )
{
    HRESULT hr = S_OK;

    if ((NULL == pcbNeeded) ||
        ((OEMGI_GETSIGNATURE != dwMode) &&
         (OEMGI_GETVERSION != dwMode) &&
         (OEMGI_GETREQUESTEDHELPERINTERFACES != dwMode)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        hr = E_INVALIDARG;
    }
    else
    {
        *pcbNeeded = sizeof(DWORD);

        if ((cbSize < *pcbNeeded) ||
            (NULL == pBuffer))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            switch (dwMode)
            {
                case OEMGI_GETSIGNATURE:
                {
                    *reinterpret_cast<PDWORD>(pBuffer) = OEM_SIGNATURE;
                }
                break;

                case OEMGI_GETVERSION:
                {
                    *reinterpret_cast<PDWORD>(pBuffer) = OEM_VERSION;
                }
                break;

                default:
                {
                    *pcbNeeded = 0;
                    hr = E_NOTIMPL;
                }
                break;
            }
        }
    }

    ERR_ON_HR_EXC(hr, E_NOTIMPL);
    return hr;
}

/*++

Routine Name:

    CXDSmplUI::DevMode

Routine Description:

    Performs operation on UI Plugins Private DevMode Members.
    Called via IPrintOemUI::DevMode

Arguments:

    dwMode - Supported modes are OEMDM_SIZE, OEMDM_DEFAULT, OEMDM_CONVERT and OEMDM_MERGE.
    pOemDMParam - Caller-supplied pointer to an OEMDMPARAM structure.
                  Dependent on which dwMode is set.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplUI::DevMode(
    _In_ DWORD       dwMode,
    _In_ POEMDMPARAM pOemDMParam
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pOemDMParam, E_POINTER)))
    {
        if ((OEMDM_SIZE != dwMode) &&
            (OEMDM_DEFAULT != dwMode) &&
            (OEMDM_CONVERT != dwMode)&&
            (OEMDM_MERGE != dwMode))
        {
            hr = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hr))
    {
        switch (dwMode)
        {
            //
            // The method should return the size of the memory allocation needed to store the UI plugin Private DEVMODE.
            //
            case OEMDM_SIZE:
            {
                pOemDMParam->cbBufSize = sizeof(OEMDEV);
            }
            break;

            //
            // Should fill the Private DEVMODE with the default values.
            //
            case OEMDM_DEFAULT:
            {
                try
                {
                    if (SUCCEEDED(hr = CHECK_POINTER(pOemDMParam->pOEMDMOut, E_POINTER)))
                    {
                        CUIProperties oemDevOut = CUIProperties(reinterpret_cast<POEMDEV>(pOemDMParam->pOEMDMOut));

                        hr = oemDevOut.SetDefaults();
                    }
                }
                catch (CXDException &e)
                {
                    hr = e;
                }
                catch (...)
                {
                    hr = E_FAIL;
                }
            }
            break;

            //
            // The method should convert private DEVMODE members to the current version, if necessary.
            //
            case OEMDM_CONVERT:
            {
                try
                {
                    if (SUCCEEDED(hr = CHECK_POINTER(pOemDMParam->pOEMDMOut, E_POINTER)) &&
                        SUCCEEDED(hr = CHECK_POINTER(pOemDMParam->pOEMDMIn, E_POINTER)))
                    {
                        CUIProperties oemDevIn  = CUIProperties(reinterpret_cast<POEMDEV>(pOemDMParam->pOEMDMIn));
                        CUIProperties oemDevOut = CUIProperties(reinterpret_cast<POEMDEV>(pOemDMParam->pOEMDMOut));

                        hr = oemDevOut.Convert(&oemDevIn);
                    }
                }
                catch (CXDException &e)
                {
                    hr = e;
                }
                catch (...)
                {
                    hr = E_FAIL;
                }
            }
            break;

            //
            //The method should validate the information contained in private DEVMODE members and merge validated values into a private DEVMODE structure containing default values
            //
            case OEMDM_MERGE:
            {
                try
                {
                    if (SUCCEEDED(hr = CHECK_POINTER(pOemDMParam->pOEMDMOut, E_POINTER)) &&
                        SUCCEEDED(hr = CHECK_POINTER(pOemDMParam->pOEMDMIn, E_POINTER)))
                    {
                        CUIProperties oemDevIn  = CUIProperties(reinterpret_cast<POEMDEV>(pOemDMParam->pOEMDMIn));
                        CUIProperties oemDevOut = CUIProperties(reinterpret_cast<POEMDEV>(pOemDMParam->pOEMDMOut));

                        if (SUCCEEDED(hr = oemDevOut.Convert(&oemDevIn)))
                        {
                            hr = oemDevOut.Validate();
                        }
                    }
                }
                catch (CXDException &e)
                {
                    hr = e;
                }
                catch (...)
                {
                    hr = E_FAIL;
                }
            }
            break;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplUI::CommonUIProp

Routine Description:

    This method allows a user interface plug-in to modify an existing printer property sheet page.

Arguments:

    dwMode - Supported modes are OEMCUIP_DOCPROP and OEMCUIP_PRNPROP.
    pOemCUIPParam - Caller-supplied pointer to an OEMCUIPPARAM structure.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplUI::CommonUIProp(
    _In_ DWORD dwMode,
    _In_ POEMCUIPPARAM pOemCUIPParam
    )
{
    HRESULT hr = S_OK;

    if (dwMode == OEMCUIP_PRNPROP)
    {
        //
        // We do not implement any printer property sheets
        //
        hr = E_NOTIMPL;
    }
    else if (dwMode != OEMCUIP_DOCPROP)
    {
        //
        // Unknown mode encountered
        //
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pOemCUIPParam, E_POINTER)))
    {
        //
        // Store the OEMCUIPPARAM pointer so we can modify data and OPTITEMS.
        //
        m_pOemCUIPParam = pOemCUIPParam;

        //
        // The pDrvOptItems member is NULL on the first call through to CommonUIProp().
        //
        if (pOemCUIPParam->pDrvOptItems != NULL)
        {
            try
            {
                CUIProperties uiProperties;
                hr = uiProperties.HideOptItems(pOemCUIPParam);
            }
            catch (CXDException& e)
            {
                hr = e;
            }
            catch (...)
            {
                hr = E_FAIL;
            }
        }
    }

    ERR_ON_HR_EXC(hr, E_NOTIMPL);
    return hr;
}

/*++

Routine Name:

    CXDSmplUI::DocumentPropertySheets

Routine Description:

    This method allows a user interface plug-in to append a new page to a printer device's
    document property sheet.

Arguments:

    pPSUIInfo - Caller-supplied pointer to a PROPSHEETUI_INFO structure.
    lParam - Caller-supplied value that depends on the reason value in pPSUIInfo->Reason.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplUI::DocumentPropertySheets(
    _In_ PPROPSHEETUI_INFO pPSUIInfo,
    _In_ LPARAM            lParam
    )
{
    HRESULT hr = S_OK;

    if (pPSUIInfo == NULL ||
        pPSUIInfo->Version != PROPSHEETUI_INFO_VERSION)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        hr = E_INVALIDARG;
    }
    else
    {
        switch (pPSUIInfo->Reason)
        {
            case PROPSHEETUI_REASON_INIT:
            {
                //
                // We need to report the OEMCUIPPARAM structure to the prop sheet
                // so ASSERT it is valid. We will let the prop sheet decide if it
                // is critical to have a valid pointer or not
                //
                ASSERTMSG(m_pOemCUIPParam != NULL, "NULL pointer to OEMCUIPPARAM structure.\n");

                try
                {
                    //
                    // Create the UI properties object
                    //
                    m_pUIProperties = new(std::nothrow) CUIProperties(reinterpret_cast<POEMDEV>(m_pOemCUIPParam->pOEMDM));

                    //
                    // Create the property pages for the document property sheet
                    //
                    if (SUCCEEDED(hr = CHECK_POINTER(m_pUIProperties, E_OUTOFMEMORY)) &&
                        SUCCEEDED(hr = CreatePropertyPages()))
                    {
                        DocPropertyPageMap::iterator iterPropSheets = m_vectPropPages.begin();

                        //
                        // Make sure the helper interfaces and the OEMCUIPPARAM structure
                        // are published to the page first. This allows the prop sheet to
                        // propogate these interfaces to the UI control objects when initializing
                        //
                        while (iterPropSheets != m_vectPropPages.end() &&
                               SUCCEEDED(hr = CHECK_POINTER(*iterPropSheets, E_FAIL)) &&
                               SUCCEEDED(hr = (*iterPropSheets)->SetPrintOemDriverUI(m_pDriverUIHelp)) &&
                               SUCCEEDED(hr = (*iterPropSheets)->SetOemCUIPParam(m_pOemCUIPParam))&&
                               SUCCEEDED(hr = (*iterPropSheets)->SetUIProperties(m_pUIProperties))&&
                               SUCCEEDED(hr = (*iterPropSheets)->PropPageInit(pPSUIInfo)))
                        {
                            iterPropSheets++;
                        }
                    }
                }
                catch (CXDException &e)
                {
                    hr = e;
                }
                catch (exception& DBG_ONLY(e))
                {
                    ERR(e.what());
                    hr = E_FAIL;
                }
            }
            break;

            case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                if (SUCCEEDED(hr = CHECK_POINTER(pPSUIInfo, E_POINTER)))
                {
                    pPSUIInfo->Result = TRUE;
                }
            }
            break;

            case PROPSHEETUI_REASON_DESTROY:
            {
                if (SUCCEEDED(hr = CHECK_POINTER(pPSUIInfo, E_POINTER)))
                {
                    pPSUIInfo->Result = TRUE;
                }
            }
            break;

            case PROPSHEETUI_REASON_SET_RESULT:
            {
                if (SUCCEEDED(hr = CHECK_POINTER(pPSUIInfo, E_POINTER)))
                {
                    pPSUIInfo->Result = reinterpret_cast<PSETRESULT_INFO>(lParam)->Result;
                }
            }
            break;

            case PROPSHEETUI_REASON_GET_ICON:
            {
                if (SUCCEEDED(hr = CHECK_POINTER(pPSUIInfo, E_POINTER)))
                {
                    //
                    // No icon
                    //
                    pPSUIInfo->Result = FALSE;
                }
            }
            break;

            default:
            {
                hr = E_FAIL;
            }
            break;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CXDSmplUI::DevicePropertySheets

Routine Description:

    This method allows a user interface plug-in to append a new page to a printer device's
    printer property sheet.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::DevicePropertySheets(
     _In_ PPROPSHEETUI_INFO,
     _In_ LPARAM
     )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::DevQueryPrintEx

Routine Description:

    This method allows a user interface plug-in to help determine if a print job is printable.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::DevQueryPrintEx(
    _In_ POEMUIOBJ,
    _In_ PDEVQUERYPRINT_INFO,
    _In_ PDEVMODE,
    _In_ PVOID
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::DeviceCapabilities

Routine Description:

    This method enables a user interface plug-in to specify customized device capabilities.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::DeviceCapabilities(
    _In_   POEMUIOBJ,
    _In_   HANDLE,
    _In_z_ PWSTR,
    _In_   WORD,
    _In_   PVOID,
    _In_   PDEVMODE,
    _In_   PVOID,
    _In_   DWORD,
    _In_   DWORD*
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::UpgradePrinter

Routine Description:

    This method allows a user interface plug-in to upgrade device option values
    that are stored in the registry.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::UpgradePrinter(
    _In_ DWORD,
    _In_ PBYTE
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::PrinterEvent

Routine Description:

    This method allows a user interface plug-in to process printer events.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::PrinterEvent(
    _In_ PWSTR ,
    _In_ INT ,
    _In_ DWORD ,
    _In_ LPARAM
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::DriverEvent

Routine Description:

    The printer driver's DrvDriverEvent function calls a user interface plug-in's
    IPrintOemUI::DriverEvent method for additional processing of printer driver events.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::DriverEvent(
    _In_ DWORD ,
    _In_ DWORD ,
    _In_reads_(_Inexpressible_("varies")) PBYTE ,
    _In_ LPARAM
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::QueryColorProfile

Routine Description:

    This method allows a user interface plug-in to specify an ICC profile to use for color management.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::QueryColorProfile(
    _In_ HANDLE ,
    _In_ POEMUIOBJ ,
    _In_ PDEVMODE ,
    _In_ PVOID ,
    _In_ ULONG ,
    _Out_writes_(*pcbProfileData) VOID* ,
    _Inout_ ULONG* pcbProfileData,
    _Out_ FLONG*
    )
{
    UNREFERENCED_PARAMETER(pcbProfileData);
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::FontInstallerDlgProc

Routine Description:

    A user interface plug-in's IPrintOemUI::FontInstallerDlgProc method replaces
    the Unidrv font installer's user interface.

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::FontInstallerDlgProc(
    _In_ HWND ,
    _In_ UINT ,
    _In_ WPARAM ,
    _In_ LPARAM
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::UpdateExternalFonts

Routine Description:

    This allows a user interface plug-in to update a printer's Unidrv Font Format Files (.uff file).

Arguments:

    None referenced.

Return Value:

    HRESULT
    E_NOTIMPL - Method not implemented

--*/
HRESULT
CXDSmplUI::UpdateExternalFonts(
    _In_   HANDLE,
    _In_   HANDLE,
    _In_z_ PWSTR
    )
{
    return E_NOTIMPL;
}

/*++

Routine Name:

    CXDSmplUI::CreatePropertyPages

Routine Description:

    Creates all property page handler objects.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CXDSmplUI::CreatePropertyPages(
    VOID
    )
{
    HRESULT hr = S_OK;
    CDocPropPage* pPropPage = NULL;

//
// This Prefast warning indicates that memory could be leaked
// in the event of an exception. We suppress this false positive because we
// know that the local try/catch block will clean up a single
// CDocPropPage, and the destructor will clean up any that are
// successfully added to the vector.
//
#pragma prefast(push)
#pragma prefast(disable:__WARNING_ALIASED_MEMORY_LEAK_EXCEPTION)

    try
    {
        //
        // Populate the property page vector
        //
        pPropPage = new(std::nothrow) CColorPropPage();

        hr = AddPropPage(pPropPage);

        if (SUCCEEDED(hr))
        {
            pPropPage = new(std::nothrow) CWatermarkPropPage();
            hr = AddPropPage(pPropPage);
        }

        if (SUCCEEDED(hr))
        {
            pPropPage = new(std::nothrow) CFeaturePropPage();
            hr = AddPropPage(pPropPage);
        }
    }
    catch (CXDException &e)
    {
        hr = e;
    }
    catch (exception& DBG_ONLY(e))
    {
        ERR(e.what());
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        //
        // If we successfully created a property page but failed to push it onto
        // the vector we need to free the allocated property page
        //
        if (pPropPage != NULL)
        {
            delete pPropPage;
            pPropPage = NULL;
        }
    }

#pragma prefast(pop)

    return hr;
}

/*++

Routine Name:

    CXDSmplUI::DestroyPropPages

Routine Description:

    Destroy all property page handler classes that have been added into the collection.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
inline VOID
CXDSmplUI::DestroyPropPages(
    VOID
    )
{
    while (!m_vectPropPages.empty())
    {
        if (m_vectPropPages.back() != NULL)
        {
            delete m_vectPropPages.back();
            m_vectPropPages.back() = NULL;
        }

        m_vectPropPages.pop_back();
    }

    if (m_pUIProperties != NULL)
    {
        delete m_pUIProperties;
        m_pUIProperties = NULL;
    }
}

//
// Use __drv_aliasesMem annotation to avoid PREfast warning 28197: Possibly leaking memory,
//
HRESULT
// Prefast warning 28194: The function was declared as aliasing the value in variable and exited without doing so.
// We suppress this false positive because STL vector does not have annotation, and we know the pointer has been saved
// to STL vector, which is released in DestroyPropPages().
#pragma warning(suppress: 28194)
CXDSmplUI::AddPropPage(
    _In_opt_ __drv_aliasesMem CDocPropPage* pPropPage
    )
{
    HRESULT hr = CHECK_POINTER(pPropPage, E_OUTOFMEMORY);

    if (SUCCEEDED(hr))
    {
        m_vectPropPages.push_back(pPropPage);
    }

    return hr;
}


//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2006  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:       Intrface.cpp
//
//  PURPOSE:    Generic abstract base class useful for creating implementation 
//              of interface for Unidrv5 UI plug-ins.   This base class 
//              implements all methods which have a standard implementation 
//              and / or are optional.  Non-optional mathods are pure virtual 
//              in this base class.
//
//--------------------------------------------------------------------------

#include "precomp.h"

#include "debug.h"
#include "devmode.h"
#include "intrface.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::QueryInterface
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::QueryInterface(
    CONST IID& iid, 
    VOID** ppv
    )
{
    if (iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (iid == IID_IPrintOemUI2)
    {
        *ppv = static_cast<IPrintOemUI2*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::AddRef
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
ULONG __stdcall 
CAbstractOemUI2::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::Release
//
//  Synopsis:
//      Standard COM IUnknown implementation.
//
//
//----------------------------------------------------------------------------
_At_(this, __drv_freesMem(object))
ULONG __stdcall 
CAbstractOemUI2::Release() 
{
   ASSERT(0 != m_cRef);
   
   ULONG cRef = InterlockedDecrement(&m_cRef);

   if (0 == cRef)
   {
      delete this;
      return 0;
        
   }

   return cRef;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::HideStandardUI
//
//  Synopsis:
//      This routine is critical to supporting full UI replacement.  This
//      routine allows the plug-in to disable Unidrv's (or PScript's) standard
//      UI panels.  If this returns E_NOTIMPL, the panel isn't hidden.  If 
//      this routine returns S_OK, Unidrv will not display any UI for the 
//      specified mode.  If this returns E_NOTIMPL, Unidrv will display it's
//      standard UI.
//
//  Returns:
//      S_OK to hide the indicated UI if desired, otherwise E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::HideStandardUI(
    DWORD       dwMode
        // document property sheets, printer property sheets, or possibly
        // something not yet defined.
    )
{
    UNREFERENCED_PARAMETER(dwMode);

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::DocumentPropertySheets
//
//  Synopsis:
//      A plug-in may optionally add property sheets.  If the plug-in has
//      chosen to hide the standard UI, then it would instead provide 
//      replacements for the standard property sheets.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    UNREFERENCED_PARAMETER(pPSUIInfo);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::DevicePropertySheets
//
//  Synopsis:
//      A plug-in may optionally add property sheets.  If the plug-in has
//      chosen to hide the standard UI, then it would instead provide 
//      replacements for the standard property sheets.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )
{
    UNREFERENCED_PARAMETER(pPSUIInfo);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::CommonUIProp
//
//  Synopsis:
//      This routine is used to interact with the Unidrv-supplied
//      property sheet pages.  Since this plug-in implements full-UI
//      replacement, those property sheet pages are disabled, and this
//      routine does not need to do anything.
//
//  Returns:
//      S_OK
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam
    )
{
    UNREFERENCED_PARAMETER(dwMode);
    UNREFERENCED_PARAMETER(pOemCUIPParam);

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::DeviceCapabilities
//
//  Synopsis:
//      This routine can be used to replace the Unidrv's device capabilities
//      handling.  In this implementation, no custom capabilities handling is
//      provided.
//
//  Returns:
//      E_FAIL
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::DeviceCapabilities(
    _Inout_ POEMUIOBJ poemuiobj,
        // OEM settings & information
    _In_  HANDLE      hPrinter,
        // Handle to the printer that the capabilities are being requested for
    _In_  PWSTR       pDeviceName,
        // Name of the device / driver
    WORD        wCapability,
        // The DC_ ID indicating which capability was requested.
    _Out_ PVOID       pOutput,
        // Buffer to write requested information to.
    _In_  PDEVMODE    pPublicDM,
        // Pointer to the public DEVMODE representing the settings to get
        // capabilities with respect to.
    _In_  PVOID       pOEMDM,
        // OEM private DEVMODE settings.
    DWORD       dwOld,
        // Result from previous plug-in call to this routine.
    _Out_ DWORD       *dwResult
        // Result of this call.  If there are multiple UI plug-ins, this 
        // result is passed to the next one as dwOld
    )
{
    UNREFERENCED_PARAMETER(poemuiobj);
    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(pDeviceName);
    UNREFERENCED_PARAMETER(wCapability);
    UNREFERENCED_PARAMETER(pOutput);
    UNREFERENCED_PARAMETER(pPublicDM);
    UNREFERENCED_PARAMETER(pOEMDM);
    UNREFERENCED_PARAMETER(dwOld);
    UNREFERENCED_PARAMETER(dwResult);

    //
    // Do nothing.  Let Unidrv handle the device capabilities processing.
    //

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::DevQueryPrintEx
//
//  Synopsis:
//      This routine is used to determine print job compatibility with the
//      current driver.  The plug-in can supplement Unidrv's handling of this
//      routine.  If Unidrv determines that a job can be printed on the current
//      printer it will call this routine to ask the plug-in whether it can
//      also handle the current job.  If Unidrv determines that the job cannot
//      be printed, it will not call the plug-in.  Additionally, XPS drivers
//      built on the Unidrv and PScript UI modules may not recieve this call-
//      back.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM
    )
{
    UNREFERENCED_PARAMETER(poemuiobj);
    UNREFERENCED_PARAMETER(pDQPInfo);
    UNREFERENCED_PARAMETER(pPublicDM);
    UNREFERENCED_PARAMETER(pOEMDM);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::UpgradePrinter
//
//  Synopsis:
//      Use this callback to upgrade any settings from previous versions 
//      that are stored in the registry by the plug-in Not applicabe to 
//      this sample.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::UpgradePrinter(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo
    )
{
    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(pDriverUpgradeInfo);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::PrinterEvent
//
//  Synopsis:
//      Perform any special processing needed when various events occur to the
//      print queue.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::PrinterEvent(
    _In_ PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam
    )
{
    UNREFERENCED_PARAMETER(pPrinterName);
    UNREFERENCED_PARAMETER(iDriverEvent);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::DriverEvent
//
//  Synopsis:
//      Notifies the plug-in of changes or events relevant to the driver,
//      such as installing and upgrading.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::DriverEvent(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam
    )
{
    UNREFERENCED_PARAMETER(dwDriverEvent);
    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(pDriverInfo);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
};


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::QueryColorProfile
//
//  Synopsis:
//      This routine can be implemented to provide a color profile from the 
//      driver.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulQueryMode,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData
            )
{
    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(poemuiobj);
    UNREFERENCED_PARAMETER(pPublicDM);
    UNREFERENCED_PARAMETER(pOEMDM);
    UNREFERENCED_PARAMETER(ulQueryMode);
    UNREFERENCED_PARAMETER(pvProfileData);
    UNREFERENCED_PARAMETER(pcbProfileData);
    UNREFERENCED_PARAMETER(pflProfileData);

    return E_NOTIMPL;
};

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::FontInstallerDlgProc
//
//  Synopsis:
//      Plug-ins can use this method to replace Unidrv's provided soft-font
//      installer dialog.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::FontInstallerDlgProc(
    HWND    hWnd,
    UINT    usMsg,
    WPARAM  wParam,
    LPARAM  lParam
    ) 
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(usMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return E_NOTIMPL;
};

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::UpdateExternalFonts
//
//  Synopsis:
//      This routine is used to notify the UI of any changes to installed
//      font cartridges, primarily for supporting soft-font UI replacement.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::UpdateExternalFonts(
        _In_ HANDLE  hPrinter,
        _In_ HANDLE  hHeap,
        _In_ PWSTR   pwstrCartridges
        )
{
    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(hHeap);
    UNREFERENCED_PARAMETER(pwstrCartridges);

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::QueryJobAttributes
//
//  Synopsis:
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::QueryJobAttributes(
    HANDLE      hPrinter,
    PDEVMODE    pDevmode,
    DWORD       dwLevel,
    LPBYTE      lpAttributeInfo
    )
{
    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(pDevmode);
    UNREFERENCED_PARAMETER(dwLevel);
    UNREFERENCED_PARAMETER(lpAttributeInfo);

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CAbstractOemUI2::DocumentEvent
//
//  Synopsis:
//      Perform any special processing needed at various points in time while
//      a job is printing.
//
//      Implementation of this routine is optional.  To indicate that the 
//      plug-in does not support this call, return E_NOTIMPL.
//
//
//----------------------------------------------------------------------------
HRESULT __stdcall 
CAbstractOemUI2::DocumentEvent(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pbIn,
    ULONG       cbOut,
    PVOID       pbOut,
    PINT        piResult
    )
{
    UNREFERENCED_PARAMETER(hPrinter);
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(iEsc);
    UNREFERENCED_PARAMETER(cbIn);
    UNREFERENCED_PARAMETER(pbIn);
    UNREFERENCED_PARAMETER(cbOut);
    UNREFERENCED_PARAMETER(pbOut);
    UNREFERENCED_PARAMETER(piResult);

    return E_NOTIMPL;
}


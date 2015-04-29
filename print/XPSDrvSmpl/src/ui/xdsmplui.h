/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdsmplui.h

Abstract:

   Definition of the UI plugin. This is responsible for initialising and maintining
   the property pages used in the XPSDrv feature sample UI.

--*/

#pragma once

#include "cunknown.h"
#include "docppg.h"

typedef std::vector<CDocPropPage*> DocPropertyPageMap;

class CXDSmplUI : public CUnknown<IPrintOemUI>
{
public:
    //
    // Construction and Destruction
    //
    CXDSmplUI();

    virtual ~CXDSmplUI();

    //
    // IPrintOemUI methods
    //
    STDMETHOD(PublishDriverInterface)(THIS_
        _In_ IUnknown* pIUnknown
        );

    STDMETHOD(GetInfo)(THIS_
        _In_  DWORD  dwMode,
        _Out_writes_bytes_(cbSize) PVOID  pBuffer,
        _In_  DWORD  cbSize,
        _Out_ PDWORD pcbNeeded
        );

    STDMETHOD(DevMode)(THIS_
        _In_ DWORD       dwMode,
        _In_ POEMDMPARAM pOemDMParam
        );

    STDMETHOD(CommonUIProp)(THIS_
        _In_ DWORD         dwMode,
        _In_ POEMCUIPPARAM pOemCUIPParam
        );

    STDMETHOD(DocumentPropertySheets)(THIS_
        _In_ PPROPSHEETUI_INFO pPSUIInfo,
        _In_ LPARAM            lParam
        );

    STDMETHOD(DevicePropertySheets)(THIS_
         _In_ PPROPSHEETUI_INFO pPSUIInfo,
         _In_ LPARAM            lParam
         );

    STDMETHOD(DevQueryPrintEx)(THIS_
        _In_ POEMUIOBJ           poemuiobj,
        _In_ PDEVQUERYPRINT_INFO pDQPInfo,
        _In_ PDEVMODE            pPublicDM,
        _In_ PVOID               pOEMDM
        );

    STDMETHOD(DeviceCapabilities)(THIS_
        _In_   POEMUIOBJ poemuiobj,
        _In_   HANDLE    hPrinter,
        _In_z_ PWSTR     pDeviceName,
        _In_   WORD      wCapability,
        _In_   PVOID     pOutput,
        _In_   PDEVMODE  pPublicDM,
        _In_   PVOID     pOEMDM,
        _In_   DWORD     dwOld,
        _In_   DWORD*    dwResult
        );

    STDMETHOD(UpgradePrinter)(THIS_
        _In_ DWORD dwLevel,
        _In_ PBYTE pDriverUpgradeInfo
        );

    STDMETHOD(PrinterEvent)(THIS_
        _In_   PWSTR  pPrinterName,
        _In_   INT    iDriverEvent,
        _In_   DWORD  dwFlags,
        _In_   LPARAM lParam
        );

    STDMETHOD(DriverEvent)(THIS_
        _In_ DWORD  dwDriverEvent,
        _In_ DWORD  dwLevel,
        _In_reads_(_Inexpressible_("varies")) PBYTE  pDriverInfo,
        _In_ LPARAM lParam
        );

    STDMETHOD(QueryColorProfile)(THIS_
        _In_    HANDLE    hPrinter,
        _In_    POEMUIOBJ poemuiobj,
        _In_    PDEVMODE  pPublicDM,
        _In_    PVOID     pOEMDM,
        _In_    ULONG     ulReserved,
        _Out_writes_(*pcbProfileData) VOID*     pvProfileData,
        _Inout_ ULONG*    pcbProfileData,
        _Out_   FLONG*    pflProfileData
        );

    STDMETHOD(FontInstallerDlgProc)(THIS_
        _In_ HWND    hWnd,
        _In_ UINT    usMsg,
        _In_ WPARAM  wParam,
        _In_ LPARAM  lParam
        );

    STDMETHOD(UpdateExternalFonts)(THIS_
        _In_   HANDLE hPrinter,
        _In_   HANDLE hHeap,
        _In_z_ PWSTR  pwstrCartridges
        );

private:
    HRESULT
    CreatePropertyPages(
        VOID
        );

    HRESULT
    AddPropPage(
        _In_opt_ __drv_aliasesMem CDocPropPage* pPropPage
        );

    inline VOID
    DestroyPropPages(
        VOID
        );

private:
    POEMCUIPPARAM                m_pOemCUIPParam;

    CUIProperties*               m_pUIProperties;

    CComPtr<IPrintOemDriverUI>   m_pDriverUIHelp;

    DocPropertyPageMap           m_vectPropPages;
};


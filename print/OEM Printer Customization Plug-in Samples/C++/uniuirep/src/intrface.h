//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Intrface.h
//
//  PURPOSE:    Define COM interface for User Mode Printer Customization DLL.
//
//--------------------------------------------------------------------------

#pragma once

///////////////////////////////////////
//          Globals
///////////////////////////////////////

extern HINSTANCE   ghInstance;
    // Module's Instance handle from DLLEntry of process.

class COemUI2: public IPrintOemUI2
{
public:
    //
    // *** IUnknown methods ***
    // 

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, _COM_Outptr_ LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG,AddRef)  (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    //
    // Method for publishing Driver interface.
    //
    STDMETHOD(PublishDriverInterface)(THIS_ IUnknown *pIUnknown);

    //
    // Get OEM dll related information
    //
    STDMETHOD(GetInfo) (THIS_ 
        _In_  DWORD  dwMode, 
        _Out_writes_bytes_to_(cbSize, *pcbNeeded) PVOID  pBuffer, 
        _In_  DWORD  cbSize,
        _Out_ PDWORD pcbNeeded);

    //
    // OEMDevMode
    //
    STDMETHOD(DevMode) (THIS_  DWORD  dwMode, POEMDMPARAM pOemDMParam);

    //
    // OEMCommonUIProp
    //
    STDMETHOD(CommonUIProp) (THIS_  
            DWORD  dwMode, 
            POEMCUIPPARAM   pOemCUIPParam
            );

    //
    // OEMDocumentPropertySheets
    //
    STDMETHOD(DocumentPropertySheets) (THIS_
            PPROPSHEETUI_INFO   pPSUIInfo,
            LPARAM              lParam
            );

    //
    // OEMDevicePropertySheets
    //
    STDMETHOD(DevicePropertySheets) (THIS_
            PPROPSHEETUI_INFO   pPSUIInfo,
            LPARAM              lParam
            );


    //
    // OEMDevQueryPrintEx
    //
    STDMETHOD(DevQueryPrintEx) (THIS_
            POEMUIOBJ               poemuiobj,
            PDEVQUERYPRINT_INFO     pDQPInfo,
            PDEVMODE                pPublicDM,
            PVOID                   pOEMDM
            );

    //
    // OEMDeviceCapabilities
    //
    STDMETHOD(DeviceCapabilities) (THIS_
            _Inout_ POEMUIOBJ   poemuiobj,
            _In_  HANDLE      hPrinter,
            _In_  PWSTR       pDeviceName,
            _In_  WORD        wCapability,
            _Out_writes_(_Inexpressible_("varies with wCapability"))  PVOID       pOutput,
            _In_  PDEVMODE    pPublicDM,
            _In_  PVOID       pOEMDM,
            _In_  DWORD       dwOld,
            _Out_ DWORD       *dwResult
            );

    //
    // OEMUpgradePrinter
    //
    STDMETHOD(UpgradePrinter) (THIS_
            _In_ DWORD   dwLevel,
            _At_((PDRIVER_UPGRADE_INFO_1)pDriverUpgradeInfo, _In_) PBYTE   pDriverUpgradeInfo
            );

    //
    // OEMPrinterEvent
    //
    STDMETHOD(PrinterEvent) (THIS_
            _In_ PWSTR   pPrinterName,
            _In_ INT     iDriverEvent,
            _In_ DWORD   dwFlags,
            _In_ LPARAM  lParam
            );

    //
    // OEMDriverEvent
    //
    STDMETHOD(DriverEvent)(THIS_
            _In_ DWORD   dwDriverEvent,
            _In_ DWORD   dwLevel,
            _In_reads_(_Inexpressible_("varies")) LPBYTE  pDriverInfo,
            _In_ LPARAM  lParam
            );
 
    //
    // OEMQueryColorProfile
    //
    STDMETHOD( QueryColorProfile) (THIS_
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulReserved,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData);

    //
    // OEMFontInstallerDlgProc
    //
    STDMETHOD(FontInstallerDlgProc) (THIS_ 
            HWND    hWnd,
            UINT    usMsg,
            WPARAM  wParam,
            LPARAM  lParam
            );
    //
    // UpdateExternalFonts
    //
    STDMETHOD(UpdateExternalFonts) (THIS_
            _In_ HANDLE  hPrinter,
            _In_ HANDLE  hHeap,
            _In_ PWSTR   pwstrCartridges
            );

    //
    // IPrintOemUI2 methods
    //

    //
    // QueryJobAttributes
    //
    STDMETHOD(QueryJobAttributes)  (THIS_
            _In_             HANDLE      hPrinter,
            _In_             PDEVMODE    pDevmode,
            _In_range_(1, 4) DWORD       dwLevel,
            _In_reads_(_Inexpressible_("varies")) LPBYTE      lpAttributeInfo
           );

    //
    // Hide Standard UI
    //
    STDMETHOD(HideStandardUI)  (THIS_
            DWORD       dwMode
           );

    //
    // DocumentEvent
    //
    STDMETHOD(DocumentEvent) (THIS_
            HANDLE      hPrinter,
            HDC         hdc,
            INT         iEsc,
            ULONG       cbIn,
            PVOID       pbIn,
            ULONG       cbOut,
            PVOID       pbOut,
            PINT        piResult
           );

    COemUI2()
    {
        m_cRef = 1;
        m_pCoreHelper = NULL;
    };
    
    virtual ~COemUI2();

protected:

    LONG m_cRef;
        // reference count of this object

    IPrintCoreHelper* m_pCoreHelper;
        // Pointer to the helper interface introduced in Windows Vista.

    CFeatureCollection m_Features;
        // Collection of features advertised as supported by the helper 
        // interface.
};




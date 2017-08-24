//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2006  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Intrface.h
//
//  PURPOSE:    Defines a class with default implementations of the UI 
//              interface plug=-in methods that are optional.
//
//--------------------------------------------------------------------------

#pragma once

class CAbstractOemUI2: public IPrintOemUI2
{
public:
    //
    // *** IUnknown methods ***
    // 
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    
    STDMETHOD_(ULONG,AddRef)  (THIS);

    // the _At_ tag here tells prefast that once release 
    // is called, the memory should not be considered leaked
    _At_(this, __drv_freesMem(object))
    STDMETHOD_(ULONG,Release) (THIS);

    //
    // Method for publishing Driver interface.
    //
    STDMETHOD(PublishDriverInterface)(THIS_ IUnknown *pIUnknown) = 0;

    //
    // Get OEM dll related information
    //
    STDMETHOD(GetInfo) (THIS_ 
        DWORD  dwMode, 
        _Out_writes_bytes_(cbSize) PVOID  pBuffer, 
        DWORD  cbSize,
        PDWORD pcbNeeded) = 0;

    //
    // OEMDevMode
    //
    STDMETHOD(DevMode) (THIS_  DWORD  dwMode, POEMDMPARAM pOemDMParam) = 0;

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
                  WORD        wCapability,
            _Out_ PVOID       pOutput,
            _In_  PDEVMODE    pPublicDM,
            _In_  PVOID       pOEMDM,
                  DWORD       dwOld,
            _Out_ DWORD       *dwResult
            );

    //
    // OEMUpgradePrinter
    //
    STDMETHOD(UpgradePrinter) (THIS_
            DWORD   dwLevel,
            PBYTE   pDriverUpgradeInfo
            );

    //
    // OEMPrinterEvent
    //
    STDMETHOD(PrinterEvent) (THIS_
            _In_ PWSTR   pPrinterName,
            INT     iDriverEvent,
            DWORD   dwFlags,
            LPARAM  lParam
            );

    //
    // OEMDriverEvent
    //
    STDMETHOD(DriverEvent)(THIS_
            DWORD   dwDriverEvent,
            DWORD   dwLevel,
            LPBYTE  pDriverInfo,
            LPARAM  lParam
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
            HANDLE      hPrinter,
            PDEVMODE    pDevmode,
            DWORD       dwLevel,
            LPBYTE      lpAttributeInfo
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

    CAbstractOemUI2()
    {
        m_cRef = 1;
    };
    
    virtual ~CAbstractOemUI2() {};

protected:

    LONG m_cRef;
        // reference count of this object

};




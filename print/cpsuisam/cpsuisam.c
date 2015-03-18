/*++

Copyright (c) 1990-2003 Microsoft Corporation
All Rights Reserved


Module Name:

    cpsuisam.c


Abstract:

    This module contrains sample prototype for the 
    Windows 2000/Windows XP/Windows Server 2003 common Property
    Sheet user interface

--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_CPSUIFILENAME   DbgCPSUISam



#define DBG_WINMAINPROC     0x00000001

DEFINE_DBGVAR(0);



#define HAS_TVTEST_PAGES    0x01
#define HAS_DOCPROP_PAGES   0x02



extern  HINSTANCE   hInstApp;
extern  TCHAR       TitleName[];
extern  TCHAR       ClassName[];
extern  TCHAR       MenuName[];
extern  TCHAR       szWinSpool[];
extern  CHAR        szDocPropSheets[];
extern  BOOL        UpdatePermission;
extern  BOOL        UseStdAbout;


HWND    hWndApp = NULL;


typedef struct _CPSUISAMPLE
{
    HANDLE                  hParent;
    HANDLE                  hCPSUI;
    HANDLE                  hDocProp;
    PFNCOMPROPSHEET         pfnCPS;
    COMPROPSHEETUI          CPSUI;
    DOCUMENTPROPERTYHEADER  DPHdr;
    HGLOBAL                 hDevMode;
    HGLOBAL                 hDevNames;
} CPSUISAMPLE, *PCPSUISAMPLE;

BOOL
GetDefPrinter
(
    PCPSUISAMPLE    pCPSUISample
)
{
    HGLOBAL         hDevMode = NULL;
    HGLOBAL         hDevNames = NULL;
    LPTSTR          pszPrinterName = NULL;
    DEVMODE         *pDM = NULL;
    DEVNAMES        *pDN = NULL;
    HANDLE          hPrinter = NULL;
    PAGESETUPDLG    PSD = {0};
    BOOL            Ok = FALSE;

    PSD.lStructSize = sizeof(PSD);
    PSD.Flags       = PSD_RETURNDEFAULT;

    if (PageSetupDlg(&PSD))
    {
        hDevMode = PSD.hDevMode;

        if (NULL != hDevMode)
        {
            pDM = (DEVMODE *)GlobalLock(hDevMode);
            if (NULL != pDM)
            {
                hDevNames = PSD.hDevNames;                
                if (hDevNames)
                {
                    pDN = (DEVNAMES *)GlobalLock(hDevNames);
                    if (NULL != pDN)
                    {
                        pszPrinterName = (LPTSTR)pDN + pDN->wDeviceOffset;
                        if (pszPrinterName != NULL)
                        {
                            if (OpenPrinter(pszPrinterName, &hPrinter, NULL))
                            {
                                Ok = TRUE;
                            }
                        }
                    }
                }
            }
        }

        if (Ok)
        {
            HGLOBAL h = pCPSUISample->hDevMode;

            if (NULL != h)
            {
                GlobalUnlock(h);
                GlobalFree(h);
            }

            h = pCPSUISample->hDevNames;

            if (NULL != h)
            {
                GlobalUnlock(h);
                GlobalFree(h);
            }

            if (pCPSUISample->DPHdr.hPrinter)
            {
                ClosePrinter(pCPSUISample->DPHdr.hPrinter);
            }

            pCPSUISample->hDevMode             = PSD.hDevMode;
            pCPSUISample->hDevNames            = PSD.hDevNames;

            pCPSUISample->DPHdr.cbSize         = sizeof(DOCUMENTPROPERTYHEADER);
            pCPSUISample->DPHdr.hPrinter       = hPrinter;
            pCPSUISample->DPHdr.pszPrinterName = pszPrinterName;
            pCPSUISample->DPHdr.pdmIn          =
            pCPSUISample->DPHdr.pdmOut         = pDM;
            pCPSUISample->DPHdr.fMode          = (DM_IN_BUFFER | DM_IN_PROMPT | DM_OUT_BUFFER);

            if (!UpdatePermission)
            {
                pCPSUISample->DPHdr.fMode |= DM_NOPERMISSION;
            }
        }
        else
        {
            hDevMode = PSD.hDevMode;
            hDevNames = PSD.hDevNames;

            if (NULL != hDevMode)
            {
                GlobalUnlock(hDevMode);
                GlobalFree(hDevMode);
            }

            if (NULL != hDevNames)
            {
                GlobalUnlock(hDevNames);
                GlobalFree(hDevNames);
            }

            if (hPrinter)
            {
                ClosePrinter(hPrinter);
            }
        }
    }

    return Ok;
}


LONG
CALLBACK
CPSUIFunc
(
    _In_ PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
)
{
    PPROPSHEETUI_INFO_HEADER    pPSUIInfoHdr;
    PCPSUISAMPLE                pCPSUISample;
    HANDLE                      h;
    INSERTPSUIPAGE_INFO         InsPI;

    if (!pPSUIInfo)
    {
        return FALSE;
    }

    switch (pPSUIInfo->Reason)
    {
    case PROPSHEETUI_REASON_INIT:

        pCPSUISample = (PCPSUISAMPLE)LocalAlloc(LPTR, sizeof(CPSUISAMPLE));
        if (NULL == pCPSUISample)
        {
            return(-1);
        }

        pPSUIInfo->UserData   = (ULONG_PTR)pCPSUISample;
        pCPSUISample->hParent = pPSUIInfo->hComPropSheet;
        pCPSUISample->pfnCPS  = pPSUIInfo->pfnComPropSheet;

        //
        // Add Document Property Sheet for current default Printer
        //

        switch (pPSUIInfo->lParamInit)
        {
        case IDM_DOCPROP:
        case IDM_DOCPROP_TVTEST:

            if (GetDefPrinter(pCPSUISample))
            {
                InsPI.cbSize  = sizeof(INSERTPSUIPAGE_INFO);
                InsPI.Type    = PSUIPAGEINSERT_DLL;
                InsPI.Mode    = INSPSUIPAGE_MODE_FIRST_CHILD;
                InsPI.dwData1 = (ULONG_PTR)szWinSpool;
                InsPI.dwData2 = (ULONG_PTR)szDocPropSheets;
                InsPI.dwData3 = (ULONG_PTR)&(pCPSUISample->DPHdr);

                pCPSUISample->hDocProp =
                        (HANDLE)pCPSUISample->pfnCPS( pCPSUISample->hParent,
                                                      CPSFUNC_INSERT_PSUIPAGE,
                                                      (LPARAM)0,
                                                      (LPARAM)&InsPI
                                                    );
            }
            break;
        }

        //
        // Add TreeView Page
        //

        switch (pPSUIInfo->lParamInit)
        {
        case IDM_TVTEST:
        case IDM_DOCPROP_TVTEST:

            if (SetupComPropSheetUI(&(pCPSUISample->CPSUI)))
            {
                InsPI.cbSize  = sizeof(INSERTPSUIPAGE_INFO);
                InsPI.Type    = PSUIPAGEINSERT_PCOMPROPSHEETUI;
                InsPI.Mode    = INSPSUIPAGE_MODE_FIRST_CHILD;
                InsPI.dwData1 = (ULONG_PTR)&(pCPSUISample->CPSUI);
                InsPI.dwData2 =
                InsPI.dwData3 = 0;

                pCPSUISample->hCPSUI =
                        (HANDLE)pCPSUISample->pfnCPS( pCPSUISample->hParent,
                                                      CPSFUNC_INSERT_PSUIPAGE,
                                                      (LPARAM)0,
                                                      (LPARAM)&InsPI
                                                    );
            }
            break;
        }

        if (  (pCPSUISample->hCPSUI)
           || (pCPSUISample->hDocProp)
           )
        {
            return 1;
        }

        break;

    case PROPSHEETUI_REASON_GET_INFO_HEADER:

        pPSUIInfoHdr = (PPROPSHEETUI_INFO_HEADER)lParam;
        if (NULL != pPSUIInfoHdr)
        {
            pPSUIInfoHdr->pTitle = (LPTSTR)TitleName;

            switch (pPSUIInfo->lParamInit)
            {
            case IDM_DOCPROP:

                pPSUIInfoHdr->IconID = IDI_CPSUI_PRINTER2;
                break;

            case IDM_TVTEST:

                pPSUIInfoHdr->IconID = IDI_CPSUI_OPTION2;
                break;

            case IDM_DOCPROP_TVTEST:

                pPSUIInfoHdr->IconID = IDI_CPSUI_RUN_DIALOG;
                break;
            }

            pPSUIInfoHdr->Flags      = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pPSUIInfoHdr->hWndParent = hWndApp;
            pPSUIInfoHdr->hInst      = hInstApp;

            return 1;
        }

        break;

    case PROPSHEETUI_REASON_SET_RESULT:

        pCPSUISample = (PCPSUISAMPLE)pPSUIInfo->UserData;
        if ( (NULL != pCPSUISample) 
             && (pCPSUISample->hCPSUI == ((PSETRESULT_INFO)lParam)->hSetResult)
             )
        {
            //
            // Save the result and propagate to its owner
            //
            pPSUIInfo->Result = ((PSETRESULT_INFO)lParam)->Result;
            return 1;
        }

        break;

    case PROPSHEETUI_REASON_DESTROY:

        pCPSUISample = (PCPSUISAMPLE)pPSUIInfo->UserData;
        if (NULL != pCPSUISample)
        {
            h = pCPSUISample->hDevMode;
            if (NULL != h)
            {
                GlobalUnlock(h);
                GlobalFree(h);
            }

            h = pCPSUISample->hDevNames;
            if (NULL != h)
            {
                GlobalUnlock(h);
                GlobalFree(h);
            }

            if (pCPSUISample->DPHdr.hPrinter)
            {
                ClosePrinter(pCPSUISample->DPHdr.hPrinter);
            }

            LocalFree((HLOCAL)pCPSUISample);
            pPSUIInfo->UserData = 0;
        }
        return 1;
    }
    return -1;
}

LRESULT
CALLBACK 
MainWndProc
(
    HWND      hWnd,
    UINT      Msg,
    WPARAM    wParam,
    LPARAM    lParam
)
/*++

Routine Description:

    This is the main window procedure to the testing program


Arguments:

    See SDK


Return Value:

    See SDK

--*/
{
    DWORD   Result;
    LONG    Ret;

    switch (Msg)
    {
    case WM_INITMENUPOPUP:

        if (!HIWORD(lParam))
        {
            CheckMenuItem( (HMENU)UIntToPtr((UINT)wParam),
                           IDM_PERMISSION,
                           MF_BYCOMMAND | ((UpdatePermission) ? MF_CHECKED : MF_UNCHECKED)
                         );

            CheckMenuItem( (HMENU)UIntToPtr((UINT)wParam),
                           IDM_USESTDABOUT,
                           MF_BYCOMMAND | ((UseStdAbout) ? MF_CHECKED : MF_UNCHECKED)
                         );
        }
        break;

    case WM_COMMAND:

        switch (wParam)
        {
        case IDM_USESTDABOUT:

            UseStdAbout = !UseStdAbout;
            break;

        case IDM_PERMISSION:

            UpdatePermission = !UpdatePermission;
            break;

        case IDM_DOCPROP:
        case IDM_TVTEST:
        case IDM_DOCPROP_TVTEST:

            Ret = CommonPropertySheetUI( hWnd,
                                         (PFNPROPSHEETUI)CPSUIFunc,
                                         (LPARAM)LOWORD(wParam),
                                         &Result
                                       );

            CPSUIDBG( DBG_WINMAINPROC,
                      ("CommonPropertySheetUI()=%ld, Result=%ld", Ret, Result)
                    );

            break;

        default:

            break;
        }
        break;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;

    default:

        return (DefWindowProc(hWnd, Msg, wParam, lParam));
    }

    return 0L;
}

BOOL
InitInstance
(
    HANDLE  hInstance,
    INT     nCmdShow
)
/*++

Routine Description:

    Saves instance handle and creates main window

    This function is called at initialization time for every instance of
    this application.  This function performs initialization tasks that
    cannot be shared by multiple instances.

    In this case, we save the instance handle in a static variable and
    create and display the main program window.


Arguments:

    hInstance   - Current instance identifier

    nComShow    - Param for first ShowWindow() call.



Return Value:

    TRUE/FALSE


--*/
{
    //
    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.
    //

    hInstApp = hInstance;

    //
    // Create a main window for this application instance.
    //

    hWndApp = CreateWindow( ClassName,
                            TitleName,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            NULL,
                            NULL,
                            hInstance,
                            NULL
                          );
    if (NULL != hWndApp)
    {
        //
        // Make the window visible; update its client area;
        // and send WM_PAINT message
        //

        ShowWindow(hWndApp, nCmdShow);
        UpdateWindow(hWndApp);
    }

    return ((hWndApp) ? TRUE : FALSE);
}


BOOL
InitApplication
(
    HANDLE  hInstance
)
/*++

Routine Description:

    Initializes window data and registers window class

    This function is called at initialization time only if no other
    instances of the application are running.  This function performs
    initialization tasks that can be done once for any number of running
    instances.

    In this case, we initialize a window class by filling out a data
    structure of type WNDCLASS and calling the Windows RegisterClass()
    function.  Since all instances of this application use the same window
    class, we only need to do this when the first instance is initialized.


Arguments:

    hInstance   - current instance


Return Value:

    BOOLEAN

--*/
{
    WNDCLASS  wc;

    //
    // Fill in window class structure with parameters that describe the
    // main window.
    //

    wc.style         = 0L;
    wc.lpfnWndProc   = MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_CPSUISAMPLE);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName  = MenuName;
    wc.lpszClassName = ClassName;

    //
    // Register the window class and return success/failure code.
    //
    return RegisterClass(&wc);
}


INT
APIENTRY
WinMain
(
    _In_ HINSTANCE      hInstance,
    _In_opt_ HINSTANCE  hPrevInstance,
    _In_ LPSTR          lpCmdLine,
    _In_ INT            nCmdShow
)
/*++

Routine Description:

    calls initialization function, processes message loop

    Windows recognizes this function by name as the initial entry point
    for the program.  This function calls the application initialization
    routine, if no other instance of the program is running, and always
    calls the instance initialization routine.  It then executes a message
    retrieval and dispatch loop that is the top-level control structure
    for the remainder of execution.  The loop is terminated when a WM_QUIT
    message is received, at which time this function exits the application
    instance by returning the value passed by PostQuitMessage().

    If this function must abort before entering the message loop, it
    returns the conventional value NULL.


Arguments:



Return Value:

    Integer


--*/
{
    MSG Msg;

    UNREFERENCED_PARAMETER(lpCmdLine);

    //
    // Other instances of app running?
    //

    if (!hPrevInstance)
    {
        if (!InitApplication(hInstance))
        {
            //
            // Initialize shared things, Exits if unable to initialize
            //
            return FALSE;
        }
    }

    //
    // Perform initializations that apply to a specific instance
    //
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    //
    // Acquire and dispatch messages until a WM_QUIT message is received.
    //
    while (GetMessage(&Msg, NULL, 0L, 0L))
    {
        //
        // Translates virtual key codes and Dispatches message to window
        //
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    //
    // Returns the value from PostQuitMessage
    //
    return((INT)Msg.wParam);
}


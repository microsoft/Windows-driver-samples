#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_driver_)

#include "testmcro.h"
#include "wiamicro.h"
#include "resource.h"

#include <STI.H>
#include <math.h>
#include <winioctl.h>
#include <usbscan.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include <strsafe.h>

// #define BUTTON_SUPPORT // (uncomment this to allow BUTTON SUPPORT)
                          // button support is not functional in the test device

#define MAX_BUTTONS 1
#define MAX_BUTTON_NAME 255

HINSTANCE g_hInst; // instance of this MicroDriver (used for loading from a resource)


// note: MEMORYBMP, and BMP file will be added by wiafbdrv host driver.
//       do not include them in your extended list.
//

// #define _USE_EXTENDED_FORMAT_LIST (uncomment this to allow Extented file and memory formats)

#define NUM_SUPPORTED_FILEFORMATS 1
GUID g_SupportedFileFormats[NUM_SUPPORTED_FILEFORMATS];

#define NUM_SUPPORTED_MEMORYFORMATS 2
GUID g_SupportedMemoryFormats[NUM_SUPPORTED_MEMORYFORMATS];

//
// Button GUID array used in Capability negotiation.
// Set your BUTTON guids here.  These must match the GUIDS specified in
// your INF.  The Scan Button GUID is public to all scanners with a
// scan button.
//

GUID g_Buttons[MAX_BUTTONS] ={{0xa6c5a715, 0x8c6e, 0x11d2,{ 0x97, 0x7a,  0x0,  0x0, 0xf8, 0x7a, 0x92, 0x6f}}};
BOOL g_bButtonNamesCreated = FALSE;
WCHAR* g_ButtonNames[MAX_BUTTONS] = {0};

INT g_PalIndex = 0;     // simple palette index counter (test driver specific)
BOOL g_bDown = FALSE;   // simple band direction bool   (test drvier specific)

BOOL    InitializeScanner(PSCANINFO pScanInfo);
VOID    InitScannerDefaults(PSCANINFO pScanInfo);
BOOL    SetScannerSettings(PSCANINFO pScanInfo);
VOID    CheckButtonStatus(PVAL pValue);
VOID    GetButtonPress(LONG *pButtonValue);
HRESULT GetInterruptEvent(PVAL pValue);
LONG    GetButtonCount();
HRESULT GetOLESTRResourceString(LONG lResourceID,_Outptr_ LPOLESTR *ppsz,BOOL bLocal);
VOID    ReadRegistryInformation(PVAL pValue);

BOOL APIENTRY DllMain( HANDLE hModule,DWORD  dwreason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    g_hInst = (HINSTANCE)hModule;
    switch(dwreason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

/**************************************************************************\
* MicroEntry (MicroDriver Entry point)
*
*   Called by the WIA driver to communicate with the MicroDriver.
*
* Arguments:
*
*   lCommand     - MicroDriver Command, sent from the WIA driver
*   pValue       - VAL structure used for settings
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

WIAMICRO_API HRESULT MicroEntry(LONG lCommand, _Inout_ PVAL pValue)
{
    HRESULT hr = E_NOTIMPL;
    INT index = 0;

//#define _DEBUG_COMMANDS

#ifdef _DEBUG_COMMANDS
    if(lCommand != CMD_STI_GETSTATUS)
        Trace(TEXT("Command Value (%d)"),lCommand);
#endif

    if( !pValue || !(pValue->pScanInfo))
    {
        return E_INVALIDARG;
    }

    switch(lCommand)
    {
    case CMD_INITIALIZE:
        hr = S_OK;

        //
        // create any DeviceIO handles needed, use index (1 - MAX_IO_HANDLES) to store these handles.
        // Index '0' is reserved by the WIA flatbed driver. The CreateFile Name is stored in the szVal
        // member of the VAL structure.
        //

        // pValue->pScanInfo->DeviceIOHandles[1] = CreateFileA( pValue->szVal,
        //                                   GENERIC_READ | GENERIC_WRITE, // Access mask
        //                                   0,                            // Share mode
        //                                   NULL,                         // SA
        //                                   OPEN_EXISTING,                // Create disposition
        //                                   FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,        // Attributes
        //                                   NULL );

        //
        // if your device supports buttons, create the BUTTON name information here..
        //

        if (!g_bButtonNamesCreated)
        {
            for(index = 0; index < MAX_BUTTONS; index++)
            {
                g_ButtonNames[index] = (WCHAR*)CoTaskMemAlloc(MAX_BUTTON_NAME);
                if (!g_ButtonNames[index])
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }

            if(SUCCEEDED(hr))
            {
                hr = GetOLESTRResourceString(IDS_SCAN_BUTTON_NAME,&g_ButtonNames[0],TRUE);
            }

            if(SUCCEEDED(hr))
            {
                g_bButtonNamesCreated = TRUE;
            }
            else
            {
                for(index = 0; index < MAX_BUTTONS; index++)
                {
                    if (g_ButtonNames[index])
                    {
                        CoTaskMemFree(g_ButtonNames[index]);
                        g_ButtonNames[index] = NULL;
                    }
                }
            }
        }

        //
        // Initialize the scanner's default settings
        //

        InitScannerDefaults(pValue->pScanInfo);

        break;
    case CMD_UNINITIALIZE:

        //
        // close any open handles created by the Micro driver
        //

        if(pValue->pScanInfo->DeviceIOHandles[1] != NULL)
        {
            CloseHandle(pValue->pScanInfo->DeviceIOHandles[1]);
        }


        //
        // if your device supports buttons, free/destroy the BUTTON name information here..
        //

        if(g_bButtonNamesCreated)
        {
            g_bButtonNamesCreated = FALSE;

            for(index = 0; index < MAX_BUTTONS; index++)
            {
                if (g_ButtonNames[index])
                {
                    CoTaskMemFree(g_ButtonNames[index]);
                    g_ButtonNames[index] = NULL;
                }
            }
        }

        //
        // close/unload libraries
        //

        hr = S_OK;
        break;
    case CMD_RESETSCANNER:

        //
        // reset scanner
        //

        hr = S_OK;
        break;
    case CMD_STI_DIAGNOSTIC:
    case CMD_STI_DEVICERESET:

        //
        // reset device
        //

        hr = S_OK;
        break;
    case CMD_STI_GETSTATUS:

        //
        // set status flag to ON-LINE
        //

        pValue->lVal = MCRO_STATUS_OK;
        pValue->pGuid = (GUID*) &GUID_NULL;

        //
        // button polling support
        //

#ifdef BUTTON_SUPPORT
        CheckButtonStatus(pValue);
#endif

        hr = S_OK;
        break;
    case CMD_SETXRESOLUTION:
        pValue->pScanInfo->Xresolution = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETYRESOLUTION:
        pValue->pScanInfo->Yresolution = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETCONTRAST:
        pValue->pScanInfo->Contrast    = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETINTENSITY:
        pValue->pScanInfo->Intensity   = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETDATATYPE:
        pValue->pScanInfo->DataType    = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_SETNEGATIVE:
        pValue->pScanInfo->Negative    = pValue->lVal;
        hr = S_OK;
        break;
    case CMD_GETADFSTATUS:
    case CMD_GETADFHASPAPER:
        // pValue->lVal = MCRO_ERROR_PAPER_EMPTY;
        // hr = S_OK;
        break;
    case CMD_GET_INTERRUPT_EVENT:
        hr = GetInterruptEvent(pValue);
        break;
    case CMD_GETCAPABILITIES:
        pValue->lVal = 0;
        pValue->pGuid = NULL;
        pValue->ppButtonNames = NULL;
        hr = S_OK;
        break;

    case CMD_SETSCANMODE:
        hr = S_OK;
        switch(pValue->lVal)
        {
        case SCANMODE_FINALSCAN:
            Trace(TEXT("Final Scan"));
            break;
        case SCANMODE_PREVIEWSCAN:
            Trace(TEXT("Preview Scan"));
            break;
        default:
            Trace(TEXT("Unknown Scan Mode (%d)"),pValue->lVal);
            hr = E_FAIL;
            break;
        }
        break;
    case CMD_SETSTIDEVICEHKEY:
        ReadRegistryInformation(pValue);
        break;

#ifdef _USE_EXTENDED_FORMAT_LIST

    // note: MEMORYBMP, and BMP file will be added by wiafbdrv host driver.
    //       do not include them in your extended list.
    //

    case CMD_GETSUPPORTEDFILEFORMATS:
        g_SupportedFileFormats[0] = WiaImgFmt_JPEG;
        pValue->lVal = NUM_SUPPORTED_FILEFORMATS;
        pValue->pGuid = g_SupportedFileFormats;
        hr = S_OK;
        break;

    case CMD_GETSUPPORTEDMEMORYFORMATS:
        g_SupportedMemoryFormats[0] = WiaImgFmt_TIFF;
        g_SupportedMemoryFormats[1] = WiaImgFmt_MYNEWFORMAT;
        pValue->lVal = NUM_SUPPORTED_MEMORYFORMATS;
        pValue->pGuid = g_SupportedMemoryFormats;
        hr = S_OK;
        break;
#endif

    default:
        Trace(TEXT("Unknown Command (%d)"),lCommand);
        break;
    }

    return hr;
}

/**************************************************************************\
* Scan (MicroDriver Entry point)
*
*   Called by the WIA driver to acquire data from the MicroDriver.
*
* Arguments:
*
*   pScanInfo    - SCANINFO structure used for settings
*   lPhase       - Current Scan phase, SCAN_FIRST, SCAN_NEXT, SCAN_FINISH...
*   pBuffer      - data buffer to be filled with scanned data
*   lLength      - Maximum length of pBuffer
*   plReceived   - Number of actual bytes written to pBuffer.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

WIAMICRO_API HRESULT Scan(_Inout_ PSCANINFO pScanInfo, LONG lPhase, _Out_writes_bytes_(lLength) PBYTE pBuffer, LONG lLength, _Out_ LONG *plReceived)
{
    if(pScanInfo == NULL) {
        return E_INVALIDARG;
    }

    INT i = 0;
    *plReceived = 0; // Initialize *plReceived as 0. It has a return value in case of SCAN_FIRST and SCAN_NEXT.

    Trace(TEXT("------ Scan Requesting %d ------"),lLength);
    switch (lPhase) {
    case SCAN_FIRST:
        if (!SetScannerSettings(pScanInfo)) {
            return E_FAIL;
        }

        Trace(TEXT("SCAN_FIRST"));

        g_PalIndex = 0;
        g_bDown = FALSE;

        //
        // first phase
        //

        Trace(TEXT("Start Scan.."));

    case SCAN_NEXT: // SCAN_FIRST will fall through to SCAN_NEXT (because it is expecting data)

        //
        // next phase
        //

        if(lPhase == SCAN_NEXT)
            Trace(TEXT("SCAN_NEXT"));

        //
        // get data from the scanner and set plReceived value
        //

        //
        // read data
        //

        switch(pScanInfo->DataType) {
        case WIA_DATA_THRESHOLD:

            //
            // make buffer alternate black/White, for sample 1-bit data
            //

            memset(pBuffer,0,lLength);
            memset(pBuffer,255,lLength/2);
            break;
        case WIA_DATA_GRAYSCALE:

            //
            // make buffer grayscale data, for sample 8-bit data
            //

            if(!g_bDown){
                g_PalIndex+=10;
                if(g_PalIndex > 255){
                    g_PalIndex = 255;
                    g_bDown = TRUE;
                }
            }
            else {
                g_PalIndex-=10;
                if(g_PalIndex < 0){
                    g_PalIndex = 0;
                    g_bDown = FALSE;
                }
            }
            memset(pBuffer,g_PalIndex,lLength);
            break;
        case WIA_DATA_COLOR:

            //
            // make buffer red, for sample color data
            //

            for (i = 0;i+2<lLength;i+=3) {
                memset(pBuffer+i,255,1);
                memset(pBuffer+(i+1),0,1);
                memset(pBuffer+(i+2),0,1);
            }
            break;
        default:
            break;
        }

        //
        // test device always returns the exact amount of scanned data
        //

        *plReceived = lLength;
        break;
    case SCAN_FINISHED:
    default:
        Trace(TEXT("SCAN_FINISHED"));

        //
        // stop scanner, do not set lRecieved, or write any data to pBuffer.  Those values
        // will be NULL.  This lPhase is only to allow you to stop scanning, and return the
        // scan head to the HOME position. SCAN_FINISHED will be called always for regular scans, and
        // for cancelled scans.
        //

        break;
    }

    return S_OK;
}

/**************************************************************************\
* SetPixelWindow (MicroDriver Entry point)
*
*   Called by the WIA driver to set the scan selection area to the MicroDriver.
*
* Arguments:
*
*   pScanInfo    - SCANINFO structure used for settings
*   pValue       - VAL structure used for settings
*   x            - X Position of scan rect (upper left x coordinate)
*   y            - Y Position of scan rect (upper left y coordinate)
*   xExtent      - Width of scan rect  (in pixels)
*   yExtent      - Height of scan rect (in pixels)
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

WIAMICRO_API HRESULT SetPixelWindow(_Inout_ PSCANINFO pScanInfo, LONG x, LONG y, LONG xExtent, LONG yExtent)
{
    if(pScanInfo == NULL) {
        return E_INVALIDARG;
    }

    pScanInfo->Window.xPos = x;
    pScanInfo->Window.yPos = y;
    pScanInfo->Window.xExtent = xExtent;
    pScanInfo->Window.yExtent = yExtent;
    return S_OK;
}


/**************************************************************************\
* ReadRegistryInformation (helper)
*
*   Called by the MicroDriver to Read registry information from the device's
*   installed device section. The HKEY passed in will be closed by the host
*   driver after CMD_INITIALIZE is completed.
*
* Arguments:
*
*    none
*
* Return Value:
*
*    void
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/
VOID ReadRegistryInformation(PVAL pValue)
{
    HKEY hKey = NULL;
    if(NULL != pValue->pHandle){
        hKey = (HKEY)*pValue->pHandle;

        //
        // Open DeviceData section to read driver specific information
        //

        HKEY hOpenKey = NULL;
        if (RegOpenKeyEx(hKey,                     // handle to open key
                         TEXT("DeviceData"),       // address of name of subkey to open
                         0,                        // options (must be NULL)
                         KEY_QUERY_VALUE|KEY_READ, // just want to QUERY a value
                         &hOpenKey                 // address of handle to open key
                        ) == ERROR_SUCCESS) {

            DWORD dwWritten = sizeof(DWORD);
            DWORD dwType = REG_DWORD;

            LONG lSampleEntry = 0;
            RegQueryValueEx(hOpenKey,
                            TEXT("Sample Entry"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lSampleEntry,
                            &dwWritten);
            Trace(TEXT("lSampleEntry Value = %d"),lSampleEntry);
        } else {
            Trace(TEXT("Could not open DeviceData section"));
        }
    }
}

/**************************************************************************\
* InitScannerDefaults (helper)
*
*   Called by the MicroDriver to Initialize the SCANINFO structure
*
* Arguments:
*
*    none
*
* Return Value:
*
*    void
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

VOID InitScannerDefaults(PSCANINFO pScanInfo)
{

    pScanInfo->ADF                    = 0; // set to no ADF in Test device
    pScanInfo->RawDataFormat          = WIA_PACKED_PIXEL;
    pScanInfo->RawPixelOrder          = WIA_ORDER_BGR;
    pScanInfo->bNeedDataAlignment     = TRUE;

    pScanInfo->SupportedCompressionType = 0;
    pScanInfo->SupportedDataTypes     = SUPPORT_BW|SUPPORT_GRAYSCALE|SUPPORT_COLOR;

    pScanInfo->BedWidth               = 8500;  // 1000's of an inch (WIA compatible unit)
    pScanInfo->BedHeight              = 11000; // 1000's of an inch (WIA compatible unit)

    pScanInfo->OpticalXResolution     = 300;
    pScanInfo->OpticalYResolution     = 300;

    pScanInfo->IntensityRange.lMin    = -127;
    pScanInfo->IntensityRange.lMax    =  127;
    pScanInfo->IntensityRange.lStep   = 1;

    pScanInfo->ContrastRange.lMin     = -127;
    pScanInfo->ContrastRange.lMax     = 127;
    pScanInfo->ContrastRange.lStep    = 1;

    // Scanner settings
    pScanInfo->Intensity              = 0;
    pScanInfo->Contrast               = 0;

    pScanInfo->Xresolution            = 150;
    pScanInfo->Yresolution            = 150;

    pScanInfo->Window.xPos            = 0;
    pScanInfo->Window.yPos            = 0;
    pScanInfo->Window.xExtent         = (pScanInfo->Xresolution * pScanInfo->BedWidth)/1000;
    pScanInfo->Window.yExtent         = (pScanInfo->Yresolution * pScanInfo->BedHeight)/1000;

    // Scanner options
    pScanInfo->DitherPattern          = 0;
    pScanInfo->Negative               = 0;
    pScanInfo->Mirror                 = 0;
    pScanInfo->AutoBack               = 0;
    pScanInfo->ColorDitherPattern     = 0;
    pScanInfo->ToneMap                = 0;
    pScanInfo->Compression            = 0;

        // Image Info
    pScanInfo->DataType               = WIA_DATA_GRAYSCALE;
    pScanInfo->WidthPixels            = (pScanInfo->Window.xExtent)-(pScanInfo->Window.xPos);

    switch(pScanInfo->DataType) {
    case WIA_DATA_THRESHOLD:
        pScanInfo->PixelBits = 1;
        break;
    case WIA_DATA_COLOR:
        pScanInfo->PixelBits = 24;
        break;
    case WIA_DATA_GRAYSCALE:
    default:
        pScanInfo->PixelBits = 8;
        break;
    }

    pScanInfo->WidthBytes = pScanInfo->Window.xExtent * (pScanInfo->PixelBits/8);
    pScanInfo->Lines      = pScanInfo->Window.yExtent;
}

/**************************************************************************\
* SetScannerSettings (helper)
*
*   Called by the MicroDriver to set the values stored in the SCANINFO structure
*   to the actual device.
*
* Arguments:
*
*     none
*
*
* Return Value:
*
*    TRUE - Success, FALSE - Failure
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

BOOL SetScannerSettings(PSCANINFO pScanInfo)
{
    if(pScanInfo->DataType == WIA_DATA_THRESHOLD) {
        pScanInfo->PixelBits = 1;
        pScanInfo->WidthBytes         = (pScanInfo->Window.xExtent)-(pScanInfo->Window.xPos) * (pScanInfo->PixelBits/7);

        //
        // Set data type to device
        //

        // if the set fails..
        // return FALSE;
    }
    else if(pScanInfo->DataType == WIA_DATA_GRAYSCALE) {
        pScanInfo->PixelBits = 8;
        pScanInfo->WidthBytes         = (pScanInfo->Window.xExtent)-(pScanInfo->Window.xPos) * (pScanInfo->PixelBits/8);

        //
        // Set data type to device
        //

        // if the set fails..
        // return FALSE;

    }
    else {
        pScanInfo->PixelBits = 24;
        pScanInfo->WidthBytes         = (pScanInfo->Window.xExtent)-(pScanInfo->Window.xPos) * (pScanInfo->PixelBits/8);

        //
        // Set data type to device
        //

        // if the set fails..
        // return FALSE;

    }

#ifdef DEBUG
    Trace(TEXT("ScanInfo"));
    Trace(TEXT("x res = %d"),pScanInfo->Xresolution);
    Trace(TEXT("y res = %d"),pScanInfo->Yresolution);
    Trace(TEXT("bpp   = %d"),pScanInfo->PixelBits);
    Trace(TEXT("xpos  = %d"),pScanInfo->Window.xPos);
    Trace(TEXT("ypos  = %d"),pScanInfo->Window.yPos);
    Trace(TEXT("xext  = %d"),pScanInfo->Window.xExtent);
    Trace(TEXT("yext  = %d"),pScanInfo->Window.yExtent);
#endif

    //
    // send other values to device, use the values set in pScanInfo to set them to your
    // device.
    //

    return TRUE;
}

/**************************************************************************\
* InitializeScanner (helper)
*
*   Called by the MicroDriver to Iniitialize any device specific operations
*
* Arguments:
*
*    none
*
* Return Value:
*
*    TRUE - Success, FALSE - Failure
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

BOOL InitializeScanner(PSCANINFO pScanInfo)
{
    UNREFERENCED_PARAMETER(pScanInfo);

    HRESULT hr = S_OK;

    //
    // Do any device initialization here...
    // The test device does not need any.
    //

    if (SUCCEEDED(hr)) {
        return TRUE;
    }
    return FALSE;
}

/**************************************************************************\
* CheckButtonStatus (helper)
*
*   Called by the MicroDriver to Set the current Button pressed value.
*
* Arguments:
*
*   pValue       - VAL structure used for settings
*
*
* Return Value:
*
*    VOID
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/


VOID CheckButtonStatus(PVAL pValue)
{
    //
    // Button Polling is done here...
    //

    //
    // Check your device for button presses
    //

    LONG lButtonValue = 0;

    GetButtonPress(&lButtonValue);
    switch (lButtonValue) {
    case 1:
        pValue->pGuid = (GUID*) &guidScanButton;
        Trace(TEXT("Scan Button Pressed!"));
        break;
    default:
        pValue->pGuid = (GUID*) &GUID_NULL;
        break;
    }
}
/**************************************************************************\
* GetInterruptEvent (helper)
*
*   Called by the MicroDriver to handle USB interrupt events.
*
* Arguments:
*
*   pValue       - VAL structure used for settings
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

HRESULT GetInterruptEvent(PVAL pValue)
{
    //
    // Below is a simple example of how DeviceIOControl() can be used to
    // determine interrupts with a USB device.
    //
    // The test device does not support events,
    // So this should not be called.
    //

    HRESULT hr = S_OK;
    BYTE    InterruptData;
    DWORD   dwIndex;
    DWORD   dwError;

    OVERLAPPED Overlapped;
    ZeroMemory( &Overlapped, sizeof( Overlapped ));
    Overlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    HANDLE  hEventArray[2] = {pValue->handle, Overlapped.hEvent};
    BOOL    fLooping = TRUE;
    BOOL    bRet = TRUE;

    //
    // use the Handle created in CMD_INITIALIZE.
    //

    HANDLE  InterruptHandle = pValue->pScanInfo->DeviceIOHandles[1];

    while (fLooping) {

        //
        // Set the wait event, for the interrupt
        //

        bRet = DeviceIoControl( InterruptHandle,
                                (DWORD) IOCTL_WAIT_ON_DEVICE_EVENT,
                                NULL,
                                0,
                                &InterruptData,
                                sizeof(InterruptData),
                                &dwError,
                                &Overlapped );

        if ( bRet || ( !bRet && ( ::GetLastError() == ERROR_IO_PENDING ))) {

            //
            // Wait for the event to happen
            //

            dwIndex = WaitForMultipleObjects( 2,
                                              hEventArray,
                                              FALSE,
                                              INFINITE );

            //
            // Trap the result of the event
            //

            switch ( dwIndex ) {
                case WAIT_OBJECT_0+1:
                    DWORD dwBytesRet;
                    bRet = GetOverlappedResult( InterruptHandle, &Overlapped, &dwBytesRet, FALSE );

                    if ( dwBytesRet ) {

                        //
                        // assign the corresponding button GUID to the *pValue->pGuid
                        // member., and Set the event.
                        //

                        // Change detected - signal
                        if (*pValue->pHandle != INVALID_HANDLE_VALUE) {
                            switch ( InterruptData ) {
                            case 1:
                                *pValue->pGuid = guidScanButton;
                                Trace(TEXT("Scan Button Pressed!"));
                                break;
                            default:
                                *pValue->pGuid = GUID_NULL;
                                break;
                            }
                            Trace(TEXT("Setting This Event by Handle %d"),*pValue->pHandle);

                            //
                            // signal the event, after a button GUID was assigned.
                            //

                            SetEvent(*pValue->pHandle);
                        }
                        break;
                    }

                    //
                    // reset the overlapped event
                    //

                    ResetEvent( Overlapped.hEvent );
                    break;

                case WAIT_OBJECT_0:
                    // Fall through
                default:
                    fLooping = FALSE;
            }
        }
        else {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            break;
        }
    }
    return hr;
}

/**************************************************************************\
* GetButtonPress (helper)
*
*   Called by the MicroDriver to set the actual button value pressed
*
* Arguments:
*
*   pButtonValue       - actual button pressed
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

VOID GetButtonPress(LONG *pButtonValue)
{

    //
    // This where you can set your button value
    //

    pButtonValue = 0;
}

/**************************************************************************\
* GetButtonCount (helper)
*
*   Called by the MicroDriver to get the number of buttons a device supports
*
* Arguments:
*
*    none
*
* Return Value:
*
*    LONG - number of supported buttons
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

LONG GetButtonCount()
{
    LONG ButtonCount  = 0;

    //
    // Since the test device does not have a button,
    // set this value to 0.  For a real device with a button,
    // set (LONG ButtonCount  = 1;)
    //

    //
    // determine the button count of your device
    //

    return ButtonCount;
}

/**************************************************************************\
* GetOLDSTRResourceString (helper)
*
*   Called by the MicroDriver to Load a resource string in OLESTR format
*
* Arguments:
*
*   lResourceID  - String resource ID
*   ppsz         - Pointer to a OLESTR to be filled with the loaded string
*                  value
*   bLocal       - Possible, other source for loading a resource string.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

HRESULT GetOLESTRResourceString(LONG lResourceID,_Outptr_ LPOLESTR *ppsz,BOOL bLocal)
{
    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        INT NumTCHARs = LoadString(g_hInst, lResourceID, szStringValue, sizeof(szStringValue)/sizeof(TCHAR));

        if (NumTCHARs <= 0)
        {

#ifdef UNICODE
            DWORD dwError = GetLastError();
            Trace(TEXT("NumTCHARs = %d dwError = %d Resource ID = %d (UNICODE)szString = %ws"),
                  NumTCHARs,
                  dwError,
                  lResourceID,
                  szStringValue);
#else
            DWORD dwError = GetLastError();
            Trace(TEXT("NumTCHARs = %d dwError = %d Resource ID = %d (ANSI)szString = %s"),
                  NumTCHARs,
                  dwError,
                  lResourceID,
                  szStringValue);
#endif

            return E_FAIL;
        }

        //
        // NOTE: caller must free this allocated BSTR
        //

#ifdef UNICODE

       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(szStringValue));
       if(*ppsz != NULL)
       {

           //
           // The call to LoadString previously guarantees that szStringValue is null terminated (maybe truncated)
           // so a buffer of 'sizeof(szStringValue)/sizeof(TCHAR)' should suffice
           //

           hr = StringCchCopy(*ppsz, sizeof(szStringValue)/sizeof(TCHAR), szStringValue);
       }
       else
       {
           hr =  E_OUTOFMEMORY;
       }

#else
       WCHAR wszStringValue[255];
       ZeroMemory(wszStringValue,sizeof(wszStringValue));

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           wszStringValue,
                           (sizeof(wszStringValue)/sizeof(WCHAR)));

       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(wszStringValue));
       if(*ppsz != NULL)
       {

           //
           // The call to LoadString & MultiByteToWideChar previously guarantees that wszStringValue is null terminated
           // (maybe truncated) so a buffer of 'sizeof(wszStringValue)/sizeof(WCHAR)' should suffice
           //

           hr = StringCchCopyW(*ppsz,sizeof(wszStringValue)/sizeof(WCHAR),wszStringValue);
       }
       else
       {
           hr =  E_OUTOFMEMORY;
       }
#endif

    }
    else
    {

        //
        // looking another place for resources??
        //

        hr = E_NOTIMPL;
    }

    return hr;
}

/**************************************************************************\
* Trace
*
*   Called by the MicroDriver to output strings to a debugger
*
* Arguments:
*
*   format       - formatted string to output
*
*
* Return Value:
*
*    VOID
*
* History:
*
*    1/20/2000 Original Version
*
\**************************************************************************/

VOID Trace(_In_ LPCTSTR format,...)
{

#ifdef DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);

    //
    // StringCchVPrintf API guarantees the buffer to be null terminated (though it maybe truncated)
    //

    StringCchVPrintf(Buffer, sizeof(Buffer)/sizeof(TCHAR), format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

#else

    UNREFERENCED_PARAMETER(format);

#endif

}



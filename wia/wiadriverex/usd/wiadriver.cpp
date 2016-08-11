/**************************************************************************
*
*  Copyright (c) 2003  Microsoft Corporation
*
*  Title: wiadriver.cpp
*
*  Description: This file contains the implementation of IStiUSD and IWiaMiniDrv
*               in the class CWIADriver.
*               The file also contains all COM DLL entry point functions and an
*               implementation of IClassFactory, CWIADriverClassFactory.
*
***************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <initguid.h>
#include "stdafx.h"
#include <strsafe.h>
#include <limits.h>

HINSTANCE g_hInst = NULL;

///////////////////////////////////////////////////////////////////////////////
// WIA driver GUID
///////////////////////////////////////////////////////////////////////////////

// {EEA1E6F7-A59C-487a-BFFA-BD8AA99FE501}
DEFINE_GUID(CLSID_WIADriver, 0xeea1e6f7, 0xa59c, 0x487a, 0xbf, 0xfa, 0xbd, 0x8a, 0xa9, 0x9f, 0xe5, 0x3);

#define HANDLED_PRIVATE_STATUS_ERROR_1      MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_ITF, 1001)
#define UNHANDLED_PRIVATE_STATUS_ERROR_1    MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_ITF, 1002)
#define UNHANDLED_PRIVATE_STATUS_MESSAGE_1  MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 1001)


///////////////////////////////////////////////////////////////////////////
// Construction/Destruction Section
///////////////////////////////////////////////////////////////////////////

CWIADriver::CWIADriver(_In_opt_ LPUNKNOWN punkOuter) : m_cRef(1),
    m_punkOuter(NULL),
    m_pIDrvItemRoot(NULL),
    m_lClientsConnected(0),
    m_pFormats(NULL),
    m_ulNumFormats(0),
    m_bstrDeviceID(NULL),
    m_bstrRootFullItemName(NULL),
    m_ulImageLibraryToken(0),
    m_pIStiDevice(NULL)
{
    if(punkOuter)
    {
        m_punkOuter = punkOuter;
    }
    else
    {
        m_punkOuter = reinterpret_cast<IUnknown*>(static_cast<INonDelegatingUnknown*>(this));
    }

    memset(m_wszStoragePath,0,sizeof(m_wszStoragePath));

    //
    // Intialize GDI+ image library for image manipulation
    //

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if(GdiplusStartup(&m_ulImageLibraryToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
    {
        WIAS_ERROR((g_hInst, "GDI+ image library could not be initialized"));
    }
}

CWIADriver::~CWIADriver()
{
    if(m_bstrDeviceID)
    {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }

    if(m_bstrRootFullItemName)
    {
        SysFreeString(m_bstrRootFullItemName);
        m_bstrRootFullItemName = NULL;
    }

    //
    // Free cached driver capability array
    //

    m_CapabilityManager.Destroy();

    //
    // Free cached driver format array
    //

    if(m_pFormats)
    {
        WIAS_TRACE((g_hInst,"Deleting WIA format array memory"));
        delete [] m_pFormats;
        m_pFormats = NULL;
        m_ulNumFormats = 0;
    }

    //
    // Unlink and release the cached IWiaDrvItem root item interface.
    //

    DestroyDriverItemTree();

    //
    // Unintialize/shutdown GDI+ image library
    //

    if(m_ulImageLibraryToken)
    {
        Gdiplus::GdiplusShutdown(m_ulImageLibraryToken);
        m_ulImageLibraryToken = 0;
    }
}

///////////////////////////////////////////////////////////////////////////
// Standard COM Section
///////////////////////////////////////////////////////////////////////////

HRESULT CWIADriver::QueryInterface(REFIID riid, _COM_Outptr_ LPVOID * ppvObj)
{
    if (ppvObj == NULL)
    {
        return E_INVALIDARG;
    }
    *ppvObj = NULL;

    if(!m_punkOuter)
    {
        return E_NOINTERFACE;
    }
    return m_punkOuter->QueryInterface(riid,ppvObj);
}
ULONG CWIADriver::AddRef()
{
    if(!m_punkOuter)
    {
        return 0;
    }
    return m_punkOuter->AddRef();
}
ULONG CWIADriver::Release()
{
    if(!m_punkOuter)
    {
        return 0;
    }
    return m_punkOuter->Release();
}

///////////////////////////////////////////////////////////////////////////
// IStiUSD Interface Section (for all WIA drivers)
///////////////////////////////////////////////////////////////////////////

HRESULT CWIADriver::Initialize(_In_     PSTIDEVICECONTROL pHelDcb,
                                        DWORD             dwStiVersion,
                               _In_     HKEY              hParametersKey)
{
    UNREFERENCED_PARAMETER(dwStiVersion);

    HRESULT hr = E_INVALIDARG;
    if((pHelDcb)&&(hParametersKey))
    {
        //
        // Open DeviceData section in the registry
        //

        HKEY hDeviceDataKey = NULL;
        if(RegOpenKeyEx(hParametersKey,REG_ENTRY_DEVICEDATA,0,KEY_QUERY_VALUE|KEY_READ,&hDeviceDataKey) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(m_wszStoragePath);
            DWORD dwType = REG_SZ;
            if(RegQueryValueEx(hDeviceDataKey,REG_ENTRY_STORAGEPATH,NULL,&dwType,(BYTE*)m_wszStoragePath,&dwSize) == ERROR_SUCCESS)
            {
                WIAS_TRACE((g_hInst,"WIA storage path = %ws",m_wszStoragePath));
                hr = S_OK;
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to read (%ws) entry under %ws section of device registry",REG_ENTRY_STORAGEPATH,REG_ENTRY_DEVICEDATA));
            }

            hr = S_OK;

            //
            // close open DeviceData registry key
            //

            RegCloseKey(hDeviceDataKey);
            hDeviceDataKey = NULL;
        }

        hr = m_CapabilityManager.Initialize(g_hInst);
        if(FAILED(hr))
        {
            WIAS_ERROR((g_hInst, "Failed to initialize the WIA driver capability manager object, hr = 0x%lx",hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::GetCapabilities(_Out_ PSTI_USD_CAPS pDevCaps)
{
    HRESULT hr = E_INVALIDARG;
    if(pDevCaps)
    {
        memset(pDevCaps, 0, sizeof(STI_USD_CAPS));
        pDevCaps->dwVersion     = STI_VERSION_3;
        pDevCaps->dwGenericCaps = STI_GENCAP_WIA                    |
                                  STI_USD_GENCAP_NATIVE_PUSHSUPPORT |
                                  STI_GENCAP_NOTIFICATIONS          |
                                  STI_GENCAP_POLLING_NEEDED;

        WIAS_TRACE((g_hInst,"========================================================"));
        WIAS_TRACE((g_hInst,"STI Capabilities information reported to the WIA Service"));
        WIAS_TRACE((g_hInst,"Version:     0x%lx",pDevCaps->dwVersion));
        WIAS_TRACE((g_hInst,"GenericCaps: 0x%lx", pDevCaps->dwGenericCaps));
        WIAS_TRACE((g_hInst,"========================================================"));

        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::GetStatus(_Inout_ PSTI_DEVICE_STATUS pDevStatus)
{
    HRESULT hr = E_INVALIDARG;
    if(pDevStatus)
    {
        //
        // assume successful status checks
        //

        hr = S_OK;

        if(pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)
        {
            //
            // check if the device is ON-LINE
            //

            WIAS_TRACE((g_hInst,"Checking device online status..."));
            pDevStatus->dwOnlineState = 0L;

            if(SUCCEEDED(hr))
            {
                pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
                WIAS_TRACE((g_hInst,"The device is online"));
            }
            else
            {
                WIAS_TRACE((g_hInst,"The device is offline"));
            }
        }

        if(pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE)
        {
            //
            // check for polled events
            //

            pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;

            hr = S_FALSE; // no are events detected

            if(hr == S_OK)
            {
                pDevStatus->dwEventHandlingState |= STI_EVENTHANDLING_PENDING;
                WIAS_TRACE((g_hInst,"The device reported a polled event"));
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}
HRESULT CWIADriver::DeviceReset()
{
    return S_OK;
}
HRESULT CWIADriver::Diagnostic(_Out_ LPDIAG pBuffer)
{
    HRESULT hr = E_INVALIDARG;
    if(pBuffer)
    {
        memset(pBuffer,0,sizeof(DIAG));
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}
HRESULT CWIADriver::Escape(                              STI_RAW_CONTROL_CODE EscapeFunction,
                           _In_reads_bytes_(cbInDataSize)     LPVOID               lpInData,
                                                         DWORD                cbInDataSize,
                           _Out_writes_bytes_(dwOutDataSize)   LPVOID               pOutData,
                                                         DWORD                dwOutDataSize,
                           _Out_                         LPDWORD              pdwActualData)
{
    UNREFERENCED_PARAMETER(EscapeFunction);
    UNREFERENCED_PARAMETER(lpInData);
    UNREFERENCED_PARAMETER(cbInDataSize);
    UNREFERENCED_PARAMETER(pOutData);
    UNREFERENCED_PARAMETER(dwOutDataSize);
    UNREFERENCED_PARAMETER(pdwActualData);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::GetLastError(_Out_ LPDWORD pdwLastDeviceError)
{
    HRESULT hr = E_INVALIDARG;
    if(pdwLastDeviceError)
    {
        *pdwLastDeviceError = 0;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}
HRESULT CWIADriver::LockDevice()
{
    return S_OK;
}
HRESULT CWIADriver::UnLockDevice()
{
    return S_OK;
}
HRESULT CWIADriver::RawReadData(_Out_writes_bytes_(*lpdwNumberOfBytes)      LPVOID       lpBuffer,
                                _Out_                                 LPDWORD      lpdwNumberOfBytes,
                                _Out_                                 LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(lpdwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::RawWriteData(_In_reads_bytes_(dwNumberOfBytes)    LPVOID       lpBuffer,
                                                                 DWORD        dwNumberOfBytes,
                                                                 _Out_ LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::RawReadCommand(_Out_writes_bytes_(*lpdwNumberOfBytes) LPVOID       lpBuffer,
                                   _Out_                            LPDWORD      lpdwNumberOfBytes,
                                   _Out_                            LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(lpdwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::RawWriteCommand(_In_reads_bytes_(dwNumberOfBytes) LPVOID       lpBuffer,
                                                                 DWORD        dwNumberOfBytes,
                                                                 _Out_ LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpBuffer);
    UNREFERENCED_PARAMETER(dwNumberOfBytes);
    UNREFERENCED_PARAMETER(lpOverlapped);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}

HRESULT CWIADriver::SetNotificationHandle(_In_ HANDLE hEvent)
{
    UNREFERENCED_PARAMETER(hEvent);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::GetNotificationData(_In_ LPSTINOTIFY lpNotify)
{
    UNREFERENCED_PARAMETER(lpNotify);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::GetLastErrorInfo(_Out_ STI_ERROR_INFO *pLastErrorInfo)
{
    HRESULT hr = E_INVALIDARG;
    if(pLastErrorInfo)
    {
        memset(pLastErrorInfo,0,sizeof(STI_ERROR_INFO));
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// IWiaMiniDrv Interface Section (for all WIA drivers)                 //
/////////////////////////////////////////////////////////////////////////

HRESULT CWIADriver::drvInitializeWia(_Inout_ BYTE        *pWiasContext,
                                             LONG        lFlags,
                                     _In_    BSTR        bstrDeviceID,
                                     _In_    BSTR        bstrRootFullItemName,
                                     _In_    IUnknown    *pStiDevice,
                                     _In_    IUnknown    *pIUnknownOuter,
                                     _Out_   IWiaDrvItem **ppIDrvItemRoot,
                                     _Out_   IUnknown    **ppIUnknownInner,
                                     _Out_   LONG        *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(pIUnknownOuter);

    HRESULT hr = S_OK;
    if((pWiasContext)&&(plDevErrVal)&&(ppIDrvItemRoot))
    {
        *plDevErrVal = 0;
        *ppIDrvItemRoot = NULL;
        *ppIUnknownInner = NULL;

        if(!m_bstrDeviceID)
        {
            m_bstrDeviceID = SysAllocString(bstrDeviceID);
            if(!m_bstrDeviceID)
            {
                hr = E_OUTOFMEMORY;
                WIAS_ERROR((g_hInst, "Failed to allocate BSTR DeviceID string, hr = 0x%lx",hr));
            }
        }

        if(!m_pIStiDevice)
        {
            m_pIStiDevice = reinterpret_cast<IStiDevice*>(pStiDevice);
        }

        if(!m_bstrRootFullItemName)
        {
            m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);
            if(!m_bstrRootFullItemName)
            {
                hr = E_OUTOFMEMORY;
                WIAS_ERROR((g_hInst, "Failed to allocate BSTR Root full item name string, hr = 0x%lx",hr));
            }
        }

        if(SUCCEEDED(hr))
        {
            if(!m_pIDrvItemRoot)
            {
                hr = BuildDriverItemTree();
            }
            else
            {

                //
                // A WIA item tree already exists.  The root item of this item tree
                // should be returned to the WIA service.
                //

                hr = S_OK;
            }
        }

        //
        // Make PREfast happy by inspecting m_pIDrvItemRoot.
        // PREfast doesn't seem to figure out that m_pIDrvItemRoot is set by
        // BuildDriverTree()'s call to waisCreateDrvItem() only on success.
        //

        if(SUCCEEDED(hr) && !m_pIDrvItemRoot)
        {
            hr = E_UNEXPECTED;
            WIAS_ERROR((g_hInst, "Missing driver item tree root unexpected, hr = 0x%lx",hr));
        }

        //
        // Only increment the client connection count, when the driver
        // has successfully created all the necessary WIA items for
        // a client to use.
        //

        if(SUCCEEDED(hr))
        {
            *ppIDrvItemRoot = m_pIDrvItemRoot;
            InterlockedIncrement(&m_lClientsConnected);
            WIAS_TRACE((g_hInst,"%d client(s) are currently connected to this driver.",m_lClientsConnected));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

UINT CWIADriver::GetBitmapResourceIDFromCategory(const GUID &guidItemCategory)
{
    UINT uiBitmapResourceID = 0;

    if (guidItemCategory == WIA_CATEGORY_FLATBED)
    {
        uiBitmapResourceID = IDB_FLATBED;
    }
    else if (guidItemCategory == WIA_CATEGORY_FEEDER)
    {
        uiBitmapResourceID = IDB_FEEDER;
    }
    else if (guidItemCategory == WIA_CATEGORY_FILM)
    {
        uiBitmapResourceID = IDB_FILM;
    }
    else
    {
        uiBitmapResourceID = IDB_FLATBED;
    }

    return uiBitmapResourceID;
}


/*++

Routine Name:        CWIADriver::DownloadRawHeader

Routine Description: Builds and downloads to the specified ouput stream the WIA Raw Format header.
                     It should be called only from within CWIADriver::DownloadToStream
                     after WiaDevice::InitializeForDownload was executed
Arguments:
                     pDestination      - the output stream (same as used in DownloadToStream)
                     pWiasContext      - WIA service context, passed by caller (DownloadToStream)
                     pmdtc             - the stream WIA mini-driver context (see DownloadToStream)

Return Value:
                     HRESULT (S_OK in case the operation succeeds)
Last Error:
                     -
++*/
HRESULT
CWIADriver::DownloadRawHeader(
    _In_    IStream                     *pDestination,
    _Inout_ BYTE                        *pWiasContext,
    _In_    PMINIDRV_TRANSFER_CONTEXT   pmdtc
    )
{
    HRESULT hr = S_OK;
    LONG lValue = 0;
    WIA_RAW_HEADER& RawHeader = m_WiaDevice.m_RawHeader;

    //
    // Verify input parameters:
    //
    if((!pDestination) || (!pmdtc))
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameter(s) for DownloadRawHeader, hr: 0x%X", hr));
    }

    if(S_OK == hr)
    {
        //
        // The 'WRAW' 4 ASCII character signature is required at the begining of all WIA Raw transfers:
        //
        const char szSignature[] = "WRAW";
        memcpy(&RawHeader.Tag, szSignature, sizeof(DWORD));

        //
        // Fill in the fields describing version identity for this header:
        //
        RawHeader.Version = 0x00010000;
        RawHeader.HeaderSize = sizeof(WIA_RAW_HEADER);

        //
        // Fill in all the fields that we can retrieve directly from the current MINIDRV_TRANSFER_CONTEXT:
        //
        RawHeader.XRes = pmdtc->lXRes;
        RawHeader.YRes = pmdtc->lYRes;
        RawHeader.XExtent = pmdtc->lWidthInPixels;
        RawHeader.YExtent = pmdtc->lLines;
        RawHeader.BitsPerPixel = pmdtc->lDepth;
        RawHeader.Compression = pmdtc->lCompression;

        //
        // Raw data: the offset is the size of the header (we don't have a color palette in this case):
        //
        RawHeader.RawDataOffset = RawHeader.HeaderSize;

        //
        // Notes:
        //
        // RawHeader.RawDataSize is filled in already by CWiaDevice::InitializeForDownload
        // Same for RawHeader.BytesPerLine.
        //
    }

    //
    // The remaining fields have to be filled in reading the rescctive current property values:
    //

    //
    // The pixel/data type is described by WIA_IPA_FORMAT:
    //
    if(S_OK == hr)
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_FORMAT, &lValue, NULL, true);
        if(S_OK == hr)
        {
            RawHeader.DataType = lValue;
        }
        else
        {
            WIAS_ERROR((g_hInst, "wiasReadPropLong(WIA_IPA_FORMAT) failed, hr: 0x%X", hr));
        }
    }

    //
    // The number of channels per pixel is described by WIA_IPA_CHANNELS_PER_PIXEL:
    //
    if(S_OK == hr)
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_CHANNELS_PER_PIXEL, &lValue, NULL, true);
        if(S_OK == hr)
        {
            RawHeader.ChannelsPerPixel = lValue;
        }
        else
        {
            WIAS_ERROR((g_hInst, "wiasReadPropLong(WIA_IPA_CHANNELS_PER_PIXEL) failed, hr: 0x%X", hr));
        }
    }

    //
    // The photometric interpretation is described by WIA_IPS_PHOTOMETRIC_INTERP:
    //
    if(S_OK == hr)
    {
        hr = wiasReadPropLong(pWiasContext, WIA_IPS_PHOTOMETRIC_INTERP, &lValue, NULL, true);
        if(S_OK == hr)
        {
            RawHeader.PhotometricInterp = lValue;
        }
        else
        {
            WIAS_ERROR((g_hInst, "wiasReadPropLong(WIA_IPS_PHOTOMETRIC_INTERP) failed, hr: 0x%X", hr));
        }
    }

    //
    // The discrete bits per channel table is described by the new WIA_IPA_RAW_BITS_PER_CHANNEL:
    //
    if(S_OK == hr)
    {
        memset(&RawHeader.BitsPerChannel[0], 0, sizeof(RawHeader.BitsPerChannel));

        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = WIA_IPA_RAW_BITS_PER_CHANNEL;
        PROPVARIANT pv = {0};

        hr = wiasReadMultiple(pWiasContext, 1, &ps, &pv, NULL);
        if(S_OK == hr)
        {
            ULONG ulItemCount = (pv.caub.cElems > 8) ? 8 : pv.caub.cElems;
            for(ULONG i = 0; i < ulItemCount; i++)
            {
                RawHeader.BitsPerChannel[i] = *(BYTE *)((BYTE *)pv.caub.pElems + i * sizeof(BYTE));
            }
        }
    }

    //
    // In the case of this sample the image data is retrieved from a resource bitmap.
    //
    // Important: this bitmap must be initialized by the WiaDevice::InitializeForDownload
    // before calling this function.
    //
    if(S_OK == hr)
    {
        if(!m_WiaDevice.InitializedForDownload())
        {
            //
            // S_FALSE returned from this function would be interpreted as a cancel request:
            //
            hr = E_INVALIDARG;
            WIAS_ERROR((g_hInst, "Bitmap not initialized correctly, hr: 0x%X", hr));
        }
    }

    if(S_OK == hr)
    {
        //
        // For the line order use the BitmapData object initialized for the sample bitmap that
        // we are using: the "Stride" field value sign indicates the line order:
        //
        RawHeader.LineOrder = ((m_WiaDevice.GetBitmapData())->Stride < 0) ?
            WIA_LINE_ORDER_BOTTOM_TO_TOP : WIA_LINE_ORDER_TOP_TO_BOTTOM;

        //
        // We won't be using a color palette here but it would be possible to try to retrieve
        // the color palette, if any, from the DIB header describing the sample bitmap:
        //
        RawHeader.PaletteSize = 0;
        RawHeader.PaletteOffset = 0;
    }

    //
    // Write the header to the stream provided to us:
    //
    ULONG ulBytesWritten = 0;
    if(S_OK == hr)
    {
        hr = pDestination->Write(&RawHeader, RawHeader.HeaderSize, &ulBytesWritten);
    }

    return hr;
}



HRESULT CWIADriver::DownloadToStream(           LONG                           lFlags,
                                     _In_       BYTE                           *pWiasContext,
                                     _In_       PMINIDRV_TRANSFER_CONTEXT      pmdtc,
                                     const      GUID                           &guidItemCategory,
                                     const      GUID                           &guidFormatID,
                                     __callback IWiaMiniDrvTransferCallback    *pTransferCallback,
                                     _Out_      LONG                           *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr                  = S_OK;
    BSTR    bstrItemName        = NULL;
    BSTR    bstrFullItemName    = NULL;
    UINT    uiBitmapResourceID  = GetBitmapResourceIDFromCategory(guidItemCategory);

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //
    // A maximum of 10 image transfers (including final and preview scans) can be requested
    // from the Feeder item before the driver will return WIA_ERROR_PAPER_EMPTY. In order to
    // reset the counter (used only for the Feeder item) the application must change a Feeder
    // item property current value or reload the driver.
    //
    // IMPORTANT:
    //
    // Legacy WIA applications such as Scan Wizard requires WIA_ERROR_PAPER_EMPTY
    // (as the return code for IWiaMiniDrv::drvAcquireItemData) in order to stop
    // normally a Feeder acquisition sequence.
    //
    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
    hr = wiasGetDriverItemPrivateContext(pWiasContext, (BYTE**)&pWiaDriverItemContext);
    if ((!pWiaDriverItemContext) && (SUCCEEDED(hr)))
    {
        hr = E_POINTER;
    }
    if (FAILED(hr))
    {
        WIAS_ERROR((g_hInst, "Failed to get private driver item context data, hr = 0x%lx", hr));
    }

    const ULONG ulMaxTransfers = 10;
    if ((SUCCEEDED(hr)) && (IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory)))
    {
        //
        // Limit the number of "continuous" transfers from the Feeder item -
        // without this Scan Wizard would not stop requesting transfers:
        //
        if (pWiaDriverItemContext->ulFeederTransferCount >= ulMaxTransfers)
        {
            hr = WIA_ERROR_PAPER_EMPTY;
        }
    }

    if (S_OK == hr)
    {
        //  Get the item name
        hr = wiasReadPropStr(pWiasContext, WIA_IPA_ITEM_NAME, &bstrItemName, NULL, TRUE);
        if (SUCCEEDED(hr))
        {
            //  Get the full item name
            hr = wiasReadPropStr(pWiasContext, WIA_IPA_FULL_ITEM_NAME, &bstrFullItemName, NULL, TRUE);
            if (SUCCEEDED(hr))
            {
                //  Get the destination stream
                IStream *pDestination = NULL;
                _Analysis_assume_nullterminated_(bstrItemName);
                hr = pTransferCallback->GetNextStream(0, bstrItemName, bstrFullItemName, &pDestination);
                if (hr == S_OK)
                {
                    WiaTransferParams *pParams = (WiaTransferParams*)CoTaskMemAlloc(sizeof(WiaTransferParams));
                    if (pParams)
                    {
                        memset(pParams, 0, sizeof(WiaTransferParams));
                        BYTE    *pBuffer        = NULL;
                        ULONG   ulBufferSize    = 0;
                        hr = AllocateTransferBuffer(pWiasContext, &pBuffer, &ulBufferSize);
                        if (SUCCEEDED(hr))
                        {
                            if ((S_OK == hr) && (guidItemCategory != WIA_CATEGORY_FINISHED_FILE) && (WIA_CATEGORY_FOLDER != guidItemCategory))
                            {
                                LONG    lErrorHandling = ERROR_HANDLING_NONE;

                                hr = wiasReadPropLong(pWiasContext, MY_WIA_ERROR_HANDLING_PROP, &lErrorHandling, NULL, TRUE);

                                BOOL    bSendWarmingUpMsg       = lErrorHandling & ERROR_HANDLING_WARMING_UP;
                                BOOL    bSendCoverOpenMsg       = lErrorHandling & ERROR_HANDLING_COVER_OPEN;
                                BOOL    bSendPrivateErrorMsg    = lErrorHandling & ERROR_HANDLING_PRIVATE_ERROR;
                                BOOL    bSendUnhandledStatusMsg = lErrorHandling & ERROR_HANDLING_UNHANDLED_STATUS;
                                BOOL    bSendUnhandledErrorMsg  = lErrorHandling & ERROR_HANDLING_UNHANDLED_ERROR;

                                //  We need to initialize our device object for each item we transfer.
                                //  Each item may have it's own selection area, data type and so on.
                                hr = m_WiaDevice.InitializeForDownload(pWiasContext,
                                                                    g_hInst,
                                                                    uiBitmapResourceID,
                                                                    guidFormatID);

                                if ((S_OK == hr) && bSendWarmingUpMsg)
                                {
                                    //
                                    // Send non-modal warming up message. To be catched by default UI
                                    // unless application handles it (WiaPreview does not handle this
                                    // message).
                                    //
                                    // Sending "update messages" makes it possible for a user to cancel transfer
                                    // and also for an error handler to provide progress dialog.
                                    //
                                    for (int i = 0; i < 10 ; i++)
                                    {
                                        pParams->lMessage           = WIA_TRANSFER_MSG_DEVICE_STATUS;
                                        pParams->hrErrorStatus      = WIA_STATUS_WARMING_UP;
                                        pParams->lPercentComplete   = i * 10;
                                        pParams->ulTransferredBytes = 0;

                                        hr = pTransferCallback->SendMessage(0, pParams);

                                        if (S_OK != hr)
                                        {
                                            break;
                                        }

                                        Sleep(500);
                                    }
                                }

                                if (S_OK == hr)
                                {
                                    BOOL    bProblemFixed = FALSE;

                                    //  Data transfer loop
                                    //  Read from device
                                    ULONG   ulBytesRead         = 0;
                                    LONG    lPercentComplete    = 0;

                                    if (bSendUnhandledStatusMsg)
                                    {
                                        //
                                        // Send "special" unhandled status message
                                        //
                                        pParams->lMessage           = WIA_TRANSFER_MSG_DEVICE_STATUS;
                                        pParams->hrErrorStatus      = UNHANDLED_PRIVATE_STATUS_MESSAGE_1;
                                        pParams->lPercentComplete   = 0;
                                        pParams->ulTransferredBytes = 0;

                                        hr = pTransferCallback->SendMessage(0, pParams);
                                    }

                                    if ((S_OK == hr) && bSendUnhandledErrorMsg)
                                    {

                                        //
                                        // Since none handles this device error it will cause our transfer to be
                                        // be aborted.
                                        //
                                        pParams->lMessage           = WIA_TRANSFER_MSG_DEVICE_STATUS;
                                        pParams->hrErrorStatus      = UNHANDLED_PRIVATE_STATUS_ERROR_1;
                                        pParams->lPercentComplete   = 0;
                                        pParams->ulTransferredBytes = 0;

                                        hr = pTransferCallback->SendMessage(0, pParams);
                                    }

                                    if ((S_OK == hr) && bSendCoverOpenMsg)
                                    {
                                        pParams->lMessage           = WIA_TRANSFER_MSG_DEVICE_STATUS;
                                        pParams->hrErrorStatus      = WIA_ERROR_COVER_OPEN;
                                        pParams->lPercentComplete   = 0;
                                        pParams->ulTransferredBytes = 0;

                                        hr = pTransferCallback->SendMessage(0, pParams);
                                    }

                                    //
                                    // If this is a Raw format transfer we should transfer the raw header first.
                                    // WiaDevice::InitializeForDownload suceedeed and it is safe to execute
                                    // now DownloadRawHeader:
                                    //
                                    if((S_OK == hr) && (IsEqualGUID(guidFormatID, WiaImgFmt_RAW)))
                                    {
                                        hr = DownloadRawHeader(pDestination, pWiasContext, pmdtc);

                                        if(S_OK == hr)
                                        {
                                            WIA_RAW_HEADER& RawHeader = m_WiaDevice.m_RawHeader;
                                            lPercentComplete  = (LONG)((((float)RawHeader.HeaderSize /
                                                (float)(RawHeader.RawDataSize + RawHeader.HeaderSize + RawHeader.PaletteSize))) * 100.0f);

                                            pParams->lMessage            = WIA_TRANSFER_MSG_STATUS;
                                            pParams->lPercentComplete    = lPercentComplete;
                                            pParams->ulTransferredBytes += RawHeader.HeaderSize;

                                            hr = pTransferCallback->SendMessage(0, pParams);
                                        }
                                    }

                                    while((S_OK == hr) &&
                                        ((hr = m_WiaDevice.GetNextBand(pBuffer, ulBufferSize, &ulBytesRead, &lPercentComplete, guidFormatID)) == S_OK))
                                    {
                                        ULONG   ulBytesWritten = 0;
                                        LARGE_INTEGER li = {0};

                                        //
                                        // Write to stream after seeking to end of stream as it could
                                        // be randomized intially or during the callback
                                        //
                                        hr = pDestination->Seek(li, STREAM_SEEK_END, NULL);

                                        if (S_OK == hr)
                                        {
                                            hr = pDestination->Write(pBuffer, ulBytesRead, &ulBytesWritten);
                                        }

                                        if (S_OK == hr)
                                        {
                                            //
                                            // Make progress callback
                                            //
                                            pParams->lMessage            = WIA_TRANSFER_MSG_STATUS;
                                            pParams->lPercentComplete    = lPercentComplete;
                                            pParams->ulTransferredBytes += ulBytesWritten;

                                            hr = pTransferCallback->SendMessage(0, pParams);
                                            if (FAILED(hr))
                                            {
                                                WIAS_ERROR((g_hInst, "Failed to send progress notification during download, hr = 0x%lx",hr));
                                                break;
                                            }
                                            else if (S_FALSE == hr)
                                            {
                                                //
                                                // Transfer cancelled
                                                //
                                                break;
                                            }
                                            else if (S_OK != hr)
                                            {
                                                WIAS_ERROR((g_hInst, "SendMessage returned unknown Success value, hr = 0x%lx",hr));
                                                hr = E_UNEXPECTED;
                                                break;
                                            }

                                            if ((lPercentComplete > 50) && !bProblemFixed)
                                            {

                                                if (bSendPrivateErrorMsg)
                                                {
                                                    //
                                                    // Send "special" driver status message that only our error handling extension knows about
                                                    //
                                                    pParams->lMessage                       = WIA_TRANSFER_MSG_DEVICE_STATUS;
                                                    pParams->hrErrorStatus                  = HANDLED_PRIVATE_STATUS_ERROR_1;

                                                    hr = pTransferCallback->SendMessage(0, pParams);
                                                }

                                                if (S_OK == hr)
                                                {
                                                    bProblemFixed = TRUE;
                                                }
                                            }
                                        }
                                    }

                                    if ((pWiaDriverItemContext) && (IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory)))
                                    {
                                        //
                                        // Increment the feeder transfer counter for both preview and final scans:
                                        //
                                        if (pWiaDriverItemContext->ulFeederTransferCount < ulMaxTransfers)
                                        {
                                            pWiaDriverItemContext->ulFeederTransferCount += 1;
                                        }
                                    }

                                    if (WIA_STATUS_END_OF_MEDIA == hr)
                                    {
                                        hr = S_OK;
                                    }

                                    m_WiaDevice.UninitializeForDownload();
                                }
                                else
                                {
                                    WIAS_ERROR((g_hInst, "Failed to initialize device for download, hr = 0x%lx",hr));
                                }
                            }
                            else
                            {
                                IStream *pStorageDataStream = NULL;

                                hr = SHCreateStreamOnFile(pWiaDriverItemContext->bstrStorageDataPath,STGM_READ,&pStorageDataStream);

                                if(SUCCEEDED(hr))
                                {
                                    STATSTG statstg = {0};
                                    ULONG ulTotalBytesToWrite = 0;

                                    hr = pStorageDataStream->Stat(&statstg,  STATFLAG_NONAME);

                                    if (SUCCEEDED(hr))
                                    {
                                        ulTotalBytesToWrite = statstg.cbSize.LowPart;

                                        if (!ulTotalBytesToWrite)
                                        {
                                            hr = E_UNEXPECTED;
                                            WIAS_ERROR((g_hInst, "Storage item has zero size, hr = %#x", hr));
                                        }
                                    }

                                    if (SUCCEEDED(hr))
                                    {
                                        ULONG ulBytesRead      = 0;
                                        ULONG ulTotalBytesWritten = 0;
                                        LONG  lPercentComplete = -1;

                                        while((SUCCEEDED(pStorageDataStream->Read(pBuffer, ulBufferSize, &ulBytesRead)) && ulBytesRead))
                                        {
                                            //
                                            // Write to stream
                                            //
                                            ULONG   ulBytesWritten = 0;
                                            hr = pDestination->Write(pBuffer, ulBytesRead, &ulBytesWritten);

                                            if (SUCCEEDED(hr))
                                            {
                                                ulTotalBytesWritten += ulBytesWritten;
                                                lPercentComplete = (LONG)((((float)ulTotalBytesWritten/(float)ulTotalBytesToWrite)) * 100.0f);

                                                //
                                                // Make progress callback
                                                //
                                                pParams->lMessage            = WIA_TRANSFER_MSG_STATUS;
                                                pParams->lPercentComplete    = lPercentComplete;
                                                pParams->ulTransferredBytes += ulBytesWritten;

                                                hr = pTransferCallback->SendMessage(0, pParams);
                                                if (hr != S_OK)
                                                {
                                                    if (FAILED(hr))
                                                    {
                                                        WIAS_ERROR((g_hInst, "Failed to send progress notification during download, hr = 0x%lx",hr));
                                                    }
                                                    else if (S_FALSE == hr)
                                                    {
                                                        WIAS_TRACE((g_hInst, "Download was cancelled"));
                                                    }
                                                    else
                                                    {
                                                        WIAS_ERROR((g_hInst, "SendMessage returned unknown Success value, hr = 0x%lx",hr));
                                                        hr = E_UNEXPECTED;
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    pStorageDataStream->Release();
                                    pStorageDataStream = NULL;
                                }
                                else
                                {
                                    WIAS_ERROR((g_hInst, "Failed to create a source stream on storage item data content file (%ws), hr = 0x%lx",pWiaDriverItemContext->bstrStorageDataPath,hr));
                                }
                            }
                            FreeTransferBuffer(pBuffer);
                        }
                        else
                        {
                            WIAS_ERROR((g_hInst, "Failed to allocate memory for transfer buffer, hr = 0x%lx",hr));
                        }
                        CoTaskMemFree(pParams);
                        pParams = NULL;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        WIAS_ERROR((g_hInst, "Failed to allocate memory for WiaTransferParams structure, hr = 0x%lx",hr));
                    }
                    pDestination->Release();
                    pDestination = NULL;
                }
                else if(!((S_FALSE == hr) || (WIA_STATUS_SKIP_ITEM == hr)))
                {
                    WIAS_ERROR((g_hInst, "GetNextStream returned unknown Success value, hr = 0x%lx",hr));
                    hr = E_UNEXPECTED;
                }
                else
                {
                    WIAS_ERROR((g_hInst, "Failed to get the destination stream for download, hr = 0x%lx",hr));
                }

                SysFreeString(bstrFullItemName);
                bstrFullItemName = NULL;
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to read the WIA_IPA_FULL_ITEM_NAME property, hr = 0x%lx",hr));
            }
            SysFreeString(bstrItemName);
            bstrItemName = NULL;
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to read the WIA_IPA_ITEM_NAME property, hr = 0x%lx",hr));
        }
    }
    return hr;
}

HRESULT CWIADriver::UploadFromStream(           LONG                           lFlags,
                                     _In_       BYTE                           *pWiasContext,
                                                const GUID                     &guidItemCategory,
                                     __callback IWiaMiniDrvTransferCallback    *pTransferCallback,
                                     _Out_      LONG                           *plDevErrVal)
{
    UNREFERENCED_PARAMETER(guidItemCategory);

    HRESULT hr = S_OK;
    BSTR bstrItemName = NULL;
    BSTR bstrFullItemName = NULL;

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    //  Get the item name
    hr = wiasReadPropStr(pWiasContext, WIA_IPA_ITEM_NAME, &bstrItemName, NULL, TRUE);
    if (SUCCEEDED(hr))
    {
        //  Get the full item name
        hr = wiasReadPropStr(pWiasContext, WIA_IPA_FULL_ITEM_NAME, &bstrFullItemName, NULL, TRUE);
        if (SUCCEEDED(hr))
        {
            //  Get the source stream
            IStream *pSourceStream = NULL;
            _Analysis_assume_nullterminated_(bstrItemName);
            hr = pTransferCallback->GetNextStream(lFlags, bstrItemName, bstrFullItemName, &pSourceStream);
            if (S_OK == hr)
            {
                hr = wiasReadPropStr(pWiasContext,WIA_IPA_ITEM_NAME,&bstrItemName,NULL,TRUE);
                if(SUCCEEDED(hr))
                {
                    STATSTG statstg = {0};

                    hr = pSourceStream->Stat(&statstg, STATFLAG_NONAME);
                    if(SUCCEEDED(hr))
                    {
                        WiaTransferParams *pParams = (WiaTransferParams*)CoTaskMemAlloc(sizeof(WiaTransferParams));
                        if (pParams)
                        {
                            memset(pParams, 0, sizeof(WiaTransferParams));

                            hr = m_WiaDevice.Upload(bstrItemName, statstg.cbSize.LowPart, pSourceStream,pTransferCallback, pParams,m_wszStoragePath);
                            if(SUCCEEDED(hr))
                            {
                                // Succeeded with upload.  We expect the App to do a synchronize to get the new items,
                                // so there's nothing further we need to do.

                                //
                                // TBD: Ideal case would be to create a WIA driver item, and link it to the existing
                                //      application item.  This will also be the place that a item created/added event
                                //      would be sent to the other clients, allowing them to reenumerate and pick up the
                                //      freshly uploaded item.
                                //
                            }
                            else
                            {
                                WIAS_ERROR((g_hInst, "Failed to upload data to the device, hr = 0x%lx",hr));
                            }

                            CoTaskMemFree(pParams);
                            pParams = NULL;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            WIAS_ERROR((g_hInst, "Failed to allocate memory for WiaTransferParams structure, hr = 0x%lx",hr));
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "Failed to call IStream::Stat on application provided stream, hr = 0x%lx",hr));
                    }
                    SysFreeString(bstrItemName);
                    bstrItemName = NULL;
                }
                else
                {
                    WIAS_ERROR((g_hInst, "Failed to read WIA_IPA_ITEM_NAME property, hr = 0x%lx",hr));
                }

                pSourceStream->Release();
                pSourceStream = NULL;
            }
            else if(!((S_FALSE == hr) ||(WIA_STATUS_SKIP_ITEM == hr)))
            {
                WIAS_ERROR((g_hInst, "GetNextStream returned unknown Success value, hr = 0x%lx",hr));
                hr = E_UNEXPECTED;
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to get the source stream for upload, hr = 0x%lx",hr));
            }

            SysFreeString(bstrFullItemName);
            bstrFullItemName = NULL;
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to read the WIA_IPA_FULL_ITEM_NAME property, hr = 0x%lx",hr));
        }
        SysFreeString(bstrItemName);
        bstrItemName = NULL;
    }
    else
    {
        WIAS_ERROR((g_hInst, "Failed to read the WIA_IPA_ITEM_NAME property, hr = 0x%lx",hr));
    }

    return hr;
}

HRESULT CWIADriver::drvAcquireItemData(_In_    BYTE                      *pWiasContext,
                                               LONG                      lFlags,
                                       _In_    PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                       _Out_   LONG                      *plDevErrVal)
{
    HRESULT hr                  = E_INVALIDARG;
    GUID    guidItemCategory    = GUID_NULL;
    GUID    guidFormatID        = GUID_NULL;

    if((pWiasContext)&&(pmdtc)&&(plDevErrVal))
    {
        *plDevErrVal = 0;

        //
        // Read the current transfer format that we are requested to use:
        //
        guidFormatID = pmdtc->guidFormatID;

        //
        // Read the WIA item category, to decide which data transfer handler should
        // be used.
        //
        hr = wiasReadPropGuid(pWiasContext,WIA_IPA_ITEM_CATEGORY,&guidItemCategory,NULL,TRUE);
        if (SUCCEEDED(hr))
        {
            //
            //  Check what kind of data transfer is requested.  This driver
            //  supports 2 transfer modes:
            //  1.  Stream-based download
            //  2.  Stream-based upload
            //

            if (lFlags & WIA_MINIDRV_TRANSFER_DOWNLOAD)
            {
                // This is stream-based download
                IWiaMiniDrvTransferCallback *pTransferCallback = NULL;
                hr = GetTransferCallback(pmdtc, &pTransferCallback);
                if (SUCCEEDED(hr))
                {
                    LONG lStreamsToDownload = 0;
                    LONG lStreamCount = 0;

                    if (!IsEqualGUID(guidItemCategory, WIA_CATEGORY_FEEDER))
                    {
                        lStreamsToDownload = 1;
                    }
                    else
                    {
                        hr = wiasReadPropLong(pWiasContext, WIA_IPS_PAGES, &lStreamsToDownload, NULL, TRUE);
                        if (FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "drvAcquireItemData: failure reading WIA_IPS_PAGES property for Feeder item, hr = 0x%lx", hr));
                        }
                        else if (ALL_PAGES == lStreamsToDownload)
                        {
                            //
                            // When WIA_IPS_PAGES is set to 0 (ALL_PAGES) meaning "scan as many documents
                            // as there may be loaded into the feeder"
                            // We assume 5 pages are in feeder
                            //
                            lStreamsToDownload = 5;
                        }
                    }

                    //
                    // We support only TYMED_FILE for WIA_IPA_TYMED so we should call DownloadToStream
                    // for each individual image transfer. If WIA_IPA_TYMED would support and would be
                    // set to TYMED_MULTIPAGE_FILE then all images acquired in a continous sequence should
                    // be transferred to the same strem (GetNextStream called just once):
                    //
                    while ((SUCCEEDED(hr)) && (lStreamCount < lStreamsToDownload))
                    {
                        //
                        // DownloadToStream writes its own trace message in case of failure:
                        //
                        hr = DownloadToStream(lFlags, pWiasContext, pmdtc, guidItemCategory, guidFormatID, pTransferCallback, plDevErrVal);
                        lStreamCount++;
                    }

                    pTransferCallback->Release();
                    pTransferCallback = NULL;
                }
                else
                {
                    WIAS_ERROR((g_hInst, "Could not get our IWiaMiniDrvTransferCallback for download"));
                }
            }
            else if (lFlags & WIA_MINIDRV_TRANSFER_UPLOAD)
            {
                //
                // We only want to do "Upload" if category of the item is WIA_CATEGORY_FINISHED_FILE and it is not the root storage item:
                //
                LONG    lItemType = 0;

                hr = wiasGetItemType(pWiasContext,&lItemType);
                if (SUCCEEDED(hr))
                {
                    if ((guidItemCategory == WIA_CATEGORY_FINISHED_FILE) && !(lItemType & WiaItemTypeStorage))
                    {
                        // This is stream-based upload
                        IWiaMiniDrvTransferCallback *pTransferCallback = NULL;
                        hr = GetTransferCallback(pmdtc, &pTransferCallback);
                        if (SUCCEEDED(hr))
                        {
                            hr = UploadFromStream(lFlags, pWiasContext, guidItemCategory, pTransferCallback, plDevErrVal);
                            pTransferCallback->Release();
                            pTransferCallback = NULL;
                        }
                        else
                        {
                            WIAS_ERROR((g_hInst, "Could not get our IWiaMiniDrvTransferCallback for upload"));
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        WIAS_ERROR((g_hInst, "Cannot do Upload to selected item, hr = 0x%lx",hr));
                    }
                }
                else
                {
                    WIAS_ERROR((g_hInst, "Failed to get the WIA item type, hr = 0x%lx",hr));
                }
            }
            else
            {
                // This should not happen!
                hr = E_INVALIDARG;
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to read the WIA_IPA_ITEM_CATEGORY property, hr = 0x%lx",hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}
HRESULT CWIADriver::drvInitItemProperties(_Inout_   BYTE *pWiasContext,
                                                    LONG lFlags,
                                          _Out_     LONG *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr          = E_INVALIDARG;
    LONG    lItemFlags  = 0;
    if((pWiasContext)&&(plDevErrVal))
    {
        *plDevErrVal = 0;

        //
        // Initialize individual storage item properties using the CWIAStorage object
        //
        hr = wiasReadPropLong(pWiasContext,WIA_IPA_ITEM_FLAGS,&lItemFlags,NULL,TRUE);
        if(SUCCEEDED(hr))
        {
            if((lItemFlags & WiaItemTypeRoot))
            {
                //
                // Add any root item properties needed.
                //
                hr = InitializeRootItemProperties(pWiasContext);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to initialize generic WIA root item properties, hr = 0x%lx",hr));
                }
            }
            else
            {
                //
                // Add any non-root item properties needed.
                //
                GUID guidItemCategory = GUID_NULL;

                //
                // Use the WIA category setting to determine what type of property
                // set should be created for this WIA item.
                //
                if((lItemFlags & WiaItemTypeGenerated) == FALSE)
                {
                    //
                    // Item is not a generated item, assume that this was created by this WIA driver
                    // and the WIA_ITEM_CATEGORY setting can be read from the WIA_DRIVER_ITEM_CONTEXT
                    // structure stored with the WIA driver item.
                    //

                    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
                    hr = wiasGetDriverItemPrivateContext(pWiasContext,(BYTE**)&pWiaDriverItemContext);
                    if(SUCCEEDED(hr) && (pWiaDriverItemContext))
                    {
                        guidItemCategory = pWiaDriverItemContext->guidItemCategory;
                    }
                    else
                    {
                        //
                        // This WIA item has no item context and will receive default item
                        // property initialization.  This allows applications to create child items
                        // for storing private data.
                        // NOTE: Data transfers on these types of items will probably not succeed since
                        //       the driver does not have a category to help classify the behavior of the
                        //       item.
                        //
                        hr = S_OK;
                    }
                }
                else
                {
                    //
                    // Read the parents WIA_ITEM_CATEGORY property setting to determine this new
                    // child item's category setting.
                    //

                    BYTE *pWiasParentContext = NULL;
                    hr = wiasGetAppItemParent(pWiasContext,&pWiasParentContext);
                    if(SUCCEEDED(hr))
                    {
                        hr = wiasReadPropGuid(pWiasParentContext,WIA_IPA_ITEM_CATEGORY,&guidItemCategory,NULL,TRUE);
                        if(FAILED(hr))
                        {
                            WIAS_TRACE((g_hInst,"The item does not have a category property setting. Assuming that it is unknown."));
                            //
                            // This WIA item has no item category property setting and will receive default item
                            // property initialization.  This allows applications to create child items
                            // for storing private data.
                            // NOTE: Data transfers on these types of items will probably not succeed since
                            //       the driver does not have a category to help classify the behavior of the
                            //       item.
                            //
                            hr = S_OK;
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "Failed to obtain the WIA application item's parent, hr = 0x%lx",hr));
                    }
                }

                if(SUCCEEDED(hr))
                {
                    //
                    // Initialize the WIA item property set according to the category specified
                    //

                    if(guidItemCategory == WIA_CATEGORY_FLATBED)
                    {
                        //
                        // We do not support folder items to be created under the flatbed item:
                        //
                        if ((lItemFlags & WiaItemTypeFolder) && (lItemFlags & WiaItemTypeGenerated))
                        {
                            //
                            // This is a folder item to be created under the base flatbed item, deny the request:
                            //
                            hr = E_INVALIDARG;
                        }

                        if(SUCCEEDED(hr))
                        {
                            hr = InitializeWIAItemProperties(pWiasContext,g_hInst,IDB_FLATBED);
                        }
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to initialize the flatbed item's property set. hr = 0x%lx",hr));
                        }
                    }
                    else if(guidItemCategory == WIA_CATEGORY_FEEDER)
                    {
                        hr = InitializeWIAItemProperties(pWiasContext,g_hInst,IDB_FEEDER);
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to initialize the feeder item's property set. hr = 0x%lx",hr));
                        }
                    }
                    else if(guidItemCategory == WIA_CATEGORY_FILM)
                    {
                        hr = InitializeWIAItemProperties(pWiasContext,g_hInst,IDB_FILM);
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to initialize the film item's property set. hr = 0x%lx",hr));
                        }
                    }
                    else if((guidItemCategory == WIA_CATEGORY_FINISHED_FILE) || (WIA_CATEGORY_FOLDER == guidItemCategory))
                    {
                        hr = InitializeWIAStorageItemProperties(pWiasContext, FALSE,
                            (BOOL)((WIA_CATEGORY_FOLDER == guidItemCategory) && (lItemFlags & WiaItemTypeFolder)));

                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to initialize the storage item's property set. hr = 0x%lx",hr));
                        }
                    }
                    else
                    {
                        hr = S_OK;
                    }
                }
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to read WIA_IPA_ITEM_FLAGS property, hr = 0x%lx",hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }

    if ((FAILED(hr)) && (plDevErrVal))
    {
        *plDevErrVal = (E_INVALIDARG == hr) ? WIA_ERROR_INVALID_COMMAND : WIA_ERROR_GENERAL_ERROR;
    }

    return hr;
}
HRESULT CWIADriver::drvValidateItemProperties(_Inout_   BYTE           *pWiasContext,
                                                        LONG           lFlags,
                                                        ULONG          nPropSpec,
                                              _In_      const PROPSPEC *pPropSpec,
                                              _Out_     LONG           *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = E_INVALIDARG;
    if((pWiasContext)&&(pPropSpec)&&(plDevErrVal)&&(nPropSpec))
    {
        *plDevErrVal = 0;

        LONG lItemType = 0;
        hr = wiasGetItemType(pWiasContext,&lItemType);
        if(SUCCEEDED(hr))
        {
            if(lItemType & WiaItemTypeRoot)
            {
                //
                // Validate root item property settings, if needed.
                //

                hr = S_OK;
            }
            else
            {
                //
                // Validate child item property settings, if needed.
                //
                LONG                    lScanningSurfaceWidth   = 0;
                LONG                    lScanningSurfaceHeight  = 0;
                GUID                    guidItemCategory        = GUID_NULL;
                GUID                    guidFormat              = GUID_NULL;
                WIA_PROPERTY_CONTEXT    PropertyContext         = {0};
                BOOL                    bUpdateFileExt          = FALSE;

                //
                // Use the WIA item category to help classify and gather WIA item information
                // needed to validate the property set.
                //
                hr = wiasReadPropGuid(pWiasContext,WIA_IPA_ITEM_CATEGORY,&guidItemCategory,NULL,TRUE);
                if(SUCCEEDED(hr))
                {
                    BOOL bValidCategory = FALSE;
                    //
                    // Validate the selection area against the entire scanning surface of the device.
                    // The scanning surface may be different sizes depending on the type of WIA item.
                    // (ie. Flatbed glass platen sizes may be different to film scanning surfaces, and
                    // feeder sizes.)
                    //
                    if((guidItemCategory == WIA_CATEGORY_FLATBED)||(guidItemCategory == WIA_CATEGORY_FILM))
                    {
                        bValidCategory = TRUE;

                        //
                        // Flatbed items and Film items use the same WIA properties to describe their scanning
                        // surface.
                        //
                        hr = wiasReadPropLong(pWiasContext,WIA_IPS_MAX_HORIZONTAL_SIZE,&lScanningSurfaceWidth,NULL,TRUE);
                        if(SUCCEEDED(hr))
                        {
                            hr = wiasReadPropLong(pWiasContext,WIA_IPS_MAX_VERTICAL_SIZE,&lScanningSurfaceHeight,NULL,TRUE);
                            if(FAILED(hr))
                            {
                                WIAS_ERROR((g_hInst, "Failed to read WIA_IPS_MAX_VERTICAL_SIZE property, hr = 0x%lx",hr));
                            }
                        }
                        else
                        {
                            WIAS_ERROR((g_hInst, "Failed to read WIA_IPS_MAX_HORIZONTAL_SIZE property, hr = 0x%lx",hr));
                        }
                    }
                    else if(guidItemCategory == WIA_CATEGORY_FEEDER)
                    {
                        bValidCategory = TRUE;

                        //
                        // Feeder items use a different set of properties to describe the scanning surface.
                        //
                        hr = wiasReadPropLong(pWiasContext,WIA_IPS_MAX_HORIZONTAL_SIZE,&lScanningSurfaceWidth,NULL,TRUE);
                        if(SUCCEEDED(hr))
                        {
                            hr = wiasReadPropLong(pWiasContext,WIA_IPS_MAX_VERTICAL_SIZE,&lScanningSurfaceHeight,NULL,TRUE);
                            if(FAILED(hr))
                            {
                                WIAS_ERROR((g_hInst, "Failed to read WIA_IPS_MAX_VERTICAL_SIZE property, hr = 0x%lx",hr));
                            }
                        }
                        else
                        {
                            WIAS_ERROR((g_hInst, "Failed to read WIA_IPS_MAX_HORIZONTAL_SIZE property, hr = 0x%lx",hr));
                        }
                    }
                    else if((guidItemCategory == WIA_CATEGORY_FINISHED_FILE) || (WIA_CATEGORY_FOLDER == guidItemCategory))
                    {
                        bValidCategory = TRUE;
                        hr = S_OK;
                    }
                    else
                    {
                        hr = S_OK;
                        WIAS_TRACE((g_hInst,"Unknown WIA category read from WIA item, hr = 0x%lx",hr));
                    }

                    if((SUCCEEDED(hr))&&(bValidCategory))
                    {
                        hr = wiasCreatePropContext(nPropSpec,(PROPSPEC*)pPropSpec,0,NULL,&PropertyContext);
                        if(SUCCEEDED(hr))
                        {
                            //
                            // Only perform extent validation for items that contain extent properties.
                            //

                            if((guidItemCategory != WIA_CATEGORY_FINISHED_FILE) && (WIA_CATEGORY_FOLDER != guidItemCategory))
                            {
                                if(SUCCEEDED(hr))
                                {
                                    hr = wiasUpdateValidFormat(pWiasContext,&PropertyContext,(IWiaMiniDrv*)this);
                                    if(FAILED(hr))
                                    {
                                        WIAS_ERROR((g_hInst, "Failed to validate supported formats, hr = %lx",hr));
                                    }
                                }


                                if(SUCCEEDED(hr))
                                {
                                    hr = wiasUpdateScanRect(pWiasContext,&PropertyContext,lScanningSurfaceWidth, lScanningSurfaceHeight);
                                    if(FAILED(hr))
                                    {
                                        WIAS_ERROR((g_hInst, "Failed to validate extent settings. (current selection area), hr = %lx",hr));
                                    }
                                }
                            }

                            HRESULT FreePropContextHR = wiasFreePropContext(&PropertyContext);
                            if(FAILED(FreePropContextHR))
                            {
                                WIAS_ERROR((g_hInst, "wiasFreePropContext failed, hr = 0x%lx",FreePropContextHR));
                            }
                        }
                        else
                        {
                            WIAS_ERROR((g_hInst, "Failed to create WIA property context for validation, hr = 0x%lx",hr));
                        }
                    }
                }

                if (SUCCEEDED(hr) && (guidItemCategory != WIA_CATEGORY_FINISHED_FILE) && (WIA_CATEGORY_FOLDER != guidItemCategory))
                {
                    //
                    // We have several properties dependent on the current image transfer format
                    // (currently this sample supports WiaImgFmt_BMP and WiaImgFmt_RAW):
                    //
                    BOOL bRawFormat = FALSE;
                    hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidFormat, NULL, TRUE);
                    if (SUCCEEDED(hr))
                    {
                        bRawFormat = (BOOL)IsEqualGUID(guidFormat, WiaImgFmt_RAW);
                    }

                    //
                    // Update format dependent properties according with the current WIA_IPA_FORMAT value:
                    //
                    BSTR bstrFileExtension = NULL;
                    if(SUCCEEDED(hr))
                    {
                        //
                        // Read the current WIA_IPA_FILENAME_EXTENSION and see if a change is needed:
                        //
                        hr = wiasReadPropStr(pWiasContext, WIA_IPA_FILENAME_EXTENSION, &bstrFileExtension, NULL, TRUE);
                        if(SUCCEEDED(hr))
                        {
                            bUpdateFileExt = (BOOL)(((IsEqualGUID(guidFormat, WiaImgFmt_RAW)) && (wcscmp(bstrFileExtension, TEXT("RAW")))) ||
                                ((IsEqualGUID(guidFormat, WiaImgFmt_BMP)) && (wcscmp(bstrFileExtension, TEXT("BMP")))));
                            SysFreeString(bstrFileExtension);
                            bstrFileExtension = NULL;

                            if(bUpdateFileExt)
                            {
                                if(SUCCEEDED(hr))
                                {
                                    if(bRawFormat)
                                    {
                                        bstrFileExtension = SysAllocString(L"RAW");
                                    }
                                    else
                                    {
                                        bstrFileExtension = SysAllocString(L"BMP");
                                    }

                                    if(bstrFileExtension)
                                    {
                                        hr = wiasWritePropStr(pWiasContext, WIA_IPA_FILENAME_EXTENSION, bstrFileExtension);

                                        SysFreeString(bstrFileExtension);
                                        bstrFileExtension = NULL;

                                        if(FAILED(hr))
                                        {
                                            WIAS_ERROR((g_hInst, "Could not update the file name extension property value, hr = 0x%lx.", hr));
                                        }
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                        WIAS_ERROR((g_hInst, "Could not allocate the file name extension property value, hr = 0x%lx.", hr));
                                    }
                                }
                                else
                                {
                                    WIAS_ERROR((g_hInst, "Cannot remove current file name extension property, hr = 0x%lx.", hr));
                                }
                            }
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "Cannot read current format property, hr = 0x%lx.", hr));
                    }
                }
           }

            //
            // Only call wiasValidateItemProperties if the validation above
            // succeeded.
            //
            if(SUCCEEDED(hr))
            {
                hr = wiasValidateItemProperties(pWiasContext,nPropSpec,pPropSpec);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to validate remaining properties using wiasValidateItemProperties, hr = 0x%lx",hr));
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::drvWriteItemProperties(_Inout_  BYTE                      *pWiasContext,
                                                    LONG                      lFlags,
                                           _In_     PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                           _Out_    LONG                      *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = E_INVALIDARG;
    if ((pWiasContext) && (pmdtc) && (plDevErrVal))
    {
        *plDevErrVal = 0;

        //
        // We have to reset the counter from time to time to allow other
        // acquisitions from the feeder item without to reload the Monster
        // driver and open a new session - we can do this when a property
        // is changed on the Feeder item.
        //
        // (in the case of a driver deserving a real scanner device such a counter
        // would have to be replaced with feeder status checking).
        //
        WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
        hr = wiasGetDriverItemPrivateContext(pWiasContext, (BYTE**)&pWiaDriverItemContext);
        if ((SUCCEEDED(hr)) && (pWiaDriverItemContext))
        {
            pWiaDriverItemContext->ulFeederTransferCount = 0;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}
HRESULT CWIADriver::drvReadItemProperties(_In_      BYTE           *pWiasContext,
                                                    LONG           lFlags,
                                                    ULONG          nPropSpec,
                                          _In_      const PROPSPEC *pPropSpec,
                                          _Out_     LONG           *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(nPropSpec);

    HRESULT hr = E_INVALIDARG;
    if((pWiasContext)&&(pPropSpec)&&(plDevErrVal))
    {
        *plDevErrVal = 0;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}
HRESULT CWIADriver::drvLockWiaDevice(_In_       BYTE *pWiasContext,
                                                LONG lFlags,
                                     _Out_      LONG *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = E_INVALIDARG;
    if((pWiasContext)&&(plDevErrVal))
    {
        *plDevErrVal = 0;

        if(m_pIStiDevice)
        {
            hr = m_pIStiDevice->LockDevice(DEFAULT_LOCK_TIMEOUT);
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::drvUnLockWiaDevice(_In_     BYTE *pWiasContext,
                                                LONG lFlags,
                                       _Out_    LONG *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = E_INVALIDARG;
    if((pWiasContext)&&(plDevErrVal))
    {
        *plDevErrVal = 0;

        if(m_pIStiDevice)
        {
            hr = m_pIStiDevice->UnLockDevice();
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::drvAnalyzeItem(_In_     BYTE *pWiasContext,
                                            LONG lFlags,
                                   _Out_    LONG *plDevErrVal)
{
    UNREFERENCED_PARAMETER(pWiasContext);
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(plDevErrVal);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::drvGetDeviceErrorStr(         LONG     lFlags,
                                                  LONG     lDevErrVal,
                                         _Out_    LPOLESTR *ppszDevErrStr,
                                         _Out_    LONG     *plDevErr)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(lDevErrVal);
    UNREFERENCED_PARAMETER(ppszDevErrStr);
    UNREFERENCED_PARAMETER(plDevErr);

    WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    return E_NOTIMPL;
}
HRESULT CWIADriver::DestroyDriverItemTree()
{
    HRESULT hr = S_OK;

    if(m_pIDrvItemRoot)
    {
        WIAS_TRACE((g_hInst,"Unlinking WIA item tree"));
        hr = m_pIDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);
        if(FAILED(hr))
        {
            WIAS_ERROR((g_hInst, "Failed to unlink WIA item tree before being released, hr = 0x%lx",hr));
        }

        WIAS_TRACE((g_hInst,"Releasing IDrvItemRoot interface"));
        m_pIDrvItemRoot->Release();
        m_pIDrvItemRoot = NULL;
    }

    return hr;
}

HRESULT CWIADriver::BuildDriverItemTree()
{
    HRESULT hr = S_OK;
    if(!m_pIDrvItemRoot)
    {
        LONG lItemFlags = WiaItemTypeFolder | WiaItemTypeDevice | WiaItemTypeRoot;
        BSTR bstrRootItemName = SysAllocString(WIA_DRIVER_ROOT_NAME);
        if(bstrRootItemName)
        {
            //
            // Create a default WIA root item
            //
            hr = wiasCreateDrvItem(lItemFlags,
                                   bstrRootItemName,
                                   m_bstrRootFullItemName,
                                   (IWiaMiniDrv*)this,
                                   0,
                                   NULL,
                                   &m_pIDrvItemRoot);
            //
            // Create child items that represent the data or programmable data sources.
            //
            if(SUCCEEDED(hr))
            {
                hr = CreateWIAFlatbedItem(WIA_DRIVER_FLATBED_NAME,(IWiaMiniDrv*)this,m_pIDrvItemRoot);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to create WIA flatbed item, hr = 0x%lx",hr));
                }
            }

            if(SUCCEEDED(hr))
            {
                hr = CreateWIAFeederItem(WIA_DRIVER_FEEDER_NAME,(IWiaMiniDrv*)this,m_pIDrvItemRoot);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to create WIA feeder item, hr = 0x%lx",hr));
                }
            }

            if(SUCCEEDED(hr))
            {
                hr = CreateWIAFilmItem(WIA_DRIVER_FILM_NAME,(IWiaMiniDrv*)this,m_pIDrvItemRoot);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to create WIA film item, hr = 0x%lx",hr));
                }
            }

            if(SUCCEEDED(hr))
            {
                hr = CreateWIAStorageItem(WIA_DRIVER_STORAGE_NAME,(IWiaMiniDrv*)this,m_pIDrvItemRoot,m_wszStoragePath);
                if(FAILED(hr))
                {
                    WIAS_ERROR((g_hInst, "Failed to create WIA storage item, hr = 0x%lx",hr));
                }
            }

            SysFreeString(bstrRootItemName);
            bstrRootItemName = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate memory for the root item name, hr = 0x%lx",hr));
        }
    }

    return hr;
}


HRESULT CWIADriver::DoSynchronizeCommand(
    _Inout_ BYTE *pWiasContext)
{
    HRESULT hr = S_OK;

    hr = DestroyDriverItemTree();
    if (SUCCEEDED(hr))
    {
        hr = BuildDriverItemTree();

        //
        //  Queue tree updated event, regardless ofwhether it
        //  succeeded, since we can't guarantee that the tree
        //  was left in the same condition.
        //
        QueueWIAEvent(pWiasContext, WIA_EVENT_TREE_UPDATED);
    }
    else
    {
        WIAS_ERROR((g_hInst, " failed, hr = 0x%lx", hr));
    }

    return hr;
}

HRESULT CWIADriver::drvDeviceCommand(_Inout_  BYTE        *pWiasContext,
                                              LONG        lFlags,
                                     _In_     const GUID  *pguidCommand,
                                     _Out_    IWiaDrvItem **ppWiaDrvItem,
                                     _Out_    LONG        *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = E_NOTIMPL;

    if (ppWiaDrvItem)
    {
        *ppWiaDrvItem = NULL;
    }

    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    if (pguidCommand)
    {
        if (*pguidCommand == WIA_CMD_SYNCHRONIZE)
        {
            hr = DoSynchronizeCommand(pWiasContext);
        }
    }
    else
    {
        hr = E_NOTIMPL;
        WIAS_ERROR((g_hInst, "This method is not implemented or supported for this driver"));
    }

    return hr;
}

HRESULT CWIADriver::drvGetCapabilities(_In_   BYTE            *pWiasContext,
                                              LONG            ulFlags,
                                       _Out_  LONG            *pcelt,
                                       _Out_  WIA_DEV_CAP_DRV **ppCapabilities,
                                       _Out_  LONG            *plDevErrVal)
{
    UNREFERENCED_PARAMETER(pWiasContext);

    HRESULT hr = E_INVALIDARG;
    if((pcelt)&&(ppCapabilities)&&(plDevErrVal))
    {
        hr = S_OK;

        *pcelt = 0;
        *ppCapabilities = NULL;
        *plDevErrVal = 0;

        if(m_CapabilityManager.GetNumCapabilities() == 0)
        {
            hr = m_CapabilityManager.AddCapability(WIA_EVENT_DEVICE_CONNECTED,
                                                   IDS_EVENT_DEVICE_CONNECTED_NAME,
                                                   IDS_EVENT_DEVICE_CONNECTED_DESCRIPTION,
                                                   WIA_NOTIFICATION_EVENT,
                                                   WIA_ICON_DEVICE_CONNECTED);
            if(SUCCEEDED(hr))
            {
                hr = m_CapabilityManager.AddCapability(WIA_EVENT_TREE_UPDATED,
                                                       IDS_EVENT_TREE_UPDATED_NAME,
                                                       IDS_EVENT_TREE_UPDATED_DESCRIPTION,
                                                       WIA_NOTIFICATION_EVENT,
                                                       WIA_ICON_TREE_UPDATED);
                if(SUCCEEDED(hr))
                {
                    hr = m_CapabilityManager.AddCapability(WIA_EVENT_DEVICE_DISCONNECTED,
                                                           IDS_EVENT_DEVICE_DISCONNECTED_NAME,
                                                           IDS_EVENT_DEVICE_DISCONNECTED_DESCRIPTION,
                                                           WIA_NOTIFICATION_EVENT,
                                                           WIA_ICON_DEVICE_DISCONNECTED);
                    if(SUCCEEDED(hr))
                    {
                        hr = m_CapabilityManager.AddCapability(WIA_CMD_SYNCHRONIZE,
                                                               IDS_CMD_SYNCHRONIZE_NAME,
                                                               IDS_CMD_SYNCHRONIZE_DESCRIPTION,
                                                               0,
                                                               WIA_ICON_SYNCHRONIZE);
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "Failed to add WIA_CMD_SYNCHRONIZE to capability manager, hr = 0x%lx",hr));
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "Failed to add WIA_EVENT_DEVICE_DISCONNECTED to capability manager, hr = 0x%lx",hr));
                    }
                }
                else
                {
                    WIAS_ERROR((g_hInst, "Failed to add WIA_EVENT_TREE_UPDATED to capability manager, hr = 0x%lx",hr));
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to add WIA_EVENT_DEVICE_CONNECTED to capability manager, hr = 0x%lx",hr));
            }
        }

        if(SUCCEEDED(hr))
        {
            if(((ulFlags & WIA_DEVICE_COMMANDS) == WIA_DEVICE_COMMANDS)&&(ulFlags & WIA_DEVICE_EVENTS) == WIA_DEVICE_EVENTS)
            {
                *ppCapabilities = m_CapabilityManager.GetCapabilities();
                *pcelt          = m_CapabilityManager.GetNumCapabilities();
                WIAS_TRACE((g_hInst,"Application is asking for Commands and Events, and we have %d total capabilities",*pcelt));
            }
            else if((ulFlags & WIA_DEVICE_COMMANDS) == WIA_DEVICE_COMMANDS)
            {
                *ppCapabilities = m_CapabilityManager.GetCommands();
                *pcelt          = m_CapabilityManager.GetNumCommands();
                WIAS_TRACE((g_hInst,"Application is asking for Commands, and we have %d",*pcelt));
            }
            else if((ulFlags & WIA_DEVICE_EVENTS) == WIA_DEVICE_EVENTS)
            {
                *ppCapabilities = m_CapabilityManager.GetEvents();
                *pcelt          = m_CapabilityManager.GetNumEvents();
                WIAS_TRACE((g_hInst,"Application is asking for Events, and we have %d",*pcelt));
            }

            WIAS_TRACE((g_hInst,"========================================================"));
            WIAS_TRACE((g_hInst,"WIA driver capability information"));
            WIAS_TRACE((g_hInst,"========================================================"));

            WIA_DEV_CAP_DRV *pCapabilities      = m_CapabilityManager.GetCapabilities();
            LONG            lNumCapabilities    = m_CapabilityManager.GetNumCapabilities();

            for(LONG i = 0; i < lNumCapabilities; i++)
            {
                if(pCapabilities[i].ulFlags & WIA_NOTIFICATION_EVENT)
                {
                    WIAS_TRACE((g_hInst,"Event Name:        %ws",pCapabilities[i].wszName));
                    WIAS_TRACE((g_hInst,"Event Description: %ws",pCapabilities[i].wszDescription));
                }
                else
                {
                    WIAS_TRACE((g_hInst,"Command Name:        %ws",pCapabilities[i].wszName));
                    WIAS_TRACE((g_hInst,"Command Description: %ws",pCapabilities[i].wszDescription));
                }
            }
            WIAS_TRACE((g_hInst,"========================================================"));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::drvDeleteItem(_Inout_ BYTE *pWiasContext,
                                          LONG lFlags,
                                  _Out_   LONG *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = E_INVALIDARG;
    if((pWiasContext)&&(plDevErrVal))
    {
        *plDevErrVal = 0;

        GUID guidWiaItemCategory = GUID_NULL;
        hr = wiasReadPropGuid(pWiasContext,WIA_IPA_ITEM_CATEGORY,&guidWiaItemCategory,NULL,TRUE);
        if(SUCCEEDED(hr))
        {
            if(guidWiaItemCategory == WIA_CATEGORY_FINISHED_FILE)
            {
                WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;
                hr = wiasGetDriverItemPrivateContext(pWiasContext,(BYTE**)&pWiaDriverItemContext);
                if(SUCCEEDED(hr))
                {
                    DeleteFile(pWiaDriverItemContext->bstrStorageDataPath);
                }
                else
                {
                    //
                    // If the WIA item does not have a driver item context, then
                    // assume that there is no associated storage data with it.
                    //

                    hr = S_OK;
                }
            }
            else
            {
                //
                // If the WIA item is not of finished file category, then
                // assume that there is no associated storage data with it.
                //

                hr = S_OK;
            }
        }
        else
        {
            WIAS_ERROR((g_hInst, "Failed to read the WIA_IPA_ITEM_CATEGORY property, hr = 0x%lx",hr));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }

    //
    // Only queue the deleted event, if the deletion was a success
    //

    if(SUCCEEDED(hr))
    {
        QueueWIAEvent(pWiasContext,WIA_EVENT_ITEM_DELETED);
    }

    return hr;
}
HRESULT CWIADriver::drvFreeDrvItemContext(
                                                    LONG  lFlags,
    _Inout_updates_bytes_(sizeof(WIA_DRIVER_ITEM_CONTEXT)) BYTE *pSpecContext,
    _Out_                                           LONG *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    if (plDevErrVal)
    {
        *plDevErrVal = NULL;
    }

    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = (WIA_DRIVER_ITEM_CONTEXT*)pSpecContext;
    if(pWiaDriverItemContext)
    {
        // Free allocated BSTR if it exists.
        if(pWiaDriverItemContext->bstrStorageDataPath)
        {
            SysFreeString(pWiaDriverItemContext->bstrStorageDataPath);
            pWiaDriverItemContext->bstrStorageDataPath = NULL;
        }
    }

    return S_OK;
}

HRESULT CWIADriver::drvGetWiaFormatInfo(_In_  BYTE            *pWiasContext,
                                              LONG            lFlags,
                                        _Out_ LONG            *pcelt,
                                        _Out_ WIA_FORMAT_INFO **ppwfi,
                                        _Out_ LONG            *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;

    if ((!plDevErrVal) || (!pcelt) || (!ppwfi))
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;

        if (m_pFormats)
        {
            delete [] m_pFormats;
            m_pFormats = NULL;
        }

        m_ulNumFormats = DEFAULT_NUM_DRIVER_FORMATS;

        CBasicDynamicArray<GUID> FileFormats;

        //
        // add the default formats to the corresponding arrays
        //
        if (pWiasContext)
        {
            //
            // Create a format list that is specific to the WIA item.
            //
            LONG lItemType = 0;
            hr = wiasGetItemType(pWiasContext, &lItemType);
            if (SUCCEEDED(hr))
            {
                if (lItemType & WiaItemTypeImage)
                {
                    FileFormats.Append(WiaImgFmt_BMP);
                    FileFormats.Append(WiaImgFmt_RAW);
                }
                else
                {
                    FileFormats.Append(WiaImgFmt_UNDEFINED);
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "Failed to get WIA item type, hr = 0x%lx",hr));
            }
        }
        else
        {
            //
            // Create a default format list
            //
            // For this sample driver we are assuming that the majority of data
            // transferred will be image data, so when a query for formats fails,
            // it is safe to default to DIB and Raw as the formats.
            //
            FileFormats.Append(WiaImgFmt_BMP);
            FileFormats.Append(WiaImgFmt_RAW);
        }

        *pcelt = 0;
        *ppwfi = NULL;

        if (SUCCEEDED(hr))
        {
            m_ulNumFormats = FileFormats.Size();
            m_pFormats = new WIA_FORMAT_INFO[m_ulNumFormats];
            if (m_pFormats)
            {
                //
                // add file (TYMED_FILE) formats to format array
                //
                for (ULONG iIndex = 0; iIndex < m_ulNumFormats; iIndex++)
                {
                    m_pFormats[iIndex].guidFormatID = FileFormats[iIndex];
                    m_pFormats[iIndex].lTymed       = TYMED_FILE;
                }

                *pcelt = m_ulNumFormats;
                *ppwfi = &m_pFormats[0];
            }
            else
            {
                hr = E_OUTOFMEMORY;
                WIAS_ERROR((g_hInst, "Failed to allocate memory for WIA_FORMAT_INFO structure array, hr = 0x%lx",hr));

                m_ulNumFormats = 0;
            }
        }
    }

    return hr;
}

HRESULT CWIADriver::drvNotifyPnpEvent(_In_      const GUID *pEventGUID,
                                      _In_      BSTR       bstrDeviceID,
                                                ULONG      ulReserved)
{
    UNREFERENCED_PARAMETER(bstrDeviceID);
    UNREFERENCED_PARAMETER(ulReserved);

    HRESULT hr = E_INVALIDARG;
    if(pEventGUID)
    {
        // TBD: Add any special event handling here.
        //      Power management, canceling pending I/O etc.
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

HRESULT CWIADriver::drvUnInitializeWia(_Inout_ BYTE *pWiasContext)
{
    HRESULT hr = E_INVALIDARG;
    if(pWiasContext)
    {
        if(InterlockedDecrement(&m_lClientsConnected) < 0)
        {
            WIAS_TRACE((g_hInst, "The client connection counter decremented below zero. Assuming no clients are currently connected and automatically setting to 0"));
            m_lClientsConnected = 0;
        }

        WIAS_TRACE((g_hInst,"%d client(s) are currently connected to this driver.",m_lClientsConnected));

        if(m_lClientsConnected == 0)
        {

            //
            // When the last client disconnects, destroy the WIA item tree.
            // This should reduce the idle memory foot print of this driver
            //

            DestroyDriverItemTree();
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIAS_ERROR((g_hInst, "Invalid parameters were passed, hr = 0x%lx",hr));
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// INonDelegating Interface Section (for all WIA drivers)              //
/////////////////////////////////////////////////////////////////////////

HRESULT CWIADriver::NonDelegatingQueryInterface(REFIID  riid,LPVOID  *ppvObj)
{
    if(!ppvObj)
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
        return E_INVALIDARG;
    }

    *ppvObj = NULL;

    if(IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObj = static_cast<INonDelegatingUnknown*>(this);
    }
    else if(IsEqualIID( riid, IID_IStiUSD ))
    {
        *ppvObj = static_cast<IStiUSD*>(this);
    }
    else if(IsEqualIID( riid, IID_IWiaMiniDrv ))
    {
        *ppvObj = static_cast<IWiaMiniDrv*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppvObj)->AddRef();
    return S_OK;
}

ULONG CWIADriver::NonDelegatingAddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CWIADriver::NonDelegatingRelease()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if(ulRef == 0)
    {
        delete this;
        return 0;
    }
    return ulRef;
}

/////////////////////////////////////////////////////////////////////////
// IClassFactory Interface Section (for all COM objects)               //
/////////////////////////////////////////////////////////////////////////

class CWIADriverClassFactory : public IClassFactory
{
public:
    CWIADriverClassFactory() : m_cRef(1) {}
    ~CWIADriverClassFactory(){}
    HRESULT __stdcall QueryInterface(REFIID riid, _COM_Outptr_ LPVOID *ppv)
    {
        if(!ppv)
        {
            WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
            return E_INVALIDARG;
        }

        *ppv = NULL;
        HRESULT hr = E_NOINTERFACE;
        if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
        {
            *ppv = static_cast<IClassFactory*>(this);
            reinterpret_cast<IUnknown*>(*ppv)->AddRef();
            hr = S_OK;
        }
        return hr;
    }
    ULONG __stdcall AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }
    ULONG __stdcall Release()
    {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if(ulRef == 0)
        {
            delete this;
            return 0;
        }
        return ulRef;
    }
#pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_2, "Set ppvObject to NULL if failed.")
    HRESULT __stdcall CreateInstance(_In_opt_ IUnknown* pUnkOuter, _In_ REFIID riid, _COM_Outptr_ void** ppvObject)
    {
        if (ppvObject == NULL)
        {
            return E_INVALIDARG;
        }
        *ppvObject = NULL;

        if((pUnkOuter)&&(!IsEqualIID(riid,IID_IUnknown)))
        {
            return CLASS_E_NOAGGREGATION;
        }

        HRESULT hr = E_NOINTERFACE;
#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pDev is freed on release.")
        CWIADriver *pDev = new CWIADriver(pUnkOuter);
        if(pDev)
        {
            hr = pDev->NonDelegatingQueryInterface(riid,ppvObject);
            pDev->NonDelegatingRelease();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate WIA driver class object, hr = 0x%lx",hr));
        }

        return hr;
    }
    HRESULT __stdcall LockServer(BOOL fLock)
    {
        UNREFERENCED_PARAMETER(fLock);

        return S_OK;
    }
private:
    LONG m_cRef;
};

/////////////////////////////////////////////////////////////////////////
// DLL Entry Point Section (for all COM objects, in a DLL)             //
/////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hinst,DWORD dwReason, _Reserved_ LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    g_hInst = hinst;
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        break;
    }
    return TRUE;
}

extern "C" HRESULT __stdcall DllCanUnloadNow(void)
{
    return S_OK;
}
extern "C" HRESULT __stdcall DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID *ppv)
{
    if(!ppv)
    {
        WIAS_ERROR((g_hInst, "Invalid parameters were passed"));
        return E_INVALIDARG;
    }
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;
    if(IsEqualCLSID(rclsid, CLSID_WIADriver))
    {
#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pcf is freed on release.")
        CWIADriverClassFactory *pcf = new CWIADriverClassFactory;
        if(pcf)
        {
            hr = pcf->QueryInterface(riid,ppv);
            pcf->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            WIAS_ERROR((g_hInst, "Failed to allocate WIA driver class factory object, hr = 0x%lx",hr));
        }
    }
    return hr;
}

extern "C" HRESULT __stdcall DllRegisterServer()
{
    return S_OK;
}

extern "C" HRESULT __stdcall DllUnregisterServer()
{
    return S_OK;
}

/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  MiniDrv.h
*
*  Project:     Production Scanner Driver Sample
*
*  Description: Contains the declaration for the CWiaDriver class
*               implementing the main driver object
*
***************************************************************************/

#pragma once

extern HINSTANCE g_hInst;

//
// WIA mini-driver item context data, customized for this sample driver:
//
typedef struct _WIA_DRIVER_ITEM_CONTEXT
{
    //
    // In-memory stream holding an images optionally uploaded by
    // the WIA application to the Imprinter or the Endorser item.
    // If such images are not uploaded by the application in the
    // current WIA app session, when a image download is requested
    // from one of these items, the driver loads its test image
    // from resources:
    //
    IStream* m_pUploadedImage;

} WIA_DRIVER_ITEM_CONTEXT, *PWIA_DRIVER_ITEM_CONTEXT;

//
// INonDelegatingUnknown
//

class INonDelegatingUnknown
{
public:
    virtual STDMETHODIMP
    NonDelegatingQueryInterface(REFIID riid,LPVOID *ppvObj) = 0;

    virtual STDMETHODIMP_(ULONG)
    NonDelegatingAddRef() = 0;

    virtual STDMETHODIMP_(ULONG)
    NonDelegatingRelease() = 0;
};

//
// CWiaDriver class implementing INonDelegatingUnknown,
// IStiUSD (STI USD interface) and IWiaMiniDrv (WIA mini-driver interface):
//

class CWiaDriver :
    public INonDelegatingUnknown,
    public IStiUSD,
    public IWiaMiniDrv
{
public:

    //
    // Construction/Destruction Section
    //

    CWiaDriver(
        _In_opt_ LPUNKNOWN punkOuter);

    ~CWiaDriver();

private:

    //
    // WIA driver internals
    //

    LONG         m_cRef;                 // Device object reference count.
    LPUNKNOWN    m_punkOuter;            // Pointer to outer unknown.
    IStiDevice*  m_pIStiDevice;          // STI device interface
    IWiaDrvItem* m_pIDrvItemRoot;        // WIA root item
    IWiaLog*     m_pIWiaLog;             // WIA logging object
    LONG         m_lClientsConnected;    // number of applications connected
    BSTR         m_bstrDeviceID;         // WIA device ID
    BSTR         m_bstrRootFullItemName; // WIA root item (full item name)
    HRESULT      m_hrLastEdviceError;    // Used for IStiUSD::GetLastError

    //
    // Device path received by IStiUSD::Initialize, recorded for late initialization:
    //
    WCHAR m_wszDevicePath[MAX_PATH];

    //
    // Device Registry key path received by IStiUSD::Initialize, recorded for late initialization:
    //
    HKEY m_hDeviceKey;

    //
    // Supported transfer file format - tymed combinations:
    //
    CBasicDynamicArray<WIA_FORMAT_INFO> m_tFormatInfo;
    CBasicDynamicArray<WIA_FORMAT_INFO> m_tFormatInfoImprinterEndorser;
    CBasicDynamicArray<WIA_FORMAT_INFO> m_tFormatInfoBarcodeReader;
    CBasicDynamicArray<WIA_FORMAT_INFO> m_tFormatInfoPatchCodeReader;
    CBasicDynamicArray<WIA_FORMAT_INFO> m_tFormatInfoMicrReader;

    //
    // Driver capabilities (events and commands):
    //
    CWIACapabilityManager m_tCapabilityManager;

    //
    // WIA event handle to signal to the WIA service
    // when notifications from scanner device arrive.
    // Currently used for scan events only:
    //
    HANDLE m_hWiaEvent;
    HANDLE m_hWiaEventStoredCopy;

    //
    // The last input source signaled for an unconsumed
    // ScanAvailable event, if any, or an empty string
    // otherwise (scanner not in scan available state
    // -or- scan available input source unknown).
    // When set the value is the format of a WIA item
    // name (as reported by WIA_IPA_ITEM_NAME):
    //
    BSTR m_bstrScanAvailableItem;

    //
    // Flag indicating if driver initialization performed
    // during IStiUSD::Initialize is complete:
    //
    BOOL m_bInitialized;

    //
    // Critical section to prevent race conditions when calling
    // DestroyDriverItemTree from multiple threads (for example
    // when the device is disconnected at the same time as the
    // only WIA application accessing the driver ends its WIA
    // session releasing its Application Item Tree root item):
    //
    CRITICAL_SECTION m_csDestroyDriverItemTree;

public:

    //
    // IUnknown methods:
    //

    STDMETHODIMP
    QueryInterface(
                        REFIID  riid,
        _COM_Outptr_    LPVOID *ppvObj);

    STDMETHODIMP_(ULONG)
    AddRef();

    STDMETHODIMP_(ULONG)
    Release();

    //
    // IStiUSD methods:
    //

    STDMETHOD(Initialize)(THIS_
        _In_  PSTIDEVICECONTROL pIStiDevControl,
              DWORD             dwStiVersion,
        _In_  HKEY              hParametersKey);

    STDMETHOD(GetCapabilities)(THIS_
        _Out_ PSTI_USD_CAPS pDevCaps);

    STDMETHOD(GetStatus)(THIS_
        _Inout_ PSTI_DEVICE_STATUS pDevStatus);

    STDMETHOD(DeviceReset)(THIS);

    STDMETHOD(Diagnostic)(THIS_
        _Inout_ LPDIAG pBuffer);

    STDMETHOD(Escape)(THIS_
        STI_RAW_CONTROL_CODE EscapeFunction,
        _In_reads_bytes_(cbInDataSize) LPVOID lpInData,
        DWORD cbInDataSize,
        _Out_writes_bytes_(cbOutDataSize) LPVOID pOutData,
        DWORD cbOutDataSize,
        _Out_ LPDWORD pdwActualData);

    STDMETHOD(GetLastError)(THIS_
        _Out_ LPDWORD pdwLastDeviceError);

    STDMETHOD(LockDevice)();

    STDMETHOD(UnLockDevice)();

    STDMETHOD(RawReadData)(THIS_
        _Out_writes_bytes_(*lpdwNumberOfBytes) LPVOID lpBuffer,
        _Inout_ LPDWORD lpdwNumberOfBytes,
        _In_opt_ LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawWriteData)(THIS_
        _In_reads_bytes_(dwNumberOfBytes) LPVOID lpBuffer,
        DWORD dwNumberOfBytes,
        _In_opt_ LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawReadCommand)(THIS_
        _Out_writes_bytes_(*lpdwNumberOfBytes) LPVOID lpBuffer,
        _Inout_ LPDWORD lpdwNumberOfBytes,
        _In_opt_ LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawWriteCommand)(THIS_
        _In_reads_bytes_(dwNumberOfBytes) LPVOID lpBuffer,
        DWORD dwNumberOfBytes,
        _In_opt_ LPOVERLAPPED lpOverlapped);

    STDMETHOD(SetNotificationHandle)(THIS_
        _In_opt_ HANDLE hEvent);

    STDMETHOD(GetNotificationData)(THIS_
        _Out_ LPSTINOTIFY lpNotify);

    STDMETHOD(GetLastErrorInfo)(THIS_
        _Out_ STI_ERROR_INFO *pLastErrorInfo);

    //
    // IWiaMiniDrv methods:
    //

    STDMETHOD(drvInitializeWia)(THIS_
        _Inout_   BYTE*         pWiasContext,
                  LONG          lFlags,
        _In_      BSTR          bstrDeviceID,
        _In_      BSTR          bstrRootFullItemName,
        _In_      IUnknown*     pStiDevice,
        _In_      IUnknown*     pIUnknownOuter,
        _Out_     IWiaDrvItem** ppIDrvItemRoot,
        _Out_     IUnknown**    ppIUnknownInner,
        _Out_     LONG*         plDevErrVal);

    STDMETHOD(drvAcquireItemData)(THIS_
        _In_  BYTE*                     pWiasContext,
              LONG                      lFlags,
        _In_  PMINIDRV_TRANSFER_CONTEXT pmdtc,
        _Out_ LONG*                     plDevErrVal);

    STDMETHOD(drvInitItemProperties)(THIS_
        _Inout_ BYTE* pWiasContext,
                LONG  lFlags,
        _Out_   LONG* plDevErrVal);

    STDMETHOD(drvValidateItemProperties)(THIS_
        _Inout_ BYTE*          pWiasContext,
                LONG           lFlags,
                ULONG          nPropSpec,
        _In_reads_(nPropSpec)
                const PROPSPEC *pPropSpec,
        _Out_   LONG*          plDevErrVal);

    STDMETHOD(drvWriteItemProperties)(THIS_
        _Inout_  BYTE*                     pWiasContext,
                 LONG                      lFlags,
        _In_     PMINIDRV_TRANSFER_CONTEXT pmdtc,
        _Out_    LONG*                     plDevErrVal);

    STDMETHOD(drvReadItemProperties)(THIS_
        _In_ BYTE* pWiasContext,
        LONG lFlags,
        ULONG nPropSpec,
        _In_ const PROPSPEC* pPropSpec,
        _Out_ LONG* plDevErrVal);

    STDMETHOD(drvLockWiaDevice)(THIS_
        _In_      BYTE*  pWiasContext,
                  LONG   lFlags,
        _Out_     LONG*  plDevErrVal);

    STDMETHOD(drvUnLockWiaDevice)(THIS_
        _In_      BYTE* pWiasContext,
                  LONG  lFlags,
        _Out_     LONG* plDevErrVal);

    STDMETHOD(drvAnalyzeItem)(THIS_
        _In_      BYTE* pWiasContext,
                  LONG  lFlags,
        _Out_     LONG* plDevErrVal);

    STDMETHOD(drvGetDeviceErrorStr)(THIS_
              LONG lFlags,
              LONG  lDevErrVal,
        _Out_ LPOLESTR* ppszDevErrStr,
        _Out_ LONG* plDevErr);

    STDMETHOD(drvDeviceCommand)(THIS_
        _Inout_   BYTE*         pWiasContext,
                  LONG          lFlags,
        _In_      const GUID*   plCommand,
        _Out_     IWiaDrvItem** ppWiaDrvItem,
        _Out_     LONG*         plDevErrVal);

    STDMETHOD(drvGetCapabilities)(THIS_
        _In_opt_  BYTE*             pWiasContext,
                  LONG              ulFlags,
        _Out_     LONG*             pcelt,
        _Out_     WIA_DEV_CAP_DRV** ppCapabilities,
        _Out_     LONG*             plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
        _Inout_   BYTE* pWiasContext,
                  LONG  lFlags,
        _Out_     LONG* plDevErrVal);

    STDMETHOD(drvFreeDrvItemContext)(THIS_
                  LONG lFlags,
        _In_reads_bytes_(sizeof(WIA_DRIVER_ITEM_CONTEXT))
                  BYTE *pSpecContext,
        _Out_     LONG *plDevErrVal);

    STDMETHOD(drvGetWiaFormatInfo)(THIS_
        _In_      BYTE*             pWiasContext,
                  LONG              lFlags,
        _Out_     LONG*             pcelt,
        _Out_     WIA_FORMAT_INFO** ppwfi,
        _Out_     LONG*             plDevErrVal);

    STDMETHOD(drvNotifyPnpEvent)(THIS_
        _In_  const GUID* pEventGUID,
        _In_  BSTR        bstrDeviceID,
              ULONG       ulReserved);

    STDMETHOD(drvUnInitializeWia)(THIS_
        _Inout_ BYTE* pWiasContext);

public:

    //
    // INonDelegating Interface Section:
    //

    STDMETHODIMP
    NonDelegatingQueryInterface(
        REFIID riid,
        LPVOID* ppvObj);

    STDMETHODIMP_(ULONG)
    NonDelegatingAddRef();

    STDMETHODIMP_(ULONG)
    NonDelegatingRelease();

private:

    //
    // WIA_IPS_PAGE_SIZE valid values, kept in separate arrays for each orientation.
    // This sample driver supports a single standard page size and a single orientation
    // (portrait) but a real device driver should use all standard page sizes that
    // fit into the available physical scan document dimensions for the feeder:
    //
    CBasicDynamicArray<LONG> m_lPortraitSizesArray;
    CBasicDynamicArray<LONG> m_lLandscapeSizesArray;

    //
    // Member that keeps track of the scanner's feeder control status:
    //
    BOOL m_bFeederStarted;

    //
    // Mini-driver private methods:
    //

    HRESULT InitializeDeviceConnection(
        _In_ LPCWSTR wszDevicePath,
        _In_ HKEY    hDeviceKey);

    HRESULT
    BuildDriverItemTree();

    HRESULT
    DestroyDriverItemTree();

    //
    // Property initialization methods:
    //

    HRESULT
    InitializeRootItemProperties(
        _In_ BYTE* pWiasContext);

    HRESULT
    InitializeChildItemProperties(
        _In_ BYTE* pWiasContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    InitializeCommonChildProperties(
        _In_ BYTE* pWiasContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    InitializeFlatbedFeederProperties(
        _In_ BYTE* pWiasContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    InitializeFeederSpecificProperties(
        _In_ BYTE* pWiasContext);

    HRESULT
    InitializeImprinterEndorserProperties(
        _In_ BYTE* pWiasContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    InitializeBarcodeReaderProperties(
        _In_ BYTE* pWiasContext);

    HRESULT
    InitializePatchCodeReaderProperties(
        _In_ BYTE* pWiasContext);

    HRESULT
    InitializeMicrReaderProperties(
        _In_ BYTE* pWiasContext);

    HRESULT
    InitializeFormatInfoArrays();

    //
    // Property validation methods:
    //

    HRESULT
    ValidateFormatProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    ValidateRegionProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    ValidateImageInfoProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    ValidateFeedProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext);

    HRESULT
    ValidateImprinterEndorserProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext,
        UINT nDocumentHandlingSelect);

    HRESULT
    ValidateBarcodeReaderProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext);

    HRESULT
    ValidatePatchCodeReaderProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext);

    HRESULT
    ValidateMicrReaderProperties(
        _In_ BYTE* pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext);

    HRESULT
    ValidateColorDropProperty(
    _In_ BYTE*                     pWiasContext,
    _In_ WIA_PROPERTY_CONTEXT*     pPropertyContext,
    UINT                           nChannel);

    HRESULT
    ValidateColorDropProperties(
        _In_ BYTE*                 pWiasContext,
        _In_ WIA_PROPERTY_CONTEXT* pPropertyContext,
        UINT                       nDocumentHandlingSelect);

    HRESULT
    UpdateImageInfoProperties(
        _In_ BYTE *pWiasContext,
        LONG lDataType);

    HRESULT
    UpdateScanAvailableItemName(
        _In_opt_ LPCWSTR wszInputSource);

    HRESULT
    UpdateScanAvailableItemProperty(
        _In_ BYTE *pWiasContext);

    LONG
    GetValidPageSizes(
        LONG lMaxWidth,
        LONG lMaxHeight,
        LONG lMinWidth,
        LONG lMinHeight,
        BOOL bPortrait,
        CBasicDynamicArray<LONG>& arrayPageSizes);

    HRESULT
    GetPageDimensions(
        LONG lPageSize,
        BOOL bPortrait,
        LONG& lPageWidth,
        LONG& lPageHeight);

    //
    // Data transfer methods:
    //

    HRESULT
    Download(
        _In_ BYTE* pWiasContext,
        GUID guidItemCategory,
        _In_ BSTR bstrItemName,
        _In_ BSTR bstrFullItemName,
        _In_ PMINIDRV_TRANSFER_CONTEXT pmdtc,
        _In_reads_bytes_(ulBufferSize) BYTE* pTransferBuffer,
        ULONG ulBufferSize,
        _In_ IWiaMiniDrvTransferCallback* pTransferCallback,
        _In_ WiaTransferParams* pCallbackTransferParams,
        _In_ WIA_DRIVER_ITEM_CONTEXT* pWiaDriverItemContext);

    HRESULT
    Upload(
        _In_ BYTE* pWiasContext,
        GUID guidItemCategory,
        _In_ BSTR bstrItemName,
        _In_ BSTR bstrFullItemName,
        _In_reads_bytes_(ulBufferSize) BYTE* pTransferBuffer,
        ULONG ulBufferSize,
        _In_ IWiaMiniDrvTransferCallback* pTransferCallback,
        _In_ WiaTransferParams* pCallbackTransferParams,
        _In_ WIA_DRIVER_ITEM_CONTEXT* pWiaDriverItemContext);

    HRESULT
    TransferFile(
        _In_ IStream* pInputStream,
        _In_ IStream* pDestinationStream,
        _In_reads_bytes_(ulBufferSize) BYTE * pTransferBuffer,
        ULONG ulBufferSize,
        _In_opt_ IWiaMiniDrvTransferCallback* pTransferCallback,
        _In_opt_ WiaTransferParams* pCallbackTransferParams,
        ULONG ulEstimatedFileSize,
        _Out_ BOOL* pbCancelTransfer);

    HRESULT
    IsDibValid(
        _In_ IStream* pStream,
        LONG lBitDepth,
        LONG lWidth,
        LONG lHeight);

    HRESULT
    IsImprinterEndorserTextValid(
        _In_ BYTE* pWiasContext,
        _In_ IStream* pStream,
        LONG nDocumentHandlingSelect);

    HRESULT
    LoadTestDataResourceToStream(
        ULONG ulResourceId,
        _In_ IStream* pStream,
        _Out_ ULONG* pulDataSize);

    HRESULT
    LoadTestDataToStream(
        _In_ BYTE* pWiasContext,
        GUID guidItemCategory,
        GUID guidFormat,
        _In_ IStream* pStream,
        _Out_ ULONG* pulDataSize);

    inline void
    ComputeTransferProgress(
        _Inout_ ULONG *pulPercentComplete,
        ULONG ulEstimatedFileSize,
        ULONG ulFileBytesWritten,
        BOOL bDirectWIATransfer = TRUE,
        ULONG ulResumeFrom = 0);

    //
    // Manual feeder control methods:
    //

    HRESULT
    StartFeeder();

    HRESULT
    StopFeeder();
};

//
// Simple inline function that converts an HRESULT to a Win32 error code:
//
inline DWORD
WIN32_FROM_HRESULT(HRESULT hr)
{
    return ((SUCCEEDED(hr) ? ERROR_SUCCESS :  (HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : (hr))));
}

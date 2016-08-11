/**************************************************************************
*
*  Copyright (c) 2003  Microsoft Corporation
*
*  Title: wiadriver.h
*
*  Description: This contains the WIA driver class definition and needed
*               defines.
*
***************************************************************************/

#pragma once

#define MY_WIA_ERROR_HANDLING_PROP         WIA_PRIVATE_ITEMPROP
#define MY_WIA_ERROR_HANDLING_PROP_STR     L"My error handling property"

#define ERROR_HANDLING_NONE                0x00000000
#define ERROR_HANDLING_WARMING_UP          0x00000001
#define ERROR_HANDLING_COVER_OPEN          0x00000002
#define ERROR_HANDLING_PRIVATE_ERROR       0x00000004
#define ERROR_HANDLING_UNHANDLED_STATUS    0x00000008
#define ERROR_HANDLING_UNHANDLED_ERROR     0x00000010

//
// The only purpose of the MY_TEST_FILTER_PROP property is to illustrate
// the IWiaImageFilter::ApplyProperties method. It is never used by the
// driver itself.
//
#define MY_TEST_FILTER_PROP                WIA_PRIVATE_ITEMPROP+1
#define MY_TEST_FILTER_PROP_STR            L"My test filter property"

#define REG_ENTRY_DEVICEDATA            TEXT("DeviceData")
#define REG_ENTRY_STORAGEPATH           TEXT("StoragePath")

#define WIA_DRIVER_ROOT_NAME            L"Root"    // THIS SHOULD NOT BE LOCALIZED
#define WIA_DRIVER_FLATBED_NAME         L"Flatbed" // THIS SHOULD NOT BE LOCALIZED
#define WIA_DRIVER_FEEDER_NAME          L"Feeder"  // THIS SHOULD NOT BE LOCALIZED
#define WIA_DRIVER_FILM_NAME            L"Film"    // THIS SHOULD NOT BE LOCALIZED
#define WIA_DRIVER_STORAGE_NAME         L"Storage" // THIS SHOULD NOT BE LOCALIZED

#define DEFAULT_LOCK_TIMEOUT            1000
#define DEFAULT_NUM_DRIVER_EVENTS       2
#define DEFAULT_NUM_DRIVER_COMMANDS     0
#define DEFAULT_NUM_DRIVER_FORMATS      2

typedef struct _WIA_DRIVER_ITEM_CONTEXT
{
    GUID    guidItemCategory;
    LONG    lNumItemsStored;
    BSTR    bstrStorageDataPath;
    ULONG   ulFeederTransferCount;
}WIA_DRIVER_ITEM_CONTEXT,*PWIA_DRIVER_ITEM_CONTEXT;

class INonDelegatingUnknown {
public:
    virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,LPVOID *ppvObj) = 0;
    virtual STDMETHODIMP_(ULONG) NonDelegatingAddRef() = 0;
    virtual STDMETHODIMP_(ULONG) NonDelegatingRelease() = 0;
};

class CWIADriver : public INonDelegatingUnknown, // NonDelegatingUnknown
                   public IStiUSD,               // STI USD interface
                   public IWiaMiniDrv            // WIA Minidriver interface
{
public:

    ///////////////////////////////////////////////////////////////////////////
    // Construction/Destruction Section
    ///////////////////////////////////////////////////////////////////////////

    CWIADriver(_In_opt_ LPUNKNOWN punkOuter);
    ~CWIADriver();

private:

    ///////////////////////////////////////////////////////////////////////////
    // WIA driver internals
    ///////////////////////////////////////////////////////////////////////////

    LONG                    m_cRef;                     // Device object reference count.
    LPUNKNOWN               m_punkOuter;                // Pointer to outer unknown.
    IStiDevice             *m_pIStiDevice;              // STI device interface for locking
    IWiaDrvItem            *m_pIDrvItemRoot;            // WIA root item
    LONG                    m_lClientsConnected;        // number of applications connected
    CWIACapabilityManager   m_CapabilityManager;        // WIA driver capabilities
    WIA_FORMAT_INFO        *m_pFormats;                 // WIA format information
    ULONG                   m_ulNumFormats;             // number of data formats
    BSTR                    m_bstrDeviceID;             // WIA device ID;
    ULONG_PTR               m_ulImageLibraryToken;      // GDI plus token
    WiaDevice               m_WiaDevice;                // Simulated device object
    WCHAR                   m_wszStoragePath[MAX_PATH]; // WIA storage path
    BSTR                    m_bstrRootFullItemName;     // WIA root item (full item name)

public:

    ///////////////////////////////////////////////////////////////////////////
    // Standard COM Section
    ///////////////////////////////////////////////////////////////////////////

    STDMETHODIMP QueryInterface(REFIID riid, _COM_Outptr_ LPVOID * ppvObj);

    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    ///////////////////////////////////////////////////////////////////////////
    // IStiUSD Interface Section (for all WIA drivers)
    ///////////////////////////////////////////////////////////////////////////

    STDMETHOD(Initialize)(THIS_
                          _In_  PSTIDEVICECONTROL pHelDcb,
                                DWORD             dwStiVersion,
                          _In_  HKEY              hParametersKey);

    STDMETHOD(GetCapabilities)(THIS_ _Out_ PSTI_USD_CAPS pDevCaps);

    STDMETHOD(GetStatus)(THIS_ _Inout_ PSTI_DEVICE_STATUS pDevStatus);

    STDMETHOD(DeviceReset)(THIS);

    STDMETHOD(Diagnostic)(THIS_ _Out_ LPDIAG pBuffer);

    STDMETHOD(Escape)(THIS_
                                                    STI_RAW_CONTROL_CODE EscapeFunction,
                      _In_reads_bytes_(cbInDataSize)     LPVOID               lpInData,
                                                    DWORD                cbInDataSize,
                      _Out_writes_bytes_(dwOutDataSize)   LPVOID               pOutData,
                                                    DWORD                dwOutDataSize,
                      _Out_                         LPDWORD              pdwActualData);

    STDMETHOD(GetLastError)(THIS_ _Out_ LPDWORD pdwLastDeviceError);

    STDMETHOD(LockDevice)();

    STDMETHOD(UnLockDevice)();

    STDMETHOD(RawReadData)(THIS_
                           _Out_writes_bytes_(*lpdwNumberOfBytes)   LPVOID       lpBuffer,
                           _Out_                              LPDWORD      lpdwNumberOfBytes,
                           _Out_                              LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawWriteData)(THIS_
                            _In_reads_bytes_(dwNumberOfBytes) LPVOID       lpBuffer,
                                                         DWORD        dwNumberOfBytes,
                            _Out_                        LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawReadCommand)(THIS_
                              _Out_writes_bytes_(*lpdwNumberOfBytes)    LPVOID       lpBuffer,
                              _Out_                               LPDWORD      lpdwNumberOfBytes,
                              _Out_                               LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawWriteCommand)(THIS_
                               _In_reads_bytes_(dwNumberOfBytes)  LPVOID       lpBuffer,
                                                             DWORD        dwNumberOfBytes,
                               _Out_                         LPOVERLAPPED lpOverlapped);

    STDMETHOD(SetNotificationHandle)(THIS_ _In_ HANDLE hEvent);

    STDMETHOD(GetNotificationData)(THIS_ _In_ LPSTINOTIFY lpNotify);

    STDMETHOD(GetLastErrorInfo)(THIS_ _Out_ STI_ERROR_INFO *pLastErrorInfo);

    /////////////////////////////////////////////////////////////////////////
    // IWiaMiniDrv Interface Section (for all WIA drivers)                 //
    /////////////////////////////////////////////////////////////////////////

    STDMETHOD(drvInitializeWia)(THIS_
                                _Inout_ BYTE        *pWiasContext,
                                        LONG        lFlags,
                                _In_    BSTR        bstrDeviceID,
                                _In_    BSTR        bstrRootFullItemName,
                                _In_    IUnknown    *pStiDevice,
                                _In_    IUnknown    *pIUnknownOuter,
                                _Out_   IWiaDrvItem **ppIDrvItemRoot,
                                _Out_   IUnknown    **ppIUnknownInner,
                                _Out_   LONG        *plDevErrVal);

    STDMETHOD(drvAcquireItemData)(THIS_
                                  _In_      BYTE                      *pWiasContext,
                                            LONG                      lFlags,
                                  _In_      PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                  _Out_     LONG                      *plDevErrVal);

    STDMETHOD(drvInitItemProperties)(THIS_
                                     _Inout_    BYTE *pWiasContext,
                                                LONG lFlags,
                                     _Out_      LONG *plDevErrVal);

    STDMETHOD(drvValidateItemProperties)(THIS_
                                         _Inout_    BYTE           *pWiasContext,
                                                    LONG           lFlags,
                                                    ULONG          nPropSpec,
                                         _In_       const PROPSPEC *pPropSpec,
                                         _Out_      LONG           *plDevErrVal);

    STDMETHOD(drvWriteItemProperties)(THIS_
                                      _Inout_   BYTE                      *pWiasContext,
                                                LONG                      lFlags,
                                      _In_      PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                      _Out_     LONG                      *plDevErrVal);

    STDMETHOD(drvReadItemProperties)(THIS_
                                     _In_       BYTE           *pWiasContext,
                                                LONG           lFlags,
                                                ULONG          nPropSpec,
                                     _In_       const PROPSPEC *pPropSpec,
                                     _Out_      LONG           *plDevErrVal);

    STDMETHOD(drvLockWiaDevice)(THIS_
                                _In_    BYTE *pWiasContext,
                                        LONG lFlags,
                                _Out_   LONG *plDevErrVal);

    STDMETHOD(drvUnLockWiaDevice)(THIS_
                                  _In_      BYTE *pWiasContext,
                                            LONG lFlags,
                                  _Out_     LONG *plDevErrVal);

    STDMETHOD(drvAnalyzeItem)(THIS_
                              _In_      BYTE *pWiasContext,
                                        LONG lFlags,
                              _Out_     LONG *plDevErrVal);

    STDMETHOD(drvGetDeviceErrorStr)(THIS_
                                              LONG     lFlags,
                                              LONG     lDevErrVal,
                                    _Out_     LPOLESTR *ppszDevErrStr,
                                    _Out_     LONG     *plDevErr);

    STDMETHOD(drvDeviceCommand)(THIS_
                                _Inout_     BYTE            *pWiasContext,
                                            LONG            lFlags,
                                _In_        const GUID      *plCommand,
                                _Out_       IWiaDrvItem     **ppWiaDrvItem,
                                _Out_       LONG            *plDevErrVal);

    STDMETHOD(drvGetCapabilities)(THIS_
                                  _In_      BYTE            *pWiasContext,
                                            LONG            ulFlags,
                                  _Out_     LONG            *pcelt,
                                  _Out_     WIA_DEV_CAP_DRV **ppCapabilities,
                                  _Out_     LONG            *plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
                             _Inout_    BYTE *pWiasContext,
                                        LONG lFlags,
                             _Out_      LONG *plDevErrVal);

    STDMETHOD(drvFreeDrvItemContext)(THIS_
                                                                                     LONG  lFlags,
                                     _Inout_updates_bytes_(sizeof(WIA_DRIVER_ITEM_CONTEXT)) BYTE *pSpecContext,
                                     _Out_                                           LONG *plDevErrVal);

    STDMETHOD(drvGetWiaFormatInfo)(THIS_
                                   _In_     BYTE            *pWiasContext,
                                            LONG            lFlags,
                                   _Out_    LONG            *pcelt,
                                   _Out_    WIA_FORMAT_INFO **ppwfi,
                                   _Out_    LONG            *plDevErrVal);

    STDMETHOD(drvNotifyPnpEvent)(THIS_
                                 _In_   const GUID *pEventGUID,
                                 _In_   BSTR       bstrDeviceID,
                                        ULONG      ulReserved);

    STDMETHOD(drvUnInitializeWia)(THIS_ _Inout_ BYTE *pWiasContext);

public:

    /////////////////////////////////////////////////////////////////////////
    // INonDelegating Interface Section (for all WIA drivers)              //
    /////////////////////////////////////////////////////////////////////////

    STDMETHODIMP NonDelegatingQueryInterface(REFIID  riid,LPVOID  *ppvObj);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

private:

    /////////////////////////////////////////////////////////////////////////
    // Minidriver private methods specific Section                         //
    /////////////////////////////////////////////////////////////////////////

    UINT GetBitmapResourceIDFromCategory(const GUID &guidItemCategory);

    HRESULT DownloadToStream(           LONG                           lFlags,
                             _In_       BYTE                           *pWiasContext,
                             _In_       PMINIDRV_TRANSFER_CONTEXT      pmdtc,
                                        const GUID                     &guidItemCategory,
                                        const GUID                     &guidFormatID,
                             __callback IWiaMiniDrvTransferCallback    *pTransferCallback,
                             _Out_      LONG                           *plDevErrVal);

    HRESULT DownloadRawHeader(_In_      IStream                       *pDestination,
                              _Inout_   BYTE                          *pWiasContext,
                              _In_      PMINIDRV_TRANSFER_CONTEXT     pmdtc);

    HRESULT UploadFromStream(           LONG                           lFlags,
                             _In_       BYTE                           *pWiasContext,
                                        const GUID                     &guidItemCategory,
                             __callback IWiaMiniDrvTransferCallback    *pTransferCallback,
                             _Out_      LONG                           *plDevErrVal);

    HRESULT LegacyDownload(LONG                      lFlags,
                           BYTE                      *pWiasContext,
                           const GUID                &guidItemCategory,
                           PMINIDRV_TRANSFER_CONTEXT pmdtc,
                           LONG                      *plDevErrVal);

    HRESULT BuildDriverItemTree();

    HRESULT DestroyDriverItemTree();

    HRESULT DoSynchronizeCommand(_Inout_ BYTE *pWiasContext);

};

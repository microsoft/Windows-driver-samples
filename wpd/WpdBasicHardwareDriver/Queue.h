// Queue.h : Declaration of the CQueue

#pragma once
#include "resource.h"       // main symbols
#include "WpdBasicHardwareDriver.h"

class ATL_NO_VTABLE CQueue :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IQueueCallbackDeviceIoControl,
    public IQueueCallbackCreate,
    public IObjectCleanup
{
public:
    CQueue()
    {

    }

    DECLARE_NOT_AGGREGATABLE(CQueue)

    BEGIN_COM_MAP(CQueue)
        COM_INTERFACE_ENTRY(IQueueCallbackDeviceIoControl)
        COM_INTERFACE_ENTRY(IQueueCallbackCreate)
    END_COM_MAP()

public:
    static
    HRESULT CreateInstance(
        _COM_Outptr_ IUnknown   **ppUkwn)
    {
        *ppUkwn = NULL;
        CComObject< CQueue> *pMyQueue = NULL;
        HRESULT hr = CComObject<CQueue>::CreateInstance( &pMyQueue );
        if( SUCCEEDED (hr) )
        {
            pMyQueue->AddRef();
            hr = pMyQueue->QueryInterface( __uuidof(IUnknown), (void **) ppUkwn );
            pMyQueue->Release();
            pMyQueue = NULL;
        }

        return hr;
    }

    //
    // Wdf Callbacks
    //

    // IQueueCallbackCreateClose
    //
    STDMETHOD_ (void, OnCreateFile)(
        _In_ IWDFIoQueue *pQueue,
        _In_ IWDFIoRequest *pRequest,
        _In_ IWDFFile *pFileObject
        );

    //
    // IQueueCallbackDeviceIoControl
    //
    STDMETHOD_ (void, OnDeviceIoControl)(
        _In_ IWDFIoQueue*    pQueue,
        _In_ IWDFIoRequest*  pRequest,
             ULONG           ControlCode,
             SIZE_T          InputBufferSizeInBytes,
             SIZE_T          OutputBufferSizeInBytes
        );

    //
    // IObjectCleanup
    //
    STDMETHOD_ (void, OnCleanup)(
        _In_ IWDFObject* pWdfObject
        );

private:
    HRESULT ProcessWpdMessage(
                 ULONG       ControlCode,
        _In_     ContextMap* pClientContextMap,
        _In_     IWDFDevice* pDevice,
        _In_reads_bytes_(ulInputBufferLength) PVOID pInBuffer,
                 ULONG       ulInputBufferLength,
        _Out_writes_bytes_to_(ulOutputBufferLength, *pdwBytesWritten) PVOID pOutBuffer,
                 ULONG       ulOutputBufferLength,
        _Out_    DWORD*      pdwBytesWritten);

    HRESULT GetWpdBaseDriver(
        _In_                          IWDFDevice*     pDevice,
        _Outptr_result_nullonfailure_ WpdBaseDriver** ppWpdBaseDriver);

    CComPtr<IWpdSerializer>         m_pWpdSerializer;
    CComAutoCriticalSection         m_CriticalSection;
};


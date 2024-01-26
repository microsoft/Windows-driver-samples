//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//
#pragma once
#include "stdafx.h"
#include "common.h"
#include "multipinmfthelpers.h"


extern DeviceStreamState pinStateTransition[][4];


class CPinQueue;
class CPinState;
class CMultipinMft;

class CBasePin:
    public IMFAttributes,
    public IKsControl
{
public:
    CBasePin( _In_ ULONG _id=0, _In_ CMultipinMft *parent=NULL);

    virtual ~CBasePin() = 0;
    virtual STDMETHODIMP_(DeviceStreamState) GetState();
    virtual STDMETHODIMP_(DeviceStreamState) SetState( _In_ DeviceStreamState State);
 

    //
    //IUnknown Interface functions
    //

    STDMETHODIMP_(ULONG) AddRef(
        void
        )
    {
        return InterlockedIncrement(&m_nRefCount);
    }
    STDMETHODIMP_(ULONG) Release(
        void
        )
    {
        ULONG uCount = InterlockedDecrement(&m_nRefCount);
        if (uCount == 0)
        {
            delete this;
        }
        return uCount;
    }

    STDMETHODIMP_(HRESULT) QueryInterface(
        _In_   REFIID riid,
        _Outptr_result_maybenull_  void **ppvObject
        );
    //
    // IKsControl Interface functions
    //
  
  STDMETHOD(KsProperty)(
      _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
      _In_ ULONG ulPropertyLength,
      _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
      _In_ ULONG ulDataLength,
      _Out_opt_ ULONG* pBytesReturned
      )
  {
      if ( m_spIkscontrol!=nullptr )
      {
          return m_spIkscontrol->KsProperty(pProperty,
              ulPropertyLength,
              pPropertyData,
              ulDataLength,
              pBytesReturned);
      }
      else
      {
          return E_NOTIMPL;
      }
  }
  virtual STDMETHODIMP FlushQueues(
  )
  {
      return S_OK;
  }
  //
  // NOOPs for this iteration..
  //
  STDMETHOD(KsMethod)(
      _In_reads_bytes_(ulMethodLength) PKSMETHOD pMethod,
      _In_ ULONG ulMethodLength,
      _Inout_updates_bytes_(ulDataLength) LPVOID pMethodData,
      _In_ ULONG ulDataLength,
      _Out_opt_ ULONG* pBytesReturned
      )
  {
      UNREFERENCED_PARAMETER(pBytesReturned);
      UNREFERENCED_PARAMETER(ulDataLength);
      UNREFERENCED_PARAMETER(pMethodData);
      UNREFERENCED_PARAMETER(pMethod);
      UNREFERENCED_PARAMETER(ulMethodLength);
      return S_OK;
  }
  
  STDMETHOD(KsEvent)(
      _In_reads_bytes_(ulEventLength) PKSEVENT pEvent,
      _In_ ULONG ulEventLength,
      _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData,
      _In_ ULONG ulDataLength,
      _Out_opt_ ULONG* pBytesReturned
      )
  {
      UNREFERENCED_PARAMETER(pBytesReturned);
      UNREFERENCED_PARAMETER(ulDataLength);
      UNREFERENCED_PARAMETER(pEventData);
      UNREFERENCED_PARAMETER(pEvent);
      UNREFERENCED_PARAMETER(ulEventLength);
      return S_OK;
  }

    //
    //IMFAttributes implementation
    //
    STDMETHOD(GetItem)(
        _In_ REFGUID guidKey,
        _Inout_opt_  PROPVARIANT* pValue
        )
    {
        return m_spAttributes->GetItem(guidKey, pValue);
    }

    STDMETHOD(GetItemType)(
        _In_ REFGUID guidKey,
        _Out_ MF_ATTRIBUTE_TYPE* pType
        )
    {
        return m_spAttributes->GetItemType(guidKey, pType);
    }

    STDMETHOD(CompareItem)(
        _In_ REFGUID guidKey,
        _In_ REFPROPVARIANT Value,
        _Out_ BOOL* pbResult
        )
    {
        return m_spAttributes->CompareItem(guidKey, Value, pbResult);
    }

    STDMETHOD(Compare)(
        _In_ IMFAttributes* pTheirs,
        _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType,
        _Out_ BOOL* pbResult
        )
    {
        return m_spAttributes->Compare(pTheirs, MatchType, pbResult);
    }

    STDMETHOD(GetUINT32)(
        _In_ REFGUID guidKey,
        _Out_ UINT32* punValue
        )
    {
        return m_spAttributes->GetUINT32(guidKey, punValue);
    }

    STDMETHOD(GetUINT64)(
        _In_ REFGUID guidKey,
        _Out_ UINT64* punValue
        )
    {
        return m_spAttributes->GetUINT64(guidKey, punValue);
    }

    STDMETHOD(GetDouble)(
        _In_ REFGUID guidKey,
        _Out_ double* pfValue
        )
    {
        return m_spAttributes->GetDouble(guidKey, pfValue);
    }

    STDMETHOD(GetGUID)(
        _In_ REFGUID guidKey,
        _Out_ GUID* pguidValue
        )
    {
        return m_spAttributes->GetGUID(guidKey, pguidValue);
    }

    STDMETHOD(GetStringLength)(
        _In_ REFGUID guidKey,
        _Out_ UINT32* pcchLength
        )
    {
        return m_spAttributes->GetStringLength(guidKey, pcchLength);
    }

    STDMETHOD(GetString)(
        _In_ REFGUID guidKey,
        _Out_writes_(cchBufSize) LPWSTR pwszValue,
        _In_ UINT32 cchBufSize,
        _Inout_opt_ UINT32* pcchLength
        )
    {
        return m_spAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    STDMETHOD(GetAllocatedString)(
        _In_ REFGUID guidKey,
        _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue,
        _Inout_  UINT32* pcchLength
        )
    {
        return m_spAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    STDMETHOD(GetBlobSize)(
        _In_ REFGUID guidKey,
        _Out_ UINT32* pcbBlobSize
        )
    {
        return m_spAttributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    STDMETHOD(GetBlob)(
        _In_                    REFGUID  guidKey,
        _Out_writes_(cbBufSize) UINT8* pBuf,
        UINT32 cbBufSize,
        _Inout_  UINT32* pcbBlobSize
        )
    {
        return m_spAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    STDMETHOD(GetAllocatedBlob)(
        __RPC__in REFGUID guidKey,
        __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf,
        __RPC__out UINT32* pcbSize
        )
    {
        return m_spAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    STDMETHOD(GetUnknown)(
        __RPC__in REFGUID guidKey,
        __RPC__in REFIID riid,
        __RPC__deref_out_opt LPVOID *ppv
        )
    {
        return m_spAttributes->GetUnknown(guidKey, riid, ppv);
    }

    STDMETHOD(SetItem)(
        _In_ REFGUID guidKey,
        _In_ REFPROPVARIANT Value
        )
    {
        return m_spAttributes->SetItem(guidKey, Value);
    }

    STDMETHOD(DeleteItem)(
        _In_ REFGUID guidKey
        )
    {
        return m_spAttributes->DeleteItem(guidKey);
    }

    STDMETHOD(DeleteAllItems)()
    {
        return m_spAttributes->DeleteAllItems();
    }

    STDMETHOD(SetUINT32)(
        _In_ REFGUID guidKey,
        _In_ UINT32  unValue
        )
    {
        return m_spAttributes->SetUINT32(guidKey, unValue);
    }

    STDMETHOD(SetUINT64)(
        _In_ REFGUID guidKey,
        _In_ UINT64  unValue
        )
    {
        return m_spAttributes->SetUINT64(guidKey, unValue);
    }

    STDMETHOD(SetDouble)(
        _In_ REFGUID guidKey,
        _In_ double  fValue
        )
    {
        return m_spAttributes->SetDouble(guidKey, fValue);
    }

    STDMETHOD(SetGUID)(
        _In_ REFGUID guidKey,
        _In_ REFGUID guidValue
        )
    {
        return m_spAttributes->SetGUID(guidKey, guidValue);
    }

    STDMETHOD(SetString)(
        _In_ REFGUID guidKey,
        _In_ LPCWSTR wszValue
        )
    {
        return m_spAttributes->SetString(guidKey, wszValue);
    }

    STDMETHOD(SetBlob)(
        _In_ REFGUID guidKey,
        _In_reads_(cbBufSize) const UINT8* pBuf,
        UINT32 cbBufSize
        )
    {
        return m_spAttributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    STDMETHOD(SetUnknown)(
        _In_ REFGUID guidKey,
        _In_ IUnknown* pUnknown
        )
    {
        return m_spAttributes->SetUnknown(guidKey, pUnknown);
    }

    STDMETHOD(LockStore)()
    {
        return m_spAttributes->LockStore();
    }

    STDMETHOD(UnlockStore)()
    {
        return m_spAttributes->UnlockStore();
    }

    STDMETHOD(GetCount)(
        _Out_ UINT32* pcItems
        )
    {
        return m_spAttributes->GetCount(pcItems);
    }

    STDMETHOD(GetItemByIndex)(
        UINT32 unIndex,
        _Out_ GUID* pguidKey,
        _Inout_ PROPVARIANT* pValue
        )
    {
        return m_spAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    STDMETHOD(CopyAllItems)(
        _In_ IMFAttributes* pDest
        )
    {
        return m_spAttributes->CopyAllItems(pDest);
    }

    //
    //Helper Functions
    //
    __requires_lock_held(m_lock)
        __inline HRESULT Active()
    {
        return (m_state == DeviceStreamState_Run)?S_OK:HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    }
    __inline DWORD streamId()
    {
        return m_StreamId;
    }

    __inline VOID setMediaType(_In_opt_ IMFMediaType *pMediaType)
    {
        m_setMediaType = pMediaType;
    }

    __inline HRESULT getMediaType(_Outptr_opt_result_maybenull_ IMFMediaType **ppMediaType)
    {
        HRESULT hr = S_OK;
        if (!ppMediaType)
            return E_INVALIDARG;

        if (m_setMediaType != nullptr)
        {
            hr = m_setMediaType.CopyTo(ppMediaType);
        }
        else
        {
            hr = MF_E_TRANSFORM_TYPE_NOT_SET;
        }
        return hr;
    }

    __inline  STDMETHOD (getPinAttributes) (_In_ IMFAttributes **ppAttributes)
    {
        return QueryInterface( IID_PPV_ARGS(ppAttributes) );
    }

    STDMETHOD(AddMediaType)(
        _Inout_ DWORD *pos,
        _In_ IMFMediaType *pMediatype);      /*Filling the media types data structure*/
    STDMETHODIMP GetMediaTypeAt(
         _In_ DWORD pos,
         _Outptr_result_maybenull_ IMFMediaType **pMediaType);   /* getting the data from the data structure*/
    STDMETHOD_(BOOL, IsMediaTypeSupported)(
        _In_ IMFMediaType *pMediaType, 
        _When_(ppIMFMediaTypeFull != nullptr, _Outptr_result_maybenull_)
        IMFMediaType **ppIMFMediaTypeFull);
    STDMETHODIMP GetOutputAvailableType(
        _In_ DWORD dwTypeIndex,
        _Out_opt_ IMFMediaType **ppType);

    VOID SetD3DManager(_In_opt_ IUnknown* pManager);
    VOID SetWorkQueue(_In_ DWORD dwQueueId)
    {
        m_dwWorkQueueId = dwQueueId;
    }
protected:
    //
    //Inline helper functions
    //
        _inline CMultipinMft* Parent()
        {
            return m_Parent;
        }
        __inline HRESULT setAttributes(_In_ IMFAttributes* _pAttributes)
        {
            m_spAttributes = _pAttributes;
            return S_OK;
        }
        __inline CCritSec& lock()
        {
            return m_lock;
        }
        IMFMediaTypeArray       m_listOfMediaTypes;
        ComPtr<IMFAttributes>   m_spAttributes;
        ComPtr<IKsControl>      m_spIkscontrol;
        DeviceStreamState       m_state;
        ComPtr<IUnknown>        m_spDxgiManager;
        DWORD                   m_dwWorkQueueId;
private:
    ULONG                       m_StreamId;                  /*Device Stream Id*/
    CCritSec                    m_lock;                      /*This is only used to change the reference count i.e. active users of this stream*/
    ComPtr<IMFMediaType>        m_setMediaType;
    CMultipinMft*               m_Parent;
    ULONG                       m_nRefCount;
};



class CInPin: public CBasePin{
public:
    CInPin( _In_opt_ IMFAttributes*, _In_ ULONG ulPinId = 0, _In_ CMultipinMft *pParent=NULL);
    virtual ~CInPin();

    STDMETHOD ( Init )(
        _In_ IMFDeviceTransform * 
        );
    STDMETHOD_( VOID, ConnectPin)(
        _In_ CBasePin * 
        );
    STDMETHOD (SendSample)(
        _In_ IMFSample *
        );
    HRESULT GenerateMFMediaTypeListFromDevice(
        _In_ UINT uiStreamId
        );
    STDMETHODIMP WaitForSetInputPinMediaChange(
        );
    //
    //Corresponding IMFDeviceTransform functions for the Pin
    //
    HRESULT GetInputStreamPreferredState(
        _Inout_ DeviceStreamState *value,
        _Outptr_opt_result_maybenull_ IMFMediaType** ppMediaType
        );
    HRESULT SetInputStreamState(
        _In_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState value,
        _In_ DWORD dwFlags
        );

    virtual STDMETHODIMP FlushQueues()
    {
        return S_OK;
    }
    //
    //Inline functions
    //
    __inline IMFMediaType* getPreferredMediaType()
    {
        return m_spPrefferedMediaType.Get();
    }
    __inline VOID setPreferredMediaType( _In_ IMFMediaType *pMediaType)
    {
        m_spPrefferedMediaType = pMediaType;
    }
    __inline DeviceStreamState setPreferredStreamState(_In_ DeviceStreamState streamState)
    {
        return (DeviceStreamState)InterlockedCompareExchange((LONG*)&m_preferredStreamState, (LONG)streamState, (LONG)m_preferredStreamState);
    }
    __inline DeviceStreamState getPreferredStreamState()
    {
        return m_preferredStreamState;
    }

    STDMETHOD_( VOID, ShutdownPin)();

protected:
    ComPtr<IMFDeviceTransform>  m_spSourceTransform;  /*Source Transform i.e. DevProxy*/
    GUID                        m_stStreamType;      /*GUID representing the GUID*/
    ComPtr<CBasePin>            m_outpin;            //Only one output pin connected per input pin. There can be multiple pins connected and this could be a list   
    DeviceStreamState           m_preferredStreamState;
    ComPtr<IMFMediaType>        m_spPrefferedMediaType;
    HANDLE                      m_waitInputMediaTypeWaiter; /*Set when the input media type is changed*/

    //  Helper functions
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    HRESULT ForwardSecureBuffer(
        _In_    IMFSample *sample
    );
#endif
};



class COutPin: public CBasePin{
public:
    COutPin(
        _In_ ULONG         id = 0,
        _In_opt_  CMultipinMft *pparent = NULL,
        _In_     IKsControl*   iksControl=NULL
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
        , _In_     MFSampleAllocatorUsage allocatorUsage = MFSampleAllocatorUsage_DoesNotAllocate
#endif
    );
    virtual ~COutPin();
    STDMETHODIMP FlushQueues();
    STDMETHODIMP AddPin(
        _In_ DWORD pinId
        );
    virtual STDMETHODIMP AddSample(
        _In_ IMFSample *pSample,
        _In_ CBasePin *inPin
        );
    STDMETHODIMP GetOutputStreamInfo(
        _Out_  MFT_OUTPUT_STREAM_INFO *pStreamInfo
        );
    virtual STDMETHODIMP ChangeMediaTypeFromInpin(
        _In_ IMFMediaType *pInMediatype,
        _In_ IMFMediaType* pOutMediaType,
        _In_ DeviceStreamState state );
    STDMETHODIMP  ProcessOutput (
        _In_      DWORD dwFlags,
        _Inout_   MFT_OUTPUT_DATA_BUFFER  *pOutputSample,
        _Out_     DWORD                   *pdwStatus
        );
    STDMETHODIMP KsProperty(
        _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Out_opt_ ULONG* pBytesReturned
        );
    STDMETHODIMP_(VOID) SetFirstSample(
        _In_    BOOL 
        );

    STDMETHODIMP_(VOID) SetAllocator(
        _In_    IMFVideoSampleAllocator* pAllocator
    );
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    MFSampleAllocatorUsage GetSampleAllocatorUsage()
    {
        return m_allocatorUsage;
    }
#endif
    UINT32 GetMediatypeCount()
    {
        return (UINT32)m_listOfMediaTypes.size();
    }
    
protected:
    CPinQueue *               m_queue;           /* Queue where the sample will be stored*/
    BOOL                      m_firstSample;
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
    MFSampleAllocatorUsage    m_allocatorUsage;
#endif
    wil::com_ptr_nothrow<IMFVideoSampleAllocator> m_spDefaultAllocator;

};


class CAsyncInPin: public CInPin {
public:

    STDMETHODIMP SendSample(
        _In_ IMFSample *
        );
    STDMETHODIMP Invoke(
        _In_ IMFAsyncResult *
    );
    STDMETHODIMP Init();
    CAsyncInPin(
        _In_opt_ IMFAttributes *pAttributes,
        _In_ ULONG ulPinId,
        _In_ CMultipinMft *pParent) : CInPin(pAttributes,
            ulPinId, pParent)
        , m_asyncCallback(nullptr)

    {
        Init();
    }
    STDMETHOD_(VOID, ShutdownPin)();
    virtual ~CAsyncInPin()
    {
        FlushQueues();
    }


    ComPtr<CDMFTAsyncCallback<CAsyncInPin,&CAsyncInPin::Invoke> >  m_asyncCallback;   // Callback object
    ////////////////////////////////////////////////////////////////////////////////////////
    // End of Asynchronous callback definitions
    ////////////////////////////////////////////////////////////////////////////////////////

};

class CTranslateOutPin : public COutPin {
    /*List of GUIDS to be translated*/
    const GUID tranlateGUIDS[2] =  {
        MFVideoFormat_H264,
        MFVideoFormat_MJPG
    };

    // @@@@README 
    // If you translate to YUY2 in D3D mode it is a suboptimal path, because the 
    // pipeline i.e. Frameserver will lock the surface into a staging buffer and 
    // map it to the client process like Teams, Camera App etc.
    // Ideally when translating to YUY2, don't pass the D3D Manager to the 
    // Decoder (CDecoderTee) or the Video Processor (CXVPTee). The pipeline will
    // shove the system buffer back into the DX surface on the client side, if the App
    // demands DX surfaces. NV12 is sharable from frameserver to clients and hence
    // the preferred format to decode into.
    // The below subtype is what the compressed media types will be translated into.
    const GUID translatedGUID = MFVideoFormat_NV12; // Translating to NV12
public:
    CTranslateOutPin(_In_ ULONG         id = 0,
        _In_opt_  CMultipinMft *pparent = NULL,
        _In_     IKsControl*   iksControl = NULL)
        : COutPin(id, pparent, iksControl
#if ((defined NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB))
            , MFSampleAllocatorUsage_UsesCustomAllocator
#endif
        )
    {
        SetUINT32(MF_SD_VIDEO_SPHERICAL, TRUE);
    }
    STDMETHOD(AddMediaType)(
        _Inout_ DWORD *pos,
        _In_ IMFMediaType *pMediatype);
    STDMETHOD_(BOOL, IsMediaTypeSupported)(
        _In_ IMFMediaType *pMediaType,
        _When_(ppIMFMediaTypeFull != nullptr, _Outptr_result_maybenull_)
        IMFMediaType **ppIMFMediaTypeFull);
    STDMETHOD(ChangeMediaTypeFromInpin)(
        _In_ IMFMediaType *pInMediatype,
        _In_ IMFMediaType* pOutMediaType,
        _In_ DeviceStreamState state);
    virtual ~CTranslateOutPin() {}
protected:

    map<ComPtr<IMFMediaType>, ComPtr<IMFMediaType>> m_TranslatedMediaTypes;
};



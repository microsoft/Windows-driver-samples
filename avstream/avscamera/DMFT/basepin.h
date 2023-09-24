//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once
#include "stdafx.h"

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
    virtual IFACEMETHODIMP_(DeviceStreamState) GetState();
    virtual IFACEMETHODIMP_(DeviceStreamState) SetState( _In_ DeviceStreamState State);
 

    //
    //IUnknown Interface functions
    //

    IFACEMETHODIMP_(ULONG) AddRef(
        void
        )
    {
        return InterlockedIncrement(&m_nRefCount);
    }
    IFACEMETHODIMP_(ULONG) Release(
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

    IFACEMETHODIMP_(HRESULT) QueryInterface(
        _In_   REFIID riid,
        _Outptr_result_maybenull_  void **ppvObject
        );
    //
    // IKsControl Interface functions
    //
  
  IFACEMETHODIMP KsProperty(
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
  virtual IFACEMETHODIMP FlushQueues(
  )
  {
      return S_OK;
  }
  //
  // NOOPs for this iteration..
  //
  IFACEMETHODIMP KsMethod(
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
  
  IFACEMETHODIMP KsEvent(
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
    IFACEMETHODIMP GetItem(
        _In_ REFGUID guidKey,
        _Inout_opt_  PROPVARIANT* pValue
        )
    {
        return m_spAttributes->GetItem(guidKey, pValue);
    }

    IFACEMETHODIMP GetItemType(
        _In_ REFGUID guidKey,
        _Out_ MF_ATTRIBUTE_TYPE* pType
        )
    {
        return m_spAttributes->GetItemType(guidKey, pType);
    }

    IFACEMETHODIMP CompareItem(
        _In_ REFGUID guidKey,
        _In_ REFPROPVARIANT Value,
        _Out_ BOOL* pbResult
        )
    {
        return m_spAttributes->CompareItem(guidKey, Value, pbResult);
    }

    IFACEMETHODIMP Compare(
        _In_ IMFAttributes* pTheirs,
        _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType,
        _Out_ BOOL* pbResult
        )
    {
        return m_spAttributes->Compare(pTheirs, MatchType, pbResult);
    }

    IFACEMETHODIMP GetUINT32(
        _In_ REFGUID guidKey,
        _Out_ UINT32* punValue
        )
    {
        return m_spAttributes->GetUINT32(guidKey, punValue);
    }

    IFACEMETHODIMP GetUINT64(
        _In_ REFGUID guidKey,
        _Out_ UINT64* punValue
        )
    {
        return m_spAttributes->GetUINT64(guidKey, punValue);
    }

    IFACEMETHODIMP GetDouble(
        _In_ REFGUID guidKey,
        _Out_ double* pfValue
        )
    {
        return m_spAttributes->GetDouble(guidKey, pfValue);
    }

    IFACEMETHODIMP GetGUID(
        _In_ REFGUID guidKey,
        _Out_ GUID* pguidValue
        )
    {
        return m_spAttributes->GetGUID(guidKey, pguidValue);
    }

    IFACEMETHODIMP GetStringLength(
        _In_ REFGUID guidKey,
        _Out_ UINT32* pcchLength
        )
    {
        return m_spAttributes->GetStringLength(guidKey, pcchLength);
    }

    IFACEMETHODIMP GetString(
        _In_ REFGUID guidKey,
        _Out_writes_(cchBufSize) LPWSTR pwszValue,
        _In_ UINT32 cchBufSize,
        _Inout_opt_ UINT32* pcchLength
        )
    {
        return m_spAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    IFACEMETHODIMP GetAllocatedString(
        _In_ REFGUID guidKey,
        _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue,
        _Inout_  UINT32* pcchLength
        )
    {
        return m_spAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    IFACEMETHODIMP GetBlobSize(
        _In_ REFGUID guidKey,
        _Out_ UINT32* pcbBlobSize
        )
    {
        return m_spAttributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    IFACEMETHODIMP GetBlob(
        _In_                    REFGUID  guidKey,
        _Out_writes_(cbBufSize) UINT8* pBuf,
        UINT32 cbBufSize,
        _Inout_  UINT32* pcbBlobSize
        )
    {
        return m_spAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    IFACEMETHODIMP GetAllocatedBlob(
        __RPC__in REFGUID guidKey,
        __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf,
        __RPC__out UINT32* pcbSize
        )
    {
        return m_spAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    IFACEMETHODIMP GetUnknown(
        __RPC__in REFGUID guidKey,
        __RPC__in REFIID riid,
        __RPC__deref_out_opt LPVOID *ppv
        )
    {
        return m_spAttributes->GetUnknown(guidKey, riid, ppv);
    }

    IFACEMETHODIMP SetItem(
        _In_ REFGUID guidKey,
        _In_ REFPROPVARIANT Value
        )
    {
        return m_spAttributes->SetItem(guidKey, Value);
    }

    IFACEMETHODIMP DeleteItem(
        _In_ REFGUID guidKey
        )
    {
        return m_spAttributes->DeleteItem(guidKey);
    }

    IFACEMETHODIMP DeleteAllItems()
    {
        return m_spAttributes->DeleteAllItems();
    }

    IFACEMETHODIMP SetUINT32(
        _In_ REFGUID guidKey,
        _In_ UINT32  unValue
        )
    {
        return m_spAttributes->SetUINT32(guidKey, unValue);
    }

    IFACEMETHODIMP SetUINT64(
        _In_ REFGUID guidKey,
        _In_ UINT64  unValue
        )
    {
        return m_spAttributes->SetUINT64(guidKey, unValue);
    }

    IFACEMETHODIMP SetDouble(
        _In_ REFGUID guidKey,
        _In_ double  fValue
        )
    {
        return m_spAttributes->SetDouble(guidKey, fValue);
    }

    IFACEMETHODIMP SetGUID(
        _In_ REFGUID guidKey,
        _In_ REFGUID guidValue
        )
    {
        return m_spAttributes->SetGUID(guidKey, guidValue);
    }

    IFACEMETHODIMP SetString(
        _In_ REFGUID guidKey,
        _In_ LPCWSTR wszValue
        )
    {
        return m_spAttributes->SetString(guidKey, wszValue);
    }

    IFACEMETHODIMP SetBlob(
        _In_ REFGUID guidKey,
        _In_reads_(cbBufSize) const UINT8* pBuf,
        UINT32 cbBufSize
        )
    {
        return m_spAttributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    IFACEMETHODIMP SetUnknown(
        _In_ REFGUID guidKey,
        _In_ IUnknown* pUnknown
        )
    {
        return m_spAttributes->SetUnknown(guidKey, pUnknown);
    }

    IFACEMETHODIMP LockStore()
    {
        return m_spAttributes->LockStore();
    }

    IFACEMETHODIMP UnlockStore()
    {
        return m_spAttributes->UnlockStore();
    }

    IFACEMETHODIMP GetCount(
        _Out_ UINT32* pcItems
        )
    {
        return m_spAttributes->GetCount(pcItems);
    }

    IFACEMETHODIMP GetItemByIndex(
        UINT32 unIndex,
        _Out_ GUID* pguidKey,
        _Inout_ PROPVARIANT* pValue
        )
    {
        return m_spAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    IFACEMETHODIMP CopyAllItems(
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

    __inline  IFACEMETHODIMP getPinAttributes (_In_ IMFAttributes **ppAttributes)
    {
        return QueryInterface( IID_PPV_ARGS(ppAttributes) );
    }

    IFACEMETHODIMP AddMediaType(
        _Inout_ DWORD *pos,
        _In_ IMFMediaType *pMediatype);      /*Filling the media types data structure*/
    IFACEMETHODIMP GetMediaTypeAt(
         _In_ DWORD pos,
         _Outptr_result_maybenull_ IMFMediaType **pMediaType);   /* getting the data from the data structure*/
    IFACEMETHODIMP_(BOOL) IsMediaTypeSupported(
        _In_ IMFMediaType *pMediaType, 
        _When_(ppIMFMediaTypeFull != nullptr, _Outptr_result_maybenull_)
        IMFMediaType **ppIMFMediaTypeFull);
    IFACEMETHODIMP GetOutputAvailableType(
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
    ~CInPin();

    IFACEMETHODIMP Init(
        _In_ IMFDeviceTransform * 
        );
    IFACEMETHODIMP_(VOID) ConnectPin(
        _In_ CBasePin * 
        );
    IFACEMETHODIMP SendSample(
        _In_ IMFSample *
        );
    HRESULT GenerateMFMediaTypeListFromDevice(
        _In_ UINT uiStreamId
        );
    IFACEMETHODIMP WaitForSetInputPinMediaChange(
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

    virtual IFACEMETHODIMP FlushQueues()
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

    IFACEMETHODIMP_( VOID) ShutdownPin();

protected:
    ComPtr<IMFDeviceTransform>        m_spSourceTransform;  /*Source Transform i.e. DevProxy*/
    GUID                        m_stStreamType;      /*GUID representing the GUID*/
    ComPtr<CBasePin>            m_outpin;            //Only one output pin connected per input pin. There can be multiple pins connected and this could be a list   
    DeviceStreamState           m_preferredStreamState;
    ComPtr<IMFMediaType>        m_spPrefferedMediaType;
    HANDLE                      m_waitInputMediaTypeWaiter; /*Set when the input media type is changed*/

};



class COutPin: public CBasePin{
public:
    COutPin(
        _In_ ULONG         id = 0,
        _In_opt_  CMultipinMft *pparent = NULL,
        _In_     IKsControl*   iksControl=NULL
    );
    ~COutPin();
    IFACEMETHODIMP FlushQueues();
    IFACEMETHODIMP AddPin(
        _In_ DWORD pinId
        );
    virtual IFACEMETHODIMP AddSample(
        _In_ IMFSample *pSample,
        _In_ CBasePin *inPin
        );
    IFACEMETHODIMP GetOutputStreamInfo(
        _Out_  MFT_OUTPUT_STREAM_INFO *pStreamInfo
        );
    virtual IFACEMETHODIMP ChangeMediaTypeFromInpin(
        _In_ IMFMediaType* pOutMediaType,
        _In_ DeviceStreamState state );
    IFACEMETHODIMP  ProcessOutput (
        _In_      DWORD dwFlags,
        _Inout_   MFT_OUTPUT_DATA_BUFFER  *pOutputSample,
        _Out_     DWORD                   *pdwStatus
        );
    IFACEMETHODIMP KsProperty(
        _In_reads_bytes_(ulPropertyLength) PKSPROPERTY pProperty,
        _In_ ULONG ulPropertyLength,
        _Inout_updates_bytes_(ulDataLength) LPVOID pPropertyData,
        _In_ ULONG ulDataLength,
        _Out_opt_ ULONG* pBytesReturned
        );
    IFACEMETHODIMP_(VOID) SetFirstSample(
        _In_    BOOL 
        );

    UINT32 GetMediatypeCount()
    {
        return (UINT32)m_listOfMediaTypes.size();
    }
    
protected:
    CPinQueue *               m_queue;           /* Queue where the sample will be stored*/
    BOOL                      m_firstSample;
};


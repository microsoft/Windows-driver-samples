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
    virtual STDMETHODIMP_(DeviceStreamState) GetState() = 0;
    virtual STDMETHODIMP_(DeviceStreamState) SetState( _In_ DeviceStreamState State) = 0;
 

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
      if ( m_Ikscontrol!=nullptr )
      {
          return m_Ikscontrol->KsProperty(pProperty,
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
  //
  //NOOPs for this iteration..
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
        return m_Attributes->GetItem(guidKey, pValue);
    }

    STDMETHOD(GetItemType)(
        _In_ REFGUID guidKey,
        _Out_ MF_ATTRIBUTE_TYPE* pType
        )
    {
        return m_Attributes->GetItemType(guidKey, pType);
    }

    STDMETHOD(CompareItem)(
        _In_ REFGUID guidKey,
        _In_ REFPROPVARIANT Value,
        _Out_ BOOL* pbResult
        )
    {
        return m_Attributes->CompareItem(guidKey, Value, pbResult);
    }

    STDMETHOD(Compare)(
        _In_ IMFAttributes* pTheirs,
        _In_ MF_ATTRIBUTES_MATCH_TYPE MatchType,
        _Out_ BOOL* pbResult
        )
    {
        return m_Attributes->Compare(pTheirs, MatchType, pbResult);
    }

    STDMETHOD(GetUINT32)(
        _In_ REFGUID guidKey,
        _Out_ UINT32* punValue
        )
    {
        return m_Attributes->GetUINT32(guidKey, punValue);
    }

    STDMETHOD(GetUINT64)(
        _In_ REFGUID guidKey,
        _Out_ UINT64* punValue
        )
    {
        return m_Attributes->GetUINT64(guidKey, punValue);
    }

    STDMETHOD(GetDouble)(
        _In_ REFGUID guidKey,
        _Out_ double* pfValue
        )
    {
        return m_Attributes->GetDouble(guidKey, pfValue);
    }

    STDMETHOD(GetGUID)(
        _In_ REFGUID guidKey,
        _Out_ GUID* pguidValue
        )
    {
        return m_Attributes->GetGUID(guidKey, pguidValue);
    }

    STDMETHOD(GetStringLength)(
        _In_ REFGUID guidKey,
        _Out_ UINT32* pcchLength
        )
    {
        return m_Attributes->GetStringLength(guidKey, pcchLength);
    }

    STDMETHOD(GetString)(
        _In_ REFGUID guidKey,
        _Out_writes_(cchBufSize) LPWSTR pwszValue,
        _In_ UINT32 cchBufSize,
        _Inout_opt_ UINT32* pcchLength
        )
    {
        return m_Attributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    STDMETHOD(GetAllocatedString)(
        _In_ REFGUID guidKey,
        _Out_writes_(*pcchLength + 1) LPWSTR* ppwszValue,
        _Inout_  UINT32* pcchLength
        )
    {
        return m_Attributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    STDMETHOD(GetBlobSize)(
        _In_ REFGUID guidKey,
        _Out_ UINT32* pcbBlobSize
        )
    {
        return m_Attributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    STDMETHOD(GetBlob)(
        _In_                    REFGUID  guidKey,
        _Out_writes_(cbBufSize) UINT8* pBuf,
        UINT32 cbBufSize,
        _Inout_  UINT32* pcbBlobSize
        )
    {
        return m_Attributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    STDMETHOD(GetAllocatedBlob)(
        __RPC__in REFGUID guidKey,
        __RPC__deref_out_ecount_full_opt(*pcbSize) UINT8** ppBuf,
        __RPC__out UINT32* pcbSize
        )
    {
        return m_Attributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    STDMETHOD(GetUnknown)(
        __RPC__in REFGUID guidKey,
        __RPC__in REFIID riid,
        __RPC__deref_out_opt LPVOID *ppv
        )
    {
        return m_Attributes->GetUnknown(guidKey, riid, ppv);
    }

    STDMETHOD(SetItem)(
        _In_ REFGUID guidKey,
        _In_ REFPROPVARIANT Value
        )
    {
        return m_Attributes->SetItem(guidKey, Value);
    }

    STDMETHOD(DeleteItem)(
        _In_ REFGUID guidKey
        )
    {
        return m_Attributes->DeleteItem(guidKey);
    }

    STDMETHOD(DeleteAllItems)()
    {
        return m_Attributes->DeleteAllItems();
    }

    STDMETHOD(SetUINT32)(
        _In_ REFGUID guidKey,
        _In_ UINT32  unValue
        )
    {
        return m_Attributes->SetUINT32(guidKey, unValue);
    }

    STDMETHOD(SetUINT64)(
        _In_ REFGUID guidKey,
        _In_ UINT64  unValue
        )
    {
        return m_Attributes->SetUINT64(guidKey, unValue);
    }

    STDMETHOD(SetDouble)(
        _In_ REFGUID guidKey,
        _In_ double  fValue
        )
    {
        return m_Attributes->SetDouble(guidKey, fValue);
    }

    STDMETHOD(SetGUID)(
        _In_ REFGUID guidKey,
        _In_ REFGUID guidValue
        )
    {
        return m_Attributes->SetGUID(guidKey, guidValue);
    }

    STDMETHOD(SetString)(
        _In_ REFGUID guidKey,
        _In_ LPCWSTR wszValue
        )
    {
        return m_Attributes->SetString(guidKey, wszValue);
    }

    STDMETHOD(SetBlob)(
        _In_ REFGUID guidKey,
        _In_reads_(cbBufSize) const UINT8* pBuf,
        UINT32 cbBufSize
        )
    {
        return m_Attributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    STDMETHOD(SetUnknown)(
        _In_ REFGUID guidKey,
        _In_ IUnknown* pUnknown
        )
    {
        return m_Attributes->SetUnknown(guidKey, pUnknown);
    }

    STDMETHOD(LockStore)()
    {
        return m_Attributes->LockStore();
    }

    STDMETHOD(UnlockStore)()
    {
        return m_Attributes->UnlockStore();
    }

    STDMETHOD(GetCount)(
        _Out_ UINT32* pcItems
        )
    {
        return m_Attributes->GetCount(pcItems);
    }

    STDMETHOD(GetItemByIndex)(
        UINT32 unIndex,
        _Out_ GUID* pguidKey,
        _Inout_ PROPVARIANT* pValue
        )
    {
        return m_Attributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    STDMETHOD(CopyAllItems)(
        _In_ IMFAttributes* pDest
        )
    {
        return m_Attributes->CopyAllItems(pDest);
    }

    //
    //Helper Functions
    //
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
        HRESULT hr = S_OK;
        DMFTCHECKNULL_GOTO( ppAttributes, done, E_INVALIDARG );
        DMFTCHECKHR_GOTO  ( QueryInterface( IID_PPV_ARGS(ppAttributes) ), done );
    done:
        return hr;
    }

    STDMETHODIMP AddMediaType(
        _Inout_ DWORD *pos,
        _In_ IMFMediaType *pMediatype);      /*Filling the media types data structure*/
    STDMETHODIMP GetMediaTypeAt(
         _In_ DWORD pos,
         _Outptr_result_maybenull_ IMFMediaType **pMediaType);   /* getting the data from the data structure*/
    STDMETHODIMP_(BOOL) IsMediaTypeSupported(
        _In_ IMFMediaType *pMediaType, 
        _When_(ppIMFMediaTypeFull != nullptr, _Outptr_result_maybenull_)
        IMFMediaType **ppIMFMediaTypeFull);
    STDMETHODIMP GetOutputAvailableType(
        _In_ DWORD dwTypeIndex,
        _Out_opt_ IMFMediaType **ppType);

protected:
    //
    //Inline helper functions
    //
        _inline CMultipinMft* Parent()
        {
            return m_Parent.Get();
        }
        __inline HRESULT setAttributes(_In_ IMFAttributes* _pAttributes)
        {
            m_Attributes = _pAttributes;
            return S_OK;
        }
        __inline CCritSec& lock()
        {
            return m_lock;
        }
        IMFMediaTypeArray        m_listOfMediaTypes;
        ComPtr<IMFAttributes>   m_Attributes;
        ComPtr<IKsControl>      m_Ikscontrol;

private:
    ULONG                        m_StreamId;                  /*Device Stream Id*/
    CCritSec                     m_lock;                      /*This is only used to change the reference count i.e. active users of this stream*/
    ComPtr<IMFMediaType>        m_setMediaType;
    ComPtr<CMultipinMft>        m_Parent;
    ULONG                        m_nRefCount;
};



class CInPin: public CBasePin{
public:
    CInPin( _In_opt_ IMFAttributes*, _In_ ULONG ulPinId = 0, _In_ CMultipinMft *pParent=NULL);
    ~CInPin();

    STDMETHOD ( Init )(
        _In_ IMFTransform * 
        );
    STDMETHOD_( VOID, ConnectPin)(
        _In_ CBasePin * 
        );
    STDMETHOD (SendSample)(
        _In_ IMFSample *
        );
    STDMETHODIMP GenerateMFMediaTypeListFromDevice(
        _In_ UINT uiStreamId
        );
    STDMETHODIMP WaitForSetInputPinMediaChange(
        );
    STDMETHOD_ (DeviceStreamState, SetState)(
        _In_ DeviceStreamState
        ); /*True for Active and False for Stop*/
    STDMETHOD_(DeviceStreamState, GetState)(
        VOID
        );
    //
    //Corresponding IMFDeviceTransform functions for the Pin
    //
    STDMETHODIMP_(HRESULT) GetInputStreamPreferredState(
        _Inout_ DeviceStreamState *value,
        _Outptr_opt_result_maybenull_ IMFMediaType** ppMediaType
        );
    STDMETHODIMP_(HRESULT) SetInputStreamState(
        _In_ IMFMediaType *pMediaType,
        _In_ DeviceStreamState value,
        _In_ DWORD dwFlags
        );

    //
    //Inline functions
    //
    __inline IMFMediaType* getPreferredMediaType()
    {
        return m_prefferedMediaType.Get();
    }
    __inline VOID setPreferredMediaType( _In_ IMFMediaType *pMediaType)
    {
        m_prefferedMediaType = pMediaType;
    }
    __inline DeviceStreamState setPreferredStreamState(_In_ DeviceStreamState streamState)
    {
        return (DeviceStreamState)InterlockedCompareExchange((LONG*)&m_preferredStreamState, (LONG)streamState, (LONG)m_preferredStreamState);
    }
    __inline DeviceStreamState getPreferredStreamState()
    {
        return m_preferredStreamState;
    }

protected:
    ComPtr<IMFTransform>        m_pSourceTransform;  /*Source Transform*/
private:
    GUID                        m_stStreamType;      /*GUID representing the GUID*/
    ULONG                       m_activeStreamCount; /*Set when this stream is active*/
    vector<CBasePin*>           m_outpins;
    DeviceStreamState           m_state;
    DeviceStreamState           m_preferredStreamState;
    ComPtr<IMFMediaType>        m_prefferedMediaType;
    HANDLE                      m_waitInputMediaTypeWaiter; /*Set when the input media type is changed*/
};



class COutPin: public CBasePin{
public:
    COutPin( _In_ ULONG         id = 0,
        _In_opt_  CMultipinMft *pparent = NULL,
         _In_     IKsControl*   iksControl=NULL);
    ~COutPin();
    STDMETHODIMP AddPin(
        _In_ DWORD pinId
        );
    STDMETHODIMP AddSample(
        _In_ IMFSample *pSample,
        _In_ CBasePin *inPin
        );
    STDMETHODIMP AddSampleInternal(
        _In_ IMFSample *pSample,
        _In_ CBasePin *inPin
        );
    STDMETHODIMP RemoveSample(
        _Out_ IMFSample **
        );
    STDMETHODIMP_(DeviceStreamState) SetState(
        _In_ DeviceStreamState
        ); /*True for Active and False for Stop*/
    STDMETHODIMP_(DeviceStreamState) GetState(
        );
    STDMETHODIMP FlushQueues(
        );
    STDMETHODIMP GetOutputStreamInfo(
        _Out_  MFT_OUTPUT_STREAM_INFO *pStreamInfo
        );
    STDMETHODIMP ChangeMediaTypeFromInpin(
        _In_ CInPin* inPin,
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
    STDMETHODIMP_(VOID) SetD3Dmanager(
        _In_opt_  IUnknown *
        );
    STDMETHODIMP_(VOID) SetFirstSample(
        _In_    BOOL 
        );

    
private:
    vector< CPinState *>      m_states;          /*Array of possible states*/
    CPinState*                m_state;            /*Current state*/
    vector< CPinQueue *>      m_queues;           /*List of Queues corresponding to input pins*/
    BOOL                      m_firstSample;
   friend class CPinState;
};

//
//Not Implemented!!!
//

class CImagePin : public COutPin{
private:
    BOOL isTriggerSent;
public:
    CImagePin();
    ~CImagePin();
    STDMETHODIMP  ProcessOutput(
    _In_  DWORD dwFlags,
    _Inout_  MFT_OUTPUT_DATA_BUFFER  *pOutputSample,
    _Out_   DWORD                       *pdwStatus
    );
};



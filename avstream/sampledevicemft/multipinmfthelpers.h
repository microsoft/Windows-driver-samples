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



//
//Queue class!!!
//
class Ctee;
//typedef CMFAttributesTrace CMediaTypeTrace; /* Only used for debug. take this out*/


class CPinQueue{
public:
    CPinQueue(_In_ DWORD _inPinId);
    ~CPinQueue();

    STDMETHODIMP_(BOOL) SetState        ( _In_  BOOL state ); 
    STDMETHODIMP_(VOID) InsertInternal  ( _In_  IMFSample *pSample = nullptr );
    STDMETHODIMP_(BOOL) Insert          ( _In_ IMFSample *pSample );
    STDMETHODIMP_(BOOL) Remove          (_Outptr_result_maybenull_ IMFSample **pSample);
    STDMETHODIMP        RecreateTee     ( _In_  IMFMediaType *inMediatype, _In_ IMFMediaType *outMediatype, _In_opt_ IUnknown* punkManager );
    STDMETHODIMP_(VOID) Clear();
  
    //
    //Inline functions
    //
    __inline BOOL Empty()
    {
        return (!m_sampleList.size());
    }

    __inline DWORD pinStreamId()
    {
        return m_dwInPinId;
    }

private:
    DWORD                m_dwInPinId;           /*This is the input pin       */
    IMFSampleList        m_sampleList;          /*List storing the samples    */
    ULONG                m_sampleCount;         /*Numebr of sampels           */     
    Ctee*                m_teer;                /*Tee that acts as a passthrough or an XVP  */
    BOOL                 m_discotinuity;        /*Set after the queue is emptied or flushed */

};

class CPinState{
public:
    virtual STDMETHODIMP Open() = 0;
    CPinState( _In_ DeviceStreamState _state = DeviceStreamState_Disabled) :m_State( _state )
    {

    }
    STDMETHODIMP_(DeviceStreamState) SetState( _In_ DeviceStreamState ); /*True for Active and False for Stop*/
    STDMETHODIMP_(DeviceStreamState) GetState();

    __inline STDMETHODIMP_(DeviceStreamState) State()
    {
        return m_State;
    }
    virtual ~CPinState() = 0
    {
    }
protected:
    DeviceStreamState m_State;
};

class CPinOpenState :public CPinState{
public:
    CPinOpenState( _In_ CBasePin *p = NULL );
    //
    //Inline
    //
    __inline STDMETHODIMP Open()
    {
        return S_OK;
    }
    
private:
    CBasePin *m_pin;
};
class CPinClosedState : public CPinState{
public:
    CPinClosedState(DeviceStreamState _state = DeviceStreamState_Stop) 
        :CPinState(_state)
    {

    }
    //
    //Inline 
    //
    __inline STDMETHODIMP Open()
    {
        return MF_INVALID_STATE_ERR;
    }
};



//
// Define these in different components
// The below classes are used to add the
// XVP and the Decoder components
//
class Ctee{
public:
   virtual STDMETHODIMP PassThrough( _In_ IMFSample *, _In_ CPinQueue * ) = 0;
};


class CNullTee:public Ctee{
public:
    STDMETHODIMP PassThrough( _In_ IMFSample*, _In_ CPinQueue * );
};


class CWrapTee : public Ctee{
public:
    CWrapTee(_In_ Ctee *tee=nullptr) 
:   m_objectWrapped(tee)
    , m_pInputMediaType(nullptr)
    , m_pOutputMediaType(nullptr)
    {

    }
    virtual ~CWrapTee()=0
    {
        m_videoProcessor = nullptr;
        if (m_objectWrapped)
        {
            delete(m_objectWrapped);
        }
    }

    STDMETHODIMP PassThrough        ( _In_ IMFSample*, _In_ CPinQueue * );
    virtual STDMETHODIMP Configure  ( _In_ IMFMediaType *, _In_ IMFMediaType *, _Inout_ IMFTransform** ) = 0;
    virtual STDMETHODIMP Do         ( _In_ IMFSample* pSample, _Out_ IMFSample ** ) = 0;
    STDMETHODIMP SetMediaTypes      ( _In_ IMFMediaType* pInMediaType, _In_ IMFMediaType* pOutMediaType );
    
    //
    //Inline functions
    //
protected:
    __inline IMFTransform* Transform()
    {
        return m_videoProcessor.Get();
    }
    __inline IMFMediaType* getInMediaType()
    {
        IMFMediaType* pmediaType = nullptr;
        m_pInputMediaType.CopyTo(&pmediaType);
        return pmediaType;
    }
    __inline IMFMediaType* getOutMediaType()
    {
        IMFMediaType* pmediaType = nullptr;
        m_pOutputMediaType.CopyTo(&pmediaType);
        return pmediaType;
    }

private:
    ComPtr< IMFTransform > m_videoProcessor;
    ComPtr< IMFMediaType > m_pInputMediaType;
    ComPtr< IMFMediaType > m_pOutputMediaType;
    Ctee *m_objectWrapped;
};


class CXvptee :public CWrapTee{
public:
    CXvptee( _In_ Ctee * );
    ~CXvptee();
    STDMETHODIMP Do             (   _In_ IMFSample* pSample, _Outptr_ IMFSample ** );
    STDMETHODIMP Configure      (   _In_opt_ IMFMediaType *, _In_opt_ IMFMediaType *, _Outptr_ IMFTransform** );
    STDMETHODIMP_(VOID) SetD3DManager( IUnknown* punk );

private:
    BOOL  m_isoptimizedPlanarInputOutput;
    BOOL  m_isOutPutImage;
    UINT32 m_uWidth;
    UINT32 m_uHeight;
    ComPtr<IUnknown> m_spDeviceManagerUnk;
};

class CDecoderTee : public CWrapTee{
public:
    CDecoderTee(Ctee* *);
    ~CDecoderTee();

    STDMETHODIMP Do( _In_ IMFSample* pSample, _Out_ IMFSample ** );
    STDMETHODIMP Configure( _In_opt_ IMFMediaType *, _In_opt_ IMFMediaType *, _Outptr_ IMFTransform** );
    STDMETHODIMP ConfigureEncoder( _In_ IMFTransform *pTransform );
    STDMETHODIMP ConfigureDecoder( _In_ IMFTransform *pTransform );
private:
    ComPtr<IUnknown> m_spDeviceManagerUnk;
};



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


/*
################## EVENT HANDLING #############################################
Events are usually divided into two categories by the Capture Pipeline
1) Regular Event / Manual Reset Event
2) One Shot Event

Regular events are set and cleared by the pipeline
One shot Events will only be set by the pipeline and it should be cleared
by the Component managing the Event Store i.e. KS or DMFT. For Redstone
The pipeline should not send any Non One shot events.This sample however
does show the handling of Regular/ Manual Reset events

This clearing of One shot events is done when the event is fired..
E.g before sending the warm start command The pipeline will send a one shot event
KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART and when the operation completes the
event should be fired.
The Device MFT should usually keep a watch for one shot events sent by the Pipeline
for Async Extended controls..The list of Asynchronous controls are as follows..

KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMODE
KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOMAXFRAMERATE
KSPROPERTY_CAMERACONTROL_EXTENDED_FOCUSMODE
KSPROPERTY_CAMERACONTROL_REGION_OF_INTEREST_PROPERTY_ID
KSPROPERTY_CAMERACONTROL_EXTENDED_ISO
KSPROPERTY_CAMERACONTROL_EXTENDED_ISO_ADVANCED
KSPROPERTY_CAMERACONTROL_EXTENDED_EVCOMPENSATION
KSPROPERTY_CAMERACONTROL_EXTENDED_WHITEBALANCEMODE
KSPROPERTY_CAMERACONTROL_EXTENDED_EXPOSUREMODE
KSPROPERTY_CAMERACONTROL_EXTENDED_SCENEMODE
KSPROPERTY_CAMERACONTROL_EXTENDED_PHOTOTHUMBNAIL
KSPROPERTY_CAMERACONTROL_EXTENDED_WARMSTART
KSPROPERTY_CAMERACONTROL_EXTENDED_ROI_ISPCONTROL
KSPROPERTY_CAMERACONTROL_EXTENDED_PROFILE
The complete list can be found from the Camera DDI spec
###############################################################################
*/

typedef struct _DMFTEventEntry{
    ULONG   m_ulEventId;        // KSEVENT->Id
    PVOID   m_pEventData;       // Lookup for events in the data structure
    HANDLE  m_hHandle;          // The duplicate handle stored from the event
    //
    // Constructor. We simply cache the handles, the property id and the KSEVENTDATA buffer sent from the user mode
    //
    _DMFTEventEntry( _In_ ULONG ulEventId, _In_ PVOID pEventData, _In_ HANDLE pHandle):m_pEventData(pEventData)
        , m_ulEventId(ulEventId)
        , m_hHandle(pHandle)
    {
    }
    ~_DMFTEventEntry()
    {
        if ( m_hHandle != nullptr )
        {
            CloseHandle(m_hHandle);
            m_hHandle = nullptr;
        }
    }

}DMFTEventEntry, *PDMFTEventEntry;

//
// Handler for one shot events and Normal events
//
class CDMFTEventHandler{
public:
    //
    // Handle the events here
    //
    STDMETHOD(KSEvent)(_In_reads_bytes_(ulEventLength) PKSEVENT pEvent,
        _In_ ULONG ulEventLength,
        _Inout_updates_bytes_opt_(ulDataLength) LPVOID pEventData,
        _In_ ULONG ulDataLength,
        _Inout_ ULONG* pBytesReturned);
    STDMETHOD(SetOneShot)(ULONG);
    STDMETHOD(SetRegularEvent)(ULONG);
    STDMETHOD(Clear)();
protected:
    STDMETHOD(Dupe)(_In_ HANDLE hEventHandle, _Outptr_ LPHANDLE lpTargetHandle);
private:
    map< ULONG, HANDLE >        m_OneShotEventMap;
    vector< PDMFTEventEntry >   m_RegularEventList;
};




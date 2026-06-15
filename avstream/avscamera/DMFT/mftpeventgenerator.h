//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media Foundation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
//
#pragma once

class CMediaEventGenerator :
    public IMFMediaEventGenerator
{

public:

    //
    // IUnknown
    //
    STDMETHOD_(ULONG, AddRef)(
        void
        );
    
    STDMETHOD_(ULONG, Release)(
        void
        );
    
    STDMETHOD(QueryInterface)(
        _In_ REFIID iid,
        _COM_Outptr_ void** ppv);


    //
    // IMFMediaEventGenerator
    //
    STDMETHOD(BeginGetEvent)(
        _In_ IMFAsyncCallback* pCallback,
        _In_ IUnknown* pState
        );
    
    STDMETHOD(EndGetEvent)(
        _In_ IMFAsyncResult* pResult,
        _Outptr_result_maybenull_ IMFMediaEvent** ppEvent
        );
    
    STDMETHOD(GetEvent)(
        _In_ DWORD dwFlags,
        _Outptr_result_maybenull_ IMFMediaEvent** ppEvent
        );
    
    STDMETHOD(QueueEvent)(
        _In_ MediaEventType met,
        _In_ REFGUID extendedType,
        _In_ HRESULT hrStatus,
        _In_opt_ const PROPVARIANT* pvValue
        );

    STDMETHOD(QueueEvent)(
        _In_ IMFMediaEvent* pEvent
        );

protected:

    CMediaEventGenerator(
        void
        );

    virtual ~CMediaEventGenerator (
        void
        );
    //
    // Utility Methods
    //
    STDMETHOD(ShutdownEventGenerator)(
        void
        );
    
    STDMETHOD (InitMediaEventGenerator)(
        void
        );

    __inline HRESULT (CheckShutdown)(
        void
        ) const 
    {
        return (m_bShutdown? MF_E_SHUTDOWN : S_OK);
    }

private:

    long                m_nRefCount;
    CCritSec            m_critSec;
    IMFMediaEventQueue* m_pQueue;
    BOOL                m_bShutdown;
};

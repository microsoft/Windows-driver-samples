/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    queue.h

Abstract:

    This file defines the queue callback interface.

Environment:

    user mode only

Revision History:

--*/

#pragma once

#include "device.h"
#include "Connection.h"

static const int KilobytesPerSecond = 20;

static const int MaxCbPayload = 10240;
static const int MaxCchType = 507; // maximum message type length is 5 for "?ubs\\", 250 for protocol + 250 for subtype + 1 each for dot "." and NULL terminator.
static const int MaxCchTypeNetwork = 502; // maximum message type length over the network is 250 for protocol + 250 for subtype + 1 each for dot "." and NULL terminator.
static const int MinCchType = 2;
static const int MaxCchMimeType = 256;

#define WINDOWS_PROTOCOL        L"Windows."
#define WINDOWS_PROTOCOL_CHARS  8

#define WINDOWSURI_PROTOCOL  L"WindowsUri"

#define WINDOWSMIME_PROTOCOL       L"WindowsMime"
#define WINDOWSMIME_PROTOCOL_CHARS 11

#define PUBS_NAMESPACE        L"Pubs\\"
#define PUBS_NAMESPACE_CHARS  5

#define SUBS_NAMESPACE        L"Subs\\"
#define SUBS_NAMESPACE_CHARS  5

#define DEVICE_ARRIVED       L"DeviceArrived"
#define DEVICE_DEPARTED      L"DeviceDeparted"

#define PAIRING_PROTOCOL        L"Pairing:"
#define PAIRING_PROTOCOL_CHARS  8

#define NDEF_PROTOCOL           L"NDEF"
#define NDEF_PROTOCOL_CHARS     4

#define NDEF_EMPTY_TYPE         L"Empty"
#define NDEF_EMPTY_TYPE_CHARS   5

struct MESSAGE
{
    MESSAGE() :
        m_cbPayload(0)
    {
        ZeroMemory(m_szType, sizeof(m_szType));
        ZeroMemory(m_Payload, sizeof(m_Payload));
    }
    
    void Initialize(
        _In_                   PCWSTR szType,
        _In_                   DWORD  cbPayload, 
        _In_reads_bytes_(cbPayload) PBYTE  pbPayload
        )
    {
        ZeroMemory(m_szType, sizeof(m_szType));
        ZeroMemory(m_Payload, sizeof(m_Payload));
        
        m_cbPayload = cbPayload;
        StringCchCopy(m_szType, ARRAY_SIZE(m_szType), szType);
        CopyMemory(m_Payload, pbPayload, cbPayload);
    }
    
    wchar_t m_szType[MaxCchTypeNetwork];
    DWORD   m_cbPayload;
    BYTE    m_Payload[MaxCbPayload];
};


//
// Queue Callback Object.
//

class ATL_NO_VTABLE CMyQueue : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IQueueCallbackCreate,
    public IQueueCallbackDeviceIoControl,
    public IRequestCallbackCancel,
    public IObjectCleanup,
    public IValidateAccept,
    public IConnectionCallback
{
public:

DECLARE_NOT_AGGREGATABLE(CMyQueue)

BEGIN_COM_MAP(CMyQueue)
    COM_INTERFACE_ENTRY(IQueueCallbackCreate)
    COM_INTERFACE_ENTRY(IQueueCallbackDeviceIoControl)
    COM_INTERFACE_ENTRY(IRequestCallbackCancel)
    COM_INTERFACE_ENTRY(IObjectCleanup)
END_COM_MAP()

public:

    //IQueueCallbackCreate
    STDMETHOD_(void,OnCreateFile)(_In_ IWDFIoQueue* pWdfQueue, _In_ IWDFIoRequest* pWDFRequest, _In_ IWDFFile* pWdfFileObject);

    //IQueueCallbackDeviceIoControl
    STDMETHOD_(void,OnDeviceIoControl)(_In_ IWDFIoQueue* pWdfQueue, _In_ IWDFIoRequest* pWDFRequest, _In_ ULONG ControlCode, _In_ SIZE_T InBufferSize, _In_ SIZE_T OutBufferSize);
    
    //IObjectCleanup
    STDMETHOD_(void,OnCleanup)(_In_ IWDFObject* pWdfObject);

    //IValidateAccept
    void ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket);


    //IRequestCallbackCancel
    STDMETHODIMP_(void)
    OnCancel(
        _In_ IWDFIoRequest* pWdfRequest
        );

    //IConnectionCallback
    virtual void HandleReceivedMessage(_In_ MESSAGE* pMessageData);
    virtual void ConnectionEstablished(_In_ CConnection* pBthConnection);
    virtual BOOL ConnectionTerminated(_In_ CConnection* pBthConnection);

public:
    CMyQueue();
    ~CMyQueue();

    STDMETHOD(Initialize)(_In_ CMyDevice * Device);

    HRESULT
    Configure(
        VOID
        );
    
    STDMETHODIMP_(void)
    OnCloseFile(
        _In_ IWDFFile* pWdfFileObject
        );

private:
    
    void AddArrivalEvent();
    void AddRemovalEvent();

private:
    CComPtr<IWDFIoQueue> m_FxQueue;
    
    LIST_ENTRY       m_SubsHead;
    LIST_ENTRY       m_ArrivalSubsHead;
    LIST_ENTRY       m_DepartureSubsHead;
    CRITICAL_SECTION m_SubsLock;
    
    LIST_ENTRY       m_PubsHead;
    CRITICAL_SECTION m_PubsLock;
    
    LIST_ENTRY       m_ConnectionHead;
    CRITICAL_SECTION m_ConnectionLock;

    CSocketListener m_SocketListener;
};

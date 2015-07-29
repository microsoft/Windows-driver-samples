/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    queue.h

Abstract:

    This file defines the queue callback interface.

Environment:

    Windows User-Mode Driver Framework (WUDF)

Revision History:

--*/

#pragma once

static const int KilobytesPerSecond = 20;
static const int MinCbPayload = 1;
static const int MaxCbPayload = 10240;
static const int MaxCchType = 507;          // maximum message type length is 5 for "?ubs\\", 250 for protocol + 250 for subtype + 1 each for dot "." and nullptr terminator.
static const int MaxCchTypeNetwork = 502;   // maximum message type length over the network is 250 for protocol + 250 for subtype + 1 each for dot "." and nullptr terminator.
static const int MinCchType = 2;
static const int MaxCchMimeType = 256;
static const int MaxSecureElements = 16;

#define SIM_NAMESPACE                           L"Simulator"
#define SIM_NAMESPACE_CHARS                     ARRAYSIZE(SIM_NAMESPACE) - 1

#define NFP_RADIO_TURNED_OFF_KEY                L"NfpRadioTurnedOff"
#define NFP_RADIO_FLIGHT_MODE_KEY               L"NfpRadioFlightMode"

#define SE_ENDPOINTS_KEY                        L"SEEndpoints"
#define HCE_MESSAGE_TYPE_TRANSMIT               L"HCETransmit"

struct MESSAGE
{
    MESSAGE() : m_cbPayload(0)
    {
        RtlZeroMemory(m_szType, sizeof(m_szType));
        RtlZeroMemory(m_Payload, sizeof(m_Payload));
    }
    
    void Initialize(
        _In_ PCWSTR szType,
        _In_ DWORD cbPayload,
        _In_reads_bytes_(cbPayload) PBYTE pbPayload
        )
    {
        RtlZeroMemory(m_szType, sizeof(m_szType));
        RtlZeroMemory(m_Payload, sizeof(m_Payload));
        
        m_cbPayload = cbPayload;
        StringCchCopy(m_szType, _countof(m_szType), szType);
        RtlCopyMemory(m_Payload, pbPayload, cbPayload);
    }
    
    wchar_t m_szType[MaxCchTypeNetwork];
    DWORD m_cbPayload;
    BYTE m_Payload[MaxCbPayload];
};

class CQueue
    : public IValidateAccept,
      public IConnectionCallback
{
public:
    CQueue(WDFQUEUE Queue);
    ~CQueue();

public:
    static EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL OnIoDeviceControl;

    NTSTATUS Initialize();
    NTSTATUS Deinitialize();

    BOOLEAN IsNfpRadioEnabled();
    BOOLEAN IsSERadioEnabled();

    void OnFileCreate(_In_ WDFDEVICE Device, _In_ WDFREQUEST Request, _In_ WDFFILEOBJECT FileObject);
    void OnFileClose(_In_ WDFFILEOBJECT FileObject);

    void OnIoDeviceControl(_In_ WDFREQUEST Request, _In_ ULONG IoControlCode, _In_ size_t InputBufferLength, _In_ size_t OutputBufferLength);

public:
    //IValidateAccept
    void ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket);

    //IConnectionCallback
    virtual void HandleReceivedMessage(_In_ CONNECTION_TYPE ConnType, _In_ MESSAGE* pMessageData);
    virtual void ConnectionEstablished(_In_ CConnection* pConnection);
    virtual BOOL ConnectionTerminated(_In_ CConnection* pConnection);

public:
    CSmartCardReader* GetSmartCardReader() { return &m_SmartCardReader; }
    NTSTATUS GetSecureElementId(SECURE_ELEMENT_TYPE eType, GUID *pSecureElementId);

private:
    NTSTATUS DetectRole(CFileObject *pFileObject, PCWSTR pszFileName);
    void HandleArrivalEvent();
    void HandleRemovalEvent();
    void HandleSecureElementEvent(SECURE_ELEMENT_EVENT_INFO *pInfo);
    void EnumerateSecureElements();
    void ReadSettingsFromRegistry();
    void WriteSettingsToRegistry();
    NTSTATUS GetSecureElementObject(GUID &SecureElementId, CSecureElement **ppSecureElement);

private:
    typedef NTSTATUS (CQueue::EVT_DISPATCH_HANDLER)(
        _In_ CFileObject *pFileObject,
        _In_ WDFREQUEST Request,
        _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
        _In_ size_t InputBufferLength,
        _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
        _In_ size_t OutputBufferLength);

    typedef NTSTATUS (CQueue::*PFN_DISPATCH_HANDLER)(
        _In_ CFileObject *pFileObject,
        _In_ WDFREQUEST Request,
        _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
        _In_ size_t InputBufferLength,
        _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
        _In_ size_t OutputBufferLength);

private:
    // Proximity DDI
    EVT_DISPATCH_HANDLER OnNfpEnable;
    EVT_DISPATCH_HANDLER OnNfpDisable;
    EVT_DISPATCH_HANDLER OnNfpSetPayload;
    EVT_DISPATCH_HANDLER OnNfpGetNextSubscribedMessage;
    EVT_DISPATCH_HANDLER OnNfpGetNextTransmittedMessage;
    EVT_DISPATCH_HANDLER OnNfpGetMaxMessageBytes;
    EVT_DISPATCH_HANDLER OnNfpGetTransmissionRateKbps;
    EVT_DISPATCH_HANDLER OnNfpBeginProximity;
    EVT_DISPATCH_HANDLER OnNfpSetRadioState;
    EVT_DISPATCH_HANDLER OnNfpQueryRadioState;

    // Secure Element DDI
    EVT_DISPATCH_HANDLER OnSEEnumEndpoints;
    EVT_DISPATCH_HANDLER OnSESubscribeForEvent;
    EVT_DISPATCH_HANDLER OnSEGetNextEvent;
    EVT_DISPATCH_HANDLER OnSESetCardEmulationMode;
    EVT_DISPATCH_HANDLER OnSETriggerEvent;
    EVT_DISPATCH_HANDLER OnSEGetNfccCapabilities;
    EVT_DISPATCH_HANDLER OnSESetRoutingTable;
    EVT_DISPATCH_HANDLER OnSEGetRoutingTable;
    EVT_DISPATCH_HANDLER OnSEHCERemoteRecv;
    EVT_DISPATCH_HANDLER OnSEHCERemoteSend;

    // Smart Card DDI
    EVT_DISPATCH_HANDLER OnSCGetAttribute;
    EVT_DISPATCH_HANDLER OnSCSetAttribute;
    EVT_DISPATCH_HANDLER OnSCGetState;
    EVT_DISPATCH_HANDLER OnSCIsAbsent;
    EVT_DISPATCH_HANDLER OnSCIsPresent;
    EVT_DISPATCH_HANDLER OnSCPower;
    EVT_DISPATCH_HANDLER OnSCSetProtocol;
    EVT_DISPATCH_HANDLER OnSCTransmit;
    EVT_DISPATCH_HANDLER OnSCGetLastError;

private:
    typedef struct _DISPATCH_ENTRY
    {
        ULONG IoControlCode;
        size_t MinInputBufferLength;
        size_t MinOutputBufferLength;
        PFN_DISPATCH_HANDLER DispatchHandler;
    }
    DISPATCH_ENTRY, *PDISPATCH_ENTRY;

    NTSTATUS DispatchMessage(
        _In_reads_(TableEntries) const DISPATCH_ENTRY rgDispatchTable[],
        _In_ DWORD TableEntries,
        _In_ WDFREQUEST Request,
        _In_ ULONG IoControlCode,
        _In_ size_t InputBufferLength,
        _In_ size_t OutputBufferLength);
    
    BOOL ValidateMessage(_In_ CFileObject *pFileObject, _In_ ULONG IoControlCode);

    WDFQUEUE             m_Queue;
    LIST_ENTRY           m_SubsList;
    LIST_ENTRY           m_ArrivalSubsList;
    LIST_ENTRY           m_DepartureSubsList;
    CRITICAL_SECTION     m_SubsLock;
    LIST_ENTRY           m_PubsList;
    CRITICAL_SECTION     m_PubsLock;
    LIST_ENTRY           m_ConnectionList;
    CRITICAL_SECTION     m_ConnectionLock;
    CSocketListener      m_SocketListener;
    LIST_ENTRY           m_SecureElementList;
    LIST_ENTRY           m_SEEventsList;
    CRITICAL_SECTION     m_SEEventsLock;
    CFileObject*         m_pSEManager;
    CRITICAL_SECTION     m_SEManagerLock;
    CSmartCardReader     m_SmartCardReader;
    CRITICAL_SECTION     m_pHCEConnectionLock;
    CConnection*         m_pHCEConnection;
    USHORT               m_HCEConnectionId;
    CRITICAL_SECTION     m_RadioLock;
    BOOLEAN              m_NfpRadioState;
    BOOLEAN              m_NfpRadioOffPolicyOverride;
    BOOLEAN              m_NfpRadioOffSystemOverride;
    BOOLEAN              m_SERadioState;
    BOOLEAN              m_NfpInterfaceCreated;
    BOOLEAN              m_ScInterfaceCreated;
    BOOLEAN              m_SEInterfaceCreated;
    CRoutingTable        m_RoutingTable;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CQueue, GetQueueObject);


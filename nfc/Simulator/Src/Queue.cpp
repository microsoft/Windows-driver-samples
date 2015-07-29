/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    queue.cpp

Abstract:

    This file implements the I/O queue interface and performs the ioctl operations.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "Internal.h"
#include "Queue.tmh"

CQueue::CQueue(WDFQUEUE Queue) : 
    m_Queue(Queue), 
    m_SmartCardReader(WdfIoQueueGetDevice(Queue)),
    m_SocketListener(this),
    m_pSEManager(nullptr),
    m_pHCEConnection(nullptr),
    m_NfpRadioState(TRUE),
    m_NfpRadioOffPolicyOverride(FALSE),
    m_NfpRadioOffSystemOverride(FALSE),
    m_SERadioState(TRUE),
    m_NfpInterfaceCreated(FALSE),
    m_ScInterfaceCreated(FALSE),
    m_SEInterfaceCreated(FALSE),
    m_RoutingTable(this),
    m_HCEConnectionId(0)
{
    NT_ASSERT(m_Queue != nullptr);

    InitializeListHead(&m_SubsList);
    InitializeListHead(&m_ArrivalSubsList);
    InitializeListHead(&m_DepartureSubsList);
    InitializeListHead(&m_PubsList);
    InitializeListHead(&m_ConnectionList);
    InitializeListHead(&m_SecureElementList);
    InitializeListHead(&m_SEEventsList);

    InitializeCriticalSection(&m_SubsLock);
    InitializeCriticalSection(&m_PubsLock);
    InitializeCriticalSection(&m_ConnectionLock);
    InitializeCriticalSection(&m_SEManagerLock);
    InitializeCriticalSection(&m_SEEventsLock);
    InitializeCriticalSection(&m_RadioLock);
    InitializeCriticalSection(&m_pHCEConnectionLock);
}

CQueue::~CQueue()
{
    DeleteCriticalSection(&m_SubsLock);
    DeleteCriticalSection(&m_PubsLock);
    DeleteCriticalSection(&m_ConnectionLock);
    DeleteCriticalSection(&m_SEEventsLock);
    DeleteCriticalSection(&m_RadioLock);
    DeleteCriticalSection(&m_pHCEConnectionLock);

    m_Queue = nullptr;
}

NTSTATUS CQueue::Initialize()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WSADATA WsaData = {};

    DECLARE_CONST_UNICODE_STRING(RMNamespace, RM_NAMESPACE);
    DECLARE_CONST_UNICODE_STRING(SCNamespace, SMARTCARD_READER_NAMESPACE);
    DECLARE_CONST_UNICODE_STRING(SimulatorNamespace, SIM_NAMESPACE);

    ReadSettingsFromRegistry();

    if (WSAStartup(MAKEWORD(2,2), &WsaData) != 0) {
        Status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }


    if (!NT_SUCCESS(m_SmartCardReader.Initialize())) {
        Status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }

    if (FAILED(m_SocketListener.Bind())) {
        Status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }

    if (FAILED(m_SocketListener.EnableAccepting())) {
        Status = STATUS_INTERNAL_ERROR;
        goto Exit;
    }

    EnumerateSecureElements();
    m_RoutingTable.Initialize(WdfIoQueueGetDevice(m_Queue));

    // Register for NFP Radio Manager interface
    Status = WdfDeviceCreateDeviceInterface(
                     WdfIoQueueGetDevice(m_Queue),
                     (LPGUID) &GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE,
                     &RMNamespace);

    if (!NT_SUCCESS(Status)) {
        TraceInfo("WdfDeviceCreateDeviceInterface failed with %!STATUS!", Status);
        goto Exit;
    }

    if (IsNfpRadioEnabled()) {
        // Register for Proximity interface
        Status = WdfDeviceCreateDeviceInterface(
                         WdfIoQueueGetDevice(m_Queue),
                         (LPGUID) &GUID_DEVINTERFACE_NFP,
                         nullptr);

        if (!NT_SUCCESS(Status)) {
            TraceInfo("WdfDeviceCreateDeviceInterface failed with Status %!STATUS!", Status);
            goto Exit;
        }

        m_NfpInterfaceCreated = TRUE;

        // Register for Smart Card interface
        Status = WdfDeviceCreateDeviceInterface(
                         WdfIoQueueGetDevice(m_Queue),
                         (LPGUID) &GUID_DEVINTERFACE_SMARTCARD_READER,
                         &SCNamespace);

        if (!NT_SUCCESS(Status)) {
            TraceInfo("WdfDeviceCreateDeviceInterface failed with Status %!STATUS!", Status);
            goto Exit;
        }

        m_ScInterfaceCreated = TRUE;
    }

    if (IsSERadioEnabled()) {
        // Register for Secure Element interface
        Status = WdfDeviceCreateDeviceInterface(
                         WdfIoQueueGetDevice(m_Queue),
                         (LPGUID) &GUID_DEVINTERFACE_NFCSE,
                         nullptr);

        if (!NT_SUCCESS(Status)) {
            TraceInfo("WdfDeviceCreateDeviceInterface failed with Status %!STATUS!", Status);
            goto Exit;
        }

        m_SEInterfaceCreated = TRUE;
    }

    // Register for Simulator interface
    Status = WdfDeviceCreateDeviceInterface(
                     WdfIoQueueGetDevice(m_Queue),
                     (LPGUID) &GUID_DEVINTERFACE_NFCSIM,
                     &SimulatorNamespace);

    if (!NT_SUCCESS(Status)) {
        TraceInfo("WdfDeviceCreateDeviceInterface failed with %!STATUS!", Status);
        goto Exit;
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CQueue::Deinitialize()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    while (!IsListEmpty(&m_SecureElementList)) {
        delete CSecureElement::FromListEntry(RemoveHeadList(&m_SecureElementList));
    }


    m_SocketListener.StopAccepting();
    m_SmartCardReader.Deinitialize();

    while (!IsListEmpty(&m_ConnectionList)) {
        delete CConnection::FromListEntry(RemoveHeadList(&m_ConnectionList));
    }

    WSACleanup();

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

BOOLEAN CQueue::IsNfpRadioEnabled()
{
    BOOLEAN NfpRadioState;

    EnterCriticalSection(&m_RadioLock);
    NfpRadioState = m_NfpRadioState;
    LeaveCriticalSection(&m_RadioLock);

    return NfpRadioState;
}

BOOLEAN CQueue::IsSERadioEnabled()
{
    BOOLEAN SERadioState;

    EnterCriticalSection(&m_RadioLock);
    SERadioState = m_SERadioState;
    LeaveCriticalSection(&m_RadioLock);

    return SERadioState;
}

NTSTATUS CQueue::DetectRole(CFileObject *pFileObject, PCWSTR pszFileName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCWSTR pszProtocol = nullptr;
    PCWSTR pszType = nullptr;

    MethodEntry("pszFileName = %S", pszFileName);

    if (IsStringPrefixed(pszFileName, PUBS_NAMESPACE)) {
        if (IsNfpRadioEnabled()) {
            pFileObject->SetRolePublication();
            pszProtocol = pszFileName + PUBS_NAMESPACE_CHARS;
        }
        else {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }
    else if (IsStringPrefixed(pszFileName, SUBS_NAMESPACE)) {
        if (IsNfpRadioEnabled()) {
            pFileObject->SetRoleSubcription();
            pszProtocol = pszFileName + SUBS_NAMESPACE_CHARS;
        }
        else {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }
    else if (IsStringEqual(pszFileName, SEEVENTS_NAMESPACE)) {
        if (IsSERadioEnabled()) {
            pFileObject->SetRoleSecureElementEvent();
        }
        else {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }
    else if (IsStringEqual(pszFileName, SEMANAGE_NAMESPACE)) {
        if (IsSERadioEnabled()) {
            pFileObject->SetRoleSecureElementManager();
        }
        else {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }
    else if (IsStringEqual(pszFileName, SMARTCARD_READER_NAMESPACE)) {
        if (IsNfpRadioEnabled()) {
            pFileObject->SetRoleSmartCardReader();
        }
        else {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
    }
    else if (IsStringEqual(pszFileName, SIM_NAMESPACE)) {
        pFileObject->SetRoleSimulation();
    }
    else if (IsStringEqual(pszFileName, RM_NAMESPACE)) {
        pFileObject->SetRoleRadioManager();
    }
    else {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    if (NT_SUCCESS(Status) && pszProtocol != nullptr) {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;

        if (IsStringPrefixed(pszProtocol, WINDOWS_PROTOCOL)) {
            pszType = pszProtocol + WINDOWS_PROTOCOL_CHARS;

            if (*pszType == L'.') {
                Status = pFileObject->SetType(pszProtocol);
            }
            else if (*pszType == L':') {
                pszType = pszType + 1;

                if (pFileObject->IsPublication() && IsStringEqual(pszType, WRITETAG_TYPE)) {
                    Status = pFileObject->SetType(pszProtocol);
                }
            }
            else if (IsStringPrefixed(pszType, WINDOWSURI_TYPE)) {
                pszType = pszType + WINDOWSURI_TYPE_CHARS;

                if (*pszType == L'\0') {
                    Status = pFileObject->SetType(pszProtocol);
                }
                else if (*pszType == L':') {
                    pszType = pszType + 1;

                    if (pFileObject->IsPublication() && IsStringEqual(pszType, WRITETAG_TYPE)) {
                        Status = pFileObject->SetType(pszProtocol);
                    }
                }
            }
            else if (IsStringPrefixed(pszType, WINDOWSMIME_TYPE)) {
                pszType = pszType + WINDOWSMIME_TYPE_CHARS;

                if (*pszType == L'\0') {
                    if (pFileObject->IsNormalSubscription()) {
                        Status = pFileObject->SetType(pszProtocol);
                    }
                }
                else if (*pszType == L':') {
                    pszType = pszType + 1;

                    if (pFileObject->IsPublication() && IsStringEqual(pszType, WRITETAG_TYPE)) {
                        Status = pFileObject->SetType(pszProtocol);
                    }
                }
                else if (*pszType == L'.') {
                    Status = pFileObject->SetType(pszProtocol);
                }
            }
        }
        else if (IsStringPrefixed(pszProtocol, NDEF_PROTOCOL)) {
            pszType = pszProtocol + NDEF_PROTOCOL_CHARS;

            if (*pszType == L'\0') {
                Status = pFileObject->SetType(pszProtocol);
            }
            else if (*pszType == L':') {
                pszType = pszType + 1;

                if (pFileObject->IsPublication()) {
                    if (IsStringEqual(pszType, WRITETAG_TYPE)) {
                        Status = pFileObject->SetType(pszProtocol);
                    }
                }
                else {
                    if (IsStringPrefixed(pszType, NDEF_EXT_TYPE) ||
                        IsStringPrefixed(pszType, NDEF_MIME_TYPE) ||
                        IsStringPrefixed(pszType, NDEF_URI_TYPE) ||
                        IsStringPrefixed(pszType, NDEF_WKT_TYPE) ||
                        IsStringPrefixed(pszType, NDEF_UNKNOWN_TYPE)) {
                        Status = pFileObject->SetType(pszProtocol);
                    }
                }
            }
        }
        else if (pFileObject->IsPublication()) {
            if (IsStringEqual(pszProtocol, LAUNCHAPP_WRITETAG_PROTOCOL)) {
                Status = pFileObject->SetType(pszProtocol);
            }
        }
        else if (pFileObject->IsNormalSubscription()) {
            if (IsStringEqual(pszProtocol, DEVICE_ARRIVED)) {
                if (pFileObject->SetRoleArrivedSubcription()) {
                    Status = STATUS_SUCCESS;
                }
            }
            else if (IsStringEqual(pszProtocol, DEVICE_DEPARTED)) {
                if (pFileObject->SetRoleDepartedSubcription()) {
                    Status = STATUS_SUCCESS;
                }
            }
            else if (IsStringEqual(pszProtocol, PAIRING_BLUETOOTH_PROTOCOL) ||
                     IsStringEqual(pszProtocol, PAIRING_UPNP_PROTOCOL) ||
                     IsStringEqual(pszProtocol, WRITEABLETAG_PROTOCOL)) {
                Status = pFileObject->SetType(pszProtocol);
            }
        }
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

void
CQueue::OnFileCreate(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    )
{
    MethodEntry("Request = %p, FileObject = %p", Request, FileObject);

    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING FileName = nullptr;
    PCWSTR pszFileName = nullptr;    
    CFileObject *pFileObject = nullptr;
    
    UNREFERENCED_PARAMETER(Device);

    // Construct file object on the preallocated buffer using placement 'new'
    pFileObject = new (GetFileObject(FileObject)) CFileObject(FileObject);

    if (pFileObject == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    FileName = WdfFileObjectGetFileName(FileObject);

    if ((FileName != nullptr) && (FileName->Length > 0)) {
        NT_ASSERT(FileName->Buffer != nullptr);

        if ((FileName->Length / sizeof(WCHAR)) > MaxCchType) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        pszFileName = FileName->Buffer;
        pszFileName = (pszFileName[0] == L'\\') ? pszFileName+1 : pszFileName;

        Status = DetectRole(pFileObject, pszFileName);

        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }

        TraceInfo("pszFileName = %S pFileObject = %p", pszFileName, pFileObject);
    }

    if (pFileObject->IsNormalSubscription()) {
        EnterCriticalSection(&m_SubsLock);
        InsertHeadList(&m_SubsList, pFileObject->GetListEntry());
        LeaveCriticalSection(&m_SubsLock);
    }
    else if (pFileObject->IsArrivedSubscription()) {
        EnterCriticalSection(&m_SubsLock);
        InsertHeadList(&m_ArrivalSubsList, pFileObject->GetListEntry());
        LeaveCriticalSection(&m_SubsLock);
    }
    else if (pFileObject->IsDepartedSubscription()) {
        EnterCriticalSection(&m_SubsLock);
        InsertHeadList(&m_DepartureSubsList, pFileObject->GetListEntry());
        LeaveCriticalSection(&m_SubsLock);
    }
    else if (pFileObject->IsSmartCardReader()) {
        Status = m_SmartCardReader.AddClient(pFileObject);
    }
    else if (pFileObject->IsSecureElementManager()) {
        EnterCriticalSection(&m_SEManagerLock);
        Status = InterlockedCompareExchangePointer((void**)&m_pSEManager, pFileObject, nullptr) == nullptr ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
        LeaveCriticalSection(&m_SEManagerLock);
    }

Exit:
    TraceInfo("%!FUNC! Completing Request with Status %!STATUS!", Status);
    WdfRequestComplete(Request, Status);

    MethodReturnVoid();
}

void CQueue::OnFileClose(_In_ WDFFILEOBJECT FileObject)
{
    MethodEntry("...");

    CFileObject* pFileObject = GetFileObject(FileObject);
    CRITICAL_SECTION *pLock = nullptr;
    LIST_ENTRY* pHead = nullptr;

    NT_ASSERT(pFileObject != nullptr);

    if (pFileObject->IsNormalSubscription()) {
        pLock = &m_SubsLock;
        pHead = &m_SubsList;
    }
    else if (pFileObject->IsArrivedSubscription()) {
        pLock = &m_SubsLock;
        pHead = &m_ArrivalSubsList;
    }
    else if (pFileObject->IsDepartedSubscription()) {
        pLock = &m_SubsLock;
        pHead = &m_DepartureSubsList;
    }
    else if (pFileObject->IsPublication()) {
        pLock = &m_PubsLock;
        pHead = &m_PubsList;
    }
    else if (pFileObject->IsSmartCardReader()) {
        m_SmartCardReader.RemoveClient(pFileObject);
    }
    else if (pFileObject->IsSecureElementEvent()) {
        pLock = &m_SEEventsLock;
        pHead = &m_SEEventsList;
    }
    else if (pFileObject->IsSecureElementManager()) {
        EnterCriticalSection(&m_SEManagerLock);
        InterlockedCompareExchangePointer((void**)&m_pSEManager, nullptr, pFileObject);
        LeaveCriticalSection(&m_SEManagerLock);

        for (LIST_ENTRY* pEntry = m_SecureElementList.Flink;
             pEntry != &m_SecureElementList;
             pEntry = pEntry->Flink) {
            CSecureElement::FromListEntry(pEntry)->SetEmulationMode(EmulationOff);
        }
    }

    if (pHead != nullptr) {
        EnterCriticalSection(pLock);

        for (LIST_ENTRY* pEntry = pHead->Flink;
             pEntry != pHead;
             pEntry = pEntry->Flink) {
            if (pEntry == pFileObject->GetListEntry()) {
                RemoveEntryList(pEntry);
                break;
            }
        }
        LeaveCriticalSection(pLock);
    }

    MethodReturnVoid();
}

VOID
CQueue::OnIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    FunctionEntry("...");

    CQueue *pQueue = GetQueueObject(Queue);

    NT_ASSERT(pQueue != nullptr);
    pQueue->OnIoDeviceControl(Request, IoControlCode, InputBufferLength, OutputBufferLength);

    FunctionReturnVoid();
}

void
CQueue::OnIoDeviceControl(
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("Request = %p, IoControlCode = 0x%x InputBufferLength=%d OutputBufferLength=%d",
                 Request, IoControlCode, (DWORD)InputBufferLength, (DWORD)OutputBufferLength);

    NTSTATUS Status = STATUS_INVALID_DEVICE_STATE;

    switch (IoControlCode)
    {
        case IOCTL_NFP_GET_MAX_MESSAGE_BYTES:
        case IOCTL_NFP_GET_KILO_BYTES_PER_SECOND:
        case IOCTL_NFP_DISABLE:
        case IOCTL_NFP_ENABLE:
        case IOCTL_NFP_SET_PAYLOAD:
        case IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE:
        case IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE:
        case IOCTL_NFCSIM_BEGIN_PROXIMITY:
        {
            const DISPATCH_ENTRY DispatchTable[] =
            {
                { IOCTL_NFP_ENABLE,                         0,                              0,                  &CQueue::OnNfpEnable },
                { IOCTL_NFP_DISABLE,                        0,                              0,                  &CQueue::OnNfpDisable },
                { IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE,    0,                              sizeof(DWORD),      &CQueue::OnNfpGetNextSubscribedMessage },
                { IOCTL_NFP_SET_PAYLOAD,                    MinCbPayload,                   0,                  &CQueue::OnNfpSetPayload },
                { IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE,   0,                              0,                  &CQueue::OnNfpGetNextTransmittedMessage },
                { IOCTL_NFP_GET_MAX_MESSAGE_BYTES,          0,                              sizeof(DWORD),      &CQueue::OnNfpGetMaxMessageBytes },
                { IOCTL_NFP_GET_KILO_BYTES_PER_SECOND,      0,                              sizeof(DWORD),      &CQueue::OnNfpGetTransmissionRateKbps },
                { IOCTL_NFCSIM_BEGIN_PROXIMITY,             sizeof(BEGIN_PROXIMITY_ARGS),   0,                  &CQueue::OnNfpBeginProximity },
            };

            if (IsNfpRadioEnabled()) {
                Status = DispatchMessage(DispatchTable, _countof(DispatchTable), Request, IoControlCode, InputBufferLength, OutputBufferLength);
            }
        }
        break;

        case IOCTL_NFCSE_ENUM_ENDPOINTS:
        case IOCTL_NFCSE_SUBSCRIBE_FOR_EVENT:
        case IOCTL_NFCSE_GET_NEXT_EVENT:
        case IOCTL_NFCSE_SET_CARD_EMULATION_MODE:
        case IOCTL_NFCSIM_TRIGGER_SEEVENT:
        case IOCTL_NFCSE_GET_NFCC_CAPABILITIES:
        case IOCTL_NFCSE_GET_ROUTING_TABLE:
        case IOCTL_NFCSE_SET_ROUTING_TABLE:
        case IOCTL_NFCSE_HCE_REMOTE_RECV:
        case IOCTL_NFCSE_HCE_REMOTE_SEND:
        {
            const DISPATCH_ENTRY DispatchTable[] =
            {
                { IOCTL_NFCSE_SET_CARD_EMULATION_MODE,  sizeof(SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO),    0,                                              &CQueue::OnSESetCardEmulationMode },
                { IOCTL_NFCSE_GET_NEXT_EVENT,           0,                                                      sizeof(DWORD),                                  &CQueue::OnSEGetNextEvent },
                { IOCTL_NFCSE_SUBSCRIBE_FOR_EVENT,      sizeof(SECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO),         0,                                              &CQueue::OnSESubscribeForEvent },
                { IOCTL_NFCSE_ENUM_ENDPOINTS,           0,                                                      sizeof(DWORD),                                  &CQueue::OnSEEnumEndpoints },
                { IOCTL_NFCSE_GET_NFCC_CAPABILITIES,    0,                                                      sizeof(SECURE_ELEMENT_NFCC_CAPABILITIES),       &CQueue::OnSEGetNfccCapabilities },
                { IOCTL_NFCSE_GET_ROUTING_TABLE,        0,                                                      sizeof(DWORD),                                  &CQueue::OnSEGetRoutingTable },
                { IOCTL_NFCSE_SET_ROUTING_TABLE,        sizeof(SECURE_ELEMENT_ROUTING_TABLE),                   0,                                              &CQueue::OnSESetRoutingTable },
                { IOCTL_NFCSE_HCE_REMOTE_RECV,          0,                                                      sizeof(DWORD),                                  &CQueue::OnSEHCERemoteRecv },
                { IOCTL_NFCSE_HCE_REMOTE_SEND,          2 * sizeof(USHORT) + 2,                                 0,                                              &CQueue::OnSEHCERemoteSend },
                { IOCTL_NFCSIM_TRIGGER_SEEVENT,         SECURE_ELEMENT_EVENT_INFO_HEADER,                       0,                                              &CQueue::OnSETriggerEvent },
            };

            if (IsSERadioEnabled()) {
                Status = DispatchMessage(DispatchTable, _countof(DispatchTable), Request, IoControlCode, InputBufferLength, OutputBufferLength);
            }
        }
        break;

        case IOCTL_SMARTCARD_EJECT:
        case IOCTL_SMARTCARD_GET_LAST_ERROR:
        case IOCTL_SMARTCARD_SET_ATTRIBUTE:
        case IOCTL_SMARTCARD_SWALLOW:
        case IOCTL_SMARTCARD_GET_ATTRIBUTE:
        case IOCTL_SMARTCARD_GET_STATE:
        case IOCTL_SMARTCARD_IS_ABSENT:
        case IOCTL_SMARTCARD_IS_PRESENT:
        case IOCTL_SMARTCARD_POWER:
        case IOCTL_SMARTCARD_SET_PROTOCOL:
        case IOCTL_SMARTCARD_TRANSMIT:
        {
            const DISPATCH_ENTRY DispatchTable[] =
            {
                { IOCTL_SMARTCARD_GET_ATTRIBUTE,    sizeof(DWORD),              0,                          &CQueue::OnSCGetAttribute },
                { IOCTL_SMARTCARD_SET_ATTRIBUTE,    sizeof(DWORD),              0,                          &CQueue::OnSCSetAttribute },
                { IOCTL_SMARTCARD_GET_STATE,        0,                          sizeof(DWORD),              &CQueue::OnSCGetState },
                { IOCTL_SMARTCARD_POWER,            sizeof(DWORD),              0,                          &CQueue::OnSCPower },
                { IOCTL_SMARTCARD_SET_PROTOCOL,     sizeof(DWORD),              sizeof(DWORD),              &CQueue::OnSCSetProtocol },
                { IOCTL_SMARTCARD_IS_ABSENT,        0,                          0,                          &CQueue::OnSCIsAbsent },
                { IOCTL_SMARTCARD_IS_PRESENT,       0,                          0,                          &CQueue::OnSCIsPresent },             
                { IOCTL_SMARTCARD_TRANSMIT,         sizeof(SCARD_IO_REQUEST)+1, sizeof(SCARD_IO_REQUEST)+2, &CQueue::OnSCTransmit },
                { IOCTL_SMARTCARD_GET_LAST_ERROR,   0,                          0,                          &CQueue::OnSCGetLastError },
            };

            if (IsNfpRadioEnabled()) {
                Status = DispatchMessage(DispatchTable, _countof(DispatchTable), Request, IoControlCode, InputBufferLength, OutputBufferLength);
            }
        }
        break;

        case IOCTL_NFCRM_SET_RADIO_STATE:
        case IOCTL_NFCRM_QUERY_RADIO_STATE:
        case IOCTL_NFCSERM_SET_RADIO_STATE:
        case IOCTL_NFCSERM_QUERY_RADIO_STATE:
        {
            const DISPATCH_ENTRY DispatchTable[] =
            {
                { IOCTL_NFCRM_SET_RADIO_STATE,      sizeof(NFCRM_SET_RADIO_STATE),  0,                          &CQueue::OnNfpSetRadioState },
                { IOCTL_NFCRM_QUERY_RADIO_STATE,    0,                              sizeof(NFCRM_RADIO_STATE),  &CQueue::OnNfpQueryRadioState },
            };

            Status = DispatchMessage(DispatchTable, _countof(DispatchTable), Request, IoControlCode, InputBufferLength, OutputBufferLength);
        }
        break;

        default:
        {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
    }

    if (Status != STATUS_PENDING) {
        TraceInfo("%!FUNC! Completing Request with Status %!STATUS!", Status);
        WdfRequestComplete(Request, Status);
    }
    
    MethodReturnVoid();
}

BOOL CQueue::ValidateMessage(_In_ CFileObject *pFileObject, _In_ ULONG IoControlCode)
{
    switch (IoControlCode)
    {
    case IOCTL_NFP_GET_MAX_MESSAGE_BYTES:
    case IOCTL_NFP_GET_KILO_BYTES_PER_SECOND:
        {
            return TRUE;
        }

    case IOCTL_NFP_DISABLE:
    case IOCTL_NFP_ENABLE:
        {
            return (pFileObject->IsPublication() || pFileObject->IsSubscription());
        }

    case IOCTL_NFP_SET_PAYLOAD:
    case IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE:
        {
            return pFileObject->IsPublication();
        }

    case IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE:
        {
            return pFileObject->IsSubscription();
        }

    case IOCTL_NFCSE_GET_NFCC_CAPABILITIES:
    case IOCTL_NFCSE_ENUM_ENDPOINTS:
        {
            return TRUE;
        }

    case IOCTL_NFCSE_GET_NEXT_EVENT:
    case IOCTL_NFCSE_SUBSCRIBE_FOR_EVENT:
        {
            return pFileObject->IsSecureElementEvent();
        }

    case IOCTL_NFCSE_GET_ROUTING_TABLE:
    case IOCTL_NFCSE_SET_ROUTING_TABLE:
    case IOCTL_NFCSE_HCE_REMOTE_SEND:
    case IOCTL_NFCSE_HCE_REMOTE_RECV:
    case IOCTL_NFCSE_SET_CARD_EMULATION_MODE:
        {
            return pFileObject->IsSecureElementManager();
        }

    case IOCTL_SMARTCARD_EJECT:
    case IOCTL_SMARTCARD_GET_LAST_ERROR:
    case IOCTL_SMARTCARD_SET_ATTRIBUTE:
    case IOCTL_SMARTCARD_SWALLOW:
    case IOCTL_SMARTCARD_GET_ATTRIBUTE:
    case IOCTL_SMARTCARD_GET_STATE:
    case IOCTL_SMARTCARD_IS_ABSENT:
    case IOCTL_SMARTCARD_IS_PRESENT:
    case IOCTL_SMARTCARD_POWER:
    case IOCTL_SMARTCARD_SET_PROTOCOL:
    case IOCTL_SMARTCARD_TRANSMIT:
        {
            return pFileObject->IsSmartCardReader();
        }

    case IOCTL_NFCRM_SET_RADIO_STATE:
    case IOCTL_NFCRM_QUERY_RADIO_STATE:
        {
            return pFileObject->IsRoleRadioManager();
        }

    case IOCTL_NFCSIM_BEGIN_PROXIMITY:
    case IOCTL_NFCSIM_TRIGGER_SEEVENT:
        {
            return pFileObject->IsRoleSimulation();
        }
    }

    return FALSE;
}

NTSTATUS
CQueue::DispatchMessage(
    _In_reads_(TableEntries) const DISPATCH_ENTRY rgDispatchTable[],
    _In_ DWORD TableEntries,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_ size_t InputBufferLength,
    _In_ size_t OutputBufferLength
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PVOID InputBuffer = nullptr;
    PVOID OutputBuffer = nullptr;
    CFileObject *pFileObject = GetFileObject(WdfRequestGetFileObject(Request));

    for (DWORD TableEntry = 0; TableEntry < TableEntries; TableEntry++) {
        if (rgDispatchTable[TableEntry].IoControlCode != IoControlCode)
            continue;

        if (InputBufferLength >= rgDispatchTable[TableEntry].MinInputBufferLength &&
            OutputBufferLength >= rgDispatchTable[TableEntry].MinOutputBufferLength) {
            if (InputBufferLength != 0) {
                Status = WdfRequestRetrieveInputBuffer(Request, 0, &InputBuffer, nullptr);
                NT_ASSERT(NT_SUCCESS(Status));
            }

            if (OutputBufferLength != 0) {
                Status = WdfRequestRetrieveOutputBuffer(Request, 0, &OutputBuffer, nullptr);
                NT_ASSERT(NT_SUCCESS(Status));
            }

            if (ValidateMessage(pFileObject, IoControlCode)) {
                Status = (this->*rgDispatchTable[TableEntry].DispatchHandler)(
                                    pFileObject,
                                    Request,
                                    InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,
                                    OutputBufferLength);
            }
            else {
                Status = STATUS_INVALID_DEVICE_STATE;
            }
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
        break;
    }

    return Status;
}

NTSTATUS
CQueue::OnNfpEnable(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = pFileObject->Enable();

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpDisable(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = pFileObject->Disable();

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpSetPayload(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (InputBufferLength > MaxCbPayload) {
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    Status = pFileObject->SetPayload((DWORD)InputBufferLength, (PBYTE)InputBuffer);

    if (NT_SUCCESS(Status)) {
        MESSAGE* pMessage = new MESSAGE();

        if (pMessage != nullptr) {
            pMessage->Initialize(pFileObject->GetType(), pFileObject->GetSize(), pFileObject->GetPayload());

            EnterCriticalSection(&m_ConnectionLock);

            for (LIST_ENTRY* pEntry = m_ConnectionList.Flink; 
                 pEntry != &m_ConnectionList;
                 pEntry = pEntry->Flink) {
                CConnection* pConnection = CConnection::FromListEntry(pEntry);

                if (SUCCEEDED(pConnection->TransmitMessage(pMessage))) {
                    pFileObject->HandleMessageTransmitted();
                }
            }
            LeaveCriticalSection(&m_ConnectionLock);
            
            EnterCriticalSection(&m_PubsLock);
            InsertHeadList(&m_PubsList, pFileObject->GetListEntry());
            LeaveCriticalSection(&m_PubsLock);

            delete pMessage;
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpGetNextSubscribedMessage(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = pFileObject->GetNextSubscribedMessage(Request);
    fCompleteRequest = !NT_SUCCESS(Status);

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpGetNextTransmittedMessage(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = pFileObject->GetNextTransmittedMessage(Request);
    fCompleteRequest = !NT_SUCCESS(Status);

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpGetMaxMessageBytes(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    DWORD *pcbMaxPayload = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Since the output buffer has already been validated
    // to contain 4 bytes, we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    *pcbMaxPayload = MaxCbPayload;
    WdfRequestCompleteWithInformation(Request, Status, sizeof(DWORD));
    fCompleteRequest = FALSE;

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpGetTransmissionRateKbps(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    DWORD *pdwKilobytesPerSecond = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Since the output buffer has already been validated
    // to contain 4 bytes, we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    *pdwKilobytesPerSecond = KilobytesPerSecond;
    WdfRequestCompleteWithInformation(Request, Status, sizeof(DWORD));
    fCompleteRequest = FALSE;

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpBeginProximity(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BEGIN_PROXIMITY_ARGS *pArgs = (BEGIN_PROXIMITY_ARGS*)InputBuffer;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(BEGIN_PROXIMITY_ARGS) <= InputBufferLength);

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = pFileObject->BeginProximity(pArgs, this);
    
Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpSetRadioState(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    PNFCRM_SET_RADIO_STATE pRadioState = (PNFCRM_SET_RADIO_STATE)InputBuffer;
    BOOLEAN NfpRadioState;
    DECLARE_CONST_UNICODE_STRING(SCNamespace, SMARTCARD_READER_NAMESPACE);

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    _Analysis_assume_(sizeof(NFCRM_SET_RADIO_STATE) <= InputBufferLength);

    EnterCriticalSection(&m_RadioLock);

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (pRadioState->SystemStateUpdate) {
        m_NfpRadioOffSystemOverride = !pRadioState->MediaRadioOn;
    }
    else {
        //
        // Since the request is modifying the radio state independent of the 
        // system state, set the system state override to FALSE
        //
        m_NfpRadioOffSystemOverride = FALSE;
        m_NfpRadioOffPolicyOverride = !pRadioState->MediaRadioOn;
    }

    NfpRadioState = !m_NfpRadioOffSystemOverride && !m_NfpRadioOffPolicyOverride;

    if (NfpRadioState == m_NfpRadioState) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    if (NfpRadioState) {
        if (!m_NfpInterfaceCreated) {
            // Register for Proximity interface
            Status = WdfDeviceCreateDeviceInterface(
                             WdfIoQueueGetDevice(m_Queue),
                             (LPGUID) &GUID_DEVINTERFACE_NFP,
                             nullptr);

            if (!NT_SUCCESS(Status)) {
                TraceInfo("WdfDeviceCreateDeviceInterface failed with Status %!STATUS!", Status);
                goto Exit;
            }

            m_NfpInterfaceCreated = TRUE;
        }

        WdfDeviceSetDeviceInterfaceState(
                 WdfIoQueueGetDevice(m_Queue),
                 (LPGUID) &GUID_DEVINTERFACE_NFP,
                 nullptr,
                 TRUE);

        if (!m_ScInterfaceCreated) {
            // Register for Smart Card interface
            Status = WdfDeviceCreateDeviceInterface(
                             WdfIoQueueGetDevice(m_Queue),
                             (LPGUID) &GUID_DEVINTERFACE_SMARTCARD_READER,
                             &SCNamespace);

            if (!NT_SUCCESS(Status)) {
                TraceInfo("WdfDeviceCreateDeviceInterface failed with Status %!STATUS!", Status);
                goto Exit;
            }

            m_ScInterfaceCreated = TRUE;
        }

        WdfDeviceSetDeviceInterfaceState(
                 WdfIoQueueGetDevice(m_Queue),
                 (LPGUID) &GUID_DEVINTERFACE_SMARTCARD_READER,
                 &SCNamespace,
                 TRUE);
    }
    else {
        if (m_NfpInterfaceCreated) {
            WdfDeviceSetDeviceInterfaceState(
                     WdfIoQueueGetDevice(m_Queue),
                     (LPGUID) &GUID_DEVINTERFACE_NFP,
                     nullptr,
                     FALSE);
        }

        if (m_ScInterfaceCreated) {
            WdfDeviceSetDeviceInterfaceState(
                 WdfIoQueueGetDevice(m_Queue),
                 (LPGUID) &GUID_DEVINTERFACE_SMARTCARD_READER,
                 &SCNamespace,
                 FALSE);
        }
    }

    m_NfpRadioState = NfpRadioState;
    
Exit:
    WriteSettingsToRegistry();
    LeaveCriticalSection(&m_RadioLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnNfpQueryRadioState(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    PNFCRM_RADIO_STATE pRadioState = (PNFCRM_RADIO_STATE)OutputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(sizeof(NFCRM_RADIO_STATE) <= OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    pRadioState->MediaRadioOn = IsNfpRadioEnabled();
    WdfRequestCompleteWithInformation(Request, Status, sizeof(NFCRM_RADIO_STATE));
    fCompleteRequest = FALSE;
    
Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSEEnumEndpoints(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    SECURE_ELEMENT_ENDPOINT_INFO rgEndpointInfo[MaxSecureElements] = {};
    SECURE_ELEMENT_ENDPOINT_LIST *pEndpointList = (SECURE_ELEMENT_ENDPOINT_LIST*)OutputBuffer;
    DWORD cbOutputBuffer = 0;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);

    //
    // Since the output buffer has already been validated
    // for output buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    pEndpointList->NumberOfEndpoints = 0;

    for (LIST_ENTRY* pEntry = m_SecureElementList.Flink;
         pEntry != &m_SecureElementList && pEndpointList->NumberOfEndpoints < _countof(rgEndpointInfo);
         pEntry = pEntry->Flink) {
        SECURE_ELEMENT_ENDPOINT_INFO Info;

        Info.guidSecureElementId = CSecureElement::FromListEntry(pEntry)->GetIdentifier();
        Info.eSecureElementType = CSecureElement::FromListEntry(pEntry)->GetType();

        rgEndpointInfo[pEndpointList->NumberOfEndpoints++] = Info;
    }

    cbOutputBuffer = sizeof(DWORD);

    if ((cbOutputBuffer + (pEndpointList->NumberOfEndpoints * sizeof(SECURE_ELEMENT_ENDPOINT_INFO))) <= OutputBufferLength) {
        RtlCopyMemory(pEndpointList->EndpointList, rgEndpointInfo, pEndpointList->NumberOfEndpoints * sizeof(SECURE_ELEMENT_ENDPOINT_INFO));
        cbOutputBuffer += (pEndpointList->NumberOfEndpoints * sizeof(SECURE_ELEMENT_ENDPOINT_INFO));
    }
    else {
        // Returning this signals to the client to send a bigger buffer
        Status = STATUS_BUFFER_OVERFLOW;
    }

    WdfRequestCompleteWithInformation(Request, Status, cbOutputBuffer);
    fCompleteRequest = FALSE;

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSESubscribeForEvent(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    SECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO *pInfo = (SECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO*)InputBuffer;
    CSecureElement *pSecureElement = nullptr;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_EVENT_SUBSCRIPTION_INFO) <= InputBufferLength);

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (IsEqualGUID(pInfo->guidSecureElementId, GUID_NULL) ||
        NT_SUCCESS(Status = GetSecureElementObject(pInfo->guidSecureElementId, &pSecureElement))) {
        Status = pFileObject->SubscribeForEvent(pInfo->guidSecureElementId, pInfo->eEventType);

        if (NT_SUCCESS(Status)) {
            EnterCriticalSection(&m_SEEventsLock);
            InsertHeadList(&m_SEEventsList, pFileObject->GetListEntry());
            LeaveCriticalSection(&m_SEEventsLock);
        }
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSEGetNextEvent(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = pFileObject->GetNextSecureElementPayload(Request);
    fCompleteRequest = !NT_SUCCESS(Status);

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSESetCardEmulationMode(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO *pMode = (SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO*)InputBuffer;
    CSecureElement *pSecureElement = nullptr;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_SET_CARD_EMULATION_MODE_INFO) <= InputBufferLength);

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = GetSecureElementObject(pMode->guidSecureElementId, &pSecureElement);

    if (NT_SUCCESS(Status)) {
        Status = pSecureElement->SetEmulationMode(pMode->eMode);
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSETriggerEvent(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    SECURE_ELEMENT_EVENT_INFO *pInfo = (SECURE_ELEMENT_EVENT_INFO*)InputBuffer;
    CSecureElement *pSecureElement = nullptr;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(SECURE_ELEMENT_EVENT_INFO_HEADER <= InputBufferLength);

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (pInfo->guidSecureElementId == GUID_NULL) {
        for (LIST_ENTRY* pEntry = m_SecureElementList.Flink;
             pEntry != &m_SecureElementList;
             pEntry = pEntry->Flink) {
            pInfo->guidSecureElementId = CSecureElement::FromListEntry(pEntry)->GetIdentifier();
            HandleSecureElementEvent(pInfo);
        }
    }
    else {
        Status = GetSecureElementObject(pInfo->guidSecureElementId, &pSecureElement);

        if (NT_SUCCESS(Status)) {
            HandleSecureElementEvent(pInfo);
        }
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSEGetNfccCapabilities(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    PSECURE_ELEMENT_NFCC_CAPABILITIES pCapabilities = (PSECURE_ELEMENT_NFCC_CAPABILITIES)OutputBuffer;
    BOOL fCompleteRequest = TRUE;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_NFCC_CAPABILITIES) <= OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    pCapabilities->cbMaxRoutingTableSize = m_RoutingTable.MaxRoutingTableSize();
    pCapabilities->IsAidRoutingSupported = m_RoutingTable.IsAidRoutingSupported();
    pCapabilities->IsProtocolRoutingSupported = m_RoutingTable.IsProtocolRoutingSupported();
    pCapabilities->IsTechRoutingSupported = m_RoutingTable.IsTechRoutingSupported();

    WdfRequestCompleteWithInformation(Request, Status, sizeof(SECURE_ELEMENT_NFCC_CAPABILITIES));
    fCompleteRequest = FALSE;

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSESetRoutingTable(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable = (PSECURE_ELEMENT_ROUTING_TABLE)InputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(SECURE_ELEMENT_ROUTING_TABLE) <= InputBufferLength);

    if (pRoutingTable->NumberOfEntries == 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((sizeof(SECURE_ELEMENT_ROUTING_TABLE) + (pRoutingTable->NumberOfEntries - 1) * sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY)) < InputBufferLength) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = m_RoutingTable.SetRoutingTable(pRoutingTable);

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSEGetRoutingTable(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    DWORD cbOutputBuffer = 0;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBuffer);

    //
    // Since the output buffer has already been validated
    // for output buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = m_RoutingTable.GetRoutingTable(OutputBuffer, OutputBufferLength, &cbOutputBuffer);
    WdfRequestCompleteWithInformation(Request, Status, cbOutputBuffer);
    fCompleteRequest = FALSE;

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSEHCERemoteRecv(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    GUID guidSecureElementId;
    BOOL fCompleteRequest = TRUE;

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!NT_SUCCESS(GetSecureElementId(DeviceHost, &guidSecureElementId))) {
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    Status = pFileObject->GetNextSecureElementPayload(Request);
    fCompleteRequest = !NT_SUCCESS(Status);

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSEHCERemoteSend(
    _In_ CFileObject *pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    GUID guidSecureElementId;
    MESSAGE* pMessage = nullptr;
    PSECURE_ELEMENT_HCE_DATA_PACKET pDataPacket = (PSECURE_ELEMENT_HCE_DATA_PACKET)InputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    //
    // Since the input buffer has already been validated
    // for input buffer length so we know we can safely make this assumption
    //
    _Analysis_assume_(2 * sizeof(USHORT) + 2 <= InputBufferLength);

    EnterCriticalSection(&m_pHCEConnectionLock);

    if (OutputBufferLength != 0 ||
        pDataPacket->bConnectionId != m_HCEConnectionId) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (pDataPacket->cbPayload > MaxCbPayload) {
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    if (!NT_SUCCESS(GetSecureElementId(DeviceHost, &guidSecureElementId))) {
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if (m_pHCEConnection == nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    pMessage = new MESSAGE();

    if (pMessage == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    pMessage->Initialize(HCE_MESSAGE_TYPE_TRANSMIT, pDataPacket->cbPayload, pDataPacket->pbPayload);

    if (FAILED(m_pHCEConnection->TransmitMessage(pMessage))) {
        Status = STATUS_DATA_ERROR;
        goto Exit;
    }

Exit:
    SAFE_DELETE(pMessage);
    LeaveCriticalSection(&m_pHCEConnectionLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCGetLastError(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    DWORD *pdwError = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    *pdwError = STATUS_SUCCESS;
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(DWORD));
    
    MethodReturn(STATUS_PENDING, "Status = %!STATUS!", STATUS_SUCCESS);
}

NTSTATUS
CQueue::OnSCGetAttribute(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    DWORD *pdwAttributeId = (DWORD*)InputBuffer;
    DWORD cbOutputBuffer = (DWORD)OutputBufferLength;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBufferLength);

    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    Status = m_SmartCardReader.GetAttribute(*pdwAttributeId, (PBYTE)OutputBuffer, &cbOutputBuffer);

    if (NT_SUCCESS(Status)) {
        WdfRequestCompleteWithInformation(Request, Status, cbOutputBuffer);
        fCompleteRequest = FALSE;
    }
    
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCSetAttribute(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD *pdwAttributeId = (DWORD*)InputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);

    Status = m_SmartCardReader.SetAttribute(*pdwAttributeId);
    
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCGetState(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD *pdwState = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    *pdwState = m_SmartCardReader.GetState();
    WdfRequestCompleteWithInformation(Request, Status, sizeof(DWORD));
    
    MethodReturn(STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCIsAbsent(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = m_SmartCardReader.IsAbsent(Request);

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCIsPresent(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(OutputBuffer);

    if (InputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (OutputBufferLength != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = m_SmartCardReader.IsPresent(Request);
    
Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCPower(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD *pdwPower = (DWORD*)InputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);

    Status = m_SmartCardReader.SetPower(*pdwPower);

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCSetProtocol(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    DWORD *pdwProtocol = (DWORD*)InputBuffer;
    DWORD *pdwSelectedProtocol = (DWORD*)OutputBuffer;

    UNREFERENCED_PARAMETER(pFileObject);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    _Analysis_assume_(sizeof(DWORD) <= InputBufferLength);
    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    Status = m_SmartCardReader.SetProtocol(*pdwProtocol, pdwSelectedProtocol);

    if (NT_SUCCESS(Status)) {
        WdfRequestCompleteWithInformation(Request, Status, sizeof(DWORD));
        fCompleteRequest = FALSE;
    }
    
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CQueue::OnSCTransmit(
    _In_ CFileObject* pFileObject,
    _In_ WDFREQUEST Request,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fCompleteRequest = TRUE;
    size_t BytesTransferred = 0;

    UNREFERENCED_PARAMETER(pFileObject);

    //
    // Since the input and buffer buffer length has already been validated
    // for input buffer length we need to have atleast one byte to transfer (discarding the header)
    // for output buffer length we should atleast be able to store status codes SW1+SW2
    //
    _Analysis_assume_(sizeof(SCARD_IO_REQUEST)+1 <= InputBufferLength);
    _Analysis_assume_(sizeof(SCARD_IO_REQUEST)+2 <= OutputBufferLength);

    Status = m_SmartCardReader.Transmit((PBYTE)InputBuffer, InputBufferLength, (PBYTE)OutputBuffer, OutputBufferLength, &BytesTransferred);

    if (NT_SUCCESS(Status)) {
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, BytesTransferred);
        fCompleteRequest = FALSE;
    }
    
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

void CQueue::ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket)
{
    MethodEntry("...");

    CConnection* pConnection;

    if (CConnection::Create(this, &pConnection)) {
        // Mark it as an inbound connection, so we know to delete it when it's removed from the list
        pConnection->SetInboundConnection();
        pConnection->ValidateAccept(Socket, pMagicPacket);

        Socket = INVALID_SOCKET;
    }

    if (Socket != INVALID_SOCKET) {
        closesocket(Socket);
    }

    MethodReturnVoid();
}

void CQueue::ConnectionEstablished(_In_ CConnection* pConnection)
{
    MethodEntry("...");

    if (pConnection->GetConnectionType() == CONNECTION_TYPE_P2P) {
        EnterCriticalSection(&m_ConnectionLock);

        if (IsListEmpty(&m_ConnectionList)) {
            HandleArrivalEvent();
        }

        InsertHeadList(&m_ConnectionList, pConnection->GetListEntry());
        LeaveCriticalSection(&m_ConnectionLock);
    
        MESSAGE* pMessage = new MESSAGE();

        if (pMessage != nullptr) {
            EnterCriticalSection(&m_PubsLock);

            for (LIST_ENTRY* pEntry = m_PubsList.Flink;
                 pEntry != &m_PubsList;
                 pEntry = pEntry->Flink) {
                CFileObject* pPub = CFileObject::FromListEntry(pEntry);

                if (pPub->IsEnabled()) {
                    pMessage->Initialize(pPub->GetType(), pPub->GetSize(), pPub->GetPayload());

                    if (SUCCEEDED(pConnection->TransmitMessage(pMessage))) {
                        pPub->HandleMessageTransmitted();
                    }
                }
            }
            LeaveCriticalSection(&m_PubsLock);

            delete pMessage;
        }
    }
    else if (pConnection->GetConnectionType() == CONNECTION_TYPE_TAG) {
        m_SmartCardReader.CardArrived(pConnection);
    }
    else if (pConnection->GetConnectionType() == CONNECTION_TYPE_HCE) {
        SECURE_ELEMENT_EVENT_INFO_AND_PAYLOAD(sizeof(SECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD)) EventInfo;

        if (NT_SUCCESS(GetSecureElementId(DeviceHost, &EventInfo.Info.guidSecureElementId))) {
            EnterCriticalSection(&m_pHCEConnectionLock);

            if (m_pHCEConnection == nullptr) {
                PSECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD pbEventPayload = NULL;

                m_pHCEConnection = pConnection;
                m_HCEConnectionId++;

                EventInfo.Info.eEventType = HceActivated;
                EventInfo.Info.cbEventData = sizeof(SECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD);

                pbEventPayload = (PSECURE_ELEMENT_HCE_ACTIVATION_PAYLOAD)EventInfo.Info.pbEventData;

                pbEventPayload->bConnectionId = m_HCEConnectionId;
                pbEventPayload->eRfTechType = NFC_RF_TECHNOLOGY_A;
                pbEventPayload->eRfProtocolType = PROTOCOL_ISO_DEP;

                HandleSecureElementEvent(&EventInfo.Info);
            }

            LeaveCriticalSection(&m_pHCEConnectionLock);
        }
    }

    MethodReturnVoid();
}

BOOL CQueue::ConnectionTerminated(_In_ CConnection* pConnection)
{
    MethodEntry("pConnection = 0x%p", pConnection);

    BOOL fConnectionDeleted = FALSE;

    if (pConnection->GetConnectionType() == CONNECTION_TYPE_P2P) {
        LIST_ENTRY* pRemoveListEntry = pConnection->GetListEntry();
    
        EnterCriticalSection(&m_ConnectionLock);

        for (LIST_ENTRY* pEntry = m_ConnectionList.Flink;
             pEntry != &m_ConnectionList;
             pEntry = pEntry->Flink) {
            if (pEntry == pRemoveListEntry) {
                RemoveEntryList(pEntry);
                break;
            }
        }

        if (IsListEmpty(&m_ConnectionList)) {
            HandleRemovalEvent();
        }
        LeaveCriticalSection(&m_ConnectionLock);

        if (pConnection->IsInboundConnection()) {
            delete pConnection;
            fConnectionDeleted = TRUE;
        }
    }
    else if (pConnection->GetConnectionType() == CONNECTION_TYPE_TAG) {
        fConnectionDeleted = m_SmartCardReader.CardRemoved(pConnection);
    }
    else if (pConnection->GetConnectionType() == CONNECTION_TYPE_HCE) {
        SECURE_ELEMENT_EVENT_INFO_AND_PAYLOAD(sizeof(USHORT)) EventInfo;

        if (NT_SUCCESS(GetSecureElementId(DeviceHost, &EventInfo.Info.guidSecureElementId))) {
            EnterCriticalSection(&m_pHCEConnectionLock);

            if (m_pHCEConnection != nullptr) {
                m_pHCEConnection = nullptr;

                EventInfo.Info.eEventType = HceDeactivated;
                EventInfo.Info.cbEventData = sizeof(USHORT);

                *((USHORT*)EventInfo.Info.pbEventData) = m_HCEConnectionId;

                HandleSecureElementEvent(&EventInfo.Info);
            }
            LeaveCriticalSection(&m_pHCEConnectionLock);
        }
    }
    
    MethodReturnBool(fConnectionDeleted);
}

void CQueue::HandleArrivalEvent()
{
    MethodEntry("void");
    
    EnterCriticalSection(&m_SubsLock);
    
    for (LIST_ENTRY* pEntry = m_ArrivalSubsList.Flink;
         pEntry != &m_ArrivalSubsList;
         pEntry = pEntry->Flink) {
        CFileObject::FromListEntry(pEntry)->HandleArrivalEvent();
    }
    LeaveCriticalSection(&m_SubsLock);

    MethodReturnVoid();
}

void CQueue::HandleRemovalEvent()
{
    MethodEntry("void");
    
    EnterCriticalSection(&m_SubsLock);
    
    for (LIST_ENTRY* pEntry = m_DepartureSubsList.Flink;
         pEntry != &m_DepartureSubsList;
         pEntry = pEntry->Flink) {
        CFileObject::FromListEntry(pEntry)->HandleRemovalEvent();
    }
    LeaveCriticalSection(&m_SubsLock);

    MethodReturnVoid();
}

void CQueue::HandleReceivedMessage(_In_ CONNECTION_TYPE ConnType, _In_ MESSAGE* pMessage)
{
    MethodEntry("pMessage->m_szType = '%S'", pMessage->m_szType);

    if (ConnType == CONNECTION_TYPE_P2P) {
        if ((pMessage->m_cbPayload > 0) && (pMessage->m_cbPayload <= MaxCbPayload)) {
            EnterCriticalSection(&m_SubsLock);

            for (LIST_ENTRY* pEntry = m_SubsList.Flink; pEntry != &m_SubsList; pEntry = pEntry->Flink) {
                CFileObject* pSub = CFileObject::FromListEntry(pEntry);
                pSub->HandleReceivedMessage(pMessage->m_szType, pMessage->m_cbPayload, pMessage->m_Payload);
            }
            LeaveCriticalSection(&m_SubsLock);
        }
    }
    else if (ConnType == CONNECTION_TYPE_TAG) {
        m_SmartCardReader.MessageReceived(pMessage);
    }
    else if (ConnType == CONNECTION_TYPE_HCE) {
        EnterCriticalSection(&m_SEManagerLock);

        if (m_pSEManager != nullptr) {
            m_pSEManager->HandleReceiveHcePacket((USHORT)m_HCEConnectionId, pMessage->m_cbPayload, pMessage->m_Payload);
        }
        LeaveCriticalSection(&m_SEManagerLock);
    }

    MethodReturnVoid();
}

void CQueue::HandleSecureElementEvent(SECURE_ELEMENT_EVENT_INFO *pInfo)
{
    MethodEntry("...");

    CSecureElement *pSecureElement = nullptr;

    if (NT_SUCCESS(GetSecureElementObject(pInfo->guidSecureElementId, &pSecureElement))) {
        if (pSecureElement->GetEmulationMode() != EmulationOff) {
            EnterCriticalSection(&m_SEEventsLock);

            for (LIST_ENTRY* pEntry = m_SEEventsList.Flink;
                 pEntry != &m_SEEventsList;
                 pEntry = pEntry->Flink) {
                CFileObject::FromListEntry(pEntry)->HandleSecureElementEvent(pInfo);
            }
            LeaveCriticalSection(&m_SEEventsLock);
        }
    }

    MethodReturnVoid();
}

void CQueue::EnumerateSecureElements()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFKEY KeyHandle = nullptr;
    WDFCOLLECTION Collection = nullptr;
    GUID SecureElementId;
    UNICODE_STRING GuidString;
    CSecureElement *pSecureElement = nullptr;

    DECLARE_CONST_UNICODE_STRING(SEEndpointsKey, SE_ENDPOINTS_KEY);

    Status = WdfDeviceOpenRegistryKey(
                    WdfIoQueueGetDevice(m_Queue),
                    PLUGPLAY_REGKEY_DEVICE,
                    KEY_READ,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &KeyHandle);

    if (NT_SUCCESS(Status) && (KeyHandle != nullptr)) {
        Status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES, &Collection);

        if (NT_SUCCESS(Status) && (Collection != nullptr)) {
            Status = WdfRegistryQueryMultiString(
                            KeyHandle,
                            &SEEndpointsKey,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            Collection);

            if (NT_SUCCESS(Status)) {
                ULONG Count = WdfCollectionGetCount(Collection);

                for (ULONG Index = 0; Index < Count; Index++) {
                    WdfStringGetUnicodeString((WDFSTRING) WdfCollectionGetItem(Collection, Index), &GuidString);

                    if (SUCCEEDED(CLSIDFromString(GuidString.Buffer, &SecureElementId)) &&
                        (pSecureElement = new CSecureElement(SecureElementId, IsEqualGUID(SecureElementId, GUID_DH_SECURE_ELEMENT) ? DeviceHost : External)) != nullptr) {
                        TraceInfo("Secure Element Id=%S Type=%d", GuidString.Buffer, pSecureElement->GetType());
                        InsertHeadList(&m_SecureElementList, pSecureElement->GetListEntry());
                    }
                }
            }

            WdfObjectDelete(Collection);
        }

        WdfRegistryClose(KeyHandle);
    }

    MethodReturnVoid();
}

void CQueue::ReadSettingsFromRegistry()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFKEY KeyHandle = nullptr;
    ULONG NfpRadioOffPolicyOverride = 0, NfpRadioOffSystemOverride = 0;

    DECLARE_CONST_UNICODE_STRING(NfpRadioTurnedOffKey, NFP_RADIO_TURNED_OFF_KEY);
    DECLARE_CONST_UNICODE_STRING(NfpRadioFlightModeKey, NFP_RADIO_FLIGHT_MODE_KEY);

    Status = WdfDeviceOpenRegistryKey(
        WdfIoQueueGetDevice(m_Queue),
        PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &KeyHandle);

    if (NT_SUCCESS(Status) && (KeyHandle != nullptr)) {
        Status = WdfRegistryQueryULong(
            KeyHandle,
            &NfpRadioTurnedOffKey,
            &NfpRadioOffPolicyOverride);

        if (NT_SUCCESS(Status)) {
            TraceInfo("%S = %d", NFP_RADIO_TURNED_OFF_KEY, NfpRadioOffPolicyOverride);
            m_NfpRadioOffPolicyOverride = (NfpRadioOffPolicyOverride != 0);
        }

        Status = WdfRegistryQueryULong(
            KeyHandle,
            &NfpRadioFlightModeKey,
            &NfpRadioOffSystemOverride);

        if (NT_SUCCESS(Status)) {
            TraceInfo("%S = %d", NFP_RADIO_FLIGHT_MODE_KEY, NfpRadioOffSystemOverride);
            m_NfpRadioOffSystemOverride = (NfpRadioOffSystemOverride != 0);
        }

        m_NfpRadioState = !m_NfpRadioOffPolicyOverride && !m_NfpRadioOffSystemOverride;

        WdfRegistryClose(KeyHandle);
    }

    MethodReturnVoid();
}

void CQueue::WriteSettingsToRegistry()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFKEY KeyHandle = nullptr;

    DECLARE_CONST_UNICODE_STRING(NfpRadioTurnedOffKey, NFP_RADIO_TURNED_OFF_KEY);
    DECLARE_CONST_UNICODE_STRING(NfpRadioFlightModeKey, NFP_RADIO_FLIGHT_MODE_KEY);

    Status = WdfDeviceOpenRegistryKey(
                    WdfIoQueueGetDevice(m_Queue),
                    PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
                    GENERIC_ALL & ~(GENERIC_WRITE | KEY_CREATE_SUB_KEY | WRITE_DAC),
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &KeyHandle);

    if (NT_SUCCESS(Status) && (KeyHandle != nullptr)) {
        Status = WdfRegistryAssignULong(
            KeyHandle,
            &NfpRadioTurnedOffKey,
            m_NfpRadioOffPolicyOverride);

        if (NT_SUCCESS(Status)) {
            TraceInfo("%S = %d successfully persisted", NFP_RADIO_TURNED_OFF_KEY, m_NfpRadioOffPolicyOverride);
        }

        Status = WdfRegistryAssignULong(
            KeyHandle,
            &NfpRadioFlightModeKey,
            m_NfpRadioOffSystemOverride);

        if (NT_SUCCESS(Status)) {
            TraceInfo("%S = %d successfully persisted", NFP_RADIO_FLIGHT_MODE_KEY, m_NfpRadioOffSystemOverride);
        }

        WdfRegistryClose(KeyHandle);
    }

    MethodReturnVoid();
}

NTSTATUS CQueue::GetSecureElementObject(GUID &SecureElementId, CSecureElement **ppSecureElement)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    for (LIST_ENTRY* pEntry = m_SecureElementList.Flink;
         pEntry != &m_SecureElementList;
         pEntry = pEntry->Flink) {
        if (SecureElementId == CSecureElement::FromListEntry(pEntry)->GetIdentifier()) {
            *ppSecureElement = CSecureElement::FromListEntry(pEntry);
            Status = STATUS_SUCCESS;
            break;
        }
    }

    return Status;
}

NTSTATUS CQueue::GetSecureElementId(SECURE_ELEMENT_TYPE eType, GUID *pSecureElementId)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    *pSecureElementId = GUID_NULL;

    for (LIST_ENTRY* pEntry = m_SecureElementList.Flink;
        (pEntry != &m_SecureElementList);
        pEntry = pEntry->Flink) {
        if (CSecureElement::FromListEntry(pEntry)->GetType() == eType) {
            *pSecureElementId = CSecureElement::FromListEntry(pEntry)->GetIdentifier();
            Status = STATUS_SUCCESS;
            break;
        }
    }

    return Status;
}


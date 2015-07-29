/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smartcardreader.cpp

Abstract:

    Implements the NFC smart card reader class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "Internal.h"
#include "SmartCardReader.tmh"

CSmartCardReader::CSmartCardReader(WDFDEVICE Device) : 
    m_Device(Device), 
    m_pSmartCard(nullptr),
    m_fVirtualPowered(FALSE),
    m_pCurrentClient(nullptr)
{
    srand((unsigned) GetTickCount());
    InitializeCriticalSection(&m_SmartCardLock);
}

CSmartCardReader::~CSmartCardReader()
{
    DeleteCriticalSection(&m_SmartCardLock);
}

NTSTATUS CSmartCardReader::Initialize()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG QueueConfig;

    WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchManual);
    QueueConfig.PowerManaged = WdfFalse;
    
    Status = WdfIoQueueCreate(
                    m_Device,
                    &QueueConfig,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &m_PresentQueue);

    if (NT_SUCCESS(Status)) {
        WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchManual);
        QueueConfig.PowerManaged = WdfFalse;

        Status = WdfIoQueueCreate(
                    m_Device,
                    &QueueConfig,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &m_AbsentQueue);
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

VOID CSmartCardReader::Deinitialize()
{
    MethodEntry("...");

    //
    // Queue objects are parented to device by default so they 
    // are automatically deleted by framework when parent is deleted
    //

    SAFE_DELETE(m_pSmartCard);

    MethodReturnVoid();
}

NTSTATUS CSmartCardReader::AddClient(CFileObject *pFileObject)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    if (InterlockedCompareExchangePointer((void**)&m_pCurrentClient, pFileObject, nullptr) != nullptr) {
        Status = STATUS_ACCESS_DENIED;
        goto Exit;
    }
    
Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

VOID CSmartCardReader::RemoveClient(CFileObject *pFileObject)
{
    MethodEntry("...");

    InterlockedCompareExchangePointer((void**)&m_pCurrentClient, nullptr, pFileObject);

    MethodReturnVoid();
}

NTSTATUS
CSmartCardReader::GetAttribute(
    _In_ DWORD dwAttributeId,
    _Out_writes_bytes_(*pcbAttributeValue) PBYTE pbAttributeValue,
    _Inout_ LPDWORD pcbAttributeValue
    )
{
    MethodEntry("Attribute Id = 0x%x", dwAttributeId);

    static const CHAR SCReaderVendorName[] = "Microsoft";
    static const CHAR SCReaderVendorIfd[] = "IFD";
    static const DWORD SCReaderVendorIfdVersion = 0x01000010; //1.0.0.1
    static const DWORD SCReaderChannelId = SCARD_READER_TYPE_NFC << 16;
    static const DWORD SCReaderProtocolTypes = SCARD_PROTOCOL_T1;
    static const DWORD SCReaderDeviceUnit = 0;
    static const DWORD SCReaderDefaultClk = 13560; // Default ICC CLK frequency
    static const DWORD SCReaderMaxClk = 13560; // Max supported ICC CLK
    static const DWORD SCReaderDefaultDataRate = 1; // ICC IO data rate
    static const DWORD SCReaderMaxDataRate = 1;
    static const DWORD SCReaderMaxIfsd = 254; // Max IFSD supported
    static const DWORD SCReaderCharacteristics = SCARD_READER_CONTACTLESS;
    static const DWORD SCReaderCurrentProtocolType = SCARD_PROTOCOL_T1;
    static const DWORD SCReaderCurrentClk = 13560; // Current ICC CLK frequency in KHz
    static const DWORD SCReaderCurrentD = 1;
    static const DWORD SCReaderCurrentIfsc = 32; // Valid if current protocol is T=1
    static const DWORD SCReaderCurrentIfsd = 254; // Valid if current protocol is T=1
    static const DWORD SCReaderCurrentBwt = 4; // Valid if current protocol is T=1

    const DISPATCH_ENTRY DispatchTable[] =
    {
        { SCARD_ATTR_VENDOR_NAME,               (PBYTE)SCReaderVendorName,          sizeof(SCReaderVendorName),             &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_VENDOR_IFD_TYPE,           (PBYTE)SCReaderVendorIfd,           sizeof(SCReaderVendorIfd),              &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_VENDOR_IFD_VERSION,        (PBYTE)&SCReaderVendorIfdVersion,   sizeof(SCReaderVendorIfdVersion),       &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CHANNEL_ID,                (PBYTE)&SCReaderChannelId,          sizeof(SCReaderChannelId),              &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_PROTOCOL_TYPES,            (PBYTE)&SCReaderProtocolTypes,      sizeof(SCReaderProtocolTypes),          &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_DEVICE_UNIT,               (PBYTE)&SCReaderDeviceUnit,         sizeof(SCReaderDeviceUnit),             &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_DEFAULT_CLK,               (PBYTE)&SCReaderDefaultClk,         sizeof(SCReaderDefaultClk),             &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_MAX_CLK,                   (PBYTE)&SCReaderMaxClk,             sizeof(SCReaderMaxClk),                 &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_DEFAULT_DATA_RATE,         (PBYTE)&SCReaderDefaultDataRate,    sizeof(SCReaderDefaultDataRate),        &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_MAX_DATA_RATE,             (PBYTE)&SCReaderMaxDataRate,        sizeof(SCReaderMaxDataRate),            &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_MAX_IFSD,                  (PBYTE)&SCReaderMaxIfsd,            sizeof(SCReaderMaxIfsd),                &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CHARACTERISTICS,           (PBYTE)&SCReaderCharacteristics,    sizeof(SCReaderCharacteristics),        &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CURRENT_PROTOCOL_TYPE,     (PBYTE)&SCReaderCurrentProtocolType,sizeof(SCReaderCurrentProtocolType),    &CSmartCardReader::GetCurrentProtocolType },
        { SCARD_ATTR_CURRENT_CLK,               (PBYTE)&SCReaderCurrentClk,         sizeof(SCReaderCurrentClk),             &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CURRENT_D,                 (PBYTE)&SCReaderCurrentD,           sizeof(SCReaderCurrentD),               &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CURRENT_IFSC,              (PBYTE)&SCReaderCurrentIfsc,        sizeof(SCReaderCurrentIfsc),            &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CURRENT_IFSD,              (PBYTE)&SCReaderCurrentIfsd,        sizeof(SCReaderCurrentIfsd),            &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_CURRENT_BWT,               (PBYTE)&SCReaderCurrentBwt,         sizeof(SCReaderCurrentBwt),             &CSmartCardReader::GetAttributeGeneric },
        { SCARD_ATTR_ICC_PRESENCE,              nullptr,                            0,                                      &CSmartCardReader::GetIccPresence },
        { SCARD_ATTR_ATR_STRING,                nullptr,                            0,                                      &CSmartCardReader::GetAtrString },
        { SCARD_ATTR_ICC_TYPE_PER_ATR,          nullptr,                            0,                                      &CSmartCardReader::GetIccTypePerAtr },
    };

    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    for (DWORD TableEntry = 0; TableEntry < _countof(DispatchTable); TableEntry++) {
        if (DispatchTable[TableEntry].dwAttributeId == dwAttributeId) {
            Status = (this->*DispatchTable[TableEntry].pfnDispatchHandler)(DispatchTable[TableEntry].pbAttributeValue,
                                                                           (DWORD)DispatchTable[TableEntry].cbAttributeValue,
                                                                           pbAttributeValue, 
                                                                           pcbAttributeValue);
            break;
        }
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CSmartCardReader::SetAttribute(_In_ DWORD dwAttributeId)
{
    return (dwAttributeId == SCARD_ATTR_DEVICE_IN_USE ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED);
}

NTSTATUS CSmartCardReader::SetPower(_In_ DWORD dwPower)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    EnterCriticalSection(&m_SmartCardLock);

    if (m_pSmartCard == nullptr) {
        Status = STATUS_NO_MEDIA;
        goto Exit;
    }

    switch (dwPower)
    {
    case SCARD_COLD_RESET:
    case SCARD_WARM_RESET:
        m_fVirtualPowered = TRUE;
        break;

    case SCARD_POWER_DOWN:
        m_fVirtualPowered = FALSE;
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

Exit:
    LeaveCriticalSection(&m_SmartCardLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CSmartCardReader::SetProtocol(_In_ DWORD dwProtocol, _Out_ DWORD *pdwSelectedProtocol)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    if (m_pSmartCard == nullptr) {
        Status = STATUS_NO_MEDIA;
        goto Exit;
    }

    if (((dwProtocol & SCARD_PROTOCOL_DEFAULT) != 0) ||
        ((dwProtocol & SCARD_PROTOCOL_T1) != 0) ) {
        *pdwSelectedProtocol = SCARD_PROTOCOL_T1;
    }
    else if (((dwProtocol & SCARD_PROTOCOL_RAW) != 0) ||
             ((dwProtocol & SCARD_PROTOCOL_T0) != 0) ||
             ((dwProtocol & SCARD_PROTOCOL_Tx) != 0) ) {
        Status = STATUS_NOT_SUPPORTED;
    }
    else if (dwProtocol == SCARD_PROTOCOL_OPTIMAL) {
        *pdwSelectedProtocol = SCARD_PROTOCOL_T1;
    }
    else {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CSmartCardReader::IsAbsent(_In_ WDFREQUEST Request)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFREQUEST OutRequest = nullptr;
    BOOL fCompleteRequest = TRUE;

    if (m_pSmartCard == nullptr) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (NT_SUCCESS(WdfIoQueueFindRequest(
                        m_AbsentQueue,
                        nullptr,
                        WdfRequestGetFileObject(Request),
                        nullptr,
                        &OutRequest))) {
        WdfObjectDereference(OutRequest);
        Status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    Status = WdfRequestForwardToIoQueue(Request, m_AbsentQueue);
    fCompleteRequest = !NT_SUCCESS(Status);

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS CSmartCardReader::IsPresent(_In_ WDFREQUEST Request)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFREQUEST OutRequest = nullptr;
    BOOL fCompleteRequest = TRUE;

    if (m_pSmartCard != nullptr) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (NT_SUCCESS(WdfIoQueueFindRequest(
                        m_PresentQueue,
                        nullptr,
                        WdfRequestGetFileObject(Request),
                        nullptr,
                        &OutRequest))) {
        WdfObjectDereference(OutRequest);
        Status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    Status = WdfRequestForwardToIoQueue(Request, m_PresentQueue);
    fCompleteRequest = !NT_SUCCESS(Status);

Exit:
    MethodReturn(fCompleteRequest ? Status : STATUS_PENDING, "Status = %!STATUS!", Status);
}

NTSTATUS
CSmartCardReader::Transmit(
    _In_reads_bytes_opt_(InputBufferLength) PBYTE InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PBYTE OutputBuffer,
    _In_ size_t OutputBufferLength,
    _In_ size_t* BytesTransferred
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    SCARD_IO_REQUEST *InputRequest = (SCARD_IO_REQUEST*)InputBuffer;
    SCARD_IO_REQUEST *OutputRequest = (SCARD_IO_REQUEST*)OutputBuffer;
    
    EnterCriticalSection(&m_SmartCardLock); 

    if (m_pSmartCard == nullptr) {
        Status = STATUS_NO_MEDIA;
        goto Exit;
    }

    if (InputRequest->dwProtocol != SCARD_PROTOCOL_T1) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    Status = m_pSmartCard->SendCommand(
                                InputBuffer + sizeof(SCARD_IO_REQUEST),
                                InputBufferLength - sizeof(SCARD_IO_REQUEST),
                                OutputBuffer + sizeof(SCARD_IO_REQUEST),
                                OutputBufferLength - sizeof(SCARD_IO_REQUEST),
                                BytesTransferred);

    if (NT_SUCCESS(Status)) {
        // Store the header information (SCARD_IO_REQUEST) in the output buffer
        OutputRequest->dwProtocol = SCARD_PROTOCOL_T1;
        OutputRequest->cbPciLength = sizeof(SCARD_IO_REQUEST);
        *BytesTransferred += sizeof(SCARD_IO_REQUEST);
    }

Exit:
    LeaveCriticalSection(&m_SmartCardLock); 
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

DWORD CSmartCardReader::GetState()
{
    if (m_pSmartCard != nullptr) {
        return (m_fVirtualPowered ? SCARD_SPECIFIC : SCARD_SWALLOWED);
    }
    
    return SCARD_ABSENT;
}

BOOL CSmartCardReader::MessageReceived(_In_ MESSAGE* pMessage)
{
    MethodEntry("...");

    BOOL fHandled = FALSE;

    if (m_pSmartCard != nullptr) {
        fHandled = m_pSmartCard->ResponseReceived(pMessage);
    }

    MethodReturnBool(fHandled);
}

BOOL CSmartCardReader::CardArrived(_In_ CConnection* pConnection)
{
    MethodEntry("void");

    BOOL fTakeConnection = FALSE;
    WDFREQUEST Request = nullptr;

    EnterCriticalSection(&m_SmartCardLock);

    m_fVirtualPowered = TRUE;

    if (m_pSmartCard == nullptr) {
        m_pSmartCard = new CSmartCard(m_Device, pConnection);

        if (m_pSmartCard != nullptr) {
            fTakeConnection = TRUE;

            while (NT_SUCCESS(WdfIoQueueRetrieveNextRequest(m_PresentQueue, &Request))) {
                TraceInfo("%!FUNC! Completing Request with Status %!STATUS!", STATUS_SUCCESS);
                WdfRequestComplete(Request, STATUS_SUCCESS);
            }
        }
    }

    LeaveCriticalSection(&m_SmartCardLock);

    MethodReturnBool(fTakeConnection);
}

BOOL CSmartCardReader::CardRemoved(_In_ CConnection* pConnection)
{
    MethodEntry("void");

    UNREFERENCED_PARAMETER(pConnection);

    BOOL fConnectionDeleted = FALSE;
    WDFREQUEST Request = nullptr;

    EnterCriticalSection(&m_SmartCardLock);

    m_fVirtualPowered = FALSE;

    if (m_pSmartCard != nullptr) {
        SAFE_DELETE(m_pSmartCard);

        while (NT_SUCCESS(WdfIoQueueRetrieveNextRequest(m_AbsentQueue, &Request))) {
            TraceInfo("%!FUNC! Completing Request with Status %!STATUS!", STATUS_SUCCESS);
            WdfRequestComplete(Request, STATUS_SUCCESS);
        }

        fConnectionDeleted = TRUE;
    }

    LeaveCriticalSection(&m_SmartCardLock);

    MethodReturnBool(fConnectionDeleted);
}

FORCEINLINE NTSTATUS
CopyToBuffer(
    _In_reads_bytes_(cbAttributeValue) const VOID *pbAttributeValue,
    _In_ DWORD cbAttributeValue,
    _Out_writes_bytes_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ LPDWORD pcbOutputBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (*pcbOutputBuffer < cbAttributeValue) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    RtlCopyMemory(pbOutputBuffer, pbAttributeValue, cbAttributeValue);
    *pcbOutputBuffer = cbAttributeValue;

Exit:
    return Status;
}

NTSTATUS
CSmartCardReader::GetAttributeGeneric(
    _In_opt_bytecount_(cbAttributeValue) PBYTE pbAttributeValue,
    _In_ DWORD cbAttributeValue,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ LPDWORD pcbOutputBuffer
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    Status = CopyToBuffer(pbAttributeValue, cbAttributeValue, pbOutputBuffer, pcbOutputBuffer);

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CSmartCardReader::GetCurrentProtocolType(
    _In_opt_bytecount_(cbCurrentProtocolType) PBYTE pbCurrentProtocolType,
    _In_ DWORD cbCurrentProtocolType,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ LPDWORD pcbOutputBuffer
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    if (m_pSmartCard == nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    Status = CopyToBuffer(pbCurrentProtocolType, cbCurrentProtocolType, pbOutputBuffer, pcbOutputBuffer);

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CSmartCardReader::GetAtrString(
    _In_opt_bytecount_(cbAttributeValue) PBYTE pbAttributeValue,
    _In_ DWORD cbAttributeValue,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ LPDWORD pcbOutputBuffer
    )
{
    MethodEntry("...");

    UNREFERENCED_PARAMETER(pbAttributeValue);
    UNREFERENCED_PARAMETER(cbAttributeValue);

    NTSTATUS Status = STATUS_SUCCESS;

    if (m_pSmartCard == nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    Status = m_pSmartCard->GetAtr(pbOutputBuffer, pcbOutputBuffer);

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CSmartCardReader::GetIccTypePerAtr(
    _In_opt_bytecount_(cbAttributeValue) PBYTE pbAttributeValue,
    _In_ DWORD cbAttributeValue,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ LPDWORD pcbOutputBuffer
    )
{
    MethodEntry("...");

    UNREFERENCED_PARAMETER(pbAttributeValue);
    UNREFERENCED_PARAMETER(cbAttributeValue);

    NTSTATUS Status = STATUS_SUCCESS;

    if (m_pSmartCard == nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    Status = m_pSmartCard->GetIccType(pbOutputBuffer, pcbOutputBuffer);

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CSmartCardReader::GetIccPresence(
    _In_opt_bytecount_(cbAttributeValue) PBYTE pbAttributeValue,
    _In_ DWORD cbAttributeValue,
    _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
    _Inout_ LPDWORD pcbOutputBuffer
    )
{
    MethodEntry("...");

    UNREFERENCED_PARAMETER(pbAttributeValue);
    UNREFERENCED_PARAMETER(cbAttributeValue);

    NTSTATUS Status = STATUS_SUCCESS;
    BYTE IccPresence = (m_pSmartCard == nullptr ? 0x0 : 0x1);

    Status = CopyToBuffer(&IccPresence, sizeof(IccPresence), pbOutputBuffer, pcbOutputBuffer);

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

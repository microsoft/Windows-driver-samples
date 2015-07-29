/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smartcard.cpp

Abstract:

    This file implements the NFC smart card class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "Internal.h"
#include "SmartCard.tmh"

CSmartCardRequest::CSmartCardRequest(_In_ CSmartCard *pSmartCard)
    : m_pSmartCard(pSmartCard),
      m_cbPayload(0),
      m_hCompletionEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr))
{
    RtlZeroMemory(m_Payload, sizeof(m_Payload));
    InitializeListHead(&m_ListEntry);
    m_dwSequenceNum = m_pSmartCard->AddRequest(this);
}

CSmartCardRequest::~CSmartCardRequest()
{
    m_pSmartCard->RemoveRequest(this);
    SAFE_CLOSEHANDLE(m_hCompletionEvent);
}

VOID
CSmartCardRequest::CompleteRequest(
    _In_ DWORD cbPayload,
    _In_reads_bytes_(cbPayload) PBYTE pbPayload
    )
{
    RtlZeroMemory(m_Payload, sizeof(m_Payload));

    if (cbPayload <= sizeof(m_Payload)) {
        RtlCopyMemory(m_Payload, pbPayload, cbPayload);
        m_cbPayload = cbPayload;
    }

    SetEvent(m_hCompletionEvent);
}

NTSTATUS CSmartCardRequest::WaitForCompletion()
{
    return (WaitForSingleObject(m_hCompletionEvent, ResponseTimeoutInMs) == WAIT_OBJECT_0) ? STATUS_SUCCESS : STATUS_IO_TIMEOUT;
}

CSmartCard::CSmartCard(_In_ WDFDEVICE Device, _In_ CConnection* pConnection)
    : m_Device(Device),
      m_pConnection(pConnection),
      m_dwSequenceNum(0)
{
    InitializeListHead(&m_RequestList);
    InitializeCriticalSection(&m_ConnectionLock);
    InitializeCriticalSection(&m_RequestListLock);
}

CSmartCard::~CSmartCard()
{
    EnterCriticalSection(&m_RequestListLock);

    while (!IsListEmpty(&m_RequestList)) {
        delete CSmartCardRequest::FromListEntry(RemoveHeadList(&m_RequestList));
    }
    LeaveCriticalSection(&m_RequestListLock);

    DeleteCriticalSection(&m_ConnectionLock);
    DeleteCriticalSection(&m_RequestListLock);

    SAFE_DELETE(m_pConnection);
}

NTSTATUS
CSmartCard::SendCommand(
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ size_t InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ size_t OutputBufferLength,
    _Out_ size_t* BytesTransferred
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    MESSAGE *pMessage = nullptr;
    DWORD dwSequenceNum = 0;
    CSmartCardRequest *pRequest = nullptr;
    BYTE* pbPayload = nullptr;

    *BytesTransferred = 0;

    if (InputBufferLength > MaxCbPayload) {
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Exit;
    }

    pRequest = new CSmartCardRequest(this);

    if (pRequest == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    pbPayload = new BYTE[InputBufferLength + sizeof(DWORD)];

    if (pbPayload == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    dwSequenceNum = pRequest->GetSequenceNum();
    
    RtlCopyMemory(pbPayload, &dwSequenceNum, sizeof(DWORD));
    RtlCopyMemory(pbPayload + sizeof(DWORD), InputBuffer, InputBufferLength);

    pMessage = new MESSAGE();

    if (pMessage == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    pMessage->Initialize(SMARTCARD_MESSAGE_TYPE_TRANSMIT, (DWORD)InputBufferLength + sizeof(DWORD), pbPayload);

    EnterCriticalSection(&m_ConnectionLock);

    if (FAILED(m_pConnection->TransmitMessage(pMessage))) {
        Status = STATUS_DATA_ERROR;
        LeaveCriticalSection(&m_ConnectionLock);
        goto Exit;
    }
    LeaveCriticalSection(&m_ConnectionLock);

    Status = pRequest->WaitForCompletion();

    if (NT_SUCCESS(Status)) {
        *BytesTransferred = pRequest->GetPayloadSize();

        if (OutputBufferLength >= *BytesTransferred) {
            RtlCopyMemory(OutputBuffer, pRequest->GetPayload(), *BytesTransferred);
        }
        else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

Exit:
    SAFE_DELETE(pMessage);
    SAFE_DELETEARRAY(pbPayload);
    SAFE_DELETE(pRequest);

    MethodReturn(Status, "Status = %!STATUS! Output buffer size = %d and payload size = %d", Status, (DWORD)OutputBufferLength, (DWORD)*BytesTransferred);
}

BOOL CSmartCard::ResponseReceived(_In_ MESSAGE* pMessage)
{
    MethodEntry("...");

    BOOL fHandled = FALSE;
    DWORD dwSequenceNum = 0;

    if (CompareStringOrdinal(pMessage->m_szType, -1, SMARTCARD_MESSAGE_TYPE_TRANSMIT, -1, FALSE) == CSTR_EQUAL) {
        if (pMessage->m_cbPayload >= sizeof(DWORD)) {
            RtlCopyMemory(&dwSequenceNum, pMessage->m_Payload, sizeof(DWORD));
            TraceInfo("Sequence number = %d and Payload size = %d", dwSequenceNum, pMessage->m_cbPayload);

            EnterCriticalSection(&m_RequestListLock);

            for (LIST_ENTRY* pEntry = m_RequestList.Flink;
                    pEntry != &m_RequestList;
                    pEntry = pEntry->Flink) {
                CSmartCardRequest *pRequest = CSmartCardRequest::FromListEntry(pEntry);

                if (dwSequenceNum == pRequest->GetSequenceNum()) {
                    pRequest->CompleteRequest(pMessage->m_cbPayload - sizeof(DWORD), pMessage->m_Payload + sizeof(DWORD));
                    fHandled = TRUE;
                    break;
                }
            }
            LeaveCriticalSection(&m_RequestListLock);
        }
    }

    MethodReturnBool(fHandled);
}

DWORD CSmartCard::AddRequest(_In_ CSmartCardRequest *pRequest)
{
    NT_ASSERT(pRequest != nullptr);

    EnterCriticalSection(&m_RequestListLock);
    InsertHeadList(&m_RequestList, pRequest->GetListEntry());
    LeaveCriticalSection(&m_RequestListLock);

    return InterlockedIncrement(&m_dwSequenceNum);
}

VOID CSmartCard::RemoveRequest(_In_ CSmartCardRequest *pRequest)
{
    NT_ASSERT(pRequest != nullptr);

    EnterCriticalSection(&m_RequestListLock);

    for (LIST_ENTRY* pEntry = m_RequestList.Flink;
            pEntry != &m_RequestList;
            pEntry = pEntry->Flink) {
        if (pRequest == CSmartCardRequest::FromListEntry(pEntry)) {
            RemoveEntryList(pEntry);
            break;
        }
    }
    LeaveCriticalSection(&m_RequestListLock);
}

NTSTATUS
CSmartCard::GetAtr(
    _Out_writes_bytes_(*pcbAtr) PBYTE pbAtr,
    _Inout_ LPDWORD pcbAtr
    )
{
    FunctionEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFKEY hKey = nullptr;
    WDFMEMORY Memory;
    DWORD Type;
    PBYTE Buffer = nullptr;        
    size_t BufferSize = 0;
    DECLARE_CONST_UNICODE_STRING(KeyValue, L"Atr");

    Status = WdfDeviceOpenRegistryKey(
                    m_Device,
                    PLUGPLAY_REGKEY_DEVICE,
                    KEY_READ,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &hKey);

    if (NT_SUCCESS(Status)) {
        Status = WdfRegistryQueryMemory(hKey, &KeyValue, PagedPool, nullptr, &Memory, &Type);

        if (NT_SUCCESS(Status)) {
            NT_ASSERT(Type == REG_BINARY);
                
            Buffer = (PBYTE) WdfMemoryGetBuffer(Memory, &BufferSize);
                
            if (*pcbAtr < BufferSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else {
                *pcbAtr = (DWORD)BufferSize;
                RtlCopyMemory(pbAtr, Buffer, BufferSize);
            }

            WdfObjectDelete(Memory);
        }

        WdfRegistryClose(hKey);
    }

    FunctionReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CSmartCard::GetIccType(
    _Out_writes_bytes_(*pcbIccType) PBYTE pbIccType,
    _Inout_ LPDWORD pcbIccType
    )
{
    FunctionEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFKEY hKey = nullptr;
    DWORD Value = 0;
    BYTE bValue = 0;
    DECLARE_CONST_UNICODE_STRING(KeyValue, L"IccType");

    Status = WdfDeviceOpenRegistryKey(
                    m_Device,
                    PLUGPLAY_REGKEY_DEVICE,
                    KEY_READ,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &hKey);

    if (NT_SUCCESS(Status)) {
        (VOID) WdfRegistryQueryULong(hKey, &KeyValue, &Value);

        bValue = (BYTE)(Value & 0xFF);

        if (*pcbIccType < sizeof(bValue)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            *pcbIccType = sizeof(bValue);
            RtlCopyMemory(pbIccType, &bValue, sizeof(bValue));
        }

        WdfRegistryClose(hKey);
    }

    FunctionReturn(Status, "Status = %!STATUS!", Status);
}

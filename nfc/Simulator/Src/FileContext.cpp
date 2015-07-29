/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    filecontext.cpp

Abstract:

    This file implements the class for context associated with the file object 

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "Internal.h"
#include "FileContext.tmh"

CFileObject::CFileObject(
    WDFFILEOBJECT FileObject
    )
    : m_FileObject(FileObject),
      m_Role(ROLE_UNDEFINED),
      m_pszType(nullptr),
      m_fEnabled(TRUE),
      m_dwQueueSize(0),
      m_cCompleteReady(0),
      m_pConnection(nullptr),
      m_Request(nullptr),
      m_SecureElementEventType(ExternalReaderArrival)
{
    NT_ASSERT(m_FileObject != nullptr);

    InitializeListHead(&m_Queue);
    InitializeListHead(&m_ListEntry);
    InitializeCriticalSection(&m_RoleLock);

    RtlZeroMemory(&m_SecureElementId, sizeof(GUID));
}

CFileObject::~CFileObject()
{
    //
    // The object is bound to the file handle and so this happens after close and the 
    // framework guarentees all requests on the file handle are cancelled prior to close
    //
    EnterCriticalSection(&m_RoleLock);
    NT_ASSERT(m_Request == nullptr);
    LeaveCriticalSection(&m_RoleLock);

    PurgeQueue();

    SAFE_DELETEARRAY(m_pszType);
    SAFE_DELETE(m_pConnection);

    DeleteCriticalSection(&m_RoleLock);

    m_FileObject = nullptr;
}

VOID CFileObject::OnDestroy(_In_ WDFOBJECT FileObject)
{
    FunctionEntry("...");

    CFileObject *pFileObject = GetFileObject(FileObject);

    NT_ASSERT(pFileObject != nullptr);

    if (pFileObject->m_FileObject == FileObject) {
        // File object constructed using placement 'new' so explicitly invoke destructor
        pFileObject->~CFileObject();
    }

    FunctionReturnVoid();
}

NTSTATUS CFileObject::Enable()
{
    MethodEntry("void");
    
    NTSTATUS Status = STATUS_SUCCESS;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsPublication() || IsSubscription());

    if (m_fEnabled) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    m_fEnabled = TRUE;

Exit:
    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::Disable()
{
    MethodEntry("void");
    
    NTSTATUS Status = STATUS_SUCCESS;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsPublication() || IsSubscription());

    if (!m_fEnabled) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    m_fEnabled = FALSE;
    CompleteRequest(STATUS_CANCELLED, 0, true);
    PurgeQueue();

Exit:
    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::SetType(_In_ PCWSTR pszType)
{
    MethodEntry("...");
    
    NTSTATUS Status = STATUS_SUCCESS;
    size_t cchType = wcslen(pszType) + 1;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(m_pszType == nullptr);

    NT_ASSERT(IsPublication() || IsSubscription());

    if ((cchType > MinCchType) && (cchType < MaxCchType)) {
        m_pszType = new WCHAR[cchType];

        if (m_pszType != nullptr) {
            StringCchCopy(m_pszType, cchType, pszType);
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        Status = STATUS_INVALID_PARAMETER;
    }

    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::GetNextSubscribedMessage(_In_ WDFREQUEST Request)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    CPayload* pPayload = nullptr;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsSubscription());

    if (m_Request != nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }
    
    if (!m_fEnabled) {
        Status = STATUS_CANCELLED;
        goto Exit;
    }

    Status = WdfRequestMarkCancelableEx(Request, CFileObject::OnRequestCancel);

    if (NT_SUCCESS(Status)) {
        m_Request = Request;

        if (!IsListEmpty(&m_Queue)) {
            pPayload = CPayload::FromListEntry(m_Queue.Flink);

            if (CompleteRequest(pPayload->GetSize(), pPayload->GetPayload())) {
                m_dwQueueSize--;
                RemoveHeadList(&m_Queue);
                delete pPayload;
            }
        }
    }

Exit:
    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::SetPayload(_In_ DWORD cbPayload, _In_reads_bytes_(cbPayload) PBYTE pbPayload)
{
    MethodEntry("...");
    
    NTSTATUS Status = STATUS_SUCCESS;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsPublication());

    if (m_Payload.GetPayload() != nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }
    
    Status = m_Payload.Initialize(cbPayload, pbPayload);

Exit:
    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::GetNextTransmittedMessage(_In_ WDFREQUEST Request)
{
    MethodEntry("...");
    
    NTSTATUS Status = STATUS_SUCCESS;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsPublication());

    if (m_Request != nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }
    
    if (!m_fEnabled) {
        Status = STATUS_CANCELLED;
        goto Exit;
    }

    Status = WdfRequestMarkCancelableEx(Request, CFileObject::OnRequestCancel);

    if (NT_SUCCESS(Status)) {
        m_Request = Request;
        
        if (m_cCompleteReady > 0) {
            if (CompleteRequest(STATUS_SUCCESS, 0, true)) {
                m_cCompleteReady--;
            }
        }
    }

Exit:
    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::GetNextSecureElementPayload(_In_ WDFREQUEST Request)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    CPayload* pPayload = nullptr;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsSecureElementEvent() || IsSecureElementManager());

    if (m_Request != nullptr) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    Status = WdfRequestMarkCancelableEx(Request, CFileObject::OnRequestCancel);
    
    if (NT_SUCCESS(Status)) {
        m_Request = Request;

        if (!IsListEmpty(&m_Queue)) {
            pPayload = CPayload::FromListEntry(m_Queue.Flink);

            if (CompleteRequest(pPayload->GetSize(), pPayload->GetPayload())) {
                m_dwQueueSize--;
                RemoveHeadList(&m_Queue);
                delete pPayload;
            }
        }
    }

Exit:
    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CFileObject::SubscribeForEvent(_In_ GUID& SecureElementId, _In_ SECURE_ELEMENT_EVENT_TYPE SecureElementEventType)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsSecureElementEvent());

    m_SecureElementId = SecureElementId;
    m_SecureElementEventType = SecureElementEventType;

    LeaveCriticalSection(&m_RoleLock);
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS
CFileObject::BeginProximity(
    _In_ BEGIN_PROXIMITY_ARGS *pArgs,
    _In_ IConnectionCallback* pCallback
    )
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    
    NT_ASSERT(IsRoleSimulation());

    if (CConnection::Create(pCallback, &m_pConnection)) {
        if (SUCCEEDED(m_pConnection->InitializeAsClient(pArgs))) {
            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_INTERNAL_ERROR;
        }
    }
    else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

void CFileObject::HandleArrivalEvent()
{
    MethodEntry("void");

    DWORD dwFlags = 1; // payload for arrival event

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsArrivedSubscription());
    
    if (m_fEnabled) {
        HandleReceivedMessage(sizeof(DWORD), (PBYTE) &dwFlags);
    }
    
    LeaveCriticalSection(&m_RoleLock);
    MethodReturnVoid();
}

void CFileObject::HandleRemovalEvent()
{
    MethodEntry("void");

    DWORD dwFlags = 0; // payload for removal event
    
    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsDepartedSubscription());

    if (m_fEnabled) {
        HandleReceivedMessage(sizeof(DWORD), (PBYTE) &dwFlags);
    }
    
    LeaveCriticalSection(&m_RoleLock);
    MethodReturnVoid();
}

void CFileObject::HandleMessageTransmitted(void)
{
    MethodEntry("m_fEnabled=%!bool!", m_fEnabled);
    
    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsPublication());

    if (m_fEnabled) {
        if (!CompleteRequest(STATUS_SUCCESS, 0, true)) {
            m_cCompleteReady++;
        }
    }

    LeaveCriticalSection(&m_RoleLock);
    MethodReturnVoid();
}

void
CFileObject::HandleReceivedMessage(
    _In_ PCWSTR pszType,
    _In_ DWORD  cbPayload, 
    _In_reads_bytes_(cbPayload) PBYTE pbPayload
    )
{
    MethodEntry("Enabled=%!bool!", m_fEnabled);
    
    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsNormalSubscription());

    if (m_fEnabled) {
        if ((CompareStringOrdinal(m_pszType, -1, WINDOWSMIME_PROTOCOL, -1, FALSE) == CSTR_EQUAL) &&
            (wcslen(pszType) > WINDOWSMIME_PROTOCOL_CHARS) &&
            (CompareStringOrdinal(pszType, WINDOWSMIME_PROTOCOL_CHARS, WINDOWSMIME_PROTOCOL, -1, FALSE) == CSTR_EQUAL)) {

            CHAR szMimeType[MaxCchMimeType + 1] = {};
            BYTE* pbNewPayload = new BYTE[cbPayload + MaxCchMimeType];

            if (pbNewPayload != nullptr) {
                if (SUCCEEDED(StringCchPrintfA(szMimeType, _countof(szMimeType), "%S", pszType + WINDOWSMIME_PROTOCOL_CHARS + 1))) {
                    RtlCopyMemory(pbNewPayload, szMimeType, MaxCchMimeType);
                    RtlCopyMemory(pbNewPayload + MaxCchMimeType, pbPayload, cbPayload);

                    HandleReceivedMessage(cbPayload + MaxCchMimeType, pbNewPayload);
                }

                SAFE_DELETEARRAY(pbNewPayload);
            }
        }
        else if (CompareStringOrdinal(pszType, -1, m_pszType, -1, FALSE) == CSTR_EQUAL) {
            if (CompareStringOrdinal(m_pszType, -1, WINDOWSURI_PROTOCOL, -1, FALSE) == CSTR_EQUAL) {
                // WindowsUri must be returned as NULL terminated UTF16
                DWORD cbNewPayload = cbPayload + sizeof(WCHAR);
                BYTE* pbNewPayload = new BYTE[cbNewPayload];

                if (pbNewPayload != nullptr) {
                    RtlCopyMemory(pbNewPayload, pbPayload, cbPayload);
                    RtlZeroMemory(pbNewPayload + cbPayload, sizeof(WCHAR));

                    HandleReceivedMessage(cbNewPayload, pbNewPayload);

                    SAFE_DELETEARRAY(pbNewPayload);
                }
            }
            else {
                HandleReceivedMessage(cbPayload, pbPayload);
            }
        }
    }

    LeaveCriticalSection(&m_RoleLock);
    MethodReturnVoid();
}

void CFileObject::HandleReceiveHcePacket(
    _In_ USHORT uConnectionId,
    _In_ DWORD cbPayload,
    _In_reads_bytes_(cbPayload) PBYTE pbPayload
    )
{
    MethodEntry("...");

    size_t cbUsedBufferSize = 0;
    DWORD cbNewPayload = cbPayload + 2 * sizeof(USHORT);

    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsSecureElementManager());

    BYTE* pbNewPayload = new BYTE[cbNewPayload];

    if (pbNewPayload != nullptr) {
        RtlCopyMemory(pbNewPayload + cbUsedBufferSize, &uConnectionId, sizeof(USHORT));
        cbUsedBufferSize += sizeof(USHORT);
        USHORT cbSize = (USHORT)cbPayload;

        RtlCopyMemory(pbNewPayload + cbUsedBufferSize, &cbSize, sizeof(USHORT));
        cbUsedBufferSize += sizeof(USHORT);
        RtlCopyMemory(pbNewPayload + cbUsedBufferSize, pbPayload, cbPayload);

        HandleReceivedMessage(cbNewPayload, pbNewPayload);

        SAFE_DELETEARRAY(pbNewPayload);
    }
    LeaveCriticalSection(&m_RoleLock);
    MethodReturnVoid();
}

void CFileObject::HandleSecureElementEvent(SECURE_ELEMENT_EVENT_INFO* pInfo)
{
    MethodEntry("...");
    
    EnterCriticalSection(&m_RoleLock);

    NT_ASSERT(IsSecureElementEvent());

    if ((IsEqualGUID(m_SecureElementId, pInfo->guidSecureElementId) || IsEqualGUID(m_SecureElementId, GUID_NULL)) &&
        (m_SecureElementEventType == pInfo->eEventType)) {
        BYTE* pbPayload = (BYTE*)pInfo;
        DWORD cbPayload = SECURE_ELEMENT_EVENT_INFO_HEADER + pInfo->cbEventData;
        
        HandleReceivedMessage(cbPayload, pbPayload);
    }

    LeaveCriticalSection(&m_RoleLock);
    MethodReturnVoid();
}

VOID CFileObject::HandleReceivedMessage(
    _In_ DWORD cbPayload,
    _In_reads_bytes_(cbPayload) PBYTE pbPayload
    )
{
    bool fDelivered = false;

    if (m_Request != nullptr) {
        fDelivered = CompleteRequest(cbPayload, pbPayload);
    }

    if ((!fDelivered) && (m_dwQueueSize < MAX_MESSAGE_QUEUE_SIZE)) {
        CPayload* pPayload = new CPayload();

        if (pPayload != nullptr) {
            if (NT_SUCCESS(pPayload->Initialize(cbPayload, pbPayload))) {
                InsertTailList(&m_Queue, pPayload->GetListEntry());
                m_dwQueueSize++;
            }
            else {
                delete pPayload;
            }
        }
    }
}

VOID CFileObject::OnRequestCancel(_In_ WDFREQUEST Request)
{
    FunctionEntry("...");

    CFileObject *pFileObject = GetFileObject(WdfRequestGetFileObject(Request));

    NT_ASSERT(pFileObject != nullptr);
    pFileObject->Cancel();

    FunctionReturnVoid();
}

void CFileObject::Cancel()
{
    MethodEntry("...");

    EnterCriticalSection(&m_RoleLock);
    CompleteRequest(STATUS_CANCELLED, 0, false);
    LeaveCriticalSection(&m_RoleLock);

    MethodReturnVoid();
}

bool
CFileObject::CompleteRequest(
    _In_ DWORD cbPayload,
    _In_reads_bytes_opt_(cbPayload) PBYTE pbPayload
    )
{
    MethodEntry("cbPayload = %d", cbPayload);

    NTSTATUS Status = STATUS_SUCCESS;
    bool fDelivered = false;
    WDFMEMORY OutputMemory;
    size_t cbUsedBufferSize = 0;
    size_t cbMaxBufferSize = 0;

    NT_ASSERT(m_Request != nullptr);

    Status = WdfRequestRetrieveOutputMemory(m_Request, &OutputMemory);

    if (NT_SUCCESS(Status) && OutputMemory != nullptr) {
        // Set the first 4 bytes as the size of the payload as a hint for future subscriptions.
        Status = WdfMemoryCopyFromBuffer(OutputMemory, cbUsedBufferSize, &cbPayload, sizeof(DWORD));
        cbUsedBufferSize += sizeof(DWORD);

        if (NT_SUCCESS(Status)) {
            if (pbPayload != nullptr) {
                WdfMemoryGetBuffer(OutputMemory, &cbMaxBufferSize);

                if (cbMaxBufferSize < (cbPayload + cbUsedBufferSize)) {
                    // We are unable to copy the payload into the output memory,
                    // Returning this signals to the client to send a bigger buffer
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else {
                    Status = WdfMemoryCopyFromBuffer(OutputMemory, cbUsedBufferSize, pbPayload, cbPayload);
                }
            }
            
            if (NT_SUCCESS(Status)) {
                fDelivered = true;
                cbUsedBufferSize += cbPayload;
            }
        }

        if (!CompleteRequest(Status, cbUsedBufferSize, true)) {
            fDelivered = false;
        }
    }

    MethodReturnBool(fDelivered);
}


bool
CFileObject::CompleteRequest(
    _In_ NTSTATUS CompletionStatus,
    _In_ size_t cbSize,
    _In_ bool fIsCancelable
    )
{
    MethodEntry("CompletionStatus = %!STATUS!, cbSize = %d, fIsCancelable = %!bool!",
                 CompletionStatus, (DWORD)cbSize, fIsCancelable);
    
    NTSTATUS Status = STATUS_SUCCESS;
    bool fCompleted = false;

    if (m_Request != nullptr) {
        if (fIsCancelable) {
            Status = WdfRequestUnmarkCancelable(m_Request);
        }

        if (NT_SUCCESS(Status)) {
            WdfRequestCompleteWithInformation(m_Request, CompletionStatus, cbSize);
            m_Request = nullptr;
            fCompleted = true;
        }
    }

    MethodReturnBool(fCompleted);
}

void CFileObject::PurgeQueue()
{
    m_dwQueueSize = 0;

    while (!IsListEmpty(&m_Queue)) {  
        delete CPayload::FromListEntry(RemoveHeadList(&m_Queue));  
    }
}
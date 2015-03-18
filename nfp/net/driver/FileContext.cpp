/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    filecontext.cpp

Abstract:

    This file implements the class for context associated with the file object 

Environment:

    user mode only

Revision History:

--*/
#include "internal.h"

#include "FileContext.tmh"

CFileContext::~CFileContext()
{
    MethodEntry("void");
    
    while (!IsListEmpty(&m_SubscribedMessageQueue))
    {
        delete CMyPayload::FromListEntry(RemoveHeadList(&m_SubscribedMessageQueue));
    }
    
    if (m_pszType != NULL)
    {
        delete [] m_pszType;
        m_pszType = NULL;
    }

    if (m_pConnection != NULL)
    {
        delete m_pConnection;
        m_pConnection = NULL;
    }
    
    m_pWdfFile = NULL;

    EnterCriticalSection(&m_RoleLock);
    CompleteRequest(E_ABORT, 0, true);
    LeaveCriticalSection(&m_RoleLock);
    
    DeleteCriticalSection(&m_RoleLock);

    MethodReturnVoid();
}

HRESULT
CFileContext::Disable()
{
    MethodEntry("void");
    
    EnterCriticalSection(&m_RoleLock);
    
    HRESULT hr = S_OK;
    if ((m_Role == ROLE_UNDEFINED) || (m_Role == ROLE_PROXIMITY))
    {
        // Only Pub/Sub handles can be disabled
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }

    if (!m_fEnabled)
    {
        // Already disabled
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }

    if (SUCCEEDED(hr))
    {
        m_fEnabled = FALSE;

        CompleteRequest(HRESULT_FROM_NT(STATUS_CANCELLED), 0, true);

        // Purge all already received payloads
        while (!IsListEmpty(&m_SubscribedMessageQueue))
        {
            delete CMyPayload::FromListEntry(RemoveHeadList(&m_SubscribedMessageQueue));
        }
    }
    
    LeaveCriticalSection(&m_RoleLock);
    
    MethodReturnHR(hr);
}

HRESULT
CFileContext::Enable()
{
    MethodEntry("void");
    
    EnterCriticalSection(&m_RoleLock);
    
    HRESULT hr = S_OK;
    if ((m_Role == ROLE_UNDEFINED) || (m_Role == ROLE_PROXIMITY))
    {
        // Only Pub/Sub handles can be enabled
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }

    if (m_fEnabled)
    {
        // Already enabled
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }

    if (SUCCEEDED(hr))
    {
        m_fEnabled = TRUE;
    }
        
    LeaveCriticalSection(&m_RoleLock);
    
    MethodReturnHR(hr);
}

HRESULT
CFileContext::SetType(_In_ PCWSTR pszType)
{
    MethodEntry("pszType = '%S'", pszType);
    
    HRESULT hr = S_OK;
    WUDF_SAMPLE_DRIVER_ASSERT(m_pszType == NULL);
    
    SIZE_T cchStr = wcslen(pszType) + 1;
    if ((cchStr > MinCchType) && (cchStr < MaxCchType))
    {
        m_pszType = new WCHAR[cchStr];
        if (m_pszType != NULL)
        {
            hr = StringCchCopy(m_pszType, cchStr, pszType);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    MethodReturnHR(hr);
}

#define STATUS_BUFFER_TOO_SMALL 0xC0000023L
#define STATUS_BUFFER_OVERFLOW  0x80000005L


bool
CFileContext::CompleteOneGetNextSubscribedMessage(
    _In_                       DWORD cbPayload,
    _In_reads_bytes_opt_(cbPayload) PBYTE pbPayload
    )
/*
 * m_RoleLock must already be acquired
 */
{
    MethodEntry("cbPayload = 0x%d",
          (DWORD)cbPayload);
    
    WUDF_SAMPLE_DRIVER_ASSERT(m_pWdfRequest != NULL);

    bool fDelivered = false;
    IWDFMemory* pWdfOutputMemory;
    m_pWdfRequest->GetOutputMemory(&pWdfOutputMemory);
    if (pWdfOutputMemory != NULL)
    {
        SIZE_T cbOutputBuffer = 0;
        // Set the first 4 bytes as the size of the payload as a hint for future
        // subscriptions.
        HRESULT hr = pWdfOutputMemory->CopyFromBuffer(0, &cbPayload, 4);
        if (SUCCEEDED(hr))
        {
            cbOutputBuffer = 4;
            if (pbPayload != NULL)
            {
                if (pWdfOutputMemory->GetSize() < (cbPayload + 4))
                {
                    // We are unable to copy the payload into the output memory,
                    // Returning this signals to the client to send a bigger buffer
                    hr = HRESULT_FROM_NT(STATUS_BUFFER_OVERFLOW);
                }
                else
                {
                    hr = pWdfOutputMemory->CopyFromBuffer(4, pbPayload, cbPayload);
                }
            }
            
            if (SUCCEEDED(hr))
            {
                fDelivered = true;
                cbOutputBuffer += cbPayload;
                if (m_dwQueueSize > 0)
                {
                    m_dwQueueSize--;
                }
            } 
        } 
        pWdfOutputMemory->Release();
        
        if (!CompleteRequest(hr, cbOutputBuffer, true))
        {
            fDelivered = false;
        }
    }

    MethodReturnBool(fDelivered);
}

HRESULT
CFileContext::GetNextSubscribedMessage(_In_ IRequestCallbackCancel* pCallbackCancel, _In_ IWDFIoRequest* pWdfRequest)
{
    MethodEntry("...");
    
    EnterCriticalSection(&m_RoleLock);

    HRESULT hr = S_OK;
    if (m_pWdfRequest != NULL)
    {
        // Only one pended request at a time allowed
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }
    
    if (!m_fEnabled)
    {
        // The handle is disabled
        hr = HRESULT_FROM_NT(HRESULT_FROM_NT(STATUS_CANCELLED));
    }

    if (SUCCEEDED(hr))
    {
        IWDFMemory* pWdfInputMemory;
        pWdfRequest->GetInputMemory(&pWdfInputMemory);
        SIZE_T cbInput;
        if (pWdfInputMemory->GetDataBuffer(&cbInput) == NULL)
        {
            if (m_Role == ROLE_SUBSCRIPTION)
            {
                m_pWdfRequest = pWdfRequest;
                m_pWdfRequest->MarkCancelable(pCallbackCancel); // MarkCancelable can run the OnCancel routine in this thread

                if ((m_pWdfRequest != NULL) && !IsListEmpty(&m_SubscribedMessageQueue))
                {
                    CMyPayload* pMyPayload = 
                        CMyPayload::FromListEntry(m_SubscribedMessageQueue.Flink);

                    if (CompleteOneGetNextSubscribedMessage(pMyPayload->GetSize(), 
                                                            pMyPayload->GetPayload()))
                    {
                        RemoveHeadList(&m_SubscribedMessageQueue);
                        delete pMyPayload;
                    }
                }
            }
            else if (m_Role == ROLE_ARRIVEDSUBSCRIPTION)
            {
                m_pWdfRequest = pWdfRequest;
                m_pWdfRequest->MarkCancelable(pCallbackCancel); // MarkCancelable can run the OnCancel routine in this thread
                if (m_pWdfRequest != NULL)
                {
                    CompleteOneArrivalEvent();
                }
            }
            else if (m_Role == ROLE_DEPARTEDSUBSCRIPTION)
            {
                m_pWdfRequest = pWdfRequest;
                m_pWdfRequest->MarkCancelable(pCallbackCancel); // MarkCancelable can run the OnCancel routine in this thread
                if (m_pWdfRequest != NULL)
                {
                    CompleteOneRemovalEvent();
                }
            }
            else
            {
                hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        pWdfInputMemory->Release();
    }
    
    LeaveCriticalSection(&m_RoleLock);

    MethodReturnHR(hr);
}

HRESULT
CFileContext::SetPayload(_In_ IWDFIoRequest* pWdfRequest)
{
    MethodEntry("...");
    
    EnterCriticalSection(&m_RoleLock);
    
    HRESULT hr = S_OK;
    if ((m_Role != ROLE_PUBLICATION) || (m_MyPayload.GetPayload() != NULL))
    {
        // SetPayload can only be called once per handle
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }
    
    if (SUCCEEDED(hr))
    {
        IWDFMemory* pWdfOutputMemory;
        pWdfRequest->GetOutputMemory(&pWdfOutputMemory); 
        SIZE_T cbOutput;
        if (pWdfOutputMemory->GetDataBuffer(&cbOutput) == NULL)
        {
            IWDFMemory* pWdfMemory;
            pWdfRequest->GetInputMemory(&pWdfMemory);
            if (pWdfMemory != NULL)
            {
                SIZE_T cbPayload = pWdfMemory->GetSize();
                if ((cbPayload > 0) && (cbPayload <= MaxCbPayload))
                {
                    hr = m_MyPayload.Initialize((DWORD)cbPayload, (PBYTE)pWdfMemory->GetDataBuffer(NULL));
                }
                else
                {
                    hr = HRESULT_FROM_NT(STATUS_INVALID_BUFFER_SIZE);
                }

                pWdfMemory->Release();
            }
            else
            {
                hr = E_INVALIDARG;
            }
            pWdfOutputMemory->Release();
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    LeaveCriticalSection(&m_RoleLock);

    MethodReturnHR(hr);
}

HRESULT
CFileContext::GetNextTransmittedMessage(_In_ IRequestCallbackCancel* pCallbackCancel, _In_ IWDFIoRequest* pWdfRequest)
{
    MethodEntry("...");
    
    EnterCriticalSection(&m_RoleLock);
    
    HRESULT hr = S_OK;
    if ((m_Role != ROLE_PUBLICATION) || (m_pWdfRequest != NULL))
    {
        // Only one pended request at a time allowed
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }
    
    if (!m_fEnabled)
    {
        // The handle is disabled
        hr = HRESULT_FROM_NT(HRESULT_FROM_NT(STATUS_CANCELLED));
    }
    
    if (SUCCEEDED(hr))
    {
        IWDFMemory* pWdfInputMemory;
        pWdfRequest->GetInputMemory(&pWdfInputMemory);
        SIZE_T cbInput;
        if (pWdfInputMemory->GetDataBuffer(&cbInput) == NULL)
        {
            IWDFMemory* pWdfOutputMemory;
            pWdfRequest->GetOutputMemory(&pWdfOutputMemory);
            SIZE_T cbOutput;
            if (pWdfOutputMemory->GetDataBuffer(&cbOutput) == NULL)
            {
                m_pWdfRequest = pWdfRequest;

                if (m_cCompleteReady > 0)
                {
                    if (CompleteRequest(S_OK, 0, false))
                    {
                        m_cCompleteReady--;
                    }
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            pWdfOutputMemory->Release();
        }
        else
        {
            hr = E_INVALIDARG;
        }
        pWdfInputMemory->Release();
    }

    if (SUCCEEDED(hr) && (m_pWdfRequest != NULL))
    {
        m_pWdfRequest->MarkCancelable(pCallbackCancel);
    }
    
    LeaveCriticalSection(&m_RoleLock);

    MethodReturnHR(hr);
}

HRESULT
CFileContext::BeginProximity(
    _In_ IWDFIoRequest*       pWdfRequest,
    _In_ IConnectionCallback* pCallback
    )
{
    MethodEntry("...");
    
    EnterCriticalSection(&m_RoleLock);
    
    HRESULT hr = S_OK;
    if (m_Role != ROLE_UNDEFINED)
    {
        // BeginProximity can only be called once per handle
        hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
    }
    else
    {
        m_Role = ROLE_PROXIMITY;
    }

    LeaveCriticalSection(&m_RoleLock);

    if (SUCCEEDED(hr))
    {
        IWDFMemory* pWdfMemory;
        pWdfRequest->GetInputMemory(&pWdfMemory);
        if (pWdfMemory != NULL)
        {
            if (pWdfMemory->GetSize() == sizeof(BEGIN_PROXIMITY_ARGS))
            {
                hr = CConnection::Create(pCallback, &m_pConnection);
                if (SUCCEEDED(hr))
                {
                    BEGIN_PROXIMITY_ARGS* pArgs = 
                        (BEGIN_PROXIMITY_ARGS*)pWdfMemory->GetDataBuffer(NULL);
                    hr = m_pConnection->InitializeAsClient(pArgs);
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            
            pWdfMemory->Release();
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    
    if (FAILED(hr))
    {
        m_pConnection = NULL;
    }
    
    MethodReturnHR(hr);
}

VOID
CFileContext::HandleArrivalEvent()
{
    WUDF_SAMPLE_DRIVER_ASSERT(m_Role == ROLE_ARRIVEDSUBSCRIPTION);

    EnterCriticalSection(&m_RoleLock);
    
    if (m_fEnabled)
    {
        m_cCompleteReady++;
        CompleteOneArrivalEvent();
    }
    
    LeaveCriticalSection(&m_RoleLock);
}

VOID
CFileContext::HandleRemovalEvent()
{
    WUDF_SAMPLE_DRIVER_ASSERT(m_Role == ROLE_DEPARTEDSUBSCRIPTION);
    
    EnterCriticalSection(&m_RoleLock);

    if (m_fEnabled)
    {
        m_cCompleteReady++;
        CompleteOneRemovalEvent();
    }
    
    LeaveCriticalSection(&m_RoleLock);
}

VOID
CFileContext::CompleteOneArrivalEvent()
{
    MethodEntry("void");
    
    WUDF_SAMPLE_DRIVER_ASSERT(m_Role == ROLE_ARRIVEDSUBSCRIPTION);
    
    EnterCriticalSection(&m_RoleLock);

    if (m_cCompleteReady > 0)
    {
        if (m_pWdfRequest != NULL)
        {
            // Arrival payload should either be a DWORD = 1 or 0
            // 1 == Device capable of bi-directional communication
            // 0 == Device is a dumb tag
            DWORD ArrivalFlags = 0x1;
            if (CompleteOneGetNextSubscribedMessage(sizeof(ArrivalFlags), (PBYTE)&ArrivalFlags))
            {
                m_cCompleteReady--;
            }
        }
    }
    
    LeaveCriticalSection(&m_RoleLock);

    MethodReturnVoid();
}

VOID
CFileContext::CompleteOneRemovalEvent()
{
    MethodEntry("void");
    
    WUDF_SAMPLE_DRIVER_ASSERT(m_Role == ROLE_DEPARTEDSUBSCRIPTION);
    
    EnterCriticalSection(&m_RoleLock);

    if (m_cCompleteReady > 0)
    {
        if (m_pWdfRequest != NULL)
        {
            // Removal payload should be a single zeroed DWORD
            DWORD RemovalFlags = 0x0;
            if (CompleteOneGetNextSubscribedMessage(sizeof(RemovalFlags), (PBYTE)&RemovalFlags))
            {
                m_cCompleteReady--;
            }
        }
    }
    
    LeaveCriticalSection(&m_RoleLock);

    MethodReturnVoid();
}

void
CFileContext::HandleReceivedPublication(
    _In_                   PCWSTR pszType,
    _In_                   DWORD  cbPayload, 
    _In_reads_bytes_(cbPayload) PBYTE  pbPayload
    )
{
    MethodEntry("...");
    
    WUDF_SAMPLE_DRIVER_ASSERT(m_Role == ROLE_SUBSCRIPTION);
    
    EnterCriticalSection(&m_RoleLock);

    if (m_fEnabled)
    {
        bool fSubscriptionMatches = false;
        BYTE* pbNewPayload = NULL;

        if ((CompareStringOrdinal(m_pszType, -1, WINDOWSMIME_PROTOCOL, -1, FALSE) == CSTR_EQUAL) &&
            (wcslen(pszType) > WINDOWSMIME_PROTOCOL_CHARS) &&
            (CompareStringOrdinal(pszType, WINDOWSMIME_PROTOCOL_CHARS,
                                  WINDOWSMIME_PROTOCOL, -1, FALSE) == CSTR_EQUAL))
        {
            // If this is a WindowsMime message, and the subscription is for the general WINDOWSMIME_PROTOCOL type,
            // the Mime type needs to be added to the message payload.
            CHAR szMimeType[MaxCchMimeType + 1] = {};
            // Copy the mime type and convert from wide to multibyte chars
            if (SUCCEEDED(StringCchPrintfA(szMimeType, ARRAYSIZE(szMimeType), "%S", pszType + WINDOWSMIME_PROTOCOL_CHARS + 1)))
            {
                pbNewPayload = new BYTE[cbPayload + MaxCchMimeType];
                if (pbNewPayload != NULL)
                {
                    TraceInfo("Received Mime message of type = '%s'", szMimeType);
                    CopyMemory(pbNewPayload, szMimeType, MaxCchMimeType);
                    CopyMemory(pbNewPayload + MaxCchMimeType, pbPayload, cbPayload);
                    cbPayload += MaxCchMimeType;
                    pbPayload = pbNewPayload;

                    fSubscriptionMatches = true;
                }
            }
        }
        else if (CompareStringOrdinal(pszType, -1, m_pszType, -1, FALSE) == CSTR_EQUAL)
        {
            fSubscriptionMatches = true;
        }

        if (fSubscriptionMatches)
        {
            bool fDelivered = false;
            
            if (m_pWdfRequest != NULL)
            {
                fDelivered = CompleteOneGetNextSubscribedMessage(cbPayload, pbPayload);
            }

            if ((!fDelivered) && (m_dwQueueSize < MAX_MESSAGE_QUEUE_SIZE))
            {
                // Add message to the client delivery queue
                
                CMyPayload* pMyPayload = new CMyPayload();
                if (pMyPayload)
                {
                    if (SUCCEEDED(pMyPayload->Initialize(cbPayload, pbPayload)))
                    {
                        InsertTailList(&m_SubscribedMessageQueue, pMyPayload->GetListEntry());
                        m_dwQueueSize++;
                    }
                    else
                    {
                        delete pMyPayload;
                    }
                }
            }
        }

        delete [] pbNewPayload;
    }
    else
    {
        TraceErrorHR(HRESULT_FROM_NT(STATUS_CANCELLED), "Subscription Disabled!");
    }
    
    LeaveCriticalSection(&m_RoleLock);

    MethodReturnVoid();
}

void
CFileContext::HandleMessageTransmitted()
{
    MethodEntry("void");
    
    WUDF_SAMPLE_DRIVER_ASSERT(m_Role == ROLE_PUBLICATION);
    
    EnterCriticalSection(&m_RoleLock);

    if (!CompleteRequest(S_OK, 0, true))
    {
        m_cCompleteReady++;
    }
    
    LeaveCriticalSection(&m_RoleLock);
    
    MethodReturnVoid();
}

void
CFileContext::OnCancel()
{
    EnterCriticalSection(&m_RoleLock);

    CompleteRequest(E_ABORT, 0, false);
    
    LeaveCriticalSection(&m_RoleLock);
}

bool
CFileContext::CompleteRequest(_In_ HRESULT hr, _In_ SIZE_T cbSize, _In_ bool fIsCancelable)
/*
 * m_RoleLock must already be acquired
 */
{
    MethodEntry("hr = %!HRESULT!, cbSize = %d, fIsCancelable = %!bool!",
                 hr,       (DWORD)cbSize,      fIsCancelable);
    
    bool fCompleted = false;
    if (m_pWdfRequest != NULL)
    {
        bool fCompleteRequest = true;
        if (fIsCancelable)
        {
            if (m_pWdfRequest->UnmarkCancelable() == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
            {
                fCompleteRequest = false;
            }
        }

        if (fCompleteRequest)
        {
            m_pWdfRequest->CompleteWithInformation(hr, cbSize);
            m_pWdfRequest = NULL;
            fCompleted = true;
        }
    }

    MethodReturnBool(fCompleted);
}

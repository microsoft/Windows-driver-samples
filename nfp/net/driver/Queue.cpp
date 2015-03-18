/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    queue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the read/write/ioctl operations.

Environment:

    user mode only

Revision History:

--*/
#include "internal.h"

#include "queue.tmh"

CMyQueue::CMyQueue(
    VOID
    )
{
    InitializeListHead(&m_SubsHead);
    InitializeListHead(&m_ArrivalSubsHead);
    InitializeListHead(&m_DepartureSubsHead);
    InitializeListHead(&m_PubsHead);
    InitializeListHead(&m_ConnectionHead);

    InitializeCriticalSection(&m_SubsLock);
    InitializeCriticalSection(&m_PubsLock);
    InitializeCriticalSection(&m_ConnectionLock);
}

CMyQueue::~CMyQueue(
    VOID
    )
{
    MethodEntry("void");
    
    m_SocketListener.StopAccepting();

    while (!IsListEmpty(&m_ConnectionHead))
    {
        delete CConnection::FromListEntry(RemoveHeadList(&m_ConnectionHead));
    }
    
    DeleteCriticalSection(&m_SubsLock);
    DeleteCriticalSection(&m_PubsLock);
    DeleteCriticalSection(&m_ConnectionLock);
}

//
// Initialize 
//

HRESULT
CMyQueue::Initialize(
    _In_ CMyDevice * Device
    )
/*++

Routine Description:

   Queue Initialize helper routine.
   This routine will Create a default parallel queue associated with the Fx device object
   and pass the IUnknown for this queue

Aruments:
    Device object pointer

Return Value:

    S_OK if Initialize succeeds

--*/    
{
    MethodEntry("...");

    CComPtr<IWDFIoQueue> fxQueue;
    
    HRESULT hr;

    //
    // Create the I/O Queue object.
    //
    {
        CComPtr<IUnknown> pUnk;

        HRESULT hrQI = this->QueryInterface(__uuidof(IUnknown),(void**)&pUnk);
        
        WUDF_SAMPLE_DRIVER_ASSERT(SUCCEEDED(hrQI));

        hr = Device->GetFxDevice()->CreateIoQueue(
                                        pUnk,
                                        TRUE,
                                        WdfIoQueueDispatchParallel,
                                        TRUE,
                                        FALSE,
                                        &fxQueue
                                        );
    }

    if (FAILED(hr))
    {
        TraceErrorHR(hr, "Failed to initialize driver queue");
    }

    if (SUCCEEDED(hr))
    {
        hr = m_SocketListener.Bind();
        if (FAILED(hr))
        {
            TraceErrorHR(hr, "Failed to Bind");
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_SocketListener.EnableAccepting(this);
        if (FAILED(hr))
        {
            TraceErrorHR(hr, "Failed to EnableAccepting");
        }
    }

    if (SUCCEEDED(hr))
    {
        m_FxQueue = fxQueue;
    }
    
    MethodReturnHR(hr);
}

HRESULT
CMyQueue::Configure(
    VOID
    )
/*++

Routine Description:

    Queue configuration function .
    It is called after queue object has been succesfully initialized.
    
Aruments:
    
    NONE

 Return Value:

    S_OK if succeeds. 

--*/    
{
    MethodEntry("void");
    
    HRESULT hr = S_OK;

    MethodReturnHR(hr);
}


STDMETHODIMP_(void)
CMyQueue::OnCreateFile(
    _In_ IWDFIoQueue* /*pWdfQueue*/,
    _In_ IWDFIoRequest* pWdfRequest,
    _In_ IWDFFile* pWdfFile
    )

/*++

Routine Description:

    Create callback from the framework for this default parallel queue 
    
    The create request will create a socket connection , create a file i/o target associated 
    with the socket handle for this connection and store in the file object context.

Aruments:
    
    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    pWdfFile - WDF file object for this create

 Return Value:

    VOID

--*/
{
    MethodEntry("pWdfRequest = %p, pWdfFile = %p",
                 pWdfRequest,      pWdfFile);
    
    HRESULT hr = S_OK;

    //
    // Create file context for this file object 
    //

    CFileContext *pContext = new CFileContext(); 
    if (NULL == pContext)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        TraceErrorHR(hr, "Could not create file context");
    }

    if (SUCCEEDED(hr))
    {
        DWORD cchFileName = 0;
        hr = pWdfFile->RetrieveFileName(NULL, &cchFileName);
        if (SUCCEEDED(hr) && (cchFileName > 0) && (cchFileName <= MaxCchType))
        {
            // Allocate a buffer big enough for the filename plus some extra to prevent 
            // overruns in the parsing below: The extra needs to be larger than the biggest
            // *_CHARS value. The value 20 is overly big, but allows room to grow without
            // hitting the OACR issue again. If the OACR issue is hit, just increase this
            // to be larger than the biggest *_CHARS value used below in parsing.
            DWORD cchFileNameBuffer = cchFileName + 20;
            PWSTR pszFileNameBuffer = new WCHAR[cchFileNameBuffer];
            hr = (pszFileNameBuffer != NULL) ? S_OK : E_OUTOFMEMORY;
            if (SUCCEEDED(hr))
            {
                ZeroMemory(pszFileNameBuffer, cchFileNameBuffer * sizeof(WCHAR));
                
                hr = pWdfFile->RetrieveFileName(pszFileNameBuffer, &cchFileName);
            }
            
            if (SUCCEEDED(hr))
            {
                PCWSTR pszFileName = pszFileNameBuffer;
                if (pszFileNameBuffer[0] == L'\\')
                {
                    // If it exists, remove the inital slash
                    pszFileName++;
                    cchFileName--;
                }

                TraceInfo("cchFileName = %d, pszFileName = %S", cchFileName, pszFileName);
                
                PCWSTR pszProtocol = NULL;
                if (CompareStringOrdinal(pszFileName, PUBS_NAMESPACE_CHARS, 
                                         PUBS_NAMESPACE, PUBS_NAMESPACE_CHARS,
                                         TRUE) == CSTR_EQUAL)
                {
                    pContext->SetRolePublication();
                    pszProtocol = pszFileName + PUBS_NAMESPACE_CHARS;
                }
                else if (CompareStringOrdinal(pszFileName, SUBS_NAMESPACE_CHARS, 
                                              SUBS_NAMESPACE, SUBS_NAMESPACE_CHARS,
                                              TRUE) == CSTR_EQUAL)
                {
                    pContext->SetRoleSubcription();
                    pszProtocol = pszFileName + SUBS_NAMESPACE_CHARS;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }

                if (SUCCEEDED(hr))
                {
                    if (CompareStringOrdinal(pszProtocol, WINDOWS_PROTOCOL_CHARS, 
                                             WINDOWS_PROTOCOL, WINDOWS_PROTOCOL_CHARS,
                                             TRUE) == CSTR_EQUAL)
                    {
                        pContext->SetType(pszProtocol);
                    }
                    else if (CompareStringOrdinal(pszProtocol, -1,
                                                  WINDOWSURI_PROTOCOL, -1,
                                                  TRUE) == CSTR_EQUAL)
                    {
                        pContext->SetType(pszProtocol);
                    }
                    else if (CompareStringOrdinal(pszProtocol, WINDOWSMIME_PROTOCOL_CHARS,
                                                  WINDOWSMIME_PROTOCOL, WINDOWSMIME_PROTOCOL_CHARS,
                                                  TRUE) == CSTR_EQUAL)
                    {
                        pContext->SetType(pszProtocol);
                    }
                    else if (CompareStringOrdinal(pszProtocol, -1, DEVICE_ARRIVED, -1,
                                                  TRUE) == CSTR_EQUAL)
                    {
                        if (!pContext->SetRoleArrivedSubcription())
                        {
                            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                        }
                    }
                    else if (CompareStringOrdinal(pszProtocol, -1, DEVICE_DEPARTED, -1,
                                                  TRUE) == CSTR_EQUAL)
                    {
                        if (!pContext->SetRoleDepartedSubcription())
                        {
                            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                        }
                    }
                    else if (CompareStringOrdinal(pszProtocol, PAIRING_PROTOCOL_CHARS,
                                                  PAIRING_PROTOCOL, PAIRING_PROTOCOL_CHARS,
                                                  TRUE) == CSTR_EQUAL)
                    {
                        pContext->SetType(pszProtocol);
                    }
                    else if (CompareStringOrdinal(pszProtocol, NDEF_PROTOCOL_CHARS,
                                                  NDEF_PROTOCOL, NDEF_PROTOCOL_CHARS,
                                                  TRUE) == CSTR_EQUAL)
                    {
                        PCWSTR pszType = pszProtocol + NDEF_PROTOCOL_CHARS;

                        if (CompareStringOrdinal(pszType, NDEF_EMPTY_TYPE_CHARS,
                                                 NDEF_EMPTY_TYPE, NDEF_EMPTY_TYPE_CHARS,
                                                 TRUE) != CSTR_EQUAL)
                        {
                            pContext->SetType(pszProtocol);
                        }
                        else
                        {
                            hr = HRESULT_FROM_NT(STATUS_INVALID_PARAMETER);
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_NT(STATUS_OBJECT_PATH_NOT_FOUND);
                    }
                }
            }
            
            if (pszFileNameBuffer != NULL)
            {
                delete [] pszFileNameBuffer;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pWdfFile->AssignContext(NULL, (void*)pContext);
        if (FAILED(hr))
        {
            TraceErrorHR(hr, "Unable to Assign Context to this File Object");
        }
    }

    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_SubsLock);

        if (pContext->IsNormalSubscription())
        {
            // Place this CFileContext into a list of Subscriptions
            InsertHeadList(&m_SubsHead, pContext->GetListEntry());
        }
        else if (pContext->IsArrivedSubscription())
        {
            // Place this CFileContext into a list of arrival registrations
            InsertHeadList(&m_ArrivalSubsHead, pContext->GetListEntry());
        }
        else if (pContext->IsDepartedSubscription())
        {
            // Place this CFileContext into a list of removal registrations
            InsertHeadList(&m_DepartureSubsHead, pContext->GetListEntry());
        }

        LeaveCriticalSection(&m_SubsLock);
    }

    if (FAILED(hr))
    {
        if (pContext != NULL)
        {
            delete pContext;            
            pContext = NULL;
        }
    }
    
    pWdfRequest->Complete(hr);

    MethodReturnVoid();
}

STDMETHODIMP_(void)
CMyQueue::OnCloseFile(
    _In_ IWDFFile* pWdfFileObject
    )
/*++

  Routine Description:

    This method is called when an app closes the file handle to this device.
    This will free the context memory associated with this file object, close
    the connection object associated with this file object and delete the file
    handle i/o target object associated with this file object.

  Arguments:

    pWdfFileObject - the framework file object for which close is handled.

  Return Value:

    None

--*/
{
    MethodEntry("...");

    HRESULT hr = S_OK ;
    CFileContext* pContext = NULL;
    hr = pWdfFileObject->RetrieveContext((void**)&pContext);
    if (SUCCEEDED(hr) && (pContext != NULL))
    {
        CRITICAL_SECTION* pCritSec = NULL;
        LIST_ENTRY* pHead = NULL;
        
        if (pContext->IsNormalSubscription())
        {
            pCritSec = &m_SubsLock;
            pHead = &m_SubsHead;
        }
        else if (pContext->IsArrivedSubscription())
        {
            pCritSec = &m_SubsLock;
            pHead = &m_ArrivalSubsHead;
        }
        else if (pContext->IsDepartedSubscription())
        {
            pCritSec = &m_SubsLock;
            pHead = &m_DepartureSubsHead;
        }
        else if (pContext->IsPublication())
        {
            pCritSec = &m_PubsLock;
            pHead = &m_PubsHead;
        }
        
        if (pHead != NULL)
        {
            EnterCriticalSection(pCritSec);
            
            LIST_ENTRY* pFindEntry = pContext->GetListEntry();
            LIST_ENTRY* pEntry = pHead->Flink;
            while (pEntry != pHead)
            {
                if (pEntry == pFindEntry)
                {
                    RemoveEntryList(pEntry);
                    break;
                }
                
                pEntry = pEntry->Flink;
            }
            
            LeaveCriticalSection(pCritSec);
        }
        
        delete pContext;
    }

    MethodReturnVoid();
}

STDMETHODIMP_ (void)
CMyQueue::OnCancel(
    _In_ IWDFIoRequest* pWdfRequest
    )
{
    MethodEntry("pWdfRequest = %p",
                 pWdfRequest);
    
    IWDFFile* pFxFile;
    pWdfRequest->GetFileObject(&pFxFile);
    if (pFxFile != NULL)
    {
        CFileContext* pFileContext;
        HRESULT hr = pFxFile->RetrieveContext((void**)&pFileContext);
        if (SUCCEEDED(hr))
        {
            pFileContext->OnCancel();
        }
    }
    
    MethodReturnVoid();
}

#define IOCTL_BEGIN_PROXIMITY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS)

STDMETHODIMP_ (void)
CMyQueue::OnDeviceIoControl(
    _In_ IWDFIoQueue* /*pWdfQueue*/, 
    _In_ IWDFIoRequest* pWdfRequest, 
    _In_ ULONG ControlCode, 
    _In_ SIZE_T /*InBufferSize*/, 
    _In_ SIZE_T /*OutBufferSize*/
    )
{
    MethodEntry("pWdfRequest = %p, ControlCode = %d",
                 pWdfRequest,      ControlCode);
    
    IWDFFile* pFxFile;

    pWdfRequest->GetFileObject(&pFxFile);

    bool fCompleteRequest = true;

    CFileContext *pFileContext;
    HRESULT hr = pFxFile->RetrieveContext((void**)&pFileContext);
    if (SUCCEEDED(hr))
    {
        switch (ControlCode)
        {
            case IOCTL_NFP_GET_MAX_MESSAGE_BYTES:
                {
                    IWDFMemory* pWdfOutputMemory;
                    pWdfRequest->GetOutputMemory(&pWdfOutputMemory);
                    if (pWdfOutputMemory != NULL)
                    {
                        SIZE_T cbOutputBuffer = 0;
                        // Set the first 4 bytes as the maximum message size of this device/driver
                        DWORD dwMaxCbPayload = MaxCbPayload;
                        hr = pWdfOutputMemory->CopyFromBuffer(0, &dwMaxCbPayload, 4);
                        if (SUCCEEDED(hr))
                        {
                            cbOutputBuffer = 4;
                        } 
                        pWdfOutputMemory->Release();
                        
                        pWdfRequest->CompleteWithInformation(hr, cbOutputBuffer);
                        fCompleteRequest = false;
                    }
                }
            break;
            
            case IOCTL_NFP_GET_KILO_BYTES_PER_SECOND:
                {
                    IWDFMemory* pWdfOutputMemory;
                    pWdfRequest->GetOutputMemory(&pWdfOutputMemory);
                    if (pWdfOutputMemory != NULL)
                    {
                        SIZE_T cbOutputBuffer = 0;
                        // Set the first 4 bytes as transfer speed of this device/driver
                        DWORD dwKilobytesPerSecond = KilobytesPerSecond;
                        hr = pWdfOutputMemory->CopyFromBuffer(0, &dwKilobytesPerSecond, 4);
                        if (SUCCEEDED(hr))
                        {
                            cbOutputBuffer = 4;
                        } 
                        pWdfOutputMemory->Release();
                        
                        pWdfRequest->CompleteWithInformation(hr, cbOutputBuffer);
                        fCompleteRequest = false;
                    }
                }
            break;

            case IOCTL_NFP_DISABLE:
                hr = pFileContext->Disable();
            break;
            
            case IOCTL_NFP_ENABLE:
                hr = pFileContext->Enable();
            break;
                
            case IOCTL_NFP_SET_PAYLOAD:
                hr = pFileContext->SetPayload(pWdfRequest);
                if (SUCCEEDED(hr))
                {
                    MESSAGE* pMessage = new MESSAGE();
                    hr = (pMessage != NULL) ? S_OK : E_OUTOFMEMORY;
                    if (SUCCEEDED(hr))
                    {
                        pMessage->Initialize(pFileContext->GetType(), pFileContext->GetSize(), pFileContext->GetPayload());
                        
                        EnterCriticalSection(&m_ConnectionLock);
                        
                        for (LIST_ENTRY* pEntry = m_ConnectionHead.Flink; 
                             pEntry != &m_ConnectionHead;
                             pEntry = pEntry->Flink)
                        {
                            CConnection* pConnection = CConnection::FromListEntry(pEntry);
                            if (SUCCEEDED(pConnection->TransmitMessage(pMessage)))
                            {
                                pFileContext->HandleMessageTransmitted();
                            }
                        }
                        
                        LeaveCriticalSection(&m_ConnectionLock);
                        
                        // Place this CFileContext into a list of published messages
                        EnterCriticalSection(&m_PubsLock);
                        InsertHeadList(&m_PubsHead, pFileContext->GetListEntry());
                        LeaveCriticalSection(&m_PubsLock);
                        
                        delete pMessage;
                    }
                }
            break;
            
            case IOCTL_BEGIN_PROXIMITY:
                hr = pFileContext->BeginProximity(pWdfRequest, this);
            break;
            
            case IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE:
                hr = pFileContext->GetNextSubscribedMessage(this, pWdfRequest);
                if (SUCCEEDED(hr))
                {
                    fCompleteRequest = false;
                }
            break;
            
            case IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE:
                hr = pFileContext->GetNextTransmittedMessage(this, pWdfRequest);
                if (SUCCEEDED(hr))
                {
                    fCompleteRequest = false;
                }
            break;

            default:
                hr = HRESULT_FROM_NT(STATUS_INVALID_DEVICE_STATE);
            break;
        }
    }
    
    if (fCompleteRequest)
    {
        TraceInfo("Completing Request: %!HRESULT!", hr);
        pWdfRequest->Complete(hr);
    }
    
    MethodReturnVoid();
}

void
CMyQueue::ValidateAccept(_In_ SOCKET Socket, _In_ GUID* pMagicPacket)
{
    MethodEntry("...");
    
    CConnection* pConnection;
    HRESULT hr = CConnection::Create(this, &pConnection);
    if (SUCCEEDED(hr))
    {
        // Mark it as an Inbound connection, so we know to delete it when 
        // it's removed from the list
        pConnection->SetInboundConnection();
        
        pConnection->ValidateAccept(Socket, pMagicPacket);
        Socket = INVALID_SOCKET;
    }

    if (Socket != INVALID_SOCKET)
    {
        closesocket(Socket);
    }

    MethodReturnVoid();
}

void
CMyQueue::HandleReceivedMessage(_In_ MESSAGE* pMessage)
{
    MethodEntry("pMessage->m_szType = '%S'",
                 pMessage->m_szType);

    if ((pMessage->m_cbPayload > 0) && (pMessage->m_cbPayload <= MaxCbPayload))
    {
        EnterCriticalSection(&m_SubsLock);
        
        LIST_ENTRY* pEntry = m_SubsHead.Flink;
        while (pEntry != &m_SubsHead)
        {
            CFileContext* pSub = CFileContext::FromListEntry(pEntry);
            
            pSub->HandleReceivedPublication(pMessage->m_szType, 
                                            pMessage->m_cbPayload, 
                                            pMessage->m_Payload);

            pEntry = pEntry->Flink;
        }
        
        LeaveCriticalSection(&m_SubsLock);
    }
    
    MethodReturnVoid();
}

void
CMyQueue::ConnectionEstablished(_In_ CConnection* pConnection)
{
    MethodEntry("...");
    
    EnterCriticalSection(&m_ConnectionLock);
    BOOL fFirstConnection = IsListEmpty(&m_ConnectionHead);
    InsertHeadList(&m_ConnectionHead, pConnection->GetListEntry());
    LeaveCriticalSection(&m_ConnectionLock);
    
    if (fFirstConnection)
    {
        AddArrivalEvent();
    }
    
    MESSAGE* pMessage = new MESSAGE();
    if (pMessage != NULL)
    {
        EnterCriticalSection(&m_PubsLock);
        
        LIST_ENTRY* pEntry = m_PubsHead.Flink;
        while (pEntry != &m_PubsHead)
        {
            CFileContext* pPub = CFileContext::FromListEntry(pEntry);
            if (pPub->IsEnabled())
            {
                pMessage->Initialize(pPub->GetType(), pPub->GetSize(), pPub->GetPayload());
                if (SUCCEEDED(pConnection->TransmitMessage(pMessage)))
                {
                    pPub->HandleMessageTransmitted();
                }
            }
            pEntry = pEntry->Flink;
        }

        LeaveCriticalSection(&m_PubsLock);
        delete pMessage;
    }
    
    MethodReturnVoid();
}

BOOL
CMyQueue::ConnectionTerminated(_In_ CConnection* pConnection)
{
    MethodEntry("pConnection = 0x%p",
                 pConnection);

    LIST_ENTRY* pRemoveListEntry = pConnection->GetListEntry();
    
    EnterCriticalSection(&m_ConnectionLock);
    LIST_ENTRY* pEntry = m_ConnectionHead.Flink;
    while (pEntry != &m_ConnectionHead)
    {
        if (pEntry == pRemoveListEntry)
        {
            RemoveEntryList(pEntry);
            break;
        }
        pEntry = pEntry->Flink;
    }
    BOOL fNoMoreConnections = IsListEmpty(&m_ConnectionHead);
    
    LeaveCriticalSection(&m_ConnectionLock);

    BOOL fConnectionDeleted = FALSE;
    if (pConnection->IsInboundConnection())
    {
        delete pConnection;
        fConnectionDeleted = TRUE;
    }

    if (fNoMoreConnections)
    {
        AddRemovalEvent();
    }

    MethodReturnBool(fConnectionDeleted);
}

void
CMyQueue::AddArrivalEvent()
{
    MethodEntry("void");
    
    EnterCriticalSection(&m_SubsLock);
    
    for (LIST_ENTRY* pEntry = m_ArrivalSubsHead.Flink;
         pEntry != &m_ArrivalSubsHead;
         pEntry = pEntry->Flink)
    {
        CFileContext::FromListEntry(pEntry)->HandleArrivalEvent();
    }

    LeaveCriticalSection(&m_SubsLock);
    
    MethodReturnVoid();
}

void
CMyQueue::AddRemovalEvent()
{
    MethodEntry("void");
    
    EnterCriticalSection(&m_SubsLock);
    
    for (LIST_ENTRY* pEntry = m_DepartureSubsHead.Flink;
         pEntry != &m_DepartureSubsHead;
         pEntry = pEntry->Flink)
    {
        CFileContext::FromListEntry(pEntry)->HandleRemovalEvent();
    }

    LeaveCriticalSection(&m_SubsLock);
    
    MethodReturnVoid();
}

STDMETHODIMP_(void)
CMyQueue::OnCleanup(
    _In_ IWDFObject* /*pWdfObject*/
    )
{
    MethodEntry("...");
    
    //
    // CMyQueue has a reference to framework device object via m_Queue. 
    // Framework queue object has a reference to CMyQueue object via the callbacks. 
    // This leads to circular reference and both the objects can't be destroyed until this circular reference is broken. 
    // To break the circular reference we release the reference to the framework queue object here in OnCleanup.
    //

    m_FxQueue = NULL;

    MethodReturnVoid();
}

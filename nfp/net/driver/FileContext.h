/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    filecontext.h

Abstract:

    This header file defines the structure type for context associated with the file object 

Environment:

    user mode only

Revision History:

--*/
#pragma once

class CMyPayload
{
public:
    CMyPayload()
    {
        m_pbPayload = NULL;
        m_cbPayload = 0;
        
        InitializeListHead(&m_ListEntry);
    }
    ~CMyPayload()
    {
        if (m_pbPayload != NULL)
        {
            delete [] m_pbPayload;            
            m_pbPayload = NULL;
        }
    }

    STDMETHOD(Initialize)(
        _In_                   DWORD cbPayload, 
        _In_reads_bytes_(cbPayload) PBYTE pbPayload
        )
    {
        HRESULT hr = S_OK;
        m_pbPayload = new BYTE[cbPayload];
        if (m_pbPayload != NULL)
        {
            m_cbPayload = cbPayload;
            CopyMemory(m_pbPayload, pbPayload, cbPayload);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        
        return hr;
    }

    PBYTE GetPayload()
    {
        return m_pbPayload;
    }
    DWORD GetSize()
    {
        return m_cbPayload;
    }
    
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CMyPayload* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CMyPayload*) CONTAINING_RECORD(pEntry, CMyPayload, m_ListEntry);
    }

private:
    PBYTE m_pbPayload;
    DWORD m_cbPayload;
    
    LIST_ENTRY m_ListEntry;
};

/*
 * Use this to refactor to only keep one copy of received messages
 *
class CMyPayloadItem
{
public:
    CMyPayloadItem(_In_ CMyPayload* pPayload)
    {
        m_spPayload = pPayload;
        InitializeListHead(&m_ListEntry);
    }
    ~CMyPayloadItem()
    {
    }

    PBYTE GetPayload()
    {
        return m_spPayload->GetPayload();
    }
    DWORD GetSize()
    {
        return m_spPayload->GetSize();
    }

    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CMyPayloadItem* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CMyPayloadItem*) CONTAINING_RECORD(pEntry, CMyPayloadItem, m_ListEntry);
    }

private:
    CComPtr<CMyPayload> m_spPayload;
    
    LIST_ENTRY m_ListEntry;
};
*/

class CFileContext
{
public:

    CFileContext() :
        m_Role(ROLE_UNDEFINED),
        m_pszType(NULL),
        m_fEnabled(TRUE),
        m_dwQueueSize(0),
        m_cCompleteReady(0),
        m_pConnection(NULL),
        m_pWdfRequest(NULL)
    {
        InitializeListHead(&m_SubscribedMessageQueue);

        InitializeListHead(&m_ListEntry);
        
        InitializeCriticalSection(&m_RoleLock);
    }

    ~CFileContext();

    HRESULT
    Disable();
    
    HRESULT
    Enable();
    
    HRESULT
    SetType(_In_ PCWSTR pszType);

    HRESULT
    GetNextSubscribedMessage(_In_ IRequestCallbackCancel* pCallbackCancel, _In_ IWDFIoRequest* pWdfRequest);
    
    HRESULT
    SetPayload(_In_ IWDFIoRequest* pWdfRequest);

    HRESULT
    GetNextTransmittedMessage(_In_ IRequestCallbackCancel* pCallbackCancel, _In_ IWDFIoRequest* pWdfRequest);
    
    HRESULT
    BeginProximity(
        _In_ IWDFIoRequest*       pWdfRequest,
        _In_ IConnectionCallback* pCallback
        );

    VOID
    HandleArrivalEvent();
    VOID
    HandleRemovalEvent();

    void
    HandleReceivedPublication(
        _In_                   PCWSTR pszType,
        _In_                   DWORD  cbPayload, 
        _In_reads_bytes_(cbPayload) PBYTE  pbPayload
        );
    
    void
    HandleMessageTransmitted();

    void
    OnCancel();

    void
    SetRoleSubcription()
    {
        m_Role = ROLE_SUBSCRIPTION;
    }

    void
    SetRolePublication()
    {
        m_Role = ROLE_PUBLICATION;
    }

    BOOL
    SetRoleArrivedSubcription()
    {
        if (m_Role == ROLE_SUBSCRIPTION)
        {
            m_Role = ROLE_ARRIVEDSUBSCRIPTION;
            return TRUE;
        }
        return FALSE;
    }

    BOOL
    SetRoleDepartedSubcription()
    {
        if (m_Role == ROLE_SUBSCRIPTION)
        {
            m_Role = ROLE_DEPARTEDSUBSCRIPTION;
            return TRUE;
        }
        return FALSE;
    }

    BOOL
    IsNormalSubscription()
    {
        return (m_Role == ROLE_SUBSCRIPTION);
    }
    BOOL
    IsArrivedSubscription()
    {
        return (m_Role == ROLE_ARRIVEDSUBSCRIPTION);
    }
    BOOL
    IsDepartedSubscription()
    {
        return (m_Role == ROLE_DEPARTEDSUBSCRIPTION);
    }
    BOOL
    IsSubscription()
    {
        return (m_Role == ROLE_SUBSCRIPTION);
    }
    BOOL
    IsPublication()
    {
        return (m_Role == ROLE_PUBLICATION);
    }
        
    PCWSTR
    GetType()
    {
        return m_pszType;
    }

    DWORD
    GetSize()
    {
        return m_MyPayload.GetSize();
    }

    PBYTE
    GetPayload()
    {
        return m_MyPayload.GetPayload();
    }

    BOOL
    IsEnabled()
    {
        return m_fEnabled;
    }
    
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CFileContext* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CFileContext*) CONTAINING_RECORD(pEntry, CFileContext, m_ListEntry);
    }
    
private:
    
    VOID
    CompleteOneArrivalEvent();
    VOID
    CompleteOneRemovalEvent();
    
    bool
    CompleteOneGetNextSubscribedMessage(
        _In_                       DWORD cbPayload,
        _In_reads_bytes_opt_(cbPayload) PBYTE pbPayload
        );
    
    bool
    CompleteRequest(
        _In_ HRESULT hr,
        _In_ SIZE_T  cbSize,
        _In_ bool    fIsCancelable
        );

private:

    enum ROLE
    {
        ROLE_UNDEFINED,
        ROLE_SUBSCRIPTION,
        ROLE_ARRIVEDSUBSCRIPTION,
        ROLE_DEPARTEDSUBSCRIPTION,
        ROLE_PUBLICATION,
        ROLE_PROXIMITY
    };

    ROLE m_Role;

    PWSTR m_pszType;

    BOOL m_fEnabled;

    DWORD m_dwQueueSize;
    
    // Queue of received messages
    LIST_ENTRY m_SubscribedMessageQueue; // Unique to ROLE_SUBSCRIPTION

    CMyPayload m_MyPayload;  // Unique to ROLE_PUBLICATION
    SIZE_T m_cCompleteReady; // Unique to ROLE_PUBLICATION, ROLE_ARRIVEDSUBSCRIPTION, and ROLE_DEPARTEDSUBSCRIPTION

    CConnection* m_pConnection; // Unique to ROLE_PROXIMITY
    
    // Pended "Get Next" Request.
    IWDFIoRequest* m_pWdfRequest;

    // The Fx File object this CFileObject is a companion to
    IWDFFile* m_pWdfFile;

    CRITICAL_SECTION m_RoleLock;
    
    LIST_ENTRY m_ListEntry;
};

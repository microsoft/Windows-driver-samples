/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    filecontext.h

Abstract:

    This header file defines the structure type for context associated with the file object 

Environment:

    Windows User-Mode Driver Framework (WUDF)

Revision History:

--*/
#pragma once

class CPayload
{
public:
    CPayload()
    {
        m_pbPayload = nullptr;
        m_cbPayload = 0;
        InitializeListHead(&m_ListEntry);
    }
    ~CPayload()
    {
        SAFE_DELETEARRAY(m_pbPayload);
    }
    NTSTATUS Initialize(
        _In_ DWORD cbPayload, 
        _In_reads_bytes_(cbPayload) PBYTE pbPayload
        )
    {
        NTSTATUS Status = STATUS_SUCCESS;

        m_pbPayload = new BYTE[cbPayload];
        
        if (m_pbPayload != nullptr) {
            m_cbPayload = cbPayload;
            RtlCopyMemory(m_pbPayload, pbPayload, cbPayload);
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        return Status;
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
    static CPayload* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CPayload*) CONTAINING_RECORD(pEntry, CPayload, m_ListEntry);
    }

private:
    PBYTE       m_pbPayload;
    DWORD       m_cbPayload;    
    LIST_ENTRY  m_ListEntry;
};

class CFileObject
{
public:
    CFileObject(WDFFILEOBJECT FileObject);
    ~CFileObject();

public:
    static EVT_WDF_OBJECT_CONTEXT_DESTROY OnDestroy;
    static EVT_WDF_REQUEST_CANCEL OnRequestCancel;

public:
    NTSTATUS Enable();
    NTSTATUS Disable();
    NTSTATUS SetType(_In_ PCWSTR pszType);
    NTSTATUS GetNextSubscribedMessage(_In_ WDFREQUEST Request);
    NTSTATUS SetPayload(_In_ DWORD cbPayload, _In_reads_bytes_(cbPayload) PBYTE pbPayload);
    NTSTATUS GetNextTransmittedMessage(_In_ WDFREQUEST Request);
    NTSTATUS GetNextSecureElementPayload(_In_ WDFREQUEST Request);
    NTSTATUS SubscribeForEvent(_In_ GUID& SecureElementId, _In_ SECURE_ELEMENT_EVENT_TYPE SecureElementEventType);
    NTSTATUS BeginProximity(_In_ BEGIN_PROXIMITY_ARGS *pArgs, _In_ IConnectionCallback* pCallback);

    void HandleArrivalEvent();
    void HandleRemovalEvent();
    void HandleReceivedMessage(_In_ PCWSTR pszType, _In_ DWORD cbPayload, _In_reads_bytes_(cbPayload) PBYTE pbPayload);
    void HandleMessageTransmitted();
    void HandleReceiveHcePacket(_In_ USHORT uConnectionId, _In_ DWORD cbPayload, _In_reads_bytes_(cbPayload) PBYTE pbPayload);
    void HandleSecureElementEvent(SECURE_ELEMENT_EVENT_INFO* pInfo);

public:
    void SetRoleSubcription()
    {
        m_Role = ROLE_SUBSCRIPTION;
    }
    void SetRolePublication()
    {
        m_Role = ROLE_PUBLICATION;
    }
    BOOL SetRoleArrivedSubcription()
    {
        if (m_Role == ROLE_SUBSCRIPTION)
        {
            m_Role = ROLE_ARRIVEDSUBSCRIPTION;
            return TRUE;
        }
        return FALSE;
    }
    BOOL SetRoleDepartedSubcription()
    {
        if (m_Role == ROLE_SUBSCRIPTION)
        {
            m_Role = ROLE_DEPARTEDSUBSCRIPTION;
            return TRUE;
        }
        return FALSE;
    }
    void SetRoleSmartCardReader()
    {
        m_Role = ROLE_SMARTCARDREADER;
    }
    void SetRoleSecureElementEvent()
    {
        m_Role = ROLE_SECUREELEMENTEVENT;
    }
    void SetRoleSecureElementManager()
    {
        m_Role = ROLE_SECUREELEMENTMANAGER;
    }
    void SetRoleSimulation()
    {
        m_Role = ROLE_SIMULATION;
    }
    void SetRoleRadioManager()
    {
        m_Role = ROLE_RADIOMANAGER;
    }

public:
    BOOL IsNormalSubscription()
    {
        return (m_Role == ROLE_SUBSCRIPTION);
    }
    BOOL IsArrivedSubscription()
    {
        return (m_Role == ROLE_ARRIVEDSUBSCRIPTION);
    }
    BOOL IsDepartedSubscription()
    {
        return (m_Role == ROLE_DEPARTEDSUBSCRIPTION);
    }
    BOOL IsSubscription()
    {
        return (m_Role == ROLE_SUBSCRIPTION) || (m_Role == ROLE_ARRIVEDSUBSCRIPTION) || (m_Role == ROLE_DEPARTEDSUBSCRIPTION);
    }
    BOOL IsPublication()
    {
        return (m_Role == ROLE_PUBLICATION);
    }
    BOOL IsSmartCardReader()
    {
        return (m_Role == ROLE_SMARTCARDREADER);
    }
    BOOL IsSecureElementEvent()
    {
        return (m_Role == ROLE_SECUREELEMENTEVENT);
    }
    BOOL IsSecureElementManager()
    {
        return (m_Role == ROLE_SECUREELEMENTMANAGER);
    }
    BOOL IsRoleSimulation()
    {
        return (m_Role == ROLE_SIMULATION);
    }
    BOOL IsRoleRadioManager()
    {
        return (m_Role == ROLE_RADIOMANAGER);
    }
    BOOL IsRoleUndefined()
    {
        return (m_Role == ROLE_UNDEFINED);
    }
    PCWSTR GetType()
    {
        return m_pszType;
    }
    DWORD GetSize()
    {
        return m_Payload.GetSize();
    }
    PBYTE GetPayload()
    {
        return m_Payload.GetPayload();
    }
    BOOL IsEnabled()
    {
        return m_fEnabled;
    }
    GUID& GetSecureElementId()
    {
        return m_SecureElementId;
    }
    SECURE_ELEMENT_EVENT_TYPE GetSecureElementEventType()
    {
        return m_SecureElementEventType;
    }
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CFileObject* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CFileObject*) CONTAINING_RECORD(pEntry, CFileObject, m_ListEntry);
    }

private:
    void HandleReceivedMessage(_In_ DWORD cbPayload, _In_reads_bytes_(cbPayload) PBYTE pbPayload);
    bool CompleteRequest(_In_ DWORD cbPayload, _In_reads_bytes_opt_(cbPayload) PBYTE pbPayload);
    bool CompleteRequest(_In_ NTSTATUS CompletionStatus, _In_ size_t cbSize, _In_ bool fIsCancelable);
    void PurgeQueue();
    void Cancel();

private:
    enum ROLE
    {
        ROLE_UNDEFINED,
        ROLE_SUBSCRIPTION,
        ROLE_ARRIVEDSUBSCRIPTION,
        ROLE_DEPARTEDSUBSCRIPTION,
        ROLE_PUBLICATION,
        ROLE_SMARTCARDREADER,
        ROLE_SECUREELEMENTEVENT,
        ROLE_SECUREELEMENTMANAGER,
        ROLE_SIMULATION,
        ROLE_RADIOMANAGER
    };

    WDFFILEOBJECT               m_FileObject;             // The Fx File object this CFileObject is associated with
    ROLE                        m_Role;
    PWSTR                       m_pszType;
    BOOL                        m_fEnabled;
    DWORD                       m_dwQueueSize;
    LIST_ENTRY                  m_Queue;                  // Unique to ROLE_SUBSCRIPTION and ROLE_SECUREELEMENTEVENT
    CPayload                    m_Payload;                // Unique to ROLE_PUBLICATION
    size_t                      m_cCompleteReady;         // Unique to ROLE_PUBLICATION
    CConnection*                m_pConnection;            // Unique to ROLE_SIMULATION
    WDFREQUEST                  m_Request;                // Pended "Get Next" Request
    CRITICAL_SECTION            m_RoleLock;
    LIST_ENTRY                  m_ListEntry;
    GUID                        m_SecureElementId;        // Unique to ROLE_SECUREELEMENTEVENT
    SECURE_ELEMENT_EVENT_TYPE   m_SecureElementEventType; // Unique to ROLE_SECUREELEMENTEVENT
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CFileObject, GetFileObject);
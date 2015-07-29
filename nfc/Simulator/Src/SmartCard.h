/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smartcard.h

Abstract:

    The SmartCard simulates the communication channel with the selected smart card.

Environment:

    User Mode

--*/
#pragma once

static const int MaxResponsePayload = 1024;
static const int ResponseTimeoutInMs = 10000;

#define SMARTCARD_MESSAGE_TYPE_TRANSMIT L"SCTransmit"

class CSmartCard;

class CSmartCardRequest
{
public:
    CSmartCardRequest(_In_ CSmartCard *pSmartCard);
    ~CSmartCardRequest();

public:
    VOID CompleteRequest(_In_ DWORD cbPayload, _In_reads_bytes_(cbPayload) PBYTE pbPayload);
    NTSTATUS WaitForCompletion();

public:
    DWORD GetSequenceNum()
    {
        return m_dwSequenceNum;
    }
    DWORD GetPayloadSize()
    {
        return m_cbPayload;
    }
    BYTE* GetPayload()
    {
        return (BYTE*)m_Payload;
    }
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CSmartCardRequest* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CSmartCardRequest*) CONTAINING_RECORD(pEntry, CSmartCardRequest, m_ListEntry);
    }

private:
    CSmartCard*         m_pSmartCard;                   // smart card object
    HANDLE              m_hCompletionEvent;             // event indicate if the request is completed
    DWORD               m_dwSequenceNum;                // sequence number for the transmit request/response
    DWORD               m_cbPayload;                    // size of the payload
    BYTE                m_Payload[MaxResponsePayload];  // payload buffer
    LIST_ENTRY          m_ListEntry;                    // list entry
};

class CSmartCard
{
public:
    CSmartCard(_In_ WDFDEVICE Device, _In_ CConnection* pConnection);
    ~CSmartCard();

public:
    NTSTATUS SendCommand(
        _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
        _In_ size_t InputBufferLength,
        _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
        _In_ size_t OutputBufferLength,
        _Out_ size_t* BytesTransferred);

    BOOL ResponseReceived(_In_ MESSAGE* pMessage);

    NTSTATUS GetAtr(_Out_writes_bytes_(*pcbAtr) PBYTE pbAtr, _Inout_ LPDWORD pcbAtr);
    NTSTATUS GetIccType(_Out_writes_bytes_(*pcbIccType) PBYTE pbIccType, _Inout_ LPDWORD pcbIccType);

private:
    DWORD AddRequest(_In_ CSmartCardRequest *pRequest);
    VOID RemoveRequest(_In_ CSmartCardRequest *pRequest);

private:
    friend class CSmartCardRequest;

    WDFDEVICE               m_Device;
    CRITICAL_SECTION        m_ConnectionLock;         // lock for the connection pointer
    CConnection*            m_pConnection;            // connection object
    DWORD volatile          m_dwSequenceNum;          // sequence number of the transmit command
    LIST_ENTRY              m_RequestList;            // list of pending request
    CRITICAL_SECTION        m_RequestListLock;        // lock for the response list
};

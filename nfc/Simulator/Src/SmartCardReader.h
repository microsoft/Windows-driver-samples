/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smartcardreader.h

Abstract:

    The SmartCardReader encapsulate all the smart card related operations.

Environment:

    User Mode

--*/
#pragma once

class CSmartCardReader
{
public:
    CSmartCardReader(WDFDEVICE Device);
    ~CSmartCardReader();

public:
    NTSTATUS Initialize();
    VOID Deinitialize();
    NTSTATUS AddClient(CFileObject *pFileObject);
    VOID RemoveClient(CFileObject *pFileObject);

    NTSTATUS GetAttribute(_In_ DWORD dwAttributeId, _Out_writes_bytes_(*pcbAttributeValue) PBYTE pbAttributeValue, _Inout_ LPDWORD pcbAttributeValue);
    NTSTATUS SetAttribute(_In_ DWORD dwAttributeId);
    NTSTATUS IsAbsent(_In_ WDFREQUEST Request);
    NTSTATUS IsPresent(_In_ WDFREQUEST Request);
    NTSTATUS SetPower(_In_ DWORD dwPower);
    NTSTATUS SetProtocol(_In_ DWORD dwProtocol, _Out_ DWORD *pdwSelectedProtocol);

    NTSTATUS Transmit(
        _In_reads_bytes_opt_(InputBufferLength) PBYTE InputBuffer,
        _In_ size_t InputBufferLength,
        _Out_writes_bytes_opt_(OutputBufferLength) PBYTE OutputBuffer,
        _In_ size_t OutputBufferLength,
        _In_ size_t* BytesTransferred);

    DWORD GetState();
    BOOL CardArrived(_In_ CConnection* pConnection);
    BOOL CardRemoved(_In_ CConnection* pConnection);
    BOOL MessageReceived(_In_ MESSAGE* pMessage);

private:
    typedef NTSTATUS
    (CSmartCardReader::EVT_DISPATCH_HANDLER) (
        _In_opt_bytecount_(cbAttributeValue) PBYTE pbAttributeValue,
        _In_ DWORD cbAttributeValue,
        _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
        _Inout_ LPDWORD pcbOutputBuffer);

    typedef NTSTATUS
    (CSmartCardReader::*PFN_DISPATCH_HANDLER) (
        _In_opt_bytecount_(cbAttributeValue) PBYTE pbAttributeValue,
        _In_ DWORD cbAttributeValue,
        _Out_bytecap_(*pcbOutputBuffer) PBYTE pbOutputBuffer,
        _Inout_ LPDWORD pcbOutputBuffer);

    EVT_DISPATCH_HANDLER GetAttributeGeneric;
    EVT_DISPATCH_HANDLER GetCurrentProtocolType;
    EVT_DISPATCH_HANDLER GetIccPresence;
    EVT_DISPATCH_HANDLER GetAtrString;
    EVT_DISPATCH_HANDLER GetIccTypePerAtr;

private:
    typedef struct _DISPATCH_ENTRY {
        DWORD dwAttributeId;
        PBYTE pbAttributeValue;
        size_t cbAttributeValue;
        PFN_DISPATCH_HANDLER pfnDispatchHandler;
    } DISPATCH_ENTRY, *PDISPATCH_ENTRY;

    WDFDEVICE               m_Device;
    WDFQUEUE                m_PresentQueue;
    WDFQUEUE                m_AbsentQueue;
    CSmartCard*             m_pSmartCard;
    CRITICAL_SECTION        m_SmartCardLock;
    BOOL                    m_fVirtualPowered;
    CFileObject*            m_pCurrentClient;
};

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    RoutingTable.h

Abstract:

    This header file defines the listen mode routing table

Environment:

    User Mode

--*/
#pragma once

class CRoutingTable
{
public:
    CRoutingTable(_In_ CQueue *pQueue);
    ~CRoutingTable();

    VOID Initialize(_In_ WDFDEVICE Device);
    NTSTATUS SetRoutingTable(_In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable);
    NTSTATUS GetRoutingTable(_Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer, size_t OutputBufferLength, DWORD* pcbOutputBuffer);

    FORCEINLINE BOOLEAN IsAidRoutingSupported() const { return m_IsAidRoutingSupported; }
    FORCEINLINE BOOLEAN IsTechRoutingSupported() const { return m_IsTechRoutingSupported; }
    FORCEINLINE BOOLEAN IsProtocolRoutingSupported() const { return m_IsProtocolRoutingSupported; }
    FORCEINLINE USHORT MaxRoutingTableSize() const { return m_cbMaxRoutingTableSize; }

private:
    NTSTATUS ValidateRoutingTable(_In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable);
    NTSTATUS ValidateAidRoute(_In_ PSECURE_ELEMENT_ROUTING_TABLE_ENTRY pRoutingEntry);
    NTSTATUS ValidateProtocolRoute(_In_ PSECURE_ELEMENT_ROUTING_TABLE_ENTRY pRoutingEntry);
    NTSTATUS ValidateTechRoute(_In_ PSECURE_ELEMENT_ROUTING_TABLE_ENTRY pRoutingEntry);
    NTSTATUS ValidateRouteUnique(_In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable, _In_ DWORD EndIndex);

    NTSTATUS SetDefaultRoutingTable();
    NTSTATUS ResizeRoutingTable(DWORD NumberOfEntries);

private:
    CQueue*                 m_pQueue;
    PSECURE_ELEMENT_ROUTING_TABLE
                            m_pRoutingTable;
    CRITICAL_SECTION        m_RoutingTableLock;
    DWORD                   m_RoutingTableMaxSize;
    BOOLEAN                 m_IsAidRoutingSupported;
    BOOLEAN                 m_IsTechRoutingSupported;
    BOOLEAN                 m_IsProtocolRoutingSupported;
    USHORT                  m_cbMaxRoutingTableSize;
};

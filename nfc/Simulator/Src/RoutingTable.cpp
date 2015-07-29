/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    routingtable.cpp

Abstract:

    Implements the NFC routing table class

Environment:

    User-mode only.

--*/

#include "Internal.h"
#include "RoutingTable.tmh"

#define MAX_ROUTING_TABLE_SIZE_KEY          L"MaxRoutingTableSize"
#define AID_ROUTING_SUPPORTED_KEY           L"IsAidRoutingSupported"
#define PROTOCOL_ROUTING_SUPPORTED_KEY      L"IsProtRoutingSupported"
#define TECH_ROUTING_SUPPORTED_KEY          L"IsTechRoutingSupported"

#define DEFAULT_MAX_ROUTING_TABLE_SIZE      255

CRoutingTable::CRoutingTable(_In_ CQueue *pQueue)
    : m_pQueue(pQueue),
      m_pRoutingTable(nullptr),
      m_RoutingTableMaxSize(0),
      m_IsAidRoutingSupported(TRUE),
      m_IsTechRoutingSupported(TRUE),
      m_IsProtocolRoutingSupported(TRUE),
      m_cbMaxRoutingTableSize(DEFAULT_MAX_ROUTING_TABLE_SIZE)
{
    InitializeCriticalSection(&m_RoutingTableLock);
}

CRoutingTable::~CRoutingTable()
{
    SAFE_DELETEARRAY(m_pRoutingTable);
    m_RoutingTableMaxSize = 0;
    DeleteCriticalSection(&m_RoutingTableLock);
}

VOID CRoutingTable::Initialize(WDFDEVICE Device)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    WDFKEY KeyHandle = nullptr;
    ULONG uValue = 0;

    DECLARE_CONST_UNICODE_STRING(MaxRoutingTableSizeKey, MAX_ROUTING_TABLE_SIZE_KEY);
    DECLARE_CONST_UNICODE_STRING(AidRoutingSupportedKey, AID_ROUTING_SUPPORTED_KEY);
    DECLARE_CONST_UNICODE_STRING(ProtocolRoutingSupportedKey, PROTOCOL_ROUTING_SUPPORTED_KEY);
    DECLARE_CONST_UNICODE_STRING(TechRoutingSupportedKey, TECH_ROUTING_SUPPORTED_KEY);

    Status = WdfDeviceOpenRegistryKey(
                    Device,
                    PLUGPLAY_REGKEY_DEVICE | WDF_REGKEY_DEVICE_SUBKEY,
                    KEY_READ,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &KeyHandle);

    if (NT_SUCCESS(Status) && (KeyHandle != nullptr)) {
        Status = WdfRegistryQueryULong(
                            KeyHandle,
                            &MaxRoutingTableSizeKey,
                            &uValue);

        if (NT_SUCCESS(Status) && uValue <= ((USHORT)0xFFFF)) { // MAX_USHORT
            TraceInfo("MaxRoutingTableSize=%d", uValue);
            m_cbMaxRoutingTableSize = (USHORT)uValue;
        }

        Status = WdfRegistryQueryULong(
            KeyHandle,
            &AidRoutingSupportedKey,
            &uValue);

        if (NT_SUCCESS(Status)) {
            TraceInfo("IsAidRoutingSupported=%d", uValue);
            m_IsAidRoutingSupported = (uValue != 0);
        }

        Status = WdfRegistryQueryULong(
            KeyHandle,
            &ProtocolRoutingSupportedKey,
            &uValue);

        if (NT_SUCCESS(Status)) {
            TraceInfo("IsProtocolRoutingSupported=%d", uValue);
            m_IsProtocolRoutingSupported = (uValue != 0);
        }

        Status = WdfRegistryQueryULong(
            KeyHandle,
            &TechRoutingSupportedKey,
            &uValue);

        if (NT_SUCCESS(Status)) {
            TraceInfo("IsTechRoutingSupported=%d", uValue);
            m_IsTechRoutingSupported = (uValue != 0);
        }

        WdfRegistryClose(KeyHandle);
    }

    Status = SetDefaultRoutingTable();

    MethodReturnVoid();
}

NTSTATUS CRoutingTable::ValidateRoutingTable(_In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD cbTableSize = NCI_PROTO_ROUTING_ENTRY_SIZE; // Implicit NFC-DEP route

    for (DWORD nIndex = 0; (nIndex < pRoutingTable->NumberOfEntries) && NT_SUCCESS(Status); nIndex++) {
        switch (pRoutingTable->TableEntries[nIndex].eRoutingType) {
        case RoutingTypeAid:
            Status = ValidateAidRoute(&pRoutingTable->TableEntries[nIndex]);
            cbTableSize += NCI_AID_ROUTING_ENTRY_SIZE(pRoutingTable->TableEntries[nIndex].AidRoutingInfo.cbAid);
            break;

        case RoutingTypeProtocol:
            Status = ValidateProtocolRoute(&pRoutingTable->TableEntries[nIndex]);
            cbTableSize += NCI_PROTO_ROUTING_ENTRY_SIZE;
            break;

        case RoutingTypeTech:
            Status = ValidateTechRoute(&pRoutingTable->TableEntries[nIndex]);
            cbTableSize += NCI_TECH_ROUTING_ENTRY_SIZE;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (NT_SUCCESS(Status)) {
            Status = ValidateRouteUnique(pRoutingTable, nIndex);
        }
    }

    if (NT_SUCCESS(Status) && cbTableSize > MaxRoutingTableSize()) {
        Status = STATUS_INVALID_BUFFER_SIZE;
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CRoutingTable::ValidateAidRoute(_In_ PSECURE_ELEMENT_ROUTING_TABLE_ENTRY pRoutingEntry)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    NT_ASSERT(pRoutingEntry->eRoutingType == RoutingTypeAid);

    if (!IsAidRoutingSupported()) {
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if ((pRoutingEntry->AidRoutingInfo.cbAid < ISO_7816_MINIMUM_AID_LENGTH) ||
        (pRoutingEntry->AidRoutingInfo.cbAid > ISO_7816_MAXIMUM_AID_LENGTH)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CRoutingTable::ValidateProtocolRoute(_In_ PSECURE_ELEMENT_ROUTING_TABLE_ENTRY pRoutingEntry)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    NT_ASSERT(pRoutingEntry->eRoutingType == RoutingTypeProtocol);

    if (!IsProtocolRoutingSupported()) {
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if (pRoutingEntry->ProtoRoutingInfo.eRfProtocolType != PROTOCOL_T1T &&
        pRoutingEntry->ProtoRoutingInfo.eRfProtocolType != PROTOCOL_T2T &&
        pRoutingEntry->ProtoRoutingInfo.eRfProtocolType != PROTOCOL_T3T &&
        pRoutingEntry->ProtoRoutingInfo.eRfProtocolType != PROTOCOL_ISO_DEP) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CRoutingTable::ValidateTechRoute(_In_ PSECURE_ELEMENT_ROUTING_TABLE_ENTRY pRoutingEntry)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    NT_ASSERT(pRoutingEntry->eRoutingType == RoutingTypeTech);

    if (!IsTechRoutingSupported()) {
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if (pRoutingEntry->TechRoutingInfo.eRfTechType != NFC_RF_TECHNOLOGY_A &&
        pRoutingEntry->TechRoutingInfo.eRfTechType != NFC_RF_TECHNOLOGY_B &&
        pRoutingEntry->TechRoutingInfo.eRfTechType != NFC_RF_TECHNOLOGY_F) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CRoutingTable::ValidateRouteUnique(_In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable, _In_ DWORD EndIndex)
{
    _Analysis_assume_(EndIndex < pRoutingTable->NumberOfEntries);

    for (DWORD nIndex = 0; (nIndex < EndIndex); nIndex++) {
        if (pRoutingTable->TableEntries[nIndex].eRoutingType !=
            pRoutingTable->TableEntries[EndIndex].eRoutingType) {
            continue;
        }

        switch (pRoutingTable->TableEntries[nIndex].eRoutingType) {
        case RoutingTypeAid:
            if ((pRoutingTable->TableEntries[nIndex].AidRoutingInfo.cbAid ==
                    pRoutingTable->TableEntries[EndIndex].AidRoutingInfo.cbAid) &&
                (memcmp(pRoutingTable->TableEntries[nIndex].AidRoutingInfo.pbAid,
                        pRoutingTable->TableEntries[EndIndex].AidRoutingInfo.pbAid,
                        pRoutingTable->TableEntries[nIndex].AidRoutingInfo.cbAid) == 0)) {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        case RoutingTypeProtocol:
            if (pRoutingTable->TableEntries[nIndex].ProtoRoutingInfo.eRfProtocolType ==
                pRoutingTable->TableEntries[EndIndex].ProtoRoutingInfo.eRfProtocolType) {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        case RoutingTypeTech:
            if (pRoutingTable->TableEntries[nIndex].TechRoutingInfo.eRfTechType ==
                pRoutingTable->TableEntries[EndIndex].TechRoutingInfo.eRfTechType) {
                return STATUS_INVALID_PARAMETER;
            }
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS CRoutingTable::GetRoutingTable(_Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer, size_t OutputBufferLength, DWORD* pcbOutputBuffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable = (PSECURE_ELEMENT_ROUTING_TABLE)OutputBuffer;

    _Analysis_assume_(sizeof(DWORD) <= OutputBufferLength);

    EnterCriticalSection(&m_RoutingTableLock);

    pRoutingTable->NumberOfEntries = m_pRoutingTable->NumberOfEntries;
    *pcbOutputBuffer = sizeof(m_pRoutingTable->NumberOfEntries);

    if ((*pcbOutputBuffer + m_pRoutingTable->NumberOfEntries * sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY)) > OutputBufferLength) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    RtlCopyMemory(pRoutingTable->TableEntries, m_pRoutingTable->TableEntries, m_pRoutingTable->NumberOfEntries * sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY));
    *pcbOutputBuffer += m_pRoutingTable->NumberOfEntries * sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY);

Exit:
    LeaveCriticalSection(&m_RoutingTableLock);
    return Status;
}

NTSTATUS CRoutingTable::SetDefaultRoutingTable()
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;
    GUID SecureElementId = GUID_NULL;

    Status = m_pQueue->GetSecureElementId(External, &SecureElementId);

    if (NT_SUCCESS(Status))
    {
        if (IsTechRoutingSupported()) {
            SECURE_ELEMENT_ROUTING_TABLE_AND_ENTRIES(3) RoutingTable;

            RoutingTable.Table.NumberOfEntries = 3;
            RoutingTable.Table.TableEntries[0].eRoutingType = RoutingTypeTech;
            RoutingTable.Table.TableEntries[0].TechRoutingInfo.eRfTechType = NFC_RF_TECHNOLOGY_A;
            RoutingTable.Table.TableEntries[0].TechRoutingInfo.guidSecureElementId = SecureElementId;
            RoutingTable.Table.TableEntries[1].eRoutingType = RoutingTypeTech;
            RoutingTable.Table.TableEntries[1].TechRoutingInfo.eRfTechType = NFC_RF_TECHNOLOGY_B;
            RoutingTable.Table.TableEntries[1].TechRoutingInfo.guidSecureElementId = SecureElementId;
            RoutingTable.Table.TableEntries[2].eRoutingType = RoutingTypeTech;
            RoutingTable.Table.TableEntries[2].TechRoutingInfo.eRfTechType = NFC_RF_TECHNOLOGY_F;
            RoutingTable.Table.TableEntries[2].TechRoutingInfo.guidSecureElementId = SecureElementId;

            Status = SetRoutingTable(&RoutingTable.Table);
        }
        else if (IsProtocolRoutingSupported()) {
            SECURE_ELEMENT_ROUTING_TABLE RoutingTable;

            RoutingTable.NumberOfEntries = 1;
            RoutingTable.TableEntries[0].eRoutingType = RoutingTypeProtocol;
            RoutingTable.TableEntries[0].TechRoutingInfo.eRfTechType = PROTOCOL_ISO_DEP;
            RoutingTable.TableEntries[0].TechRoutingInfo.guidSecureElementId = SecureElementId;

            Status = SetRoutingTable(&RoutingTable);
        }
        else {
            Status = STATUS_NOT_SUPPORTED;
            TraceInfo("Neither Technology and Procotol routing is supported");
        }
    }

    MethodReturn(Status, "Status = %!STATUS!", Status);
}

NTSTATUS CRoutingTable::ResizeRoutingTable(DWORD NumberOfEntries)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DWORD cbRoutingTable = sizeof(SECURE_ELEMENT_ROUTING_TABLE) +
                           sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY) * (NumberOfEntries-1);

    if (m_RoutingTableMaxSize >= NumberOfEntries) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    SAFE_DELETEARRAY(m_pRoutingTable);
    m_RoutingTableMaxSize = 0;

    m_pRoutingTable = (PSECURE_ELEMENT_ROUTING_TABLE) new BYTE[cbRoutingTable];

    if (m_pRoutingTable == nullptr) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    m_pRoutingTable->NumberOfEntries = 0;
    m_RoutingTableMaxSize = NumberOfEntries;

Exit:
    return Status;
}

NTSTATUS CRoutingTable::SetRoutingTable(_In_ PSECURE_ELEMENT_ROUTING_TABLE pRoutingTable)
{
    NTSTATUS Status = ValidateRoutingTable(pRoutingTable);
    
    if (NT_SUCCESS(Status)) {
        EnterCriticalSection(&m_RoutingTableLock);

        Status = ResizeRoutingTable(pRoutingTable->NumberOfEntries);

        if (NT_SUCCESS(Status)) {
            DWORD cbSize = sizeof(SECURE_ELEMENT_ROUTING_TABLE) + (pRoutingTable->NumberOfEntries - 1) * sizeof(SECURE_ELEMENT_ROUTING_TABLE_ENTRY);
            RtlCopyMemory(m_pRoutingTable, pRoutingTable, cbSize);
        }

        LeaveCriticalSection(&m_RoutingTableLock);
    }

    return Status;
}

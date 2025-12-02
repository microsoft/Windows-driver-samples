//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

NTSTATUS
MbbNtbDetectNdp32Loop(_In_ PNCM_NTH32 Nth)
{
    PNCM_NDP32 FirstNdp;
    PNCM_NDP32 Ndp;
    PNCM_NDP32 LoopNdp;

    if ((FirstNdp = MBB_NTH32_GET_FIRST_NDP(Nth)) == NULL)
        return STATUS_SUCCESS;

    if (!MBB_NTB32_IS_VALID_NDP_LENGTH(Nth, FirstNdp))
    {
        return STATUS_UNSUCCESSFUL;
    }
    LoopNdp = MBB_NDP32_GET_NEXT_NDP(Nth, FirstNdp);

    for (Ndp = FirstNdp; Ndp != NULL && LoopNdp != NULL; Ndp = MBB_NDP32_GET_NEXT_NDP(Nth, Ndp))
    {
        if (!MBB_NTB32_IS_VALID_NDP_LENGTH(Nth, LoopNdp))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (!MBB_NTB32_IS_VALID_NDP_LENGTH(Nth, Ndp))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (LoopNdp == Ndp)
        {
            return STATUS_UNSUCCESSFUL;
        }

        if ((LoopNdp = MBB_NDP32_GET_NEXT_NDP(Nth, LoopNdp)) != NULL)
        {
            if (!MBB_NTB32_IS_VALID_NDP_LENGTH(Nth, LoopNdp))
            {
                return STATUS_UNSUCCESSFUL;
            }
            LoopNdp = MBB_NDP32_GET_NEXT_NDP(Nth, LoopNdp);
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MbbNtbValidateNdp32(_In_ PNCM_NTH32 Nth, _In_ PNCM_NDP32 Ndp)
{
    ULONG Index;
    ULONG DatagramCount = MBB_NDP32_GET_DATAGRAM_COUNT(Ndp);

    if (!MBB_NTB32_IS_VALID_NDP_SIGNATURE(Ndp))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (MBB_NDP32_GET_SIGNATURE_TYPE(Ndp) == NCM_NDP32_IPS)
    {
        //
        // Check if the session id is valid, else discard
        //

        // Check if session Id is greater than maximum supported. Here
        // we can also check against MaxActivatedContexts, but this would
        // mean we need to take a lock for getting the value of maxactivatedcontexts
        // The lock can be avoided by checking against the maximum number of ports
        // supported by the class driver. SESSION_PORT_TABLE.InUse can be used
        // to check whether the session id is in use.

        if (MBB_NDP32_GET_SESSIONID(Ndp) >= MBB_MAX_NUMBER_OF_SESSIONS)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (!MBB_NTB32_IS_VALID_NDP_LENGTH(Nth, Ndp))
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (Index = 0; Index < DatagramCount; Index++)
    {
        if (MBB_NTB32_IS_END_DATAGRAM(Nth, Ndp, Index))
            return STATUS_SUCCESS;

        if (!MBB_NTB32_IS_VALID_DATAGRAM(Nth, Ndp, Index))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MbbNtbValidateNth32(_In_ PNCM_NTH32 Nth, _In_ ULONG BufferLength, _Out_opt_ ULONG* NdpCount)
{
    PNCM_NDP32 Ndp;
    ULONG ndpCount = 0;
    if (BufferLength < sizeof(NCM_NTH32))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH32_IS_VALID_SIGNATURE(Nth))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH32_IS_VALID_HEADER_LENGTH(Nth))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH32_IS_VALID_BLOCK_LENGTH(Nth, BufferLength))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH32_IS_VALID_FIRST_NDP(Nth))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (MbbNtbDetectNdp32Loop(Nth) != STATUS_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    for (Ndp = MBB_NTH32_GET_FIRST_NDP(Nth); Ndp != NULL; Ndp = MBB_NDP32_GET_NEXT_NDP(Nth, Ndp))
    {
        if (MbbNtbValidateNdp32(Nth, Ndp) != STATUS_SUCCESS)
            return STATUS_UNSUCCESSFUL;
        ndpCount++;
    }
    if (NdpCount != NULL)
    {
        *NdpCount = ndpCount;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MbbNtbDetectNdp16Loop(_In_ PNCM_NTH16 Nth)
{
    PNCM_NDP16 FirstNdp;
    PNCM_NDP16 Ndp;
    PNCM_NDP16 LoopNdp;

    if ((FirstNdp = MBB_NTH16_GET_FIRST_NDP(Nth)) == NULL)
        return STATUS_SUCCESS;

    if (!MBB_NTB16_IS_VALID_NDP_LENGTH(Nth, FirstNdp))
    {
        return STATUS_UNSUCCESSFUL;
    }
    LoopNdp = MBB_NDP16_GET_NEXT_NDP(Nth, FirstNdp);

    for (Ndp = FirstNdp; Ndp != NULL && LoopNdp != NULL; Ndp = MBB_NDP16_GET_NEXT_NDP(Nth, Ndp))
    {
        if (!MBB_NTB16_IS_VALID_NDP_LENGTH(Nth, LoopNdp))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (!MBB_NTB16_IS_VALID_NDP_LENGTH(Nth, Ndp))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (LoopNdp == Ndp)
        {
            return STATUS_UNSUCCESSFUL;
        }

        if ((LoopNdp = MBB_NDP16_GET_NEXT_NDP(Nth, LoopNdp)) != NULL)
        {
            if (!MBB_NTB16_IS_VALID_NDP_LENGTH(Nth, LoopNdp))
            {
                return STATUS_UNSUCCESSFUL;
            }
            LoopNdp = MBB_NDP16_GET_NEXT_NDP(Nth, LoopNdp);
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MbbNtbValidateNdp16(_In_ PNCM_NTH16 Nth, _In_ PNCM_NDP16 Ndp)
{
    ULONG Index;
    ULONG DatagramCount = MBB_NDP16_GET_DATAGRAM_COUNT(Ndp);

    if (!MBB_NTB16_IS_VALID_NDP_SIGNATURE(Ndp))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (MBB_NDP16_GET_SIGNATURE_TYPE(Ndp) == NCM_NDP16_IPS)
    {
        //
        // Check if the session id is valid, else discard
        //

        // Check if session Id is greater than maximum supported. Here
        // we can also check against MaxActivatedContexts, but this would
        // mean we need to take a lock for getting the value of maxactivatedcontexts
        // The lock can be avoided by checking against the maximum number of ports
        // supported by the class driver. SESSION_PORT_TABLE.InUse can be used
        // to check whether the session id is in use.

        if (MBB_NDP16_GET_SESSIONID(Ndp) >= MBB_MAX_NUMBER_OF_SESSIONS)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (!MBB_NTB16_IS_VALID_NDP_LENGTH(Nth, Ndp))
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (Index = 0; Index < DatagramCount; Index++)
    {
        if (MBB_NTB16_IS_END_DATAGRAM(Nth, Ndp, Index))
            return STATUS_SUCCESS;

        if (!MBB_NTB16_IS_VALID_DATAGRAM(Nth, Ndp, Index))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MbbNtbValidateNth16(_In_ PNCM_NTH16 Nth, _In_ ULONG BufferLength, _Out_opt_ ULONG* NdpCount)
{
    PNCM_NDP16 Ndp;
    ULONG ndpCount = 0;
    if (BufferLength < sizeof(NCM_NTH16))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH16_IS_VALID_SIGNATURE(Nth))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH16_IS_VALID_HEADER_LENGTH(Nth))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH16_IS_VALID_BLOCK_LENGTH(Nth, BufferLength))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!MBB_NTH16_IS_VALID_FIRST_NDP(Nth))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (MbbNtbDetectNdp16Loop(Nth) != STATUS_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    for (Ndp = MBB_NTH16_GET_FIRST_NDP(Nth); Ndp != NULL; Ndp = MBB_NDP16_GET_NEXT_NDP(Nth, Ndp))
    {
        if (MbbNtbValidateNdp16(Nth, Ndp) != STATUS_SUCCESS)
            return STATUS_UNSUCCESSFUL;
        ndpCount++;
    }

    if (NdpCount != NULL)
    {
        *NdpCount = ndpCount;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MbbNtbValidate(_In_ PVOID Nth, _In_ ULONG BufferLength, _In_ BOOLEAN Is32Bit, _Out_opt_ ULONG* NdpCount)
{
    if (Is32Bit == TRUE)
        return MbbNtbValidateNth32((PNCM_NTH32)Nth, BufferLength, NdpCount);
    else
        return MbbNtbValidateNth16((PNCM_NTH16)Nth, BufferLength, NdpCount);
}

NTSTATUS
CreateNonPagedWdfMemory(_In_ ULONG ObjectSize, _Out_ WDFMEMORY* WdfMemory, _Out_opt_ PVOID* ObjectMemory, _In_ WDFOBJECT Parent, _In_ ULONG PoolTag = 0)
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objectAttribs;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
    objectAttribs.ParentObject = Parent;

    status = WdfMemoryCreate(&objectAttribs, NonPagedPoolNx, PoolTag, ObjectSize, WdfMemory, ObjectMemory);

    return status;
}

PMDL AllocateNonPagedMdl(_In_reads_bytes_(Length) PVOID VirtualAddress, _In_ ULONG Length)
{
    PMDL Mdl;

    Mdl = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, NULL);

    if (Mdl)
    {
        MmBuildMdlForNonPagedPool(Mdl);
        Mdl->Next = NULL;
    }

    return Mdl;
}

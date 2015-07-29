/******************************Module*Header*******************************\
* Module Name: bdd_util.cxx
*
* Basic Display Driver utility functions
*
* Created: 29-Mar-2011
* Author: Amos Eshel [amosesh]
*
* Copyright (c) 2011 Microsoft Corporation
\**************************************************************************/

#include "BDD.hxx"


#pragma code_seg("PAGE")

//
// EDID validation
//

BOOLEAN IsEdidHeaderValid(_In_reads_bytes_(EDID_V1_BLOCK_SIZE) const BYTE* pEdid)
{
    PAGED_CODE();

    static const UCHAR EDID1Header[8] = {0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0};
    return memcmp(pEdid, EDID1Header, sizeof(EDID1Header)) == 0;
}

BOOLEAN IsEdidChecksumValid(_In_reads_bytes_(EDID_V1_BLOCK_SIZE) const BYTE* pEdid)
{
    PAGED_CODE();

    BYTE CheckSum = 0;
    for (const BYTE* pEdidStart = pEdid; pEdidStart < (pEdid + EDID_V1_BLOCK_SIZE); ++pEdidStart)
    {
        CheckSum += *pEdidStart;
    }

    return CheckSum == 0;
}

//
// Frame buffer map/unmap
//

NTSTATUS
MapFrameBuffer(
    _In_                       PHYSICAL_ADDRESS    PhysicalAddress,
    _In_                       ULONG               Length,
    _Outptr_result_bytebuffer_(Length) VOID**              VirtualAddress)
{
    PAGED_CODE();

    //
    // Check for parameters
    //
    if ((PhysicalAddress.QuadPart == (ULONGLONG)0) ||
        (Length == 0) ||
        (VirtualAddress == NULL))
    {
        BDD_LOG_ERROR3("One of PhysicalAddress.QuadPart (0x%I64x), Length (0x%I64x), VirtualAddress (0x%I64x) is NULL or 0",
                        PhysicalAddress.QuadPart, Length, VirtualAddress);
        return STATUS_INVALID_PARAMETER;
    }

    *VirtualAddress = MmMapIoSpaceEx(PhysicalAddress,
                                   Length,
                                   PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (*VirtualAddress == NULL)
    {
        // The underlying call to MmMapIoSpace failed. This may be because, MmWriteCombined
        // isn't supported, so try again with MmNonCached

        *VirtualAddress = MmMapIoSpaceEx(PhysicalAddress,
                                       Length,
                                       PAGE_READWRITE | PAGE_NOCACHE);
        if (*VirtualAddress == NULL)
        {
            BDD_LOG_LOW_RESOURCE1("MmMapIoSpace returned a NULL buffer when trying to allocate 0x%I64x bytes", Length);
            return STATUS_NO_MEMORY;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
UnmapFrameBuffer(
    _In_reads_bytes_(Length) VOID* VirtualAddress,
    _In_                ULONG Length)
{
    PAGED_CODE();


    //
    // Check for parameters
    //
    if ((VirtualAddress == NULL) && (Length == 0))
    {
        // Allow this function to be called when there's no work to do, and treat as successful
        return STATUS_SUCCESS;
    }
    else if ((VirtualAddress == NULL) || (Length == 0))
    {
        BDD_LOG_ERROR2("Only one of Length (0x%I64x), VirtualAddress (0x%I64x) is NULL or 0",
                        Length, VirtualAddress);
        return STATUS_INVALID_PARAMETER;
    }

    MmUnmapIoSpace(VirtualAddress,
                   Length);

    return STATUS_SUCCESS;
}


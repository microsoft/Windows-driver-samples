// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once
#include "precomp.h"

#include "winerror.h"

#ifdef _KERNEL_MODE
#include "ntintsafe.h"
#else
#include "intsafe.h"
#endif

namespace Wifi 
{
    _inline
    NTSTATUS ConvertHRESULTToNTSTATUS(HRESULT hr) {
        if ((hr & FACILITY_NT_BIT) == FACILITY_NT_BIT) {
            return static_cast<NTSTATUS>(hr & ~FACILITY_NT_BIT); // Strip NT bit safely
        }

        // Trace the HRESULT value for diagnostics
        return SUCCEEDED(hr) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    __inline
    NTSTATUS ConvertNDISSTATUSToNTSTATUS(NDIS_STATUS NdisStatus)
    {
        if (NT_SUCCESS(NdisStatus) && NdisStatus != NDIS_STATUS_SUCCESS && NdisStatus != NDIS_STATUS_PENDING &&
            NdisStatus != NDIS_STATUS_INDICATION_REQUIRED)
        {
            // Case where an NDIS error is incorrectly mapped as a success by NT_SUCCESS macro
            return STATUS_UNSUCCESSFUL;
        }
        else
        {
            switch (NdisStatus)
            {
            case NDIS_STATUS_BUFFER_TOO_SHORT:
                return STATUS_BUFFER_TOO_SMALL;
                break;
            default:
                return (NTSTATUS)NdisStatus;
                break;
            }
        }
    }

    __inline
    NDIS_STATUS ConvertNTSTATUSToNDISSTATUS(NTSTATUS NtStatus)
    {
        if (NT_SUCCESS(NtStatus) && NtStatus != STATUS_PENDING && NtStatus != STATUS_NDIS_INDICATION_REQUIRED)
        {
            return NDIS_STATUS_SUCCESS;
        }
        else
        {
            switch (NtStatus)
            {
            case STATUS_BUFFER_TOO_SMALL:
                return NDIS_STATUS_BUFFER_TOO_SHORT;
                break;
            default:
                return (NDIS_STATUS)NtStatus;
                break;
            }
        }
    }

    _Must_inspect_result_
    _inline
    NTSTATUS SizeTAddSafe(_In_ size_t Augend, _In_ size_t Addend, _Out_ _Deref_out_range_(== , Augend + Addend) size_t* pResult) 
    {
#ifdef _KERNEL_MODE
        return RtlSizeTAdd(Augend, Addend, pResult);
#else
        return ConvertHRESULTToNTSTATUS(SizeTAdd(Augend, Addend, pResult));
#endif
    }

    _Must_inspect_result_
    __inline
    NTSTATUS ULongPtrAddSafe(
        _In_ ULONGLONG ullAugend,
        _In_ ULONGLONG ullAddend,
        _Out_ _Deref_out_range_(== , ullAugend + ullAddend) ULONGLONG* pullResult)
    {
#ifdef _KERNEL_MODE
        return RtlULongPtrAdd(ullAugend, ullAddend, pullResult);
#else
        return ConvertHRESULTToNTSTATUS(ULongPtrAdd(ullAugend, ullAddend, pullResult));
#endif
    }

    _Must_inspect_result_
        __inline
        NTSTATUS ULongPtrSubSafe(
            _In_ ULONGLONG ullMinuend,
            _In_ ULONGLONG ullSubtrahend,
            _Out_ _Deref_out_range_(== , ullMinuend - ullSubtrahend) ULONGLONG* pullResult)
    {
#ifdef _KERNEL_MODE
        return RtlULongPtrSub(ullMinuend, ullSubtrahend, pullResult);
#else
        return ConvertHRESULTToNTSTATUS(ULongPtrSub(ullMinuend, ullSubtrahend, pullResult));
#endif
    }
}

#ifndef _KERNEL_MODE
// TODO: Cleanup when EWDK contains the payload of https://microsoft.visualstudio.com/OS/_workitems/edit/58447986
typedef enum _NDIS_FRAME_HEADER
{
    NdisFrameHeaderUndefined,
    NdisFrameHeaderMac,
    NdisFrameHeaderArp,
    NdisFrameHeaderIPv4,
    NdisFrameHeaderIPv6,
    NdisFrameHeaderUdp,
    NdisFrameHeaderMaximum
}NDIS_FRAME_HEADER, * PNDIS_FRAME_HEADER;

typedef enum _NDIS_RECEIVE_FILTER_TEST
{
    NdisReceiveFilterTestUndefined,
    NdisReceiveFilterTestEqual,
    NdisReceiveFilterTestMaskEqual,
    NdisReceiveFilterTestNotEqual,
    NdisReceiveFilterTestMaximum
}NDIS_RECEIVE_FILTER_TEST, * PNDIS_RECEIVE_FILTER_TEST;
// End of TODO
#endif // !_KERNEL_MODE
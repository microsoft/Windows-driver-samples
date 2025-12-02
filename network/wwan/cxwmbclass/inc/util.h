//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once

#define MbbPoolTagDefault '0CBM'
#define MbbPoolTagNtbSend '1CBM'
#define MbbPoolTagNblSend '2CBM'
#define MbbPoolTagNbSend '3CBM'
#define MbbPoolTagMdlReceive '6CBM'

#define ALLOCATE_PAGED_POOL(_y) ExAllocatePool2(POOL_FLAG_PAGED, _y, MbbPoolTagDefault)
#define ALLOCATE_NONPAGED_POOL(_y) ExAllocatePool2(POOL_FLAG_NON_PAGED, _y, MbbPoolTagDefault)
#define ALLOCATE_NONPAGED_POOL_WITH_TAG(_x, _y) ExAllocatePool2(POOL_FLAG_NON_PAGED, _x, _y)

#define FREE_POOL(_x) \
    { \
        ExFreePool(_x); \
        _x = NULL; \
    };

#define MIN(_X_, _Y_) (((_X_) < (_Y_)) ? (_X_) : (_Y_))
#define MAX(_X_, _Y_) (((_X_) > (_Y_)) ? (_X_) : (_Y_))

#define ALIGN_FLOOR(_VALUE_, _ALIGN_) ALIGN_DOWN_BY(_VALUE_, _ALIGN_)
#define ALIGN_CIELING(_VALUE_, _ALIGN_) ALIGN_UP_BY(_VALUE_, _ALIGN_)
#define ALIGN(_VALUE_, _ALIGN_) ALIGN_CIELING(_VALUE_, _ALIGN_)
#define ALIGN_AT_OFFSET(_VALUE_, _ALIGN_, _OFFSET_) \
    (((SIZE_T)(_VALUE_) <= (ALIGN_FLOOR(_VALUE_, _ALIGN_) + (_OFFSET_))) ? (ALIGN_FLOOR(_VALUE_, _ALIGN_) + (_OFFSET_)) \
                                                                         : (ALIGN(_VALUE_, _ALIGN_) + (_OFFSET_)))

NTSTATUS
MbbNtbValidate(_In_ PVOID Nth, _In_ ULONG BufferLength, _In_ BOOLEAN Is32Bit, _Out_opt_ ULONG* NdpCount);

NTSTATUS
CreateNonPagedWdfMemory(_In_ ULONG ObjectSize, _Out_ WDFMEMORY* WdfMemory, _Out_opt_ PVOID* ObjectMemory, _In_ WDFOBJECT Parent, _In_ ULONG PoolTag);

PMDL AllocateNonPagedMdl(_In_reads_bytes_(Length) PVOID VirtualAddress, _In_ ULONG Length);

VOID MbbRecvCancelNdps(_In_ PWMBCLASS_DEVICE_CONTEXT DeviceContext, _In_ ULONG SessionId);

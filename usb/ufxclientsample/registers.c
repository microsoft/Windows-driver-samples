/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    registers.c

Abstract:

    Memory mapping the controller's registers

Environment:

    Kernel mode

--*/

#include "device.h"
#include "registers.h"
#include "registers.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, RegistersCreate)
#endif


_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RegistersCreate(
    _In_ WDFDEVICE Device,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR  RegistersResource
    )
/*++

Routine Description:

    Helper function to map the memory resources to the HW registers.

Arguments:

    Device - Wdf device object corresponding to the FDO

    RegisterResource -  Raw resource for the memory

Return Value:

    Appropriate NTSTATUS value

--*/
{
    NTSTATUS Status;
    PREGISTERS_CONTEXT Context;
    WDF_OBJECT_ATTRIBUTES Attributes;

    TraceEntry();

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, REGISTERS_CONTEXT);

    Status = WdfObjectAllocateContext(Device, &Attributes, &Context);
    if (Status == STATUS_OBJECT_NAME_EXISTS) {
        //
        // In the case of a resource rebalance, the context allocated
        // previously still exists.
        //
        Status = STATUS_SUCCESS;
        RtlZeroMemory(Context, sizeof(*Context));
    }
    CHK_NT_MSG(Status, "Failed to allocate context for registers");

    Context->RegisterBase = MmMapIoSpaceEx(
                                RegistersResource->u.Memory.Start,
                                RegistersResource->u.Memory.Length,
                                PAGE_NOCACHE | PAGE_READWRITE);

    if (Context->RegisterBase == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        CHK_NT_MSG(Status, "MmMapIoSpaceEx failed");
    }

    Context->RegistersLength = RegistersResource->u.Memory.Length;

End:

    TraceExit();
    return Status;
}
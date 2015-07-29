/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    registers.h

Abstract:

Environment:

    Kernel mode

--*/
#pragma once

#include <pshpack4.h>
#pragma warning(push)
//
// #### TODO: Add controller data structure definitions here ####
//
#pragma warning(pop)
#include <poppack.h>


#define USB_LINK_STATE_U0 0
#define USB_LINK_STATE_U1 1
#define USB_LINK_STATE_U2 2
#define USB_LINK_STATE_U3 3
#define USB_LINK_STATE_SS_DISABLED 4
#define USB_LINK_STATE_RX_DETECT 5
#define USB_LINK_STATE_SS_INACTIVE 6
#define USB_LINK_STATE_POLLING 7
#define USB_LINK_STATE_RECOVERY 8
#define USB_LINK_STATE_HOT_RESET 9
#define USB_LINK_STATE_COMPLIANCE_MODE 10
#define USB_LINK_STATE_LOOPBACK 11
#define USB_LINK_STATE_RESUME_RESET 15


typedef struct _REGISTERS_CONTEXT {

    PUCHAR RegisterBase;

    SIZE_T RegistersLength;

} REGISTERS_CONTEXT, *PREGISTERS_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REGISTERS_CONTEXT, DeviceGetRegistersContext)

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RegistersCreate(
    _In_ WDFDEVICE Device,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR  RegistersResource
    );
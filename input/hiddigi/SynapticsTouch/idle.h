/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Sample code. Dealpoint ID #843729.

    Module Name:

        idle.c

    Abstract:

        This file contains the declarations for Power Idle specific callbacks
		and function declarations

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once


//
// Power Idle Workitem context
// 
typedef struct _IDLE_WORKITEM_CONTEXT
{    
    // Handle to a WDF device object
    WDFDEVICE FxDevice;

    // Handle to a WDF request object
    WDFREQUEST FxRequest;

} IDLE_WORKITEM_CONTEXT, *PIDLE_WORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(IDLE_WORKITEM_CONTEXT, GetWorkItemContext)

NTSTATUS
TchProcessIdleRequest(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request,
    OUT BOOLEAN *Pending
    );

VOID
TchCompleteIdleIrp(
    IN PDEVICE_EXTENSION FxDeviceContext
    );

EVT_WDF_WORKITEM TchIdleIrpWorkitem;



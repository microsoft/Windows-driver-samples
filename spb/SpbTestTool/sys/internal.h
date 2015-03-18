/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    internal.h

Abstract:

    This module contains the common internal type and function
    definitions for the SPB peripheral driver.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4480)

#define SPBT_POOL_TAG ((ULONG) 'TBPS')

/////////////////////////////////////////////////
//
// Common includes.
//
/////////////////////////////////////////////////

#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>
#include <ntstrsafe.h>

#include "spb.h"
#include "spbtestioctl.h"

#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"

#include "trace.h"

//
// Forward Declarations
//

typedef struct _DEVICE_CONTEXT  DEVICE_CONTEXT,  *PDEVICE_CONTEXT;
typedef struct _REQUEST_CONTEXT  REQUEST_CONTEXT,  *PREQUEST_CONTEXT;

struct _DEVICE_CONTEXT 
{
    //
    // Handle back to the WDFDEVICE
    //

    WDFDEVICE FxDevice;

    //
    // Handle to the sequential SPB queue
    //

    WDFQUEUE SpbQueue;

    //
    // Connection ID for SPB peripheral
    //

    LARGE_INTEGER PeripheralId;
    
    //
    // Interrupt object and wait event
    //

    WDFINTERRUPT Interrupt;
    KEVENT IsrWaitEvent;

    //
    // Setting indicating whether the interrupt should be connected
    //

    BOOLEAN ConnectInterrupt;

    //
    // SPB controller target
    //

    WDFIOTARGET SpbController;

    //
    // SPB request object
    //

    WDFREQUEST SpbRequest;

    //
    // Input memory for request. Valid while request in progress.
    //

    WDFMEMORY InputMemory;

    //
    // Client request object
    //

    WDFREQUEST ClientRequest;

    //
    // WaitOnInterrupt request object
    //

    WDFREQUEST WaitOnInterruptRequest;
};

struct _REQUEST_CONTEXT
{    
    //
    // Associated framework device object
    //

    WDFDEVICE FxDevice;

    //
    // Variables to track write length for a sequence request.
    // There are needed to complete the client request with
    // correct bytesReturned value.
    //

    BOOLEAN IsSpbSequenceRequest;
    ULONG_PTR SequenceWriteLength;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext);

#pragma warning(pop)

#endif // _INTERNAL_H_

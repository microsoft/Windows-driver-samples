/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    driver.h

Abstract:

    This is a C version of a very simple sample driver that illustrates
    how to use the driver framework and demonstrates best practices.

--*/

#define INITGUID

#include <ntddk.h>
#include <wdf.h>

#include "device.h"
#include "queue.h"

typedef struct _REQUEST_CONTEXT {
    //
    // Count to use when trying to claim completion ownership of a cancelable
    // request when clearing the cancel routine.  If the caller can clear the
    // cancel routine successfully, the caller is *NOT* responsible for decrementing
    // the count if the request is going to be completed immediately (and a
    // cancel routine is not going to be set in the future).
    //
    LONG CancelCompletionOwnershipCount;

} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, RequestGetContext);

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EchoEvtDeviceAdd;

NTSTATUS
EchoPrintDriverVersion(
    );


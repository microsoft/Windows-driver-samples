/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    filter.h

Abstract:

    Contains structure definitions and function prototypes for a generic filter driver.	

Environment:

    User mode

--*/

#include <windows.h>
#include <winioctl.h>
#pragma warning( disable: 4201 )    // nonstandard extension used : nameless struct/union
#include <ntstatus.h>
#include <devpropdef.h>
#include <wudfwdm.h>
#include <wdf.h>

#if !defined(_FILTER_H_)
#define _FILTER_H_


#define DRIVERNAME "Generic.sys: "

//
// Change the following define to 1 if you want to forward
// the request with a completion routine.
//
#define FORWARD_REQUEST_WITH_COMPLETION 0


typedef struct _FILTER_EXTENSION
{
    WDFDEVICE WdfDevice;
    // More context data here

}FILTER_EXTENSION, *PFILTER_EXTENSION;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER_EXTENSION,
                                        FilterGetData)

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD FilterEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL FilterEvtIoDeviceControl;

VOID
FilterForwardRequest(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target
    );

#if FORWARD_REQUEST_WITH_COMPLETION

VOID
FilterForwardRequestWithCompletionRoutine(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target
    );

VOID
FilterRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
   );

#endif //FORWARD_REQUEST_WITH_COMPLETION

#endif


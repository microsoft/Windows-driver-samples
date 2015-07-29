/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    eventbuffer.h

Abstract:

    Defines the sample controller's events

Environment:

    Kernel mode

--*/
#pragma once

typedef enum _CONTROLLER_EVENT_TYPE {
    EventTypeDevice = 0,
    EventTypeEndpoint = 1
} CONTROLLER_EVENT_TYPE;

typedef enum _DEVICE_EVENT {
    DeviceEventDisconnect = 0,
    DeviceEventUSBReset = 1,
    DeviceEventConnect = 2,
    DeviceEventUSBLinkStateChange = 3,
    DeviceEventWakeUp = 4,
    DeviceEventHibernationRequest = 5,
    DeviceEventSuspend = 6,
    DeviceEventCommandComplete = 7
} DEVICE_EVENT, *PDEVICE_EVENT;

typedef enum _ENDPOINT_EVENT {
    EndpointEventTransferComplete = 0,
    EndpointEventStartTransferComplete = 1,
    EndpointEventEndTransferComplete = 2,
    EndpointEventSetStall = 3,
    EndpointEventClearStall = 4
} ENDPOINT_EVENT, *PENDPOINT_EVENT;

typedef struct _CONTROLLER_EVENT {
    CONTROLLER_EVENT_TYPE Type;
    
    union {
        DEVICE_EVENT DeviceEvent;
        ENDPOINT_EVENT EndpointEvent;
    } u;
} CONTROLLER_EVENT;

_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleEndpointEvent (
    WDFDEVICE WdfDevice,
    ENDPOINT_EVENT EndpointEvent
    );

_IRQL_requires_(DISPATCH_LEVEL)
VOID
HandleDeviceEvent (
    WDFDEVICE WdfDevice,
    DEVICE_EVENT DeviceEvent
    );

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_Events.h
//
//   Abstract:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRAMEWORK_EVENTS_H
#define FRAMEWORK_EVENTS_H

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_DRIVER_UNLOAD)
VOID EventDriverUnload(_In_ WDFDRIVER wdfDriver);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
VOID EventCleanupDriverObject(_In_ WDFOBJECT driverObject);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
VOID EventCleanupDeviceObject(_In_ WDFOBJECT deviceObject);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL)
VOID EventIODeviceControl(_In_ WDFQUEUE wdfQueue,
                          _In_ WDFREQUEST wdfRequest,
                          _In_ size_t outputBufferLength,
                          _In_ size_t inputBufferLength,
                          _In_ ULONG ioControlCode);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_IO_QUEUE_IO_READ)
VOID EventIORead(_In_ WDFQUEUE wdfQueue,
                 _In_ WDFREQUEST wdfRequest,
                 _In_ size_t length);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Function_class_(EVT_WDF_IO_QUEUE_IO_WRITE)
VOID EventIOWrite(_In_ WDFQUEUE wdfQueue,
                  _In_ WDFREQUEST wdfRequest,
                  _In_ size_t length);

#endif /// FRAMEWORK_EVENTS_H
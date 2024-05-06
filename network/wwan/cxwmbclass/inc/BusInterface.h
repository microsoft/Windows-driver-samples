/***************************************************************************

Copyright (c) 2010 Microsoft Corporation

Module Name:

    BusInterface.h

Abstract:

    This module defines the interface between the upper NDIS layer and lower
    bus layer of the MBB (Mobile BroadBand) Class driver.

Environment:

    kernel mode only

Notes:

    The upper ndis layer may only communicate with the lower bus layer through
    these interfaces.

Revision History:

    2/7/2010 : created

Authors:

    TriRoy

****************************************************************************/

#ifndef _BusInterface_H_
#define _BusInterface_H_

////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#if 0
#include "MbbDebug.h"
#endif
////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////

#define MAX_PARAMETER_STRING (128)

#define ALT_DATA_SETTING_0 (0)
#define ALT_DATA_SETTING_1 (1)

#define MAX_PENDING_SENDS (3)

////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////
typedef PVOID MBB_BUS_HANDLE;
typedef PVOID MBB_PROTOCOL_HANDLE;
typedef PVOID MBB_REQUEST_HANDLE;
typedef PVOID MBB_RECEIVE_CONTEXT;

typedef struct _MBB_BUS_PARAMETERS
{
    ULONG FragmentSize;

    ULONG MaxSegmentSize;

    BOOLEAN ChainedMdlsSupported;
    BOOLEAN Ntb32BitSupported;
    BOOLEAN CurrentMode32Bit;
    BOOLEAN SelectiveSuspendSupported;

    ULONG MaxOutNtb;
    USHORT MaxOutDatagrams;
    USHORT NdpOutDivisor;
    USHORT NdpOutRemainder;
    USHORT NdpOutAlignment;

    BYTE PowerFiltersSupported;
    BYTE MaxPowerFilterSize;

    WCHAR Manufacturer[MAX_PARAMETER_STRING];
    WCHAR Model[MAX_PARAMETER_STRING];

    BYTE MaxOutstandingCommandMessages;
    USHORT MTU;
    BOOLEAN IsErrataDevice;
    USHORT MbimVersion;
    BOOLEAN RemoteWakeCapable;
    USHORT MbimExtendedVersion;

} MBB_BUS_PARAMETERS, *PMBB_BUS_PARAMETERS;

typedef struct _MBB_CONNECTION_STATE
{

    BOOLEAN ConnectionUp;
    ULONGLONG UpStreamBitRate;
    ULONGLONG DownStreamBitRate;

} MBB_CONNECTION_STATE, *PMBB_CONNECTION_STATE;

typedef __callback VOID (*MBB_BUS_SEND_COMPLETION_CALLBACK)(__in MBB_PROTOCOL_HANDLE ProtocolHandle, __in MBB_REQUEST_HANDLE RequestHandle, __in NTSTATUS Status);

typedef __callback VOID (*MBB_BUS_RECEIVE_COMPLETION_CALLBACK)(
    __in MBB_PROTOCOL_HANDLE ProtocolHandle, __in MBB_REQUEST_HANDLE RequestHandle, __in NTSTATUS Status, __in ULONG_PTR ReceivedLength);

typedef __callback VOID (*MBB_BUS_RESPONSE_AVAILABLE_CALLBACK)(__in MBB_PROTOCOL_HANDLE ProtocolHandle);

typedef __callback VOID (*MBB_BUS_SEND_DATA_COMPLETION_CALLBACK)(
    __in MBB_PROTOCOL_HANDLE ProtocolHandle, __in MBB_REQUEST_HANDLE RequestHandle, __in NTSTATUS Status, __in PMDL Mdl);

typedef __callback VOID (*MBB_BUS_DATA_RECEIVE_CALLBACK)(__in MBB_PROTOCOL_HANDLE ProtocolHandle, __in MBB_RECEIVE_CONTEXT ReceiveContext, __in PMDL Mdl);

typedef __callback VOID (*MBB_BUS_SS_IDLE_CONFIRM_CALLBACK)(__in MBB_PROTOCOL_HANDLE ProtocolHandle, __in DEVICE_POWER_STATE PowerState);

typedef __callback VOID (*MBB_BUS_SS_IDLE_NOTIFICATION_COMPLETE_CALLBACK)(__in MBB_PROTOCOL_HANDLE ProtocolHandle, __in NTSTATUS Status);

////////////////////////////////////////////////////////////////////////////////
//
//  INTERFACE
//
////////////////////////////////////////////////////////////////////////////////
EXTERN_C
NTSTATUS
MbbBusInitialize(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PDEVICE_OBJECT NextDeviceObject,
    _In_ MBB_BUS_RESPONSE_AVAILABLE_CALLBACK ResponseAvailableCallback,
    _In_ MBB_BUS_DATA_RECEIVE_CALLBACK ReceiveDataCallback,
    _In_ MBB_BUS_SS_IDLE_CONFIRM_CALLBACK IdleConfirmCallback,
    _In_ MBB_BUS_SS_IDLE_NOTIFICATION_COMPLETE_CALLBACK IdleNotificationComplete,
    _In_ MBB_PROTOCOL_HANDLE ProtocolHandle,
    _Outptr_ MBB_BUS_HANDLE* BusHandle);
/*
    Description
        This routine initializes the bus layer i.e. the lower layer in the
        MBB Class Driver. All bus layer implementations need to have this
        function. The Protocol Layer i.e. the upper layer in the MBB Class
        Driver will call this function before calling any other function in
        the Bus Layer.

    Parameters
        _In_     PDEVICE_OBJECT      DeviceObject,
            The WDM device representation of the device.

        _In_     MBB_BUS_NOTIFICATION_CALLBACK NotificationCallback,
            The routine the bus layer calls when it needs to notify
            the upper protocol layer.

        _In_     MBB_PROTOCOL_HANDLE ProtocolHandle,
            The handle the bus layer passes back to the protocol layer
            when it calls any callback. The protocol layer uses this handle
            to identify the instance of the MBB device the callback is meant for.

        _Outptr_ MBB_BUS_HANDLE      BusHandle
            The handle returned by the bus layer on successful initialization.
            The protocol layer passes this handle to the bus layer on subsequent
            calls to the bus layer. The bus layer uses this handle to indetify
            the instance of the MBB device.

    Return Value
        NTSTATUS_SUCCESS
            Initialization was successful.

        Other failure code
*/

EXTERN_C
NTSTATUS
MbbBusInitializeByWdf(
    _In_ WDFDEVICE WdfDevice,
    _In_ MBB_BUS_RESPONSE_AVAILABLE_CALLBACK ResponseAvailableCallback,
    _In_ MBB_BUS_DATA_RECEIVE_CALLBACK ReceiveDataCallback,
    _In_ MBB_BUS_SS_IDLE_CONFIRM_CALLBACK IdleConfirmCallback,
    _In_ MBB_BUS_SS_IDLE_NOTIFICATION_COMPLETE_CALLBACK IdleNotificationComplete,
    _In_ MBB_PROTOCOL_HANDLE ProtocolHandle,
    _Outptr_ MBB_BUS_HANDLE* BusHandle,
    _Inout_opt_ MBB_BUS_HANDLE preAllocatedBusObject);

EXTERN_C
VOID MbbBusCleanup(__in MBB_BUS_HANDLE BusHandle);
/*
    Description
        This will be last call from Protocol layer in to Bus Layer
        to cleanup the bus layer. The bus layer should free all
        resources. The BusHandle will not be used for subsequent calls.
        If there are pending requests the Bus Layer should not return
        from this call unless all operations are complete.

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle
            BusHandle identifies the instance of the bus layer.

    Return Value
        None
*/

EXTERN_C
NTSTATUS
MbbBusQueryBusParameters(__in MBB_BUS_HANDLE BusHandle, __out PMBB_BUS_PARAMETERS BusParameters);
/*
    Description
        This routine queries the bus specific parameters like transfer size,
        DMA support etc... This is called when the bus is initialized to format
        requests correctly before sending to the bus.

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.

        __out   PMBB_BUS_PARAMETERS BusParameters
            The buffer where the bus layer returns the information to the caller.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/
EXTERN_C
NTSTATUS
MbbBusSendMessageFragment(
    __in MBB_BUS_HANDLE BusHandle,
    __in MBB_REQUEST_HANDLE RequestHandle,
    __in PVOID MessageFragment,
    __in ULONG FragmentLength,
    __in LPGUID ActivityId,
    __in MBB_BUS_SEND_COMPLETION_CALLBACK SendCompletionCallback);
/*
    Description
        The protocol layer call this routine to request the bus layer to
        send a message fragment. Fragmentation / Reassembly is handled by
        the protocol layer and it will only handle fragments that are within
        the maximum transfer size of the bus.

        This routine is asynchronous and returns immediately after queueing
        the transfer. The caller is notified of the completion through the
        callback.

    Parameters
        __in MBB_BUS_HANDLE BusHandle,
            BusHandle identifies the instance of the bus layer.

        __in MBB_REQUEST_HANDLE RequestHandle,
            Identifies the request.

        __in PVOID MessageFragment,
            The data payload that needs to be sent.

        __in ULONG FragmentLength,
            Length of the data payload. This will not be greater than the
            maximum transfer size supported by the bus.

        __in MBB_BUS_SEND_COMPLETION_CALLBACK SendCompletionCallback
            The completion callback routine that will be called by the bus
            when the transfer is complete.

    Return Value

        NTSTATUS_SUCCESS
            The transfer has completed successfully. SendCompletionCallback will NOT be called.

        NTSTATUS_PENDING
            The transfer was queued. SendCompletionCallback will be called on completion.

        Other failure code
            The transfer could not be queued. SendCompletionCallback will NOT be called.
*/

EXTERN_C
NTSTATUS
MbbBusReceiveMessageFragment(
    _In_ MBB_BUS_HANDLE BusHandle,
    _In_ MBB_REQUEST_HANDLE RequestHandle,
    _In_ __drv_aliasesMem PVOID MessageFragment,
    _In_ ULONG FragmentLength,
    _In_ LPGUID ActivityId,
    _In_ MBB_BUS_RECEIVE_COMPLETION_CALLBACK ReceiveCompletionCallback);
/*
    Description

    Parameters


    Return Value

        NTSTATUS_SUCCESS
            Initialization was successful.

        Other failure code
*/
/*
    Description
        The protocol layer call this routine to request the bus layer to
        receive data from the device. Reassembly is handled by the protocol layer.

        This routine is asynchronous and returns immediately after queueing
        the transfer. The caller is notified of the completion through the
        callback.

    Parameters
        _In_ MBB_BUS_HANDLE BusHandle,
            BusHandle identifies the instance of the bus layer.

        _In_ MBB_REQUEST_HANDLE RequestHandle,
            Identifies the request.

        _InOut_ PVOID MessageFragment,
            The data buffer that would be filled with the received data.

        _In_ ULONG FragmentLength,
            Length of the data requested from the device. This will not be
            greater than the maximum transfer size supported by the bus.

        _In_ MBB_BUS_RECEIVE_COMPLETION_CALLBACK ReceiveCompletionCallback
            The completion callback routine that will be called by the bus
            when the transfer is complete.

    Return Value

        NTSTATUS_SUCCESS
            The transfer has completed successfully. ReceiveCompletionCallback will NOT be called.

        NTSTATUS_PENDING
            The transfer was queued. ReceiveCompletionCallback will be called on completion.

        Other failure code
            The transfer could not be queued. ReceiveCompletionCallback will NOT be called.
*/

EXTERN_C
NTSTATUS
MbbBusSetPacketFilter(__in MBB_BUS_HANDLE BusHandle, __in ULONG PacketFilter);
/*
    Description
        Sets the packet filter on the device.
        For MBB only directed is supported

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.

        __in ULONG               PacketFilter
            the filter to apply


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

EXTERN_C
NTSTATUS
MbbBusGetStat(__in MBB_BUS_HANDLE BusHandle, __in USHORT StatIndex, __out ULONGLONG* Value);
/*
    Description
        Retrieves a status from the device

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.

        __in ULONG               PacketFilter
            the filter to apply


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

EXTERN_C
NTSTATUS
MbbBusStart(_In_ MBB_BUS_HANDLE BusHandle);

EXTERN_C
NTSTATUS
MbbBusStop(__in MBB_BUS_HANDLE BusHandle);

EXTERN_C
BOOLEAN
MbbBusIsStoped(_In_ MBB_BUS_HANDLE BusHandle);

EXTERN_C
NTSTATUS
MbbBusOpen(_In_ MBB_BUS_HANDLE BusHandle, _In_ ULONG TransactionId, _In_opt_ PVOID FastIOSendNetBufferListsComplete, _In_opt_ PVOID FastIOIndicateReceiveNetBufferLists);
/*
    Description

        Opens the session with the device

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

EXTERN_C
NTSTATUS
MbbBusClose(__in MBB_BUS_HANDLE BusHandle, __in ULONG TransactionId, __in BOOLEAN ForceClose);
/*
    Description

        Opens the session with the device

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

EXTERN_C
NTSTATUS
MbbBusStartDataPipes(__in MBB_BUS_HANDLE BusHandle);
/*
    Description

        Selects alt data interface 1 and starts the IoTargets

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

EXTERN_C
VOID MbbBusStopDataPipes(__in MBB_BUS_HANDLE BusHandle);
/*
    Description

        Resets the IoTargets canceling all and waiting for io to compelte
        Selects alt data interface 0 disabling the pipes

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

EXTERN_C
void MbbBusResetDataPipes(_In_ MBB_BUS_HANDLE BusHandle);

EXTERN_C
NTSTATUS
MbbBusWriteData(__in MBB_BUS_HANDLE BusHandle, __in MBB_REQUEST_HANDLE RequestHandle, __in PMDL Mdl, __in MBB_BUS_SEND_DATA_COMPLETION_CALLBACK Callback);
/*
    Description

        Writes data to device bulk out pipe

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.

        __in MBB_REQUEST_HANDLE RequestHandle,
            Identifies the request.

        __in    PMDL                Mdl
            pointer to an MDL identifying the data to send. The bus layer will own this until the completion callback is called.

        __in    MBB_BUS_SEND_DATA_COMPLETION_CALLBACK   Callback
            Callback that will be called when write is complete.

    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/
EXTERN_C
VOID MbbBusReturnReceiveBuffer(__in MBB_BUS_HANDLE BusHandle, __in MBB_RECEIVE_CONTEXT ReceiveContext, __in PMDL Mdl);

EXTERN_C
NTSTATUS
MbbBusSelectDataAltSetting(__in MBB_BUS_HANDLE BusHandle, __in UCHAR AltSetting);

EXTERN_C
NTSTATUS
MbbBusIdleNotification(__in MBB_BUS_HANDLE BusHandle, __in BOOLEAN ForceIdle);

EXTERN_C
VOID MbbBusCancelIdleNotification(__in MBB_BUS_HANDLE BusHandle);

EXTERN_C
NTSTATUS
MbbBusSetPowerFilterPattern(
    __in MBB_BUS_HANDLE BusHandle,
    __in ULONG PatternId,
    __in_bcount_opt(MaskSize) PCUCHAR Mask,
    __in ULONG MaskSize,
    __in_bcount_opt(PatternSize) PCUCHAR Pattern,
    __in ULONG PatternSize);

EXTERN_C
NTSTATUS
MbbBusResetBulkPipe(__in MBB_BUS_HANDLE BusHandle, __in BOOLEAN Out);

EXTERN_C
VOID MbbBusSetNotificationState(__in MBB_BUS_HANDLE BusHandle, __in BOOLEAN Enabled);

EXTERN_C
BOOLEAN
MbbBusIsUde(_In_ MBB_BUS_HANDLE BusHandle);

#endif

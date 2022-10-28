//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include <precomp.h>

VOID MbbBusSendNetBufferLists(_In_ MBB_BUS_HANDLE BusHandle, _In_ PNET_BUFFER_LIST NetBufferList, _In_ ULONG SessionId, _In_ ULONG SendFlags)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    BusObject->SendNetBufferListsHandler(BusObject->ModemContext, NetBufferList, SessionId, SendFlags);
}

VOID MbbBusReturnNetBufferLists(_In_ MBB_BUS_HANDLE BusHandle, _In_ PNET_BUFFER_LIST NetBufferList, _In_ ULONG ReturnFlags)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    BusObject->ReturnNetBufferListsHandler(BusObject->ModemContext, NetBufferList, ReturnFlags);
}

VOID MbbBusCancelSendHandler(_In_ MBB_BUS_HANDLE BusHandle, _In_ PVOID CancelId)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    BusObject->CancelSendHandler(BusObject->ModemContext, CancelId);
}

VOID MbbBusHalt(_In_ MBB_BUS_HANDLE BusHandle, _In_ NDIS_HALT_ACTION HaltAction)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    if (BusObject->State == BUS_STATE_OPENED)
    {
        BusObject->HaltHandler(BusObject->ModemContext, HaltAction);
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
MbbBusPause(_In_ MBB_BUS_HANDLE BusHandle)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    if (BusObject->State == BUS_STATE_OPENED)
    {
        Status = BusObject->PauseHandler(BusObject->ModemContext);
    }
    return Status;
}

_When_(ShutdownAction == NdisShutdownPowerOff, _IRQL_requires_(PASSIVE_LEVEL))
    _When_(ShutdownAction == NdisShutdownBugCheck, _IRQL_requires_(HIGH_LEVEL)) VOID
    MbbBusShutdown(_In_ MBB_BUS_HANDLE BusHandle, _In_ NDIS_SHUTDOWN_ACTION ShutdownAction)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    if (BusObject->ShutdownHandler && BusObject->State == BUS_STATE_OPENED)
    {
        BusObject->ShutdownHandler(BusObject->ModemContext, ShutdownAction);
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL) NDIS_STATUS MbbBusReset(_In_ MBB_BUS_HANDLE BusHandle)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    if (BusObject->ResetHandler && BusObject->State == BUS_STATE_OPENED)
    {
        Status = BusObject->ResetHandler(BusObject->ModemContext);
    }
    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL) NDIS_STATUS MbbBusRestart(_In_ MBB_BUS_HANDLE BusHandle)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    if (BusObject->State == BUS_STATE_OPENED)
    {
        Status = BusObject->RestartHandler(BusObject->ModemContext);
    }
    return Status;
}

BOOLEAN
MbbBusIsFastIO(_In_ MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    return (BusObject->UsbCapDeviceInfo.DeviceInfoHeader.DeviceType == USB_CAP_DEVICE_TYPE_UDE_MBIM_FASTIO);
}

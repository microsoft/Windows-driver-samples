// Copyright (c) Microsoft Corporation.  All rights reserved.
#include "precomp.h"

#include "device.h"

#include "adapter.h"
#include "adapter.tmh"

extern UCHAR NetvMacAddressBase[MAC_ADDR_LEN];

NetvAdapter* NetvAdapterGetContextFromWDFObject(NETADAPTER netAdapter)
{
    WifiNetvAdapter* wifiNetvAdapter = WifiNetvAdapterGetContext(netAdapter);
    NetvAdapter* netvAdapter{ wifiNetvAdapter };
    return netvAdapter;
}

WifiNetvAdapter::WifiNetvAdapter(NETADAPTER Handle, WDFDEVICE Device) : NetvAdapter(Handle, Device)
{
}

NTSTATUS WifiNetvAdapter::Initialize()
{
    if (WifiGetIhvDeviceContext(m_device)->netAdapters[WifiAdapterGetPortId(m_handle)] != WDF_NO_HANDLE)
    {
        return STATUS_SUCCESS;
    }
    return NetvAdapter::Initialize();
}

// the Xfilter.h should be included in the share(currently in km only)
#ifndef _KERNEL_MODE
//
// This macro is used to copy from one network address to
// another.
//
#define ETH_COPY_NETWORK_ADDRESS(_D, _S) \
{ \
    *((ULONG UNALIGNED *)(_D)) = *((ULONG UNALIGNED *)(_S)); \
    *((USHORT UNALIGNED *)((UCHAR *)(_D)+4)) = *((USHORT UNALIGNED *)((UCHAR *)(_S)+4)); \
}
//
// ZZZ This is a little-endian specific check.
//
#define ETH_IS_MULTICAST(Address) \
    (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x01))

//
// Check whether an address is broadcast.
//
#define ETH_IS_BROADCAST(Address)               \
    ((((PUCHAR)(Address))[0] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[1] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[2] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[3] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[4] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[5] == ((UCHAR)0xff)))

#endif // _KERNEL_MODE

NTSTATUS WifiNetvAdapter::NetvAdapterReadAddress()
{
    UCHAR MACLastByteFinial = (UCHAR)MACLastByte;
    if (MACLastByteFinial == 0)
    {
        MACLastByteFinial++;
    }

    PermanentAddress.Length = MAC_ADDR_LEN;

    ETH_COPY_NETWORK_ADDRESS(PermanentAddress.Address, NetvMacAddressBase);
    PermanentAddress.Address[MAC_ADDR_LEN - 1] = MACLastByteFinial;
    if (ETH_IS_MULTICAST(PermanentAddress.Address) || ETH_IS_BROADCAST(PermanentAddress.Address))
    {
        WFCError("%!FUNC!: Failed with %!STATUS!", STATUS_INVALID_ADDRESS);
        return STATUS_INVALID_ADDRESS;
    }

    RtlCopyMemory(&CurrentAddress, &PermanentAddress, sizeof(PermanentAddress));

    EnlIndex = (MACLastByteFinial - 1) >> 1;
    EnlPortIndex = (MACLastByteFinial - 1) & 1;
    EnlIndexValid = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS WifiNetvAdapter::AdapterStart()
{
    TraceEntry();

    NTSTATUS status = STATUS_SUCCESS;

    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES wakeMediaChangeCapabilities;
    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES_INIT(&wakeMediaChangeCapabilities);

    wakeMediaChangeCapabilities.MediaConnect = TRUE;
    wakeMediaChangeCapabilities.MediaDisconnect = TRUE;

    NetAdapterWakeSetMediaChangeCapabilities(m_handle, &wakeMediaChangeCapabilities);

    WIFI_ADAPTER_WAKE_CAPABILITIES wakeCap{};
    WIFI_ADAPTER_WAKE_CAPABILITIES_INIT(&wakeCap);
    if (WIFI_IS_FIELD_AVAILABLE(WIFI_ADAPTER_WAKE_CAPABILITIES, ClientDriverDiagnostic))
    {
        wakeCap.ClientDriverDiagnostic = true;
    }
    WifiAdapterSetWakeCapabilities(m_handle, &wakeCap);

    status = NetvAdapter::ConfigureDataCapabilities();
    if (!NT_SUCCESS(status))
    {
        WFCError("%!FUNC!: NetvAdapter::ConfigureDataCapabilities failed with %!STATUS!", status);
        return status;
    }

    status = NetAdapterStart(m_handle);
    if (!NT_SUCCESS(status))
    {
        WFCError("%!FUNC!: NetAdapterStart failed with %!STATUS!", status);
        return status;
    }

    ASSERT(STATUS_SUCCESS == status);
    TraceExit(status);

    return status;
}

NTSTATUS WifiNetvAdapter::CreateRxQueue(NETRXQUEUE_INIT* NetRxQueueInit)
{
    return NetvAdapter::CreateRxQueue(NetRxQueueInit);
}

NTSTATUS WifiNetvAdapter::CreateTxQueue(NETTXQUEUE_INIT* NetTxQueueInit)
{
    return NetvAdapter::CreateTxQueue(NetTxQueueInit);
}

void WifiNetvAdapter::Destroy(void)
{
    return NetvAdapter::Destroy();
}
//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include "precomp.h"

#include "device.h"
#include "txqueue.h"
#include "rxqueue.h"

_Use_decl_annotations_ NTSTATUS WmbClassAdapterStart(NETADAPTER netAdapter)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = WmbClassGetNetAdapterContext(netAdapter);
    PWMBCLASS_DEVICE_CONTEXT deviceContext = netAdapterContext->WmbDeviceContext;

    ULONG64 maxXmitLinkSpeed = MBB_MEDIA_MAX_SPEED;
    ULONG64 maxRcvLinkSpeed = MBB_MEDIA_MAX_SPEED;
    DWORD maxPowerFilterSize = 0;

    NET_ADAPTER_LINK_LAYER_CAPABILITIES linkLayerCapabilities;
    NET_ADAPTER_LINK_LAYER_CAPABILITIES_INIT(&linkLayerCapabilities, maxXmitLinkSpeed, maxRcvLinkSpeed);

    NetAdapterSetLinkLayerCapabilities(netAdapter, &linkLayerCapabilities);

    if (deviceContext->BusParams.IsErrataDevice)
    {
        NetAdapterSetLinkLayerMtuSize(netAdapter, deviceContext->BusParams.MTU);
    }
    else
    {
        NetAdapterSetLinkLayerMtuSize(netAdapter, deviceContext->BusParams.MaxSegmentSize);
    }

    if (deviceContext->BusParams.PowerFiltersSupported > 0)
    {
        NET_ADAPTER_WAKE_BITMAP_CAPABILITIES bitmapCapabilities;
        NET_ADAPTER_WAKE_BITMAP_CAPABILITIES_INIT(&bitmapCapabilities);

        bitmapCapabilities.BitmapPattern = TRUE;
        bitmapCapabilities.MaximumPatternCount = deviceContext->BusParams.PowerFiltersSupported;

        if (deviceContext->BusParams.IsErrataDevice)
        {
            // For errata devices we will report the device specific max pattern size.
            // 192 bytes is the maximum power filter size as per the MBIM spec.
            // If a device reports more than 192 bytes as the power filter size, we
            // normalize it here to 192 bytes. This will prevent erroneous devices
            // from indicating arbitrary pattern size. This needs to change with any
            // MBIM spec revision.

            if (deviceContext->BusParams.MaxPowerFilterSize > WMBCLASS_MAX_MBIM_WOL_PATTERN)
            {
                maxPowerFilterSize = WMBCLASS_MAX_MBIM_WOL_PATTERN;
            }
            else
            {
                maxPowerFilterSize = deviceContext->BusParams.MaxPowerFilterSize;
            }
        }
        else
        {
            // To maintain backward compatibility with Win8 devices, we continue
            // reporting 256 bytes as the wake pattern size.
            maxPowerFilterSize = WMBCLASS_MAX_WOL_PATTERN;
        }

        bitmapCapabilities.MaximumPatternSize = maxPowerFilterSize;

        NetAdapterWakeSetBitmapCapabilities(netAdapter, &bitmapCapabilities);
    }

    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES wakeMediaChangeCapabilities;
    NET_ADAPTER_WAKE_MEDIA_CHANGE_CAPABILITIES_INIT(&wakeMediaChangeCapabilities);

    wakeMediaChangeCapabilities.MediaConnect = TRUE;
    wakeMediaChangeCapabilities.MediaDisconnect = TRUE;

    NetAdapterWakeSetMediaChangeCapabilities(netAdapter, &wakeMediaChangeCapabilities);

    if (deviceContext->BusParams.SelectiveSuspendSupported)
    {
        NET_ADAPTER_WAKE_PACKET_FILTER_CAPABILITIES wakePacketFilterCapabilities;
        NET_ADAPTER_WAKE_PACKET_FILTER_CAPABILITIES_INIT(&wakePacketFilterCapabilities);
        wakePacketFilterCapabilities.PacketFilterMatch = TRUE;

        NetAdapterWakeSetPacketFilterCapabilities(netAdapter, &wakePacketFilterCapabilities);
    }

    NET_ADAPTER_TX_CAPABILITIES txCapabilities;
    NET_ADAPTER_TX_CAPABILITIES_INIT(&txCapabilities, 1);
    NET_ADAPTER_RX_CAPABILITIES rxCapabilities;
    NET_ADAPTER_RX_CAPABILITIES_INIT_DRIVER_MANAGED(&rxCapabilities, EvtAdapterReturnRxBuffer, MBB_MAX_PACKET_SIZE, 1);

    NetAdapterSetDataPathCapabilities(netAdapter, &txCapabilities, &rxCapabilities);

    status = NetAdapterStart(netAdapter);

    return status;
}

NTSTATUS
EvtAdapterCreateTxQueue(_In_ NETADAPTER netAdapter, _Inout_ NETTXQUEUE_INIT* txQueueInit)
{
    NTSTATUS status = STATUS_SUCCESS;
    PMBB_TXQUEUE_CONTEXT txQueueContext = NULL;
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = WmbClassGetNetAdapterContext(netAdapter);
    PWMBCLASS_DEVICE_CONTEXT deviceContext = netAdapterContext->WmbDeviceContext;
    WDF_OBJECT_ATTRIBUTES txAttributes;
    NET_EXTENSION_QUERY extension;

    WDF_OBJECT_ATTRIBUTES_INIT(&txAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&txAttributes, MBB_TXQUEUE_CONTEXT);

    txAttributes.EvtDestroyCallback = EvtTxQueueDestroy;

    NET_PACKET_QUEUE_CONFIG queueConfig;
    NET_PACKET_QUEUE_CONFIG_INIT(&queueConfig, EvtTxQueueAdvance, EvtTxQueueSetNotificationEnabled, EvtTxQueueCancel);

    NETPACKETQUEUE txQueue;
    status = NetTxQueueCreate(txQueueInit, &txAttributes, &queueConfig, &txQueue);

    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    txQueueContext = MbbGetTxQueueContext(txQueue);


    txQueueContext->NetAdapterContext = netAdapterContext;
    txQueueContext->DatapathDescriptor = NetTxQueueGetRingCollection(txQueue);

    // Try to use MaxOutDatagrams parameter as the tx completion batch size, but limit to half of the packet ring buffer
    auto maxBatchSize = NetRingCollectionGetPacketRing(txQueueContext->DatapathDescriptor)->NumberOfElements / 2;
    txQueueContext->CompletionBatchSize = min(deviceContext->BusParams.MaxOutDatagrams, maxBatchSize);

    NET_EXTENSION_QUERY_INIT(&extension, NET_FRAGMENT_EXTENSION_MDL_NAME, NET_FRAGMENT_EXTENSION_MDL_VERSION_1, NetExtensionTypeFragment);

    NetTxQueueGetExtension(txQueue, &extension, &txQueueContext->MdlExtension);

    netAdapterContext->TxQueue = txQueue;
Exit:
    return status;
}

void EvtRxQueueStart(_In_ NETPACKETQUEUE rxQueue)
{
    MbbGetRxQueueContext(rxQueue)->NetAdapterContext->AllowRxTraffic = TRUE;
}

NTSTATUS
EvtAdapterCreateRxQueue(_In_ NETADAPTER netAdapter, _Inout_ NETRXQUEUE_INIT* rxQueueInit)
{
    NTSTATUS status = STATUS_SUCCESS;

    PMBB_RXQUEUE_CONTEXT rxQueueContext = NULL;
    WDF_OBJECT_ATTRIBUTES rxAttributes;
    NET_EXTENSION_QUERY extension;
    PWMBCLASS_NETADAPTER_CONTEXT netAdapterContext = WmbClassGetNetAdapterContext(netAdapter);

    NET_PACKET_QUEUE_CONFIG rxConfig;
    NET_PACKET_QUEUE_CONFIG_INIT(&rxConfig, EvtRxQueueAdvance, EvtRxQueueSetNotificationEnabled, EvtRxQueueCancel);
    rxConfig.EvtStart = EvtRxQueueStart;

    WDF_OBJECT_ATTRIBUTES_INIT(&rxAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&rxAttributes, MBB_RXQUEUE_CONTEXT);
    rxAttributes.EvtDestroyCallback = EvtRxQueueDestroy;

    NETPACKETQUEUE rxQueue;
    status = NetRxQueueCreate(rxQueueInit, &rxAttributes, &rxConfig, &rxQueue);

    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    rxQueueContext = MbbGetRxQueueContext(rxQueue);

    NET_EXTENSION_QUERY_INIT(&extension, NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_NAME, NET_FRAGMENT_EXTENSION_VIRTUAL_ADDRESS_VERSION_1, NetExtensionTypeFragment);

    NetRxQueueGetExtension(rxQueue, &extension, &rxQueueContext->VirtualAddressExtension);

    rxQueueContext->NetAdapterContext = netAdapterContext;
    rxQueueContext->DatapathDescriptor = NetRxQueueGetRingCollection(rxQueue);

    NET_EXTENSION_QUERY_INIT(&extension, NET_FRAGMENT_EXTENSION_RETURN_CONTEXT_NAME, NET_FRAGMENT_EXTENSION_RETURN_CONTEXT_VERSION_1, NetExtensionTypeFragment);

    NetRxQueueGetExtension(rxQueue, &extension, &rxQueueContext->ReturnContextExtension);

    // Set netAdapterContext->RxQueue to mean EvtAdapterCreateRxQueue completed successfully
    netAdapterContext->RxQueue = rxQueue;

Exit:

    return status;
}

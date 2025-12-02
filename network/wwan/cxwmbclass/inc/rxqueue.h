//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

typedef struct _MBB_RXQUEUE_CONTEXT
{
    PWMBCLASS_NETADAPTER_CONTEXT NetAdapterContext;
    LONG NotificationEnabled;
    NET_RING_COLLECTION const* DatapathDescriptor;
    NET_EXTENSION ReturnContextExtension;
    NET_EXTENSION VirtualAddressExtension;
} MBB_RXQUEUE_CONTEXT, *PMBB_RXQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MBB_RXQUEUE_CONTEXT, MbbGetRxQueueContext);

EVT_WDF_OBJECT_CONTEXT_DESTROY EvtRxQueueDestroy;

EVT_PACKET_QUEUE_SET_NOTIFICATION_ENABLED EvtRxQueueSetNotificationEnabled;
EVT_PACKET_QUEUE_ADVANCE EvtRxQueueAdvance;
EVT_PACKET_QUEUE_CANCEL EvtRxQueueCancel;

VOID MbbNdisReceiveCallback(_In_ MBB_PROTOCOL_HANDLE ProtocolHandle, _In_ MBB_RECEIVE_CONTEXT ReceiveContext, _In_ PMDL Mdl);

VOID EvtAdapterReturnRxBuffer(_In_ NETADAPTER Adapter, _In_ NET_FRAGMENT_RETURN_CONTEXT_HANDLE RxBufferReturnContext);

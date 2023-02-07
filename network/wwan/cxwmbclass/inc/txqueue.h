//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once

typedef struct _MBB_TXQUEUE_CONTEXT
{
    PWMBCLASS_NETADAPTER_CONTEXT NetAdapterContext;
    LONG NotificationEnabled;
    UINT32 CompletionBatchSize;
    NET_RING_COLLECTION const* DatapathDescriptor;
    NET_EXTENSION MdlExtension;
} MBB_TXQUEUE_CONTEXT, *PMBB_TXQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MBB_TXQUEUE_CONTEXT, MbbGetTxQueueContext);

EVT_WDF_OBJECT_CONTEXT_DESTROY EvtTxQueueDestroy;
EVT_PACKET_QUEUE_SET_NOTIFICATION_ENABLED EvtTxQueueSetNotificationEnabled;
EVT_PACKET_QUEUE_CANCEL EvtTxQueueCancel;
EVT_PACKET_QUEUE_ADVANCE EvtTxQueueAdvance;

EVT_MBB_DEVICE_SEND_DEVICE_SERVICE_SESSION_DATA EvtMbbDeviceSendDeviceServiceSessionData;

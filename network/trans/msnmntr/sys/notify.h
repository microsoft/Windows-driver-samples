/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Monitor Sample driver notification header

Environment:

    Kernel mode
    
--*/

#pragma once

#define TAG_NOTIFY 'yftN'

NTSTATUS
MonitorNfInitialize(
   _In_ DEVICE_OBJECT* deviceObject);

NTSTATUS
MonitorNfUninitialize(void);

NTSTATUS MonitorNfNotifyMessage(
   _In_ const FWPS_STREAM_DATA* streamBuffer,
   _In_ BOOLEAN inbound,
   _In_ USHORT localPort,
   _In_ USHORT remotePort);



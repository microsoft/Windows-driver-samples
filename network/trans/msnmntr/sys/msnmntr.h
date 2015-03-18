/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Monitor Sample driver callout header

Environment:

    Kernel mode
    
--*/

#pragma once

typedef struct _FLOW_DATA
{
   UINT64      flowHandle;
   UINT64      flowContext;
   UINT64      calloutId;
   ULONG       localAddressV4;
   USHORT      localPort;
   USHORT      ipProto;
   ULONG       remoteAddressV4;
   USHORT      remotePort;
   WCHAR*      processPath;
   LIST_ENTRY  listEntry;
   BOOLEAN     deleting;
} FLOW_DATA;

NTSTATUS
MonitorCoInitialize(_Inout_ DEVICE_OBJECT* deviceObject);

void
MonitorCoUninitialize(void);

NTSTATUS 
MonitorCoRegisterCallouts(_Inout_ void* deviceObject);

NTSTATUS 
MonitorCoUnregisterCallouts(void);

NTSTATUS 
MonitorCoSetApplication(
   _In_reads_(size) wchar_t*    fileName,
   _In_ ULONG size);

NTSTATUS 
MonitorCoEnableMonitoring(
   _In_   MONITOR_SETTINGS* monitorSettings);

void 
MonitorCoDisableMonitoring(void);


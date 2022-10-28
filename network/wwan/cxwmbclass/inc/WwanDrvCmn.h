/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    WwanDrvCmn.h

Abstract:
    Contains common functions declarations used by WWAN
    class driver and WWAN virtual miniport.
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    12-18-2012    Created

Notes:

--*/
#pragma once

EXTERN_C
VOID WwanAdjustDhcpSettings(__in NET_LUID AdapterNetLuid, __in GUID AdapterNetCfgId, __in BOOLEAN EnableDhcp);

EXTERN_C
NDIS_STATUS
WwanUpdateIPStack(
    __in NDIS_HANDLE MiniportAdapterHandle,
    __in PWWAN_IP_ADDRESS_STATE IpAddressState,
    __in NET_LUID AdapterNetLuid,
    __in GUID AdapterNetCfgId,
    __in ULONG AdapterIfIndex,
    __in BOOLEAN IsDeviceReportedChange);

/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simemi.h

Abstract:

    This module contains the internal declarations of the simemi driver.

--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include "simemipublic.h"

#define SIM_EMI_TAG 'IMES'

typedef struct _SIM_EMI_BUS_FDO_DATA {
    FAST_MUTEX BusMutex;
    LIST_ENTRY ChildDevices;
} SIM_EMI_BUS_FDO_DATA, *PSIM_EMI_BUS_FDO_DATA;

typedef struct _SIM_EMI_CHANNEL_DATA {
    ULONG64 LastPollTime;
    ULONG64 LastAbsoluteEnergy;

    SIM_EMI_BUS_CHANNEL_INFO Info;
} SIM_EMI_CHANNEL_DATA, *PSIM_EMI_CHANNEL_DATA;

#define SIM_EMI_CHANNEL_DATA_SIZE(_ChannelNameSize) \
    (FIELD_OFFSET(SIM_EMI_CHANNEL_DATA, Info) + \
        SIM_EMI_BUS_CHANNEL_INFO_SIZE(_ChannelNameSize))

#define SIM_EMI_CHANNEL_DATA_NEXT_CHANNEL_DATA(_Channel) \
    ((PSIM_EMI_CHANNEL_DATA)((PUCHAR)(_Channel) + \
        SIM_EMI_CHANNEL_DATA_SIZE((_Channel)->Info.ChannelNameSize)))

typedef struct _SIM_EMI_BUS_PDO_DATA {
    LIST_ENTRY Link;
    PFAST_MUTEX BusMutex;

    USHORT EmiVersion;
    ULONG ChildDeviceHandle;
    USHORT ChannelCount;
    SIM_EMI_CHANNEL_DATA ChannelData[ANYSIZE_ARRAY];
} SIM_EMI_BUS_PDO_DATA, *PSIM_EMI_BUS_PDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SIM_EMI_BUS_FDO_DATA, SimEmiGetFdoData);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SIM_EMI_BUS_PDO_DATA, SimEmiGetPdoData);

typedef struct _SIM_EMI_BUS_PDO_IDENTIFICATION_INFO {
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header;

    USHORT EmiVersion;
    ULONG ChildDeviceHandle;

    USHORT ChannelCount;
    ULONG ChannelInfoSize;
    PSIM_EMI_BUS_CHANNEL_INFO ChannelInfo;
} SIM_EMI_BUS_PDO_IDENTIFICATION_INFO, *PSIM_EMI_BUS_PDO_IDENTIFICATION_INFO;

static LPCWSTR SimEmiHardwareOEM = L"SimEmi";
static LPCWSTR SimEmiHardwareModelV1 = L"SimEmiV1";
static LPCWSTR SimEmiHardwareModelV2 = L"SimEmiV2";
static const USHORT SimEmiHardwareRevisionV1 = 1;
static const USHORT SimEmiHardwareRevisionV2 = 2;

//
// simemifdo.c routine declarations
//

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL SimEmiFdoControl;
EVT_WDF_DRIVER_DEVICE_ADD SimEmiFdoCreateDevice;

//
// simemipdo.c routine declarations
//

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL SimEmiPdoControl;
EVT_WDF_CHILD_LIST_CREATE_DEVICE SimEmiPdoCreateDevice;
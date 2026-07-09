// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 EvtDeviceArmWakeFromS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 EvtDeviceDisarmWakeFromS0;

void
NetvDeviceInitializePowerManagement(
    _In_ NetvDevice * Device
);

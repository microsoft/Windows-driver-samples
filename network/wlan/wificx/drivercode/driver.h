// Copyright (c) Microsoft Corporation.  All rights reserved.
#pragma once
#include "precomp.h"

WDF_EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD EvtWifiDriverDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWifiDriverContextCleanup;

WDF_EXTERN_C_END
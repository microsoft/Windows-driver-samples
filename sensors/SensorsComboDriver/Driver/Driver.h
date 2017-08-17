// Copyright (C) Microsoft Corporation, All Rights Reserved
//
// Abstract:
//
//  This module contains the type definitions for the combo
//  driver callback class.
//
// Environment:
//
//  Windows User-Mode Driver Framework (WUDF)

#pragma once

WDF_EXTERN_C_START

//
// Driver Entry/Exit Functions
//
DRIVER_INITIALIZE                        DriverEntry;
EVT_WDF_DRIVER_UNLOAD                    OnDriverUnload;

//
// WDF Device Callbacks
//
EVT_WDF_DRIVER_DEVICE_ADD                OnDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE          OnPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE          OnReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY                  OnD0Entry;
EVT_WDF_DEVICE_D0_EXIT                   OnD0Exit;

WDF_EXTERN_C_END
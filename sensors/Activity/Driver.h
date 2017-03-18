// Copyright (C) Microsoft Corporation, All Rights Reserved.
//
// Abstract:
//     This module contains the implementation of entry and exit point of activity sample driver.
//
// Environment:
//     Windows User-Mode Driver Framework (UMDF)

#pragma once

WDF_EXTERN_C_START

DRIVER_INITIALIZE       DriverEntry;
EVT_WDF_DRIVER_UNLOAD   OnDriverUnload;

WDF_EXTERN_C_END
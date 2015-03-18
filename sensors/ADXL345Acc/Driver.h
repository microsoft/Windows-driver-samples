//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//    This module contains the type definitions for the ADXL345 accelerometer
//    driver callback class.
//
//Environment:
//
//    Windows User-Mode Driver Framework (UMDF)

#pragma once

WDF_EXTERN_C_START

DRIVER_INITIALIZE           DriverEntry;
EVT_WDF_DRIVER_UNLOAD       OnDriverUnload;

WDF_EXTERN_C_END
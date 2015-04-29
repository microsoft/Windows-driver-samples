/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Driver.h

Abstract:

    This module contains the type definitions for the VirtualSerial sample's
    driver callback class.

Environment:

    Windows Driver Framework

--*/

#pragma once

//
// This class handles driver events for the VirtualSerial sample.  In particular
// it supports the OnDeviceAdd event, which occurs when the driver is called
// to setup per-device handlers for a new device stack.
//

DRIVER_INITIALIZE           DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD   EvtDeviceAdd;

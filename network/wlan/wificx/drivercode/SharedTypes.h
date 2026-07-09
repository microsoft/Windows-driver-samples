// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

//
// SharedTypes.h
// Central header for global definitions, macros, and shared structures.
//
#include "precomp.h"
#include <initguid.h> // for GUID defination

// =============================
// GUIDs or Constants
// =============================
// {bb67559a-06f6-4eb0-81e9-21fdc3b60efb}
DEFINE_GUID(GUID_WIFICX_SAMPLE_CLIENT_INTERFACE, 0xbb67559a, 0x06f6, 0x4eb0, 0x81, 0xe9, 0x21, 0xfd, 0xc3, 0xb6, 0x0e, 0xfb);
#define WIFI_DRIVER_DEFAULT_POOL_TAG 'shiW' // WIFI IHV Sample Driver

// =============================
// OEM Device Service contract
// =============================
// This GUID/opcode pair MUST match the OEM user-mode app (OEM\OemDeviceService.cpp).
// {2d6f9a14-3a1d-4f0a-9b7e-1c2e3a4b5c6d}
DEFINE_GUID(GUID_OEM_SAMPLE_DEVICE_SERVICE,
    0x2d6f9a14, 0x3a1d, 0x4f0a, 0x9b, 0x7e, 0x1c, 0x2e, 0x3a, 0x4b, 0x5c, 0x6d);

// Opcode understood by the driver for the "hello / nice to meet you" exchange.
#define OEM_DEVICE_SERVICE_OPCODE_HELLO   0x00000001

// Payload strings exchanged with the OEM app.
#define OEM_DEVICE_SERVICE_REQUEST_STRING   "Hello, My Driver"
#define OEM_DEVICE_SERVICE_RESPONSE_STRING  "Nice to meet you, My OEM"
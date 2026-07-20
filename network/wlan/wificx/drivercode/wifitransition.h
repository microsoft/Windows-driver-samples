// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once
#include "precomp.h"
#include "wifiHAL.h"
#include "wifirequest.h"

// Generic execution context passed between steps and parse/handle
struct TransitionContext
{
    WDFDEVICE Device;
    PWIFI_IHV_DEVICE_CONTEXT DevCtx;
    WIFIREQUEST WifiRequest;
    PWDI_MESSAGE_HEADER Header;
    void* RawBuffer; // from WifiRequestGetInOutBuffer in EvtWifiDeviceSendCommand
    UINT InLen;
    UINT OutLen;
};

// -------- Runtime dispatcher (decl) --------
NTSTATUS RunTransitionByMessage(TransitionContext& ctx, UINT16 messageId);
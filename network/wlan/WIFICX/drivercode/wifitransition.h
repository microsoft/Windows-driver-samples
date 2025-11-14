#pragma once
#include "precomp.h"
#include "wifiHAL.h"
#include "wifirequest.h" // WifiIhvSendM4IndicationToOs, DumpMessageTlvByteStream, Wifi::ConvertNDISSTATUSToNTSTATUS

// Generic execution context passed between steps and parse/handle
struct TransitionContext
{
    WDFDEVICE Device;
    PWIFI_IHV_DEVICE_CONTEXT DevCtx;
    WIFIREQUEST WifiRequest;
    PWDI_MESSAGE_HEADER Header;
    void* RawBuffer;
    UINT InLen;
    UINT OutLen;
};

// -------- Runtime dispatcher (decl) --------
NTSTATUS RunTransitionByMessage(TransitionContext& ctx, UINT16 messageId);
// Copyright (C) Microsoft Corporation. All rights reserved.

#include "wifitransition.h"

// Common TLV parsing helper: computes TLV span, optional dump, calls a parser, converts status.
template<typename Param, typename ParserFn>
NTSTATUS ParseTlvCommon(TransitionContext& ctx,
    UINT16 messageId,
    ParserFn parser,
    Param& outParams,
    bool dumpStream = true)
{
    if (ctx.InLen < sizeof(WDI_MESSAGE_HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    auto* tlvBytes = static_cast<UCHAR*>(ctx.RawBuffer) + sizeof(WDI_MESSAGE_HEADER);
    auto tlvLen = static_cast<ULONG>(ctx.InLen - sizeof(WDI_MESSAGE_HEADER));

    if (dumpStream)
    {
        DumpMessageTlvByteStream(
            messageId,
            TRUE,
            ctx.DevCtx->TlvContext.PeerVersion,
            tlvLen,
            tlvBytes,
            0,
            nullptr);
    }

    auto ndisStatus = parser(tlvLen, tlvBytes, &ctx.DevCtx->TlvContext, &outParams);
    return Wifi::ConvertNDISSTATUSToNTSTATUS(ndisStatus);
}


// Primary traits template (specialize per MessageId)
template<UINT16 MsgId>
struct TransitionTraits;

// --- Generic pure-type traits template (add before existing specializations) ---
// WIFIREQUEST typically requires M3 notification, making TPreM3Fn important. 
// WIFICX expects M3 -> M4 order under normal conditions;
// using template parameters to configure parsing, cleanup, M3/M4 steps.
// to make sure that all transitions have consistent implementations.
template<
    UINT16 TMsgId,
    typename TParam,
    UINT16 TCompleteIndication,
    bool TDumpTlvStream,
    NDIS_STATUS (*TParseFn)(ULONG, const UINT8*, PCTLV_CONTEXT, TParam*),
    void (*TCleanupFn)(TParam*),
    NTSTATUS (WifiHAL::*TPreM3Fn)(),                                                                   // mandatory pre-M3 hook
    NTSTATUS (WifiHAL::*THalM3Fn)(const TParam&, const PWDI_MESSAGE_HEADER, UINT BytesWriten),         // optional HAL M3 (may be nullptr)
    NTSTATUS (WifiHAL::*TPreM4Fn)(),                                                                   // optional pre-M4 hook (may be nullptr)
    NTSTATUS (WifiHAL::*THalM4Fn)(const PWDI_MESSAGE_HEADER)                                           // optional HAL M4 (may be nullptr)
>
struct GenericTransitionTraits
{
    using ParamType = TParam;
    enum : UINT16 { CompleteIndication = TCompleteIndication };

    NTSTATUS Parse(TransitionContext& ctx, ParamType& p)
    {
        if (ctx.InLen < sizeof(WDI_MESSAGE_HEADER))
        {
            return STATUS_INVALID_PARAMETER;
        }

        auto* tlvBytes = static_cast<UCHAR*>(ctx.RawBuffer) + sizeof(WDI_MESSAGE_HEADER);
        auto tlvLen = static_cast<ULONG>(ctx.InLen - sizeof(WDI_MESSAGE_HEADER));

        if (TDumpTlvStream)
        {
            DumpMessageTlvByteStream(
                TMsgId,
                TRUE,
                ctx.DevCtx->TlvContext.PeerVersion,
                tlvLen,
                tlvBytes,
                0,
                nullptr);
        }

        auto ndisStatus = TParseFn(tlvLen, tlvBytes, &ctx.DevCtx->TlvContext, &p);
        return Wifi::ConvertNDISSTATUSToNTSTATUS(ndisStatus);
    }

    void Cleanup(ParamType& p) { TCleanupFn(&p); }

    // Make static so pointer matches ExecuteSteps expected callable type (no implicit this)
    static NTSTATUS StepM3(TransitionContext& c, ParamType& p, UINT& bytesWritten)
    {
        bytesWritten = sizeof(WDI_MESSAGE_HEADER);
        ASSERT(TPreM3Fn);

        if (TPreM3Fn)
        {
            // Call member function pointer on WifiHAL instance
            NTSTATUS preStatus = (GetWifiHalFromHandle(c.DevCtx->WdfDevice)->*TPreM3Fn)();
            if (!NT_SUCCESS(preStatus))
            {
                return preStatus;
            }
        }

        if (THalM3Fn)
        {
            // Pass required third argument (BytesWriten) to HAL M3 function
            return (GetWifiHalFromHandle(c.DevCtx->WdfDevice)->*THalM3Fn)(p, c.Header, bytesWritten);
        }

        return STATUS_SUCCESS;
    }

    static NTSTATUS StepM4(TransitionContext& c, ParamType&)
    {
        if (TPreM4Fn)
        {
            NTSTATUS preStatus = (GetWifiHalFromHandle(c.DevCtx->WdfDevice)->*TPreM4Fn)();
            if (!NT_SUCCESS(preStatus))
            {
                return preStatus;
            }
        }

        if (THalM4Fn)
        {
            return (GetWifiHalFromHandle(c.DevCtx->WdfDevice)->*THalM4Fn)(c.Header);
        }

        return (TPreM4Fn == nullptr && THalM4Fn == nullptr) ? STATUS_PENDING : STATUS_SUCCESS;
    }

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p, &GenericTransitionTraits::StepM3, &GenericTransitionTraits::StepM4);
    }

    bool ShouldSendComplete(NTSTATUS s) const { return s != STATUS_PENDING; }
};

// Execute two step callables.
// StepM3Fn signature: NTSTATUS (TransitionContext&, Param&, UINT& bytesWritten)
// StepM4Fn signature: NTSTATUS (TransitionContext&, Param&)
// Always calls WifiRequestComplete after StepM3 with the bytesWritten produced by StepM3.
// Skips StepM4 if StepM3 failed
template<typename Param, typename StepM3Fn, typename StepM4Fn>
NTSTATUS ExecuteSteps(TransitionContext& ctx, Param& p, StepM3Fn stepM3, StepM4Fn stepM4)
{
    UINT bytesWritten = sizeof(WDI_MESSAGE_HEADER); // default minimum
    NTSTATUS m3Status = stepM3(ctx, p, bytesWritten);
    // Report the M3 status back to OS
    // OS expects M3 before the M4
    WifiIhvNotifyM3Completion(ctx.WifiRequest, m3Status, bytesWritten);
    if (!NT_SUCCESS(m3Status))
    {
        return m3Status;
    }
    return stepM4(ctx, p);
}

// -------- Generic runner (compile-time) --------
template<UINT16 MsgId>
NTSTATUS RunTransition(TransitionContext& ctx)
{
    TransitionTraits<MsgId> traits;
    typename TransitionTraits<MsgId>::ParamType params{};
    NTSTATUS parseStatus = traits.Parse(ctx, params);
    if (!NT_SUCCESS(parseStatus))
    {
        traits.Cleanup(params);
        // Report Failed M3 to OS
        // Note: No M4 indication on parse failure
        WifiIhvNotifyM3Completion(ctx.WifiRequest, parseStatus, 0);
        return parseStatus;
    }

    NTSTATUS m4Status = traits.Handle(ctx, params);

    if (traits.ShouldSendComplete(m4Status))
    {
        WifiIhvSendM4IndicationToOs(
            ctx.Device,
            TransitionTraits<MsgId>::CompleteIndication,
            ctx.Header,
            m4Status);
    }

    traits.Cleanup(params);
    return m4Status;
}

//// -------- SCENARIO: [Connect with a SAE WI-FI7 network --------
///     Demo: Handle WDI_TASK_CONNECT + WDI_SET_SAE_AUTH_PARAMS then WDI_TASK_DISCONNECT
///     Scope:
///            -WifiRequest WDI_TASK_CONNECT & WDI_TASK_DISCONNECT are both WIFICX task commands, which is a two step M3/M4 transition
///            -The direct WifiRequest WDI_SET_SAE_AUTH_PARAMS, which is a single step transition but
///             is logically part of the connect scenario. since WDI_SET_SAE_AUTH_PARAMS is WIFICX property command,
///             it only has M3 step, no M4 step.
///            - The WifiCx unsolicited indication e.g. WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED is sent from the HAL during the connect process,
///     Notes:
///            - M3 and M4 status mainly used for WifiCx to track progress of the transition. e.g. the hung detection and trigger recovery.
///            - The actual scenario result is reported through unsolicited indication.
/// 

// -------- WDI_TASK_CONNECT --------
template<>
struct TransitionTraits<WDI_TASK_CONNECT>
    : GenericTransitionTraits <
    WDI_TASK_CONNECT,
    WDI_TASK_CONNECT_PARAMETERS,
    WDI_INDICATION_CONNECT_COMPLETE,
    true, // dump TLV stream? (was true in original)
    ParseWdiTaskConnect,
    CleanupParsedWdiTaskConnect,
    &WifiHAL::WifiIhvIsDeviceReadyForRequest, // pre-M3
    &WifiHAL::WifiIhvConnect, // HAL M3
    &WifiHAL::WifiIhvGetPendingTransitionStatus, // pre-M4
    nullptr
    >
{
};

// --- WDI_SET_SAE_AUTH_PARAMS ---
template<>
struct TransitionTraits<WDI_SET_SAE_AUTH_PARAMS>
    : GenericTransitionTraits<
        WDI_SET_SAE_AUTH_PARAMS,
        WDI_SET_SAE_AUTH_PARAMS_COMMAND,
        WDI_INDICATION_CONNECT_COMPLETE,
        false, // dump TLV stream? (was false in original)
        ParseWdiSetSaeAuthParams,
        CleanupParsedWdiSetSaeAuthParams,
    &WifiHAL::WifiIhvIsDeviceReadyForRequest, // pre-M3
    &WifiHAL::WifiIhvSetSaeAuthParams, // HAL M3
    &WifiHAL::WifiIhvGetPendingTransitionStatus, // pre-M4
    nullptr// HAL M4
    >
{};

// -------- WDI_TASK_DISCONNECT --------
template<>
struct TransitionTraits<WDI_TASK_DISCONNECT>
    : GenericTransitionTraits<
    WDI_TASK_DISCONNECT,
    WDI_TASK_DISCONNECT_PARAMETERS,
    WDI_INDICATION_DISCONNECT_COMPLETE,
    false, // dump TLV stream? (was false in original)
    ParseWdiTaskDisconnect,
    CleanupParsedWdiTaskDisconnect,
    &WifiHAL::WifiIhvIsDeviceReadyForRequest, // pre-M3
    &WifiHAL::WifiIhvDisconnect, // HAL M3
    &WifiHAL::WifiIhvGetPendingTransitionStatus, // pre-M4
    nullptr // HAL M4
    >
{
};
/// ----- End of scenario [Connect with a SAE WI-FI7 network]-----

// -------- WDI_TASK_DOT11_RESET --------
template<>
struct TransitionTraits<WDI_TASK_DOT11_RESET>
    : GenericTransitionTraits <
    WDI_TASK_DOT11_RESET,
    WDI_TASK_DOT11_RESET_PARAMETERS,
    WDI_INDICATION_DOT11_RESET_COMPLETE,
    false, // dump TLV stream? (was false in original)
    ParseWdiTaskDot11Reset,
    CleanupParsedWdiTaskDot11Reset,
    &WifiHAL::WifiIhvIsDeviceReadyForRequest, // pre-M3
    &WifiHAL::WifiIhvReset, // HAL M3
    &WifiHAL::WifiIhvGetPendingTransitionStatus, // pre-M4
    nullptr  // HAL M4
    >
{
};

// -------- WDI_TASK_SCAN --------
template<>
struct TransitionTraits<WDI_TASK_SCAN>
    : GenericTransitionTraits <
    WDI_TASK_SCAN,
    WDI_SCAN_PARAMETERS,
    WDI_INDICATION_SCAN_COMPLETE,
    true, // dump TLV stream? (was true in original)
    ParseWdiTaskScan,
    CleanupParsedWdiTaskScan,
    &WifiHAL::WifiIhvIsDeviceReadyForRequest, // pre-M3
    &WifiHAL::WifiIhvScan, // HAL M3
    &WifiHAL::WifiIhvGetPendingTransitionStatus, // pre-M4
    nullptr  // HAL M4
    >
{
};

// -------- WDI_TASK_SET_RADIO_STATE --------
template<>
struct TransitionTraits<WDI_TASK_SET_RADIO_STATE>
    : GenericTransitionTraits <
    WDI_TASK_SET_RADIO_STATE,
    WDI_SET_RADIO_STATE_PARAMETERS,
    WDI_INDICATION_SET_RADIO_STATE_COMPLETE,
    true, // dump TLV stream? (was true in original)
    ParseWdiTaskSetRadioState,
    CleanupParsedWdiTaskSetRadioState,
    &WifiHAL::WifiIhvIsDeviceReadyForRequest, // pre-M3
    &WifiHAL::WifiIhvSetRadioState, // HAL M3
    &WifiHAL::WifiIhvGetPendingTransitionStatus, // pre-M4
    nullptr  // HAL M4
    >
{
};

// Runtime dispatcher switches on MessageId and invokes the matching compile-time runner.
NTSTATUS RunTransitionByMessage(TransitionContext& ctx, UINT16 messageId)
{
    switch (messageId)
    {
    case WDI_TASK_SET_RADIO_STATE:
        return RunTransition<WDI_TASK_SET_RADIO_STATE>(ctx);
    case WDI_TASK_SCAN:
        return RunTransition<WDI_TASK_SCAN>(ctx);
    case WDI_TASK_DOT11_RESET:
        return RunTransition<WDI_TASK_DOT11_RESET>(ctx);
    case WDI_TASK_CONNECT:
        return RunTransition<WDI_TASK_CONNECT>(ctx);
    case WDI_TASK_DISCONNECT:
        return RunTransition<WDI_TASK_DISCONNECT>(ctx);
    case WDI_SET_SAE_AUTH_PARAMS:
        return RunTransition<WDI_SET_SAE_AUTH_PARAMS>(ctx);
    default:
        UINT bytesWritten = sizeof(WDI_MESSAGE_HEADER);
        WifiRequestComplete(ctx.WifiRequest, STATUS_NOT_SUPPORTED, bytesWritten);
        return STATUS_NOT_SUPPORTED;
    }
}
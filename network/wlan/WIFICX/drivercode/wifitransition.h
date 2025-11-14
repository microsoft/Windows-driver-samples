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
// Primary traits template (specialize per MessageId)
template<UINT16 MsgId>
struct TransitionTraits;

// -------- WDI_TASK_SET_RADIO_STATE --------
template<>
struct TransitionTraits<WDI_TASK_SET_RADIO_STATE>
{
    using ParamType = WDI_SET_RADIO_STATE_PARAMETERS;
    enum : UINT16 { CompleteIndication = WDI_INDICATION_SET_RADIO_STATE_COMPLETE };

    NTSTATUS Parse(TransitionContext& ctx, ParamType& p)
    {
        return ParseTlvCommon(ctx,
            WDI_TASK_SET_RADIO_STATE,
            [](ULONG len, UINT8* bytes, PCTLV_CONTEXT tlvCtx, ParamType* out)
            {
                return ParseWdiTaskSetRadioState(len, bytes, tlvCtx, out);
            },
            p,
            true);
    }
    void Cleanup(ParamType& p) { CleanupParsedWdiTaskSetRadioState(&p); }

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p,
            // STEPM3
            [](TransitionContext&, ParamType&, UINT& bytesWritten) 
            { 
                bytesWritten = sizeof(WDI_MESSAGE_HEADER);
                return STATUS_SUCCESS; 
            },
            // STEPM4
            [](TransitionContext& c, ParamType& par)
            { 
                return c.DevCtx->wifiHAL->WifiIhvSetRadioState(par, c.Header);
            }
            );
    }
    bool ShouldSendComplete(NTSTATUS) const { return true; }
};

// -------- WDI_TASK_SCAN --------
template<>
struct TransitionTraits<WDI_TASK_SCAN>
{
    using ParamType = WDI_SCAN_PARAMETERS;
    enum : UINT16 { CompleteIndication = WDI_INDICATION_SCAN_COMPLETE };

    NTSTATUS Parse(TransitionContext& ctx, ParamType& p)
    {
        return ParseTlvCommon(ctx,
            WDI_TASK_SCAN,
            [](ULONG len, UINT8* bytes, PCTLV_CONTEXT tlvCtx, ParamType* out)
            {
                return ParseWdiTaskScan(len, bytes, tlvCtx, out);
            },
            p,
            true);
    }
    void Cleanup(ParamType& p){ CleanupParsedWdiTaskScan(&p); }

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p,
            // STEPM3
            [](TransitionContext&, ParamType&, UINT& bytesWritten)
            {
                bytesWritten = sizeof(WDI_MESSAGE_HEADER);
                return STATUS_SUCCESS;
            },
            // STEPM4
            [](TransitionContext& c, ParamType& par)
            {
                return c.DevCtx->wifiHAL->WifiIhvScan(par, c.Header);
            }
        );
    }
    bool ShouldSendComplete(NTSTATUS) const { return true; }
};

// -------- WDI_TASK_DOT11_RESET --------
template<>
struct TransitionTraits<WDI_TASK_DOT11_RESET>
{
    using ParamType = WDI_TASK_DOT11_RESET_PARAMETERS;
    enum : UINT16 { CompleteIndication = WDI_INDICATION_DOT11_RESET_COMPLETE };

    NTSTATUS Parse(TransitionContext& ctx, ParamType& p)
    {
        return ParseTlvCommon(ctx,
            WDI_TASK_DOT11_RESET,
            [](ULONG len, UINT8* bytes, PCTLV_CONTEXT tlvCtx, ParamType* out)
            {
                return ParseWdiTaskDot11Reset(len, bytes, tlvCtx, out);
            },
            p,
            false);
    }
    void Cleanup(ParamType& p){ CleanupParsedWdiTaskDot11Reset(&p); }

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p,
            // STEPM3
            [](TransitionContext&, ParamType&, UINT& bytesWritten)
            {
                bytesWritten = sizeof(WDI_MESSAGE_HEADER);
                return STATUS_SUCCESS;
            },
            // STEPM4
            [](TransitionContext& c, ParamType& par)
            {
                return c.DevCtx->wifiHAL->WifiIhvReset(par, c.Header);
            });
    }
    bool ShouldSendComplete(NTSTATUS) const { return true; }
};

// -------- WDI_TASK_CONNECT --------
template<>
struct TransitionTraits<WDI_TASK_CONNECT>
{
    using ParamType = WDI_TASK_CONNECT_PARAMETERS;
    enum : UINT16 { CompleteIndication = WDI_INDICATION_CONNECT_COMPLETE };

    NTSTATUS Parse(TransitionContext& ctx, ParamType& p)
    {
        return ParseTlvCommon(ctx,
            WDI_TASK_CONNECT,
            [](ULONG len, UINT8* bytes, PCTLV_CONTEXT tlvCtx, ParamType* out)
            {
                return ParseWdiTaskConnect(len, bytes, tlvCtx, out);
            },
            p,
            true);
    }
    void Cleanup(ParamType& p){ CleanupParsedWdiTaskConnect(&p); }

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p,
            // STEPM3
            [](TransitionContext&, ParamType&, UINT& bytesWritten)
            {
                bytesWritten = sizeof(WDI_MESSAGE_HEADER);
                return STATUS_SUCCESS;
            },
            // STEPM4
            [](TransitionContext& c, ParamType& par)
            {
                return c.DevCtx->wifiHAL->WifiIhvConnect(par, c.Header);
            });
    }
    bool ShouldSendComplete(NTSTATUS s) const { return s != STATUS_PENDING; }
};

// -------- WDI_TASK_DISCONNECT --------
template<>
struct TransitionTraits<WDI_TASK_DISCONNECT>
{
    struct ParamType {};
    enum : UINT16 { CompleteIndication = WDI_INDICATION_DISCONNECT_COMPLETE };

    NTSTATUS Parse(TransitionContext&, ParamType&){ return STATUS_SUCCESS; }
    void Cleanup(ParamType&){}

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p,
            // STEPM3
            [](TransitionContext&, ParamType&, UINT& bytesWritten)
            {
                bytesWritten = sizeof(WDI_MESSAGE_HEADER);
                return STATUS_SUCCESS;
            },
            // STEPM4
            [](TransitionContext& c, ParamType&)
            {
                c.DevCtx->wifiHAL->WifiIhvPerformDisassociation(c.Header, WDI_ASSOC_STATUS_DISASSOCIATED_BY_HOST);
                return STATUS_SUCCESS;
            });
    }
    bool ShouldSendComplete(NTSTATUS) const { return true; }
};

// -------- WDI_SET_SAE_AUTH_PARAMS --------
template<>
struct TransitionTraits<WDI_SET_SAE_AUTH_PARAMS>
{
    using ParamType = WDI_SET_SAE_AUTH_PARAMS_COMMAND;
    enum : UINT16 { CompleteIndication = 0 };

    NTSTATUS Parse(TransitionContext& ctx, ParamType& p)
    {
        return ParseTlvCommon(ctx,
            WDI_SET_SAE_AUTH_PARAMS,
            [](ULONG len, UINT8* bytes, PCTLV_CONTEXT tlvCtx, ParamType* out)
            {
                return ParseWdiSetSaeAuthParams(len, bytes, tlvCtx, out);
            },
            p,
            /*dumpStream*/ false);
    }

    void Cleanup(ParamType& p)
    {
        CleanupParsedWdiSetSaeAuthParams(&p);
    }

    NTSTATUS Handle(TransitionContext& ctx, ParamType& p)
    {
        return ExecuteSteps(ctx, p,
            // Step M3: invoke HAL handler, report bytes written (just header)
            [](TransitionContext& c, ParamType& par, UINT& bytesWritten)
            {
                bytesWritten = sizeof(WDI_MESSAGE_HEADER);
                return c.DevCtx->wifiHAL->WifiIhvSetSaeAuthParams(par, c.Header);
            },
            // Step M4: no-op (no completion indication for this OID)
            [](TransitionContext&, ParamType&) { return STATUS_SUCCESS; });
    }

    bool ShouldSendComplete(NTSTATUS) const
    {
        // Do not send M4 indication for WDI_SET_SAE_AUTH_PARAMS (original code omitted it).
        return false;
    }
};

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

// -------- Runtime dispatcher (decl) --------
NTSTATUS RunTransitionByMessage(TransitionContext& ctx, UINT16 messageId);
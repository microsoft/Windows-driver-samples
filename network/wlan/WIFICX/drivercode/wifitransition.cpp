#include "wifitransition.h"

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
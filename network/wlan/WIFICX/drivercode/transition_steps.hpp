template<typename Param, typename... StepFns>
NTSTATUS ExecuteSteps(TransitionContext& ctx, Param& p, StepFns... fns)
{
    NTSTATUS status = STATUS_SUCCESS;
    auto seq = { (status = (fns(ctx, p)), status == STATUS_SUCCESS ? 0 : 1)... };
    UNREFERENCED_PARAMETER(seq);
    return status;
}

// In a traits Handle():
static NTSTATUS Handle(TransitionContext& ctx, ParamType& params)
{
    return ExecuteSteps(ctx, params,
        [](TransitionContext& c, ParamType& par){ return c.DevCtx->wifiHAL->WifiIhvSetRadioState(par, c.Header); },
        [](TransitionContext& c, ParamType&){ return STATUS_SUCCESS; } // extra step
    );
}
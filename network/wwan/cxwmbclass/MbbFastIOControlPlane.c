#include "precomp.h"
#include "MbbFastIOControlPlane.tmh"

BOOL FastIOPause(_In_ PSTATE_CHANGE_EVENT StateChange)
{
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        NdisAcquireSpinLock(&Adapter->Lock);
        ASSERT(Adapter->AdapterState.Started);
        Adapter->AdapterState.Started = FALSE;
        NdisReleaseSpinLock(&Adapter->Lock);

        //MbbBusPause must return NDIS_STATUS_SUCCESS since we only support synchronous FastIOPause now
        Status = MbbBusPause(Adapter->BusHandle);
        ASSERT(Status == NDIS_STATUS_SUCCESS);
        TraceInfo(WMBCLASS_INIT, "FastIO Completing Pause");

        MbbWriteEvent(
            &PAUSE_COMPLETE_EVENT,
            NULL,
            NULL,
            2,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &Status,
            sizeof(Status)
        );

        NdisMPauseComplete(Adapter->MiniportAdapterHandle);
        CompleteStateChange(
            &Adapter->AdapterState,
            StateChange
        );
        return TRUE;
    }

    return FALSE;
}

BOOL FastIOReset(_In_ PSTATE_CHANGE_EVENT StateChange)
{
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        //FastIOReset must return NDIS_STATUS_SUCCESS since we only support synchronous FastIOReset now
        Status = MbbBusReset(Adapter->BusHandle);
        ASSERT(Status == NDIS_STATUS_SUCCESS);
        TraceInfo(WMBCLASS_INIT, "FastIO Completing Reset");

        MbbWriteEvent(
            &RESET_COMPLETE_EVENT,
            NULL,
            NULL,
            2,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &Status,
            sizeof(Status)
        );
        NdisMResetComplete(
            Adapter->MiniportAdapterHandle,
            StateChange->Reset.PipeStartStatus,
            FALSE
        );
        CompleteStateChange(
            &Adapter->AdapterState,
            StateChange
        );
        return TRUE;
    }

    return FALSE;
}

BOOL FastIORestart(_In_ PSTATE_CHANGE_EVENT StateChange)
{
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)StateChange->Context1;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        NdisAcquireSpinLock(&Adapter->Lock);
        Adapter->AdapterState.Started = TRUE;
        NdisReleaseSpinLock(&Adapter->Lock);

        //FastIORestart must return NDIS_STATUS_SUCCESS since we only support synchronous FastIORestart now
        Status = MbbBusRestart(Adapter->BusHandle);
        ASSERT(Status == NDIS_STATUS_SUCCESS);
        TraceInfo(WMBCLASS_INIT, "FastIO Completing Restart");

        MbbWriteEvent(
            &RESTART_COMPLETE_EVENT,
            NULL,
            NULL,
            2,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &Status,
            sizeof(Status)
        );

        NdisMRestartComplete(Adapter->MiniportAdapterHandle, Status);
        CompleteStateChange(
            &Adapter->AdapterState,
            StateChange
        );
        return TRUE;
    }

    return FALSE;
}
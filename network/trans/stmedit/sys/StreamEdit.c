/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:
   Stream Edit Callout Driver Sample.

   This sample demonstrates finding and replacing a string pattern from a
   live TCP stream via the WFP stream API.

   The driver demonstrates the two modes of stream editing/inspection --

      o  Inline Editing where all modification is carried out within the
         WFP ClassifyFn callout function.

      o  Out-of-band (OOB) Editing where all modification is done by a 
         worker thread. (this is the default)

   The mode setting, along with other inspection parameters are configurable
   via the following registry values

  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\StmEdit\Parameters
      
      o  StringToFind	 (REG_SZ, default = "rainy")
      o  StringX		 (REG_SZ, default = "cloudy")
      o  StringToReplace (REG_SZ, default = "sunny")

      o  InspectionLocalPort (REG_DWORD, default = 8888)

      o  InspectionRemotePort (REG_DWORD, default = 0)
            Note: for this sample, a local or remote port is mandatory. Both cannot be zero.

      o  InspectioDirection (REG_DWORD, default = 2)
            possible values : 2 (inbound + outbound), 0 (FWP_DIRECTION_OUTBOUND), 1 (FWP_DIRECTION_INBOUND)

      o  MultipleCallouts (REG_DWORD, default = true/1)
            controls registration of multiple callouts. Set 0 for false, other for TRUE

      o  BusyThreshold (REG_DWORD, default = 16KB)
            BusyThreshold value is in KBs (e.g. a value of 5 means 5KB)

   The sample is IP version agnostic. It is capable of performing inspections
   on both IPv4 and IPv6 data streams

   Before experimenting with the sample, please be sure to add an exception for
   the InspectionPort configured to the firewall. 

Environment:
    Kernel mode

--*/

#include "Trace.h"
#include "StreamEdit.h"
#include "StreamEdit.tmh"

STMEDIT_GLOBALS         Globals;

DRIVER_INITIALIZE       DriverEntry;
EVT_WDF_DRIVER_UNLOAD   StreamEditEvtDriverUnload;

#if defined _MODULE_ID
#undef _MODULE_ID
#endif
#define _MODULE_ID  'S'

VOID
StmEditReferenceFlow(
    _Inout_ PSTREAM_FLOW_CONTEXT FlowContext,
    _In_    char Module,
    _In_    UINT Line
    )
{
    LONG Count = InterlockedIncrement((LONG *)&FlowContext->RefCount);
    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_REFCOUNT, "FlowCtx %p RefCount++ @ %c/%lu = %lu", FlowContext, Module, Line, Count);
}

VOID
StmEditDeReferenceFlow(
    _Inout_ PSTREAM_FLOW_CONTEXT FlowContext,
    _In_    char Module,
    _In_    UINT Line
    )
{
    LONG Count;
    KLOCK_QUEUE_HANDLE LockHandle;

    NT_ASSERT(FlowContext->RefCount > 0);

    Count = InterlockedDecrement((LONG *)&FlowContext->RefCount);
    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_REFCOUNT, "FlowCtx %p RefCount-- @ %c/%lu = %lu", FlowContext, Module, Line, Count);

    if (Count == 0)
    {
        NT_ASSERT( ! FlowContext->bFlowActive);

        // Remove the context from global context list
        //
        KeAcquireInStackQueuedSpinLock(&Globals.FlowContextListLock, &LockHandle);

        if (!FlowContext->bEntryRemoved) 
		{
            RemoveEntryList(&FlowContext->Link);
            FlowContext->bEntryRemoved = TRUE;
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, -- Link removed", FlowContext);
        }
        Count = --Globals.FlowContextCount;

        if (Globals.FlowContextCount == 0)
        {
            NT_ASSERT(IsListEmpty(&Globals.FlowContextList));
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Setting ZeroFlowCountEvent.");
            KeSetEvent(&Globals.ZeroFlowCountEvent, IO_NO_INCREMENT, FALSE);
        }
        KeReleaseInStackQueuedSpinLock(&LockHandle);

        if (!FlowContext->bEditInline) 
		{
            NT_ASSERT(IsListEmpty(&FlowContext->OobInfo.OutgoingDataQueue));
        }

        if (FlowContext->ScratchBuffer) 
		{
            ExFreePoolWithTag(FlowContext->ScratchBuffer, STMEDIT_TAG_FLAT_BUFFER);
        }

        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p is being freed., %lu remain @--", FlowContext, Count);
        ExFreePoolWithTag(FlowContext, STMEDIT_TAG_FLOWCTX);
    }
}


NTSTATUS
StreamEditNotifyFunction(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE NotifyType,
   _In_ const GUID* FilterKey,
   _In_ const FWPS_FILTER* Filter
   )
{
/*
    Notify Function.
*/
    UNREFERENCED_PARAMETER(FilterKey);

#if 0
   UNREFERENCED_PARAMETER(notifyType);
   UNREFERENCED_PARAMETER(filter);
#else
    NT_ASSERT(Filter != NULL);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
                "-><- %!FUNC! invoked with %I64u for Filter ID %I64u",
                       NotifyType, Filter->filterId);

#endif
   return STATUS_SUCCESS;
}

void
NTAPI
StreamEditInjectCompletionFn(
    _In_ VOID* Context,
    _Inout_ NET_BUFFER_LIST* NetBufferList,
    _In_ BOOLEAN DispatchLevel
    )
/*
    Injection completion function for injecting an NBL created using
    FwpsAllocateNetBufferAndNetBufferList.
*/
{
    MDL* mdl = (MDL*)Context;

    UNREFERENCED_PARAMETER(DispatchLevel);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "-><- %!FUNC!: NBL %p (%!STATUS!), MDL %p", NetBufferList, NetBufferList->Status, mdl);

    // Supress warning 28922: Redundant test against NULL. Pointer is already guaranteed to be non-NULL.
    // Rationale : mdl is not guaranteed to be non-NULL here.
#pragma prefast(push)
#pragma prefast(disable:28922)
 
    if (mdl != NULL) 
	{
        //
        // The MDL mapped over a pool alloc which we need to free here.
        //

        ExFreePoolWithTag(mdl->MappedSystemVa, STMEDIT_TAG_MDL_DATA);

        IoFreeMdl(mdl);
    }
#pragma prefast(pop)

    NT_ASSERT(NetBufferList != NULL);
    FwpsFreeNetBufferList(NetBufferList);
}

NTSTATUS
StreamEditRemoveFlowCtx(
    _In_ PSTREAM_FLOW_CONTEXT Context
    )
/*
    Function to disassociate a previously associated context from a data flow.
    This will cause flowDelete function to be invoked (either synchronously or asynchronously).

    Remarks @ http://msdn.microsoft.com/en-us/library/windows/hardware/ff551169.aspx
    
    If the FwpsFlowRemoveContext0 function returns STATUS_SUCCESS, FwpsFlowRemoveContext0
    calls the flowDeleteFn callout function synchronously.If FwpsFlowRemoveContext0 returns
    STATUS_PENDING, FwpsFlowRemoveContext0 calls flowDeleteFn asynchronously because an
    active callout classification is in progress.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    //
    // Possible synchronization problem for accessing bFlowActive...
    // while we are flushing the data, FlowDeleteFn can get invoked.
    //

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!: FlowCtx %p", Context);

    NT_ASSERT(Context);

    if (Context->bEditInline) 
	{
        (VOID) InlineEditFlushData(Context, 0, Context->PartialSFlags);
    }
    else {
        (VOID) StreamOobFlushOutgoingData(Context);
    }

    if (Context->bFlowActive) 
	{

        Status = FwpsFlowRemoveContext(
                        Context->FlowHandle,
                        Context->LayerId,
                        Context->CalloutId);
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: FlowCtx %p, %!STATUS!",  Context, Status);
    return Status;
}

VOID
StreamEditSignalShutdown(
)
/*
    This function attempts to Disassociate all active FlowContexts so that
    a shutdown can be performed.
*/
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PSTREAM_FLOW_CONTEXT FlowContext;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!");

    KeAcquireInStackQueuedSpinLock(&Globals.FlowContextListLock, &LockHandle);
    while ( ! IsListEmpty(&Globals.FlowContextList) )
    {
        PLIST_ENTRY Entry = RemoveHeadList(&Globals.FlowContextList);

        FlowContext = CONTAINING_RECORD(Entry, STREAM_FLOW_CONTEXT, Link);
        FlowContext->bEntryRemoved = TRUE;
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, -- Link removed at shutdown", FlowContext);

        if (FlowContext->bFlowActive)
        {
            KeReleaseInStackQueuedSpinLock(&LockHandle);
            (VOID)StreamEditRemoveFlowCtx(FlowContext);
            KeAcquireInStackQueuedSpinLock(&Globals.FlowContextListLock, &LockHandle);
        }
    }
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!");
}

VOID
StreamEditFlowDeleteFunction(
    _In_ UINT16 LayerId,
    _In_ UINT32 CalloutId,
    _In_ UINT64 Context
    )
/*
    This is the flowDeleteFn function. This callback is invoked when a flow is
    terminated or due to call to FwpsFlowRemoveContext0.

    We removes the FlowContext from the global FlowcCntextList and releases resources.

    IRQL <= DISPATCH_LEVEL
*/
{
    PSTREAM_FLOW_CONTEXT FlowCtx = (PSTREAM_FLOW_CONTEXT)(ULONG_PTR)Context;

    NT_ASSERT(NULL != FlowCtx);
    NT_ASSERT(TRUE == FlowCtx->bFlowActive);

    InterlockedExchange8(&FlowCtx->bFlowActive, FALSE);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!: FlowCtx %p, LayerId %hu, CalloutId %u, FlowId %I64u (RefCt = %lu)",
        FlowCtx, LayerId, CalloutId, FlowCtx->FlowHandle, FlowCtx->RefCount );

    // Deref the reference taken in Flow-established when the FlowCtx was allocated
    //
    StmEditDeReferenceFlow(FlowCtx, _MODULE_ID,  __LINE__);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: FlowCtx %p", FlowCtx);
}


VOID
NTAPI
StreamEditCommonStreamClassify(
    _In_ const FWPS_INCOMING_VALUES* InFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES* InMetaValues,
    _In_ PVOID LayerData,
#if(NTDDI_VERSION >= NTDDI_WIN7)
    _In_ const VOID* ClassifyContext,
#endif
    _In_ const FWPS_FILTER* Filter,
    _In_ UINT64 InFlowContext,
    _Inout_ FWPS_CLASSIFY_OUT* ClassifyOut
    )
/*
    Common classifyFn for both Inline and Out-of-band Stream layer callouts.
    Invokes corresponding classify-function based on FlowContext->bEditInline flag.
*/
{
    PSTREAM_FLOW_CONTEXT FlowContext = (PSTREAM_FLOW_CONTEXT)(ULONG_PTR)InFlowContext;

#if(NTDDI_VERSION >= NTDDI_WIN7)
    UNREFERENCED_PARAMETER(ClassifyContext);
#endif

    NT_ASSERT(FlowContext);

    // Reference the flow to keep around while we are in classifyFn
    //
    StmEditReferenceFlow(FlowContext, _MODULE_ID, __LINE__);

    if (FlowContext->bEditInline) 
    {
        InlineEditClassify(
            InFixedValues,
            InMetaValues,
            LayerData,
            Filter,
			InFlowContext,
            ClassifyOut
            );
    }
    else
    {
        OobEditClassify (
            InFixedValues,
            InMetaValues,
            LayerData,
            Filter,
			InFlowContext,
            ClassifyOut
            );
    }

    StmEditDeReferenceFlow(FlowContext, _MODULE_ID, __LINE__);
}

VOID 
StreamEditFlowEstablishedClassify(
    _In_ const FWPS_INCOMING_VALUES* InFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES* InMetaValues,
    _In_ PVOID Packet,
#if(NTDDI_VERSION >= NTDDI_WIN7)
    _In_ const void* ClassifyContext,
#endif  
    _In_ const FWPS_FILTER* Filter,
    _In_ UINT64 InFlowContext,
    _Inout_ FWPS_CLASSIFY_OUT* ClassifyOut
)
/*
    Flow-established call out for IPV4 and IPV6 traffic.
    Allocates and sets up a flow-context, and associate it with the flow.
*/
{
    NTSTATUS Status;
    PSTREAM_FLOW_CONTEXT StreamFlowContext;
    UINT32 StreamCalloutId;
    UINT16  StreamLayerId;
    KLOCK_QUEUE_HANDLE lockHandle;
    USHORT ipProtIndex;

    int CalloutSet = 0;

#if(NTDDI_VERSION >= NTDDI_WIN7)
    UNREFERENCED_PARAMETER(ClassifyContext);
#endif  
    UNREFERENCED_PARAMETER(InFlowContext);
    UNREFERENCED_PARAMETER(Packet);


    if ((Filter->action.calloutId == Globals.FlowEstablishedV4Callout1) ||
        (Filter->action.calloutId == Globals.FlowEstablishedV6Callout1)) 
	{
        CalloutSet = 1;
    }
    else
    if ((Filter->action.calloutId == Globals.FlowEstablishedV4Callout2) ||
        (Filter->action.calloutId == Globals.FlowEstablishedV6Callout2)) 
	{
        CalloutSet = 2;
        NT_ASSERT(TRUE == Globals.MultipleCallouts);
    }
    else
	{
        NT_ASSERT(FALSE);
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!%d: LayerId %hu, CalloutId %u, FlowId %I64u",
        CalloutSet, InFixedValues->layerId, Filter->action.calloutId, InMetaValues->flowHandle);

    ClassifyOut->actionType = FWP_ACTION_CONTINUE;

    // Lets not entertain any new flows if the driver is unloading!
    //
    if (Globals.DriverUnloading) 
	{
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,"<-- %!FUNC!: -- Driver unloading, flow not being associated with");
        return;
    }

    //
    // Setup the flow context for IPV4 Flows
    //
    if (FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4 == InFixedValues->layerId) 
	{
        ipProtIndex = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL;
        StreamLayerId = FWPS_LAYER_STREAM_V4;

        StreamCalloutId = CalloutSet == 1 ? Globals.StreamLayerV4Callout1 : Globals.StreamLayerV4Callout2;
    }
    //
    // Setup the flow context for IPV6 Flows
    //
    else if (FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6 == InFixedValues->layerId) 
	{
        ipProtIndex = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL;
        StreamLayerId = FWPS_LAYER_STREAM_V6;

        StreamCalloutId = CalloutSet == 1 ? Globals.StreamLayerV6Callout1 : Globals.StreamLayerV6Callout2;
    }
    else
    {
        // We should not be here.
        //
        NT_ASSERT(FALSE);
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: -- Invalid layer.");
        return;
    }

    //
    // Creates a flow context and associate it with the current flow
    // FlowContext gets deleted via flowDeleteFn
    //

    do
    {
        StreamFlowContext = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(STREAM_FLOW_CONTEXT), STMEDIT_TAG_FLOWCTX);

        if (StreamFlowContext == NULL) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Unable to allocate flow context");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Initialize the flow-context
        //

        StreamFlowContext->IpProto = InFixedValues->incomingValue[ipProtIndex].value.uint16;
        StreamFlowContext->bFlowActive = TRUE;

        StreamFlowContext->FlowHandle = InMetaValues->flowHandle;
        StreamFlowContext->LayerId = StreamLayerId;
        StreamFlowContext->CalloutId = StreamCalloutId;

        // Reference to take ownership!
        StmEditReferenceFlow(StreamFlowContext, _MODULE_ID, __LINE__);

        // Callout Set #1 is for Out of Band editing
        //
        if (CalloutSet == 1)
        {
            // Initialize OOB editing specific flow context structure fields
            // this includes, creating a worker thread to handle 

            KeInitializeSpinLock(&StreamFlowContext->OobInfo.EditLock);
            InitializeListHead(&StreamFlowContext->OobInfo.OutgoingDataQueue);

            StreamFlowContext->OobInfo.EditState = OOB_EDIT_IDLE;
            StreamFlowContext->OobInfo.QueueNumber = InterlockedIncrement((LONG *)&Globals.QueueIndex) % NUM_WORKITEM_QUEUES;

        }
        // Callout Set #2 is for InLine editing
        //
        else
        {
            // Initialize inline editing specific flow context structure areas
            //
            StreamFlowContext->InlineEditState = INLINE_EDIT_IDLE;
            StreamFlowContext->bEditInline = TRUE;

            StreamFlowContext->CurrentProcessor = INVALID_PROC_NUMBER;
        }

        // Add the newly created context on global context list
        KeAcquireInStackQueuedSpinLock(&Globals.FlowContextListLock, &lockHandle);
        InsertTailList(&Globals.FlowContextList, &StreamFlowContext->Link);
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, ++ Link inserted into global list", StreamFlowContext);

        ++Globals.FlowContextCount;
        if (Globals.FlowContextCount == 1) 
		{
            // Reset the shut-down event in case it was set due to no active flows
            //
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Clearing ZeroFlowCountEvent.");
            KeClearEvent(&Globals.ZeroFlowCountEvent);
        }
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p is allocated., Total %lu @++", StreamFlowContext, Globals.FlowContextCount);

        KeReleaseInStackQueuedSpinLock(&lockHandle);

        Status = FwpsFlowAssociateContext(
                        StreamFlowContext->FlowHandle,
                        StreamLayerId,
                        StreamCalloutId,
                        (UINT64)StreamFlowContext);

        //
        // If not able to associate a flow context, free the memory and return.
        //
        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "FlowContext association to FlowId %I64u failed with %!STATUS!", 
				InMetaValues->flowHandle, Status);
            break;
        }

    } while (FALSE);

    if (!NT_SUCCESS(Status) && StreamFlowContext) 
	{
        StmEditDeReferenceFlow(StreamFlowContext, _MODULE_ID, __LINE__);
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!%d: FlowCtx %p, cOut->Action %#x, %!STATUS!", 
		CalloutSet, StreamFlowContext, ClassifyOut->actionType, Status);
    return;
}

NTSTATUS
StreamEditRegisterFlowEstablishedCallouts(
    _In_  PVOID DeviceObject,
    _In_  const GUID* LayerKey,
    _In_  const GUID* CalloutKey,
    _In_  FWPM_DISPLAY_DATA* DisplayData,
    _Out_ UINT32* CalloutId,
    _In_ int CalloutNum
    )
/*
    This function registers callouts and filters that intercept TCP
    traffic at WFP FWPM_LAYER_STREAM_V4 or FWPM_LAYER_STREAM_V6 layer.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT condIndex = 0;
    BOOLEAN calloutRegistered = FALSE;

    FWPS_CALLOUT sCallout = { 0 };
    FWPM_CALLOUT mCallout = { 0 };

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!%d", CalloutNum);

    sCallout.calloutKey = *CalloutKey; // STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V4 / V6;
    sCallout.notifyFn = StreamEditNotifyFunction;

    sCallout.classifyFn = StreamEditFlowEstablishedClassify;

    Status = FwpsCalloutRegister(DeviceObject, &sCallout, CalloutId);

    if (NT_SUCCESS(Status))
    {
        calloutRegistered = TRUE;

        mCallout.calloutKey = *CalloutKey;
        mCallout.displayData = *DisplayData;
        mCallout.applicableLayer = *LayerKey; // FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 / V6

        Status = FwpmCalloutAdd(Globals.EngineHandle, &mCallout, NULL, NULL);

        if (NT_SUCCESS(Status))
        {
            FWPM_FILTER filter = { 0 };
            FWPM_FILTER_CONDITION filterConditions[4] = { 0 };

            // Add Filters for StreamEditFlowEstablishedClassify
            //

            filter.layerKey = *LayerKey;
            filter.displayData.name = L"Stream Edit Sample Filter";
            filter.displayData.description = L"Filter that finds and replaces a token from a TCP stream (@ Flow Established)";

            filter.action.type = FWP_ACTION_CALLOUT_INSPECTION; // FWP_ACTION_CALLOUT_TERMINATING;
            filter.action.calloutKey = *CalloutKey;
            filter.filterCondition = filterConditions;

            // In this sample, we edit TCP streams only
            //
            filterConditions[condIndex].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
            filterConditions[condIndex].matchType = FWP_MATCH_EQUAL;
            filterConditions[condIndex].conditionValue.type = FWP_UINT8;
            filterConditions[condIndex].conditionValue.uint8 = IPPROTO_TCP;
            filter.numFilterConditions++;
            condIndex++;

            // Filter according to the direction of the flow we are interested in
            //
            // @ http://msdn.microsoft.com/en-us/library/windows/desktop/aa364005.aspx
            //
            // For stream layers (FWPM_LAYER_STREAM_*) and  flow established layers
            // ( FWPM_LAYER_ALE_FLOW_ESTABLISHED_* ), the value will be the same as
            // direction of the connection.
            //
            // For example, when a local application initiates the connection, an
            // inbound packet has FWPM_CONDITION_DIRECTION set to FWP_DIRECTION_OUTBOUND.
            // 

            if (Globals.InspectionDirection != FWP_DIRECTION_MAX)
            {
                filterConditions[condIndex].fieldKey = FWPM_CONDITION_DIRECTION;
                filterConditions[condIndex].matchType = FWP_MATCH_EQUAL;
                filterConditions[condIndex].conditionValue.type = FWP_UINT32;
                filterConditions[condIndex].conditionValue.uint32 = Globals.InspectionDirection;
                filter.numFilterConditions++;
                condIndex++;
            }

            // Make sure that either the remote or the local port is specified...
            // i.e. both ports are not zero
            //

            if (Globals.InspectionLocalPort > 0)
            {
                filterConditions[condIndex].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
                filterConditions[condIndex].matchType = FWP_MATCH_EQUAL;
                filterConditions[condIndex].conditionValue.type = FWP_UINT16;
                filterConditions[condIndex].conditionValue.uint16 = Globals.InspectionLocalPort;
                filter.numFilterConditions++;
                condIndex++;
            }

            if (Globals.InspectionRemotePort > 0)
            {
                filterConditions[condIndex].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
                filterConditions[condIndex].matchType = FWP_MATCH_EQUAL;
                filterConditions[condIndex].conditionValue.type = FWP_UINT16;
                filterConditions[condIndex].conditionValue.uint16 = Globals.InspectionRemotePort;
                filter.numFilterConditions++;
            }

            filter.subLayerKey = CalloutNum == 1 ? STREAM_EDITOR_SUBLAYER_1 : STREAM_EDITOR_SUBLAYER_2;
            filter.weight.type = FWP_EMPTY;

            Status = FwpmFilterAdd(Globals.EngineHandle, &filter, NULL, NULL);

        } //FwpmCalloutAdd
    } //FwpsCalloutRegister

    if (!NT_SUCCESS(Status))
    {
        if (calloutRegistered) 
		{
            FwpsCalloutUnregisterById(*CalloutId);
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!, %!STATUS!", Status);
    return Status;
}

NTSTATUS
StreamEditRegisterStreamLayerCallouts(
    _In_  PVOID DeviceObject,
    _In_  const GUID* LayerKey,
    _In_  const GUID* CalloutKey,
    _In_  FWPM_DISPLAY_DATA* DisplayData,
    _Out_ UINT32* CalloutId,
    _In_  const int CalloutNum
    )
/*
    This function registers callouts that intercept TCP traffic
    at WFP FWPM_LAYER_STREAM_V4 or FWPM_LAYER_STREAM_V6 layer.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    FWPS_CALLOUT sCallout = {0};
    FWPM_CALLOUT mCallout = {0};
    BOOLEAN calloutRegistered = FALSE;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!%d", CalloutNum);

    sCallout.calloutKey     = *CalloutKey;
    sCallout.classifyFn     = StreamEditCommonStreamClassify;
    sCallout.notifyFn       = StreamEditNotifyFunction;
    sCallout.flowDeleteFn   = StreamEditFlowDeleteFunction;
    
    // http://msdn.microsoft.com/en-us/library/windows/hardware/ff551224.aspx
    //
    // FWPS_CALLOUT0 structure
    //
    // FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW
    // If this flag is specified, the filter engine calls the callout driver's
    // classifyFn callout function only if there is a context associated with
    // the data flow.
    //

    sCallout.flags = FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW;

    Status = FwpsCalloutRegister(
                        DeviceObject,
                        &sCallout,
                        CalloutId );

    if (NT_SUCCESS(Status))
    {
        calloutRegistered = TRUE;

        mCallout.calloutKey = *CalloutKey;
        mCallout.displayData = *DisplayData;
        mCallout.applicableLayer = *LayerKey; // FWPM_LAYER_STREAM_V4 / V6

        Status = FwpmCalloutAdd(
                    Globals.EngineHandle,
                    &mCallout,
                    NULL,
                    NULL );

        if (NT_SUCCESS(Status))
        {
            //
            //  Add Filters for Stream Classify
            //
            //  Note : we are adding a filter with no filter conditions -- i.e. this classifyFn callout
            //  will be classified for ALL streams/flows.
            //  However, due to FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW set above,
            //  the classifyFn will be classified only for flows that have a context associated.
            //

            FWPM_FILTER filter = { 0 };

            filter.layerKey = *LayerKey;
            filter.displayData.name = L"Stream Edit Sample Filter";
            filter.displayData.description = L"Filter that finds and replaces a token from a TCP stream (@ Stream Layer)";

            filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
            filter.action.calloutKey = *CalloutKey;
            filter.numFilterConditions = 0;
            filter.filterCondition = 0;

            filter.subLayerKey = CalloutNum == 1 ? STREAM_EDITOR_SUBLAYER_1 : STREAM_EDITOR_SUBLAYER_2;
            filter.weight.type = FWP_EMPTY; // auto-weight

            Status = FwpmFilterAdd(Globals.EngineHandle, &filter, NULL, NULL);
        }
    }

    if (!NT_SUCCESS(Status)) 
	{
        if (calloutRegistered)  
		{
            FwpsCalloutUnregisterById(*CalloutId);
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!, %!STATUS!", Status);
    return Status;
}


NTSTATUS
StreamEditRegisterCallouts(
_In_  PVOID DeviceObject
    )
/* 
    This function registers dynamic callouts and filters that intercept
    TCP traffic at WFP FWPM_LAYER_STREAM_V4 and FWPM_LAYER_STREAM_V6 
    layer.

    Callouts and filters will be removed during DriverUnload.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN EngineOpened = FALSE;
    BOOLEAN InTransaction = FALSE;

    FWPM_SESSION session = {0};


    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,"--> %!FUNC!");

    session.flags = FWPM_SESSION_FLAG_DYNAMIC;

    Status = FwpmEngineOpen(
                    NULL,
                    RPC_C_AUTHN_WINNT,
                    NULL,
                    &session,
                    &Globals.EngineHandle
                    );

    if (NT_SUCCESS(Status))
    {
        EngineOpened = TRUE;

        Status = FwpmTransactionBegin(Globals.EngineHandle, 0);
        if (NT_SUCCESS(Status))
        {
            FWPM_SUBLAYER0 StreamEditSubLayer = { 0 };
            FWPM_DISPLAY_DATA DisplayData;
            NTSTATUS StatusV6;

            InTransaction = TRUE;

            // Add SubLayer for Callout Set 1 (OoB V4/V6 callouts)
            // Register the first set of callouts at Flow-established V4 and V6 layers
            //
            StreamEditSubLayer.subLayerKey = STREAM_EDITOR_SUBLAYER_1;
            StreamEditSubLayer.displayData.name = L"Stream Edit Sample Sub-Layer 1";
            StreamEditSubLayer.displayData.description = L"Sub-Layer for use by Stream Edit Sample callouts";
            StreamEditSubLayer.weight = 0x40;

            Status = FwpmSubLayerAdd(Globals.EngineHandle, &StreamEditSubLayer, NULL);

            if (NT_SUCCESS(Status))
            {
                DisplayData.name = L"Stream Editor Sample ALE Flow Established V4 Callout #1";
                DisplayData.description = L"Flow Established V4 Callout to associate flow-contexts with flows";

                Status = StreamEditRegisterFlowEstablishedCallouts(
                                DeviceObject,
                                &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
                                &STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V4,
                                &DisplayData,
                                &Globals.FlowEstablishedV4Callout1,
                                1);

                DisplayData.name = L"Stream Editor Sample ALE Flow Established V6 Callout #1";
                DisplayData.description = L"Flow Established V6 Callout to associate flow-contexts with flows";

                StatusV6 = StreamEditRegisterFlowEstablishedCallouts(
                                DeviceObject,
                                &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
                                &STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V6,
                                &DisplayData,
                                &Globals.FlowEstablishedV6Callout1,
                                1);

                if (NT_SUCCESS(Status) || NT_SUCCESS(StatusV6))
                {
                    DisplayData.name = L"Stream Editor Sample Stream Layer V4 Callout #1";
                    DisplayData.description = L"Stream-Layer V4 Callout finds and replaces token(s) from a TCP stream";

                    Status = StreamEditRegisterStreamLayerCallouts(
                                DeviceObject,
                                &FWPM_LAYER_STREAM_V4,
                                &STREAM_EDITOR_STREAM_CALLOUT_V4,
                                &DisplayData,
                                &Globals.StreamLayerV4Callout1,
                                1);

                    DisplayData.name = L"Stream Editor Sample Stream Layer V6 Callout #1";
                    DisplayData.description = L"Stream-Layer V6 Callout finds and replaces token(s) from a TCP stream";

                    StatusV6 = StreamEditRegisterStreamLayerCallouts(
                                DeviceObject,
                                &FWPM_LAYER_STREAM_V6,
                                &STREAM_EDITOR_STREAM_CALLOUT_V6,
                                &DisplayData,
                                &Globals.StreamLayerV6Callout1,
                                1);

                    if (!(NT_SUCCESS(Status) || NT_SUCCESS(StatusV6))) 
					{
                        NT_ASSERT(FALSE);
                    }
                } // RegisterStreamLayerCallouts 
            }//FwpmSubLayerAdd




            if (Globals.MultipleCallouts)
            {
                // Add SubLayer for Callout Set 2 (OoB V4/V6 callouts)
                // Register the second set of callouts at Flow-established V4 and V6 layers
                //

                StreamEditSubLayer.subLayerKey = STREAM_EDITOR_SUBLAYER_2;
                StreamEditSubLayer.displayData.name = L"Stream Edit Sample Sub-Layer 2";
                StreamEditSubLayer.displayData.description = L"Sub-Layer for use by Stream Edit Sample callouts";
                StreamEditSubLayer.flags = 0;
                StreamEditSubLayer.weight = 0x20;

                Status = FwpmSubLayerAdd(Globals.EngineHandle, &StreamEditSubLayer, NULL);

                if (NT_SUCCESS(Status))
                {
                    // Register second set of callouts at Flow-established V4 and V6 layers
                    //
                    DisplayData.name = L"Stream Editor Sample ALE Flow Established V4 Callout #2";
                    DisplayData.description = L"Flow Established V4 Callout to associate flow-contexts with flows";

                    Status = StreamEditRegisterFlowEstablishedCallouts(
                        DeviceObject,
                        &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
                        &STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V4_2,
                        &DisplayData,
                        &Globals.FlowEstablishedV4Callout2,
                        2);

                    DisplayData.name = L"Stream Editor Sample ALE Flow Established V6 Callout #2";
                    DisplayData.description = L"Flow Established V6 Callout to associate flow-contexts with flows";

                    StatusV6 = StreamEditRegisterFlowEstablishedCallouts(
                        DeviceObject,
                        &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
                        &STREAM_EDITOR_FLOW_ESTABLISHED_CALLOUT_V6_2,
                        &DisplayData,
                        &Globals.FlowEstablishedV6Callout2,
                        2);

                    if (NT_SUCCESS(Status) || NT_SUCCESS(StatusV6))
                    {
                        DisplayData.name = L"Stream Editor Sample Stream Layer V4 Callout #2";
                        DisplayData.description = L"Stream-Layer V4 Callout finds and replaces token(s) from a TCP stream";

                        Status = StreamEditRegisterStreamLayerCallouts(
                            DeviceObject,
                            &FWPM_LAYER_STREAM_V4,
                            &STREAM_EDITOR_STREAM_CALLOUT_V4_2,
                            &DisplayData,
                            &Globals.StreamLayerV4Callout2,
                            2);

                        DisplayData.name = L"Stream Editor Sample Stream Layer V6 Callout #2";
                        DisplayData.description = L"Stream-Layer V6 Callout finds and replaces token(s) from a TCP stream";

                        StatusV6 = StreamEditRegisterStreamLayerCallouts(
                            DeviceObject,
                            &FWPM_LAYER_STREAM_V6,
                            &STREAM_EDITOR_STREAM_CALLOUT_V6_2,
                            &DisplayData,
                            &Globals.StreamLayerV6Callout2,
                            2);

                        if (!(NT_SUCCESS(Status) || NT_SUCCESS(StatusV6))) 
						{
                            NT_ASSERT(FALSE);
                        }
                    }
                }
            } // MultiCallout.

            Status = FwpmTransactionCommit(Globals.EngineHandle);

            if (NT_SUCCESS(Status)) 
			{
                InTransaction = FALSE;
            }
        } // FwpmTransactionBegin
    } // FwpmEngineOpen

    if (!NT_SUCCESS(Status))
    {
        if (InTransaction) 
		{
            NTSTATUS AbortStatus;
            AbortStatus = FwpmTransactionAbort(Globals.EngineHandle);
            _Analysis_assume_(NT_SUCCESS(AbortStatus));
        }

        if (EngineOpened) 
		{
            FwpmEngineClose(Globals.EngineHandle);
            Globals.EngineHandle = NULL;
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!, %!STATUS!", Status);
    return Status;
}

VOID
StreamEditUnregisterCallout(VOID)
{
    // Unregister the callouts for Callout #1
    //
    FwpsCalloutUnregisterById(Globals.FlowEstablishedV4Callout1);
    FwpsCalloutUnregisterById(Globals.StreamLayerV4Callout1);

    FwpsCalloutUnregisterById(Globals.FlowEstablishedV6Callout1);
    FwpsCalloutUnregisterById(Globals.StreamLayerV6Callout1);

    FwpmSubLayerDeleteByKey(Globals.EngineHandle, &STREAM_EDITOR_SUBLAYER_1);

    // Unregister the callouts for Callout #2
    //
    if (Globals.MultipleCallouts)
    {
        FwpsCalloutUnregisterById(Globals.FlowEstablishedV4Callout2);
        FwpsCalloutUnregisterById(Globals.StreamLayerV4Callout2);

        FwpsCalloutUnregisterById(Globals.FlowEstablishedV6Callout2);
        FwpsCalloutUnregisterById(Globals.StreamLayerV6Callout2);

        FwpmSubLayerDeleteByKey(Globals.EngineHandle, &STREAM_EDITOR_SUBLAYER_2);
    }

    NT_ASSERT(Globals.EngineHandle != NULL);
    FwpmEngineClose(Globals.EngineHandle);
    Globals.EngineHandle = NULL;
}

_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
StreamEditEvtDriverUnload(
   _In_ WDFDRIVER DriverObject
   )
{
    ULONG nCount;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!: (DrvObj %p)", DriverObject);
    InterlockedExchange8( &Globals.DriverUnloading, TRUE);

    StreamEditSignalShutdown();

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "DriverUnload -- Waiting for all the flows to terminate");
    KeWaitForSingleObject(&Globals.ZeroFlowCountEvent, Executive, KernelMode, FALSE, NULL);


    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "DriverUnload -- Calling FwpsInjectionHandleDestroy");
    // FwpsInjectionHandleDestroy will _not_ return to the
    // caller until all pending injections are completed.
    //
    if (Globals.InjectionHandle != NULL)
        FwpsInjectionHandleDestroy(Globals.InjectionHandle);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "DriverUnload -- Now, uninitializing LW Queues");
    for (nCount = 0; nCount < NUM_WORKITEM_QUEUES; ++nCount)
    {
        LwUninitializeQueue(&Globals.ProcessingQueues[nCount]);
    }

    if (Globals.LookasideCreated)
        ExDeleteLookasideListEx(&Globals.LookasideList);

    if (Globals.EngineHandle != NULL)
        StreamEditUnregisterCallout();

    if (Globals.NetBufferListPool != NULL)
        NdisFreeNetBufferListPool(Globals.NetBufferListPool);

    if (Globals.NdisGenericObj != NULL)
        NdisFreeGenericObject(Globals.NdisGenericObj);

    if (Globals.StringToReplaceMdl != NULL)
        IoFreeMdl(Globals.StringToReplaceMdl);

    if (Globals.StringXMdl != NULL)
        IoFreeMdl(Globals.StringXMdl);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!");
    WPP_CLEANUP(DriverObject);
}

VOID
StreamEditInitConfig(
    _In_ const WDFDRIVER driver
    )
/*
    This function loads the default StreamEditor configuration values,
    then overrides any values specified in the registry.
*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    DECLARE_CONST_UNICODE_STRING(stringToFindKey, L"StringToFind");
    DECLARE_CONST_UNICODE_STRING(stringInMiddleKey, L"StringX");
    DECLARE_CONST_UNICODE_STRING(stringToReplaceKey, L"StringToReplace");
    DECLARE_CONST_UNICODE_STRING(inspectionLocalPortKey, L"InspectionLocalPort");
    DECLARE_CONST_UNICODE_STRING(inspectionRemotePortKey, L"InspectionRemotePort");
    DECLARE_CONST_UNICODE_STRING(multiCalloutKey, L"MultipleCallouts");
    DECLARE_CONST_UNICODE_STRING(inspectionDirectionKey, L"InspectionDirection");
    DECLARE_CONST_UNICODE_STRING(thresholdKey, L"BusyThreshold");

    UNICODE_STRING stringValue;
    WCHAR buffer[STR_MAX_SIZE];
    USHORT requiredSize;
    ULONG valueSize;
    ULONG ulongValue;
    WDFKEY hKey;

    // Initialize with default values.
    // String lengths will be initialized later (below).
    //
    Globals.InspectionLocalPort = CFG_LOCAL_PORT;
    Globals.InspectionRemotePort = 0;
    Globals.InspectionDirection = FWP_DIRECTION_MAX; // Inbound + outbound
    Globals.BusyThreshold = 0x4000; // == 16K;
    Globals.MultipleCallouts = TRUE;

    Globals.StringToFind[0] = Globals.StringX[0] = Globals.StringToReplace[0] = '\0';

    Status = WdfDriverOpenParametersRegistryKey(driver, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &hKey);
	if (NT_SUCCESS(Status))
	{

		if (NT_SUCCESS(WdfRegistryQueryULong(hKey, &inspectionLocalPortKey, &ulongValue)))
		{
			Globals.InspectionLocalPort = (USHORT)ulongValue;
		}

		if (NT_SUCCESS(WdfRegistryQueryULong(hKey, &inspectionRemotePortKey, &ulongValue)))
		{
			Globals.InspectionRemotePort = (USHORT)ulongValue;
		}

		if (NT_SUCCESS(WdfRegistryQueryULong(hKey, &inspectionDirectionKey, &ulongValue)))
		{
			Globals.InspectionDirection = (UCHAR)ulongValue;
			NT_ASSERT((Globals.InspectionDirection >= 0 && Globals.InspectionDirection <= FWP_DIRECTION_MAX));

			if (Globals.InspectionDirection > FWP_DIRECTION_MAX)
				Globals.InspectionDirection = FWP_DIRECTION_MAX;
		}

		if (NT_SUCCESS(WdfRegistryQueryULong(hKey, &multiCalloutKey, &ulongValue)))
		{
			Globals.MultipleCallouts = !(ulongValue == 0);
		}


		// Attempt to read StringToFind value from registry
		//
		stringValue.Buffer = buffer;
		stringValue.Length = 0;
		stringValue.MaximumLength = sizeof(buffer);			

		Status = WdfRegistryQueryUnicodeString(hKey, &stringToFindKey, &requiredSize, &stringValue);
		if (NT_SUCCESS(Status))
		{
		    // stringValue is NULL terminated.

		    // Translate Unicode string
		    Status = RtlUnicodeToMultiByteN(
		                    Globals.StringToFind,
		                    sizeof(Globals.StringToFind),
		                    &valueSize,
		                    stringValue.Buffer,
		                    (ULONG)requiredSize);

		    if (NT_SUCCESS(Status)) 
			{
		        valueSize -= sizeof(char);
		        Globals.StringToFindLength = valueSize;
		        NT_ASSERT(Globals.StringToFind[valueSize] == '\0');
		    }
		}

		stringValue.MaximumLength = sizeof(buffer);
		//Attempt to read StringX value from registry
		// 
		Status = WdfRegistryQueryUnicodeString(hKey, &stringInMiddleKey, &requiredSize, &stringValue);
		if (NT_SUCCESS(Status))
		{
		    // Translate Unicode string
		    Status = RtlUnicodeToMultiByteN(
		                    Globals.StringX,
		                    sizeof(Globals.StringX),
		                    &valueSize,
		                    stringValue.Buffer,
		                    (ULONG)requiredSize);

		    if (NT_SUCCESS(Status)) 
			{
		        valueSize -= sizeof(char); // NULL terminator.
		        Globals.StringXLength = valueSize;
		        NT_ASSERT(Globals.StringX[valueSize] == '\0');
		    }
		}

		stringValue.MaximumLength = sizeof(buffer);
		// Attempt to read StringToReplace value from registry
		//
        Status = WdfRegistryQueryUnicodeString(hKey, &stringToReplaceKey, &requiredSize, &stringValue);
        if (NT_SUCCESS(Status))
        {
            // Translate Unicode string
            Status = RtlUnicodeToMultiByteN(
                            Globals.StringToReplace,
                            sizeof(Globals.StringToReplace),
                            &valueSize,
                            stringValue.Buffer,
                            (ULONG)requiredSize);

            if (NT_SUCCESS(Status)) 
			{
                valueSize -= sizeof(char);
                Globals.StringToReplaceLength = valueSize;
                NT_ASSERT(Globals.StringToReplace[valueSize] == '\0');
            }
        }

        if (NT_SUCCESS(WdfRegistryQueryULong(hKey, &thresholdKey, &ulongValue))) 
		{
            Globals.BusyThreshold = (size_t)ulongValue << 10;
            NT_ASSERT(Globals.BusyThreshold != 0);
        }

        WdfRegistryClose(hKey);
    }
    else
    {
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "WdfDriverOpenParametersRegistryKey failed with %!STATUS!", Status);
    }

    // Calculate the length of tokens to be found/replaced.
    //
	if (Globals.StringToFindLength == 0)
	{
		NT_ASSERT(Globals.StringToFind[0] == 0);

		RtlStringCchCopyA(Globals.StringToFind, STR_MAX_SIZE, "rainy");
		Status = RtlStringCchLengthA(Globals.StringToFind, STR_MAX_SIZE, &Globals.StringToFindLength);

		//Handle Error.
		NT_ASSERT(NT_SUCCESS(Status));
	}

    if (Globals.StringXLength == 0)
    {
        NT_ASSERT(Globals.StringX[0] == 0);

        RtlStringCchCopyA(Globals.StringX, STR_MAX_SIZE, "cloudy");
        Status = RtlStringCchLengthA(Globals.StringX, STR_MAX_SIZE, &Globals.StringXLength);
	
		//Handle Error.
		NT_ASSERT(NT_SUCCESS(Status));
	}

    if (Globals.StringToReplaceLength == 0)
    {
        NT_ASSERT(Globals.StringToReplace[0] == 0);

        RtlStringCchCopyA(Globals.StringToReplace, STR_MAX_SIZE, "sunny");
        Status = RtlStringCchLengthA(Globals.StringToReplace, STR_MAX_SIZE, &Globals.StringToReplaceLength);

		//Handle Error.
		NT_ASSERT(NT_SUCCESS(Status));
    }

    NT_ASSERT(Globals.StringToFindLength != 0);
    NT_ASSERT(Globals.StringXLength != 0);
    NT_ASSERT(Globals.StringToReplaceLength != 0);

    // In this sample, we want to make sure that at least one port (either local or remote) is non-zero.
    //
    if ((Globals.InspectionLocalPort == 0) && (Globals.InspectionRemotePort == 0)) 
	{
        Globals.InspectionLocalPort = CFG_LOCAL_PORT;
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,"StreamEdit Configuration\r"
                            "\tStringToFind: %s\r\tStringToReplace: %s\r\tStringX: %s\r"
                            "\tInspectionLocalPort: %hu\r\tInspectionRemotePort: %hu\r"
                            "\tInspectionDirection: %!FWP_DIRECTION!\r\tBusyThreshold: 0x%IX\r\tMultiple Callouts: %!bool!",
                            Globals.StringToFind,
                            Globals.StringToReplace,
                            Globals.StringX,
                            Globals.InspectionLocalPort,
                            Globals.InspectionRemotePort,
                            Globals.InspectionDirection,
                            Globals.BusyThreshold,
                            Globals.MultipleCallouts );

}

NTSTATUS
StreamEditInitDriverObjects(
   _Inout_ DRIVER_OBJECT* driverObject,
   _In_ const UNICODE_STRING* registryPath,
   _Out_ WDFDRIVER* pDriver,
   _Out_ WDFDEVICE* pDevice
   )
{
   NTSTATUS Status;
   WDF_DRIVER_CONFIG config;
   PWDFDEVICE_INIT pInit = NULL;

   WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

   config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
   config.EvtDriverUnload = StreamEditEvtDriverUnload;

   Status = WdfDriverCreate(
               driverObject,
               registryPath,
               WDF_NO_OBJECT_ATTRIBUTES,
               &config,
               pDriver
               );

   if (NT_SUCCESS(Status))
   {
       Status = STATUS_INSUFFICIENT_RESOURCES;
       pInit = WdfControlDeviceInitAllocate(*pDriver, &SDDL_DEVOBJ_KERNEL_ONLY);

       if (pInit)
       {
           WdfDeviceInitSetCharacteristics(pInit, FILE_AUTOGENERATED_DEVICE_NAME, TRUE);
           //WdfDeviceInitSetDeviceType(pInit, FILE_DEVICE_NETWORK);
		   WdfDeviceInitSetDeviceClass(pInit, &WFP_DRIVER_CLASS_GUID);
           WdfDeviceInitSetCharacteristics(pInit, FILE_DEVICE_SECURE_OPEN, TRUE);
           
		   Status = WdfDeviceCreate(&pInit, WDF_NO_OBJECT_ATTRIBUTES, pDevice);

           if (NT_SUCCESS(Status))
           {
               WdfControlFinishInitializing(*pDevice);
           }
       }
   }

   if (!NT_SUCCESS(Status))
   {
       if ( pInit)
			WdfDeviceInitFree(pInit);
   }

   return Status;
}

NTSTATUS
DriverEntry(
    _In_ DRIVER_OBJECT* DriverObject,
    _In_ UNICODE_STRING* RegistryPath
    )
{
   NTSTATUS Status;
   WDFDEVICE WdfDevice;
   WDFDRIVER WdfDriver;
   NET_BUFFER_LIST_POOL_PARAMETERS nblPoolParams = {0};

   WPP_INIT_TRACING(DriverObject, RegistryPath);

   DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,"--> %!FUNC!: DrvObj %p, Regpath %wZ",  DriverObject, RegistryPath);

   do {

       // Request NX Non-Paged Pool when available
       ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

        //
        // Initialize globals and Configuration structures.
        //

        RtlZeroMemory(&Globals, sizeof(Globals));
        Globals.QueueIndex = (ULONG)-1;

        InitializeListHead(&Globals.FlowContextList);
        KeInitializeSpinLock(&Globals.FlowContextListLock);

        // Initialize DriverUnload/Shutdown Event (to a signalled state)
        //
        KeInitializeEvent(&Globals.ZeroFlowCountEvent, NotificationEvent, TRUE);

        Status = StreamEditInitDriverObjects(
                        DriverObject,
                        RegistryPath,
                        &WdfDriver,
                        &WdfDevice);

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "StreamEditInitDriverObjects failed with 0x%X", Status);
            break;
        }

        Globals.WdmDevice = WdfDeviceWdmGetDeviceObject(WdfDevice);

        // Initialize and read driver configuration overrides.
        //
        StreamEditInitConfig(WdfDriver);

        Globals.StringToReplaceMdl = IoAllocateMdl(
                                            Globals.StringToReplace,
                                            (ULONG)Globals.StringToReplaceLength,
                                            FALSE,
                                            FALSE,
                                            NULL);
        
		if (Globals.StringToReplaceMdl == NULL) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Unable to allocate StringToReplace Mdl");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        MmBuildMdlForNonPagedPool(Globals.StringToReplaceMdl);

        Globals.StringXMdl = IoAllocateMdl(
                                            Globals.StringX,
                                            (ULONG)Globals.StringXLength,
                                            FALSE,
                                            FALSE,
                                            NULL);

        if (Globals.StringXMdl == NULL) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Unable to allocate Mdl#2");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        MmBuildMdlForNonPagedPool(Globals.StringXMdl);

        Globals.NdisGenericObj = NdisAllocateGenericObject(DriverObject, STMEDIT_TAG_NDIS_OBJ, 0);
        if (Globals.NdisGenericObj == NULL)
		{
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "NdisAllocateGenericObject failed.");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Allocate a NDIS/NBL Pool

        nblPoolParams.Header.Type       = NDIS_OBJECT_TYPE_DEFAULT;
        nblPoolParams.Header.Revision   = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        nblPoolParams.Header.Size       = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        nblPoolParams.fAllocateNetBuffer    = TRUE;
        nblPoolParams.DataSize          = 0;
        nblPoolParams.PoolTag           = STMEDIT_TAG_NBL_POOL;

        Globals.NetBufferListPool = NdisAllocateNetBufferListPool(
                                        Globals.NdisGenericObj,
                                        &nblPoolParams);

        if (Globals.NetBufferListPool == NULL)
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "NdisAllocateNetBufferListPool failed.");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = ExInitializeLookasideListEx(
                        &Globals.LookasideList,
                        NULL,
                        NULL,
                        NonPagedPool, // POOL_NX_OPTIN_AUTO ==> NonPagedPool := NonPagedPoolNx
                        0,
                        max(sizeof(TASK_ENTRY), sizeof(OUTGOING_STREAM_DATA)),
                        STMEDIT_TAG_TASK_ENTRY,
                        0);

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Task LookasideList Creation failed with %!STATUS!", Status);
            break;
        }

        Globals.LookasideCreated = TRUE;

        Status = StreamEditInitializeWorkitemPool();

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "InitializeWorkerPool failed with %!STATUS!", Status);
            break;
        }

       // Create WFP Injection handle
       //
       Status = FwpsInjectionHandleCreate(AF_UNSPEC, FWPS_INJECTION_TYPE_STREAM, &Globals.InjectionHandle);
       if (!NT_SUCCESS(Status)) 
	   {
           DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FwpsInjectionHandleCreate failed with %!STATUS!", Status);
           break;
       }

       //
       // Finally, register the sublayer(s) and callouts with WFP
       //
       Status = StreamEditRegisterCallouts(Globals.WdmDevice);
       if (!NT_SUCCESS(Status)) 
	   {
           DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "StreamEditRegisterCallouts failed with %!STATUS!", Status);
           break;
       }

   } while (FALSE);

   DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!, %!STATUS!", Status);

   if (!NT_SUCCESS(Status)) 
   {
	   StreamEditEvtDriverUnload(WdfDriver);
   }

   return Status;
}

BOOLEAN
StreamEditCopyDataForInspection(
_In_ STREAM_FLOW_CONTEXT *FlowContext,
_In_ const FWPS_STREAM_DATA* StreamData,
_In_ SIZE_T BytesToCopy
)
/*
   This function copies stream data described by the FWPS_STREAM_DATA
   structure into a flat buffer.

   Return : TRUE if able to copy stream-data to a flat buffer successfully, FALSE otherwise.

*/
{
    SIZE_T BytesCopied;
    size_t ExistingDataLength = FlowContext->ScratchDataLength;

    NT_ASSERT(BytesToCopy > 0);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "--> %!FUNC!: FlowCtx %p, streamData %p, copy %Iu of %Iu, old ScratchLength %Iu",
                    FlowContext,
					StreamData,
                    BytesToCopy,
					StreamData->dataLength,
                    ExistingDataLength);

    NT_ASSERT(FlowContext->ScratchDataOffset == 0);

    // If the existing scratch buffer is not sufficient to accommodate the new data
    // try to allocate a bigger buffer (so that we don't have to keep allocating
    // these for some time.
    //
    if (FlowContext->ScratchBufferSize - ExistingDataLength < BytesToCopy)
    {
        size_t NewBufferSize = BytesToCopy + ExistingDataLength;

        PVOID  NewBuffer = ExAllocatePool2(
                                    POOL_FLAG_NON_PAGED,
                                    (NewBufferSize + (NewBufferSize >> 1) ), // 1.5 times the needed size.
                                    STMEDIT_TAG_FLAT_BUFFER);

        if (NewBuffer == NULL)
		{

            // We are not able to allocate a much bigger buffer ... lets try an exact fit.
            //
            NewBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, NewBufferSize, STMEDIT_TAG_FLAT_BUFFER);
        }

        if (NewBuffer != NULL) 
		{

            // Move the existing contents of scratch buffer over to newly allocated buffer
            //
            if (ExistingDataLength > 0) 
			{

                NT_ASSERT(FlowContext->ScratchBuffer != NULL);
                RtlCopyMemory(NewBuffer, FlowContext->ScratchBuffer, ExistingDataLength);
            }
        }

        // Free the old scratch buffer...
        //
        if (FlowContext->ScratchBuffer) 
		{

            ExFreePoolWithTag(FlowContext->ScratchBuffer, STMEDIT_TAG_FLAT_BUFFER);

            FlowContext->ScratchBuffer = NULL;
            FlowContext->ScratchBufferSize = 0;
            FlowContext->ScratchDataLength = 0;
        }

        if (NewBuffer) 
		{

            FlowContext->ScratchBuffer = NewBuffer;
            FlowContext->ScratchBufferSize = NewBufferSize;
            FlowContext->ScratchDataLength = ExistingDataLength;
        }
        else 
		{

            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_ENTER_EXIT,
                    "<-- %!FUNC!: FlowCtx %p, Failed to allocate flat buffer for NBL %p",
                            FlowContext, StreamData->netBufferListChain);
            return FALSE;
        }
    }

    // Append the NBL chain data on to (any) existing data in scratch buffer
    //
    FwpsCopyStreamDataToBuffer(
				StreamData,
                (BYTE*)FlowContext->ScratchBuffer + FlowContext->ScratchDataLength,
                BytesToCopy,
                &BytesCopied
                );
    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
            "FlowCtx %p, FwpsCopyStreamDataToBuffer flattened %Iu of %Iu bytes",
                FlowContext, BytesCopied, StreamData->dataLength);

    NT_ASSERT(BytesCopied == BytesToCopy);
    FlowContext->ScratchDataLength += BytesCopied;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "<-- %!FUNC!: FlowCtx %p, new ScratchLength %Iu, return TRUE", FlowContext, FlowContext->ScratchDataLength);
    return TRUE;
}

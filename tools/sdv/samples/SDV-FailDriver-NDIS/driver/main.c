/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    main.c

Abstract:

    This is a sample driver that contains intentionally placed
    code defects in order to illustrate how Static Driver Verifier
    works. This driver is not functional and not intended as a 
    sample for real driver development projects.

Environment:

    Kernel mode

--*/

#include "mp.h"

NDIS_HANDLE     NdisMiniportDriverHandle = NULL;
NDIS_HANDLE     MiniportDriverContext = NULL;


NDIS_STATUS DriverEntry(
    _In_  PDRIVER_OBJECT   DriverObject,
    _In_  PUNICODE_STRING  RegistryPath
    )
/*++
Routine Description:

Arguments:

    DriverObject    -   pointer to the driver object
    RegistryPath    -   pointer to the driver registry path
     
Return Value:
    
    NDIS_STATUS - the value returned by NdisMRegisterMiniport 
    
--*/
{
    NDIS_STATUS                         Status;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS MPChar;
        
    //
    // Fill in the Miniport characteristics structure with the version numbers 
    // and the entry points for driver-supplied MiniportXxx 
    //
    NdisZeroMemory(&MPChar, sizeof(MPChar));

    MPChar.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS,
    MPChar.Header.Size = sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS);
    MPChar.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_1;

    MPChar.MajorNdisVersion             = MP_NDIS_MAJOR_VERSION;
    MPChar.MinorNdisVersion             = MP_NDIS_MINOR_VERSION;
    MPChar.MajorDriverVersion           = MP_NDIS_MAJOR_VERSION;
    MPChar.MinorDriverVersion           = MP_NDIS_MINOR_VERSION;

    MPChar.SetOptionsHandler            = MPSetOptions;
    
    MPChar.InitializeHandlerEx          = MPInitialize;
    MPChar.HaltHandlerEx                = MPHalt;

    MPChar.UnloadHandler                = MPUnload,
    
    MPChar.PauseHandler                 = MPPause;      
    MPChar.RestartHandler               = MPRestart;    
    MPChar.OidRequestHandler            = MPOidRequest;    
    MPChar.SendNetBufferListsHandler    = MPSendNetBufferLists;
    MPChar.ReturnNetBufferListsHandler  = MPReturnNetBufferLists;
    MPChar.CancelSendHandler            = MPCancelSendNetBufferLists;
    MPChar.DevicePnPEventNotifyHandler  = MPPnPEventNotify;
    MPChar.ShutdownHandlerEx            = MPShutdown;
    MPChar.CheckForHangHandlerEx        = MPCheckForHang;
    MPChar.ResetHandlerEx               = MPReset;
    MPChar.CancelOidRequestHandler      = MPCancelOidRequest;
    
    Status = NdisMRegisterMiniportDriver(DriverObject,
                                         RegistryPath,
                                         (PNDIS_HANDLE)MiniportDriverContext,
                                         &MPChar,
                                         &NdisMiniportDriverHandle);
    return Status;
}


NDIS_STATUS
MPSetOptions(
    _In_ NDIS_HANDLE  NdisMiniportDriverHandleLocal,
    _In_ NDIS_HANDLE  MiniportDriverContextLocal
    )
{
    UNREFERENCED_PARAMETER(NdisMiniportDriverHandleLocal);
    UNREFERENCED_PARAMETER(MiniportDriverContextLocal);
    
    //MINIPORT_SET_OPTIONS
    
    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS 
MPInitialize(
    _In_  NDIS_HANDLE                        MiniportAdapterHandle,
    _In_  NDIS_HANDLE                        MiniportDriverContextLocal,
    _In_  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
    )
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER     pAdapter = NULL;
    BOOLEAN         iterate  = FALSE;
    NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES   RegistrationAttributes;
    UNREFERENCED_PARAMETER(MiniportDriverContextLocal);
    UNREFERENCED_PARAMETER(MiniportInitParameters);
    //MINIPORT_INITIALIZE

    do
    {
        // ------------------------------------------------------------------------------------
        // SDV - Injected Defect - Rule: NdisAllocateMemoryWithTagPriority
        // Note: You should always tag the allocated memory with a value != 0, this will allow
        //       you to easily track the memory you allocated in the pool 
        // ------------------------------------------------------------------------------------ 
        pAdapter = (PMP_ADAPTER)NdisAllocateMemoryWithTagPriority(MiniportAdapterHandle,
                                                                  sizeof(MP_ADAPTER),
                                                                  0,
                                                                  LowPoolPriority
                                                                  );
        __sdv_save_adapter_context((PVOID*)&pAdapter);
        if (pAdapter == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pAdapter, sizeof(MP_ADAPTER));
        
        pAdapter->AdapterHandle = MiniportAdapterHandle;
        pAdapter->PauseState = NicPaused;
        //
        // setting registration attributes
        //
        RegistrationAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        RegistrationAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
        RegistrationAttributes.Header.Size = sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES);

        RegistrationAttributes.MiniportAdapterContext = (NDIS_HANDLE)pAdapter;
        RegistrationAttributes.AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_HARDWARE_DEVICE | 
                                                NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER;
        
        RegistrationAttributes.CheckForHangTimeInSeconds = 2;
        RegistrationAttributes.InterfaceType = NIC_INTERFACE_TYPE;

        Status = NdisMSetMiniportAttributes(MiniportAdapterHandle,
                                            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&RegistrationAttributes);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        NdisAllocateSpinLock(&pAdapter->Lock);

        // Allocate Adapter memory
        Status = NICAllocAdapterMemory(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS) 
        {
            break;
        }
        
        // ------------------------------------------------------------------------------------
        // SDV - Injected Defect - Rule: Init_RegisterSG
        // Note: Commenting the line below, the driver will lost knowledge about having 
        //        allocated Scatter\Gather memory for the driver 
        // ------------------------------------------------------------------------------------ 
        // pAdapter->AdapetrMemoryAllocated = TRUE;
        // ------------------------------------------------------------------------------------
        
        //
        // Register the interrupt
        //
        Status = MpRegisterInterrupt(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        pAdapter->InterruptRegistered = TRUE;

        //
        // Register a timer
        //
        Status = MpQueueTimer(pAdapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        pAdapter->TimerRegistered = TRUE;

        pAdapter->PNPState = NicStarted;
    }while(iterate); 

    if (pAdapter && (Status != NDIS_STATUS_SUCCESS))
    {
        //
        // Undo everything if it failed
        //
        MPHalt(pAdapter, NdisHaltDeviceInitializationFailed);
    }
    if(pAdapter!=NULL)
    {
        NdisFreeMemory(pAdapter,sizeof(MP_ADAPTER),0);
    }
    return Status;
}


NDIS_STATUS 
MPPause(
    _In_  NDIS_HANDLE                         MiniportAdapterContextLocal,    
    _In_  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
    )
{
    PMP_ADAPTER         pAdapter = (PMP_ADAPTER) MiniportAdapterContextLocal;
    NDIS_STATUS         Status = NDIS_STATUS_PENDING;
    UNREFERENCED_PARAMETER(MiniportPauseParameters);
	
    //MINIPORT_PAUSE
 
    NdisAcquireSpinLock(&pAdapter->Lock);
    pAdapter->PauseState = NicPausing;

    //
    // If there are not pending operations
    //
    if (pAdapter->RefCount == 0)
    {
        Status = NDIS_STATUS_SUCCESS;
        pAdapter->PauseState = NicPaused;            
    }
    // ------------------------------------------------------------------------------------
    // SDV - Injected Defect - Rule: MiniportPause_Return
    // Note: The MiniportPause callback should only return SUCCESS or PENDING, in this
    //       case it will return FAILURE 
    // ------------------------------------------------------------------------------------ 
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }
    // ------------------------------------------------------------------------------------
    
    NdisReleaseSpinLock(&pAdapter->Lock);

    return Status;
}
        
    
NDIS_STATUS 
MPRestart(
    _In_  NDIS_HANDLE                         MiniportAdapterContext,    
    _In_  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
    )
{
    PMP_ADAPTER                  pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
    UNREFERENCED_PARAMETER(MiniportRestartParameters);
    //MINIPORT_RESTART
    NdisAcquireSpinLock(&pAdapter->Lock);
    pAdapter->PauseState = NicRunning;
    NdisReleaseSpinLock(&pAdapter->Lock);    
    return NDIS_STATUS_SUCCESS;
}
 
  
BOOLEAN 
MPCheckForHang(
    _In_  NDIS_HANDLE     MiniportAdapterContext
    )
{
    PMP_ADAPTER         pAdapter = (PMP_ADAPTER) MiniportAdapterContext;
    BOOLEAN             NeedsReset = FALSE;

    //MINIPORT_CHECK_FOR_HANG

    NdisAcquireSpinLock(&pAdapter->Lock);
    if (pAdapter->LastRefCount <= pAdapter->RefCount)
    {
      
        //
        // We claim we need a reset
        //
        // ------------------------------------------------------------------------------------
        // SDV - Injected Defect - Rule: MiniportCheckForHang_Return
        // Note:  Setting NeedsReset to false int this branch, it will cause the MPCheckForHang 
        //        function to always retun false. This is a violation. 
        // ------------------------------------------------------------------------------------- 
        NeedsReset = FALSE;
        // -------------------------------------------------------------------------------------
    }
    pAdapter->LastRefCount = pAdapter->RefCount;
    NdisReleaseSpinLock(&pAdapter->Lock);    
    return NeedsReset;
}


VOID MPHalt(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  NDIS_HALT_ACTION        HaltAction
    )

/*++

Routine Description:
    
    MiniportHalt handler
    
Arguments:

    MiniportAdapterContext  Pointer to our adapter
    HaltAction              The reason adapter is being halted

Return Value:

    None
    
--*/
{
    PMP_ADAPTER     pAdapter = (PMP_ADAPTER) MiniportAdapterContext;
    UNREFERENCED_PARAMETER(HaltAction);
    // MINIPORT_HALT

    NdisAcquireSpinLock(&pAdapter->Lock);
    pAdapter->PNPState = NicHalted;
    NdisReleaseSpinLock(&pAdapter->Lock);    

    // ------------------------------------------------------------------------------------
    // SDV - Injected Defect - Rule: NdisStallExecution_Delay
    // Note:  NdisStallExecution should be called with a value <= 50ms. 
    //        If a driver must wait for an interval longer than 50 microseconds, 
    //        it should call NdisMSleep.  
    // ------------------------------------------------------------------------------------- 
    // .... Waiting for the HW to reset
    NdisStallExecution(100);
    // -------------------------------------------------------------------------------------

    //
    // Deregister interrupt as early as possible
    //

    if (pAdapter->InterruptRegistered)
    {
        NdisMDeregisterInterruptEx(pAdapter->InterruptHandle);
        pAdapter->InterruptRegistered = FALSE;
    }

    if (pAdapter->TimerRegistered)
    {
        NdisCancelTimerObject (pAdapter->TimerHandle);
        pAdapter->TimerRegistered = FALSE;
    }

    if (pAdapter->AdapetrMemoryAllocated) 
    {
        NdisMDeregisterScatterGatherDma(pAdapter->AdapterHandle);
        pAdapter->AdapetrMemoryAllocated = FALSE;
    }

    NdisFreeSpinLock(&(pAdapter->Lock));
    NdisFreeMemory(pAdapter, 0, 0);
}

NDIS_STATUS 
MPReset(
    _In_  NDIS_HANDLE     MiniportAdapterContext,
    OUT PBOOLEAN        AddressingReset
    )
{
    PMP_ADAPTER         pAdapter = (PMP_ADAPTER) MiniportAdapterContext;

    *AddressingReset = TRUE;

    //MINIPORT_RESET
    NdisAcquireSpinLock(&pAdapter->Lock);
    pAdapter->PNPState = NicInReset;
    pAdapter->RefCount = 0;
    pAdapter->PNPState = NicStarted;
    NdisReleaseSpinLock(&pAdapter->Lock);
    
    return NDIS_STATUS_SUCCESS;
}

VOID 
MPReturnNetBufferLists(
    _In_  NDIS_HANDLE         MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST    NetBufferLists,
    _In_  ULONG               ReturnFlags
    )
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(NetBufferLists);
    UNREFERENCED_PARAMETER(ReturnFlags);
    
    // ------------------------------------------------------------------------------------
    // SDV - Injected Defect - Rule: Flags_Irql.slic
    // Note: KeGetCurrentIrql should not be used in this case, I already have the current
    //       IRQL information in the ReturnFlags variable
    // ------------------------------------------------------------------------------------ 
    if ( KeGetCurrentIrql() < DISPATCH_LEVEL )
    {
        // Do something
    }
    // ------------------------------------------------------------------------------------
    
    //MINIPORT_RETURN_NET_BUFFER_LISTS
}


VOID 
MPSendNetBufferLists(
    _In_  NDIS_HANDLE         MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST    NetBufferList,
    _In_  NDIS_PORT_NUMBER    PortNumber,
    _In_  ULONG               SendFlags
    )
{
    PMP_ADAPTER         pAdapter = (PMP_ADAPTER) MiniportAdapterContext;
    PNET_BUFFER_LIST    CurrNetBufferList;
    PNET_BUFFER_LIST    NextNetBufferList;
    ULONG               SendCompleteFlags = 0;
    NDIS_HANDLE         NdisIoWorkItemHandle = NULL;
    UNREFERENCED_PARAMETER(PortNumber);
    //MINIPORT_SEND_NET_BUFFER_LISTS
    
    for (CurrNetBufferList = NetBufferList;
             CurrNetBufferList != NULL;
             CurrNetBufferList = NextNetBufferList)
    {
        NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrNetBufferList);
        NET_BUFFER_LIST_STATUS(CurrNetBufferList) = NDIS_STATUS_SUCCESS;
    }

    if (NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags))
    {
        NDIS_SET_SEND_COMPLETE_FLAG(SendCompleteFlags, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
    }
    NdisIoWorkItemHandle = NdisAllocateIoWorkItem(pAdapter->AdapterHandle);
    if(NdisIoWorkItemHandle!=NULL&&MPQueuedWorkItem!=NULL)
    {
        NdisQueueIoWorkItem(NdisIoWorkItemHandle,
                            MPQueuedWorkItem,
                            MiniportAdapterContext);
	NdisMSendNetBufferListsComplete(pAdapter->AdapterHandle,
                                        NetBufferList,
                                        SendCompleteFlags);
    }
}

VOID MPShutdown(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  NDIS_SHUTDOWN_ACTION    ShutdownAction
    )
{
    PMP_ADAPTER     pAdapter = (PMP_ADAPTER)MiniportAdapterContext;

    // MINIPORT_SHUTDOWN

    UNREFERENCED_PARAMETER(ShutdownAction);

    pAdapter->PNPState = NicShutdown;
}
 
BOOLEAN 
MPIsr(
    _In_  NDIS_HANDLE     MiniportInterruptContext,
    _Out_ PBOOLEAN        QueueMiniportInterruptDpcHandler,
    _Out_ PULONG          TargetProcessors
    )
 {
    PMP_ADAPTER  pAdapter = (PMP_ADAPTER)MiniportInterruptContext;
    BOOLEAN      InterruptRecognized = FALSE;
    UNREFERENCED_PARAMETER(TargetProcessors);
    UNREFERENCED_PARAMETER(QueueMiniportInterruptDpcHandler);

    //MINIPORT_ISR

    // ------------------------------------------------------------------------------------
    // SDV - Injected Defect - Rule: Irql_Synch_Function
    // Note: Callers of NdisAcquireSpinLock must run at IRQL <= DISPATCH_LEVEL, in this case 
    //       it's being called during MiniportIsr at DIRQL level.
    // ------------------------------------------------------------------------------------        
    NdisAcquireSpinLock(&pAdapter->Lock);
    // ------------------------------------------------------------------------------------
    if ((pAdapter->PNPState == NicStarted) || (pAdapter->PNPState == NicInReset))
    {
        // Yeah, I know I am accepting all interrupts. This will put the 
        // machine into unstable state, but this is just a test driver
        InterruptRecognized = TRUE;
    }
    NdisReleaseSpinLock(&pAdapter->Lock);
    *TargetProcessors=FALSE;
    *QueueMiniportInterruptDpcHandler=FALSE;
    return InterruptRecognized;
}



VOID 
MPCancelSendNetBufferLists(
    _In_  NDIS_HANDLE     MiniportAdapterContext,
    _In_  PVOID           CancelId
    )
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(CancelId);
    
    //MINIPORT_CANCEL_SEND

}

VOID 
MPPnPEventNotify(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent
    )
{
    PMP_ADAPTER     Adapter = (PMP_ADAPTER)MiniportAdapterContext;
    NDIS_DEVICE_PNP_EVENT   PnPEvent = NetDevicePnPEvent->DevicePnPEvent;
    PVOID                   InformationBuffer = NetDevicePnPEvent->InformationBuffer;
    ULONG                   InformationBufferLength = NetDevicePnPEvent->InformationBufferLength;

    //
    // Turn off the warings.
    //
    UNREFERENCED_PARAMETER(InformationBuffer);
    UNREFERENCED_PARAMETER(InformationBufferLength);
    UNREFERENCED_PARAMETER(Adapter);
    
    //MINIPORT_DEVICE_PNP_EVENT_NOTIFY


    switch (PnPEvent)
    {
        case NdisDevicePnPEventQueryRemoved:
            break;

        case NdisDevicePnPEventRemoved:
            break;       

        case NdisDevicePnPEventSurpriseRemoved:
            break;

        case NdisDevicePnPEventQueryStopped:
            break;

        case NdisDevicePnPEventStopped:
            break;      
            
        case NdisDevicePnPEventPowerProfileChanged:
            break;      
            
        default:
            break;         
    }
}


VOID 
MPUnload(
    _In_  PDRIVER_OBJECT  DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    //
    // Deregister Miniport driver
    //
    NdisMDeregisterMiniportDriver(NdisMiniportDriverHandle);
    //MINIPORT_UNLOAD
}


NDIS_STATUS 
MPOidRequest(
    _In_  NDIS_HANDLE        MiniportAdapterContext,
    _In_  PNDIS_OID_REQUEST  NdisRequest
    )
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(NdisRequest);

    //MINIPORT_OID_REQUEST

    return NDIS_STATUS_SUCCESS;
}

VOID
MPCancelOidRequest(
    _In_  NDIS_HANDLE            MiniportAdapterContext,
    _In_  PVOID                  RequestId
    )
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(RequestId);
    
    //MINIPORT_CANCEL_OID_REQUEST
}

NDIS_STATUS 
MpRegisterInterrupt(
    _In_ PMP_ADAPTER         pAdapter
    )
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS  Interrupt;

    //
    // the embeded NDIS interrupt structure is already zero'ed out
    // as part of the adapter structure
    //
    NdisZeroMemory(&Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS));
    
    Interrupt.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_INTERRUPT;
    Interrupt.Header.Revision = NDIS_MINIPORT_INTERRUPT_REVISION_1;
    Interrupt.Header.Size = sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS);

    Interrupt.InterruptHandler = MPIsr;
    Interrupt.InterruptDpcHandler = MPHandleInterrupt;
    Interrupt.DisableInterruptHandler = MpDisableInterrupt;
    Interrupt.EnableInterruptHandler = MpEnableInterrupt;

    Status = NdisMRegisterInterruptEx(pAdapter->AdapterHandle,
                                      pAdapter,
                                      &Interrupt,
                                      &pAdapter->InterruptHandle
                                      );

    return Status;
}

VOID MpEnableInterrupt(
    _In_ PVOID pAdapter
    )
{
    NDIS_SPIN_LOCK DummySpinLock;
    NdisAllocateSpinLock( &DummySpinLock );

    UNREFERENCED_PARAMETER(pAdapter);
    
    //MINIPORT_ENABLE_INTERRUPT


}

VOID MpDisableInterrupt(
    _In_ PVOID pAdapter
    )
{
    UNREFERENCED_PARAMETER(pAdapter);
    // MINIPORT_DISABLE_INTERRUPT;
}


NDIS_STATUS
MpQueueTimer(
    _In_ PMP_ADAPTER         pAdapter
    )
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    LARGE_INTEGER   liDueTime;
    NDIS_TIMER_CHARACTERISTICS                      Timer;               

    //         
    NdisZeroMemory(&Timer, sizeof(NDIS_TIMER_CHARACTERISTICS));
    
    Timer.Header.Type = NDIS_OBJECT_TYPE_TIMER_CHARACTERISTICS;
    Timer.Header.Revision = NDIS_TIMER_CHARACTERISTICS_REVISION_1;
    Timer.Header.Size = sizeof(NDIS_TIMER_CHARACTERISTICS);

    Timer.AllocationTag = NIC_TAG;
    Timer.TimerFunction = MpDemonstrationTimer;
    Timer.FunctionContext = pAdapter;

    //
    // Minimize init-time
    //
    Status = NdisAllocateTimerObject(pAdapter->AdapterHandle,
                                     &Timer,
                                     &pAdapter->TimerHandle);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        return Status;
    }

    liDueTime.QuadPart = NIC_LINK_DETECTION_DELAY;
    NdisSetTimerObject(pAdapter->TimerHandle, liDueTime, 0, NULL);

    return NDIS_STATUS_SUCCESS;
}

VOID 
MpDemonstrationTimer(
    _In_  PVOID	    SystemSpecific1,
    _In_  PVOID	    FunctionContext,
    _In_  PVOID	    SystemSpecific2, 
    _In_  PVOID	    SystemSpecific3
    )
{
    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(FunctionContext);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);
    
    //NDIS_TIMER_FUNCTION
}

VOID MPProcessSGList(
    _In_ PDEVICE_OBJECT  pDO,
    _In_ PVOID  Reserved,
    _In_ PSCATTER_GATHER_LIST  pSGL,
    _In_ PVOID  Context
    )
{
	
    NDIS_SPIN_LOCK DummySpinLock;
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(pSGL);
    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(pDO);
    NdisAllocateSpinLock( &DummySpinLock );

    //MINIPORT_PROCESS_SG_LIST
    
}


VOID
MPHandleInterrupt(
    _In_  PVOID  MiniportInterruptContext,
    _In_  PVOID        MiniportDpcContext,
    _In_  PVOID       NdisReserved1,
    _In_  PVOID       NdisReserved2
    )
{
    UNREFERENCED_PARAMETER(MiniportInterruptContext);
    UNREFERENCED_PARAMETER(MiniportDpcContext);
    UNREFERENCED_PARAMETER(NdisReserved1);
    UNREFERENCED_PARAMETER(NdisReserved2);
}

VOID
  MPAllocateComplete(
    _In_ NDIS_HANDLE  MiniportAdapterContext,
    _In_ PVOID  VirtualAddress,
    _In_ PNDIS_PHYSICAL_ADDRESS  PhysicalAddress,
    _In_ ULONG  Length,
    _In_ PVOID  Context
    )
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(VirtualAddress);
    UNREFERENCED_PARAMETER(PhysicalAddress);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Context);
    
    //MINIPORT_ALLOCATE_SHARED_MEM_COMPLETE
} 
    
NDIS_STATUS 
NICAllocAdapterMemory(
    _In_  PMP_ADAPTER     pAdapter
    )
{
    NDIS_STATUS     Status;
    NDIS_SG_DMA_DESCRIPTION     DmaDescription;

    NdisZeroMemory(&DmaDescription, sizeof(DmaDescription));
    
    DmaDescription.Header.Type = NDIS_OBJECT_TYPE_SG_DMA_DESCRIPTION;
    DmaDescription.Header.Revision = NDIS_SG_DMA_DESCRIPTION_REVISION_1;
    DmaDescription.Header.Size = sizeof(NDIS_SG_DMA_DESCRIPTION);
    DmaDescription.ProcessSGListHandler = MPProcessSGList;
    DmaDescription.SharedMemAllocateCompleteHandler = MPAllocateComplete;
    
    Status = NdisMRegisterScatterGatherDma(pAdapter->AdapterHandle,
                                           &DmaDescription,
                                           &pAdapter->NdisMiniportDmaHandle);
                
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisWriteErrorLogEntry(pAdapter->AdapterHandle,
                               NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                               1,
                               0); 
    }

    return Status;
}

VOID
MPQueuedWorkItem(
    _In_opt_ PVOID                    WorkItemContext,
    _In_ NDIS_HANDLE                  NdisIoWorkItemHandle
    )
{
    // NOP   
    UNREFERENCED_PARAMETER(WorkItemContext);
    //NDIS_IO_WORKITEM
    NdisFreeIoWorkItem((NDIS_HANDLE)NdisIoWorkItemHandle);
}
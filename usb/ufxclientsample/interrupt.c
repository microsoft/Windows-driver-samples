/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    interrupt.c

Abstract:

    Implements the WDF interrupt object's callbacks and functions to manage 
    the interrupts.

Environment:

    Kernel mode

--*/


#include "device.h"
#include "interrupt.h"
#include "interrupt.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InterruptCreate)
#endif

EVT_WDF_INTERRUPT_ISR DeviceInterrupt_EvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC DeviceInterrupt_EvtInterruptDpc;
EVT_WDF_INTERRUPT_ISR DeviceInterrupt_EvtAttachDetachInterruptIsr;

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
InterruptCreate (
    _In_ WDFDEVICE Device,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR  InterruptResourceRaw,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR  InterruptResourceTranslated
    )
/*++

Routine Description:

    Helper function to create device's WDFINTERRUPT object and any other 
    needed WDF resources that the interrupt needs for proper functioning.

    It assumes the first interrupt resource with which this is invoked is 
    the device interrupt, and the second is the attach/detach interrupt.

Arguments:

    Device - Wdf device object corresponding to the FDO

    InterruptResourceRaw -  Raw resource for the interrupt

    InterruptResourceTranslated - Translated resource for the interrupt

Return Value:

    Appropriate NTSTATUS value

--*/
{
    NTSTATUS Status;
    WDF_INTERRUPT_CONFIG InterruptConfig;
    PCONTROLLER_CONTEXT ControllerContext;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFINTERRUPT* InterruptToCreate;

    TraceEntry();

    PAGED_CODE();

    ControllerContext = DeviceGetControllerContext(Device);

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

    if (ControllerContext->DeviceInterrupt == NULL) {
        WDF_INTERRUPT_CONFIG_INIT(
            &InterruptConfig, 
            DeviceInterrupt_EvtInterruptIsr,
            DeviceInterrupt_EvtInterruptDpc);

        WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(
            &Attributes, 
            DEVICE_INTERRUPT_CONTEXT);

        InterruptToCreate = &ControllerContext->DeviceInterrupt;

    } else if (ControllerContext->AttachDetachInterrupt == NULL) {
        WDF_INTERRUPT_CONFIG_INIT(
            &InterruptConfig,
            DeviceInterrupt_EvtAttachDetachInterruptIsr,
            DeviceInterrupt_EvtInterruptDpc);
        InterruptConfig.CanWakeDevice = TRUE;
        InterruptConfig.PassiveHandling = TRUE;
        InterruptToCreate = &ControllerContext->AttachDetachInterrupt;

    } else {
        TraceWarning("Other interrupt resource [0X%X] is detected and is being"
                        " ignored", InterruptResourceRaw->u.Interrupt.Vector);
        Status = STATUS_SUCCESS;
        goto End;
    }
    
    InterruptConfig.InterruptRaw = InterruptResourceRaw;
    InterruptConfig.InterruptTranslated = InterruptResourceTranslated;

    Status = WdfInterruptCreate(
                 Device,
                 &InterruptConfig,
                 &Attributes,
                 InterruptToCreate);
    CHK_NT_MSG(Status, "Failed to create Device interrupt");

End:
    TraceExit();
    return Status;
}

BOOLEAN 
DeviceInterrupt_EvtInterruptIsr(
    _In_ WDFINTERRUPT Interrupt,
    _In_ ULONG MessageID
    )
/*++

Routine Description:

    'EvtInterruptIsr' handler for the device interrupt object.
    http://msdn.microsoft.com/en-us/library/windows/hardware/ff541735(v=vs.85).aspx

Arguments:

    Interrupt - Associated interrupt object.

    MessageID - Message IDs for MSI

Return Value:

    Appropriate NTSTATUS value

--*/
{
    BOOLEAN PendingEvents;

    TraceEntry();

    UNREFERENCED_PARAMETER(MessageID);

    //
    // #### TODO: Determine if controller has pending events. ####
    // 
    
    // Sample will assume there is always a pending event for illustration purposes.
    PendingEvents = TRUE;

    if (PendingEvents) {
        //    
        // Enqueue the DPC to handle the events.
        //
        WdfInterruptQueueDpcForIsr(Interrupt);
    }

    TraceExit();
    return TRUE;
}

VOID 
DeviceInterrupt_EvtInterruptDpc (
    _In_ WDFINTERRUPT Interrupt,
    _In_ WDFOBJECT AssociatedObject
    )
/*++

Routine Description:

    'EvtInterruptDpc' handler for the device interrupt object.
    http://msdn.microsoft.com/en-us/library/windows/hardware/ff541721(v=vs.85).aspx

Arguments:

    Interrupt - Associated interrupt object.

    AssociatedObject - FDO Object

--*/
{
    WDFDEVICE WdfDevice;
    PDEVICE_INTERRUPT_CONTEXT InterruptContext;
    PCONTROLLER_CONTEXT ControllerContext;
    BOOLEAN Attached;
    BOOLEAN GotAttachOrDetach;
    CONTROLLER_EVENT ControllerEvent;

    UNREFERENCED_PARAMETER(Interrupt);

    TraceEntry();

    WdfDevice = (WDFDEVICE) AssociatedObject;
    ControllerContext = DeviceGetControllerContext(WdfDevice);

    WdfSpinLockAcquire(ControllerContext->DpcLock);

    WdfInterruptAcquireLock(ControllerContext->DeviceInterrupt);
    Attached = ControllerContext->Attached;
    GotAttachOrDetach = ControllerContext->GotAttachOrDetach;
    ControllerContext->GotAttachOrDetach = FALSE;
    WdfInterruptReleaseLock(ControllerContext->DeviceInterrupt);

    //
    // Handle attach/detach events
    //
    if (GotAttachOrDetach) {
        if (Attached && ControllerContext->WasAttached) {
            //
            // We must have gotten at least one detach. Need to reset the state.
            //        
            ControllerContext->RemoteWakeupRequested = FALSE;
            ControllerContext->Suspended = FALSE;
            UfxDeviceNotifyDetach(ControllerContext->UfxDevice);
        }

        if (Attached) {
            ControllerContext->RemoteWakeupRequested = FALSE;
            ControllerContext->Suspended = FALSE;
            UfxDeviceNotifyAttach(ControllerContext->UfxDevice);
        }
    }

    ControllerContext->WasAttached = Attached;

    InterruptContext = DeviceInterruptGetContext(ControllerContext->DeviceInterrupt);

    //
    // #### TODO: Insert code to read and dispatch events from the controller ####
    // 

    // The sample will assume an endpoint event of EndpointEventTransferComplete
    ControllerEvent.Type = EventTypeEndpoint;
    ControllerEvent.u.EndpointEvent = EndpointEventTransferComplete;
    
    //
    // Handle events from the controller
    //
    switch (ControllerEvent.Type) {
    case EventTypeDevice:
        HandleDeviceEvent(WdfDevice,  ControllerEvent.u.DeviceEvent);
        break;

    case EventTypeEndpoint:
        HandleEndpointEvent(WdfDevice, ControllerEvent.u.EndpointEvent);
        break;
    }

    WdfSpinLockRelease(ControllerContext->DpcLock);

    TraceExit();
}

BOOLEAN 
DeviceInterrupt_EvtAttachDetachInterruptIsr (
    _In_ WDFINTERRUPT Interrupt,
    _In_ ULONG MessageID
    )
/*++

Routine Description:

    'EvtInterruptIsr' handler for the attach/detach interrupt object.
    http://msdn.microsoft.com/en-us/library/windows/hardware/ff541735(v=vs.85).aspx

Arguments:

    Interrupt - Associated interrupt object.

    MessageID - Message IDs for MSI

Return Value:

    BOOLEAN

--*/
{
    PCONTROLLER_CONTEXT ControllerContext;

    UNREFERENCED_PARAMETER(MessageID);

    TraceEntry();

    ControllerContext = DeviceGetControllerContext(WdfInterruptGetDevice(Interrupt));
    
    WdfInterruptAcquireLock(ControllerContext->DeviceInterrupt);

    //
    // This is an ActiveBoth interrupt used for attach/detach.  State is determined
    // by counting interrupts.  Previous state was attached, so this is a detach.
    //
    if (!ControllerContext->Attached) {
        ControllerContext->Attached = TRUE;
        ControllerContext->GotAttachOrDetach = TRUE;

        (void) WdfInterruptQueueDpcForIsr(Interrupt);
    } else  {
        ControllerContext->Attached = FALSE;
        ControllerContext->GotAttachOrDetach = TRUE;
        (void) WdfInterruptQueueDpcForIsr(Interrupt);
    }
        
    WdfInterruptReleaseLock(ControllerContext->DeviceInterrupt);
        
    TraceExit();
    return TRUE;
}

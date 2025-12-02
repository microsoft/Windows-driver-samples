/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fail_driver1.c

Abstract:

    This is a sample driver that contains intentionally placed
    code defects in order to illustrate how Static Driver Verifier
    works. This driver is not functional and not intended as a 
    sample for real driver development projects.

Environment:

    Kernel mode

--*/

#include "fail_driver1.h"

#define _DRIVER_NAME_ "fail_driver1"

#ifndef __cplusplus
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, DriverAddDevice)
#pragma alloc_text (PAGE, DispatchCreate)
#pragma alloc_text (PAGE, DispatchRead)
#pragma alloc_text (PAGE, DispatchPower)
#pragma alloc_text (PAGE, DispatchSystemControl)
#pragma alloc_text (PAGE, DispatchPnp)
#pragma alloc_text (PAGE, DriverUnload)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{

    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_READ]           = DispatchRead;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = DispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DispatchSystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = DispatchPnp;
    DriverObject->DriverExtension->AddDevice           = DriverAddDevice;
    DriverObject->DriverUnload                         = DriverUnload;

    return STATUS_SUCCESS;
}

NTSTATUS
DriverAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    PDEVICE_OBJECT device;
	PDEVICE_OBJECT TopOfStack;
    PDRIVER_DEVICE_EXTENSION extension ;
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);
    
    PAGED_CODE();
    
    // Injected CA defect for C28171
    PAGED_CODE();

    status = IoCreateDevice(DriverObject,                 
                            sizeof(DRIVER_DEVICE_EXTENSION), 
                            NULL,                   
                            FILE_DEVICE_DISK,  
                            0,                     
                            FALSE,                 
                            &device                
                            );
    if(status==STATUS_SUCCESS)
    {
  
       extension = (PDRIVER_DEVICE_EXTENSION)(device->DeviceExtension);

       TopOfStack = IoAttachDeviceToDeviceStack (
                                       device,
                                       PhysicalDeviceObject);
       if (NULL == TopOfStack) 
	   {
           IoDeleteDevice(device);
           return STATUS_DEVICE_REMOVED;
       }


       IoInitializeDpcRequest(device,DpcForIsrRoutine);

       device->Flags &= ~DO_DEVICE_INITIALIZING;


    }

   return status;
}

NTSTATUS
DispatchCreate (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{   
    KAFFINITY ProcessorMask;
    PDRIVER_DEVICE_EXTENSION extension ;
    
    PVOID *badPointer = NULL;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    PAGED_CODE();
    

    ExFreePool(badPointer);


    extension = (PDRIVER_DEVICE_EXTENSION)DeviceObject -> DeviceExtension;

    ProcessorMask   =  (KAFFINITY)1;
    
    IoConnectInterrupt( &extension->InterruptObject,
                         InterruptServiceRoutine,
                         extension,
                         NULL,
                         extension->ControllerVector,
                         PASSIVE_LEVEL,
                         PASSIVE_LEVEL,
                         LevelSensitive,
                         TRUE,
                         ProcessorMask,
                         TRUE );
	
    return STATUS_SUCCESS;
}

NTSTATUS
DispatchRead (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{  
    /*
       This defect is injected for the "SpinLock" rule.
    */
    KSPIN_LOCK  queueLock;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    PAGED_CODE();

    KeInitializeSpinLock(&queueLock);
     

    KeAcquireSpinLock(&queueLock, &oldIrql);
	
    return STATUS_SUCCESS;
}

NTSTATUS
DispatchPower (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    NTSTATUS status;
    PDRIVER_DEVICE_EXTENSION extension = (PDRIVER_DEVICE_EXTENSION)(DeviceObject->DeviceExtension); 
    PAGED_CODE();
    
    
    IoSetCompletionRoutine(Irp, CompletionRoutine, extension, TRUE, TRUE, TRUE);
    
    status = IoCallDriver(DeviceObject,Irp);
    return status;
}

NTSTATUS
DispatchSystemControl (
    _In_  PDEVICE_OBJECT  DeviceObject,
    _Inout_  PIRP            Irp
    )
{   
    /*
       This defect is injected for the "CancelSpinLock" rule.
    */
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    PAGED_CODE();
   
    IoAcquireCancelSpinLock(&oldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS
DispatchPnp (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{   
	
    /*
       This defect is injected for "LowerDriverReturn" rule.
    */
    NTSTATUS status = IoCallDriver(DeviceObject,Irp);
    PAGED_CODE();

    status = STATUS_SUCCESS;
    return status;
}

NTSTATUS
CompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID EventIn
    )
{
    
    PKEVENT Event = (PKEVENT)EventIn;
    KIRQL oldIrql;
    PDRIVER_DEVICE_EXTENSION extension = (PDRIVER_DEVICE_EXTENSION)(DeviceObject->DeviceExtension); 
    UNREFERENCED_PARAMETER(Irp);
    _Analysis_assume_(EventIn != NULL);
    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

    /*
       This defect is injected for IrqlKeSetEvent rule
    */ 
    KeSetEvent(Event, extension->Increment, TRUE);
    return STATUS_SUCCESS;
}

BOOLEAN
InterruptServiceRoutine (
    _In_      PKINTERRUPT Interrupt,
    _In_opt_  PVOID DeviceExtensionIn
    )
{     
    PDRIVER_DEVICE_EXTENSION DeviceExtension = (PDRIVER_DEVICE_EXTENSION)DeviceExtensionIn;
    PVOID Context = NULL;        
    _Analysis_assume_(DeviceExtension != NULL);
    UNREFERENCED_PARAMETER(Interrupt); 

    IoRequestDpc(DeviceExtension->DeviceObject, DeviceExtension->Irp, Context);
    return TRUE;
}

VOID
DpcForIsrRoutine(    
    _In_ PKDPC  Dpc,    
    _In_ struct _DEVICE_OBJECT  *DeviceObject,    
    _Inout_ struct _IRP  *Irp,    
    _In_opt_ PVOID  Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Dpc);
    /*
       This defect is injected for IrqlIoApcLte rule

    */
    IoGetInitialStack();
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();
     
    return;
}

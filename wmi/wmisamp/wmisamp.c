/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
    EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

Module Name:

    WmiSamp.c

Abstract:

    --

Environment:

    Kernel mode

--*/


#include "WmiSamp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, WmiSampEvtDeviceAdd)
#endif


//
// Define a tag for use by the memory allocation routines.
//
#define WMI_SAMPLE_TAG (ULONG)'SimW'


//
// Private methods.
//

NTSTATUS
PriStartNewPeriodicTimer(
    _In_ WDFDEVICE DeviceObject,
    _In_ ULONG TimeInMilliSeconds,
    _In_ PFN_WDF_TIMER CallbackFunction
    );


EVT_WDF_TIMER PriTimerCallback;


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG config;
 
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    //
    // Initialize the Driver Config structure.
    //
    WDF_DRIVER_CONFIG_INIT(&config,
                           WmiSampEvtDeviceAdd);

    //
    // Create the Framework Driver object.
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);
    return status;
}


NTSTATUS
WmiSampEvtDeviceAdd(
    WDFDRIVER DriverObject,
    PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;
    WDFDEVICE deviceObject;
    WDF_OBJECT_ATTRIBUTES wdfObjAttributes;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    ULONG i;
    ULONG oneMinute = 60*1000;
    EC1 Ec1 = {0};
    EC2 Ec2 = {0};

    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    //
    // Initialize all the properties specific to the device. Default values are
    // set for the ones that are not set explicitly here.
    //
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    //
    // Initialize attributes structure.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&wdfObjAttributes);

    //
    // Specify the context type for the WDF device object.
    //
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&wdfObjAttributes, WMI_SAMPLE_DEVICE_DATA);

    //
    // Specify the callback function for cleaning up the WDF device object specific
    // memory allocations.
    //
    wdfObjAttributes.EvtDestroyCallback = WmiSampDeviceEvtDestroyCallback;

    //
    // Create a Framework Device object.
    //
    status = WdfDeviceCreate(&DeviceInit, &wdfObjAttributes, &deviceObject);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampEvtDeviceAdd\n", status));
        return status;
    }

    //
    // Get the Device Object context and initialize the data blocks with specific
    // data.
    //
    wmiDeviceData = GetWmiSampleDeviceData(deviceObject);
    wmiDeviceData->Ec1Count = EC1_COUNT;
    wmiDeviceData->Ec2Count = EC2_COUNT;

    //
    // Create a wdf spin lock object to protect access to the EC1 data and parent
    // the object to the device object.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&wdfObjAttributes);
    wdfObjAttributes.ParentObject = deviceObject;

    status = WdfSpinLockCreate(&wdfObjAttributes, &wmiDeviceData->Ec1Lock);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampEvtDeviceAdd\n", status));
        return status;
    }

    //
    // Create a wdf spin lock object to protect access to the EC2 data and parent
    // the object to the device object.
    //
    status = WdfSpinLockCreate(&wdfObjAttributes, &wmiDeviceData->Ec2Lock);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampEvtDeviceAdd\n", status));
        return status;
    }

    for (i = 0; i < EC1_COUNT; i++) {
        WmiSampSetEc1(wmiDeviceData,
                      &Ec1,
                      EC1_SIZE,
                      i);
    }

    for (i = 0; i < EC2_COUNT; i++) {
        WmiSampSetEc2(wmiDeviceData,
                      &Ec2,
                      EC2_SIZE,
                      i);
    }

    //
    // Register the WMI providers and create provider instances for this Framework
    // device object.
    //
    status = WmiSampWmiRegistration(deviceObject);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampEvtDeviceAdd\n", status));
        return status;
    }

    //
    // Start a periodic timer to dynamically initialize/uninitialize WMI providers
    // for this device object.
    //
    status = PriStartNewPeriodicTimer(deviceObject, oneMinute, PriTimerCallback);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, WmiSampEvtDeviceAdd\n", status));
        return status;
    }

    return status;
}


VOID
WmiSampDeviceEvtDestroyCallback(
    WDFOBJECT DeviceObject
    )

/*++

Routine Description:

    This callback function is called by the framework when the reference count
    on the WDF device object is zero and the framework is ready to release the
    resouces held by this object. All object specific resource allocations that
    were NOT made by the framework must be released here to avoid a resource
    leak.

Arguments:

    DeviceObject - The WDF device object that is about to be destroyed by the
        framework.

Return Value:

    VOID.

--*/

{
    ULONG index;
    PWMI_SAMPLE_DEVICE_DATA wmiDeviceData;

    wmiDeviceData = GetWmiSampleDeviceData((WDFDEVICE)DeviceObject);

    //
    // Release the memory allocated for EC1. The wdfspinlock Ec1Lock memory
    // will be released automatically by the framework as part of the device
    // object cleanup.
    //
    for (index = 0; index < EC1_COUNT; index++) {
        if (wmiDeviceData->Ec1[index] != NULL) {
            ExFreePool(wmiDeviceData->Ec1[index]);
            wmiDeviceData->Ec1[index] = NULL;
        }
    }

    //
    // Release the memory allocated for EC2. The wdfspinlock Ec2Lock memory
    // will be released automatically by the framework as part of the device
    // object cleanup.
    //
    for (index = 0; index < EC2_COUNT; index++) {
        if (wmiDeviceData->Ec2[index] != NULL) {
            ExFreePool(wmiDeviceData->Ec2[index]);
            wmiDeviceData->Ec2[index] = NULL;
        }
    }

    return;
}


_Success_(return > 0)
ULONG
WmiSampGetEc1(
    _In_    PWMI_SAMPLE_DEVICE_DATA WmiDeviceData,
    _Out_ PVOID Buffer,
    _In_    ULONG Index
    )
{
    ULONG ec1Length = 0;
    if (Index >= EC1_COUNT) {
        return ec1Length;
    }

    //
    // Acquire the lock to protect access to the EC1 data since multiple
    // threads could be trying to access the common data concurrently.
    //
    WdfSpinLockAcquire(WmiDeviceData->Ec1Lock);

    RtlCopyMemory(Buffer,
                  WmiDeviceData->Ec1[Index],
                  WmiDeviceData->Ec1Length[Index]);

    ec1Length = WmiDeviceData->Ec1Length[Index];

    //
    // Release the lock.
    //
    WdfSpinLockRelease(WmiDeviceData->Ec1Lock);

    return ec1Length;
}


VOID
WmiSampSetEc1(
    _In_ PWMI_SAMPLE_DEVICE_DATA WmiDeviceData,
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG Index
    )
{
    PEC1 ec1;
    ULONG ec1Length = ALIGN_UP(Length, PVOID);
    PVOID oldBuffer = NULL;

    if (Index >= EC1_COUNT) {
        return;
    }

    ec1 = ExAllocatePoolZero(NonPagedPoolNx, ec1Length, WMI_SAMPLE_TAG);
    if (ec1 != NULL) {

        RtlCopyMemory(ec1, Buffer, Length);

        //
        // Acquire the lock to protect access to the EC1 data since multiple
        // threads could be trying to access the common data concurrently.
        //
        WdfSpinLockAcquire(WmiDeviceData->Ec1Lock);

        oldBuffer = WmiDeviceData->Ec1[Index];
        WmiDeviceData->Ec1[Index] = ec1;
        WmiDeviceData->Ec1Length[Index] = ec1Length;
        WmiDeviceData->Ec1ActualLength[Index] = Length;

        //
        // Release the lock.
        //
        WdfSpinLockRelease(WmiDeviceData->Ec1Lock);

        if (oldBuffer != NULL) {
            ExFreePool(oldBuffer);
        }
    }

    return;
}


_Success_(return > 0)
ULONG
WmiSampGetEc2(
    _In_    PWMI_SAMPLE_DEVICE_DATA WmiDeviceData,
    _Out_ PVOID Buffer,
    _In_    ULONG Index
    )
{
    ULONG ec2Length = 0;
    if (Index >= EC2_COUNT) {
        return ec2Length;
    }

    //
    // Acquire the lock to protect access to the EC2 data since multiple
    // threads could be trying to access the common data concurrently.
    //
    WdfSpinLockAcquire(WmiDeviceData->Ec2Lock);

    RtlCopyMemory(Buffer,
                  WmiDeviceData->Ec2[Index],
                  WmiDeviceData->Ec2Length[Index]);

    ec2Length = WmiDeviceData->Ec2Length[Index];

    //
    // Release the lock.
    //
    WdfSpinLockRelease(WmiDeviceData->Ec2Lock);

    return ec2Length;
}


VOID
WmiSampSetEc2(
    _In_ PWMI_SAMPLE_DEVICE_DATA WmiDeviceData,
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG Index
    )
{
    PEC2 ec2;
    ULONG ec2Length = ALIGN_UP(Length, PVOID);
    PVOID oldBuffer = NULL;

    if (Index >= EC2_COUNT) {
        return;
    }

    ec2 = ExAllocatePoolZero(NonPagedPoolNx, ec2Length, WMI_SAMPLE_TAG);
    if (ec2 != NULL) {

        RtlCopyMemory(ec2, Buffer, Length);

        //
        // Acquire the lock to protect access to the EC2 data since multiple
        // threads could be trying to access the common data concurrently.
        //
        WdfSpinLockAcquire(WmiDeviceData->Ec2Lock);

        oldBuffer = WmiDeviceData->Ec2[Index];
        WmiDeviceData->Ec2[Index] = ec2;
        WmiDeviceData->Ec2Length[Index] = ec2Length;
        WmiDeviceData->Ec2ActualLength[Index] = Length;

        //
        // Release the lock.
        //
        WdfSpinLockRelease(WmiDeviceData->Ec2Lock);

        if (oldBuffer != NULL) {
            ExFreePool(oldBuffer);
        }
    }

    return;
}


NTSTATUS
PriStartNewPeriodicTimer(
    _In_ WDFDEVICE DeviceObject,
    _In_ ULONG TimeInMilliSeconds,
    _In_ PFN_WDF_TIMER CallbackFunction
    )

/*++

Routine Description:

    This function creates a new Framework timer object and sets it to fire
    periodically at that specifed time interval.

Arguments:

    DeviceObject - The Framework device object for which the new timer is to be
        created. This device object will be the parent object of the new timer
        object.

    TimeInMilliSeconds - The intervals at which the timer should fire.

    CallbackFunction - The function that needs to get invoked each time the timer
        is fired.

Return Value:

    NT Status code.

--*/

{
    NTSTATUS status;
    WDFTIMER timerObject;
    WDF_TIMER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Configure the timer to periodically fire.
    //
    WDF_TIMER_CONFIG_INIT_PERIODIC(&config, CallbackFunction, TimeInMilliSeconds);
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    //
    // Set the parent object of the timer as the given device object. This causes
    // the timer object to get cleaned up (destroyed) whenever the device object
    // gets destroyed.
    //
    attributes.ParentObject = DeviceObject;

    //
    // Create and start a new Framework timer object.
    //
    status = WdfTimerCreate(&config, &attributes, &timerObject);
    if (!NT_SUCCESS(status)) {

        DebugPrint(("[WmiSamp] Status = 0x%08x, PriStartNewPeriodicTimer\n", status));
        return status;
    }

    WdfTimerStart(timerObject, WDF_REL_TIMEOUT_IN_MS(TimeInMilliSeconds));

    return status;
}


VOID
PriTimerCallback(
    _In_ WDFTIMER Timer
    )

/*++

Routine Description:

    This is the callback function that gets invoked whenever the timer fires.
    The Dynamic WMI registration function is called from this callback function.

Arguments:

    Timer - The Framework timer object that was fired.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    WDFDEVICE deviceObject;

    //
    // Get the parent object for this timer. While creating the timer object, the
    // device object was specified as the parent object for the timer.
    //
    deviceObject = WdfTimerGetParentObject(Timer);

    //
    // Perform the dynamic WMI registration.
    //
    status = WmiSampDynamicWmiRegistration(deviceObject);
    if (!NT_SUCCESS(status)) {
        DebugPrint(("[WmiSamp] Status = 0x%08x, PriTimerCallback\n", status));
    }

}

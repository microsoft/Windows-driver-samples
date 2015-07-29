/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    testdevice.c

Abstract:

    This module defines the device-specific notification handlers.

    The routines below demonstrate three different types of request processing
    supported by the PEP common library.
    1. Synchronous processing: The request is completed in the context of
         of the PEP notification. The handler returns
         PEP_NOTIFICATION_HANDLER_COMPLETE to the PEP common library to indicate
         all work is completed.

    2. Async processing: The request processing is deferred to a worker
         thread by returning PEP_NOTIFICATION_HANDLER_MORE_WORK from the sync
         handler. The common library will then invoke the async handler in the
         context of a worker thread (created by the library).

    3. Self-managed work: The request processing is deferred but a worker
         owned and managed by this module. This is suitable if the driver
         maintains its own queue for requests. Although the request is
         deferred, the sync handler must return
         PEP_NOTIFICATION_HANDLER_COMPLETE to the PEP common library to indicate
         no async processing is needed. Later when the work actually completes,
         it is completed back to PoFx by creating a PepCompleteSelfManagedWork()
         request.



Environment:

    Kernel mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include "pch.h"
#include "acpispecific.h"

#if defined(EVENT_TRACING)
#include "testdevice.tmh"
#endif

//
//----------------------------------------------------------------- Definitions
//

#define GET_STRING_LENGTH(_Str) ((ULONG)(strlen((_Str)) + 1) * sizeof(CHAR))

//
// Define the resource requirements for test devices. These resources
// are device-specific hence should be adjusted accordingly.
//

#define TST1_MEM_RES_RW 0
#define TST1_MEM_RES_MIN_ADDR 0x9fc00
#define TST1_MEM_RES_MAX_ADDR 0x9fe00
#define TST1_MEM_RES_ALIGNMENT 0x4
#define TST1_MEM_RES_MEMSIZE 0x10

#define TST1_IO_RES_DECODE 1
#define TST1_IO_RES_MIN_ADDR 0x0
#define TST1_IO_RES_MAX_ADDR 0x100
#define TST1_IO_RES_ALIGNMENT 0x4
#define TST1_IO_RES_PORT_LEN 1

#define TST1_INT_RES_EDGE_LVL Latched
#define TST1_INT_RES_INT_LVL InterruptActiveLow
#define TST1_INT_RES_SHARE_TYPE FALSE
#define TST1_INT_RES_WAKE FALSE

#define TST2_GPIOINT_RES_INT_TYPE Latched
#define TST2_GPIOINT_RES_INT_LVL InterruptActiveLow
#define TST2_GPIOINT_RES_PIN_CONFIG PullDefault
#define TST2_GPIOINT_RES_DEBOUNCE 0

#define TST2_SPB_RES_SLAVE_ADDR 0x0
#define TST2_SPB_RES_CONN_SPD 100

//
//--------------------------------------------------------------------- Types
//

typedef struct _TST3_CID_REQUEST_CONTEXT {
    PEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    BOOLEAN Synchronous;
} TST3_CID_REQUEST_CONTEXT, *PTST3_CID_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TST3_CID_REQUEST_CONTEXT, Tst3GetCidRequestContext)

//
//--------------------------------------------------------------------- Globals
//

USHORT Tst1GpioPinTable[] = {1};
ULONG Tst1IntPinTable[] = {0xa};
USHORT Tst2GpioIntPinTable[] = {2};

//
// The UNICODE string for GPIO resource name.
//

UNICODE_STRING GpioResourceName = {
    sizeof(SAMPLE_GPIO_ACPI_NAME_WCHAR) - sizeof(WCHAR),
    sizeof(SAMPLE_GPIO_ACPI_NAME_WCHAR),
    SAMPLE_GPIO_ACPI_NAME_WCHAR
};

UNICODE_STRING SpbResourceName = {
    sizeof(SAMPLE_SPBD_ACPI_NAME_WCHAR) - sizeof(WCHAR),
    sizeof(SAMPLE_SPBD_ACPI_NAME_WCHAR),
    SAMPLE_SPBD_ACPI_NAME_WCHAR
};

//
// Define the state of the power rail device, where 1 means on, 0 means off.
//

ULONG PowerRailState = 0;

//
//------------------------------------------------------------------ Prototypes
//

VOID
ReportNeedMoreWork (
    _Out_ PNTSTATUS Status,
    _In_opt_ PCHAR MethodName,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    );

VOID
ReportNotSupported (
    _Out_ PNTSTATUS Status,
    _Out_ PULONG Count,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    );

VOID
ReportNeedMoreSelfManagedWork (
    _Out_ PNTSTATUS Status,
    _In_opt_ PCHAR MethodName,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    );

VOID
HandleTst3DsmByUuid (
    _In_ ULONG UuidIndex,
    _In_ ULONG RevisionLevel,
    _In_ ULONG FunctionIndex,
    _In_ PVOID Parameters,
    _In_ ULONG ParametersSize,
    _Out_ PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    );

NTSTATUS
Tst3CreateSelfManagedCidRequest (
    _In_ PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer
    );

VOID
Tst3SelfManagedCidRequestWorkerWrapper (
    __in WDFWORKITEM WorkItem
    );

VOID
Tst3ProcessSelfManagedCidRequest (
    _In_ PTST3_CID_REQUEST_CONTEXT RequestContext
    );

//
//------------------------------------------------------------------- Functions
//

//
// Notification handlers for \_SB.
//

PEP_NOTIFICATION_HANDLER_RESULT
RootSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the bus device.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

//
// Notification handlers for \_SB.BUSD.
//

PEP_NOTIFICATION_HANDLER_RESULT
BusdSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the bus device. It will try to evaluate the
    _HID method synchronously, while leaving _CID to the asynchronous
    method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_HID:
        PepReturnAcpiData(BUSD_HID,
                          ACPI_METHOD_ARGUMENT_STRING,
                          GET_STRING_LENGTH(BUSD_HID),
                          FALSE,
                          EcmBuffer->OutputArguments,
                          &EcmBuffer->OutputArgumentSize,
                          &EcmBuffer->OutputArgumentCount,
                          &EcmBuffer->MethodStatus,
                          "HID",
                          "BUSD",
                          &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_CID:
        ReportNeedMoreWork(&EcmBuffer->MethodStatus,
                           "CID",
                           "BUSD",
                           &CompleteStatus);

        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

PEP_NOTIFICATION_HANDLER_RESULT
BusdWorkerCallbackEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the bus device. It will be scheduled to run
    asychronously. In this example, it only handles _CID method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Supplies a pointer to the PEP_WORK structure used to
        report result to PoFx.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;

    TraceEvents(INFO,
                DBG_PEP,
                "%s <BUSD>: Asychronous method scheduled to run.\n",
                __FUNCTION__);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EcmBuffer->DeviceHandle;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    PoFxWorkInfo->WorkType = PepWorkAcpiEvaluateControlMethodComplete;
    PoFxWorkInfo->ControlMethodComplete.DeviceHandle =
        PepInternalDevice->KernelHandle;

    PoFxWorkInfo->ControlMethodComplete.CompletionFlags = 0;
    PoFxWorkInfo->ControlMethodComplete.CompletionContext =
        EcmBuffer->CompletionContext;

    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_CID:
        PepReturnAcpiData(
            BUSD_CID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(BUSD_CID),
            FALSE,
            PoFxWorkInfo->ControlMethodComplete.OutputArguments,
            &PoFxWorkInfo->ControlMethodComplete.OutputArgumentSize,
            NULL,
            &PoFxWorkInfo->ControlMethodComplete.MethodStatus,
            "CID",
            "BUSD",
            &CompleteStatus);

        break;

    default:
        ReportNotSupported(&PoFxWorkInfo->ControlMethodComplete.MethodStatus,
                           (PULONG)(&PoFxWorkInfo->
                               ControlMethodComplete.OutputArgumentSize),
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

//
// Notification handlers for \_SB.GPIO.
//

PEP_NOTIFICATION_HANDLER_RESULT
GpioSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the GPIO controller. It will try to evaluate the
    _HID method synchronously.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_HID:
        PepReturnAcpiData(
            GPIO_HID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(GPIO_HID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "HID",
            "GPIO",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_CID:
        PepReturnAcpiData(
            GPIO_SPBD_CID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(GPIO_SPBD_CID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "CID",
            "GPIO",
            &CompleteStatus);

        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

//
// Notification handlers for \_SB.SPBD.
//

PEP_NOTIFICATION_HANDLER_RESULT
SpbdSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the SPBD controller. It will try to evaluate the
    _HID method synchronously.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_HID:
        PepReturnAcpiData(
            SPBD_HID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(SPBD_HID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "HID",
            "SPBD",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_CID:
        PepReturnAcpiData(
            GPIO_SPBD_CID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(GPIO_SPBD_CID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "CID",
            "SPBD",
            &CompleteStatus);

        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

//
// Notification handlers for \_SB.BUSD.TST1.
//

PEP_NOTIFICATION_HANDLER_RESULT
Tst1SyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the test device 1. It will try to evaluate the
    _ADR and _DEP method synchronously, while leaving _CRS to the
    asynchronous method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    ULONG DeviceAdr;
    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    DeviceAdr = TST1_ADR;
    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_ADR:
        PepReturnAcpiData(
            &DeviceAdr,
            ACPI_METHOD_ARGUMENT_INTEGER,
            (ULONG)(sizeof(ULONG)),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "ADR",
            "TST1",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_DEP:

        //
        // Test device 1 has a dependency on the GPIO controller.
        //

        PepReturnAcpiData(
            SAMPLE_GPIO_ACPI_NAME_ANSI,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(SAMPLE_GPIO_ACPI_NAME_ANSI),
            TRUE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "DEP",
            "TST1",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_CRS:
        ReportNeedMoreWork(&EcmBuffer->MethodStatus,
                           "CRS",
                           "TST1",
                           &CompleteStatus);
        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

PEP_NOTIFICATION_HANDLER_RESULT
Tst1WorkerCallbackEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the test device 1. It will be scheduled to run
    asychronously. In this example, it only handles _CRS method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Supplies a pointer to the PEP_WORK structure used to
        report result to PoFx.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;

#if !defined(ARM) && !defined(ARM64)

    PEP_ACPI_RESOURCE Resources[3];
    NT_ASSERT(sizeof(Resources) == 3*sizeof(PEP_ACPI_RESOURCE));

#else

    PEP_ACPI_RESOURCE Resources[2];

#endif

    SIZE_T RequiredSize;
    NTSTATUS Status;

    TraceEvents(INFO,
                DBG_PEP,
                "%s <TST1>: Asychronous method scheduled to run.\n",
                __FUNCTION__);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EcmBuffer->DeviceHandle;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    PoFxWorkInfo->WorkType = PepWorkAcpiEvaluateControlMethodComplete;
    PoFxWorkInfo->ControlMethodComplete.DeviceHandle =
        PepInternalDevice->KernelHandle;

    PoFxWorkInfo->ControlMethodComplete.CompletionFlags = 0;
    PoFxWorkInfo->ControlMethodComplete.CompletionContext =
        EcmBuffer->CompletionContext;

    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_CRS:

        //
        // Test device 1 requires memory resource.
        //

        PEP_ACPI_INITIALIZE_MEMORY_RESOURCE(
            TST1_MEM_RES_RW,
            TST1_MEM_RES_MIN_ADDR,
            TST1_MEM_RES_MAX_ADDR,
            TST1_MEM_RES_ALIGNMENT,
            TST1_MEM_RES_MEMSIZE,
            &Resources[0]);

        //
        // Test device 1 needs INT resource.
        //

        PEP_ACPI_INITIALIZE_INTERRUPT_RESOURCE(
            FALSE,
            TST1_INT_RES_EDGE_LVL,
            TST1_INT_RES_INT_LVL,
            TST1_INT_RES_SHARE_TYPE,
            TST1_INT_RES_WAKE,
            Tst1IntPinTable,
            (UCHAR)(ARRAYSIZE(Tst1IntPinTable)),
            &Resources[1]);

#if !defined(ARM) || !defined(ARM64)

        //
        // Test device 1 needs I/O port resource.
        //

        PEP_ACPI_INITIALIZE_IOPORT_RESOURCE(
            TST1_IO_RES_DECODE,
            TST1_IO_RES_MIN_ADDR,
            TST1_IO_RES_MAX_ADDR,
            TST1_IO_RES_ALIGNMENT,
            TST1_IO_RES_PORT_LEN,
            &Resources[2]);

#endif

        //
        // Find the length required to hold the BIOS resources.
        //

        Status = PepGetBiosResourceSize(Resources,
                                        sizeof(Resources),
                                        "TST1",
                                        &RequiredSize);

        if(!NT_SUCCESS(Status)) {
            PoFxWorkInfo->ControlMethodComplete.MethodStatus = Status;
            PoFxWorkInfo->ControlMethodComplete.OutputArgumentSize = 0;
            goto Tst1WorkerCallbackEvaluateControlMethodEnd;
        }

        PepReturnBiosResource(
            Resources,
            sizeof(Resources),
            RequiredSize,
            PoFxWorkInfo->ControlMethodComplete.OutputArguments,
            &PoFxWorkInfo->ControlMethodComplete.OutputArgumentSize,
            "TST1",
            &PoFxWorkInfo->ControlMethodComplete.MethodStatus);

        break;

    default:
        ReportNotSupported(
            &PoFxWorkInfo->ControlMethodComplete.MethodStatus,
            (PULONG)(&PoFxWorkInfo->ControlMethodComplete.OutputArgumentSize),
            &CompleteStatus);
    }

    //
    // Return complete status.
    //

Tst1WorkerCallbackEvaluateControlMethodEnd:
    return CompleteStatus;
}

PEP_NOTIFICATION_HANDLER_RESULT
Tst1SyncQueryControlResources (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES
    notification for the test device 1. In this example, control resource
    needed for the firmware is GPIO I/O.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    PEP_ACPI_RESOURCE GpioIoResource;
    PPEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES QueryResourcesBuffer;
    SIZE_T RequiredSize;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    TraceEvents(INFO,
                DBG_PEP,
                "%s <TST1>: %s Start processing.\n",
                __FUNCTION__,
                PepAcpiNotificationHandlers[
                    PEP_NOTIFY_ACPI_QUERY_DEVICE_CONTROL_RESOURCES].Name);

    QueryResourcesBuffer = (PPEP_ACPI_QUERY_DEVICE_CONTROL_RESOURCES)Data;

    //
    // Initialize the GPIO IO resource that test device 1 needs.
    //

    PEP_ACPI_INITIALIZE_GPIO_IO_RESOURCE(
        FALSE,
        FALSE,
        PullDefault,
        0,
        0,
        IoRestrictionNone,
        0,
        &GpioResourceName,
        FALSE,
        NULL,
        0,
        (PUSHORT)Tst1GpioPinTable,
        (USHORT)(ARRAYSIZE(Tst1GpioPinTable)),
        &GpioIoResource);

    //
    // Find the length required to hold the BIOS resources.
    //

    Status = PepGetBiosResourceSize(&GpioIoResource,
                                 sizeof(PEP_ACPI_RESOURCE),
                                 "TST1",
                                 &RequiredSize);

    if(!NT_SUCCESS(Status)) {
        QueryResourcesBuffer->Status = Status;
        goto QueryControlResourcesEnd;
    }

    PepReturnBiosResource(&GpioIoResource,
                       sizeof(PEP_ACPI_RESOURCE),
                       RequiredSize,
                       QueryResourcesBuffer->BiosResources,
                       &QueryResourcesBuffer->BiosResourcesSize,
                       "TST1",
                       &QueryResourcesBuffer->Status);

QueryControlResourcesEnd:
    return PEP_NOTIFICATION_HANDLER_COMPLETE;
}

//
// Notification handlers for \_SB.BUSD.TST2.
//

PEP_NOTIFICATION_HANDLER_RESULT
Tst2SyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the test device 2. It will try to evaluate the
    _ADR method synchronously, while leaving _CRS to the asynchronous
    method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    ULONG DeviceAdr;
    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    DeviceAdr = TST2_ADR;
    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_ADR:
        PepReturnAcpiData(
            &DeviceAdr,
            ACPI_METHOD_ARGUMENT_INTEGER,
            (ULONG)(sizeof(ULONG)),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "ADR",
            "TST2",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_CRS:
        ReportNeedMoreWork(&EcmBuffer->MethodStatus,
                           "CRS",
                           "TST2",
                           &CompleteStatus);
        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

PEP_NOTIFICATION_HANDLER_RESULT
Tst2WorkerCallbackEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the test device 2. It will be scheduled to run
    asychronously. In this example, it only handles _CRS method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Supplies a pointer to the PEP_WORK structure used to
        report result to PoFx.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PEP_ACPI_RESOURCE Resources[2];
    SIZE_T RequiredSize;
    NTSTATUS Status;

    TraceEvents(INFO,
                DBG_PEP,
                "%s <TST2>: Asychronous method scheduled to run.\n",
                __FUNCTION__);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EcmBuffer->DeviceHandle;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    PoFxWorkInfo->WorkType = PepWorkAcpiEvaluateControlMethodComplete;
    PoFxWorkInfo->ControlMethodComplete.DeviceHandle =
        PepInternalDevice->KernelHandle;

    PoFxWorkInfo->ControlMethodComplete.CompletionFlags = 0;
    PoFxWorkInfo->ControlMethodComplete.CompletionContext =
        EcmBuffer->CompletionContext;

    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_CRS:

        //
        // Test device 2 needs GPIO INT resource.
        //

        PEP_ACPI_INITIALIZE_GPIO_INT_RESOURCE(
            TST2_GPIOINT_RES_INT_TYPE,
            TST2_GPIOINT_RES_INT_LVL,
            FALSE,
            FALSE,
            TST2_GPIOINT_RES_PIN_CONFIG,
            TST2_GPIOINT_RES_DEBOUNCE,
            0,
            &GpioResourceName,
            FALSE,
            NULL,
            0,
            Tst2GpioIntPinTable,
            (UCHAR)(ARRAYSIZE(Tst2GpioIntPinTable)),
            &Resources[0]);

        //
        // Test device 2 needs SPB resource.
        //

        PEP_ACPI_INITIALIZE_SPB_I2C_RESOURCE(
            TST2_SPB_RES_SLAVE_ADDR,
            FALSE,
            TST2_SPB_RES_CONN_SPD,
            FALSE,
            &SpbResourceName,
            0,
            FALSE,
            FALSE,
            NULL,
            0,
            &Resources[1]);

        //
        // Find the length required to hold the BIOS resources.
        //

        Status = PepGetBiosResourceSize(Resources,
                                     sizeof(Resources),
                                     "TST2",
                                     &RequiredSize);

        if(!NT_SUCCESS(Status)) {
            PoFxWorkInfo->ControlMethodComplete.MethodStatus = Status;
            PoFxWorkInfo->ControlMethodComplete.OutputArgumentSize = 0;
            goto Tst2WorkerCallbackEvaluateControlMethodEnd;
        }

        PepReturnBiosResource(Resources,
                           sizeof(Resources),
                           RequiredSize,
                           PoFxWorkInfo->ControlMethodComplete.OutputArguments,
                           &PoFxWorkInfo->ControlMethodComplete.OutputArgumentSize,
                           "TST2",
                           &PoFxWorkInfo->ControlMethodComplete.MethodStatus);

        break;

    default:
        ReportNotSupported(&PoFxWorkInfo->ControlMethodComplete.MethodStatus,
                           (PULONG)(&PoFxWorkInfo->
                               ControlMethodComplete.OutputArgumentSize),
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

Tst2WorkerCallbackEvaluateControlMethodEnd:
    return CompleteStatus;
}

PEP_NOTIFICATION_HANDLER_RESULT
Tst3SyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the test device 1. It will be scheduled to run
    asychronously. In this example, it only handles _CRS method.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Supplies a pointer to the PEP_WORK structure used to
        report result to PoFx.

Return Value:

    None.

--*/

{

    PACPI_METHOD_ARGUMENT InputArgument;
    LPGUID DsmUUID;
    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PVOID FunctionArguments;
    ULONG FunctionArgumentSize;
    ULONG FunctionIndex;
    NTSTATUS LocalStatus;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    ULONG RevisionLevel;
    ULONG UuidIndex;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EcmBuffer->DeviceHandle;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_HID:
        PepReturnAcpiData(
            TST3_HID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(TST3_HID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "CRS",
            "TST3",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_CID:
        LocalStatus = Tst3CreateSelfManagedCidRequest(EcmBuffer);
        if (NT_SUCCESS(LocalStatus)) {
            ReportNeedMoreSelfManagedWork(&EcmBuffer->MethodStatus,
                                          "CID",
                                          "TST3",
                                          &CompleteStatus);

        } else {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s [TST3 _CID failed with status = %#x!\n",
                        PepAcpiNotificationHandlers[
                            PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD].Name,
                        LocalStatus);

            EcmBuffer->MethodStatus = LocalStatus;
            EcmBuffer->OutputArgumentCount = 0;
            EcmBuffer->OutputArgumentSize = 0;
            CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
        }

        break;

    case ACPI_OBJECT_NAME_PR0:
    case ACPI_OBJECT_NAME_PR2:

        //
        // Test device 3 needs power rail device.
        //

        PepReturnAcpiData(
            SAMPLE_PWRR_ACPI_NAME_ANSI,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(SAMPLE_PWRR_ACPI_NAME_ANSI),
            TRUE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "CRS",
            "TST3",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_DSM:

        //
        // Test device 3 defines 2 DSMs.
        //

        //
        // _DSM requires 4 input arguments.
        //

        EcmBuffer->OutputArgumentCount = 0;
        EcmBuffer->OutputArgumentSize = 0;
        if (EcmBuffer->InputArgumentCount != 4) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s <TST3>: Invalid number of DSM input arguments."
                        "Required = 4, Provided = %d.\n",
                        __FUNCTION__,
                        EcmBuffer->InputArgumentCount);

            EcmBuffer->MethodStatus = STATUS_INVALID_PARAMETER;
            goto Tst3SyncEvaluateControlMethodEnd;
        }

        //
        // The first argument must be the UUID of the DSM.
        //

        InputArgument = EcmBuffer->InputArguments;
        if (InputArgument->Type != ACPI_METHOD_ARGUMENT_BUFFER) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s <TST3>: Invalid type of the first DSM argument."
                        "Required = ACPI_METHOD_ARGUMENT_BUFFER, "
                        "Provided = %d.\n",
                        __FUNCTION__,
                        InputArgument->Type);

            EcmBuffer->MethodStatus = STATUS_INVALID_PARAMETER_1;
            goto Tst3SyncEvaluateControlMethodEnd;
        }

        if (InputArgument->DataLength != (USHORT)(sizeof(GUID))) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s <TST3>: Invalid size of the first DSM argument."
                        "Required = %d, Provided = %d.\n",
                        __FUNCTION__,
                        (ULONG)(sizeof(GUID)),
                        (ULONG)(InputArgument->DataLength));

            EcmBuffer->MethodStatus = STATUS_INVALID_PARAMETER_1;
            goto Tst3SyncEvaluateControlMethodEnd;
        }

        DsmUUID = (LPGUID)(&InputArgument->Data[0]);

        //
        // The second argument must be the revision level.
        //

        InputArgument = ACPI_METHOD_NEXT_ARGUMENT(InputArgument);
        if (InputArgument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s <TST3>: Invalid type of the second DSM argument."
                        "Required = ACPI_METHOD_ARGUMENT_INTEGER, "
                        "Provided = %d.\n",
                        __FUNCTION__,
                        InputArgument->Type);

            EcmBuffer->MethodStatus = STATUS_INVALID_PARAMETER_2;
            goto Tst3SyncEvaluateControlMethodEnd;
        }

        RevisionLevel = InputArgument->Argument;

        //
        // The third argument must be function index.
        //

        InputArgument = ACPI_METHOD_NEXT_ARGUMENT(InputArgument);
        if (InputArgument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s <TST3>: Invalid type of the third DSM argument."
                        "Required = ACPI_METHOD_ARGUMENT_INTEGER, "
                        "Provided = %d.\n",
                        __FUNCTION__,
                        InputArgument->Type);

            EcmBuffer->MethodStatus = STATUS_INVALID_PARAMETER_3;
            goto Tst3SyncEvaluateControlMethodEnd;
        }

        FunctionIndex = InputArgument->Argument;

        //
        // The fourth argument must be a package.
        //

        InputArgument = ACPI_METHOD_NEXT_ARGUMENT(InputArgument);
        if (InputArgument->Type != ACPI_METHOD_ARGUMENT_PACKAGE &&
            InputArgument->Type != ACPI_METHOD_ARGUMENT_PACKAGE_EX) {
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s <TST3>: Invalid type of the fourth DSM argument."
                        "Required = ACPI_METHOD_ARGUMENT_PACKAGE(_EX), "
                        "Provided = %d.\n",
                        __FUNCTION__,
                        InputArgument->Type);

            EcmBuffer->MethodStatus = STATUS_INVALID_PARAMETER_4;
            goto Tst3SyncEvaluateControlMethodEnd;
        }

        FunctionArguments =
            &(((PACPI_METHOD_ARGUMENT)(&InputArgument->Data[0]))->Data[0]);

        FunctionArgumentSize =
            (ULONG)(((PACPI_METHOD_ARGUMENT)
                (&InputArgument->Data[0]))->DataLength);

        //
        // Check the first input argument to decide which UUID is called, and
        // invoke the corresponding methods.
        //

        UuidIndex = 0;
        if (RtlCompareMemory(DsmUUID, &TST3_DSM_UUID1, sizeof(GUID)) == 0) {
            UuidIndex = 1;

        } else if (RtlCompareMemory(DsmUUID, &TST3_DSM_UUID2, sizeof(GUID)) == 0) {
            UuidIndex = 2;

        } else {
            UuidIndex = 0;
        }

        HandleTst3DsmByUuid(UuidIndex,
                            RevisionLevel,
                            FunctionIndex,
                            FunctionArguments,
                            FunctionArgumentSize,
                            EcmBuffer,
                            &CompleteStatus);

        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

Tst3SyncEvaluateControlMethodEnd:
    return CompleteStatus;
}

NTSTATUS
Tst3CreateSelfManagedCidRequest (
    _In_ PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD for _CID
    method on the TST3 device asynchronously in a self-managed manner (i.e.,
    doesn't rely on the common library worker).

Arguments:

    EcmBuffer - Supplies a pointer to the PEP_ACPI_EVALUATE_CONTROL_METHOD
        buffer.

Return Value:

    NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    PTST3_CID_REQUEST_CONTEXT Context;
    NTSTATUS Status;
    WDFWORKITEM WorkItem;
    WDF_WORKITEM_CONFIG WorkItemConfiguration;

    WorkItem = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&Attributes,
                                           TST3_CID_REQUEST_CONTEXT);

    Attributes.ParentObject = PepGlobalWdfDevice;

    //
    // Initialize the handler routine and create a new workitem.
    //

    WDF_WORKITEM_CONFIG_INIT(&WorkItemConfiguration,
                             Tst3SelfManagedCidRequestWorkerWrapper);

    WorkItemConfiguration.AutomaticSerialization = FALSE;

    //
    // Create the work item and queue it.
    //

    Status = WdfWorkItemCreate(&WorkItemConfiguration,
                               &Attributes,
                               &WorkItem);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_PEP,
                    "Failed to allocate work item for TST3 _CID request!"
                    "Status = %#x\n",
                    Status);

        goto Tst3CreateSelfManagedCidRequestEnd;
    }

    Context = Tst3GetCidRequestContext(WorkItem);

    //
    // Initialize the ECM buffer in the context to be supplied to the workitem
    // handler.
    //

    RtlZeroMemory(Context, sizeof(TST3_CID_REQUEST_CONTEXT));
    RtlCopyMemory(&Context->EcmBuffer,
                  EcmBuffer,
                  sizeof(PEP_ACPI_EVALUATE_CONTROL_METHOD));

    //
    // Queue a workitem to run the worker routine.
    //

    WdfWorkItemEnqueue(WorkItem);
    Status = STATUS_SUCCESS;

Tst3CreateSelfManagedCidRequestEnd:
    return Status;
}

VOID
Tst3SelfManagedCidRequestWorkerWrapper (
    __in WDFWORKITEM WorkItem
    )

/*++

Routine Description:

    This function is a wrapper that invokes the appropriate handler.

Arguments:

    WorkItem -  Supplies a handle to the workitem supplying the context.

Return Value:

    None.

--*/

{

    PTST3_CID_REQUEST_CONTEXT Context;

    Context = Tst3GetCidRequestContext(WorkItem);
    Tst3ProcessSelfManagedCidRequest(Context);

    //
    // Delete the work item as it is no longer required.
    //

    WdfObjectDelete(WorkItem);
    return;
}

VOID
Tst3ProcessSelfManagedCidRequest (
    _In_ PTST3_CID_REQUEST_CONTEXT Tst3CidRequestContext
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD for _CID
    method on the TST3 device asynchronously in a self-managed manner (i.e.,
    doesn't rely on the common library worker).

Arguments:

    Tst3CidRequestContext - Supplies a pointer to request context.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PEP_WORK_INFORMATION WorkInformation;

    EcmBuffer =
        (PPEP_ACPI_EVALUATE_CONTROL_METHOD)
            &Tst3CidRequestContext->EcmBuffer;

    PepInternalDevice = (PPEP_INTERNAL_DEVICE_HEADER)EcmBuffer->DeviceHandle;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_CID:
        PepReturnAcpiData(
            TST3_CID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(TST3_CID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "CID",
            "TST3",
            &CompleteStatus);

        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);

        EcmBuffer->OutputArgumentSize = 0;
    }

    //
    // Fill PEP_WORK_INFORMATION based on the ACPI data to be returned.
    //

    RtlZeroMemory(&WorkInformation, sizeof(PEP_WORK_INFORMATION));
    WorkInformation.WorkType = PepWorkAcpiEvaluateControlMethodComplete;
    WorkInformation.ControlMethodComplete.DeviceHandle =
        PepInternalDevice->KernelHandle;

    WorkInformation.ControlMethodComplete.CompletionFlags = 0;
    WorkInformation.ControlMethodComplete.CompletionContext =
        EcmBuffer->CompletionContext;

    WorkInformation.ControlMethodComplete.MethodStatus =
        EcmBuffer->MethodStatus;

    WorkInformation.ControlMethodComplete.OutputArgumentSize  =
        EcmBuffer->OutputArgumentSize;

    WorkInformation.ControlMethodComplete.OutputArguments =
        EcmBuffer->OutputArguments;

    PepCompleteSelfManagedWork(
        PEP_NOTIFICATION_CLASS_ACPI,
        PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD,
        PepInternalDevice,
        &WorkInformation);

    return;
}

//
// Notification handlers for \_SB.PWRR.
//

PEP_NOTIFICATION_HANDLER_RESULT
PwrrSyncEvaluateControlMethod (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine handles PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD
    notification for the power rail device. It will evaluate _HID, _STA,
    _ON and _OFF synchronously.

Arguments:

    Data - Supplies a pointer to parameters buffer for this notification.

    PoFxWorkInfo - Unused.

Return Value:

    None.

--*/

{

    PEP_NOTIFICATION_HANDLER_RESULT CompleteStatus;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;

    UNREFERENCED_PARAMETER(PoFxWorkInfo);

    EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
    CompleteStatus = PEP_NOTIFICATION_HANDLER_COMPLETE;
    switch(EcmBuffer->MethodName) {
    case ACPI_OBJECT_NAME_HID:
        PepReturnAcpiData(
            PWRR_HID,
            ACPI_METHOD_ARGUMENT_STRING,
            GET_STRING_LENGTH(PWRR_HID),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "HID",
            "PWRR",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_STA:
        PepReturnAcpiData(
            &PowerRailState,
            ACPI_METHOD_ARGUMENT_INTEGER,
            (ULONG)(sizeof(ULONG)),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "STA",
            "PWRR",
            &CompleteStatus);

        break;

    case ACPI_OBJECT_NAME_ON:
    case ACPI_OBJECT_NAME_OFF:
        if (EcmBuffer->MethodName == ACPI_OBJECT_NAME_ON) {
            PowerRailState = 1;
            TraceEvents(INFO,
                        DBG_PEP,
                        "%s <PWRR>: %s Power rail is turned ON.\n",
                        __FUNCTION__,
                        PepAcpiNotificationHandlers[
                            PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD].Name);

        } else {
            PowerRailState = 0;
            TraceEvents(INFO,
                        DBG_PEP,
                        "%s <PWRR>: %s Power rail is turned OFF.\n",
                        __FUNCTION__,
                        PepAcpiNotificationHandlers[
                            PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD].Name);
        }

        EcmBuffer->OutputArgumentCount = 0;
        EcmBuffer->OutputArgumentSize = 0;
        EcmBuffer->MethodStatus = STATUS_SUCCESS;
        break;

    default:
        ReportNotSupported(&EcmBuffer->MethodStatus,
                           &EcmBuffer->OutputArgumentCount,
                           &CompleteStatus);
    }

    //
    // Return complete status.
    //

    return CompleteStatus;
}

//
// Helper functions.
//

VOID
ReportNeedMoreWork (
    _Out_ PNTSTATUS Status,
    _In_opt_ PCHAR MethodName,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    )

/*++

Routine Description:

    This routine reports to PoFx that more work is needed.

Arguments:

    Status - Supplies a pointer to receive the STATUS_PENDING.

    MethodName - Supplies an optional string that names the native method
        used for logging.

    DebugInfo - Supplies an optional string that contains the debugging
        information to be included in the log.

    CompleteResult - Supplies a pointer to receive the complete result.

Return Value:

    None.

--*/

{

    *Status = STATUS_PENDING;
    TraceEvents(INFO,
                DBG_PEP,
                "%s <%s> [%s] More work needed.\n",
                __FUNCTION__,
                NAME_DEBUG_INFO(DebugInfo),
                NAME_NATIVE_METHOD(MethodName));

    //
    // Do nothing here but set the return status to
    // PEP_NOTIFICATION_HANDLER_MORE_WORK, so that the async method will
    // be invoked instead.
    //

    *CompleteResult = PEP_NOTIFICATION_HANDLER_MORE_WORK;
    return;
}

VOID
ReportNotSupported (
    _Out_ PNTSTATUS Status,
    _Out_ PULONG Count,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    )

/*++

Routine Description:

    This routine reports to PoFx that the notification is not supported.

Arguments:

    Status - Supplies a pointer to receive the evaluation status.

    Count - Supplies a pointer to receive the output argument count/size.

    CompleteResult - Supplies a pointer to receive the complete result.

Return Value:

    None.

--*/

{

    *Count = 0;
    *Status = STATUS_NOT_SUPPORTED;
    TraceEvents(ERROR,
                DBG_PEP,
                "%s [UNKNOWN] Native method not supported.\n",
                PepAcpiNotificationHandlers[
                    PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD].Name);

    *CompleteResult = PEP_NOTIFICATION_HANDLER_COMPLETE;
    return;
}

VOID
ReportNeedMoreSelfManagedWork (
    _Out_ PNTSTATUS Status,
    _In_opt_ PCHAR MethodName,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    )

/*++

Routine Description:

    This routine reports to PoFx that more work is needed.

Arguments:

    Status - Supplies a pointer to receive the STATUS_PENDING.

    MethodName - Supplies an optional string that names the native method
        used for logging.

    DebugInfo - Supplies an optional string that contains the debugging
        information to be included in the log.

    CompleteResult - Supplies a pointer to receive the complete result.

Return Value:

    None.

--*/

{

    *Status = STATUS_PENDING;
    TraceEvents(INFO,
                DBG_PEP,
                "%s <%s> [%s] More work needed - SELF Managed.\n",
                __FUNCTION__,
                NAME_DEBUG_INFO(DebugInfo),
                NAME_NATIVE_METHOD(MethodName));

    //
    // Set the status to PEP_NOTIFICATION_HANDLER_COMPLETE to prevent the
    // common library from invoking the async method. Instead, the work will
    // be reported as finally complete when desired.
    //

    *CompleteResult = PEP_NOTIFICATION_HANDLER_COMPLETE;
    return;
}

VOID
HandleTst3DsmByUuid (
    _In_ ULONG UuidIndex,
    _In_ ULONG RevisionLevel,
    _In_ ULONG FunctionIndex,
    _In_ PVOID Parameters,
    _In_ ULONG ParametersSize,
    _Out_ PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    )

/*++

Routine Description:

    This routine handles the DSM for test device 3 and report the result
    back to PoFx.

Arguments:

    UuidIndex - Supplies which DSM to handle.
        1 = TST3_DSM_UUID1;
        2 = TST3_DSM_UUID2;
        anything else = UUID not supported.

    RevisionLevel - Supplies the revision of the DSM.

    FunctionIndex - Supplies the function index of the DSM.

    Parameters - Supplies the parameters for the DSM.

    ParametersSize - Supplies the size of the parameter buffer.

    EcmBuffer - Supplies a pointer to the output buffer.

    CompleteResult - Supplies a pointer to receive the complete result.

Return Value:

    None.

--*/

{

    UCHAR ReturnBuffer[1];
    ULONG ReturnInteger;
    NTSTATUS ReturnStatus;
    USHORT ReturnType;

    UNREFERENCED_PARAMETER(Parameters);
    UNREFERENCED_PARAMETER(ParametersSize);

    RtlZeroMemory(ReturnBuffer, sizeof(ReturnBuffer));
    ReturnInteger = 0;
    ReturnStatus = STATUS_SUCCESS;
    switch (UuidIndex) {
    case 1: // TST3_DSM_UUID1
        switch (FunctionIndex) {
        case 0:

            //
            // If function index is 0, the return must be a buffer containing
            // one bit for each supported function index.
            //

            ReturnType = ACPI_METHOD_ARGUMENT_BUFFER;

            //
            // Return a bitmap containing the supported function
            // indicies according to the revision level. In this
            // example, for DSM TST3_DSM_UUID1 revision 1, function
            // 1 is supported. For revision 2, function 1 and 2 are
            // supported. For any other revision, function 1 ~ 3 are
            // supported. (Note that function 0 is always supported).
            //

            switch (RevisionLevel) {
            case 1:
                ReturnBuffer[0] = 0x3;
                break;

            case 2:
                ReturnBuffer[0] = 0x7;
                break;

            default:
                ReturnBuffer[0] = 0xF;
                break;
            }

            break;

        case 1:

            //
            // In this example, for simplicity, all the functions will just
            // return their function index as integer.
            //

            ReturnType = ACPI_METHOD_ARGUMENT_INTEGER;
            ReturnInteger = 1;
            break;

        case 2:

            //
            // Check the revision here to make sure revision level is enough
            // to run function 2, do the same for function 3.
            //

            ReturnType = ACPI_METHOD_ARGUMENT_INTEGER;
            if (RevisionLevel < 2) {
                ReturnStatus = STATUS_INVALID_PARAMETER;

            } else {
                ReturnInteger = 2;
            }

            break;

        case 3:
            ReturnType = ACPI_METHOD_ARGUMENT_INTEGER;
            if (RevisionLevel < 3) {
                ReturnStatus = STATUS_INVALID_PARAMETER;

            } else {
                ReturnInteger = 3;
            }

            break;

        default:
            ReturnType = ACPI_METHOD_ARGUMENT_INTEGER;
            ReturnStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        break;

    case 2: // TST3_DSM_UUID2
        switch (FunctionIndex) {
        case 0:
            ReturnType = ACPI_METHOD_ARGUMENT_BUFFER;

            //
            // In this example, for DSM TST3_DSM_UUID2, only function 1
            // is supported.
            //

            ReturnBuffer[0] = 0x3;
            break;

        case 1:
            ReturnType = ACPI_METHOD_ARGUMENT_INTEGER;
            ReturnInteger = 1;
            break;

        default:
            ReturnType = ACPI_METHOD_ARGUMENT_INTEGER;
            ReturnStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        break;

    default: // DSM not supported.

        //
        // If the UUID is not supported, the return must be a buffer with
        // bit 0 set to 0.
        //

        ReturnType = ACPI_METHOD_ARGUMENT_BUFFER;
        ReturnBuffer[0] = 0x0;
        break;
    }

    //
    // Return the value back to PoFx.
    //

    EcmBuffer->MethodStatus = ReturnStatus;
    if (!NT_SUCCESS(ReturnStatus)) {
        EcmBuffer->OutputArgumentCount = 0;
        EcmBuffer->OutputArgumentSize = 0;
        return;
    }

    switch (ReturnType) {
    case ACPI_METHOD_ARGUMENT_BUFFER:
        PepReturnAcpiData(
            ReturnBuffer,
            ACPI_METHOD_ARGUMENT_BUFFER,
            (ULONG)(sizeof(ReturnBuffer)),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "DSM",
            "TST3",
            CompleteResult);

        return;

    case ACPI_METHOD_ARGUMENT_INTEGER:
        PepReturnAcpiData(
            &ReturnInteger,
            ACPI_METHOD_ARGUMENT_INTEGER,
            (ULONG)(sizeof(ULONG)),
            FALSE,
            EcmBuffer->OutputArguments,
            &EcmBuffer->OutputArgumentSize,
            &EcmBuffer->OutputArgumentCount,
            &EcmBuffer->MethodStatus,
            "DSM",
            "TST3",
            CompleteResult);

        return;

    default:
        NT_ASSERT(FALSE);
        return;
    }
}



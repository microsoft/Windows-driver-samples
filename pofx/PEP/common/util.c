/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    util.c

Abstract:

    This file implements all the utility routines.


Environment:

    Kernel mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include "pch.h"

#if defined(EVENT_TRACING)
#include "util.tmh"
#endif

//
//--------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(PAGE, PepQueryRegistry)

//
//------------------------------------------------------------------- Functions
//

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
PepQueryRegistry (
    _Inout_opt_ WDFKEY *ParametersKey,
    _In_opt_ WDFDRIVER WdfDriver,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PCUNICODE_STRING KeyValueName,
    _Out_ PULONG OptionValue
    )

/*++

Routine Description:

    This routine retrieves driver-wide flags from the registry. Path is:
       HKLM\CCS\SERVICES\<driver name>\Parameters\<key>

Arguments:

    ParametersKey - Supplies an optional pointer that provides a handle to
        the registry to use. If the handle is NULL, then on output, this
        parameters receives a handle to the "Parameters" key. The caller is
        responsible for closing the handle later on.

    WdfDriver - Supplies an optional handle to the WDF driver object. This is
        required if *ParametersKey == NULL.

    RegistryPath - Supplies a pointer to the driver specific registry key.

    KeyValueName - Supplies the key-value name to be queried.

    OptionValue - Supplies a pointer that receives the registry value.

Return Value:

    NTSTATUS

--*/

{
    BOOLEAN CloseKey;
    WDFKEY Key;
    NTSTATUS Status;

    PAGED_CODE();

    CloseKey = FALSE;
    Key = NULL;
    if ((ParametersKey != NULL) && (*ParametersKey != NULL)) {
        Key = *ParametersKey;

    } else {
        Status = WdfDriverOpenParametersRegistryKey(WdfDriver,
                                                    KEY_READ,
                                                    WDF_NO_OBJECT_ATTRIBUTES,
                                                    &Key);

        if (!NT_SUCCESS(Status)) {
            TraceEvents(ERROR,
                        DBG_INIT,
                        "%s: Failed to open parameters registry key! "
                        "Path = %S\\%S, Status= %!STATUS!.\n",
                        __FUNCTION__,
                        RegistryPath->Buffer,
                        L"Parameters",
                        Status);

            goto QueryRegistryEnd;
        }

        CloseKey = TRUE;
    }

    Status = WdfRegistryQueryULong(Key, KeyValueName, OptionValue);
    if (!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND)) {
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: Failed to query registry flags! Path = %S\\%S, "
                    "ValueKey = %S, Status= %!STATUS!.\n",
                    __FUNCTION__,
                    RegistryPath->Buffer,
                    L"Parameters",
                    KeyValueName->Buffer,
                    Status);

        goto QueryRegistryEnd;

    } else if (NT_SUCCESS(Status)) {
        TraceEvents(VERBOSE,
                    DBG_INIT,
                    "%s: Successfully queried registry flags! Path = %S\\%S, "
                    "ValueKey = %S, Value = %#x.\n",
                    __FUNCTION__,
                    RegistryPath->Buffer,
                    L"Parameters",
                    KeyValueName->Buffer,
                    *OptionValue);
    }

    //
    // On success return the parameter key to caller if desired.
    //

QueryRegistryEnd:
    if (NT_SUCCESS(Status) &&
        (ParametersKey != NULL) &&
        (*ParametersKey != NULL)) {

        *ParametersKey = Key;

    } else if ((CloseKey != FALSE) && (Key != NULL)) {
        WdfRegistryClose(Key);
    }

    return Status;
}

BOOLEAN
PepIsDeviceAccepted (
    _In_ PEP_NOTIFICATION_CLASS OwnedType,
    _In_ PCUNICODE_STRING DeviceId,
    _Out_ PPEP_DEVICE_DEFINITION *DeviceDefinition
    )

/*++

Routine Description:

    This routine is called to see if a device should be accepted by this PEP.

Arguments:

    OwnedType - Supplies the expected type of this PEP, which can be a
        combination of ACPI, DPM, PPM or NONE.

    DeviceId - Supplies a pointer to a unicode string that contains the
        device ID or instance path for the device.

    DeviceDefinition - Supplies the pointer for storing the accepted device's
        device-specific definitions.

Return Value:

    TRUE if this PEP owns this device;
    FALSE otherwise.

--*/

{

    PEP_DEVICE_ID_MATCH Compare;
    ULONG Index;
    BOOLEAN Match;
    PEP_DEVICE_TYPE Type;

    Match = FALSE;
    Type = PEP_INVALID_DEVICE_TYPE;
    for (Index = 0; Index < PepDeviceMatchArraySize; Index += 1) {
        if (PEP_CHECK_DEVICE_TYPE_ACCEPTED(
                (PepDeviceMatchArray[Index].OwnedType),
                OwnedType) == FALSE) {

            continue;
        }

        //
        // If the type is owned by this PEP, check the device id.
        //

        Compare = PepDeviceMatchArray[Index].CompareMethod;
        Match = PepIsDeviceIdMatched(
                    DeviceId->Buffer,
                    DeviceId->Length / sizeof(WCHAR),
                    PepDeviceMatchArray[Index].DeviceId,
                    (ULONG)wcslen(PepDeviceMatchArray[Index].DeviceId),
                    Compare);

        if (Match != FALSE) {
            TraceEvents(VERBOSE,
                        DBG_INIT,
                        "%s: Found device whose type matches. "
                        "DeviceId: %S\n",
                        __FUNCTION__,
                        DeviceId->Buffer);

            Type = PepDeviceMatchArray[Index].Type;
            break;
        }
    }

    if (Match == FALSE) {
        goto IsDeviceAcceptedEnd;
    }

    Match = FALSE;
    for (Index = 0; Index < PepDeviceDefinitionArraySize; Index += 1) {
        if (PepDeviceDefinitionArray[Index].Type == Type) {
            TraceEvents(VERBOSE,
                        DBG_INIT,
                        "%s: Found device definition of the given type.\n",
                        __FUNCTION__);

            Match = TRUE;
            *DeviceDefinition = &PepDeviceDefinitionArray[Index];
            break;
        }
    }

IsDeviceAcceptedEnd:
    return Match;
}

BOOLEAN
PepIsDeviceIdMatched (
    _In_ PWSTR String,
    _In_ ULONG StringLength,
    _In_ PWSTR SearchString,
    _In_ ULONG SearchStringLength,
    _In_ PEP_DEVICE_ID_MATCH DeviceIdCompareMethod
    )

/*++

Routine Description:

    This routine check whether two given strings matches in the way specified
    by the DeviceIdCompare flag.

Arguments:

    String - Supplies a pointer to a unicode string to search within.

    StringLength - Supplies the length of the string to search within.

    SearchString - Supplies a pointer to a unicode string to search for.

    SearchStringLength - Supplies the length of the string to search for.

    DeviceIdCompareMethod - Supplies the flag indicating the way two
        strings should match.
        - PepDeviceIdMatchPartial: substring match;
        - PepDeviceIdMatchFull: whole string match.

Return Value:

    TRUE if the search string is found;
    FALSE otherwise.

--*/

{

    BOOLEAN Found;
    ULONG Index;
    ULONG Result;
    UNICODE_STRING SearchStringUnicode;
    UNICODE_STRING SourceStringUnicode;

    Found = FALSE;
    switch (DeviceIdCompareMethod) {
    case PepDeviceIdMatchFull:
        RtlInitUnicodeString(&SourceStringUnicode, String);
        RtlInitUnicodeString(&SearchStringUnicode, SearchString);
        Result = RtlCompareUnicodeString(&SourceStringUnicode,
                                         &SearchStringUnicode,
                                         FALSE);

        if (Result == 0) {
            Found = TRUE;
        }

        break;

    case PepDeviceIdMatchPartial:
        if (StringLength < SearchStringLength) {
            goto IsDeviceIdMatchedEnd;
        }

        for (Index = 0; Index <= (StringLength - SearchStringLength);
             Index += 1) {

            if(!_wcsnicmp(String + Index,
                          SearchString,
                          SearchStringLength)) {

                Found = TRUE;
                goto IsDeviceIdMatchedEnd;
            }
        }

        break;

    default:
        TraceEvents(ERROR,
                    DBG_INIT,
                    "%s: Unknown DeviceIdCompare = %d.\n",
                    __FUNCTION__,
                    (ULONG)DeviceIdCompareMethod);
    }

IsDeviceIdMatchedEnd:
    return Found;
}

VOID
PepScheduleNotificationHandler (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_ ULONG NotificationId,
    _In_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_opt_ PVOID WorkContext,
    _In_ SIZE_T WorkContextSize,
    _In_opt_ PNTSTATUS WorkRequestStatus
    )

/*++

Routine Description:

    This routine schedules the device-specific handler.

Arguments:

    WorkType - Supplies the type of the work (ACPI/DPM/PPM).

    NotificationId - Supplies the PEP notification type.

    PepInternalDevice - Supplies the internal PEP device.

    WorkContext - Supplies optional pointer to the context of the work request.

    WorkContextSize - Supplies the size of the work request context.

    WorkRequestStatus -  Supplies optional pointer to report the
        status of the work request.

Return Value:

    None.

--*/

{

    PPEP_DEVICE_DEFINITION DeviceDefinition;
    NTSTATUS Status;
    PPEP_WORK_CONTEXT WorkRequest;

    DeviceDefinition = PepInternalDevice->DeviceDefinition;
    Status = PepCreateWorkRequest(WorkType,
                                  NotificationId,
                                  PepInternalDevice,
                                  DeviceDefinition,
                                  WorkContext,
                                  WorkContextSize,
                                  WorkRequestStatus,
                                  &WorkRequest);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: PepCreateWorkRequest() failed!. "
                    "Status = %!STATUS!.\n ",
                    __FUNCTION__,
                    Status);

        goto ScheduleNotificationHandlerEnd;
    }

    PepPendWorkRequest(WorkRequest);

    //
    // Mark the work request status as pending.
    //

    if (WorkRequestStatus != NULL) {
        *WorkRequestStatus = STATUS_PENDING;
    }

ScheduleNotificationHandlerEnd:
    return;
}

_Requires_lock_not_held_(PepWorkListLock)
VOID
PepInvokeNotificationHandler (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_opt_ PPEP_WORK_CONTEXT WorkRequest,
    _In_ PEP_HANDLER_TYPE HandlerType,
    _In_ ULONG NotificationId,
    _In_opt_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_ PVOID Data,
    _In_ SIZE_T DataSize,
    _In_opt_ PNTSTATUS WorkRequestStatus
    )

/*++

Routine Description:

    This routine invokes the handler of the specified type if one is
    registered.

Arguments:

    WorkType - Supplies the type of the work (ACPI/DPM/PPM).

    WorkRequest -Supplies optional pointer to the work context for async work.

    HandlerType - Supplies the type of handler to be invoked.

    NotificationId - Supplies the PEP notification type.

    PepInternalDevice - Supplies the internal PEP device.

    Data - Supplies a pointer to data to be passed to the handler.

    DataSize - Supplies the size of the data.

    WorkRequestStatus -  Supplies optional pointer to report the
        status of the work request.

Return Value:

    None.

--*/

{

    ULONG Count;
    PPEP_DEVICE_DEFINITION DeviceDefinition;
    PPEP_ACPI_EVALUATE_CONTROL_METHOD EcmBuffer;
    PPEP_NOTIFICATION_HANDLER_ROUTINE Handler;
    PEP_NOTIFICATION_HANDLER_RESULT HandlerResult;
    ULONG Index;
    BOOLEAN NoSyncHandler;
    PPEP_WORK_INFORMATION PoFxWorkInfo;
    PPEP_DEVICE_NOTIFICATION_HANDLER Table;

    DeviceDefinition = PepInternalDevice->DeviceDefinition;
    if (WorkRequest != NULL) {
        PoFxWorkInfo = &(WorkRequest->LocalPoFxWorkInfo);
    } else {
        PoFxWorkInfo = NULL;
    }

    switch (WorkType) {
    case PEP_NOTIFICATION_CLASS_ACPI:
        Count = DeviceDefinition->AcpiNotificationHandlerCount;
        Table = DeviceDefinition->AcpiNotificationHandlers;
        break;

    case PEP_NOTIFICATION_CLASS_DPM:
        Count = DeviceDefinition->DpmNotificationHandlerCount;
        Table = DeviceDefinition->DpmNotificationHandlers;
        break;

    default:
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: Unknown WorkType = %d.\n",
                    __FUNCTION__,
                    (ULONG)WorkType);

        goto InvokeNotificationHandlerEnd;
    }

    for (Index = 0; Index < Count; Index += 1) {
        if (Table[Index].Notification != NotificationId) {
            continue;
        }

        Handler = NULL;
        NoSyncHandler = FALSE;
        switch (HandlerType) {
        case PepHandlerTypeSyncCritical:
            Handler = Table[Index].Handler;
            if (Handler == NULL) {
                NoSyncHandler = TRUE;
                Handler = Table[Index].WorkerCallbackHandler;
            }

            break;

        case PepHandlerTypeWorkerCallback:
            Handler = Table[Index].WorkerCallbackHandler;
            break;

        default:
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s: Unknown HandlerType = %d.\n",
                        __FUNCTION__,
                        (ULONG)HandlerType);

            goto InvokeNotificationHandlerEnd;
        }

        if ((WorkType == PEP_NOTIFICATION_CLASS_ACPI) &&
            (WorkRequest != NULL)) {

            switch(NotificationId) {
            case PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD:
                EcmBuffer = (PPEP_ACPI_EVALUATE_CONTROL_METHOD)Data;
                PoFxWorkInfo->ControlMethodComplete.
                    OutputArguments = EcmBuffer->OutputArguments;

                PoFxWorkInfo->ControlMethodComplete.
                    OutputArgumentSize = EcmBuffer->OutputArgumentSize;

                break;

            default:
                break;
            }
        }

        HandlerResult = PEP_NOTIFICATION_HANDLER_MAX;
        if (Handler != NULL) {
            if (NoSyncHandler == FALSE) {
                HandlerResult = Handler(Data, PoFxWorkInfo);

                //
                // Result is expected to be either complete or need more work.
                //

                NT_ASSERT(HandlerResult < PEP_NOTIFICATION_HANDLER_MAX);
            }

            //
            // If the handler completes the request and the work context is
            // not NULL, report to PoFx.
            //

            if (NoSyncHandler == FALSE &&
                HandlerResult == PEP_NOTIFICATION_HANDLER_COMPLETE) {
                if (WorkRequest != NULL) {
                    PepCompleteWorkRequest(WorkRequest);
                }

            } else {

                //
                // Make sure the request has been dequeued.
                //

                NT_ASSERT((WorkRequest == NULL) ||
                          (IsListEmpty(&WorkRequest->ListEntry) != FALSE));

                NT_ASSERT((NoSyncHandler != FALSE) ||
                          (HandlerResult == PEP_NOTIFICATION_HANDLER_MORE_WORK));

                //
                // If the handler needs to do async work, schedule a worker.
                //

                PepScheduleNotificationHandler(WorkType,
                                               NotificationId,
                                               PepInternalDevice,
                                               Data,
                                               DataSize,
                                               WorkRequestStatus);
            }
        }

        break;
    }

InvokeNotificationHandlerEnd:
    return;
}

PWSTR
PepGetDeviceName (
    _In_ ULONG DeviceType
    )

/*++

Routine Description:

    This routine retrives the device Id by device type.

Arguments:

    DeviceType - Supplies a unique identifier of the device.

Return Value:

    DeviceId if the device is accepted by PEP;
    NULL otherwise.

--*/

{

    ULONG Index;
    PWSTR DeviceId;

    DeviceId = NULL;
    for (Index = 0; Index < PepDeviceMatchArraySize; Index += 1) {
        if (PepDeviceMatchArray[Index].Type == DeviceType) {
            DeviceId = PepDeviceMatchArray[Index].DeviceId;
            break;
        }
    }

    return DeviceId;
}

VOID
PepRequestCommonWork (
    _In_ PPEP_ACPI_REQUEST_CONVERT_TO_BIOS_RESOURCES Request
    )

/*++

Routine Description:

    This routine retrives the device Id by device type.

Arguments:

    DeviceType - Supplies a unique identifier of the device.

Return Value:

    DeviceId if the device is accepted by PEP;
    NULL otherwise.

--*/

{

    PepKernelInformation.RequestCommon(
        PEP_ACPI_REQUEST_COMMON_CONVERT_TO_BIOS_RESOURCES,
        Request);

    return;

}

NTSTATUS
PepGetBiosResourceSize (
    _In_ PPEP_ACPI_RESOURCE ResourceBuffer,
    _In_ ULONG ResourceBufferSize,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PSIZE_T BiosResourceSize
    )

/*++

Routine Description:

    This routine queries the size of the BIOS resource.

Arguments:

    ResourceBuffer - Supplies a pointer to the buffer containing the resource.

    ResourceBufferSize - Supplies the size of the resource buffer.

    DebugInfo - Supplies an optional string that contains the debugging
        information to be included in the log.

    BiosResourceSize - Supplies a pointer to receive the required size.

Return Value:

    NTSTATUS.

--*/

{

    PEP_ACPI_REQUEST_CONVERT_TO_BIOS_RESOURCES Request;
    ULONG RequiredSize;
    NTSTATUS Status;

    //
    // Initalize BIOS Resource Request.
    //

    Request.InputBuffer = ResourceBuffer;
    Request.InputBufferSize = ResourceBufferSize;
    Request.OutputBuffer = NULL;
    Request.OutputBufferSize = 0;
    Request.Flags = 0;

    //
    // Find the length required to hold the BIOS resources.
    //

    RequiredSize = 0;
    PepRequestCommonWork(&Request);
    Status = Request.TranslationStatus;
    if(Status != STATUS_BUFFER_TOO_SMALL) {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s <%s>: PepRequestCommonWork() failed to determine "
                    "the size of BIOS resource. Status = %!STATUS!.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo),
                    Request.TranslationStatus);

    } else {
        Status = STATUS_SUCCESS;
        RequiredSize = (ULONG)Request.OutputBufferSize;
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s <%s>: PepRequestCommonWork() determines "
                    "RequiredSize = %d.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo),
                    RequiredSize);
    }

    *BiosResourceSize = RequiredSize;
    return Status;
}

VOID
PepReturnBiosResource (
    _In_ PPEP_ACPI_RESOURCE ResourceBuffer,
    _In_ ULONG ResourceBufferSize,
    _In_ SIZE_T RequiredSize,
    _In_ PACPI_METHOD_ARGUMENT Arguments,
    _Inout_ PSIZE_T BiosResourcesSize,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PNTSTATUS Status
    )

/*++

Routine Description:

    This routine reports to PoFx the BIOS resources.

Arguments:

    ResourceBuffer - Supplies a pointer to the buffer containing the resource.

    ResourceBufferSize - Supplies the size of the resource buffer.

    RequiredSize - Supplies the required size of the output buffer.

    Arguments - Supplies a pointer to receive the returned resource.

    BiosResourcesSize - Supplies a pointer to receive the returned resource size.

    DebugInfo - Supplies an optional string that contains the debugging
        information to be included in the log.

    Status -Supplies a pointer to receive the returned evaluation status.

Return Value:

    None.

--*/

{

    PUCHAR BiosResource;
    PEP_ACPI_REQUEST_CONVERT_TO_BIOS_RESOURCES Request;
    SIZE_T RequiredSizeAugmented;

    //
    // Return the required size to the caller if the input buffer size
    // is not sufficient.
    //

    RequiredSizeAugmented = ACPI_METHOD_ARGUMENT_LENGTH(RequiredSize);
    if ((*BiosResourcesSize) < RequiredSizeAugmented) {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s <%s>: "
                    "Buffer too small, Required=%d, Provided=%d.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo),
                    (ULONG)RequiredSizeAugmented,
                    (ULONG)(*BiosResourcesSize));

        *BiosResourcesSize = RequiredSizeAugmented;
        *Status = STATUS_BUFFER_TOO_SMALL;

    } else {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s <%s>: "
                    "Convert to BIOS resource, "
                    "BufferSize = %d, RequiredSize = %d, ProvidedSize = %d.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo),
                    (ULONG)RequiredSize,
                    (ULONG)RequiredSizeAugmented,
                    (ULONG)(*BiosResourcesSize));

        BiosResource = ExAllocatePoolWithTag(NonPagedPool,
                                             RequiredSize,
                                             PEP_POOL_TAG);

        Request.InputBuffer = ResourceBuffer;
        Request.InputBufferSize = ResourceBufferSize;
        Request.OutputBuffer = BiosResource;
        Request.OutputBufferSize = RequiredSize;
        Request.Flags = 0;

        //
        // Call Pep Request Common to convert ACPI resources to BIOS resources.
        //

        PepRequestCommonWork(&Request);
        if(!NT_SUCCESS(Request.TranslationStatus)) {
            TraceEvents(INFO,
                    DBG_PEP,
                    "%s <%s>: PepRequestCommonWork() failed convert "
                    "to BIOS resource. Status = %!STATUS!.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo),
                    Request.TranslationStatus);

            ExFreePoolWithTag(BiosResource, PEP_POOL_TAG);
            *Status = Request.TranslationStatus;
            goto ReturnBiosResourceEnd;

        }

        //
        // N.B. ACPI_METHOD_SET_ARGUMENT_BUFFER will copy the buffer as well.
        //

        ACPI_METHOD_SET_ARGUMENT_BUFFER(Arguments,
                                        BiosResource,
                                        (USHORT)Request.OutputBufferSize);

        ExFreePoolWithTag(BiosResource, PEP_POOL_TAG);

        //
        // Return the output argument count, size and status.
        //

        *Status = STATUS_SUCCESS;
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s <%s>: Successfully converted to BIOS resource.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo));
    }

ReturnBiosResourceEnd:
    return;
}

VOID
PepReturnAcpiData (
    _In_ PVOID Value,
    _In_ USHORT ValueType,
    _In_ ULONG ValueLength,
    _In_ BOOLEAN ReturnAsPackage,
    _Out_ PACPI_METHOD_ARGUMENT Arguments,
    _Inout_ PSIZE_T OutputArgumentSize,
    _Out_opt_ PULONG OutputArgumentCount,
    _Out_ PNTSTATUS Status,
    _In_opt_ PCHAR MethodName,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    )

/*++

Routine Description:

    This routine returns data of specific type back to PoFx.

Arguments:

    Value - Supplies the pointer to the data returned.

    ValueType - Supplies the type of the data.

    ValueLength - Supplies the length (raw, without ACPI method argument)
        of the data.

    ReturnAsPackage - Supplies a flag indicating whether to return the data
        in a package.

    Arguments - Supplies a pointer to receive the returned package.

    OutputArgumentSize - Supplies a pointer to receive the returned
        argument size.

    OutputArgumentCount - Supplies an optional pointer to receive the returned
        argument number.

    Status - Supplies a pointer to receive the evaluation result.

    MethodName - Supplies an optional string that names the native method
        used for logging.

    DebugInfo - Supplies an optional string that contains the debugging
        information to be included in the log.

    CompleteResult - Supplies a pointer to receive the complete result.

Return Value:

    None.

--*/

{

    PACPI_METHOD_ARGUMENT ArgumentLocal;
    ULONG RequiredSize;
    PULONG ValueAsInteger;
    PUCHAR ValueAsString;

    TraceEvents(INFO,
                DBG_PEP,
                "%s <%s> [%s]: Start processing.\n",
                __FUNCTION__,
                NAME_DEBUG_INFO(DebugInfo),
                NAME_NATIVE_METHOD(MethodName));

    RequiredSize = ACPI_METHOD_ARGUMENT_LENGTH(ValueLength);
    if (ReturnAsPackage != FALSE) {
        ArgumentLocal = (PACPI_METHOD_ARGUMENT)&Arguments->Data[0];
    } else {
        ArgumentLocal = Arguments;
    }

    if ((*OutputArgumentSize) < RequiredSize) {
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s <%s> [%s]: "
                    "Buffer too small, Required=%d, Provided=%d.\n",
                    __FUNCTION__,
                    NAME_DEBUG_INFO(DebugInfo),
                    NAME_NATIVE_METHOD(MethodName),
                    (ULONG)RequiredSize,
                    (ULONG)(*OutputArgumentSize));

        *OutputArgumentSize = RequiredSize;
        *Status = STATUS_BUFFER_TOO_SMALL;
        if (OutputArgumentCount != NULL) {
            *OutputArgumentCount = 0;
        }

    } else {

        //
        // Set the returned value base on the type.
        //

        switch (ValueType) {
        case ACPI_METHOD_ARGUMENT_INTEGER:
            ValueAsInteger = (PULONG)Value;
            ACPI_METHOD_SET_ARGUMENT_INTEGER(ArgumentLocal, (*ValueAsInteger));
            TraceEvents(INFO,
                        DBG_PEP,
                        "%s <%s> [%s]: Returntype = Integer, Result = %#x.\n",
                        __FUNCTION__,
                        NAME_DEBUG_INFO(DebugInfo),
                        NAME_NATIVE_METHOD(MethodName),
                        (ULONG)ArgumentLocal->Argument);

            break;

        case ACPI_METHOD_ARGUMENT_STRING:
            ValueAsString = (PUCHAR)Value;

            //
            // N.B. ACPI_METHOD_SET_ARGUMENT_STRING will copy the string as
            //      well.
            //      ACPI_METHOD_SET_ARGUMENT_STRING currently has a bug:
            //      error C4267: '=' : conversion from 'size_t' to 'USHORT',
            //      possible loss of data.
            //
            //      error C4057: char * is different from PUCHAR.
            //

            #pragma warning(suppress:4267 4057 4244)
            ACPI_METHOD_SET_ARGUMENT_STRING(ArgumentLocal, ValueAsString);
            TraceEvents(INFO,
                        DBG_PEP,
                        "%s <%s> [%s]: ReturnType = String, Result = %s.\n",
                        __FUNCTION__,
                        NAME_DEBUG_INFO(DebugInfo),
                        NAME_NATIVE_METHOD(MethodName),
                        (PSTR)&ArgumentLocal->Data[0]);

            break;

        case ACPI_METHOD_ARGUMENT_BUFFER:
            ValueAsString = (PUCHAR)Value;
            ACPI_METHOD_SET_ARGUMENT_BUFFER(ArgumentLocal,
                                            ValueAsString,
                                            (USHORT)ValueLength);

            TraceEvents(INFO,
                        DBG_PEP,
                        "%s <%s> [%s]: ReturnType = Buffer.\n",
                        __FUNCTION__,
                        NAME_DEBUG_INFO(DebugInfo),
                        NAME_NATIVE_METHOD(MethodName));

            break;

        default:
            NT_ASSERT(FALSE);
            return;
        }

        if (ReturnAsPackage != FALSE) {
            Arguments->Type = ACPI_METHOD_ARGUMENT_PACKAGE_EX;
            Arguments->DataLength =
                ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(ArgumentLocal);
        }

        //
        // Return the output argument count, size and status.
        //

        if (OutputArgumentCount != NULL) {
            *OutputArgumentCount = 1;
        }

        *OutputArgumentSize =
            ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(Arguments);

        *Status = STATUS_SUCCESS;
    }

    *CompleteResult = PEP_NOTIFICATION_HANDLER_COMPLETE;
    return;
}


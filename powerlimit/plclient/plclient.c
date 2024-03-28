/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    plclient.c

Abstract:

    This module implements power limit related operations for the simulated power
    limit client driver.

--*/

//-------------------------------------------------------------------- Includes

#include "plclient.h"

//--------------------------------------------------------------------- Pragmas

#pragma alloc_text(PAGE, InitPowerLimitValues)
#pragma alloc_text(PAGE, CleanupPowerLimitValues)
#pragma alloc_text(PAGE, PLCQueryAttributes)
#pragma alloc_text(PAGE, PLCSetLimits)
#pragma alloc_text(PAGE, PLCQueryLimitValues)

//------------------------------------------------------------------- Functions

_Use_decl_annotations_
NTSTATUS
InitPowerLimitValues (
    PFDO_DATA DevExt
    )

/*++

Routine Description:

    This routine initializes simulated limit values and attributes for the supplied
    device extension.

Parameters Description:

    DevExt - Supplies a pointer to the device extension to be udpated.

Return Value:

    NTSTATUS.

--*/

{

    ULONG DomainId;
    ULONG Index;
    PPOWER_LIMIT_ATTRIBUTES LimitAttributes;
    ULONG LimitCount;
    PPOWER_LIMIT_VALUE LimitValues;
    NTSTATUS Status;
    ULONG Type;

    PAGED_CODE();

    LimitAttributes = NULL;
    LimitValues = NULL;
    LimitCount = PLCLIENT_DEFAULT_DOMAIN_COUNT * PLCLIENT_DEFAULT_LIMIT_COUNT_PER_DOMAIN;
    LimitAttributes = ExAllocatePool2(POOL_FLAG_PAGED,
                                      LimitCount * sizeof(POWER_LIMIT_ATTRIBUTES),
                                      PLCLIENT_TAG);

    LimitValues = ExAllocatePool2(POOL_FLAG_PAGED,
                                  LimitCount * sizeof(POWER_LIMIT_VALUE),
                                  PLCLIENT_TAG);

    if ((LimitAttributes == NULL) || (LimitValues == NULL)) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto InitPowerLimitValuesEnd;
    }

    for (DomainId = 0; DomainId < PLCLIENT_DEFAULT_DOMAIN_COUNT; DomainId += 1) {
        for (Type = 0; Type < PLCLIENT_DEFAULT_LIMIT_COUNT_PER_DOMAIN; Type += 1) {
            Index = (PLCLIENT_DEFAULT_LIMIT_COUNT_PER_DOMAIN * DomainId) + Type;

            //
            // Set init attributes.
            //

            LimitAttributes[Index].Type = Type;
            LimitAttributes[Index].DomainId = DomainId;
            LimitAttributes[Index].MaxValue = PLCLIENT_DEFAULT_MAX_VALUE;
            LimitAttributes[Index].MinValue = PLCLIENT_DEFAULT_MIN_VALUE;
            LimitAttributes[Index].DefaultACValue = POWER_LIMIT_VALUE_NO_CONTROL;
            LimitAttributes[Index].DefaultDCValue = POWER_LIMIT_VALUE_NO_CONTROL;

            if (Type == PowerLimitContinuous) {
                LimitAttributes[Index].MinTimeParameter = PLCLIENT_DEFAULT_MIN_VALUE;
                LimitAttributes[Index].MaxTimeParameter = PLCLIENT_DEFAULT_MAX_VALUE;
                LimitAttributes[Index].Flags.SupportTimeParameter = 1;
            }

            //
            // Set init values.
            //

            LimitValues[Index].Type = Type;
            LimitValues[Index].DomainId = DomainId;
            LimitValues[Index].TargetValue = POWER_LIMIT_VALUE_NO_CONTROL;
            LimitValues[Index].TimeParameter = POWER_LIMIT_VALUE_NO_CONTROL;
        }
    }

    DevExt->LimitCount = LimitCount;
    DevExt->LimitAttributes = LimitAttributes;
    DevExt->LimitValues = LimitValues;
    LimitAttributes = NULL;
    LimitValues = NULL;
    Status = STATUS_SUCCESS;

InitPowerLimitValuesEnd:
    if (LimitAttributes != NULL) {
        ExFreePoolWithTag(LimitAttributes, PLCLIENT_TAG);
        LimitAttributes = NULL;
    }

    if (LimitValues != NULL) {
        ExFreePoolWithTag(LimitValues, PLCLIENT_TAG);
        LimitValues = NULL;
    }

    return Status;
}

_Use_decl_annotations_
VOID
CleanupPowerLimitValues (
    PFDO_DATA DevExt
    )

/*++

Routine Description:

    This routine cleans up simulated limit values and attributes for the supplied
    device extension.

Parameters Description:

    DevExt - Supplies a pointer to the device extension to be udpated.

Return Value:

    NTSTATUS.

--*/

{

    PAGED_CODE();

    if (DevExt == NULL) {
        goto CleanupPowerLimitValuesEnd;
    }

    if (DevExt->LimitAttributes != NULL) {
        ExFreePoolWithTag(DevExt->LimitAttributes, PLCLIENT_TAG);
        DevExt->LimitAttributes = NULL;
    }

    if (DevExt->LimitValues != NULL) {
        ExFreePoolWithTag(DevExt->LimitValues, PLCLIENT_TAG);
        DevExt->LimitValues = NULL;
    }

    DevExt->LimitCount = 0;

CleanupPowerLimitValuesEnd:
    return;
}

_Use_decl_annotations_
NTSTATUS
PLCQueryAttributes (
    PVOID Context,
    ULONG BufferCount,
    PVOID Buffer,
    PULONG AttributeCount
    )

/*++

Routine Description:

    This is the callback function which returns power limit attributes.

Parameters Description:

    Context - Supplies a pointer to the device handle.

    BufferCount - Supplies count of Buffer entries.

    Buffer - Supplies a pointer to the buffer to store power limit attributes.

    AttributeCount - Supplies a pointer to save the number of attributes.

Return Value:

    Returns STATUS_BUFFER_TOO_SMALL if the supplied buffer is not big enough, otherwise
    other NTSTATUS values.

--*/

{

    PFDO_DATA DevExt;
    WDFDEVICE DeviceHandle;
    BOOLEAN ReleaseLock;
    NTSTATUS Status;

    PAGED_CODE();

    ReleaseLock = FALSE;
    if (Context == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto QueryAttributesEnd;
    }

    DeviceHandle = (WDFDEVICE)Context;
    DevExt = GetDeviceExtension(DeviceHandle);
    AcquireGlobalMutex();
    ReleaseLock = TRUE;
    if (BufferCount < DevExt->LimitCount) {
        if (AttributeCount != NULL) {
            *AttributeCount = DevExt->LimitCount;
        }

        Status = STATUS_BUFFER_TOO_SMALL;
        goto QueryAttributesEnd;
    }

    RtlCopyMemory(Buffer,
                  DevExt->LimitAttributes,
                  sizeof(POWER_LIMIT_ATTRIBUTES) * DevExt->LimitCount);

    Status = STATUS_SUCCESS;

QueryAttributesEnd:
    if (ReleaseLock != FALSE) {
        ReleaseGlobalMutex();
    }

    return Status;
}

_Use_decl_annotations_
NTSTATUS
PLCSetLimits (
    PVOID Context,
    ULONG ValueCount,
    PVOID Values
    )

/*++

Routine Description:

    This is the callback function which takes requests to set power limit values.

Parameters Description:

    Context - Supplies a pointer to the device handle.

    ValueCount - Supplies count of Value entries.

    Values - Supplies a pointer to the buffer contains values to be updated.

Return Value:

    NTSTATUS.

--*/

{

    PPOWER_LIMIT_ATTRIBUTES Attributes;
    PFDO_DATA DevExt;
    WDFDEVICE DeviceHandle;
    ULONG Index;
    BOOLEAN ReleaseLock;
    NTSTATUS Status;
    BOOLEAN Valid;
    PPOWER_LIMIT_VALUE ValueBuffer;
    ULONG ValueIndex;

    PAGED_CODE();

    ReleaseLock = FALSE;
    if ((Context == NULL) || (ValueCount == 0) || (Values == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto SetLimitsEnd;
    }

    ValueBuffer = (PPOWER_LIMIT_VALUE)Values;
    DeviceHandle = (WDFDEVICE)Context;
    DevExt = GetDeviceExtension(DeviceHandle);
    AcquireGlobalMutex();
    ReleaseLock = TRUE;

    //
    // Sanity check on proposed values before update.
    //

    if (DevExt->LimitCount < ValueCount) {
        Status = STATUS_INVALID_PARAMETER;
        goto SetLimitsEnd;
    }

    //
    // N.B. On a production driver, those values should be used as power limit targets 
    //      for the hardware.
    //

    for (Index = 0; Index < ValueCount; Index += 1) {
        Valid = FALSE;
        for (ValueIndex = 0; ValueIndex < DevExt->LimitCount; ValueIndex += 1) {
            if ((ValueBuffer[Index].Type != DevExt->LimitAttributes[ValueIndex].Type) ||
                (ValueBuffer[Index].DomainId != DevExt->LimitAttributes[ValueIndex].DomainId)) {

                continue;
            }

            Attributes = &DevExt->LimitAttributes[ValueIndex];
            if ((ValueBuffer[Index].TargetValue == POWER_LIMIT_VALUE_NO_CONTROL) ||
                ((ValueBuffer[Index].TargetValue >= Attributes->MinValue) &&
                 (ValueBuffer[Index].TargetValue <= Attributes->MaxValue))) {

                Valid = TRUE;
            }

            if (ValueBuffer[Index].TimeParameter != POWER_LIMIT_VALUE_NO_CONTROL) {
                if ((Attributes->Flags.SupportTimeParameter != 0) &&
                    (ValueBuffer[Index].TimeParameter >= Attributes->MinTimeParameter) &&
                    (ValueBuffer[Index].TimeParameter <= Attributes->MaxTimeParameter)) {

                    Valid = TRUE;
                }
            }

            break;
        }

        //
        // N.B. Bail out if this proposed value is not valid.
        //

        if (Valid == FALSE) {
            Status = STATUS_INVALID_PARAMETER;
            goto SetLimitsEnd;
        }
    }

    for (Index = 0; Index < ValueCount; Index += 1) {
        for (ValueIndex = 0; ValueIndex < DevExt->LimitCount; ValueIndex += 1) {
            if ((ValueBuffer[Index].Type != DevExt->LimitValues[ValueIndex].Type) ||
                (ValueBuffer[Index].DomainId != DevExt->LimitValues[ValueIndex].DomainId)) {

                continue;
            }

            DevExt->LimitValues[ValueIndex].TargetValue = ValueBuffer[Index].TargetValue;
            DevExt->LimitValues[ValueIndex].TimeParameter = ValueBuffer[Index].TimeParameter;
            break;
        }
    }

    Status = STATUS_SUCCESS;

SetLimitsEnd:
    if (ReleaseLock != FALSE) {
        ReleaseGlobalMutex();
    }

    return Status;
}

_Use_decl_annotations_
NTSTATUS
PLCQueryLimitValues (
    PVOID Context,
    ULONG ValueCount,
    PVOID Values
    )

/*++

Routine Description:

    This is the callback function which returns power limit values.

Parameters Description:

    Context - Supplies a pointer to the device handle.

    ValueCount - Supplies count of Value entries.

    Values - Supplies a pointer to the buffer to store power limit values.

Return Value:

    Returns STATUS_BUFFER_TOO_SMALL if the supplied buffer is not big enough, otherwise
    other NTSTATUS values.

--*/

{

    PFDO_DATA DevExt;
    WDFDEVICE DeviceHandle;
    BOOLEAN ReleaseLock;
    NTSTATUS Status;

    PAGED_CODE();

    ReleaseLock = FALSE;
    if ((Context == NULL) || (ValueCount == 0) || (Values == NULL)){
        Status = STATUS_INVALID_PARAMETER;
        goto QueryLimitsEnd;
    }

    DeviceHandle = (WDFDEVICE)Context;
    DevExt = GetDeviceExtension(DeviceHandle);
    AcquireGlobalMutex();
    ReleaseLock = TRUE;
    if (ValueCount < DevExt->LimitCount) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto QueryLimitsEnd;
    }

    RtlCopyMemory(Values,
                  DevExt->LimitValues,
                  sizeof(POWER_LIMIT_VALUE) * DevExt->LimitCount);

    Status = STATUS_SUCCESS;

QueryLimitsEnd:
    if (ReleaseLock != FALSE) {
        ReleaseGlobalMutex();
    }

    return Status;
}

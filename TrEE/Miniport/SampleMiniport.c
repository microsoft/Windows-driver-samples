/*++

Copyright (c) Microsoft Corporation

Module Name:

    sample.c

Abstract:

    This file demonstrates a simple TrEE miniport.

Environment:

    Kernel mode

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <wdmguid.h>
#include <ntstrsafe.h>
#include <TrustedRuntimeClx.h>

//
// Header file <TrEEVariableService.h> requires prior definition of a CHAR16.
// For now we need to define this in sample.
//
typedef UINT16 CHAR16;

#include <TrEEVariableService.h>
#include "sampleminiport.h"
#include "..\inc\SampleSecureService.h"
#include "..\inc\SampleOSService.h"

#define SDDL_SAMPLE_TEST2_SERVICE L"D:P(A;;FRFW;;;WD)(A;;FRFW;;;RC)(A;;FRFW;;;AC)"

//
// Driver entry point
//

DRIVER_INITIALIZE DriverEntry;

//
// Device callbacks
//

EVT_WDF_DRIVER_UNLOAD DriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD TreeSampleEvtAddDevice;

//
// Class Extension callbacks
//

EVT_TR_CREATE_SECURE_DEVICE_CONTEXT TreeSampleCreateSecureDeviceContext;
EVT_TR_DESTROY_SECURE_DEVICE_CONTEXT TreeSampleDestroySecureDeviceContext;
EVT_TR_PREPARE_HARDWARE_SECURE_ENVIRONMENT TreeSamplePrepareHardwareSecureEnvironment;
EVT_TR_RELEASE_HARDWARE_SECURE_ENVIRONMENT TreeSampleReleaseHardwareSecureEnvironment;
EVT_TR_CONNECT_SECURE_ENVIRONMENT TreeSampleConnectSecureEnvironment;
EVT_TR_DISCONNECT_SECURE_ENVIRONMENT TreeSampleDisconnectSecureEnvironment;
EVT_TR_ENUMERATE_SECURE_SERVICES TreeSampleEnumerateSecureServices;
EVT_TR_PROCESS_OTHER_DEVICE_IO TreeSampleProcessOtherDeviceIo;
EVT_TR_CREATE_SECURE_SERVICE_CONTEXT TreeSampleCreateSecureServiceContext;
EVT_TR_QUERY_SERVICE_CALLBACKS TreeSampleQueryServiceCallbacks;

#pragma data_seg("PAGED")

TR_SECURE_DEVICE_CALLBACKS TreeSampleCallbacks = {
    TR_DEVICE_SERIALIZE_ALL_REQUESTS |
    TR_DEVICE_STACK_RESERVE_8K,

    &TreeSampleCreateSecureDeviceContext,
    &TreeSampleDestroySecureDeviceContext,

    &TreeSamplePrepareHardwareSecureEnvironment,
    &TreeSampleReleaseHardwareSecureEnvironment,

    &TreeSampleConnectSecureEnvironment,
    &TreeSampleDisconnectSecureEnvironment,

    &TreeSampleEnumerateSecureServices,
    &TreeSampleProcessOtherDeviceIo,

    &TreeSampleQueryServiceCallbacks
};

#pragma data_seg()

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, TreeSampleEvtAddDevice)
#pragma alloc_text(PAGE, TreeSampleCreateSecureDeviceContext)
#pragma alloc_text(PAGE, TreeSampleDestroySecureDeviceContext)
#pragma alloc_text(PAGE, TreeSamplePrepareHardwareSecureEnvironment)
#pragma alloc_text(PAGE, TreeSampleReleaseHardwareSecureEnvironment)
#pragma alloc_text(PAGE, TreeSampleEnumerateSecureServices)
#pragma alloc_text(PAGE, TreeSampleProcessOtherDeviceIo)
#pragma alloc_text(PAGE, TreeSampleQueryServiceCallbacks)

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )

/*++

    Routine Description:

        This is the initialization routine for the device driver. This routine
        creates the driver object for the TrEE miniport.

    Arguments:

        DriverObject - Supplies a pointer to driver object created by the
                       system.

        RegistryPath - Supplies a unicode string indentifying where the
                       parameters for this driver are located in the registry.

    Return Value:

        NTSTATUS code.

--*/

{

    WDFDRIVER Driver;
    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;

    Driver = NULL;

    //
    // Create the WDF driver object
    //

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, TreeSampleEvtAddDevice);
    DriverConfig.EvtDriverUnload = DriverUnload;

    Status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DriverConfig,
        &Driver);

    if (!NT_SUCCESS(Status)) {
        goto DriverEntryEnd;
    }


DriverEntryEnd:

    return Status;
}

_Use_decl_annotations_
VOID
DriverUnload(
    WDFDRIVER Driver
    )

/*++

    Routine Description:

        This is the cleanup function called when the driver is unloaded.

    Arguments:

        Driver - Supplies a handle to WDFDRIVER object created in DriverEntry.

    Return Value:

        None.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);
}

_Use_decl_annotations_
NTSTATUS
TreeSampleEvtAddDevice(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++

    Routine Description:

        This routine is called when the driver is being attached to a specific
        device.

    Arguments:

        Driver - Supplies a handle to the framework driver object.

        DeviceInit - Supplies a pointer to the device initialization parameters.

    Return Value:

        NTSTATUS code.

--*/

{

    WDFDEVICE Device;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    Status = TrSecureDeviceHandoffMasterDeviceControl(
                DeviceInit,
                &TreeSampleCallbacks,
                &Device);

    TrSecureDeviceLogMessage(Device,
                             STATUS_SEVERITY_INFORMATIONAL,
                             "Master device discovered\n");

    return Status;
}

_Use_decl_annotations_
NTSTATUS
TreeSampleCreateSecureDeviceContext(
    WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment is first started.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to store any context information
                        required for future calls.

    Return Value:

        NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    PTREE_SAMPLE_DEVICE_CONTEXT MasterContext;
    NTSTATUS Status;
    DECLARE_CONST_UNICODE_STRING(SymbolicLink, L"\\DosDevices\\SampleTrEEDriver");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                            TREE_SAMPLE_DEVICE_CONTEXT);

    Status = WdfObjectAllocateContext(MasterDevice, &ContextAttributes, &MasterContext);
    if (!NT_SUCCESS(Status)) {
        goto TreeSampleCreateSecureDeviceContextEnd;
    }

    MasterContext->MasterDevice = MasterDevice;

    //
    // Create a symbolic link so that usermode program can access master device.
    //
    Status = WdfDeviceCreateSymbolicLink(MasterDevice, &SymbolicLink);
    if (!NT_SUCCESS(Status)) {
        goto TreeSampleCreateSecureDeviceContextEnd;
    }

TreeSampleCreateSecureDeviceContextEnd:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
TreeSampleDestroySecureDeviceContext(
    WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment is no longer used.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

    Return Value:

        NTSTATUS code.

--*/

{

    PTREE_SAMPLE_DEVICE_CONTEXT MasterContext;

    MasterContext = WdfObjectGet_TREE_SAMPLE_DEVICE_CONTEXT(MasterDevice);

    //
    // No member needs cleanup here. ServiceCollection will be automatically
    // reclaimed by the framework, since the parent object is going away.
    //

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TreeSamplePrepareHardwareSecureEnvironment(
    WDFDEVICE MasterDevice,
    WDFCMRESLIST RawResources,
    WDFCMRESLIST TranslatedResources
    )

/*++

    Routine Description:

        This routine is called to handle any resources used by a secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        RawResources - Supplies a pointer to the raw resources.

        TranslatedResources - Supplies a pointer to the translated resources.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(MasterDevice);
    UNREFERENCED_PARAMETER(RawResources);
    UNREFERENCED_PARAMETER(TranslatedResources);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TreeSampleReleaseHardwareSecureEnvironment(
    WDFDEVICE MasterDevice,
    WDFCMRESLIST TranslatedResources
    )

/*++

    Routine Description:

        This routine is called to handle the displosal of any resources used by
        a secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        TranslatedResources - Supplies a pointer to the translated resources.

    Return Value:

        NTSTATUS code.

--*/

{

    PAGED_CODE();

    UNREFERENCED_PARAMETER(MasterDevice);
    UNREFERENCED_PARAMETER(TranslatedResources);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TreeSampleConnectSecureEnvironment(
    WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment should be prepared
        for use either the first time or after a possible power state change.

        This routine must be marked as pagable, since it is called during power
        state transition.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(MasterDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TreeSampleDisconnectSecureEnvironment(
    WDFDEVICE MasterDevice
    )

/*++

    Routine Description:

        This routine is called when the secure environment should be prepared
        for a possible power state change.

        This routine must be marked as pagable, since it is called during power
        state transition.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

    Return Value:

        NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(MasterDevice);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
TreeSampleEnumerateSecureServices(
    WDFDEVICE MasterDevice,
    ULONG Index,
    PUCHAR SecureServiceDescription,
    ULONG *DescriptionSize
    )

/*++

    Routine Description:

        This routine is called to enumerate the supported secure services
        provided by this secure device. The zero-based Index parameter is used
        to determine which secure service information is being requested for.
        If Index is larger than the number of available services then
        STATUS_NO_MORE_ENTRIES is returned to indicate the end of the list has
        been reached.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        Index - Supplies the zero-based index for the secure service whose
                description is being requested.

        SecureServiceDescription - Supplies a pointer to a buffer to hold the
                                   secure service description. The description
                                   is of type TR_SECURE_SERVICE.

        DescriptionSize - Supplies a pointer to the size in bytes of
                          SecureServiceDescription on input, and holds the
                          number of bytes required on output.

    Return Value:

        NTSTATUS code.

--*/

{

    ULONG DescriptionSizeRequired;
    PTR_SECURE_SERVICE SecureService;
    PTR_SECURE_SERVICE_EXTENSION ServiceExtension;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(MasterDevice);

    SecureService = (PTR_SECURE_SERVICE)SecureServiceDescription;
    switch (Index) {
    case 0:
        //
        // [0] Test service
        // Major version = 1
        // Minor version = 0
        // No OS dependencies
        // Open to all except restricted code (default)
        //
        DescriptionSizeRequired = FIELD_OFFSET(TR_SECURE_SERVICE, Dependencies);
        if (*DescriptionSize < DescriptionSizeRequired) {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto TreeSampleEnumerateSecureServicesEnd;
        }

        SecureService->DescriptionSize = DescriptionSizeRequired;
        SecureService->ServiceGuid = GUID_SAMPLE_TEST_SERVICE;
        SecureService->MajorVersion = 1;
        SecureService->MinorVersion = 0;
        SecureService->ExtensionOffset = 0;
        SecureService->CountDependencies = 0;

        Status = STATUS_SUCCESS;
        break;

    case 1:
        //
        // [1] Test2 service
        // Major version = 1
        // Minor version = 0
        // OS dependency on GUID_ECHO_SERVICE and GUID_KERNEL_MEMORY_SERVICE
        // Open to all including restricted code
        //
        DescriptionSizeRequired = FIELD_OFFSET(TR_SECURE_SERVICE, Dependencies) +
                                  sizeof(TR_SECURE_DEPENDENCY_V1) * 2 +
                                  sizeof(TR_SECURE_SERVICE_EXTENSION) +
                                  sizeof(SDDL_SAMPLE_TEST2_SERVICE);

        if (*DescriptionSize < DescriptionSizeRequired) {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto TreeSampleEnumerateSecureServicesEnd;
        }

        SecureService->DescriptionSize = DescriptionSizeRequired;
        SecureService->ServiceGuid = GUID_SAMPLE_TEST2_SERVICE;
        SecureService->MajorVersion = 1;
        SecureService->MinorVersion = 0;
        SecureService->ExtensionOffset = FIELD_OFFSET(TR_SECURE_SERVICE, Dependencies) +
                                         sizeof(TR_SECURE_DEPENDENCY_V1) * 2;
        SecureService->CountDependencies = 2;
        SecureService->Dependencies[0].Type = TRSecureOSDependency;
        SecureService->Dependencies[0].Id = GUID_ECHO_SERVICE;
        SecureService->Dependencies[0].MaxRequired = 1;
        SecureService->Dependencies[1].Type = TRSecureOSDependency;
        SecureService->Dependencies[1].Id = GUID_KERNEL_MEMORY_SERVICE;
        SecureService->Dependencies[1].MaxRequired = 1;
        ServiceExtension = (PTR_SECURE_SERVICE_EXTENSION)(
                                ((ULONG_PTR)SecureService) +
                                SecureService->ExtensionOffset);

        ServiceExtension->ExtensionVersion = TR_SECURE_SERVICE_EXTENSION_VERSION;
        ServiceExtension->SecurityDescriptorStringOffset = SecureService->ExtensionOffset +
                                                           sizeof(TR_SECURE_SERVICE_EXTENSION);
        RtlCopyMemory((PVOID)(((ULONG_PTR)SecureService) + ServiceExtension->SecurityDescriptorStringOffset),
                      SDDL_SAMPLE_TEST2_SERVICE,
                      sizeof(SDDL_SAMPLE_TEST2_SERVICE));

        Status = STATUS_SUCCESS;
        break;

    default:
        DescriptionSizeRequired = 0;
        Status = STATUS_NO_MORE_ENTRIES;
        break;
    }

TreeSampleEnumerateSecureServicesEnd:
    *DescriptionSize = DescriptionSizeRequired;
    return Status;
}

_Use_decl_annotations_
VOID
TreeSampleProcessOtherDeviceIo(
    WDFDEVICE MasterDevice,
    WDFREQUEST Request
    )

/*++

    Routine Description:

        This routine is called when an unrecognized IO request is made to the
        device. This can be used to process private calls directly to the
        secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        Request - Supplies a pointer to the WDF request object.

    Return Value:

        NTSTATUS code.

--*/

{

    PWCHAR Buffer;
    size_t BufferSize;
    ULONG BytesWritten;
    WDF_REQUEST_PARAMETERS Parameters;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(MasterDevice);

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    BytesWritten = 0;
    switch (Parameters.Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SAMPLE_DBGPRINT:
        Status = WdfRequestRetrieveInputBuffer(Request,
                                               0,
                                               (PVOID*)&Buffer,
                                               &BufferSize);

        if (!NT_SUCCESS(Status)) {
            goto TreeSampleProcessOtherDeviceIoEnd;
        }

        //
        // Must be NULL-terminated
        //
        if (Buffer[BufferSize / sizeof(WCHAR) - 1] != L'\0') {
            Status = STATUS_INVALID_PARAMETER;
            goto TreeSampleProcessOtherDeviceIoEnd;
        }

        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_ERROR_LEVEL,
                   "[TrEEMiniportSample] %ws\n",
                   (PWSTR)Buffer);

        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

TreeSampleProcessOtherDeviceIoEnd:
    WdfRequestCompleteWithInformation(Request, Status, BytesWritten);
}

_Use_decl_annotations_
PTR_SECURE_SERVICE_CALLBACKS
TreeSampleQueryServiceCallbacks(
    WDFDEVICE MasterDevice,
    LPGUID ServiceGuid
    )

/*++

    Routine Description:

        This routine is called when an unrecognized IO request is made to the
        device. This can be used to process private calls directly to the
        secure device.

    Arguments:

        MasterDevice - Supplies a handle to the master device object.

        DeviceContext - Supplies a pointer to the context.

        Request - Supplies a pointer to the WDF request object.

    Return Value:

        NTSTATUS code.

--*/

{

    PTR_SECURE_SERVICE_CALLBACKS ServiceCallbacks;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(MasterDevice);

    if (IsEqualGUID(ServiceGuid, &GUID_SAMPLE_TEST_SERVICE)) {

        ServiceCallbacks = &TestServiceCallbacks;

    } else if (IsEqualGUID(ServiceGuid, &GUID_SAMPLE_TEST2_SERVICE)) {

        ServiceCallbacks = &Test2ServiceCallbacks;

    } else {

        ServiceCallbacks = NULL;
    }

    return ServiceCallbacks;
}

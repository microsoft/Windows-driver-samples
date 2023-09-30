/*++

Copyright (c) 1999 - 2003  Microsoft Corporation

Module Name:

    MetadataManagerInit.c

Abstract:

    This is the main module of the kernel mode filter driver implementing
    filter metadata management.


Environment:

    Kernel mode


--*/

#include "pch.h"

//
//  Global variables
//

FMM_GLOBAL_DATA Globals;


//
//  Local constants
//

#define FMM_UNSUPPORTED_DEVICE_CHARACS   FILE_FLOPPY_DISKETTE |         \
                                         FILE_READ_ONLY_DEVICE |        \
                                         FILE_VIRTUAL_VOLUME

//
//  Local function prototypes
//

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
FmmUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

VOID
FmmContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
FmmInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
FmmInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
FmmInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
FmmInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

#if DBG

typedef
NTSTATUS
(*PFN_IoOpenDriverRegistryKey) (
    PDRIVER_OBJECT     DriverObject,
    DRIVER_REGKEY_TYPE RegKeyType,
    ACCESS_MASK        DesiredAccess,
    ULONG              Flags,
    PHANDLE            DriverRegKey
    );

PFN_IoOpenDriverRegistryKey
FmmGetIoOpenDriverRegistryKey (
    VOID
    );

NTSTATUS
FmmOpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    );

VOID
FmmInitializeDebugLevel (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

#endif

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#if DBG
#pragma alloc_text(INIT, FmmGetIoOpenDriverRegistryKey)
#pragma alloc_text(INIT, FmmOpenServiceParametersKey)
#pragma alloc_text(INIT, FmmInitializeDebugLevel)
#endif

#pragma alloc_text(PAGE, FmmUnload)
#pragma alloc_text(PAGE, FmmContextCleanup)
#pragma alloc_text(PAGE, FmmInstanceSetup)
#pragma alloc_text(PAGE, FmmInstanceQueryTeardown)
#pragma alloc_text(PAGE, FmmInstanceTeardownStart)
#pragma alloc_text(PAGE, FmmInstanceTeardownComplete)
#endif


//
//  If we need to verify that the metadata file is indeed open whenever
//  a create suceeds on the volume, then we need to monitor all creates
//  not just DASD creates.

//  If that is not the case, then we are better off telling filter manager
//  to show us only DASD creates. That way we can avoid the performance
//  penalty of being called for all creates when we only have use for DASD
//  creates.
//


#if VERIFY_METADATA_OPENED

#define OPERATION_REGISTRATION_FLAGS_FOR_CREATE (FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO)

#else

#define OPERATION_REGISTRATION_FLAGS_FOR_CREATE (FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO | FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO)

#endif



//
//  Filters callback routines
//

FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      OPERATION_REGISTRATION_FLAGS_FOR_CREATE,
      FmmPreCreate,
      FmmPostCreate },

    { IRP_MJ_CLEANUP,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO | FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO,
      FmmPreCleanup,
      FmmPostCleanup },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO | FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO,
      FmmPreFSControl,
      FmmPostFSControl },

    { IRP_MJ_DEVICE_CONTROL,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      FmmPreDeviceControl,
      FmmPostDeviceControl },

    { IRP_MJ_SHUTDOWN,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      FmmPreShutdown,
      NULL },

    { IRP_MJ_PNP,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      FmmPrePnp,
      FmmPostPnp },

    { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_INSTANCE_CONTEXT,
      0,
      FmmContextCleanup,
      FMM_INSTANCE_CONTEXT_SIZE,
      FMM_INSTANCE_CONTEXT_TAG },

    { FLT_CONTEXT_END }
};

//
// Filters registration data structure
//

FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),                     //  Size
    FLT_REGISTRATION_VERSION,                       //  Version
    0,                                              //  Flags
    ContextRegistration,                            //  Context
    Callbacks,                                      //  Operation callbacks
    FmmUnload,                                      //  Filters unload routine
    FmmInstanceSetup,                               //  InstanceSetup routine
    FmmInstanceQueryTeardown,                       //  InstanceQueryTeardown routine
    FmmInstanceTeardownStart,                       //  InstanceTeardownStart routine
    FmmInstanceTeardownComplete,                    //  InstanceTeardownComplete routine
    NULL, NULL, NULL                                //  Unused naming support callbacks
};

//
//  Filter driver initialization and unload routines
//

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this filter driver. It registers
    itself with the filter manager and initializes all its global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    NTSTATUS status;

    //
    //  Default to NonPagedPoolNx for non paged pool allocations where supported.
    //

    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );


    RtlZeroMemory( &Globals, sizeof( Globals ) );

#if DBG

    //
    //  Initialize global debug level
    //

    FmmInitializeDebugLevel( DriverObject, RegistryPath );

#else

    UNREFERENCED_PARAMETER( RegistryPath );

#endif

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[Fmm]: Driver being loaded\n") );



    //
    //  Register with the filter manager
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &Globals.Filter );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Start filtering I/O
    //

    status = FltStartFiltering( Globals.Filter );

    if (!NT_SUCCESS( status )) {

        FltUnregisterFilter( Globals.Filter );
    }

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[Fmm]: Driver loaded complete (Status = 0x%08X)\n",
                status) );

    return status;
}

#if DBG

PFN_IoOpenDriverRegistryKey
FmmGetIoOpenDriverRegistryKey (
    VOID
    )
{
    static PFN_IoOpenDriverRegistryKey pIoOpenDriverRegistryKey = NULL;
    UNICODE_STRING FunctionName = {0};

    if (pIoOpenDriverRegistryKey == NULL) {

        RtlInitUnicodeString(&FunctionName, L"IoOpenDriverRegistryKey");

        pIoOpenDriverRegistryKey = (PFN_IoOpenDriverRegistryKey)MmGetSystemRoutineAddress(&FunctionName);
    }

    return pIoOpenDriverRegistryKey;
}

NTSTATUS
FmmOpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    )
/*++

Routine Description:

    This routine opens the service parameters key, using the isolation-compliant
    APIs when possible.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - The path key passed to the driver during DriverEntry.

    ServiceParametersKey - Returns a handle to the service parameters subkey.

Return Value:

    STATUS_SUCCESS if the function completes successfully.  Otherwise a valid
    NTSTATUS code is returned.

--*/
{
    NTSTATUS status;
    PFN_IoOpenDriverRegistryKey pIoOpenDriverRegistryKey;
    UNICODE_STRING Subkey;
    HANDLE ParametersKey = NULL;
    HANDLE ServiceRegKey = NULL;
    OBJECT_ATTRIBUTES Attributes;

    //
    //  Open the parameters key to read values from the INF, using the API to
    //  open the key if possible
    //

    pIoOpenDriverRegistryKey = FmmGetIoOpenDriverRegistryKey();

    if (pIoOpenDriverRegistryKey != NULL) {

        //
        //  Open the parameters key using the API
        //

        status = pIoOpenDriverRegistryKey( DriverObject,
                                           DriverRegKeyParameters,
                                           KEY_READ,
                                           0,
                                           &ParametersKey );

        if (!NT_SUCCESS( status )) {

            goto cleanup;
        }

    } else {

        //
        //  Open specified service root key
        //

        InitializeObjectAttributes( &Attributes,
                                    ServiceRegistryPath,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        status = ZwOpenKey( &ServiceRegKey,
                            KEY_READ,
                            &Attributes );

        if (!NT_SUCCESS( status )) {

            goto cleanup;
        }

        //
        //  Open the parameters key relative to service key path
        //

        RtlInitUnicodeString( &Subkey, L"Parameters" );

        InitializeObjectAttributes( &Attributes,
                                    &Subkey,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    ServiceRegKey,
                                    NULL );

        status = ZwOpenKey( &ParametersKey,
                            KEY_READ,
                            &Attributes );

        if (!NT_SUCCESS( status )) {

            goto cleanup;
        }
    }

    //
    //  Return value to caller
    //

    *ServiceParametersKey = ParametersKey;

cleanup:

    if (ServiceRegKey != NULL) {

        ZwClose( ServiceRegKey );
    }

    return status;

}

VOID
FmmInitializeDebugLevel (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the filter DebugLevel parameter from
    the registry.  This value will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - The path key passed to the driver during DriverEntry.

Return Value:

    None.

--*/
{
    HANDLE driverRegKey = NULL;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

    Globals.DebugLevel = DEBUG_TRACE_ERROR;

    //
    //  Open service parameters key to query values from.
    //

    status = FmmOpenServiceParametersKey( DriverObject,
                                          RegistryPath,
                                          &driverRegKey );

    if (!NT_SUCCESS( status )) {

        driverRegKey = NULL;
        goto cleanup;
    }

    //
    // Read the DebugFlags value from the registry.
    //

    RtlInitUnicodeString( &valueName, L"DebugLevel" );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              sizeof(buffer),
                              &resultLength );

    if (NT_SUCCESS( status )) {

        Globals.DebugLevel = *((PULONG) &(((PKEY_VALUE_PARTIAL_INFORMATION) buffer)->Data));
    }

cleanup:

    //
    //  Close the registry entry
    //

    if (driverRegKey != NULL) {
        ZwClose( driverRegKey );
    }
}

#endif

NTSTATUS
FmmUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this filter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unloaded indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[Fmm]: Unloading driver\n") );


    FltUnregisterFilter( Globals.Filter );
    Globals.Filter = NULL;

    return STATUS_SUCCESS;
}

VOID
FmmContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
{
    PFMM_INSTANCE_CONTEXT instanceContext;

    PAGED_CODE();

    switch(ContextType) {

    case FLT_INSTANCE_CONTEXT:

        instanceContext = Context;

        DebugTrace( DEBUG_TRACE_INFO,
                    ("[Fmm]: Cleaning up instance context for volume (Context = %p)\n",
                    instanceContext) );

        ExDeleteResourceLite( &instanceContext->MetadataResource );

        break;

    }

    DebugTrace( DEBUG_TRACE_INFO,
                ("[Fmm]: Context cleanup complete.\n") );

}

//
//  Instance setup/teardown routines.
//

NTSTATUS
FmmInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    PFMM_INSTANCE_CONTEXT instanceContext = NULL;
    PDEVICE_OBJECT diskDeviceObject;
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( VolumeDeviceType );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance setup started (Volume = %p, Instance = %p)\n",
                 FltObjects->Volume,
                 FltObjects->Instance) );

    //
    //  Check if the file system mounted is ntfs or fat
    //
    //  The sample picks NTFS, FAT and ReFS as examples. The metadata
    //  handling demostrated in the sample can be applied
    //  to any file system
    //

    if (VolumeFilesystemType != FLT_FSTYPE_NTFS && VolumeFilesystemType != FLT_FSTYPE_FAT && VolumeFilesystemType != FLT_FSTYPE_REFS) {

        //
        //  An unknown file system is mounted which we do not care
        //

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Fmm]: Unsupported file system mounted (Volume = %p, Instance = %p)\n",
                     FltObjects->Volume,
                     FltObjects->Instance) );

        status = STATUS_NOT_SUPPORTED;
        goto FmmInstanceSetupCleanup;
    }

    //
    //  Get the disk device object and make sure it is a disk device type and does not
    //  have any of the device characteristics we do not support.
    //
    //  The sample picks the device characteristics to demonstrate how to access and
    //  check the device characteristics in order to make a decision to attach. The
    //  metadata handling demostrated in the sample is not limited to the
    //  characteristics we have used in the sample.
    //

    status = FltGetDiskDeviceObject( FltObjects->Volume, &diskDeviceObject );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCES | DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to get device object (Volume = %p, Status = 0x%08X)\n",
                     FltObjects->Volume,
                     status) );
        goto FmmInstanceSetupCleanup;
    }

    if (diskDeviceObject->DeviceType != FILE_DEVICE_DISK ||
        FlagOn( diskDeviceObject->Characteristics, FMM_UNSUPPORTED_DEVICE_CHARACS )) {

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Fmm]: Unsupported device type or device characteristics (Volume = %p, Instance = %p DiskDeviceObjectDeviceTYpe = 0x%x, DiskDeviceObjectCharacteristics = 0x%x)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     diskDeviceObject->DeviceType,
                     diskDeviceObject->Characteristics) );

        ObDereferenceObject( diskDeviceObject );
        status = STATUS_NOT_SUPPORTED;
        goto FmmInstanceSetupCleanup;
    }

    ObDereferenceObject( diskDeviceObject );

    //
    //  Allocate and initialize the context for this volume
    //

    status = FltAllocateContext( FltObjects->Filter,
                                 FLT_INSTANCE_CONTEXT,
                                 FMM_INSTANCE_CONTEXT_SIZE,
                                 NonPagedPool,
                                 &instanceContext );

    if( !NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCES | DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to allocate instance context (Volume = %p, Instance = %p, Status = 0x%08X)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );

        goto FmmInstanceSetupCleanup;
    }

    FLT_ASSERT( instanceContext != NULL );

    RtlZeroMemory( instanceContext, FMM_INSTANCE_CONTEXT_SIZE );

    instanceContext->Flags = 0;
    instanceContext->Instance = FltObjects->Instance;
    instanceContext->FilesystemType = VolumeFilesystemType;
    instanceContext->Volume = FltObjects->Volume;
    ExInitializeResourceLite( &instanceContext->MetadataResource );


    //
    //  Set the instance context.
    //

    status = FltSetInstanceContext( FltObjects->Instance,
                                    FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                    instanceContext,
                                    NULL );

    if( !NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCES | DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to set instance context (Volume = %p, Instance = %p, Status = 0x%08X)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );
        goto FmmInstanceSetupCleanup;
    }

    //
    //  Acquire exclusive access to the instance context
    //

    FmmAcquireResourceExclusive( &instanceContext->MetadataResource );

    //
    //  Sanity - the instance context cannot be in a transition state during instance setup
    //

    FLT_ASSERT( !FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION) );

    //
    //  Open the filter metadata on disk
    //
    //  The sample will attach to volume if it finds its metadata file on the volume.
    //  If this is a manual attachment then the sample filter will create its metadata
    //  file and attach to the volume.
    //

    status = FmmOpenMetadata( instanceContext,
                              BooleanFlagOn( Flags, FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT ) );

    //
    //  Relinquish exclusive access to the instance context
    //

    FmmReleaseResource( &instanceContext->MetadataResource );

    if (!NT_SUCCESS( status )) {

        goto FmmInstanceSetupCleanup;
    }


FmmInstanceSetupCleanup:

    //
    //  If FltAllocateContext suceeded then we MUST release the context,
    //  irrespective of whether FltSetInstanceContext suceeded or not.
    //
    //  FltAllocateContext increments the ref count by one.
    //  A successful FltSetInstanceContext increments the ref count by one
    //  and also associates the context with the file system object
    //
    //  FltReleaseContext decrements the ref count by one.
    //
    //  When FltSetInstanceContext succeeds, calling FltReleaseContext will
    //  leave the context with a ref count of 1 corresponding to the internal
    //  reference to the context from the file system structures
    //
    //  When FltSetInstanceContext fails, calling FltReleaseContext will
    //  leave the context with a ref count of 0 which is correct since
    //  there is no reference to the context from the file system structures
    //

    if ( instanceContext != NULL ) {

        FltReleaseContext( instanceContext );
    }


    if (NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Fmm]: Instance setup complete (Volume = %p, Instance = %p). Filter will attach to the volume.\n",
                     FltObjects->Volume,
                     FltObjects->Instance) );
    } else {

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Fmm]: Instance setup complete (Volume = %p, Instance = %p). Filter will not attach to the volume.\n",
                     FltObjects->Volume,
                     FltObjects->Instance) );
    }

    //
    //  If this is an automatic attachment (mount, load, etc) and we are not
    //  attaching to this volume because we do not support attaching to this
    //  volume, then simply return STATUS_FLT_DO_NOT_ATTACH. If we return
    //  anything else fltmgr logs an event log indicating failure to attach.
    //  Since this failure to attach is not really an error, we do not want
    //  this failure to be logged as an error in the event log. For all other
    //  error codes besides the ones we consider "normal", if is ok for fltmgr
    //  to actually log the failure to attach.
    //
    //  If this is a manual attach attempt that we have failed then we want to
    //  give the user a clear indication of why the attachment failed. Hence in
    //  this case, we will not override the error status with STATUS_FLT_DO_NOT_ATTACH
    //  irrespective of the cause of the failure to attach
    //

    if (status == STATUS_NOT_SUPPORTED &&
       !FlagOn( Flags, FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT )) {

        status = STATUS_FLT_DO_NOT_ATTACH;
    }

    return status;
}


NTSTATUS
FmmInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance query teardown started (Instance = %p)\n",
                 FltObjects->Instance) );


    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance query teadown ended (Instance = %p)\n",
                 FltObjects->Instance) );
    return STATUS_SUCCESS;
}


VOID
FmmInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is been deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance teardown start started (Instance = %p)\n",
                 FltObjects->Instance) );


    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance teardown start ended (Instance = %p)\n",
                 FltObjects->Instance) );
}


VOID
FmmInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is been deleted.

Return Value:

    None.

--*/
{
    PFMM_INSTANCE_CONTEXT instanceContext;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance teardown complete started (Instance = %p)\n",
                 FltObjects->Instance) );

    status = FltGetInstanceContext( FltObjects->Instance,
                                    &instanceContext );

    if (NT_SUCCESS( status )) {

        //
        //  Acquire exclusive access to the instance context
        //

        FmmAcquireResourceExclusive( &instanceContext->MetadataResource );

        //
        //  Sanity - the instance context cannot be in a transition state during instance teardown complete
        //

        FLT_ASSERT( !FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_TRANSITION) );


        if (FlagOn( instanceContext->Flags, INSTANCE_CONTEXT_F_METADATA_OPENED )) {

            //
            //  Close the metadata file
            //

            FmmCloseMetadata( instanceContext );
        }


        //
        //  Relinquish exclusive access to the instance context
        //

        FmmReleaseResource( &instanceContext->MetadataResource );

        FltReleaseContext( instanceContext );
    }

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Fmm]: Instance teardown complete ended (Instance = %p)\n",
                 FltObjects->Instance) );
}


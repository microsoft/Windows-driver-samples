/*++

Copyright (c) 1999 - 2003  Microsoft Corporation

Module Name:

    ContextInit.c

Abstract:

    This is the main module of the kernel mode filter driver implementing
    the context sample.


Environment:

    Kernel mode


--*/

#include "pch.h"

//
//  Global variables
//

CTX_GLOBAL_DATA Globals;


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
CtxUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

VOID
CtxContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
CtxInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
CtxInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
CtxInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
CtxInstanceTeardownComplete (
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
CtxGetIoOpenDriverRegistryKey (
    VOID
    );

NTSTATUS
CtxOpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    );

VOID
CtxInitializeDebugLevel (
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
#pragma alloc_text(INIT, CtxGetIoOpenDriverRegistryKey)
#pragma alloc_text(INIT, CtxOpenServiceParametersKey)
#pragma alloc_text(INIT, CtxInitializeDebugLevel)
#endif

#pragma alloc_text(PAGE, CtxUnload)
#pragma alloc_text(PAGE, CtxContextCleanup)
#pragma alloc_text(PAGE, CtxInstanceSetup)
#pragma alloc_text(PAGE, CtxInstanceQueryTeardown)
#pragma alloc_text(PAGE, CtxInstanceTeardownStart)
#pragma alloc_text(PAGE, CtxInstanceTeardownComplete)
#endif


//
//  Filters callback routines
//

FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      CtxPreCreate,
      CtxPostCreate },

    { IRP_MJ_CLEANUP,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      CtxPreCleanup,
      NULL },

    { IRP_MJ_CLOSE,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      CtxPreClose,
      NULL },

    { IRP_MJ_SET_INFORMATION,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      CtxPreSetInfo,
      CtxPostSetInfo },

    { IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_INSTANCE_CONTEXT,
      0,
      CtxContextCleanup,
      CTX_INSTANCE_CONTEXT_SIZE,
      CTX_INSTANCE_CONTEXT_TAG },

    { FLT_FILE_CONTEXT,
      0,
      CtxContextCleanup,
      CTX_FILE_CONTEXT_SIZE,
      CTX_FILE_CONTEXT_TAG },

    { FLT_STREAM_CONTEXT,
      0,
      CtxContextCleanup,
      CTX_STREAM_CONTEXT_SIZE,
      CTX_STREAM_CONTEXT_TAG },

    { FLT_STREAMHANDLE_CONTEXT,
      0,
      CtxContextCleanup,
      CTX_STREAMHANDLE_CONTEXT_SIZE,
      CTX_STREAMHANDLE_CONTEXT_TAG },

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
    CtxUnload,                                      //  Filters unload routine
    CtxInstanceSetup,                               //  InstanceSetup routine
    CtxInstanceQueryTeardown,                       //  InstanceQueryTeardown routine
    CtxInstanceTeardownStart,                       //  InstanceTeardownStart routine
    CtxInstanceTeardownComplete,                    //  InstanceTeardownComplete routine
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

    CtxInitializeDebugLevel( DriverObject, RegistryPath );

#else

    UNREFERENCED_PARAMETER( RegistryPath );

#endif

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[Ctx]: Driver being loaded\n") );



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
                ("[Ctx]: Driver loaded complete (Status = 0x%08X)\n",
                status) );

    return status;
}

#if DBG

PFN_IoOpenDriverRegistryKey
CtxGetIoOpenDriverRegistryKey (
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
CtxOpenServiceParametersKey (
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

    pIoOpenDriverRegistryKey = CtxGetIoOpenDriverRegistryKey();

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
CtxInitializeDebugLevel (
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

    status = CtxOpenServiceParametersKey( DriverObject,
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
CtxUnload (
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
                ("[Ctx]: Unloading driver\n") );


    FltUnregisterFilter( Globals.Filter );
    Globals.Filter = NULL;

    return STATUS_SUCCESS;
}

VOID
CtxContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
{
    PCTX_INSTANCE_CONTEXT instanceContext;
    PCTX_FILE_CONTEXT fileContext;
    PCTX_STREAM_CONTEXT streamContext;
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext;

    PAGED_CODE();

    switch(ContextType) {

    case FLT_INSTANCE_CONTEXT:

        instanceContext = (PCTX_INSTANCE_CONTEXT) Context;

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Cleaning up instance context for volume %wZ (Context = %p)\n",
                     &instanceContext->VolumeName,
                     Context) );

        //
        //  Here the filter should free memory or synchronization objects allocated to
        //  objects within the instance context. The instance context itself should NOT
        //  be freed. It will be freed by Filter Manager when the ref count on the
        //  context falls to zero.
        //

        CtxFreeUnicodeString( &instanceContext->VolumeName );

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Instance context cleanup complete.\n") );

        break;


    case FLT_FILE_CONTEXT:

        fileContext = (PCTX_FILE_CONTEXT) Context;

        DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Cleaning up file context for file %wZ (FileContext = %p)\n",
                     &fileContext->FileName,
                     fileContext) );


        //
        //  Free the file name
        //

        if (fileContext->FileName.Buffer != NULL) {

            CtxFreeUnicodeString(&fileContext->FileName);
        }

        DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                    ("[Ctx]: File context cleanup complete.\n") );

        break;

    case FLT_STREAM_CONTEXT:

        streamContext = (PCTX_STREAM_CONTEXT) Context;

        DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: Cleaning up stream context for file %wZ (StreamContext = %p) \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                     &streamContext->FileName,
                     streamContext,
                     streamContext->CreateCount,
                     streamContext->CleanupCount,
                     streamContext->CloseCount) );

        //
        //  Delete the resource and memory the memory allocated for the resource
        //

        if (streamContext->Resource != NULL) {

            ExDeleteResourceLite( streamContext->Resource );
            CtxFreeResource( streamContext->Resource );
        }

        //
        //  Free the file name
        //

        if (streamContext->FileName.Buffer != NULL) {

            CtxFreeUnicodeString(&streamContext->FileName);
        }

        DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: Stream context cleanup complete.\n") );

        break;

    case FLT_STREAMHANDLE_CONTEXT:

        streamHandleContext = (PCTX_STREAMHANDLE_CONTEXT) Context;

        DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Cleaning up stream handle context for file %wZ (StreamContext = %p)\n",
                     &streamHandleContext->FileName,
                     streamHandleContext) );

        //
        //  Delete the resource and memory the memory allocated for the resource
        //

        if (streamHandleContext->Resource != NULL) {

            ExDeleteResourceLite( streamHandleContext->Resource );
            CtxFreeResource( streamHandleContext->Resource );
        }

        //
        //  Free the file name
        //

        if (streamHandleContext->FileName.Buffer != NULL) {

            CtxFreeUnicodeString(&streamHandleContext->FileName);
        }

        DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Stream handle context cleanup complete.\n") );

        break;

    }

}

//
//  Instance setup/teardown routines.
//

NTSTATUS
CtxInstanceSetup (
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
    PCTX_INSTANCE_CONTEXT instanceContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG volumeNameLength;

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Ctx]: Instance setup started (Volume = %p, Instance = %p)\n",
                 FltObjects->Volume,
                 FltObjects->Instance) );


    //
    //  Allocate and initialize the context for this volume
    //


    //
    //  Allocate the instance context
    //

    DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                ("[Ctx]: Allocating instance context (Volume = %p, Instance = %p)\n",
                 FltObjects->Volume,
                 FltObjects->Instance) );

    status = FltAllocateContext( FltObjects->Filter,
                                 FLT_INSTANCE_CONTEXT,
                                 CTX_INSTANCE_CONTEXT_SIZE,
                                 NonPagedPool,
                                 &instanceContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to allocate instance context (Volume = %p, Instance = %p, Status = 0x%x)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );

        goto CtxInstanceSetupCleanup;
    }

    //
    //  Get the NT volume name length
    //

    status = FltGetVolumeName( FltObjects->Volume, NULL, &volumeNameLength );

    if( !NT_SUCCESS( status ) &&
        (status != STATUS_BUFFER_TOO_SMALL) ) {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Unexpected failure in FltGetVolumeName. (Volume = %p, Instance = %p, Status = 0x%x)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );

        goto CtxInstanceSetupCleanup;
    }

    //
    //  Allocate a string big enough to take the volume name
    //

    instanceContext->VolumeName.MaximumLength = (USHORT) volumeNameLength;
    status = CtxAllocateUnicodeString( &instanceContext->VolumeName );

    if( !NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to allocate volume name string. (Volume = %p, Instance = %p, Status = 0x%x)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );

        goto CtxInstanceSetupCleanup;
    }

    //
    //  Get the NT volume name
    //

    status = FltGetVolumeName( FltObjects->Volume, &instanceContext->VolumeName, &volumeNameLength );

    if( !NT_SUCCESS( status ) ) {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Unexpected failure in FltGetVolumeName. (Volume = %p, Instance = %p, Status = 0x%x)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );

        goto CtxInstanceSetupCleanup;
    }


    instanceContext->Instance = FltObjects->Instance;
    instanceContext->Volume = FltObjects->Volume;

    //
    //  Set the instance context.
    //

    DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                ("[Ctx]: Setting instance context %p for volume %wZ (Volume = %p, Instance = %p)\n",
                 instanceContext,
                 &instanceContext->VolumeName,
                 FltObjects->Volume,
                 FltObjects->Instance) );

    status = FltSetInstanceContext( FltObjects->Instance,
                                    FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                    instanceContext,
                                    NULL );

    if( !NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCES | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to set instance context for volume %wZ (Volume = %p, Instance = %p, Status = 0x%08X)\n",
                     &instanceContext->VolumeName,
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );
        goto CtxInstanceSetupCleanup;
    }


CtxInstanceSetupCleanup:

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

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Releasing instance context %p (Volume = %p, Instance = %p)\n",
                     instanceContext,
                     FltObjects->Volume,
                     FltObjects->Instance) );

        FltReleaseContext( instanceContext );
    }


    if (NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Ctx]: Instance setup complete (Volume = %p, Instance = %p). Filter will attach to the volume.\n",
                     FltObjects->Volume,
                     FltObjects->Instance) );
    } else {

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Ctx]: Instance setup complete (Volume = %p, Instance = %p). Filter will not attach to the volume.\n",
                     FltObjects->Volume,
                     FltObjects->Instance) );
    }

    return status;
}


NTSTATUS
CtxInstanceQueryTeardown (
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
                ("[Ctx]: Instance query teardown started (Instance = %p)\n",
                 FltObjects->Instance) );


    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Ctx]: Instance query teadown ended (Instance = %p)\n",
                 FltObjects->Instance) );
    return STATUS_SUCCESS;
}


VOID
CtxInstanceTeardownStart (
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
                ("[Ctx]: Instance teardown start started (Instance = %p)\n",
                 FltObjects->Instance) );


    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Ctx]: Instance teardown start ended (Instance = %p)\n",
                 FltObjects->Instance) );
}


VOID
CtxInstanceTeardownComplete (
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
    PCTX_INSTANCE_CONTEXT instanceContext;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Ctx]: Instance teardown complete started (Instance = %p)\n",
                 FltObjects->Instance) );

    DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                ("[Ctx]: Getting instance context (Volume = %p, Instance = %p)\n",
                 FltObjects->Volume,
                 FltObjects->Instance) );

    status = FltGetInstanceContext( FltObjects->Instance,
                                    &instanceContext );

    if (NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Instance teardown for volume %wZ (Volume = %p, Instance = %p, InstanceContext = %p)\n",
                     &instanceContext->VolumeName,
                     FltObjects->Volume,
                     FltObjects->Instance,
                     instanceContext) );


        //
        //  Here the filter may perform any teardown of its own structures associated
        //  with this instance.
        //
        //  The filter should not free memory or synchronization objects allocated to
        //  objects within the instance context. That should be performed in the
        //  cleanup callback for the instance context
        //

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Releasing instance context %p for volume %wZ (Volume = %p, Instance = %p)\n",
                     instanceContext,
                     &instanceContext->VolumeName,
                     FltObjects->Volume,
                     FltObjects->Instance) );

        FltReleaseContext( instanceContext );
    } else {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to get instance context (Volume = %p, Instance = %p Status = 0x%x)\n",
                     FltObjects->Volume,
                     FltObjects->Instance,
                     status) );
    }

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[Ctx]: Instance teardown complete ended (Instance = %p)\n",
                 FltObjects->Instance) );
}


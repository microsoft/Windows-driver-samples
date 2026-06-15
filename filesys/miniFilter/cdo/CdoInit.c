/*++

Copyright (c) 1999 - 2003  Microsoft Corporation

Module Name:

    CdoInit.c

Abstract:

    This is the main module of the kernel mode filter driver implementing
    the control device object sample.


Environment:

    Kernel mode


--*/

#include "pch.h"


//
//  Local function prototypes
//

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
CdoUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
CdoInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
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
CdoGetIoOpenDriverRegistryKey (
    VOID
    );

NTSTATUS
CdoOpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    );

VOID
CdoInitializeDebugLevel (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

#endif


//
//  Global variables
//

CDO_GLOBAL_DATA Globals;


//
//  Pragma defintiion table
//

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#if DBG
#pragma alloc_text(INIT, CdoGetIoOpenDriverRegistryKey)
#pragma alloc_text(INIT, CdoOpenServiceParametersKey)
#pragma alloc_text(INIT, CdoInitializeDebugLevel)
#endif

#pragma alloc_text(PAGE, CdoUnload)
#endif



NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;


    //
    //  This defines what we want to filter with FltMgr
    //

    CONST FLT_REGISTRATION filterRegistration = {
        sizeof( FLT_REGISTRATION ),         //  Size
        FLT_REGISTRATION_VERSION,           //  Version
        0,                                  //  Flags
        NULL,                               //  Context
        NULL,                               //  Operation callbacks
        CdoUnload,                          //  MiniFilterUnload
        CdoInstanceSetup,                   //  InstanceSetup
        NULL,                               //  InstanceQueryTeardown
        NULL,                               //  InstanceTeardownStart
        NULL,                               //  InstanceTeardownComplete
        NULL,NULL                           //  NameProvider callbacks
    };


    RtlZeroMemory( &Globals, sizeof( Globals ) );

#if DBG

    //
    //  Initialize global debug level
    //

    CdoInitializeDebugLevel( DriverObject, RegistryPath );

#else

    UNREFERENCED_PARAMETER( RegistryPath );

#endif

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[Cdo]: Driver being loaded\n") );

    //
    //  Initialize the resource
    //

    ExInitializeResourceLite( &Globals.Resource );

    //
    //  Record the driver object
    //

    Globals.FilterDriverObject = DriverObject;

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &filterRegistration,
                                &Globals.Filter );

    if (!NT_SUCCESS( status )) {

        ExDeleteResourceLite( &Globals.Resource );
        return status;
    }

    //
    //  Now create our control device object
    //

    status = CdoCreateControlDeviceObject( DriverObject );

    if (!NT_SUCCESS( status )) {

        FltUnregisterFilter( Globals.Filter );
        ExDeleteResourceLite( &Globals.Resource );
        return status;
    }

    //
    //  Start filtering i/o
    //

    status = FltStartFiltering( Globals.Filter );

    if (!NT_SUCCESS( status )) {

        CdoDeleteControlDeviceObject();
        FltUnregisterFilter( Globals.Filter );
        ExDeleteResourceLite( &Globals.Resource );
        return status;
    }

    return status;
}

#if DBG

PFN_IoOpenDriverRegistryKey
CdoGetIoOpenDriverRegistryKey (
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
CdoOpenServiceParametersKey (
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
    NTSTATUS Status;
    PFN_IoOpenDriverRegistryKey pIoOpenDriverRegistryKey;
    UNICODE_STRING Subkey;
    HANDLE ParametersKey = NULL;
    HANDLE ServiceRegKey = NULL;
    OBJECT_ATTRIBUTES Attributes;

    //
    //  Open the parameters key to read values from the INF, using the API to
    //  open the key if possible.
    //

    pIoOpenDriverRegistryKey = CdoGetIoOpenDriverRegistryKey();

    if (pIoOpenDriverRegistryKey != NULL) {

        //
        //  Open the parameters key using the API.
        //

        Status = pIoOpenDriverRegistryKey( DriverObject,
                                           DriverRegKeyParameters,
                                           KEY_READ,
                                           0,
                                           &ParametersKey );

        if (!NT_SUCCESS( Status )) {

            goto cleanup;
        }

    } else {

        //
        //  Open specified service root key.
        //

        InitializeObjectAttributes( &Attributes,
                                    ServiceRegistryPath,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = ZwOpenKey( &ServiceRegKey,
                            KEY_READ,
                            &Attributes );

        if (!NT_SUCCESS( Status )) {

            goto cleanup;
        }

        //
        //  Open the parameters key relative to service key path.
        //

        RtlInitUnicodeString( &Subkey, L"Parameters" );

        InitializeObjectAttributes( &Attributes,
                                    &Subkey,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    ServiceRegKey,
                                    NULL );

        Status = ZwOpenKey( &ParametersKey,
                            KEY_READ,
                            &Attributes );

        if (!NT_SUCCESS( Status )) {

            goto cleanup;
        }
    }

    //
    //  Return value to caller.
    //

    *ServiceParametersKey = ParametersKey;

cleanup:

    if (ServiceRegKey != NULL) {

        ZwClose( ServiceRegKey );
    }

    return Status;

}

VOID
CdoInitializeDebugLevel (
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

    status = CdoOpenServiceParametersKey( DriverObject,
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
CdoUnload (
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

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[Cdo]: Unloading driver\n") );

    //
    //  If the CDO is still referenced and the unload is not mandatry
    //  then fail the unload
    //

    CdoAcquireResourceShared( &Globals.Resource );

    if (FlagOn( Globals.Flags, GLOBAL_DATA_F_CDO_OPEN_REF) &&
        !FlagOn(Flags,FLTFL_FILTER_UNLOAD_MANDATORY)) {

        DebugTrace( DEBUG_TRACE_LOAD_UNLOAD | DEBUG_TRACE_ERROR,
                    ("[Cdo]: Fail unloading driver since the unload is optional and the CDO is open\n") );
        CdoReleaseResource( &Globals.Resource );
        return STATUS_FLT_DO_NOT_DETACH;
    }


    //
    //  Cleanup and unload
    //

    FltUnregisterFilter( Globals.Filter );
    Globals.Filter = NULL;
    CdoDeleteControlDeviceObject();


    CdoReleaseResource( &Globals.Resource );

    ExDeleteResourceLite( &Globals.Resource );

    return STATUS_SUCCESS;
}


//
//  Instance setup routine
//

NTSTATUS
CdoInstanceSetup (
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

    STATUS_FLT_DO_NOT_ATTACH - do not attach because we do not want to
    attach to any volume

--*/
{

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    return STATUS_FLT_DO_NOT_ATTACH;
}




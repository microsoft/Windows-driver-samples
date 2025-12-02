/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    cancelSafe.c

Abstract:

    This is the main module of the cancelSafe miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>


//
//  Debug flags and helper functions
//

#define CSQ_TRACE_ERROR                     0x00000001
#define CSQ_TRACE_LOAD_UNLOAD               0x00000002
#define CSQ_TRACE_INSTANCE_CALLBACK         0x00000004
#define CSQ_TRACE_CONTEXT_CALLBACK          0x00000008
#define CSQ_TRACE_CBDQ_CALLBACK             0x00000010
#define CSQ_TRACE_PRE_READ                  0x00000020
#define CSQ_TRACE_ALL                       0xFFFFFFFF

#define DebugTrace(Level, Data)               \
    if ((Level) & Globals.DebugLevel) {       \
        DbgPrint Data;                        \
    }

//
//  Memory Pool Tags
//

#define INSTANCE_CONTEXT_TAG              'IqsC'
#define QUEUE_CONTEXT_TAG                 'QqsC'
#define CSQ_REG_TAG                       'RqsC'
#define CSQ_STRING_TAG                    'SqsC'

//
// Registry value names and default values
//

#define CSQ_DEFAULT_TIME_DELAY            150000000
#define CSQ_DEFAULT_MAPPING_PATH          L"\\"
#define CSQ_KEY_NAME_DELAY                L"OperatingDelay"
#define CSQ_KEY_NAME_PATH                 L"OperatingPath"
#define CSQ_KEY_NAME_DEBUG_LEVEL          L"DebugLevel"
#define CSQ_MAX_PATH_LENGTH               256


//
//  Prototypes
//

//
//  Queue context data structure
//

typedef struct _QUEUE_CONTEXT {

    FLT_CALLBACK_DATA_QUEUE_IO_CONTEXT CbdqIoContext;

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

//
//  Instance context data structure
//

typedef struct _INSTANCE_CONTEXT {

    //
    //  Instance for this context.
    //

    PFLT_INSTANCE Instance;

    //
    //  Cancel safe queue members
    //

    FLT_CALLBACK_DATA_QUEUE Cbdq;
    LIST_ENTRY QueueHead;
    FAST_MUTEX Lock;

    //
    //  Flag to control the life/death of the work item thread
    //

    volatile LONG WorkerThreadFlag;

    //
    //  Notify the worker thread that the instance is being torndown
    //

    KEVENT TeardownEvent;

} INSTANCE_CONTEXT, *PINSTANCE_CONTEXT;


typedef struct _CSQ_GLOBAL_DATA {

    ULONG  DebugLevel;

    PFLT_FILTER  FilterHandle;

    NPAGED_LOOKASIDE_LIST QueueContextLookaside;

    UNICODE_STRING MappingPath;

    PWSTR PathBuffer;

    LONGLONG TimeDelay;

} CSQ_GLOBAL_DATA;



//
//  Global variables
//

CSQ_GLOBAL_DATA Globals;


//
//  Local function prototypes
//

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

VOID
FreeGlobals(
    );

NTSTATUS
Unload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

VOID
ContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
InstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
InstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
InstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
InstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

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
GetIoOpenDriverRegistryKey (
    VOID
    );

NTSTATUS
OpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    );

NTSTATUS
SetConfiguration (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

VOID
_IRQL_requires_max_(APC_LEVEL)
_IRQL_raises_(APC_LEVEL)
_Requires_lock_not_held_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
_Acquires_lock_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
CsqAcquire(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _Out_ PKIRQL Irql
    );

VOID
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(APC_LEVEL)
_IRQL_raises_(PASSIVE_LEVEL)
_Requires_lock_held_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
_Releases_lock_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
CsqRelease(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_ KIRQL Irql
    );

NTSTATUS
CsqInsertIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PVOID Context
    );
VOID
CsqRemoveIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_ PFLT_CALLBACK_DATA Data
    );
PFLT_CALLBACK_DATA
CsqPeekNextIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PVOID PeekContext
    );
VOID
CsqCompleteCanceledIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _Inout_ PFLT_CALLBACK_DATA Data
    );

FLT_PREOP_CALLBACK_STATUS
PreRead (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

VOID
PreReadWorkItemRoutine(
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID Context
    );

NTSTATUS
PreReadPendIo(
    _In_ PINSTANCE_CONTEXT InstanceContext
    );

NTSTATUS
PreReadProcessIo(
    _Inout_ PFLT_CALLBACK_DATA Data
    );

VOID
PreReadEmptyQueueAndComplete(
    _In_ PINSTANCE_CONTEXT InstanceContext
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, GetIoOpenDriverRegistryKey)
#pragma alloc_text(INIT, OpenServiceParametersKey)
#pragma alloc_text(INIT, SetConfiguration)
#pragma alloc_text(PAGE, Unload)
#pragma alloc_text(PAGE, FreeGlobals)
#pragma alloc_text(PAGE, ContextCleanup)
#pragma alloc_text(PAGE, InstanceSetup)
#pragma alloc_text(PAGE, InstanceQueryTeardown)
#pragma alloc_text(PAGE, InstanceTeardownStart)
#pragma alloc_text(PAGE, InstanceTeardownComplete)

#endif

//
//  Filters callback routines
//

FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_READ,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      PreRead,
      NULL },

    { IRP_MJ_OPERATION_END }
};

//
// Filters context registration data structure
//

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_INSTANCE_CONTEXT,
        0,
        ContextCleanup,
        sizeof( INSTANCE_CONTEXT ),
        INSTANCE_CONTEXT_TAG },

    { FLT_CONTEXT_END }
};

//
// Filters registration data structure
//

FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),             //  Size
    FLT_REGISTRATION_VERSION,               //  Version
    0,                                      //  Flags
    ContextRegistration,                    //  Context
    Callbacks,                              //  Operation callbacks
    Unload,                                 //  Filters unload routine
    InstanceSetup,                          //  InstanceSetup routine
    InstanceQueryTeardown,                  //  InstanceQueryTeardown routine
    InstanceTeardownStart,                  //  InstanceTeardownStart routine
    InstanceTeardownComplete,               //  InstanceTeardownComplete routine
    NULL, NULL, NULL                        //  Unused naming support callbacks
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
    NTSTATUS Status;

    //
    //  Default to NonPagedPoolNx for non paged pool allocations where supported.
    //

    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    //
    //  Initialize global lookaside list
    //

    ExInitializeNPagedLookasideList( &Globals.QueueContextLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( QUEUE_CONTEXT ),
                                     QUEUE_CONTEXT_TAG,
                                     0 );

    //
    //  Initialize the configuration to default values
    //

    Globals.DebugLevel = CSQ_TRACE_ERROR;

    Globals.TimeDelay = CSQ_DEFAULT_TIME_DELAY;

    Globals.PathBuffer = NULL;

    RtlInitUnicodeString( &Globals.MappingPath, CSQ_DEFAULT_MAPPING_PATH );


    //
    //  Modify the configuration based on values in the registry
    //

    Status = SetConfiguration( DriverObject, RegistryPath );

    if (!NT_SUCCESS( Status )) {

        goto DriverEntryCleanup;
    }

    DebugTrace( CSQ_TRACE_LOAD_UNLOAD,
                ("[Csq]: CancelSafe!DriverEntry\n") );



    //
    //  Register with the filter manager
    //

    Status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &Globals.FilterHandle );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_LOAD_UNLOAD | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to register filter (Status = 0x%x)\n",
                    Status) );

        goto DriverEntryCleanup;

    }

    //
    //  Start filtering I/O
    //

    Status = FltStartFiltering( Globals.FilterHandle );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_LOAD_UNLOAD | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to start filtering (Status = 0x%x)\n",
                    Status) );

        FltUnregisterFilter( Globals.FilterHandle );

        goto DriverEntryCleanup;

    }


    DebugTrace( CSQ_TRACE_LOAD_UNLOAD,
                ("[Csq]: Driver loaded complete\n") );

DriverEntryCleanup:

    if (!NT_SUCCESS( Status )) {

        FreeGlobals();
    }

    return Status;
}

PFN_IoOpenDriverRegistryKey
GetIoOpenDriverRegistryKey (
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
OpenServiceParametersKey (
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

    pIoOpenDriverRegistryKey = GetIoOpenDriverRegistryKey();

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

            goto OpenServiceParametersKeyCleanup;
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

            goto OpenServiceParametersKeyCleanup;
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

            goto OpenServiceParametersKeyCleanup;
        }
    }

    //
    //  Return value to caller.
    //

    *ServiceParametersKey = ParametersKey;

OpenServiceParametersKeyCleanup:

    if (ServiceRegKey != NULL) {

        ZwClose( ServiceRegKey );
    }

    return Status;

}

NTSTATUS
SetConfiguration (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to configure the debuglevel, mapping path and
    queue delay based on values in the registry.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - The path key passed to the driver during DriverEntry.

Return Value:

    STATUS_SUCCESS if the function completes successfully.  Otherwise a valid
    NTSTATUS code is returned.

--*/
{
    NTSTATUS Status;
    HANDLE DriverRegKey = NULL;
    UNICODE_STRING ValueName;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + CSQ_MAX_PATH_LENGTH  * sizeof(WCHAR)];
    PKEY_VALUE_PARTIAL_INFORMATION Value = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    ULONG ValueLength = sizeof(Buffer);
    ULONG ResultLength;
    ULONG Length;

    //
    //  Open service parameters key to query values from.
    //

    Status = OpenServiceParametersKey( DriverObject,
                                       RegistryPath,
                                       &DriverRegKey );

    if (!NT_SUCCESS( Status )) {

        DriverRegKey = NULL;
        goto SetConfigurationCleanup;
    }

    //
    //  Query the debug level.
    //

    RtlInitUnicodeString( &ValueName, CSQ_KEY_NAME_DEBUG_LEVEL );

    Status = ZwQueryValueKey( DriverRegKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              Value,
                              ValueLength,
                              &ResultLength );

    if (NT_SUCCESS( Status )) {

        Globals.DebugLevel = *(PULONG)(Value->Data);
    }

    //
    //  Query the queue time delay.
    //

    RtlInitUnicodeString( &ValueName, CSQ_KEY_NAME_DELAY );

    Status = ZwQueryValueKey( DriverRegKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              Value,
                              ValueLength,
                              &ResultLength );

    if (NT_SUCCESS( Status )) {

        if (Value->Type != REG_DWORD) {

            Status = STATUS_INVALID_PARAMETER;
            goto SetConfigurationCleanup;
        }

        Globals.TimeDelay = (LONGLONG)(*(PULONG)(Value->Data));

    }

    //
    //  Query the mapping path.
    //

    RtlInitUnicodeString( &ValueName, CSQ_KEY_NAME_PATH );

    //
    //  For simplicity of this sample, the length of the mapping path
    //  allowed in the registry is limited to CSQ_MAX_PATH_LENGTH
    //  characters. If this size is exceeded the default mapping path
    //  will be used.
    //

    Status = ZwQueryValueKey( DriverRegKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              Value,
                              ValueLength,
                              &ValueLength );

    if (NT_SUCCESS( Status )) {

        //
        //  Set up the mapping and ensure the mapping string format is "\a\...\".
        //  If the mapping path doesn't begin with '\' fail, if it doesn't end
        //  with a '\' append one.
        //

        if (*(PWCHAR)(Value->Data) != L'\\') {

            Status = STATUS_INVALID_PARAMETER;
            goto SetConfigurationCleanup;
        }

        //
        //  Allocate enough space for an extra character in case a trailing '\'
        //  is missing and needs to be added.
        //

        Length = Value->DataLength + sizeof(WCHAR),

        Globals.PathBuffer = ExAllocatePoolZero( NonPagedPool, Length, CSQ_STRING_TAG );

        if (Globals.PathBuffer == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetConfigurationCleanup;
        }

        RtlCopyMemory( Globals.PathBuffer, Value->Data, Value->DataLength );

        Globals.PathBuffer[Length / sizeof(WCHAR) - 1] = L'\0';

        //
        //  Add a trailing '\' if one is missing.
        //

        if (Globals.PathBuffer[Length/sizeof(WCHAR) - 3] != L'\\') {

            Globals.PathBuffer[Length/sizeof(WCHAR) - 2] = L'\\';

        }

        RtlInitUnicodeString(&Globals.MappingPath, Globals.PathBuffer);

    }

    //
    //  Ignore errors when looking for values in the registry.
    //  Default values will be used.
    //

    Status = STATUS_SUCCESS;

SetConfigurationCleanup:

    if (DriverRegKey != NULL) {

        ZwClose( DriverRegKey );
    }

    return Status;

}


VOID
FreeGlobals(
    )
/*++

Routine Descrition:

    This routine cleans up the global buffers on both
    teardown and initialization failure.

Arguments:

Return Value:

    None.

--*/
{
    PAGED_CODE();

    Globals.FilterHandle = NULL;

    ExDeleteNPagedLookasideList( &Globals.QueueContextLookaside );

    if (Globals.PathBuffer != NULL) {

        ExFreePoolWithTag( Globals.PathBuffer, CSQ_STRING_TAG );
        Globals.PathBuffer = NULL;
    }

    RtlInitUnicodeString( &Globals.MappingPath, NULL );
}


NTSTATUS
Unload (
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

    DebugTrace( CSQ_TRACE_LOAD_UNLOAD,
                ("[Csq]: CancelSafe!Unload\n") );

    FltUnregisterFilter( Globals.FilterHandle );

    FreeGlobals();

    return STATUS_SUCCESS;
}


//
//  Context cleanup routine.
//

VOID
ContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    FltMgr calls this routine immediately before it deletes the context.

Arguments:

    Context - Pointer to the minifilter driver's portion of the context.

    ContextType - Type of context. Must be one of the following values:
        FLT_FILE_CONTEXT (Microsoft Windows Vista and later only.),
        FLT_INSTANCE_CONTEXT, FLT_STREAM_CONTEXT, FLT_STREAMHANDLE_CONTEXT,
        FLT_TRANSACTION_CONTEXT (Windows Vista and later only.), and
        FLT_VOLUME_CONTEXT

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    DebugTrace( CSQ_TRACE_CONTEXT_CALLBACK,
                ("[Csq]: CancelSafe!ContextCleanup\n") );
}

//
//  Instance setup/teardown routines.
//

NTSTATUS
InstanceSetup (
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

    VolumeDeviceType - Device type of the file system volume.
        Must be one of the following: FILE_DEVICE_CD_ROM_FILE_SYSTEM,
        FILE_DEVICE_DISK_FILE_SYSTEM, and FILE_DEVICE_NETWORK_FILE_SYSTEM.

    VolumeFilesystemType - File system type of the volume.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    PINSTANCE_CONTEXT InstCtx = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK,
                ("[Csq]: CancelSafe!InstanceSetup\n") );

    //
    //  Allocate and initialize the instance context.
    //

    Status = FltAllocateContext( FltObjects->Filter,
                                 FLT_INSTANCE_CONTEXT,
                                 sizeof( INSTANCE_CONTEXT ),
                                 NonPagedPool,
                                 &InstCtx );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to allocate instance context (Volume = %p, Instance = %p, Status = 0x%x)\n",
                    FltObjects->Volume,
                    FltObjects->Instance,
                    Status) );

        goto InstanceSetupCleanup;
    }

    Status = FltCbdqInitialize( FltObjects->Instance,
                                &InstCtx->Cbdq,
                                CsqInsertIo,
                                CsqRemoveIo,
                                CsqPeekNextIo,
                                CsqAcquire,
                                CsqRelease,
                                CsqCompleteCanceledIo );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to initialize callback data queue (Volume = %p, Instance = %p, Status = 0x%x)\n",
                    FltObjects->Volume,
                    FltObjects->Instance,
                    Status) );

        goto InstanceSetupCleanup;
    }

    //
    //  Initialize the internal queue head and lock of the cancel safe queue.
    //

    InitializeListHead( &InstCtx->QueueHead );

    ExInitializeFastMutex( &InstCtx->Lock );

    //
    //  Initialize other members of the instance context.
    //

    InstCtx->Instance = FltObjects->Instance;

    InstCtx->WorkerThreadFlag = 0;

    KeInitializeEvent( &InstCtx->TeardownEvent, NotificationEvent, FALSE );

    //
    //  Set the instance context.
    //

    Status = FltSetInstanceContext( FltObjects->Instance,
                                    FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                    InstCtx,
                                    NULL );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to set instance context (Volume = %p, Instance = %p, Status = 0x%x)\n",
                    FltObjects->Volume,
                    FltObjects->Instance,
                    Status) );

        goto InstanceSetupCleanup;
    }


InstanceSetupCleanup:

    if (InstCtx != NULL) {

        FltReleaseContext( InstCtx );
    }

    return Status;
}


NTSTATUS
InstanceQueryTeardown (
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

    DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK,
                ("[Csq]: CancelSafe!InstanceQueryTeardown\n") );

    return STATUS_SUCCESS;
}


VOID
InstanceTeardownStart (
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
    PINSTANCE_CONTEXT InstCtx = 0;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK,
                ("[Csq]: CancelSafe!InstanceTeardownStart\n") );

    //
    //  Get a pointer to the instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstCtx );

    if (!NT_SUCCESS( Status ))
    {
        FLT_ASSERT( !"Instance Context is missing" );
        return;
    }

    //
    //  Disable the insert to the cancel safe queue.
    //

    FltCbdqDisable( &InstCtx->Cbdq );

    //
    //  Remove all callback data from the queue and complete them.
    //

    PreReadEmptyQueueAndComplete( InstCtx );

    //
    //  Signal the worker thread if it is pended.
    //

    KeSetEvent( &InstCtx->TeardownEvent, 0, FALSE );

    //
    //  Cleanup
    //

    FltReleaseContext( InstCtx );
}


VOID
InstanceTeardownComplete (
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
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    DebugTrace( CSQ_TRACE_INSTANCE_CALLBACK,
                ("[Csq]: CancelSafe!InstanceTeardownComplete\n") );

    PAGED_CODE();
}


//
//  Cbdq callback routines.
//

VOID
_IRQL_requires_max_(APC_LEVEL)
_IRQL_raises_(APC_LEVEL)
_Requires_lock_not_held_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
_Acquires_lock_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
CsqAcquire(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _Out_ PKIRQL Irql
    )
/*++

Routine Description:

    FltMgr calls this routine to acquire the lock protecting the queue.

Arguments:

    DataQueue - Supplies a pointer to the queue itself.

    Irql - Returns the previous IRQL if a spinlock is acquired.  We do not use
           any spinlocks, so we ignore this.

Return Value:

    None.

--*/
{
    PINSTANCE_CONTEXT InstCtx;

    DebugTrace( CSQ_TRACE_CBDQ_CALLBACK,
                ("[Csq]: CancelSafe!CsqAcquire\n") );

    //
    //  Get a pointer to the instance context.
    //

    InstCtx = CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq );

    //
    //  Acquire the lock.
    //

    ExAcquireFastMutex( &InstCtx->Lock );

    *Irql = 0;
}


VOID
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(APC_LEVEL)
_IRQL_raises_(PASSIVE_LEVEL)
_Requires_lock_held_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
_Releases_lock_((CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq ))->Lock)
CsqRelease(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_ KIRQL Irql
    )
/*++

Routine Description:

    FltMgr calls this routine to release the lock protecting the queue.

Arguments:

    DataQueue - Supplies a pointer to the queue itself.

    Irql - Supplies the previous IRQL if a spinlock is acquired.  We do not use
           any spinlocks, so we ignore this.

Return Value:

    None.

--*/
{
    PINSTANCE_CONTEXT InstCtx;

    UNREFERENCED_PARAMETER( Irql );

    DebugTrace( CSQ_TRACE_CBDQ_CALLBACK,
                ("[Csq]: CancelSafe!CsqRelease\n") );

    //
    //  Get a pointer to the instance context.
    //

    InstCtx = CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq );

    //
    //  Release the lock.
    //

    ExReleaseFastMutex( &InstCtx->Lock );
}


NTSTATUS
CsqInsertIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PVOID Context
    )
/*++

Routine Description:

    FltMgr calls this routine to insert an entry into our pending I/O queue.
    The queue is already locked before this routine is called.

Arguments:

    DataQueue - Supplies a pointer to the queue itself.

    Data - Supplies the callback data for the operation that is being
           inserted into the queue.

    Context - Supplies user-defined context information.

Return Value:

    STATUS_SUCCESS if the function completes successfully.  Otherwise a valid
    NTSTATUS code is returned.

--*/
{
    PINSTANCE_CONTEXT InstCtx;
    PFLT_GENERIC_WORKITEM WorkItem = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WasQueueEmpty;

    UNREFERENCED_PARAMETER( Context );

    DebugTrace( CSQ_TRACE_CBDQ_CALLBACK,
                ("[Csq]: CancelSafe!CsqInsertIo\n") );

    //
    //  Get a pointer to the instance context.
    //

    InstCtx = CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq );

    //
    //  Save the queue state before inserting to it.
    //

    WasQueueEmpty = IsListEmpty( &InstCtx->QueueHead );

    //
    //  Insert the callback data entry into the queue.
    //

    InsertTailList( &InstCtx->QueueHead,
                    &Data->QueueLinks );

    //
    //  Queue a work item if no worker thread present.
    //

    if (WasQueueEmpty &&
        InterlockedIncrement( &InstCtx->WorkerThreadFlag ) == 1) {

        WorkItem = FltAllocateGenericWorkItem();

        if (WorkItem) {

            Status = FltQueueGenericWorkItem( WorkItem,
                                              InstCtx->Instance,
                                              PreReadWorkItemRoutine,
                                              DelayedWorkQueue,
                                              InstCtx->Instance );

            if (!NT_SUCCESS( Status )) {

                DebugTrace( CSQ_TRACE_CBDQ_CALLBACK | CSQ_TRACE_ERROR,
                            ("[Csq]: Failed to queue the work item (Status = 0x%x)\n",
                            Status) );

                FltFreeGenericWorkItem( WorkItem );
            }

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS( Status )) {

            //
            //  If we failed to queue a workitem we need to
            //  decrement the worker thread flag. If we did
            //  not decrement it future queue insertions would
            //  not trigger a workitem and requests added to
            //  the queue would be orphaned. We can safely
            //  decrement the flag here because we are
            //  guaranteed that no worker routine is currently
            //  running and that the queue is currently locked.
            //

            InterlockedDecrement( &InstCtx->WorkerThreadFlag );
            NT_ASSERT( InstCtx->WorkerThreadFlag == 0 );

            //
            //  Remove the callback data that was inserted into the queue.
            //

            RemoveTailList( &InstCtx->QueueHead );
        }
    }

    return Status;
}


VOID
CsqRemoveIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    FltMgr calls this routine to remove an entry from our pending I/O queue.
    The queue is already locked before this routine is called.

Arguments:

    DataQueue - Supplies a pointer to the queue itself.

    Data - Supplies the callback data that is to be removed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( DataQueue );

    DebugTrace( CSQ_TRACE_CBDQ_CALLBACK,
                ("[Csq]: CancelSafe!CsqRemoveIo\n") );

    //
    //  Remove the callback data entry from the queue.
    //

    RemoveEntryList( &Data->QueueLinks );
}


PFLT_CALLBACK_DATA
CsqPeekNextIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PVOID PeekContext
    )
/*++

Routine Description:

    FltMgr calls this routine to look for an entry on our pending I/O queue.
    The queue is already locked before this routine is called.

Arguments:

    DataQueue - Supplies a pointer to the queue itself.

    Data - Supplies the callback data we should start our search from.
           If this is NULL, we start at the beginning of the list.

    PeekContext - Supplies user-defined context information.

Return Value:

    A pointer to the next callback data structure, or NULL.

--*/
{
    PINSTANCE_CONTEXT InstCtx;
    PLIST_ENTRY NextEntry;
    PFLT_CALLBACK_DATA NextData;

    UNREFERENCED_PARAMETER( PeekContext );

    DebugTrace( CSQ_TRACE_CBDQ_CALLBACK,
                ("[Csq]: CancelSafe!CsqPeekNextIo\n") );

    //
    //  Get a pointer to the instance context.
    //

    InstCtx = CONTAINING_RECORD( DataQueue, INSTANCE_CONTEXT, Cbdq );

    //
    //  If the supplied callback "Data" is NULL, the "NextIo" is the first entry
    //  in the queue; or it is the next list entry in the queue.
    //

    if (Data == NULL) {

        NextEntry = InstCtx->QueueHead.Flink;

    } else {

        NextEntry =  Data->QueueLinks.Flink;
    }

    //
    //  Return NULL if we hit the end of the queue or the queue is empty.
    //

    if (NextEntry == &InstCtx->QueueHead) {

        return NULL;
    }

    NextData = CONTAINING_RECORD( NextEntry, FLT_CALLBACK_DATA, QueueLinks );

    return NextData;
}


VOID
CsqCompleteCanceledIo(
    _In_ PFLT_CALLBACK_DATA_QUEUE DataQueue,
    _Inout_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    FltMgr calls this routine to complete an operation as cancelled that was
    previously pended. The queue is already locked before this routine is called.

Arguments:

    DataQueue - Supplies a pointer to the queue itself.

    Data - Supplies the callback data that is to be canceled.

Return Value:

    None.

--*/
{
    PQUEUE_CONTEXT QueueCtx;

    UNREFERENCED_PARAMETER( DataQueue );

    DebugTrace( CSQ_TRACE_CBDQ_CALLBACK,
                ("[Csq]: CancelSafe!CsqCompleteCanceledIo\n") );

    QueueCtx = (PQUEUE_CONTEXT) Data->QueueContext[0];

    //
    //  Just complete the operation as canceled.
    //

    Data->IoStatus.Status = STATUS_CANCELLED;
    Data->IoStatus.Information = 0;

    FltCompletePendedPreOperation( Data,
                                   FLT_PREOP_COMPLETE,
                                   0 );

    //
    //  Free the extra storage that was allocated for this canceled I/O.
    //

    ExFreeToNPagedLookasideList( &Globals.QueueContextLookaside,
                                 QueueCtx );
}


FLT_PREOP_CALLBACK_STATUS
PreRead (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Handle pre-read.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{

    PINSTANCE_CONTEXT InstCtx = NULL;
    PQUEUE_CONTEXT QueueCtx = NULL;
    PFLT_FILE_NAME_INFORMATION NameInfo = NULL;
    NTSTATUS CbStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( CompletionContext );

    DebugTrace( CSQ_TRACE_PRE_READ,
                ("[Csq]: CancelSafe!PreRead\n") );

    //
    //  Skip IRP_PAGING_IO, IRP_SYNCHRONOUS_PAGING_IO and
    //  TopLevelIrp.
    //

    if ((Data->Iopb->IrpFlags & IRP_PAGING_IO) ||
        (Data->Iopb->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ||
        IoGetTopLevelIrp()) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  Get and parse the file name
    //

    Status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_NORMALIZED
                                          | FLT_FILE_NAME_QUERY_DEFAULT,
                                        &NameInfo );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_PRE_READ | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to get filename (Status = 0x%x)\n",
                    Status) );

        goto PreReadCleanup;
    }

    Status = FltParseFileNameInformation( NameInfo );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( CSQ_TRACE_PRE_READ | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to parse filename (Name = %wZ, Status = 0x%x)\n",
                    &NameInfo->Name,
                    Status) );

        goto PreReadCleanup;
    }

    //
    //  Compare to see if this file I/O is to be pended.
    //

    if (!RtlPrefixUnicodeString( &Globals.MappingPath, &NameInfo->ParentDir, TRUE )) {

        goto PreReadCleanup;
    }

    //
    //  Since Fast I/O operations cannot be queued, we could return
    //  FLT_PREOP_SUCCESS_NO_CALLBACK at this point. In this sample,
    //  we disallow Fast I/O for this magic file in order to force an IRP
    //  to be sent to us again. The purpose of doing that is to demonstrate
    //  the cancel safe queue, which may not be true in the real world.
    //

    if (!FLT_IS_IRP_OPERATION( Data )) {

        CbStatus = FLT_PREOP_DISALLOW_FASTIO;
        goto PreReadCleanup;
    }

    //
    //  Allocate a context for each I/O to be inserted into the queue.
    //

    QueueCtx = ExAllocateFromNPagedLookasideList( &Globals.QueueContextLookaside );

    if (QueueCtx == NULL) {

        DebugTrace( CSQ_TRACE_PRE_READ | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to allocate from NPagedLookasideList (Status = 0x%x)\n",
                    Status) );

        goto PreReadCleanup;
    }

    RtlZeroMemory(QueueCtx, sizeof(QUEUE_CONTEXT));

    //
    //  Get the instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstCtx );

    if (!NT_SUCCESS( Status )) {

        FLT_ASSERT( !"Instance context is missing" );
        goto PreReadCleanup;
    }

    //
    //  Set the queue context
    //

    Data->QueueContext[0] = (PVOID) QueueCtx;
    Data->QueueContext[1] = NULL;

    //
    //  Insert the callback data into the cancel safe queue
    //

    Status = FltCbdqInsertIo( &InstCtx->Cbdq,
                              Data,
                              &QueueCtx->CbdqIoContext,
                              0 );

    if (Status == STATUS_SUCCESS) {

        //
        //  In general, we can create a worker thread here as long as we can
        //  correctly handle the insert/remove race conditions b/w multi threads.
        //  In this sample, the worker thread creation is done in CsqInsertIo.
        //  This is a simpler solution because CsqInsertIo is atomic with
        //  respect to other CsqXxxIo callback routines.
        //

        CbStatus = FLT_PREOP_PENDING;

    } else {

        DebugTrace( CSQ_TRACE_PRE_READ | CSQ_TRACE_ERROR,
                    ("[Csq]: Failed to insert into cbdq (Status = 0x%x)\n",
                    Status) );
    }

PreReadCleanup:

    //
    //  Clean up
    //

    if (QueueCtx && CbStatus != FLT_PREOP_PENDING) {

        ExFreeToNPagedLookasideList( &Globals.QueueContextLookaside, QueueCtx );
    }

    if (NameInfo) {

        FltReleaseFileNameInformation( NameInfo );
    }

    if (InstCtx) {

        FltReleaseContext( InstCtx );
    }

    return CbStatus;
}


VOID
PreReadWorkItemRoutine(
    _In_ PFLT_GENERIC_WORKITEM WorkItem,
    _In_ PFLT_FILTER Filter,
    _In_ PVOID Context
    )
/*++

Routine Description:

    This WorkItem routine is called in the system thread context to process
    all the pended I/O in this mini filter's cancel safe queue. For each I/O
    in the queue, it completes the I/O after pending the operation for a
    period of time. The thread exits when the queue is empty.

Arguments:

    WorkItem - Unused.

    Filter - Unused.

    Context - Context information.

Return Value:

    None.

--*/
{
    PINSTANCE_CONTEXT InstCtx = NULL;
    PFLT_CALLBACK_DATA Data;
    PFLT_INSTANCE Instance = (PFLT_INSTANCE)Context;
    PQUEUE_CONTEXT QueueCtx;
    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;

    UNREFERENCED_PARAMETER( WorkItem );
    UNREFERENCED_PARAMETER( Filter );

    DebugTrace( CSQ_TRACE_PRE_READ,
                ("[Csq]: CancelSafe!PreReadWorkItemRoutine\n") );

    //
    //  Get a pointer to the instance context.
    //

    Status = FltGetInstanceContext( Instance,
                                    &InstCtx );

    if (!NT_SUCCESS( Status ))
    {
        FLT_ASSERT( !"Instance Context is missing" );
        return;
    }

    //
    //  Process all the pended I/O in the cancel safe queue
    //

    for (;;) {

        callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

        PreReadPendIo( InstCtx );

        //
        //  WorkerThreadFlag >= 1;
        //  Here we reduce it to 1.
        //

        InterlockedExchange( &InstCtx->WorkerThreadFlag, 1 );

        //
        //  Remove an I/O from the cancel safe queue.
        //

        Data = FltCbdqRemoveNextIo( &InstCtx->Cbdq,
                                    NULL);

        if (Data) {

            QueueCtx = (PQUEUE_CONTEXT) Data->QueueContext[0];

            PreReadProcessIo( Data );

            //
            //  Check to see if we need to lock the user buffer.
            //
            //  If the FLTFL_CALLBACK_DATA_SYSTEM_BUFFER flag is set we don't
            //  have to lock the buffer because its already a system buffer.
            //
            //  If the MdlAddress is NULL and the buffer is a user buffer,
            //  then we have to construct one in order to look at the buffer.
            //
            //  If the length of the buffer is zero there is nothing to read,
            //  so we cannot construct a MDL.
            //

            if (!FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) &&
                Data->Iopb->Parameters.Read.MdlAddress == NULL &&
                Data->Iopb->Parameters.Read.Length > 0) {

                Status = FltLockUserBuffer( Data );

                if (!NT_SUCCESS( Status )) {

                    //
                    //  If could not lock the user buffer we cannot
                    //  allow the IO to go below us. Because we are
                    //  in a different VA space and the buffer is a
                    //  user mode address, we will either fault or
                    //  corrpt data
                    //

                    DebugTrace( CSQ_TRACE_PRE_READ | CSQ_TRACE_ERROR,
                                ("[Csq]: Failed to lock user buffer (Status = 0x%x)\n",
                                Status) );

                    callbackStatus = FLT_PREOP_COMPLETE;
                    Data->IoStatus.Status = Status;
                }
            }

            //
            //  Complete the I/O
            //

            FltCompletePendedPreOperation( Data,
                                           callbackStatus,
                                           NULL );

            //
            //  Free the extra storage that was allocated for this I/O.
            //

            ExFreeToNPagedLookasideList( &Globals.QueueContextLookaside,
                                         QueueCtx );

        } else {

            //
            //  At this moment it is possible that a new IO is being inserted
            //  into the queue in the CsqInsertIo routine. Now that the queue is
            //  empty, CsqInsertIo needs to make a decision on whether to create
            //  a new worker thread. The decision is based on the race between
            //  the InterlockedIncrement in CsqInsertIo and the
            //  InterlockedDecrement as below. There are two situations:
            //
            //  (1) If the decrement executes earlier before the increment,
            //      the flag will be decremented to 0 so this worker thread
            //      will return. Then CsqInsertIo will increment the flag
            //      from 0 to 1, and therefore create a new worker thread.
            //  (2) If the increment executes earlier before the decrement,
            //      the flag will be first incremented to 2 in CsqInsertIo
            //      so a new worker thread will not be satisfied. Then the
            //      decrement as below will lower the flag down to 1, and
            //      therefore continue this worker thread.
            //

            if (InterlockedDecrement( &InstCtx->WorkerThreadFlag ) == 0) {

                break;
            }

        }
    }

    //
    //  Clean up
    //

    FltReleaseContext(InstCtx);

    FltFreeGenericWorkItem(WorkItem);
}


NTSTATUS
PreReadPendIo(
    _In_ PINSTANCE_CONTEXT InstanceContext
    )
/*++

Routine Description:

    This routine waits for a period of time or until the instance is
    torndown.

Arguments:

    InstanceContext - Supplies a pointer to the instance context.

Return Value:

    The return value is the status of the operation.

--*/
{
    LARGE_INTEGER DueTime;
    NTSTATUS Status;

    //
    //  Delay or get signaled if the instance is torndown.
    //

    DueTime.QuadPart = (LONGLONG) - Globals.TimeDelay;

    Status = KeWaitForSingleObject( &InstanceContext->TeardownEvent,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    &DueTime );

    return Status;
}


NTSTATUS
PreReadProcessIo(
    _Inout_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This routine process the I/O that was removed from the queue.

Arguments:

    Data - Supplies the callback data that was removed from the queue.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );

    return STATUS_SUCCESS;
}


VOID
PreReadEmptyQueueAndComplete(
    _In_ PINSTANCE_CONTEXT InstanceContext
    )
/*++

Routine Description:

    This routine empties the cancel safe queue and complete all the
    pended pre-read operations.

Arguments:

    InstanceContext - Supplies a pointer to the instance context.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;
    PFLT_CALLBACK_DATA Data;
    PQUEUE_CONTEXT QueueCtx;

    do {

        callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

        Data = FltCbdqRemoveNextIo( &InstanceContext->Cbdq,
                                    NULL );

        if (Data) {

            QueueCtx = (PQUEUE_CONTEXT) Data->QueueContext[0];

            //
            //  Check to see if we need to lock the user buffer.
            //
            //  If the FLTFL_CALLBACK_DATA_SYSTEM_BUFFER flag is set we don't
            //  have to lock the buffer because its already a system buffer.
            //
            //  If the MdlAddress is NULL and the buffer is a user buffer,
            //  then we have to construct one in order to look at the buffer.
            //
            //  If the length of the buffer is zero there is nothing to read,
            //  so we cannot construct a MDL.
            //

            if (!FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) &&
                Data->Iopb->Parameters.Read.MdlAddress == NULL &&
                Data->Iopb->Parameters.Read.Length > 0) {

                Status = FltLockUserBuffer( Data );

                if (!NT_SUCCESS( Status )) {

                    //
                    //  If could not lock the user buffer we cannot
                    //  allow the IO to go below us. Because we are
                    //  in a different VA space and the buffer is a
                    //  user mode address, we will either fault or
                    //  corrpt data
                    //

                    callbackStatus = FLT_PREOP_COMPLETE;
                    Data->IoStatus.Status = Status;
                }
            }

            FltCompletePendedPreOperation( Data,
                                           callbackStatus,
                                           NULL );

            ExFreeToNPagedLookasideList( &Globals.QueueContextLookaside,
                                         QueueCtx );
        }

    } while (Data);
}


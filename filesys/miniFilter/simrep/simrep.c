/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    SimRep.c

Abstract:

The Simulate Reparse Sample demonstrates how to return STATUS_REPARSE
on precreates. This allows the filter to redirect opens down one path
to another path. The Precreate path is complicated by network query opens
which come down as Fast IO. Fast IO cannot be redirected with Status Reparse
because reparse only works on IRP based IO.

Simulating reparse points requires that the filter replace the name in the
file object. This will cause Driver Verifier to complain that the filter is
leaking pool and will prevent it from being unloaded. To solve this issue
SimRep attempts to use a Windows 7 Function called IoReplaceFileObjectName
which will allow IO Mgr to replace the name for us with the correct pool tag.
However, on downlevel OS Versions SimRep will go ahead and replace the name
itself.

It is important to note that SimRep only demonstrates how to return
STATUS_REPARSE, not how to deal with file names on NT. SimRep uses two strings
to act as a mapping. When the file open name starts with the "old name mapping"
string the filter replaces it with the "new name mapping" string. This does not
take short names into account.

SimRep can also be configured to redirect renames and creation of hardlinks.
This functionality is demonstrated in the code and can be turned on with a
registry key value indicated in the inf file. To correctly handle rename and
set link operations:
1.  SimRep has to reparse opens with the SL_OPEN_TARGET_DIRECTORY flag set in
    the pre-create, since this is the create that IoManager uses to open the
    target of the rename.
2.  SimRep implements a "pass-through" name provider. It needs to do this so
    that the creates issued to resolve normalized name queries will be seen by
    SimRep and it can redirect them correctly, so as to provide consistent names
    to other filters.
3.  SimRep has to monitor IRP_MJ_SET_INFORMATION for rename and set link
    operations and re-issue them for the correct destination so that filters
    below SimRep are made aware of this redirection.

Note that SimRep simply redirects creates (and optionally renames and set
hardlink) operations. It makes no attempt to virtualize the namespace for
filters above SimRep. So the layers above SimRep will be aware of the
redirection if they query the name of the file once the create, rename or set
hardlink operation is complete.



Environment:

    Kernel mode


--*/

//
//  Enabled warnings
//

#pragma warning(error:4100)     //  Enable-Unreferenced formal parameter
#pragma warning(error:4101)     //  Enable-Unreferenced local variable
#pragma warning(error:4061)     //  Enable-missing enumeration in switch statement
#pragma warning(error:4505)     //  Enable-identify dead functions

//
//  Includes
//


//
// This sample contains OS version specific code. If compiled for VISTA it
// will not run properly on older versions of Windows.
//
#define SIMREP_VISTA (NTDDI_VERSION >= NTDDI_VISTA)

#include <fltKernel.h>


//
//  Memory Pool Tags
//

#define SIMREP_STRING_TAG            'tSpR'
#define SIMREP_REG_TAG               'eRpR'

//
// Constants
//

#define REPLACE_ROUTINE_NAME_STRING L"IoReplaceFileObjectName"

#define REPLACE_QUERY_DIRECTORY_FILE_ROUTINE_NAME_STRING "FltQueryDirectoryFile"


//
//  Context sample filter global data structures.
//

typedef struct _MAPPING_ENTRY {

    //
    //  Path underwhich we want to reparse.
    //

    UNICODE_STRING OldName;

    //
    //  Path to reparse to.
    //

    UNICODE_STRING NewName;

} MAPPING_ENTRY, *PMAPPING_ENTRY;


//
//  Starting with windows 7, the IO Manager provides IoReplaceFileObjectName,
//  but old versions of Windows will not have this function. Rather than just
//  writing our own function, and forfeiting future windows functionality, we can
//  use MmGetRoutineAddr, which will allow us to dynamically import IoReplaceFileObjectName
//  if it exists. If not it allows us to implement the function ourselves.
//

typedef
NTSTATUS
(* PReplaceFileObjectName ) (
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
    _In_ USHORT FileNameLength
    );

typedef
NTSTATUS
(FLTAPI *PFltQueryDirectoryFile)(
            _In_ PFLT_INSTANCE Instance,
            _In_ PFILE_OBJECT FileObject,
            _In_reads_bytes_(Length) PVOID FileInformationBuffer,
            _In_ ULONG Length,
            _In_ FILE_INFORMATION_CLASS FileInformationClass,
            _In_ BOOLEAN ReturnSingleEntry,
            _In_opt_ PUNICODE_STRING FileName,
            _In_ BOOLEAN RestartScan,
            _Out_opt_ PULONG LengthReturned
            );


typedef struct _SIMREP_GLOBAL_DATA {

    //
    // Handle to minifilter returned from FltRegisterFilter()
    //

    PFLT_FILTER Filter;

    //
    //  Structure to hold mapping information.
    //

    MAPPING_ENTRY Mapping;

    //
    //  Pointer to the function we will use to
    //  replace file names.
    //

    PReplaceFileObjectName ReplaceFileNameFunction;

    //
    //  Pointer to the function we will use to
    //  query directory file.
    //

    PFltQueryDirectoryFile QueryDirectoryFileFunction;

    //
    // Flag to control if the filter remaps renames
    //

    BOOLEAN RemapRenamesAndLinks;

#if DBG

    //
    // Field to control nature of debug output
    //

    ULONG DebugLevel;
#endif

} SIMREP_GLOBAL_DATA, *PSIMREP_GLOBAL_DATA;


//
//  Debug helper functions
//

#if DBG


#define DEBUG_TRACE_ERROR                               0x00000001  // Errors - whenever we return a failure code
#define DEBUG_TRACE_LOAD_UNLOAD                         0x00000002  // Loading/unloading of the filter
#define DEBUG_TRACE_INSTANCES                           0x00000004  // Attach / detach of instances

#define DEBUG_TRACE_REPARSE_OPERATIONS                  0x00000008  // Operations that are performed to determine if we should return STATUS_REPARSE
#define DEBUG_TRACE_REPARSED_OPERATIONS                 0x00000010  // Operations that return STATUS_REPARSE
#define DEBUG_TRACE_REPARSED_REISSUE                    0X00000020  // Operations that need to be reissued with an IRP.

#define DEBUG_TRACE_NAME_OPERATIONS                     0x00000040  // Operations involving name provider callbacks

#define DEBUG_TRACE_RENAME_REDIRECTION_OPERATIONS       0x00000080  // Operations involving rename or hardlink redirection

#define DEBUG_TRACE_ALL_IO                              0x00000100  // All IO operations tracked by this filter

#define DEBUG_TRACE_ALL                                 0xFFFFFFFF  // All flags


#define DebugTrace(Level, Data)                     \
    if ((Level) & Globals.DebugLevel) {             \
        DbgPrint Data;                              \
    }


#else

#define DebugTrace(Level, Data)             {NOTHING;}

#endif


//
//  Function that handle driver load/unload and instance setup/cleanup
//

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
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
SimRepGetIoOpenDriverRegistryKey (
    VOID
    );

NTSTATUS
SimRepOpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    );

NTSTATUS
SimRepSetConfiguration(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

VOID SimRepFreeGlobals(
    );

NTSTATUS
SimRepUnload (
    FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
SimRepInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
SimRepInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

//
//  Functions that track operations on the volume
//

FLT_PREOP_CALLBACK_STATUS
SimRepPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SimRepPreNetworkQueryOpen (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

//
//  Functions to support rename and hard link creation remapping
//

FLT_PREOP_CALLBACK_STATUS
SimRepPreSetInformation (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

//
//  Functions that provide string allocation support
//

_When_(return==0, _Post_satisfies_(String->Buffer != NULL))
NTSTATUS
SimRepAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    );

VOID
SimRepFreeUnicodeString (
    _Inout_ PUNICODE_STRING String
    );

NTSTATUS
SimRepReplaceFileObjectName (
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
    _In_ USHORT FileNameLength
    );

BOOLEAN
SimRepCompareMapping(
    _In_ PFLT_FILE_NAME_INFORMATION NameInfo,
    _In_ PUNICODE_STRING MappingPath,
    _In_ BOOLEAN IgnoreCase,
    _Out_opt_ PBOOLEAN ExactMatch
    );

NTSTATUS
SimRepMungeName(
    _In_ PFLT_FILE_NAME_INFORMATION NameInfo,
    _In_ PUNICODE_STRING SubPath,
    _In_ PUNICODE_STRING NewSubPath,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN ExactMatch,
    _Out_ PUNICODE_STRING MungedPath
    );

//
//  Functions that implement a pass through name provider
//

NTSTATUS
SimRepGenerateFileName (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _When_(FileObject->FsContext != NULL, _In_opt_)
    _When_(FileObject->FsContext == NULL, _In_)
    PFLT_CALLBACK_DATA Cbd,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Inout_ PFLT_NAME_CONTROL FileName
    );

NTSTATUS
SimRepNormalizeNameComponent (
    _In_ PFLT_INSTANCE Instance,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    );

#if SIMREP_VISTA
NTSTATUS
SimRepNormalizeNameComponentEx (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    );
#endif

NTSTATUS
SimRepQueryDirectoryFile (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(Length) PVOID FileInformationBuffer,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned
    );


//
//  Filter callback routines
//

FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
        FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
        SimRepPreCreate,
        NULL },

    { IRP_MJ_NETWORK_QUERY_OPEN,
        FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
        SimRepPreNetworkQueryOpen,
        NULL },

    { IRP_MJ_OPERATION_END }
};

//
// Filter registration data structure
//

FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),                     //  Size
    FLT_REGISTRATION_VERSION,                       //  Version
    0,                                              //  Flags
    NULL,                                           //  Context
    Callbacks,                                      //  Operation callbacks
    SimRepUnload,                                   //  Filters unload routine
    SimRepInstanceSetup,                            //  InstanceSetup routine
    SimRepInstanceQueryTeardown,                    //  InstanceQueryTeardown routine
    NULL,                                           //  InstanceTeardownStart routine
    NULL,                                           //  InstanceTeardownComplete routine
    NULL,                                           //  Filename generation support callback
    NULL,                                           //  Filename normalization support callback
    NULL,                                           //  Normalize name component cleanup callback
#if SIMREP_VISTA
    NULL,                                           //  Transaction notification callback
    NULL                                            //  Filename normalization support callback

#endif // SIMREP_VISTA
};


//
//  Filter callback routines with rename handling
//

FLT_OPERATION_REGISTRATION CallbacksWithRename[] = {

    { IRP_MJ_CREATE,
        FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
        SimRepPreCreate,
        NULL },

    { IRP_MJ_NETWORK_QUERY_OPEN,
        FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
        SimRepPreNetworkQueryOpen,
        NULL },

    { IRP_MJ_SET_INFORMATION,
        FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
        SimRepPreSetInformation,
        NULL },

    { IRP_MJ_OPERATION_END }
};

//
// Filter registration data structure with renames
// Filter registers as a name provider and for SetInformation
//

FLT_REGISTRATION FilterRegistrationWithRename = {

    sizeof( FLT_REGISTRATION ),                     //  Size
    FLT_REGISTRATION_VERSION,                       //  Version
    0,                                              //  Flags
    NULL,                                           //  Context
    CallbacksWithRename,                            //  Operation callbacks
    SimRepUnload,                                   //  Filters unload routine
    SimRepInstanceSetup,                            //  InstanceSetup routine
    SimRepInstanceQueryTeardown,                    //  InstanceQueryTeardown routine
    NULL,                                           //  InstanceTeardownStart routine
    NULL,                                           //  InstanceTeardownComplete routine
    SimRepGenerateFileName,                         //  Filename generation support callback
    SimRepNormalizeNameComponent,                   //  Filename normalization support callback
    NULL,                                           //  Normalize name component cleanup callback
#if SIMREP_VISTA
    NULL,                                           //  Transaction notification callback
    SimRepNormalizeNameComponentEx                  //  Filename normalization support callback

#endif // SIMREP_VISTA
};


//
//  Global variables
//

SIMREP_GLOBAL_DATA Globals;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, SimRepGetIoOpenDriverRegistryKey)
#pragma alloc_text(INIT, SimRepOpenServiceParametersKey)
#pragma alloc_text(INIT, SimRepSetConfiguration)
#pragma alloc_text(PAGE, SimRepUnload)
#pragma alloc_text(PAGE, SimRepInstanceSetup)
#pragma alloc_text(PAGE, SimRepInstanceQueryTeardown)
#pragma alloc_text(PAGE, SimRepAllocateUnicodeString)
#pragma alloc_text(PAGE, SimRepFreeUnicodeString)
#pragma alloc_text(PAGE, SimRepReplaceFileObjectName)
#pragma alloc_text(PAGE, SimRepCompareMapping)
#pragma alloc_text(PAGE, SimRepMungeName)
#pragma alloc_text(PAGE, SimRepPreCreate)
#pragma alloc_text(PAGE, SimRepPreNetworkQueryOpen)
#pragma alloc_text(PAGE, SimRepPreSetInformation)
#pragma alloc_text(PAGE, SimRepFreeGlobals)
#pragma alloc_text(PAGE, SimRepGenerateFileName)
#pragma alloc_text(PAGE, SimRepNormalizeNameComponent)
#if SIMREP_VISTA
#pragma alloc_text(PAGE, SimRepNormalizeNameComponentEx)
#endif
#pragma alloc_text(PAGE, SimRepQueryDirectoryFile)

#endif

//
//  Filter driver initialization and unload routines
//

#pragma warning(push)
#pragma warning(disable:4152) // nonstandard extension, function/data pointer conversion in expression

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
    UNICODE_STRING replaceRoutineName;
    PFLT_REGISTRATION Registration;

    //
    //  Default to NonPagedPoolNx for non paged pool allocations where supported.
    //

    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    //
    //  Set default global configuration
    //

#if DBG

    Globals.DebugLevel = DEBUG_TRACE_ALL;

#endif

    Globals.RemapRenamesAndLinks = FALSE;

    RtlInitUnicodeString( &Globals.Mapping.NewName, NULL );

    RtlInitUnicodeString( &Globals.Mapping.OldName, NULL );

    //
    //  Import function to replace file names.
    //

    RtlInitUnicodeString( &replaceRoutineName, REPLACE_ROUTINE_NAME_STRING );

    Globals.ReplaceFileNameFunction = MmGetSystemRoutineAddress( &replaceRoutineName );
    if (Globals.ReplaceFileNameFunction == NULL) {

        Globals.ReplaceFileNameFunction = SimRepReplaceFileObjectName;
    }

    //
    //  If available (Windows Vista or later), use the FltQueryDirectoryFile API.
    //

    Globals.QueryDirectoryFileFunction = FltGetRoutineAddress( REPLACE_QUERY_DIRECTORY_FILE_ROUTINE_NAME_STRING );

    //
    //  Set the filter configuration based on registry keys
    //

    status = SimRepSetConfiguration( DriverObject, RegistryPath );

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[SimRep]: Driver being loaded\n") );

    if (!NT_SUCCESS( status )) {

        goto DriverEntryCleanup;
    }

    //
    //  Register with the filter manager. If the filter is not
    //  configured to remap renames and hardlink creation do not
    //  register name provider or SetInformation callbacks.
    //

    Registration = (Globals.RemapRenamesAndLinks == FALSE) ?
                     &FilterRegistration : &FilterRegistrationWithRename;

    status = FltRegisterFilter( DriverObject,
                                Registration,
                                &Globals.Filter );

    if (!NT_SUCCESS( status )) {

        goto DriverEntryCleanup;
    }

    //
    //  Start filtering I/O
    //

    status = FltStartFiltering( Globals.Filter );

    if (!NT_SUCCESS( status )) {

        FltUnregisterFilter( Globals.Filter );
    }


DriverEntryCleanup:

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[SimRep]: Driver loaded complete (Status = 0x%08X)\n",
                status) );

    if (!NT_SUCCESS( status )) {

        SimRepFreeGlobals();
    }

    return status;
}
#pragma warning(pop)

PFN_IoOpenDriverRegistryKey
SimRepGetIoOpenDriverRegistryKey (
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
SimRepOpenServiceParametersKey (
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

    pIoOpenDriverRegistryKey = SimRepGetIoOpenDriverRegistryKey();

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

            goto SimRepOpenServiceParametersKeyCleanup;
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

            goto SimRepOpenServiceParametersKeyCleanup;
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

            goto SimRepOpenServiceParametersKeyCleanup;
        }
    }

    //
    //  Return value to caller
    //

    *ServiceParametersKey = ParametersKey;

SimRepOpenServiceParametersKeyCleanup:

    if (ServiceRegKey != NULL) {

        ZwClose( ServiceRegKey );
    }

    return status;

}

NTSTATUS
SimRepSetConfiguration(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Descrition:

    This routine sets the filter configuration based on registry values.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - The path key passed to the driver during DriverEntry.

Return Value:

    Returns the status of this operation.


--*/
{
    NTSTATUS status;
    HANDLE driverRegKey = NULL;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG valueLength = sizeof(buffer);
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION mappingValue = NULL;
    ULONG mappingValueLength = 0;
    WCHAR oldMappingTail;
    WCHAR newMappingTail;

    PAGED_CODE();

    //
    //  Open service parameters key to query values from
    //

    status = SimRepOpenServiceParametersKey( DriverObject,
                                             RegistryPath,
                                             &driverRegKey );

    if (!NT_SUCCESS( status )) {

        driverRegKey = NULL;
        goto SimRepSetConfigurationCleanup;
    }

#if DBG

    //
    // Query the debug level
    //

    RtlInitUnicodeString( &valueName, L"DebugLevel" );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              value,
                              valueLength,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        Globals.DebugLevel = *(PULONG)value->Data;
    }

#endif


    //
    // Query the remap rename flag
    //

    RtlInitUnicodeString( &valueName, L"RemapRenamesAndLinks" );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              value,
                              valueLength,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        Globals.RemapRenamesAndLinks = *(PULONG)value->Data > 0 ? TRUE : FALSE;
    }

    //
    //  Query the length of the old mapping.
    //

    RtlInitUnicodeString( &valueName, L"OldMapping" );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &mappingValueLength );

    if (status!=STATUS_BUFFER_TOO_SMALL && status!=STATUS_BUFFER_OVERFLOW) {

        status = STATUS_INVALID_PARAMETER;
        goto SimRepSetConfigurationCleanup;
    }

    //
    //  Extract the old mapping string.
    //

    mappingValue = ExAllocatePoolZero( PagedPool,
                                       mappingValueLength,
                                       SIMREP_REG_TAG );

    if (mappingValue == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto SimRepSetConfigurationCleanup;
    }

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              mappingValue,
                              mappingValueLength,
                              &resultLength );

    if (!NT_SUCCESS( status )) {

        goto SimRepSetConfigurationCleanup;
    }

    if (mappingValue->Type != REG_SZ) {

        status = STATUS_INVALID_PARAMETER;
        goto SimRepSetConfigurationCleanup;
    }

    Globals.Mapping.OldName.MaximumLength = (USHORT)mappingValue->DataLength;

    status = SimRepAllocateUnicodeString( &Globals.Mapping.OldName );

    if (!NT_SUCCESS( status )) {

        goto SimRepSetConfigurationCleanup;
    }

    //
    //  The length which we receive from ZwQueryValueKey contains size for
    //  the NULL termination as well. Since we are dealing with unicode
    //  string we'll chop off the null termination in the length.
    //

    Globals.Mapping.OldName.Length = (USHORT)mappingValue->DataLength - sizeof( UNICODE_NULL );

    RtlCopyMemory(Globals.Mapping.OldName.Buffer,
                  mappingValue->Data,
                  Globals.Mapping.OldName.Length);

    //
    //  Query the length of the new mapping.
    //

    RtlInitUnicodeString( &valueName, L"NewMapping" );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              mappingValue,
                              mappingValueLength,
                              &mappingValueLength );

    if (!NT_SUCCESS( status )) {

        if (status!=STATUS_BUFFER_TOO_SMALL && status!=STATUS_BUFFER_OVERFLOW) {

            goto SimRepSetConfigurationCleanup;
        }

        ExFreePoolWithTag( mappingValue, SIMREP_REG_TAG );

        mappingValue = ExAllocatePoolZero( PagedPool,
                                           mappingValueLength,
                                           SIMREP_REG_TAG );

        if (mappingValue == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SimRepSetConfigurationCleanup;
        }

    }

    //
    //  Extract the new mapping string.
    //

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              mappingValue,
                              mappingValueLength,
                              &mappingValueLength );

    if (!NT_SUCCESS( status )) {

        goto SimRepSetConfigurationCleanup;
    }


    if (mappingValue->Type != REG_SZ) {

        status = STATUS_INVALID_PARAMETER;
        goto SimRepSetConfigurationCleanup;
    }

    Globals.Mapping.NewName.MaximumLength = (USHORT) mappingValue->DataLength;

    status = SimRepAllocateUnicodeString( &Globals.Mapping.NewName );

    if (!NT_SUCCESS( status )) {

        goto SimRepSetConfigurationCleanup;
    }

    //
    //  The length which we receive from ZwQueryValueKey contains size for
    //  the NULL termination as well. Since we are dealing with unicode
    //  string we'll chop off the null termination in the length.
    //

    Globals.Mapping.NewName.Length = (USHORT)mappingValue->DataLength - sizeof( UNICODE_NULL );

    RtlCopyMemory(Globals.Mapping.NewName.Buffer,
                  mappingValue->Data,
                  Globals.Mapping.NewName.Length);


    //
    //  Ensure the old and new mapping are consistent in specifying either files or directories
    //  as determined by the presence of a trailing backslash
    //

    oldMappingTail = (WCHAR)Globals.Mapping.OldName.Buffer[Globals.Mapping.OldName.Length / sizeof( WCHAR ) - 1];
    newMappingTail = (WCHAR)Globals.Mapping.NewName.Buffer[Globals.Mapping.NewName.Length / sizeof( WCHAR ) - 1];

    if ((oldMappingTail != newMappingTail) &&
        ((oldMappingTail == OBJ_NAME_PATH_SEPARATOR) ||
         (newMappingTail == OBJ_NAME_PATH_SEPARATOR))) {

        status = STATUS_INVALID_PARAMETER;
        goto SimRepSetConfigurationCleanup;
    }


SimRepSetConfigurationCleanup:

    if (mappingValue != NULL) {

        ExFreePoolWithTag( mappingValue, SIMREP_REG_TAG );
        mappingValue = NULL;
    }

    if (driverRegKey != NULL) {

        ZwClose( driverRegKey );
    }

    if (!NT_SUCCESS( status )) {

        SimRepFreeUnicodeString( &Globals.Mapping.NewName );
        SimRepFreeUnicodeString( &Globals.Mapping.OldName );
    }

    return status;
}



VOID SimRepFreeGlobals(
    )
/*++

Routine Descrition:

    This routine cleans up the global structure on both
    teardown and initialization failure.

Arguments:

Return Value:

    None.

--*/
{
    PAGED_CODE();

    SimRepFreeUnicodeString( &Globals.Mapping.NewName );
    SimRepFreeUnicodeString( &Globals.Mapping.OldName );
}

NTSTATUS
SimRepUnload (
    FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this filter driver. This is called
    when the minifilter is about to be unloaded. SimRep can unload
    easily because it does not own any IOs. When the filter is unloaded
    existing reparsed creates will continue to work, but new creates will
    not be reparsed. This is fine from the filter's perspective, but could
    result in unexpected bahavior for apps.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_LOAD_UNLOAD,
                ("[SimRep]: Unloading driver\n") );

    FltUnregisterFilter( Globals.Filter );

    SimRepFreeGlobals();

    return STATUS_SUCCESS;
}


//
//  Instance setup/teardown routines.
//

NTSTATUS
SimRepInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.
    SimRep does not attach on automatic attachment, but will attach when asked
    manually.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    if ( FlagOn( Flags, FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT ) ) {

        //
        //  Do not automatically attach to a volume.
        //

        DebugTrace( DEBUG_TRACE_INSTANCES,
                    ("[Simrep]: Instance setup skipped (Volume = %p, Instance = %p)\n",
                    FltObjects->Volume,
                    FltObjects->Instance) );

        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Attach on manual attachment.
    //

    DebugTrace( DEBUG_TRACE_INSTANCES,
                ("[SimRep]: Instance setup started (Volume = %p, Instance = %p)\n",
                 FltObjects->Volume,
                 FltObjects->Instance) );


    return STATUS_SUCCESS;
}


NTSTATUS
SimRepInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request. SimRep only implements it
    because otherwise calls to FltDetachVolume or FilterDetach would
    fail to detach.

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
                ("[SimRep]: Instance query teadown ended (Instance = %p)\n",
                 FltObjects->Instance) );

    return STATUS_SUCCESS;
}


FLT_PREOP_CALLBACK_STATUS
SimRepPreNetworkQueryOpen (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Because network query opens are FastIo operations, they cannot be reparsed.
    This means network query opens which need to be redirected must be failed
    with FLT_PREOP_DISALLOW_FASTIO. This will cause the Io Manager to reissue
    the open as a regular IRP based open. To prevent performance regression,
    only fail network query opens which need to be reparsed.

    This is pageable because it can not be called on the paging path

Arguments:

    Cbd - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    NTSTATUS status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;
    BOOLEAN match;
    PIO_STACK_LOCATION irpSp;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[SimRep]: SimRepPreNetworkQueryOpen -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    // Initialize defaults
    //

    status = STATUS_SUCCESS;
    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK; // pass through - default is no post op callback

    //
    // We only registered for this IRP, so thats all we better get!
    //

    NT_ASSERT( Cbd->Iopb->MajorFunction == IRP_MJ_NETWORK_QUERY_OPEN );
    NT_ASSERT( FLT_IS_FASTIO_OPERATION( Cbd ) );

    irpSp = IoGetCurrentIrpStackLocation(Cbd->Iopb->Parameters.NetworkQueryOpen.Irp);

    //
    //  Check if this is a paging file as we don't want to redirect
    //  the location of the paging file.
    //

    if (FlagOn( irpSp->Flags, SL_OPEN_PAGING_FILE )) {

        DebugTrace( DEBUG_TRACE_ALL_IO,
                    ("[SimRep]: SimRepPreNetworkQueryOpen -> Ignoring paging file open (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreNetworkQueryOpenCleanup;
    }

    //
    //  We are not allowing volume opens to be reparsed in the sample.
    //

    if (FlagOn( Cbd->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN )) {

        DebugTrace( DEBUG_TRACE_ALL_IO,
                    ("[SimRep]: SimRepPreNetworkQueryOpen -> Ignoring volume open (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreNetworkQueryOpenCleanup;

    }

    //
    //  Don't reparse an open by ID because it is not possible to determine create path intent.
    //

    if (FlagOn( irpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID )) {

        goto SimRepPreNetworkQueryOpenCleanup;
    }

    //
    //  A rename should never come on the fast IO path
    //

    NT_ASSERT( irpSp->Flags != SL_OPEN_TARGET_DIRECTORY );

    status = FltGetFileNameInformation( Cbd,
                                        FLT_FILE_NAME_OPENED |
                                        FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreNetworkQueryOpen -> Failed to get name information (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreNetworkQueryOpenCleanup;
    }


    DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS,
                ("[SimRep]: SimRepPreNetworkQueryOpen -> Processing create for file %wZ (Cbd = %p, FileObject = %p)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject) );

    //
    //  Parse the filename information
    //

    status = FltParseFileNameInformation( nameInfo );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreNetworkQueryOpen -> Failed to parse name information for file %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreNetworkQueryOpenCleanup;
    }

    //
    //  Determine if this query involes a path that matches the remapping path.
    //  Note: if the create is case sensitive this comparison must be as well.
    //

    match = SimRepCompareMapping( nameInfo,
                                  &Globals.Mapping.OldName,
                                  !FlagOn( irpSp->Flags, SL_CASE_SENSITIVE ),
                                  NULL );

    if (match) {

        DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS,
                    ("[SimRep]: SimRepPreNetworkQueryOpen -> File name %wZ matches mapping. (Cbd = %p, FileObject = %p)\n"
                     "\tMapping.OldFileName = %wZ\n"
                     "\tMapping.NewFileName = %wZ\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject,
                     Globals.Mapping.OldName,
                     Globals.Mapping.NewName) );

        //
        // Because the file matched the mapping, we need to redirect this open with a new name.
        //

        //
        // We can't return STATUS_REPARSE because it is FastIO. Return
        // FLT_PREOP_DISALLOW_FASTIO, so it will be reissued down the slow path.
        //

        DebugTrace(DEBUG_TRACE_REPARSED_REISSUE,
                    ("[SimRep]: Disallow fast IO that is to a mapped path! %wZ\n",
                     &nameInfo->Name) );

        callbackStatus = FLT_PREOP_DISALLOW_FASTIO;

    }


SimRepPreNetworkQueryOpenCleanup:

    //
    //  Release the references we have acquired
    //

    if (nameInfo != NULL) {

        FltReleaseFileNameInformation( nameInfo );
    }

    if (!NT_SUCCESS( status )) {

        //
        //  An error occurred, fail the query
        //

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreNetworkQueryOpen -> Failed with status 0x%x \n",
                    status) );

        Cbd->IoStatus.Status = status;
        callbackStatus = FLT_PREOP_COMPLETE;
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[SimRep]: SimRepPreNetworkQueryOpen -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return callbackStatus;

}


FLT_PREOP_CALLBACK_STATUS
SimRepPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine does the work for SimRep sample. SimRepPreCreate is called in
    the pre-operation path for IRP_MJ_CREATE and IRP_MJ_NETWORK_QUERY_OPEN.
    The function queries the requested file name for  the create and compares
    it to the mapping path. If the file is down the "old mapping path", the
    filter checks to see if the request is fast io based. If it is we cannot
    reparse the create because fast io does not support STATUS_REPARSE.
    Instead we return FLT_PREOP_DISALLOW_FASTIO to force the io to be reissued
    on the IRP path. If the create is IRP based, then we replace the file
    object's file name field with a new path based on the "new mapping path".

    This is pageable because it could not be called on the paging path

Arguments:

    Cbd - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    NTSTATUS status;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;
    UNICODE_STRING newFileName;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[SimRep]: SimRepPreCreate -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );


    //
    // Initialize defaults
    //

    status = STATUS_SUCCESS;
    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK; // pass through - default is no post op callback

    RtlInitUnicodeString( &newFileName, NULL );

    //
    // We only registered for this irp, so thats all we better get!
    //

    NT_ASSERT( Cbd->Iopb->MajorFunction == IRP_MJ_CREATE );

    //
    //  Check if this is a paging file as we don't want to redirect
    //  the location of the paging file.
    //

    if (FlagOn( Cbd->Iopb->OperationFlags, SL_OPEN_PAGING_FILE )) {

        DebugTrace( DEBUG_TRACE_ALL_IO,
                    ("[SimRep]: SimRepPreCreate -> Ignoring paging file open (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreCreateCleanup;
    }

    //
    //  We are not allowing volume opens to be reparsed in the sample.
    //

    if (FlagOn( Cbd->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN )) {

        DebugTrace( DEBUG_TRACE_ALL_IO,
                    ("[SimRep]: SimRepPreCreate -> Ignoring volume open (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreCreateCleanup;

    }

    //
    //  SimRep does not honor the FILE_OPEN_REPARSE_POINT create option. For a
    //  symbolic the caller would pass this flag, for example, in order to open
    //  the link for deletion. There is no concept of deleting the mapping for
    //  this filter so it is not clear what the purpose of honoring this flag
    //  would be.
    //

    //
    //  Don't reparse an open by ID because it is not possible to determine create path intent.
    //

    if (FlagOn( Cbd->Iopb->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID )) {

        goto SimRepPreCreateCleanup;
    }

    if (FlagOn( Cbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY ) &&
        !Globals.RemapRenamesAndLinks) {

        //
        //  This is a prelude to a rename or hard link creation but the filter
        //  is NOT configured to filter these operations. To perform the operation
        //  successfully and in a consistent manner this create must not trigger
        //  a reparse. Pass through the create without attempting any redirection.
        //

        goto SimRepPreCreateCleanup;

    }

    //
    //  Get the name information.
    //

    if (FlagOn( Cbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY )) {

        //
        //  The SL_OPEN_TARGET_DIRECTORY flag indicates the caller is attempting
        //  to open the target of a rename or hard link creation operation. We
        //  must clear this flag when asking fltmgr for the name or the result
        //  will not include the final component. We need the full path in order
        //  to compare the name to our mapping.
        //

        ClearFlag( Cbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY );

        DebugTrace( DEBUG_TRACE_RENAME_REDIRECTION_OPERATIONS,
                    ("[SimRep]: SimRepPreCreate -> Clearing SL_OPEN_TARGET_DIRECTORY for %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );


        //
        //  Get the filename as it appears below this filter. Note that we use
        //  FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY when querying the filename
        //  so that the filename as it appears below this filter does not end up
        //  in filter manager's name cache.
        //

        status = FltGetFileNameInformation( Cbd,
                                            FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY,
                                            &nameInfo );

        //
        //  Restore the SL_OPEN_TARGET_DIRECTORY flag so the create will proceed
        //  for the target. The file systems depend on this flag being set in
        //  the target create in order for the subsequent SET_INFORMATION
        //  operation to proceed correctly.
        //

        SetFlag( Cbd->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY );


    } else {

        //
        //  Note that we use FLT_FILE_NAME_QUERY_DEFAULT when querying the
        //  filename. In the precreate the filename should not be in filter
        //  manager's name cache so there is no point looking there.
        //

        status = FltGetFileNameInformation( Cbd,
                                            FLT_FILE_NAME_OPENED |
                                            FLT_FILE_NAME_QUERY_DEFAULT,
                                            &nameInfo );
    }

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreCreate -> Failed to get name information (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreCreateCleanup;
    }


    DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS,
                ("[SimRep]: SimRepPreCreate -> Processing create for file %wZ (Cbd = %p, FileObject = %p)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject) );

    //
    //  Parse the filename information
    //

    status = FltParseFileNameInformation( nameInfo );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreCreate -> Failed to parse name information for file %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreCreateCleanup;
    }

    //
    //  Munge the path from the old mapping to new mapping if the query overlaps
    //  the mapping path. Note: if the create is case sensitive this comparison
    //  must be as well.
    //

    status = SimRepMungeName( nameInfo,
                              &Globals.Mapping.OldName,
                              &Globals.Mapping.NewName,
                              !FlagOn( Cbd->Iopb->OperationFlags, SL_CASE_SENSITIVE ),
                              FALSE,
                              &newFileName);

    if (!NT_SUCCESS( status )) {

        if (status == STATUS_NOT_FOUND) {
            status = STATUS_SUCCESS;
        }

        goto SimRepPreCreateCleanup;
    }

    DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS,
                ("[SimRep]: SimRepPreCreate -> File name %wZ matches mapping. (Cbd = %p, FileObject = %p)\n"
                 "\tMapping.OldFileName = %wZ\n"
                 "\tMapping.NewFileName = %wZ\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 Globals.Mapping.OldName,
                 Globals.Mapping.NewName) );


    //
    //  Switch names
    //

    status = Globals.ReplaceFileNameFunction( Cbd->Iopb->TargetFileObject,
                                              newFileName.Buffer,
                                              newFileName.Length );

    if ( !NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreCreate -> Failed to allocate string for file %wZ (Cbd = %p, FileObject = %p)\n",
                    &nameInfo->Name,
                    Cbd,
                    FltObjects->FileObject ));

        goto SimRepPreCreateCleanup;
    }

    //
    //  Set the status to STATUS_REPARSE
    //

    status = STATUS_REPARSE;


    DebugTrace( DEBUG_TRACE_REPARSE_OPERATIONS | DEBUG_TRACE_REPARSED_OPERATIONS,
                ("[SimRep]: SimRepPreCreate -> Returning STATUS_REPARSE for file %wZ. (Cbd = %p, FileObject = %p)\n"
                 "\tNewName = %wZ\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 &newFileName) );

SimRepPreCreateCleanup:

    //
    //  Release the references we have acquired
    //

    SimRepFreeUnicodeString( &newFileName );

    if (nameInfo != NULL) {

        FltReleaseFileNameInformation( nameInfo );
    }

    if (status == STATUS_REPARSE) {

        //
        //  Reparse the open
        //

        Cbd->IoStatus.Status = STATUS_REPARSE;
        Cbd->IoStatus.Information = IO_REPARSE;
        callbackStatus = FLT_PREOP_COMPLETE;

    } else if (!NT_SUCCESS( status )) {

        //
        //  An error occurred, fail the open
        //

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreCreate -> Failed with status 0x%x \n",
                    status) );

        Cbd->IoStatus.Status = status;
        callbackStatus = FLT_PREOP_COMPLETE;
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[SimRep]: SimRepPreCreate -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return callbackStatus;

}


FLT_PREOP_CALLBACK_STATUS
SimRepPreSetInformation (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre callback for handling SetInformation.

Arguments:

    Cdb - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    FLT_PREOP_CALLBACK_STATUS returnStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    NTSTATUS status = STATUS_SUCCESS;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    FILE_INFORMATION_CLASS fileInfoClass;
    PFILE_RENAME_INFORMATION renameInfo = NULL;
    PFILE_RENAME_INFORMATION newRenameInfo = NULL;
    PFILE_LINK_INFORMATION linkInfo = NULL;
    PFILE_LINK_INFORMATION newLinkInfo = NULL;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    UNICODE_STRING newFileName;

    struct {
        HANDLE RootDirectory;
        ULONG FileNameLength;
        PWSTR FileName;
    } setInfo;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( CompletionContext );

    RtlInitUnicodeString(&newFileName, NULL);

    NT_ASSERT( Globals.RemapRenamesAndLinks );

    fileInfoClass = Cbd->Iopb->Parameters.SetFileInformation.FileInformationClass;

#pragma warning( push )
#pragma warning( disable:4061 )
    switch (fileInfoClass) {

        case FileRenameInformation:
        case FileRenameInformationEx:

            //
            //  Note: We should never see a rename of the mapping path \x\y itself
            //  because the name would have been reparsed to the new mapping \a\b.
            //  This is different than the behavior of normal reparse points where
            //  the same operation would reassign the reparse point.
            //

            renameInfo = Cbd->Iopb->Parameters.SetFileInformation.InfoBuffer;

            //
            //  Accessing ReplaceIfExists -
            //    Using FileRenameInformation - renameInfo->ReplaceIfExists
            //    Using FileRenameInformationEx - FlagOn( renameInfo->Flags, FILE_RENAME_REPLACE_IF_EXISTS )
            //

            setInfo.RootDirectory = renameInfo->RootDirectory;
            setInfo.FileNameLength = renameInfo->FileNameLength;
            setInfo.FileName = renameInfo->FileName;

            break;

        case FileLinkInformation:

            linkInfo = Cbd->Iopb->Parameters.SetFileInformation.InfoBuffer;

            //  Accessing ReplaceIfExists - linkInfo->ReplaceIfExists

            setInfo.RootDirectory = linkInfo->RootDirectory;
            setInfo.FileNameLength = linkInfo->FileNameLength;
            setInfo.FileName = linkInfo->FileName;

            break;

        case FileDirectoryInformation:       // 1
        case FileFullDirectoryInformation:   // 2
        case FileBothDirectoryInformation:   // 3
        case FileBasicInformation:           // 4  wdm
        case FileStandardInformation:        // 5  wdm
        case FileInternalInformation:        // 6
        case FileEaInformation:              // 7
        case FileAccessInformation:          // 8
        case FileNameInformation:            // 9
        case FileNamesInformation:           // 12
        case FileDispositionInformation:     // 13
        case FilePositionInformation:        // 14 wdm
        case FileFullEaInformation:          // 15
        case FileModeInformation:            // 16
        case FileAlignmentInformation:       // 17
        case FileAllInformation:             // 18
        case FileAllocationInformation:      // 19
        case FileEndOfFileInformation:       // 20 wdm
        case FileAlternateNameInformation:   // 21
        case FileStreamInformation:          // 22
        case FilePipeInformation:            // 23
        case FilePipeLocalInformation:       // 24
        case FilePipeRemoteInformation:      // 25
        case FileMailslotQueryInformation:   // 26
        case FileMailslotSetInformation:     // 27
        case FileCompressionInformation:     // 28
        case FileObjectIdInformation:        // 29
        case FileCompletionInformation:      // 30
        case FileMoveClusterInformation:     // 31
        case FileQuotaInformation:           // 32
        case FileReparsePointInformation:    // 33
        case FileNetworkOpenInformation:     // 34
        case FileAttributeTagInformation:    // 35
        case FileTrackingInformation:        // 36
        case FileIdBothDirectoryInformation: // 37
        case FileIdFullDirectoryInformation: // 38
        case FileValidDataLengthInformation: // 39
        case FileShortNameInformation:       // 40
        case FileDispositionInformationEx:   // 64

            goto SimRepPreSetInformationCleanup;

        default:

            //
            //  It is risky to pass through information classes that we don't
            //  know about. Try to catch new or invalid classes in testing.
            //

            NT_ASSERTMSG("SimRep passing through unknown information class\n", FALSE);
            goto SimRepPreSetInformationCleanup;
    }
#pragma warning( pop )

    //
    //    When this filter is configured to remap renames and hardlinks we need
    //    to ensure other filters see a consistent destination for the
    //    operation. The FileName buffer will not match the actual rename path
    //    when a reparse is involved and if lower filters pass it to
    //    FltGetDestinationFileNameInformation they will get back the wrong
    //    destination. To fix this we'll need to munge the FileName buffer
    //    explicitly.
    //
    //    The reason FltGetDestinationFileNameInformation gives the correct
    //    destination here is because we send it to ourselves (the current
    //    provider) and, as a name provider, our filter will get the creates
    //    issued for the parent directory name normalization and perform the
    //    reparse.
    //

    status = FltGetDestinationFileNameInformation( FltObjects->Instance,
                                                   FltObjects->FileObject,
                                                   setInfo.RootDirectory,
                                                   setInfo.FileName,
                                                   setInfo.FileNameLength,
                                                   FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER | FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
                                                   &nameInfo );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_RENAME_REDIRECTION_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreSetInformation -> Failed to get destination filename information (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreSetInformationCleanup;
    }

    status = FltParseFileNameInformation( nameInfo );

    if (!NT_SUCCESS( status )) {

        goto SimRepPreSetInformationCleanup;
    }

    //
    //  Stream operations are already consistent regardless of whether the file
    //  is redirected so there is nothing to do.
    //

    if (nameInfo->Stream.Length != 0) {

        goto SimRepPreSetInformationCleanup;
    }

    //
    //  If the operation destion overlaps the new mapping get a new filename
    //  string to send in the request.
    //

    status = SimRepMungeName( nameInfo,
                              &Globals.Mapping.NewName,
                              &Globals.Mapping.NewName,
                              !FlagOn( FltObjects->FileObject->Flags, FO_OPENED_CASE_SENSITIVE ),
                              FALSE,
                              &newFileName );

    if (status == STATUS_NOT_FOUND) {

        //
        //  If the operation destination overlaps the old mapping exactly, get
        //  a new filename string munged with the new mapping to send in the
        //  request. This is a special case where our name provider will not
        //  perform the reparse during name resolution because the parent
        //  directories don't overlap the mapping.
        //

        status = SimRepMungeName( nameInfo,
                                  &Globals.Mapping.OldName,
                                  &Globals.Mapping.NewName,
                                  !FlagOn( FltObjects->FileObject->Flags, FO_OPENED_CASE_SENSITIVE ),
                                  TRUE,
                                  &newFileName );
    }

    if (!NT_SUCCESS( status )) {

        //
        //  The rename doesn't overlap the mapping at all. No need to munge
        //

        if (status == STATUS_NOT_FOUND) {
            status = STATUS_SUCCESS;
        }

        goto SimRepPreSetInformationCleanup;
    }

    //
    //  Explicitly set the munged the name in the set information structure so
    //  lower filters who see this operation will see the correct
    //  destination from FLT_GET_DESTINATION_FILE_NAME_INFORMATION.
    //

    if ((fileInfoClass == FileRenameInformation) ||
        (fileInfoClass == FileRenameInformationEx)) {

         bufferLength = FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName ) + newFileName.Length;

         buffer = ExAllocatePoolZero( PagedPool, bufferLength, SIMREP_STRING_TAG );

         if (buffer == NULL) {

             status = STATUS_INSUFFICIENT_RESOURCES;
             goto SimRepPreSetInformationCleanup;
         }

         newRenameInfo = (PFILE_RENAME_INFORMATION)buffer;

         newRenameInfo->Flags = renameInfo->Flags;
         newRenameInfo->RootDirectory = NULL;
         newRenameInfo->FileNameLength = newFileName.Length;

         RtlCopyMemory( &newRenameInfo->FileName, newFileName.Buffer, newFileName.Length );

     } else if (fileInfoClass == FileLinkInformation) {

        bufferLength = FIELD_OFFSET( FILE_LINK_INFORMATION, FileName ) + newFileName.Length;

        buffer = ExAllocatePoolZero( PagedPool, bufferLength, SIMREP_STRING_TAG );

        if (buffer == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SimRepPreSetInformationCleanup;
        }

        newLinkInfo = (PFILE_LINK_INFORMATION)buffer;

        newLinkInfo->ReplaceIfExists = linkInfo->ReplaceIfExists;
        newLinkInfo->RootDirectory = NULL;
        newLinkInfo->FileNameLength = newFileName.Length;

        RtlCopyMemory( &newLinkInfo->FileName, newFileName.Buffer, newFileName.Length );

    }

    status = FltSetInformationFile( FltObjects->Instance,
                                    FltObjects->FileObject,
                                    buffer,
                                    bufferLength,
                                    fileInfoClass );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_RENAME_REDIRECTION_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepPreSetInformation -> Failed sending FltSetInformationFile (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto SimRepPreSetInformationCleanup;
    }

    Cbd->IoStatus.Status = status;

    returnStatus = FLT_PREOP_COMPLETE;


SimRepPreSetInformationCleanup:

    if (nameInfo) {

        FltReleaseFileNameInformation( nameInfo );
    }

    if (buffer) {

        ExFreePoolWithTag( buffer, SIMREP_STRING_TAG );
    }

    SimRepFreeUnicodeString( &newFileName );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[SimRep]: SimRepSetInformation -> Failed with status 0x%x \n",
                    status) );

        Cbd->IoStatus.Status = status;

        returnStatus = FLT_PREOP_COMPLETE;
    }

    return returnStatus;

}


//
//  Support Routines
//

_When_(return==0, _Post_satisfies_(String->Buffer != NULL))
NTSTATUS
SimRepAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine allocates a unicode string

Arguments:

    Size - the size in bytes needed for the string buffer

    String - supplies the size of the string to be allocated in the MaximumLength field
             return the unicode string

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{

    PAGED_CODE();

    String->Buffer = ExAllocatePoolZero( NonPagedPool,
                                         String->MaximumLength,
                                         SIMREP_STRING_TAG );

    if (String->Buffer == NULL) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[SimRep]: Failed to allocate unicode string of size 0x%x\n",
                    String->MaximumLength) );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    String->Length = 0;

    return STATUS_SUCCESS;
}


VOID
SimRepFreeUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine frees a unicode string

Arguments:

    String - supplies the string to be freed

Return Value:

    None

--*/
{
    PAGED_CODE();

    if (String->Buffer) {

        ExFreePoolWithTag( String->Buffer,
                           SIMREP_STRING_TAG );
        String->Buffer = NULL;
    }

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}


NTSTATUS
SimRepReplaceFileObjectName (
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
    _In_ USHORT FileNameLength
    )
/*++
Routine Description:

    This routine is used to replace a file object's name
    with a provided name. This should only be called if
    IoReplaceFileObjectName is not on the system.
    If this function is used and verifier is enabled
    the filter will fail to unload due to a false
    positive on the leaked pool test.

Arguments:

    FileObject - Pointer to file object whose name is to be replaced.

    NewFileName - Pointer to buffer containing the new name.

    FileNameLength - Length of the new name in bytes.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - No memory to allocate the new buffer.

    STATUS_SUCCESS otherwise.

--*/
{
    PWSTR buffer;
    PUNICODE_STRING fileName;
    USHORT newMaxLength;

    PAGED_CODE();

    fileName = &FileObject->FileName;

    //
    // If the new name fits inside the current buffer we simply copy it over
    // instead of allocating a new buffer (and keep the MaximumLength value
    // the same).
    //
    if (FileNameLength <= fileName->MaximumLength) {

        RtlZeroMemory(fileName->Buffer, fileName->MaximumLength);
        goto CopyAndReturn;
    }

    //
    // Use an optimal buffer size
    //
    newMaxLength = FileNameLength;

    buffer = ExAllocatePoolZero( PagedPool,
                                 newMaxLength,
                                 SIMREP_STRING_TAG );

    if (!buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (fileName->Buffer != NULL) {

        ExFreePool(fileName->Buffer);
    }

    fileName->Buffer = buffer;
    fileName->MaximumLength = newMaxLength;

CopyAndReturn:

    fileName->Length = FileNameLength;
    RtlCopyMemory(fileName->Buffer, NewFileName, FileNameLength);

    return STATUS_SUCCESS;
}


NTSTATUS
SimRepMungeName(
    _In_ PFLT_FILE_NAME_INFORMATION NameInfo,
    _In_ PUNICODE_STRING SubPath,
    _In_ PUNICODE_STRING NewSubPath,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN ExactMatch,
    _Out_ PUNICODE_STRING MungedPath
    )
/*++
Routine Description:

    This routine will create a new path by munginging a new subpath
    over and existing subpath.

Arguments:

    NameInfo - Pointer to the name information for the file.

    SubPath - The path to munge.

    IgnoreCase - If TRUE do a case insenstive comparison.

    ExactMatch - If TRUE only proceed if the whole path will be replaced

    MungedPath - A unicode string to received the munged path created. The
                 buffer of the string will be allocated in this function.

Return Value:

    STATUS_SUCCESS - the path was successfully munged
    STATUS_NOT_FOUND - the SubPath was not found or is not an exact match
    An appropriate NTSTATUS error otherwise.

--*/
{
    NTSTATUS status = STATUS_NOT_FOUND;
    BOOLEAN match;
    BOOLEAN exactMatch;
    USHORT length;

    PAGED_CODE();

    match = SimRepCompareMapping( NameInfo, SubPath, IgnoreCase, &exactMatch );

    if (match) {

        if (ExactMatch && !exactMatch) {

            goto SimRepMungeNameCleanup;
        }

        NT_ASSERT( NameInfo->Name.Length >= SubPath->Length );

        length = NameInfo->Name.Length - SubPath->Length + NewSubPath->Length;

        RtlInitUnicodeString( MungedPath, NULL );

        MungedPath->MaximumLength = (USHORT)length;

        status = SimRepAllocateUnicodeString( MungedPath );

        if (!NT_SUCCESS( status )) {

            goto SimRepMungeNameCleanup;
        }

        //
        //  Copy the volume portion of the name (part of the name preceding the matching part)
        //

        RtlCopyUnicodeString( MungedPath, &NameInfo->Volume );

        //
        //  Copy the new file name in place of the matching part of the name
        //

        status = RtlAppendUnicodeStringToString( MungedPath, NewSubPath );

        NT_ASSERT( NT_SUCCESS( status ) );

        //
        //  Copy the portion of the name following the matching part of the name
        //

        RtlCopyMemory( Add2Ptr( MungedPath->Buffer, NameInfo->Volume.Length + NewSubPath->Length ),
                       Add2Ptr( NameInfo->Name.Buffer, NameInfo->Volume.Length + SubPath->Length ),
                       NameInfo->Name.Length - NameInfo->Volume.Length - SubPath->Length );

        //
        //  Compute the final length of the new name
        //

        MungedPath->Length = length;

    }

SimRepMungeNameCleanup:

    return status;
}

BOOLEAN
SimRepCompareMapping(
    _In_ PFLT_FILE_NAME_INFORMATION NameInfo,
    _In_ PUNICODE_STRING MappingPath,
    _In_ BOOLEAN IgnoreCase,
    _Out_opt_ PBOOLEAN ExactMatch
    )
/*++
Routine Description:

    This routine will compare the file specified by the
    name information structure to the given mapping path
    to determine if the file is the mapping path itself
    or a child of the mapping path.

Arguments:

    NameInfo - Pointer to the name information for the file.

    MappingPath - The mapping path to compare against.

    IgnoreCase - If TRUE do a case insenstive comparison.

    ExactMatch - If supplied receives TRUE if the name exactly
                 matches the mapping path.

Return Value:

    TRUE - the file matches the mapping path

    FALSE - the file is not in the mapping path

--*/
{
    UNICODE_STRING fileName;
    BOOLEAN match;
    BOOLEAN exactMatch;

    PAGED_CODE();

    //
    //  The NameInfo parameter is assumed to have been parsed
    //

    NT_ASSERT (FlagOn(NameInfo->NamesParsed, FLTFL_FILE_NAME_PARSED_FINAL_COMPONENT) &&
               FlagOn(NameInfo->NamesParsed, FLTFL_FILE_NAME_PARSED_EXTENSION) &&
               FlagOn(NameInfo->NamesParsed, FLTFL_FILE_NAME_PARSED_STREAM) &&
               FlagOn(NameInfo->NamesParsed, FLTFL_FILE_NAME_PARSED_PARENT_DIR));

    //
    //  Point filename to the name of the file, excluding the name of the volume
    //

    NT_ASSERT( NameInfo->Name.Buffer == NameInfo->Volume.Buffer );
    NT_ASSERT( NameInfo->Name.Length >= NameInfo->Volume.Length);

    match = FALSE;
    exactMatch = FALSE;
    fileName.Buffer = Add2Ptr( NameInfo->Name.Buffer, NameInfo->Volume.Length );
    fileName.MaximumLength = NameInfo->Name.Length - NameInfo->Volume.Length;
    fileName.Length = fileName.MaximumLength;

    //
    //  Check if the filename matches this mapping entry (is the mapping
    //  entry itself or some child directory of the mapping entry)
    //

    if (RtlPrefixUnicodeString( MappingPath, &fileName, IgnoreCase )) {

        if (fileName.Length == MappingPath->Length) {

            //
            //  This path is the mapping itself
            //

            match = TRUE;

            exactMatch = TRUE;

        } else if (fileName.Buffer[(MappingPath->Length/sizeof( WCHAR ))] == OBJ_NAME_PATH_SEPARATOR) {

            //
            //  This path is a child of the mapping
            //

            match = TRUE;
        }

        //
        //  No match here means the path simply overlaps the mapping like
        //  \a\b\c overlaps \a\b\cd.txt
        //

    }

    if (ARGUMENT_PRESENT( ExactMatch )) {
        *ExactMatch = exactMatch;
    }

    return match;
}


//
//  In order to remap renames and hard links correctly SimRep needs
//  to be called as part of name resolution. To achieve this SimRep
//  must be a name provider, albeit a simple "pass through" provider.
//  SimRep is is only demonstrating how to simulate reparse points,
//  not how to virtualize a namespace. Hence the name provider does
//  not munge the names but simply passes the name queries through.
//

NTSTATUS
SimRepGenerateFileName (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _When_(FileObject->FsContext != NULL, _In_opt_)
    _When_(FileObject->FsContext == NULL, _In_)
    PFLT_CALLBACK_DATA Cbd,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Inout_ PFLT_NAME_CONTROL FileName
    )
/*++

Routine Description:

    This routine generates a file name of the type specified in NameFormat
    for the specified file object.

Arguments:

    Instance - Opaque instance pointer for the minifilter driver instance that
    this callback routine is registered for.

    FileObject - The fileobject for which the name is being requested.

    Cbd - If non-NULL, the CallbackData structure defining the operation
        we are in the midst of processing when this name is queried.

    NameOptions - value that specifies the name format, query method, and flags
    for this file name information query

    CacheFileNameInformation - A pointer to a Boolean value specifying whether
    this name can be cached.

    FileName - A pointer to a filter manager-allocated FLT_NAME_CONTROL
    structure to receive the file name on output

Return Value:

    Returns STATUS_SUCCESS if a name could be returned, or the appropriate
    error otherwise.

--*/
{
    PFLT_FILE_NAME_INFORMATION userFileNameInfo = NULL;
    PUNICODE_STRING userFileName;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();


    //
    //  Clear FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER from the name options
    //  We pass the same name options when we issue a name query to satisfy this
    //  name query. We want that name query to be targeted below simrep.sys and
    //  not recurse into simrep.sys
    //

    ClearFlag( NameOptions, FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER );

    if (FileObject->FsContext == NULL) {


        //
        //  This file object has not yet been opened.  We will query the filter
        //  manager for the name and return that name. We must use the original
        //  NameOptions we received in the query. If we were to swallow flags
        //  such as FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY or
        //  FLT_FILE_NAME_DO_NOT_CACHE we could corrupt the name cache.
        //

        status = FltGetFileNameInformation( Cbd,
                                            NameOptions,
                                            &userFileNameInfo );

        if (!NT_SUCCESS( status )) {

            goto SimRepGenerateFileNameCleanup;
        }

        userFileName = &userFileNameInfo->Name;

    } else {

        //
        //  The file has been opened. If the call is not in the context of an IO
        //  operation (we don't have a callback data), we have to get the
        //  filename with FltGetFilenameInformationUnsafe using the fileobject.
        //  Note, the only way we won't have a callback is if someone called
        //  FltGetFileNameInformationUnsafe already.
        //

        if (ARGUMENT_PRESENT( Cbd )) {

            status = FltGetFileNameInformation( Cbd,
                                                NameOptions,
                                                &userFileNameInfo );

        } else {

            status = FltGetFileNameInformationUnsafe( FileObject,
                                                      Instance,
                                                      NameOptions,
                                                      &userFileNameInfo );

        }

        if (!NT_SUCCESS( status )) {

            goto SimRepGenerateFileNameCleanup;
        }

        userFileName = &userFileNameInfo->Name;

    }

    status = FltCheckAndGrowNameControl( FileName,
                                         userFileName->Length );

    if (!NT_SUCCESS( status )) {

        goto SimRepGenerateFileNameCleanup;
    }

    RtlCopyUnicodeString( &FileName->Name, userFileName );

    //
    //  If the file object is unopened then the name of the stream represented by
    //  the file object may change from pre-create to post-create.
    //  For example, the name being opened could actually be a symbolic link
    //

    *CacheFileNameInformation = (FileObject->FsContext != NULL);


SimRepGenerateFileNameCleanup:

    if (userFileNameInfo != NULL) {

        FltReleaseFileNameInformation( userFileNameInfo );
    }

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_NAME_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("SimRepGenerateFileName: failed %x\n",
                    status) );
    }

    return status;
}


NTSTATUS
SimRepNormalizeNameComponent (
    _In_ PFLT_INSTANCE Instance,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    )
/*++

Routine Description:

    This routine normalizes, converts to a long name if needed, a name component.

Arguments:

    Instance - Opaque instance pointer for the minifilter driver instance that
    this callback routine is registered for.

    ParentDirectory - Pointer to a UNICODE_STRING structure that contains the
    name of the parent directory for this name component.

    VolumeNameLength - Length, in bytes, of the parent directory name that is
    stored in the structure that the ParentDirectory parameter points to.

    Component - Pointer to a UNICODE_STRING structure that contains the name
    component to be expanded.

    ExpandComponentName - Pointer to a FILE_NAMES_INFORMATION structure that
    receives the expanded (normalized) file name information for the name component.

    ExpandComponentNameLength - Length, in bytes, of the buffer that the
    ExpandComponentName parameter points to.

    Flags - Name normalization flags.

    NormalizationContext - Pointer to minifilter driver-provided context
    information to be passed in any subsequent calls to this callback routine
    that are made to normalize the remaining components in the same file name
    path.

Return Value:

    Returns STATUS_SUCCESS if a name could be returned, or the appropriate
    error otherwise.

--*/
{
    NTSTATUS status;
    HANDLE directoryHandle = NULL;
    PFILE_OBJECT directoryFileObject = NULL;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN ignoreCase = !BooleanFlagOn( Flags,
                                         FLTFL_NORMALIZE_NAME_CASE_SENSITIVE );

    UNREFERENCED_PARAMETER( NormalizationContext );
    UNREFERENCED_PARAMETER( DeviceNameLength );

    PAGED_CODE();

    //
    //  Validate the buffer is big enough
    //

    if (ExpandComponentNameLength < sizeof(FILE_NAMES_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    InitializeObjectAttributes( &objAttributes,
                                (PUNICODE_STRING)ParentDirectory,
                                OBJ_KERNEL_HANDLE
                                  | (ignoreCase ? OBJ_CASE_INSENSITIVE : 0),
                                NULL,
                                NULL );

    status = FltCreateFile( Globals.Filter,
                            Instance,
                            &directoryHandle,
                            FILE_LIST_DIRECTORY | SYNCHRONIZE, // DesiredAccess
                            &objAttributes,
                            &ioStatusBlock,
                            NULL,                              // AllocationSize
                            FILE_ATTRIBUTE_DIRECTORY
                              | FILE_ATTRIBUTE_NORMAL,         // FileAttributes
                            FILE_SHARE_READ
                              | FILE_SHARE_WRITE
                              | FILE_SHARE_DELETE,             // ShareAccess
                            FILE_OPEN,                         // CreateDisposition
                            FILE_DIRECTORY_FILE
                              | FILE_SYNCHRONOUS_IO_NONALERT
                              | FILE_OPEN_FOR_BACKUP_INTENT,   // CreateOptions
                            NULL,                              // EaBuffer
                            0,                                 // EaLength
                            IO_IGNORE_SHARE_ACCESS_CHECK );    // Flags

    if (!NT_SUCCESS( status )) {

        goto SimRepNormalizeNameComponentCleanup;
    }

    status = ObReferenceObjectByHandle( directoryHandle,
                                        FILE_LIST_DIRECTORY | SYNCHRONIZE, // DesiredAccess
                                        *IoFileObjectType,
                                        KernelMode,
                                        &directoryFileObject,
                                        NULL );


    if (!NT_SUCCESS( status )) {

        goto SimRepNormalizeNameComponentCleanup;
    }

    //
    //  Query the file entry to get the long name
    //

    status = SimRepQueryDirectoryFile( Instance,
                                       directoryFileObject,
                                       ExpandComponentName,
                                       ExpandComponentNameLength,
                                       FileNamesInformation,
                                       TRUE, /* ReturnSingleEntry */
                                       (PUNICODE_STRING)Component,
                                       TRUE, /* restartScan */
                                       NULL );


SimRepNormalizeNameComponentCleanup:


    if (NULL != directoryHandle) {

        FltClose( directoryHandle );
    }

    if (NULL != directoryFileObject) {

        ObDereferenceObject( directoryFileObject );
    }

    return status;

}


#if SIMREP_VISTA
NTSTATUS
SimRepNormalizeNameComponentEx (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    )
/*++

Routine Description:

    This routine normalizes, converts to a long name if needed, a name component.

Arguments:

    Instance - Opaque instance pointer for the minifilter driver instance that
    this callback routine is registered for.

    FileObject - Pointer to the file object for the file whose name is being
    requested or the file that is the target of the IRP_MJ_SET_INFORMATION
    operation if the FLTFL_NORMALIZE_NAME_DESTINATION_FILE_NAME flag is set.
    ee the Flags parameter below for more information.

    ParentDirectory - Pointer to a UNICODE_STRING structure that contains the
    name of the parent directory for this name component.

    VolumeNameLength - Length, in bytes, of the parent directory name that is
    stored in the structure that the ParentDirectory parameter points to.

    Component - Pointer to a UNICODE_STRING structure that contains the name
    component to be expanded.

    ExpandComponentName - Pointer to a FILE_NAMES_INFORMATION structure that
    receives the expanded (normalized) file name information for the name component.

    ExpandComponentNameLength - Length, in bytes, of the buffer that the
    ExpandComponentName parameter points to.

    Flags - Name normalization flags.

    NormalizationContext - Pointer to minifilter driver-provided context
    information to be passed in any subsequent calls to this callback routine
    that are made to normalize the remaining components in the same file name
    path.

Return Value:

    Returns STATUS_SUCCESS if a name could be returned, or the appropriate
    error otherwise.

--*/
{
    NTSTATUS status;
    HANDLE directoryHandle = NULL;
    PFILE_OBJECT directoryFileObject = NULL;
    OBJECT_ATTRIBUTES objAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN ignoreCase = !BooleanFlagOn( Flags,
                                         FLTFL_NORMALIZE_NAME_CASE_SENSITIVE );
    IO_DRIVER_CREATE_CONTEXT createContext;
    TXN_PARAMETER_BLOCK txnBlock;
    PTXN_PARAMETER_BLOCK originalTxnBlock;

    UNREFERENCED_PARAMETER( NormalizationContext );
    UNREFERENCED_PARAMETER( DeviceNameLength );

    PAGED_CODE();

    //
    //  Validate the buffer is big enough
    //

    if (ExpandComponentNameLength < sizeof(FILE_NAMES_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    InitializeObjectAttributes( &objAttributes,
                                (PUNICODE_STRING)ParentDirectory,
                                OBJ_KERNEL_HANDLE
                                  | (ignoreCase ? OBJ_CASE_INSENSITIVE : 0),
                                NULL,
                                NULL );

    ASSERT( ARGUMENT_PRESENT( FileObject ) );

    //
    //  On Vista and beyond, we need to query the normalized name in the context
    //  of the same transaction as the name query
    //

    IoInitializeDriverCreateContext( &createContext );

    originalTxnBlock = IoGetTransactionParameterBlock( FileObject );

    if (originalTxnBlock != NULL) {

        //
        //  Do not propagate the miniversion for the parent open
        //  as directories don't have a miniversion.
        //

        txnBlock.Length = sizeof( txnBlock );
        txnBlock.TransactionObject = originalTxnBlock->TransactionObject;
        txnBlock.TxFsContext = TXF_MINIVERSION_DEFAULT_VIEW;

        createContext.TxnParameters = &txnBlock;
    }

    status = FltCreateFileEx2( Globals.Filter,
                               Instance,
                               &directoryHandle,
                               &directoryFileObject,
                               FILE_LIST_DIRECTORY | SYNCHRONIZE, // DesiredAccess
                               &objAttributes,
                               &ioStatusBlock,
                               NULL,                              // AllocationSize
                               FILE_ATTRIBUTE_DIRECTORY
                                 | FILE_ATTRIBUTE_NORMAL,         // FileAttributes
                               FILE_SHARE_READ
                                 | FILE_SHARE_WRITE
                                 | FILE_SHARE_DELETE,             // ShareAccess
                               FILE_OPEN,                         // CreateDisposition
                               FILE_DIRECTORY_FILE
                                 | FILE_SYNCHRONOUS_IO_NONALERT
                                 | FILE_OPEN_FOR_BACKUP_INTENT,   // CreateOptions
                               NULL,                              // EaBuffer
                               0,                                 // EaLength
                               IO_IGNORE_SHARE_ACCESS_CHECK,      // Flags
                               &createContext );


    if (!NT_SUCCESS( status )) {

        goto SimRepNormalizeNameComponentExCleanup;
    }

    //
    //  Query the file entry to get the long name
    //

    status = SimRepQueryDirectoryFile( Instance,
                                       directoryFileObject,
                                       ExpandComponentName,
                                       ExpandComponentNameLength,
                                       FileNamesInformation,
                                       TRUE, /* ReturnSingleEntry */
                                       (PUNICODE_STRING)Component,
                                       TRUE, /* restartScan */
                                       NULL );


SimRepNormalizeNameComponentExCleanup:


    if (NULL != directoryHandle) {

        FltClose( directoryHandle );
    }

    if (NULL != directoryFileObject) {

        ObDereferenceObject( directoryFileObject );
    }

    return status;
}
#endif

NTSTATUS
SimRepQueryDirectoryFile (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(Length) PVOID FileInformationBuffer,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned
    )
/*++

Routine Description:

    This function is like ZwQueryDirectoryFile for filters

Arguments:

    Instance - Supplies the Instance initiating this IO.

    FileObject - Supplies the file object about which the requested
        information should be changed.

    FileInformation - Supplies a buffer containing the information which should
        be changed on the file.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        changed about the file.

    ReturnSingleEntry - If this parameter is TRUE, SimRepQueryDirectoryFile
    returns only the first entry that is found.

    FileName - An optional pointer to a caller-allocated Unicode string
    containing the name of a file (or multiple files, if wildcards are used)
    within the directory specified by FileHandle. This parameter is optional
    and can be NULL.

    RestartScan - Set to TRUE if the scan is to start at the first entry in
    the directory. Set to FALSE if resuming the scan from a previous call.


Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PFLT_CALLBACK_DATA data;
    NTSTATUS status;

    PAGED_CODE();

    if (Globals.QueryDirectoryFileFunction != NULL) {

        return Globals.QueryDirectoryFileFunction( Instance,
                                                   FileObject,
                                                   FileInformationBuffer,
                                                   Length,
                                                   FileInformationClass,
                                                   ReturnSingleEntry,
                                                   FileName,
                                                   RestartScan,
                                                   LengthReturned );
    }

    //
    //  Customized FltQueryDirectoryFile if it is not exported from FltMgr.
    //

    status = FltAllocateCallbackData( Instance, FileObject, &data );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    data->Iopb->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    data->Iopb->MinorFunction = IRP_MN_QUERY_DIRECTORY;

    data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length = Length;
    data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName = FileName;
    data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass = FileInformationClass;
    data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileIndex = 0;

    data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = FileInformationBuffer;
    data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress = NULL;

    if (RestartScan) {

        data->Iopb->OperationFlags |= SL_RESTART_SCAN;
    }

    if (ReturnSingleEntry) {

        data->Iopb->OperationFlags |= SL_RETURN_SINGLE_ENTRY;
    }

    //
    //  Perform a synchronous operation.
    //

    FltPerformSynchronousIo( data );

    status = data->IoStatus.Status;

    if (ARGUMENT_PRESENT(LengthReturned) &&
        NT_SUCCESS( status )) {

        *LengthReturned = (ULONG) data->IoStatus.Information;
    }

    FltFreeCallbackData( data );

    return status;
}




/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    avscan.c

Abstract:

    This is the main module of the avscan mini-filter driver.
    This filter demonstrates how to implement a transaction-aware
    anti-virus filter.

    Av prefix denotes "Anti-virus" module.

Environment:

    Kernel mode

--*/

#include <initguid.h>
#include "avscan.h"

/*************************************************************************
    Local Function Prototypes
*************************************************************************/

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
AvGetIoOpenDriverRegistryKey (
    VOID
    );

NTSTATUS
AvOpenServiceParametersKey (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServiceRegistryPath,
    _Out_ PHANDLE ServiceParametersKey
    );


NTSTATUS
AvSetConfiguration (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
AvInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
AvInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Unreferenced_parameter_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
AvInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
AvUnload (
    _Unreferenced_parameter_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
AvInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
AvPreOperationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
AvPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
AvPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
AvPreCleanup (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
AvPreFsControl (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

NTSTATUS
AvKtmNotificationCallback (
    _Unreferenced_parameter_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_CONTEXT TransactionContext,
    _In_ ULONG TransactionNotification
    );

NTSTATUS
AvScanAbortCallbackAsync (
    _Unreferenced_parameter_ PFLT_INSTANCE Instance,
    _In_ PFLT_CONTEXT Context,
    _Unreferenced_parameter_ PFLT_CALLBACK_DATA Data
    );

//
//  Local routines
//

BOOLEAN
AvOperationsModifyingFile (
    _In_ PFLT_CALLBACK_DATA Data
    );

NTSTATUS
AvQueryTransactionOutcome(
    _In_ PKTRANSACTION Transaction,
    _Out_ PULONG TxOutcome
    );

NTSTATUS
AvProcessPreviousTransaction (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PAV_STREAM_CONTEXT StreamContext
    );

NTSTATUS
AvProcessTransactionOutcome (
    _Inout_ PAV_TRANSACTION_CONTEXT TransactionContext,
    _In_ ULONG TransactionOutcome
    );

NTSTATUS
AvLoadFileStateFromCache (
    _In_ PFLT_INSTANCE Instance,
    _In_ PAV_FILE_REFERENCE FileId,
    _Out_ LONG volatile* State,
    _Out_ PLONGLONG VolumeRevision,
    _Out_ PLONGLONG CacheRevision,
    _Out_ PLONGLONG FileRevision
    );

NTSTATUS
AvSyncCache (
    _In_     PFLT_INSTANCE      Instance,
    _In_     PAV_STREAM_CONTEXT   StreamContext
    );

BOOLEAN
AvIsPrefetchEcpPresent (
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CALLBACK_DATA Data
    );

BOOLEAN
AvIsStreamAlternate (
    _Inout_ PFLT_CALLBACK_DATA Data
    );

NTSTATUS
AvScan (
    _Inout_  PFLT_CALLBACK_DATA    Data,
    _In_     PCFLT_RELATED_OBJECTS FltObjects,
    _In_     AV_SCAN_MODE          ScanMode,
    _In_     UCHAR                 IOMajorFunctionAtScan,
    _In_     BOOLEAN               IsInTxWriter,
    _Inout_  PAV_STREAM_CONTEXT      StreamContext
    );

VOID
AvDoCancelScanAndRelease (
    _In_ PAV_SCAN_CONTEXT ScanContext,
    _In_ PAV_SECTION_CONTEXT SectionContext
    );

NTSTATUS
AvSendUnloadingToUser (
    VOID
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, AvGetIoOpenDriverRegistryKey)
#pragma alloc_text(INIT, AvOpenServiceParametersKey)
#pragma alloc_text(INIT, AvSetConfiguration)
#pragma alloc_text(PAGE, AvUnload)
#pragma alloc_text(PAGE, AvInstanceQueryTeardown)
#pragma alloc_text(PAGE, AvInstanceSetup)
#pragma alloc_text(PAGE, AvInstanceTeardownStart)
#pragma alloc_text(PAGE, AvInstanceTeardownComplete)
#pragma alloc_text(PAGE, AvPreCreate)
#pragma alloc_text(PAGE, AvPostCreate)
#pragma alloc_text(PAGE, AvPreFsControl)
#pragma alloc_text(PAGE, AvPreCleanup)
#pragma alloc_text(PAGE, AvKtmNotificationCallback)
#pragma alloc_text(PAGE, AvScanAbortCallbackAsync)
#pragma alloc_text(PAGE, AvOperationsModifyingFile)
#pragma alloc_text(PAGE, AvQueryTransactionOutcome)
#pragma alloc_text(PAGE, AvProcessPreviousTransaction)
#pragma alloc_text(PAGE, AvProcessTransactionOutcome)
#pragma alloc_text(PAGE, AvLoadFileStateFromCache)
#pragma alloc_text(PAGE, AvSyncCache)
#pragma alloc_text(PAGE, AvIsPrefetchEcpPresent)
#pragma alloc_text(PAGE, AvIsStreamAlternate)
#pragma alloc_text(PAGE, AvScan)
#pragma alloc_text(PAGE, AvDoCancelScanAndRelease)
#pragma alloc_text(PAGE, AvSendAbortToUser)
#pragma alloc_text(PAGE, AvSendUnloadingToUser)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      AvPreCreate,
      AvPostCreate },

    { IRP_MJ_CLEANUP,
      0,
      AvPreCleanup,
      NULL },

    { IRP_MJ_WRITE,
      0,
      AvPreOperationCallback,
      NULL },

    { IRP_MJ_SET_INFORMATION,
      0,
      AvPreOperationCallback,
      NULL },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      AvPreFsControl,
      NULL },

    { IRP_MJ_OPERATION_END }
};

//
//  Context registraction construct defined in context.c
//

extern const FLT_CONTEXT_REGISTRATION ContextRegistration[];

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextRegistration,                //  Context
    Callbacks,                          //  Operation callbacks

    AvUnload,                           //  MiniFilterUnload

    AvInstanceSetup,                    //  InstanceSetup
    AvInstanceQueryTeardown,            //  InstanceQueryTeardown
    AvInstanceTeardownStart,            //  InstanceTeardownStart
    AvInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  NormalizeNameComponentCallback
    NULL,                               //  NormalizeContextCleanupCallback
    AvKtmNotificationCallback,          //  TransactionNotificationCallback
    NULL,                               //  NormalizeNameComponentExCallback
    AvScanAbortCallbackAsync            //  SectionNotificationCallback
};



NTSTATUS
AvInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are alwasys created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    NTSTATUS status;
    PAV_INSTANCE_CONTEXT instanceContext = NULL;
    BOOLEAN isOnCsv = FALSE;

    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvInstanceSetup: Entered\n") );

    //
    //  Don't attach to network volumes.
    //

    if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

       return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Determine if the filter is attaching to the hidden NTFS volume
    //  that corresponds to a CSV volume. If so do not attach. Note
    //  that it would be feasible for the filter to attach to this
    //  volume as part of a distrubuted filter implementation but that
    //  is beyond the scope of this sample.
    //

    if (VolumeFilesystemType == FLT_FSTYPE_NTFS) {
        isOnCsv = AvIsVolumeOnCsvDisk( FltObjects->Volume );
        if (isOnCsv) {

           return STATUS_FLT_DO_NOT_ATTACH;
        }
    }

    status = FltAllocateContext( Globals.Filter,
                                 FLT_INSTANCE_CONTEXT,
                                 AV_INSTANCE_CONTEXT_SIZE,
                                 NonPagedPoolNx,
                                 &instanceContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvInstanceSetup: allocate instance context failed. status = 0x%x\n", status) );

        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Setup instance context
    //

    RtlZeroMemory(instanceContext, AV_INSTANCE_CONTEXT_SIZE);
    instanceContext->Volume = FltObjects->Volume;
    instanceContext->Instance = FltObjects->Instance;
    instanceContext->VolumeFSType = VolumeFilesystemType;
    instanceContext->IsOnCsvMDS = isOnCsv;

    //
    //  There will be a file state cache table for each NTFS volume instance.
    //  As for other file systems, file id is not unique, and thus we do
    //  not have cache for other kinds of file systems. Since the cache
    //  table is not mandatory to implement an anti-virus filter, we
    //  only have the volatile cache for NTFS, CSVFS and REFS.
    //
    //  It is worth mentioning that the table is potentially very large.
    //  We use an AVL tree to improve insertion and query times. We do not
    //  set an upper bound for the size of the tree which is not optimal.
    //  Consider limiting the size of the tree for a production filter.
    //

    if (FS_SUPPORTS_FILE_STATE_CACHE( VolumeFilesystemType )) {

        //
        //  Initialize file state cache in the instance context.
        //

        ExInitializeResourceLite( &instanceContext->Resource );

        RtlInitializeGenericTable( &instanceContext->FileStateCacheTable,
                                    AvCompareEntry,
                                    AvAllocateGenericTableEntry,
                                    AvFreeGenericTableEntry,
                                   NULL );
    }

    status = FltSetInstanceContext( FltObjects->Instance,
                                    FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                    instanceContext,
                                    NULL );

    //
    //  In all cases, we need to release the instance context at this time.
    //  If we hit an error, it will get freed now.
    //

    FltReleaseContext( instanceContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvInstanceSetup: set instance context failed. status = 0x%x\n", status) );
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Register this instance as a datascan filter. If this call
    //  fails the underlying filesystem does not support using
    //  the filter manager datascan API. Currently only the
    //  the namedpipe and mailslot file systems are unsupported.
    //

    status = FltRegisterForDataScan( FltObjects->Instance );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvInstanceSetup: FltRegisterForDataScan failed. status = 0x%x\n", status) );
        return STATUS_FLT_DO_NOT_ATTACH;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
AvInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

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

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}

VOID
AvInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Unreferenced_parameter_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.
    If we have cache table, we have to clean up the table at this point.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is been deleted.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PLIST_ENTRY scan;
    PLIST_ENTRY next;
    PAV_SCAN_CONTEXT scanCtx = NULL;
    PAV_INSTANCE_CONTEXT instanceContext = NULL;

    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                  ("[AV] AvInstanceTeardownStart: Entered\n") );

    status = FltGetInstanceContext( FltObjects->Instance,
                                    &instanceContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvInstanceTeardownStart: FltGetInstanceContext failed. status = 0x%x\n", status) );
        return;
    }

    //
    //  Search the scan context from the global list.
    //

    AvAcquireResourceExclusive( &Globals.ScanCtxListLock );

    LIST_FOR_EACH_SAFE( scan, next, &Globals.ScanCtxListHead ) {

        scanCtx = CONTAINING_RECORD( scan, AV_SCAN_CONTEXT, List );

        if (scanCtx->FilterInstance != FltObjects->Instance) {

            continue;
        }

        //
        //  Notify the user scan thread to abort the scan.
        //
        status = AvSendAbortToUser(scanCtx->ScanThreadId,
                                   scanCtx->ScanId);


        //
        //  If we fail to send message to the user, then we
        //  do the cancel and cleanup by ourself; otherwise,
        //  the listening thread will call back to cleanup and
        //  I/O request thred will tear down the scan context.
        //

        if (!NT_SUCCESS( status ) || status == STATUS_TIMEOUT) {

            AvFinalizeScanAndSection(scanCtx);
        }
    }

    AvReleaseResource( &Globals.ScanCtxListLock );

    //
    //  Clean up the cache table if the volume supports one.
    //

    if (FS_SUPPORTS_FILE_STATE_CACHE( instanceContext->VolumeFSType )) {
        PAV_GENERIC_TABLE_ENTRY entry = NULL;
        AvAcquireResourceExclusive( &instanceContext->Resource );

        while (!RtlIsGenericTableEmpty( &instanceContext->FileStateCacheTable ) ) {
            entry = RtlGetElementGenericTable(&instanceContext->FileStateCacheTable, 0);
            AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvInstanceTeardownStart: %I64x,%I64x requesting deletion, state:%d\n",
                        entry->FileId.FileId64.UpperZeroes,
                        entry->FileId.FileId64.Value,
                        entry->InfectedState) );
            RtlDeleteElementGenericTable(&instanceContext->FileStateCacheTable, entry);
        }

        AvReleaseResource( &instanceContext->Resource );
    }

    FltReleaseContext( instanceContext );

    FltDeleteInstanceContext( FltObjects->Instance, NULL );
}

VOID
AvInstanceTeardownComplete (
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

    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns the final status of this operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR sd = NULL;

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] DriverEntry: Entered\n") );

    //
    //  Set default global configuration
    //

    RtlZeroMemory( &Globals, sizeof(Globals) );
    InitializeListHead( &Globals.ScanCtxListHead );
    ExInitializeResourceLite( &Globals.ScanCtxListLock );

    Globals.ScanIdCounter = 0;
    Globals.LocalScanTimeout = 30000;
    Globals.NetworkScanTimeout = 60000;

#if DBG

    Globals.DebugLevel = 0xffffffff; // AVDBG_TRACE_ERROR | AVDBG_TRACE_DEBUG;

#endif

    try {

        //
        //  Set the filter configuration based on registry keys
        //

        status = AvSetConfiguration( DriverObject, RegistryPath );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                         ("[AV]: DriverEntry: SetConfiguration FAILED. status = 0x%x\n", status) );

            leave;
        }

        //
        //  Register with FltMgr to tell it our callback routines
        //

        status = FltRegisterFilter( DriverObject,
                                    &FilterRegistration,
                                    &Globals.Filter );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] DriverEntry: FltRegisterFilter FAILED. status = 0x%x\n", status) );
            leave;
        }

        //
        //  Builds a default security descriptor for use with FltCreateCommunicationPort.
        //

        status  = FltBuildDefaultSecurityDescriptor( &sd,
                                                     FLT_PORT_ALL_ACCESS );


        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] DriverEntry: FltBuildDefaultSecurityDescriptor FAILED. status = 0x%x\n", status) );
            leave;
        }
        //
        //  Prepare ports between kernel and user.
        //

        status = AvPrepareServerPort( sd, AvConnectForScan );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] DriverEntry: AvPrepareServerPort Scan Port FAILED. status = 0x%x\n", status) );
            leave;
        }

        status = AvPrepareServerPort( sd, AvConnectForAbort );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] DriverEntry: AvPrepareServerPort Abort Port FAILED. status = 0x%x\n", status) );
            leave;
        }

        status = AvPrepareServerPort( sd, AvConnectForQuery );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] DriverEntry: AvPrepareServerPort Query Port FAILED. status = 0x%x\n", status) );
            leave;
        }

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( Globals.Filter );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] DriverEntry: FltStartFiltering FAILED. status = 0x%x\n", status) );
            leave;
        }

    } finally {

        if ( sd != NULL ) {

            FltFreeSecurityDescriptor( sd );
        }

        if (!NT_SUCCESS( status ) ) {

            if (NULL != Globals.ScanServerPort) {

                 FltCloseCommunicationPort( Globals.ScanServerPort );
            }
            if (NULL != Globals.AbortServerPort) {

                 FltCloseCommunicationPort( Globals.AbortServerPort );
            }
            if (NULL != Globals.QueryServerPort) {

                 FltCloseCommunicationPort( Globals.QueryServerPort );
            }
            if (NULL != Globals.Filter) {

                FltUnregisterFilter( Globals.Filter );
                Globals.Filter = NULL;
            }

            ExDeleteResourceLite( &Globals.ScanCtxListLock );
        }
    }

    return status;
}

NTSTATUS
AvUnload (
    _Unreferenced_parameter_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
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

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                  ("[AV] AvUnload: Entered\n") );

    //
    //  Traverse the scan context list, and cancel the scan if it exists.
    //

    AvAcquireResourceExclusive( &Globals.ScanCtxListLock );
    Globals.Unloading = TRUE;
    AvReleaseResource( &Globals.ScanCtxListLock );

    //
    //  This function will wait for the user to abort the outstanding scan and
    //  close the section
    //

    AvSendUnloadingToUser();

    FltCloseCommunicationPort( Globals.ScanServerPort );
    Globals.ScanServerPort = NULL;
    FltCloseCommunicationPort( Globals.AbortServerPort );
    Globals.AbortServerPort = NULL;
    FltCloseCommunicationPort( Globals.QueryServerPort );
    Globals.QueryServerPort = NULL;
    FltUnregisterFilter( Globals.Filter );  // This will typically trigger instance tear down.
    Globals.Filter = NULL;

    ExDeleteResourceLite( &Globals.ScanCtxListLock );

    return STATUS_SUCCESS;
}


/*************************************************************************
    Local utility routines.
*************************************************************************/

BOOLEAN
AvOperationsModifyingFile (
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we need to set the file to be modified.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

Return Value:

    TRUE - If we want the file associated with the request to be modified.
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    PAGED_CODE();

    switch(iopb->MajorFunction) {

        case IRP_MJ_WRITE:
            return TRUE;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            switch ( iopb->Parameters.FileSystemControl.Common.FsControlCode ) {
                case FSCTL_OFFLOAD_WRITE:
                case FSCTL_WRITE_RAW_ENCRYPTED:
                case FSCTL_SET_ZERO_DATA:
                    return TRUE;
                default: break;
            }
            break;

        case IRP_MJ_SET_INFORMATION:
            switch ( iopb->Parameters.SetFileInformation.FileInformationClass ) {
                case FileEndOfFileInformation:
                case FileValidDataLengthInformation:
                    return TRUE;
                default: break;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

NTSTATUS
AvQueryTransactionOutcome(
    _In_ PKTRANSACTION Transaction,
    _Out_ PULONG TxOutcome
    )
/*++

Routine Description:

    This is a helper function that qeury the KTM that how trasnaction was ended.

Arguments:

    Transaction - Pointer to transaction object.

    TxOutcome - Output. Specifies the type of transaction outcome.

Return Value:

    The status of the operation
--*/
{
    HANDLE transactionHandle;
    NTSTATUS status;
    TRANSACTION_BASIC_INFORMATION txBasicInfo = {0};

    PAGED_CODE();

    status = ObOpenObjectByPointer( Transaction,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    GENERIC_READ,
                                    *TmTransactionObjectType,
                                    KernelMode,
                                    &transactionHandle );

    if (!NT_SUCCESS(status)) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvQueryTransactionOutcome: ObOpenObjectByPointer failed.\n") );
        return status;
    }

    status = ZwQueryInformationTransaction( transactionHandle,
                                            TransactionBasicInformation,
                                            &txBasicInfo,
                                            sizeof(TRANSACTION_BASIC_INFORMATION),
                                            NULL );
    if (!NT_SUCCESS(status)) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvQueryTransactionOutcome: ObOpenObjectByPointer failed.\n") );
        goto Cleanup;
    }

    *TxOutcome = txBasicInfo.Outcome;

Cleanup:

    ZwClose(transactionHandle);

    return status;
}

FORCEINLINE
VOID
AvPropagateFileState(
    _Inout_ PAV_STREAM_CONTEXT StreamContext,
    _In_ ULONG TransactionOutcome
    )
/*++

Routine Description:

    An inline function that propagate the TxState to State in stream context.

Arguments:

    StreamContext - The stream context to be propagated.

    TransactionOutcome - TRANSACTION_OUTCOME enumeration indicating how transaction was ended.


Return Value:

    None.

--*/
{
    //
    //  Only when the transaction was committed will we propagate the state.
    //

    if (TransactionOutcome == TransactionOutcomeCommitted) {

        AV_FILE_INFECTED_STATE oldTxState = InterlockedExchange( &StreamContext->TxState, AvFileModified );
        switch (oldTxState) {
            case AvFileModified:
            case AvFileInfected:
            case AvFileNotInfected:

                //
                //  Propagate the file state from TxState to State.
                //

                InterlockedExchange( &StreamContext->State, oldTxState );
                break;
            case AvFileScanning:

                //
                //  It is possible at KTM callback, file Tx state is still in scanning.
                //  All we can do here is to be conservative, that is to assume that
                //  this commit did involve the modification of the file.
                //

                InterlockedExchange( &StreamContext->State, AvFileModified );
                break;
            default:
                FLT_ASSERTMSG("AvPropagateFileState does not handle the state", FALSE);
                break;
        }
    }

    //
    //  Either cleanup or commited, we need to reset TxState to be default state.
    //

    SET_FILE_TX_MODIFIED( StreamContext );
}

NTSTATUS
AvProcessTransactionOutcome (
    _Inout_ PAV_TRANSACTION_CONTEXT TransactionContext,
    _In_ ULONG TransactionOutcome
    )
/*++

Routine Description:

    This is a helper function that process transaction commitment or rollback

Arguments:

    TransactionContext - Pointer to the minifilter driver's transaction context
        set at PostCreate.

    TransactionOutcome - Specifies the type of notifications. Should be either
        TransactionOutcomeCommitted or TransactionOutcomeAborted

Return Value:

    STATUS_SUCCESS - Returning this status value indicates that the minifilter
        driver is finished with the transaction. This is a success code.

--*/
{
    PLIST_ENTRY      scan;
    PLIST_ENTRY      next;
    PAV_STREAM_CONTEXT streamContext = NULL;
    PAV_TRANSACTION_CONTEXT oldTxCtx = NULL;

    PAGED_CODE();

    //
    //  Tranversing the stream context list, and
    //  sync the TxState -> State.
    //
    //  Either commit or rollback, we need to cleanup the list
    //  Tear down stream context list inside transactionContext
    //

    AvAcquireResourceExclusive( TransactionContext->Resource );

    LIST_FOR_EACH_SAFE( scan, next, &TransactionContext->ScListHead ) {

        streamContext = CONTAINING_RECORD( scan, AV_STREAM_CONTEXT, ListInTransaction );
        oldTxCtx = InterlockedCompareExchangePointer( &streamContext->TxContext, NULL, TransactionContext );
        if (oldTxCtx == TransactionContext) {

            //
            //  The exchange pointer was successful
            //

            RemoveEntryList ( scan );

            AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
              ("[AV] AvProcessTransactionOutcome: Requesting deletion of entry in transaction context: %I64x,%I64x, modified: %d\n",
               streamContext->FileId.FileId64.UpperZeroes,
               streamContext->FileId.FileId64.Value,
               IS_FILE_MODIFIED( streamContext ) ) );
            AvPropagateFileState( streamContext, TransactionOutcome );
            FltReleaseContext( oldTxCtx );
            FltReleaseContext( streamContext );
        }
    }
    SetFlag( TransactionContext->Flags, AV_TXCTX_LISTDRAINED );
    AvReleaseResource( TransactionContext->Resource );

    return STATUS_SUCCESS;
}

NTSTATUS
AvLoadFileStateFromCache (
    _In_ PFLT_INSTANCE Instance,
    _In_ PAV_FILE_REFERENCE FileId,
    _Out_ LONG volatile *State,
    _Out_ PLONGLONG VolumeRevision,
    _Out_ PLONGLONG CacheRevision,
    _Out_ PLONGLONG FileRevision
    )
/*++

Routine Description:

    This routine lookups the file state in the cache table.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

    FileID - The ID to lookup in the cache

    State - The cached state for the file

Return Value:

    Returns the final status of this operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PAV_INSTANCE_CONTEXT instanceContext = NULL;
    AV_GENERIC_TABLE_ENTRY query = {0};
    PAV_GENERIC_TABLE_ENTRY entry = NULL;

    PAGED_CODE();

    //
    //  We should never be trying to cache with an invalid fileID.
    //

    ASSERT( !AV_INVALID_FILE_REFERENCE(*FileId) );

    status = FltGetInstanceContext( Instance,
                                    &instanceContext );

    if (!NT_SUCCESS( status )){
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvLoadFileStateFromCache: failed to get instance context.\n") );
        return status;
    }

    if (! FS_SUPPORTS_FILE_STATE_CACHE( instanceContext->VolumeFSType )) {

        status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    RtlCopyMemory( &query.FileId, FileId, sizeof(query.FileId) );

    AvAcquireResourceShared( &instanceContext->Resource );

    entry = RtlLookupElementGenericTable( &instanceContext->FileStateCacheTable,
                                          &query );

    if (entry != NULL) {
        *State = entry->InfectedState;
        *VolumeRevision = entry->VolumeRevision;
        *CacheRevision = entry->CacheRevision;
        *FileRevision = entry->FileRevision;
    } else {
        status = STATUS_NOT_FOUND;
    }

    AvReleaseResource( &instanceContext->Resource );

Cleanup:

    FltReleaseContext( instanceContext );
    return status;
}

NTSTATUS
AvSyncCache (
    _In_ PFLT_INSTANCE Instance,
    _In_ PAV_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description:

    This routine sync the file state from stream context to volatile cache table.
    It is file system transparent.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

    StreamContext - The stream context of the target file.

Return Value:

    Returns the final status of this operation.

--*/
{
    NTSTATUS  status = STATUS_SUCCESS;
    BOOLEAN   inserted = FALSE;
    AV_GENERIC_TABLE_ENTRY entry = {0};
    PAV_GENERIC_TABLE_ENTRY pEntry = NULL;
    PAV_INSTANCE_CONTEXT   instanceContext = NULL;

    PAGED_CODE();

    if ((NULL == Instance) ||
        (NULL == StreamContext)) {

        return STATUS_INVALID_PARAMETER;
    }

    status = FltGetInstanceContext( Instance, &instanceContext );

    if (!NT_SUCCESS( status )){
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvSyncCache: failed to get instance context.\n") );
        return status;
    }

    //
    //  If the file system is not NTFS, CSVFS or REFS, do nothing
    //

    if (!FS_SUPPORTS_FILE_STATE_CACHE( instanceContext->VolumeFSType )) {
        goto Cleanup;
    }

    //
    //  If originally, we failed to get the file id,
    //  then we do not cache it.
    //

    if (AV_INVALID_FILE_REFERENCE( StreamContext->FileId )) {
        goto Cleanup;
    }

    //
    //  If the file system is NTFS, CSVFS or REFS, overwrite the entry in the
    //  cache table if exists
    //

    RtlCopyMemory( &entry.FileId, &StreamContext->FileId, sizeof(entry.FileId) );

    AvAcquireResourceExclusive( &instanceContext->Resource );

    pEntry = RtlInsertElementGenericTable( &instanceContext->FileStateCacheTable,
                                          (PVOID) &entry,
                                          AV_GENERIC_TABLE_ENTRY_SIZE,
                                          &inserted);
    if (pEntry) {

        //
        //  Note the cache may become stale as files are modified.
        //

        //
        //  It is possible that after entering the following else-if
        //  branch, thread A modifies the file, and before thread A
        //  closes the handle, thread B opens the same file. This
        //  is fine because in such a case, the streamcontext exists
        //  AvLoadFileStateFromCache would return the state in stream
        //  context. Thus, thread B will need to scan the file.
        //

        pEntry->InfectedState = StreamContext->State;
        pEntry->VolumeRevision = StreamContext->VolumeRevision;
        pEntry->CacheRevision = StreamContext->CacheRevision;
        pEntry->FileRevision = StreamContext->FileRevision;

    }

    AvReleaseResource( &instanceContext->Resource );

    if (!pEntry) {
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvSyncCache: RtlInsertElementGenericTable failed.\n") );
    }

Cleanup:

    FltReleaseContext( instanceContext );
    return status;
}

BOOLEAN
AvIsPrefetchEcpPresent (
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CALLBACK_DATA Data
     )
/*++

Routine Description:

    This local function will return if this data stream is alternate or not.
    It by default returns FALSE if it fails to retrieve the name information
    from the file system.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

Return Value:

    TRUE - This data stream is alternate.
    FALSE - This data stream is NOT alternate.

--*/
{
    NTSTATUS status;
    PECP_LIST ecpList;
    PVOID ecpContext;

    PAGED_CODE();

    status = FltGetEcpListFromCallbackData( Filter, Data, &ecpList );

    if (NT_SUCCESS(status) && (ecpList != NULL)) {

        status = FltFindExtraCreateParameter( Filter,
                                              ecpList,
                                              &GUID_ECP_PREFETCH_OPEN,
                                              &ecpContext,
                                              NULL );

        if (NT_SUCCESS(status)) {

            if (!FltIsEcpFromUserMode( Filter, ecpContext )) {
                 return TRUE;
            }
        }
    }

    return FALSE;
}

BOOLEAN
AvIsStreamAlternate(
    _Inout_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This local function will return if this data stream is alternate or not.
    It by default returns FALSE if it fails to retrieve the name information
    from the file system.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

Return Value:

    TRUE - This data stream is alternate.
    FALSE - This data stream is NOT alternate.

--*/
{
    NTSTATUS status;
    BOOLEAN alternate = FALSE;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;

    PAGED_CODE();

    status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
                                        &nameInfo );

    if (!NT_SUCCESS(status)) {

        goto Cleanup;
    }

    status = FltParseFileNameInformation( nameInfo );

    if (!NT_SUCCESS(status)) {

        goto Cleanup;
    }

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                ("[Av]: Dir: %wZ, FinalComponent: %wZ, Stream: %wZ, sLen: %d\n",
                  nameInfo->ParentDir,
                  nameInfo->FinalComponent,
                  nameInfo->Stream,
                  nameInfo->Stream.Length) );

    alternate = (nameInfo->Stream.Length > 0);

Cleanup:
    if (nameInfo != NULL) {

        FltReleaseFileNameInformation( nameInfo );
        nameInfo = NULL;
    }
    return alternate;
}

NTSTATUS
AvScan (
    _Inout_  PFLT_CALLBACK_DATA    Data,
    _In_     PCFLT_RELATED_OBJECTS FltObjects,
    _In_     AV_SCAN_MODE          ScanMode,
    _In_     UCHAR                 IOMajorFunctionAtScan,
    _In_     BOOLEAN               IsInTxWriter,
    _Inout_  PAV_STREAM_CONTEXT    StreamContext
    )
/*++

Routine Description:

    This routine kicks of a scan.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    ScanMode - Can either be AvUserMode or AvKernelMode.

    IOMajorFunctionAtScan - Major function of an IRP.

    StreamContext - The stream context of the target file.

Return Value:

    Returns the final status of this operation.
    STATUS_TIMEOUT - if scan in user mode and the thread reference fails,
                     then it would wait for the scan finish event with a timeout.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    LONGLONG fileSize;
    FLT_VOLUME_PROPERTIES volumeProperties;
    ULONG volumePropertiesLength;

    PAGED_CODE();

    //
    //  Skip the empty file.
    //

    status = AvGetFileSize( FltObjects->Instance,
                            FltObjects->FileObject,
                            &fileSize );

    if (NT_SUCCESS( status ) &&
        (0 == fileSize)) {

        AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                 ("[Av]: AvScan: Skip the EMPTY file.\n") );

        // As if we have 'scanned' this empty file.
        SET_FILE_NOT_INFECTED( StreamContext );
        return STATUS_SUCCESS;
    }

    //
    //  We could cause deadlocks if the thread were suspended once
    //  we have started scanning so enter a critical region.
    //

    FsRtlEnterFileSystem();

    //
    //  Wait here for an existing scan on the stream to complete.
    //  We wait indefinitely since scans themselves will timeout.
    //

    status = FltCancellableWaitForSingleObject( StreamContext->ScanSynchronizationEvent,
                                                NULL,
                                                Data );

    if (NT_SUCCESS(status)) {

        //
        //  Check again in case the file was scanned during the wait
        //  and is already known to be clean
        //

        if (IS_FILE_NEED_SCAN( StreamContext )){

            if (ScanMode == AvUserMode) {

                status = FltGetVolumeProperties( FltObjects->Volume,
                                                 &volumeProperties,
                                                 sizeof(volumeProperties),
                                                 &volumePropertiesLength );
                if (!NT_SUCCESS(status)) {
                    volumeProperties.DeviceType = FILE_DEVICE_NETWORK;
                }

                //
                //  If the scan mode is user mode, the section context will
                //  be created as needed (at MessageNotification callback).
                //
                //  Setting the file state will be done at
                //  MessageNotification callback as well.
                //

                status = AvScanInUser( Data,
                                       FltObjects,
                                       IOMajorFunctionAtScan,
                                       IsInTxWriter,
                                       volumeProperties.DeviceType );

                if (!NT_SUCCESS( status ) || status == STATUS_TIMEOUT) {

                    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                      ("[AV] AvScan: failed to scan the file.\n") );
                }

            } else {

                status = AvScanInKernel( FltObjects,
                                         IOMajorFunctionAtScan,
                                         IsInTxWriter,
                                         StreamContext );

                if (!NT_SUCCESS( status )) {

                    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                      ("[AV] AvScan: failed to scan the file.\n") );
                }

            }

        }

        //
        //  Signal ScanSynchronizationEvent to release any con-current scan of the stream,
        //
        KeSetEvent( StreamContext->ScanSynchronizationEvent, 0, FALSE );

    } else if (IOMajorFunctionAtScan == IRP_MJ_CREATE) {

        //
        //  I/O requesting thread if waiting on synchronization event is cancelled,
        //  we need to clean up the file object too.
        //
        AvCancelFileOpen(Data, FltObjects, status);
    }

    FsRtlExitFileSystem();

    return status;
}

VOID
AvDoCancelScanAndRelease (
    _In_ PAV_SCAN_CONTEXT ScanContext,
    _In_ PAV_SECTION_CONTEXT SectionContext
    )
/*++

Routine Description:

    This routine closes the section object, and released all waiting threads.

Arguments:

    ScanContext - The scan context.

    SectionContext - The section context associated with the scan context.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PAV_STREAM_CONTEXT streamContext = NULL;

    PAGED_CODE();

    AvFinalizeSectionContext( SectionContext );

    status = FltGetStreamContext( ScanContext->FilterInstance,
                                  ScanContext->FileObject,
                                  &streamContext );

    if (NT_SUCCESS( status )) {

        KeSetEvent( streamContext->ScanSynchronizationEvent, 0, FALSE );
        FltReleaseContext( streamContext );
    }

    //
    //  Release I/O request thread.
    //

    KeSetEvent( &ScanContext->ScanCompleteNotification, 0, FALSE );
    return;
}

NTSTATUS
AvSendAbortToUser (
    _In_  ULONG  ScanThreadId,
    _In_  LONGLONG  ScanId
    )
/*++

Routine Description:

    This routine sends an abortion message to the user scan thread.
    The cancel callback is asynchronous and thus we send which
    scan id to abort; otherwise the worker thread in the user
    may abort the 'next' scan task.

Arguments:

    ScanThreadId - The thread identifier of whom to be aborted.

    ScanId - Which scan task to be aborted.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG   replyLength = 0;
    LARGE_INTEGER timeout = {0};
    AV_SCANNER_NOTIFICATION notification = {0};

    PAGED_CODE();

    notification.Message = AvMsgAbortScanning;
    notification.ScanThreadId = ScanThreadId;
    notification.ScanId = ScanId;

    timeout.QuadPart = -((LONGLONG)10) * (LONGLONG)1000 * (LONGLONG)1000; // 1s

    //
    //  Tell the user-scanner to abort the scan.
    //

    status = FltSendMessage( Globals.Filter,
                             &Globals.AbortClientPort,
                             &notification,
                             sizeof(AV_SCANNER_NOTIFICATION),
                             NULL,
                             &replyLength,
                             &timeout );


    if (!NT_SUCCESS( status ) ||
        (status == STATUS_TIMEOUT)) {

        if ((status != STATUS_PORT_DISCONNECTED) &&
            (status != STATUS_TIMEOUT)) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[Av]: AvSendAbortToUser: Failed to FltSendMessage.\n, 0x%08x\n",
                    status) );
        }
        return status;
    }
    return status;
}

NTSTATUS
AvSendUnloadingToUser (
    VOID
    )
/*++

Routine Description:

    This routine sends unloading message to the user program.

Arguments:

    None.

Return Value:

    The return value is the status of the operation.

--*/
{
    ULONG abortThreadId;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyLength = sizeof(ULONG);
    AV_SCANNER_NOTIFICATION notification = {0};

    PAGED_CODE();

    notification.Message = AvMsgFilterUnloading;

    //
    //  Tell the user-scanner that we are unloading the filter.
    //  and waits for its reply.
    //

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                  ("[Av]: AvSendUnloadingToUser: BEFORE...\n") );

    status = FltSendMessage( Globals.Filter,
                             &Globals.AbortClientPort,
                             &notification,
                             sizeof(AV_SCANNER_NOTIFICATION),
                             &abortThreadId,
                             &replyLength,
                             NULL );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[Av]: AvSendUnloadingToUser: Failed to FltSendMessage.\n, 0x%08x\n",
                status) );
    }

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                  ("[Av]: AvSendUnloadingToUser: After...\n") );

    return status;
}

/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/

FLT_PREOP_CALLBACK_STATUS
AvPreOperationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the registered callback routine for filtering
    the "write" operation, i.e. the operations that have potentials
    to modify the file.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACK or
        FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
        the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;
    PAV_STREAM_CONTEXT streamContext = NULL;
    PAV_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    ULONG flags;

    UNREFERENCED_PARAMETER( CompletionContext );

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvPreOperationCallback: Entered\n") );

    if (!AvOperationsModifyingFile(Data)) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  Skip prefetcher handles to avoid deadlocks
    //

    status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &streamHandleContext );
    if (NT_SUCCESS(status)) {

        flags = streamHandleContext->Flags;

        FltReleaseContext( streamHandleContext );

        if (FlagOn( flags, AV_FLAG_PREFETCH )) {
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
    }

    status = FltGetStreamContext( FltObjects->Instance,
                                  FltObjects->FileObject,
                                  &streamContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
              ("[AV] AvPreOperationCallback: get stream context failed. rq: %d\n",
                Data->Iopb->MajorFunction) );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  If this operation is performed in a transacted writer view.
    //

    if ((streamContext->TxContext != NULL) &&
        (FltObjects->Transaction != NULL)) {

#if DBG
        PAV_TRANSACTION_CONTEXT transactionContext = NULL;

        NTSTATUS statusTx = FltGetTransactionContext( FltObjects->Instance,
                                                      FltObjects->Transaction,
                                                      &transactionContext );

        FLT_ASSERTMSG( "Transaction context should not fail, because it is supposed to be created at post create.\n", NT_SUCCESS( statusTx ));
        FLT_ASSERTMSG( "The file's TxCtx should be identical with the target TxCtx.\n",
                       streamContext->TxContext == transactionContext);

        if (NT_SUCCESS( statusTx )) {
            FltReleaseContext( transactionContext );
        }

#endif // DBG

        //
        //  Instead of updating State, we update TxState here,
        //  because the file is part of a transaction writer
        //

        SET_FILE_TX_MODIFIED( streamContext );

    } else {

        //
        //  Consider an optimization for the case where another thread
        //  is already scanning the file as it is being modified here.
        //

        SET_FILE_MODIFIED( streamContext );
    }

    FltReleaseContext( streamContext );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
AvPreFsControl (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre-file system control callback. This filter example does not support save point feature.
    So, we explicitly fail the request here.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACK or
        FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
        the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

    The return value is the status of the operation.

--*/
{

    PAGED_CODE();

    if (Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_TXFS_SAVEPOINT_INFORMATION ) {

        //
        //  We explicitly fail the request of save point here since we
        //  are deprecating savepoint support for the OS version targeted
        //  for this filter.
        //

        Data->IoStatus.Status = STATUS_NOT_SUPPORTED;
        return FLT_PREOP_COMPLETE;
    }
    return AvPreOperationCallback(Data, FltObjects, CompletionContext);
}

FLT_PREOP_CALLBACK_STATUS
AvPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the pre-create completion routine.


Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACK or
        FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
        the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

    FLT_PREOP_SYNCHRONIZE - PostCreate needs to be called back synchronizedly.
    FLT_PREOP_SUCCESS_NO_CALLBACK - PostCreate does not need to be called.

--*/
{
    ULONG_PTR stackLow;
    ULONG_PTR stackHigh;
    PFILE_OBJECT FileObject = Data->Iopb->TargetFileObject;
    AV_STREAMHANDLE_CONTEXT streamHandleContext;


    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvPreCreate: Entered\n") );

    streamHandleContext.Flags = 0;

    //
    //  Stack file objects are never scanned.
    //

    IoGetStackLimits( &stackLow, &stackHigh );

    if (((ULONG_PTR)FileObject > stackLow) &&
        ((ULONG_PTR)FileObject < stackHigh)) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  Directory opens don't need to be scanned.
    //

    if (FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  Skip pre-rename operations which always open a directory.
    //

    if ( FlagOn( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY )) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  Skip paging files.
    //

    if (FlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE )) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  Skip scanning DASD opens
    //

    if (FlagOn( FltObjects->FileObject->Flags, FO_VOLUME_OPEN )) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    // Skip scanning any files being opened by CSVFS for its downlevel
    // processing. This includes filters on the hidden NTFS stack and
    // for filters attached to MUP
    //
    if (AvIsCsvDlEcpPresent( FltObjects->Filter, Data ) ) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }


    //
    //  Flag prefetch handles so they can be skipped. Performing IO
    //  using a prefetch fileobject could lead to a deadlock.
    //

    if (AvIsPrefetchEcpPresent( FltObjects->Filter, Data )) {

        SetFlag( streamHandleContext.Flags, AV_FLAG_PREFETCH );
    }

    *CompletionContext = (PVOID)streamHandleContext.Flags;

    //
    // Perform any CSVFS pre create processing
    //
    AvPreCreateCsvfs( Data, FltObjects );

    //
    // return status can be safely ignored
    //

    //
    //  Return FLT_PREOP_SYNCHRONIZE at PreCreate to ensure PostCreate
    //  is in the same thread at passive level.
    //  EResource can't be acquired at DPC.
    //

    return FLT_PREOP_SYNCHRONIZE;

}

NTSTATUS
AvProcessPreviousTransaction (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PAV_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description:

    This routine is transaction related implementation, and is expected to be
    invoked at post-create. Note that this function will enlist the newly
    allocated transaction context via FltEnlistInTransaction if it needs to.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - The stream context.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PAV_TRANSACTION_CONTEXT oldTxCtx = NULL;
    PAV_TRANSACTION_CONTEXT transactionContext = NULL;

    PAGED_CODE();

    if (FltObjects->Transaction != NULL ) {

        //
        //  Get transaction context
        //

        status = AvFindOrCreateTransactionContext( FltObjects,
                                                   &transactionContext );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvProcessPreviousTransaction: AvFindOrCreateTransactionContext FAILED\n") );
            transactionContext = NULL;
            goto Cleanup;
        }

        //
        //  Enlist it if haven't.
        //

        if (! FlagOn(transactionContext->Flags, AV_TXCTX_ENLISTED) ) {

            //
            //  You can also consider to register TRANSACTION_NOTIFY_PREPARE,
            //  and scan the file at TRANSACTION_NOTIFY_PREPARE callback if it was modified.
            //

            status = FltEnlistInTransaction( FltObjects->Instance,
                                             FltObjects->Transaction,
                                             transactionContext,
                                             TRANSACTION_NOTIFY_COMMIT_FINALIZE | TRANSACTION_NOTIFY_ROLLBACK  );

            if (!NT_SUCCESS( status ) &&
                (status != STATUS_FLT_ALREADY_ENLISTED)) {

                AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                      ("[AV] AvProcessPreviousTransaction: FltEnlistInTransaction FAILED!!!!\n") );
                goto Cleanup;
            }
            status = STATUS_SUCCESS;
            SetFlag( transactionContext->Flags, AV_TXCTX_ENLISTED );
        }
    }

    //
    //  Here we have five cases:
    //
    //  1)
    //    oldTxCtx : NULL
    //    transCtx : B
    //  2)
    //    oldTxCtx : A
    //    transCtx : NULL
    //  3)
    //    oldTxCtx : A
    //    transCtx : B
    //  4)
    //    oldTxCtx : A
    //    transCtx : A
    //  5)
    //    oldTxCtx : NULL
    //    transCtx : NULL
    //

    //
    //  Synchronize the replacement of StreamContext->TxContext with KTM callback.
    //

    oldTxCtx = InterlockedExchangePointer( &StreamContext->TxContext, transactionContext );

    if (oldTxCtx != transactionContext) {  // case 1,2,3

        //
        //  txOutcome is by default set as committed because we are conservative about
        //  propagating the file state if AvQueryTransactionOutcome failed, it may cause
        //  redundant scan but will not overlook infected file anyway.
        //

        ULONG txOutcome = TransactionOutcomeCommitted;

        if ( oldTxCtx == NULL ) { // case 1

            //  This file was not linked in a transaction context yet, and is about to.
            //
            //  Increment TxContext's reference count because stream context has a reference to it.
            //

            FltReferenceContext ( transactionContext );

            //
            //  Before insertion into the FcList in transaction context, we increment stream context's ref count
            //

            AvAcquireResourceExclusive( transactionContext->Resource );

            if (!FlagOn(transactionContext->Flags, AV_TXCTX_LISTDRAINED)) {

                FltReferenceContext ( StreamContext );   // Q
                InsertTailList( &transactionContext->ScListHead,
                                &StreamContext->ListInTransaction );
            }

            AvReleaseResource( transactionContext->Resource );

            goto Cleanup;
        }

        // case 2,3

        //
        //  We have to query transaction outcome in order to know how we
        //  can process the previously outstanding transaction context.
        //

        status = AvQueryTransactionOutcome( oldTxCtx->Transaction, &txOutcome );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[AV] AvProcessPreviousTransaction: AvQueryTransactionOutcome FAILED!!!!\n") );

            //
            //  We have exchanged the pointer anyway, if we cannot query its outcome,
            //  we have to go through.
            //
        }

        AvAcquireResourceExclusive( oldTxCtx->Resource );
        RemoveEntryList ( &StreamContext->ListInTransaction );
        AvReleaseResource( oldTxCtx->Resource );

        AvPropagateFileState ( StreamContext, txOutcome );

        if ( transactionContext ) {  // case 3

            FltReferenceContext( transactionContext );

            AvAcquireResourceExclusive( transactionContext->Resource );

            if (!FlagOn(transactionContext->Flags, AV_TXCTX_LISTDRAINED)) {

                InsertTailList( &transactionContext->ScListHead,
                                &StreamContext->ListInTransaction );

            } else {

                FltReleaseContext( StreamContext );
            }

            AvReleaseResource( transactionContext->Resource );

        } else { // case 2

            FltReleaseContext ( StreamContext );  // Release reference count at Q
        }

        // case 2,3

        FltReleaseContext( oldTxCtx );  // Release reference count in stream context originally.

    }

    //
    //  We don't care about case 4, 5.
    //

Cleanup:

    if (transactionContext) {

        FltReleaseContext( transactionContext );  // Release the ref count grabbed at AvFindOrCreateTransactionContext(...)
    }

    return status;
}

FLT_POSTOP_CALLBACK_STATUS
AvPostCreate (_Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-create completion routine.
    In this routine, stream context and/or transaction context shall be
    created if not exits.

    Note that we only allocate and set the stream context to filter manager
    at post create.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-create routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status = Data->IoStatus.Status;
    BOOLEAN isDir = FALSE;
    BOOLEAN isTxWriter = FALSE;

    PAV_STREAM_CONTEXT streamContext = NULL;
    PAV_STREAM_CONTEXT oldStreamContext = NULL;
    PAV_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    ACCESS_MASK desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;

    BOOLEAN updateRevisionNumbers;
    LONGLONG VolumeRevision, CacheRevision, FileRevision;

    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    if (!NT_SUCCESS( status ) ||
        (status == STATUS_REPARSE)) {

        //
        //  File Creation may fail.
        //

        AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvPostCreate: file creation failed\n") );

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  After creation, skip it if it is directory.
    //

    status = FltIsDirectory( FltObjects->FileObject,
                             FltObjects->Instance,
                             &isDir );

    //
    //  If FltIsDirectory failed, we do not know if it is a directoy,
    //  we let it go through because if it is a directory, it will fail
    //  at section creation anyway.
    //

    if ( NT_SUCCESS( status ) && isDir ) {

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  We skip the encrypted file open without FILE_WRITE_DATA and FILE_READ_DATA
    //  This is because if application calls OpenEncryptedFileRaw(...) for backup,
    //  it won't have to decrypt the file. In such case, if we scan it, we will hit
    //  an assertion error in NTFS because it does not have the encryption context.
    //  Thus, we have to skip the encrypted file not open for read/write.
    //

    if (!(FlagOn(desiredAccess, FILE_WRITE_DATA)) &&
        !(FlagOn(desiredAccess, FILE_READ_DATA)) ) {

        BOOLEAN encrypted = FALSE;
        status = AvGetFileEncrypted( FltObjects->Instance,
                                     FltObjects->FileObject,
                                     &encrypted );
        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvPostCreate: AvGetFileEncrypted FAILED!! \n0x%x\n", status) );
        }
        if (encrypted) {

            return FLT_POSTOP_FINISHED_PROCESSING;
        }
    }

    //
    //  In this sample, we skip the alternate data stream. However, you may decide
    //  to scan it and modify accordingly.
    //

    if (AvIsStreamAlternate( Data )) {

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  Skip a prefetch open and flag it so we skip subsequent
    //  IO operations on the handle.
    //

    if (FlagOn((ULONG_PTR)CompletionContext, AV_FLAG_PREFETCH)) {

        if (!FltSupportsStreamHandleContexts( FltObjects->FileObject )) {

            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        status = AvCreateStreamHandleContext( FltObjects->Filter,
                                              &streamHandleContext );

        if (!NT_SUCCESS(status)) {

            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        SetFlag( streamHandleContext->Flags, AV_FLAG_PREFETCH );

        status = FltSetStreamHandleContext( FltObjects->Instance,
                                            FltObjects->FileObject,
                                            FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                            streamHandleContext,
                                            NULL );

        FltReleaseContext( streamHandleContext );

        if (!NT_SUCCESS(status)) {

            //
            // Shouldn't find the handle already set
            //

            ASSERT( status != STATUS_FLT_CONTEXT_ALREADY_DEFINED );
        }

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  Find or create a stream context
    //

    status = FltGetStreamContext( FltObjects->Instance,
                                  FltObjects->FileObject,
                                  &streamContext );

    if (status == STATUS_NOT_FOUND) {

        //
        //  Create a stream context
        //

        status = AvCreateStreamContext( FltObjects->Filter, &streamContext );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                          ("[Av]: Failed to create stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                           status,
                           FltObjects->FileObject,
                           FltObjects->Instance) );

            return FLT_POSTOP_FINISHED_PROCESSING;
        }

        //
        //  Attempt to get the stream infected state from our cache
        //

        status = AvGetFileId( FltObjects->Instance, FltObjects->FileObject, &streamContext->FileId );

        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                          ("[Av]: Failed to get file id with status 0x%x. (FileObject = %p, Instance = %p)\n",
                            status,
                            FltObjects->FileObject,
                            FltObjects->Instance) );

            //
            //  File id is optional and therefore should not affect the scan logic.
            //

            AV_SET_INVALID_FILE_REFERENCE( streamContext->FileId )

        } else {

            //
            //  This function will load the file infected state from the
            //  cache if the fileID is valid. Even if this function fails,
            //  we still have to move on because the cache is optional.
            //

            AvLoadFileStateFromCache( FltObjects->Instance,
                                      &streamContext->FileId,
                                      &streamContext->State,
                                      &streamContext->VolumeRevision,
                                      &streamContext->CacheRevision,
                                      &streamContext->FileRevision );
        }

        //
        //  Set the new context we just allocated on the file object
        //

        status = FltSetStreamContext( FltObjects->Instance,
                                      FltObjects->FileObject,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      streamContext,
                                      &oldStreamContext );

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

                //
                //  Race condition. Someone has set a context after we queried it.
                //  Use the already set context instead
                //

                AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                              ("[Av]: Race: Stream context already defined. Retaining old stream context %p (FileObject = %p, Instance = %p)\n",
                               oldStreamContext,
                               FltObjects->FileObject,
                               FltObjects->Instance) );

                FltReleaseContext( streamContext );

                streamContext = oldStreamContext;

            } else {

                AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                              ("[Av]: Failed to set stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                               status,
                               FltObjects->FileObject,
                               FltObjects->Instance) );
                goto Cleanup;
            }
        }

    } else if (!NT_SUCCESS(status)) {

        //
        //  We will get here if stream contexts are not supported
        //

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                      ("[Av]: Failed to get stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                       status,
                       FltObjects->FileObject,
                       FltObjects->Instance) );

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  If successfully opened a file with the desired access matching
    //  the "exclusive write" from a TxF point of view, we can guarantee that
    //  if previous transaction context exists, it must have been comitted
    //  or rollbacked.
    //

    if (FlagOn( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess,
                FILE_WRITE_DATA | FILE_APPEND_DATA |
                DELETE | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
                WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY ) ) {

        //
        //  Either this file is opened in a transaction context or not,
        //  we need to process the previous transaction if it exists.
        //  AvProcessPreviousTransaction(...) handles these cases.
        //

        status = AvProcessPreviousTransaction ( FltObjects,
                                                streamContext );
        if (!NT_SUCCESS( status )) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvPostCreate: AvProcessTransaction FAILED!! \n") );

            goto Cleanup;
        }

        isTxWriter = (FltObjects->Transaction != NULL);
    }

    //
    // Perform any CSVFS specific processing
    //
    AvPostCreateCsvfs( Data,
                       FltObjects,
                       streamContext,
                       &updateRevisionNumbers,
                       &VolumeRevision,
                       &CacheRevision,
                       &FileRevision );
    //
    // Ignore return status
    //



    if (IS_FILE_NEED_SCAN( streamContext )) {

        status = AvScan( Data,
                         FltObjects,
                         AvUserMode,
                         Data->Iopb->MajorFunction,
                         isTxWriter,
                         streamContext );
        if (!NT_SUCCESS( status ) ||
            (STATUS_TIMEOUT == status)) {

            AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvPostCreate: AvScan FAILED!! \n") );

            goto Cleanup;
        }
    }


    //
    // If needed, update the stream context with the latest revision
    // numbers that correspond to the verion just scanned
    //
    if (updateRevisionNumbers) {
        streamContext->VolumeRevision = VolumeRevision;
        streamContext->CacheRevision = CacheRevision;
        streamContext->FileRevision = FileRevision;

        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                      ("[Av]: AvPostCreate: RevisionNumbers updated to %I64x:%I64x:%I64x\n",
                      VolumeRevision,
                      CacheRevision,
                      FileRevision)
                      );
    }

    if (IS_FILE_INFECTED( streamContext )) {

        //
        //  If the file is infected, deny the access.
        //
        AvCancelFileOpen(Data, FltObjects, STATUS_VIRUS_INFECTED);

        //
        //  If the scan timed-out or scan was failed, we let the create succeed,
        //  and it may cause security hole;
        //
        //  Alternatively, you can add a state called AvFileScanFailure or equivalent,
        //  add a condition here and fail the create.  This option will have better
        //  protection from viruses, but the apps will see the failures due to a
        //  lengthy scan or scan failure. It's a trade-off.
        //
        goto Cleanup;
    }

Cleanup:

    FltReleaseContext( streamContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
AvPreCleanup (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre-cleanup callback. Make the stream context persistent in the volatile cache.
    If the file is transacted, it will be synced at KTM notification callback
    if committed.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACK or
        FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
        the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;
    BOOLEAN encrypted = FALSE;
    PAV_STREAM_CONTEXT streamContext = NULL;
    PAV_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    ULONG_PTR stackLow;
    ULONG_PTR stackHigh;

    BOOLEAN updateRevisionNumbers;
    LONGLONG VolumeRevision, CacheRevision, FileRevision;

    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    //
    //  Skip scan on prefetcher handles to avoid deadlocks
    //

    status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &streamHandleContext );
    if (NT_SUCCESS(status)) {

        if (FlagOn( streamHandleContext->Flags, AV_FLAG_PREFETCH )) {

            //
            //  Because the Memory Manager can cache the file object
            //  and use it for other applications performing mapped I/O,
            //  whenever a Cleanup operation is seen on a prefetcher
            //  file object, that file object should no longer be
            //  considered prefetcher-opened.
            //

            RtlInterlockedClearBits( &streamHandleContext->Flags,
                                     AV_FLAG_PREFETCH );

            FltDeleteStreamHandleContext( FltObjects->Instance,
                                          FltObjects->FileObject,
                                          NULL );

            FltReleaseContext( streamHandleContext );

            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }

        FltReleaseContext( streamHandleContext );
    }

    //
    //  Stack file objects are never scanned.
    //

    IoGetStackLimits( &stackLow, &stackHigh );

    if (((ULONG_PTR)FltObjects->FileObject > stackLow) &&
        ((ULONG_PTR)FltObjects->FileObject < stackHigh)) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltGetStreamContext( FltObjects->Instance,
                                  FltObjects->FileObject,
                                  &streamContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
              ("[AV] AvPreCleanup: find stream context failed.\n") );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  We skip encrypted files at cleanup time because we cannot be
    //  sure if the file is open raw for backup. It will get scanned
    //  on the next open anyway.
    //

    status = AvGetFileEncrypted( FltObjects->Instance,
                                 FltObjects->FileObject,
                                 &encrypted );
    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvPreCleanup: AvGetFileEncrypted FAILED!! \n") );

        goto Cleanup;
    }

    if (encrypted) {

        goto Cleanup;
    }

    AvPreCleanupCsvfs( Data,
                       FltObjects,
                       streamContext,
                       &updateRevisionNumbers,
                       &VolumeRevision,
                       &CacheRevision,
                       &FileRevision );

    //
    //  For applications, the typical calling sequence is, close the file handle
    //  and commit/rollback the changes. We skip the scan here for
    //  transacted writer because we do not know if the change will be
    //  rollbacked or not. If it eventually commits, it will be scanned
    //  at next create anyway. However, if it rollbacks, the scan here will
    //  be redundant.
    //

    if ((streamContext->TxContext == NULL) &&
        IS_FILE_MODIFIED( streamContext )) {

        status = AvScan( Data,
                         FltObjects,
                         AvUserMode,
                         Data->Iopb->MajorFunction,
                         FALSE,
                         streamContext );

        if (!NT_SUCCESS( status ) || STATUS_TIMEOUT == status) {

            AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvPreCleanup: AvScan FAILED!! \n") );

            goto Cleanup;
        }


        //
        // If needed, update the stream context with the latest revision
        // numbers that correspond to the verion just scanned
        //
        if (updateRevisionNumbers) {
            streamContext->VolumeRevision = VolumeRevision;
            streamContext->CacheRevision = CacheRevision;
            streamContext->FileRevision = FileRevision;

            AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                          ("[Av]: AvPreCleanup: RevisionNumbers updated to %I64x:%I64x:%I64x\n",
                          VolumeRevision,
                          CacheRevision,
                          FileRevision)
                          );
        }

    }

Cleanup:

    //
    //  We only insert the entry when the file is clean or infected.
    //

    if (!IS_FILE_MODIFIED( streamContext ) ||
        IS_FILE_INFECTED( streamContext )) {

        if (!NT_SUCCESS ( AvSyncCache( FltObjects->Instance, streamContext ))) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                      ("[AV] AvPreCleanup: AvSyncCache FAILED!! \n") );
        }
    }

    FltReleaseContext( streamContext );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS
AvKtmNotificationCallback (
    _Unreferenced_parameter_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_CONTEXT TransactionContext,
    _In_ ULONG TransactionNotification
    )
/*++

Routine Description:

    The registered routine of type PFLT_TRANSACTION_NOTIFICATION_CALLBACK
    in FLT_REGISTRATION structure.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    TransactionContext - Pointer to the minifilter driver's transaction context
        set at PostCreate.

    TransactionNotification - Specifies the type of notifications that the
        filter manager is sending to the minifilter driver.

Return Value:

    STATUS_SUCCESS - Returning this status value indicates that the minifilter
        driver is finished with the transaction. This is a success code.

    STATUS_PENDING - Returning this status value indicates that the minifilter
        driver is not yet finished with the transaction. This is a success code.

--*/
{
    PAV_TRANSACTION_CONTEXT transactionContext = (PAV_TRANSACTION_CONTEXT) TransactionContext;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( FltObjects );

    FLT_ASSERTMSG("[AV] AvKtmNotificationCallback: The expected type of notifications registered at FltEnlistInTransaction(...).\n",
                  FlagOn( TransactionNotification,
                          (TRANSACTION_NOTIFY_COMMIT_FINALIZE | TRANSACTION_NOTIFY_ROLLBACK) ) );

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvKtmNotificationCallback: Entered\n") );

    if (NULL != transactionContext) {

        if ( FlagOn( TransactionNotification, TRANSACTION_NOTIFY_COMMIT_FINALIZE ) ) {

            return AvProcessTransactionOutcome( TransactionContext, TransactionOutcomeCommitted );

        } else {

            return AvProcessTransactionOutcome( TransactionContext, TransactionOutcomeAborted );
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AvScanAbortCallbackAsync (
    _Unreferenced_parameter_ PFLT_INSTANCE Instance,
    _In_ PFLT_CONTEXT  Context,
    _Unreferenced_parameter_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This routine is the registered cancel callback function in FLT_REGISTRATION.
    It would be invoked by the file system if it decides to abort the scan.
    As its name suggests, this function is asynchrounous, so the caller is not
    blocked.

    Note: This routine may be called before FltCreateSectionForDataScan returns.
    This means the SectionHandle and SectionObject may not yet be set in the
    SectionContext. We can't take a dependency on these being set before needing
    to abort the scan.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

    Context - The section context.

    Data - Pointer to the filter callbackData that is passed to us.

Return Value:

    Returns the final status of this operation.

--*/
{
    PAV_SECTION_CONTEXT sectionCtx = (PAV_SECTION_CONTEXT) Context;
    PAV_SCAN_CONTEXT scanCtx = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Instance );
    UNREFERENCED_PARAMETER( Data );

    if (NULL == sectionCtx) {
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
             ("[AV] AvScanAbortCallbackAsync: INVALID ARGUMENT.\n") );
        return STATUS_INVALID_PARAMETER_2;
    }

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
             ("[AV] AvScanAbortCallbackAsync: closesection handle=%p, object=%p, cancelable=%d\n",
              sectionCtx->SectionHandle,
              sectionCtx->SectionObject,
              sectionCtx->CancelableOnConflictingIo) );

    //
    //  Send abort signal only when the scanning
    //  happens in cancelable context (such as pre-cleanup).
    //

    if (sectionCtx->CancelableOnConflictingIo) {

        //
        //  The only reason of scan context being NULL is that
        //  the section context is about to close anyway.
        //  Please see AvCloseSectionForDataScan(...)
        //
        scanCtx = InterlockedExchangePointer( &sectionCtx->ScanContext, NULL );

        if (scanCtx == NULL) {

            return STATUS_SUCCESS;
        }

        sectionCtx->Aborted = TRUE;
        AvSendAbortToUser( scanCtx->ScanThreadId, scanCtx->ScanId );

    }

    return STATUS_SUCCESS;
}

PFN_IoOpenDriverRegistryKey
AvGetIoOpenDriverRegistryKey (
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
AvOpenServiceParametersKey (
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
    //  open the key if possible
    //

    pIoOpenDriverRegistryKey = AvGetIoOpenDriverRegistryKey();

    if (pIoOpenDriverRegistryKey != NULL) {

        //
        //  Open the parameters key using the API
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
        //  Open specified service root key
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
        //  Open the parameters key relative to service key path
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
    //  Return value to caller
    //

    *ServiceParametersKey = ParametersKey;

OpenServiceParametersKeyCleanup:

    if (ServiceRegKey != NULL) {

        ZwClose( ServiceRegKey );
    }

    return Status;

}

NTSTATUS
AvSetConfiguration (
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
    HANDLE settingsKey = NULL;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG valueLength = sizeof(buffer);
    ULONG resultLength;

    //
    //  Open service parameters key to query values from
    //

    status = AvOpenServiceParametersKey( DriverObject,
                                         RegistryPath,
                                         &settingsKey );

    if (!NT_SUCCESS( status )) {

        goto Cleanup;
    }

#if DBG

    //
    // Query the debug level
    //

    RtlInitUnicodeString( &valueName, L"DebugLevel" );

    status = ZwQueryValueKey( settingsKey,
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
    // Query the local scan timeout
    //

    RtlInitUnicodeString( &valueName, L"LocalScanTimeout" );

    status = ZwQueryValueKey( settingsKey,
                              &valueName,
                              KeyValuePartialInformation,
                              value,
                              valueLength,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        Globals.LocalScanTimeout = (LONGLONG)(*(PULONG)value->Data);
    }

    //
    // Query the network scan timeout
    //

    RtlInitUnicodeString( &valueName, L"NetworkScanTimeout" );

    status = ZwQueryValueKey( settingsKey,
                              &valueName,
                              KeyValuePartialInformation,
                              value,
                              valueLength,
                              &resultLength );

    if (NT_SUCCESS( status )) {

        Globals.NetworkScanTimeout = (LONGLONG)(*(PULONG)value->Data);
    }

    status = STATUS_SUCCESS;

Cleanup:

    if (settingsKey != NULL) {

        ZwClose( settingsKey );
    }

    return status;
}



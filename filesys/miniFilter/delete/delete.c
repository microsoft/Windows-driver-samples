/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    delete.c

Abstract:

    This is the main file for the delete detection sample minifilter.


Environment:

    Kernel mode


--*/


#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")



#define DFDBG_TRACE_ERRORS              0x00000001
#define DFDBG_TRACE_ROUTINES            0x00000002
#define DFDBG_TRACE_OPERATION_STATUS    0x00000004

#define DF_VOLUME_GUID_NAME_SIZE        48

#define DF_INSTANCE_CONTEXT_POOL_TAG    'nIfD'
#define DF_STREAM_CONTEXT_POOL_TAG      'xSfD'
#define DF_TRANSACTION_CONTEXT_POOL_TAG 'xTfD'
#define DF_ERESOURCE_POOL_TAG           'sRfD'
#define DF_DELETE_NOTIFY_POOL_TAG       'nDfD'
#define DF_STRING_POOL_TAG              'rSfD'

#define DF_CONTEXT_POOL_TYPE            PagedPool

#define DF_NOTIFICATION_MASK            (TRANSACTION_NOTIFY_COMMIT_FINALIZE | \
                                         TRANSACTION_NOTIFY_ROLLBACK)


//////////////////////////////////////////////////////////////////////////////
//  Macros                                                                  //
//////////////////////////////////////////////////////////////////////////////

#define DF_PRINT( ... )                                                      \
    DbgPrintEx( DPFLTR_FLTMGR_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__ )

#define DF_DBG_PRINT( _dbgLevel, ... )                                       \
    (FlagOn( gTraceFlags, (_dbgLevel) ) ?                                    \
        DF_PRINT( __VA_ARGS__ ):                                             \
        (0))

#define FlagOnAll( F, T )                                                    \
    (FlagOn( F, T ) == T)


//////////////////////////////////////////////////////////////////////////////
//  Main Globals                                                            //
//////////////////////////////////////////////////////////////////////////////

PFLT_FILTER gFilterHandle;
ULONG gTraceFlags = DFDBG_TRACE_ERRORS;


//////////////////////////////////////////////////////////////////////////////
//  ReFS Compatibility Helpers                                              //
//////////////////////////////////////////////////////////////////////////////

//
//  This helps us deal with ReFS 128-bit file IDs and NTFS 64-bit file IDs.
//

typedef union _DF_FILE_REFERENCE {

    struct {
        ULONGLONG   Value;          //  The 64-bit file ID lives here.
        ULONGLONG   UpperZeroes;    //  In a 64-bit file ID this will be 0.
    } FileId64;

    UCHAR           FileId128[16];  //  The 128-bit file ID lives here.

} DF_FILE_REFERENCE, *PDF_FILE_REFERENCE;

#define DfSizeofFileId(FID) (               \
    ((FID).FileId64.UpperZeroes == 0ll) ?   \
        sizeof((FID).FileId64.Value)    :   \
        sizeof((FID).FileId128)             \
    )


//////////////////////////////////////////////////////////////////////////////
//  Types                                                                   //
//////////////////////////////////////////////////////////////////////////////

//
//  This is the instance context for this minifilter, it stores the volume's
//  GUID name.
//

typedef struct _DF_INSTANCE_CONTEXT {

    //
    //  Volume GUID name.
    //

    UNICODE_STRING VolumeGuidName;

} DF_INSTANCE_CONTEXT, *PDF_INSTANCE_CONTEXT;


//
//  This is the stream context for this minifilter, attached whenever a stream
//  becomes a candidate for deletion.
//

typedef struct _DF_STREAM_CONTEXT {

    //
    //  FLT_FILE_NAME_INFORMATION structure with the names for this stream
    //  and file. This is only used for printing out the opened name when
    //  notifying deletes. This will be the result of an opened query name
    //  done at the last pre-cleanup on the file/stream.
    //
    //  Therefore, there is no requirement of maintaining the file name
    //  information (for the purposes we use it) in sync with the FltMgr name
    //  cache or the file system. This makes it okay to store it in the stream
    //  context.
    //

    PFLT_FILE_NAME_INFORMATION  NameInfo;

    //
    //  File ID, obtained from querying the file system for FileInternalInformation.
    //  If the File ID is 128 bits (as in ReFS) we get it via FileIdInformation.
    //

    DF_FILE_REFERENCE           FileId;

    //
    //  Number of SetDisp operations in flight.
    //

    volatile LONG               NumOps;

    //
    //  IsNotified == 1 means a file/stream deletion was already notified.
    //

    volatile LONG               IsNotified;

    //
    //  Whether or not we've already queried the file ID.
    //

    BOOLEAN                     FileIdSet;

    //
    //  Delete Disposition for this stream.
    //

    BOOLEAN                     SetDisp;

    //
    //  Delete-on-Close state for this stream.
    //

    BOOLEAN                     DeleteOnClose;

} DF_STREAM_CONTEXT, *PDF_STREAM_CONTEXT;


//
//  This is the transaction context for this minifilter, attached at post-
//  -cleanup when notifying a delete within a transaction.
//

typedef struct _DF_TRANSACTION_CONTEXT {

    //
    //  List of DF_DELETE_NOTIFY structures representing pending delete
    //  notifications.
    //

    LIST_ENTRY DeleteNotifyList;

    //
    //  ERESOURCE for synchronized access to the DeleteNotifyList.
    //
    //  ERESOURCEs must be allocated from NonPagedPool. If an ERESOURCE was
    //  declared here as a direct member of a structure, instead of just a
    //  pointer, then the whole transaction context would need to be allocated
    //  out of NonPagedPool.
    //
    //  Therefore, declaring it as a pointer and only allocating at context
    //  initialization time helps us save some NonPagedPool. This is
    //  particularly important in larger context structures.
    //

    PERESOURCE Resource;

} DF_TRANSACTION_CONTEXT, *PDF_TRANSACTION_CONTEXT;


//
//  This structure represents pending delete notifications for files that have
//  been deleted in an open transaction.
//

typedef struct _DF_DELETE_NOTIFY {

    //
    //  Links to other DF_DELETE_NOTIFY structures in the list.
    //

    LIST_ENTRY Links;

    //
    //  Pointer to the stream context for the deleted stream/file.
    //

    PDF_STREAM_CONTEXT StreamContext;

    //
    //  TRUE for a deleted file, FALSE for a stream.
    //

    BOOLEAN FileDelete;

} DF_DELETE_NOTIFY, *PDF_DELETE_NOTIFY;


//////////////////////////////////////////////////////////////////////////////
//  Prototypes                                                              //
//////////////////////////////////////////////////////////////////////////////

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
DfUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
DfInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
DfInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
DfInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
DfInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
DfSetupInstanceContext(
    _In_ PCFLT_RELATED_OBJECTS FltObjects
    );

NTSTATUS
DfAllocateContext (
    _In_ FLT_CONTEXT_TYPE ContextType,
    _Outptr_ PFLT_CONTEXT *Context
    );

NTSTATUS
DfSetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _In_ FLT_CONTEXT_TYPE ContextType,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext
    );

NTSTATUS
DfGetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _In_ FLT_CONTEXT_TYPE ContextType,
    _Outptr_ PFLT_CONTEXT *Context
    );

NTSTATUS
DfGetOrSetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _Outptr_ _Pre_valid_ PFLT_CONTEXT *Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
DfStreamContextCleanupCallback (
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
DfTransactionContextCleanupCallback (
    _In_ PDF_TRANSACTION_CONTEXT TransactionContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
DfInstanceContextCleanupCallback (
    _In_ PDF_INSTANCE_CONTEXT InstanceContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
DfGetFileNameInformation (
    _In_ PFLT_CALLBACK_DATA Data,
    _Inout_ PDF_STREAM_CONTEXT StreamContext
    );

NTSTATUS
DfAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    );

VOID
DfFreeUnicodeString (
    _Inout_ PUNICODE_STRING String
    );

NTSTATUS
DfBuildFileIdString (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _Out_ PUNICODE_STRING String
    );

NTSTATUS
DfDetectDeleteByFileId (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT StreamContext
    );

NTSTATUS
DfIsFileDeleted (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ BOOLEAN IsTransaction
    );

NTSTATUS
DfAddTransDeleteNotify (
    _Inout_ PDF_STREAM_CONTEXT StreamContext,
    _Inout_ PDF_TRANSACTION_CONTEXT TransactionContext,
    _In_ BOOLEAN FileDelete
    );

VOID
DfNotifyDelete (
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ BOOLEAN IsFile,
    _Inout_opt_ PDF_TRANSACTION_CONTEXT TransactionContext
    );

VOID
DfNotifyDeleteOnTransactionEnd (
    _In_ PDF_DELETE_NOTIFY DeleteNotify,
    _In_ BOOLEAN Commit
    );

NTSTATUS
DfProcessDelete (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT    StreamContext
    );

FLT_PREOP_CALLBACK_STATUS
DfPreCreateCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
DfPostCreateCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
DfPreSetInfoCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
DfPostSetInfoCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
DfPreCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
DfPostCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS
DfTransactionNotificationCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_TRANSACTION_CONTEXT TransactionContext,
    _In_ ULONG NotificationMask
    );

NTSTATUS
DfGetVolumeGuidName (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PUNICODE_STRING VolumeGuidName
    );

NTSTATUS
DfGetFileId (
    _In_  PFLT_CALLBACK_DATA Data,
    _Inout_ PDF_STREAM_CONTEXT StreamContext
    );

//////////////////////////////////////////////////////////////////////////////
//  Text section assignments for all routines                               //
//////////////////////////////////////////////////////////////////////////////


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DfUnload)
#pragma alloc_text(PAGE, DfInstanceSetup)
#pragma alloc_text(PAGE, DfInstanceQueryTeardown)
#pragma alloc_text(PAGE, DfInstanceTeardownStart)
#pragma alloc_text(PAGE, DfInstanceTeardownComplete)
#pragma alloc_text(PAGE, DfSetupInstanceContext)
#pragma alloc_text(PAGE, DfAllocateContext)
#pragma alloc_text(PAGE, DfSetContext)
#pragma alloc_text(PAGE, DfGetContext)
#pragma alloc_text(PAGE, DfGetOrSetContext)
#pragma alloc_text(PAGE, DfStreamContextCleanupCallback)
#pragma alloc_text(PAGE, DfTransactionContextCleanupCallback)
#pragma alloc_text(PAGE, DfInstanceContextCleanupCallback)
#pragma alloc_text(PAGE, DfGetFileNameInformation)
#pragma alloc_text(PAGE, DfAllocateUnicodeString)
#pragma alloc_text(PAGE, DfFreeUnicodeString)
#pragma alloc_text(PAGE, DfBuildFileIdString)
#pragma alloc_text(PAGE, DfDetectDeleteByFileId)
#pragma alloc_text(PAGE, DfIsFileDeleted)
#pragma alloc_text(PAGE, DfAddTransDeleteNotify)
#pragma alloc_text(PAGE, DfNotifyDelete)
#pragma alloc_text(PAGE, DfNotifyDeleteOnTransactionEnd)
#pragma alloc_text(PAGE, DfProcessDelete)
#pragma alloc_text(PAGE, DfPreCreateCallback)
#pragma alloc_text(PAGE, DfPostCreateCallback)
#pragma alloc_text(PAGE, DfPreSetInfoCallback)
#pragma alloc_text(PAGE, DfPostSetInfoCallback)
#pragma alloc_text(PAGE, DfPreCleanupCallback)
#pragma alloc_text(PAGE, DfPostCleanupCallback)
#pragma alloc_text(PAGE, DfTransactionNotificationCallback)
#pragma alloc_text(PAGE, DfGetVolumeGuidName)
#pragma alloc_text(PAGE, DfGetFileId)
#endif


//////////////////////////////////////////////////////////////////////////////
//  Context Registration                                                    //
//////////////////////////////////////////////////////////////////////////////

CONST FLT_CONTEXT_REGISTRATION Contexts[] = {

    { FLT_INSTANCE_CONTEXT,
      0,
      DfInstanceContextCleanupCallback,
      sizeof(DF_INSTANCE_CONTEXT),
      DF_INSTANCE_CONTEXT_POOL_TAG,
      NULL,
      NULL,
      NULL },

    { FLT_STREAM_CONTEXT,
      0,
      DfStreamContextCleanupCallback,
      sizeof(DF_STREAM_CONTEXT),
      DF_STREAM_CONTEXT_POOL_TAG,
      NULL,
      NULL,
      NULL },

    { FLT_TRANSACTION_CONTEXT,
      0,
      DfTransactionContextCleanupCallback,
      sizeof(DF_TRANSACTION_CONTEXT),
      DF_TRANSACTION_CONTEXT_POOL_TAG,
      NULL,
      NULL,
      NULL },

    { FLT_CONTEXT_END }

};


//////////////////////////////////////////////////////////////////////////////
//  Operation Registration                                                  //
//////////////////////////////////////////////////////////////////////////////

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      DfPreCreateCallback,
      DfPostCreateCallback },

    { IRP_MJ_SET_INFORMATION,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      DfPreSetInfoCallback,
      DfPostSetInfoCallback },

    { IRP_MJ_CLEANUP,
      0,
      DfPreCleanupCallback,
      DfPostCleanupCallback },

    { IRP_MJ_OPERATION_END }

};


//////////////////////////////////////////////////////////////////////////////
//  Filter Registration                                                     //
//////////////////////////////////////////////////////////////////////////////

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    Contexts,                           //  Context
    Callbacks,                          //  Operation callbacks

    DfUnload,                           //  MiniFilterUnload

    DfInstanceSetup,                    //  InstanceSetup
    DfInstanceQueryTeardown,            //  InstanceQueryTeardown
    DfInstanceTeardownStart,            //  InstanceTeardownStart
    DfInstanceTeardownComplete,         //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  NormalizeNameComponent
    NULL,                               //  NormalizeContextCleanup
    DfTransactionNotificationCallback,  //  TransactionNotification
    NULL                                //  NormalizeNameComponentEx

};


//////////////////////////////////////////////////////////////////////////////
//  MiniFilter initialization and unload routines                           //
//////////////////////////////////////////////////////////////////////////////

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

    Returns STATUS_SUCCESS.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DriverEntry: Entered\n" );

    //
    //  Default to NonPagedPoolNx for non paged pool allocations where supported.
    //

    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}


NTSTATUS
DfUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfUnload: Entered\n" );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
//  Filter Instance Callbacks (Setup/Teardown/QueryTeardown)                //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
DfInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    New instances are only created and attached to a volume if it is a writable
    NTFS or ReFS volume.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

    VolumeFilesystemType - A FLT_FSTYPE_* value indicating which file system type
        the Filter Manager is offering to attach us to.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isWritable = FALSE;

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );

    PAGED_CODE();

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfInstanceSetup: Entered\n" );

    status = FltIsVolumeWritable( FltObjects->Volume,
                                  &isWritable );

    if (!NT_SUCCESS( status )) {

        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Attaching to read-only volumes is pointless as you should not be able
    //  to delete files on such a volume.
    //

    if (isWritable) {

        switch (VolumeFilesystemType) {

            case FLT_FSTYPE_NTFS:
            case FLT_FSTYPE_REFS:

                status = STATUS_SUCCESS;
                break;

            default:

                return STATUS_FLT_DO_NOT_ATTACH;
        }

    } else {

        return STATUS_FLT_DO_NOT_ATTACH;
    }

    return status;
}


NTSTATUS
DfInstanceQueryTeardown (
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

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfInstanceQueryTeardown: Entered\n" );

    return STATUS_SUCCESS;
}


VOID
DfInstanceTeardownStart (
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

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfInstanceTeardownStart: Entered\n" );
}


VOID
DfInstanceTeardownComplete (
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

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfInstanceTeardownComplete: Entered\n" );
}


//////////////////////////////////////////////////////////////////////////////
//  Context manipulation functions                                          //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
DfAllocateContext (
    _In_ FLT_CONTEXT_TYPE ContextType,
    _Outptr_ PFLT_CONTEXT *Context
    )
/*++

Routine Description:

    This routine allocates and initializes a context of given type.

Arguments:

    ContextType   - Type of context to be allocated/initialized.

    Context       - Pointer to a context pointer.

Return Value:

    Returns a status forwarded from FltAllocateContext.

--*/
{
    NTSTATUS status;
    PDF_TRANSACTION_CONTEXT transactionContext;

    PAGED_CODE();

    switch (ContextType) {

        case FLT_STREAM_CONTEXT:

            status = FltAllocateContext( gFilterHandle,
                                         FLT_STREAM_CONTEXT,
                                         sizeof(DF_STREAM_CONTEXT),
                                         DF_CONTEXT_POOL_TYPE,
                                         Context );

            if (NT_SUCCESS( status )) {
                RtlZeroMemory( *Context, sizeof(DF_STREAM_CONTEXT) );
            }

            return status;

        case FLT_TRANSACTION_CONTEXT:

            status = FltAllocateContext( gFilterHandle,
                                         FLT_TRANSACTION_CONTEXT,
                                         sizeof(DF_TRANSACTION_CONTEXT),
                                         DF_CONTEXT_POOL_TYPE,
                                         Context );

            if (NT_SUCCESS( status )) {
                RtlZeroMemory( *Context, sizeof(DF_TRANSACTION_CONTEXT) );

                transactionContext = *Context;

                InitializeListHead( &transactionContext->DeleteNotifyList );

                transactionContext->Resource = ExAllocatePoolZero( NonPagedPool,
                                                                   sizeof(ERESOURCE),
                                                                   DF_ERESOURCE_POOL_TAG );

                if (NULL == transactionContext->Resource) {
                    FltReleaseContext( transactionContext );
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                ExInitializeResourceLite( transactionContext->Resource );
            }

            return status;

        case FLT_INSTANCE_CONTEXT:

            status = FltAllocateContext( gFilterHandle,
                                         FLT_INSTANCE_CONTEXT,
                                         sizeof(DF_INSTANCE_CONTEXT),
                                         DF_CONTEXT_POOL_TYPE,
                                         Context );

            if (NT_SUCCESS( status )) {
                RtlZeroMemory( *Context, sizeof(DF_INSTANCE_CONTEXT) );
            }

            return status;

        default:

            return STATUS_INVALID_PARAMETER;
    }
}


NTSTATUS
DfSetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _In_ FLT_CONTEXT_TYPE ContextType,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext
    )
/*++

Routine Description:

    This routine sets the given context to the target.

Arguments:

    FltObjects    - Pointer to the FLT_RELATED_OBJECTS data structure containing
                    opaque handles to this filter, instance and its associated volume.

    Target        - Pointer to the target to which we want to attach the
                    context. It will actually be either a FILE_OBJECT or
                    a KTRANSACTION. For instance contexts, it's ignored, as
                    the target is the FLT_INSTANCE itself, obtained from
                    Data->Iopb->TargetInstance.

    ContextType   - Type of context to get/allocate/attach. Also used to
                    disambiguate the target/context type as this minifilter
                    only has one type of context per target.

    NewContext    - Pointer to the context the caller wants to attach.

    OldContext    - Returns the context already attached to the target, if
                    that is the case.

Return Value:

    Returns a status forwarded from FltSetXxxContext.

--*/
{
    PAGED_CODE();

    switch (ContextType) {

        case FLT_STREAM_CONTEXT:

            return FltSetStreamContext( FltObjects->Instance,
                                        (PFILE_OBJECT)Target,
                                        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                        NewContext,
                                        OldContext );

        case FLT_TRANSACTION_CONTEXT:

            return FltSetTransactionContext( FltObjects->Instance,
                                             (PKTRANSACTION)Target,
                                             FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                             NewContext,
                                             OldContext );

        case FLT_INSTANCE_CONTEXT:

            return FltSetInstanceContext( FltObjects->Instance,
                                          FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                          NewContext,
                                          OldContext );

        default:

            ASSERT( !"Unexpected context type!\n" );

            return STATUS_INVALID_PARAMETER;
    }
}


NTSTATUS
DfGetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _In_ FLT_CONTEXT_TYPE ContextType,
    _Outptr_ PFLT_CONTEXT *Context
    )
/*++

Routine Description:

    This routine gets the given context from the target.

Arguments:

    FltObjects    - Pointer to the FLT_RELATED_OBJECTS data structure containing
                    opaque handles to this filter, instance and its associated volume.

    Target        - Pointer to the target from which we want to obtain the
                    context. It will actually be either a FILE_OBJECT or
                    a KTRANSACTION. For instance contexts, it's ignored, as
                    the target is the FLT_INSTANCE itself, obtained from
                    Data->Iopb->TargetInstance.

    ContextType   - Type of context to get. Also used to disambiguate
                    the target/context type as this minifilter
                    only has one type of context per target.

    Context       - Pointer returning a pointer to the attached context.

Return Value:

    Returns a status forwarded from FltSetXxxContext.

--*/
{
    PAGED_CODE();

    switch (ContextType) {

        case FLT_STREAM_CONTEXT:

            return FltGetStreamContext( FltObjects->Instance,
                                        (PFILE_OBJECT)Target,
                                        Context );

        case FLT_TRANSACTION_CONTEXT:

            return FltGetTransactionContext( FltObjects->Instance,
                                             (PKTRANSACTION)Target,
                                             Context );

        case FLT_INSTANCE_CONTEXT:

            return FltGetInstanceContext( FltObjects->Instance,
                                          Context );

        default:

            return STATUS_INVALID_PARAMETER;
    }
}


NTSTATUS
DfGetOrSetContext (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(ContextType==FLT_INSTANCE_CONTEXT, _In_opt_) _When_(ContextType!=FLT_INSTANCE_CONTEXT, _In_) PVOID Target,
    _Outptr_ _Pre_valid_ PFLT_CONTEXT *Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine obtains a context of type ContextType that is attached to
    Target.

    If a context is already attached to Target, it will be returned in
    *Context. If a context is already attached, but *Context points to
    another context, *Context will be released.

    If no context is attached, and *Context points to a previously allocated
    context, *Context will be attached to the Target.

    Finally, if no previously allocated context is passed to this routine
    (*Context is a NULL pointer), a new Context is created and then attached
    to Target.

    In case of race conditions (or the presence of a previously allocated
    context at *Context), the existing attached context is returned via
    *Context.

    In case of a transaction context, this function will also enlist in the
    transaction.

Arguments:

    FltObjects    - Pointer to the FLT_RELATED_OBJECTS data structure containing
                    opaque handles to this filter, instance and its associated volume.

    Target        - Pointer to the target to which we want to attach the
                    context. It will actually be either a FILE_OBJECT or
                    a KTRANSACTION.  It is NULL for an Instance context.

    Context       - Pointer to a pointer to a context. Used both for
                    returning an allocated/attached context or for receiving
                    a context to attach to the Target.

    ContextType   - Type of context to get/allocate/attach. Also used to
                    disambiguate the target/context type as this minifilter
                    only has one type of context per target.

Return Value:

    Returns a status forwarded from Flt(((Get|Set)Xxx)|Allocate)Context or
    FltEnlistInTransaction.

--*/
{
    NTSTATUS status;
    PFLT_CONTEXT newContext;
    PFLT_CONTEXT oldContext;

    PAGED_CODE();

    ASSERT( NULL != Context );

    newContext = *Context;

    //
    //  Is there already a context attached to the target?
    //

    status = DfGetContext( FltObjects,
                           Target,
                           ContextType,
                           &oldContext );

    if (STATUS_NOT_FOUND == status) {

    //
    //  There is no attached context. This means we have to either attach the
    //  one provided by the caller or allocate a new one and attach it.
    //

        if (NULL == newContext) {

            //
            //  No provided context. Allocate one.
            //

            status = DfAllocateContext( ContextType, &newContext );

            if (!NT_SUCCESS( status )) {

                //
                //  We failed to allocate.
                //

                return status;
            }
        }

    } else if (!NT_SUCCESS( status )) {

        //
        //  We failed trying to get a context from the target.
        //

        return status;

    } else {

        //
        //  There is already a context attached to the target, so return
        //  that context.
        //
        //  If a context was provided by the caller, release it if it's not
        //  the one attached to the target.
        //

        //
        //  The caller is not allowed to set the same context on the target
        //  twice.
        //
        ASSERT( newContext != oldContext );

        if (NULL != newContext) {

            FltReleaseContext( newContext );
        }

        *Context = oldContext;
        return status;
    }

    //
    //  At this point we should have a context to set on the target (newContext).
    //

    status = DfSetContext( FltObjects,
                           Target,
                           ContextType,
                           newContext,
                           &oldContext );

    if (!NT_SUCCESS( status )) {

        //
        //  FltSetStreamContext failed so we must release the new context.
        //

        FltReleaseContext( newContext );

        if (STATUS_FLT_CONTEXT_ALREADY_DEFINED == status) {

            //
            //  We're racing with some other call which managed to set the
            //  context before us. We will return that context instead, which
            //  will be in oldContext.
            //

            *Context = oldContext;
            return STATUS_SUCCESS;

        } else {

            //
            //  Failed to set the context. Return NULL.
            //

            *Context = NULL;
            return status;
        }
    }

    //
    //  If this is setting a transaction context, we want to enlist in the
    //  transaction as well.
    //

    if (FLT_TRANSACTION_CONTEXT == ContextType) {

        status = FltEnlistInTransaction( FltObjects->Instance,
                                         (PKTRANSACTION)Target,
                                         newContext,
                                         DF_NOTIFICATION_MASK );

    }

    //
    //  Setting the context was successful so just return newContext.
    //

    *Context = newContext;
    return status;
}


//////////////////////////////////////////////////////////////////////////////
//  Context Cleanup Callbacks                                               //
//////////////////////////////////////////////////////////////////////////////

VOID
DfStreamContextCleanupCallback (
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine cleans up a stream context. The only cleanup necessary is
    releasing the FLT_FILE_NAME_INFORMATION object of the NameInfo field.

Arguments:

    StreamContext - Pointer to DF_STREAM_CONTEXT to be cleaned up.

    ContextType   - Type of StreamContext. Must be FLT_STREAM_CONTEXT.

--*/
{
    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    ASSERT( ContextType == FLT_STREAM_CONTEXT );

    //
    //  Release NameInfo if present.
    //

    if (StreamContext->NameInfo != NULL) {

        FltReleaseFileNameInformation(StreamContext->NameInfo);
        StreamContext->NameInfo = NULL;
    }
}


VOID
DfTransactionContextCleanupCallback (
    _In_ PDF_TRANSACTION_CONTEXT TransactionContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine cleans up a transaction context.
    This operation consists basically of walking the DeleteNotifyList and
    deleting all the deletion notifications pending on behalf of this
    transaction.

Arguments:

    TransactionContext - Pointer to DF_TRANSACTION_CONTEXT to be cleaned up.

    ContextType - Type of TransactionContext. Must be FLT_TRANSACTION_CONTEXT.

--*/
{
    PDF_DELETE_NOTIFY deleteNotify = NULL;

    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    ASSERT( ContextType == FLT_TRANSACTION_CONTEXT );

    if (NULL != TransactionContext->Resource) {

        FltAcquireResourceExclusive( TransactionContext->Resource );

        while (!IsListEmpty( &TransactionContext->DeleteNotifyList )) {

            //
            //  Remove every DF_DELETE_NOTIFY, releasing their corresponding
            //  FLT_FILE_NAME_INFORMATION objects and freeing pool used by
            //  them.
            //

            deleteNotify = CONTAINING_RECORD( RemoveHeadList( &TransactionContext->DeleteNotifyList ),
                                              DF_DELETE_NOTIFY,
                                              Links );

            FltReleaseContext( deleteNotify->StreamContext );
            ExFreePool( deleteNotify );

        }

        FltReleaseResource( TransactionContext->Resource );

        //
        //  Delete and free the DeleteNotifyList synchronization resource.
        //

        ExDeleteResourceLite( TransactionContext->Resource );
        ExFreePool( TransactionContext->Resource );
    }
}


VOID
DfInstanceContextCleanupCallback (
    _In_ PDF_INSTANCE_CONTEXT InstanceContext,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine cleans up an instance context, which consists on freeing
    pool used by the volume GUID name string.

Arguments:

    InstanceContext - Pointer to DF_INSTANCE_CONTEXT to be cleaned up.

    ContextType - Type of InstanceContext. Must be FLT_INSTANCE_CONTEXT.

--*/
{
    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    ASSERT( ContextType == FLT_INSTANCE_CONTEXT );

    DfFreeUnicodeString( &InstanceContext->VolumeGuidName );
}


//////////////////////////////////////////////////////////////////////////////
//  Miscellaneous String, File Name and File ID Functions                   //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
DfGetFileNameInformation (
    _In_  PFLT_CALLBACK_DATA Data,
    _Inout_ PDF_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description:

    This routine gets and parses the file name information, obtains the File
    ID and saves them in the stream context.

Arguments:

    Data  - Pointer to FLT_CALLBACK_DATA.

    StreamContext - Pointer to stream context that will receive the file
                    information.

Return Value:

    Returns statuses forwarded from Flt(Get|Parse)FileNameInformation or
    FltQueryInformationFile.

--*/
{
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION oldNameInfo;
    PFLT_FILE_NAME_INFORMATION newNameInfo;

    PAGED_CODE();

    //
    //  FltGetFileNameInformation - this is enough for a file name.
    //

    status = FltGetFileNameInformation( Data,
                                        (FLT_FILE_NAME_OPENED |
                                         FLT_FILE_NAME_QUERY_DEFAULT),
                                        &newNameInfo );

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    //  FltParseFileNameInformation - this fills in the other gaps, like the
    //  stream name, if present.
    //

    status = FltParseFileNameInformation( newNameInfo );

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    //  Now that we have a good NameInfo, set it in the context, replacing
    //  the previous one.
    //

    oldNameInfo = InterlockedExchangePointer( &StreamContext->NameInfo,
                                              newNameInfo );

    if (NULL != oldNameInfo) {

        FltReleaseFileNameInformation( oldNameInfo );
    }

    return status;
}


NTSTATUS
DfGetFileId (
    _In_  PFLT_CALLBACK_DATA Data,
    _Inout_ PDF_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description:

    This routine obtains the File ID and saves it in the stream context.

Arguments:

    Data  - Pointer to FLT_CALLBACK_DATA.

    StreamContext - Pointer to stream context that will receive the file
                    ID.

Return Value:

    Returns statuses forwarded from FltQueryInformationFile, including
    STATUS_FILE_DELETED.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_INTERNAL_INFORMATION fileInternalInformation;

    PAGED_CODE();

    //
    //  Only query the file system for the file ID for the first time.
    //  This is just an optimization.  It doesn't need any real synchronization
    //  because file IDs don't change.
    //

    if (!StreamContext->FileIdSet) {

        //
        //  Querying for FileInternalInformation gives you the file ID.
        //

        status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                          Data->Iopb->TargetFileObject,
                                          &fileInternalInformation,
                                          sizeof(FILE_INTERNAL_INFORMATION),
                                          FileInternalInformation,
                                          NULL );

        if (NT_SUCCESS( status )) {

            //
            //  ReFS uses 128-bit file IDs.  FileInternalInformation supports 64-
            //  bit file IDs.  ReFS signals that a particular file ID can only
            //  be represented in 128 bits by returning FILE_INVALID_FILE_ID as
            //  the file ID.  In that case we need to use FileIdInformation.
            //

            if (fileInternalInformation.IndexNumber.QuadPart == FILE_INVALID_FILE_ID) {

                FILE_ID_INFORMATION fileIdInformation;

                status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                                  Data->Iopb->TargetFileObject,
                                                  &fileIdInformation,
                                                  sizeof(FILE_ID_INFORMATION),
                                                  FileIdInformation,
                                                  NULL );

                if (NT_SUCCESS( status )) {

                    //
                    //  We don't use DfSizeofFileId() here because we are not
                    //  measuring the size of a DF_FILE_REFERENCE.  We know we have
                    //  a 128-bit value.
                    //

                    RtlCopyMemory( &StreamContext->FileId,
                                   &fileIdInformation.FileId,
                                   sizeof(StreamContext->FileId) );

                    //
                    //  Because there's (currently) no support for 128-bit values in
                    //  the compiler we need to ensure the setting of the ID and our
                    //  remembering that the file ID was set occur in the right order.
                    //

                    KeMemoryBarrier();

                    StreamContext->FileIdSet = TRUE;
                }

            } else {

                StreamContext->FileId.FileId64.Value = fileInternalInformation.IndexNumber.QuadPart;
                StreamContext->FileId.FileId64.UpperZeroes = 0ll;

                //
                //  Because there's (currently) no support for 128-bit values in
                //  the compiler we need to ensure the setting of the ID and our
                //  remembering that the file ID was set occur in the right order.
                //

                KeMemoryBarrier();

                StreamContext->FileIdSet = TRUE;
            }
        }
    }

    return status;
}


NTSTATUS
DfAllocateUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This helper routine simply allocates a buffer for a UNICODE_STRING and
    initializes its Length to zero.

    It uses whatever value is present in the MaximumLength field as the size
    for the allocation.

Arguments:

    String - Pointer to UNICODE_STRING.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if it was not possible to allocate the
    buffer from pool.

    STATUS_SUCCESS otherwise.

--*/
{
    PAGED_CODE();

    ASSERT( NULL != String );
    ASSERT( 0 != String->MaximumLength );

    String->Length = 0;

    String->Buffer = ExAllocatePoolZero( DF_CONTEXT_POOL_TYPE,
                                         String->MaximumLength,
                                         DF_STRING_POOL_TAG );

    if (NULL == String->Buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


VOID
DfFreeUnicodeString (
    _Inout_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This helper routine frees the buffer of a UNICODE_STRING and resets its
    Length to zero.

Arguments:

    String - Pointer to UNICODE_STRING.

--*/
{
    PAGED_CODE();

    ASSERT( NULL != String );
    ASSERT( 0 != String->MaximumLength );

    String->Length = 0;

    if ( NULL != String->Buffer ) {

        String->MaximumLength = 0;
        ExFreePool( String->Buffer );
        String->Buffer = NULL;
    }
}


NTSTATUS
DfGetVolumeGuidName (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PUNICODE_STRING VolumeGuidName
    )
/*++

Routine Description:

    This helper routine returns a volume GUID name (with an added trailing
    backslash for convenience) in the VolumeGuidName string passed by the
    caller.

    The volume GUID name is cached in the instance context for the instance
    attached to the volume, and this function will set up an instance context
    with the cached name on it if there isn't one already attached to the
    instance.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    VolumeGuidName - Pointer to UNICODE_STRING, returning the volume GUID name.

Return Value:

    Return statuses forwarded by DfAllocateUnicodeString or
    FltGetVolumeGuidName. On error, caller needs to DfFreeUnicodeString on
    VolumeGuidName.

--*/
{
    NTSTATUS status;
    PUNICODE_STRING sourceGuidName;
    PDF_INSTANCE_CONTEXT instanceContext = NULL;

    PAGED_CODE();

    //
    //  Obtain an instance context. Target is NULL for instance context, as
    //  the FLT_INSTANCE can be obtained from the FltObjects.
    //

    status = DfGetOrSetContext( FltObjects,
                                NULL,
                                &instanceContext,
                                FLT_INSTANCE_CONTEXT );

    if (NT_SUCCESS( status )) {

        //
        //  sourceGuidName is the source from where we'll copy the volume
        //  GUID name. Hopefully the name is present in the instance context
        //  already (buffer is not NULL) so we'll try to use that.
        //

        sourceGuidName = &instanceContext->VolumeGuidName;

        if (NULL == sourceGuidName->Buffer) {

            //
            //  The volume GUID name is not cached in the instance context
            //  yet, so we will have to query the volume for it and put it
            //  in the instance context, so future queries can get it directly
            //  from the context.
            //

            UNICODE_STRING tempString;

            //
            //  Add sizeof(WCHAR) so it's possible to add a trailing backslash here.
            //

            tempString.MaximumLength = DF_VOLUME_GUID_NAME_SIZE *
                                       sizeof(WCHAR) +
                                       sizeof(WCHAR);

            status = DfAllocateUnicodeString( &tempString );

            if (!NT_SUCCESS( status )) {

                DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                              "delete!%s: DfAllocateUnicodeString returned 0x%08x!\n",
                              __FUNCTION__,
                              status );

                return status;
            }

            //  while there is no guid name, don't do the open by id deletion logic.
            //  (it's actually better to defer obtaining the volume GUID name up to
            //   the point when we actually need it, in the open by ID scenario.)
            status = FltGetVolumeGuidName( FltObjects->Volume,
                                           &tempString,
                                           NULL );

            if (!NT_SUCCESS( status )) {

                DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                              "delete!%s: FltGetVolumeGuidName returned 0x%08x!\n",
                              __FUNCTION__,
                              status );

                DfFreeUnicodeString( &tempString );

                return status;
            }

            //
            //  Append trailing backslash.
            //

            RtlAppendUnicodeToString( &tempString, L"\\" );

            //
            //  Now set the sourceGuidName to the tempString. It is okay to
            //  set Length and MaximumLength with no synchronization because
            //  those will always be the same value (size of a volume GUID
            //  name with an extra trailing backslash).
            //

            sourceGuidName->Length = tempString.Length;
            sourceGuidName->MaximumLength = tempString.MaximumLength;

            //
            //  Setting the buffer, however, requires some synchronization,
            //  because another thread might be attempting to do the same,
            //  and even though they're exactly the same string, they're
            //  different allocations (buffers) so if the other thread we're
            //  racing with manages to set the buffer before us, we need to
            //  free our temporary string buffer.
            //

            InterlockedCompareExchangePointer( &sourceGuidName->Buffer,
                                               tempString.Buffer,
                                               NULL );

            if (sourceGuidName->Buffer != tempString.Buffer) {

                //
                //  We didn't manage to set the buffer, so let's free the
                //  tempString buffer.
                //

                DfFreeUnicodeString( &tempString );
            }
        }

        //
        //  sourceGuidName now contains the correct GUID name, so copy that
        //  to the caller string.
        //

        RtlCopyUnicodeString( VolumeGuidName, sourceGuidName );

        //
        //  We're done with the instance context.
        //

        FltReleaseContext( instanceContext );
    }

    return status;
}


NTSTATUS
DfBuildFileIdString (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _Out_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This helper routine builds a string used to open a file by its ID.

    It will assume the file ID is properly loaded in the stream context
    (StreamContext->FileId).

Arguments:

    Data - Pointer to FLT_CALLBACK_DATA.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context.

    String - Pointer to UNICODE_STRING (output).

Return Value:

    Return statuses forwarded by DfAllocateUnicodeString or
    FltGetInstanceContext.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    ASSERT( NULL != String );

    //
    //  We'll compose the string with:
    //  1. The volume GUID name.
    //  2. A backslash
    //  3. The File ID.
    //

    //
    //  Make sure the file ID is loaded in the StreamContext.  Note that if the
    //  file has been deleted DfGetFileId will return STATUS_FILE_DELETED.
    //  Since we're interested in detecting whether the file has been deleted
    //  that's fine; the open-by-ID will not actually take place.  We have to
    //  ensure it is loaded before building the string length below since we
    //  may get either a 64-bit or 128-bit file ID back.
    //

    status = DfGetFileId( Data,
                          StreamContext );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  First add the lengths of 1, 2, 3 and allocate accordingly.
    //  Note that ReFS understands both 64- and 128-bit file IDs when opening
    //  by ID, so whichever size we get back from DfSizeofFileId will work.
    //

    String->MaximumLength = DF_VOLUME_GUID_NAME_SIZE * sizeof(WCHAR) +
                            sizeof(WCHAR) +
                            DfSizeofFileId( StreamContext->FileId );

    status = DfAllocateUnicodeString( String );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Now obtain the volume GUID name with a trailing backslash (1 + 2).
    //

    // obtain volume GUID name here and cache it in the InstanceContext.
    status = DfGetVolumeGuidName( FltObjects,
                                  String );

    if (!NT_SUCCESS( status )) {

        DfFreeUnicodeString( String );

        return status;
    }

    //
    //  Now append the file ID to the end of the string.
    //

    RtlCopyMemory( Add2Ptr( String->Buffer, String->Length ),
                   &StreamContext->FileId,
                   DfSizeofFileId( StreamContext->FileId ));

    String->Length += DfSizeofFileId( StreamContext->FileId );

    ASSERT( String->Length == String->MaximumLength );

    return status;
}


NTSTATUS
DfDetectDeleteByFileId (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description:

    This helper routine detects a deleted file by attempting to open it using
    its file ID.

    If the file is successfully opened this routine closes the file before returning.

Arguments:

    Data - Pointer to FLT_CALLBACK_DATA.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context.

Return Value:

    STATUS_FILE_DELETED - Returned through DfBuildFileIdString if the file has
                          been deleted.

    STATUS_INVALID_PARAMETER - Returned from FltCreateFileEx2 when opening by ID
                               a file that doesn't exist.

    STATUS_DELETE_PENDING - The file has been set to be deleted when the last handle
                            goes away, but there are still open handles.

    Also any other NTSTATUS returned from DfBuildFileIdString, FltCreateFileEx2,
    or FltClose.

--*/
{
    NTSTATUS status;
    UNICODE_STRING fileIdString;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    IO_DRIVER_CREATE_CONTEXT driverCreateContext;

    PAGED_CODE();

    //
    //  First build the file ID string.  Note that this may fail with STATUS_FILE_DELETED
    //  and short-circuit our open-by-ID.  Since we're really trying to see if
    //  the file is deleted, that's perfectly okay.
    //

    status = DfBuildFileIdString( Data,
                                  FltObjects,
                                  StreamContext,
                                  &fileIdString );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &fileIdString,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    //
    //  It is important to initialize the IO_DRIVER_CREATE_CONTEXT structure's
    //  TxnParameters. We'll always want to do this open on behalf of a
    //  transaction because opening the file by ID is the method we use to
    //  detect if the whole file still exists when we're in a transaction.
    //

    IoInitializeDriverCreateContext( &driverCreateContext );
    driverCreateContext.TxnParameters =
        IoGetTransactionParameterBlock( Data->Iopb->TargetFileObject );

    status = FltCreateFileEx2( gFilterHandle,
                               Data->Iopb->TargetInstance,
                               &handle,
                               NULL,
                               FILE_READ_ATTRIBUTES,
                               &objectAttributes,
                               &ioStatus,
                               (PLARGE_INTEGER) NULL,
                               0L,
                               FILE_SHARE_VALID_FLAGS,
                               FILE_OPEN,
                               FILE_OPEN_REPARSE_POINT | FILE_OPEN_BY_FILE_ID,
                               (PVOID) NULL,
                               0L,
                               IO_IGNORE_SHARE_ACCESS_CHECK,
                               &driverCreateContext );

    if (NT_SUCCESS( status )) {

        status = FltClose( handle );
        ASSERT( NT_SUCCESS( status ) );
    }

    DfFreeUnicodeString( &fileIdString );

    return status;
}


//////////////////////////////////////////////////////////////////////////////
//  Deletion Verification & Processing Functions                            //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
DfIsFileDeleted (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ BOOLEAN IsTransaction
    )
/*++

Routine Description:

    This routine returns whether a file was deleted. It is called from
    DfProcessDelete after an alternate data stream is deleted. This needs to
    be done for the case when the last outstanding handle to a delete-pending
    file is a handle to a delete-pending alternate data stream. When that
    handle is closed, the whole file goes away, and we want to report a whole
    file deletion, not just an alternate data stream deletion.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context.

    IsTransaction - TRUE if in a transaction, FALSE otherwise.

Return Value:

    STATUS_FILE_DELETED - The whole file was deleted.
    Successful status - The file still exists, this was probably just a named
                        data stream being deleted.
    Anything else - Failure in finding out if the file was deleted.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_OBJECTID_BUFFER fileObjectIdBuf;

    FLT_FILESYSTEM_TYPE fileSystemType;

    PAGED_CODE();

    //
    //  We need to know whether we're on ReFS or NTFS.
    //

    status = FltGetFileSystemType( FltObjects->Instance,
                                   &fileSystemType );

    if (status != STATUS_SUCCESS) {

        return status;
    }

    //
    //  FSCTL_GET_OBJECT_ID does not return STATUS_FILE_DELETED if the
    //  file was deleted in a transaction, and this is why we need another
    //  method for detecting if the file is still present: opening by ID.
    //
    //  If we're on ReFS we also need to open by file ID because ReFS does not
    //  support object IDs.
    //

    if (IsTransaction ||
        (fileSystemType == FLT_FSTYPE_REFS)) {

        status = DfDetectDeleteByFileId( Data,
                                         FltObjects,
                                         StreamContext );

        switch (status) {

            case STATUS_INVALID_PARAMETER:

                //
                //  The file was deleted. In this case, trying to open it
                //  by ID returns STATUS_INVALID_PARAMETER.
                //

                return STATUS_FILE_DELETED;

            case STATUS_DELETE_PENDING:

                //
                //  In this case, the main file still exists, but is in
                //  a delete pending state, so we return STATUS_SUCCESS,
                //  signaling it still exists and wasn't deleted by this
                //  operation.
                //

                return STATUS_SUCCESS;

            default:

                return status;
        }

    } else {

        //
        //  When not in a transaction, attempting to get the object ID of the
        //  file is a cheaper alternative compared to opening the file by ID.
        //

        status = FltFsControlFile( Data->Iopb->TargetInstance,
                                   Data->Iopb->TargetFileObject,
                                   FSCTL_GET_OBJECT_ID,
                                   NULL,
                                   0,
                                   &fileObjectIdBuf,
                                   sizeof(FILE_OBJECTID_BUFFER),
                                   NULL );

        switch (status) {

            case STATUS_OBJECTID_NOT_FOUND:

                //
                //  Getting back STATUS_OBJECTID_NOT_FOUND means the file
                //  still exists, it just doesn't have an object ID.

                return STATUS_SUCCESS;

            default:

                //
                //  Else we just get back STATUS_FILE_DELETED if the file
                //  doesn't exist anymore, or some error status, so no
                //  status conversion is necessary.
                //

                NOTHING;
        }
    }

    return status;
}


NTSTATUS
DfAddTransDeleteNotify (
    _Inout_ PDF_STREAM_CONTEXT StreamContext,
    _Inout_ PDF_TRANSACTION_CONTEXT TransactionContext,
    _In_ BOOLEAN FileDelete
    )
/*++

Routine Description:

    This routine adds a pending deletion notification (DF_DELETE_NOTIFY)
    object to the transaction context DeleteNotifyList. It is called from
    DfNotifyDelete when a file or stream gets deleted in a transaction.

Arguments:

    StreamContext - Pointer to the stream context.

    TransactionContext - Pointer to the transaction context.

    FileDelete - TRUE if this is a FILE deletion, FALSE if it's a STREAM
                 deletion.

Return Value:

    STATUS_SUCCESS.

--*/
{
    PDF_DELETE_NOTIFY deleteNotify;

    PAGED_CODE();

    ASSERT( NULL != TransactionContext->Resource );

    ASSERT( NULL != StreamContext );

    deleteNotify = ExAllocatePoolZero( DF_CONTEXT_POOL_TYPE,
                                       sizeof(DF_DELETE_NOTIFY),
                                       DF_DELETE_NOTIFY_POOL_TAG );

    if (NULL == deleteNotify) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FltReferenceContext( StreamContext );
    deleteNotify->StreamContext = StreamContext;
    deleteNotify->FileDelete = FileDelete;

    FltAcquireResourceExclusive( TransactionContext->Resource );

    InsertTailList( &TransactionContext->DeleteNotifyList,
                    &deleteNotify->Links );

    FltReleaseResource( TransactionContext->Resource );

    return STATUS_SUCCESS;
}


VOID
DfNotifyDelete (
    _In_ PDF_STREAM_CONTEXT StreamContext,
    _In_ BOOLEAN IsFile,
    _Inout_opt_ PDF_TRANSACTION_CONTEXT TransactionContext
    )
/*++

Routine Description:

    This routine does the processing after it is verified, in the post-cleanup
    callback, that a file or stream were deleted. It sorts out whether it's a
    file or a stream delete, whether this is in a transacted context or not,
    and issues the appropriate notifications.

Arguments:

    StreamContext - Pointer to the stream context of the deleted file/stream.

    IsFile - TRUE if deleting a file, FALSE for an alternate data stream.

    TransactionContext - The transaction context. Present if in a transaction,
                         NULL otherwise.

--*/
{
    PAGED_CODE();

    if (InterlockedIncrement( &StreamContext->IsNotified ) <= 1) {

        if (IsFile) {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          "delete!DfPostCleanupCallback: "
                          "A file \"%wZ\" (%p) has been",
                          &StreamContext->NameInfo->Name,
                          StreamContext );

        } else {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          "delete!DfPostCleanupCallback: "
                          "An alternate data stream \"%wZ\" (%p) has been",
                          &StreamContext->NameInfo->Name,
                          StreamContext );
        }

        //
        //  Flag that a delete has been notified on this file/stream.
        //

        if (NULL == TransactionContext) {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          " deleted!\n" );

        } else {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          " deleted in a transaction!\n" );

            DfAddTransDeleteNotify( StreamContext,
                                    TransactionContext,
                                    IsFile );
        }
    }
}


VOID
DfNotifyDeleteOnTransactionEnd (
    _In_ PDF_DELETE_NOTIFY DeleteNotify,
    _In_ BOOLEAN Commit
    )
/*++

Routine Description:

    This routine is called by the transaction notification callback to issue
    the proper notifications for a file that has been deleted in the context
    of that transaction.
    The file will be reported as finally deleted, if the transaction was
    committed, or "saved" if the transaction was rolled back.

Arguments:

    DeleteNotify - Pointer to the DF_DELETE_NOTIFY object that contains the
                   data necessary for issuing this notification.

    Commit       - TRUE if the transaction was committed, FALSE if it was
                   rolled back.

--*/
{
    PAGED_CODE();

    if (DeleteNotify->FileDelete) {

        DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                      "delete!DfTransactionNotificationCallback: "
                      "A file \"%wZ\" (%p) has been",
                      &DeleteNotify->StreamContext->NameInfo->Name,
                      DeleteNotify->StreamContext );

    } else {

        DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                      "delete!DfTransactionNotificationCallback: "
                      "An alternate data stream \"%wZ\" (%p) has been",
                      &DeleteNotify->StreamContext->NameInfo->Name,
                      DeleteNotify->StreamContext );
    }

    if (Commit) {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          " deleted due to a transaction commit!\n" );

    } else {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          " saved due to a transaction rollback!\n" );
    }
}


NTSTATUS
DfProcessDelete (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_STREAM_CONTEXT    StreamContext
    )
/*++

Routine Description:

    This routine does the processing after it is verified, in the post-cleanup
    callback, that a file or stream were deleted. It sorts out whether it's a
    file or a stream delete, whether this is in a transacted context or not,
    and issues the appropriate notifications.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the stream context of the deleted file/stream.

Return Value:

    STATUS_SUCCESS.

--*/
{
    BOOLEAN isTransaction;
    BOOLEAN isFileDeleted = FALSE;
    NTSTATUS status;
    PDF_TRANSACTION_CONTEXT transactionContext = NULL;

	PAGED_CODE();

    //  Is this in a transacted context?
    isTransaction = (NULL != FltObjects->Transaction);

    if (isTransaction) {
        DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                      "delete!DfProcessDelete: In a transaction!\n" );

        status = DfGetOrSetContext( FltObjects,
                                    FltObjects->Transaction,
                                    &transactionContext,
                                    FLT_TRANSACTION_CONTEXT );

        if (!NT_SUCCESS( status )) {

            return status;
        }
    }

    //
    //  Notify deletion. If this is an Alternate Data Stream being deleted,
    //  check if the whole file was deleted (by calling DfIsFileDeleted) as
    //  this could be the last handle to a delete-pending file.
    //

    status = DfIsFileDeleted( Data,
                              FltObjects,
                              StreamContext,
                              isTransaction );

    if (STATUS_FILE_DELETED == status) {

        isFileDeleted = TRUE;
        status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS( status )) {

        DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                      "delete!%s: DfIsFileDeleted returned 0x%08x!\n",
                      __FUNCTION__,
                      status );

        goto _exit;
    }

    DfNotifyDelete( StreamContext,
                    isFileDeleted,
                    transactionContext );

_exit:

    if (NULL != transactionContext) {

        FltReleaseContext( transactionContext );
    }

    return status;
}


//////////////////////////////////////////////////////////////////////////////
//  MiniFilter Operation Callback Routines                                  //
//////////////////////////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS
DfPreCreateCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the pre-operation completion routine for
    IRP_MJ_CREATE in this miniFilter.

    In the pre-create phase we're concerned with creates with
    FILE_DELETE_ON_CLOSE set, and in those cases we want to flag
    this stream as a candidate for being deleted.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - When FILE_DELETE_ON_CLOSE is set and
                                      a stream context is created.

    FLT_PREOP_SUCCESS_NO_CALLBACK   - When FILE_DELETE_ON_CLOSE is not set
                                      and no stream context is created.

--*/
{
    PDF_STREAM_CONTEXT streamContext;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfPreCreateCallback: Entered\n" );

    //
    //  Creates are only interesting in the FILE_DELETE_ON_CLOSE scenario,
    //  in which we'll want to flag this file as a candidate for being
    //  deleted.
    //
    //  The way we do that is allocate a stream context for this and return
    //  FLT_PREOP_SUCCESS_NO_CALLBACK, passing down the stream context via
    //  the completion context, so that the post-create callback can, in case
    //  of a successful create, attach this context to the stream and flag it
    //  as a real deletion candidate.
    //

    if (FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )) {

        status = DfAllocateContext( FLT_STREAM_CONTEXT,
                                    &streamContext );

        if (NT_SUCCESS( status )) {

            *CompletionContext = (PVOID)streamContext;

            return FLT_PREOP_SYNCHRONIZE;

        } else {

            DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                          "delete!DfPreCreateCallback: An error occurred with DfAllocateStreamContext!\n" );
        }
    }

    *CompletionContext = NULL;

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
DfPostCreateCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for
    IRP_MJ_CREATE in this miniFilter.

    The post-create callback will only be called when this is a create with
    FILE_DELETE_ON_CLOSE, meaning we have to flag it as a deletion candidate.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation. This will point to a DF_STREAM_CONTEXT allocated by
        DfPreCreateCallback, which will be used for flagging this stream
        as a deletion candidate.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - we never do any sort of asynchronous
        processing here.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PDF_STREAM_CONTEXT streamContext = NULL;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    ASSERT( NULL != CompletionContext );

    streamContext = (PDF_STREAM_CONTEXT)CompletionContext;

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfPostCreateCallback: Entered\n" );

    // this status check handles the draining scenario.
    if (NT_SUCCESS( Data->IoStatus.Status ) &&
        (STATUS_REPARSE != Data->IoStatus.Status)) {

        // assert we're not draining.
        ASSERT( !FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) );

        //
        //  Flag the stream as a deletion candidate: try setting the stream
        //  context on it to the stream context allocated by DfPreCreateCallback.
        //  If a context is already attached to the stream, DfGetOrSetContext
        //  will do the right thing and set streamContext to it, freeing the
        //  other context.
        //

        status = DfGetOrSetContext( FltObjects,
                                    Data->Iopb->TargetFileObject,
                                    &streamContext,
                                    FLT_STREAM_CONTEXT );

        if (NT_SUCCESS( status )) {

            //
            //  Set DeleteOnClose on the stream context: a delete-on-close stream will
            //  always be checked for deletion on cleanup.
            //

            streamContext->DeleteOnClose = BooleanFlagOn( Data->Iopb->Parameters.Create.Options,
                                                          FILE_DELETE_ON_CLOSE );
        }
    }

    //
    //  We will have a context in streamContext, because if allocation fails
    //  in DfPreCreateCallback, FLT_PREOP_SUCCESS_NO_CALLBACK is returned, so
    //  there is no post-create callback.
    //
    //  If DfGetOrSetContext failed, if will have released streamContext
    //  already, so only release it if status is successful.
    //

    if (NT_SUCCESS( status )) {

        FltReleaseContext( streamContext );
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
DfPreSetInfoCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the pre-operation completion routine for
    IRP_MJ_SET_INFORMATION in this miniFilter.

    The pre-setinfo callback is important because setting
    FileDispositionInformation/FileDispositionInformationEx is another way of
    putting the file in a delete-pending state.

    Since the delete disposition is a reversible condition, we have to
    make sure to do the right thing when multiple operations are racing:
    we won't be able to tell the the final outcome of the delete
    disposition state of the stream, so everytime a race like that happens,
    we assume this stream as a permanent deletion candidate, so it will be
    checked for deletion in the post-cleanup callback.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    FLT_PREOP_SYNCHRONIZE - we never do any sort of asynchronous processing
        here, and we synchronize postop.

    FLT_PREOP_SUCCESS_NO_CALLBACK - if not FileDispositionInformation/FileDispositionInformationEx
        or we can't set a streamcontext.

--*/
{
    NTSTATUS status;
    PDF_STREAM_CONTEXT streamContext = NULL;
    BOOLEAN race;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass) {

        case FileDispositionInformation:
        case FileDispositionInformationEx:

            //
            //  We're interested when the file delete disposition changes.
            //

            status = DfGetOrSetContext( FltObjects,
                                        Data->Iopb->TargetFileObject,
                                        &streamContext,
                                        FLT_STREAM_CONTEXT );

            if (!NT_SUCCESS( status )) {

                return FLT_PREOP_SUCCESS_NO_CALLBACK;
            }

            //
            //  Race detection logic. The NumOps field in the StreamContext
            //  counts the number of in-flight changes to delete disposition
            //  on the stream.
            //
            //  If there's already some operations in flight, don't bother
            //  doing postop. Since there will be no postop, this value won't
            //  be decremented, staying forever 2 or more, which is one of
            //  the conditions for checking deletion at post-cleanup.
            //

            race = (InterlockedIncrement( &streamContext->NumOps ) > 1);

            if (!race) {

                //
                //  This is the only operation in flight, so do a postop on
                //  it because the final outcome of the delete disposition
                //  state of the stream is deterministic.
                //

                *CompletionContext = (PVOID)streamContext;

                return FLT_PREOP_SYNCHRONIZE;

            } else {

                FltReleaseContext( streamContext );
            }

            // FALL_THROUGH

        default:

            return FLT_PREOP_SUCCESS_NO_CALLBACK;

            break;
    }
}


FLT_POSTOP_CALLBACK_STATUS
DfPostSetInfoCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for
    IRP_MJ_SET_INFORMATION in this miniFilter.

    In this postop callback we will update the deletion disposition state
    of this stream in the stream context. This callback will only be reached
    when there's a single change to deletion disposition in flight for the
    stream or when this was the first of many racing ops to hit the preop.

    In the latter case, the race is already detected and adequately flagged
    in the other preops, so we're safe just decrementing NumOps, because the
    other operations will never reach postop and NumOps won't ever be
    decremented for them, guaranteeing that NumOps will stay nonzero forever,
    effectively flagging the race.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - we never do any sort of asynchronous
        processing here.

--*/
{
    PDF_STREAM_CONTEXT streamContext;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    // assert on FileDispositionInformation/FileDispositionInformationEx
    ASSERT( (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation) ||
            (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformationEx) );

    // pass from pre-callback to post-callback
    ASSERT( NULL != CompletionContext );
    streamContext = (PDF_STREAM_CONTEXT) CompletionContext;

    //
    //  Reaching a postop for FileDispositionInformation/FileDispositionInformationEx means we
    //  MUST have a stream context passed in the CompletionContext.
    //

    if (NT_SUCCESS( Data->IoStatus.Status )) {

        //
        //  No synchronization is needed to set the SetDisp field,
        //  because in case of races, the NumOps field will be perpetually
        //  positive, and it being positive is already an indication this
        //  file is a delete candidate, so it will be checked at post-
        //  -cleanup regardless of the value of SetDisp.
        //

        //
        //  Using FileDispositinInformationEx -
        //    FILE_DISPOSITION_ON_CLOSE controls delete on close
        //    or set disposition behavior. It uses FILE_DISPOSITION_INFORMATION_EX structure.
        //    FILE_DISPOSITION_ON_CLOSE is set - Set or clear DeleteOnClose
        //    depending on FILE_DISPOSITION_DELETE flag.
        //    FILE_DISPOSITION_ON_CLOSE is NOT set - Set or clear disposition information
        //    depending on the flag FILE_DISPOSITION_DELETE.
        //
        //
        //   Using FileDispositionInformation -
        //    Controls only set disposition information behavior. It uses FILE_DISPOSITION_INFORMATION structure.
        //

        if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformationEx) {

            ULONG flags = ((PFILE_DISPOSITION_INFORMATION_EX) Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->Flags;

            if (FlagOn( flags, FILE_DISPOSITION_ON_CLOSE )) {

                streamContext->DeleteOnClose = BooleanFlagOn( flags, FILE_DISPOSITION_DELETE );

            } else {

                streamContext->SetDisp = BooleanFlagOn( flags, FILE_DISPOSITION_DELETE );
            }

        } else {

            streamContext->SetDisp = ((PFILE_DISPOSITION_INFORMATION) Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile;
        }
    }

    //
    //  Now that the operation is over, decrement NumOps.
    //

    InterlockedDecrement( &streamContext->NumOps );

    FltReleaseContext( streamContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
DfPreCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the pre-operation completion routine for
    IRP_MJ_CLEANUP in this miniFilter.

    In the preop callback for cleanup, we obtain the file information and
    save it in the stream context, just so we have a name to use when
    reporting file deletions.

    That is done for every stream with an attached stream context because
    those will be deletion candidates most of the time.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    FLT_PREOP_SYNCHRONIZE - we never do any sort of asynchronous processing
        here, and we want to synchronize the postop.

    FLT_PREOP_SUCCESS_NO_CALLBACK - when we don't manage to get a stream
        context.

--*/
{
    PDF_STREAM_CONTEXT streamContext;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfPreCleanupCallback: Entered\n" );

    status = FltGetStreamContext( Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &streamContext );

    if (NT_SUCCESS( status )) {

        //
        //  Only streams with stream context will be sent for deletion check
        //  in post-cleanup, which makes sense because they would only ever
        //  have one if they were flagged as candidates at some point.
        //
        //  Gather file information here so that we have a name to report.
        //  The name will be accurate most of the times, and in the cases it
        //  won't, it serves as a good clue and the stream context pointer
        //  value should offer a way to disambiguate that in case of renames
        //  etc.
        //

        status = DfGetFileNameInformation( Data, streamContext );

        if (NT_SUCCESS( status )) {

            // pass from pre-callback to post-callback
            *CompletionContext = (PVOID)streamContext;

            return FLT_PREOP_SYNCHRONIZE;

        } else {

            FltReleaseContext( streamContext );
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
DfPostCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for
    IRP_MJ_CLEANUP in this miniFilter.

    Post-cleanup is the core of this minifilter. Here we check to see if
    the stream or file were deleted and report that through DbgPrint.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - we never do any sort of asynchronous
        processing here.

--*/
{
    FILE_STANDARD_INFORMATION fileInfo;
    PDF_STREAM_CONTEXT streamContext = NULL;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( CompletionContext );

    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    DF_DBG_PRINT( DFDBG_TRACE_ROUTINES,
                  "delete!DfPostCleanupCallback: Entered\n" );

    // assert we're not draining.
    ASSERT( !FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING ) );

    // pass from pre-callback to post-callback
    ASSERT( NULL != CompletionContext );
    streamContext = (PDF_STREAM_CONTEXT) CompletionContext;

    if (NT_SUCCESS( Data->IoStatus.Status )) {

        //
        //  Determine whether or not we should check for deletion. What
        //  flags a file as a deletion candidate is one or more of the following:
        //
        //  1. NumOps > 0. This means there are or were racing changes to
        //  the file delete disposition state, and, in that case,
        //  we don't know what that state is. So, let's err to the side of
        //  caution and check if it was deleted.
        //
        //  2. SetDisp. If this is TRUE and we haven't raced in setting delete
        //  disposition, this reflects the true delete disposition state of the
        //  file, meaning we must check for deletes if it is set to TRUE.
        //
        //  3. DeleteOnClose. If the file was ever opened with
        //  FILE_DELETE_ON_CLOSE, we must check to see if it was deleted.
        //  FileDispositionInformationEx allows the this flag to be unset.
        //
        //  Also, if a deletion of this stream was already notified, there is no
        //  point notifying it again.
        //

        if (((streamContext->NumOps > 0) ||
             (streamContext->SetDisp) ||
             (streamContext->DeleteOnClose)) &&
            (0 == streamContext->IsNotified)) {

            //
            //  The check for deletion is done via a query to
            //  FileStandardInformation. If that returns STATUS_FILE_DELETED
            //  it means the stream was deleted.
            //

            status = FltQueryInformationFile( Data->Iopb->TargetInstance,
                                              Data->Iopb->TargetFileObject,
                                              &fileInfo,
                                              sizeof(fileInfo),
                                              FileStandardInformation,
                                              NULL );

            if (STATUS_FILE_DELETED == status) {

                status = DfProcessDelete( Data,
                                          FltObjects,
                                          streamContext );

                if (!NT_SUCCESS( status )) {

                    DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                                  "delete!%s: It was not possible to verify "
                                  "deletion due to an error in DfProcessDelete (0x%08x)!\n",
                                  __FUNCTION__,
                                  status );
                }
            }
        }
    }

    FltReleaseContext( streamContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


NTSTATUS
DfTransactionNotificationCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PDF_TRANSACTION_CONTEXT TransactionContext,
    _In_ ULONG NotificationMask
    )
/*++

Routine Description:

    This routine is the transaction notification callback for this minifilter.
    It is called when a transaction we're enlisted in is committed or rolled
    back so that it's possible to emit notifications about files that were
    deleted in that transaction.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    TransactionContext - The transaction context, set/modified when a delete
        is detected.

    NotificationMask - A mask of flags indicating the notifications received
        from FltMgr. Should be either TRANSACTION_NOTIFY_COMMIT or
        TRANSACTION_NOTIFY_ROLLBACK.

Return Value:

    STATUS_SUCCESS - This operation is never pended.

--*/
{
    BOOLEAN commit = BooleanFlagOn( NotificationMask, TRANSACTION_NOTIFY_COMMIT_FINALIZE );
    PDF_DELETE_NOTIFY deleteNotify = NULL;

    UNREFERENCED_PARAMETER( FltObjects );

    PAGED_CODE();

    //
    //  There is no such thing as a simultaneous commit and rollback, nor
    //  should we get notifications for events other than a commit or a
    //  rollback.
    //

    ASSERT( (!FlagOnAll( NotificationMask, (DF_NOTIFICATION_MASK) )) &&
            FlagOn( NotificationMask, (DF_NOTIFICATION_MASK) ) );

    if (commit) {

        DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                      "delete!DfTransactionNotificationCallback: COMMIT!\n" );

    } else {

        DF_DBG_PRINT( DFDBG_TRACE_ERRORS,
                      "delete!DfTransactionNotificationCallback: ROLLBACK!\n" );
    }

    ASSERT( NULL != TransactionContext->Resource );

    FltAcquireResourceExclusive( TransactionContext->Resource );

    while (!IsListEmpty( &TransactionContext->DeleteNotifyList )) {

        deleteNotify = CONTAINING_RECORD( RemoveHeadList( &TransactionContext->DeleteNotifyList ),
                                          DF_DELETE_NOTIFY,
                                          Links );

        ASSERT( NULL != deleteNotify->StreamContext );

        if (!commit) {
            InterlockedDecrement( &deleteNotify->StreamContext->IsNotified );
        }

        DfNotifyDeleteOnTransactionEnd( deleteNotify,
                                        commit );

        // release stream context
        FltReleaseContext( deleteNotify->StreamContext );
        ExFreePool( deleteNotify );
    }

    FltReleaseResource( TransactionContext->Resource );

    return STATUS_SUCCESS;
}




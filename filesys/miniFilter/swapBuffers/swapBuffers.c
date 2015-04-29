/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    SwapBuffers.c

Abstract:

    This is a sample filter which demonstrates proper access of data buffer
    and a general guideline of how to swap buffers.
    For now it only swaps buffers for:

    IRP_MJ_READ
    IRP_MJ_WRITE
    IRP_MJ_DIRECTORY_CONTROL

    By default this filter attaches to all volumes it is notified about.  It
    does support having multiple instances on a given volume.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;

/*************************************************************************
    Pool Tags
*************************************************************************/

#define BUFFER_SWAP_TAG     'bdBS'
#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'
#define PRE_2_POST_TAG      'ppBS'

/*************************************************************************
    Local structures
*************************************************************************/

//
//  This is a volume context, one of these are attached to each volume
//  we monitor.  This is used to get a "DOS" name for debug display.
//

typedef struct _VOLUME_CONTEXT {

    //
    //  Holds the name to display
    //

    UNICODE_STRING Name;

    //
    //  Holds the sector size for this volume.
    //

    ULONG SectorSize;

} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

#define MIN_SECTOR_SIZE 0x200


//
//  This is a context structure that is used to pass state from our
//  pre-operation callback to our post-operation callback.
//

typedef struct _PRE_2_POST_CONTEXT {

    //
    //  Pointer to our volume context structure.  We always get the context
    //  in the preOperation path because you can not safely get it at DPC
    //  level.  We then release it in the postOperation path.  It is safe
    //  to release contexts at DPC level.
    //

    PVOLUME_CONTEXT VolCtx;

    //
    //  Since the post-operation parameters always receive the "original"
    //  parameters passed to the operation, we need to pass our new destination
    //  buffer to our post operation routine so we can free it.
    //

    PVOID SwappedBuffer;

} PRE_2_POST_CONTEXT, *PPRE_2_POST_CONTEXT;

//
//  This is a lookAside list used to allocate our pre-2-post structure.
//

NPAGED_LOOKASIDE_LIST Pre2PostContextList;

/*************************************************************************
    Prototypes
*************************************************************************/

NTSTATUS
InstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
CleanupVolumeContext(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
InstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
FilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreReadBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffersWhenSafe (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreDirCtrlBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffersWhenSafe (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreWriteBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostWriteBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

VOID
ReadDriverParameters (
    _In_ PUNICODE_STRING RegistryPath
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InstanceSetup)
#pragma alloc_text(PAGE, CleanupVolumeContext)
#pragma alloc_text(PAGE, InstanceQueryTeardown)
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, ReadDriverParameters)
#pragma alloc_text(PAGE, FilterUnload)
#endif

//
//  Operation we currently care about.
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_READ,
      0,
      SwapPreReadBuffers,
      SwapPostReadBuffers },

    { IRP_MJ_WRITE,
      0,
      SwapPreWriteBuffers,
      SwapPostWriteBuffers },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      SwapPreDirCtrlBuffers,
      SwapPostDirCtrlBuffers },

    { IRP_MJ_OPERATION_END }
};

//
//  Context definitions we currently care about.  Note that the system will
//  create a lookAside list for the volume context because an explicit size
//  of the context is specified.
//

CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

     { FLT_VOLUME_CONTEXT,
       0,
       CleanupVolumeContext,
       sizeof(VOLUME_CONTEXT),
       CONTEXT_TAG },

     { FLT_CONTEXT_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextNotifications,               //  Context
    Callbacks,                          //  Operation callbacks

    FilterUnload,                       //  MiniFilterUnload

    InstanceSetup,                      //  InstanceSetup
    InstanceQueryTeardown,              //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};

/*************************************************************************
    Debug tracing information
*************************************************************************/

//
//  Definitions to display log messages.  The registry DWORD entry:
//  "hklm\system\CurrentControlSet\Services\Swapbuffers\DebugFlags" defines
//  the default state of these logging flags
//

#define LOGFL_ERRORS    0x00000001  // if set, display error messages
#define LOGFL_READ      0x00000002  // if set, display READ operation info
#define LOGFL_WRITE     0x00000004  // if set, display WRITE operation info
#define LOGFL_DIRCTRL   0x00000008  // if set, display DIRCTRL operation info
#define LOGFL_VOLCTX    0x00000010  // if set, display VOLCTX operation info

ULONG LoggingFlags = 0;             // all disabled by default

#define LOG_PRINT( _logFlag, _string )                              \
    (FlagOn(LoggingFlags,(_logFlag)) ?                              \
        DbgPrint _string  :                                         \
        ((int)0))

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//                      Routines
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


NTSTATUS
InstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume.

    By default we want to attach to all volumes.  This routine will try and
    get a "DOS" name for the given volume.  If it can't, it will try and
    get the "NT" name for the volume (which is what happens on network
    volumes).  If a name is retrieved a volume context will be created with
    that name.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    PDEVICE_OBJECT devObj = NULL;
    PVOLUME_CONTEXT ctx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG retLen;
    PUNICODE_STRING workingName;
    USHORT size;
    UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES)+512];
    PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    try {

        //
        //  Allocate a volume context structure.
        //

        status = FltAllocateContext( FltObjects->Filter,
                                     FLT_VOLUME_CONTEXT,
                                     sizeof(VOLUME_CONTEXT),
                                     NonPagedPool,
                                     &ctx );

        if (!NT_SUCCESS(status)) {

            //
            //  We could not allocate a context, quit now
            //

            leave;
        }

        //
        //  Always get the volume properties, so I can get a sector size
        //

        status = FltGetVolumeProperties( FltObjects->Volume,
                                         volProp,
                                         sizeof(volPropBuffer),
                                         &retLen );

        if (!NT_SUCCESS(status)) {

            leave;
        }

        //
        //  Save the sector size in the context for later use.  Note that
        //  we will pick a minimum sector size if a sector size is not
        //  specified.
        //

        FLT_ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= MIN_SECTOR_SIZE));

        ctx->SectorSize = max(volProp->SectorSize,MIN_SECTOR_SIZE);

        //
        //  Init the buffer field (which may be allocated later).
        //

        ctx->Name.Buffer = NULL;

        //
        //  Get the storage device object we want a name for.
        //

        status = FltGetDiskDeviceObject( FltObjects->Volume, &devObj );

        if (NT_SUCCESS(status)) {

            //
            //  Try and get the DOS name.  If it succeeds we will have
            //  an allocated name buffer.  If not, it will be NULL
            //

            status = IoVolumeDeviceToDosName( devObj, &ctx->Name );
        }

        //
        //  If we could not get a DOS name, get the NT name.
        //

        if (!NT_SUCCESS(status)) {

            FLT_ASSERT(ctx->Name.Buffer == NULL);

            //
            //  Figure out which name to use from the properties
            //

            if (volProp->RealDeviceName.Length > 0) {

                workingName = &volProp->RealDeviceName;

            } else if (volProp->FileSystemDeviceName.Length > 0) {

                workingName = &volProp->FileSystemDeviceName;

            } else {

                //
                //  No name, don't save the context
                //

                status = STATUS_FLT_DO_NOT_ATTACH;
                leave;
            }

            //
            //  Get size of buffer to allocate.  This is the length of the
            //  string plus room for a trailing colon.
            //

            size = workingName->Length + sizeof(WCHAR);

            //
            //  Now allocate a buffer to hold this name
            //

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "ctx->Name.Buffer will not be leaked because it is freed in CleanupVolumeContext")
            ctx->Name.Buffer = ExAllocatePoolWithTag( NonPagedPool,
                                                      size,
                                                      NAME_TAG );
            if (ctx->Name.Buffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

            //
            //  Init the rest of the fields
            //

            ctx->Name.Length = 0;
            ctx->Name.MaximumLength = size;

            //
            //  Copy the name in
            //

            RtlCopyUnicodeString( &ctx->Name,
                                  workingName );

            //
            //  Put a trailing colon to make the display look good
            //

            RtlAppendUnicodeToString( &ctx->Name,
                                      L":" );
        }

        //
        //  Set the context
        //

        status = FltSetVolumeContext( FltObjects->Volume,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      ctx,
                                      NULL );

        //
        //  Log debug info
        //

        LOG_PRINT( LOGFL_VOLCTX,
                   ("SwapBuffers!InstanceSetup:                  Real SectSize=0x%04x, Used SectSize=0x%04x, Name=\"%wZ\"\n",
                    volProp->SectorSize,
                    ctx->SectorSize,
                    &ctx->Name) );

        //
        //  It is OK for the context to already be defined.
        //

        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

            status = STATUS_SUCCESS;
        }

    } finally {

        //
        //  Always release the context.  If the set failed, it will free the
        //  context.  If not, it will remove the reference added by the set.
        //  Note that the name buffer in the ctx will get freed by the context
        //  cleanup routine.
        //

        if (ctx) {

            FltReleaseContext( ctx );
        }

        //
        //  Remove the reference added to the device object by
        //  FltGetDiskDeviceObject.
        //

        if (devObj) {

            ObDereferenceObject( devObj );
        }
    }

    return status;
}


VOID
CleanupVolumeContext(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    The given context is being freed.
    Free the allocated name buffer if there one.

Arguments:

    Context - The context being freed

    ContextType - The type of context this is

Return Value:

    None

--*/
{
    PVOLUME_CONTEXT ctx = Context;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( ContextType );

    FLT_ASSERT(ContextType == FLT_VOLUME_CONTEXT);

    if (ctx->Name.Buffer != NULL) {

        ExFreePool(ctx->Name.Buffer);
        ctx->Name.Buffer = NULL;
    }
}


NTSTATUS
InstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach.  We always return it is OK to
    detach.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Always succeed.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    return STATUS_SUCCESS;
}


/*************************************************************************
    Initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine.  This registers with FltMgr and
    initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS status;

    //
    //  Default to NonPagedPoolNx for non paged pool allocations where supported.
    //
    
    ExInitializeDriverRuntime( DrvRtPoolNxOptIn );

    //
    //  Get debug trace flags
    //

    ReadDriverParameters( RegistryPath );

    //
    //  Init lookaside list used to allocate our context structure used to
    //  pass information from out preOperation callback to our postOperation
    //  callback.
    //

    ExInitializeNPagedLookasideList( &Pre2PostContextList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(PRE_2_POST_CONTEXT),
                                     PRE_2_POST_TAG,
                                     0 );

    //
    //  Register with FltMgr
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    if (! NT_SUCCESS( status )) {

        goto SwapDriverEntryExit;
    }

    //
    //  Start filtering i/o
    //

    status = FltStartFiltering( gFilterHandle );

    if (! NT_SUCCESS( status )) {

        FltUnregisterFilter( gFilterHandle );
        goto SwapDriverEntryExit;
    }

SwapDriverEntryExit:

    if(! NT_SUCCESS( status )) {

        ExDeleteNPagedLookasideList( &Pre2PostContextList );
    }

    return status;
}


NTSTATUS
FilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    Called when this mini-filter is about to be unloaded.  We unregister
    from the FltMgr and then return it is OK to unload

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );

    //
    //  Unregister from FLT mgr
    //

    FltUnregisterFilter( gFilterHandle );

    //
    //  Delete lookaside list
    //

    ExDeleteNPagedLookasideList( &Pre2PostContextList );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/

FLT_PREOP_CALLBACK_STATUS
SwapPreReadBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the READ operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    NTSTATUS status;
    ULONG readLen = iopb->Parameters.Read.Length;

    try {

        //
        //  If they are trying to read ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        //

        if (readLen == 0) {

            leave;
        }

        //
        //  Get our volume context so we can display our volume name in the
        //  debug output.
        //

        status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volCtx );

        if (!NT_SUCCESS(status)) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreReadBuffers:             Error getting volume context, status=%x\n",
                        status) );

            leave;
        }

        //
        //  If this is a non-cached I/O we need to round the length up to the
        //  sector size for this device.  We must do this because the file
        //  systems do this and we need to make sure our buffer is as big
        //  as they are expecting.
        //

        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags)) {

            readLen = (ULONG)ROUND_TO_SIZE(readLen,volCtx->SectorSize);
        }

        //
        //  Allocate aligned nonPaged memory for the buffer we are swapping
        //  to. This is really only necessary for noncached IO but we always
        //  do it here for simplification. If we fail to get the memory, just
        //  don't swap buffers on this operation.
        //

        newBuf = FltAllocatePoolAlignedWithTag( FltObjects->Instance, 
                                                NonPagedPool,
                                                (SIZE_T) readLen,
                                                BUFFER_SWAP_TAG );
        if (newBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreReadBuffers:             %wZ Failed to allocate %d bytes of memory\n",
                        &volCtx->Name,
                        readLen) );

            leave;
        }

        //
        //  We only need to build a MDL for IRP operations.  We don't need to
        //  do this for a FASTIO operation since the FASTIO interface has no
        //  parameter for passing the MDL to the file system.
        //

        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

            //
            //  Allocate a MDL for the new allocated memory.  If we fail
            //  the MDL allocation then we won't swap buffer for this operation
            //

            newMdl = IoAllocateMdl( newBuf,
                                    readLen,
                                    FALSE,
                                    FALSE,
                                    NULL );

            if (newMdl == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPreReadBuffers:             %wZ Failed to allocate MDL\n",
                            &volCtx->Name) );

                leave;
            }

            //
            //  setup the MDL for the non-paged pool we just allocated
            //

            MmBuildMdlForNonPagedPool( newMdl );
        }

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        //

        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );

        if (p2pCtx == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreReadBuffers:             %wZ Failed to allocate pre2Post context structure\n",
                        &volCtx->Name) );

            leave;
        }

        //
        //  Log that we are swapping
        //

        LOG_PRINT( LOGFL_READ,
                   ("SwapBuffers!SwapPreReadBuffers:             %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &volCtx->Name,
                    newBuf,
                    newMdl,
                    iopb->Parameters.Read.ReadBuffer,
                    iopb->Parameters.Read.MdlAddress,
                    readLen) );

        //
        //  Update the buffer pointers and MDL address, mark we have changed
        //  something.
        //

        iopb->Parameters.Read.ReadBuffer = newBuf;
        iopb->Parameters.Read.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //
        //  Pass state to our post-operation callback.
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  Return we want a post-operation callback
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } finally {

        //
        //  If we don't want a post-operation callback, then cleanup state.
        //

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                FltFreePoolAlignedWithTag( FltObjects->Instance,
                                           newBuf,
                                           BUFFER_SWAP_TAG );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }

            if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine does postRead buffer swap handling

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING
    FLT_POSTOP_MORE_PROCESSING_REQUIRED

--*/
{
    PVOID origBuf;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    //
    //  This system won't draining an operation with swapped buffers, verify
    //  the draining flag is not set.
    //

    FLT_ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    try {

        //
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        //

        if (!NT_SUCCESS(Data->IoStatus.Status) ||
            (Data->IoStatus.Information == 0)) {

            LOG_PRINT( LOGFL_READ,
                       ("SwapBuffers!SwapPostReadBuffers:            %wZ newB=%p No data read, status=%x, info=%Iu\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );

            leave;
        }

        //
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers.
        //

        if (iopb->Parameters.Read.MdlAddress != NULL) {

            //
            //  This should be a simple MDL. We don't expect chained MDLs
            //  this high up the stack
            //
            
            FLT_ASSERT( ((PMDL)iopb->Parameters.Read.MdlAddress)->Next == NULL);

            //
            //  Since there is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.
            //

            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress,
                                                    NormalPagePriority | MdlMappingNoExecute );

            if (origBuf == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostReadBuffers:            %wZ Failed to get system address for MDL: %p\n",
                            &p2pCtx->VolCtx->Name,
                            iopb->Parameters.Read.MdlAddress) );

                //
                //  If we failed to get a SYSTEM address, mark that the read
                //  failed and return.
                //

                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                leave;
            }

        } else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
                   FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) {

            //
            //  If this is a system buffer, just use the given address because
            //      it is valid in all thread contexts.
            //  If this is a FASTIO operation, we can just use the
            //      buffer (inside a try/except) since we know we are in
            //      the correct thread context (you can't pend FASTIO's).
            //

            origBuf = iopb->Parameters.Read.ReadBuffer;

        } else {

            //
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.
            //

            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   SwapPostReadBuffersWhenSafe,
                                                   &retValue )) {

                //
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                //

                cleanupAllocatedBuffer = FALSE;

            } else {

                //
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                //

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostReadBuffers:            %wZ Unable to post to a safe IRQL\n",
                            &p2pCtx->VolCtx->Name) );

                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }

            leave;
        }

        //
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        //

        try {

            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           Data->IoStatus.Information );

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  The copy failed, return an error, failing the operation.
            //

            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostReadBuffers:            %wZ Invalid user buffer, oldB=%p, status=%x\n",
                        &p2pCtx->VolCtx->Name,
                        origBuf,
                        Data->IoStatus.Status) );
        }

    } finally {

        //
        //  If we are supposed to, cleanup the allocated memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        //

        if (cleanupAllocatedBuffer) {

            LOG_PRINT( LOGFL_READ,
                       ("SwapBuffers!SwapPostReadBuffers:            %wZ newB=%p info=%Iu Freeing\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Information) );

            FltFreePoolAlignedWithTag( FltObjects->Instance,
                                       p2pCtx->SwappedBuffer,
                                       BUFFER_SWAP_TAG );

            FltReleaseContext( p2pCtx->VolCtx );

            ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                         p2pCtx );
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
SwapPostReadBuffersWhenSafe (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    We had an arbitrary users buffer without a MDL so we needed to get
    to a safe IRQL so we could lock it and then copy the data.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Contains state from our PreOperation callback

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - This is always returned.

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    FLT_ASSERT(Data->IoStatus.Information != 0);

    //
    //  This is some sort of user buffer without a MDL, lock the user buffer
    //  so we can access it.  This will create a MDL for it.
    //

    status = FltLockUserBuffer( Data );

    if (!NT_SUCCESS(status)) {

        LOG_PRINT( LOGFL_ERRORS,
                   ("SwapBuffers!SwapPostReadBuffersWhenSafe:    %wZ Could not lock user buffer, oldB=%p, status=%x\n",
                    &p2pCtx->VolCtx->Name,
                    iopb->Parameters.Read.ReadBuffer,
                    status) );

        //
        //  If we can't lock the buffer, fail the operation
        //

        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    } else {

        //
        //  Get a system address for this buffer.
        //

        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress,
                                                NormalPagePriority | MdlMappingNoExecute );

        if (origBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostReadBuffersWhenSafe:    %wZ Failed to get system address for MDL: %p\n",
                        &p2pCtx->VolCtx->Name,
                        iopb->Parameters.Read.MdlAddress) );

            //
            //  If we couldn't get a SYSTEM buffer address, fail the operation
            //

            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        } else {

            //
            //  Copy the data back to the original buffer.  Note that we
            //  don't need a try/except because we will always have a system
            //  buffer address.
            //

            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           Data->IoStatus.Information );
        }
    }

    //
    //  Free allocated memory and release the volume context
    //

    LOG_PRINT( LOGFL_READ,
               ("SwapBuffers!SwapPostReadBuffersWhenSafe:    %wZ newB=%p info=%Iu Freeing\n",
                &p2pCtx->VolCtx->Name,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    FltFreePoolAlignedWithTag( FltObjects->Instance,
                               p2pCtx->SwappedBuffer,
                               BUFFER_SWAP_TAG );

    FltReleaseContext( p2pCtx->VolCtx );

    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
SwapPreDirCtrlBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the Directory Control
    operations.  The reason this routine is here is because directory change
    notifications are long lived and this allows you to see how FltMgr
    handles long lived IRP operations that have swapped buffers when the
    mini-filter is unloaded.  It does this by canceling the IRP.

    Note that it handles all errors by simply not doing the
    buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    NTSTATUS status;

    try {

        //
        //  If they are trying to get ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        //

        if (iopb->Parameters.DirectoryControl.QueryDirectory.Length == 0) {

            leave;
        }

        //
        //  Get our volume context.  If we can't get it, just return.
        //

        status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volCtx );

        if (!NT_SUCCESS(status)) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          Error getting volume context, status=%x\n",
                        status) );

            leave;
        }

        //
        //  Allocate nonPaged memory for the buffer we are swapping to.
        //  If we fail to get the memory, just don't swap buffers on this
        //  operation.
        //

        newBuf = ExAllocatePoolWithTag( NonPagedPool,
                                        iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                        BUFFER_SWAP_TAG );

        if (newBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ Failed to allocate %d bytes of memory.\n",
                        &volCtx->Name,
                        iopb->Parameters.DirectoryControl.QueryDirectory.Length) );

            leave;
        }

        //
        //  Zero the new buffer so as not to potentially expose any sensitive
        //  data to the user.
        //

        RtlZeroMemory( newBuf, iopb->Parameters.DirectoryControl.QueryDirectory.Length );


        //
        //  We need to build a MDL because Directory Control Operations are always IRP operations.  
        //


        //
        //  Allocate a MDL for the new allocated memory.  If we fail
        //  the MDL allocation then we won't swap buffer for this operation
        //

        newMdl = IoAllocateMdl( newBuf,
                                iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                FALSE,
                                FALSE,
                                NULL );

        if (newMdl == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ Failed to allocate MDL.\n",
                        &volCtx->Name) );

           leave;
        }

        //
        //  setup the MDL for the non-paged pool we just allocated
        //

        MmBuildMdlForNonPagedPool( newMdl );

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        //

        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );

        if (p2pCtx == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ Failed to allocate pre2Post context structure\n",
                        &volCtx->Name) );

            leave;
        }

        //
        //  Log that we are swapping
        //

        LOG_PRINT( LOGFL_DIRCTRL,
                   ("SwapBuffers!SwapPreDirCtrlBuffers:          %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &volCtx->Name,
                    newBuf,
                    newMdl,
                    iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer,
                    iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                    iopb->Parameters.DirectoryControl.QueryDirectory.Length) );

        //
        //  Update the buffer pointers and MDL address
        //

        iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = newBuf;
        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //
        //  Pass state to our post-operation callback.
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  Return we want a post-operation callback
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } finally {

        //
        //  If we don't want a post-operation callback, then cleanup state.
        //

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                ExFreePool( newBuf );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }

            if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine does the post Directory Control buffer swap handling.

Arguments:

    This routine does postRead buffer swap handling
    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING
    FLT_POSTOP_MORE_PROCESSING_REQUIRED

--*/
{
    PVOID origBuf;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    //
    //  Verify we are not draining an operation with swapped buffers
    //

    FLT_ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    try {

        //
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        //

        if (!NT_SUCCESS(Data->IoStatus.Status) ||
            (Data->IoStatus.Information == 0)) {

            LOG_PRINT( LOGFL_DIRCTRL,
                       ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ newB=%p No data read, status=%x, info=%Ix\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );

            leave;
        }

        //
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers
        //

        if (iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL) {

            //
            //  There is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.
            //

            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                                                    NormalPagePriority | MdlMappingNoExecute );

            if (origBuf == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ Failed to get system address for MDL: %p\n",
                            &p2pCtx->VolCtx->Name,
                            iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress) );

                //
                //  If we failed to get a SYSTEM address, mark that the
                //  operation failed and return.
                //

                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                leave;
            }

        } else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
                   FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) {

            //
            //  If this is a system buffer, just use the given address because
            //      it is valid in all thread contexts.
            //  If this is a FASTIO operation, we can just use the
            //      buffer (inside a try/except) since we know we are in
            //      the correct thread context.
            //

            origBuf = iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

        } else {

            //
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.
            //

            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   SwapPostDirCtrlBuffersWhenSafe,
                                                   &retValue )) {

                //
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                //

                cleanupAllocatedBuffer = FALSE;

            } else {

                //
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                //

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ Unable to post to a safe IRQL\n",
                            &p2pCtx->VolCtx->Name) );

                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }

            leave;
        }

        //
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        //
        //  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
        //         length in the information field (it is sort) we are always
        //         going to copy the original buffer length. Please note that
        //         this is a potential security problem because we will copy 
        //         more than what was touched by the FS. So we have to make 
        //         sure the buffer is clean before calling into the FS or we 
        //         risk exposing sensitive data to the user.
        //

        try {

            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ Invalid user buffer, oldB=%p, status=%x, info=%Iu\n",
                        &p2pCtx->VolCtx->Name,
                        origBuf,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );
        }

    } finally {

        //
        //  If we are supposed to, cleanup the allocate memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        //

        if (cleanupAllocatedBuffer) {

            LOG_PRINT( LOGFL_DIRCTRL,
                       ("SwapBuffers!SwapPostDirCtrlBuffers:         %wZ newB=%p info=%Iu Freeing\n",
                        &p2pCtx->VolCtx->Name,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Information) );

            ExFreePool( p2pCtx->SwappedBuffer );
            FltReleaseContext( p2pCtx->VolCtx );

            ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                         p2pCtx );
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffersWhenSafe (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    We had an arbitrary users buffer without a MDL so we needed to get
    to a safe IRQL so we could lock it and then copy the data.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The buffer we allocated and swapped to

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - This is always returned.

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    FLT_ASSERT(Data->IoStatus.Information != 0);

    //
    //  This is some sort of user buffer without a MDL, lock the
    //  user buffer so we can access it
    //

    status = FltLockUserBuffer( Data );

    if (!NT_SUCCESS(status)) {

        LOG_PRINT( LOGFL_ERRORS,
                   ("SwapBuffers!SwapPostDirCtrlBuffersWhenSafe: %wZ Could not lock user buffer, oldB=%p, status=%x\n",
                    &p2pCtx->VolCtx->Name,
                    iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer,
                    status) );

        //
        //  If we can't lock the buffer, fail the operation
        //

        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    } else {

        //
        //  Get a system address for this buffer.
        //

        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                                                NormalPagePriority | MdlMappingNoExecute );

        if (origBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPostDirCtrlBuffersWhenSafe: %wZ Failed to get System address for MDL: %p\n",
                        &p2pCtx->VolCtx->Name,
                        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress) );

            //
            //  If we couldn't get a SYSTEM buffer address, fail the operation
            //

            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        } else {

            //
            //  Copy the data back to the original buffer
            //
            //  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
            //         length in the information field (it is short) we are
            //         always going to copy the original buffer length.
            //

            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );
        }
    }

    //
    //  Free the memory we allocated and return
    //

    LOG_PRINT( LOGFL_DIRCTRL,
               ("SwapBuffers!SwapPostDirCtrlBuffersWhenSafe: %wZ newB=%p info=%Iu Freeing\n",
                &p2pCtx->VolCtx->Name,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    ExFreePool( p2pCtx->SwappedBuffer );
    FltReleaseContext( p2pCtx->VolCtx );

    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
SwapPreWriteBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the WRITE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback
    FLT_PREOP_COMPLETE -
--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PVOLUME_CONTEXT volCtx = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    PVOID origBuf;
    NTSTATUS status;
    ULONG writeLen = iopb->Parameters.Write.Length;

    try {

        //
        //  If they are trying to write ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        //

        if (writeLen == 0) {

            leave;
        }

        //
        //  Get our volume context so we can display our volume name in the
        //  debug output.
        //

        status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volCtx );

        if (!NT_SUCCESS(status)) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            Error getting volume context, status=%x\n",
                        status) );

            leave;
        }

        //
        //  If this is a non-cached I/O we need to round the length up to the
        //  sector size for this device.  We must do this because the file
        //  systems do this and we need to make sure our buffer is as big
        //  as they are expecting.
        //

        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags)) {

            writeLen = (ULONG)ROUND_TO_SIZE(writeLen,volCtx->SectorSize);
        }

        //
        //  Allocate aligned nonPaged memory for the buffer we are swapping
        //  to. This is really only necessary for noncached IO but we always
        //  do it here for simplification. If we fail to get the memory, just
        //  don't swap buffers on this operation.
        //

        newBuf = FltAllocatePoolAlignedWithTag( FltObjects->Instance, 
                                                NonPagedPool,
                                                (SIZE_T) writeLen,
                                                BUFFER_SWAP_TAG );

        if (newBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to allocate %d bytes of memory.\n",
                        &volCtx->Name,
                        writeLen) );

            leave;
        }

        //
        //  We only need to build a MDL for IRP operations.  We don't need to
        //  do this for a FASTIO operation because it is a waste of time since
        //  the FASTIO interface has no parameter for passing the MDL to the
        //  file system.
        //

        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

            //
            //  Allocate a MDL for the new allocated memory.  If we fail
            //  the MDL allocation then we won't swap buffer for this operation
            //

            newMdl = IoAllocateMdl( newBuf,
                                    writeLen,
                                    FALSE,
                                    FALSE,
                                    NULL );

            if (newMdl == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to allocate MDL.\n",
                            &volCtx->Name) );

                leave;
            }

            //
            //  setup the MDL for the non-paged pool we just allocated
            //

            MmBuildMdlForNonPagedPool( newMdl );
        }

        //
        //  If the users original buffer had a MDL, get a system address.
        //

        if (iopb->Parameters.Write.MdlAddress != NULL) {

            //
            //  This should be a simple MDL. We don't expect chained MDLs
            //  this high up the stack
            //
            
            FLT_ASSERT( ((PMDL)iopb->Parameters.Write.MdlAddress)->Next == NULL);

            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Write.MdlAddress,
                                                    NormalPagePriority | MdlMappingNoExecute );

            if (origBuf == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to get system address for MDL: %p\n",
                            &volCtx->Name,
                            iopb->Parameters.Write.MdlAddress) );

                //
                //  If we could not get a system address for the users buffer,
                //  then we are going to fail this operation.
                //

                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                retValue = FLT_PREOP_COMPLETE;
                leave;
            }

        } else {

            //
            //  There was no MDL defined, use the given buffer address.
            //

            origBuf = iopb->Parameters.Write.WriteBuffer;
        }

        //
        //  Copy the memory, we must do this inside the try/except because we
        //  may be using a users buffer address
        //

        try {

            RtlCopyMemory( newBuf,
                           origBuf,
                           writeLen );

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  The copy failed, return an error, failing the operation.
            //

            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;
            retValue = FLT_PREOP_COMPLETE;

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            %wZ Invalid user buffer, oldB=%p, status=%x\n",
                        &volCtx->Name,
                        origBuf,
                        Data->IoStatus.Status) );

            leave;
        }

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        //

        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );

        if (p2pCtx == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("SwapBuffers!SwapPreWriteBuffers:            %wZ Failed to allocate pre2Post context structure\n",
                        &volCtx->Name) );

            leave;
        }

        //
        //  Set new buffers
        //

        LOG_PRINT( LOGFL_WRITE,
                   ("SwapBuffers!SwapPreWriteBuffers:            %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &volCtx->Name,
                    newBuf,
                    newMdl,
                    iopb->Parameters.Write.WriteBuffer,
                    iopb->Parameters.Write.MdlAddress,
                    writeLen) );

        iopb->Parameters.Write.WriteBuffer = newBuf;
        iopb->Parameters.Write.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //
        //  Pass state to our post-operation callback.
        //

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = volCtx;

        *CompletionContext = p2pCtx;

        //
        //  Return we want a post-operation callback
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } finally {

        //
        //  If we don't want a post-operation callback, then free the buffer
        //  or MDL if it was allocated.
        //

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                FltFreePoolAlignedWithTag( FltObjects->Instance,
                                           newBuf,
                                           BUFFER_SWAP_TAG );

            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }

            if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }
        }
    }

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
SwapPostWriteBuffers(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    LOG_PRINT( LOGFL_WRITE,
               ("SwapBuffers!SwapPostWriteBuffers:           %wZ newB=%p info=%Iu Freeing\n",
                &p2pCtx->VolCtx->Name,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    //
    //  Free allocate POOL and volume context
    //

    FltFreePoolAlignedWithTag( FltObjects->Instance,
                               p2pCtx->SwappedBuffer,
                               BUFFER_SWAP_TAG );

    FltReleaseContext( p2pCtx->VolCtx );

    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


VOID
ReadDriverParameters (
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the driver-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key passed to the driver during driver entry.

Return Value:

    None.

--*/
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

    //
    //  If this value is not zero then somebody has already explicitly set it
    //  so don't override those settings.
    //

    if (0 == LoggingFlags) {

        //
        //  Open the desired registry key
        //

        InitializeObjectAttributes( &attributes,
                                    RegistryPath,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        status = ZwOpenKey( &driverRegKey,
                            KEY_READ,
                            &attributes );

        if (!NT_SUCCESS( status )) {

            return;
        }

        //
        // Read the given value from the registry.
        //

        RtlInitUnicodeString( &valueName, L"DebugFlags" );

        status = ZwQueryValueKey( driverRegKey,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  buffer,
                                  sizeof(buffer),
                                  &resultLength );

        if (NT_SUCCESS( status )) {

            LoggingFlags = *((PULONG) &(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));
        }

        //
        //  Close the registry entry
        //

        ZwClose(driverRegKey);
    }
}


/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    context.c

Abstract:

    Filter Context-related module implementation.

Environment:

    Kernel mode

--*/

#include "avscan.h"

//
//  Local function prototypes.
//

VOID
AvStreamContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
AvTransactionContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
AvSectionContextCleanup(
  _In_ PFLT_CONTEXT Context,
  _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
AvInstanceContextCleanup(
  _In_ PFLT_CONTEXT Context,
  _In_ FLT_CONTEXT_TYPE ContextType
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AvCreateStreamContext)
#pragma alloc_text(PAGE, AvCreateStreamHandleContext)
#pragma alloc_text(PAGE, AvFindOrCreateTransactionContext)
#pragma alloc_text(PAGE, AvCreateSectionContext)
#pragma alloc_text(PAGE, AvStreamContextCleanup)
#pragma alloc_text(PAGE, AvTransactionContextCleanup)
#pragma alloc_text(PAGE, AvSectionContextCleanup)
#pragma alloc_text(PAGE, AvInstanceContextCleanup)
#pragma alloc_text(PAGE, AvAllocateScanContext)
#pragma alloc_text(PAGE, AvReferenceScanContext)
#pragma alloc_text(PAGE, AvReleaseScanContext)
#endif


//
//  Context registration structure
//

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_STREAM_CONTEXT,
      0,
      AvStreamContextCleanup,
      AV_STREAM_CONTEXT_SIZE,
      AV_STREAM_CONTEXT_TAG },

    { FLT_STREAMHANDLE_CONTEXT,
      0,
      NULL,
      AV_STREAMHANDLE_CONTEXT_SIZE,
      AV_STREAMHANDLE_CONTEXT_TAG },

    { FLT_TRANSACTION_CONTEXT,
      0,
      AvTransactionContextCleanup,
      AV_TRANSACTION_CONTEXT_SIZE,
      AV_TRANSACTION_CONTEXT_TAG },

    { FLT_SECTION_CONTEXT,
      0,
      AvSectionContextCleanup,
      AV_SECTION_CONTEXT_SIZE,
      AV_SECTION_CONTEXT_TAG },

    { FLT_INSTANCE_CONTEXT,
      0,
      AvInstanceContextCleanup,
      AV_INSTANCE_CONTEXT_SIZE,
      AV_INSTANCE_CONTEXT_TAG },

    { FLT_CONTEXT_END }
};


VOID
AvStreamContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This function is called by the filter manager before freeing any of the minifilter
    driver's contexts of that type.

    In this routine, the driver has to perform any needed cleanup, such as freeing
    additional memory that the minifilter driver allocated inside the context structure

Arguments:

    Context - Pointer to the minifilter driver's portion of the context.
    ContextType - Supposed to be FLT_STREAM_CONTEXT.

Return Value:

    None

--*/
{
    PAV_STREAM_CONTEXT streamContext = (PAV_STREAM_CONTEXT) Context;
    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    FLT_ASSERTMSG( "[AV]: Stream context is not supposed to be in the transaction context list at cleanup.!\n",
                   NULL == streamContext->TxContext );

    AvFreeKevent( streamContext->ScanSynchronizationEvent );
}

VOID
AvTransactionContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This function is called by the filter manager before freeing any of the minifilter
    driver's contexts of that type.

    In this routine, the driver has to perform any needed cleanup, such as freeing
    additional memory that the minifilter driver allocated inside the context structure

    We delete the stream context list in transaction context here.

Arguments:

    Context - Pointer to the minifilter driver's portion of the context.
    ContextType - Supposed to be FLT_TRANSACTION_CONTEXT.

Return Value:

    None

--*/
{
    PAV_TRANSACTION_CONTEXT transactionContext = (PAV_TRANSACTION_CONTEXT) Context;

    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                    ("[Av]: AvTransactionContextCleanup context cleanup entered.\n") );

    ExDeleteResourceLite( transactionContext->Resource );
    AvFreeResource( transactionContext->Resource );
    transactionContext->Resource = NULL;
    ObDereferenceObject( transactionContext->Transaction );
    transactionContext->Transaction = NULL;
}

VOID
AvSectionContextCleanup(
  _In_ PFLT_CONTEXT Context,
  _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This function is called by the filter manager before freeing any of the minifilter
    driver's contexts of that type.

    In this routine, the driver has to perform any needed cleanup, such as freeing
    additional memory that the minifilter driver allocated inside the context structure

Arguments:

    Context - Pointer to the minifilter driver's portion of the context.
    ContextType - Supposed to be FLT_SECTION_CONTEXT (win8 or later).

Return Value:

    None

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( ContextType );

    FLT_ASSERTMSG( "[AV] AvSectionContextCleanup: Section handle should be NULL at cleanup.\n",
                   ((PAV_SECTION_CONTEXT) Context)->SectionHandle == NULL );
    FLT_ASSERTMSG( "[AV] AvSectionContextCleanup: Section object should be NULL at cleanup.\n",
                   ((PAV_SECTION_CONTEXT) Context)->SectionObject == NULL );

}

VOID
AvInstanceContextCleanup(
  _In_ PFLT_CONTEXT Context,
  _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This function is called by the filter manager before freeing any of the minifilter
    driver's contexts of that type.

    In this routine, the driver has to perform any needed cleanup, such as freeing
    additional memory that the minifilter driver allocated inside the context structure.

    We delete the cache table if the file system supports one.

Arguments:

    Context - Pointer to the minifilter driver's portion of the context.
    ContextType - Supposed to be FLT_INSTANCE_CONTEXT (win8 or later).

Return Value:

    None

--*/
{

    PAV_INSTANCE_CONTEXT instanceContext = (PAV_INSTANCE_CONTEXT) Context;

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( ContextType );

    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                ( "[Av]: AvInstanceContextCleanup context cleanup entered\n") );

    if (FS_SUPPORTS_FILE_STATE_CACHE( instanceContext->VolumeFSType )) {

        FLT_ASSERTMSG( "[AV] AvInstanceContextCleanup: The generic table should be empty at cleanup.\n",
                        RtlIsGenericTableEmpty( &instanceContext->FileStateCacheTable ) );
        ExDeleteResourceLite( &instanceContext->Resource );
    }
}

NTSTATUS
AvCreateStreamHandleContext (
    _In_ PFLT_FILTER Filter,
    _Outptr_ PAV_STREAMHANDLE_CONTEXT *StreamHandleContext
    )
/*++

Routine Description:

    This routine creates a new streamhandle context

Arguments:

    StreamHandleContext - Returns the streamhandle context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PAV_STREAMHANDLE_CONTEXT streamHandleContext;

    PAGED_CODE();

    //
    //  Allocate a streamhandle context
    //

    status = FltAllocateContext( Filter,
                                 FLT_STREAMHANDLE_CONTEXT,
                                 AV_STREAMHANDLE_CONTEXT_SIZE,
                                 PagedPool,
                                 &streamHandleContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[Av]: Failed to allocate stream handle context with status 0x%x \n",
                     status) );
        return status;
    }

    //
    //  Initialize the newly created context
    //

    RtlZeroMemory(streamHandleContext, AV_STREAMHANDLE_CONTEXT_SIZE);
    *StreamHandleContext = streamHandleContext;

    return STATUS_SUCCESS;
}

NTSTATUS
AvCreateStreamContext (
    _In_ PFLT_FILTER Filter,
    _Outptr_ PAV_STREAM_CONTEXT *StreamContext
    )
/*++

Routine Description:

    This routine creates a new stream context

Arguments:

    StreamContext - Returns the stream context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PKEVENT  event = NULL;
    PAV_STREAM_CONTEXT streamContext;

    PAGED_CODE();

    //
    //  Allocate the kernel event object
    //

    event = AvAllocateKevent();

    if (NULL == event) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Allocate a stream context
    //

    status = FltAllocateContext( Filter,
                                 FLT_STREAM_CONTEXT,
                                 AV_STREAM_CONTEXT_SIZE,
                                 PagedPool,
                                 &streamContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[Av]: Failed to allocate stream context with status 0x%x \n",
                     status) );
        AvFreeKevent( event );
        return status;
    }

    //
    //  Initialize the newly created context
    //

    RtlZeroMemory(streamContext, AV_STREAM_CONTEXT_SIZE);
    streamContext->ScanSynchronizationEvent = event;
    KeInitializeEvent( streamContext->ScanSynchronizationEvent, SynchronizationEvent, TRUE );
    SET_FILE_MODIFIED( streamContext );
    SET_FILE_TX_MODIFIED( streamContext );
    *StreamContext = streamContext;

    return STATUS_SUCCESS;
}

NTSTATUS
AvFindOrCreateTransactionContext(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_ PAV_TRANSACTION_CONTEXT *TransactionContext
    )
/*++

Routine Description

    This routine finds the transaction context, if not found, it will
    try to create a new one. The caller is responsible for calling
    FltReleaseContext to decrement its reference count.

Arguments

    FltObjects - Contains parameters required to enlist in a transaction.

    TransactionContext - Returns the transaction context

Return value

    Returns STATUS_SUCCESS if we were able to successfully find/create
    a transaction context. Returns an appropriate error code on a failure.

--*/
{
    NTSTATUS status;
    PAV_TRANSACTION_CONTEXT transactionContext = NULL;
    PAV_TRANSACTION_CONTEXT oldTransactionContext = NULL;
    PERESOURCE pResource = NULL;

    PAGED_CODE();

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                ("[Av]: AvFindOrCreateTransactionContext entered. \n") );

    status = FltGetTransactionContext( FltObjects->Instance,
                                       FltObjects->Transaction,
                                       &transactionContext );

    if (NT_SUCCESS( status )) {

        *TransactionContext = transactionContext;
        return STATUS_SUCCESS;
    }

    if (status != STATUS_NOT_FOUND) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[AV]: Failed to get transaction context with status 0x%x \n",
                     status) );
        return status;
    }

    //
    //  Allocate the resource
    //

    pResource = AvAllocateResource();

    if ( NULL == pResource ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Allocate a transaction context.
    //

    status = FltAllocateContext( Globals.Filter,
                                 FLT_TRANSACTION_CONTEXT,
                                 AV_TRANSACTION_CONTEXT_SIZE,
                                 PagedPool,
                                 &transactionContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV]: Failed to allocate transaction context with status 0x%x \n",
                 status) );
        AvFreeResource( pResource );
        return status;
    }

    FLT_ASSERTMSG( "[AV]: Transaction object pointer is not supposed to be NULL !\n", FltObjects->Transaction != NULL);

    //
    //  Initialization of transaction context.
    //  The reason we allocate eResource seperately is because
    //  eResource has to be allocated in the non-paged pool.
    //

    RtlZeroMemory(transactionContext, AV_TRANSACTION_CONTEXT_SIZE);
    transactionContext->Resource = pResource;
    ObReferenceObject( FltObjects->Transaction );
    transactionContext->Transaction = FltObjects->Transaction;
    InitializeListHead( &transactionContext->ScListHead );
    ExInitializeResourceLite( transactionContext->Resource );

    status = FltSetTransactionContext( FltObjects->Instance,
                                       FltObjects->Transaction,
                                       FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                       transactionContext,
                                       &oldTransactionContext );

    if (NT_SUCCESS( status )) {

        *TransactionContext = transactionContext;
        return STATUS_SUCCESS;
    }

    FltReleaseContext( transactionContext );

    if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("[Av]: Failed to set transaction context with status 0x%x \n",
             status) );

        return status;
    }

    if (NULL == oldTransactionContext) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("[Av]: Failed to set transaction context oldTransactionContext is NULL \n") );

        return status;
    }

    *TransactionContext = oldTransactionContext;


    return STATUS_SUCCESS;
}

NTSTATUS
AvCreateSectionContext (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Outptr_ PAV_SECTION_CONTEXT *SectionContext
    )
/*++

Routine Description:

    This routine creates a new section context.

Arguments:

    Instance - Opaque instance pointer for the caller. This parameter is required and cannot be NULL.

    FileObject - File object pointer for the file. This parameter is required and cannot be NULL.

    SectionContext - Returns the section context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    LONGLONG fileSize;
    PAV_SECTION_CONTEXT sectionContext = NULL;

    PAGED_CODE();

    status = FltAllocateContext( Globals.Filter,
                                 FLT_SECTION_CONTEXT,
                                 AV_SECTION_CONTEXT_SIZE,
                                 PagedPool,
                                 &sectionContext );
    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                 ("[Av]: Failed to allocate section context.\n, 0x%08x\n",
                  status) );
        return status;
    }

    RtlZeroMemory(sectionContext, AV_SECTION_CONTEXT_SIZE);

    status = AvGetFileSize( Instance,
                            FileObject,
                            &fileSize );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[Av]: Failed to get file size with status 0x%x. (FileObject = %p, Instance = %p)\n",
                    status,
                    FileObject,
                    Instance ));
    } else {

        sectionContext->FileSize = fileSize;
    }

    *SectionContext = sectionContext;

    return STATUS_SUCCESS;
}

NTSTATUS
AvEnumerateInstances(
    _Outptr_result_buffer_(*NumberInstances) PFLT_INSTANCE **InstanceArray,
    _Out_ PULONG NumberInstances
    )
/*++

Routine Description:

    This routine returns all the instances available of this filter
    The caller is responsible for calling AvFreeInstances(...) to release the instance references.

Arguments:

    InstanceArray - This function will allocate the memory of the arrary containing the instances.

    NumberInstances - The number of instances in InstanceArray.

Return Value:

    Status

--*/
{
    PFLT_INSTANCE *instArray = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i = 0;
    ULONG instCnt = 0;
    ULONG newCount = 0;


    //
    //  Get a count of how many instances there are
    //

    status = FltEnumerateInstances( NULL,
                                    Globals.Filter,
                                    NULL,
                                    0,
                                    &instCnt );

    if (!NT_SUCCESS(status) &&
        (status != STATUS_BUFFER_TOO_SMALL)) {

        goto Cleanup;
    }

    //
    //  Get handles for all instances.  This will loop in case too many
    //  filters load between the time we got the count and the time
    //  we actually get the list.
    //

    for (;;) {

        //
        //  Free old memory if we have some
        //

        if (instArray != NULL) {

            ExFreePoolWithTag( instArray,  AV_INSTANCES_ARRAY_TAG);
            instArray = NULL;
        }

        //
        //  Allocate memory for list, add a couple of entries in case
        //  a filter loads while we are doing this
        //

        instCnt += 2;

        instArray = ExAllocatePoolZero( PagedPool,
                                        (instCnt * sizeof(PFLT_INSTANCE)),
                                        AV_INSTANCES_ARRAY_TAG );

        if (instArray == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        //  This time get list of filters (and a new count)
        //

        status = FltEnumerateInstances( NULL,
                                        Globals.Filter,
                                        instArray,
                                        instCnt,
                                        &newCount );

        //
        //  exit loop if we succeeded
        //

        if (NT_SUCCESS(status)) {

            instCnt = newCount;
            break;
        }

        //
        //  If it was an unexpected error, quit processing, else allocate
        //  more memory and try again
        //

        if (status != STATUS_BUFFER_TOO_SMALL) {

            goto Cleanup;
        }

        //
        //  The buffer was too small, try again
        //

        FLT_ASSERT(newCount > instCnt);
        instCnt = newCount;
    }

    *InstanceArray = instArray;
    *NumberInstances = instCnt;

Cleanup:

    if ( !NT_SUCCESS(status) ) {

        if (instArray) {

            //
            //  Release all the objects in the array
            //

            for (i = 0; i < instCnt; i++) {

                FltObjectDereference( instArray[i] );
                instArray[i] = NULL;
            }

            ExFreePoolWithTag( instArray, AV_INSTANCES_ARRAY_TAG );
            instArray = NULL;
        }
    }


    return status;

}

VOID
AvFreeInstances (
    _In_reads_(InstanceCount) PFLT_INSTANCE *InstanceArray,
    _In_ ULONG InstanceCount
    )
/*++

Routine Description:

    This routine frees the reference count and memory of instance array obtained from AvEnumerateInstances(...).

Arguments:

    InstanceArray -  The instance arrary to be freed.

    NumberInstances - The number of instances in InstanceArray.

Return Value:

    None.

--*/
{
    ULONG i = 0;

    //
    //  Release all the objects in the array
    //

    for (i = 0; i < InstanceCount; i++) {

        FltObjectDereference( InstanceArray[i] );
        InstanceArray[i] = NULL;
    }

    ExFreePoolWithTag( InstanceArray, AV_INSTANCES_ARRAY_TAG );
}

NTSTATUS
AvAllocateScanContext(
    _In_     PFLT_INSTANCE Instance,
    _In_     PFILE_OBJECT FileObject,
    _Outptr_ PAV_SCAN_CONTEXT *ScanContext
    )
/*++

Routine Description:

    The routine allocates the scan context

Arguments:

    Instance - Opaque instance pointer for the caller. This parameter is required and cannot be NULL.

    FileObject - File object pointer for the file. This parameter is required and cannot be NULL.

    ScanContext - The output scan context.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if allocation failed.
    STATUS_SUCCESS if successfully allocated.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PAV_SCAN_CONTEXT scanCtx = NULL;

    PAGED_CODE();

    ASSERT(Instance != NULL);
    ASSERT(FileObject != NULL);

    scanCtx = ExAllocatePoolZero( NonPagedPoolNx,
                                  sizeof(AV_SCAN_CONTEXT),
                                  AV_SCAN_CTX_TAG );

    if (NULL == scanCtx) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    scanCtx->RefCount = 1;
    scanCtx->IoWaitOnScanCompleteNotificationAborted = FALSE;

    //
    //  Keeps a reference in scan contex.
    //  We also handle the case that the instance is being torn down.
    //
    status = FltObjectReference( Instance );
    if (!NT_SUCCESS(status)) {

        ExFreePoolWithTag( scanCtx, AV_SCAN_CTX_TAG );
        return status;
    }
    scanCtx->FilterInstance = Instance;

    //
    //  Keeps a reference in scan context
    //
    ObReferenceObject( FileObject );
    scanCtx->FileObject = FileObject;

    *ScanContext = scanCtx;
    return STATUS_SUCCESS;
}

NTSTATUS
AvReferenceScanContext(
    _In_ PAV_SCAN_CONTEXT ScanContext
    )
/*++

Routine Description:

    The routine increments the reference count of scan context to prevent it from deletion.

Arguments:

    ScanContext - The scan context to be added reference.

Return Value:

    STATUS_INVALID_PARAMETER if ScanContext is NULL.
    STATUS_SUCCESS if successfully incremented.

--*/
{
    PAGED_CODE();

    if (ScanContext == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(ScanContext->RefCount != 0);
    ASSERT(ScanContext->FilterInstance != NULL);
    ASSERT(ScanContext->FileObject != NULL);

    InterlockedIncrement(&ScanContext->RefCount);

    return STATUS_SUCCESS;
}

NTSTATUS
AvReleaseScanContext(
    _In_ PAV_SCAN_CONTEXT ScanContext
    )
/*++

Routine Description:

    The routine decrements the reference count of scan context.
    Release it if reference count goes to zero.

Arguments:

    ScanContext - The scan context to be released.

Return Value:

    STATUS_INVALID_PARAMETER if ScanContext is NULL.
    STATUS_SUCCESS if successfully decremented.

--*/
{
    ULONG newRefCount = 0;

    PAGED_CODE();

    if (ScanContext == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(ScanContext->FilterInstance != NULL);
    ASSERT(ScanContext->FileObject != NULL);

    //
    //  Assume the usage of AvReferenceScanContext and AvReleaseScanContext are not raced,
    //  This simple version would suffice.
    //

    newRefCount = InterlockedDecrement(&ScanContext->RefCount);
    if (newRefCount == 0) {

        //
        //  Before freeing scan context, we need to release the file object and instance.
        //
        FltObjectDereference( ScanContext->FilterInstance );
        ObDereferenceObject( ScanContext->FileObject );
        ExFreePoolWithTag( ScanContext, AV_SCAN_CTX_TAG );
    }
    return STATUS_SUCCESS;
}



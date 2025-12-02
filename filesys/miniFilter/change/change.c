/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    change.c

Abstract:

    This is the main module of the change miniFilter driver.
    This transaction-aware filter monitors file changes in real time.
    Cg prefix denotes "Change" module.
    
    This module tracks if the files are dirty. In order to do this,
    we have to intercept the "write" I/O requests. In particular, 
    the operations are collected in CgOperationsNeedDirty(...) function.
    If you care about more than the contents of the file, you may need to 
    modify this function accordingly.
    
    This sample demonstrates how to track whether a file has been modified.
    
    In addition, this filter handles the case that the transaction 
    commits or rollbacks. The overview of a transaction-aware 
    minifilter is stated as follows
    
    1. At post create, if a transacted file is open with attribute 
    FILE_WRITE_DATA or FILE_APPEND_DATA, then we would enlist its 
    file context into the transaction context.
    
    2. At pre-operation callback, if the operation needs to be dirty, 
    such as IRP_MJ_WRITE and the file is part of a transaction, 
    we update its TxDirty instead of Dirty.
    
    3. At KTM notification callback, if the transaction committed, 
    then propagate the dirty information from TxDirty to Dirty; 
    if rollbacked, do not propagate.

    4. Properly remove the list at TransactionContextCleanup.

Environment:

    Kernel mode

--*/

#include "change.h"
    
/*************************************************************************
    Local Function Prototypes
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
CgInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
CgInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
CgInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
CgUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
CgInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
CgPreOperationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
CgPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
CgPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
CgPreClose (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
CgPreFsControl (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

NTSTATUS
CgKtmNotificationCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_CONTEXT TransactionContext,
    _In_ ULONG TransactionNotification
    );
    
//
//  Local routines
//

BOOLEAN
CgOperationsNeedDirty ( 
    _In_ PFLT_CALLBACK_DATA Data
    );

NTSTATUS
CgQueryTransactionOutcome(
    _In_ PKTRANSACTION Transaction,
    _Out_ PULONG TxOutcome
    );

NTSTATUS
CgProcessPreviousTransaction (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PCG_FILE_CONTEXT FileContext
    );
    
NTSTATUS 
CgProcessTransactionOutcome(
    _Inout_ PCG_TRANSACTION_CONTEXT TransactionContext,
    _In_ ULONG TransactionOutcome
    );
    

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, CgUnload)
#pragma alloc_text(PAGE, CgInstanceQueryTeardown)
#pragma alloc_text(PAGE, CgInstanceSetup)
#pragma alloc_text(PAGE, CgInstanceTeardownStart)
#pragma alloc_text(PAGE, CgInstanceTeardownComplete)
#pragma alloc_text(PAGE, CgInstanceTeardownComplete)
#pragma alloc_text(PAGE, CgPreCreate)
#pragma alloc_text(PAGE, CgPreFsControl)
#pragma alloc_text(PAGE, CgPostCreate)
#pragma alloc_text(PAGE, CgPreClose)
#pragma alloc_text(PAGE, CgKtmNotificationCallback)
#pragma alloc_text(PAGE, CgProcessPreviousTransaction)
#pragma alloc_text(PAGE, CgProcessTransactionOutcome)
#pragma alloc_text(PAGE, CgQueryTransactionOutcome)
#endif
    
    
//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      CgPreCreate,
      CgPostCreate },

    { IRP_MJ_CLOSE,
      0,
      CgPreClose,
      NULL },

    { IRP_MJ_WRITE,
      0,
      CgPreOperationCallback,
      NULL },
      
    { IRP_MJ_SET_INFORMATION,
      0,
      CgPreOperationCallback,
      NULL },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      CgPreFsControl,
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

    CgUnload,                           //  MiniFilterUnload

    CgInstanceSetup,                    //  InstanceSetup
    CgInstanceQueryTeardown,            //  InstanceQueryTeardown
    CgInstanceTeardownStart,            //  InstanceTeardownStart
    CgInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL,                               //  NormalizeNameComponent
    CgKtmNotificationCallback           //  KTM notification callback

};



NTSTATUS
CgInstanceSetup (
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
    instances are always created.

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

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}


NTSTATUS
CgInstanceQueryTeardown (
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

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
CgInstanceTeardownStart (
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

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgInstanceTeardownStart: Entered\n") );
}


VOID
CgInstanceTeardownComplete (
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

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgInstanceTeardownComplete: Entered\n") );
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
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterInstance );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterInstance );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterInstance );
        }
    }

    return status;
}

NTSTATUS
CgUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
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
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgUnload: Entered\n") );

    
    FltUnregisterFilter( gFilterInstance );
    gFilterInstance = NULL;

    return STATUS_SUCCESS;
}


/*************************************************************************
    Local utility routines.
*************************************************************************/

BOOLEAN
CgOperationsNeedDirty ( 
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we need to set the file to be dirty.
    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

Return Value:

    TRUE - If we want the file associated with the request to be dirty.
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    
    //
    //  In this example, we only care about the "contents" of the file.
    //  The dirty concept depends on what you care about. If you care 
    //  about the file metadata, for example, then you should have to add 
    //  the operations that modify the file metadata as well.
    //
    
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
CgQueryTransactionOutcome(
    _In_ PKTRANSACTION Transaction,
    _Out_ PULONG TxOutcome
    )
/*++

Routine Description:

    This is a helper function that query the KTM how transaction was ended.
    
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
    
        CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                  ("[CG] CgQueryTransactionOutcome: ObOpenObjectByPointer failed.\n") );
        return status;
    }

    status = ZwQueryInformationTransaction( transactionHandle,
                                            TransactionBasicInformation,
                                            &txBasicInfo,
                                            sizeof(TRANSACTION_BASIC_INFORMATION),
                                            NULL );
    if (!NT_SUCCESS(status)) {
            
        CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                  ("[CG] CgQueryTransactionOutcome: ObOpenObjectByPointer failed.\n") );
        goto Cleanup;
    }
    
    *TxOutcome = txBasicInfo.Outcome;
                                   
Cleanup:

    ZwClose(transactionHandle);
    
    return status;
}

FORCEINLINE
VOID
CgPropagateDirty(
    _Inout_ PCG_FILE_CONTEXT FileContext,
    _In_ ULONG TransactionOutcome
    )
{
    
    if (TransactionOutcome == TransactionOutcomeCommitted) {

        //
        //  The 'or' operator here handles the case below:
        //
        //  It is possible that the user only read the file even if it opens the file transacted file read and write.
        //  So, fileContext->TxDirty is possible to be FALSE.
        //  
        //  Since KTM callback is asynchrounous, notifications are not necessarily received in order.
        //  It is possible that dirty information will be wiped out if we use 
        //
        //      fileContext->Dirty = fileContext->TxDirty;
        //
        
        FileContext->Dirty |= FileContext->TxDirty;
    }
    
    //
    //  Clear TxDirty regardless of transaction outcome.
    //
    
    FileContext->TxDirty = FALSE;
}

NTSTATUS
CgProcessPreviousTransaction (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PCG_FILE_CONTEXT FileContext
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

    FileContext - The file context.

Return Value:

    The return value is the status of the operation.

--*/
{
    ULONG txOutcome = TransactionOutcomeUndetermined;
    NTSTATUS status = STATUS_SUCCESS;
    PCG_TRANSACTION_CONTEXT oldTxCtx = NULL;
    PCG_TRANSACTION_CONTEXT transactionContext = NULL;
    
    PAGED_CODE();

    if (FltObjects->Transaction != NULL) { 
    
        //
        //  Get transaction context
        //

        status = CgFindOrCreateTransactionContext( FltObjects,
                                                   &transactionContext );
                                            
        if (!NT_SUCCESS( status )) {
        
            CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                  ("[CG] CgProcessPreviousTransaction: CgFindOrCreateTransactionContext FAILED\n") );
            transactionContext = NULL;
            goto Cleanup;
        }
        
        //
        //  Enlist it if haven't.
        //
        
        if (!transactionContext->Enlisted) {

            status = FltEnlistInTransaction( FltObjects->Instance,
                                             FltObjects->Transaction,
                                             transactionContext,
                                             TRANSACTION_NOTIFY_COMMIT_FINALIZE | TRANSACTION_NOTIFY_ROLLBACK  );
                                         
            if (!NT_SUCCESS( status ) && (status != STATUS_FLT_ALREADY_ENLISTED) ) {
            
                CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                      ("[CG] CgProcessPreviousTransaction: FltEnlistInTransaction FAILED!!!!\n") );
                goto Cleanup;
            }
            
            status = STATUS_SUCCESS;
            transactionContext->Enlisted = TRUE;
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
    //  Synchronize the replacement of FileContext->TxContext with KTM callback.
    //
        
    oldTxCtx = InterlockedExchangePointer( &FileContext->TxContext, transactionContext );
    
    if (oldTxCtx != transactionContext) {  // case 1,2,3
    
            if ( oldTxCtx == NULL ) { // case 1
            
                //
                //  Since we exchanged the pointer, we need to increment the referece count
                //
                
                FltReferenceContext ( transactionContext );
                
                //
                //  Before insertion into the FcList in transaction context, we increment file context's ref count
                //
                
                ExAcquireFastMutex( transactionContext->Mutex );
                
                if (!transactionContext->ListDrained) {
                
                    FltReferenceContext ( FileContext );   // Q
                    InsertTailList( &transactionContext->ScListHead,
                                    &FileContext->ListInTransaction );
                }
                
                ExReleaseFastMutex( transactionContext->Mutex );
                
                goto Cleanup;
            }
    
            // case 2,3
        
            //
            //  There can only be one transacted writer for the file so the previous
            //  transaction must have finished. Whether or not the TxDirty state is propagated
            //  depends on its outcome so query that now.
            //
            
            status = CgQueryTransactionOutcome( oldTxCtx->Transaction, &txOutcome );
            
            if (!NT_SUCCESS( status )) {
                
                //
                //  We have exchanged the transaction pointer already. If we can't get the outcome, 
                //  we have to proceed anyway.
                //
                
                CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                        ("[CG] CgProcessPreviousTransaction: CgQueryTransactionOutcome FAILED!!!!\n") );
            }
            
            //
            //  Remove the file context from the original transaction context.
            //
            
            ExAcquireFastMutex( oldTxCtx->Mutex );
            RemoveEntryList ( &FileContext->ListInTransaction );
            ExReleaseFastMutex( oldTxCtx->Mutex );
            
            CgPropagateDirty ( FileContext, txOutcome );
            
            if ( transactionContext ) {  // case 3
                
                FltReferenceContext( transactionContext );
                
                ExAcquireFastMutex( transactionContext->Mutex );

                if (!transactionContext->ListDrained) {
                
                    InsertTailList( &transactionContext->ScListHead,
                                    &FileContext->ListInTransaction );
                                    
                } else {
                    
                    FltReleaseContext( FileContext );
                }
                
                ExReleaseFastMutex( transactionContext->Mutex );
                
            } else { // case 2
            
                FltReleaseContext ( FileContext );  // Release reference count at Q
            }
            
            // case 2,3
            
            FltReleaseContext( oldTxCtx );  // Release reference count in file context originally.
        
    } 
    //
    //  We don't care about case 4, 5.
    //
    
Cleanup:

    if (transactionContext) {
        
        FltReleaseContext( transactionContext );  // Release the ref count grabbed at CgFindOrCreateTransactionContext(...)
    }

    return status;
}

NTSTATUS 
CgProcessTransactionOutcome(
    _Inout_ PCG_TRANSACTION_CONTEXT TransactionContext,
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
    PCG_FILE_CONTEXT fileContext = NULL;
    PCG_TRANSACTION_CONTEXT oldTxCtx = NULL;

    PAGED_CODE();
    
    //
    //  Tranversing the file context list, and
    //  sync the TxDirty -> Dirty.
    //
    //  Either commit or rollback, we need to cleanup the list
    //  Tear down file context list inside transactionContext
    //
    
    ExAcquireFastMutex( TransactionContext->Mutex );
    
    LIST_FOR_EACH_SAFE( scan, next, &TransactionContext->ScListHead ) {
    
        fileContext = CONTAINING_RECORD( scan, CG_FILE_CONTEXT, ListInTransaction );
        oldTxCtx = InterlockedCompareExchangePointer( &fileContext->TxContext, NULL, TransactionContext );
        if (oldTxCtx == TransactionContext) {
        
            //
            //  When oldTxCtx and TransactionContext are equal, it means that 
            //  fileContext->TxContext has been successfully set to NULL.
            //

            RemoveEntryList ( scan );
            CgPropagateDirty( fileContext, TransactionOutcome );
            FltReleaseContext( oldTxCtx );
            
            //
            //  This sample demonstrates how we propagate TxDirty to Dirty.
            //  If the file becomes dirty, we print out here.
            //
            
            if (fileContext->Dirty) {
            
                CG_DBG_PRINT( CGDBG_TRACE_DEBUG,
                                ("[CG] CgProcessTransactionOutcome: Transacted file ID %I64x,%I64x is dirty\n",
                                 fileContext->FileID.FileId64.UpperZeroes,
                                 fileContext->FileID.FileId64.Value) );
            }
            
            FltReleaseContext( fileContext );
        }
    }
    TransactionContext->ListDrained = TRUE;
    ExReleaseFastMutex( TransactionContext->Mutex );
    

    
    return STATUS_SUCCESS;
}

/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/

FLT_PREOP_CALLBACK_STATUS
CgPreOperationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the registered callback routine for filtering 
    the "write" operation, i.e. the operations that have potential 
    to make the file dirty.
    
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
    PCG_FILE_CONTEXT fileContext = NULL;
    
    UNREFERENCED_PARAMETER( CompletionContext );

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgPreOperationCallback: Entered\n") );

    if (!CgOperationsNeedDirty(Data)) {

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    status = FltGetFileContext( FltObjects->Instance,
                                FltObjects->FileObject,
                                &fileContext );
    
    if (!NT_SUCCESS( status )) {
    
        CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
              ("[CG] CgPreOperationCallback: get file context failed. rq: %d\n", 
                Data->Iopb->MajorFunction) );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  If this operation is performed in a transacted writer view.
    //
    
    if (fileContext->TxContext != NULL) {

#if DBG
        PCG_TRANSACTION_CONTEXT transactionContext = NULL;

        NTSTATUS statusTx = FltGetTransactionContext( FltObjects->Instance,
                                                      FltObjects->Transaction,
                                                      &transactionContext );

        FLT_ASSERTMSG( "Transaction context should not fail, because it is supposed to be created at post create.\n", NT_SUCCESS( statusTx ));
        FLT_ASSERTMSG( "The file's TxCtx should be identical with the target TxCtx.\n", 
                       fileContext->TxContext == transactionContext);
        
        if (NT_SUCCESS( statusTx )) {
            FltReleaseContext( transactionContext );
        }

#endif // DBG
        
        //
        //  Instead of updating Dirty, we update TxDirty here, 
        //  because this modification is occurring in the context of transaction 
        //  so if the transaction rolls back then the file will not be set as 
        //  Dirty, i.e. the dirty will not be propagated from TxDirty to Dity.
        //  This is why we have TxDirty here.
        //
        
        fileContext->TxDirty = TRUE;
        
    } else {
        
        fileContext->Dirty = TRUE;
    }
    
    FltReleaseContext( fileContext );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
CgPreFsControl (
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
        //  We explicitly fail the request of save point here.
        //
        
        Data->IoStatus.Status = STATUS_NOT_SUPPORTED;
        return FLT_PREOP_COMPLETE;
    }
    return CgPreOperationCallback(Data, FltObjects, CompletionContext);
}


FLT_PREOP_CALLBACK_STATUS
CgPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the pre-create completion routine.
    In this routine, file context and/or transaction context shall be
    created if not exits.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACK or 
        FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
        the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

    FLT_PREOP_SYNCHRONIZE

--*/
{

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    
    PAGED_CODE();

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgPreOperationCallback: Entered\n") );

    //
    //  Return FLT_PREOP_SYNCHRONIZE at PreCreate due to 
    //  some callback of PostCreate may be at DPC level.
    //  eResource is required at level < DPC.
    //
    
    return FLT_PREOP_SYNCHRONIZE;

}

FLT_POSTOP_CALLBACK_STATUS
CgPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-create completion routine.
    In this routine, file context and/or transaction context shall be
    created if not exits.

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
    PCG_FILE_CONTEXT fileContext = NULL;
    
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );
    
    PAGED_CODE();
    
    if (!NT_SUCCESS( status ) || 
        (status == STATUS_REPARSE)) {
        
        //
        //  File creation may fail.
        //

        CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgPostCreate: file creation failed\n") );

        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    
    //
    //  Find or create a file context
    //

    status = CgFindOrCreateFileContext( Data, 
                                        &fileContext );
    
    if (!NT_SUCCESS( status )) {
    
        //
        //  In this filter sample, if creation or retrieval of the contexts fails, 
        //  we let the creation go through because this example focuses on being 
        //  an non-intrusive filter. However, if tracking is critical for your 
        //  filter, then you should fail the create via FltCancelFileOpen(...).
        //
        
        CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
              ("[CG] CgPostCreate: find file context failed. \n") );

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    //
    //  If successfully opened a file with the desired access matching
    //  the "exclusive write" from a TxF point of view, we can know that 
    //  if previous transaction context exists, it must have been comitted 
    //  or rollbacked.
    //
    
    if (FlagOn( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess,
                FILE_WRITE_DATA | FILE_APPEND_DATA |
                DELETE | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
                WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY ) ) {
        
        status = CgProcessPreviousTransaction ( FltObjects,
                                                fileContext );
        if (!NT_SUCCESS( status )) {
                                         
            CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                  ("[CG] CgPostCreate: CgProcessTransaction FAILED!! \n") );

            goto Cleanup;
        }
    }
    
    
Cleanup:
    
    FltReleaseContext( fileContext );
    
    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
CgPreClose (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre-close callback. Make the file context persistent in the volatile cache.
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
    PCG_FILE_CONTEXT fileContext = NULL;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    status = FltGetFileContext( FltObjects->Instance,
                                FltObjects->FileObject,
                                &fileContext );
        
    if (!NT_SUCCESS( status )) {
    
        CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
              ("[CG] CgPreClose: find file context failed.\n") );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    //
    //  For non-transacted files,
    //  we just print out the file is dirty or not
    //
 
    if ((FltObjects->Transaction == NULL) &&
        fileContext->Dirty) {
    
        CG_DBG_PRINT( CGDBG_TRACE_DEBUG,
                ("[CG] CgPreClose: Non-transacted file ID %I64x,%I64x is dirty\n",
                 fileContext->FileID.FileId64.UpperZeroes,
                 fileContext->FileID.FileId64.Value) );
    }

    FltReleaseContext( fileContext );

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS
CgKtmNotificationCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
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
        
--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    
    PAGED_CODE();
    
    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                  ("[CG] CgKtmNotificationCallback: Entered\n") );
                  
    FLT_ASSERTMSG("[CG] CgKtmNotificationCallback: The expected type of notifications registered at FltEnlistInTransaction(...).\n",
                  FlagOn( TransactionNotification,
                         (TRANSACTION_NOTIFY_COMMIT_FINALIZE | TRANSACTION_NOTIFY_ROLLBACK) ) );
                  
    if (NULL != TransactionContext) {
    
        if ( FlagOn( TransactionNotification, TRANSACTION_NOTIFY_COMMIT_FINALIZE ) ) {
        
            return CgProcessTransactionOutcome( TransactionContext, TransactionOutcomeCommitted );
            
        } else {
        
            return CgProcessTransactionOutcome( TransactionContext, TransactionOutcomeAborted );
        }
    }
    
    return STATUS_SUCCESS;
}


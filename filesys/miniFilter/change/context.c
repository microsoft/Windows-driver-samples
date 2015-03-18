/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

Module Name:

    context.c

Abstract:

    Filter Context-related module implementation.
	
Environment:

    Kernel mode

--*/

#include "change.h"

//
//  Local function prototypes.
//

NTSTATUS
CgCreateFileContext (
    _Outptr_ PCG_FILE_CONTEXT *FileContext
    );

VOID
CgFileContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );

VOID
CgTransactionContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    );
    

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CgCreateFileContext)
#pragma alloc_text(PAGE, CgFindOrCreateFileContext)
#pragma alloc_text(PAGE, CgFindOrCreateTransactionContext)
#pragma alloc_text(PAGE, CgFileContextCleanup)
#pragma alloc_text(PAGE, CgTransactionContextCleanup)
#endif
	
//
//  Context registration structure
//

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_FILE_CONTEXT,
      0,
      CgFileContextCleanup,
      CG_FILE_CONTEXT_SIZE,
      CG_FILE_CONTEXT_TAG },
      
    { FLT_TRANSACTION_CONTEXT,
      0,
      CgTransactionContextCleanup,
      CG_TRANSACTION_CONTEXT_SIZE,
      CG_TRANSACTION_CONTEXT_TAG },

    { FLT_CONTEXT_END }
};


VOID
CgFileContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine is called whenever the file context is about to be destroyed.
    Typically we need to clean the data structure inside it.

Arguments:

    Context - Pointer to the PCG_FILE_CONTEXT data structure.

    ContextType - This value should be FLT_FILE_CONTEXT.

Return Value:

    None

--*/
{
    PCG_FILE_CONTEXT fileContext = NULL;
    
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER( ContextType );
 
    fileContext = (PCG_FILE_CONTEXT) Context;
    
    FLT_ASSERTMSG( "[CG]: File context is not supposed to be in the transaction context list at cleanup.!\n", 
                   NULL == fileContext->TxContext );

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                ("[CG]: Cleaning up file context for file ID %I64x,%I64x (FileContext = %p), dirty = %d\n",
                 fileContext->FileID.FileId64.UpperZeroes,
                 fileContext->FileID.FileId64.Value,
                 fileContext,
                 fileContext->Dirty) );

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                ("[CG]: File context cleanup complete.\n") );


}

VOID
CgTransactionContextCleanup (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine is called whenever the file context is about to be destroyed.
    Typically we need to clean the data structure inside it.

Arguments:

    Context - Pointer to the PCG_TRANSACTION_CONTEXT data structure.

    ContextType - This value should be FLT_TRANSACTION_CONTEXT.

Return Value:

    None

--*/
{
    PCG_TRANSACTION_CONTEXT transactionContext = (PCG_TRANSACTION_CONTEXT) Context;
    
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER( ContextType );
        
    CG_DBG_PRINT( CGDBG_TRACE_DEBUG,
                    ("[CG]: CgTransactionContextCleanup context cleanup entered.\n") );
    
    CgFreeMutex( transactionContext->Mutex );
    transactionContext->Mutex = NULL;
    ObDereferenceObject( transactionContext->Transaction );
    transactionContext->Transaction = NULL;
}

NTSTATUS
CgGetFileId (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_ PCG_FILE_REFERENCE FileId
    )
/*++

Routine Description:

    This routine gets the File ID, given a file object.  It deals with both,
    the 128-bit (ReFS) and 64-bits FileIDs.

Arguments:

    Instance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.
    
    FileObject - File object pointer for the file. This parameter is required and cannot be NULL.

    FileId - Pointer to file id. This is the output

Return Value:

    Returns status forwarded from FltQueryInformationFile.
    On success, FileId will hold the FileID for the file.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    FLT_FILESYSTEM_TYPE type;

    //
    //  Query for what type of filesystem we are sitting on top of.
    //

    status = FltGetFileSystemType( Instance, &type );

    if (NT_SUCCESS( status )) {

        if (type == FLT_FSTYPE_REFS) {

            FILE_ID_INFORMATION fileIdInformation;

            status = FltQueryInformationFile( Instance,
                                              FileObject,
                                              &fileIdInformation,
                                              sizeof(FILE_ID_INFORMATION),
                                              FileIdInformation,
                                              NULL );

            if (NT_SUCCESS( status )) {

                RtlCopyMemory(&FileId->FileId128, &fileIdInformation.FileId, sizeof(FileId->FileId128) );
            }

        } else {

            FILE_INTERNAL_INFORMATION fileInternalInformation;

            status = FltQueryInformationFile( Instance,
                                              FileObject,
                                              &fileInternalInformation,
                                              sizeof(FILE_INTERNAL_INFORMATION),
                                              FileInternalInformation,
                                              NULL );

            if (NT_SUCCESS( status )) {

                FileId->FileId64.Value = fileInternalInformation.IndexNumber.QuadPart;
                FileId->FileId64.UpperZeroes = 0LL;
            }
        }
    }

    return status;
}

NTSTATUS
CgFindOrCreateFileContext (
    _In_ PFLT_CALLBACK_DATA Cbd,
    _Outptr_ PCG_FILE_CONTEXT *FileContext
    )
/*++

Routine Description:

    This routine finds the file context for the target file.
    If the context does not exist this routing creates
    a new one and attaches the context to the file.

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    FileContext           - Returns the file context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCG_FILE_CONTEXT fileContext;
    PCG_FILE_CONTEXT oldFileContext;
    
    PAGED_CODE();

    *FileContext = NULL;

    //
    //  First try to get the file context.
    //

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                ("[CG]: Trying to get file context (FileObject = %p, Instance = %p, rq = %d)\n",
                 Cbd->Iopb->TargetFileObject,
                 Cbd->Iopb->TargetInstance,
                 Cbd->Iopb->MajorFunction) );

    status = FltGetFileContext( Cbd->Iopb->TargetInstance,
                                Cbd->Iopb->TargetFileObject,
                                &fileContext );

    //
    //  If the call failed because the context does not exist
    //  and the user wants to creat a new one, then create a
    //  new context
    //

    if (status == STATUS_NOT_FOUND) {

        CG_FILE_REFERENCE fileID;

        status = CgGetFileId( Cbd->Iopb->TargetInstance,
                              Cbd->Iopb->TargetFileObject,
                              &fileID );
        
        if (!NT_SUCCESS( status )) {

            CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                        ("[CG]: Failed to get file id with status 0x%x. (FileObject = %p, Instance = %p, rq = %d)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance,
                        Cbd->Iopb->MajorFunction) );

            return status;
            
        }

        //
        //  Create a file context
        //
        
        status = CgCreateFileContext( &fileContext );

        if (!NT_SUCCESS( status )) {

            CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                        ("[CG]: Failed to create file context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance) );

            return status;
        }

        //
        //  Initiailize fileContext
        //
        
        RtlCopyMemory( &fileContext->FileID, &fileID, sizeof(fileContext->FileID) ); 

        //
        //  Set the new context we just allocated on the file object
        //

        CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                    ("[CG]: Setting file context %p (FileObject = %p, Instance = %p)\n",
                     fileContext,
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        status = FltSetFileContext( Cbd->Iopb->TargetInstance,
                                    Cbd->Iopb->TargetFileObject,
                                    FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                    fileContext,
                                    &oldFileContext );

        if (!NT_SUCCESS( status )) {

            CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                        ("[CG]: Failed to set file context with status 0x%x. (FileObject = %p, Instance = %p, rq = %d)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance,
                        Cbd->Iopb->MajorFunction) );
            //
            //  We release the context here because FltSetFileContext failed
            //
            //  If FltSetFileContext succeeded then the context will be returned
            //  to the caller. The caller will use the context and then release it
            //  when he is done with the context.
            //

            CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                        ("[CG]: Releasing file context %p (FileObject = %p, Instance = %p)\n",
                         fileContext,
                         Cbd->Iopb->TargetFileObject,
                         Cbd->Iopb->TargetInstance) );

            FltReleaseContext( fileContext );

            if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

                //
                //  FltSetFileContext failed for a reason other than the context already
                //  existing on the file. So the object now does not have any context set
                //  on it. So we return failure to the caller.
                //

                CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                            ("[CG]: Failed to set file context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
                            status,
                            Cbd->Iopb->TargetFileObject,
                            Cbd->Iopb->TargetInstance) );

                return status;
            }

            //
            //  Race condition. Someone has set a context after we queried it.
            //  Use the already set context instead
            //

            CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                        ("[CG]: Race: File context already defined. Retaining old file context %p (FileObject = %p, Instance = %p)\n",
                         oldFileContext,
                         Cbd->Iopb->TargetFileObject,
                         Cbd->Iopb->TargetInstance) );

            //
            //  Return the existing context. Note that the new context that we allocated has already been
            //  realeased above.
            //

            fileContext = oldFileContext;
            status = STATUS_SUCCESS;

        }
    }
    
    *FileContext = fileContext;

    return status;
}


NTSTATUS
CgCreateFileContext (
    _Outptr_ PCG_FILE_CONTEXT *FileContext
    )
/*++

Routine Description:

    This routine creates a new file context

Arguments:

    FileContext         - Returns the file context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCG_FILE_CONTEXT fileContext;
    
    PAGED_CODE();
    
    //
    //  Allocate a file context
    //

    CG_DBG_PRINT( CGDBG_TRACE_ROUTINES,
                ("[CG]: Allocating file context \n") );

    status = FltAllocateContext( gFilterInstance,
                                 FLT_FILE_CONTEXT,
                                 CG_FILE_CONTEXT_SIZE,
                                 PagedPool,
                                 &fileContext );

    if (!NT_SUCCESS( status )) {

        CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                    ("[CG]: Failed to allocate file context with status 0x%x \n",
                     status) );
        return status;
    }

    //
    //  Initialize the newly created context
    //
    
    RtlZeroMemory(fileContext, CG_FILE_CONTEXT_SIZE);
    *FileContext = fileContext;

    return STATUS_SUCCESS;
}

NTSTATUS
CgFindOrCreateTransactionContext(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_ PCG_TRANSACTION_CONTEXT *TransactionContext
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
    PCG_TRANSACTION_CONTEXT transactionContext = NULL;
    PCG_TRANSACTION_CONTEXT oldTransactionContext = NULL;
    PFAST_MUTEX pFastMutex = NULL;
    
    PAGED_CODE();
    
    CG_DBG_PRINT( CGDBG_TRACE_DEBUG,
                ("[CG]: CgFindOrCreateTransactionContext entered. \n") );
                
    FLT_ASSERTMSG( "[CG]: Transaction object pointer is not supposed to be NULL !\n", FltObjects->Transaction != NULL);
    
    status = FltGetTransactionContext( FltObjects->Instance,
                                       FltObjects->Transaction,
                                       &transactionContext );
        
    if (NT_SUCCESS( status )) {
    
        *TransactionContext = transactionContext;
        return STATUS_SUCCESS;
    }
    
    if (status != STATUS_NOT_FOUND) {
        
        CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                    ("[CG]: Failed to get transaction context with status 0x%x \n",
                     status) );
        return status;
    }
    
    //
    //  Allocate the resource
    //
    
    pFastMutex = CgAllocateMutex();
    
    if ( NULL == pFastMutex ) {
    
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    //  Allocate a transaction context.
    //

    status = FltAllocateContext( gFilterInstance,
                                 FLT_TRANSACTION_CONTEXT,
                                 CG_TRANSACTION_CONTEXT_SIZE,
                                 PagedPool,
                                 &transactionContext );

    if (!NT_SUCCESS( status )) {

        CG_DBG_PRINT( CGDBG_TRACE_ERROR,
                ("[CG]: Failed to allocate transaction context with status 0x%x \n",
                 status) );

        CgFreeMutex( pFastMutex );
        return status;
    }
    
    FLT_ASSERTMSG( "[CG]: Transaction object pointer is not supposed to be NULL !\n", FltObjects->Transaction != NULL);
    
    //
    //  Initialization of transaction context.
    //  The reason we allocate eResource seperately is because 
    //  eResource has to be allocated in the non-paged pool.
    //
    
    RtlZeroMemory(transactionContext, CG_TRANSACTION_CONTEXT_SIZE);
    transactionContext->Mutex = pFastMutex;
    ObReferenceObject( FltObjects->Transaction );
    transactionContext->Transaction = FltObjects->Transaction;
    InitializeListHead( &transactionContext->ScListHead );
    ExInitializeFastMutex( transactionContext->Mutex );
    
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

        CG_DBG_PRINT( CGDBG_TRACE_ERROR,
            ("[CG]: Failed to set transaction context with status 0x%x \n",
             status) );

        return status;
    }
    
    FLT_ASSERTMSG( "[CG]: if FltSetTransactionContext returns STATUS_FLT_CONTEXT_ALREADY_DEFINED, the pointer should not be NULL.\n",
                   oldTransactionContext != NULL);

    *TransactionContext = oldTransactionContext;


    return STATUS_SUCCESS;
}



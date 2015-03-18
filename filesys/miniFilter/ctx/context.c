/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This is the stream nd stream handle context module of the kernel mode
    context sample filter driver


Environment:

    Kernel mode


--*/


#include "pch.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CtxFindOrCreateFileContext)
#pragma alloc_text(PAGE, CtxCreateFileContext)
#pragma alloc_text(PAGE, CtxFindOrCreateStreamContext)
#pragma alloc_text(PAGE, CtxCreateStreamContext)
#pragma alloc_text(PAGE, CtxUpdateNameInStreamContext)
#pragma alloc_text(PAGE, CtxCreateOrReplaceStreamHandleContext)
#pragma alloc_text(PAGE, CtxCreateStreamHandleContext)
#pragma alloc_text(PAGE, CtxUpdateNameInStreamHandleContext)
#endif




NTSTATUS
CtxFindOrCreateFileContext (
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_ BOOLEAN CreateIfNotFound,
   _When_( CreateIfNotFound != FALSE, _In_ ) _When_( CreateIfNotFound == FALSE, _In_opt_ ) PUNICODE_STRING FileName,
    _Outptr_ PCTX_FILE_CONTEXT *FileContext,
    _Out_opt_ PBOOLEAN ContextCreated
    )
/*++

Routine Description:

    This routine finds the file context for the target file.
    Optionally, if the context does not exist this routing creates
    a new one and attaches the context to the file.

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    CreateIfNotFound      - Supplies if the file context must be created if missing
    FileName              - Supplies the file name
    FileContext           - Returns the file context
    ContextCreated        - Returns if a new context was created

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCTX_FILE_CONTEXT fileContext;
    PCTX_FILE_CONTEXT oldFileContext;

    PAGED_CODE();

    *FileContext = NULL;
    if (ContextCreated != NULL) *ContextCreated = FALSE;

    //
    //  First try to get the file context.
    //

    DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                ("[Ctx]: Trying to get file context (FileObject = %p, Instance = %p)\n",
                 Cbd->Iopb->TargetFileObject,
                 Cbd->Iopb->TargetInstance) );

    status = FltGetFileContext( Cbd->Iopb->TargetInstance,
                                  Cbd->Iopb->TargetFileObject,
                                  &fileContext );

    //
    //  If the call failed because the context does not exist
    //  and the user wants to creat a new one, the create a
    //  new context
    //

    if (!NT_SUCCESS( status ) &&
        (status == STATUS_NOT_FOUND) &&
        CreateIfNotFound) {


        //
        //  Create a file context
        //

        DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Creating file context (FileObject = %p, Instance = %p)\n",
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        status = CtxCreateFileContext( FileName, &fileContext );

        if (!NT_SUCCESS( status )) {

            DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                        ("[Ctx]: Failed to create file context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance) );

            return status;
        }


        //
        //  Set the new context we just allocated on the file object
        //

        DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Setting file context %p (FileObject = %p, Instance = %p)\n",
                     fileContext,
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        status = FltSetFileContext( Cbd->Iopb->TargetInstance,
                                      Cbd->Iopb->TargetFileObject,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      fileContext,
                                      &oldFileContext );

        if (!NT_SUCCESS( status )) {

            DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                        ("[Ctx]: Failed to set file context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance) );
            //
            //  We release the context here because FltSetFileContext failed
            //
            //  If FltSetFileContext succeeded then the context will be returned
            //  to the caller. The caller will use the context and then release it
            //  when he is done with the context.
            //

            DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                        ("[Ctx]: Releasing file context %p (FileObject = %p, Instance = %p)\n",
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

                DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                            ("[Ctx]: Failed to set file context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
                            status,
                            Cbd->Iopb->TargetFileObject,
                            Cbd->Iopb->TargetInstance) );

                return status;
            }

            //
            //  Race condition. Someone has set a context after we queried it.
            //  Use the already set context instead
            //

            DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                        ("[Ctx]: File context already defined. Retaining old file context %p (FileObject = %p, Instance = %p)\n",
                         oldFileContext,
                         Cbd->Iopb->TargetFileObject,
                         Cbd->Iopb->TargetInstance) );

            //
            //  Return the existing context. Note that the new context that we allocated has already been
            //  realeased above.
            //

            fileContext = oldFileContext;
            status = STATUS_SUCCESS;

        } else {

            if (ContextCreated != NULL) *ContextCreated = TRUE;
        }
    }

    *FileContext = fileContext;

    return status;
}


NTSTATUS
CtxCreateFileContext (
    _In_ PUNICODE_STRING FileName,
    _Outptr_ PCTX_FILE_CONTEXT *FileContext
    )
/*++

Routine Description:

    This routine creates a new file context

Arguments:

    FileName            - Supplies the file name
    FileContext         - Returns the file context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCTX_FILE_CONTEXT fileContext;

    PAGED_CODE();

    //
    //  Allocate a file context
    //

    DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                ("[Ctx]: Allocating file context \n") );

    status = FltAllocateContext( Globals.Filter,
                                 FLT_FILE_CONTEXT,
                                 CTX_FILE_CONTEXT_SIZE,
                                 PagedPool,
                                 &fileContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to allocate file context with status 0x%x \n",
                     status) );
        return status;
    }

    //
    //  Initialize the newly created context
    //

    //
    //  Allocate and copy off the file name
    //

    fileContext->FileName.MaximumLength = FileName->Length;
    status = CtxAllocateUnicodeString( &fileContext->FileName );
    if (NT_SUCCESS( status )) {

        RtlCopyUnicodeString( &fileContext->FileName, FileName );
    }

    *FileContext = fileContext;

    return STATUS_SUCCESS;
}


NTSTATUS
CtxFindOrCreateStreamContext (
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_ BOOLEAN CreateIfNotFound,
    _Outptr_ PCTX_STREAM_CONTEXT *StreamContext,
    _Out_opt_ PBOOLEAN ContextCreated
    )
/*++

Routine Description:

    This routine finds the stream context for the target stream.
    Optionally, if the context does not exist this routing creates
    a new one and attaches the context to the stream.

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    CreateIfNotFound      - Supplies if the stream must be created if missing
    StreamContext         - Returns the stream context
    ContextCreated        - Returns if a new context was created

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCTX_STREAM_CONTEXT streamContext;
    PCTX_STREAM_CONTEXT oldStreamContext;

    PAGED_CODE();

    *StreamContext = NULL;
    if (ContextCreated != NULL) *ContextCreated = FALSE;

    //
    //  First try to get the stream context.
    //

    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: Trying to get stream context (FileObject = %p, Instance = %p)\n",
                 Cbd->Iopb->TargetFileObject,
                 Cbd->Iopb->TargetInstance) );

    status = FltGetStreamContext( Cbd->Iopb->TargetInstance,
                                  Cbd->Iopb->TargetFileObject,
                                  &streamContext );

    //
    //  If the call failed because the context does not exist
    //  and the user wants to creat a new one, the create a
    //  new context
    //

    if (!NT_SUCCESS( status ) &&
        (status == STATUS_NOT_FOUND) &&
        CreateIfNotFound) {


        //
        //  Create a stream context
        //

        DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: Creating stream context (FileObject = %p, Instance = %p)\n",
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        status = CtxCreateStreamContext( &streamContext );

        if (!NT_SUCCESS( status )) {

            DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                        ("[Ctx]: Failed to create stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance) );

            return status;
        }


        //
        //  Set the new context we just allocated on the file object
        //

        DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: Setting stream context %p (FileObject = %p, Instance = %p)\n",
                     streamContext,
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        status = FltSetStreamContext( Cbd->Iopb->TargetInstance,
                                      Cbd->Iopb->TargetFileObject,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      streamContext,
                                      &oldStreamContext );

        if (!NT_SUCCESS( status )) {

            DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                        ("[Ctx]: Failed to set stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance) );
            //
            //  We release the context here because FltSetStreamContext failed
            //
            //  If FltSetStreamContext succeeded then the context will be returned
            //  to the caller. The caller will use the context and then release it
            //  when he is done with the context.
            //

            DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                        ("[Ctx]: Releasing stream context %p (FileObject = %p, Instance = %p)\n",
                         streamContext,
                         Cbd->Iopb->TargetFileObject,
                         Cbd->Iopb->TargetInstance) );

            FltReleaseContext( streamContext );

            if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

                //
                //  FltSetStreamContext failed for a reason other than the context already
                //  existing on the stream. So the object now does not have any context set
                //  on it. So we return failure to the caller.
                //

                DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                            ("[Ctx]: Failed to set stream context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
                            status,
                            Cbd->Iopb->TargetFileObject,
                            Cbd->Iopb->TargetInstance) );

                return status;
            }

            //
            //  Race condition. Someone has set a context after we queried it.
            //  Use the already set context instead
            //

            DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                        ("[Ctx]: Stream context already defined. Retaining old stream context %p (FileObject = %p, Instance = %p)\n",
                         oldStreamContext,
                         Cbd->Iopb->TargetFileObject,
                         Cbd->Iopb->TargetInstance) );

            //
            //  Return the existing context. Note that the new context that we allocated has already been
            //  realeased above.
            //

            streamContext = oldStreamContext;
            status = STATUS_SUCCESS;

        } else {

            if (ContextCreated != NULL) *ContextCreated = TRUE;
        }
    }

    *StreamContext = streamContext;

    return status;
}




NTSTATUS
CtxCreateStreamContext (
    _Outptr_ PCTX_STREAM_CONTEXT *StreamContext
    )
/*++

Routine Description:

    This routine creates a new stream context

Arguments:

    StreamContext         - Returns the stream context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCTX_STREAM_CONTEXT streamContext;

    PAGED_CODE();

    //
    //  Allocate a stream context
    //

    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: Allocating stream context \n") );

    status = FltAllocateContext( Globals.Filter,
                                 FLT_STREAM_CONTEXT,
                                 CTX_STREAM_CONTEXT_SIZE,
                                 PagedPool,
                                 &streamContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to allocate stream context with status 0x%x \n",
                     status) );
        return status;
    }

    //
    //  Initialize the newly created context
    //

    RtlZeroMemory( streamContext, CTX_STREAM_CONTEXT_SIZE );

    streamContext->Resource = CtxAllocateResource();
    if(streamContext->Resource == NULL) {

        FltReleaseContext( streamContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite( streamContext->Resource );

    *StreamContext = streamContext;

    return STATUS_SUCCESS;
}


NTSTATUS
CtxUpdateNameInStreamContext (
    _In_ PUNICODE_STRING DirectoryName,
    _Inout_ PCTX_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description:

    This routine updates the name of the target in the supplied stream context

Arguments:

    DirectoryName         - Supplies the directory name
    StreamContext    - Returns the updated name in the stream context

Return Value:

    Status

Note:

    The caller must synchronize access to the context. This routine does no
    synchronization

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    //
    //  Free any existing name
    //

    if (StreamContext->FileName.Buffer != NULL) {

        CtxFreeUnicodeString(&StreamContext->FileName);
    }


    //
    //  Allocate and copy off the directory name
    //

    StreamContext->FileName.MaximumLength = DirectoryName->Length;
    status = CtxAllocateUnicodeString(&StreamContext->FileName);
    if (NT_SUCCESS(status)) {

        RtlCopyUnicodeString(&StreamContext->FileName, DirectoryName);
    }

    return status;
}




NTSTATUS
CtxCreateOrReplaceStreamHandleContext (
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_ BOOLEAN ReplaceIfExists,
    _Outptr_ PCTX_STREAMHANDLE_CONTEXT *StreamHandleContext,
    _Out_opt_ PBOOLEAN ContextReplaced
    )
/*++

Routine Description:

    This routine creates a stream handle context for the target stream
    handle. Optionally, if the context already exists, this routine
    replaces it with the new context and releases the old context

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    ReplaceIfExists       - Supplies if the stream handle context must be
                            replaced if already present
    StreamContext         - Returns the stream context
    ContextReplaced       - Returns if an existing context was replaced

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext;
    PCTX_STREAMHANDLE_CONTEXT oldStreamHandleContext;

    PAGED_CODE();

    *StreamHandleContext = NULL;
    if (ContextReplaced != NULL) *ContextReplaced = FALSE;

    //
    //  Create a stream context
    //

    DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: Creating stream handle context (FileObject = %p, Instance = %p)\n",
                 Cbd->Iopb->TargetFileObject,
                 Cbd->Iopb->TargetInstance) );

    status = CtxCreateStreamHandleContext( &streamHandleContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Failed to create stream context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                    status,
                    Cbd->Iopb->TargetFileObject,
                    Cbd->Iopb->TargetInstance) );

        return status;
    }

    //
    //  Set the new context we just allocated on the file object
    //

    DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: Setting stream context %p (FileObject = %p, Instance = %p, ReplaceIfExists = %x)\n",
                 streamHandleContext,
                 Cbd->Iopb->TargetFileObject,
                 Cbd->Iopb->TargetInstance,
                 ReplaceIfExists) );

    status = FltSetStreamHandleContext( Cbd->Iopb->TargetInstance,
                                        Cbd->Iopb->TargetFileObject,
                                        ReplaceIfExists ? FLT_SET_CONTEXT_REPLACE_IF_EXISTS : FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                        streamHandleContext,
                                        &oldStreamHandleContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Failed to set stream handle context with status 0x%x. (FileObject = %p, Instance = %p)\n",
                    status,
                    Cbd->Iopb->TargetFileObject,
                    Cbd->Iopb->TargetInstance) );

        //
        //  We release the context here because FltSetStreamContext failed
        //
        //  If FltSetStreamContext succeeded then the context will be returned
        //  to the caller. The caller will use the context and then release it
        //  when he is done with the context.
        //

        DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Releasing stream handle context %p (FileObject = %p, Instance = %p)\n",
                     streamHandleContext,
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        FltReleaseContext( streamHandleContext );

        if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

            //
            //  FltSetStreamContext failed for a reason other than the context already
            //  existing on the stream. So the object now does not have any context set
            //  on it. So we return failure to the caller.
            //

            DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                        ("[Ctx]: Failed to set stream context with status 0x%x != STATUS_FLT_CONTEXT_ALREADY_DEFINED. (FileObject = %p, Instance = %p)\n",
                        status,
                        Cbd->Iopb->TargetFileObject,
                        Cbd->Iopb->TargetInstance) );

            return status;
        }

        //
        //  We will reach here only if we have failed with STATUS_FLT_CONTEXT_ALREADY_DEFINED
        //  and we can fail with that code only if the context already exists and we have used
        //  the FLT_SET_CONTEXT_KEEP_IF_EXISTS flag

        FLT_ASSERT( ReplaceIfExists  == FALSE );

        //
        //  Race condition. Someone has set a context after we queried it.
        //  Use the already set context instead
        //

        DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: Stream context already defined. Retaining old stream context %p (FileObject = %p, Instance = %p)\n",
                     oldStreamHandleContext,
                     Cbd->Iopb->TargetFileObject,
                     Cbd->Iopb->TargetInstance) );

        //
        //  Return the existing context. Note that the new context that we allocated has already been
        //  realeased above.
        //

        streamHandleContext = oldStreamHandleContext;
        status = STATUS_SUCCESS;

    } else {

        //
        //  FltSetStreamContext has suceeded. The new context will be returned
        //  to the caller. The caller will use the context and then release it
        //  when he is done with the context.
        //
        //  However, if we have replaced an existing context then we need to
        //  release the old context so as to decrement the ref count on it.
        //
        //  Note that the memory allocated to the objects within the context
        //  will be freed in the context cleanup and must not be done here.
        //

        if ( ReplaceIfExists &&
             oldStreamHandleContext != NULL) {

            DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                        ("[Ctx]: Releasing old stream handle context %p (FileObject = %p, Instance = %p)\n",
                         oldStreamHandleContext,
                         Cbd->Iopb->TargetFileObject,
                         Cbd->Iopb->TargetInstance) );

            FltReleaseContext( oldStreamHandleContext );
            if (ContextReplaced != NULL) *ContextReplaced = TRUE;
        }
    }

    *StreamHandleContext = streamHandleContext;

    return status;
}





NTSTATUS
CtxCreateStreamHandleContext (
    _Outptr_ PCTX_STREAMHANDLE_CONTEXT *StreamHandleContext
    )
/*++

Routine Description:

    This routine creates a new stream context

Arguments:

    StreamContext         - Returns the stream context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext;

    PAGED_CODE();

    //
    //  Allocate a stream context
    //

    DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: Allocating stream handle context \n") );

    status = FltAllocateContext( Globals.Filter,
                                 FLT_STREAMHANDLE_CONTEXT,
                                 CTX_STREAMHANDLE_CONTEXT_SIZE,
                                 PagedPool,
                                 &streamHandleContext );

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to allocate stream handle context with status 0x%x \n",
                     status) );

        return status;
    }

    //
    //  Initialize the newly created context
    //

    RtlZeroMemory( streamHandleContext, CTX_STREAMHANDLE_CONTEXT_SIZE );

    streamHandleContext->Resource = CtxAllocateResource();
    if(streamHandleContext->Resource == NULL) {

        FltReleaseContext( streamHandleContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite( streamHandleContext->Resource );

    *StreamHandleContext = streamHandleContext;

    return STATUS_SUCCESS;
}



NTSTATUS
CtxUpdateNameInStreamHandleContext (
    _In_ PUNICODE_STRING DirectoryName,
    _Inout_ PCTX_STREAMHANDLE_CONTEXT StreamHandleContext
    )
/*++

Routine Description:

    This routine updates the name of the target in the supplied stream handle context

Arguments:

    DirectoryName             - Supplies the directory name
    StreamHandleContext   - Returns the updated name in the stream context

Return Value:

    Status

Note:

    The caller must synchronize access to the context. This routine does no
    synchronization

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    //
    //  Free any existing name
    //

    if (StreamHandleContext->FileName.Buffer != NULL) {

        CtxFreeUnicodeString(&StreamHandleContext->FileName);
    }


    //
    //  Allocate and copy off the directory name
    //

    StreamHandleContext->FileName.MaximumLength = DirectoryName->Length;
    status = CtxAllocateUnicodeString(&StreamHandleContext->FileName);
    if (NT_SUCCESS(status)) {

        RtlCopyUnicodeString(&StreamHandleContext->FileName, DirectoryName);
    }

    return status;
}


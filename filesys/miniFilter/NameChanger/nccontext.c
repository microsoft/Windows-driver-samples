/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    nccontext.c

Abstract:

    Contains routines to manage the lifetime of instance contexts and
    stream handle contexts.  This file performs context-wide setup
    and teardown, with callouts for specific functions to perform their
    own setup and teardown.

Environment:

    Kernel mode

--*/

#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcInstanceContextClose)
#pragma alloc_text(PAGE, NcStreamHandleContextAllocAndAttach)
#pragma alloc_text(PAGE, NcStreamHandleContextClose)
#endif

VOID
NcInstanceContextClose (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine is used to tear down and remove an instance context.

Arguments:

    Context - The context to destroy.
    
    ContextType - The type of the context.  Since this function is specific
        to instance contexts, we expect this to always be the instance
        context type.

Return Value:

    None.

--*/
{
    PNC_INSTANCE_CONTEXT InstanceContext = (PNC_INSTANCE_CONTEXT) Context;
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ContextType );
    FLT_ASSERT( ContextType == FLT_INSTANCE_CONTEXT );

    NcTeardownMapping( &InstanceContext->Mapping );
}

VOID
NcStreamHandleContextClose (
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    This routine is used to tear down and remove a stream handle context.

Arguments:

    Context - The context to destroy.
    
    ContextType - The type of the context.  Since this function is specific
        to stream handle contexts, we expect this to always be the stream
        handle context type.

Return Value:

    None.

--*/
{
    PNC_STREAM_HANDLE_CONTEXT StreamContext = (PNC_STREAM_HANDLE_CONTEXT) Context;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( ContextType );
    FLT_ASSERT( ContextType == FLT_STREAMHANDLE_CONTEXT );

    //
    //  Clean up notification context 
    //

    NcStreamHandleContextEnumClose( &StreamContext->DirectoryQueryContext );

    //
    //  Clean up enumeration context
    //

    NcStreamHandleContextNotClose( &StreamContext->DirectoryNotificationContext );

    //
    //  Clean up find by SID context
    //

    NcStreamHandleContextFindBySidClose( &StreamContext->FindBySidContext );

    //
    //  Clean up shared context information
    //

    if (StreamContext->Lock != NULL) {

        NcFreeEResource( StreamContext->Lock );
        StreamContext->Lock = NULL;
    }
}

NTSTATUS
NcStreamHandleContextAllocAndAttach (
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_ PNC_STREAM_HANDLE_CONTEXT * Context
    )
/*++

Routine Description:

    Allocates and initializes an empty stream handle context.

Arguments:

    Filter - The filter we are going to attach with.
    
    Instance - The instance we are going to attach with.

    FileObject - File object we are going to attach to.

    Context - Pointer to a user allocated PNC_STREAM_HANDLE_CONTEXT.

Return Value:

    On success, returns STATUS_SUCCESS and a pointer to the context in *Context.
    Otherwise returns an error and NULL in *Context.

    The user must call FltReleaseContext to balance ref count.

--*/
{
    PNC_STREAM_HANDLE_CONTEXT OurContext = NULL;
    PNC_STREAM_HANDLE_CONTEXT TheirContext = NULL;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Default to no return context.
    //

    *Context = NULL;

    //
    //  Check if a context already exists.  If so, return it.
    //

    Status = FltGetStreamHandleContext( Instance,
                                        FileObject,
                                        &TheirContext );

    if (NT_SUCCESS( Status )) {

        *Context = TheirContext;
        return Status;
    }

    //
    //  Allocate a new context.
    //

    Status = FltAllocateContext( Filter,
                                 FLT_STREAMHANDLE_CONTEXT,
                                 sizeof(NC_STREAM_HANDLE_CONTEXT),
                                 PagedPool, 
                                 &OurContext );

    if (!NT_SUCCESS( Status )) {

        goto NcStreamHandleContextAllocAndAttachClose;
    }

    //
    //  Zero out the buffer so we can unwind on failure.
    //

    RtlZeroMemory( OurContext, sizeof(NC_STREAM_HANDLE_CONTEXT));

    //
    //  Initialize shared context information.
    //

    Status = NcAllocateEResource( &OurContext->Lock );

    if (!NT_SUCCESS( Status )) {

        goto NcStreamHandleContextAllocAndAttachClose;
    }

    //
    //  Initialize the Directory Notification Structure.
    //
 
    Status = NcStreamHandleContextNotCreate( &OurContext->DirectoryNotificationContext );

    if (!NT_SUCCESS( Status )) {

        goto NcStreamHandleContextAllocAndAttachClose;
    }

    //
    //  Initialize the Directory Enumeration Structure
    //
    
    Status = NcStreamHandleContextDirEnumCreate( &OurContext->DirectoryQueryContext );

    if (!NT_SUCCESS( Status )) {

        goto NcStreamHandleContextAllocAndAttachClose;
    }

    //
    //  Initialize the Find by SID Structure
    //
    
    Status = NcStreamHandleContextFindBySidCreate( &OurContext->FindBySidContext );

    if (!NT_SUCCESS( Status )) {

        goto NcStreamHandleContextAllocAndAttachClose;
    }

    //
    //  Now we have a new context, but we could be racing with another thread
    //  attaching a context to this stream.  We will now try to attach, but
    //  keep the existing context if it is there. If there is an existing
    //  context, then we return the pre-existing context and free ours.
    //
    
    Status = FltSetStreamHandleContext( Instance,
                                        FileObject,
                                        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                        OurContext,
                                        &TheirContext );

    if (Status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {
            
        Status = STATUS_SUCCESS;

    } else if (!NT_SUCCESS( Status )) {

        goto NcStreamHandleContextAllocAndAttachClose;
    }

    //
    //  We need to keep 1 ref count on the context we are sticking with.
    //  However, if there are two contexts we need to free one of them.
    //  We therefore NULL out the context that is being returned so as
    //  to only clean up the other (if it exists.)
    //
    
    if (TheirContext != NULL) {

        *Context = TheirContext;
        TheirContext = NULL;

    } else {

        *Context = OurContext;
        OurContext = NULL;
    }
    
NcStreamHandleContextAllocAndAttachClose:

    if (OurContext != NULL) {

        FltReleaseContext( OurContext );
    }

    if (TheirContext != NULL) {

        FltReleaseContext( TheirContext );
    }

    return Status;
}



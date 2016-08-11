/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    operations.c

Abstract:

    This is the i/o operations module of the kernel mode filter driver implementing
    context sample


Environment:

    Kernel mode


--*/

#include "pch.h"


//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CtxPreCreate)
#pragma alloc_text(PAGE, CtxPostCreate)
#pragma alloc_text(PAGE, CtxPreCleanup)
#pragma alloc_text(PAGE, CtxPreClose)
#pragma alloc_text(PAGE, CtxPreSetInfo)
#pragma alloc_text(PAGE, CtxPostSetInfo)
#endif


FLT_PREOP_CALLBACK_STATUS
CtxPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{     
    UNREFERENCED_PARAMETER( Cbd );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
        
    PAGED_CODE();
    
    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreCreate -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreCreate -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    //  Force a post-op callback so we can add our contexts to the opened 
    //  objects
    //

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;

}


FLT_POSTOP_CALLBACK_STATUS
CtxPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_opt_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{

    PCTX_FILE_CONTEXT fileContext = NULL;    
    PCTX_STREAM_CONTEXT streamContext = NULL;    
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext = NULL;    
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    UNICODE_STRING fileName;

    NTSTATUS status;
    BOOLEAN fileContextCreated, streamContextCreated, streamHandleContextReplaced;


    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( CbdContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPostCreate -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    // Initialize defaults
    //

    status = STATUS_SUCCESS;

    //
    //  If the Create has failed, do nothing
    //

    if (!NT_SUCCESS( Cbd->IoStatus.Status )) {
        
        goto CtxPostCreateCleanup;        
    }


    //
    // Get the file name
    //
    
    status = FltGetFileNameInformation( Cbd,
                                        FLT_FILE_NAME_NORMALIZED |
                                        FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );    

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostCreate -> Failed to get name information (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostCreateCleanup;
    }

    
    //
    // Find or create a stream context
    //

    status = CtxFindOrCreateStreamContext(Cbd, 
                                          TRUE,
                                          &streamContext,
                                          &streamContextCreated);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because stream contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostCreate -> Failed to find or create stream context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPostCreateCleanup;
    }        

    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostCreate -> Getting/Creating stream context for file %wZ (Cbd = %p, FileObject = %p, StreamContext = %p. StreamContextCreated = %x)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 streamContextCreated) );

    //
    //  Acquire write acccess to the context
    //
    
    CtxAcquireResourceExclusive(streamContext->Resource);

    //
    //  Increment the create count
    //

    streamContext->CreateCount++;
    
    //
    //  Update the file name in the context
    //
    
    status = CtxUpdateNameInStreamContext( &nameInfo->Name, 
                                                  streamContext);


    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostCreate -> Stream context info for file %wZ (Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );

    //
    //  Relinquish write acccess to the context
    //
    
    CtxReleaseResource(streamContext->Resource);

    //
    //  Quit on failure after we have given up
    //  the resource
    //

    if (!NT_SUCCESS( status )) {
    
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostCreate -> Failed to update name in stream context for file %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostCreateCleanup;
    }
            


    //
    // Create or replace a stream handle context
    //

    status = CtxCreateOrReplaceStreamHandleContext(Cbd, 
                                                   TRUE,
                                                   &streamHandleContext,
                                                   &streamHandleContextReplaced);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because stream contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostCreate -> Failed to find or create stream handle context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostCreateCleanup;
    }        

    DebugTrace(  DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostCreate -> Creating/Replacing stream handle context for file %wZ (Cbd = %p, FileObject = %p StreamHandleContext = %p, StreamHandleContextReplaced = %x)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamHandleContext,
                 streamHandleContextReplaced) );

    //
    //  Acquire write acccess to the context
    //

    CtxAcquireResourceExclusive( streamHandleContext->Resource );

    //
    //  Update the file name in the context
    //

    status = CtxUpdateNameInStreamHandleContext( &nameInfo->Name, 
                                                 streamHandleContext);

    DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostCreate -> Stream handle context info for file %wZ (Cbd = %p, FileObject = %p, StreamHandleContext = %p) \n\tName = %wZ\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamHandleContext,
                 &streamHandleContext->FileName) );

    //
    //  Relinquish write acccess to the context
    //

    CtxReleaseResource(streamHandleContext->Resource);

    //
    //  Quit on failure after we have given up
    //  the resource
    //

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostCreate -> Failed to update name in stream handle context for file %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostCreateCleanup;
    }
    
    //
    //  After FltParseFileNameInformation, nameInfo->Name also
    //  contains the stream name. We need only the filename and do
    //  not want to include the stream name in the file context
    //

    fileName.Buffer = nameInfo->Name.Buffer;
    fileName.Length = nameInfo->Name.Length - nameInfo->Stream.Length;
    fileName.MaximumLength = fileName.Length;

    //
    // Find or create a file context
    //

    status = CtxFindOrCreateFileContext( Cbd, 
                                         TRUE,
                                         &fileName,
                                         &fileContext,
                                         &fileContextCreated);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because file contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostCreate -> Failed to find or create file context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostCreateCleanup;
    }        

    DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostCreate -> Getting/Creating file context for file %wZ (Cbd = %p, FileObject = %p, FileContext = %p. FileContextCreated = %x)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 fileContext,
                 fileContextCreated) );


    DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostCreate -> File context info for file %wZ (Cbd = %p, FileObject = %p, FileContext = %p) \n\tName = %wZ\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 fileContext,
                 &fileContext->FileName) );


CtxPostCreateCleanup:
    
    
    //
    // Release the references we have acquired
    //    
        
    if (nameInfo != NULL) {

        FltReleaseFileNameInformation( nameInfo );
    }
    
    if (fileContext != NULL) {

        FltReleaseContext( fileContext );            
    }

    if (streamContext != NULL) {

        FltReleaseContext( streamContext );            
    }

    if (streamHandleContext != NULL) {

        FltReleaseContext( streamHandleContext );            
    }
    
    if (!NT_SUCCESS( status )) {
    
        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Ctx]: CtxPostCreate -> Failed with status 0x%x \n",
                    status) );

        //
        //  It doesn't make sense to udate Cbd->IoStatus.Status on failure since the
        //  file system has successfully completed the operation
        //
        
    }


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPostCreate -> Exit (Cbd = %p, FileObject = %p, Status = 0x%x)\n",
                 Cbd,
                 FltObjects->FileObject,
                 Cbd->IoStatus.Status) );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
CtxPreCleanup (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

    PCTX_STREAM_CONTEXT streamContext = NULL;    
    NTSTATUS status;
    BOOLEAN streamContextCreated;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreCleanup -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    // Get the stream context
    //

    status = CtxFindOrCreateStreamContext(Cbd, 
                                          FALSE,     // do not create if one does not exist
                                          &streamContext,
                                          &streamContextCreated);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because stream contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPreCleanup -> Failed to find stream context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPreCleanupCleanup;
    }        

    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPreCleanup -> Getting stream context for file (Cbd = %p, FileObject = %p, StreamContext = %p. StreamContextCreated = %x)\n",
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 streamContextCreated) );

    //
    //  Acquire write acccess to the context
    //
    
    CtxAcquireResourceExclusive(streamContext->Resource);


    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPreCleanup -> Old info in stream context for file(Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );


    
    //
    //  Update the cleanup count in the context
    //
    
    streamContext->CleanupCount++;

    
    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPreCleanup -> New info in stream context for file (Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );

    //
    //  Relinquish write acccess to the context
    //
    
    CtxReleaseResource(streamContext->Resource);


CtxPreCleanupCleanup:

    //
    // Release the references we have acquired
    //    

    if (streamContext != NULL) {

        FltReleaseContext( streamContext );            
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreCleanup -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    //  It doesn't make sense to fail the cleanup - so ignore any errors we may 
    //  encounter and return success
    //

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}





FLT_PREOP_CALLBACK_STATUS
CtxPreClose (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

    PCTX_STREAM_CONTEXT streamContext = NULL;    
    NTSTATUS status;
    BOOLEAN streamContextCreated;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreClose -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    // Get the stream context
    //

    status = CtxFindOrCreateStreamContext(Cbd, 
                                          FALSE,     // do not create if one does not exist
                                          &streamContext,
                                          &streamContextCreated);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because stream contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPreClose -> Failed to find stream context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPreCloseCleanup;
    }        

    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPreClose -> Getting stream context for file (Cbd = %p, FileObject = %p, StreamContext = %p. StreamContextCreated = %x)\n",
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 streamContextCreated) );

    //
    //  Acquire write acccess to the context
    //
    
    CtxAcquireResourceExclusive(streamContext->Resource);


    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPreClose -> Old info in stream context for file(Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );
    
    //
    //  Update the close count in the context
    //
    
    streamContext->CloseCount++;

    
    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPreClose -> New info in stream context for file (Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );

    //
    //  Relinquish write acccess to the context
    //
    
    CtxReleaseResource(streamContext->Resource);


CtxPreCloseCleanup:

    //
    // Release the references we have acquired
    //    

    if (streamContext != NULL) {

        FltReleaseContext( streamContext );            
    }


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreClose -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    //
    //  It doesn't make sense to fail the cleanup - so ignore any errors we may 
    //  encounter and return success
    //

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_PREOP_CALLBACK_STATUS
CtxPreSetInfo (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    FILE_INFORMATION_CLASS fileInformationClass;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreSetInfo -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK; // pass through - default is no post op callback

    fileInformationClass = Cbd->Iopb->Parameters.SetFileInformation.FileInformationClass;

    //
    //  Ignore the ops we do not care about
    //
    
    if ((fileInformationClass != FileRenameInformation) &&
        (fileInformationClass != FileRenameInformationEx)) {

        DebugTrace( DEBUG_TRACE_ALL_IO,
                    ("[Ctx]: CtxPreSetInfo -> Ignoring SetInfo operations other than FileRenameInformation/FileRenameInformationEx (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPreSetInfoCleanup;
    }

    //
    //  We want to process renames in the post-op callback
    //
    
    callbackStatus = FLT_PREOP_SYNCHRONIZE; 


CtxPreSetInfoCleanup:


    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPreSetInfo -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return callbackStatus;

}




FLT_POSTOP_CALLBACK_STATUS
CtxPostSetInfo (
    _Inout_ PFLT_CALLBACK_DATA Cbd,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_opt_ PVOID CbdContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    PCTX_INSTANCE_CONTEXT instanceContext = NULL;
    PCTX_FILE_CONTEXT fileContext = NULL;
    PCTX_STREAM_CONTEXT streamContext = NULL;
    PCTX_STREAMHANDLE_CONTEXT streamHandleContext = NULL;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;

    NTSTATUS status;
    BOOLEAN streamContextCreated, fileContextCreated, streamHandleContextReplaced;
    
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CbdContext );

    //
    //  The pre-operation callback will return FLT_PREOP_SYNCHRONIZE if it needs a 
    //  post operation callback. In this case, the Filter Manager will call the 
    //  minifilter's post-operation callback in the context of the pre-operation 
    //  thread, at IRQL <= APC_LEVEL. This allows the post-operation code to be
    //  pagable and also allows it to access paged data
    //  

    PAGED_CODE();

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPostSetInfo -> Enter (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );


    //
    // Initialize defaults
    //
    
    status = STATUS_SUCCESS;

    //
    //  If the SetInfo has failed, do nothing
    //

    if (!NT_SUCCESS( Cbd->IoStatus.Status )) {
        
        goto CtxPostSetInfoCleanup;        
    }


    //
    //  Get the instance context for the target instance
    //

    DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Trying to get instance context (TargetInstance = %p, Cbd = %p, FileObject = %p)\n",
                 Cbd->Iopb->TargetInstance,
                 Cbd,
                 FltObjects->FileObject) );
    
    status = FltGetInstanceContext( Cbd->Iopb->TargetInstance,
                                    &instanceContext );
    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS | DEBUG_TRACE_ERROR,
                    ("[Ctx]: CtxPostSetInfo -> Failed to get instance context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPostSetInfoCleanup;
    }

    DebugTrace( DEBUG_TRACE_INSTANCE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Instance context info for volume %wZ (Cbd = %p, FileObject = %p, InstanceContext = %p) \n\tVolumeName = %wZ \n\tInstance = %p \n\tVolume = %p\n",
                 &instanceContext->VolumeName,
                 Cbd,
                 FltObjects->FileObject,
                 instanceContext,
                 &instanceContext->VolumeName,
                 &instanceContext->Instance,
                 &instanceContext->Volume) );


    //
    // Get the directory name
    //
    
    status = FltGetFileNameInformation( Cbd,
                                        FLT_FILE_NAME_NORMALIZED |
                                        FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );
    
    if (!NT_SUCCESS( status )) {
    
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostSetInfo -> Failed to get file name information (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPostSetInfoCleanup;
    }

    
    //
    // Get the stream context
    //

    status = CtxFindOrCreateStreamContext(Cbd, 
                                          FALSE,     // do not create if one does not exist
                                          &streamContext,
                                          &streamContextCreated);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because stream contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostSetInfo -> Failed to find stream context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPostSetInfoCleanup;
    }        

    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Getting stream context for file %wZ (Cbd = %p, FileObject = %p, StreamContext = %p. StreamContextCreated = %x)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 streamContextCreated) );

    //
    //  Acquire write acccess to the context
    //
    
    CtxAcquireResourceExclusive(streamContext->Resource);


    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Old info in stream context for file %wZ (Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );


    
    //
    //  Update the file name in the context
    //
    
    status = CtxUpdateNameInStreamContext( &nameInfo->Name, 
                                                  streamContext);


    DebugTrace( DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> New info in stream context for file %wZ (Cbd = %p, FileObject = %p, StreamContext = %p) \n\tName = %wZ \n\tCreateCount = %x \n\tCleanupCount = %x, \n\tCloseCount = %x\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamContext,
                 &streamContext->FileName,
                 streamContext->CreateCount,
                 streamContext->CleanupCount,
                 streamContext->CloseCount) );

    //
    //  Relinquish write acccess to the context
    //
    
    CtxReleaseResource(streamContext->Resource);

    //
    //  Quit on failure after we have given up
    //  the resource
    //

    if (!NT_SUCCESS( status )) {
    
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAM_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostSetInfo -> Failed to update name in stream context for file %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostSetInfoCleanup;
    }
            


    //
    // Create or replace a stream handle context
    //

    status = CtxCreateOrReplaceStreamHandleContext(Cbd, 
                                                   TRUE,
                                                   &streamHandleContext,
                                                   &streamHandleContextReplaced);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because stream contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostSetInfo -> Failed to find or create stream handle context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostSetInfoCleanup;
    }        

    DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Creating/Replacing stream handle context for file %wZ (Cbd = %p, FileObject = %p StreamHandleContext = %p, StreamHandleContextReplaced = %x)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamHandleContext,
                 streamHandleContextReplaced) );

    //
    //  Acquire write acccess to the context
    //

    CtxAcquireResourceExclusive(streamHandleContext->Resource);

    //
    //  Update the file name in the context
    //

    status = CtxUpdateNameInStreamHandleContext( &nameInfo->Name, 
                                                 streamHandleContext);

    DebugTrace( DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Stream handle context info for file %wZ (Cbd = %p, FileObject = %p, StreamHandleContext = %p) \n\tName = %wZ\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 streamHandleContext,
                 &streamHandleContext->FileName) );

    //
    //  Relinquish write acccess to the context
    //

    CtxReleaseResource( streamHandleContext->Resource );

    //
    //  Quit on failure after we have given up
    //  the resource
    //

    if (!NT_SUCCESS( status )) {

        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_STREAMHANDLE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostSetInfo -> Failed to update name in stream handle context for file %wZ (Cbd = %p, FileObject = %p)\n",
                     &nameInfo->Name,
                     Cbd,
                     FltObjects->FileObject) );

        goto CtxPostSetInfoCleanup;
    }

    
    //
    // Get the file context
    //

    status = CtxFindOrCreateFileContext( Cbd, 
                                         FALSE,     // do not create if one does not exist
                                         NULL,
                                         &fileContext,
                                         &fileContextCreated);
    if (!NT_SUCCESS( status )) {

        //
        //  This failure will most likely be because file contexts are not supported
        //  on the object we are trying to assign a context to or the object is being 
        //  deleted
        //  
                
        DebugTrace( DEBUG_TRACE_ERROR | DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                    ("[Ctx]: CtxPostSetInfo -> Failed to find file context (Cbd = %p, FileObject = %p)\n",
                     Cbd,
                     FltObjects->FileObject) );
    
        goto CtxPostSetInfoCleanup;
    }        

    DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> Getting file context for file %wZ (Cbd = %p, FileObject = %p, FileContext = %p. FileContextCreated = %x)\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 fileContext,
                 fileContextCreated) );

    DebugTrace( DEBUG_TRACE_FILE_CONTEXT_OPERATIONS,
                ("[Ctx]: CtxPostSetInfo -> File context info for file %wZ (Cbd = %p, FileObject = %p, FileContext = %p) \n\tName = %wZ\n",
                 &nameInfo->Name,
                 Cbd,
                 FltObjects->FileObject,
                 fileContext,
                 &fileContext->FileName) );
    

CtxPostSetInfoCleanup:

    
    //
    // Release the references we have acquired
    //    
        
    if (instanceContext != NULL) {

        FltReleaseContext( instanceContext );            
    }

    if (fileContext != NULL) {

        FltReleaseContext( fileContext );            
    }

    if (streamContext != NULL) {

        FltReleaseContext( streamContext );            
    }

    if (streamHandleContext != NULL) {

        FltReleaseContext( streamHandleContext );            
    }

    if (nameInfo != NULL) {

        FltReleaseFileNameInformation( nameInfo );
    }

    if (!NT_SUCCESS( status )) {
    
        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Ctx]: CtxPostSetInfo -> Failed with status 0x%x \n",
                    status) );

        //
        //  It doesn't make sense to udate Cbd->IoStatus.Status on failure since the
        //  file system has suceesfully completed the operation
        //
    }

    DebugTrace( DEBUG_TRACE_ALL_IO,
                ("[Ctx]: CtxPostSetInfo -> Exit (Cbd = %p, FileObject = %p)\n",
                 Cbd,
                 FltObjects->FileObject) );

    return FLT_POSTOP_FINISHED_PROCESSING;
}



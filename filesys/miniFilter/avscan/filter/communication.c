/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    communication.c

Abstract:

    Communication module implementation.
    This module contains the routines that involves the communication 
    between kernel mode and user mode.

Environment:

    Kernel mode

--*/

#include "avscan.h"

NTSTATUS
AvConnectNotifyCallback (
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionCookie
    );
    
VOID
AvDisconnectNotifyCallback(
    _In_opt_ PVOID ConnectionCookie
   );
   
NTSTATUS
AvMessageNotifyCallback (
    _In_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG ReturnOutputBufferLength
    );

//
//  Local routines
//

NTSTATUS
AvGetScanCtxSynchronized (
    _In_    LONGLONG  ScanId,
    _Out_   PAV_SCAN_CONTEXT  *ScanCtx
    );
    
NTSTATUS
AvGetInstanceContextByVolume (
    _In_    PFLT_VOLUME  volumeObject,
    _Out_   PAV_INSTANCE_CONTEXT  *InstanceContext
    );
    
NTSTATUS
AvGetInstanceContextByFileHandle (
    _In_  HANDLE                Handle,
    _Out_ PAV_INSTANCE_CONTEXT  *InstanceContext
    );

NTSTATUS
AvGetStreamContextByHandle (
    _In_    HANDLE           Handle,
    _Out_   PAV_STREAM_CONTEXT *StreamContext
    );

NTSTATUS
AvUpdateStreamContextWithScanResult (
    _Inout_  PAV_STREAM_CONTEXT StreamContext,
    _In_  PAV_SCAN_CONTEXT ScanContext,
    _In_  AVSCAN_RESULT ScanResult
    );
    
NTSTATUS
AvHandleCmdCreateSectionForDataScan (
    _Inout_ PAV_SCAN_CONTEXT ScanContext,
    _Out_ PHANDLE SectionHandle
    );
    
NTSTATUS
AvHandleCmdCloseSectionForDataScan (
    _Inout_  PAV_SCAN_CONTEXT ScanContext,
    _In_  AVSCAN_RESULT ScanResult
    );
    
#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, AvMessageNotifyCallback)
    #pragma alloc_text(PAGE, AvConnectNotifyCallback)
    #pragma alloc_text(PAGE, AvDisconnectNotifyCallback)
    #pragma alloc_text(PAGE, AvPrepareServerPort)
    
    #pragma alloc_text(PAGE, AvGetInstanceContextByVolume)
    #pragma alloc_text(PAGE, AvGetInstanceContextByFileHandle)
    #pragma alloc_text(PAGE, AvGetStreamContextByHandle)
    #pragma alloc_text(PAGE, AvUpdateStreamContextWithScanResult)
    #pragma alloc_text(PAGE, AvFinalizeScanAndSection)
    #pragma alloc_text(PAGE, AvFinalizeScanContext)
    #pragma alloc_text(PAGE, AvFinalizeSectionContext)
    #pragma alloc_text(PAGE, AvHandleCmdCreateSectionForDataScan)
    #pragma alloc_text(PAGE, AvHandleCmdCloseSectionForDataScan)
#endif

NTSTATUS
AvConnectNotifyCallback (
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionCookie
    )
/*++

Routine Description

    Communication connection callback routine.
    This is called when user-mode connects to the server port.

Arguments

    ClientPort - This is the client connection port that will be used to send messages from the filter
    
    ServerPortCookie - Unused
    
    ConnectionContext - The connection context passed from the user. This is to recognize which type 
            connection the user is trying to connect.
    
    SizeofContext   - The size of the connection context.
    
    ConnectionCookie - Propagation of the connection context to disconnection callback.

Return Value

    STATUS_SUCCESS - to accept the connection
    STATUS_INSUFFICIENT_RESOURCES - if memory is not enough
    STATUS_INVALID_PARAMETER_3 - Connection context is not valid.
--*/
{
    PAV_CONNECTION_CONTEXT connectionCtx = (PAV_CONNECTION_CONTEXT) ConnectionContext;
    PAVSCAN_CONNECTION_TYPE connectionCookie = NULL;
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( SizeOfContext );
    
    if (NULL == connectionCtx) {
    
        return STATUS_INVALID_PARAMETER_3;
    }
    
    //
    //  ConnectionContext passed in may be deleted. We need to make a copy of it.
    //
    
    connectionCookie = ExAllocatePoolWithTag( PagedPool,
                           sizeof(AVSCAN_CONNECTION_TYPE),
                           AV_CONNECTION_CTX_TAG );
    if (NULL ==  connectionCookie) {
    
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *connectionCookie = connectionCtx->Type;
    switch (connectionCtx->Type) {
        case AvConnectForScan:
            Globals.ScanClientPort = ClientPort;
            *ConnectionCookie = connectionCookie;
            break;
        case AvConnectForAbort:
            Globals.AbortClientPort = ClientPort;
            *ConnectionCookie = connectionCookie;
            break;
        case AvConnectForQuery:
            Globals.QueryClientPort = ClientPort;
            *ConnectionCookie = connectionCookie;
            break;
        default:
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV]: AvConnectNotifyCallback: No such connection type. \n") );
            ExFreePoolWithTag( connectionCookie,
                       AV_CONNECTION_CTX_TAG );
            *ConnectionCookie = NULL;
            return STATUS_INVALID_PARAMETER_3;
    }
    
    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
            ("[AV]: AvConnectNotifyCallback entered. type: %d \n", connectionCtx->Type) );
    
    return STATUS_SUCCESS;
}

VOID
AvDisconnectNotifyCallback(
    _In_opt_ PVOID ConnectionCookie
   )
/*++

Routine Description

    Communication disconnection callback routine.
    This is called when user-mode disconnects the server port.

Arguments
    
    ConnectionCookie - The cookie set in AvConnectNotifyCallback(...). It is connection context.

Return Value

    None
--*/
{
    PAVSCAN_CONNECTION_TYPE connectionType = (PAVSCAN_CONNECTION_TYPE) ConnectionCookie;
    
    PAGED_CODE();

    if (NULL == connectionType) {
    
        return;
    }
    //
    //  Close communication handle
    //
    switch (*connectionType) {
        case AvConnectForScan:
            FltCloseClientPort( Globals.Filter, &Globals.ScanClientPort );
            Globals.ScanClientPort = NULL;
            break;
        case AvConnectForAbort:
            FltCloseClientPort( Globals.Filter, &Globals.AbortClientPort );
            Globals.AbortClientPort = NULL;
            break;
        case AvConnectForQuery:
            FltCloseClientPort( Globals.Filter, &Globals.QueryClientPort );
            Globals.QueryClientPort = NULL;
            break;
        default:
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV]: AvDisconnectNotifyCallback: No such connection type. \n") );
            return;
    }
    
    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
            ("[AV]: AvDisconnectNotifyCallback entered. type: %d \n", *connectionType) );
            
    ExFreePoolWithTag( connectionType,
                       AV_CONNECTION_CTX_TAG );

}

NTSTATUS
AvGetScanCtxSynchronized (
    _In_    LONGLONG  ScanId,
    _Out_   PAV_SCAN_CONTEXT  *ScanCtx
    )
/*++

Routine Description

    A helper function to retrieve the scan context from its scan context id.
    It is synchronized by a lock. 

Arguments
    
    ScanId - The scan id to be found.
    ScanCtx - The output scan context. NULL if not found

Return Value

    STATUS_SUCCESS - if found.
    Otherwise - Error, or if not found.
    
--*/
{
    PLIST_ENTRY link;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN found = FALSE;
    PAV_SCAN_CONTEXT scanCtx = NULL;
    
    //
    //  We only 'read' the scan context when we traversing the list
    //
    
    AvAcquireResourceShared( &Globals.ScanCtxListLock );
    
    for (link = Globals.ScanCtxListHead.Flink;
         link != &Globals.ScanCtxListHead;
         link = link->Flink) {

        scanCtx = CONTAINING_RECORD( link, AV_SCAN_CONTEXT, List );
          
        if (scanCtx->ScanId == ScanId) {
            found = TRUE;
            AvReferenceScanContext( scanCtx );
            break;
        }
        
    }
    
    AvReleaseResource( &Globals.ScanCtxListLock );
    
    if (found) {
    
        *ScanCtx = scanCtx;
        return STATUS_SUCCESS;
    }
    
    AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("[AV] AvGetScanCtxSynchronized: scan context not found. \n") );
    
    *ScanCtx = NULL;
    
    if (NT_SUCCESS( status )){
    
        status = STATUS_UNSUCCESSFUL;
    }
    
    return status;
}

NTSTATUS
AvGetInstanceContextByVolume (
    _In_    PFLT_VOLUME  VolumeObject,
    _Out_   PAV_INSTANCE_CONTEXT  *InstanceContext
    )
/*++

Routine Description

    A helper function to retrieve the instance context from its volume object.
    
    The caller is responsible for dereference InstanceContext via calling 
    FltReleaseContext(...) if success.

Arguments
    
    VolumeObject - The volume object.
    InstanceContext - The output instance context. NULL if not found

Return Value

    STATUS_SUCCESS - if found.
    Otherwise - Error, or not found.
--*/
{
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN found = FALSE;
    PFLT_INSTANCE *instArray = NULL;
    ULONG instCnt = 0;
    PAV_INSTANCE_CONTEXT instCtx = NULL;
    
    PAGED_CODE();
    
    status = AvEnumerateInstances ( &instArray, &instCnt );
    
    if ( !NT_SUCCESS(status) ) {
    
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("[AV] AvGetInstanceContextByVolume: Failed to enumerate instances. \n") );
        return status;
    }
    
    for (i = 0; i < instCnt; i++) {
        
        status = FltGetInstanceContext( instArray[i], &instCtx );
        
        if ( !NT_SUCCESS(status) ) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV] AvGetInstanceContextByVolume: Failed to get instance context. \n") );
            break;
        }
        
        if (instCtx->Volume == VolumeObject) {
        
            //
            //  When found, we do not release the reference of instance context
            //  because the caller is responsible for releasing it.
            //
            
            found = TRUE;
            break;
        }
        
        FltReleaseContext( instCtx );
    }

    AvFreeInstances( instArray, instCnt );
    instArray = NULL;
    
    if (found) {
    
        *InstanceContext = instCtx;
        return STATUS_SUCCESS;
    }
    
    AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV] AvGetInstanceContextByVolume: instance context not found. \n") );
                
    if ( NT_SUCCESS( status ) ){
    
        status = STATUS_UNSUCCESSFUL;
    }
    
    instCtx = NULL;
    
    return status;
}

NTSTATUS
AvGetInstanceContextByFileHandle (
    _In_  HANDLE                Handle,
    _Out_ PAV_INSTANCE_CONTEXT  *InstanceContext
    )
/*++

Routine Description

    A helper function to retrieve the instance context from file handle.
    
    The caller is responsible for dereference InstanceContext via calling 
    FltReleaseContext(...) if success.

Arguments
    
    Handle - The file handle of interest.
    InstanceContext - The output instance context. NULL if not found

Return Value

    STATUS_SUCCESS - if found.
    Otherwise - if not found; it will report the corresponding status.
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_OBJECT fileObject = NULL;
    PFLT_VOLUME volumeObject = NULL;
    
    PAGED_CODE();
    
    //
    //  Get file object by handle
    //
    
    status = ObReferenceObjectByHandle (
                                    Handle,
                                    0,
                                    *IoFileObjectType,
                                    KernelMode,
                                    (PVOID *)&fileObject,
                                    NULL
                                    );  
    if (!NT_SUCCESS(status)) {
    
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV] AvGetInstanceContextByFileHandle: Failed to get file object by handle. \n") );
        return status;
    }
    
    try {
    
        status = FltGetVolumeFromFileObject( Globals.Filter,
                                             fileObject,
                                             &volumeObject );
                                             
        if (!NT_SUCCESS(status)) {
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                ("[AV] AvGetInstanceContextByFileHandle: Failed to get volume by file object. \n") );
            leave;
        }
        
        status = AvGetInstanceContextByVolume(volumeObject, InstanceContext);
        
        FltObjectDereference( volumeObject );
    
    } finally {
    
        ObDereferenceObject( fileObject );
    }
    
    return status;
}   

NTSTATUS
AvGetStreamContextByHandle (
    _In_    HANDLE           Handle,
    _Out_   PAV_STREAM_CONTEXT *StreamContext
    )
/*++

Routine Description

    A helper function to retrieve the stream context from a file handle at message 
    callback routine. This function will increment the reference count of the 
    output stream context.
    
    The caller is responsible for dereference it via calling FltReleaseContext(...)
    if success.

Arguments
    
    Handle - The file handle of interest.
    StreamContext - The output stream context. NULL if not found

Return Value

    STATUS_SUCCESS - if found.
    Otherwise - if not found; it will report the corresponding status.
--*/
{
    NTSTATUS status;
    PFILE_OBJECT fileObject = NULL;
    PAV_INSTANCE_CONTEXT instanceContext = NULL;
    
    PAGED_CODE();
    
    status = AvGetInstanceContextByFileHandle( Handle, &instanceContext);
            
    if (!NT_SUCCESS(status)) {
    
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
        ("[AV]: ***AvGetInstanceContextByFileHandle FAILED. \n") );
        return status;
    }
    try {
    
        status = ObReferenceObjectByHandle (
                                        Handle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
                                        (PVOID *)&fileObject,
                                        NULL
                                        );  
        if (!NT_SUCCESS(status)) {
        
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[AV] AvGetStreamContextByHandle: Failed to get file object by handle. \n") );
            leave;
        }
        
        status = FltGetStreamContext( instanceContext->Instance,
                                    fileObject,
                                    StreamContext );

        ObDereferenceObject( fileObject );
        
    } finally {
    
        FltReleaseContext( instanceContext );
    }
    return status;
}

NTSTATUS
AvHandleCmdCreateSectionForDataScan (
    _Inout_ PAV_SCAN_CONTEXT ScanContext,
    _Out_ PHANDLE SectionHandle
    )
/*++

Routine Description:

    This function handles CmdCreateSectionForDataScan message.
    This function will create and return the section handle to the caller. 
    If any error occurs, it will trigger events to release the waiting threads.
    
    NOTE: this function does not check the buffer size etc.
    It must be checked before passing into this function.

Arguments:

    ScanContext - The scan context.
    ScanThreadId - The thread ID of the thread doing the scan
    SectionHandle - receives the section handle

Return Value:

    Returns the status of processing the message.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PAV_STREAM_CONTEXT streamContext = NULL;
    PAV_SECTION_CONTEXT sectionContext = NULL;
    HANDLE sectionHandle = NULL;
    
    PAGED_CODE();

    status = FltGetStreamContext ( ScanContext->FilterInstance, 
                                   ScanContext->FileObject, 
                                   &streamContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvHandleCmdCreateSectionForDataScan: failed to get stream context.\n") );
        
        goto Cleanup;
    }

    //
    //  It should be impossible for the stream state to change from
    //  uknown to clean since we kicked off this scan. 
    //
    
    ASSERT(IS_FILE_NEED_SCAN( streamContext ));
    
    status = AvCreateSectionContext( ScanContext->FilterInstance, 
                                     ScanContext->FileObject, 
                                     &sectionContext);
        
    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvHandleCmdCreateSectionForDataScan: failed to create section context.\n") );
              
        goto Cleanup;
    }

    //
    //  Before we are going to create section object, if this flag is set (by the thread that requests for scan),
    //  it means that the thread is trying to cancel this scan, and thus we don't want the scan to proceed anymore.
    //
    if (ScanContext->IoWaitOnScanCompleteNotificationAborted) {
        
        status = STATUS_CANCELLED;
        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
              ("[AV] AvHandleCmdCreateSectionForDataScan: Before FltCreateSectionForDataScan, it found Io is trying to abort the wait.\n") );
              
        goto Cleanup;
    }
    
    sectionContext->ScanContext = ScanContext;
    sectionContext->CancelableOnConflictingIo = (ScanContext->IOMajorFunctionAtScan == IRP_MJ_CLEANUP);

    //
    //  Note: the section conflict callback could be called before
    //  this routine returns. It is even possible that the context
    //  passed to the callback won't have the SectionHandle and
    //  SectionObject fields set yet.
    //
        
    status = FltCreateSectionForDataScan( ScanContext->FilterInstance,
                                          ScanContext->FileObject,
                                          sectionContext,
                                          SECTION_MAP_READ,
                                          NULL,
                                          NULL,
                                          PAGE_READONLY,
                                          SEC_COMMIT,
                                          0,
                                          &sectionContext->SectionHandle,
                                          &sectionContext->SectionObject,
                                          NULL );

    sectionHandle = sectionContext->SectionHandle;
    
    if (!NT_SUCCESS( status )) {
    
#if DBG
        NTSTATUS sta = STATUS_SUCCESS;
        PFLT_VOLUME volumeObject = NULL;
        ULONG length = 0;
        UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES)+256];     //enough space for names
        PFLT_VOLUME_PROPERTIES property = (PFLT_VOLUME_PROPERTIES)volPropBuffer;

        sta = FltGetVolumeFromFileObject( Globals.Filter,
                                          ScanContext->FileObject,
                                          &volumeObject );
        if (NT_SUCCESS( sta )) {
            sta = FltGetVolumeProperties( volumeObject,
                                          property,
                                          sizeof(volPropBuffer),
                                          &length );
            if (NT_SUCCESS( sta )) {
                AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                    ("[AV] ############## %wZ, %wZ, %wZ\n",
                     property->FileSystemDriverName,
                     property->FileSystemDeviceName,
                     property->RealDeviceName) );
            }
            FltObjectDereference( volumeObject );
        }

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvHandleCmdCreateSectionForDataScan: %I64x,%I64x failed to create section object. 0x%x\n",
               streamContext->FileId.FileId64.UpperZeroes,
               streamContext->FileId.FileId64.Value,
               sta) );
#endif  // DBG

        goto Cleanup;
    }
    
    //
    //  Before scanning, we set the file status as scanning.
    //  This is important when another thread is writing to this file while we are scanning 
    //  this file.
    //
    
    SET_FILE_SCANNING_EX( ScanContext->IsFileInTxWriter, streamContext );
    
    //
    //  Only after the section object is successfully created, we put a section context pointer
    //  into the scan context. 
    //
    
    FltReferenceContext( sectionContext );
    ScanContext->SectionContext = sectionContext;

    *SectionHandle = sectionHandle;
    
Cleanup:

    //
    //  The I/O request thread is waiting for this event.
    //  If any error occurs, we have to release the waiting thread.
    //  if status is a success code, the thread will get released when 
    //  the user send message to close the section object.
    //
    
    if (!NT_SUCCESS( status )) {
    
        KeSetEvent( &ScanContext->ScanCompleteNotification, 0, FALSE );
    }
    
    if (streamContext) {

        //
        //  On error signal the event to release any threads waiting to
        //  scan the same file. On success it will get released when the 
        //  message is sent to close the section object.
        //
        
        if (!NT_SUCCESS( status )) {
        
            SET_FILE_MODIFIED_EX( ScanContext->IsFileInTxWriter, streamContext );
        }
        
        FltReleaseContext( streamContext );
        streamContext = NULL;
    }
    
    if (sectionContext) {
    
        FltReleaseContext( sectionContext );
        sectionContext = NULL;
    }

    //
    //  After this routine assigned section context into scan context and created sectionHandle,
    //  we need to check if notification abort flag was set, if it was, we need to fail the section creation
    //  because at this point, the request for scan was abondaned anyway, we 'stop' the scan by
    //  returning STATUS_CANCELLED to the user.
    //
    if (NT_SUCCESS( status ) &&
        ScanContext->IoWaitOnScanCompleteNotificationAborted) {
        
        status = STATUS_CANCELLED;
        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
              ("[AV] AvHandleCmdCreateSectionForDataScan: After FltCreateSectionForDataScan, it found Io is trying to abort the wait.\n") );

        //
        //  We explicitly call NtClose() instead of ZwClose() so that PreviousMode() will be User.  
        //  This prevents accidental closing of a kernel handle and also will not bugcheck the 
        //  system if the handle value is no longer valid
        //
        NtClose( sectionHandle );
        
        //
        //  This user mode handle is supposed to be closed in the user mode program.
        //  We close in the context of the same process context.
        //
        AvFinalizeScanAndSection( ScanContext );
    }
    
    return status;
}


NTSTATUS
AvUpdateStreamContextWithScanResult (
    _Inout_  PAV_STREAM_CONTEXT StreamContext,
    _In_  PAV_SCAN_CONTEXT ScanContext,
    _In_  AVSCAN_RESULT ScanResult
    )
/*++
        
Routine Description:

    This function updates StreamContex according to ScanResult.
    e.g. Set the stream as modified, infected, etc.
    
Arguments:

    StreamContext - The stream context to be updated.

    ScanContext - The scan context.
    
    ScanResult - The scan result. Please see the definition of AVSCAN_RESULT.

Return Value:

    Returns STATUS_SUCCESS.
        
--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    
    switch( ScanResult ) {
            
        case AvScanResultUndetermined:
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                          ("***[AV] AvUpdateScanResult: the caller did not specify the scan result.\n") );
            //
            //  If for some reason, the scan result returns undetermined, we have to 
            //  set the file state back to AvFileModifed.
            //
            
            SET_FILE_MODIFIED_EX( ScanContext->IsFileInTxWriter, StreamContext);
            break;
        case AvScanResultInfected:
            //
            //  If after the scan and before setting this file as clean, the file gets modified, 
            //  then we have to leave it as modified.
            //
            
            if (ScanContext->IsFileInTxWriter) {
                
                InterlockedCompareExchange( &StreamContext->TxState, AvFileInfected, AvFileScanning );
                
            } else {
            
                InterlockedCompareExchange( &StreamContext->State, AvFileInfected, AvFileScanning );
            }
            break;
        case AvScanResultClean:
        
            //
            //  If after the scan and before setting this file as clean, the file gets modified, 
            //  then we have to leave it as modified.
            //
            
            if (ScanContext->IsFileInTxWriter) {
                
                InterlockedCompareExchange( &StreamContext->TxState, AvFileNotInfected, AvFileScanning );
                
            } else {
            
                InterlockedCompareExchange( &StreamContext->State, AvFileNotInfected, AvFileScanning );
            }
            
            break;
        default:
            FLT_ASSERTMSG( "No such scan result.\n", FALSE);
            break;
    }

    return status;
}

NTSTATUS
AvFinalizeScanAndSection (
    _Inout_  PAV_SCAN_CONTEXT ScanContext
    )
/*++
        
Routine Description:

    This function is a wrapper function to finalize scan context and section context.
    Normally, you should call this function if you don't need to use section context before
    closing it.
    
Arguments:

    ScanContext - The scan context.

Return Value:

    Returns the status code from FltCloseSectionForDataScan.
        
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PAV_SECTION_CONTEXT sectionContext = NULL;

    PAGED_CODE();
    
    AvFinalizeScanContext( ScanContext, &sectionContext );
    
    //
    //  This thread won the race, and is responsible for finalizing the section context
    //
    if (sectionContext != NULL) {
        
        status = AvFinalizeSectionContext( sectionContext );
    }
    return status;
}

VOID
AvFinalizeScanContext (
    _Inout_  PAV_SCAN_CONTEXT ScanContext,
    _Outptr_result_maybenull_ PAV_SECTION_CONTEXT *SectionContext
    )
/*++
        
Routine Description:

    This function interlocked-exchange the section context inside the scan context and 
    release the waiting I/O request thread.

    The caller is responsible for releasing the reference count of SectionContext 
    when it successfully exchanges a non-NULL section context.
    
Arguments:

    ScanContext - The scan context.

    SectionContext - Receives the sectioncontext address indicating the caller is 
                              responsible for tearing down the sectioncontext. 
                              Receives NULL if the context is already being torn down by another thread.

Return Value:

    None.
        
--*/
{
    PAV_SECTION_CONTEXT oldSectionCtx = NULL;

    PAGED_CODE();

    *SectionContext = NULL;

    //
    //  Synchronization between AvInstanceTeardownStart(...) or timeout
    //  processing in the IO thread.
    //

    oldSectionCtx = InterlockedExchangePointer( &ScanContext->SectionContext, NULL );

    //
    //  If sectionContext is NULL, it means that another thread has 
    //  already begun teardown of the section.
    //

    if (oldSectionCtx) {

        //
        //  The caller is responsible for releasing the reference count when assigned in ScanContext.
        //
        *SectionContext = oldSectionCtx;
    }

    //
    //  The I/O request thread is waiting for this event.
    //

    KeSetEvent( &ScanContext->ScanCompleteNotification, 0, FALSE );
}

NTSTATUS
AvFinalizeSectionContext (
    _Inout_  PAV_SECTION_CONTEXT SectionContext
    )
/*++
        
Routine Description:

    This function is a wrapper function to finalize section context.
    It closes the section context/object and release its reference.
    
Arguments:

    SectionContext - The section context.

Return Value:

    Returns the status code from FltCloseSectionForDataScan.
        
--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    
    status = AvCloseSectionForDataScan( SectionContext );

    if (!NT_SUCCESS(status)) {
        
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("***[AV]: AvFinalizeSectionContext: Close section failed.\n") );
    }
    FltReleaseContext( SectionContext );
    return status;
}

NTSTATUS
AvHandleCmdCloseSectionForDataScan (
    _Inout_  PAV_SCAN_CONTEXT ScanContext,
    _In_  AVSCAN_RESULT ScanResult
    )
/*++

Routine Description:

    This function handles AvCmdCloseSectionForDataScan message.
    This function will
    
    1) close the section object
    2) Set the file clean or infected.
    3) trigger events to release the waiting threads.

Arguments:

    ScanContext - The scan context.
    
    ScanResult - The scan result. Please see the definition of AVSCAN_RESULT.

Return Value:

    Returns the status of processing the message.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PAV_STREAM_CONTEXT streamContext = NULL;
    
    PAGED_CODE();
    
    status = FltGetStreamContext ( ScanContext->FilterInstance, 
                                   ScanContext->FileObject, 
                                   &streamContext );

    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("***[AV] AvHandleCmdCloseSectionForDataScan: failed to get stream context.\n") );

        goto Cleanup;
    }
    
    //
    //  Update stream context will succeed.
    //
    AvUpdateStreamContextWithScanResult(streamContext, ScanContext, ScanResult); 

Cleanup:

    status = AvFinalizeScanAndSection( ScanContext );
    
    //
    //  Either the above operations are successful, or any error occur,
    //  we have to release the stream context.
    //

    if ( streamContext ) {
    
        FltReleaseContext( streamContext );
    }

    return status;
    
}

NTSTATUS
AvMessageNotifyCallback (
    _In_ PVOID ConnectionCookie,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ ULONG InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG ReturnOutputBufferLength
    )
/*++

Routine Description:

    This routine is called whenever the user program sends message to
    filter via FilterSendMessage(...).
    
    The user space scanner sends message to
    
    1) Create the section for data scan
    2) Close the section for data scan
    3) Set a certain file to be infected
    4) Query the file state of a file

Arguments:

    InputBuffer - A buffer containing input data, can be NULL if there
        is no input data.

    InputBufferSize - The size in bytes of the InputBuffer.

    OutputBuffer - A buffer provided by the application that originated
        the communication in which to store data to be returned to the
        application.

    OutputBufferSize - The size in bytes of the OutputBuffer.

    ReturnOutputBufferSize - The size in bytes of meaningful data
        returned in the OutputBuffer.

Return Value:

    Returns the status of processing the message.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    AVSCAN_COMMAND command;
    HANDLE hFile = NULL;
    LONGLONG scanId = 0;
    PAV_SCAN_CONTEXT scanContext = NULL;
    AVSCAN_RESULT scanResult = AvScanResultUndetermined;
    PAV_STREAM_CONTEXT streamContext;
    HANDLE sectionHandle;
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER( ConnectionCookie );
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                ("[AV]: AvMessageNotifyCallback entered. \n") );
                
                
    if ((InputBuffer == NULL) ||
        (InputBufferSize < (FIELD_OFFSET(COMMAND_MESSAGE, Command) +
                            sizeof(AVSCAN_COMMAND)))) {

        return STATUS_INVALID_PARAMETER;
    }
    
    try {

        //
        //  Probe and capture input message: the message is raw user mode
        //  buffer, so need to protect with exception handler
        //

        command = ((PCOMMAND_MESSAGE) InputBuffer)->Command;
        scanId = ((PCOMMAND_MESSAGE) InputBuffer)->ScanId;

    } except (AvExceptionFilter( GetExceptionInformation(), TRUE )) {

        return GetExceptionCode();
    }
    
    //
    //  Only 
    //        AvCmdCreateSectionForDataScan
    //        AvCmdCloseSectionForDataScan
    //  require the check of scanCtxId
    //
    //  We also check the output buffer size, and its alignment here.
    //
    
    switch (command) {

        case AvCmdCreateSectionForDataScan:
        
            if ((OutputBufferSize < sizeof (HANDLE)) ||
                    (OutputBuffer == NULL)) {

                return STATUS_INVALID_PARAMETER;
            }

            if (!IS_ALIGNED(OutputBuffer,sizeof(HANDLE))) {

                return STATUS_DATATYPE_MISALIGNMENT;
            }
            
            status = AvGetScanCtxSynchronized( scanId,
                                               &scanContext );
            
            if (!NT_SUCCESS( status )) {
            
                return STATUS_NOT_FOUND;
            }

            status = AvHandleCmdCreateSectionForDataScan( scanContext,
                                                          &sectionHandle );
            
            if (NT_SUCCESS(status)) {
                //
                //  We succesfully created a section object/handle. 
                //  Try to set the handle in the OutputBuffer
                //
                try {
                
                    (*(PHANDLE)OutputBuffer) = sectionHandle;
                    *ReturnOutputBufferLength = sizeof(HANDLE);
                    
                }  except (AvExceptionFilter( GetExceptionInformation(), TRUE )) {
                    //
                    //  We cannot depend on user service program to close this handle for us.
                    //  We explicitly call NtClose() instead of ZwClose() so that PreviousMode() will be User.  
                    //  This prevents accidental closing of a kernel handle and also will not bugcheck the 
                    //  system if the handle value is no longer valid
                    //
                    NtClose( sectionHandle );
                    
                    //
                    // Close section and release the waiting I/O request thread
                    // We treat invalid user buffer as an exception and remove
                    // section object inside scan context. You can also design a protocol 
                    // that have user program to re-try for section creation failure.
                    //
                    AvFinalizeScanAndSection( scanContext );
                    status = GetExceptionCode();
                }
            }

            //
            //  AvGetScanCtxSynchronized incremented the ref count of scan context
            //
            AvReleaseScanContext( scanContext );
            
            break;
            
        case AvCmdCloseSectionForDataScan:
        
            try {
            
                scanResult = ((PCOMMAND_MESSAGE) InputBuffer)->ScanResult;

                if (scanResult == AvScanResultInfected) {
                    AV_DBG_PRINT( AVDBG_TRACE_OPERATION_STATUS,
                        ("[AV]: *******AvCmdCreateSectionForDataScan FAILED. \n") );
                }

            } except (AvExceptionFilter( GetExceptionInformation(), TRUE )) {

                return GetExceptionCode();
            }
            
            status = AvGetScanCtxSynchronized( scanId,
                                               &scanContext );
            
            if (!NT_SUCCESS( status )) {
            
                return STATUS_NOT_FOUND;
            }

            status = AvHandleCmdCloseSectionForDataScan( scanContext, scanResult );

            if (NT_SUCCESS(status)) {
                *ReturnOutputBufferLength = 0;
            }

            //
            //  AvGetScanCtxSynchronized incremented the ref count of scan context
            //
            AvReleaseScanContext( scanContext );
            
            break;
        
        case AvIsFileModified:
        
            try {
            
                hFile = ((PCOMMAND_MESSAGE) InputBuffer)->FileHandle;
                
            } except (AvExceptionFilter( GetExceptionInformation(), TRUE )) {

                return GetExceptionCode();
            }
        
            if ((OutputBufferSize < sizeof (BOOLEAN)) ||
                        (OutputBuffer == NULL)) {

                return STATUS_INVALID_PARAMETER;
            }

            if (!IS_ALIGNED(OutputBuffer,sizeof(BOOLEAN))) {

                return STATUS_DATATYPE_MISALIGNMENT;
            }

            //
            //  Get file object by file handle
            //  Get PFLT_VOLUME  by file object
            //  Get instance context by PFLT_VOLUME 
            //  Get filter instance in instance context
            //  Get stream context by file object and instance context
            //  Return if the file was previously modified
            //
            
            status = AvGetStreamContextByHandle( hFile, &streamContext );
            
            if (!NT_SUCCESS(status)) {
    
                AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[AV]: **************************AvGetStreamContextByHandle FAILED. \n") );
                break;
            }

            try {
                
                (*(PBOOLEAN) OutputBuffer) = (BOOLEAN) IS_FILE_MODIFIED( streamContext );
                *ReturnOutputBufferLength = (ULONG) sizeof( BOOLEAN );

            } except (AvExceptionFilter( GetExceptionInformation(), TRUE )) {

                status = GetExceptionCode();
            }
            
            FltReleaseContext( streamContext );
                        
            break;
            
        default:
            return STATUS_INVALID_PARAMETER;
    }
            
    return status;

}

NTSTATUS
AvPrepareServerPort( 
    _In_  PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_  AVSCAN_CONNECTION_TYPE  ConnectionType
    )
/*++

Routine Description:

    A wrapper function that prepare the communicate port.

Arguments:

    SecurityDescriptor - Specifies a security descriptor to InitializeObjectAttributes(...).

    ConnectionType - The type of connection: AvConnectForScan, AvConnectForAbort, AvConnectForQuery

Return Value:

    Returns the status of the prepartion.

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniString;
    LONG maxConnections = 1;
    PCWSTR portName = NULL;
    PFLT_PORT *pServerPort = NULL;
    
    PAGED_CODE();
    
    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                ("[AV]: AvPrepareServerPort entered. \n") );
    
    switch( ConnectionType ) {
        case AvConnectForScan:
            portName = AV_SCAN_PORT_NAME;
            pServerPort = &Globals.ScanServerPort;
            break;
        case AvConnectForAbort:
            portName = AV_ABORT_PORT_NAME;
            pServerPort = &Globals.AbortServerPort;
            break;
        case AvConnectForQuery:
            portName = AV_QUERY_PORT_NAME;
            pServerPort = &Globals.QueryServerPort;
            break;
        default:
            FLT_ASSERTMSG( "No such connection type.\n", FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    RtlInitUnicodeString( &uniString, portName );

    InitializeObjectAttributes( &oa,
                                &uniString,
                                OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                NULL,
                                SecurityDescriptor );

    status = FltCreateCommunicationPort( Globals.Filter,
                                         pServerPort,  // this is the output to server port.
                                         &oa,
                                         NULL,
                                         AvConnectNotifyCallback,
                                         AvDisconnectNotifyCallback,
                                         AvMessageNotifyCallback,
                                         maxConnections );
    
    return status;
}



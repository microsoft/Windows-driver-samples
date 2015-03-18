/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    scan.c

Abstract:

    This modules wraps the scanning routines. 

Environment:

    Kernel mode

--*/

#include "avscan.h"

//
//  Local routines prototypes.
//

AVSCAN_RESULT
AvScanMemoryStream(
    _In_reads_bytes_(Size)    PVOID    StartingAddress,
    _In_                      SIZE_T   Size,
    _In_                      PBOOLEAN OperationCanceled
    );
    
NTSTATUS
AvMapSectionAndScan(
    _Inout_ PAV_SECTION_CONTEXT SectionContext,
    _Out_ AVSCAN_RESULT *ScanResult
    );
    
//
//  Routine implementaions
//

AVSCAN_RESULT
AvScanMemoryStream(
    _In_reads_bytes_(Size)    PVOID    StartingAddress,
    _In_                      SIZE_T   Size,
    _In_                      PBOOLEAN OperationCanceled
    )
/*++

Routine Description

    A helper function to scan the memory starting at StartingAddress.
    This function is only called if the scan mode is AvKernelMode.    

Arguments
    
    StartingAddress - The starting memory address to be scanned.
    
    Size - The size of the memory to be scanned.
    
    OperationCanceled - In the scan loop, it is supposed to poll this flag,
            to see if the operation has been canceled.
            
Return Value

   The scan result
    
--*/
{
    UCHAR targetString[AV_DEFAULT_SEARCH_PATTERN_SIZE] = {0};
    SIZE_T searchStringLength = AV_DEFAULT_SEARCH_PATTERN_SIZE-1;
    ULONG ind;
    PUCHAR p;
    PUCHAR start = StartingAddress;
    PUCHAR end = start + Size - searchStringLength;
        
    //
    //  Decode the target pattern.
    //
    
    RtlCopyMemory( (PVOID) targetString, 
                   AV_DEFAULT_SEARCH_PATTERN, 
                   AV_DEFAULT_SEARCH_PATTERN_SIZE );
                   
    for (ind = 0;
         ind < searchStringLength;
         ind++) {
         
         targetString[ind] = ((UCHAR)targetString[ind]) ^ AV_DEFAULT_PATTERN_XOR_KEY;
    }
    targetString[searchStringLength] = '\0';
    
    //
    //  Scan the memory stream for the target pattern.
    //
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                 ("[Av]: ASMS: %p, %p, %llu, %llu\n",
                  start,
                  end,
                  Size,
                  searchStringLength) );

    for (p = start; p <= end;  p++) {

        // if not canceled, continue to search for pattern
        if((*OperationCanceled)) { 

            return AvScanResultUndetermined;
        }
        
        if (RtlEqualMemory( p, targetString, searchStringLength )) {

            return AvScanResultInfected;
        }
    }
    
    *OperationCanceled = FALSE;  // Reset the cancel flag, after breaks out the loop.

    return AvScanResultClean;
}

NTSTATUS
AvMapSectionAndScan(
    _Inout_ PAV_SECTION_CONTEXT SectionContext,
    _Out_ AVSCAN_RESULT *ScanResult
    )
/*++

Routine Description

    A helper function to map the section object and scan the mapped memory.

Arguments
    
    SectionContext - Section context containing section object and handle.
            
    Infected - Return TRUE if the file is infected.

Return Value

    Returns the status of this operation.
    
--*/
{
    NTSTATUS status;
    CLIENT_ID clientId;
    OBJECT_ATTRIBUTES objAttribs;
    HANDLE processHandle = NULL;
    PVOID scanAddress = NULL;
    SIZE_T scanSize = 0;
    AVSCAN_RESULT scanResult;
    
    clientId.UniqueThread = PsGetCurrentThreadId();
    clientId.UniqueProcess = PsGetCurrentProcessId();

    InitializeObjectAttributes(&objAttribs,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL );            

    status = ZwOpenProcess( &processHandle,
                            PROCESS_ALL_ACCESS,
                            &objAttribs,
                            &clientId );

    if (!NT_SUCCESS( status )) {
    
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                 ("[Av]: AvMapSectionAndScan: Failed to open the process, 0x%08x\n",
                  status) );
        goto Cleanup;
    }

    status = ZwMapViewOfSection( SectionContext->SectionHandle,
                                 processHandle,
                                 &scanAddress,
                                 0,
                                 0,
                                 NULL,
                                 &scanSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READONLY );
    if (!NT_SUCCESS(status)) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                 ("[Av]: AvMapSectionAndScan: Failed to map the view of the section, 0x%08x\n",
                  status) );
    
        goto Cleanup;
    }
    
    //
    //  The size here may have truncation.
    //
    scanResult = AvScanMemoryStream( scanAddress, 
                                     (SIZE_T)min((LONGLONG)scanSize, SectionContext->FileSize),
                                     &SectionContext->Aborted );

    *ScanResult = scanResult;
    
Cleanup:
    
    if (scanAddress != NULL) {

        ZwUnmapViewOfSection( processHandle, scanAddress );
    }
    
    if (processHandle != NULL) {
                    
        ZwClose( processHandle );
    }

    return status;
}

NTSTATUS
AvScanInKernel (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ UCHAR IOMajorFunctionAtScan,
    _In_ BOOLEAN IsInTxWriter,
    _In_ PAV_STREAM_CONTEXT StreamContext
    )
/*++

Routine Description

    This function is a high level function which 
    will do the kernel-mode data scan.

Arguments

    FltObjects - related objects for the IO operation.

    IOMajorFunctionAtScan - The major function of the IRP that issues this scan.

    IsInTxWriter - If this file is enlisted in a transacted writer.

    StreamContext - The stream context of this data stream.

Return Value

    Returns the status of this operation.
    
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES objAttribs;
    PAV_SECTION_CONTEXT sectionContext;
    AVSCAN_RESULT scanResult = AvScanResultUndetermined;
    
    status = AvCreateSectionContext( FltObjects->Instance, 
                                     FltObjects->FileObject, 
                                     &sectionContext );
    
    if (!NT_SUCCESS( status )) {
    
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
              ("[AV] AvScanInKernel: failed to create section context.\n") );
    
        return status;
    }
    
    sectionContext->CancelableOnConflictingIo = (IOMajorFunctionAtScan == IRP_MJ_CLEANUP);

    InitializeObjectAttributes(&objAttribs,
                               NULL,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL );
    
    status = FltCreateSectionForDataScan( FltObjects->Instance,
                                          FltObjects->FileObject,
                                          sectionContext,
                                          SECTION_MAP_READ,
                                          &objAttribs,
                                          NULL,
                                          PAGE_READONLY,
                                          SEC_COMMIT,
                                          0,
                                          &sectionContext->SectionHandle,
                                          &sectionContext->SectionObject,
                                          NULL );
    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                 ("[Av]: AvScanInKernel: Failed to create section for data scan.\n, 0x%08x\n",
                  status) );
        return status;
    }
    
    status = AvMapSectionAndScan( sectionContext, &scanResult );
    
    if (!NT_SUCCESS( status )) {

        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                 ("[Av]: AvScanInKernel: Failed to scan the view of the section.\n, 0x%08x\n",
                  status) );
    }

    if (scanResult == AvScanResultClean) {
    
            AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
              ("[AV] AvScanInKernel: file %I64x,%I64x is CLEAN!!\n", 
               StreamContext->FileId.FileId64.UpperZeroes,
               StreamContext->FileId.FileId64.Value) );
               
            SET_FILE_NOT_INFECTED_EX( IsInTxWriter, StreamContext );

    } else if (scanResult == AvScanResultInfected) {

        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
          ("[AV] AvScanInKernel: file %I64x,%I64x is INFECTED!!\n", 
           StreamContext->FileId.FileId64.UpperZeroes,
           StreamContext->FileId.FileId64.Value) );

        SET_FILE_INFECTED_EX( IsInTxWriter, StreamContext );
        
    } else {

        AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
          ("[AV] AvScanInKernel: file %I64x,%I64x is UNKNOWN!!\n", 
           StreamContext->FileId.FileId64.UpperZeroes,
           StreamContext->FileId.FileId64.Value) );

        SET_FILE_UNKNOWN_EX( IsInTxWriter, StreamContext );
    }
    
    status = AvFinalizeSectionContext(sectionContext);
    
    return status;
}

NTSTATUS
AvScanInUser (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ UCHAR IOMajorFunctionAtScan,
    _In_ BOOLEAN IsInTxWriter,
    _In_ DEVICE_TYPE DeviceType
    )
/*++

Routine Description

    This function is a high level function which 
    will do the user-mode data scan.

Arguments

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - related objects for the IO operation.
    
    IOMajorFunctionAtScan - The major function of the IRP that issues this scan.
    
    IsInTxWriter - If this file is enlisted in a transacted writer.
    
    StreamContext - The stream context of this data stream.

Return Value

    Returns the status of this operation.
    
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG scanThreadId;
    ULONG replyLength = sizeof(ULONG);
    PAV_SCAN_CONTEXT scanCtx = NULL;
    AV_SCANNER_NOTIFICATION notification = {0};
    LONGLONG _1ms = 10000;    
    LARGE_INTEGER timeout = {0};
    
    status = AvAllocateScanContext(FltObjects->Instance,
                                   FltObjects->FileObject,
                                   &scanCtx);
    if (!NT_SUCCESS(status)) {
        
        return status;
    }

    //
    //  Scan context is passed to the user service program.
    //  Initialize it here.
    //
    
    KeInitializeEvent( &scanCtx->ScanCompleteNotification, NotificationEvent, FALSE );
    scanCtx->IOMajorFunctionAtScan = IOMajorFunctionAtScan;
    scanCtx->IsFileInTxWriter = IsInTxWriter;
    scanCtx->SectionContext = NULL;
    
    AvAcquireResourceExclusive( &Globals.ScanCtxListLock );
    if (Globals.Unloading) {
        //
        //  If the filter is being unloaded, we failed the scan.
        //
        AvReleaseResource( &Globals.ScanCtxListLock );
        AvReleaseScanContext( scanCtx );
        
        return STATUS_FLT_DELETING_OBJECT;
    }
    scanCtx->ScanId = (++Globals.ScanIdCounter);
    InsertTailList (&Globals.ScanCtxListHead, &scanCtx->List);
    AvReleaseResource( &Globals.ScanCtxListLock );

    //
    //  Tell the user-scanner to start to scan the file
    //
    
    notification.Message = AvMsgStartScanning;
    notification.ScanId = scanCtx->ScanId;
    notification.Reason = AvScanOnOpen;
    
    if (IOMajorFunctionAtScan == IRP_MJ_CLEANUP) {
        notification.Reason = AvScanOnCleanup;
    }

    //
    //  Set the scan timeout for this file based on if it is a local or
    //  network file.  These values can come from the registry.
    //

    if (DeviceType == FILE_DEVICE_NETWORK) {
        timeout.QuadPart = Globals.NetworkScanTimeout;
    } else {
        timeout.QuadPart = Globals.LocalScanTimeout;
    }    

    timeout.QuadPart = -(timeout.QuadPart * _1ms);

    status = FltSendMessage( Globals.Filter,
                             &Globals.ScanClientPort,
                             &notification,
                             sizeof(AV_SCANNER_NOTIFICATION),
                             &scanThreadId,
                             &replyLength,
                             &timeout );
    //
    //  If the message is not delievered or time-out, we can make sure that
    //  the scanner thread did not acknowledged this scan task, and thus
    //  we can safely remove it from the list.
    //
    if (!NT_SUCCESS( status ) || status == STATUS_TIMEOUT) {
    
        if ((status != STATUS_PORT_DISCONNECTED) &&
            (status != STATUS_TIMEOUT)) {

            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                    ("[Av]: AvScanInUser: Failed to FltSendMessage.\n, 0x%08x\n",
                    status) );
        }
        goto Cleanup;
    }

    scanCtx->ScanThreadId = scanThreadId;
                                    
    //
    //  Wait for an event that the scanner completes or aborts.
    //

    status = FltCancellableWaitForSingleObject( &scanCtx->ScanCompleteNotification, 
                                                &timeout,
                                                Data );

    if (!NT_SUCCESS(status) ||
        (status == STATUS_TIMEOUT)) {

        //
        //  At this point we came out of the wait with an error. We are in one of the following conditions:
        //
        //  1. This thread is being terminated
        //  2. The IO operation represented by Data was cancelled
        //  3. If we are in user-mode scan mode, the communication to the user mode component timed out
        //  4. If we are in user-mode scan mode, the user-mode component died and the wait timed out.
        //

        NTSTATUS statusAbort = STATUS_SUCCESS;
        //
        //  Notify the user scan thread to abort the scan.
        //
        statusAbort = AvSendAbortToUser(scanCtx->ScanThreadId,
                                        scanCtx->ScanId);
        if (NT_SUCCESS(statusAbort) &&
            (statusAbort != STATUS_TIMEOUT)) {

            LARGE_INTEGER timeoutForAbortComplete = {0};
            timeoutForAbortComplete.QuadPart = - 1000 * (LONGLONG)_1ms;  // 1s
            //
            //  Wait again on completion notification.
            //  The scan thread should close the section very soon because we have already notified
            //  the scan thread to abort the task.
            //
            statusAbort = FltCancellableWaitForSingleObject( 
                                              &scanCtx->ScanCompleteNotification, 
                                              &timeoutForAbortComplete,
                                              NULL );
        }
        //
        //  If send abortion failed or wait failed, which general means the service is dead, 
        //  we have to close section context/handle here by ourself.
        //
        if (!NT_SUCCESS(statusAbort) ||
            (statusAbort == STATUS_TIMEOUT)) {

            scanCtx->IoWaitOnScanCompleteNotificationAborted = TRUE;
            //
            // If this thread who the race, it will close the section.
            //
            AvFinalizeScanAndSection(scanCtx);
        }
    }

    //
    //  If the wait for scan to complete is cancelled (e.g. by CancelSynchronousIo )
    //
    if (!NT_SUCCESS(status) &&
        (IOMajorFunctionAtScan == IRP_MJ_CREATE)) {

        AvCancelFileOpen(Data, FltObjects, status);
    }

Cleanup:

    //
    //  Here scanCtx must be non-NULL because we checked it in the beginning.
    //
    
    AvAcquireResourceExclusive( &Globals.ScanCtxListLock );
    RemoveEntryList (&scanCtx->List);
    AvReleaseResource( &Globals.ScanCtxListLock );

    AvReleaseScanContext( scanCtx );
    
    return status;
}


NTSTATUS
AvCloseSectionForDataScan( 
    _Inout_ PAV_SECTION_CONTEXT SectionContext
    )
/*++

Routine Description

    A wrapper function that wraps FltCloseSectionForDataScan and performs appropriate cleanup.

Arguments
    
    SectionContext - The seciton handle and object will be cleaned up in sectino context.

Return Value

    Returns the status of this operation.
    
--*/
{
    //
    //  Synchronized with AvScanAbortCallbackAsync(...)
    //
    InterlockedExchangePointer( &SectionContext->ScanContext, NULL );
    ObDereferenceObject( SectionContext->SectionObject );

    SectionContext->SectionHandle = NULL;
    SectionContext->SectionObject = NULL;
    return FltCloseSectionForDataScan( (PFLT_CONTEXT)SectionContext );
}


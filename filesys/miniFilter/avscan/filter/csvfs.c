/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    csvfs.c

Abstract:

    This is the csvfs specific module of the avscan mini-filter driver.
    This filter demonstrates how to implement a transaction-aware 
    anti-virus filter.
    
    Av prefix denotes "Anti-virus" module.


    CSVFS is a distributed file system where multiple nodes in a
    cluster can expose the same volume namespace at the same time. To
    achieve this the CSVFS volumes on each node act as a proxy for an
    underlying NTFS volume where the NTFS volume runs only on one node
    of the cluster. It is this NTFS volume where the file data and file
    system metadata are stored. The node that exposes this NTFS volume
    is the coordinator node. The underlying NTFS volume is made hidden
    to keep applications from using it directly. All applications
    should use the CSVFS namespace.

    CSVFS implements functionality known as Direct I/O. This
    functionality allow the CSVFS proxy on each node to directly read
    or write to the blocks on the disk and bypass the NTFS volume on the
    coordinator node when it is safe to do so. A filter on node node
    needs to be aware that it may not see all I/O to a file. 

    In the CSVFS enviornment filters can layer on the CSVFS volume
    stack, the hidden NTFS volume stack on the coordinator node and
    also on the MUP stack. When on the MUP and the hidden NTFS stack
    the filter should not scan any files that are opened with
    GUID_ECP_CSV_DOWN_LEVEL_OPEN ECP attached. This ECP is used by
    CSVFS for its internal file opens and should be ignored by filters.
    Filters that layer on the CSVFS would be the components that scan
    the files.

    It is recommended that filters be extremely careful when layered on
    the hidden NTFS stack:

        Management issues
            Volume is hidden and thus does not have volume guid,
            mountpoint or drive letter that can be used to
            represent this volume to the user in a command line or UI.
            
            The coordinator node for a CSVFS volume can move to another
            node at any time. This would cause challenges with
            maintaining filter configuration setttings and also be
            challenging for the admin to know which node should be used.
            
        Interop with CSVFS issues
            Changing file sizes in a filter layered above the hidden
            NTFS volume may cause data corruption.
            
            Building and keeping a mapped section on hidden NTFS volume
            might prevent direct IO from happening
            
            Building and keeping mapped section on hidden NTFS volume
            might lead to cache coherency issues (stale cache) and
            eventually to data corruption.

    Note that there is a feasible model for creating a distributed
    filter with instances on the CSVFS and the hidden NTFS volume
    stacks. The instance on the hidden NTFS volume would serve as a
    centralized filter meta-data server where information shared
    between node could be maintained. Each instance on CSVFS volume
    would communicate with the hidden NTFS volume stack through a
    downlevel file handle.

    CSVFS provides a very simple centralized meta-data cache which
    contains revision numbers for tracking changes to files. This
    sample shows how those revision numbers can be used to determine if
    a file has been modified by another node and thus if it needs to be
    scanned locally. 

    Other things to be aware of:
    
    Filter should not have *any* global locks or else deadlock may
    occur. File system requests will flow through the CSVFS proxy file
    system and then be forwarded to the hidden NTFS file system. If the
    filter is layered on both file systems and the filter instance on the
    CSVFS volume takes a lock as the request passes through, when the
    request is forwarded and reaches the filter instance on the hidden
    NTFS or MUP volume, it will attempt to acquire the lock and deadlock will
    occur.

    Do not make assumption that buffered IO will always go to cache and
    eventually you will see paging IO. CsvFs, like RDR, does caching based
    on the oplock it was able to get for this stream. For instance if a
    file is being accessed from multiple nodes CsvFs would have oplock
    level RH or R or none and all cached writes to the file will be sent
    directly to NTFS without going to CC. You can think about that as if
    from the perspective of a filter sitting above CSVFS cached IO is being
    handles as if is it un buffered IO. Since in general filters would
    not know when CsvFs loses/gains oplocks they should not make an
    assumption that cached IO will go through CC.   


Environment:

    Kernel mode

--*/

#include "avscan.h"
#include <ntdddisk.h>
    
/*************************************************************************
    Local Function Prototypes
*************************************************************************/
NTSTATUS
AvAddCsvRevisionECP (
    _Inout_ PFLT_CALLBACK_DATA Data
    );

NTSTATUS
AvReadCsvRevisionECP (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _Out_ LONGLONG *VolumeRevision,
    _Out_ LONGLONG *CacheRevision,
    _Out_ LONGLONG *FileRevision
    );

NTSTATUS
AvQueryCsvRevisionNumbers (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ LONGLONG *VolumeRevision,
    _Out_ LONGLONG *CacheRevision,
    _Out_ LONGLONG *FileRevision
    );

NTSTATUS
AvFindAckedECP (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ LPCGUID EcpGuid,
    _Out_ PVOID *Ecp,
    _Out_ ULONG *EcpSize
    );
    
//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AvIsVolumeOnCsvDisk)
#pragma alloc_text(PAGE, AvIsCsvDlEcpPresent)
#pragma alloc_text(PAGE, AvPreCreateCsvfs)
#pragma alloc_text(PAGE, AvPostCreateCsvfs)
#pragma alloc_text(PAGE, AvPreCleanupCsvfs)
#pragma alloc_text(PAGE, AvQueryCsvRevisionNumbers)
#pragma alloc_text(PAGE, AvReadCsvRevisionECP)
#pragma alloc_text(PAGE, AvAddCsvRevisionECP)
#pragma alloc_text(PAGE, AvFindAckedECP)
#endif

BOOLEAN
AvIsVolumeOnCsvDisk (
    _In_ PFLT_VOLUME Volume
    )
    /*++

    Routine Description:

        This routine checks if the volume indicated by Volume belongs to a disk that is CSV or not.

    Arguments:

        Volume - Pointer to the FLT_VOLUME.

    Return Value:

        The return value is TRUE or FALSE.

    --*/
{

    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN retValue = FALSE;
    PDEVICE_OBJECT disk = NULL, refDeviceObject=NULL;
    PIRP irp;
    IO_STATUS_BLOCK iosb;  
    ULONG controlCode = IOCTL_DISK_GET_CLUSTER_INFO;
    DISK_CLUSTER_INFO outBuf;
    KEVENT event;    

    PAGED_CODE();
    
    status = FltGetDiskDeviceObject(Volume, &disk);
    if (!NT_SUCCESS(status)) {
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                      ("Failed to get disk object from volume, status 0x%x\n", status) );
        goto Cleanup;
    }    

    refDeviceObject = IoGetAttachedDeviceReference(disk);

    iosb.Information = 0;
    RtlZeroMemory(&outBuf, sizeof(outBuf));
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    irp = IoBuildDeviceIoControlRequest( controlCode,
                                         refDeviceObject,
                                         NULL,
                                         0,
                                         &outBuf,
                                         sizeof(outBuf),
                                         FALSE,
                                         &event,
                                         &iosb );
    if (irp == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("Failed to allocate Irp, status 0x%x\n", status) );
        goto Cleanup;
    }

    status = IoCallDriver( refDeviceObject, irp );
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        status = iosb.Status;        
    }

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                 ("DeviceIoControl returned status 0x%x\n", status) );

    if(!NT_SUCCESS( status )) {
        goto Cleanup;
    }

    retValue = FlagOn( outBuf.Flags, DISK_CLUSTER_FLAG_CSV ) ? TRUE : FALSE;
    if (FlagOn( outBuf.Flags, DISK_CLUSTER_FLAG_CSV) && FlagOn(outBuf.Flags, DISK_CLUSTER_FLAG_IN_MAINTENANCE )) {
        //
        // A CSV disk can be in maintenance mode. When in maintenance
        // mode the CSV namespace is no longer exposed across the
        // entire cluster but instead only exposed on the single node
        // where the NTFS volume is exposed. In this case the filter
        // should treat the volume as it would any other NTFS volume
        //
        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                     ("Disk is CSV but in Maintenance\n") );
        retValue = FALSE;
    }

    if(retValue == TRUE) {
        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                     ("Disk is CSV\n") );
    }
    else {
        AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                      ("Disk is not CSV\n") );
    }
    
Cleanup:

    if (refDeviceObject) {
        KeEnterCriticalRegion();
        ObDereferenceObject( refDeviceObject );
        refDeviceObject = NULL;
        KeLeaveCriticalRegion();
    }
  
    if (disk) {
        ObDereferenceObject( disk );
        disk = NULL;
    }        
        
    return retValue;
}

NTSTATUS 
AvAddCsvRevisionECP (
    _Inout_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This routine will include the Extra Create Parameter (ECP) that is
    used on CSVFS file systems. This ECP will return the set of
    revision numbers that are associated with the file being opened.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;
    PECP_LIST  ecpList = NULL;
    PCSV_QUERY_FILE_REVISION_ECP_CONTEXT ecpContext;

    PAGED_CODE();
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvAddCsvRevisionECP: Entered\n") );
    
    status = FltGetEcpListFromCallbackData( Globals.Filter,
                                            Data,
                                            &ecpList );
    if (!NT_SUCCESS( status )) {
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                      ("[AV] AvAddCsvRevisionECP: FltGetEcpListFromCallbackData failed 0x%x\n", status) );
        goto Cleanup;
    }
    
    if (ecpList == NULL) {
        //
        // Create a new ecplist.
        //
        status = FltAllocateExtraCreateParameterList( Globals.Filter, 0, &ecpList );
        if (!NT_SUCCESS(status)) {
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                          ("[AV] AvAddCsvRevisionECP: FltAllocateExtraCreateParameterList failed 0x%x", status) );
            goto Cleanup;
        }
        //
        // Set it into CBD.
        //
        status = FltSetEcpListIntoCallbackData( Globals.Filter, Data, ecpList );
        if (!NT_SUCCESS(status)) {
            AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                          ("[AV] AvAddCsvRevisionECP: FltSetEcpListIntoCallbackData failed 0x%x", status) );
            FltFreeExtraCreateParameterList( Globals.Filter, ecpList );
            goto Cleanup;
        }

    } else {
        //
        // See if the ECP has already been added to the ECP list
        // already.
        //
        status = FltFindExtraCreateParameter( Globals.Filter,
                                              ecpList,
                                              &GUID_ECP_CSV_QUERY_FILE_REVISION,
                                              NULL,
                                              NULL );
        if (status != STATUS_NOT_FOUND) {
            goto Cleanup;
        }

    }
    
    status = FltAllocateExtraCreateParameter( Globals.Filter,
                                              &GUID_ECP_CSV_QUERY_FILE_REVISION,
                                              sizeof(CSV_QUERY_FILE_REVISION_ECP_CONTEXT),
                                              0,
                                              NULL,
                                              AV_SCAN_CTX_TAG,
                                              &ecpContext );
                                             
    if (!NT_SUCCESS(status)) {
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
            ("[AV] AvAddCsvRevisionECP: FltAllocateExtraCreateParameterFromLookasideList failed 0x%x\n", status) );
        goto Cleanup;
    }

    RtlZeroMemory( ecpContext, sizeof(CSV_QUERY_FILE_REVISION_ECP_CONTEXT ));
    status = FltInsertExtraCreateParameter( Globals.Filter,
                                            ecpList,
                                            ecpContext );
    if (!NT_SUCCESS(status)) {
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                      ("[AV] AvAddCsvRevisionECP: FltInsertExtraCreateParameter failed 0x%x\n", status) );
        FltFreeExtraCreateParameter( Globals.Filter, ecpContext );
        goto Cleanup;
    }

Cleanup:

    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvAddCsvRevisionECP: Leave\n") );
    
    return status;
}

NTSTATUS 
AvFindAckedECP (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ LPCGUID EcpGuid,
    _Out_ PVOID *Ecp,
    _Out_ ULONG *EcpSize
    )
/*++

Routine Description:

    This routine will find the Extra Create Parameter (ECP) and if it
    exists then check if it has been acknowledged

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    EcpGuid - Pointer to the guid that represents the ECP to find

    *Ecp - returns with a pointer to the ECP data
    
    *EcpSize - returns with the size of the ECP data


Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;
    PECP_LIST  ecpList = NULL;
    PVOID ecpContext = NULL;
    ULONG ecpContextSize = 0; 
    
    PAGED_CODE();
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvFindAckedECP: Entered\n") );

    status = FltGetEcpListFromCallbackData( Globals.Filter,
                                            Data,
                                            &ecpList);
    if (NT_SUCCESS(status)) {

        if (ecpList != NULL) {
            
            status = FltFindExtraCreateParameter( Globals.Filter,
                                                  ecpList,
                                                  EcpGuid,
                                                  &ecpContext,
                                                  &ecpContextSize);

            if (NT_SUCCESS(status)) {

                if (FltIsEcpAcknowledged( Globals.Filter, ecpContext )) {
                    *Ecp = ecpContext;
                    *EcpSize = ecpContextSize;
                } else {
                    status = STATUS_UNSUCCESSFUL;
                }
            }
            
        } else {
            status = STATUS_UNSUCCESSFUL;
        }            
    }
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvFindAckedECP: leave 0x%x\n", status) );

    return status;
}

NTSTATUS
AvReadCsvRevisionECP (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _Out_ LONGLONG *VolumeRevision,
    _Out_ LONGLONG *CacheRevision,
    _Out_ LONGLONG *FileRevision
    )
/*++

Routine Description:

    This routine will read the Extra Create Parameter (ECP) that is
    returned on CSVFS file systems. This ECP returns the set of
    revision numbers that are associated with the file being opened.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    *VolumeRevision returns with the volume revision number

    *CacheRevision returns with the cache revision number

    *FileRevision returns with the file revision number

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;
    PCSV_QUERY_FILE_REVISION_ECP_CONTEXT ecpContext = NULL;
    ULONG ecpContextSize = 0; 
    
    PAGED_CODE();
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvReadCsvRevisionECP: Entered\n") );


    status = AvFindAckedECP( Data,
                             &GUID_ECP_CSV_QUERY_FILE_REVISION,
                             &ecpContext,
                             &ecpContextSize );
                       
    if (NT_SUCCESS( status )) {    
        *VolumeRevision = ecpContext->FileRevision[0];
        *CacheRevision = ecpContext->FileRevision[1];
        *FileRevision = ecpContext->FileRevision[2];
    }
    
    AV_DBG_PRINT( AVDBG_TRACE_ROUTINES,
                  ("[AV] AvReadCsvRevisionECP: leave 0x%x\n", status) );

    return status;
}

NTSTATUS
AvQueryCsvRevisionNumbers (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ LONGLONG *VolumeRevision,
    _Out_ LONGLONG *CacheRevision,
    _Out_ LONGLONG *FileRevision
    )
/*++

Routine Description:

    Obtain the most updated revision numbers for a file on CSVFS

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    *VolumeRevision returns with the volume revision number
    
    *CacheRevision returns with the cache revision number
    
    *FileRevision returns with the file revision number
    
Return Value:

    The return value is the status of the operation.

--*/                                   
{
    NTSTATUS status;
    CSV_CONTROL_PARAM request;
    CSV_QUERY_FILE_REVISION revision;
    ULONG bytesReturned;

    PAGED_CODE();

    RtlZeroMemory( &request, sizeof( request ) );
    request.Operation = CsvControlQueryFileRevision;    
    
    status = FltFsControlFile( FltObjects->Instance,
                               FltObjects->FileObject,
                               FSCTL_CSV_CONTROL,
                               &request,
                               sizeof(request),
                               &revision,
                               sizeof(revision),
                               &bytesReturned );

    if (NT_SUCCESS( status )) {
        *VolumeRevision = revision.FileRevision[0];
        *CacheRevision = revision.FileRevision[1];
        *FileRevision = revision.FileRevision[2];
    }
    
    return status;
}

BOOLEAN
AvIsCsvDlEcpPresent (
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CALLBACK_DATA Data
     )
/*++

Routine Description:

    This local function will determine if there is a CSVFS downlevel
    ECP attached. 
    
Arguments:

    Filter - Pointer to the filter structure
    
    Data - Pointer to the filter callbackData that is passed to us.
    
Return Value:

    TRUE - CSVFS downlevel ECP is present
    FALSE - CSVFS downlevel ECP is not present or an error occured

--*/
{
    NTSTATUS status;
    PECP_LIST ecpList;
    PVOID ecpContext;
    
    PAGED_CODE();

    status = FltGetEcpListFromCallbackData( Filter, Data, &ecpList );

    if (NT_SUCCESS(status) && (ecpList != NULL)) {
    
        status = FltFindExtraCreateParameter( Filter,
                                              ecpList,
                                              &GUID_ECP_CSV_DOWN_LEVEL_OPEN,
                                              &ecpContext,
                                              NULL );

        if (NT_SUCCESS(status)) {

            return TRUE;                          
        }
    }

    return FALSE;
}

NTSTATUS
AvPreCreateCsvfs (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects
    )
/*++

Routine Description:

    This function implements the PreCreate processing associated with a
    CSVFS volume. The work done is to include the file revision ECP
    into the ECP list so that CSVFS will return the revision numbers
    when the create completes.
    
Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PAV_INSTANCE_CONTEXT instanceContext = NULL;
    
    PAGED_CODE();
    
    status = FltGetInstanceContext( FltObjects->Instance,
                                    &instanceContext );
                                    
    if (NT_SUCCESS( status )) {

        if (instanceContext->VolumeFSType == FLT_FSTYPE_CSVFS) {
            //
            // Add ECP to retrieve the revision numbers
            //
            status = AvAddCsvRevisionECP( Data );

            //
            // we don't worry if this fails since if we do not get the
            // revision numbers then we just assume they have changed
            //
        }

        FltReleaseContext( instanceContext );
    } else {
        //
        // If unable to get instance context then it is no problem. It
        // means that the revision numbers won't be returned from
        // CSVFS. It is not fatal but will result in the file being
        // rescanned.
        //
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvPreCreateCsvfs: FltGetInstanceContext failed. status = 0x%x\n", status) );
    }
    
    return status;
}

NTSTATUS
AvPostCreateCsvfs (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PAV_STREAM_CONTEXT StreamContext,
    _Out_ BOOLEAN *UpdateRevisionNumbers,
    _Out_ LONGLONG *VolumeRevisionPtr,
    _Out_ LONGLONG *CacheRevisionPtr,
    _Out_ LONGLONG *FileRevisionPtr               
    )
/*++

Routine Description:

    This function implements the PostCreate processing associated with a
    CSVFS volume. The work done is to determine if the revision numbers
    have been returned and if so determine if a rescan of the file is
    needed. A recan would be needed if the revision numbers are not
    able to be retrieved, are not valid or do not match the previously
    recorded revision numbers.
    
Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the AV stream context

    *UpdateRevisionNumbers returns TRUE if the revision numbers should
        be updated in the stream context upon a successful scan

    *VolumeRevisionPtr return with the updated volume revision number
    
    *CacheRevisionPtr return with the updated cache revision number
    
    *FileRevisionPtr return with the updated file revision number
    
Return Value:

    Status

--*/
{
    PAV_INSTANCE_CONTEXT instanceContext = NULL;
    NTSTATUS status;
    LONGLONG VolumeRevision = 0, CacheRevision = 0, FileRevision = 0;
    BOOLEAN needRescanOnCsvfs = FALSE;

    PAGED_CODE();
    
    *UpdateRevisionNumbers = FALSE;
    *VolumeRevisionPtr = 0;
    *CacheRevisionPtr = 0;
    *FileRevisionPtr = 0;         
    
    status = FltGetInstanceContext( FltObjects->Instance,
                                    &instanceContext );
                                    
    if (NT_SUCCESS( status )) {

        if (instanceContext->VolumeFSType == FLT_FSTYPE_CSVFS) {
            
            //
            // Read ECP to retrieve the revision numbers
            //
            status = AvReadCsvRevisionECP( Data,
                                           &VolumeRevision,
                                           &CacheRevision,
                                           &FileRevision);

            if (NT_SUCCESS( status )) {
                AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                              (" [AV] AvPostCreateCsvfs: %I64x:%I64x:%I64x\n",
                               VolumeRevision,
                               CacheRevision,
                               FileRevision) );

                //
                // It is very possible that the file was changed by
                // another node in the cluster and so we need to check
                // this case. So if any of the revision number have
                // changed we need to assume that the file has changed.
                // Note that this is a very pessimistic assumption as
                // the Volume and Cache revision numbers could change
                // without a corresponding file change but rescanning
                // when these change will ensure that a file changed
                // on another node will not be opened without being
                // rescanned on this node.
                //
                // Also note that there are cases where the revision
                // numbers are zero and thus not at all valid. In this
                // case the file must be rescanned as it is not known
                // if it was changed or not.
                //
                needRescanOnCsvfs = ( (VolumeRevision == 0) ||
                                      (CacheRevision == 0) ||
                                      (FileRevision == 0) ||
                                      (VolumeRevision != StreamContext->VolumeRevision) ||
                                      (CacheRevision != StreamContext->CacheRevision) ||
                                      (FileRevision != StreamContext->FileRevision) );
            } else {
                AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                              (" [AV] AvPostCreateCsvfs: Status 0x%x from AvReadCsvRevisionECP\n", status) );

                //
                // In this case the revision numbers are not available.
                // Since there is no way to know if the file was
                // changed on another node, it is safest to rescan.
                //
                needRescanOnCsvfs = TRUE;
            }
        }
        FltReleaseContext( instanceContext );
    } else {
        //
        // If unable to get instance context then the code isn't sure
        // if it is on a CSVFS volume or not. The safest assumption
        // would be to rescan the file.
        //
        needRescanOnCsvfs = TRUE;
        
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvPostCreateCsvfs: FltGetInstanceContext failed. status = 0x%x\n", status) );
    }

    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                  ("[Av]: AvPostCreateCsvfs: %ws need rescan\n",
                  needRescanOnCsvfs ? L"Does" : L"Does not")
                  );

    
    //
    // if it has been determined that a rescan is needed then set the
    // file modified flag on the stream context to indicate this
    //
    if (needRescanOnCsvfs) {
        if ( StreamContext->TxContext != NULL) {

            //
            //  Instead of updating State, we update TxState here, 
            //  because the file is part of a transaction writer
            //

            SET_FILE_TX_MODIFIED( StreamContext );

        } else {

            SET_FILE_MODIFIED( StreamContext );
        }

        //
        // If CSVFS has provided us with valid revison numbers then
        // return them to the caller so it can update them in the
        // stream context if the scan is successful.
        //
        if ((VolumeRevision != 0) &&
            (CacheRevision != 0) &&
            (FileRevision != 0)) {
            *UpdateRevisionNumbers = TRUE;
            *VolumeRevisionPtr = VolumeRevision;
            *CacheRevisionPtr = CacheRevision;
            *FileRevisionPtr = FileRevision;
        }
    }
    
    return status;
}

NTSTATUS
AvPreCleanupCsvfs (
    _Unreferenced_parameter_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PAV_STREAM_CONTEXT StreamContext,
    _Out_ BOOLEAN *UpdateRevisionNumbers,
    _Out_ LONGLONG *VolumeRevisionPtr,
    _Out_ LONGLONG *CacheRevisionPtr,
    _Out_ LONGLONG *FileRevisionPtr               
    )
/*++

Routine Description:

    This function implements the PreCleanup processing associated with a
    CSVFS volume. The work done is to retrieve the current revision
    numbers and determine if a rescan of the file is needed. A recan
    would be needed if the revision numbers are not
    able to be retrieved, are not valid or do not match the previously
    recorded revision numbers.
    
Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    StreamContext - Pointer to the AV stream context
    
    *UpdateRevisionNumbers returns TRUE if the revision numbers should
        be updated in the stream context upon a successful scan

    *VolumeRevisionPtr return with the updated volume revision number
    
    *CacheRevisionPtr return with the updated cache revision number
    
    *FileRevisionPtr return with the updated file revision number
        
Return Value:

    Status

--*/
{
    PAV_INSTANCE_CONTEXT instanceContext = NULL;
    NTSTATUS status;
    LONGLONG VolumeRevision = 0, CacheRevision = 0, FileRevision = 0;
    BOOLEAN needRescanOnCsvfs = FALSE;

    UNREFERENCED_PARAMETER( Data );

    PAGED_CODE();

    *UpdateRevisionNumbers = FALSE;
    *VolumeRevisionPtr = 0;
    *CacheRevisionPtr = 0;
    *FileRevisionPtr = 0;         
    
    status = FltGetInstanceContext( FltObjects->Instance,
                                    &instanceContext );
                                    
    if (NT_SUCCESS( status )) {

        if (instanceContext->VolumeFSType == FLT_FSTYPE_CSVFS) {
            
            //
            // If this file is on CSVFS then we cannot completely rely
            // upon tracking if the file is modified only on this node,
            // but also need to track if the file has been modified on
            // any node. So query for the updated revision numbers to
            // see if a scan is needed.
            //
            status = AvQueryCsvRevisionNumbers( FltObjects,
                                                &VolumeRevision,
                                                &CacheRevision,
                                                &FileRevision );

            if (NT_SUCCESS( status )) {
                AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                              (" [AV] AvPreCleanupCsvfs: %I64x:%I64x:%I64x\n",
                               VolumeRevision,
                               CacheRevision,
                               FileRevision) );

                //
                // It is very possible that the file was changed by
                // another node in the cluster and so we need to check
                // this case. So if any of the revision number have
                // changed we need to assume that the file has changed.
                // Note that this is a very pessimistic assumption as
                // the Volume and Cache revision numbers could change
                // without a corresponding file change but rescanning
                // when these change will ensure that a file changed
                // on another node will not be opened without being
                // rescanned on this node.
                //
                // Also note that there are cases where the revision
                // numbers are zero and thus not at all valid. In this
                // case the file must be rescanned as it is not known
                // if it was changed or not.
                //
                needRescanOnCsvfs = ( (VolumeRevision == 0) ||
                                      (CacheRevision == 0) ||
                                      (FileRevision == 0) ||
                                      (VolumeRevision != StreamContext->VolumeRevision) ||
                                      (CacheRevision != StreamContext->CacheRevision) ||
                                      (FileRevision != StreamContext->FileRevision) );
            } else {
                AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                              (" [AV] AvPreCleanupCsvfs: Status 0x%x from AvReadCsvRevisionECP\n", status) );

                //
                // In this case the revision numbers are not available.
                // Since there is no way to know if the file was
                // changed on another node, it is safest to rescan.
                //
                needRescanOnCsvfs = TRUE;
            }
        }
        FltReleaseContext( instanceContext );
    } else {
        //
        // If unable to get instance context then the code isn't sure
        // if it is on a CSVFS volume or not. The safest assumption
        // would be to rescan the file.
        //
        needRescanOnCsvfs = TRUE;
        
        AV_DBG_PRINT( AVDBG_TRACE_ERROR,
                  ("[AV] AvPreCleanupCsvfs: FltGetInstanceContext failed. status = 0x%x\n", status) );
    }


    AV_DBG_PRINT( AVDBG_TRACE_DEBUG,
                  ("[Av]: AvPreCleanupCsvfs: %ws need rescan\n",
                  needRescanOnCsvfs ? L"Does" : L"Does not")
                  );

    
    //
    // if it has been determined that a rescan is needed then set the
    // file modified flag on the stream context to indicate this
    //
    if (needRescanOnCsvfs) {
        if ( StreamContext->TxContext != NULL) {

            //
            //  Instead of updating State, we update TxState here, 
            //  because the file is part of a transaction writer
            //

            SET_FILE_TX_MODIFIED( StreamContext );

        } else {

            SET_FILE_MODIFIED( StreamContext );
        }

        //
        // If CSVFS has provided us with valid revison numbers then
        // return them to the caller so it can update them in the
        // stream context if the scan is successful.
        //
        if ((VolumeRevision != 0) &&
            (CacheRevision != 0) &&
            (FileRevision != 0)) {
            *UpdateRevisionNumbers = TRUE;
            *VolumeRevisionPtr = VolumeRevision;
            *CacheRevisionPtr = CacheRevision;
            *FileRevisionPtr = FileRevision;
        }
    }
    
    return status;    
}

/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    nc.c

Abstract:

    This is the main module of the NameChanger MiniFilter driver.
    This filter grafts a directory from one part of a volume's namespace
    to another part using a mapping. The filter then maintains the illusion
    by acting as a name provider, injecting entries into directory enumerations
    and forwarding directory change notifications.

Environment:

    Kernel mode

--*/

#include <dontuse.h>
#include <suppress.h>

#include "nc.h"


//
//  Prototypes
//

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
NcInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
NcInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
NcUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
NcInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );


//
//  Assign sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NcInstanceSetup)
#pragma alloc_text(PAGE, NcInstanceTeardownStart)
#pragma alloc_text(PAGE, NcUnload)
#pragma alloc_text(PAGE, NcInstanceQueryTeardown)
#endif

//
//  IRP Callback Prototypes
//

FLT_PREOP_CALLBACK_STATUS
NcPreCreateCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
NcPreCleanupCallback (
    _Unreferenced_parameter_ PFLT_CALLBACK_DATA Data,
    _Unreferenced_parameter_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
NcPreFsctlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostFsctlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode==FSCTL_FIND_FILES_BY_SID, _In_)
    _When_(Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode!=FSCTL_FIND_FILES_BY_SID, _In_opt_)
    PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
NcPreQueryInformationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostQueryInformationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
NcPreSetInformationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
NcPreDirectoryControlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
NcPostDirectoryControlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
NcPreNetworkQueryCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcPreCreateCallback)
#pragma alloc_text(PAGE, NcPreCleanupCallback)
#pragma alloc_text(PAGE, NcPostCleanupCallback)
#pragma alloc_text(PAGE, NcPreFsctlCallback)
#pragma alloc_text(PAGE, NcPreQueryInformationCallback)
#pragma alloc_text(PAGE, NcPostQueryInformationCallback)
#pragma alloc_text(PAGE, NcPreSetInformationCallback)
#pragma alloc_text(PAGE, NcPreDirectoryControlCallback)
#pragma alloc_text(PAGE, NcPreNetworkQueryCallback)
#endif

//
//  Name Callback Prototypes
//

NTSTATUS
NcGenerateFileNameCallback (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Inout_ PFLT_NAME_CONTROL FileName
    );

NTSTATUS
NcNormalizeNameComponentExCallback (
    _In_ PFLT_INSTANCE Instance,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    );

NTSTATUS
NcNormalizeNameComponentCallback (
    _In_ PFLT_INSTANCE Instance,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT DeviceNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcGenerateFileNameCallback)
#pragma alloc_text(PAGE, NcNormalizeNameComponentCallback)
#pragma alloc_text(PAGE, NcNormalizeNameComponentExCallback)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      NcPreCreateCallback,
      NULL },

    { IRP_MJ_CLEANUP,
      0,
      NcPreCleanupCallback,
      NcPostCleanupCallback },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      NcPreQueryInformationCallback,
      NcPostQueryInformationCallback },

    { IRP_MJ_SET_INFORMATION,
      0,
      NcPreSetInformationCallback,
      NULL },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      NcPreDirectoryControlCallback,
      NcPostDirectoryControlCallback },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      NcPreNetworkQueryCallback,
      NULL },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      NcPreFsctlCallback,
      NcPostFsctlCallback },

    { IRP_MJ_OPERATION_END }
};

//
//  Context Registration
//

const FLT_CONTEXT_REGISTRATION contextRegistration[] = {

    { FLT_INSTANCE_CONTEXT,
        0,
        NcInstanceContextClose,
        sizeof(NC_INSTANCE_CONTEXT),
        NC_INSTANCE_CONTEXT_TAG,
        NULL,
        NULL,
        NULL },

    { FLT_STREAMHANDLE_CONTEXT,
        0,
        NcStreamHandleContextClose,
        sizeof(NC_STREAM_HANDLE_CONTEXT),
        NC_STREAM_HANDLE__CONTEXT_TAG,
        NULL,
        NULL,
        NULL },

    { FLT_CONTEXT_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    contextRegistration,                //  Context
    Callbacks,                          //  Operation callbacks

    NcUnload,                           //  MiniFilterUnload

    NcInstanceSetup,                    //  InstanceSetup
    NcInstanceQueryTeardown,            //  InstanceQueryTeardown
    NcInstanceTeardownStart,            //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NcGenerateFileNameCallback,         //  GenerateFileName
    NcNormalizeNameComponentCallback,   //  Normalize Component
    NULL,                               //  NormalizeContextCleanupCallback

#if FLT_MGR_LONGHORN
    NULL,                               //  Transaction Notification Callback.
    NcNormalizeNameComponentExCallback  //  Normalize Name Component with transaction handle.
#endif

};

//
// Constants
//

NC_GLOBAL_DATA NcGlobalData;

//
//  Function Definitions
//


NTSTATUS
NcInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    We attach to the volume if:
    The User mapping parent exists.
    The User mapping does not exist
    The Real mapping parent exists

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach
    Other Errorcode - Percolate errors up. (do not attach).

Logic:
    A) Validate that we should attach.
        1) Construct UserMappingPath
        2) Open User Parent (validate it exists)
        3) Query Final component (validate it does not exist)
        4) Construct RealMappingPath
        5) Open Real Parent (validate it exists)
    B) Now we know that we should attach. We need to get
        short and normalized names for the paths and then
        construct our mapping and instance context.
--*/
{

    //
    //  Status Tracking Vars.
    //

    NTSTATUS Status;
    NTSTATUS ReturnValue = STATUS_SUCCESS;

    //
    //  User mapping vars
    //
    
    NC_MAPPING_PATH UserPath;
    HANDLE UserParentHandle = 0;
    PFILE_OBJECT UserParentFileObj = NULL;
    OBJECT_ATTRIBUTES UserParentAttributes;
    IO_STATUS_BLOCK UserParentStatusBlock;
    PFILE_NAMES_INFORMATION FinalComponentQueryBuffer = NULL;
    ULONG FinalComponentQueryLength;
    
    //
    //  Real Mapping vars
    //
    
    NC_MAPPING_PATH RealPath;
    HANDLE RealParentHandle = 0;
    PFILE_OBJECT RealParentFileObj = NULL;
    OBJECT_ATTRIBUTES RealParentAttributes;
    IO_STATUS_BLOCK RealParentStatusBlock;
    
    //
    //  Context Vars
    //
    
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    
    //
    //  Temp vars
    //

    ULONG DataRead;
    NC_PATH_OVERLAP Overlap;


    PAGED_CODE();

    UNREFERENCED_PARAMETER( VolumeDeviceType );

    //
    //  Although this filter goes to great lengths to be filesystem-agnostic,
    //  it is not yet able to handle the V3 USN records supported by ReFS.
    //

    if (VolumeFilesystemType == FLT_FSTYPE_REFS) {

        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Prevent the filter from auto attaching.
    //

    if (FlagOn( Flags, FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT )) {

        //
        //  We actually return here, rather than goto cleanup, because
        //  we have not yet zeroed out the buffers in the mappings.
        //
        
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  We should not be attaching before our driver initialization
    //  has completed, which should load and sanity check these
    //  values.  If this hasn't happened, fail attachment.
    //

    if (NcGlobalData.UserMappingPath.Length == 0 ||
        NcGlobalData.UserMappingFinalComponentShort.Length == 0 ||
        NcGlobalData.UserMappingFinalComponentLong.Length == 0 ||
        NcGlobalData.RealMappingPath.Length == 0 ||
        NcGlobalData.RealMappingFinalComponent.Length == 0) {

        FLT_ASSERT( FALSE );
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Zero Out structures. (Now we can cleanup)
    //

    NcInitMappingPath( &UserPath );
    NcInitMappingPath( &RealPath );

    //
    //  Build the mapping strings for this instance from our global
    //  registry state.
    //

    Status = NcBuildMappingPathFromVolume( FltObjects->Volume,
                                           &NcGlobalData.UserMappingPath,
                                           &NcGlobalData.UserMappingFinalComponentShort,
                                           &UserPath );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = Status;
        goto NcInstanceSetupCleanup;
    }

    Status = NcBuildMappingPathFromVolume( FltObjects->Volume,
                                           &NcGlobalData.RealMappingPath,
                                           &NcGlobalData.RealMappingFinalComponent,
                                           &RealPath );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = Status;
        goto NcInstanceSetupCleanup;
    }

    //
    //  Allocate the buffer to store the directory lookup.  We
    //  reuse the buffer for long and short names, so it must
    //  be large enough to handle either.
    //

    FinalComponentQueryLength = sizeof(FILE_NAMES_INFORMATION) +
                                AlignToSize( max( NcGlobalData.UserMappingFinalComponentLong.Length, NcGlobalData.UserMappingFinalComponentShort.Length ), 8 );
    FinalComponentQueryBuffer = ExAllocatePoolWithTag( PagedPool,
                                                       FinalComponentQueryLength,
                                                       NC_NORMALIZE_NAME_TAG );

    if (FinalComponentQueryBuffer == NULL) {

        ReturnValue = STATUS_INSUFFICIENT_RESOURCES;
        goto NcInstanceSetupCleanup;
    }


    //
    //  Open Parent of user mapping, and validate that it exists.
    //  Note that we do have a race here; a user may have this object
    //  opened with FILE_DELETE_ON_CLOSE.  Under these semantics, our
    //  create will succeed, but when their handle is closed our
    //  mapping parent will go away, which this filter cannot deal
    //  with.
    //

    InitializeObjectAttributes( &UserParentAttributes,
                                &UserPath.ParentPath,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = NcCreateFileEx2( NcGlobalData.FilterHandle,            // Filter
                              FltObjects->Instance,                 // Instance
                              &UserParentHandle,                    // Returned Handle
                              &UserParentFileObj,                   // Returned FileObject
                              FILE_LIST_DIRECTORY|FILE_TRAVERSE,    // Desired Access
                              &UserParentAttributes,                // object attributes
                              &UserParentStatusBlock,               // Returned IOStatusBlock
                              0,                                    // Allocation Size
                              FILE_ATTRIBUTE_NORMAL,                // File Attributes
                              0,                                    // Share Access
                              FILE_OPEN,                            // Create Disposition
                              FILE_DIRECTORY_FILE,                  // Create Options
                              NULL,                                 // Ea Buffer
                              0,                                    // EA Length
                              IO_IGNORE_SHARE_ACCESS_CHECK,         // Flags
                              NULL );                               // Transaction/ECP

    if (!NT_SUCCESS( Status )) {

        //
        //  We could not open the parent path, fail attachment.
        //

        ReturnValue = STATUS_FLT_DO_NOT_ATTACH;
        goto NcInstanceSetupCleanup;
    }

    //
    //  Make sure that the User mapping's short final component does not exist.
    //

    Status = NcQueryDirectoryFile( FltObjects->Instance,
                                   UserParentFileObj,
                                   FinalComponentQueryBuffer,
                                   FinalComponentQueryLength,
                                   FileNamesInformation,
                                   TRUE,//Return single entry
                                   &NcGlobalData.UserMappingFinalComponentShort,
                                   TRUE,//restart scan
                                   &DataRead);

    if (Status != STATUS_NO_SUCH_FILE) {

        if (!NT_SUCCESS( Status )) {

            //
            //  An unexpected error occurred, return code.
            //

            ReturnValue = Status;
            goto NcInstanceSetupCleanup;

        } else {

            //
            //  The query worked, but the file exists.
            //  The volume does not meet our constraints,
            //  so abort attachment.
            //

            ReturnValue = STATUS_FLT_DO_NOT_ATTACH;
            goto NcInstanceSetupCleanup;
        }
    }

    //
    //  Now we have established that the short name does not exist,
    //  we must search again for the long name.  Since the search
    //  string for a directory enumeration is cached by the filesystem
    //  and cannot be changed, we have to close and re-open the file
    //  in order to do the second enumeration.
    //

    FltClose( UserParentHandle );
    UserParentHandle = NULL;

    ObDereferenceObject( UserParentFileObj );
    UserParentFileObj = NULL;

    Status = NcCreateFileEx2( NcGlobalData.FilterHandle,            // Filter
                              FltObjects->Instance,                 // Instance
                              &UserParentHandle,                    // Returned Handle
                              &UserParentFileObj,                   // Returned FileObject
                              FILE_LIST_DIRECTORY|FILE_TRAVERSE,    // Desired Access
                              &UserParentAttributes,                // object attributes
                              &UserParentStatusBlock,               // Returned IOStatusBlock
                              0,                                    // Allocation Size
                              FILE_ATTRIBUTE_NORMAL,                // File Attributes
                              0,                                    // Share Access
                              FILE_OPEN,                            // Create Disposition
                              FILE_DIRECTORY_FILE,                  // Create Options
                              NULL,                                 // Ea Buffer
                              0,                                    // EA Length
                              IO_IGNORE_SHARE_ACCESS_CHECK,         // Flags
                              NULL );                               // Transaction/ECP

    if (!NT_SUCCESS( Status )) {

        ReturnValue = Status;
        goto NcInstanceSetupCleanup;
    }

    //
    //  Make sure that the User mapping's long final component does not exist.
    //

    Status = NcQueryDirectoryFile( FltObjects->Instance,
                                   UserParentFileObj,
                                   FinalComponentQueryBuffer,
                                   FinalComponentQueryLength,
                                   FileNamesInformation,
                                   TRUE,//Return single entry
                                   &NcGlobalData.UserMappingFinalComponentLong,
                                   TRUE,//restart scan
                                   &DataRead );

    if (Status != STATUS_NO_SUCH_FILE) {

        if (!NT_SUCCESS( Status )) {

            //
            //  An unexpected error occurred, return code.
            //

            ReturnValue = Status;
            goto NcInstanceSetupCleanup;

        } else {

            //
            //  The query worked, but the file exists.
            //  The volume does not meet our constraints,
            //  so abort attachment.
            //

            ReturnValue = STATUS_FLT_DO_NOT_ATTACH;
            goto NcInstanceSetupCleanup;
        }
    }

    //
    //  Open parent of real mapping.
    //  Note that we do have a race here; a user may have this object
    //  opened with FILE_DELETE_ON_CLOSE.  Under these semantics, our
    //  create will succeed, but when their handle is closed our
    //  mapping parent will go away, which this filter cannot deal
    //  with.
    //

    InitializeObjectAttributes( &RealParentAttributes,
                                &RealPath.ParentPath,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = NcCreateFileEx2( NcGlobalData.FilterHandle,            // Filter
                              FltObjects->Instance,                 // Instance
                              &RealParentHandle,                    // Returned Handle
                              &RealParentFileObj,                   // Returned FileObject
                              FILE_LIST_DIRECTORY|FILE_TRAVERSE,    // Desired Access
                              &RealParentAttributes,                // object attributes
                              &RealParentStatusBlock,               // Returned IOStatusBlock
                              0,                                    // Allocation Size
                              FILE_ATTRIBUTE_NORMAL,                // File Attributes
                              0,                                    // Share Access
                              FILE_OPEN,                            // Create Disposition
                              FILE_DIRECTORY_FILE,                  // Create Options
                              NULL,                                 // Ea Buffer
                              0,                                    // EA Length
                              IO_IGNORE_SHARE_ACCESS_CHECK,         // Flags
                              NULL );                               // Transaction/ECP

    FLT_ASSERT( Status != STATUS_INVALID_PARAMETER );

    if (!NT_SUCCESS( Status )) {

        //
        //  We could not open the parent path, fail attachment.
        //

        ReturnValue = STATUS_FLT_DO_NOT_ATTACH;
        goto NcInstanceSetupCleanup;
    }

    //
    //  We have validated that we should attach.
    //


    //
    //  Allocate context
    //

    Status = FltAllocateContext( FltObjects->Filter,
                                 FLT_INSTANCE_CONTEXT,
                                 sizeof(NC_INSTANCE_CONTEXT),
                                 NonPagedPool,
                                 &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = Status;
        goto NcInstanceSetupCleanup;
    }

    //
    //  We have allocated an instance context. Now initialize it.
    //

    //
    //  Now that we have a context,
    //  we need to generate the mapping
    //

    NcInitMapping( &InstanceContext->Mapping );

    
    Status = NcBuildMapping( UserParentFileObj,
                             RealParentFileObj,
                             &NcGlobalData.UserMappingFinalComponentShort,
                             &NcGlobalData.UserMappingFinalComponentLong,
                             &NcGlobalData.RealMappingFinalComponent,
                             FltObjects->Instance,
                             &InstanceContext->Mapping );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = Status;
        goto NcInstanceSetupCleanup;
    }

    //
    //  Sanity check the relation between the mappings.  Force case insensitive
    //  to TRUE here, since we may be evaluating this relationship in response
    //  to case insensitive user requests in the future.
    //

    NcComparePath( &RealPath.FullPath,
                   &InstanceContext->Mapping.UserMapping,
                   NULL,
                   TRUE,
                   TRUE,
                   &Overlap );

    if (Overlap.InMapping || Overlap.Ancestor) {

        ReturnValue = STATUS_FLT_DO_NOT_ATTACH;
        goto NcInstanceSetupCleanup;
    }

    NcComparePath( &UserPath.FullPath,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   TRUE,
                   TRUE,
                   &Overlap );

    if (Overlap.InMapping || Overlap.Ancestor) {

        ReturnValue = STATUS_FLT_DO_NOT_ATTACH;
        goto NcInstanceSetupCleanup;
    }

    //
    //  Remember what file system we're attached to.
    //

    InstanceContext->VolumeFilesystemType = VolumeFilesystemType;

    //
    //  Register the instance context.
    //

    Status = FltSetInstanceContext( FltObjects->Instance,
                                    FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
                                    InstanceContext,
                                    NULL);

    if (!NT_SUCCESS( Status )) {

        ReturnValue = Status;
        goto NcInstanceSetupCleanup;
    }


NcInstanceSetupCleanup:

    if (InstanceContext != NULL ) {

        FltReleaseContext( InstanceContext );
    }

    NcTeardownMappingPath( &UserPath );

    if (UserParentHandle != 0) {

        FltClose( UserParentHandle );
    }

    if (UserParentFileObj != NULL) {

        ObDereferenceObject( UserParentFileObj );
    }

    NcTeardownMappingPath( &RealPath );

    if (RealParentHandle != 0) {

        FltClose( RealParentHandle );
    }

    if (RealParentFileObj != NULL) {

        ObDereferenceObject( RealParentFileObj );
    }

    if (FinalComponentQueryBuffer != NULL) {

        ExFreePool( FinalComponentQueryBuffer );
    }

    return ReturnValue;
}


NTSTATUS
NcInstanceQueryTeardown (
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

    Returns the Status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    return STATUS_SUCCESS;
}


VOID
NcInstanceTeardownStart (
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
    NTSTATUS Status;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( Flags );

    Status = FltGetInstanceContext( FltObjects->Instance, 
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        goto NcInstanceTeardownStartCleanup;
    }
    
NcInstanceTeardownStartCleanup:

    if (InstanceContext)  {

        FltReleaseContext( InstanceContext );
    }

}

//
//  MiniFilter initialization and unload routines.
//

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

    Returns STATUS_SUCCESS.

--*/
{
    NTSTATUS Status;

    //
    //  Import routines
    //
    
    NcCompatInit( );

    Status = NcInitializeMapping( RegistryPath );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    //  Register with FltMgr to tell it our callback routines
    //

    Status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &NcGlobalData.FilterHandle );

    FLT_ASSERT( NT_SUCCESS( Status ) );

    if (NT_SUCCESS( Status )) {

        //
        //  Start filtering i/o
        //

        Status = FltStartFiltering( NcGlobalData.FilterHandle );

        if (!NT_SUCCESS( Status )) {

            FltUnregisterFilter( NcGlobalData.FilterHandle );
        }
    }

    return Status;
}

NTSTATUS
NcUnload (
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

    Returns the final Status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    FltUnregisterFilter( NcGlobalData.FilterHandle );

    if (NcGlobalData.UserMappingPath.Buffer != NULL) {
        NcFreeUnicodeString( &NcGlobalData.UserMappingPath );
    }

    if (NcGlobalData.UserMappingFinalComponentShort.Buffer != NULL) {
        NcFreeUnicodeString( &NcGlobalData.UserMappingFinalComponentShort );
    }

    if (NcGlobalData.UserMappingFinalComponentLong.Buffer != NULL) {
        NcFreeUnicodeString( &NcGlobalData.UserMappingFinalComponentLong );
    }

    if (NcGlobalData.RealMappingPath.Buffer != NULL) {
        NcFreeUnicodeString( &NcGlobalData.RealMappingPath );
    }

    if (NcGlobalData.RealMappingFinalComponent.Buffer != NULL) {
        NcFreeUnicodeString( &NcGlobalData.RealMappingFinalComponent );
    }

    return STATUS_SUCCESS;
}

//
//  MiniFilter callback routines.
//

FLT_PREOP_CALLBACK_STATUS
NcPreCreateCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    Routine is invoked when a create is issued by a user.


Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

Logic:

    1) If file is opened by ID or is a Paging File, etc.
         A) return PASSTHROUGH_NO_CALLBACK
    2) Get the opened name and calculate overlap
    3) If File is Delete On close
         A) If the file is an ancestor of either mapping
             a) Fail Open
    4) If the path is in the real mapping
         A) Fail the Open
    5) If the path is in user mapping
         A) Recalculate the full name (without SL_TARGET_PARENT_DIRECTORY)
         B) Munge Name to Real Mapping
         C) Switch name
         D) Passthrough with callback.
    6) Else
         A) Return Passthrough No callback
--*/
{
    PAGED_CODE();

    return NcPreCreate( Data,
                        FltObjects,
                        CompletionContext );
}

FLT_PREOP_CALLBACK_STATUS
NcPreCleanupCallback (
    _Unreferenced_parameter_ PFLT_CALLBACK_DATA Data,
    _Unreferenced_parameter_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This is the pre cleanup callback.  It is called for every handle cleanup
    operation.  The role of this callback is merely to ensure that we
    also receive every post handle cleanup operation, and that it is safe
    to issue filesystem calls when we do.  We do this via
    FLT_PREOP_SYNCHRONIZE for simplicity.
    parameter.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  Not used in this function.

Return Value:

    Returns the final status of this operation.  Always FLT_PREOP_SYNCHRONIZE
    in this routine.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    //
    //  Synchronize all cleanup requests.  This enables us to ensure
    //  that any state is torn down correctly even if a request is
    //  issued between pre cleanup and post cleanup.
    //

    return FLT_PREOP_SYNCHRONIZE;
}

FLT_POSTOP_CALLBACK_STATUS
NcPostCleanupCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This is the post cleanup callback.  It is called for every handle cleanup
    operation.  The role of this callback is to tear down any pending
    directory change notification state attached to this handle.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  Not used in this function.

    Flags - The flags for this operation.

Return Value:

    Returns the final status of this operation.  Always
    FLT_POSTOP_FINISHED_PROCESSING in this routine.  Note that cleanup
    operations should never fail.

--*/
{
    NTSTATUS Status;
    PNC_STREAM_HANDLE_CONTEXT HandleContext = NULL;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    if (!FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING )) {

        //
        //  Obtain our handle context.  We should only be called here
        //  if we really have one.
        //
    
        Status = FltGetStreamHandleContext( FltObjects->Instance,
                                            FltObjects->FileObject,
                                            &HandleContext );
    
        if (!NT_SUCCESS( Status )) {
    
            goto NcPostCleanupCallbackCleanup;
    
        }
    
        FLT_ASSERT( HandleContext != NULL );
    
        NcStreamHandleContextNotCleanup( HandleContext );
    }


NcPostCleanupCallbackCleanup:

    if (HandleContext != NULL) {

        FltReleaseContext( HandleContext );

    }

    //
    //  We don't want to fail cleanup no matter what goes wrong.
    //

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
NcPreFsctlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This is the pre FSCTL callback.  It is called for every FSCTL
    operation.  This allows us to intercept and process FSCTLs which
    either consume or return names which may be affected by the namespace
    redirection performed by this filter.

    Currently we munge:
        FSCTL_ENUM_USN_DATA
        FSCTL_FIND_FILES_BY_SID
        FSCTL_LOOKUP_STREAM_FROM_CLUSTER
        FSCTL_READ_FILE_USN_DATA
        FSCTL_READ_USN_JOURNAL

    We also need to munge:
        FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  Not used in this function.

Return Value:

    Returns the final status of this operation.

--*/
{
    PAGED_CODE();

    //
    // TODO:
    //  FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES
    //
    //  The following take paths and work without any explicit change.
    //  Since Txf sends its creates to the top of the filter stack, we
    //  munge these paths on create (both for the RM and the logs.)  Thus
    //  Txf believes its logs are in the user mapping, when they are not;
    //  subsequent queries return the user mapping correctly.
    //
    //  FSCTL_TXFS_QUERY_RM_INFORMATION
    //  FSCTL_TXFS_START_RM
    //

    switch (Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode) {
        case FSCTL_ENUM_USN_DATA:
            return FLT_PREOP_SYNCHRONIZE;
            break;
        case FSCTL_FIND_FILES_BY_SID:
            return NcPreFindFilesBySid( Data,
                                        FltObjects,
                                        CompletionContext );
            break;
#if FLT_MGR_WIN7
        case FSCTL_LOOKUP_STREAM_FROM_CLUSTER:
            return FLT_PREOP_SYNCHRONIZE;
            break;
#endif
        case FSCTL_READ_FILE_USN_DATA:
            return FLT_PREOP_SYNCHRONIZE;
            break;
        case FSCTL_READ_USN_JOURNAL:
            return FLT_PREOP_SUCCESS_WITH_CALLBACK;
            break;
        default:

            //
            //  We're not processing this.  Let the filesystem handle it
            //  normally.
            //

            return FLT_PREOP_SUCCESS_NO_CALLBACK;
            break;

    }
}

FLT_POSTOP_CALLBACK_STATUS
NcPostFsctlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _When_(Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode==FSCTL_FIND_FILES_BY_SID, _In_)
    _When_(Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode!=FSCTL_FIND_FILES_BY_SID, _In_opt_)
    PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This is the post FSCTL callback.  It is called for every FSCTL
    which we need to munge, including USN, find files by SID, and lookup
    stream from cluster.  This allows us to process these FSCTLs which
    either consume or return names which may be affected by the namespace
    redirection performed by this filter.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  Not used in this function.

    Flags - The flags for this operation.

Return Value:

    Returns the final status of this operation.

--*/
{
    FLT_POSTOP_CALLBACK_STATUS Status = FLT_POSTOP_FINISHED_PROCESSING;

    if (!FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING )) {

        switch (Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode) {

            case FSCTL_ENUM_USN_DATA:

                Status = NcPostEnumUsnData( Data,
                                            FltObjects,
                                            CompletionContext,
                                            Flags );
                break;

            case FSCTL_FIND_FILES_BY_SID:
                
                Status = NcPostFindFilesBySid( Data,
                                               FltObjects,
                                               CompletionContext,
                                               Flags );

                break;
#if FLT_MGR_WIN7
            case FSCTL_LOOKUP_STREAM_FROM_CLUSTER:
                
                Status = NcPostLookupStreamFromCluster( Data,
                                                        FltObjects,
                                                        CompletionContext,
                                                        Flags );
                
                break;
#endif

            case FSCTL_READ_FILE_USN_DATA:

                Status = NcPostReadFileUsnData( Data,
                                                FltObjects,
                                                CompletionContext,
                                                Flags );
                break;

            case FSCTL_READ_USN_JOURNAL:

                Status = NcPostReadUsnJournal( Data,
                                               FltObjects,
                                               CompletionContext,
                                               Flags );
                break;

            default:
                break;
        }
    }

    return Status;
}

FLT_PREOP_CALLBACK_STATUS
NcPreQueryInformationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This callback is invoked when the user queries a file's information.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.
        Currently we munge:
        FileAllInformation
        FileAlternateNameInformation
        FileHardLinkInformation
        FileNameInformation
        FileNormalizedNameInformation

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    FLT_PREOP_CALLBACK_STATUS result;

    PAGED_CODE();

    //
    //  We only have to munge for name sensitive operations.
    //

    switch( Data->Iopb->Parameters.QueryFileInformation.FileInformationClass ) {

        case FileAllInformation:
        case FileNameInformation:
        case FileNormalizedNameInformation:

            result = FLT_PREOP_SYNCHRONIZE;
            break;

        case FileAlternateNameInformation:
            result = NcPreQueryAlternateName( Data,
                                              FltObjects,
                                              CompletionContext );
            break;

        case FileHardLinkInformation:

            result = FLT_PREOP_SYNCHRONIZE;
            break;

        default:
            result = FLT_PREOP_SUCCESS_NO_CALLBACK;
            break;
    }

    return result;
}

FLT_POSTOP_CALLBACK_STATUS
NcPostQueryInformationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This is the post query file information callback.  It is called for
    every information class we need to process.  This allows us to fixup
    names returned that would otherwise contain real mapping paths.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.  Not used in this function.

    Flags - The flags for this operation.

Return Value:

    Returns the final status of this operation.

--*/
{
    FLT_POSTOP_CALLBACK_STATUS Status = FLT_POSTOP_FINISHED_PROCESSING;

    PAGED_CODE();

    if (!FlagOn( Flags, FLTFL_POST_OPERATION_DRAINING )) {

        switch( Data->Iopb->Parameters.SetFileInformation.FileInformationClass ) {

            case FileNameInformation:
            case FileNormalizedNameInformation:
            case FileAllInformation:

                Status = NcPostQueryName( Data,
                                          FltObjects,
                                          CompletionContext,
                                          Flags );
                break;

            case FileHardLinkInformation:
                Status = NcPostQueryHardLinks( Data,
                                               FltObjects,
                                               CompletionContext,
                                               Flags );
                break;
        }
    }
    return Status;
}

FLT_PREOP_CALLBACK_STATUS
NcPreSetInformationCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This is the pre set file information callback.  It is called for every
    set file information operation.  This allows us to intercept and process
    those which either consume or return names, as well as those which would
    violate the constraints imposed by this filter.

    Currently we munge:
        FileDispositionInformation
        FileLinkInformation
        FileRenameInformation
        FileShortNameInformation

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

--*/
{

    FLT_PREOP_CALLBACK_STATUS result;

    PAGED_CODE();
    
    switch( Data->Iopb->Parameters.SetFileInformation.FileInformationClass ) {

        case FileDispositionInformation:

            result = NcPreSetDisposition( Data,
                                          FltObjects,
                                          CompletionContext );

            break;

        case FileLinkInformation:

            result = NcPreSetLinkInformation( Data,
                                              FltObjects,
                                              CompletionContext );

            break;

        case FileRenameInformation:

            result = NcPreRename( Data,
                                  FltObjects,
                                  CompletionContext );

            break;

        case FileShortNameInformation:

            result = NcPreSetShortName( Data,
                                        FltObjects,
                                        CompletionContext );

            break; 

        default:
            result = FLT_PREOP_SUCCESS_NO_CALLBACK;
            break;
    }
    
    return result;
}

FLT_PREOP_CALLBACK_STATUS
NcPreDirectoryControlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This callback is invoked when the user tries to enumerate a directory
    or get change notifications.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    FLT_PREOP_CALLBACK_STATUS Status;

    PAGED_CODE();

    switch( Data->Iopb->MinorFunction ) {

        case IRP_MN_QUERY_DIRECTORY:
            Status = NcEnumerateDirectory( Data, FltObjects, CompletionContext );
            break;

        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
            Status = NcPreNotifyDirectory( Data, FltObjects, CompletionContext );
            break;

        default:
            FLT_ASSERT( FALSE );
            Status = FLT_PREOP_SUCCESS_NO_CALLBACK;

    }

    return Status;
}

FLT_POSTOP_CALLBACK_STATUS
NcPostDirectoryControlCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This callback is invoked after the user tries to enumerate a directory
    or get change notifications.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    FLT_POSTOP_CALLBACK_STATUS Status;

    //
    //  Note this routine must be nonpaged.  It is called within completion
    //  routines.
    //

    switch( Data->Iopb->MinorFunction ) {

        case IRP_MN_QUERY_DIRECTORY:

            //
            //  Currently we don't process post directory enumeration
            //  requests.  We really should.
            //

            Status = FLT_POSTOP_FINISHED_PROCESSING;
            break;

        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

            Status = NcPostNotifyDirectory( Data, FltObjects, CompletionContext, Flags );
            break;

        default:
            FLT_ASSERT( FALSE );
            Status = FLT_POSTOP_FINISHED_PROCESSING;

    }

    return Status;
}

FLT_PREOP_CALLBACK_STATUS
NcPreNetworkQueryCallback (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This callback is invoked when the user tries to do a network query open.

Arguments:

    Data - Pointer to the filter CallbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the Status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();

    //
    //  TODO We need to pipe these through the existing create path if
    //  possible.
    //
    
    return FLT_PREOP_DISALLOW_FASTIO;
}

NTSTATUS
NcGenerateFileNameCallback (
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Inout_ PFLT_NAME_CONTROL OutputNameControl
    )
/*++

Routine Description:

    This callback is invoked to maintain FltMgr's name cache coherency
    when this filter munges names.

Arguments:

    Instance - Instance pointer for the minifilter instance that this
        callback routine is registered for. 

    FileObject - Pointer to a file object for the file whose name is being
        requested. 

    Data - Pointer to the callback data structure for the operation during
        which this name is being requested. 

    NameOptions - FLT_FILE_NAME_OPTIONS value that specifies the name format,
        query method, and flags for this file name information query. 

    CacheFileNameInformation - Pointer to a Boolean value specifying whether
        this name can be cached. Set to TRUE on output if the name can be
        cached; set to FALSE otherwise. 

    OutputNameControl - Pointer to a Filter Manager-allocated FLT_NAME_CONTROL
        structure to receive the file name on output. 

Return Value:

    The return value is the Status of the operation.

--*/
{

    PAGED_CODE();

    return NcGenerateFileName( Instance,
                               FileObject,
                               Data,
                               NameOptions,
                               CacheFileNameInformation,
                               OutputNameControl );
}

NTSTATUS
NcNormalizeNameComponentExCallback (
    _In_     PFLT_INSTANCE            Instance,
    _In_opt_ PFILE_OBJECT             FileObject,
    _In_     PCUNICODE_STRING         ParentDirectory,
    _In_     USHORT                   DeviceNameLength,
    _In_     PCUNICODE_STRING         Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_     ULONG                    ExpandComponentNameLength,
    _In_     FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID           *NormalizationContext
    )
/*++

Routine Description:

    This callback is invoked to maintain FltMgr's name cache coherency
    when this filter munges names.

Arguments:

    Instance - Instance pointer for the minifilter instance that this callback
        routine is registered for. 

    FileObject - Optionally points to a file object that the name query is
        taking place on.  We can use this to flow transaction information
        to any recursive requests.

    ParentDirectory - Pointer to a UNICODE_STRING structure that contains the
        name of the parent directory for this name component. 

    DeviceNameLength - Length, in bytes, of the parent directory name stored
        in the structure that the ParentDirectory parameter points to. 

    Component - Pointer to a UNICODE_STRING structure that contains the name
        component to be expanded. 

    ExpandComponentName - Pointer to a FILE_NAMES_INFORMATION structure that
        receives the expanded (normalized) file name information for the name
        component. 

    ExpandComponentNameLength - Length, in bytes, of the buffer that the
        ExpandComponentName parameter points to. 

    NormalizationContext - Pointer to minifilter-provided context information
        to be passed in any subsequent calls to this callback routine that are
        made to normalize the remaining components in the same file name path. 

Return Value:

    The return value is the Status of the operation.

--*/
{
    PAGED_CODE();

    return NcNormalizeNameComponentEx( Instance,
                                       FileObject,
                                       ParentDirectory,
                                       DeviceNameLength,
                                       Component,
                                       ExpandComponentName,
                                       ExpandComponentNameLength,
                                       Flags,
                                       NormalizationContext );

}

NTSTATUS
NcNormalizeNameComponentCallback (
    _In_     PFLT_INSTANCE            Instance,
    _In_     PCUNICODE_STRING         ParentDirectory,
    _In_     USHORT                   DeviceNameLength,
    _In_     PCUNICODE_STRING         Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_     ULONG                    ExpandComponentNameLength,
    _In_     FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID           *NormalizationContext
    )
/*++

Routine Description:

    This callback is invoked to maintain FltMgr's name cache coherency
    when this filter munges names.

Arguments:

    Instance - Instance pointer for the minifilter instance that this callback
        routine is registered for. 

    ParentDirectory - Pointer to a UNICODE_STRING structure that contains the
        name of the parent directory for this name component. 

    DeviceNameLength - Length, in bytes, of the parent directory name stored
        in the structure that the ParentDirectory parameter points to. 

    Component - Pointer to a UNICODE_STRING structure that contains the name
        component to be expanded. 

    ExpandComponentName - Pointer to a FILE_NAMES_INFORMATION structure that
        receives the expanded (normalized) file name information for the name
        component. 

    ExpandComponentNameLength - Length, in bytes, of the buffer that the
        ExpandComponentName parameter points to. 

    NormalizationContext - Pointer to minifilter-provided context information
        to be passed in any subsequent calls to this callback routine that are
        made to normalize the remaining components in the same file name path. 

Return Value:

    The return value is the Status of the operation.

--*/
{

    PAGED_CODE();

    return NcNormalizeNameComponentEx( Instance, 
                                       NULL,
                                       ParentDirectory, 
                                       DeviceNameLength, 
                                       Component, 
                                       ExpandComponentName,
                                       ExpandComponentNameLength,
                                       Flags,
                                       NormalizationContext );
}




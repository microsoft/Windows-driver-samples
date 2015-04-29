/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    nccreate.c

Abstract:

    Contains routines to process user-initiated creates.  We need to 
    detect if these need to be redirected to a new physical location,
    and redirect as appropriate.

Environment:

    Kernel mode

--*/

#include "nc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NcPreCreate)
#endif


FLT_PREOP_CALLBACK_STATUS
NcPreCreate (
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
         A) return FLT_PREOP_SUCCESS_NO_CALLBACK
    2) Get the opened name and calculate overlap
    3) If File is Delete On close
         A) If the file is an ancestor of either mapping
             a) Fail Open
    4) If the path is in the real mapping
         A) Fail the Open
    5) If the path is in user mapping
         A) Recalculate the full name (without SL_OPEN_TARGET_DIRECTORY)
         B) Munge Name to Real Mapping
         C) Switch name
         D) Passthrough with callback.
    6) Else
         A) Return Passthrough No callback
--*/
{


    NTSTATUS Status;
    FLT_PREOP_CALLBACK_STATUS ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PFLT_FILE_NAME_INFORMATION FileNameInformation = NULL;
    PFLT_FILE_NAME_INFORMATION FullFileNameInformation = NULL;
    PNC_INSTANCE_CONTEXT InstanceContext = NULL;
    NC_PATH_OVERLAP RealOverlap, UserOverlap, FullOverlap;
    UNICODE_STRING Remainder;
    UNICODE_STRING MungedName = EMPTY_UNICODE_STRING;
    BOOLEAN IgnoreCase = !BooleanFlagOn( Data->Iopb->OperationFlags, SL_CASE_SENSITIVE );
    UCHAR CreateDisposition = (UCHAR)(Data->Iopb->Parameters.Create.Options >> 24);

    PAGED_CODE();

    UNREFERENCED_PARAMETER( CompletionContext );

    FLT_ASSERT( IoGetTopLevelIrp() == NULL );

    //
    //  Check if this open is to the paging file.
    //  We are not going to handle munging the namespace for paging files.
    //

    if (FlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE )) {

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreCreateCleanup;
    }

    //
    //  Check if this is a volume open.
    //  Volume opens do not affect the namespace of the volume so we don't care.
    //

    if (FlagOn( Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN )) {

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreCreateCleanup;
    }

    //
    //  Check if this open is by ID.
    //  Opens by file ID are name agnostic. Thus we do not care about this open.
    //

    if (FlagOn( Data->Iopb->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID )) {

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreCreateCleanup;
    }

    //
    //  Get opened name.
    //  This is pre-create so this is the name which will be opened if allowed 
    //  to continue.  Note that unlike most other paths, we do not call
    //  ourselves here; doing so would munge the "real" name into the "user"
    //  name, which would prevent us from detecting access via the "real"
    //  name.  Because this request is not being sent to us, we need to ensure
    //  we don't poison the cache of below filters (allowing the user visible
    //  mapping to be cached beneath us.)
    //

    Status = NcGetFileNameInformation( Data,
                                       NULL,
                                       NULL,
                                       FLT_FILE_NAME_OPENED | 
                                           FLT_FILE_NAME_QUERY_DEFAULT |
                                           FLT_FILE_NAME_DO_NOT_CACHE,
                                       &FileNameInformation );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreCreateCleanup;
    }

    Status = FltParseFileNameInformation( FileNameInformation );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreCreateCleanup;
    }

    //
    //  Get the instance context.
    //

    Status = FltGetInstanceContext( FltObjects->Instance,
                                    &InstanceContext );

    if (!NT_SUCCESS( Status )) {

        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreCreateCleanup;
    }

    FLT_ASSERT( InstanceContext != NULL );

    //
    //  Get the open path's relation to the mapping paths.
    //

    NcComparePath( &FileNameInformation->Name,
                   &InstanceContext->Mapping.RealMapping,
                   NULL,
                   IgnoreCase,
                   TRUE,
                   &RealOverlap );

    NcComparePath( &FileNameInformation->Name,
                   &InstanceContext->Mapping.UserMapping,
                   &Remainder,
                   IgnoreCase,
                   TRUE,
                   &UserOverlap );

    //
    //  Name changer uses the opened name of a file as an indicator that the 
    //  file has been munged. Because of this we have to deny opens down the 
    //  real mapping in order to disambiguate whether it was opened via the 
    //  munging operation. We thus disallow opens down the real mapping path.
    //

    if (RealOverlap.Match) {

        //
        //  When opening the mapping itself, we have virtualized its
        //  nonexistence to the caller.  Therefore, any attempt to open
        //  should return a not found code, and any conditional open/create
        //  is effectively a create and should be failed with a code
        //  appropriate for that.
        //

        ReturnValue = FLT_PREOP_COMPLETE;
        switch (CreateDisposition) {
            case FILE_OPEN:
            case FILE_OVERWRITE:
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto NcPreCreateCleanup;
            case FILE_CREATE:
            case FILE_OPEN_IF:
            case FILE_OVERWRITE_IF:
            case FILE_SUPERSEDE:
                Status = STATUS_ACCESS_DENIED;
                goto NcPreCreateCleanup;
        }

        FLT_ASSERT( FALSE );
        Status = STATUS_ACCESS_DENIED;
        goto NcPreCreateCleanup;

    } else if (RealOverlap.InMapping) {

        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        ReturnValue = FLT_PREOP_COMPLETE;
        goto NcPreCreateCleanup;
    }

    //
    //  Short names around the mapping cause a problem for namechanger. 
    //  This is because the filesystem will generate new shortnames which are
    //  not told to fltmgr. Finding out the names accurately is not reliable
    //  because the name can change while fltmgr is querying the name.
    //
    //  One solution to this problem is to protect the mapping. We prevent
    //  users from renaming or deleting the mapping paths so that we know the
    //  short names do not change.
    //
    //  In create we need to make sure that the FILE_DELETE_ON_CLOSE flag is
    //  not set if the user is opening the real mapping or an ancestor of it.
    //

    if (FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )) {

        if (RealOverlap.Ancestor || UserOverlap.Ancestor) {

            Status = STATUS_ACCESS_DENIED;
            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreCreateCleanup;
        }
    }

    //
    //  Name changer munges opens which occur down the user mapping path.
    //  Here we check to see if this is a name we want to change.
    //

    if (UserOverlap.InMapping) {

        //
        //  This open is targeted inside the user mapping.
        //  This means that we need to munge the open name 
        //  to the real mapping.
        //

        //
        //  Constructing a new path for the file has a quirk because of 
        //  SL_OPEN_TARGET_DIRECTORY. If it is set, then the name has the
        //  following components: /device/ancestors/parent/final_component. 
        //  If the flag is set, then fltmgr stripped of the final component
        //  (leaving /device/ancestors/parent) when we called 
        //  FltGetFileNameInformation.
        //
        //  File systems assume that file objects for use as a target directory
        //  in rename operations are opened with SL_OPEN_TARGET_DIRECTORY.
        //  This means that unlike the related file object, which can be just
        //  cleared, opens with SL_OPEN_TARGET_DIRECTORY need to be propogated
        //  with the flag always.
        //  
        //  This means that opens which get munged and have
        //  SL_OPEN_TARGET_DIRECTORY need to be calculated with the final
        //  component in the name string.
        //
        
        if (FlagOn( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY )) {

            //
            //  Clear the flag so we can get the full name.
            //

            ClearFlag( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY );

            //
            //  Get the name with the final component.
            //

            Status = NcGetFileNameInformation( Data,
                                               NULL,
                                               NULL,
                                               FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY,
                                               &FullFileNameInformation );

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreCreateCleanup;
            }

            Status = FltParseFileNameInformation( FullFileNameInformation );

            if (!NT_SUCCESS( Status )) {

                ReturnValue = FLT_PREOP_COMPLETE;
                goto NcPreCreateCleanup;
            }

            FLT_ASSERT( FullFileNameInformation != NULL );

            //
            //  Now that we have the full path, lets put the 
            //  flag back on the file object.
            //

            SetFlag( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY );

            //
            //  We need to re-calculate the Remainder so we can
            //  construct the new name.
            //

            NcComparePath( &FullFileNameInformation->Name,
                           &InstanceContext->Mapping.UserMapping,
                           &Remainder,
                           IgnoreCase,
                           TRUE,
                           &FullOverlap );

            //
            //  Since we were dealing with a path further down
            //  the mapping, lets make sure that we are now
            //  even further down the mapping.
            //
            
            FLT_ASSERT( FullOverlap.InMapping );

        } 

        //
        //  Generate new name
        //

        Status = NcConstructPath( &InstanceContext->Mapping.RealMapping,
                                  &Remainder,
                                  FALSE,
                                  &MungedName );

        if (!NT_SUCCESS( Status )) {

            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreCreateCleanup;
        }

        //
        //  Replace open name in the file object.
        //

        Status = NcReplaceFileObjectName( Data->Iopb->TargetFileObject,
                                          MungedName.Buffer,
                                          MungedName.Length );

        if (!NT_SUCCESS(Status)) {

            ReturnValue = FLT_PREOP_COMPLETE;
            goto NcPreCreateCleanup;
        }

        //
        //  Because we calculate our own full path, the 
        //  RelatedFileObject is not needed on munged creates.
        //  Make sure that we don't send it below us.
        //

        FltObjects->FileObject->RelatedFileObject = NULL;

        //
        //  Pass the create below us.
        //

        ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto NcPreCreateCleanup;

    } //end if for in mapping

    //
    //  If we got here then we are not interested in this open.
    //  Passthrough no callback.
    //

    ReturnValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    goto NcPreCreateCleanup;

NcPreCreateCleanup:

    //
    //  Check if we care going to fail the open.
    //

    if (ReturnValue == FLT_PREOP_COMPLETE) {

        //
        //  If we get here, then Status must be a failure code.
        //

        FLT_ASSERT( !NT_SUCCESS( Status ) );

        Data->IoStatus.Status = Status;
    }

    //
    //  Clean up variables 
    //

    if (FileNameInformation != NULL) {

        FltReleaseFileNameInformation( FileNameInformation );
    }

    if (FullFileNameInformation != NULL) {

        FltReleaseFileNameInformation( FullFileNameInformation );
    }

    if (MungedName.Buffer != NULL) {

        ExFreePoolWithTag( MungedName.Buffer, NC_TAG );
    }

    if (InstanceContext != NULL) {

        FltReleaseContext( InstanceContext );
    }

    return ReturnValue;
}


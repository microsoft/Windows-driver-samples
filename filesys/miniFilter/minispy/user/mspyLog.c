/*++

Copyright (c) 1989-2002  Microsoft Corporation

Module Name:

    mspyLog.c

Abstract:

    This module contains functions used to retrieve and see the log records
    recorded by MiniSpy.sys.

Environment:

    User mode

--*/

#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <winioctl.h>
#include "mspyLog.h"

#define TIME_BUFFER_LENGTH 20
#define TIME_ERROR         "time error"

#define POLL_INTERVAL   200     // 200 milliseconds

BOOLEAN
TranslateFileTag(
    _In_ PLOG_RECORD logRecord
    )
/*++

Routine Description:

    If this is a mount point reparse point, move the given name string to the
    correct position in the log record structure so it will be displayed
    by the common routines.

Arguments:

    logRecord - The log record to update

Return Value:

    TRUE - if this is a mount point reparse point
    FALSE - otherwise

--*/
{
    PFLT_TAG_DATA_BUFFER TagData;
    ULONG Length;

    //
    // The reparse data structure starts in the NAME field, point to it.
    //

    TagData = (PFLT_TAG_DATA_BUFFER) &logRecord->Name[0];

    //
    //  See if MOUNT POINT tag
    //

    if (TagData->FileTag == IO_REPARSE_TAG_MOUNT_POINT) {

        //
        //  calculate how much to copy
        //

        Length = min( MAX_NAME_SPACE - sizeof(UNICODE_NULL), TagData->MountPointReparseBuffer.SubstituteNameLength );

        //
        //  Position the reparse name at the proper position in the buffer.
        //  Note that we are doing an overlapped copy
        //

        MoveMemory( &logRecord->Name[0],
                    TagData->MountPointReparseBuffer.PathBuffer,
                    Length );

        logRecord->Name[Length/sizeof(WCHAR)] = UNICODE_NULL;
        return TRUE;
    }

    return FALSE;
}


DWORD
WINAPI
RetrieveLogRecords(
    _In_ LPVOID lpParameter
    )
/*++

Routine Description:

    This runs as a separate thread.  Its job is to retrieve log records
    from the filter and then output them

Arguments:

    lpParameter - Contains context structure for synchronizing with the
        main program thread.

Return Value:

    The thread successfully terminated

--*/
{
    PLOG_CONTEXT context = (PLOG_CONTEXT)lpParameter;
    DWORD bytesReturned = 0;
    DWORD used;
    PVOID alignedBuffer[BUFFER_SIZE/sizeof( PVOID )];
    PCHAR buffer = (PCHAR) alignedBuffer;
    HRESULT hResult;
    PLOG_RECORD pLogRecord;
    PRECORD_DATA pRecordData;
    COMMAND_MESSAGE commandMessage;

    //printf("Log: Starting up\n");

#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant

    while (TRUE) {

#pragma warning(pop)

        //
        //  Check to see if we should shut down.
        //

        if (context->CleaningUp) {

            break;
        }

        //
        //  Request log data from MiniSpy.
        //

        commandMessage.Command = GetMiniSpyLog;

        hResult = FilterSendMessage( context->Port,
                                     &commandMessage,
                                     sizeof( COMMAND_MESSAGE ),
                                     buffer,
                                     sizeof(alignedBuffer),
                                     &bytesReturned );

        if (IS_ERROR( hResult )) {

            if (HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE ) == hResult) {

                printf( "The kernel component of minispy has unloaded. Exiting\n" );
                ExitProcess( 0 );
            } else {

                if (hResult != HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS )) {

                    printf( "UNEXPECTED ERROR received: %x\n", hResult );
                }

                Sleep( POLL_INTERVAL );
            }

            continue;
        }

        //
        //  Buffer is filled with a series of LOG_RECORD structures, one
        //  right after another.  Each LOG_RECORD says how long it is, so
        //  we know where the next LOG_RECORD begins.
        //

        pLogRecord = (PLOG_RECORD) buffer;
        used = 0;

        //
        //  Logic to write record to screen and/or file
        //

        for (;;) {

            if (used+FIELD_OFFSET(LOG_RECORD,Name) > bytesReturned) {

                break;
            }

            if (pLogRecord->Length < (sizeof(LOG_RECORD)+sizeof(WCHAR))) {

                printf( "UNEXPECTED LOG_RECORD->Length: length=%d expected>=%d\n",
                        pLogRecord->Length,
                        (ULONG)(sizeof(LOG_RECORD)+sizeof(WCHAR)));

                break;
            }

            used += pLogRecord->Length;

            if (used > bytesReturned) {

                printf( "UNEXPECTED LOG_RECORD size: used=%d bytesReturned=%d\n",
                        used,
                        bytesReturned);

                break;
            }

            pRecordData = &pLogRecord->Data;

            //
            //  See if a reparse point entry
            //

            if (FlagOn(pLogRecord->RecordType,RECORD_TYPE_FILETAG)) {

                if (!TranslateFileTag( pLogRecord )){

                    //
                    // If this is a reparse point that can't be interpreted, move on.
                    //

                    pLogRecord = (PLOG_RECORD)Add2Ptr(pLogRecord,pLogRecord->Length);
                    continue;
                }
            }

            if (context->LogToScreen) {

                ScreenDump( pLogRecord->SequenceNumber,
                            pLogRecord->Name,
                            pRecordData );
            }

            if (context->LogToFile) {

                FileDump( pLogRecord->SequenceNumber,
                          pLogRecord->Name,
                          pRecordData,
                          context->OutputFile );
            }

            //
            //  The RecordType could also designate that we are out of memory
            //  or hit our program defined memory limit, so check for these
            //  cases.
            //

            if (FlagOn(pLogRecord->RecordType,RECORD_TYPE_FLAG_OUT_OF_MEMORY)) {

                if (context->LogToScreen) {

                    printf( "M:  %08X System Out of Memory\n",
                            pLogRecord->SequenceNumber );
                }

                if (context->LogToFile) {

                    fprintf( context->OutputFile,
                             "M:\t0x%08X\tSystem Out of Memory\n",
                             pLogRecord->SequenceNumber );
                }

            } else if (FlagOn(pLogRecord->RecordType,RECORD_TYPE_FLAG_EXCEED_MEMORY_ALLOWANCE)) {

                if (context->LogToScreen) {

                    printf( "M:  %08X Exceeded Mamimum Allowed Memory Buffers\n",
                            pLogRecord->SequenceNumber );
                }

                if (context->LogToFile) {

                    fprintf( context->OutputFile,
                             "M:\t0x%08X\tExceeded Mamimum Allowed Memory Buffers\n",
                             pLogRecord->SequenceNumber );
                }
            }

            //
            // Move to next LOG_RECORD
            //

            pLogRecord = (PLOG_RECORD)Add2Ptr(pLogRecord,pLogRecord->Length);
        }

        //
        //  If we didn't get any data, pause for 1/2 second
        //

        if (bytesReturned == 0) {

            Sleep( POLL_INTERVAL );
        }
    }

    printf( "Log: Shutting down\n" );
    ReleaseSemaphore( context->ShutDown, 1, NULL );
    printf( "Log: All done\n" );
    return 0;
}


VOID
PrintIrpCode(
    _In_ UCHAR MajorCode,
    _In_ UCHAR MinorCode,
    _In_opt_ FILE *OutputFile,
    _In_ BOOLEAN PrintMajorCode
)
/*++

Routine Description:

    Display the operation code

Arguments:

    MajorCode - Major function code of operation

    MinorCode - Minor function code of operation

    OutputFile - If writing to a file (not the screen) the handle for that file

    PrintMajorCode - Only used when printing to the display:
        TRUE - if we want to display the MAJOR CODE
        FALSE - if we want to display the MINOR code

Return Value:

    None

--*/
{
    CHAR *irpMajorString, *irpMinorString = NULL;
    CHAR errorBuf[128];

    switch (MajorCode) {
        case IRP_MJ_CREATE:
            irpMajorString = IRP_MJ_CREATE_STRING;
            break;
        case IRP_MJ_CREATE_NAMED_PIPE:
            irpMajorString = IRP_MJ_CREATE_NAMED_PIPE_STRING;
            break;
        case IRP_MJ_CLOSE:
            irpMajorString = IRP_MJ_CLOSE_STRING;
            break;
        case IRP_MJ_READ:
            irpMajorString = IRP_MJ_READ_STRING;
            switch (MinorCode) {
                case IRP_MN_NORMAL:
                    irpMinorString = IRP_MN_NORMAL_STRING;
                    break;
                case IRP_MN_DPC:
                    irpMinorString = IRP_MN_DPC_STRING;
                    break;
                case IRP_MN_MDL:
                    irpMinorString = IRP_MN_MDL_STRING;
                    break;
                case IRP_MN_COMPLETE:
                    irpMinorString = IRP_MN_COMPLETE_STRING;
                    break;
                case IRP_MN_COMPRESSED:
                    irpMinorString = IRP_MN_COMPRESSED_STRING;
                    break;
                case IRP_MN_MDL_DPC:
                    irpMinorString = IRP_MN_MDL_DPC_STRING;
                    break;
                case IRP_MN_COMPLETE_MDL:
                    irpMinorString = IRP_MN_COMPLETE_MDL_STRING;
                    break;
                case IRP_MN_COMPLETE_MDL_DPC:
                    irpMinorString = IRP_MN_COMPLETE_MDL_DPC_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_WRITE:
            irpMajorString = IRP_MJ_WRITE_STRING;
            switch (MinorCode) {
                case IRP_MN_NORMAL:
                    irpMinorString = IRP_MN_NORMAL_STRING;
                    break;
                case IRP_MN_DPC:
                    irpMinorString = IRP_MN_DPC_STRING;
                    break;
                case IRP_MN_MDL:
                    irpMinorString = IRP_MN_MDL_STRING;
                    break;
                case IRP_MN_COMPLETE:
                    irpMinorString = IRP_MN_COMPLETE_STRING;
                    break;
                case IRP_MN_COMPRESSED:
                    irpMinorString = IRP_MN_COMPRESSED_STRING;
                    break;
                case IRP_MN_MDL_DPC:
                    irpMinorString = IRP_MN_MDL_DPC_STRING;
                    break;
                case IRP_MN_COMPLETE_MDL:
                    irpMinorString = IRP_MN_COMPLETE_MDL_STRING;
                    break;
                case IRP_MN_COMPLETE_MDL_DPC:
                    irpMinorString = IRP_MN_COMPLETE_MDL_DPC_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_QUERY_INFORMATION:
            irpMajorString = IRP_MJ_QUERY_INFORMATION_STRING;
            break;
        case IRP_MJ_SET_INFORMATION:
            irpMajorString = IRP_MJ_SET_INFORMATION_STRING;
            break;
        case IRP_MJ_QUERY_EA:
            irpMajorString = IRP_MJ_QUERY_EA_STRING;
            break;
        case IRP_MJ_SET_EA:
            irpMajorString = IRP_MJ_SET_EA_STRING;
            break;
        case IRP_MJ_FLUSH_BUFFERS:
            irpMajorString = IRP_MJ_FLUSH_BUFFERS_STRING;
            break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            irpMajorString = IRP_MJ_QUERY_VOLUME_INFORMATION_STRING;
            break;
        case IRP_MJ_SET_VOLUME_INFORMATION:
            irpMajorString = IRP_MJ_SET_VOLUME_INFORMATION_STRING;
            break;
        case IRP_MJ_DIRECTORY_CONTROL:
            irpMajorString = IRP_MJ_DIRECTORY_CONTROL_STRING;
            switch (MinorCode) {
                case IRP_MN_QUERY_DIRECTORY:
                    irpMinorString = IRP_MN_QUERY_DIRECTORY_STRING;
                    break;
                case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
                    irpMinorString = IRP_MN_NOTIFY_CHANGE_DIRECTORY_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            irpMajorString = IRP_MJ_FILE_SYSTEM_CONTROL_STRING;
            switch (MinorCode) {
                case IRP_MN_USER_FS_REQUEST:
                    irpMinorString = IRP_MN_USER_FS_REQUEST_STRING;
                    break;
                case IRP_MN_MOUNT_VOLUME:
                    irpMinorString = IRP_MN_MOUNT_VOLUME_STRING;
                    break;
                case IRP_MN_VERIFY_VOLUME:
                    irpMinorString = IRP_MN_VERIFY_VOLUME_STRING;
                    break;
                case IRP_MN_LOAD_FILE_SYSTEM:
                    irpMinorString = IRP_MN_LOAD_FILE_SYSTEM_STRING;
                    break;
                case IRP_MN_TRACK_LINK:
                    irpMinorString = IRP_MN_TRACK_LINK_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_DEVICE_CONTROL:
            irpMajorString = IRP_MJ_DEVICE_CONTROL_STRING;
            switch (MinorCode) {
                case IRP_MN_SCSI_CLASS:
                    irpMinorString = IRP_MN_SCSI_CLASS_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            irpMajorString = IRP_MJ_INTERNAL_DEVICE_CONTROL_STRING;
            break;
        case IRP_MJ_SHUTDOWN:
            irpMajorString = IRP_MJ_SHUTDOWN_STRING;
            break;
        case IRP_MJ_LOCK_CONTROL:
            irpMajorString = IRP_MJ_LOCK_CONTROL_STRING;
            switch (MinorCode) {
                case IRP_MN_LOCK:
                    irpMinorString = IRP_MN_LOCK_STRING;
                    break;
                case IRP_MN_UNLOCK_SINGLE:
                    irpMinorString = IRP_MN_UNLOCK_SINGLE_STRING;
                    break;
                case IRP_MN_UNLOCK_ALL:
                    irpMinorString = IRP_MN_UNLOCK_ALL_STRING;
                    break;
                case IRP_MN_UNLOCK_ALL_BY_KEY:
                    irpMinorString = IRP_MN_UNLOCK_ALL_BY_KEY_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_CLEANUP:
            irpMajorString = IRP_MJ_CLEANUP_STRING;
            break;
        case IRP_MJ_CREATE_MAILSLOT:
            irpMajorString = IRP_MJ_CREATE_MAILSLOT_STRING;
            break;
        case IRP_MJ_QUERY_SECURITY:
            irpMajorString = IRP_MJ_QUERY_SECURITY_STRING;
            break;
        case IRP_MJ_SET_SECURITY:
            irpMajorString = IRP_MJ_SET_SECURITY_STRING;
            break;
        case IRP_MJ_POWER:
            irpMajorString = IRP_MJ_POWER_STRING;
            switch (MinorCode) {
                case IRP_MN_WAIT_WAKE:
                    irpMinorString = IRP_MN_WAIT_WAKE_STRING;
                    break;
                case IRP_MN_POWER_SEQUENCE:
                    irpMinorString = IRP_MN_POWER_SEQUENCE_STRING;
                    break;
                case IRP_MN_SET_POWER:
                    irpMinorString = IRP_MN_SET_POWER_STRING;
                    break;
                case IRP_MN_QUERY_POWER:
                    irpMinorString = IRP_MN_QUERY_POWER_STRING;
                    break;
                default :
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_SYSTEM_CONTROL:
            irpMajorString = IRP_MJ_SYSTEM_CONTROL_STRING;
            switch (MinorCode) {
                case IRP_MN_QUERY_ALL_DATA:
                    irpMinorString = IRP_MN_QUERY_ALL_DATA_STRING;
                    break;
                case IRP_MN_QUERY_SINGLE_INSTANCE:
                    irpMinorString = IRP_MN_QUERY_SINGLE_INSTANCE_STRING;
                    break;
                case IRP_MN_CHANGE_SINGLE_INSTANCE:
                    irpMinorString = IRP_MN_CHANGE_SINGLE_INSTANCE_STRING;
                    break;
                case IRP_MN_CHANGE_SINGLE_ITEM:
                    irpMinorString = IRP_MN_CHANGE_SINGLE_ITEM_STRING;
                    break;
                case IRP_MN_ENABLE_EVENTS:
                    irpMinorString = IRP_MN_ENABLE_EVENTS_STRING;
                    break;
                case IRP_MN_DISABLE_EVENTS:
                    irpMinorString = IRP_MN_DISABLE_EVENTS_STRING;
                    break;
                case IRP_MN_ENABLE_COLLECTION:
                    irpMinorString = IRP_MN_ENABLE_COLLECTION_STRING;
                    break;
                case IRP_MN_DISABLE_COLLECTION:
                    irpMinorString = IRP_MN_DISABLE_COLLECTION_STRING;
                    break;
                case IRP_MN_REGINFO:
                    irpMinorString = IRP_MN_REGINFO_STRING;
                    break;
                case IRP_MN_EXECUTE_METHOD:
                    irpMinorString = IRP_MN_EXECUTE_METHOD_STRING;
                    break;
                default :
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;

        case IRP_MJ_DEVICE_CHANGE:
            irpMajorString = IRP_MJ_DEVICE_CHANGE_STRING;
            break;
        case IRP_MJ_QUERY_QUOTA:
            irpMajorString = IRP_MJ_QUERY_QUOTA_STRING;
            break;
        case IRP_MJ_SET_QUOTA:
            irpMajorString = IRP_MJ_SET_QUOTA_STRING;
            break;
        case IRP_MJ_PNP:
            irpMajorString = IRP_MJ_PNP_STRING;
            switch (MinorCode) {
                case IRP_MN_START_DEVICE:
                    irpMinorString = IRP_MN_START_DEVICE_STRING;
                    break;
                case IRP_MN_QUERY_REMOVE_DEVICE:
                    irpMinorString = IRP_MN_QUERY_REMOVE_DEVICE_STRING;
                    break;
                case IRP_MN_REMOVE_DEVICE:
                    irpMinorString = IRP_MN_REMOVE_DEVICE_STRING;
                    break;
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                    irpMinorString = IRP_MN_CANCEL_REMOVE_DEVICE_STRING;
                    break;
                case IRP_MN_STOP_DEVICE:
                    irpMinorString = IRP_MN_STOP_DEVICE_STRING;
                    break;
                case IRP_MN_QUERY_STOP_DEVICE:
                    irpMinorString = IRP_MN_QUERY_STOP_DEVICE_STRING;
                    break;
                case IRP_MN_CANCEL_STOP_DEVICE:
                    irpMinorString = IRP_MN_CANCEL_STOP_DEVICE_STRING;
                    break;
                case IRP_MN_QUERY_DEVICE_RELATIONS:
                    irpMinorString = IRP_MN_QUERY_DEVICE_RELATIONS_STRING;
                    break;
                case IRP_MN_QUERY_INTERFACE:
                    irpMinorString = IRP_MN_QUERY_INTERFACE_STRING;
                    break;
                case IRP_MN_QUERY_CAPABILITIES:
                    irpMinorString = IRP_MN_QUERY_CAPABILITIES_STRING;
                    break;
                case IRP_MN_QUERY_RESOURCES:
                    irpMinorString = IRP_MN_QUERY_RESOURCES_STRING;
                    break;
                case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                    irpMinorString = IRP_MN_QUERY_RESOURCE_REQUIREMENTS_STRING;
                    break;
                case IRP_MN_QUERY_DEVICE_TEXT:
                    irpMinorString = IRP_MN_QUERY_DEVICE_TEXT_STRING;
                    break;
                case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
                    irpMinorString = IRP_MN_FILTER_RESOURCE_REQUIREMENTS_STRING;
                    break;
                case IRP_MN_READ_CONFIG:
                    irpMinorString = IRP_MN_READ_CONFIG_STRING;
                    break;
                case IRP_MN_WRITE_CONFIG:
                    irpMinorString = IRP_MN_WRITE_CONFIG_STRING;
                    break;
                case IRP_MN_EJECT:
                    irpMinorString = IRP_MN_EJECT_STRING;
                    break;
                case IRP_MN_SET_LOCK:
                    irpMinorString = IRP_MN_SET_LOCK_STRING;
                    break;
                case IRP_MN_QUERY_ID:
                    irpMinorString = IRP_MN_QUERY_ID_STRING;
                    break;
                case IRP_MN_QUERY_PNP_DEVICE_STATE:
                    irpMinorString = IRP_MN_QUERY_PNP_DEVICE_STATE_STRING;
                    break;
                case IRP_MN_QUERY_BUS_INFORMATION:
                    irpMinorString = IRP_MN_QUERY_BUS_INFORMATION_STRING;
                    break;
                case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                    irpMinorString = IRP_MN_DEVICE_USAGE_NOTIFICATION_STRING;
                    break;
                case IRP_MN_SURPRISE_REMOVAL:
                    irpMinorString = IRP_MN_SURPRISE_REMOVAL_STRING;
                    break;
                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
                    irpMinorString = IRP_MN_QUERY_LEGACY_BUS_INFORMATION_STRING;
                    break;
                default :
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp minor code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;


        case IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION:
            irpMajorString = IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION_STRING;
            break;

        case IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION:
            irpMajorString = IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION_STRING;
            break;

        case IRP_MJ_ACQUIRE_FOR_MOD_WRITE:
            irpMajorString = IRP_MJ_ACQUIRE_FOR_MOD_WRITE_STRING;
            break;

        case IRP_MJ_RELEASE_FOR_MOD_WRITE:
            irpMajorString = IRP_MJ_RELEASE_FOR_MOD_WRITE_STRING;
            break;

        case IRP_MJ_ACQUIRE_FOR_CC_FLUSH:
            irpMajorString = IRP_MJ_ACQUIRE_FOR_CC_FLUSH_STRING;
            break;

        case IRP_MJ_RELEASE_FOR_CC_FLUSH:
            irpMajorString = IRP_MJ_RELEASE_FOR_CC_FLUSH_STRING;
            break;

        case IRP_MJ_NOTIFY_STREAM_FO_CREATION:
            irpMajorString = IRP_MJ_NOTIFY_STREAM_FO_CREATION_STRING;
            break;



        case IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE:
            irpMajorString = IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE_STRING;
            break;

        case IRP_MJ_NETWORK_QUERY_OPEN:
            irpMajorString = IRP_MJ_NETWORK_QUERY_OPEN_STRING;
            break;

        case IRP_MJ_MDL_READ:
            irpMajorString = IRP_MJ_MDL_READ_STRING;
            break;

        case IRP_MJ_MDL_READ_COMPLETE:
            irpMajorString = IRP_MJ_MDL_READ_COMPLETE_STRING;
            break;

        case IRP_MJ_PREPARE_MDL_WRITE:
            irpMajorString = IRP_MJ_PREPARE_MDL_WRITE_STRING;
            break;

        case IRP_MJ_MDL_WRITE_COMPLETE:
            irpMajorString = IRP_MJ_MDL_WRITE_COMPLETE_STRING;
            break;

        case IRP_MJ_VOLUME_MOUNT:
            irpMajorString = IRP_MJ_VOLUME_MOUNT_STRING;
            break;

        case IRP_MJ_VOLUME_DISMOUNT:
            irpMajorString = IRP_MJ_VOLUME_DISMOUNT_STRING;
            break;

        case IRP_MJ_TRANSACTION_NOTIFY:
            irpMajorString = IRP_MJ_TRANSACTION_NOTIFY_STRING;
            switch (MinorCode) {
                case 0:
                    irpMinorString = TRANSACTION_BEGIN;
                    break;
                case TRANSACTION_NOTIFY_PREPREPARE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_PREPREPARE_STRING;
                    break;
                case TRANSACTION_NOTIFY_PREPARE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_PREPARE_STRING;
                    break;
                case TRANSACTION_NOTIFY_COMMIT_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_COMMIT_STRING;
                    break;
                case TRANSACTION_NOTIFY_COMMIT_FINALIZE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_COMMIT_FINALIZE_STRING;
                    break;
                case TRANSACTION_NOTIFY_ROLLBACK_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_ROLLBACK_STRING;
                    break;
                case TRANSACTION_NOTIFY_PREPREPARE_COMPLETE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_PREPREPARE_COMPLETE_STRING;
                    break;
                case TRANSACTION_NOTIFY_PREPARE_COMPLETE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_COMMIT_COMPLETE_STRING;
                    break;
                case TRANSACTION_NOTIFY_ROLLBACK_COMPLETE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_ROLLBACK_COMPLETE_STRING;
                    break;
                case TRANSACTION_NOTIFY_RECOVER_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_RECOVER_STRING;
                    break;
                case TRANSACTION_NOTIFY_SINGLE_PHASE_COMMIT_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_SINGLE_PHASE_COMMIT_STRING;
                    break;
                case TRANSACTION_NOTIFY_DELEGATE_COMMIT_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_DELEGATE_COMMIT_STRING;
                    break;
                case TRANSACTION_NOTIFY_RECOVER_QUERY_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_RECOVER_QUERY_STRING;
                    break;
                case TRANSACTION_NOTIFY_ENLIST_PREPREPARE_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_ENLIST_PREPREPARE_STRING;
                    break;
                case TRANSACTION_NOTIFY_LAST_RECOVER_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_LAST_RECOVER_STRING;
                    break;
                case TRANSACTION_NOTIFY_INDOUBT_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_INDOUBT_STRING;
                    break;
                case TRANSACTION_NOTIFY_PROPAGATE_PULL_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_PROPAGATE_PULL_STRING;
                    break;
                case TRANSACTION_NOTIFY_PROPAGATE_PUSH_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_PROPAGATE_PUSH_STRING;
                    break;
                case TRANSACTION_NOTIFY_MARSHAL_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_MARSHAL_STRING;
                    break;
                case TRANSACTION_NOTIFY_ENLIST_MASK_CODE:
                    irpMinorString = TRANSACTION_NOTIFY_ENLIST_MASK_STRING;
                    break;
                default:
                    sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Transaction notication code (%u)",MinorCode);
                    irpMinorString = errorBuf;
            }
            break;


        default:
            sprintf_s(errorBuf,sizeof(errorBuf),"Unknown Irp major function (%d)",MajorCode);
            irpMajorString = errorBuf;
            break;
    }

    if (OutputFile) {

        if (irpMinorString) {

            fprintf(OutputFile, "\t%-35s\t%-35s", irpMajorString, irpMinorString);

        } else {

            fprintf(OutputFile, "\t%-35s\t                                   ", irpMajorString);
        }

    } else {

        if (PrintMajorCode) {

            printf("%-35s ", irpMajorString);

        } else {

            if (irpMinorString) {

                printf("                                                                     %-35s\n",
                        irpMinorString);
            }
        }
    }
}


ULONG
FormatSystemTime(
    _In_ SYSTEMTIME *SystemTime,
    _Out_writes_bytes_(BufferLength) CHAR *Buffer,
    _In_ ULONG BufferLength
    )
/*++
Routine Description:

    Formats the values in a SystemTime struct into the buffer
    passed in.  The resulting string is NULL terminated.  The format
    for the time is:
        hours:minutes:seconds:milliseconds

Arguments:

    SystemTime - the struct to format
    Buffer - the buffer to place the formatted time in
    BufferLength - the size of the buffer

Return Value:

    The length of the string returned in Buffer.

--*/
{
    ULONG returnLength = 0;

    if (BufferLength < TIME_BUFFER_LENGTH) {

        //
        // Buffer is too short so exit
        //

        return 0;
    }

    returnLength = sprintf_s( Buffer,
                            BufferLength,
                            "%02d:%02d:%02d:%03d",
                            SystemTime->wHour,
                            SystemTime->wMinute,
                            SystemTime->wSecond,
                            SystemTime->wMilliseconds );

    return returnLength;
}


VOID
FileDump (
    _In_ ULONG SequenceNumber,
    _In_ WCHAR CONST *Name,
    _In_ PRECORD_DATA RecordData,
    _In_ FILE *File
    )
/*++
Routine Description:

    Prints a Data log record to the specified file.  The output is in a tab
    delimited format with the fields in the following order:

    SequenceNumber, OriginatingTime, CompletionTime, CallbackMajorId, CallbackMinorId,
    Flags, NoCache, Paging I/O, Synchronous, Synchronous paging, FileName,
    ReturnStatus, FileName


Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the name of the file that this Irp relates to
    RecordData - the Data record to print
    File - the file to print to

Return Value:

    None.

--*/
{
    FILETIME localTime;
    SYSTEMTIME systemTime;
    CHAR time[TIME_BUFFER_LENGTH];
    static BOOLEAN didFileHeader = FALSE;

    //
    // Is this an Irp or a FastIo?
    //

    if (!didFileHeader) {

#if defined(_WIN64)
        fprintf( File, "Opr\t  SeqNum  \t PreOp Time \tPostOp Time \t Process.Thrd\t          Major Operation          \t          Minor Operation          \t   IrpFlags    \t      DevObj      \t     FileObj      \t    Transactn     \t    status:inform            \t      Arg 1       \t      Arg 2       \t      Arg 3       \t      Arg 4       \t      Arg 5       \t  Arg 6   \tName\n");
        fprintf( File, "---\t----------\t------------\t------------\t-------------\t-----------------------------------\t-----------------------------------\t---------------\t------------------\t------------------\t------------------\t-----------------------------\t------------------\t------------------\t------------------\t------------------\t------------------\t----------\t--------------------------------------------------\n");
#else
        fprintf( File, "Opr\t  SeqNum  \t PreOp Time \tPostOp Time \t Process.Thrd\t          Major Operation          \t          Minor Operation          \t   IrpFlags    \t  DevObj  \t FileObj  \tTransactn \t    status:inform    \t  Arg 1   \t  Arg 2   \t  Arg 3   \t  Arg 4   \t  Arg 5   \t  Arg 6   \tName\n");
        fprintf( File, "---\t----------\t------------\t------------\t-------------\t-----------------------------------\t-----------------------------------\t---------------\t----------\t----------\t----------\t---------------------\t----------\t----------\t----------\t----------\t----------\t----------\t--------------------------------------------------\n");
#endif
        didFileHeader = TRUE;
    }

    //
    // Is this an Irp or a FastIo?
    //

    if (RecordData->Flags & FLT_CALLBACK_DATA_IRP_OPERATION) {

        fprintf( File, "IRP");

    } else if (RecordData->Flags & FLT_CALLBACK_DATA_FAST_IO_OPERATION) {

        fprintf( File, "FIO");

    } else if (RecordData->Flags & FLT_CALLBACK_DATA_FS_FILTER_OPERATION) {

        fprintf( File, "FSF");

    } else {

        fprintf( File, "ERR");
    }

    //
    //  Print the sequence number
    //

    fprintf( File, "\t0x%08X", SequenceNumber );

    //
    // Convert originating time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordData->OriginatingTime),
                             &localTime );
    FileTimeToSystemTime( &localTime,
                          &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12s", time );

    } else {

        fprintf( File, "\t%-12s", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordData->CompletionTime),
                             &localTime );
    FileTimeToSystemTime( &localTime,
                          &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        fprintf( File, "\t%-12s", time );

    } else {

        fprintf( File, "\t%-12s", TIME_ERROR );
    }

    fprintf(File, "\t%8Ix.%-4Ix ", RecordData->ProcessId, RecordData->ThreadId);

    PrintIrpCode( RecordData->CallbackMajorId,
                  RecordData->CallbackMinorId,
                  File,
                  TRUE );

    //
    // Interpret set IrpFlags
    //

    fprintf( File, "\t0x%08lx ", RecordData->IrpFlags );
    fprintf( File, "%s", (RecordData->IrpFlags & IRP_NOCACHE) ? "N":"-" );
    fprintf( File, "%s", (RecordData->IrpFlags & IRP_PAGING_IO) ? "P":"-" );
    fprintf( File, "%s", (RecordData->IrpFlags & IRP_SYNCHRONOUS_API) ? "S":"-" );
    fprintf( File, "%s", (RecordData->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ? "Y":"-" );

    fprintf( File, "\t0x%08p", (PVOID) RecordData->DeviceObject );
    fprintf( File, "\t0x%08p", (PVOID) RecordData->FileObject );
    fprintf( File, "\t0x%08p", (PVOID) RecordData->Transaction );
    fprintf( File, "\t0x%08lx:0x%p", RecordData->Status, (PVOID)RecordData->Information );

    fprintf( File, "\t0x%p", RecordData->Arg1 );
    fprintf( File, "\t0x%p", RecordData->Arg2 );
    fprintf( File, "\t0x%p", RecordData->Arg3 );
    fprintf( File, "\t0x%p", RecordData->Arg4 );
    fprintf( File, "\t0x%p", RecordData->Arg5 );
    fprintf( File, "\t0x%08I64x", RecordData->Arg6.QuadPart );

    fprintf( File, "\t%S", Name );
    fprintf( File, "\n" );
}


VOID
ScreenDump(
    _In_ ULONG SequenceNumber,
    _In_ WCHAR CONST *Name,
    _In_ PRECORD_DATA RecordData
    )
/*++
Routine Description:

    Prints a Irp log record to the screen in the following order:
    SequenceNumber, OriginatingTime, CompletionTime, IrpMajor, IrpMinor,
    Flags, IrpFlags, NoCache, Paging I/O, Synchronous, Synchronous paging,
    FileName, ReturnStatus, FileName

Arguments:

    SequenceNumber - the sequence number for this log record
    Name - the file name to which this Irp relates
    RecordData - the Irp record to print

Return Value:

    None.

--*/
{
    FILETIME localTime;
    SYSTEMTIME systemTime;
    CHAR time[TIME_BUFFER_LENGTH];
    static BOOLEAN didScreenHeader = FALSE;

    //
    // Is this an Irp or a FastIo?
    //

    if (!didScreenHeader) {

#if defined(_WIN64)
        printf("Opr  SeqNum   PreOp Time  PostOp Time   Process.Thrd      Major/Minor Operation          IrpFlags          DevObj           FileObj          Transact       status:inform                               Arguments                                                                             Name\n");
        printf("--- -------- ------------ ------------ ------------- ----------------------------------- ------------- ---------------- ---------------- ---------------- ------------------------- --------------------------------------------------------------------------------------------------------- -----------------------------------\n");
#else
        printf("Opr  SeqNum   PreOp Time  PostOp Time   Process.Thrd      Major/Minor Operation          IrpFlags      DevObj   FileObj  Transact   status:inform                               Arguments                             Name\n");
        printf("--- -------- ------------ ------------ ------------- ----------------------------------- ------------- -------- -------- -------- ----------------- ----------------------------------------------------------------- -----------------------------------\n");
#endif
        didScreenHeader = TRUE;
    }

    //
    //  Display informatoin
    //

    if (RecordData->Flags & FLT_CALLBACK_DATA_IRP_OPERATION) {

        printf( "IRP ");

    } else if (RecordData->Flags & FLT_CALLBACK_DATA_FAST_IO_OPERATION) {

        printf( "FIO ");

    } else if (RecordData->Flags & FLT_CALLBACK_DATA_FS_FILTER_OPERATION) {

        printf( "FSF " );
    } else {

        printf( "ERR ");
    }

    printf( "%08X ", SequenceNumber );


    //
    // Convert originating time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordData->OriginatingTime),
                             &localTime );
    FileTimeToSystemTime( &localTime,
                          &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12s ", time );

    } else {

        printf( "%-12s ", TIME_ERROR );
    }

    //
    // Convert completion time
    //

    FileTimeToLocalFileTime( (FILETIME *)&(RecordData->CompletionTime),
                             &localTime );
    FileTimeToSystemTime( &localTime,
                          &systemTime );

    if (FormatSystemTime( &systemTime, time, TIME_BUFFER_LENGTH )) {

        printf( "%-12s ", time );

    } else {

        printf( "%-12s ", TIME_ERROR );
    }

    printf("%8Ix.%-4Ix ", RecordData->ProcessId, RecordData->ThreadId);

    PrintIrpCode( RecordData->CallbackMajorId,
                  RecordData->CallbackMinorId,
                  NULL,
                  TRUE );

    //
    // Interpret set IrpFlags
    //

    printf( "%08lx ", RecordData->IrpFlags );
    printf( "%s", (RecordData->IrpFlags & IRP_NOCACHE) ? "N":"-" );
    printf( "%s", (RecordData->IrpFlags & IRP_PAGING_IO) ? "P":"-" );
    printf( "%s", (RecordData->IrpFlags & IRP_SYNCHRONOUS_API) ? "S":"-" );
    printf( "%s ", (RecordData->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ? "Y":"-" );

    printf( "%08p ", (PVOID) RecordData->DeviceObject );
    printf( "%08p ", (PVOID) RecordData->FileObject );
    printf( "%08p ", (PVOID) RecordData->Transaction );
    printf( "%08lx:%p ", RecordData->Status, (PVOID)RecordData->Information );

    printf( "1:%p 2:%p 3:%p 4:%p 5:%p 6:%08I64x ",
            RecordData->Arg1,
            RecordData->Arg2,
            RecordData->Arg3,
            RecordData->Arg4,
            RecordData->Arg5,
            RecordData->Arg6.QuadPart );

    printf( "%S", Name );
    printf( "\n" );
    PrintIrpCode( RecordData->CallbackMajorId,
                  RecordData->CallbackMinorId,
                  NULL,
                  FALSE );
}


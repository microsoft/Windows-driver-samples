/*++

Copyright (c) 1989-2002  Microsoft Corporation

Module Name:

    mspyLib.c

Abstract:
    This contains library support routines for MiniSpy

Environment:

    Kernel mode

--*/

#include <initguid.h>
#include <stdio.h>

#include "mspyKern.h"

//
// Can't pull in wsk.h until after MINISPY_VISTA is defined
//

#if MINISPY_VISTA
#include <ntifs.h>
#include <wsk.h>
#endif

//---------------------------------------------------------------------------
//  Assign text sections for each routine.
//---------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, SpyReadDriverParameters)
#if MINISPY_VISTA
    #pragma alloc_text(PAGE, SpyBuildEcpDataString)
    #pragma alloc_text(PAGE, SpyParseEcps)
#endif
#endif

UCHAR TxNotificationToMinorCode (
    _In_ ULONG TxNotification
    )
/*++

Routine Description:

    This routine has been written to convert a transaction notification code
    to an Irp minor code. This function is needed because RECORD_DATA has a
    UCHAR field for the Irp minor code whereas TxNotification is ULONG. As
    of now all this function does is compute log_base_2(TxNotification) + 1.
    That fits our need for now but might have to be evolved later. This
    function is intricately tied with the enumeration TRANSACTION_NOTIFICATION_CODES
    in mspyLog.h and the case statements related to transactions in the function
    PrintIrpCode (Minispy\User\mspyLog.c).

Arguments:

    TxNotification - The transaction notification received.

Return Value:

    0 if TxNotification is 0;
    log_base_2(TxNotification) + 1 otherwise.

--*/
{
    UCHAR count = 0;

    if (TxNotification == 0)
        return 0;

    //
    //  This assert verifies if no more than one flag is set
    //  in the TxNotification variable. TxNotification flags are
    //  supposed to be mutually exclusive. The assert below verifies
    //  if the value of TxNotification is a power of 2. If it is not
    //  then we will break.
    //

    FLT_ASSERT( !(( TxNotification ) & ( TxNotification - 1 )) );

    while (TxNotification) {

        count++;

        TxNotification >>= 1;

        //
        //  If we hit this assert then we have more notification codes than
        //  can fit in a UCHAR. We need to revaluate our approach for
        //  storing minor codes now.
        //

        FLT_ASSERT( count != 0 );
    }

    return ( count );
}


//---------------------------------------------------------------------------
//                    Log Record allocation routines
//---------------------------------------------------------------------------

PRECORD_LIST
SpyAllocateBuffer (
    _Out_ PULONG RecordType
    )
/*++

Routine Description:

    Allocates a new buffer from the MiniSpyData.FreeBufferList if there is
    enough memory to do so and we have not exceed our maximum buffer
    count.

    NOTE:  Because there is no interlock between testing if we have exceeded
           the record allocation limit and actually increment the in use
           count it is possible to temporarily allocate one or two buffers
           more then the limit.  Because this is such a rare situation there
           is not point to handling this.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path or at DPC level.

Arguments:

    RecordType - Receives information on what type of record was allocated.

Return Value:

    Pointer to the allocated buffer, or NULL if the allocation failed.

--*/
{
    PVOID newBuffer;
    ULONG newRecordType = RECORD_TYPE_NORMAL;

    //
    //  See if we have room to allocate more buffers
    //

    if (MiniSpyData.RecordsAllocated < MiniSpyData.MaxRecordsToAllocate) {

        InterlockedIncrement( &MiniSpyData.RecordsAllocated );

        newBuffer = ExAllocateFromNPagedLookasideList( &MiniSpyData.FreeBufferList );

        if (newBuffer == NULL) {

            //
            //  We failed to allocate the memory.  Decrement our global count
            //  and return what type of memory we have.
            //

            InterlockedDecrement( &MiniSpyData.RecordsAllocated );

            newRecordType = RECORD_TYPE_FLAG_OUT_OF_MEMORY;
        }

    } else {

        //
        //  No more room to allocate memory, return we didn't get a buffer
        //  and why.
        //

        newRecordType = RECORD_TYPE_FLAG_EXCEED_MEMORY_ALLOWANCE;
        newBuffer = NULL;
    }

    *RecordType = newRecordType;
    return newBuffer;
}


VOID
SpyFreeBuffer (
    _In_ PVOID Buffer
    )
/*++

Routine Description:

    Free an allocate buffer.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path or at DPC level.

Arguments:

    Buffer - The buffer to free.

Return Value:

    None.

--*/
{
    //
    //  Free the memory, update the counter
    //

    InterlockedDecrement( &MiniSpyData.RecordsAllocated );
    ExFreeToNPagedLookasideList( &MiniSpyData.FreeBufferList, Buffer );
}


//---------------------------------------------------------------------------
//                    Logging routines
//---------------------------------------------------------------------------

PRECORD_LIST
SpyNewRecord (
    VOID
    )
/*++

Routine Description:

    Allocates a new RECORD_LIST structure if there is enough memory to do so. A
    sequence number is updated for each request for a new record.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path or at DPC level.

Arguments:

    None

Return Value:

    Pointer to the RECORD_LIST allocated, or NULL if no memory is available.

--*/
{
    PRECORD_LIST newRecord;
    ULONG initialRecordType;

    //
    //  Allocate the buffer
    //

    newRecord = SpyAllocateBuffer( &initialRecordType );

    if (newRecord == NULL) {

        //
        //  We could not allocate a record, see if the static buffer is
        //  in use.  If not, we will use it
        //

        if (!InterlockedExchange( &MiniSpyData.StaticBufferInUse, TRUE )) {

            newRecord = (PRECORD_LIST)MiniSpyData.OutOfMemoryBuffer;
            initialRecordType |= RECORD_TYPE_FLAG_STATIC;
        }
    }

    //
    //  If we got a record (doesn't matter if it is static or not), init it
    //

    if (newRecord != NULL) {

        //
        // Init the new record
        //

        newRecord->LogRecord.RecordType = initialRecordType;
        newRecord->LogRecord.Length = sizeof(LOG_RECORD);
        newRecord->LogRecord.SequenceNumber = InterlockedIncrement( &MiniSpyData.LogSequenceNumber );
        RtlZeroMemory( &newRecord->LogRecord.Data, sizeof( RECORD_DATA ) );
    }

    return( newRecord );
}


VOID
SpyFreeRecord (
    _In_ PRECORD_LIST Record
    )
/*++

Routine Description:

    Free the given buffer

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path or at DPC level.

Arguments:

    Record - the buffer to free

Return Value:

    None.

--*/
{
    if (FlagOn(Record->LogRecord.RecordType,RECORD_TYPE_FLAG_STATIC)) {

        //
        // This was our static buffer, mark it available.
        //

        FLT_ASSERT(MiniSpyData.StaticBufferInUse);
        MiniSpyData.StaticBufferInUse = FALSE;

    } else {

        SpyFreeBuffer( Record );
    }
}

#if MINISPY_VISTA

VOID
SpyBuildEcpDataString (
    _In_ PRECORD_LIST RecordList,
    _Inout_ PUNICODE_STRING EcpData,
    _In_reads_(NumKnownEcps) PVOID * ContextPointers
    )
/*++

Routine Description:

    Given the ECP presence data and context pointers located in SpyParseEcps,
    uses _snwprintf to write a human-readable log output to a string provided.

Arguments:

    RecordList - Pointer to the record, so we can see ECP count and masking

    EcpData - Pointer to string to receive formatted ECP log

    ContextPointers - Pointer to array of pointers, each of which is either NULL
                      or a context structure specific to a given type of ECP

Return Value:

    None.

--*/
{
    ULONG knownCount = 0;
    SHORT wcharsCopied = 0;
    PRECORD_DATA recordData = &RecordList->LogRecord.Data;
    PWCHAR printPointer = EcpData->Buffer;

#if MINISPY_WIN7
    TCHAR addressBuffer[ADDRESS_STRING_BUFFER_SIZE];
    ULONG addressBufferLen;
    LONG addressConvStatus;
#endif

    PAGED_CODE();

    FLT_ASSERT(NULL != ContextPointers);

    //
    //  Print initial ECP text
    //
    //  NOTE: We don't check the return value of _snwprintf until the very end
    //  of this function. Because of this, if we run out of buffer space before we
    //  have printed all our information, we keep calling _snwprintf, although it
    //  does nothing. This is deliberate in the interest of keeping the code
    //  somewhat clean.
    //

    #pragma prefast(push)
    #pragma prefast(disable: __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY __WARNING_BANNED_API_USAGE, "reviewed and safe usage")
    //  Prefast complains here because _snwprintf has some oddities.
    //  We've code reviewed to ensure safe usage.

    wcharsCopied = (SHORT) _snwprintf( printPointer,
                                       MAX_NAME_WCHARS_LESS_NULL,
                                       L"[%d ECPs:",
                                       recordData->EcpCount );

    //
    //  Next, check all the known ECPs against the mask that was set in SpyParseEcps.
    //  If we recognize any of the ECPs, add their data to the log string.
    //

#if MINISPY_WIN7

    //
    //  Oplock key ECP
    //

    if (FlagOn( recordData->KnownEcpMask, ECP_TYPE_FLAG_OPLOCK_KEY )) {

        POPLOCK_KEY_ECP_CONTEXT oplockEcpContext = NULL;
        LPGUID oplockKeyGuid;
        UNICODE_STRING oplockKeyGuidString;

        knownCount++;

        //
        //  We now know this context pointer points to a
        //  OPLOCK_KEY_ECP_CONTEXT structure
        //

        oplockEcpContext = (POPLOCK_KEY_ECP_CONTEXT) ContextPointers[EcpOplockKey];

        FLT_ASSERT(NULL != oplockEcpContext);

        oplockKeyGuid = &oplockEcpContext->OplockKey;

        if (NT_SUCCESS(RtlStringFromGUID( oplockKeyGuid, 
                                          &oplockKeyGuidString ))) {

            //
            //  Format an output string to display the key in GUID form
            //

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s OPLOCK KEY: %wZ;",
                                               printPointer,
                                               &oplockKeyGuidString );

            RtlFreeUnicodeString( &oplockKeyGuidString );

        } else {

            //
            //  Error processing the GUID
            //

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s INVALID OPLOCK KEY;",
                                               printPointer );
        }
    }

    //
    //  NFS ECP
    //

    if (FlagOn( recordData->KnownEcpMask, ECP_TYPE_FLAG_NFS )) {

        PNFS_OPEN_ECP_CONTEXT nfsEcpContext = NULL;
        PUNICODE_STRING nfsShareNameString;
        PSOCKADDR_STORAGE_NFS nfsClientSocketAddr;

        knownCount++;

        //
        //  We now know this context pointer points to a
        //  NFS_OPEN_ECP_CONTEXT structure
        //

        nfsEcpContext= (PNFS_OPEN_ECP_CONTEXT) ContextPointers[EcpNfsOpen];

        FLT_ASSERT(NULL != nfsEcpContext);

        nfsShareNameString = nfsEcpContext->ExportAlias;
        nfsClientSocketAddr = nfsEcpContext->ClientSocketAddress;

        //
        //  Print the share name, if the string (optional) is present
        //

        if (nfsShareNameString) {

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s NFS SHARE NAME: %wZ,",
                                               printPointer,
                                               nfsShareNameString );
        }

        FLT_ASSERT(nfsClientSocketAddr != NULL);

        addressConvStatus = STATUS_INVALID_PARAMETER;
        addressBufferLen = ADDRESS_STRING_BUFFER_SIZE;

        if (nfsClientSocketAddr->ss_family == AF_INET) {

            PSOCKADDR_IN ipv4SocketAddr = (PSOCKADDR_IN) nfsClientSocketAddr;

            //
            //  Format IPv4 address and port
            //

            addressConvStatus = RtlIpv4AddressToStringEx( 
                                    &ipv4SocketAddr->sin_addr,
                                    ipv4SocketAddr->sin_port,
                                    addressBuffer,
                                    &addressBufferLen );

        } else if (nfsClientSocketAddr->ss_family == AF_INET6) {

            PSOCKADDR_IN6 ipv6SocketAddr = (PSOCKADDR_IN6) nfsClientSocketAddr;

            //
            //  Format IPv6 address and port
            //

            addressConvStatus = RtlIpv6AddressToStringEx( 
                                    &ipv6SocketAddr->sin6_addr,
                                    0,
                                    ipv6SocketAddr->sin6_port,
                                    addressBuffer,
                                    &addressBufferLen );
        }

        //
        //  Print the address (and port)
        //

        if ((STATUS_INVALID_PARAMETER != addressConvStatus) &&
            (0 < addressBufferLen)) {

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s NFS SOCKET ADDR: %S;",
                                               printPointer,
                                               addressBuffer );

        } else {

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s NFS INVALID SOCKET ADDR;",
                                               printPointer );
        }
    }

    //
    //  SRV ECP
    //

    if (FlagOn( recordData->KnownEcpMask, ECP_TYPE_FLAG_SRV )) {

        PSRV_OPEN_ECP_CONTEXT srvEcpContext = NULL;
        PUNICODE_STRING srvShareNameString;
        PSOCKADDR_STORAGE_NFS srvClientSocketAddr;

        knownCount++;

        //
        //  We now know this context pointer points to a
        //  SRV_OPEN_ECP_CONTEXT structure
        //

        srvEcpContext= (PSRV_OPEN_ECP_CONTEXT) ContextPointers[EcpSrvOpen];

        FLT_ASSERT(NULL != srvEcpContext);

        srvShareNameString = srvEcpContext->ShareName;
        srvClientSocketAddr = srvEcpContext->SocketAddress;

        //
        //  Print the share name, if the string is present
        //

        if (srvShareNameString) {

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s SRV SHARE NAME: %wZ,",
                                               printPointer,
                                               srvShareNameString );
        }

        FLT_ASSERT(srvClientSocketAddr != NULL);

        addressConvStatus = STATUS_INVALID_PARAMETER;
        addressBufferLen = ADDRESS_STRING_BUFFER_SIZE;

        //
        //  Print the address, whether it's IPv4 or IPv6
        //

        if (srvClientSocketAddr->ss_family == AF_INET) {

            PSOCKADDR_IN ipv4SocketAddr = (PSOCKADDR_IN) srvClientSocketAddr;

            //
            //  Format IPv4 address and port
            //

            addressConvStatus = RtlIpv4AddressToStringEx( 
                                    &ipv4SocketAddr->sin_addr,
                                    ipv4SocketAddr->sin_port,
                                    addressBuffer,
                                    &addressBufferLen );

        } else if (srvClientSocketAddr->ss_family == AF_INET6) {

            PSOCKADDR_IN6 ipv6SocketAddr = (PSOCKADDR_IN6) srvClientSocketAddr;

            //
            //  Format IPv6 address and port
            //

            addressConvStatus = RtlIpv6AddressToStringEx( 
                                    &ipv6SocketAddr->sin6_addr,
                                    0,
                                    ipv6SocketAddr->sin6_port,
                                    addressBuffer,
                                    &addressBufferLen );
        }

        if ((STATUS_INVALID_PARAMETER != addressConvStatus) && 
            (0 < addressBufferLen)) {

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s SRV SOCKET ADDR: %S;",
                                               printPointer,
                                               addressBuffer );

        } else {

            wcharsCopied = (SHORT) _snwprintf( printPointer,
                                               MAX_NAME_WCHARS_LESS_NULL,
                                               L"%s SRV INVALID SOCKET ADDR;",
                                               printPointer );
        }

        //
        //  Print SRV flags
        //

        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%s SRV FLAGS: %s%s%s;",
                                           printPointer,
                                           (srvEcpContext->OplockBlockState) ? L"B" : L"-",
                                           (srvEcpContext->OplockAppState) ? L"A" : L"-",
                                           (srvEcpContext->OplockFinalState) ? L"F" : L"-" );
    }

#else
    UNREFERENCED_PARAMETER( ContextPointers );
#endif

    //
    //  Prefetch ECP
    //

    if (FlagOn( recordData->KnownEcpMask, ECP_TYPE_FLAG_PREFETCH )) {

        knownCount++;

        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%s PREFETCH;",
                                           printPointer );
    }

    //
    //  Print closing ECP text
    //

    if (knownCount < recordData->EcpCount) {

        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%s %d unknown ECPs]",
                                           printPointer,
                                           recordData->EcpCount - knownCount );

    } else {

        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%s]",
                                           printPointer );
    }

    //
    //  If wcharsCopied is negative, it means we maxed out our buffer
    //  and exited early. Otherwise, the length is the maximum space
    //  minus the leftover buffer space
    //

    if (wcharsCopied >= 0) {

        EcpData->Length = wcharsCopied * sizeof(WCHAR);

    } else {

        //
        //  There wasn't enough buffer space, so manually truncate in a NULL
        //

        EcpData->Length = MAX_NAME_SPACE_LESS_NULL;
        EcpData->Buffer[MAX_NAME_WCHARS_LESS_NULL] = UNICODE_NULL;
    }

    #pragma prefast(pop)
}

VOID
SpyParseEcps (
    _In_ PFLT_CALLBACK_DATA Data,
    _Inout_ PRECORD_LIST RecordList,
    _Inout_ PUNICODE_STRING EcpData
    )
    /*++

Routine Description:

    Extracts ECPs from the given callback data and logs them,
    then calls SpyBuildEcpDataString to write a MiniSpy-specific
    ECP log string.

Arguments:

    Data - The Data structure that contains the information we want to record.

    RecordList - Pointer to the record, so we can set ECP count and masking

    EcpData - Pointer to string to receive formatted ECP log

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PECP_LIST ecpList;
    PRECORD_DATA recordData = &RecordList->LogRecord.Data;
    PVOID ecpContext = NULL;
    GUID ecpGuid = {0};
    ULONG ecpContextSize = 0;
    ULONG ecpFlag;
    PVOID contextPointers[NumKnownEcps];
    UCHAR offset = 0;

    PAGED_CODE();

    RtlZeroMemory( contextPointers, sizeof(PVOID) * NumKnownEcps );

    //
    //  Try to get an ECP list pointer from filter manager
    //

    status = FltGetEcpListFromCallbackData( MiniSpyData.Filter, 
                                            Data,
                                            &ecpList );

    //
    //  Even if the operation was successful, ecpList may be NULL
    //  if there are no ECPs attached to this operation, so we must
    //  make both checks
    //

    if (NT_SUCCESS(status) && (NULL != ecpList)) {
        
        //
        //  Now ask filter manager for each ECP
        //

        while (NT_SUCCESS(
                   FltGetNextExtraCreateParameter( MiniSpyData.Filter, 
                                                   ecpList, 
                                                   ecpContext,
                                                   (LPGUID) &ecpGuid,
                                                   &ecpContext,
                                                   &ecpContextSize ))) {

            //
            //  At this point, we have all the information we should need for a given
            //  ECP, but processing of ECPs is contingent on knowledge of their
            //  specific context structure. From here, ECP processing is driver-specific.
            //

            //
            //  MiniSpy supports several system-defined ECPs. What follows is 
            //  MiniSpy-specific code to log any known ECPs and produce some 
            //  meaningful output for the user
            //

            ecpFlag = 0;

            if (IsEqualGUID( &GUID_ECP_PREFETCH_OPEN, &ecpGuid )) {

                //
                //  Prefetch ECP
                //

                ecpFlag = ECP_TYPE_FLAG_PREFETCH;
                offset = EcpPrefetchOpen;
            }

#if MINISPY_WIN7

            //
            //  There are three system-defined ECPs that are only available
            //  as of Windows 7
            //
              else if (IsEqualGUID( &GUID_ECP_OPLOCK_KEY, &ecpGuid )) {

                //
                //  Oplock key ECP
                //

                ecpFlag = ECP_TYPE_FLAG_OPLOCK_KEY;
                offset = EcpOplockKey;

            } else if (IsEqualGUID( &GUID_ECP_NFS_OPEN, &ecpGuid )) {

                //
                //  NFS open ECP
                //

                ecpFlag = ECP_TYPE_FLAG_NFS;
                offset = EcpNfsOpen;

            } else if (IsEqualGUID( &GUID_ECP_SRV_OPEN, &ecpGuid )) {

                //
                //  SRV ECP
                //

                ecpFlag = ECP_TYPE_FLAG_SRV;
                offset = EcpSrvOpen;
            }

#endif

            //
            //  We don't accept user mode ECPs because of the potential
            //  for bad buffers
            //

            if ((0 != ecpFlag) &&
                !FltIsEcpFromUserMode( MiniSpyData.Filter, ecpContext )) {

                //
                //  If ecpFlag was set, we found a MiniSpy-supported ECP.
                //  Make sure we have not already found an ECP of this type
                //  for this particular operation
                //

                FLT_ASSERT(!FlagOn( recordData->KnownEcpMask, ecpFlag ));

                //
                //  Set the flag to indicate a given type of ECP was found
                //

                recordData->KnownEcpMask |= ecpFlag;

                //
                //  Save the context pointer so we can get detailed data later
                //

                contextPointers[offset] = ecpContext;
            }

            //
            //  Increment the number of total ECPs (counting both known and unknown)
            //

            recordData->EcpCount++;
        }

        //
        //  Call the Minispy-specific function to format the ECP data string for
        //  output
        //

        if (0 < recordData->EcpCount) {

            SpyBuildEcpDataString( RecordList, EcpData, contextPointers );
        }
    }
}

VOID
SpySetRecordNameAndEcpData(
    _Inout_ PLOG_RECORD LogRecord,
    _In_ PUNICODE_STRING Name,
    _In_opt_ PUNICODE_STRING EcpData
    )
/*++

Routine Description:

    Sets the given file name in the LogRecord.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path.

Arguments:

    LogRecord - The record in which to set the name.

    Name - The name to insert

    EcpData - A string of variable-length ECP data to insert

Return Value:

    None.

--*/
{

    PWCHAR printPointer = (PWCHAR)LogRecord->Name;
    SHORT wcharsCopied;
    USHORT stringLength;

    FLT_ASSERT(NULL != Name);

    //
    //  Put as much of the two strings as possible into the final buffer,
    //  name first, followed by ECP information (if any)
    //
    
    if (NULL != EcpData) {

        #pragma prefast(suppress:__WARNING_BANNED_API_USAGE, "reviewed and safe usage")
        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%wZ %wZ",
                                           Name,
                                           EcpData );

    } else {

        #pragma prefast(suppress:__WARNING_BANNED_API_USAGE, "reviewed and safe usage")
        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%wZ",
                                           Name );
    }

    if (wcharsCopied >= 0) {

        stringLength = wcharsCopied * sizeof(WCHAR);

    } else {

        //
        //  There wasn't enough buffer space, so manually truncate in a NULL
        //  because we can't trust _snwprintf to do so in that case.
        //

        stringLength = MAX_NAME_SPACE_LESS_NULL;
        printPointer[MAX_NAME_WCHARS_LESS_NULL] = UNICODE_NULL;
    }

    //
    //  We will always round up log-record length to sizeof(PVOID) so that
    //  the next log record starts on the right PVOID boundary to prevent
    //  IA64 alignment faults.  The length of the record of course
    //  includes the additional NULL at the end.
    //

    LogRecord->Length = ROUND_TO_SIZE( (LogRecord->Length +
                                        stringLength +
                                        sizeof( UNICODE_NULL )),
                                        sizeof( PVOID ) );

    FLT_ASSERT(LogRecord->Length <= MAX_LOG_RECORD_LENGTH);
}

#else

VOID
SpySetRecordName(
    _Inout_ PLOG_RECORD LogRecord,
    _In_ PUNICODE_STRING Name
    )
/*++

Routine Description:

    Sets the given file name in the LogRecord.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path.

Arguments:

    LogRecord - The record in which to set the name.

    Name - The name to insert

Return Value:

    None.

--*/
{

    PWCHAR printPointer = (PWCHAR)LogRecord->Name;
    SHORT wcharsCopied;
    USHORT stringLength;

    FLT_ASSERT(NULL != Name);

    #pragma prefast(suppress:__WARNING_BANNED_API_USAGE, "reviewed and safe usage")
        wcharsCopied = (SHORT) _snwprintf( printPointer,
                                           MAX_NAME_WCHARS_LESS_NULL,
                                           L"%wZ",
                                           Name );

    if (wcharsCopied >= 0) {

        stringLength = wcharsCopied * sizeof(WCHAR);

    } else {

        //
        //  There wasn't enough buffer space, so manually truncate in a NULL
        //  because we can't trust _snwprintf to do so in that case.
        //

        stringLength = MAX_NAME_SPACE_LESS_NULL;
        printPointer[MAX_NAME_WCHARS_LESS_NULL] = UNICODE_NULL;
    }

    //
    //  We will always round up log-record length to sizeof(PVOID) so that
    //  the next log record starts on the right PVOID boundary to prevent
    //  IA64 alignment faults.  The length of the record of course
    //  includes the additional NULL at the end.
    //

    LogRecord->Length = ROUND_TO_SIZE( (LogRecord->Length +
                                        stringLength +
                                        sizeof( UNICODE_NULL )),
                                        sizeof( PVOID ) );

    FLT_ASSERT(LogRecord->Length <= MAX_LOG_RECORD_LENGTH);
}

#endif

VOID
SpyLogPreOperationData (
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PRECORD_LIST RecordList
    )
/*++

Routine Description:

    This is called from the pre-operation callback routine to copy the
    necessary information into the log record.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path.

Arguments:

    Data - The Data structure that contains the information we want to record.

    FltObjects - Pointer to the io objects involved in this operation.

    RecordList - Where we want to save the data

Return Value:

    None.

--*/
{
    PRECORD_DATA recordData = &RecordList->LogRecord.Data;
    PDEVICE_OBJECT devObj;
    NTSTATUS status;

    status = FltGetDeviceObject(FltObjects->Volume,&devObj);
    if (NT_SUCCESS(status)) {

        ObDereferenceObject(devObj);

    } else {

        devObj = NULL;
    }

    //
    //  Save the information we want
    //

    recordData->CallbackMajorId = Data->Iopb->MajorFunction;
    recordData->CallbackMinorId = Data->Iopb->MinorFunction;
    recordData->IrpFlags        = Data->Iopb->IrpFlags;
    recordData->Flags           = Data->Flags;

    recordData->DeviceObject    = (FILE_ID)devObj;
    recordData->FileObject      = (FILE_ID)FltObjects->FileObject;
    recordData->Transaction     = (FILE_ID)FltObjects->Transaction;
    recordData->ProcessId       = (FILE_ID)PsGetCurrentProcessId();
    recordData->ThreadId        = (FILE_ID)PsGetCurrentThreadId();

    recordData->Arg1 = Data->Iopb->Parameters.Others.Argument1;
    recordData->Arg2 = Data->Iopb->Parameters.Others.Argument2;
    recordData->Arg3 = Data->Iopb->Parameters.Others.Argument3;
    recordData->Arg4 = Data->Iopb->Parameters.Others.Argument4;
    recordData->Arg5 = Data->Iopb->Parameters.Others.Argument5;
    recordData->Arg6.QuadPart = Data->Iopb->Parameters.Others.Argument6.QuadPart;

    KeQuerySystemTime( &recordData->OriginatingTime );
}


VOID
SpyLogPostOperationData (
    _In_ PFLT_CALLBACK_DATA Data,
    _Inout_ PRECORD_LIST RecordList
    )
/*++

Routine Description:

    This is called from the post-operation callback routine to copy the
    necessary information into the log record.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path or at DPC level.

Arguments:

    Data - The Data structure that contains the information we want to record.

    RecordList - Where we want to save the data

Return Value:

    None.

--*/
{
    PRECORD_DATA recordData = &RecordList->LogRecord.Data;

    recordData->Status = Data->IoStatus.Status;
    recordData->Information = Data->IoStatus.Information;
    KeQuerySystemTime( &recordData->CompletionTime );
}


VOID
SpyLogTransactionNotify (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Inout_ PRECORD_LIST RecordList,
    _In_ ULONG TransactionNotification
    )
/*++

Routine Description:

    This routine logs the transaction notification.

Arguments:

    FltObjects - Pointer to the io objects involved in this operation.

    RecordList - Where we want to save the data

    TransactionNotification - Notification for this transaction.

Return Value:

    None.

--*/
{
    PRECORD_DATA recordData = &RecordList->LogRecord.Data;
    PDEVICE_OBJECT devObj;
    NTSTATUS status;

    status = FltGetDeviceObject(FltObjects->Volume,&devObj);
    if (NT_SUCCESS(status)) {

        ObDereferenceObject(devObj);

    } else {

        devObj = NULL;
    }


    recordData->CallbackMajorId = IRP_MJ_TRANSACTION_NOTIFY;
    recordData->CallbackMinorId = TxNotificationToMinorCode(TransactionNotification);

    recordData->DeviceObject    = (FILE_ID)devObj;
    recordData->FileObject      = (FILE_ID)FltObjects->FileObject;
    recordData->Transaction     = (FILE_ID)FltObjects->Transaction;
    recordData->ProcessId       = (FILE_ID)PsGetCurrentProcessId();
    recordData->ThreadId        = (FILE_ID)PsGetCurrentThreadId();

    KeQuerySystemTime( &recordData->OriginatingTime );
}


VOID
SpyLog (
    _In_ PRECORD_LIST RecordList
    )
/*++

Routine Description:

    This routine inserts the given log record into the list to be sent
    to the user mode application.

    NOTE:  This code must be NON-PAGED because it can be called on the
           paging path or at DPC level and uses a spin-lock

Arguments:

    RecordList - The record to append to the MiniSpyData.OutputBufferList

Return Value:

    The function returns STATUS_SUCCESS.



--*/
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&MiniSpyData.OutputBufferLock, &oldIrql);
    InsertTailList(&MiniSpyData.OutputBufferList, &RecordList->List);
    KeReleaseSpinLock(&MiniSpyData.OutputBufferLock, oldIrql);
}


NTSTATUS
SpyGetLog (
    _Out_writes_bytes_to_(OutputBufferLength,*ReturnOutputBufferLength) PUCHAR OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
    )
/*++

Routine Description:
    This function fills OutputBuffer with as many LOG_RECORDs as possible.
    The LOG_RECORDs are variable sizes and are tightly packed in the
    OutputBuffer.

    NOTE:  This code must be NON-PAGED because it uses a spin-lock.

Arguments:
    OutputBuffer - The user's buffer to fill with the log data we have
        collected

    OutputBufferLength - The size in bytes of OutputBuffer

    ReturnOutputBufferLength - The amount of data actually written into the
        OutputBuffer.

Return Value:
    STATUS_SUCCESS if some records were able to be written to the OutputBuffer.

    STATUS_NO_MORE_ENTRIES if we have no data to return.

    STATUS_BUFFER_TOO_SMALL if the OutputBuffer is too small to
        hold even one record and we have data to return.

--*/
{
    PLIST_ENTRY pList;
    ULONG bytesWritten = 0;
    PLOG_RECORD pLogRecord;
    NTSTATUS status = STATUS_NO_MORE_ENTRIES;
    PRECORD_LIST pRecordList;
    KIRQL oldIrql;
    BOOLEAN recordsAvailable = FALSE;

    KeAcquireSpinLock( &MiniSpyData.OutputBufferLock, &oldIrql );

    while (!IsListEmpty( &MiniSpyData.OutputBufferList ) && (OutputBufferLength > 0)) {

        //
        //  Mark we have records
        //

        recordsAvailable = TRUE;

        //
        //  Get the next available record
        //

        pList = RemoveHeadList( &MiniSpyData.OutputBufferList );

        pRecordList = CONTAINING_RECORD( pList, RECORD_LIST, List );

        pLogRecord = &pRecordList->LogRecord;

        //
        //  If no filename was set then make it into a NULL file name.
        //

        if (REMAINING_NAME_SPACE( pLogRecord ) == MAX_NAME_SPACE) {

            //
            //  We don't have a name, so return an empty string.
            //  We have to always start a new log record on a PVOID aligned boundary.
            //

            pLogRecord->Length += ROUND_TO_SIZE( sizeof( UNICODE_NULL ), sizeof( PVOID ) );
            pLogRecord->Name[0] = UNICODE_NULL;
        }

        //
        //  Put it back if we've run out of room.
        //

        if (OutputBufferLength < pLogRecord->Length) {

            InsertHeadList( &MiniSpyData.OutputBufferList, pList );
            break;
        }

        KeReleaseSpinLock( &MiniSpyData.OutputBufferLock, oldIrql );

        //
        //  The lock is released, return the data, adjust pointers.
        //  Protect access to raw user-mode OutputBuffer with an exception handler
        //

        try {
            RtlCopyMemory( OutputBuffer, pLogRecord, pLogRecord->Length );
        } except (SpyExceptionFilter( GetExceptionInformation(), TRUE )) {

            //
            //  Put the record back in
            //

            KeAcquireSpinLock( &MiniSpyData.OutputBufferLock, &oldIrql );
            InsertHeadList( &MiniSpyData.OutputBufferList, pList );
            KeReleaseSpinLock( &MiniSpyData.OutputBufferLock, oldIrql );

            return GetExceptionCode();

        }

        bytesWritten += pLogRecord->Length;

        OutputBufferLength -= pLogRecord->Length;

        OutputBuffer += pLogRecord->Length;

        SpyFreeRecord( pRecordList );

        //
        //  Relock the list
        //

        KeAcquireSpinLock( &MiniSpyData.OutputBufferLock, &oldIrql );
    }

    KeReleaseSpinLock( &MiniSpyData.OutputBufferLock, oldIrql );

    //
    //  Set proper status
    //

    if ((bytesWritten == 0) && recordsAvailable) {

        //
        //  There were records to be sent up but
        //  there was not enough room in the buffer.
        //

        status = STATUS_BUFFER_TOO_SMALL;

    } else if (bytesWritten > 0) {

        //
        //  We were able to write some data to the output buffer,
        //  so this was a success.
        //

        status = STATUS_SUCCESS;
    }

    *ReturnOutputBufferLength = bytesWritten;

    return status;
}


VOID
SpyEmptyOutputBufferList (
    VOID
    )
/*++

Routine Description:

    This routine frees all the remaining log records in the OutputBufferList
    that are not going to get sent up to the user mode application since
    MiniSpy is shutting down.

    NOTE:  This code must be NON-PAGED because it uses a spin-lock

Arguments:

    None.

Return Value:

    None.

--*/
{
    PLIST_ENTRY pList;
    PRECORD_LIST pRecordList;
    KIRQL oldIrql;

    KeAcquireSpinLock( &MiniSpyData.OutputBufferLock, &oldIrql );

    while (!IsListEmpty( &MiniSpyData.OutputBufferList )) {

        pList = RemoveHeadList( &MiniSpyData.OutputBufferList );
        KeReleaseSpinLock( &MiniSpyData.OutputBufferLock, oldIrql );

        pRecordList = CONTAINING_RECORD( pList, RECORD_LIST, List );

        SpyFreeRecord( pRecordList );

        KeAcquireSpinLock( &MiniSpyData.OutputBufferLock, &oldIrql );
    }

    KeReleaseSpinLock( &MiniSpyData.OutputBufferLock, oldIrql );
}

//---------------------------------------------------------------------------
//                    Logging routines
//---------------------------------------------------------------------------

VOID
SpyReadDriverParameters (
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the MiniSpy-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

    This processes the following registry keys:
    hklm\system\CurrentControlSet\Services\Minispy\MaxRecords
    hklm\system\CurrentControlSet\Services\Minispy\NameQueryMethod


Arguments:

    RegistryPath - the path key which contains the values that are
        the MiniSpy parameters

Return Value:

    None.

--*/
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    PKEY_VALUE_PARTIAL_INFORMATION pValuePartialInfo;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

    //
    //  Open the registry
    //

    InitializeObjectAttributes( &attributes,
                                RegistryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = ZwOpenKey( &driverRegKey,
                        KEY_READ,
                        &attributes );

    if (!NT_SUCCESS( status )) {

        return;
    }

    //
    // Read the MaxRecordsToAllocate entry from the registry
    //

    RtlInitUnicodeString( &valueName, MAX_RECORDS_TO_ALLOCATE );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              sizeof(buffer),
                              &resultLength );

    if (NT_SUCCESS( status )) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        FLT_ASSERT( pValuePartialInfo->Type == REG_DWORD );
        MiniSpyData.MaxRecordsToAllocate = *((PLONG)&(pValuePartialInfo->Data));
    }

    //
    // Read the NameQueryMethod entry from the registry
    //

    RtlInitUnicodeString( &valueName, NAME_QUERY_METHOD );

    status = ZwQueryValueKey( driverRegKey,
                              &valueName,
                              KeyValuePartialInformation,
                              buffer,
                              sizeof(buffer),
                              &resultLength );

    if (NT_SUCCESS( status )) {

        pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        FLT_ASSERT( pValuePartialInfo->Type == REG_DWORD );
        MiniSpyData.NameQueryMethod = *((PLONG)&(pValuePartialInfo->Data));
    }

    ZwClose(driverRegKey);
}


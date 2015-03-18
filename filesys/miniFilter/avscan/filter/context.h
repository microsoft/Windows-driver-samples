/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    context.h

Abstract:

    Header file which contains context-related data 
    structures, type definitions, constants, 
    global variables and function prototypes.

Environment:

    Kernel mode

--*/

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

//
//  The file infected state.
//

typedef enum _AV_FILE_INFECTED_STATE {

    AvFileUnknown,
    AvFileInfected,
    AvFileNotInfected,  // clean.
    AvFileModified,
    AvFileScanning

} AV_FILE_INFECTED_STATE;

#define AV_STREAMHANDLE_CONTEXT_TAG          'hSvA'
#define AV_STREAM_CONTEXT_TAG                'cSvA'
#define AV_TRANSACTION_CONTEXT_TAG           'cTvA'
#define AV_SECTION_CONTEXT_TAG               'eSvA'
#define AV_INSTANCE_CONTEXT_TAG              'cIvA'
#define AV_INSTANCES_ARRAY_TAG               'aIvA'
#define AV_CONNECTION_CTX_TAG                'cCvA'
#define AV_SCAN_CTX_TAG                      'cMvA'

//
//  Defines the transaction context structure
//
#define AV_TXCTX_ENLISTED      0x01
#define AV_TXCTX_LISTDRAINED   0x02

typedef struct _AV_TRANSACTION_CONTEXT {
    
    //
    //  Transaction object pointer
    //
    
    PKTRANSACTION Transaction;
    
    //
    //  List head for stream context list.
    //
    
    LIST_ENTRY        ScListHead;
    
    //
    //  Lock used to protect this context.
    //

    PERESOURCE Resource;
    
    //
    //  A flag that tracks:
    //    AV_TXCTX_ENLISTED: if it has been enlisted in transaction
    //    AV_TXCTX_LISTDRAINED: list is drained.
    //
    
    ULONG Flags;
    
} AV_TRANSACTION_CONTEXT, *PAV_TRANSACTION_CONTEXT;

#define AV_TRANSACTION_CONTEXT_SIZE         sizeof( AV_TRANSACTION_CONTEXT )


#define IS_FILE_MODIFIED( _sCtx )        ( (_sCtx)->State == AvFileModified )
#define IS_FILE_INFECTED( _sCtx )        ( (_sCtx)->State == AvFileInfected )
#define IS_FILE_NOT_INFECTED( _sCtx )    ( (_sCtx)->State == AvFileNotInfected )

#define IS_FILE_TX_MODIFIED( _sCtx )        ( (_sCtx)->TxState == AvFileModified )
#define IS_FILE_TX_INFECTED( _sCtx )       ( (_sCtx)->TxState == AvFileInfected )
#define IS_FILE_TX_NOT_INFECTED( _sCtx )   ( (_sCtx)->TxState == AvFileNotInfected )

#define IS_FILE_NEED_SCAN( _sCtx ) ((((_sCtx)->TxContext == NULL) && IS_FILE_MODIFIED( _sCtx )) || \
                                    (((_sCtx)->TxContext != NULL) && IS_FILE_TX_MODIFIED( _sCtx )))


#define SET_FILE_UNKNOWN( _sCtx )        InterlockedExchange(&(_sCtx)->State, AvFileUnknown)
#define SET_FILE_MODIFIED( _sCtx )       InterlockedExchange(&(_sCtx)->State, AvFileModified)
#define SET_FILE_INFECTED( _sCtx )       InterlockedExchange(&(_sCtx)->State, AvFileInfected)
#define SET_FILE_NOT_INFECTED( _sCtx )   InterlockedExchange(&(_sCtx)->State, AvFileNotInfected)
#define SET_FILE_SCANNING( _sCtx )       InterlockedExchange(&(_sCtx)->State, AvFileScanning)

#define SET_FILE_TX_UNKNOWN( _sCtx )        InterlockedExchange(&(_sCtx)->TxState, AvFileUnknown)
#define SET_FILE_TX_MODIFIED( _sCtx )       InterlockedExchange(&(_sCtx)->TxState, AvFileModified)
#define SET_FILE_TX_INFECTED( _sCtx )       InterlockedExchange(&(_sCtx)->TxState, AvFileInfected)
#define SET_FILE_TX_NOT_INFECTED( _sCtx )   InterlockedExchange(&(_sCtx)->TxState, AvFileNotInfected)
#define SET_FILE_TX_SCANNING( _sCtx )       InterlockedExchange(&(_sCtx)->TxState, AvFileScanning)

#define SET_FILE_UNKNOWN_EX( _flag, _sCtx )        {\
                            if (_flag) { \
                                SET_FILE_TX_UNKNOWN( _sCtx ); \
                            } else { \
                                SET_FILE_UNKNOWN( _sCtx ); \
                            } \
                        }
#define SET_FILE_MODIFIED_EX( _flag, _sCtx )       {\
                            if (_flag) { \
                                SET_FILE_TX_MODIFIED( _sCtx ); \
                            } else { \
                                SET_FILE_MODIFIED( _sCtx ); \
                            } \
                        }
#define SET_FILE_INFECTED_EX( _flag, _sCtx )       {\
                            if (_flag) { \
                                SET_FILE_TX_INFECTED( _sCtx ); \
                            } else { \
                                SET_FILE_INFECTED( _sCtx ); \
                            } \
                        }
#define SET_FILE_NOT_INFECTED_EX( _flag, _sCtx )   {\
                            if (_flag) { \
                                SET_FILE_TX_NOT_INFECTED( _sCtx ); \
                            } else { \
                                SET_FILE_NOT_INFECTED( _sCtx ); \
                            } \
                        }
#define SET_FILE_SCANNING_EX( _flag, _sCtx )       {\
                            if (_flag) { \
                                SET_FILE_TX_SCANNING( _sCtx ); \
                            } else { \
                                SET_FILE_SCANNING( _sCtx ); \
                            } \
                        }

//
//  Stream/Stream Handle flags
//

#define AV_FLAG_PREFETCH  0x00000001

typedef struct _AV_STREAMHANDLE_CONTEXT {

    //
    //  Handle flags
    //
    
    ULONG Flags;
    
} AV_STREAMHANDLE_CONTEXT, *PAV_STREAMHANDLE_CONTEXT;

#define AV_STREAMHANDLE_CONTEXT_SIZE  sizeof( AV_STREAMHANDLE_CONTEXT )

typedef struct _AV_STREAM_CONTEXT {

    //
    //  Stream flags
    //

    ULONG Flags;
    
    //
    //  File ID, obtained from querying the file system for
    //  FileInternalInformation or FileIdInformation.
    //
    
    AV_FILE_REFERENCE FileId;
    
    //
    //  A pointer to the transaction context, so we can jump to list in the transaction.
    //
    
    PAV_TRANSACTION_CONTEXT  TxContext;
    
    //
    //  This list entry is exactly the embedded entry to 
    //  form a doubly linked list inside transaction context.
    //
    
    LIST_ENTRY  ListInTransaction;
        
    //
    //  We need to synchronize the creation of the section object. 
    //  If this syncrhonization is not made, FltCreateSectionForDataScan 
    //  would return STATUS_FLT_CONTEXT_ALREADY_DEFINED when two threads 
    //  are about to create the section for the same file.
    //
    
    PKEVENT    ScanSynchronizationEvent;
    
    //
    //  Please see AV_FILE_INFECTED_STATE for the definition of file state
    //  Note that we have TxState to maintain the isolation of 
    //  the transacted writer's view.
    //
    
    volatile LONG  State;


    volatile LONG  TxState;

    //
    // Revision numbers for files on CSVFS
    //
    LONGLONG   VolumeRevision;
    LONGLONG   CacheRevision;
    LONGLONG   FileRevision;
    
} AV_STREAM_CONTEXT, *PAV_STREAM_CONTEXT;

#define AV_STREAM_CONTEXT_SIZE         sizeof( AV_STREAM_CONTEXT )

//
//  Defines the section context structure
//

typedef struct _AV_SECTION_CONTEXT {

    //
    //  The associated section handle.
    //
    
    HANDLE SectionHandle;
    
    //
    //  The associated section object.
    //
    
    PVOID  SectionObject;
    
    //
    //  The cancel flag (if scan in the kernel mode).
    //
    
    BOOLEAN  Aborted;
    
    
    //
    //  The size of the file associated with the section object.
    //
    
    LONGLONG    FileSize;
    
    //
    //  This flag indicates if this section data scan can be cancelable.    
    //  Right now, only at pre-cleanup is cancelable on conflicting Io.
    //
    
    BOOLEAN     CancelableOnConflictingIo;

    //
    //  In the context of a conflict notification callback, only section context is given.
    //  We need to remember associated scan context to have scan id, so that
    //  We know which scan to cancel.
    //
    PVOID ScanContext;
    
} AV_SECTION_CONTEXT, *PAV_SECTION_CONTEXT;

#define AV_SECTION_CONTEXT_SIZE         sizeof( AV_SECTION_CONTEXT )

//
//  Instance context
//

typedef struct _AV_INSTANCE_CONTEXT {

    //
    //  The associated volume object pointer
    //

    PFLT_VOLUME   Volume;
    
    //
    //  The associated filter instance pointer
    //
    
    PFLT_INSTANCE Instance;
    
    //
    //  The file system type of the volume
    //
    
    FLT_FILESYSTEM_TYPE VolumeFSType;
    
    //
    //  If the file system is NTFS, then it will support a file state cache table 
    //  that saves the state of the file.
    //
    
    RTL_GENERIC_TABLE  FileStateCacheTable;
    
    //
    //  The per-instance lock to protect the cache table above.
    //
    
    ERESOURCE   Resource;

    //
    //  When set this flag indicates that the filter is attached on the
    //  hidden NTFS volume corresponding to a CSVFS volume
    //
    BOOLEAN IsOnCsvMDS;

} AV_INSTANCE_CONTEXT, *PAV_INSTANCE_CONTEXT;

#define AV_INSTANCE_CONTEXT_SIZE         sizeof( AV_INSTANCE_CONTEXT )

NTSTATUS
AvFindOrCreateTransactionContext(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_ PAV_TRANSACTION_CONTEXT *TransactionContext
    );

NTSTATUS
AvCreateSectionContext (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Outptr_ PAV_SECTION_CONTEXT *SectionContext
    );

NTSTATUS
AvCreateStreamHandleContext (
    _In_ PFLT_FILTER Filter,
    _Outptr_ PAV_STREAMHANDLE_CONTEXT *StreamHandleContext
    );

NTSTATUS
AvCreateStreamContext (
    _In_ PFLT_FILTER Filter,
    _Outptr_ PAV_STREAM_CONTEXT *StreamContext
    );

NTSTATUS
AvEnumerateInstances(
    _Outptr_result_buffer_(*NumberInstances) PFLT_INSTANCE **InstanceArray,
    _Out_ PULONG NumberInstances
    );
    
VOID
AvFreeInstances (
    _In_reads_(InstanceCount) PFLT_INSTANCE *InstanceArray,
    _In_ ULONG InstanceCount
    );
 
#endif


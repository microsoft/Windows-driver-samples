/*++

Copyright (c) Microsoft Corporation.  All Rights Reserved

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

#define CG_FILE_CONTEXT_TAG                'cFcG'
#define CG_TRANSACTION_CONTEXT_TAG           'cTcG'

//
//  Defines the transaction context structure
//

typedef struct _CG_TRANSACTION_CONTEXT {

    //
    //  Transaction object pointer
    //
    
    PKTRANSACTION Transaction;
    
    //
    //  A flag that tracks if it has ben enlisted in transaction
    //
    
    BOOLEAN Enlisted;
    
    //
    //  A flag that indicates if the fc list is drained
    //
    
    BOOLEAN ListDrained;
    
    //
    //  List head for file context list.
    //  The list is grown only when transacted writers are part of the 
    //  transaction, i.e. this list contains all file contexts likely 
    //  to be modified in a transaction.
    //
    
    LIST_ENTRY   ScListHead;
    
    //
    //  Lock used to protect the list.
    //

    PFAST_MUTEX    Mutex;
    
} CG_TRANSACTION_CONTEXT, *PCG_TRANSACTION_CONTEXT;

#define CG_TRANSACTION_CONTEXT_SIZE         sizeof( CG_TRANSACTION_CONTEXT )

//
//  This is to deal with ReFS' 128-bit file IDs & NTFS' 64-bit FileIDs.
//

typedef union _CG_FILE_REFERENCE {

    //
    //  For 64-bit fileIDs the upper 64-bits are always zeroes.
    //

    struct {
        ULONGLONG   Value;
        ULONGLONG   UpperZeroes;
    } FileId64;

    FILE_ID_128 FileId128;

} CG_FILE_REFERENCE, *PCG_FILE_REFERENCE;

//
//  File context data structure
//

typedef struct _CG_FILE_CONTEXT {

    //
    //  File ID, obtained from querying the file system for
    //  FileInternalInformation or FileIdInformation.
    //
    
    CG_FILE_REFERENCE    FileID;
    
    //
    //  The flag that we use to record if the file is dirty 
    //  if we have seen it before.
    //
    
    BOOLEAN     Dirty;
    
    //
    //  TxDirty is to record if the file is dirty in a 
    //  transaction
    //
    
    BOOLEAN     TxDirty;
    
    //
    //  A pointer to the transaction context, so we can jump to list in the transaction.
    //
    
    PCG_TRANSACTION_CONTEXT  TxContext;
    
    //
    //  This list entry is exactly the embedded entry to 
    //  form a doubly linked list inside transaction context.
    //
    
    LIST_ENTRY  ListInTransaction;
    
} CG_FILE_CONTEXT, *PCG_FILE_CONTEXT;

#define CG_FILE_CONTEXT_SIZE         sizeof( CG_FILE_CONTEXT )




NTSTATUS
CgFindOrCreateFileContext (
    _In_ PFLT_CALLBACK_DATA Cbd,
    _Outptr_ PCG_FILE_CONTEXT *FileContext
    );

    
NTSTATUS
CgFindOrCreateTransactionContext(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_ PCG_TRANSACTION_CONTEXT *TransactionContext
    );


#endif


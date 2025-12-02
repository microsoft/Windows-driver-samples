/*++

Copyright (c) 2017  Microsoft Corporation

Module Name:

    data.h

Abstract:

    Definitions needed by storahci log/diagnostic function.

Author:

--*/

#pragma once

//
// Diagnostic data definitions for STORAHCI
//

#define MAX_EXECUTION_HISTORY_ENTRY_COUNT   200 // 200 entries take up 20KB

typedef struct _SLOT_MANAGER {

    ULONG HighPriorityAttribute;

    ULONG NCQueueSlice;
    ULONG NormalQueueSlice;
    ULONG SingleIoSlice;

    ULONG CommandsIssued;
    ULONG CommandsToComplete;

    //
    // These issued slices are used to determine the type of command
    // being programmed to adapter.
    // They are used instead of reading PxCI and PxSACT.
    //
    ULONG NCQueueSliceIssued;
    ULONG NormalQueueSliceIssued;
    ULONG SingleIoSliceIssued;

    ULONG Reserved;

} SLOT_MANAGER, *PSLOT_MANAGER;

typedef struct _EXECUTION_HISTORY {

    ULONG Function;
    ULONG IS;
    SLOT_MANAGER SlotManager;   //SLOT_MANAGER from _AHCI_CHANNEL_EXTENSION
    ULONG Px[0x10];      //Px registers value, end to AHCI_SNOTIFICATION -- SNTF
    LARGE_INTEGER TimeStamp;

} EXECUTION_HISTORY, *PEXECUTION_HISTORY;

typedef struct _ATA_IO_RECORD {

    ULONG   SuccessCount;

    ULONG   CrcErrorCount;
    ULONG   MediaErrorCount;
    ULONG   EndofMediaCount;
    ULONG   IllegalCommandCount;
    ULONG   AbortedCommandCount;
    ULONG   DeviceFaultCount;

    ULONG   OtherErrorCount;

    ULONG   NcqReadLogErrorCount;   // used to record the READ LOG EXT command error count when used for NCQ Error Recovery.

    ULONG   PortDriverResetCount;
    ULONG   TotalResetCount;

} ATA_IO_RECORD, *PATA_IO_RECORD;

typedef struct _STORAGE_DIAGNOSTIC_AHCI_EXECUTION_HISTORY {

    ULONG ExecutionHistoryCount;
    ULONG ExecutionHistoryIndex;
    EXECUTION_HISTORY History[MAX_EXECUTION_HISTORY_ENTRY_COUNT];

} STORAGE_DIAGNOSTIC_AHCI_EXECUTION_HISTORY, *PSTORAGE_DIAGNOSTIC_AHCI_EXECUTION_HISTORY;

typedef struct _STORAGE_DIAGNOSTIC_AHCI_DATA {

    ULONG Version;
    ULONG Count;
    STORAGE_DIAGNOSTIC_AHCI_EXECUTION_HISTORY ExecutionHistory;
    AHCI_PORT Registers;
    ATA_IO_RECORD IoRecord;

} STORAGE_DIAGNOSTIC_AHCI_DATA, *PSTORAGE_DIAGNOSTIC_AHCI_DATA;


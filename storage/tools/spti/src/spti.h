/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spti.h

Abstract:

    These are the structures and defines that are used in the
    SPTI.C. 

Author:

Revision History:

--*/


#define SPT_CDB_LENGTH 32
#define SPT_SENSE_LENGTH 32
#define SPTWB_DATA_LENGTH 512

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
    } SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;
    
    
typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS_EX {
    SCSI_PASS_THROUGH_EX spt;
    UCHAR             ucCdbBuf[SPT_CDB_LENGTH-1];       // cushion for spt.Cdb
    ULONG             Filler;      // realign buffers to double word boundary
    STOR_ADDR_BTL8    StorAddress;
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];     // buffer for DataIn or DataOut
} SCSI_PASS_THROUGH_WITH_BUFFERS_EX, *PSCSI_PASS_THROUGH_WITH_BUFFERS_EX;


typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER_EX {
    SCSI_PASS_THROUGH_DIRECT_EX sptd;
    UCHAR             ucCdbBuf[SPT_CDB_LENGTH-1];       // cushion for sptd.Cdb
    ULONG             Filler;      // realign buffer to double word boundary
    STOR_ADDR_BTL8    StorAddress;
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER_EX, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER_EX;

VOID
PrintError(ULONG);

VOID
PrintDataBuffer(_In_reads_(BufferLength) PUCHAR DataBuffer, _In_ ULONG BufferLength);

VOID
PrintInquiryData(PVOID);

_Success_(return != NULL)
_Post_writable_byte_size_(size)
PUCHAR
AllocateAlignedBuffer(_In_ ULONG size, _In_ ULONG AlignmentMask,
_Outptr_result_maybenull_ PUCHAR * pUnAlignedBuffer);

VOID
PrintStatusResults(BOOL, DWORD, PSCSI_PASS_THROUGH_WITH_BUFFERS, ULONG);

VOID
PrintSenseInfo(PSCSI_PASS_THROUGH_WITH_BUFFERS);

VOID
PrintStatusResultsEx(BOOL, DWORD, PSCSI_PASS_THROUGH_WITH_BUFFERS_EX, ULONG);

VOID
PrintSenseInfoEx(PSCSI_PASS_THROUGH_WITH_BUFFERS_EX);

_Success_(return)
BOOL
QueryPropertyForDevice(_In_ HANDLE, _Out_ PULONG, _Out_ PUCHAR);


//
// Command Descriptor Block constants.
//

#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10

#define SETBITON                             1
#define SETBITOFF                            0

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    lsisvdt.h

Abstract:

    Private header file for lsi_u3.sys modules.  This contains private
    structure and function declarations as well as constant values which do
    not need to be exported.

Environment:

    kernel mode only

Notes:


Revision History:

--*/


/*
*************************************************************************
*                                                                       *
*   Copyright 1994-2008 LSI Corporation. All rights reserved.           *
*                                                                       *
************************************************************************/



#ifndef _SYMSVDT_
#define _SYMSVDT_

/* SCRIPTS structure definitions */

/*  It is required that both DATA_IN and DATA_OUT support the same
 *  number of MOVE FRAGMENTS for this core.
 */

#define START_Q_DEPTH   256           /* number of start queue entries */
#define DONE_Q_DEPTH    10            /* number of done queue entries */
#define CA_Q_DEPTH      32            /* number of ca queue entries */


/* script instruction values */
#define DATA_IN_SCRIPT      0x01000000L     // chmov for data in
#define DATA_OUT_SCRIPT     0x00000000L     // chmov for data out
#define OR_IN_DT_DATA       0x04000000L     // or in to change to DT data
#define MOVE_CMD_SCRIPT     0x08000000L     // or in to change chmov to move
#define MOVE_IN_SCRIPT      0x09000000L     // move for data in (for req sns)
#define RETURN_SCRIPT       0x90080000L     // return command
#define OPCODE_SCRIPT       0xff000000L     // mask for chmov/move opcode
#define COUNT_SCRIPT        0x00ffffffL     // mask for chmov/move byte count
#define INT_CMD_MASK        0x98020000L     // mask for int, bad xfer direction
#define JUMP_REL_SCRIPT     0x80880000L     // jump rel command
#define ENABLE_64BITS       0x7a570100L     // move CCNTL1 | 0x01 to CCNTL1
#define DISABLE_64BITS      0x7c57fe00L     // move CCNTL1 & 0xFE to CCNTL1
#define MEMORY_MOVE_CMD     0xC0000000L     // memory move command
#define SCNTL4_1010_66      0x7A3C0C80L     // move SCNTL4 | 0x0C to SCNTL4


    /* scripts variable descriptor table semaphore definitions */

#define SVDT_SEM_FREE       0x00000000L        /* semaphore not set */
#define SVDT_SEM_START      0x00000001L        /* set to start i/o */
#define SVDT_SEM_UNLOCK     0x00000002L        /* unlock original queue */
#define SVDT_SEM_REQ_SNS    0x00000003L        /* set to start req sns */
#define SVDT_SEM_NVCONFIG   0x00000004L        /* set to do nvconfig ioctl */
#define SVDT_SEM_MASK_VALUE 0x000000F8L        
#define SVDT_SEM_MASK       0x00000007L        
#define SVDT_SEM_PTR_MASK   0x000000F8L        
#define SVDT_PHYS_MASK      0xFFFFFFF8L        /* mask off semaphore */


/* I/O tracking array entry definition */

typedef struct _IO_TRACK_ENTRY
{
    PSCSI_REQUEST_BLOCK Srb;        /* saved SRB address of active I/O */
    UCHAR               target;     /* target ID of this I/O */
    UCHAR               lun;        /* logical unit number of this I/O */
    UCHAR               queueTag;   /* queue tag value for this I/O */
    UCHAR               reserved1;  /* padding */
#ifdef _WIN64
    ULONG               reserved2;  /* alignment padding if 64-bit */
#endif
} IO_TRACK_ENTRY, *PIO_TRACK_ENTRY;

/* I/O start queue entry definition */

typedef struct _START_QUEUE_ENTRY
{
    ULONG    svdtPhysSem;       /* combined svdt pointer and semaphore */
    ULONG    svdtMoveCmd;       /* move memory command with svdt length */
    ULONG    linkPhys;          /* physical pointer to next entry */

} START_QUEUE_ENTRY, *PSTART_QUEUE_ENTRY;

/* I/O completion queue entry definition */

typedef struct _DONE_QUEUE_ENTRY
{
    ULONG    context;           /* Srb address (32), svdtPhys (64) */
    ULONG    statXferLen;       /* SCSI status and length of transfer */
    ULONG    linkPhys;          /* physical pointer to next entry */

} DONE_QUEUE_ENTRY, *PDONE_QUEUE_ENTRY;

/* ITQ and ITL nexus pointers & svdt length */

typedef struct _NEXUS_PTR
{
    ULONG nexusPtr;             /* physical address of sys mem svdt */
    ULONG svdtMoveCmd;          /* mem move command with svdt length */
} NEXUS_PTR, *PNEXUS_PTR;

/* I/O vector list entry definition */

typedef struct _IOV_ENTRY
{
    ULONG    scriptWord0;
    ULONG    scriptWord1;
} IOV_ENTRY;

/*  -- I/O vector list --
 *  The I/O vector list contains scripts block move instructions.
 *  The first instruction is an int 0x20 when data[in|out] to test
 *  for a wrong data transfer direction specified in SrbFlags.
 *  There is one instruction for each element of a simple scatter/
 *  gather list.  The list will be called by the main scripts, and
 *  must be termintated with a return instruction.
 */

typedef struct _IOV_LIST
{
    IOV_ENTRY iovEntry[MAX_SG_ELEMENTS+2];
} IOV_LIST, *PIOV_LIST;


/* NegotMsg structure holds the msg out buffers for each target ID.
 * Pointed to by the NegotMsgBufDesc array of table indirect entries.
 * Used during auto request sense to do renegotiation of wide/sync parameters.
 */

typedef struct _NEGOT_BUF
{
    UCHAR    Buf[12];
} NEGOT_BUF, *PNEGOT_BUF;


/* svdt entry */

typedef struct _SVARS_DESCRIPTOR
{
    ULONG   count;
    ULONG   paddr;
} SVARS_DESCRIPTOR, *PSVARS_DESCRIPTOR;

    /*  -- svars --
     *   The scripts variables structure is a global data area that 
     *   is shared with the scripts.  The fields are defined as follows:
     *   
     *   - currentContext -
     *   Context pointer from the active descriptor table.  This field is
     *   set by the scripts for each SCSI bus connection and is used by the 
     *   driver while servicing SCSI and DMA interrupts.  
     *   Initialization requirements:  none.
     *   - startQPhysPtr -
     *   Physical pointer used by the scripts to maintain start queue position.
     *   Initialization requirements:  must be set to point to the head of the
     *   start queue    during driver initialization and after resets.
     *   - lockedQPhysPtr - 
     *   This field is used by the scripts to store the current start queue 
     *   position when doing auto request sense.  The scripts automatically
     *   restores the current position when all CA conditions are cleared.
     *   Initialization requirements:  none.
     *   - doneQPhysPtr -
     *   Physical pointer used by the scripts to maintain done queue position.
     *   Initialization requirements:  must be set to point to the head of the
     *   done queue during driver initialization and after resets.
     *   - caPutQPhysPtr -
     *   Physical pointer used by the scripts to maintain ca queue position
     *   for inserting commands that returned with check conditions.
     *   Initialization requirements:  must be set to point to the head of the
     *   ca queue during driver initialization and after resets.
     *   - caStartQPhysPtr -
     *   Physical pointer used by the scripts to maintain ca queue position
     *   for starting request sense commands to clear check conditions.
     *   Initialization requirements:  must be set to point to the head of the
     *   ca queue during driver initialization and after resets.
     *   - ITQnexusTablePhysPtr -
     *   Physical pointer used by scripts to obtain base address of nexus
     *   pointer lookup table for tagged commmands.
     *   Initialization requirements:  must be set during driver initialization.
     *   - ITLnexusTablePhysPtr -
     *   Physical pionter used by scripts to obtain base address of nexus
     *   pointer lookup table for non-tagged commands.
     *   Initialization requirements:  must be set during driver initialization.
     *   - msgInBuf - 
     *   This field is used as a scratch area by the scripts to store 
     *   message bytes received from the target device.  If the driver
     *   needs to examine message bytes, the data is stored here, as well.
     *   Initialization requirements:  none.
     *   
     *   NOTE: Any changes to this table must be matched in scripts.
     */
    typedef struct _SVARS
    {
        /* this area is commonly defined between the driver and scripts */

        ULONG    currentContext;                        /* 0x00 */
        ULONG    startQPhysPtr;                         /* 0x04 */
        ULONG    lockedQPhysPtr;                        /* 0x08 */
        ULONG    doneQPhysPtr;                          /* 0x0C */
        ULONG    caPutQPhysPtr;                         /* 0x10 */
        ULONG    caStartQPhysPtr;                       /* 0x14 */
        ULONG    ITQnexusTablePhysPtr;                  /* 0x18 */
        ULONG    ITLnexusTablePhysPtr;                  /* 0x1C */
        UCHAR    msgInBuf[ 8 ];                         /* 0x20 */
        /* end of common scripts/driver definition */
        
    } SVARS, *PSVARS;


    /*  -- svdt --
     *  The scripts variable descriptor table is a data structure that is
     *  used to pass I/O specific information to the scripts.  Each svdt
     *  contains pointers to other data structures formatted for table
     *  indirect scripts addressing.  The SCSI I/O Processor requires 
     *  each entry in the descriptor table to be 8 bytes long.  The first 
     *  four bytes must contain a 24-bit byte count, followed by a 32-bit 
     *  physical address.  This structure also contains data elements that 
     *  is not acccessed by table indirect operations.  Table indirect
     *  fields are defined as the type SVARS_DESCRIPTOR.
     *
     *  In order to support simplified scripts calculations, the table size
     *  must be aligned on 16 byte boundaries.
     *
     *  NOTE: Any changes to this table must be matched in scripts.
     */

    typedef struct _SVARS_DESCRIPTOR_TABLE
    {
        /* this area is commonly defined between the driver and scripts */
        ULONG               context;             /* 0x00 context */
        ULONG               runningByteCount;    /* 0x04 total bytes xfered */
        SVARS_DESCRIPTOR    deviceDescriptor;    /* 0x08 selection parms */
        SVARS_DESCRIPTOR    cmdBufDescriptor;    /* 0x10 scsi cdb */
        SVARS_DESCRIPTOR    msgOutBufDescriptor; /* 0x18 scsi messages */
        UCHAR               Cdb[16];             /* 0x20 SCSI CDB */
        UCHAR               msgOutBuf[16];       /* 0x30 message out buffer */
        ULONG               nexusEntryPhys;      /* 0x40 ptr to nexus entry */
        ULONG               sysSvdtPhys;         /* 0x44 ptr to sys mem svdt */
        ULONG               svdtMoveCmd;         /* 0x48 svdt move & length */
        ULONG               iovPhys;             /* 0x4c io vector pointer */
#ifdef _WIN64
        PSCSI_REQUEST_BLOCK Srb;                 /* 0x50 Srb pointer */
#endif
        IOV_LIST            iovList;             /* 0x50 scatter/gather list */
                                                 /* 0x58 if WIN64 */
        /* end of common scripts/driver definition */
    } SVARS_DESCRIPTOR_TABLE, *PSVARS_DESCRIPTOR_TABLE;

#endif

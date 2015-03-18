/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    lsi_u3.h

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


#ifndef _LSI_U3_
#define _LSI_U3_


//
// 53C1010 SIOP I/O registers.
//

typedef struct _SIOP_REGISTER_BASE {
    UCHAR SCNTL0;          // 00     SCSI control 0
    UCHAR SCNTL1;          // 01     SCSI control 1
    UCHAR SCNTL2;          // 02     SCSI control 2
    UCHAR SCNTL3;          // 03     SCSI control 3
    UCHAR SCID;            // 04     SCSI chip ID
    UCHAR SXFER;           // 05     SCSI transfer
    UCHAR SDID;            // 06     SCSI destination ID
    UCHAR GPREG;           // 07     general purpose bits
    UCHAR SFBR;            // 08     SCSI first byte received
    UCHAR SOCL;            // 09     SCSI output control latch
    UCHAR SSID;            // 0a     SCSI selector id
    UCHAR SBCL;            // 0b     SCSI bus control lines
    UCHAR DSTAT;           // 0c     DMA status
    UCHAR SSTAT0;          // 0d     SCSI status 0
    UCHAR SSTAT1;          // 0e     SCSI status 1
    UCHAR SSTAT2;          // 0f     SCSI status 2
    ULONG DSA;             // 10-13  data structure address
    UCHAR ISTAT0;          // 14     interrupt status 0
    UCHAR ISTAT1;          // 15     interrupt status 1
    UCHAR MBOX0;           // 16     mail box 0
    UCHAR MBOX1;           // 17     mail box 1
    UCHAR CTEST0;          // 18     chip test 0
    UCHAR CTEST1;          // 19     chip test 1
    UCHAR CTEST2;          // 1a     chip test 2
    UCHAR CTEST3;          // 1b     chip test 3
    ULONG TEMP;            // 1c-1f  temporary stack
    UCHAR RESERVED1;       // 20
    UCHAR CTEST4;          // 21     chip test 4
    UCHAR CTEST5;          // 22     chip test 5
    UCHAR CTEST6;          // 23     chip test 6
    UCHAR DBC[3];          // 24-26  DMA byte counter
    UCHAR DCMD;            // 27     DMA command
    ULONG DNAD;            // 28-2b  DMA next address for data
    ULONG DSP;             // 2c-2f  DMA scripts pointer
    UCHAR DSPS[4];         // 30-33  DMA scripts pointer save
    UCHAR SCRATCHA[4];     // 34-37  general purpose scratch pad A
    UCHAR DMODE;           // 38     DMA mode
    UCHAR DIEN;            // 39     DMA interrupt enable
    UCHAR SBR;             // 3a     scratch byte register
    UCHAR DCNTL;           // 3b     DMA control
    ULONG ADDER;           // 3c-3f  sum output of internal adder
    UCHAR SIEN0;           // 40     SCSI interrupt enable 0
    UCHAR SIEN1;           // 41     SCSI interrupt enable 1
    UCHAR SIST0;           // 42     SCSI interrupt status 0
    UCHAR SIST1;           // 43     SCSI interrupt status 1
    UCHAR RESERVED2;       // 44
    UCHAR SWIDE;           // 45     SCSI wide residue
    UCHAR RESERVED3;       // 46
    UCHAR GPCNTL;          // 47     general purpose control
    UCHAR STIME0;          // 48     SCSI timer 0
    UCHAR STIME1;          // 49     SCSI timer 1
    UCHAR RESPID0;         // 4a     response ID low-byte
    UCHAR RESPID1;         // 4b     response ID high-byte
    UCHAR STEST0;          // 4c     SCSI test 0
    UCHAR STEST1;          // 4d     SCSI test 1
    UCHAR STEST2;          // 4e     SCSI test 2
    UCHAR STEST3;          // 4f     SCSI test 3
    UCHAR SIDL[2];         // 50-51  SCSI input data latch
    UCHAR STEST4;          // 52     SCSI test 4
    UCHAR CSO;             // 53     current inbound SCSI offset
    UCHAR SODL;            // 54-55  SCSI output data latch      
    UCHAR SODL_LOWER;
    UCHAR CCNTL0;          // 56     chip control 0
    UCHAR CCNTL1;          // 57     chip control 1
    UCHAR SBDL;            // 58-59  SCSI bus data lines
    UCHAR SBDL_LOWER;
    UCHAR CCNTL2;          // 5a     chip control 2
    UCHAR CCNTL3;          // 5b     chip control 3
    ULONG SCRATCHB;        // 5c-5f  general purpose scratch pad B
    ULONG SCRATCHC;        // 60-63  general purpose scratch pad C
    ULONG SCRATCHD;        // 64-67  general purpose scratch pad D
    ULONG SCRATCHE;        // 68-6b  general purpose scratch pad E
    ULONG SCRATCHF;        // 6c-6f  general purpose scratch pad F
    ULONG SCRATCHG;        // 70-73  general purpose scratch pad G
    ULONG SCRATCHH;        // 74-77  general purpose scratch pad H
    ULONG SCRATCHI;        // 78-7b  general purpose scratch pad I
    ULONG SCRATCHJ;        // 7c-7f  general purpose scratch pad J
    ULONG SCRATCHK;        // 80-83  general purpose scratch pad K
    ULONG SCRATCHL;        // 84-87  general purpose scratch pad L
    ULONG SCRATCHM;        // 88-8b  general purpose scratch pad M
    ULONG SCRATCHN;        // 8c-8f  general purpose scratch pad N
    ULONG SCRATCHO;        // 90-93  general purpose scratch pad O
    ULONG SCRATCHP;        // 94-97  general purpose scratch pad P
    ULONG SCRATCHQ;        // 98-9b  general purpose scratch pad Q
    ULONG SCRATCHR;        // 9c-9f  general purpose scratch pad R
    ULONG MMRS;            // a0-a3  memory move read selector
    ULONG MMWS;            // a4-a7  memory move write selector
    ULONG SFS;             // a8-ab  script fetch selector
    ULONG DRS;             // ac-af  DSA relative selector
    ULONG SBMS;            // b0-b3  static block move selector
    ULONG DBMS;            // b4-b7  dynamic block move selector
    ULONG DNAD64;          // b8-bb  DMA next address 64
    UCHAR SCNTL4;          // bc     SCSI control 4
    UCHAR RESERVED4;       // bd
    UCHAR AIPCNTL0;        // be     AIP control 0
    UCHAR AIPCNTL1;        // bf     AIP control 1
    ULONG PMJAD1;          // c0-c3  phase mismatch jump address 1
    ULONG PMJAD2;          // c4-c7  phase mismatch jump address 2
    ULONG RBC;             // c8-cb  remaining byte count
    ULONG UA;              // cc-cf  updated address
    ULONG ESA;             // d0-d3  entry storage address
    ULONG IA;              // d4-d7  instruction address
    UCHAR SBC[3];          // d8-da  SCSI byte count
    UCHAR RESERVED5;       // db
    ULONG CSBC;            // dc-df  cumulative SCSI byte count
    USHORT CRCPAD;         // e0-e1  CRC pad byte value
    UCHAR CRCCNTL0;        // e2     CRC control 0
    UCHAR CRCCNTL1;        // e3     CRC control 1
    ULONG CRCDATA;         // e4-e7  CRC data
} SIOP_REGISTER_BASE, *PSIOP_REGISTER_BASE;

#endif

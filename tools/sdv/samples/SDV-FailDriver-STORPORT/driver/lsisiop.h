/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    lsisiop.h

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



#ifndef _SYMSIOP_
#define _SYMSIOP_

//
// Define miniport constants.
//

// Set the next entry to the maximum block size supported (in Kbytes)
#define SYM_MAX_BLOCK_SIZE      256    // max block size in Kbytes

#define MAX_SYNCH_TABLE_ENTRY   8      // # of entries in synch period array
#define ASYNCHRONOUS_MODE_PARAMS 0x00  // asychronous xfer mode
#define MAX_SG_ELEMENTS         ((SYM_MAX_BLOCK_SIZE / 4) + 1)  // # SG entries
#define MAX_ABORT_TRIES         100    // max times we will try to abort script
#define ABORT_SCRIPTS_TRIES     5      // overall loop count to aboart scripts
#define ABORT_STALL_TIME        3      // stall time between script abort tries
#define MAX_1010_DT_OFFSET      0x3E   // 1010 DT offset - 62
#define MAX_1010_ST_OFFSET      0x1F   // 1010 ST offset - 31
#define MESSAGE_BUFFER_SIZE     8      // maximum message size
#define RESET_STALL_TIME        30     // length of time bus reset line high
#define POST_RESET_STALL_TIME   1000   // drive recovery time after reset
#define CLEAR_FIFO_STALL_TIME   500    // Time to clear SCSI and DMA fifos
#define MAX_CLEAR_FIFO_LOOP     10     // Number of times in clear loop
#define SYM_MAX_TARGETS         16
#define SYM_NARROW_MAX_TARGETS  8
#define MAX_STALL               50000
#define MAX_XFER_LENGTH 0x00FFFFFF     // maximum transfer length per request

// SCSI message byte for Logical Unit Reset (not in storport.h)
#define SCSIMESS_LOGICAL_UNIT_RESET 0x17

// define request sense command length
#define REQ_SNS_CMD_LENGTH 6

// define SetupNegotBuf constants
#define SETUP_WIDE_FIRST                0
#define SETUP_WIDE_TIN                  1
#define SETUP_SYNC_FIRST                2
#define SETUP_SYNC_CONT                 3
#define SETUP_SYNC_TIN                  4
#define SETUP_PPR_FIRST                 5
#define SETUP_PPR_TIN                   6

// some more defines for the Performanced enhanced version
#define CLEAR_PERIOD_VALUES     0x00FFFFFF
#define CLEAR_OFFSET_VALUES     0xFFFF0000
#define DISABLE_WIDE            0xF7FFFFFF
#define ENABLE_WIDE             0x08000000
#define PERIOD_MASK             0x0000FF00
//
// SCSI equates and flags not included in SCSI.H
// TODO:  Rework code to omit these
//

#define SCSIMESS_IDENTIFY_DISC_PRIV_MASK    0x40

#define CTEST3_WRITE_INVALIDATE 0x01
#define CTEST3_CLEAR_FIFO       0x04
#define CTEST3_FLUSH_FIFO       0x08
#define CTEST4_SHADOW_REG_MODE  0x10
#define CTEST5_USE_LARGE_FIFO   0x20
#define CTEST5_BURST            0x04
#define SGEST0_RD               0x20

#define DCMD_WAIT_DISCONNECT    0x48
#define SSTAT1_ORF              0x40
#define SSTAT1_OLF              0x20
#define SSTAT2_ORF              0x40
#define SSTAT2_OLF              0x20
#define SBCL_MSG                0x04
#define SBCL_CD                 0x02
#define SBCL_IO                 0x01
#define DFIFO_LOW_SEVEN         0x7F

//
// LSI_U3 script interrupt definitions.  These values are returned in the
// DSPS register when a script routine completes.
//

#define SCRIPT_INT_COMMAND_COMPLETE      0x00   // SCSI command complete
#define SCRIPT_INT_SAVE_DATA_PTRS        0x01   // save data ptrs
#define SCRIPT_INT_SAVE_WITH_DISCONNECT  0x02   // combination SDP & disconnect
#define SCRIPT_INT_DISCONNECT            0x03   // disconnect from SCSI bus
#define SCRIPT_INT_RESTORE_POINTERS      0x04   // restore data pointers
#define SCRIPT_INT_SCRIPT_ABORTED        0x05   // SCSI script aborted
#define SCRIPT_INT_TAG_RECEIVED          0x06   // Queue tag message recieved
#define SCRIPT_INT_DEV_RESET_OCCURRED    0x07   // indicates device was reset
                                                // due to wierd phase
#define SCRIPT_INT_DEV_RESET_FAILED      0x08   // indicates above effort
                                                // failed
#define SCRIPT_INT_NVCONFIG_IOCTL        0x09   // NVRAM Config operation
#define SCRIPT_INT_IDE_MSG_SENT          0x0A   // initiator detected error
#define SCRIPT_INT_SYNC_NOT_SUPP         0x0B   // synchronous not supported
#define SCRIPT_INT_SYNC_NEGOT_COMP       0x0C   // synchronous neg complete
#define SCRIPT_INT_IGNORE_WIDE_RESIDUE   0x14   // ignore wide residue received
#define SCRIPT_INT_PPR_NEGOT_COMP        0x15   // PPR neg complete
#define SCRIPT_INT_WIDE_NOT_SUPP         0x1B   // wide not supported
#define SCRIPT_INT_WIDE_NEGOT_COMP       0x1C   // wide neg complete
#define SCRIPT_INT_INVALID_RESELECT      0x0D   // reselecting device returned
                                                // invalid SCSI id.
#define SCRIPT_INT_REJECT_MSG_RECEIVED   0x0E   // message reject msg received
#define SCRIPT_INT_INVALID_TAG_MESSAGE   0x0F   // target did not send tag
#define SCRIPT_INT_ABORT_OCCURRED        0x10
#define SCRIPT_INT_ABORT_FAILED          0x11
#define SCRIPT_INT_CHECK_CONDITION       0x12   // target returned check cond
#define SCRIPT_INT_CA_QUEUE_FULL         0x13   // queue for req sns full
#define SCRIPT_INT_BAD_XFER_DIRECTION    0x20   // data xfer is other direction


//
// define LSI_U3 SCSI Script instruction size
//

#define SCRIPT_INS_SIZE         8               // size of a script instruction

//
// ISR disposition codes. these codes are returned by ISR subroutines to
// indicate what should be done next.
//

#define ISR_START_SCRIPT        0x00    // indicates bus is free for new req.
#define ISR_RESTART_SCRIPT      0x01    // indicates script restart necessary
#define ISR_RELOAD_SCRIPT       0x02    // indicates script restart necessary
                                        //  with reload of dxp parameters
#define ISR_EXIT                0x03    // indicates no action needed

//
// Device Extension driver flags.
//

#define DFLAGS_BUS_RESET         0x0001   // indicates bus was reset internally
#define DFLAGS_IRQ_NOT_CONNECTED 0x0002   // Indicates chip is 'disabled' but
                                          // resources were still assigned.
                                          // (Omniplex problem)
#define DFLAGS_LVDS_MODE         0x0004   // indicates using LVDS signaling
                                          // allowing 40MB/sec transfers
#define DFLAGS_LVDS_DROPBACK     0x0008   // indicates SCSI bus was in LVDS, but
                                          // changed mode due to a SE device
                                          // being added to the bus
#define DFLAGS_SWAP_NVM_LINES    0x0010   // GPIO0 - clock, GPIO1 - data
#define DFLAGS_NO_FLASH_PROG     0x0020   // do not program flash memory
#define DFLAGS_FORCE_NARROW      0x0040   // do not negotiate wide, narrow only
#define DFLAGS_HALF_SPEED        0x0080   // run @ half speed (if Ultra capable
                                          // use 10 Mb/s, if LVDS use 20 Mb/s)
#define DFLAGS_NO_NVM_ACCESS     0x0100   // SSID indicates NVM should not be
                                          // accessed
#define DFLAGS_NVM_FOUND         0x0200   // NVM is present on this device
#define DFLAGS_FORCE_SYNC        0x0400   // ignore DISABLE_SYNC flag when set
#define DFLAGS_64BIT_ADDRESS     0x0800   // use 64-bit DMA addresses, scripts

//
// Logical Unit flags (track negotiation status)
//

// this first section is to flag pending negotiations
#define LF_TIN_SYNC_PENDING        0x0001 // sync TIN pending
#define LF_TIN_WIDE_PENDING        0x0002 // wide TIN pending
#define LF_TIN_PPR_PENDING         0x0003 // PPR TIN pending (both sync/wide)
#define LF_SYNC_NEG_PENDING        0x0004 // sync/async negotiation pending
#define LF_WIDE_NEG_PENDING        0x0008 // wide/narrow negotiation pending
#define LF_PPR_NEG_PENDING         0x000C // PPR negot pending (both sync/wide)

#define LF_ASYNC_NEG_DONE          0x0010 // async negotiation done
#define LF_SYNC_NEG_DONE           0x0020 // sync negotiation done
#define LF_SYNC_NEG_FAILED         0x0040 // sync not supported

#define LF_NARROW_NEG_DONE         0x0080 // narrow negotiation done
#define LF_WIDE_NEG_DONE           0x0100 // wide negotiation done
#define LF_WIDE_NEG_FAILED         0x0200 // wide not supported

#define LF_NEG_NEEDED              0x0400 // need to check for negotiations
#define LF_SYNC_NEG_REJECT         0x0800 // sync rejected, never do again
#define LF_PPR_NEG_REJECT          0x1000 // PPR rejected, use WDTR/SDTR
#define LF_DT_XFERS_ON             0x2000 // use dual transition transfers

// Next 2 flags are used only in SrbExtFlags
#define LF_OSRS_WIDE_DONE          0x0010 // set when OS_RS wide negot is done
#define LF_DISABLE_SYNC            0x0020 // set to disable sync on this I/O

//
// HBA Capability Flags
//

#define HBA_CAPABILITY_WIDE               0x0001
#define HBA_CAPABILITY_DIFFERENTIAL       0x0002
#define HBA_CAPABILITY_SCRIPT_RAM         0x0004
#define HBA_CAPABILITY_64_BITS            0x0010
#define HBA_CAPABILITY_8K_SCR_RAM         0x0020
#define HBA_CAPABILITY_NO_DOM_VAL         0x0040
#define HBA_CAPABILITY_1010_66            0x0080

//
// SCSI Protocol Chip Definitions. 
//

//
// Define the SCSI Control Register 0 bit equates 
//

#define SCNTL0_ARB_MODE_1       0x80
#define SCNTL0_ARB_MODE_0       0x40
#define SCNTL0_ENA_PARITY_CHK   0x08

#define SCNTL0_ASSERT_ATN_PAR   0x02
#define SCNTL0_TAR              0x01

//
// Define the SCSI Control Register 1 bit equates 
//

#define SCNTL1_EXT_CLK_CYC      0x80
#define SCNTL1_SODLTOSCSI       0x40

#define SCNTL1_CONNECTED        0x10
#define SCNTL1_RESET_SCSI_BUS   0x08

//
// Define the SCSI Control Register 2 bit equates
//

#define SCNTL2_WSS                              0x08
#define SCNTL2_WSR                              0x01

//
// Define the SCSI Interrupt Enable register bit equates
//

#define SIEN0_PHASE_MISMATCH     0x80
#define SIEN0_FUNCTION_COMP      0x40

#define SIEN0_RESELECT           0x10
#define SIEN0_SCSI_GROSS_ERROR   0x08
#define SIEN0_UNEXPECTED_DISCON  0x04
#define SIEN0_RST_RECEIVED       0x02
#define SIEN0_PARITY_ERROR       0x01

#define SIEN1_SEL_RESEL_TIMEOUT  0x04
#define SIEN1_BUS_MODE_CHANGE    0x10
//
// Define the DMA Status Register bit equates
//

#define DSTAT_ILLEGAL_INSTRUCTION   0x01
#define DSTAT_ABORTED               0x10
#define DSTAT_SCRPTINT              0x04

//
// Define the SCSI Status Register 0 bit equates
//

#define SSTAT0_PHASE_MISMATCH           0x80
#define SSTAT0_RESELECTED               0x10
#define SSTAT0_GROSS_ERROR              0x08
#define SSTAT0_UNEXPECTED_DISCONNECT    0x04
#define SSTAT0_RESET                    0x02
#define SSTAT0_PARITY_ERROR             0x01

//
// Define the Interrupt Status Register bit equates
//

#define ISTAT_ABORT             0x80
#define ISTAT_RESET             0x40
#define ISTAT_SIGP              0x20
#define ISTAT_SEM               0x10  
#define ISTAT_CON               0x08
#define ISTAT_INTF              0x04
#define ISTAT_SCSI_INT          0x02
#define ISTAT_DMA_INT           0x01

//
// Define the DMA Mode Register bit equates
//

#define DMODE_BURST_1           0x80
#define DMODE_BURST_0           0x40
#define DMODE_ENA_READ_MULT     0x04
#define DMODE_ENA_READ_LINE     0x08

//
// Define the DMA Interrupt Enable Register bit equates
//
#define DIEN_BUS_FAULT          0x20
#define DIEN_ENA_ABRT_INT       0x10
#define DIEN_ENA_SNGL_STP_INT   0x08
#define DIEN_ENABLE_INT_RCVD    0x04
#define DIEN_ENABLE_ILL_INST    0x01

//
// Define the DMA Control Register bit equates
//

#define DCNTL_PREFETCH_FLUSH    0x40
#define DCNTL_PREFETCH_ENABLE   0x20

//
// Define the SIST1 equates.  SCSI Interrupt Status 1.
//

#define SIST1_SEL_RESEL_TIMEOUT 0x04
#define SIST1_DIFF_SENSE_CHANGE 0x10

//
// Define STEST1 equates
//

#define STEST1_DOUBLER_SELECT           0x04
#define STEST1_DOUBLER_ENABLE           0x08

//
// Define STEST2 equates
//

#define STEST2_DIFF_MODE                0x20

//
// Define STEST3 equates
//

#define STEST3_CLEAR_FIFO               0x02
#define STEST3_HALT_CLOCK               0x20

//
// Define STEST4 equates
//

#define STEST4_FREQ_LOCK                0x20
#define STEST4_OLD_DIFF                 0x40
#define STEST4_SNGL_ENDED               0x80
#define STEST4_LOWV_DIFF                0xC0
#define STEST4_BUS_TYPE_MASK            0xC0

//
// Define GPCNTL equates
//

#define GPCNTL_LED_CNTL                 0x20

//
// Define CCNTL equates
//

#define CCNTL0_DISABLE_PIPE_REQ         0x01
#define CCNTL0_ENA_PM_JUMP              0x80
#define CCNTL3_ENA_REQACK_SKEW          0x10

//
// LSI SIOP I/O macros.
//

#define READ_SIOP_UCHAR(RegisterOffset)                      \
    (StorPortReadRegisterUchar( DeviceExtension,             \
    &(DeviceExtension->SIOPRegisterBase)->RegisterOffset))   \


#define WRITE_SIOP_UCHAR(RegisterOffset, BitMask)            \
{                                                            \
    StorPortWriteRegisterUchar( DeviceExtension,             \
    &(DeviceExtension->SIOPRegisterBase)->RegisterOffset,    \
                                BitMask);                    \
}

#define READ_SIOP_ULONG(RegisterOffset)                      \
    (StorPortReadRegisterUlong( DeviceExtension,             \
    &(DeviceExtension->SIOPRegisterBase)->RegisterOffset))   \


#define WRITE_SIOP_ULONG(RegisterOffset, BitMask)            \
{                                                            \
    StorPortWriteRegisterUlong( DeviceExtension,             \
    &(DeviceExtension->SIOPRegisterBase)->RegisterOffset,    \
                                BitMask);                    \
}

#endif

/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/

/****************************************************************************
Module Name:
    e100_557.h  (82557.h)

This driver runs on the following hardware:
    - 82558 based PCI 10/100Mb ethernet adapters
    (aka Intel EtherExpress(TM) PRO Adapters)

Environment:
    Kernel Mode - Or whatever is the equivalent on WinNT

*****************************************************************************/

#ifndef _E100_557_H
#define _E100_557_H

//-------------------------------------------------------------------------
// D100 Stepping Defines
//-------------------------------------------------------------------------
#define D100_A_STEP                 0   // NEVER SHIPPED
#define D100_B_STEP                 1   // d100 first shipped silicon
#define D100_C_STEP                 2   // d100' (c-step) with vendor/id and hw fix
#define D101_A_STEP                 4   // first silicon of d101

//-------------------------------------------------------------------------
// E100 Stepping Defines - used in PoMgmt Decisions
//-------------------------------------------------------------------------
#define E100_82557_A_STEP   1
#define E100_82557_B_STEP   2
#define E100_82557_C_STEP   3
#define E100_82558_A_STEP   4
#define E100_82558_B_STEP   5
#define E100_82559_A_STEP   6
#define E100_82559_B_STEP   7
#define E100_82559_C_STEP   8
#define E100_82559ER_A_STEP 9

//-------------------------------------------------------------------------
// D100 PORT functions -- lower 4 bits
//-------------------------------------------------------------------------
#define PORT_SOFTWARE_RESET         0
#define PORT_SELFTEST               1
#define PORT_SELECTIVE_RESET        2
#define PORT_DUMP                   3


//-------------------------------------------------------------------------
// CSR field definitions -- Offsets from CSR base
//-------------------------------------------------------------------------
#define SCB_STATUS_LOW_BYTE         0x0
#define SCB_STATUS_HIGH_BYTE        0x1
#define SCB_COMMAND_LOW_BYTE        0x2
#define SCB_COMMAND_HIGH_BYTE       0x3
#define SCB_GENERAL_POINTER         0x4
#define CSR_PORT_LOW_WORD           0x8
#define CSR_PORT_HIGH_WORD          0x0a
#define CSR_FLASH_CONTROL_REG       0x0c
#define CSR_EEPROM_CONTROL_REG      0x0e
#define CSR_MDI_CONTROL_LOW_WORD    0x10
#define CSR_MDI_CONTROL_HIGH_WORD   0x12


//-------------------------------------------------------------------------
// SCB Status Word bit definitions
//-------------------------------------------------------------------------
//- Interrupt status fields
#define SCB_STATUS_MASK         BIT_12_15       // ACK Mask
#define SCB_STATUS_CX           BIT_15          // CU Completed Action Cmd
#define SCB_STATUS_FR           BIT_14          // RU Received A Frame
#define SCB_STATUS_CNA          BIT_13          // CU Became Inactive (IDLE)
#define SCB_STATUS_RNR          BIT_12          // RU Became Not Ready
#define SCB_STATUS_MDI          BIT_11          // MDI read or write done
#define SCB_STATUS_SWI          BIT_10          // Software generated interrupt

//- Interrupt ACK fields
#define SCB_ACK_MASK            (BIT_9 | BIT_12_15 | BIT_8)   // ACK Mask
#define SCB_ALL_INTERRUPT_BITS  BIT_8_15                      // if all the bits are set, no interrupt to be served
#define SCB_ACK_CX              BIT_15          // CU Completed Action Cmd
#define SCB_ACK_FR              BIT_14          // RU Received A Frame
#define SCB_ACK_CNA             BIT_13          // CU Became Inactive (IDLE)
#define SCB_ACK_RNR             BIT_12          // RU Became Not Ready
#define SCB_ACK_MDI             BIT_11          // MDI read or write done
#define SCB_ACK_SWI             BIT_10          // Software generated interrupt
#define SCB_ACK_ER              BIT_9           // Early Receive interrupt
#define SCB_ACK_FCP             BIT_8           // Flow Control Pause interrupt

//- CUS Fields
#define SCB_CUS_MASK            (BIT_6 | BIT_7) // CUS 2-bit Mask
#define SCB_CUS_IDLE            0               // CU Idle
#define SCB_CUS_SUSPEND         BIT_6           // CU Suspended
#define SCB_CUS_ACTIVE          BIT_7           // CU Active

//- RUS Fields
#define SCB_RUS_IDLE            0               // RU Idle
#define SCB_RUS_MASK            BIT_2_5         // RUS 3-bit Mask
#define SCB_RUS_SUSPEND         BIT_2           // RU Suspended
#define SCB_RUS_NO_RESOURCES    BIT_3           // RU Out Of Resources
#define SCB_RUS_READY           BIT_4           // RU Ready
#define SCB_RUS_SUSP_NO_RBDS    (BIT_2 | BIT_5) // RU No More RBDs
#define SCB_RUS_NO_RBDS         (BIT_3 | BIT_5) // RU No More RBDs
#define SCB_RUS_READY_NO_RBDS   (BIT_4 | BIT_5) // RU Ready, No RBDs


//-------------------------------------------------------------------------
// SCB Command Word bit definitions
//-------------------------------------------------------------------------
//- CUC fields
#define SCB_CUC_MASK            BIT_4_6         // CUC 3-bit Mask
#define SCB_CUC_START           BIT_4           // CU Start
#define SCB_CUC_RESUME          BIT_5           // CU Resume
#define SCB_CUC_DUMP_ADDR       BIT_6           // CU Dump Counters Address
#define SCB_CUC_DUMP_STAT       (BIT_4 | BIT_6) // CU Dump statistics counters
#define SCB_CUC_LOAD_BASE       (BIT_5 | BIT_6) // Load the CU base
#define SCB_CUC_DUMP_RST_STAT   BIT_4_6         // CU Dump and reset statistics counters
#define SCB_CUC_STATIC_RESUME   (BIT_5 | BIT_7) // CU Static Resume

//- RUC fields
#define SCB_RUC_MASK            BIT_0_2         // RUC 3-bit Mask
#define SCB_RUC_START           BIT_0           // RU Start
#define SCB_RUC_RESUME          BIT_1           // RU Resume
#define SCB_RUC_ABORT           BIT_2           // RU Abort
#define SCB_RUC_LOAD_HDS        (BIT_0 | BIT_2) // Load RFD Header Data Size
#define SCB_RUC_LOAD_BASE       (BIT_1 | BIT_2) // Load the RU base
#define SCB_RUC_RBD_RESUME      BIT_0_2         // RBD resume

// Interrupt fields (assuming byte addressing)
#define SCB_INT_MASK            BIT_0           // Mask interrupts
#define SCB_SOFT_INT            BIT_1           // Generate a software interrupt


//-------------------------------------------------------------------------
// EEPROM bit definitions
//-------------------------------------------------------------------------
//- EEPROM control register bits
#define EN_TRNF                     0x10    // Enable turnoff
#define EEDO                        0x08    // EEPROM data out
#define EEDI                        0x04    // EEPROM data in (set for writing data)
#define EECS                        0x02    // EEPROM chip select (1=high, 0=low)
#define EESK                        0x01    // EEPROM shift clock (1=high, 0=low)

//- EEPROM opcodes
#define EEPROM_READ_OPCODE          06
#define EEPROM_WRITE_OPCODE         05
#define EEPROM_ERASE_OPCODE         07
#define EEPROM_EWEN_OPCODE          19      // Erase/write enable
#define EEPROM_EWDS_OPCODE          16      // Erase/write disable

//- EEPROM data locations
#define EEPROM_NODE_ADDRESS_BYTE_0  0
#define EEPROM_FLAGS_WORD_3         3
#define EEPROM_FLAG_10MC            BIT_0
#define EEPROM_FLAG_100MC           BIT_1

//-------------------------------------------------------------------------
// MDI Control register bit definitions
//-------------------------------------------------------------------------
#define MDI_DATA_MASK           BIT_0_15        // MDI Data port
#define MDI_REG_ADDR            BIT_16_20       // which MDI register to read/write
#define MDI_PHY_ADDR            BIT_21_25       // which PHY to read/write
#define MDI_PHY_OPCODE          BIT_26_27       // which PHY to read/write
#define MDI_PHY_READY           BIT_28          // PHY is ready for another MDI cycle
#define MDI_PHY_INT_ENABLE      BIT_29          // Assert INT at MDI cycle completion


//-------------------------------------------------------------------------
// MDI Control register opcode definitions
//-------------------------------------------------------------------------
#define MDI_WRITE               1               // Phy Write
#define MDI_READ                2               // Phy read


//-------------------------------------------------------------------------
// D100 Action Commands
//-------------------------------------------------------------------------
#define CB_NOP                  0
#define CB_IA_ADDRESS           1
#define CB_CONFIGURE            2
#define CB_MULTICAST            3
#define CB_TRANSMIT             4
#define CB_LOAD_MICROCODE       5
#define CB_DUMP                 6
#define CB_DIAGNOSE             7


//-------------------------------------------------------------------------
// Command Block (CB) Field Definitions
//-------------------------------------------------------------------------
//- CB Command Word
#define CB_EL_BIT               BIT_15          // CB EL Bit
#define CB_S_BIT                BIT_14          // CB Suspend Bit
#define CB_I_BIT                BIT_13          // CB Interrupt Bit
#define CB_TX_SF_BIT            BIT_3           // TX CB Flexible Mode
#define CB_CMD_MASK             BIT_0_2         // CB 3-bit CMD Mask

//- CB Status Word
#define CB_STATUS_MASK          BIT_12_15       // CB Status Mask (4-bits)
#define CB_STATUS_COMPLETE      BIT_15          // CB Complete Bit
#define CB_STATUS_OK            BIT_13          // CB OK Bit
#define CB_STATUS_UNDERRUN      BIT_12          // CB A Bit
#define CB_STATUS_FAIL          BIT_11          // CB Fail (F) Bit

//misc command bits
#define CB_TX_EOF_BIT           BIT_15          // TX CB/TBD EOF Bit

//-------------------------------------------------------------------------
// Config CB Parameter Fields
//-------------------------------------------------------------------------
#define CB_CFIG_BYTE_COUNT          22          // 22 config bytes
#define CB_SHORT_CFIG_BYTE_COUNT    8           // 8 config bytes

// byte 0 bit definitions
#define CB_CFIG_BYTE_COUNT_MASK     BIT_0_5     // Byte count occupies bit 5-0

// byte 1 bit definitions
#define CB_CFIG_RXFIFO_LIMIT_MASK   BIT_0_4     // RxFifo limit mask
#define CB_CFIG_TXFIFO_LIMIT_MASK   BIT_4_7     // TxFifo limit mask

// byte 3 bit definitions --
#define CB_CFIG_B3_MWI_ENABLE       BIT_0       // Memory Write Invalidate Enable Bit

// byte 4 bit definitions
#define CB_CFIG_RX_MIN_DMA_MASK     BIT_0_6     // Rx minimum DMA count mask

// byte 5 bit definitions
#define CB_CFIG_TX_MIN_DMA_MASK     BIT_0_6     // Tx minimum DMA count mask
#define CB_CFIG_DMBC_EN             BIT_7       // Enable Tx/Rx minimum DMA counts

// byte 6 bit definitions
#define CB_CFIG_LATE_SCB            BIT_0       // Update SCB After New Tx Start
#define CB_CFIG_TNO_INT             BIT_2       // Tx Not OK Interrupt
#define CB_CFIG_CI_INT              BIT_3       // Command Complete Interrupt
#define CB_CFIG_SAVE_BAD_FRAMES     BIT_7       // Save Bad Frames Enabled

// byte 7 bit definitions
#define CB_CFIG_DISC_SHORT_FRAMES   BIT_0       // Discard Short Frames
#define CB_CFIG_URUN_RETRY          BIT_1_2     // Underrun Retry Count

// byte 8 bit definitions
#define CB_CFIG_503_MII             BIT_0       // 503 vs. MII mode

// byte 9 bit definitions -- pre-defined all zeros

// byte 10 bit definitions
#define CB_CFIG_NO_SRCADR           BIT_3       // No Source Address Insertion
#define CB_CFIG_PREAMBLE_LEN        BIT_4_5     // Preamble Length
#define CB_CFIG_LOOPBACK_MODE       BIT_6_7     // Loopback Mode

// byte 11 bit definitions
#define CB_CFIG_LINEAR_PRIORITY     BIT_0_2     // Linear Priority

// byte 12 bit definitions
#define CB_CFIG_LINEAR_PRI_MODE     BIT_0       // Linear Priority mode
#define CB_CFIG_IFS_MASK            BIT_4_7     // CSMA level Interframe Spacing mask

// byte 13 bit definitions -- pre-defined all zeros

// byte 14 bit definitions -- pre-defined 0xf2

// byte 15 bit definitions
#define CB_CFIG_PROMISCUOUS         BIT_0       // Promiscuous Mode Enable
#define CB_CFIG_BROADCAST_DIS       BIT_1       // Broadcast Mode Disable
#define CB_CFIG_CRS_OR_CDT          BIT_7       // CRS Or CDT

// byte 16 bit definitions -- pre-defined all zeros

// byte 17 bit definitions -- pre-defined 0x40

// byte 18 bit definitions
#define CB_CFIG_STRIPPING           BIT_0       // Stripping Disabled
#define CB_CFIG_PADDING             BIT_1       // Padding Disabled
#define CB_CFIG_CRC_IN_MEM          BIT_2       // Transfer CRC To Memory

// byte 19 bit definitions
#define CB_CFIG_FORCE_FDX           BIT_6       // Force Full Duplex
#define CB_CFIG_FDX_ENABLE          BIT_7       // Full Duplex Enabled

// byte 20 bit definitions
#define CB_CFIG_MULTI_IA            BIT_6       // Multiple IA Addr

// byte 21 bit definitions
#define CB_CFIG_MULTICAST_ALL       BIT_3       // Multicast All


//-------------------------------------------------------------------------
// Receive Frame Descriptor Fields
//-------------------------------------------------------------------------

//- RFD Status Bits
#define RFD_RECEIVE_COLLISION   BIT_0           // Collision detected on Receive
#define RFD_IA_MATCH            BIT_1           // Indv Address Match Bit
#define RFD_RX_ERR              BIT_4           // RX_ERR pin on Phy was set
#define RFD_FRAME_TOO_SHORT     BIT_7           // Receive Frame Short
#define RFD_DMA_OVERRUN         BIT_8           // Receive DMA Overrun
#define RFD_NO_RESOURCES        BIT_9           // No Buffer Space
#define RFD_ALIGNMENT_ERROR     BIT_10          // Alignment Error
#define RFD_CRC_ERROR           BIT_11          // CRC Error
#define RFD_STATUS_OK           BIT_13          // RFD OK Bit
#define RFD_STATUS_COMPLETE     BIT_15          // RFD Complete Bit

//- RFD Command Bits
#define RFD_EL_BIT              BIT_15          // RFD EL Bit
#define RFD_S_BIT               BIT_14          // RFD Suspend Bit
#define RFD_H_BIT               BIT_4           // Header RFD Bit
#define RFD_SF_BIT              BIT_3           // RFD Flexible Mode

//- RFD misc bits
#define RFD_EOF_BIT             BIT_15          // RFD End-Of-Frame Bit
#define RFD_F_BIT               BIT_14          // RFD Buffer Fetch Bit
#define RFD_ACT_COUNT_MASK      BIT_0_13        // RFD Actual Count Mask
#define RFD_HEADER_SIZE         0x10            // Size of RFD Header (16 bytes)

//-------------------------------------------------------------------------
// Receive Buffer Descriptor Fields
//-------------------------------------------------------------------------
#define RBD_EOF_BIT             BIT_15          // RBD End-Of-Frame Bit
#define RBD_F_BIT               BIT_14          // RBD Buffer Fetch Bit
#define RBD_ACT_COUNT_MASK      BIT_0_13        // RBD Actual Count Mask

#define SIZE_FIELD_MASK         BIT_0_13        // Size of the associated buffer
#define RBD_EL_BIT              BIT_15          // RBD EL Bit


//-------------------------------------------------------------------------
// Size Of Dump Buffer
//-------------------------------------------------------------------------
#define DUMP_BUFFER_SIZE            600         // size of the dump buffer


//-------------------------------------------------------------------------
// Self Test Results
//-------------------------------------------------------------------------
#define CB_SELFTEST_FAIL_BIT        BIT_12
#define CB_SELFTEST_DIAG_BIT        BIT_5
#define CB_SELFTEST_REGISTER_BIT    BIT_3
#define CB_SELFTEST_ROM_BIT         BIT_2

#define CB_SELFTEST_ERROR_MASK ( \
                CB_SELFTEST_FAIL_BIT | CB_SELFTEST_DIAG_BIT | \
                CB_SELFTEST_REGISTER_BIT | CB_SELFTEST_ROM_BIT)


//-------------------------------------------------------------------------
// Driver Configuration Default Parameters for the 557
//  Note: If the driver uses any defaults that are different from the chip's
//        defaults, it will be noted below
//-------------------------------------------------------------------------
// Byte 0 (byte count) default
#define CB_557_CFIG_DEFAULT_PARM0   CB_CFIG_BYTE_COUNT

// Byte 1 (fifo limits) default
#define DEFAULT_TX_FIFO_LIMIT       0x08
#define DEFAULT_RX_FIFO_LIMIT       0x08
#define CB_557_CFIG_DEFAULT_PARM1   0x88

// Byte 2 (IFS) default
#define CB_557_CFIG_DEFAULT_PARM2   0x00

// Byte 3 (reserved) default
#define CB_557_CFIG_DEFAULT_PARM3   0x00

// Byte 4 (Rx DMA min count) default
#define CB_557_CFIG_DEFAULT_PARM4   0x00

// Byte 5 (Tx DMA min count, DMA min count enable) default
#define CB_557_CFIG_DEFAULT_PARM5   0x00

// Byte 6 (Late SCB, TNO int, CI int, Save bad frames) default
#define CB_557_CFIG_DEFAULT_PARM6   0x32

// Byte 7 (Discard short frames, underrun retry) default
//          note: disc short frames will be enabled
#define DEFAULT_UNDERRUN_RETRY      0x01
#define CB_557_CFIG_DEFAULT_PARM7   0x01

// Byte 8 (MII or 503) default
//          note: MII will be the default
#define CB_557_CFIG_DEFAULT_PARM8   0x01

// Byte 9 - Power management for 82558B, 82559
#define CB_WAKE_ON_LINK_BYTE9 0x20
#define CB_WAKE_ON_ARP_PKT_BYTE9 0x40

#define CB_557_CFIG_DEFAULT_PARM9   0

// Byte 10 (scr addr insertion, preamble, loopback) default
#define CB_557_CFIG_DEFAULT_PARM10  0x2e

// Byte 11 (linear priority) default
#define CB_557_CFIG_DEFAULT_PARM11  0x00

// Byte 12 (IFS,linear priority mode) default
#define CB_557_CFIG_DEFAULT_PARM12  0x60

// Byte 13 (reserved) default
#define CB_557_CFIG_DEFAULT_PARM13  0x00

// Byte 14 (reserved) default
#define CB_557_CFIG_DEFAULT_PARM14  0xf2

// Byte 15 (promiscuous, broadcast, CRS/CDT) default
#define CB_557_CFIG_DEFAULT_PARM15  0xea

// Byte 16 (reserved) default
#define CB_557_CFIG_DEFAULT_PARM16  0x00

// Byte 17 (reserved) default
#define CB_557_CFIG_DEFAULT_PARM17  0x40

// Byte 18 (Stripping, padding, Rcv CRC in mem) default
//          note: padding will be enabled
#define CB_557_CFIG_DEFAULT_PARM18  0xf2

// Byte 19 (reserved) default
//          note: full duplex is enabled if FDX# pin is 0
#define CB_557_CFIG_DEFAULT_PARM19  0x80

// Byte 20 (multi-IA) default
#define CB_557_CFIG_DEFAULT_PARM20  0x3f

// Byte 21 (multicast all) default
#define CB_557_CFIG_DEFAULT_PARM21  0x05


#pragma pack(1)

//-------------------------------------------------------------------------
// Ethernet Frame Structure
//-------------------------------------------------------------------------
//- Ethernet 6-byte Address
typedef struct _ETH_ADDRESS_STRUC {
    UCHAR       EthNodeAddress[ETHERNET_ADDRESS_LENGTH];
} ETH_ADDRESS_STRUC, *PETH_ADDRESS_STRUC;


//- Ethernet 14-byte Header
typedef struct _ETH_HEADER_STRUC {
    UCHAR       Destination[ETHERNET_ADDRESS_LENGTH];
    UCHAR       Source[ETHERNET_ADDRESS_LENGTH];
    USHORT      TypeLength;
} ETH_HEADER_STRUC, *PETH_HEADER_STRUC;


//- Ethernet Buffer (Including Ethernet Header) for Transmits
typedef struct _ETH_TX_BUFFER_STRUC {
    ETH_HEADER_STRUC    TxMacHeader;
    UCHAR               TxBufferData[(TCB_BUFFER_SIZE - sizeof(ETH_HEADER_STRUC))];
} ETH_TX_BUFFER_STRUC, *PETH_TX_BUFFER_STRUC;

typedef struct _ETH_RX_BUFFER_STRUC {
    ETH_HEADER_STRUC    RxMacHeader;
    UCHAR               RxBufferData[(RCB_BUFFER_SIZE - sizeof(ETH_HEADER_STRUC))];
} ETH_RX_BUFFER_STRUC, *PETH_RX_BUFFER_STRUC;



//-------------------------------------------------------------------------
// 82557 Data Structures
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Self test
//-------------------------------------------------------------------------
typedef struct _SELF_TEST_STRUC {
    ULONG       StSignature;            // Self Test Signature
    ULONG       StResults;              // Self Test Results
} SELF_TEST_STRUC, *PSELF_TEST_STRUC;


//-------------------------------------------------------------------------
// Control/Status Registers (CSR)
//-------------------------------------------------------------------------
typedef struct _CSR_STRUC {
    USHORT      ScbStatus;              // SCB Status register
    UCHAR       ScbCommandLow;          // SCB Command register (low byte)
    UCHAR       ScbCommandHigh;         // SCB Command register (high byte)
    ULONG       ScbGeneralPointer;      // SCB General pointer
    ULONG       Port;                   // PORT register
    USHORT      FlashControl;           // Flash Control register
    USHORT      EepromControl;          // EEPROM control register
    ULONG       MDIControl;             // MDI Control Register
    ULONG       RxDMAByteCount;         // Receive DMA Byte count register
} CSR_STRUC, *PCSR_STRUC;

//-------------------------------------------------------------------------
// Error Counters
//-------------------------------------------------------------------------
typedef struct _ERR_COUNT_STRUC {
    ULONG       XmtGoodFrames;          // Good frames transmitted
    ULONG       XmtMaxCollisions;       // Fatal frames -- had max collisions
    ULONG       XmtLateCollisions;      // Fatal frames -- had a late coll.
    ULONG       XmtUnderruns;           // Transmit underruns (fatal or re-transmit)
    ULONG       XmtLostCRS;             // Frames transmitted without CRS
    ULONG       XmtDeferred;            // Deferred transmits
    ULONG       XmtSingleCollision;     // Transmits that had 1 and only 1 coll.
    ULONG       XmtMultCollisions;      // Transmits that had multiple coll.
    ULONG       XmtTotalCollisions;     // Transmits that had 1+ collisions.
    ULONG       RcvGoodFrames;          // Good frames received
    ULONG       RcvCrcErrors;           // Aligned frames that had a CRC error
    ULONG       RcvAlignmentErrors;     // Receives that had alignment errors
    ULONG       RcvResourceErrors;      // Good frame dropped due to lack of resources
    ULONG       RcvOverrunErrors;       // Overrun errors - bus was busy
    ULONG       RcvCdtErrors;           // Received frames that encountered coll.
    ULONG       RcvShortFrames;         // Received frames that were to short
    ULONG       CommandComplete;        // A005h indicates cmd completion
} ERR_COUNT_STRUC, *PERR_COUNT_STRUC;


//-------------------------------------------------------------------------
// Command Block (CB) Generic Header Structure
//-------------------------------------------------------------------------
typedef struct _CB_HEADER_STRUC {
    USHORT      CbStatus;               // Command Block Status
    USHORT      CbCommand;              // Command Block Command
    ULONG       CbLinkPointer;          // Link To Next CB
} CB_HEADER_STRUC, *PCB_HEADER_STRUC;


//-------------------------------------------------------------------------
// NOP Command Block (NOP_CB)
//-------------------------------------------------------------------------
typedef struct _NOP_CB_STRUC {
    CB_HEADER_STRUC     NopCBHeader;
} NOP_CB_STRUC, *PNOP_CB_STRUC;


//-------------------------------------------------------------------------
// Individual Address Command Block (IA_CB)
//-------------------------------------------------------------------------
typedef struct _IA_CB_STRUC {
    CB_HEADER_STRUC     IaCBHeader;
    UCHAR               IaAddress[ETHERNET_ADDRESS_LENGTH];
} IA_CB_STRUC, *PIA_CB_STRUC;


//-------------------------------------------------------------------------
// Configure Command Block (CONFIG_CB)
//-------------------------------------------------------------------------
typedef struct _CONFIG_CB_STRUC {
    CB_HEADER_STRUC     ConfigCBHeader;
    UCHAR               ConfigBytes[CB_CFIG_BYTE_COUNT];
} CONFIG_CB_STRUC, *PCONFIG_CB_STRUC;


//-------------------------------------------------------------------------
// MultiCast Command Block (MULTICAST_CB)
//-------------------------------------------------------------------------
typedef struct _MULTICAST_CB_STRUC {
    CB_HEADER_STRUC     McCBHeader;
    USHORT              McCount;        // Number of multicast addresses
    UCHAR               McAddress[(ETHERNET_ADDRESS_LENGTH * MAX_MULTICAST_ADDRESSES)];
} MULTICAST_CB_STRUC, *PMULTICAST_CB_STRUC;

//-------------------------------------------------------------------------
// WakeUp Filter Command Block (FILTER_CB)
//-------------------------------------------------------------------------
typedef struct _FILTER_CB_STRUC {
    CB_HEADER_STRUC     FilterCBHeader;
    ULONG               Pattern[16];
}FILTER_CB_STRUC , *PFILTER_CB_STRUC ;

//-------------------------------------------------------------------------
// Dump Command Block (DUMP_CB)
//-------------------------------------------------------------------------
typedef struct _DUMP_CB_STRUC {
    CB_HEADER_STRUC     DumpCBHeader;
    ULONG               DumpAreaAddress;        // Dump Buffer Area Address
} DUMP_CB_STRUC, *PDUMP_CB_STRUC;


//-------------------------------------------------------------------------
// Dump Area structure definition
//-------------------------------------------------------------------------
typedef struct _DUMP_AREA_STRUC {
    UCHAR       DumpBuffer[DUMP_BUFFER_SIZE];
} DUMP_AREA_STRUC, *PDUMP_AREA_STRUC;


//-------------------------------------------------------------------------
// Diagnose Command Block (DIAGNOSE_CB)
//-------------------------------------------------------------------------
typedef struct _DIAGNOSE_CB_STRUC {
    CB_HEADER_STRUC     DiagCBHeader;
} DIAGNOSE_CB_STRUC, *PDIAGNOSE_CB_STRUC;

//-------------------------------------------------------------------------
// Transmit Command Block (TxCB)
//-------------------------------------------------------------------------
typedef struct _GENERIC_TxCB {
    CB_HEADER_STRUC     TxCbHeader;
    ULONG               TxCbTbdPointer;         // TBD address
    USHORT              TxCbCount;              // Data Bytes In TCB past header
    UCHAR               TxCbThreshold;          // TX Threshold for FIFO Extender
    UCHAR               TxCbTbdNumber;
    ETH_TX_BUFFER_STRUC TxCbData;
    ULONG               pad0;
    ULONG               pad1;
    ULONG               pad2;
    ULONG               pad3;
} TXCB_STRUC, *PTXCB_STRUC;

//-------------------------------------------------------------------------
// Transmit Buffer Descriptor (TBD)
//-------------------------------------------------------------------------
typedef struct _TBD_STRUC {
    ULONG       TbdBufferAddress;       // Physical Transmit Buffer Address
    unsigned    TbdCount :14;
    unsigned             :1 ;           // always 0
    unsigned    EndOfList:1 ;           // EL bit in Tbd
    unsigned             :16;           // field that is always 0's in a TBD
} TBD_STRUC, *PTBD_STRUC;


//-------------------------------------------------------------------------
// Receive Frame Descriptor (RFD)
//-------------------------------------------------------------------------
typedef struct _RFD_STRUC {
    CB_HEADER_STRUC     RfdCbHeader;
    ULONG               RfdRbdPointer;  // Receive Buffer Descriptor Addr
    USHORT              RfdActualCount; // Number Of Bytes Received
    USHORT              RfdSize;        // Number Of Bytes In RFD
    ETH_RX_BUFFER_STRUC RfdBuffer;      // Data buffer in RFD
} RFD_STRUC, *PRFD_STRUC;


//-------------------------------------------------------------------------
// Receive Buffer Descriptor (RBD)
//-------------------------------------------------------------------------
typedef struct _RBD_STRUC {
    USHORT      RbdActualCount;         // Number Of Bytes Received
    USHORT      RbdFiller;
    ULONG       RbdLinkAddress;         // Link To Next RBD
    ULONG       RbdRcbAddress;          // Receive Buffer Address
    USHORT      RbdSize;                // Receive Buffer Size
    USHORT      RbdFiller1;
} RBD_STRUC, *PRBD_STRUC;

#pragma pack()

//-------------------------------------------------------------------------
// 82557 PCI Register Definitions
// Refer To The PCI Specification For Detailed Explanations
//-------------------------------------------------------------------------
//- Register Offsets
#define PCI_VENDOR_ID_REGISTER      0x00    // PCI Vendor ID Register
#define PCI_DEVICE_ID_REGISTER      0x02    // PCI Device ID Register
#define PCI_CONFIG_ID_REGISTER      0x00    // PCI Configuration ID Register
#define PCI_COMMAND_REGISTER        0x04    // PCI Command Register
#define PCI_STATUS_REGISTER         0x06    // PCI Status Register
#define PCI_REV_ID_REGISTER         0x08    // PCI Revision ID Register
#define PCI_CLASS_CODE_REGISTER     0x09    // PCI Class Code Register
#define PCI_CACHE_LINE_REGISTER     0x0C    // PCI Cache Line Register
#define PCI_LATENCY_TIMER           0x0D    // PCI Latency Timer Register
#define PCI_HEADER_TYPE             0x0E    // PCI Header Type Register
#define PCI_BIST_REGISTER           0x0F    // PCI Built-In SelfTest Register
#define PCI_BAR_0_REGISTER          0x10    // PCI Base Address Register 0
#define PCI_BAR_1_REGISTER          0x14    // PCI Base Address Register 1
#define PCI_BAR_2_REGISTER          0x18    // PCI Base Address Register 2
#define PCI_BAR_3_REGISTER          0x1C    // PCI Base Address Register 3
#define PCI_BAR_4_REGISTER          0x20    // PCI Base Address Register 4
#define PCI_BAR_5_REGISTER          0x24    // PCI Base Address Register 5
#define PCI_SUBVENDOR_ID_REGISTER   0x2C    // PCI SubVendor ID Register
#define PCI_SUBDEVICE_ID_REGISTER   0x2E    // PCI SubDevice ID Register
#define PCI_EXPANSION_ROM           0x30    // PCI Expansion ROM Base Register
#define PCI_INTERRUPT_LINE          0x3C    // PCI Interrupt Line Register
#define PCI_INTERRUPT_PIN           0x3D    // PCI Interrupt Pin Register
#define PCI_MIN_GNT_REGISTER        0x3E    // PCI Min-Gnt Register
#define PCI_MAX_LAT_REGISTER        0x3F    // PCI Max_Lat Register
#define PCI_NODE_ADDR_REGISTER      0x40    // PCI Node Address Register


//-------------------------------------------------------------------------
// PHY 100 MDI Register/Bit Definitions
//-------------------------------------------------------------------------
// MDI register set
#define MDI_CONTROL_REG             0x00        // MDI control register
#define MDI_STATUS_REG              0x01        // MDI Status regiser
#define PHY_ID_REG_1                0x02        // Phy indentification reg (word 1)
#define PHY_ID_REG_2                0x03        // Phy indentification reg (word 2)
#define AUTO_NEG_ADVERTISE_REG      0x04        // Auto-negotiation advertisement
#define AUTO_NEG_LINK_PARTNER_REG   0x05        // Auto-negotiation link partner ability
#define AUTO_NEG_EXPANSION_REG      0x06        // Auto-negotiation expansion
#define AUTO_NEG_NEXT_PAGE_REG      0x07        // Auto-negotiation next page transmit
#define EXTENDED_REG_0              0x10        // Extended reg 0 (Phy 100 modes)
#define EXTENDED_REG_1              0x14        // Extended reg 1 (Phy 100 error indications)
#define NSC_CONG_CONTROL_REG        0x17        // National (TX) congestion control
#define NSC_SPEED_IND_REG           0x19        // National (TX) speed indication
#define PHY_EQUALIZER_REG           0x1A        // Register for the Phy Equalizer values

// MDI Control register bit definitions
#define MDI_CR_COLL_TEST_ENABLE     BIT_7       // Collision test enable
#define MDI_CR_FULL_HALF            BIT_8       // FDX =1, half duplex =0
#define MDI_CR_RESTART_AUTO_NEG     BIT_9       // Restart auto negotiation
#define MDI_CR_ISOLATE              BIT_10      // Isolate PHY from MII
#define MDI_CR_POWER_DOWN           BIT_11      // Power down
#define MDI_CR_AUTO_SELECT          BIT_12      // Auto speed select enable
#define MDI_CR_10_100               BIT_13      // 0 = 10Mbs, 1 = 100Mbs
#define MDI_CR_LOOPBACK             BIT_14      // 0 = normal, 1 = loopback
#define MDI_CR_RESET                BIT_15      // 0 = normal, 1 = PHY reset

// MDI Status register bit definitions
#define MDI_SR_EXT_REG_CAPABLE      BIT_0       // Extended register capabilities
#define MDI_SR_JABBER_DETECT        BIT_1       // Jabber detected
#define MDI_SR_LINK_STATUS          BIT_2       // Link Status -- 1 = link
#define MDI_SR_AUTO_SELECT_CAPABLE  BIT_3       // Auto speed select capable
#define MDI_SR_REMOTE_FAULT_DETECT  BIT_4       // Remote fault detect
#define MDI_SR_AUTO_NEG_COMPLETE    BIT_5       // Auto negotiation complete
#define MDI_SR_10T_HALF_DPX         BIT_11      // 10BaseT Half Duplex capable
#define MDI_SR_10T_FULL_DPX         BIT_12      // 10BaseT full duplex capable
#define MDI_SR_TX_HALF_DPX          BIT_13      // TX Half Duplex capable
#define MDI_SR_TX_FULL_DPX          BIT_14      // TX full duplex capable
#define MDI_SR_T4_CAPABLE           BIT_15      // T4 capable

// Auto-Negotiation advertisement register bit definitions
#define NWAY_AD_SELCTOR_FIELD       BIT_0_4     // identifies supported protocol
#define NWAY_AD_ABILITY             BIT_5_12    // technologies that are supported
#define NWAY_AD_10T_HALF_DPX        BIT_5       // 10BaseT Half Duplex capable
#define NWAY_AD_10T_FULL_DPX        BIT_6       // 10BaseT full duplex capable
#define NWAY_AD_TX_HALF_DPX         BIT_7       // TX Half Duplex capable
#define NWAY_AD_TX_FULL_DPX         BIT_8       // TX full duplex capable
#define NWAY_AD_T4_CAPABLE          BIT_9       // T4 capable
#define NWAY_AD_REMOTE_FAULT        BIT_13      // indicates local remote fault
#define NWAY_AD_RESERVED            BIT_14      // reserved
#define NWAY_AD_NEXT_PAGE           BIT_15      // Next page (not supported)

// Auto-Negotiation link partner ability register bit definitions
#define NWAY_LP_SELCTOR_FIELD       BIT_0_4     // identifies supported protocol
#define NWAY_LP_ABILITY             BIT_5_9     // technologies that are supported
#define NWAY_LP_REMOTE_FAULT        BIT_13      // indicates partner remote fault
#define NWAY_LP_ACKNOWLEDGE         BIT_14      // acknowledge
#define NWAY_LP_NEXT_PAGE           BIT_15      // Next page (not supported)

// Auto-Negotiation expansion register bit definitions
#define NWAY_EX_LP_NWAY             BIT_0       // link partner is NWAY
#define NWAY_EX_PAGE_RECEIVED       BIT_1       // link code word received
#define NWAY_EX_NEXT_PAGE_ABLE      BIT_2       // local is next page able
#define NWAY_EX_LP_NEXT_PAGE_ABLE   BIT_3       // partner is next page able
#define NWAY_EX_PARALLEL_DET_FLT    BIT_4       // parallel detection fault
#define NWAY_EX_RESERVED            BIT_5_15    // reserved


// PHY 100 Extended Register 0 bit definitions
#define PHY_100_ER0_FDX_INDIC       BIT_0       // 1 = FDX, 0 = half duplex
#define PHY_100_ER0_SPEED_INDIC     BIT_1       // 1 = 100mbs, 0= 10mbs
#define PHY_100_ER0_WAKE_UP         BIT_2       // Wake up DAC
#define PHY_100_ER0_RESERVED        BIT_3_4     // Reserved
#define PHY_100_ER0_REV_CNTRL       BIT_5_7     // Revsion control (A step = 000)
#define PHY_100_ER0_FORCE_FAIL      BIT_8       // Force Fail is enabled
#define PHY_100_ER0_TEST            BIT_9_13    // Revsion control (A step = 000)
#define PHY_100_ER0_LINKDIS         BIT_14      // Link integrity test is disabled
#define PHY_100_ER0_JABDIS          BIT_15      // Jabber function is disabled


// PHY 100 Extended Register 1 bit definitions
#define PHY_100_ER1_RESERVED        BIT_0_8     // Reserved
#define PHY_100_ER1_CH2_DET_ERR     BIT_9       // Channel 2 EOF detection error
#define PHY_100_ER1_MANCH_CODE_ERR  BIT_10      // Manchester code error
#define PHY_100_ER1_EOP_ERR         BIT_11      // EOP error
#define PHY_100_ER1_BAD_CODE_ERR    BIT_12      // bad code error
#define PHY_100_ER1_INV_CODE_ERR    BIT_13      // invalid code error
#define PHY_100_ER1_DC_BAL_ERR      BIT_14      // DC balance error
#define PHY_100_ER1_PAIR_SKEW_ERR   BIT_15      // Pair skew error

// PHY TX Register/Bit definitions
#define PHY_TX_STATUS_CTRL_REG      0x10
#define PHY_TX_POLARITY_MASK        BIT_8       // register 10h bit 8 (the polarity bit)
#define PHY_TX_NORMAL_POLARITY      0           // register 10h bit 8 =0 (normal polarity)

#define PHY_TX_SPECIAL_CTRL_REG     0x11
#define AUTO_POLARITY_DISABLE       BIT_4       // register 11h bit 4 (0=enable, 1=disable)

#define PHY_TX_REG_18               0x18        // Error counter register
// National Semiconductor TX phy congestion control register bit definitions
#define NSC_TX_CONG_TXREADY         BIT_10      // Makes TxReady an input
#define NSC_TX_CONG_ENABLE          BIT_8       // Enables congestion control
#define NSC_TX_CONG_F_CONNECT       BIT_5       // Enables congestion control

// National Semiconductor TX phy speed indication register bit definitions
#define NSC_TX_SPD_INDC_SPEED       BIT_6       // 0 = 100mb, 1=10mb

#endif  // _E100_557_H


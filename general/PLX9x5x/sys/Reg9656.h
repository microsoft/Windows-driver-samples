#ifndef __REG9656_H_
#define __REG9656_H_

//*****************************************************************************
//  
//  File Name: Reg9656.h
// 
//  Description:  This file defines all the PLX 9656 chip Registers.
// 
//  NOTE: These definitions are for memory-mapped register access only.
//
//*****************************************************************************

//-----------------------------------------------------------------------------   
// PCI Device/Vendor Ids.
//-----------------------------------------------------------------------------   
#define PLX_PCI_VENDOR_ID               0x10B5
#define PLX_PCI_DEVICE_ID               0x9601
                                        
//-----------------------------------------------------------------------------   
// Expected size of the PCI9656RDK-Lite on-board SRAM
//-----------------------------------------------------------------------------   
#define PCI9656_SRAM_SIZE                  (0x20000)
                                                   
//-----------------------------------------------------------------------------   
// Maximum DMA transfer size (in bytes).
//
// NOTE: This value is rather abritrary for this drive, 
//       but must be between [0 - PCI9656_SRAM_SIZE] in value.
//       You can play with the value to see how requests are sequenced as a
//       set of one or more DMA operations.
//-----------------------------------------------------------------------------   
#define PCI9656_MAXIMUM_TRANSFER_LENGTH    (8*1024)

//-----------------------------------------------------------------------------   
// The DMA_TRANSFER_ELEMENTS (the 9656's hardware scatter/gather list element)
// must be aligned on a 16-byte boundry.  This is because the lower 4 bits of
// the DESC_PTR.Address contain bit fields not included in the "next" address.
//-----------------------------------------------------------------------------   
#define PCI9656_DTE_ALIGNMENT_16      FILE_OCTA_ALIGNMENT 

//-----------------------------------------------------------------------------   
// Number of DMA channels supported by PLX Chip
//-----------------------------------------------------------------------------   
#define PCI9656_DMA_CHANNELS               (2) 

//-----------------------------------------------------------------------------   
// DMA Transfer Element (DTE)
// 
// NOTE: This structure is modeled after the DMAPADRx, DMALADRx, DMASIZx and 
//       DMAADPRx registers. See DataBook Registers description: 11-74 to 11-77.
//-----------------------------------------------------------------------------   
typedef struct _DESC_PTR_ { 

    unsigned int     DescLocation  : 1  ;  // TRUE - Desc in PCI (host) memory
    unsigned int     LastElement   : 1  ;  // TRUE - last NTE in chain
    unsigned int     TermCountInt  : 1  ;  // TRUE - Interrupt on term count.
    unsigned int     DirOfTransfer : 1  ;  // see defines below
    unsigned int     Address       : 28 ;

} DESC_PTR;

#define DESC_PTR_DESC_LOCATION__LOCAL    (0)
#define DESC_PTR_DESC_LOCATION__PCI      (1)

#define DESC_PTR_DIRECTION__TO_DEVICE    (0)
#define DESC_PTR_DIRECTION__FROM_DEVICE  (1)

typedef struct _DMA_TRANSFER_ELEMENT {

    unsigned int       PciAddressLow   ;
    unsigned int       LocalAddress    ;
    unsigned int       TransferSize    ;
    DESC_PTR           DescPtr         ;
    unsigned int       PciAddressHigh  ;
    unsigned int       pad [3]         ;

} DMA_TRANSFER_ELEMENT, * PDMA_TRANSFER_ELEMENT;

#define DESC_PTR_ADDR_SHIFT (4)
#define DESC_PTR_ADDR(a) (((ULONG) a) >> DESC_PTR_ADDR_SHIFT)


//-----------------------------------------------------------------------------   
// Define the Interrupt Command Status Register (CSR)
//-----------------------------------------------------------------------------   
typedef struct _INT_CSR_ {
    unsigned int  EnableIntSources       : 2;  // bit 0-1
    unsigned int  GenPciBusSerrInt       : 1;  // bit 2
    unsigned int  MailboxIntEnable       : 1;  // bit 3
    unsigned int  PowerMgmtIntEnable     : 1;  // bit 4
    unsigned int  PowerMgmtInt           : 1;  // bit 5
    unsigned int  DM_WriteParityCheck    : 1;  // bit 6
    unsigned int  DM_WriteParityCheckErr : 1;  // bit 7
    unsigned int  PciIntEnable           : 1;  // bit 8
    unsigned int  PciDoorbellIntEnable   : 1;  // bit 9
    unsigned int  PciAbortIntEnable      : 1;  // bit 10
    unsigned int  LocalIntInputEnable    : 1;  // bit 11
    unsigned int  RetryAbortEnable       : 1;  // bit 12
    unsigned int  PciDoorbellIntActive   : 1;  // bit 13
    unsigned int  PciAbortIntActive      : 1;  // bit 14
    unsigned int  LocalIntInputActive    : 1;  // bit 15
    unsigned int  LocalIntOutputEnable   : 1;  // bit 16
    unsigned int  LocalDoorbellIntEnable : 1;  // bit 17
    unsigned int  DmaChan0IntEnable      : 1;  // bit 18
    unsigned int  DmaChan1IntEnable      : 1;  // bit 19
    unsigned int  LocalDoorbellIntActive : 1;  // bit 20
    unsigned int  DmaChan0IntActive      : 1;  // bit 21
    unsigned int  DmaChan1IntActive      : 1;  // bit 22
    unsigned int  BistIntActive          : 1;  // bit 23
    unsigned int  DM_WasBusMastOnAbort   : 1;  // bit 24
    unsigned int  Dma0_WasBusMastOnAbort : 1;  // bit 25
    unsigned int  Dma1_WasBusMastOnAbort : 1;  // bit 26
    unsigned int  AbortAfter256Retries   : 1;  // bit 27
    unsigned int  DataInMailbox0         : 1;  // bit 28
    unsigned int  DataInMailbox1         : 1;  // bit 29
    unsigned int  DataInMailbox2         : 1;  // bit 30
    unsigned int  DataInMailbox3         : 1;  // bit 31
} INT_CSR;


//-----------------------------------------------------------------------------   
// Define the EEPROM CSR (CNTRL)
//-----------------------------------------------------------------------------   
typedef struct _EEPROM_CSR_ {
    unsigned int  PciReadCmdForDma       : 4;  // bit 0-3
    unsigned int  PciWriteCmdForDma      : 4;  // bit 4-7
    unsigned int  PciMemReadCmdForDM     : 4;  // bit 8-11
    unsigned int  PciMemWriteCmdForDM    : 4;  // bit 12-15
    unsigned int  GPIO_Output            : 1;  // bit 16
    unsigned int  GPIO_Input             : 1;  // bit 17
    unsigned int  User_i_Select          : 1;  // bit 18
    unsigned int  User_o_Select          : 1;  // bit 19
    unsigned int  LINT_o_IntStatus       : 1;  // bit 20
    unsigned int  TeaIntStatus           : 1;  // bit 21
    unsigned int  reserved               : 2;  // bit 22-23
    unsigned int  SerialEEPROMClockOut   : 1;  // bit 24
    unsigned int  SerialEEPROMChipSelect : 1;  // bit 25
    unsigned int  SerialEEPROMDataIn     : 1;  // bit 26
    unsigned int  SerialEEPROMDataOut    : 1;  // bit 27
    unsigned int  SerialEEPROMPresent    : 1;  // bit 28
    unsigned int  ReloadConfigRegisters  : 1;  // bit 29
    unsigned int  SoftwareReset          : 1;  // bit 30
    unsigned int  EEDOInputEnable        : 1;  // bit 31
} EEPROM_CSR;

//-----------------------------------------------------------------------------   
// Define the DMA Mode Register
//-----------------------------------------------------------------------------   
typedef struct _DMA_MODE_ {
    unsigned int  LocalBusDataWidth      : 2 ;  // bit 0-1
    unsigned int  WaitStateCounter       : 4 ;  // bit 2-5
    unsigned int  TaRdyInputEnable       : 1 ;  // bit 6
    unsigned int  BurstEnable            : 1 ;  // bit 7
    unsigned int  LocalBurstEnable       : 1 ;  // bit 8
    unsigned int  SgModeEnable           : 1 ;  // bit 9
    unsigned int  DoneIntEnable          : 1 ;  // bit 10
    unsigned int  LocalAddressMode       : 1 ;  // bit 11
    unsigned int  DemandMode             : 1 ;  // bit 12
    unsigned int  MWIEnable              : 1 ;  // bit 13
    unsigned int  EOTEnable              : 1 ;  // bit 14
    unsigned int  TermModeSelect         : 1 ;  // bit 15
    unsigned int  ClearCountMode         : 1 ;  // bit 16
    unsigned int  IntToPci               : 1 ;  // bit 17
    unsigned int  DACChainLoad           : 1 ;  // bit 18
    unsigned int  EOTEndLink             : 1 ;  // bit 19
    unsigned int  RingMgmtValidMode      : 1 ;  // bit 20
    unsigned int  RingMgmtValidStop      : 1 ;  // bit 21
    unsigned int  reserved               : 10;  // bit 22-31
} DMA_MODE;

//-----------------------------------------------------------------------------   
// Define the DMA Command Status Register (CSR)
//-----------------------------------------------------------------------------   
#pragma warning(disable:4214)  // bit field types other than int warning

typedef struct _DMA_CSR_ {
    unsigned char  Enable                 : 1;  // bit 0
    unsigned char  Start                  : 1;  // bit 1
    unsigned char  Abort                  : 1;  // bit 2
    unsigned char  Clear                  : 1;  // bit 3
    unsigned char  Done                   : 1;  // bit 4
    unsigned char  pad                    : 1;  // bit 5-7
} DMA_CSR;

#pragma warning(default:4214) 

//-----------------------------------------------------------------------------   
// PCI9659_REGS structure
//-----------------------------------------------------------------------------   
typedef struct _PCI9656_REGS_ {

    unsigned int      Space0_Range      ;  // 0x000 
    unsigned int      Space0_Remap      ;  // 0x004 
    unsigned int      Local_DMA_Arbit   ;  // 0x008 
    unsigned int      Endian_Desc       ;  // 0x00C 
    unsigned int      Exp_XP_ROM_Range  ;  // 0x010 
    unsigned int      Exp_ROM_Remap     ;  // 0x014 
    unsigned int      Space0_ROM_Desc   ;  // 0x018 
    unsigned int      DM_Range          ;  // 0x01C 
    unsigned int      DM_Mem_Base       ;  // 0x020 
    unsigned int      DM_IO_Base        ;  // 0x024 
    unsigned int      DM_PCI_Mem_Remap  ;  // 0x028 
                     
    unsigned int      pad1 [7]          ;  // range [0x02C - 0x044]
                     
    unsigned int      Mailbox2          ;  // 0x048 
    unsigned int      Mailbox3          ;  // 0x04C 
    unsigned int      Mailbox4          ;  // 0x050 
    unsigned int      Mailbox5          ;  // 0x054 
    unsigned int      Mailbox6          ;  // 0x058 
    unsigned int      Mailbox7          ;  // 0x05C 
                     
    unsigned int      Local_Doorbell    ;  // 0x060 
    unsigned int      PCI_Doorbell      ;  // 0x064 
    INT_CSR           Int_Csr           ;  // 0x068 
    EEPROM_CSR        EEPROM_Ctrl_Stat  ;  // 0x06C 
    unsigned int      Perm_Vendor_Id    ;  // 0x070 
    unsigned int      Revision_Id       ;  // 0x074 

    unsigned int      pad2 [2]          ;  // range [0x078 - 0x07C]

    DMA_MODE          Dma0_Mode         ;  // 0x080 
    unsigned int      Dma0_PCI_Addr     ;  // 0x084 
    unsigned int      Dma0_Local_Addr   ;  // 0x088 
    unsigned int      Dma0_Count        ;  // 0x08C 
    DESC_PTR          Dma0_Desc_Ptr     ;  // 0x090 
                       
    DMA_MODE          Dma1_Mode         ;  // 0x094 
    unsigned int      Dma1_PCI_Addr     ;  // 0x098 
    unsigned int      Dma1_Local_Addr   ;  // 0x09C 
    unsigned int      Dma1_Count        ;  // 0x0A0 
    DESC_PTR          Dma1_Desc_Ptr     ;  // 0x0A4 

    DMA_CSR           Dma0_Csr          ;  // 0x0A8 
    DMA_CSR           Dma1_Csr          ;  // 0x0A9

    unsigned char     pad3 [2]          ;  // pad to next 32-bit boundry

    unsigned int      Dma_Arbit         ;  // 0x0AC 
    unsigned int      Dma_Threshold     ;  // 0x0B0 
                       
    unsigned int      Dma0_PCI_DAC      ;  // 0x0B4 
    unsigned int      Dma1_PCI_DAC      ;  // 0x0B8 
                       
    unsigned int      pad4 [13]         ;  // range [0x0BC - 0x0EC]

    unsigned int      Space1_Range      ;  // 0x0F0 
    unsigned int      Space1_Remap      ;  // 0x0F4 
    unsigned int      Space1_Desc       ;  // 0x0F8 
    unsigned int      DM_DAC            ;  // 0x0FC 
                                                                    
    unsigned int      Arbiter_Cntl      ;  // 0x100
    unsigned int      Abort_Address     ;  // 0x104

} PCI9656_REGS, * PPCI9656_REGS; 
                                            

#endif  // __REG9656_H_

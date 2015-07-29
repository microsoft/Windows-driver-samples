/*++

Copyright (c) 2014 Microsoft Corporation

Module Name:

    sdhc.h

Abstract:

    Header for default standard host controller implementation.

Author:

    Greg Garbern (greggar) 01-May-2014

Environment:

    Kernel mode only.

Revision History:

    Greg Garbern (greggar) 12-June-2014

--*/

#if defined (_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses




















//
// Memory registers
//

#define SDHC_SYSADDR                    0x00
#define SDHC_BLOCK_SIZE                 0x04
#define SDHC_BLOCK_COUNT                0x06
#define SDHC_ARGUMENT                   0x08
#define SDHC_TRANSFER_MODE              0x0c
#define SDHC_COMMAND                    0x0e
#define SDHC_RESPONSE                   0x10
#define SDHC_DATA_PORT                  0x20
#define SDHC_PRESENT_STATE              0x24
#define SDHC_HOST_CONTROL               0x28
#define SDHC_POWER_CONTROL              0x29
#define SDHC_BLOCKGAP_CONTROL           0x2a
#define SDHC_WAKEUP_CONTROL             0x2b
#define SDHC_CLOCK_CONTROL              0x2c
#define SDHC_TIMEOUT_CONTROL            0x2e
#define SDHC_RESET                      0x2f
#define SDHC_INTERRUPT_STATUS           0x30
#define SDHC_ERROR_STATUS               0x32
#define SDHC_INTERRUPT_STATUS_ENABLE    0x34
#define SDHC_ERROR_STATUS_ENABLE        0x36
#define SDHC_INTERRUPT_SIGNAL_ENABLE    0x38
#define SDHC_ERROR_SIGNAL_ENABLE        0x3a
#define SDHC_AUTO_CMD12_ERROR_STATUS    0x3c
#define SDHC_HOST_CONTROL2              0x3e
#define SDHC_CAPABILITIES               0x40
#define SDHC_CAPABILITIES2              0x44
#define SDHC_MAXIMUM_CURRENT            0x48
#define SDHC_ADMA_ERROR_STATUS          0x54
#define SDHC_ADMA_SYSADDR_LOW           0x58
#define SDHC_ADMA_SYSADDR_HIGH          0x5c


#define SDHC_SLOT_INFORMATION           0xfc
#define SDHC_VERSION                    0xfe

//
// Bits defined in SDHC_TRANSFER_MODE
//

#define SDHC_TM_DMA_ENABLE              0x0001
#define SDHC_TM_BLKCNT_ENABLE           0x0002
#define SDHC_TM_AUTO_CMD12_ENABLE       0x0004
#define SDHC_TM_AUTO_CMD23_ENABLE       0x0008
#define SDHC_TM_TRANSFER_READ           0x0010
#define SDHC_TM_MULTIBLOCK              0x0020

//
// Bits defined in SDHC_COMMAND
//

#define SDHC_CMD_RESPONSE_136BIT        0x0001
#define SDHC_CMD_RESPONSE_48BIT_NOBUSY  0x0002
#define SDHC_CMD_RESPONSE_48BIT_WBUSY   0x0003

#define SDHC_CMD_CRC_CHECK_ENABLE       0x0008
#define SDHC_CMD_INDEX_CHECK_ENABLE     0x0010
#define SDHC_CMD_DATA_PRESENT           0x0020

#define SDHC_CMD_TYPE_SUSPEND           0x0040
#define SDHC_CMD_TYPE_RESUME            0x0080
#define SDHC_CMD_TYPE_ABORT             0x0060

//
// Bits defined in SDHC_PRESENT_STATE
//

#define SDHC_PS_CMD_INHIBIT             0x00000001
#define SDHC_PS_DAT_INHIBIT             0x00000002
#define SDHC_PS_DAT_ACTIVE              0x00000004
#define SDHC_PS_RETUNING_REQUEST        0x00000008
#define SDHC_PS_WRITE_TRANSFER_ACTIVE   0x00000100
#define SDHC_PS_READ_TRANSFER_ACTIVE    0x00000200
#define SDHC_PS_BUFFER_WRITE_ENABLE     0x00000400
#define SDHC_PS_BUFFER_READ_ENABLE      0x00000800

#define SDHC_PS_CARD_INSERTED           0x00010000
#define SDHC_PS_CARD_STATE_STABLE       0x00020000
#define SDHC_PS_CARD_DETECT             0x00040000
#define SDHC_PS_WRITE_PROTECT           0x00080000

#define SDHC_PS_DAT0_SIGNAL             0x00100000
#define SDHC_PS_DAT1_SIGNAL             0x00200000
#define SDHC_PS_DAT2_SIGNAL             0x00400000
#define SDHC_PS_DAT3_SIGNAL             0x00800000

#define SDHC_PS_DAT_3_0                 (SDHC_PS_DAT0_SIGNAL |  \
                                         SDHC_PS_DAT1_SIGNAL |  \
                                         SDHC_PS_DAT2_SIGNAL |  \
                                         SDHC_PS_DAT3_SIGNAL)

#define SDHC_PS_CMD_SIGNAL              0x01000000

//
// Bits defined in SDHC_HOST_CONTROL
//

#define SDHC_HC_LED_POWER               0x01
#define SDHC_HC_DATA_WIDTH_4BIT         0x02
#define SDHC_HC_ENABLE_HIGH_SPEED       0x04
#define SDHC_HC_DATA_WIDTH_8BIT         0x20

#define SDHC_HC_DMA_SELECT_MASK         0x18
#define SDHC_HC_DMA_SELECT_SDMA         0x00
#define SDHC_HC_DMA_SELECT_ADMA32       0x10
#define SDHC_HC_DMA_SELECT_ADMA64       0x18

//
// Bits defined in SDHC_HOST_CONTROL2
//

#define SDHC_HC2_SDR12                  0x0000
#define SDHC_HC2_SDR25                  0x0001
#define SDHC_HC2_SDR50                  0x0002
#define SDHC_HC2_SDR104                 0x0003
#define SDHC_HC2_DDR50                  0x0004
#define SDHC_HC2_HS200                  0x0005

#define SDHC_HC2_UHS_MODES              (SDHC_HC2_SDR12 |   \
                                         SDHC_HC2_SDR25 |   \
                                         SDHC_HC2_SDR50 |   \
                                         SDHC_HC2_SDR104|   \
                                         SDHC_HC2_DDR50 |   \
                                         SDHC_HC2_HS200)

#define SDHC_HC2_1_8V_SIGNALING         0x0008

#define SDHC_HC2_SELECT_TYPE_B          0x0000
#define SDHC_HC2_SELECT_TYPE_A          0x0010
#define SDHC_HC2_SELECT_TYPE_C          0x0020
#define SDHC_HC2_SELECT_TYPE_D          0x0030

#define SDHC_HC2_EXECUTE_TUNING         0x0040
#define SDHC_HC2_SELECT_SAMPLING_CLOCK  0x0080
#define SDHC_HC2_ENABLE_ASYNC_INTERRUPT 0x4000
#define SDHC_HC2_ENABLE_PRESET_VALUE    0x8000

//
// Bits defined in SDHC_POWER_CONTROL
//

#define SDHC_PC_BUS_POWER               0x01
#define SDHC_PC_1_8V                    0x0a
#define SDHC_PC_3_0V                    0x0c
#define SDHC_PC_3_3V                    0x0e

#define SDHC_PC_VOLTAGE_MASK            0x0e

//
// Bits defined in SDHC_BLOCKGAP_CONTROL
//

#define SDHC_BGC_STOP_NEXT_GAP          0x01
#define SDHC_BGC_CONTINUE               0x02
#define SDHC_BGC_READ_WAIT_ENABLE       0x04
#define SDHC_BGC_INTERRUPT_ENABLE       0x08

//
// Bits defined in SDHC_WAKEUP_CONTROL
//

#define SDHC_WC_CARD_INT_ENABLE         0x01
#define SDHC_WC_CARD_INSERTION_ENABLE   0x02
#define SDHC_WC_CARD_REMOVAL_ENABLE     0x04
#define SDHC_WC_CARD_SUPPORTS_WAKEUP    0x80


//
// Bits defined in SDHC_CLOCK_CONTROL
//

#define SDHC_CC_INTERNAL_CLOCK_ENABLE   0x0001
#define SDHC_CC_CLOCK_STABLE            0x0002
#define SDHC_CC_CLOCK_ENABLE            0x0004
#define SDHC_CC_CLOCK_DIVISOR_2         0x0100
#define SDHC_CC_CLOCK_DIVISOR_4         0x0200
#define SDHC_CC_CLOCK_DIVISOR_8         0x0400
#define SDHC_CC_CLOCK_DIVISOR_16        0x0800
#define SDHC_CC_CLOCK_DIVISOR_32        0x1000
#define SDHC_CC_CLOCK_DIVISOR_64        0x2000
#define SDHC_CC_CLOCK_DIVISOR_128       0x4000
#define SDHC_CC_CLOCK_DIVISOR_256       0x8000


//
// Bits defined in SDHC_TIMEOUT_CONTROL
//

#define SDHC_TC_COUNTER_MASK            0x0f
#define SDHC_TC_MAX_DATA_TIMEOUT        0x0e

//
// Bits defined in SDHC_RESET
//

#define SDHC_RESET_ALL                  0x01
#define SDHC_RESET_CMD                  0x02
#define SDHC_RESET_DAT                  0x04

//
// Bits defined in SDHC_INTERRUPT_STATUS
//                 SDHC_INTERRUPT_STATUS_ENABLE
//                 SDHC_INTERRUPT_SIGNAL_ENABLE
//

#define SDHC_IS_CMD_COMPLETE            0x0001
#define SDHC_IS_TRANSFER_COMPLETE       0x0002
#define SDHC_IS_BLOCKGAP_EVENT          0x0004
#define SDHC_IS_DMA_EVENT               0x0008
#define SDHC_IS_BUFFER_WRITE_READY      0x0010
#define SDHC_IS_BUFFER_READ_READY       0x0020
#define SDHC_IS_CARD_INSERTION          0x0040
#define SDHC_IS_CARD_REMOVAL            0x0080
#define SDHC_IS_CARD_INTERRUPT          0x0100
#define SDHC_IS_TUNING_INTERRUPT        0x1000

#define SDHC_IS_ERROR_INTERRUPT         0x8000

#define SDHC_IS_CARD_DETECT             (SDHC_IS_CARD_INSERTION | SDHC_IS_CARD_REMOVAL)

//
// Bits defined in SDHC_ERROR_STATUS
// Bits defined in SDHC_ERROR_STATUS_ENABLE
// Bits defined in SDHC_ERROR_SIGNAL_ENABLE
//
// PLEASE NOTE: these values need to match exactly the
//              "generic" values in sdbus.h
//

#define SDHC_ES_CMD_TIMEOUT             0x0001
#define SDHC_ES_CMD_CRC_ERROR           0x0002
#define SDHC_ES_CMD_END_BIT_ERROR       0x0004
#define SDHC_ES_CMD_INDEX_ERROR         0x0008
#define SDHC_ES_DATA_TIMEOUT            0x0010
#define SDHC_ES_DATA_CRC_ERROR          0x0020
#define SDHC_ES_DATA_END_BIT_ERROR      0x0040
#define SDHC_ES_BUS_POWER_ERROR         0x0080
#define SDHC_ES_AUTO_CMD12_ERROR        0x0100
#define SDHC_ES_ADMA_ERROR              0x0200
#define SDHC_ES_BAD_DATA_SPACE_ACCESS   0x2000

//
// Bits defined in SDHC_AUTO_CMD12_ERROR_STATUS
//

#define SDHC_ACMD12_NOT_EXECUTED        0x0001
#define SDHC_ACMD12_RESPONSE_TIMEOUT    0x0002
#define SDHC_ACMD12_RESPONSE_CRC_ERROR  0x0004
#define SDHC_ACMD12_END_BIT_ERROR       0x0008
#define SDHC_ACMD12_INDEX_ERROR         0x0010
#define SDHC_ACMD12_CWODAT_NOT_EXECUTED 0x0080


#define SDHC_MAX_CLOCK_DIVISOR          256
#define SDHC_MAX_CLOCK_DIVISOR_SPEC_3   2046


//
// determines the poll interval for host reset and clock setting (10ms)
//

#define SDHC_DEFAULT_POLL_INTERVAL      10000

//
// determines the poll count (10 ms * 100 tries = 1 second)
//

#define SDHC_DEFAULT_POLL_COUNT         100

#define SDHC_MAX_SET_POWER_RETRIES      5

#define SDHC_ALIGNMENT_SDMA             1
#define SDHC_ALIGNMENT_ADMA2            3
#define SDHC_ALIGNMENT_ADMA2_64         7

#define SDHC_SPEC_VERSION_1             0
#define SDHC_SPEC_VERSION_2             1
#define SDHC_SPEC_VERSION_3             2

//----------------------------------------------------------------------------
// Host register layout
//----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//
//  SD card information
//
// ---------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)

//
// The entire layout of an SD host controller in memory for debug purposes.
//

typedef struct _SD_HOST_CONTROLLER_REGISTERS {

    union {
        struct {
            USHORT Low;
            USHORT High;
        } Argument2;
        struct {
            USHORT Low;
            USHORT High;
        } SDMAAddress;
    } Argument2;

    USHORT BlockSize;                     // 04h

    USHORT BlockCount;                    // 06h

    union {
        struct {
            USHORT Low;                  // 08h
            USHORT High;                 // 0Ah
        } Argument1;
        ULONG Ulong;
    } Argument1;

    union {
        struct {
            USHORT DMAEnable : 1;
            USHORT BlockCountEnable : 1;
            USHORT AutoCommandEnable : 2;
            USHORT DataTransferDirectionSelect : 1;
            USHORT MultiBlockSelect : 1;
            USHORT Reserved0 : 10;
        } Flags;
        USHORT Value;                     // 0Ch
    } TransferMode;

    union {
        struct {
            USHORT ResponseTypeSelect : 2;
            USHORT Reserved0 : 1;
            USHORT CommandCRCCheckEnable : 1;
            USHORT CommandIndexCheckEnable : 1;
            USHORT DataPresentSelect : 1;
            USHORT CommandType : 2;
            USHORT CommandIndex : 6;
            USHORT Reserved1 : 2;
        } Flags;
        USHORT Value;                     // 0Eh
    } Command;

    union {

        struct {
            union {
                struct {
                    ULONG ReservedManufacturerTest0 : 1;
                    ULONG ReservedManufacturerTest1 : 1;
                    ULONG ReservedAppspecific : 1;
                    ULONG AKE_SEQ_ERROR : 1;
                    ULONG ReservedSDIO : 1;
                    ULONG APP_CMD : 1;
                    ULONG Reserved0 : 2;
                    ULONG READY_FOR_DATA : 1;
                    ULONG CURRENT_STATE : 4;
                    ULONG ERASE_RESSET : 1;
                    ULONG CARD_ECC_DISABLE : 1;
                    ULONG WP_ERASE_SKIP : 1;
                    ULONG CSD_OVERWRITE : 1;
                    ULONG RESERVED_DEFERRED_RESPONSE : 1;
                    ULONG RESERVED : 1;
                    ULONG ERROR : 1;
                    ULONG CC_ERROR : 1;
                    ULONG CARD_ECC_FAILED : 1;
                    ULONG ILLEGAL_COMMAND : 1;
                    ULONG COM_CRC_ERROR : 1;
                    ULONG LOCK_UNLOCK_FAILED : 1;
                    ULONG CARD_IS_LOCKED : 1;
                    ULONG WP_VIOLATION : 1;
                    ULONG ERASE_PARAM : 1;
                    ULONG ERASE_SEQ_ERROR : 1;
                    ULONG BLOCK_LEN_ERROR : 1;
                    ULONG ADDRESS_ERROR : 1;
                    ULONG OUT_OF_RANGE : 1;
                } Flags;
                ULONG Ulong;
            } CardStatus;
        } R1;

        struct {
            ULONG Reserved0[3];
            ULONG CardStatus;
        } R1b;

        struct {
            union {

                struct {
                    USHORT ManufacturingData : 12;
                    USHORT Reserved0 : 4;
                    ULONG SerialNumber;
                    UCHAR Revision;
                    UCHAR ProductName[5];
                    USHORT ApplicationID;
                    UCHAR ManufactureID;
                } CID;

                struct {
                    UCHAR Reserved0 : 2;
                    UCHAR FILE_FORMAT : 2;
                    UCHAR TMP_WRITE_PROTECT : 1;
                    UCHAR PERM_WRITE_PROTECT : 1;
                    UCHAR COPY : 1;
                    UCHAR FILE_FORMAT_GRP : 1;

                    USHORT Reserved1 : 5;
                    USHORT WRITE_BL_PARTIAL : 1;
                    USHORT WRITE_BL_LEN : 4;
                    USHORT R2W_FACTOR : 3;
                    USHORT Reserved2 : 2;
                    USHORT WP_GRP_ENABLE : 1;

                    ULONG WP_GRP_SIZE : 7;
                    ULONG SECTOR_SIZE : 7;
                    ULONG ERASE_BLK_EN : 1;
                    ULONG C_SIZE_MULT : 3;
                    ULONG VDD_W_CURR_MAX : 3;
                    ULONG VDD_W_CURR_MIN : 3;
                    ULONG VDD_R_CURR_MAX : 3;
                    ULONG VDD_R_CURR_MIN : 3;
                    ULONG C_SIZE_LOW : 2;

                    ULONG C_SIZE_HIGH : 10;
                    ULONG Reserved3 : 2;
                    ULONG DSR_IMP : 1;
                    ULONG READ_BLK_MISALIGN : 1;
                    ULONG WRITE_BLK_MISALIGN : 1;
                    ULONG READ_BL_PARTIAL : 1;
                    ULONG READ_BL_LEN : 4;
                    ULONG CCC : 12;

                    UCHAR TRAN_SPEED;
                    UCHAR NSAC;
                    UCHAR TAAC;

                    UCHAR Reserved4 : 6;
                    UCHAR CSD_STRUCTURE : 2;
                } CSDv1;

                struct {
                    UCHAR Reserved0 : 2;
                    UCHAR FILE_FORMAT : 2;
                    UCHAR TMP_WRITE_PROTECT : 1;
                    UCHAR PERM_WRITE_PROTECT : 1;
                    UCHAR COPY : 1;
                    UCHAR FILE_FORMAT_GRP : 1;

                    USHORT Reserved1 : 5;
                    USHORT WRITE_BL_PARTIAL : 1;
                    USHORT WRITE_BL_LEN : 4;
                    USHORT R2W_FACTOR : 3;
                    USHORT Reserved2 : 2;
                    USHORT WP_GRP_ENABLE : 1;

                    ULONG WP_GRP_SIZE : 7;
                    ULONG SECTOR_SIZE : 7;
                    ULONG ERASE_BLK_EN : 1;
                    ULONG Reserved3 : 1;
                    ULONG DeviceSizeLow : 16;

                    ULONG DeviceSizeHigh : 6;
                    ULONG Reserved4 : 6;
                    ULONG DSR_IMP : 1;
                    ULONG READ_BLK_MISALIGN : 1;
                    ULONG WRITE_BLK_MISALIGN : 1;
                    ULONG READ_BL_PARTIAL : 1;
                    ULONG READ_BL_LEN : 4;
                    ULONG CCC : 12;

                    UCHAR TRAN_SPEED : 8;
                    UCHAR NSAC : 8;
                    UCHAR TAAC : 8;
                    UCHAR Reserved5 : 6;
                    UCHAR CSD_STRUCTURE : 2;
                } CSDv2;
            } Register;
        } R2;

        struct {
            union {
                struct {
                    ULONG Reserved0 : 7;
                    ULONG ReservedLowVoltageRange : 1;
                    ULONG Reserved1 : 7;
                    ULONG Voltage27V_28V : 1;
                    ULONG Voltage28V_29V : 1;
                    ULONG Voltage29V_30V : 1;
                    ULONG Voltage30V_31V : 1;
                    ULONG Voltage31V_32V : 1;
                    ULONG Voltage32V_33V : 1;
                    ULONG Voltage33V_34V : 1;
                    ULONG Voltage34V_35V : 1;
                    ULONG Voltage35V_36V : 1;
                    ULONG SwitchingTo18VAccepted : 1;
                    ULONG Reserved2 : 5;
                    ULONG CardCapacityStatus : 1;
                    ULONG CardPowerUpStatus : 1;
                } Flags;
                ULONG ULong;
            } Ocr;
        } R3;

        struct {
            UCHAR Buffer[16];
        } R4;

        struct {
            UCHAR Buffer[16];
        } R5;

        struct {
            union {
                struct {
                    USHORT ReservedManufacturerTest0 : 1;
                    USHORT ReservedManufacturerTest1 : 1;
                    USHORT ReservedAppspecific : 1;
                    USHORT AKE_SEQ_ERROR : 1;
                    USHORT ReservedSDIO : 1;
                    USHORT APP_CMDEnable : 1;
                    USHORT Reserved0 : 2;
                    USHORT READY_FOR_DATA : 1;
                    USHORT CurrentState : 4;

                    USHORT ERROR : 1;
                    USHORT ILLEGAL_COMMAND : 1;
                    USHORT COM_CRC_ERROR : 1;
                } Flags;
                USHORT Register;
            } CardStatusBits;

            USHORT NewRCA : 16;
        } R6;

        struct {
            ULONG CheckPattern : 8;
            ULONG VoltageAccepted : 4;
            ULONG Reserved : 20;
        } R7;

        struct {
            USHORT Response0;                     // 10h
            USHORT Response1;                     // 12h
            USHORT Response2;                     // 14h
            USHORT Response3;                     // 16h
            USHORT Response4;                     // 18h
            USHORT Response5;                     // 1Ah
            USHORT Response6;                     // 1Ch
            USHORT Response7;                     // 1Eh
        } Words;
        UCHAR Buffer[16];
    } Response;

    union {
        struct {
            USHORT BufferDataPort0;               // 20h
            USHORT BufferDataPort1;               // 22h
        } Words;
        ULONG Port;
    } BufferDataPort;

    union {
        struct {
            USHORT PresentState0;                 // 24h
            USHORT PresentState1;                 // 26h
        } Words;
        struct {
            ULONG CommandInhibitCMD : 1;
            ULONG CommandInhibitDAT : 1;
            ULONG DATLineActive : 1;
            ULONG ReTuningRequest : 1;
            ULONG Reserved0 : 4;
            ULONG WriteTransferActive : 1;
            ULONG ReadTransferActive : 1;
            ULONG BufferWriteEnable : 1;
            ULONG BufferReadEnable : 1;
            ULONG Reserved1 : 4;
            ULONG CardInserted : 1;
            ULONG CardStateStable : 1;
            ULONG CardDetectPinLevel : 1;
            ULONG WriteProtectSwitchPinLevel : 1;
            ULONG DATLineSignalLevel : 4;
            ULONG CMDLineSignalLevel : 1;
            ULONG Reserved2 : 7;
        } Flags;
    } PresentState;

    union {
        struct {
            UCHAR LEDControlOn : 1;
            UCHAR DataTransferWidth4Bit : 1;
            UCHAR HighSpeedEnable : 1;
            UCHAR DMASelect : 2;
            UCHAR ExtendedDataTransferWidth8Bit : 1;
            UCHAR CardDetectTestLevelCardInserted : 1;
            UCHAR CardDetectSignalSelectionEnable : 1;
        } Flags;
        UCHAR Byte;                  // 28h
    } HostControl1;

    union {
        struct {
            UCHAR SDBusPowerOn : 1;
            UCHAR SDBUSVoltageSelect : 3;
            UCHAR Reserved0 : 4;
        } Flags;
        UCHAR Byte;                  // 29h
    } PowerControl;

    union {
        struct {
            UCHAR StopAtBlockGapRequest : 1;
            UCHAR ContinueRequest : 1;
            UCHAR ReadWaitControlEnable : 1;
            UCHAR InterruptAtBlockGapEnable : 1;
            UCHAR Reserved0 : 4;
        } Flags;
        UCHAR Byte;               // 2Ah
    } BlockGapControl;

    union {
        struct {
            UCHAR WakeupEventEnableOnSDCardInterrupt : 1;
            UCHAR WakeupEventEnableOnSDCardInsertion : 1;
            UCHAR WakeupEventEnableOnSDCardRemoval : 1;
            UCHAR Reserved0 : 5;
        } Flags;
        UCHAR Byte;                 // 2Bh
    } WakeupControl;

    union {
        struct {
            USHORT InternalClockEnable : 1;
            USHORT InternalClockStable : 1;
            USHORT SDClockEnable : 1;
            USHORT Reserved0 : 2;
            USHORT ClockGeneratorSelectProgrammable : 1;
            USHORT FrequencySelectUpperBits : 2;
            USHORT FrequencySelectLowerBits : 8;
        } Flags;
        USHORT Word;                  // 2Ch
    } ClockControl;

    union {
        struct {
            UCHAR DataTimeoutCounterValue : 4;
            UCHAR Reserved0 : 4;
        } Flags;
        UCHAR Byte;                // 2Eh
    } TimeoutControl;

    union {
        struct {
            UCHAR SoftwareResetForAll : 1;
            UCHAR SoftwareResetForCMDLine : 1;
            UCHAR SoftwareResetForDATLine : 1;
            UCHAR Reserved0 : 5;
        } Flags;
        UCHAR Byte;                 // 2Fh
    } SoftwareReset;

    union {
        struct {
            USHORT CommandComplete : 1;
            USHORT TransferComplete : 1;
            USHORT BlockGapEvent : 1;
            USHORT DMAInterrupt : 1;
            USHORT BufferWriteReady : 1;
            USHORT BufferReadReady : 1;
            USHORT CardInsertion : 1;
            USHORT CardRemoval : 1;
            USHORT CardInterrupt : 1;
            USHORT INT_A : 1;
            USHORT INT_B : 1;
            USHORT INT_C : 1;
            USHORT RetuningEvent : 1;
            USHORT Reserved0 : 2;
            USHORT ErrorInterrupt : 1;
        } Flags;
        USHORT Word;
    } NormalInterruptStatus;

    union {
        struct {
            USHORT CommandTimeoutError : 1;
            USHORT CommandCRCError : 1;
            USHORT CommandEndBitError : 1;
            USHORT CommandIndexError : 1;
            USHORT DataTimeoutError : 1;
            USHORT DataCRCError : 1;
            USHORT DataEndBitError : 1;
            USHORT CurrentLimitError : 1;
            USHORT AutoCMD12Error : 1;
            USHORT ADMAError : 1;
            USHORT TuningError : 1;
            USHORT Reserved0 : 1;
            USHORT VendorSpecific : 4;
        } Flags;
        USHORT Word;
    } ErrorInterruptStatus;

    union {
        struct {
            USHORT CommandCompleteStatusEnable : 1;
            USHORT TransferCompleteStatusEnable : 1;
            USHORT BlockGapEventStatusEnable : 1;
            USHORT DMAInterruptStatusEnable : 1;
            USHORT BufferWriteReadyStatusEnable : 1;
            USHORT BufferReadReadyStatusEnable : 1;
            USHORT CardInsertionStatusEnable : 1;
            USHORT CardRemovalStatusEnable : 1;
            USHORT CardInterruptStatusEnable : 1;
            USHORT INT_AStatusEnable : 1;
            USHORT INT_BStatusEnable : 1;
            USHORT INT_CStatusEnable : 1;
            USHORT RetuningEventStatusEnable : 1;
            USHORT Reserved0 : 2;
            USHORT FixedToZero : 1;
        } Flags;
        USHORT Word;   // 34h
    } NormalInterruptStatusEnable;

    union {
        struct {
            USHORT CommandTimeoutErrorStatusEnable : 1;
            USHORT CommandCRCErrorStatusEnable : 1;
            USHORT CommandEndBitErrorStatusEnable : 1;
            USHORT CommandIndexErrorStatusEnable : 1;
            USHORT DataTimeoutErrorStatusEnable : 1;
            USHORT DataCRCErrorStatusEnable : 1;
            USHORT DataEndBitErrorStatusEnable : 1;
            USHORT CurrentLimitErrorStatusEnable : 1;
            USHORT AutoCMD12ErrorStatusEnable : 1;
            USHORT ADMAErrorStatusEnable : 1;
            USHORT TuningErrorStatusEnable : 1;
            USHORT Reserved0 : 1;
            USHORT VendorSpecificStatusEnable : 4;
        } Flags;
        USHORT Word;
    } ErrorInterruptStatusEnable;    // 36h

    union {
        struct {
            USHORT CommandCompleteSignalEnable : 1;
            USHORT TransferCompleteSignalEnable : 1;
            USHORT BlockGapEventSignalEnable : 1;
            USHORT DMAInterruptSignalEnable : 1;
            USHORT BufferWriteReadySignalEnable : 1;
            USHORT BufferReadReadySignalEnable : 1;
            USHORT CardInsertionSignalEnable : 1;
            USHORT CardRemovalSignalEnable : 1;
            USHORT CardInterruptSignalEnable : 1;
            USHORT INT_ASignalEnable : 1;
            USHORT INT_BSignalEnable : 1;
            USHORT INT_CSignalEnable : 1;
            USHORT RetuningEventSignalEnable : 1;
            USHORT Reserved0 : 2;
            USHORT FixedToZero : 1;
        } Flags;
        USHORT Word;
    } NormalInterruptSignalEnable;   // 38h

    union {
        struct {
            USHORT CommandTimeoutErrorSignalEnable : 1;
            USHORT CommandCRCErrorSignalEnable : 1;
            USHORT CommandEndBitErrorSignalEnable : 1;
            USHORT CommandIndexErrorSignalEnable : 1;
            USHORT DataTimeoutErrorSignalEnable : 1;
            USHORT DataCRCErrorSignalEnable : 1;
            USHORT DataEndBitErrorSignalEnable : 1;
            USHORT CurrentLimitErrorSignalEnable : 1;
            USHORT AutoCMD12ErrorSignalEnable : 1;
            USHORT ADMAErrorSignalEnable : 1;
            USHORT TuningErrorSignalEnable : 1;
            USHORT Reserved0 : 1;
            USHORT VendorSpecificSignalEnable : 4;
        } Flags;
        USHORT Word;
    } ErrorInterruptSignalEnable;    // 3Ah

    union {
        struct {
            USHORT AutoCMD12NotExecuted : 1;
            USHORT AutoCMD12TimeoutError : 1;
            USHORT AutoCMD12CRCError : 1;
            USHORT AutoCMD12EndBitError : 1;
            USHORT AutoCMDIndexError : 1;
            USHORT Reserved0 : 2;
            USHORT CommandNotIssuedByAutoCMD12Error : 1;
            USHORT Reserved1 : 8;
        } Flags;
        USHORT Word;
    } AutoCMDErrorStatus;            // 3Ch

    union {
        struct {
            USHORT UHSModeSelect : 3;
            USHORT Signalling18V : 1;
            USHORT DriverStrengthSelect : 2;
            USHORT ExecuteTuning : 1;
            USHORT SamplingClockSelect : 1;
            USHORT Reserved : 6;
            USHORT AsynchronousInterruptEnable : 1;
            USHORT PresetValueEnable : 1;
        } Flags;
        USHORT Word;
    } HostControl2;                  // 3Eh

    union {
        struct {
            ULONGLONG TimeoutClockFrequency : 6;
            ULONGLONG Reserved0 : 1;
            ULONGLONG TimeoutClockUnit : 1;
            ULONGLONG BaseClockFrequencyForSDClock : 8;
            ULONGLONG MaxBlockLength : 2;
            ULONGLONG EmbeddedDrive8Bit : 1;
            ULONGLONG ADMA2SUpport : 1;
            ULONGLONG Reserved1 : 1;
            ULONGLONG HighSpeedSupport : 1;
            ULONGLONG SDMASupport : 1;
            ULONGLONG SuspendResumeSupport : 1;
            ULONGLONG VoltageSupport33 : 1;
            ULONGLONG VoltageSupport30 : 1;
            ULONGLONG VoltageSupport18 : 1;
            ULONGLONG Reserved2 : 1;
            ULONGLONG SystemBus64bitSupport : 1;
            ULONGLONG AsynchronousInterruptSupport : 1;
            ULONGLONG SlotType : 2;
            ULONGLONG SDR50Support : 1;
            ULONGLONG SDR104Support : 1;
            ULONGLONG DDR50Support : 1;
            ULONGLONG Reserved3 : 1;
            ULONGLONG DriverTypeASupport : 1;
            ULONGLONG DriverTypeCSupport : 1;
            ULONGLONG DriverTypeDSupport : 1;
            ULONGLONG Reserved4 : 1;
            ULONGLONG TimerCountForRetuning : 4;
            ULONGLONG Reserved5 : 1;
            ULONGLONG UseTuningForSDR50 : 1;
            ULONGLONG RetuningModes : 2;
            ULONGLONG ClockMultiplier : 8;
            ULONGLONG Reserved6 : 8;
        } Flags;
        struct {
            USHORT Capabilities0;                 // 40h
            USHORT Capabilities1;                 // 42h
            USHORT Capabilities2;                 // 44h
            USHORT Capabilities3;                 // 46h
        } Words;
        ULONGLONG ULongLong;
    } Capabilities;

    union {
        struct {
            ULONGLONG MaxCurrent33V : 8;
            ULONGLONG MaxCurrent30V : 8;
            ULONGLONG MaxCurrent18V : 8;
            ULONGLONG Reserved0 : 40;
        } Flags;
        struct {
            USHORT MaxCurrentCaps0;               // 48h
            USHORT MaxCurrentCaps1;               // 4Ah
            USHORT MaxCurrentCaps2;               // 4Ch
            USHORT MaxCurrentCaps3;               // 4Eh
        } Words;
        ULONGLONG ULongLong;
    } MaxCurrentCaps;

    union {
        struct {
            USHORT ForceEventForAutoCMD12NotExecuted : 1;
            USHORT ForceEventForAutoCMD12TimeoutError : 1;
            USHORT ForceEventForAutoCMD12CRCError : 1;
            USHORT ForceEventForAutoCMD12EndBitError : 1;
            USHORT ForceEventForAutoCMD12IndexError : 1;
            USHORT Reserved0 : 2;
            USHORT ForceEventForCommandNotIssuedByAutoCMD12Error : 1;
            USHORT Reserved1 : 8;
        } Flags;
        USHORT Word;             // 50h    
    } ForceEventAutoCMDError;

    union {
        struct {
            USHORT ForceCommandTimeoutError : 1;
            USHORT ForceCommandCRCError : 1;
            USHORT ForceCommandEndBitError : 1;
            USHORT ForceCommandIndexError : 1;
            USHORT ForceDataTimeoutError : 1;
            USHORT ForceDataCRCError : 1;
            USHORT ForceDataEndBitError : 1;
            USHORT ForceCurrentLimitError : 1;
            USHORT ForceAutoCMD12Error : 1;
            USHORT ForceADMAError : 1;
            USHORT Reserved0 : 2;
            USHORT VendorSpecific : 4;
        } Flags;
        USHORT Word;           // 52h
    } ForceEventInterruptError;

    union {
        struct {
            UCHAR ADMAErrorStates : 2;
            UCHAR ADMALengthMismatchError : 1;
            UCHAR Reserved0 : 5;
        } Flags;
        UCHAR Byte;
    } ADMAErrorStatus;

    UCHAR Reserved0;                     // 55h
    USHORT Reserved1;                     // 56h

    union {
        struct {
            USHORT ADMASystemAddress0;            // 58h
            USHORT ADMASystemAddress1;            // 5Ah
            USHORT ADMASystemAddress2;            // 5Ch
            USHORT ADMASystemAddress3;            // 5Eh
        } Words;
        ULONGLONG ULongLong;
    } ADMASystemAddress;

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueInit;                   // 60h

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueDefaultSpeed;           // 62h

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueHighSpeed;              // 64h

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueSDR12;                  // 66h

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueSDR25;                  // 68h

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueSDR50;                  // 6Ah

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueSDR104;                 // 6Ch

    union {
        struct {
            USHORT SDCLKFrequencySelectValue : 10;
            USHORT ClockGeneratorSelectValue : 1;
            USHORT Reserved0 : 3;
            USHORT DriverStrengthSelectValue : 2;
        } Flags;
        USHORT Word;
    } PresetValueDDR50;                  // 6Eh

    UCHAR Reserved2[112];                // 70h - DFh

    union {
        struct {
            ULONG NumberOfClockPins : 3;
            ULONG Reserved0 : 1;
            ULONG NumberOfInterruptInputPins : 2;
            ULONG Reserved1 : 2;
            ULONG BusWidthPreset : 7;
            ULONG Reserved2 : 1;
            ULONG ClockPinSelect : 3;
            ULONG Reserved3 : 1;
            ULONG InterruptPinSelect : 3;
            ULONG Reserved4 : 1;
            ULONG BackEndPowerControl : 7;
            ULONG Reserved5 : 1;
        } Flags;
        struct {
            USHORT Low;           // E0h
            USHORT High;         // E2h
        } Words;
        ULONG Ulong;
    } SharedBusControl;

    UCHAR Reserved3[12];                 // E4h - EFh
    UCHAR Reserved4[12];                 // F0h - FBh

    union {
        struct {
            USHORT Slot1 : 1;
            USHORT Slot2 : 1;
            USHORT Slot3 : 1;
            USHORT Slot4 : 1;
            USHORT Slot5 : 1;
            USHORT Slot6 : 1;
            USHORT Slot7 : 1;
            USHORT Slot8 : 1;
            USHORT Reserved0 : 8;
        } Flags;
        USHORT Word;           // FCh
    } SlotInterruptStatus;

    union {
        struct {
            USHORT SpecificationVersionNumber : 8;
            USHORT VendorVersionNumber : 8;
        } Flags;
        USHORT Word;         // FEh
    } HostControllerVersion;

} SD_HOST_CONTROLLER_REGISTERS, *PSD_HOST_CONTROLLER_REGISTERS;

// the size of the SD_CONTROLLER_REGISTER structure must always be 256 bytes
C_ASSERT(sizeof(SD_HOST_CONTROLLER_REGISTERS) == 256);

#pragma pack(pop)

// ---------------------------------------------------------------------------
//
// Register layout definitions
//
// ---------------------------------------------------------------------------

#pragma pack(1)

//
// Host control register
//

typedef union _SDHC_HOST_CONTROL_REGISTER {
    struct {
        UCHAR LedControl:1;
        UCHAR DataTransferWidth:1;
        UCHAR HgihSpeedEnable:1;
        UCHAR DmaSelect:2;
        UCHAR Reserved1:1;
        UCHAR CardDetectTestLevel:1;
        UCHAR CardDetectSignalSlection:1;
    };
    UCHAR AsUchar;
} SDHC_HOST_CONTROL_REGISTER, *PSDHC_HOST_CONTROL_REGISTER;

typedef union _SDHC_HOST_CONTROL2_REGISTER {
    struct {
        USHORT UhsModeSelect:3;
        USHORT SignalingEnable1_8V:1;
        USHORT DriverStrengthSelect:2;
        USHORT ExecuteTuning:1;
        USHORT SamplingClockSelect:1;
        USHORT Reserved:6;
        USHORT AsyncInterruptEnable:1;
        USHORT PresetValueEnable:1;
    };
    USHORT AsUshort;
} SDHC_HOST_CONTROL2_REGISTER, *PSDHC_HOST_CONTROL2_REGISTER;
        

//
// Power control register
//

typedef union _SDHC_POWER_CONTROL_REGISTER {
    struct {
        UCHAR SdBusPower:1;
        UCHAR SdBusPowerSelect:3;
        UCHAR Reserved1:4;
    };
    UCHAR AsUchar;
} SDHC_POWER_CONTROL_REGISTER, *PSDHC_POWER_CONTROL_REGISTER;


//
// Clock control register
//

typedef union _SDHC_CLOCK_CONTROL_REGISTER {
    struct {
        USHORT InternalClockEnable:1;
        USHORT InternalClockStable:1;
        USHORT SdClockEnable:1;
        USHORT Reserved1:4;
        USHORT SdclkFrequencySelect:8;
    };
    UCHAR AsUshort;
} SDHC_CLOCK_CONTROL_REGISTER, *PSDHC_CLOCK_CONTROL_REGISTER;


//
// Transfer mode register
//

typedef union _SDHC_TRANSFER_MODE_REGISTER {
    struct {
        USHORT DmaEnable:1;
        USHORT BlockCountEnable:1;
        USHORT AutoCmd12Enable:1;
        USHORT AutoCmd23Enable:1;
        USHORT DataTranferDirectionSelect:1;
        USHORT BlockModeSelect:1;
        USHORT Reserved2:10;
    };
    USHORT AsUshort;
} SDHC_TRANSFER_MODE_REGISTER, *PSDHC_TRANSFER_MODE_REGISTER;


//
// Capabilities register
//

typedef union _SDHC_CAPABILITIES_REGISTER {
   struct {
        ULONG TimeoutClockFrequency:6;
        ULONG Reserved1:1;
        ULONG TimeoutClockUnit:1;
        ULONG BaseClockFrequency:8;
        ULONG MaxBlockLength:2;
        ULONG Support8BitBus:1;
        ULONG Adma2Support:1;
        ULONG Reserved2:1;
        ULONG HighSpeedSupport:1;
        ULONG DmaSupport:1;
        ULONG SuspendResumeSupport:1;
        ULONG Voltage33:1;
        ULONG Voltage30:1;
        ULONG Voltage18:1;
        ULONG Reserved3:1;
        ULONG SystemBus64Support:1;
        ULONG AsynchronousInterruptSupport:1;
        ULONG SlotType:2;
    };
    ULONG AsUlong;
} SDHC_CAPABILITIES_REGISTER, *PSDHC_CAPABILITIES_REGISTER;

typedef union _SDHC_CAPABILITIES2_REGISTER {
   struct {
        ULONG SDR50Support:1;
        ULONG SDR104Support:1;
        ULONG DDR50Support:1;
        ULONG Reserved1:1;
        ULONG DriverTypeA:1;
        ULONG DriverTypeC:1;
        ULONG DriverTypeD:1;
        ULONG Reserved2:1;
        ULONG RetuningTimerCount:4;
        ULONG Reserved3:1;
        ULONG UseTuningForSDR50:1;
        ULONG RetuningModes:2;
        ULONG ClockMultiplier:8;
        ULONG Reserved4:8;
    };
    ULONG AsUlong;
} SDHC_CAPABILITIES2_REGISTER, *PSDHC_CAPABILITIES2_REGISTER;

// ---------------------------------------------------------------------------
//
//  ADMA2 descriptor table definitions
//
// ---------------------------------------------------------------------------

//
// Bits defined in descriptor->attribute
//

#define SDHC_ADMA2_ATTRIBUTE_VALID  0x00000001
#define SDHC_ADMA2_ATTRIBUTE_END    0x00000002
#define SDHC_ADMA2_ATTRIBUTE_INT    0x00000004

//
// Bits defined in descriptor->Action
//

#define SDHC_ADMA2_ACTION_NOP       0x00000000
#define SDHC_ADMA2_ACTION_TRAN      0x00000002
#define SDHC_ADMA2_ACTION_LINK      0x00000003

//
// Max transfer length per descriptor entry
//
// SD Host Controller Spec 1.13.4
// Length field is 16 bit field so we can go up to 64K (0xFFFF).
// However, we need to limit max length to 0xF000 because using 0xFFFF
// can cause un-aligned,  In addition, some HC require the length to be
// multiple of 0x1000 (except 1st and last descriptor)
//
#define SDHC_ADMA2_MAX_LENGTH_PER_ENTRY 0x0000F000

//
// Layout of a descriptor (excluding the Address field)
//

typedef union _SDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY {
    struct {
        ULONG Attribute:3;
        ULONG Reserved1:1;
        ULONG Action:2;
        ULONG Reserved2:10;
        ULONG Length:16;
    };
    ULONG AsUlong;
} SDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY, *PSDHC_ADMA2_DESCRIPTOR_TABLE_ENTRY;

#pragma pack()

//
// Voltage profile
// 

typedef union _SDHC_VOLTAGE_PROFILE {
    struct {
        ULONG Reserved1 : 7;
        ULONG VoltageLow : 1;
        ULONG Voltage20 : 1;
        ULONG Voltage21 : 1;
        ULONG Voltage22 : 1;
        ULONG Voltage23 : 1;
        ULONG Voltage24 : 1;
        ULONG Voltage25 : 1;
        ULONG Voltage26 : 1;
        ULONG Voltage27 : 1;
        ULONG Voltage28 : 1;
        ULONG Voltage29 : 1;
        ULONG Voltage30 : 1;
        ULONG Voltage31 : 1;
        ULONG Voltage32 : 1;
        ULONG Voltage33 : 1;
        ULONG Voltage34 : 1;
        ULONG Voltage35 : 1;
    };
    ULONG AsUlong;
} SDHC_VOLTAGE_PROFILE, *PSDHC_VOLTAGE_PROFILE;

typedef enum _SDHC_SPEED_MODE {
    SdhcSpeedModeNormal = 0,
    SdhcSpeedModeHigh,
    SdhcSpeedModeSDR50,
    SdhcSpeedModeDDR50,
    SdhcSpeedModeSDR104,
    SdhcSpeedModeHS200,
    SdhcSpeedModeHS400
} SDHC_SPEED_MODE;

#define SDHC_MAX_OUTSTANDING_REQUESTS 1

typedef struct _SDHC_EXTENSION {

    BOOLEAN Removable;

    //
    // Host register space.
    //

    PHYSICAL_ADDRESS PhysicalBaseAddress;
    PVOID BaseAddress;
    SIZE_T BaseAddressSpaceSize;
    PSD_HOST_CONTROLLER_REGISTERS BaseAddressDebug;

    //
    // Capabilities.
    //

    SDPORT_CAPABILITIES Capabilities;
    SDHC_SPEED_MODE SpeedMode;
    UCHAR SpecVersion;

    //
    // Requests to handle.
    //

    PSDPORT_REQUEST OutstandingRequests[SDHC_MAX_OUTSTANDING_REQUESTS];
    USHORT UnhandledEvents;
      
} SDHC_EXTENSION, *PSDHC_EXTENSION;

// ---------------------------------------------------------------------------
// PCI Config definitions
// ---------------------------------------------------------------------------

#define SDHC_PCICFG_SLOT_INFORMATION    0x40

NTSTATUS
DriverEntry(
    _In_ PVOID DriverObject,
    _In_ PVOID RegistryPath
    );

//-----------------------------------------------------------------------------
// SlotExtension callbacks.
//-----------------------------------------------------------------------------

SDPORT_GET_SLOT_COUNT SdhcGetSlotCount;
/*
NTSTATUS
SdhcGetSlotCount(
    _In_ PVOID Argument,
    _In_ SDPORT_BUS_TYPE BusType
    );
*/

SDPORT_GET_SLOT_CAPABILITIES SdhcGetSlotCapabilities;
/*
VOID
SdhcGetSlotCapabilities(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _Inout_ PSDPORT_CAPABILITIES Capabilities
    );
*/

SDPORT_INITIALIZE SdhcSlotInitialize;
/*
NTSTATUS
SdhcSlotInitialize(
    _Inout_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_ISSUE_BUS_OPERATION SdhcSlotIssueBusOperation;
/*
NTSTATUS
SdhcSlotIssueBusOperation(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _In_ PSDPORT_REQUEST Request
    );
*/

SDPORT_GET_CARD_DETECT_STATE SdhcSlotGetCardDetectState;
/*
BOOLEAN
SdhcSlotGetCardDetectState(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_GET_WRITE_PROTECT_STATE SdhcSlotGetWriteProtectState;
/*
BOOLEAN
SdhcSlotGetWriteProtectState(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_INTERRUPT SdhcSlotInterrupt;
/*
BOOLEAN
SdhcSlotInterrupt(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _Out_ PULONG Events,
	_Out_ PULONG Errors,
    _Out_ PBOOLEAN NotifyCardChange,
    _Out_ PBOOLEAN NotifySdioInterrupt,
    _Out_ PBOOLEAN NotifyTuning
    );
*/

SDPORT_ISSUE_REQUEST SdhcSlotIssueRequest;
/*
NTSTATUS
SdhcSlotIssueRequest(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension, 
    _In_ PSDPORT_REQUEST Request
    );
*/

SDPORT_GET_RESPONSE SdhcSlotGetResponse;
/*
VOID
SdhcSlotGetResponse(
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command,
    _Out_ PVOID ResponseBuffer
    );
*/

SDPORT_TOGGLE_EVENTS SdhcSlotToggleEvents;
/*
VOID
SdhcSlotToggleEevnts(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _In_ ULONG EventMask,
    _In_ BOOLEAN Enable
    );
*/

SDPORT_CLEAR_EVENTS SdhcSlotClearEvents;
/*
VOID
SdhcSlotClearEvents(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _In_ ULONG EventMask
    );
*/

SDPORT_REQUEST_DPC SdhcRequestDpc;
/*
VOID
SdhcRequestDpc(
    _In_ PVOID PrivateExtension,
    _Inout_ PSDPORT_REQUEST Request,
    _In_ ULONG Events,
    _In_ ULONG Errors
    );
*/

SDPORT_SAVE_CONTEXT SdhcSaveContext;
/*
VOID
SdhcSaveContext(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_RESTORE_CONTEXT SdhcRestoreContext;
/*
VOID
SdhcRestoreContext(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_PO_FX_POWER_CONTROL_CALLBACK SdhcPoFxPowerControlCallback;
/*
NTSTATUS
SdhcPoFxPowerControlCallback(
    _In_ PSD_MINIPORT Miniport,
    _In_ LPCGUID PowerControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_opt_ PSIZE_T BytesReturned
    )
*/

//-----------------------------------------------------------------------------
// Hardware access routines.
//-----------------------------------------------------------------------------

NTSTATUS
SdhcResetHost(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ UCHAR ResetType
    );

NTSTATUS
SdhcSetVoltage(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ SDPORT_BUS_VOLTAGE VoltageProfile
    );

NTSTATUS
SdhcSetClock(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Frequency
    );

NTSTATUS
SdhcSetBusWidth(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ UCHAR BusWidth
    );

NTSTATUS
SdhcSetSpeed(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ SDPORT_BUS_SPEED Speed
    );

NTSTATUS
SdhcSetHighSpeed(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Enable
    );

NTSTATUS
SdhcSetUhsMode(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ USHORT Mode
    );

NTSTATUS
SdhcSetSignaling(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Enable
    );

NTSTATUS
SdhcExecuteTuning(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

VOID
SdhcSetLed(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Enable
    );

NTSTATUS
SdhcSetPresetValue(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Enable
    );

NTSTATUS
SdhcEnableBlockGapInterrupt(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Enable
    );

VOID
SdhcSetBlockGapControl(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Continue,
    _In_ BOOLEAN RequestStop
    );

VOID
SdhcSetBlockGapControl(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ BOOLEAN Continue,
    _In_ BOOLEAN RequestStop
    );

VOID
SdhcEnableInterrupt(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG NormalMask
    );

VOID
SdhcDisableInterrupt(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG NormalMask
    );

USHORT
SdhcGetInterruptStatus(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

USHORT
SdhcGetErrorStatus(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

USHORT
SdhcGetAutoCmd12ErrorStatus(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

USHORT
SdhcGetAdmaErrorStatus(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

VOID
SdhcAcknowledgeInterrupts(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ USHORT Interrupts
    );

BOOLEAN
SdhcIsCardInserted(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

BOOLEAN
SdhcIsWriteProtected(
    _In_ PSDHC_EXTENSION SdhcExtension
    );

NTSTATUS
SdhcSendCommand(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SdhcGetResponse(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_COMMAND Command,
    _Out_ PVOID ResponseBuffer
    );

NTSTATUS
SdhcSetTransferMode(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

VOID
SdhcReadDataPort(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _Out_writes_all_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length
    );

VOID
SdhcWriteDataPort(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_reads_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    );

NTSTATUS
SdhcBuildTransfer(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SdhcStartTransfer(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SdhcBuildPioTransfer(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SdhcBuildAdmaTransfer(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SdhcStartPioTransfer(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SdhcStartAdmaTransfer(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ PSDPORT_REQUEST Request
    );

//-----------------------------------------------------------------------------
// General utility functions.
//-----------------------------------------------------------------------------

USHORT
SdhcCalcClockFrequency(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG TargetFrequency,
    _Out_opt_ PULONG ActualFrequency
    );

USHORT
SdhcConvertEventsToHwMask(
    _In_ ULONG Events
    );

NTSTATUS
SdhcConvertErrorToStatus(
    _In_ USHORT Error
    );

NTSTATUS
SdhcCreateAdmaDescriptorTable(
    _In_ PSDPORT_REQUEST Request,
    _In_ BOOLEAN Use64BitDescriptor,
    _Out_ PULONG TotalTransferLength
    );

VOID
SdhcInitializePciConfigSpace(
    _In_ PSD_MINIPORT Miniport
    );

__forceinline
USHORT
SdhcGetHwUhsMode(
    _In_ SDPORT_BUS_SPEED BusSpeed
    )

{

    NT_ASSERT(BusSpeed != SdBusSpeedNormal);

    NT_ASSERT(BusSpeed != SdBusSpeedHigh);

    switch (BusSpeed) {
    case SdBusSpeedSDR12:
        return SDHC_HC2_SDR12;

    case SdBusSpeedSDR25:
        return SDHC_HC2_SDR25;

    case SdBusSpeedSDR50:
        return SDHC_HC2_SDR50;

    case SdBusSpeedDDR50:
        return SDHC_HC2_SDR50;

    case SdBusSpeedSDR104:
        return SDHC_HC2_SDR50;

    //
    // PCI controllers don't support the higher speed eMMC modes.
    //

    case SdBusSpeedHS200:
    case SdBusSpeedHS400:
    default:
        break;
    }

    NT_ASSERTMSG("SDHC - Invalid bus speed selected", FALSE);

    return 0;
}

__forceinline
USHORT
SdhcConvertEventsToHwMask(
    _In_ ULONG EventMask
    )

{

    return EventMask & 0xFFFF;
}

__forceinline
USHORT
SdhcConvertErrorsToHwMask(
    _In_ ULONG ErrorMask
    )

{

    return ErrorMask & 0xFFFF;
}

__forceinline
NTSTATUS
SdhcConvertErrorToStatus(
    _In_ USHORT Error
    )

{

    if (Error == 0) {
        return STATUS_SUCCESS;
    }

    if (Error & (SDHC_ES_CMD_TIMEOUT | SDHC_ES_DATA_TIMEOUT)) {
        return STATUS_IO_TIMEOUT;
    }

    if (Error & (SDHC_ES_CMD_CRC_ERROR | SDHC_ES_DATA_CRC_ERROR)) {
        return STATUS_CRC_ERROR;
    }

    if (Error & (SDHC_ES_CMD_END_BIT_ERROR | SDHC_ES_DATA_END_BIT_ERROR)) {
        return STATUS_DEVICE_DATA_ERROR;
    }

    if (Error & SDHC_ES_CMD_INDEX_ERROR) {
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    if (Error & SDHC_ES_BUS_POWER_ERROR) {
        return STATUS_DEVICE_POWER_FAILURE;
    }

    return STATUS_IO_DEVICE_ERROR;
}


//-----------------------------------------------------------------------------
// Register access macros.
//-----------------------------------------------------------------------------

__forceinline
VOID
SdhcWriteRegisterUlong(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _In_ ULONG Data
    )

{

    SdPortWriteRegisterUlong(SdhcExtension->BaseAddress, Register, Data);
    return;
}

__forceinline
VOID
SdhcWriteRegisterUshort(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _In_ USHORT Data
    )

{

    SdPortWriteRegisterUshort(SdhcExtension->BaseAddress, Register, Data);
    return;
}

__forceinline
VOID
SdhcWriteRegisterUchar(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _In_ UCHAR Data
    )

{

    SdPortWriteRegisterUchar(SdhcExtension->BaseAddress, Register, Data);
    return;
}

__forceinline
ULONG
SdhcReadRegisterUlong(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register
    )

{
    return SdPortReadRegisterUlong(SdhcExtension->BaseAddress, Register);
}

__forceinline
USHORT
SdhcReadRegisterUshort(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register
    )

{
    return SdPortReadRegisterUshort(SdhcExtension->BaseAddress, Register);
}

__forceinline
UCHAR
SdhcReadRegisterUchar(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register
    )

{
    return SdPortReadRegisterUchar(SdhcExtension->BaseAddress, Register);
}

__forceinline
VOID
SdhcReadRegisterBufferUlong(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _Out_writes_all_(Length) PULONG Buffer,
    _In_ ULONG Length)

{
    SdPortReadRegisterBufferUlong(SdhcExtension->BaseAddress,
                                  Register,
                                  Buffer,
                                  Length);
}

__forceinline
VOID
SdhcReadRegisterBufferUshort(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _Out_writes_all_(Length) PUSHORT Buffer,
    _In_ ULONG Length)

{
    SdPortReadRegisterBufferUshort(SdhcExtension->BaseAddress,
                                   Register,
                                   Buffer,
                                   Length);
}

__forceinline
VOID
SdhcReadRegisterBufferUchar(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _Out_writes_all_(Length) PUCHAR Buffer,
    _In_ ULONG Length)

{
    SdPortReadRegisterBufferUchar(SdhcExtension->BaseAddress,
                                  Register,
                                  Buffer,
                                  Length);
}

__forceinline
VOID
SdhcWriteRegisterBufferUlong(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _In_reads_(Length) PULONG Buffer,
    _In_ ULONG Length
    )

{

    SdPortWriteRegisterBufferUlong(SdhcExtension->BaseAddress,
                                   Register,
                                   Buffer,
                                   Length);
}

__forceinline
VOID
SdhcWriteRegisterBufferUshort(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _In_reads_(Length) PUSHORT Buffer,
    _In_ ULONG Length
    )

{

    SdPortWriteRegisterBufferUshort(SdhcExtension->BaseAddress,
                                    Register,
                                    Buffer,
                                    Length);
}

__forceinline
VOID
SdhcWriteRegisterBufferUchar(
    _In_ PSDHC_EXTENSION SdhcExtension,
    _In_ ULONG Register,
    _In_reads_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    )

{

    SdPortWriteRegisterBufferUchar(SdhcExtension->BaseAddress,
                                   Register,
                                   Buffer,
                                   Length);

}

__forceinline
UCHAR
SdhcGetResponseLength(
    _In_ PSDPORT_COMMAND Command
    )

/*++

Routine Description:

    Return the number of bytes associated with a given response type.

Arguments:

    ResponseType

Return value:

    Length of response.

--*/

{
    
    UCHAR Length;
    
    switch (Command->ResponseType) {
    case SdResponseTypeR1:
    case SdResponseTypeR3:
    case SdResponseTypeR4:
    case SdResponseTypeR5:
    case SdResponseTypeR6:
    case SdResponseTypeR1B:
    case SdResponseTypeR5B:
        Length = 4;
        break;

    case SdResponseTypeR2:
        Length = 16;
        break;

    case SdResponseTypeNone:
        Length = 0;
        break;

    default:

        NT_ASSERTMSG("Invalid response type", FALSE);

        Length = 0;
        break;
    }

    return Length;
}

#define MIN(x,y) ((x) > (y) ? (y) : (x))        // return minimum among x & y
#define MAX(x,y) ((x) > (y) ? (x) : (y))        // return maximum among x & y
#define DIV_CEIL(x, y) (((x) + (y) - 1) / (y))

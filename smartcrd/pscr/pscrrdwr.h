/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrRdWr.h

Abstract:

    Constants & access function prototypes for SCM PSCR smartcard reader

Author:

    Andreas Straub

Notes:

    The file pscrrdwr.h was reviewed by LCA in June 2011 and per license is
    acceptable for Microsoft use under Dealpoint ID 178449.

Revision History:

    Andreas Straub          7/16/1997   Initial Version

--*/

#if !defined( __PSCR_RDWR_H__ )
#define __PSCR_RDWR_H__

#pragma pack( 1 )
//
//  The usage of the PSCR_REGISTERS struct is a little bit tricky:
//  We set the address of that stucture to the IO Base Port, then
//  the other reg's can accessed by their address.
//  p.E.    &PscrRegs = 0x320 --> &PscrRegs->CmdStatusReg = 0x321...
//
typedef struct _PSCR_REGISTERS {

    UCHAR  DataReg;
    UCHAR  CmdStatusReg;
    UCHAR  SizeLSReg;
    UCHAR  SizeMSReg;

 } PSCR_REGISTERS, *PPSCR_REGISTERS;

#define SIZEOF_PSCR_REGISTERS       ( sizeof( PSCR_REGISTERS  )

#pragma pack()


typedef enum _READER_POWER_STATE {
    PowerReaderUnspecified = 0,
    PowerReaderWorking,
    PowerReaderOff
} READER_POWER_STATE, *PREADER_POWER_STATE;

typedef void ( *PTRACKING_COMPLETION )( PVOID TrackingContext, UCHAR Status, BOOLEAN Wakeup );

#define MAX_DATARATES   4

typedef struct _READER_EXTENSION {

    //  I/O address where the reader is configured.
    PPSCR_REGISTERS IOBase;
    ULONG IOWindow;
    ULONG MaxIFSD;

    //  IRQ assigned by the system
    ULONG CurrentIRQ;

    //
    //  limit for read timeout. the absolute timeout limit is
    //  MaxRetries * DELAY_PSCR_WAIT
    //

    ULONG MaxRetries;

    //  Source/Destination byte always used by the PCMCIA (PC Card) reader.
    UCHAR Device;

    //  Software revision ID of the SwapSmart firmware.
    UCHAR FirmwareMajor, FirmwareMinor, UpdateKey;

    //
    //  Interrupt status; the flag is raised if an freeze event was detected
    //  and cleared if the interface was cleared (int service routine or
    //  PscrRead)
    //
    BOOLEAN FreezePending;

    //
    //  Selected file in the reader file system; the flag is raised if the
    //  ICC1 status file was selected & is cleared if any generic ioctl was
    //  issued (in this case another file may be selected)
    //
    BOOLEAN StatusFileSelected;

    //  used to cancel a read request
    BOOLEAN         RequestPending;
    //    set TRUE if an interrupt is sending, but the reader is busy
    BOOLEAN         RequestInterrupt;

    BOOLEAN InvalidStatus;

    BOOLEAN CardPresent;

    // Current reader power state.
    READER_POWER_STATE ReaderPowerState;

    PTRACKING_COMPLETION    CompletionRoutine;

    PVOID                   TrackingContext;

    ULONG                   dataRatesSupported[MAX_DATARATES];

} READER_EXTENSION, *PREADER_EXTENSION;

#define SIZEOF_READER_EXTENSION     ( sizeof( READER_EXTENSION ))

//
//  Constants -----------------------------------------------------------------
//
#define PSCR_ID_STRING              "SCM SwapSmart 2."

#define TLV_BUFFER_SIZE             0x20
#define ATR_SIZE                    0x40    // TS + 32 + SW + PROLOGUE + EPILOGUE...

#define PSCR_MAX_RETRIES            1000

#define CLEAR_BIT                   0x00

#define DEFAULT_WAIT_TIME           0x01

#define PSCR_PROLOGUE_LENGTH        0x03
#define PSCR_EXT_PROLOGUE_LENGTH    0x05
#define PSCR_STATUS_LENGTH          0x02

#define PSCR_LRC_LENGTH             0x01
#define PSCR_CRC_LENGTH             0x02

#define PSCR_EPILOGUE_LENGTH        PSCR_LRC_LENGTH

#define PCB_DEFAULT                 0x00

#define MAX_T1_BLOCK_SIZE           270
//
//  data buffer idx
//
#define PSCR_NAD                    0x00
#define PSCR_PCB                    0x01
#define PSCR_LEN                    0x02
#define PSCR_INF                    0x03
#define PSCR_APDU                   PSCR_INF
//
//  device identifier for reset, deactivate
//
#define DEVICE_READER               0x00
#define DEVICE_ICC1                 0x01
#define DEVICE_ICC2                 0x02
//
//  NAD's
//
#define NAD_TO_ICC1                 0x02
#define NAD_TO_ICC2                 0x42
#define NAD_TO_PSCR                 0x12
#define REMOTE_NAD_TO_ICC1          0x03
#define REMOTE_NAD_TO_ICC2          0x43
#define REMOTE_NAD_TO_PSCR          0x13
//
//  PSCR Commands
//
#define CLA_SET_INTERFACE_PARAM     0x80
#define CLA_FREEZE                  0x80
#define CLA_RESET                   0x20
#define CLA_DEACTIVATE              0x20
#define CLA_SELECT_FILE             0x00
#define CLA_READ_BINARY             0x00
#define CLA_WRITE_BINARY            0x00
#define CLA_VERIFY                  0x00
#define CLA_WARM_RESET              0x20
#define CLA_SOFTWARE_UPDATE         0x80
#define CLA_SET_MODE                0x80

#define INS_SET_INTERFACE_PARAM     0x60
#define INS_FREEZE                  0x70
#define INS_RESET                   0x10
#define INS_DEACTIVATE              0x14
#define INS_SELECT_FILE             0xA4
#define INS_READ_BINARY             0xB0
#define INS_WRITE_BINARY            0xD0
#define INS_VERIFY                  0x20
#define INS_WARM_RESET              0x1F
#define INS_SOFTWARE_UPDATE         0xFF
#define INS_SET_MODE                0x61

//
//  Status Read Only Register
//
#define PSCR_DATA_AVAIL_BIT         0x80
#define PSCR_FREE_BIT               0x40
#define PSCR_WRITE_ERROR_BIT        0x02
#define PSCR_READ_ERROR_BIT         0x01
//
//  Command Write Only Register...
//
#define PSCR_RESET_BIT              0x08
#define PSCR_SIZE_READ_BIT          0x04
#define PSCR_SIZE_WRITE_BIT         0x02
#define PSCR_HOST_CONTROL_BIT       0x01
//
//  Tags...
//
#define TAG_MODULE                  0x02
#define TAG_MEMORY_SIZE             0x03
#define TAG_UPDATE_KEY              0x08
#define TAG_SOFTWARE_REV            0x0F
#define TAG_BLOCK_COMP_OPTION       0x13
#define TAG_READER_MECH_OPTIONS     0x20
#define TAG_READER_STATUS           0x21
#define TAG_ICC_PROTOCOLS           0x22
#define TAG_BI                      0x23
#define TAG_FI                      0x24
#define TAG_PTS_PARAM               0x25
#define TAG_PROTOCOL_STATUS         0x26
#define TAG_SET_NULL_BYTES          0x2d
#define TAG_FREEZE_EVENTS           0x30
#define TAG_BIT_LENGTH              0x40
#define TAG_CGT                     0x41
#define TAG_BWT                     0x42
#define TAG_CWT                     0x43
#define TAG_PROTOCOL_PARAM          0x44
//
//  card power definitions ( Tag 0x21 )
//
#define PSCR_ICC_ABSENT             0x00
#define PSCR_ICC_PRESENT            0x01
#define PSCR_ICC_POWERED            0x02
#define PSCR_ICC_IN_TRANSP_MODE     0xA0
#define PSCR_ICC_UNKNOWN            0xFF
//
//  protocol definitions ( Tag 0x22 )
//
#define PSCR_PROTOCOL_UNDEFINED     0x00
#define PSCR_PROTOCOL_T0            0x01
#define PSCR_PROTOCOL_T1            0x02
#define PSCR_PROTOCOL_T14           0x03
#define PSCR_PROTOCOL_I2C           0x80
#define PSCR_PROTOCOL_3WIRE         0x81
#define PSCR_PROTOCOL_2WIRE         0x81

#define WTX_REQUEST                 0xC3
#define WTX_REPLY                   0xE3
//
//  File ID's
//
#define FILE_MASTER                     0x3F00
#define FILE_PSCR_CONFIG                0x0020
#define FILE_PSCR_DIR                   0x7F60
#define FILE_PSCR_DIR_CONFIG            0x6020
#define FILE_PSCR_DIR_STATUS            0x6021
#define FILE_PSCR_DIR_FREEZE_CONFIG     0x6030
#define FILE_PSCR_DIR_FREEZE_STATUS     0x6031
#define FILE_ICC1_DIR                   0x7F70
#define FILE_ICC1_DIR_CONFIG            0x7020
#define FILE_ICC1_DIR_STATUS            0x7021
#define FILE_ICC2_DIR_CONFIG            0x7120
#define FILE_ICC2_DIR_STATUS            0x7121
//
//  Status Word Definitions
//
#define PSCR_SW_COMMAND_FAIL            0x6985
#define PSCR_SW_INVALID_PARAM           0x6A80
#define PSCR_SW_INCONSISTENT_DATA       0x6A85
#define PSCR_SW_NO_PROTOCOL_SUPPORT     0x62A3
#define PSCR_SW_SYNC_ATR_SUCCESS        0x9000
#define PSCR_SW_ASYNC_ATR_SUCCESS       0x9001
#define PSCR_SW_NO_PROTOCOL             0x62A5
#define PSCR_SW_NO_ATR                  0x62A6
#define PSCR_SW_NO_ATR_OR_PROTOCOL      0x62A7
#define PSCR_SW_NO_ICC                  0x64A1
#define PSCR_SW_ICC_NOT_ACTIVE          0x64A2
#define PSCR_SW_NON_SUPPORTED_PROTOCOL  0x64A3
#define PSCR_SW_PROTOCOL_ERROR          0x64A8
#define PSCR_SW_NO_ATR_OR_PROTOCOL2     0x64A7
#define PSCR_SW_FILE_NOT_FOUND          0x6A82
#define PSCR_SW_FILE_NO_ACCEPPTED_AUTH  0x6982
#define PSCR_SW_FILE_NO_ACCESS          0x6985
#define PSCR_SW_FILE_BAD_OFFSET         0x6B00
#define PSCR_SW_END_OF_FILE_READ        0x6282
#define PSCR_SW_END_OF_FILE_WRITE       0x6301
#define PSCR_SW_WRITE_FILE_FAIL         0x6500
#define PSCR_SW_NO_PASSWORD             0x6200
#define PSCR_SW_WRONG_PASSWORD          0x6300
#define PSCR_SW_VERIFY_COUNTER_FAIL     0x6983
#define PSCR_SW_NO_REF_DATA             0x6A88
#define PSCR_SW_FLASH_MEM_ERROR         0x6481
#define PSCR_SW_FLASH_MEM_ERR2          0x6581
#define PSCR_SW_WRONG_LENGTH            0x6700
#define PSCR_SW_UNKNOWN_ICC_ERROR       0x64A0
#define PSCR_SW_UNKNOWN_PROTOCOL_ERROR  0x64A9
#define PSCR_SW_NO_PROTOCOL_SELECTED    0x64A5
#define PSCR_SW_PTS_PROTOCOL_ERROR      0x64AA
#define PSCR_SW_WTX_ERROR               0x64AB
#define PSCR_SW_WTX_ERR2                0x65AB
#define PSCR_SW_INVALID_SOURCE_ADDR     0x6F82

//
//  Prototypes for access functions -------------------------------------------
//


VOID
PscrFlushInterface(
    PREADER_EXTENSION   ReaderExtension     //  context of call
    );

NTSTATUS
PscrRead(
    _In_  PREADER_EXTENSION       ReaderExtension,   //    context of call
    _Out_writes_bytes_(DataLen) PUCHAR  pData,             //    ptr to data buffer
    _In_  ULONG                   DataLen,           //    length of data
    _Out_ PULONG                  pNBytes            //    number of bytes read
    );

NTSTATUS
PscrReadDirect(
    PREADER_EXTENSION    ReaderExtension,       //    context of call
    _Out_writes_bytes_(DataLen) PUCHAR  pData,        //    ptr to data buffer
    ULONG                DataLen,               //    length of data
    PULONG                pNBytes               //    number of bytes read
    );

NTSTATUS
PscrWrite(
    PREADER_EXTENSION   ReaderExtension,        //  context of call
    PUCHAR              pData,                  //  ptr to data buffer
    ULONG               DataLength,             //  length of data
    PULONG              pNBytes                 //  number of bytes written
    );

NTSTATUS
PscrWriteDirect(
    PREADER_EXTENSION   ReaderExtension,        //  context of call
    PUCHAR              pData,                  //  ptr to data buffer
    ULONG               DataLength,             //  length of data
    PULONG              pNBytes                 //  number of bytes written
    );

UCHAR
PscrCalculateLRC(
    PUCHAR              pData,                  //  ptr to data buffer
    USHORT              DataLength              //  length of data
    );

NTSTATUS
PscrWait(
    PREADER_EXTENSION   ReaderExtension,        //  context of call
    UCHAR               Mask                    //  mask of requested bits
    );

#endif  //  __PSCR_RDWR_H__

//  ------------------------------- END OF FILE -------------------------------



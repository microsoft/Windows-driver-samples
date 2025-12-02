/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    common.h

Abstract:

    Common functions that might be moved to port lib

Notes:

Revision History:

--*/

#pragma once

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

#define ATA_BLOCK_SIZE                      512     //0x200

#define MAXULONG                            0xFFFFFFFF  // winnt

#define ATA_INQUIRYDATA_SIZE                0x3C  // contains 1 Version Descriptor for 1667

#define DEFAULT_DSM_DATA_SIZE_CAP_IN_BLOCKS 0x8

// per ATA spec, the SectorCount value range is 1 - 0xffff. 0 means this entry is not valid.
#define MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE 0xFFFF

// Max of LBAs that can be represented using 28 bits.
#define MAX_28BIT_LBA   (1 << 28)


#define NVC_ATA_NV_CACHE_COMMAND                0xB6

#define IDE_FEATURE_INVALID                     0xFF

#define SCSI_VENDOR_ID_LENGTH                   (8)

//
// ATA function code
//

// 0x100 - 0x1FF indicate ATA commands
#define ATA_FUNCTION_ATA_COMMAND                0x100
#define ATA_FUNCTION_ATA_IDENTIFY               0x101
#define ATA_FUNCTION_ATA_READ                   0x102
#define ATA_FUNCTION_ATA_WRITE                  0x103
#define ATA_FUNCTION_ATA_FLUSH                  0x104
#define ATA_FUNCTION_ATA_SMART                  0x105

// 0x200 - 0x2FF indicate ATAPI commands
#define ATA_FUNCTION_ATAPI_COMMAND              0x200
#define ATA_FUNCTION_REQUEST_SENSE              0x201

// 0x400 - 0x4FF indicate SATA CFIS format
#define ATA_FUNCTION_ATA_CFIS_PAYLOAD           0x400

//
// ATA flags
//

/* defined in ntscsi.h
#define ATA_FLAGS_DRDY_REQUIRED         (1 << 0)
#define ATA_FLAGS_DATA_IN               (1 << 1)
#define ATA_FLAGS_DATA_OUT              (1 << 2)
#define ATA_FLAGS_48BIT_COMMAND         (1 << 3)
#define ATA_FLAGS_USE_DMA               (1 << 4)
#define ATA_FLAGS_NO_MULTIPLE           (1 << 5)
*/

#define ATA_FLAGS_RETURN_RESULTS        (1 << 6)    // task file should be copied back into SenseInfoBuffer
#define ATA_FLAGS_HIGH_PRIORITY         (1 << 7)    // the request should be processed as a high priority request
#define ATA_FLAGS_NEW_CDB               (1 << 8)    // new CDB in SrbExtension should be issued to device rather than CDB in Srb
#define ATA_FLAGS_COMPLETE_SRB          (1 << 9)    // indicates the Srb should be completed, AhciCompleteRequest will not send command from SrbExtension.
#define ATA_FLAGS_ACTIVE_REFERENCE      (1 << 10)   // indicates Active Reference needs to be acquired before processing the Srb and released after processing the Srb
#define ATA_FLAGS_SENSEDATA_SET         (1 << 11)   // indicates sense data has been set to the Srb

//
// helper macros
//
#define IsAtaCommand(AtaFunction)               (AtaFunction & ATA_FUNCTION_ATA_COMMAND)
#define IsAtapiCommand(AtaFunction)             (AtaFunction & ATA_FUNCTION_ATAPI_COMMAND)
#define IsRequestSenseSrb(AtaFunction)          (AtaFunction == ATA_FUNCTION_REQUEST_SENSE)
#define IsAtaCfisPayload(AtaFunction)           (AtaFunction == ATA_FUNCTION_ATA_CFIS_PAYLOAD)

#define IsReturnResults(AtaFlags)       (AtaFlags & ATA_FLAGS_RETURN_RESULTS)
#define IsNewCdbUsed(AtaFlags)          (AtaFlags & ATA_FLAGS_NEW_CDB)
#define Is48BitCommand(AtaFlags)        (AtaFlags & ATA_FLAGS_48BIT_COMMAND)
#define IsHighPriorityCommand(AtaFlags) (AtaFlags & ATA_FLAGS_HIGH_PRIORITY)
#define IsDmaCommand(AtaFlags)          (AtaFlags & ATA_FLAGS_USE_DMA)
#define SrbShouldBeCompleted(AtaFlags)  (AtaFlags & ATA_FLAGS_COMPLETE_SRB)


#define CLRMASK(x, mask)     ((x) &= ~(mask));
#define SETMASK(x, mask)     ((x) |=  (mask));

typedef enum _AHCI_ETW_EVENT_IDS {
    AhciEtwEventSystemPowerHint = 0,
    AhciEtwEventUnitHybridGetInfo = 6,
    AhciEtwEventUnitHybridCachingMediumEnable = 7,
    AhciEtwEventUnitHybridCachingMediumDisable = 8,
    AhciEtwEventUnitHybridChangePriorityByLBA = 9,
    AhciEtwEventUnitHybridDemoteBySize = 10,
    AhciEtwEventUnitHybridEvict = 11,
    AhciEtwEventUnitHybridSetDirtyThreshold = 12,
    AhciEtwEventUnitHybridWriteThrough = 13,
    AhciEtwEventUnitFirmwareIoctl = 14,
    AhciEtwEventUnitFirmwareInfo = 15,
    AhciEtwEventUnitFirmwareDownload = 16,
    AhciEtwEventUnitFirmwareDownloadComplete = 17,
    AhciEtwEventUnitFirmwareActivate = 18,
    AhciEtwEventUnitFirmwareActivateComplete = 19,
    AhciEtwEventUnitGetPhysicalElementStatusComplete = 20,
    AhciEtwEventUnitRemoveElementAndTruncateComplete = 21,
    AhciEtwEventUnitGetInternalStatusDataHeaderComplete = 22,
    AhciEtwEventUnitGetInternalStatusDataComplete = 23,
    AhciEtwEventBuildIO = 24,
    AhciEtwEventStartIO = 25,
    AhciEtwEventHandleInterrupt = 26,
    AhciEtwEventPortReset = 27,
    AhciEtwEventIOCompletion = 28
} AHCI_ETW_EVENT_IDS, *PAHCI_ETW_EVENT_IDS;

//
// task file register contents
//
#include <pshpack1.h>
typedef struct _ATAREGISTERS {

    // ATA features/error register
    UCHAR bFeaturesReg;

    // sector count
    UCHAR bSectorCountReg;

    // block number (7:0)
    UCHAR bSectorNumberReg;

    // cylinder number (7:0) or LBA (15:8)
    UCHAR bCylLowReg;

    // cylinder number (15:8) or LBA (23:16)
    UCHAR bCylHighReg;

    // device/Head and LBA (27:24)
    UCHAR bDriveHeadReg;

    // command/status register
    UCHAR bCommandReg;

    // Reserved for future use. Shall be 0
    UCHAR bReserved;

} ATAREGISTERS, *PATAREGISTERS;
#include <poppack.h>

//
// task file (to hold 48 bit)
//
typedef struct _ATA_TASK_FILE {
    ATAREGISTERS Current;
    ATAREGISTERS Previous;
} ATA_TASK_FILE, *PATA_TASK_FILE;

// helper functions for setting TaskFile fields
#define SetFeaturesReg(reg, val)      (reg->bFeaturesReg = val)
#define SetSectorCount(reg, val)      (reg->bSectorCountReg = val)
#define SetSectorNumber(reg, val)     (reg->bSectorNumberReg = val)
#define SetCylinderLow(reg, val)      (reg->bCylLowReg = val)
#define SetCylinderHigh(reg, val)     (reg->bCylHighReg = val)
#define SetDeviceReg(reg, val)        (reg->bDriveHeadReg |= (0xA0 | val)) // only have Device0 for SATA
#define SetCommandReg(reg, val)       (reg->bCommandReg = val)

#define ATA_NCQ_FUA_BIT         (1 << 7)

//
// Device type
//
typedef enum  {
    DeviceUnknown = 0,
    DeviceIsAta,
    DeviceIsAtapi,
    DeviceNotExist
} ATA_DEVICE_TYPE;

//
// addressing mode
//
typedef enum {
    UnknownMode = 0,
    LbaMode,
    Lba48BitMode
} ATA_ADDRESS_TRANSLATION;


typedef struct _ATA_DEVICE_PARAMETERS {

    ATA_DEVICE_TYPE AtaDeviceType;  // device type (ata, atapi etc)
    UCHAR           ScsiDeviceType; // Scsi device type from inquiry data; Ata device: DIRECT_ACCESS_DEVICE

    UCHAR           MaximumLun;

    UCHAR           MaxDeviceQueueDepth;  // number of requests the device can handle at a time

    struct {
        ULONG   RemovableMedia: 1;
        ULONG   SystemPoweringDown: 1;
        ULONG   FuaSupported: 1;
        ULONG   FuaSucceeded: 1;

        ULONG   NeedUpdateIdentifyDeviceData: 1;
    } StateFlags;

    ULONG   BytesPerLogicalSector;
    ULONG   BytesPerPhysicalSector;
    ULONG   BytesOffsetForSectorAlignment;

    // Geometry
    ATA_ADDRESS_TRANSLATION AddressTranslation;

    LARGE_INTEGER           MaxLba;

    // Strings used to construct device ids, +1 for 'NULL'
    UCHAR   VendorId[40 + 1];
    UCHAR   RevisionId[8 + 1];
    UCHAR   SerialNumber[20 + 1];

    // max number of blocks that can be transferred for DSM - Trim command
    ULONG   DsmCapBlockCount;

} ATA_DEVICE_PARAMETERS, *PATA_DEVICE_PARAMETERS;

typedef struct _ATA_LBA_RANGE {
    ULONGLONG   StartSector:48;
    ULONGLONG   SectorCount:16;
} ATA_LBA_RANGE, *PATA_LBA_RANGE;

typedef struct _ATA_TRIM_CONTEXT {
    // Block Descriptor for UNMAP request
    PUNMAP_BLOCK_DESCRIPTOR BlockDescriptors;

    // Block Descriptor count for UNMAP request
    ULONG   BlockDescrCount;

    // count of ATA Lba Range entries can be sent by one Trim command
    ULONG   MaxLbaRangeEntryCountPerCmd;

    // count of ATA Lba Range entries need for an UNMAP request
    //    max Block Descriptors carried by one Unmap command: 0xFFFE; max ATA Lba Ranges need for a Unmap Block Descriptor: 0xFFFFFFFF/0xFFFF
    //    ULONG is big enough to hold the value.
    ULONG   NeededLbaRangeEntryCount;

    // the length of allocated. Need to cache the value for calling AhciFreeDmaBuffer
    ULONG   AllocatedBufferLength;

    // count of ATA Lba Range entries that already be processed.
    ULONG   ProcessedLbaRangeEntryCount;

    // current Index of the input UNMAP Block Descriptors
    ULONG   BlockDescrIndex;

    // current UNMAP Block Descriptor being processed
    UNMAP_BLOCK_DESCRIPTOR  CurrentBlockDescr;

} ATA_TRIM_CONTEXT, *PATA_TRIM_CONTEXT;

typedef struct _HYBRID_CHANGE_BY_LBA_CONTEXT {
    // Data Set Ranges of DSM CHANGE BY LBA request
    PDEVICE_DATA_SET_RANGE DataSetRanges;

    // Data Set Range count of DSM CHANGE BY LBA request
    ULONG       DataSetRangeCount;

    // count of ATA Lba Range entries need for one DSM CHANGE BY LBA request
    ULONG       NeededLbaRangeEntryCount;

    // count of ATA Lba Range entries that already be processed.
    ULONG       ProcessedLbaRangeEntryCount;

    // current Index of the input Data Set Ranges of DSM CHANGE BY LBA request
    ULONG       DataSetRangeIndex;

    // current Data Set Range (in sectors) being processed
    ULONGLONG   CurrentRangeStartLba;
    ULONGLONG   CurrentRangeLbaCount;

    UCHAR       TargetPriority;
    UCHAR       Reserved[7];
} HYBRID_CHANGE_BY_LBA_CONTEXT, *PHYBRID_CHANGE_BY_LBA_CONTEXT;

typedef struct _HYBRID_EVICT_CONTEXT {
    // Data Set Ranges of DSM EVICT request
    PDEVICE_DATA_SET_RANGE DataSetRanges;

    // Data Set Range count of DSM EVICT request
    ULONG   DataSetRangeCount;

    // count of ATA Lba Range entries can be sent by one Evict command
    ULONG   MaxLbaRangeEntryCountPerCmd;

    // count of ATA Lba Range entries need for an Evict request
    ULONG   NeededLbaRangeEntryCount;

    // the length of allocated. Need to cache the value for calling AhciFreeDmaBuffer
    ULONG   AllocatedBufferLength;

    // count of ATA Lba Range entries that already be processed.
    ULONG   ProcessedLbaRangeEntryCount;

    // current Index of the input DSM EVICT Data Set Ranges
    ULONG   DataSetRangeIndex;

    // current DSM EVICT Data Set Range (in sectors) being processed
    ULONGLONG CurrentRangeStartLba;
    ULONGLONG CurrentRangeLbaCount;

} HYBRID_EVICT_CONTEXT, *PHYBRID_EVICT_CONTEXT;

__inline
BOOLEAN
IsUnknownDevice(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AtaDeviceType == DeviceUnknown);
}

__inline
BOOLEAN
IsAtapiDevice(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AtaDeviceType == DeviceIsAtapi);
}

__inline
BOOLEAN
IsAtaDevice(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AtaDeviceType == DeviceIsAta);
}

__inline
BOOLEAN
IsRemovableMedia(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->StateFlags.RemovableMedia == 1);
}



__inline
BOOLEAN
Support48Bit(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->AddressTranslation == Lba48BitMode);
}


__inline
ULONG
BytesPerLogicalSector(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return DeviceParameters->BytesPerLogicalSector;
}

__inline
ULONG64
MaxUserAddressableLba(
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters
    )
{
    return (DeviceParameters->MaxLba.QuadPart);
}

__inline
PCDB
RequestGetSrbScsiData (
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_opt_ PULONG             CdbLength,
    _In_opt_ PUCHAR             ScsiStatus,
    _In_opt_ PVOID*             SenseInfoBuffer,
    _In_opt_ PUCHAR             SenseInfoBufferLength
    )
/*++

Routine Description:

    Helper function to retrieve SCSI related fields from a SRB.

Arguments:

    Srb - Pointer to Srb or SrbEx.

    CdbLength - Pointer to buffer to hold CdbLength field value for CDB

    ScsiStatus - Buffer to hold address ScsiStatus field value.

    SenseInfoBuffer - Pointer to SenseInfoBuffer buffer.

    SenseInfoBufferLength - Pointer to buffer to hold SenseInfoBufferLength value.

Return Value:

    Pointer to Cdb field

--*/
{
    PCDB                    cdb = NULL;

    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        //
        // This is SrbEx - STORAGE_REQUEST_BLOCK
        //
        cdb = SrbGetScsiData(Srb, (PUCHAR)CdbLength, CdbLength, ScsiStatus, SenseInfoBuffer, SenseInfoBufferLength);

    }  else if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {
        //
        // This is legacy SCSI_REQUEST_BLOCK
        //
        PSCSI_REQUEST_BLOCK  srb = (PSCSI_REQUEST_BLOCK)Srb;

        if (CdbLength) {
            *CdbLength = srb->CdbLength;
        }

        if (srb->CdbLength > 0) {
            cdb = (PCDB)srb->Cdb;
        }

        if (ScsiStatus) {
            *ScsiStatus = srb->ScsiStatus;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = srb->SenseInfoBuffer;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = srb->SenseInfoBufferLength;
        }

    } else {
        if (CdbLength) {
            *CdbLength = 0;
        }

        if (ScsiStatus) {
            *ScsiStatus = SCSISTAT_GOOD;
        }

        if (SenseInfoBuffer) {
            *SenseInfoBuffer = NULL;
        }

        if (SenseInfoBufferLength) {
            *SenseInfoBufferLength = 0;
        }
    }

    return cdb;
}


__inline
ULONG64
GetLbaFromCdb(
    _In_ PCDB  Cdb,
    _In_ ULONG CdbLength
    )
{
    LARGE_INTEGER lba;

    lba.QuadPart = 0;

    if (CdbLength == 0x10) {
        // 16 bytes CDB
        REVERSE_BYTES_QUAD((&lba.QuadPart), Cdb->CDB16.LogicalBlock);
    } else {
        lba.LowPart = (ULONG)(Cdb->CDB10.LogicalBlockByte3 |
                              Cdb->CDB10.LogicalBlockByte2 << 8 |
                              Cdb->CDB10.LogicalBlockByte1 << 16 |
                              Cdb->CDB10.LogicalBlockByte0 << 24 );
        lba.HighPart = 0;
    }

    return (ULONG64)lba.QuadPart;
}

__inline
ULONG
GetSectorCountFromCdb(
    PCDB  Cdb,
    ULONG CdbLength
    )
{
    ULONG sectorCount = 0;

    if (CdbLength == 0x10) {
        // 16 byte cdb
        REVERSE_BYTES(&sectorCount, Cdb->CDB16.TransferLength);

    } else {
        sectorCount = (ULONG)(Cdb->CDB10.TransferBlocksMsb << 8 |
                              Cdb->CDB10.TransferBlocksLsb);
    }

    return sectorCount;
}

//
// Util routines
//
VOID
__inline
ByteSwap (
    _Inout_updates_bytes_(Length) PUCHAR Buffer,
    _In_                   ULONG Length
    )
/*++

Routine Description:

    Swap the bytes in the buffer.

Arguments:

    Buffer
    Length

Return Value:

    None.

--*/
{
    UCHAR temp;
    ULONG i;

    if (Length == 0) {
        return;
    }

    // swap the bytes
    for (i=0; i < Length-1; i+=2) {
        temp = Buffer[i];
        Buffer[i] = Buffer[i+1];
#pragma warning (suppress: 6386) // i is within bound
        Buffer[i+1] = temp;
    }

    return;
}

ULONG
__inline
RemoveTrailingBlanks (
    _Inout_updates_z_(Length) PUCHAR Buffer,
    _In_                   ULONG Length
    )
/*++

Routine Description:

    Remove trailing blanks from the buffer and null terminates it

Arguments:

    Buffer
    Length

Return Value:

    Returns the length of the string excluding the null character.

--*/
{
    LONG i;

    if (Length < 2) {
        return 0;
    }

    Buffer[Length-1] = '\0';

    // find the first non-space char from the end
    for (i=Length-2; i >= 0; i--) {

        if (Buffer[i] != ' ') {
            break;
        }
    }

    NT_ASSERT((i + 1) >= 0);

    Buffer[i+1] = '\0';

    return (i+1);
}

_Success_(return == STOR_STATUS_SUCCESS)
ULONG
__inline
AhciAllocateDmaBuffer (
    _In_ PVOID   AdapterExtension,
    _In_ ULONG   BufferLength,
    _Post_writable_byte_size_(BufferLength) PVOID* Buffer,
    _Out_ PSTOR_PHYSICAL_ADDRESS PhysicalAddress
    )
{
    ULONG            status;
    PHYSICAL_ADDRESS minPhysicalAddress;
    PHYSICAL_ADDRESS maxPhysicalAddress;
    PHYSICAL_ADDRESS boundaryPhysicalAddress;

    minPhysicalAddress.QuadPart = 0;
    maxPhysicalAddress.QuadPart = 0x7FFFFFFF;   // (2GB - 1)
    boundaryPhysicalAddress.QuadPart = 0;

    status = StorPortAllocateDmaMemory(AdapterExtension,
                                       BufferLength,
                                       minPhysicalAddress,
                                       maxPhysicalAddress,
                                       boundaryPhysicalAddress,
                                       MmCached,
                                       MM_ANY_NODE_OK,
                                       Buffer,
                                       PhysicalAddress);

    return status;
}

_Success_(return == STOR_STATUS_SUCCESS)
ULONG
__inline
AhciFreeDmaBuffer (
    _In_ PVOID      AdapterExtension,
    _In_ ULONG_PTR  BufferLength,
    _In_reads_bytes_(BufferLength) _Post_invalid_ PVOID Buffer,
    _In_opt_ STOR_PHYSICAL_ADDRESS PhysicalAddress
    )
{
    ULONG status;

    status = StorPortFreeDmaMemory(AdapterExtension,
                                   Buffer,
                                   BufferLength,
                                   MmCached,
                                   PhysicalAddress);

    return status;
}

__inline
BOOLEAN
DmaSafeAtapiCommand (
    _In_ UCHAR CdbCommand
    )
{
    ULONG i;
    BOOLEAN useDma;
    static const UCHAR dmaCommandList[] =
        {SCSIOP_READ6,
         SCSIOP_READ,
         SCSIOP_READ_CD,
         SCSIOP_READ_CD_MSF,
         SCSIOP_READ12,
         SCSIOP_WRITE6,
         SCSIOP_WRITE,
         SCSIOP_WRITE12,
         SCSIOP_WRITE_VERIFY,
         SCSIOP_WRITE_VERIFY12,
         0};

    useDma = FALSE;
    i = 0;

    while (dmaCommandList[i]) {
        if (CdbCommand == dmaCommandList[i]) {
            useDma = TRUE;
            break;
        }

        i++;
    }

    return useDma;
}

__inline
BOOLEAN
IsSupportedReadCdb (
    _In_ PCDB   Cdb
    )
{
    return ((Cdb->CDB10.OperationCode == SCSIOP_READ) || (Cdb->CDB10.OperationCode == SCSIOP_READ16));
}

__inline
BOOLEAN
IsSupportedWriteCdb (
    _In_ PCDB   Cdb
    )
{
    return ((Cdb->CDB10.OperationCode == SCSIOP_WRITE) || (Cdb->CDB10.OperationCode == SCSIOP_WRITE16));
}


struct AHCI_CHANNEL_EXTENSION;
typedef struct _AHCI_CHANNEL_EXTENSION
               AHCI_CHANNEL_EXTENSION,
               *PAHCI_CHANNEL_EXTENSION;

typedef struct _AHCI_ADAPTER_EXTENSION  AHCI_ADAPTER_EXTENSION, *PAHCI_ADAPTER_EXTENSION;

ULONG
SCSItoATA(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SrbConvertToATAPICommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtapiCommonRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtapiInquiryRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtapiModeSenseRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtapiModeSelectRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
SrbConvertToATACommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
AtaReadWriteRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
AtaVerifyRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
AtaModeSenseRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaModeSelectRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaReadCapacityRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    );

ULONG
InquiryComplete(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaInquiryRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaStartStopUnitRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaTestUnitReadyRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaMediumRemovalRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaFlushCommandRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaPassThroughRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaUnmapRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
AtaSecurityProtocolRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaWriteBufferRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaReportLunsCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PVOID Context
    );

ULONG
AtaGetPhysicalElementStatusRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaRemoveElementAndTruncateRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaGetDeviceCurrentInternalStatusData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

ULONG
AtaGetDeviceCurrentInternalStatusDataHeader(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    );

UCHAR
AtaMapError(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN FUAcommand
    );

VOID
AhciSetSenseData (
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_ UCHAR                  SrbStatus,
    _In_ UCHAR                  SenseKey,
    _In_ UCHAR                  ASC,
    _In_ UCHAR                  ASCQ
);

VOID
UpdateDeviceParameters(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

VOID
DeviceInitAtapiIds(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PINQUIRYDATA InquiryData
    );

VOID
AhciPortIdentifyDevice(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
  );

ULONG
IOCTLtoATA(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

BOOLEAN
AdapterProcessIOCTL(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SmartVersion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

BOOLEAN
  FillClippedSGL(
    _In_    PSTOR_SCATTER_GATHER_LIST SourceSgl,
    _Inout_ PSTOR_SCATTER_GATHER_LIST LocalSgl,
    _In_    ULONG BytesLeft,
    _In_    ULONG BytesNeeded
    );

ULONG
SmartIdentifyData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SmartGeneric(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
NVCacheGeneric(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
HybridIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
DsmGeneralIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
DatasetManagementIoctl(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
FirmwareIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );


//
// AHCI Telemetry event related.
//
#define AHCI_TELEMETRY_EVENT_VERSION    0x1
#define AHCI_TELEMETRY_DRIVER_VERSION   0x1

#define AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING 0x1

typedef enum _AHCI_TELEMETRY_EVENT_ID {
    AhciTelemetryEventIdGeneral = 0,
    AhciTelemetryEventIdPortReset = 1,
    AhciTelemetryEventIdPortRunningStartFail = 2,
    AhciTelemetryEventIdPortErrorRecovery = 3,
    AhciTelemetryEventIdNonqueuedErrorRecovery = 4,
    AhciTelemetryEventIdNCQErrorRecovery = 5,
    AhciTelemetryEventIdNCQErrorRecoveryComplete = 6,
    AhciTelemetryEventIdResetBus = 7,
    AhciTelemetryEventIdResetDeviceRequest = 8,
    AhciTelemetryEventIdSurpriseRemove = 9,
    AhciTelemetryEventIdLpmAdaptiveSetting = 10,
    AhciTelemetryEventIdLpmSettingsModes = 11,
    AhciTelemetryEventIdPortStartSuccess = 12,
    AhciTelemetryEventIdReservedSlotStuck = 13,
    AhciTelemetryEventIdMax = 256
} AHCI_TELEMETRY_EVENT_ID, *PAHCI_TELEMETRY_EVENT_ID;

//
// AHCI mark device failure related.
//
#define AHCI_BUS_CHANGE_WARNING_THROTTLE_MASK           (0x1 << 0)
#define AHCI_BUS_CHANGE_COUNT_WARNING_THRESHOLD         (20)

#define AHCI_NCQ_ERROR_WARNING_THROTTLE_MASK            (0x1 << 1)
#define AHCI_NCQ_ERROR_COUNT_WARNING_THRESHOLD          (100)

#define AHCI_NON_QUEUED_ERROR_WARNING_THROTTLE_MASK     (0x1 << 2)
#define AHCI_NON_QUEUED_ERROR_COUNT_WARNING_THRESHOLD   (100)

#define AHCI_DEVICE_STUCK_WARNING_THROTTLE_MASK         (0x1 << 3)

typedef enum _AHCI_DEVICE_FAILURE_REASON {
    AhciDeviceFailureUnspecific = 0,
    AhciDeviceFailureTooManyBusChange = 1,
    AhciDeviceFailureTooManyNCQError = 2,
    AhciDeviceFailureTooManyNonQueuedError = 3,
    AhciDeviceFailureDeviceStuck = 4
} AHCI_DEVICE_FAILURE_REASON, *PAHCI_DEVICE_FAILURE_REASON;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif

/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    common.c

Abstract:

    This file contains common ATA related functions.


Notes:

Revision History:

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union
#pragma warning(disable:26015) //26015: "Potential overflow"


#include "generic.h"

//
// This structure is used as a wrapper of a MODE_PARAMETER_HEADER
// or MODE_PARAMETER_HEADER10. Code that uses this structure can be
// agnostic as to the underlying structure.
//
typedef struct _MODE_PARAMETER_HEADER_WRAPPER {
    //
    // Pointer to the actual MODE_PARAMETER_HEADER/10.
    //
    PVOID ModePageHeader;
    //
    // The size in bytes of the structure pointed to by ModePageHeader.
    //
    ULONG HeaderSize;
    //
    // A pointer to the DeviceSpecificParameter field of ModePageHeader.
    //
    PUCHAR DeviceSpecificParameter;
    //
    // A pointer to the ModeDataLength field of ModePageHeader. If ModePageHeader
    // is a pointer to MODE_PARAMETER_HEADER10, then this field points to the least
    // significant byte of ModePageHeader's ModeDataLength field.
    //
    PUCHAR ModeDataLength;
} MODE_PARAMETER_HEADER_WRAPPER, *PMODE_PARAMETER_HEADER_WRAPPER;

VOID
ULong2HexString (
    _In_ ULONG  Number,
    _Inout_updates_bytes_ (Length) PUCHAR StringBuffer,
    _In_ ULONG Length
    );

VOID
BuildHybridEvictCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS,
    _In_ USHORT                 BlockCount
    );

ULONG
QueryProtocolInfoIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
QueryTemperatureInfoIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
QueryPhysicalTopologyIoctlProcess(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    );

ULONG
SCSItoATA(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*
    Note: If there is a need to send a command to device,
          the translation routine shall set appropriate value to srbExtension->AtaFunction.
            For example: srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
          Not setting this field can cause the Srb being completed earlier than expected.
*/
{
    ULONG   status;
    ULONG   cdbLength = 0;
    PCDB    cdb = RequestGetSrbScsiData(Srb, &cdbLength, NULL, NULL, NULL);

    if (cdb != NULL) {
        if (IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
            // Atapi command
            status = SrbConvertToATAPICommand(ChannelExtension, Srb, cdb);
        } else if (IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters) ||
                   (cdb->CDB10.OperationCode == SCSIOP_REPORT_LUNS) ||
                   (cdb->CDB10.OperationCode == SCSIOP_INQUIRY) ) {
            // Ata command, or device enumeration commands.
            status = SrbConvertToATACommand(ChannelExtension, Srb, cdb, cdbLength);
        } else {
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            status = STOR_STATUS_INVALID_DEVICE_REQUEST;
        }
    } else {
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;
    }

    return status;
}

ULONG
SrbConvertToATAPICommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*
    Note: If there is a need to send a command to device,
          the translation routine shall set appropriate value to srbExtension->AtaFunction.
            For example: srbExtension->AtaFunction = ATA_FUNCTION_ATAPI_COMMAND;
          Not setting this field can cause the Srb being completed earlier than expected.
*/
{
    ULONG status;

    switch (Cdb->CDB10.OperationCode) {
    case SCSIOP_MODE_SENSE:

        if (IsTapeDevice(ChannelExtension->DeviceExtension)) {
            // tape drive specific.
            status = AtapiCommonRequest(ChannelExtension, Srb, Cdb);
        } else {
            // special process for 6 bytes command, storahci always send down 10 bytes command to device
            status = AtapiModeSenseRequest(ChannelExtension, Srb, Cdb);
        }
        break;

    case SCSIOP_MODE_SELECT:

        if (IsTapeDevice(ChannelExtension->DeviceExtension)) {
            // tape drive specific.
            status = AtapiCommonRequest(ChannelExtension, Srb, Cdb);
        } else {
            // special process for 6 bytes command, storahci always send down 10 bytes command to device
            status = AtapiModeSelectRequest(ChannelExtension, Srb, Cdb);
        }
        break;

    case SCSIOP_INQUIRY:

        status = AtapiInquiryRequest(ChannelExtension, Srb);
        break;

    case SCSIOP_REPORT_LUNS:
        //use cached info for this command.
        status = AtaReportLunsCommand(ChannelExtension, (PVOID)Srb);
        break;

    case SCSIOP_ATA_PASSTHROUGH16:

        status = AtaPassThroughRequest(ChannelExtension, Srb, Cdb);
        break;

    default:

        status = AtapiCommonRequest(ChannelExtension, Srb, Cdb);
        break;
    }

    return status;
}


ULONG
AtapiCommonRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*++

Routine Description:

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    ULONG               srbFlags = SrbGetSrbFlags(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    srbExtension->AtaFunction = ATA_FUNCTION_ATAPI_COMMAND;

    if (srbFlags & SRB_FLAGS_DATA_IN) {
        srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    }
    if (srbFlags & SRB_FLAGS_DATA_OUT) {
        srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    }

    // set the transfer mode to be used
    if ( DmaSafeAtapiCommand(Cdb->CDB10.OperationCode) ) {
        srbExtension->Flags |= ATA_FLAGS_USE_DMA;
    }

    return STOR_STATUS_SUCCESS;
}

VOID
AtapiInquiryCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    STOR_UNIT_ATTRIBUTES attributes = {0};
    UCHAR   pathId = 0;
    UCHAR   targetId = 0;
    UCHAR   lun = 0;

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        return;
    }

    SrbGetPathTargetLun(Srb, &pathId, &targetId, &lun);

    StorPortSetDeviceQueueDepth(ChannelExtension->AdapterExtension,
                                pathId,
                                targetId,
                                lun,
                                ChannelExtension->MaxPortQueueDepth);

    if (IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        attributes.DeviceAttentionSupported = ChannelExtension->DeviceExtension[0].IdentifyPacketData->SerialAtaCapabilities.SlimlineDeviceAttention;
        attributes.AsyncNotificationSupported = IsDeviceSupportsAN(ChannelExtension->DeviceExtension->IdentifyPacketData);
        attributes.D3ColdNotSupported = (ChannelExtension->AdapterExtension->StateFlags.SupportsAcpiDSM == 0);

        StorPortSetUnitAttributes(ChannelExtension->AdapterExtension,
                                  (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                  attributes);
    }

    return;
}

ULONG
AtapiInquiryRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++

Routine Description:

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    srbExtension->AtaFunction = ATA_FUNCTION_ATAPI_COMMAND;
    srbExtension->CompletionRoutine = AtapiInquiryCompletion;
    srbExtension->Flags |= ATA_FLAGS_DATA_IN;

    return STOR_STATUS_SUCCESS;
}

VOID
AtapiModeCommandRequestCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++

Description

    Used as completion routine for: AtapiModeSenseRequest and AtapiModeSelectRequest

Arguments:

Return value:

--*/
{
    PVOID               dataBuffer = SrbGetDataBuffer(Srb);
    UCHAR               bytesAdjust = sizeof(MODE_PARAMETER_HEADER10) - sizeof(MODE_PARAMETER_HEADER);
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if (Srb->SrbStatus == SRB_STATUS_BUS_RESET) {
        if (srbExtension->DataBuffer != NULL) {
            AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, (ULONG_PTR)srbExtension->CompletionContext, srbExtension->DataBuffer);
            srbExtension->DataBuffer = NULL;
        }
        return;
    }

    if ((dataBuffer == NULL) || (srbExtension->DataBuffer == NULL) ||
        (SrbGetDataTransferLength(Srb) < sizeof(MODE_PARAMETER_HEADER)) ||
        (srbExtension->DataTransferLength < sizeof(MODE_PARAMETER_HEADER)) ) {
        // free the memory and mark the Srb with error status.
        if (srbExtension->DataBuffer != NULL) {
            AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, (ULONG_PTR)srbExtension->CompletionContext, srbExtension->DataBuffer);
            srbExtension->DataBuffer = NULL;
        }

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return;
    }

    if (srbExtension->Cdb.CDB10.OperationCode == SCSIOP_MODE_SENSE10) {
        // if it's completion routine for MODE SENSE, copy the data back to original buffer
        PUCHAR originalBuffer;
        PUCHAR currentBuffer;
        PMODE_PARAMETER_HEADER header;
        PMODE_PARAMETER_HEADER10 header10;

        ULONG   transferLength = RequestGetDataTransferLength(Srb);

        originalBuffer = (PUCHAR)dataBuffer;
        currentBuffer = (PUCHAR)srbExtension->DataBuffer;

        NT_ASSERT(originalBuffer != NULL);
        NT_ASSERT(currentBuffer != NULL);

        header = (PMODE_PARAMETER_HEADER)originalBuffer;
        header10 = (PMODE_PARAMETER_HEADER10)currentBuffer;

        // Mode parameter header 10 and mode parameter header 6 differ by 3 bytes
        header->ModeDataLength = header10->ModeDataLength[1] - 3;
        header->MediumType = header10->MediumType;

        // ATAPI Mode Parameter Header doesn't have these fields.
        header->DeviceSpecificParameter = header10->DeviceSpecificParameter;

        header->BlockDescriptorLength = header10->BlockDescriptorLength[1];

        // copy the rest of the data
        if (transferLength > sizeof(MODE_PARAMETER_HEADER10)) {
            StorPortCopyMemory(originalBuffer+sizeof(MODE_PARAMETER_HEADER),
                               currentBuffer+sizeof(MODE_PARAMETER_HEADER10),
                               min(SrbGetDataTransferLength(Srb) - sizeof(MODE_PARAMETER_HEADER), transferLength - sizeof(MODE_PARAMETER_HEADER10)));
        }

        // adjust data transfer length.
        if (transferLength > bytesAdjust) {
            SrbSetDataTransferLength(Srb, transferLength - bytesAdjust);
        } else {
            // error. transfer length should be zero.
            // if it is less than the header, we will just pass it up.
            SrbSetDataTransferLength(Srb, transferLength);
        }
    }

    if (srbExtension->DataBuffer != NULL) {
        AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, (ULONG_PTR)srbExtension->CompletionContext, srbExtension->DataBuffer);
        srbExtension->DataBuffer = NULL;
    }

    return;
}

ULONG
AtapiModeSenseRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*++
--*/
{
    ULONG       status;
    ULONG       modeSenseBufferSize;
    ULONG       tempLength;
    PMODE_PARAMETER_HEADER10    modeSenseBuffer;
    STOR_PHYSICAL_ADDRESS       modeSensePhysialAddress;
    PUCHAR      origBuffer;
    UCHAR       bytesAdjust;

    PCDB                     cdb;
    USHORT                   allocationLength;
    PMODE_PARAMETER_HEADER   header;
    PMODE_PARAMETER_HEADER10 header10;

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    PVOID               srbDataBuffer = SrbGetDataBuffer(Srb);
    ULONG               srbDataBufferLength = SrbGetDataTransferLength(Srb);

    status = STOR_STATUS_SUCCESS;
    modeSenseBuffer = NULL;
    modeSenseBufferSize = 0;

    bytesAdjust = sizeof(MODE_PARAMETER_HEADER10) - sizeof(MODE_PARAMETER_HEADER);

    // Validate input length
    if (srbDataBufferLength < sizeof(MODE_PARAMETER_HEADER)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    if (srbDataBuffer == NULL) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto Done;
    }

    modeSenseBufferSize = srbDataBufferLength + bytesAdjust;
    origBuffer = (PUCHAR)srbDataBuffer;

    if (modeSenseBufferSize < sizeof(MODE_PARAMETER_HEADER10) ) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    // We need to allocate a new data buffer since the size is different
    status = AhciAllocateDmaBuffer(ChannelExtension->AdapterExtension, modeSenseBufferSize, (PVOID*)&modeSenseBuffer);
    if ( (status != STOR_STATUS_SUCCESS) ||
         (modeSenseBuffer == NULL) ) {
        // memory allocation failed
        Srb->SrbStatus = SRB_STATUS_ERROR;
        status = STOR_STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    AhciZeroMemory((PCHAR)modeSenseBuffer, modeSenseBufferSize);
    modeSensePhysialAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, (PVOID)modeSenseBuffer, &tempLength);

    // fill information in the new MODE_SENSE10 header
    allocationLength = Cdb->MODE_SENSE.AllocationLength;

    header = (PMODE_PARAMETER_HEADER)origBuffer;
    header10 = (PMODE_PARAMETER_HEADER10)modeSenseBuffer;

    header10->ModeDataLength[1] = header->ModeDataLength;
    header10->MediumType = header->MediumType;
    header10->BlockDescriptorLength[1] = header->BlockDescriptorLength;

    allocationLength += bytesAdjust;

    // set up cdb of MODE_SENSE10
    cdb = (PCDB)&srbExtension->Cdb;

    cdb->MODE_SENSE10.OperationCode          = SCSIOP_MODE_SENSE10;
    cdb->MODE_SENSE10.Dbd                    = Cdb->MODE_SENSE.Dbd;
    cdb->MODE_SENSE10.PageCode               = Cdb->MODE_SENSE.PageCode;
    cdb->MODE_SENSE10.Pc                     = Cdb->MODE_SENSE.Pc;
    cdb->MODE_SENSE10.AllocationLength[0]    = (UCHAR) (allocationLength >> 8);
    cdb->MODE_SENSE10.AllocationLength[1]    = (UCHAR) (allocationLength & 0xff);
    cdb->MODE_SENSE10.Control                = Cdb->MODE_SENSE.Control;

    // fill in the srbExtension fields
    srbExtension->AtaFunction = ATA_FUNCTION_ATAPI_COMMAND;
    srbExtension->Flags |= ATA_FLAGS_NEW_CDB;
    srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    srbExtension->DataBuffer = modeSenseBuffer;
    srbExtension->DataTransferLength = modeSenseBufferSize;
    srbExtension->CompletionRoutine = AtapiModeCommandRequestCompletion;
    srbExtension->CompletionContext = (PVOID)modeSenseBufferSize;    // preserve the buffer size, it's needed for freeing the memory

    srbExtension->LocalSgl.NumberOfElements = 1;
    srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = modeSensePhysialAddress.LowPart;
    srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = modeSensePhysialAddress.HighPart;
    srbExtension->LocalSgl.List[0].Length = modeSenseBufferSize;
    srbExtension->Sgl = &srbExtension->LocalSgl;

Done:

    if (status != STOR_STATUS_SUCCESS) {
        if (modeSenseBuffer != NULL) {
            AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, modeSenseBufferSize, modeSenseBuffer);
        }
        srbExtension->DataBuffer = NULL;
    }

    return status;
}


ULONG
AtapiModeSelectRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*++
    return value: STOR_STATUS
--*/
{
    ULONG   status;
    PVOID   modeSelectBuffer;
    ULONG   modeSelectBufferSize;
    ULONG   tempLength;
    STOR_PHYSICAL_ADDRESS   modeSelectPhysialAddress;
    UCHAR   bytesToSkip;
    PUCHAR  origBuffer;
    UCHAR   bytesAdjust;

    PCDB                     cdb;
    USHORT                   paramListLength;
    PMODE_PARAMETER_HEADER   header;
    PMODE_PARAMETER_HEADER10 header10;

    PAHCI_SRB_EXTENSION      srbExtension = GetSrbExtension(Srb);
    PVOID                    srbDataBuffer = SrbGetDataBuffer(Srb);
    ULONG                    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    status = STOR_STATUS_SUCCESS;
    modeSelectBuffer = NULL;
    modeSelectBufferSize = 0;

    origBuffer = (PUCHAR)srbDataBuffer;
    header = (PMODE_PARAMETER_HEADER)origBuffer;

    if (srbDataBuffer == NULL) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto Done;
    }

    // Validate input length
    if ( (srbDataBufferLength < sizeof(MODE_PARAMETER_HEADER)) ||
         (srbDataBufferLength < (sizeof(MODE_PARAMETER_HEADER) + header->BlockDescriptorLength)) ) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_BUFFER_TOO_SMALL;
        goto Done;
    }

    // Check for invalid BlockDescriptorLength
    if ( (sizeof(MODE_PARAMETER_HEADER) + header->BlockDescriptorLength) > 0xFF ) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto Done;
    }

    // Atapi devices don't use the block descriptor.
    bytesAdjust = sizeof(MODE_PARAMETER_HEADER10) - sizeof(MODE_PARAMETER_HEADER);
    modeSelectBufferSize = srbDataBufferLength + bytesAdjust - header->BlockDescriptorLength;

    // allocate buffer for the new cdb
    status = AhciAllocateDmaBuffer(ChannelExtension->AdapterExtension, modeSelectBufferSize, (PVOID*)&modeSelectBuffer);

    if ( (status != STOR_STATUS_SUCCESS) ||
         (modeSelectBuffer == NULL) ) {
        //
        Srb->SrbStatus = SRB_STATUS_ERROR;
        status = STOR_STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    AhciZeroMemory((PCHAR)modeSelectBuffer, modeSelectBufferSize);
    modeSelectPhysialAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, (PVOID)modeSelectBuffer, &tempLength);

    header10 = (PMODE_PARAMETER_HEADER10)modeSelectBuffer;

    header10->ModeDataLength[1] = header->ModeDataLength;
    header10->MediumType = header->MediumType;

    // block descriptor length in header10 should be 0 for ATAPI devices
    // do not copy the block descriptor. Atapi devices don't use the block descriptor.
    bytesToSkip = sizeof(MODE_PARAMETER_HEADER) + header->BlockDescriptorLength;

    // Copy any remaining buffer contents
    if (srbDataBufferLength > bytesToSkip) {

        StorPortCopyMemory( ((PUCHAR)modeSelectBuffer + sizeof(MODE_PARAMETER_HEADER10)),
                            ((PUCHAR)origBuffer + bytesToSkip),
                            (srbDataBufferLength - bytesToSkip) );
    }

    paramListLength = Cdb->MODE_SELECT.ParameterListLength;
    paramListLength += sizeof(MODE_PARAMETER_HEADER10);

    // Check for integer overflow
    if (paramListLength < sizeof(MODE_PARAMETER_HEADER10)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto Done;
    }

    // Invalid paramListLength in the original cdb
    if (paramListLength < bytesToSkip) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto Done;
    }

    paramListLength -= bytesToSkip;

    // fill in the srbExtension fields
    srbExtension->AtaFunction = ATA_FUNCTION_ATAPI_COMMAND;
    srbExtension->Flags |= ATA_FLAGS_NEW_CDB;
    srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    srbExtension->DataBuffer = modeSelectBuffer;
    srbExtension->DataTransferLength = modeSelectBufferSize;
    srbExtension->CompletionRoutine = AtapiModeCommandRequestCompletion;
    srbExtension->CompletionContext = (PVOID)modeSelectBufferSize;    // preserve the buffer size, it's needed for freeing the memory

    cdb = (PCDB)&srbExtension->Cdb;

    // fill in the cdb
    cdb->MODE_SELECT10.OperationCode          = SCSIOP_MODE_SELECT10;  // ATAPI device supports MODE SELECT 10
    cdb->MODE_SELECT10.SPBit                  = Cdb->MODE_SELECT.SPBit;
    cdb->MODE_SELECT10.PFBit                  = 1;
    cdb->MODE_SELECT10.LogicalUnitNumber      = Cdb->MODE_SELECT.LogicalUnitNumber;
    cdb->MODE_SELECT10.ParameterListLength[0] = (UCHAR)(paramListLength >> 8);
    cdb->MODE_SELECT10.ParameterListLength[1] = (UCHAR)(paramListLength & 0xff);
    cdb->MODE_SELECT10.Control                = Cdb->MODE_SELECT.Control;

    srbExtension->LocalSgl.NumberOfElements = 1;
    srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = modeSelectPhysialAddress.LowPart;
    srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = modeSelectPhysialAddress.HighPart;
    srbExtension->LocalSgl.List[0].Length = modeSelectBufferSize;    // preserve the buffer size, it's needed for freeing the memory
    srbExtension->Sgl = &srbExtension->LocalSgl;

Done:

    if (status != STOR_STATUS_SUCCESS) {
        if (modeSelectBuffer != NULL) {
            AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, modeSelectBufferSize, modeSelectBuffer);
        }
        srbExtension->DataBuffer = NULL;
    }

    return status;
}


ULONG
SrbConvertToATACommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    )
/*
    Srb to a ATA device must be translated to an ATA command

    Note: If there is a need to send a command to device,
          the translation routine shall set appropriate value to srbExtension->AtaFunction.
            For example: srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
          Not setting this field can cause the Srb being completed earlier than expected.
*/
{
    ULONG status;

    // translate the cdb to task file register values
    switch (Cdb->CDB10.OperationCode) {
    case SCSIOP_READ:
    case SCSIOP_WRITE:
    case SCSIOP_READ16:
    case SCSIOP_WRITE16:

        status = AtaReadWriteRequest(ChannelExtension, Srb, Cdb, CdbLength);
        break;

    case SCSIOP_VERIFY:
    case SCSIOP_VERIFY16:

        status = AtaVerifyRequest(ChannelExtension, Srb, Cdb, CdbLength);
        break;

    case SCSIOP_MODE_SENSE:
    case SCSIOP_MODE_SENSE10:

        status = AtaModeSenseRequest(ChannelExtension, Srb, Cdb);

        break;

    case SCSIOP_MODE_SELECT:
    case SCSIOP_MODE_SELECT10:

        status = AtaModeSelectRequest(ChannelExtension, Srb, Cdb);

        break;

    case SCSIOP_READ_CAPACITY:
    case SCSIOP_READ_CAPACITY16:

        status = AtaReadCapacityRequest(ChannelExtension, Srb, Cdb, CdbLength);
        break;

    case SCSIOP_INQUIRY:

        status = AtaInquiryRequest(ChannelExtension, Srb, Cdb);
        break;

    case SCSIOP_REPORT_LUNS:
        status = AtaReportLunsCommand(ChannelExtension, (PVOID)Srb);
        break;

    case SCSIOP_START_STOP_UNIT:

        status = AtaStartStopUnitRequest(ChannelExtension, Srb, Cdb);
        break;

    case SCSIOP_TEST_UNIT_READY:

        status = AtaTestUnitReadyRequest(ChannelExtension, Srb);
        break;

    case SCSIOP_MEDIUM_REMOVAL:

        status = AtaMediumRemovalRequest(ChannelExtension, Srb, Cdb);
        break;

    case SCSIOP_SYNCHRONIZE_CACHE:

        status = AtaFlushCommandRequest(ChannelExtension, Srb);
        break;

    case SCSIOP_SECURITY_PROTOCOL_IN:
    case SCSIOP_SECURITY_PROTOCOL_OUT:
        // Use a common function as SECURITY_PROTOCOL_IN and SECURITY_PROTOCOL_OUT structures are same.
        status = AtaSecurityProtocolRequest(ChannelExtension, Srb, Cdb);
        break;

    case SCSIOP_ATA_PASSTHROUGH16:

        status = AtaPassThroughRequest(ChannelExtension, Srb, Cdb);
        break;

    case SCSIOP_UNMAP:
        status = AtaUnmapRequest(ChannelExtension, Srb);
        break;

    case SCSIOP_WRITE_DATA_BUFF:
        status = AtaWriteBufferRequest(ChannelExtension, Srb, Cdb);
        break;

    default:

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return status;
}


VOID
AtaSetTaskFileDataRange(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ ULONG64 StartingLba,
    _In_ ULONG NumberOfBlocks
    )
/*++

Routine Description:

    Sets up the following registers in the task file:
    sectorCount
    sectorNum
    CylLow
    CylHigh
    deviceHead


Arguments:

    StartingLba
    NumberOfBlocks

Return Value:

    None.

--*/
{
    LARGE_INTEGER startingSector;
    PATAREGISTERS previousReg;
    PATAREGISTERS currentReg;
    ULONG         sectorCount;

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    startingSector.QuadPart = StartingLba;
    sectorCount = NumberOfBlocks;

    NT_ASSERT(sectorCount != 0);
    NT_ASSERT(sectorCount <= 0x10000);

    previousReg = &srbExtension->TaskFile.Previous;
    currentReg  = &srbExtension->TaskFile.Current;

    if (Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        // load the higher order bytes - PreviousReg
        SetSectorCount(previousReg, (UCHAR)((sectorCount & 0x0000ff00) >> 8));

        SetSectorNumber(previousReg, (UCHAR)(((startingSector.LowPart) & 0xff000000) >> 24));
        SetCylinderLow(previousReg, (UCHAR)(((startingSector.HighPart) & 0x000000ff) >> 0));
        SetCylinderHigh(previousReg, (UCHAR)(((startingSector.HighPart) & 0x0000ff00) >> 8));

        // setup the device/head register.
        SetDeviceReg(currentReg, IDE_LBA_MODE);
    } else {
        // we cannot handle more than 256 sectors
        NT_ASSERT(sectorCount <= 0x100);

        // The starting LBA should be less than 28 bits wide
        NT_ASSERT(startingSector.HighPart == 0);

        // The upper layers could still send us sectors beyond the
        // end of the disk. Do not assert. Let the device fail it.
        //
        //NT_ASSERT((startingSector.LowPart & 0xF0000000) == 0);

        // setup the device/head register.
        SetDeviceReg(currentReg, (UCHAR)(IDE_LBA_MODE | (((startingSector.LowPart) & 0x0f000000) >> 24)));
    }

    // load the lower order bytes - CurrentReg
    SetSectorCount(currentReg, (UCHAR)((sectorCount & 0x000000ff) >> 0));

    SetSectorNumber(currentReg, (UCHAR)(((startingSector.LowPart) & 0x000000ff) >> 0));
    SetCylinderLow(currentReg, (UCHAR)(((startingSector.LowPart) & 0x0000ff00) >> 8));
    SetCylinderHigh(currentReg, (UCHAR)(((startingSector.LowPart) & 0x00ff0000) >> 16));

    return;
}


VOID
HybridWriteThroughEvictCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
)
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    //
    // Free DMA buffer that allocated for EVICT command.
    //
    AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, ATA_BLOCK_SIZE, srbExtension->DataBuffer);

    return;
}

VOID
HybridWriteThroughCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
)
/*++

Routine Description:

    WRITE command is completed; Issue EVICT command to evict data out of caching medium.

Arguments:

    ChannelExtension
    Srb

Return Value:

    None

--*/
{
    ULONG                   status;
    ULONG                   i;
    PATA_LBA_RANGE          buffer = NULL;     // DMA buffer allocated for EVICT command
    STOR_PHYSICAL_ADDRESS   bufferPhysicalAddress;
    PAHCI_SRB_EXTENSION     srbExtension;

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        //
        // WRITE command failed, return to complete the request.
        //
        return;
    }

    srbExtension = GetSrbExtension(Srb);

    //
    // allocate DMA buffer, this buffer will be used to store the ATA LBA Range for EVICT command
    //
    status = AhciAllocateDmaBuffer((PVOID)ChannelExtension->AdapterExtension, ATA_BLOCK_SIZE, (PVOID*)&buffer);

    if ( (status != STOR_STATUS_SUCCESS) || (buffer == NULL) ) {
        Srb->SrbStatus = SRB_STATUS_ERROR;
        return;
    }

    //
    // Fill LBA Range into buffer for EVICT command.
    //
    AhciZeroMemory((PCHAR)buffer, ATA_BLOCK_SIZE);

    buffer->StartSector = ((ULONGLONG)srbExtension->Cfis.LBA7_0) |
                          ((ULONGLONG)srbExtension->Cfis.LBA15_8 << 8) |
                          ((ULONGLONG)srbExtension->Cfis.LBA23_16 << 16) |
                          ((ULONGLONG)srbExtension->Cfis.LBA31_24 << 24) |
                          ((ULONGLONG)srbExtension->Cfis.LBA39_32 << 32) |
                          ((ULONGLONG)srbExtension->Cfis.LBA47_40 << 40);

    buffer->SectorCount = ((ULONGLONG)srbExtension->Cfis.Feature7_0) |
                          ((ULONGLONG)srbExtension->Cfis.Feature15_8 << 8)  ;

    //
    // save values before calling HybridEvictCompletion()
    //
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;
    srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    srbExtension->DataBuffer = buffer;
    srbExtension->DataTransferLength = ATA_BLOCK_SIZE;

    bufferPhysicalAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, buffer, &i);
    srbExtension->LocalSgl.NumberOfElements = 1;
    srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = bufferPhysicalAddress.LowPart;
    srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = bufferPhysicalAddress.HighPart;
    srbExtension->LocalSgl.List[0].Length = ATA_BLOCK_SIZE;
    srbExtension->Sgl = &srbExtension->LocalSgl;

    //
    // Clear the CFIS structure before reuse it.
    //
    AhciZeroMemory((PCHAR)&srbExtension->Cfis, sizeof(AHCI_H2D_REGISTER_FIS));

    BuildHybridEvictCommand(&srbExtension->Cfis, 1);

    srbExtension->CompletionRoutine = HybridWriteThroughEvictCompletion;

    return;
}

__inline
UCHAR
GetReadWriteRequestAtaCommand (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*++

Routine Description:


Arguments:

    ChannelExtension -
    Srb -
    Cdb -

Return Value:

    The translated ATA command.

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    UCHAR               commandReg = IDE_COMMAND_NOT_VALID;

    if (IsSupportedReadCdb(Cdb)) {
        if( ChannelExtension->StateFlags.NCQ_Activated ) {
            // use NCQ command as preferred command.
            commandReg = IDE_COMMAND_READ_FPDMA_QUEUED;
        } else {
            // SATA device, use DMA read command.
            if (Is48BitCommand(srbExtension->Flags)) {
                commandReg = IDE_COMMAND_READ_DMA_EXT;
            } else {
                commandReg = IDE_COMMAND_READ_DMA;
            }
        }
    } else {
        if( ChannelExtension->StateFlags.NCQ_Activated ) {
            // change the command to be NCQ command
            commandReg = IDE_COMMAND_WRITE_FPDMA_QUEUED;
        } else {
            // Make all writes into WRITE_DMA
            // If FUA, keep track of FUA even there is nothing to do with this bit later
            if (Is48BitCommand(srbExtension->Flags)) {
                if ( (Cdb->CDB10.ForceUnitAccess == 1) && IsFuaSupported(ChannelExtension) ) {
                    // DMA write ext FUA (48 bit)
                    commandReg = IDE_COMMAND_WRITE_DMA_FUA_EXT;
                } else {
                    // DMA write ext (48 bit)
                    commandReg = IDE_COMMAND_WRITE_DMA_EXT;
                }
            } else {
                // DMA write
                commandReg = IDE_COMMAND_WRITE_DMA;
            }
        }
    }

    return commandReg;
}

VOID
BuildReadWriteCommand (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    )
/*++

Routine Description:

    Sets up SATA CFIS data structure for read/write. The routine shall not
    modify any other field in the srb or device extension.

Arguments:

    ChannelExtension
    Srb
    Cdb
    CdbLength

Return Value:

    None.

Notes:

    All the other fields in the srb should be set before calling this routine.

--*/
{
    LARGE_INTEGER   startingSector;
    ULONG           bytesPerSector;
    ULONG           sectorCount;
    BOOLEAN         hybridPriorityPassedIn = FALSE;

    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);
    ULONG                   srbDataBufferLength = SrbGetDataTransferLength(Srb);
    PAHCI_H2D_REGISTER_FIS  cfis = &srbExtension->Cfis;

    bytesPerSector = BytesPerLogicalSector(&ChannelExtension->DeviceExtension->DeviceParameters);

    //
    // Calculate sector count. Round up to next block.
    //
    sectorCount = (srbDataBufferLength + bytesPerSector - 1) / bytesPerSector;

    //
    // Get starting sector number from CDB.
    //
    startingSector.QuadPart = GetLbaFromCdb(Cdb, CdbLength);

    NT_ASSERT(sectorCount != 0);
    NT_ASSERT(sectorCount <= 0x10000);  // transfer length should not over ATA physical transfer limitation.

    //
    // Command and AtaFunction should be set first. It will be referenced later in this function.
    //
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;

    cfis->Command = GetReadWriteRequestAtaCommand(ChannelExtension, Srb, Cdb);

    //
    // Set LBA bits 0 - 23 first.
    //
    cfis->LBA7_0 = (UCHAR)(((startingSector.LowPart) & 0x000000ff) >> 0);
    cfis->LBA15_8 = (UCHAR)(((startingSector.LowPart) & 0x0000ff00) >> 8);
    cfis->LBA23_16 = (UCHAR)(((startingSector.LowPart) & 0x00ff0000) >> 16);

    if (IsNCQCommand(srbExtension) || Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        //
        // Set LBA bits 24 - 47 for devices that support 48bits commands.
        //
        cfis->LBA31_24 = (UCHAR)(((startingSector.LowPart) & 0xff000000) >> 24);
        cfis->LBA39_32 = (UCHAR)(((startingSector.HighPart) & 0x000000ff) >> 0);
        cfis->LBA47_40 = (UCHAR)(((startingSector.HighPart) & 0x0000ff00) >> 8);
    } else {
        //
        // Set LBA bits 24 - 27 for devices that support 28bits commands only.
        //
        cfis->Device = (UCHAR)(((startingSector.LowPart) & 0x0f000000) >> 24);
    }

    //
    // Set Sector Counts and Device fields.
    //
    if (IsNCQCommand(srbExtension)) {
        cfis->Feature7_0 = (UCHAR)((sectorCount & 0x000000ff) >> 0);
        cfis->Feature15_8 = (UCHAR)((sectorCount & 0x0000ff00) >> 8);

        cfis->Device |= IDE_LBA_MODE;

        if( (Cdb->CDB10.ForceUnitAccess == 1) && IsFuaSupported(ChannelExtension) ){
            cfis->Device |= ATA_NCQ_FUA_BIT;
        } else {
            cfis->Device &= ~ATA_NCQ_FUA_BIT;
        }

    } else {
        cfis->Count7_0 = (UCHAR)((sectorCount & 0x000000ff) >> 0);

        if (Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters)) {
            cfis->Count15_8 = (UCHAR)((sectorCount & 0x0000ff00) >> 8);
        } else {
            // we cannot handle more than 256 sectors
            NT_ASSERT(sectorCount <= 0x100);

            // The starting LBA should be less than 28 bits wide
            NT_ASSERT(startingSector.HighPart == 0);

            // The upper layers could still send us sectors beyond the
            // end of the disk. Do not assert. Let the device fail it.
            //
            //NT_ASSERT((startingSector.LowPart & 0xF0000000) == 0);
        }

        cfis->Device |= (0xA0 | IDE_LBA_MODE);
    }

    //
    // Normal Stack, cache priority info can be passed with READ or WRITE request.
    // For Hiberfile, only pass cache priority info for WRITE request.
    //
    if ((!IsDumpMode(ChannelExtension->AdapterExtension) && IsDeviceHybridInfoEnabled(ChannelExtension) && IsNcqReadWriteCommand(srbExtension)) ||
        ((ChannelExtension->StateFlags.HybridInfoEnabledOnHiberFile == 1) && IsNCQWriteCommand(srbExtension))) {

        PSRBEX_DATA_IO_INFO srbExInfo = (PSRBEX_DATA_IO_INFO)SrbGetSrbExDataByType(Srb, SrbExDataTypeIoInfo);

        if ((srbExInfo != NULL) &&
            ((srbExInfo->Flags & REQUEST_INFO_VALID_CACHEPRIORITY_FLAG) != 0) &&
            (srbExInfo->CachePriority <= ChannelExtension->DeviceExtension->HybridInfo.MaximumHybridPriorityLevel)) {

            ATA_HYBRID_INFO_FIELDS hybridInfo = {0};

            hybridInfo.InfoValid = 1;
            hybridInfo.HybridPriority = srbExInfo->CachePriority;

            cfis->Auxiliary23_16 = hybridInfo.AsUchar;

            //
            // For WRITE request that requires HybridPassThrough, assign completion routine
            // to issue EVICT command for evicting data out of caching medium.
            //
            if (!IsDumpMode(ChannelExtension->AdapterExtension) && IsNCQWriteCommand(srbExtension) &&
                ((srbExInfo->Flags & REQUEST_INFO_HYBRID_WRITE_THROUGH_FLAG) != 0) &&
                (ChannelExtension->DeviceExtension->SupportedCommands.HybridEvict == 1)) {

                srbExtension->CompletionRoutine = HybridWriteThroughCompletion;
            }

            hybridPriorityPassedIn = TRUE;
        }
    }


    return;
}

ULONG
AtaReadWriteRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    ULONG               srbFlags = SrbGetSrbFlags(Srb);

    if (!IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters) ||
        (ChannelExtension->DeviceExtension->DeviceParameters.BytesPerLogicalSector == 0) ) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    //
    // A read write command must have this flag set.
    // The absence of the flag indicates a null MDL. We
    // could get such an SRB with a bad passthrough
    // request. Protect against it. (This might look like
    // a hack, but it is not. We should verify our assumptions
    // before we do a translation)
    //
    if (!(srbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    // set the transfer mode to be used
    srbExtension->Flags |= ATA_FLAGS_USE_DMA;

    if (Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        srbExtension->Flags |= ATA_FLAGS_48BIT_COMMAND;
    }

    if (IsSupportedReadCdb(Cdb)) {
        srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    } else if (IsSupportedWriteCdb(Cdb)) {
        srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    } else {
        //
        // other READ/WRITE commands are not supported by StorAHCI.
        //
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    BuildReadWriteCommand(ChannelExtension, Srb, Cdb, CdbLength);

    return STOR_STATUS_SUCCESS;
}

ULONG
AtaVerifyRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    )
/*++

Routine Description:

    This routine handles IDE Verify.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet
    Cdb - SCSI command carried by Srb
    CdbLength - length of the SCSI command

Return Value:

    SRB status

--*/

{
    LARGE_INTEGER   startingSector;
    ULONG           sectorCount;
    UCHAR           commandReg;

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    // Get starting sector number from CDB.
    startingSector.QuadPart = GetLbaFromCdb(Cdb, CdbLength);

    // get the sector count from the cdb
    sectorCount = GetSectorCountFromCdb(Cdb, CdbLength);

    // Ensure that the command is small enough for us to handle
    if (sectorCount > 0x10000) {
        // Sector count cannot be longer than 2 bytes (2 ^ 16).
        // Note that a 0 sector count => 0x10000 sectors
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    if (!Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters) && (sectorCount > 0x100)) {
        // Without 48bit support the sector count cannot be greater than 256 sectors
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

    if (Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        srbExtension->Flags |= ATA_FLAGS_48BIT_COMMAND;
    }

    AtaSetTaskFileDataRange(ChannelExtension,
                            Srb,
                            startingSector.QuadPart,
                            sectorCount
                            );

    // verify command. no data transfer
    if (Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        commandReg = IDE_COMMAND_VERIFY_EXT;
    } else {
        commandReg = IDE_COMMAND_VERIFY;
    }

    SetCommandReg((&srbExtension->TaskFile.Current), commandReg);

    return STOR_STATUS_SUCCESS;

}

VOID
AtaModeSenseRequestCompletionMediaStatus (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PAHCI_SRB_EXTENSION    srbExtension = GetSrbExtension(Srb);
    PMODE_PARAMETER_HEADER modePageHeader = (PMODE_PARAMETER_HEADER)SrbGetDataBuffer(Srb);
    ULONG                  transferLength = SrbGetDataTransferLength(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    if (Srb->SrbStatus == SRB_STATUS_BUS_RESET) {
        return;
    }

    if ( transferLength < sizeof(PMODE_PARAMETER_HEADER) ) {
        NT_ASSERT(FALSE);
        return;
    }
    // update the mode page header
    NT_ASSERT(modePageHeader != NULL);

    if ( (srbExtension->AtaStatus & IDE_STATUS_ERROR) &&
         (srbExtension->AtaError & IDE_ERROR_DATA_ERROR) ) {
        modePageHeader->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
    }

    return;
}

VOID
AtaModeSenseRequestCompletionWriteCache (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PMODE_CACHING_PAGE      cachePage;
    ULONG                   cdbLength = 0;
    PCDB                    cdb = RequestGetSrbScsiData(Srb, &cdbLength, NULL, NULL, NULL);
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);
    PVOID  modePageHeader = SrbGetDataBuffer(Srb);
    PIDENTIFY_DEVICE_DATA   identifyDeviceData = (PIDENTIFY_DEVICE_DATA)srbExtension->DataBuffer;
    ULONG                   srbDataBufferLength = SrbGetDataTransferLength(Srb);
    ULONG modePageHeaderSize;

    //Initialize variables
    NT_ASSERT(identifyDeviceData != NULL);

    if (cdb == NULL) {
        NT_ASSERT(cdb != NULL);
        return;
    }

    if (cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE) {
        modePageHeaderSize = sizeof(MODE_PARAMETER_HEADER);
    } else {
        NT_ASSERT(cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE10);
        modePageHeaderSize = sizeof(MODE_PARAMETER_HEADER10);
    }

    cachePage = (PMODE_CACHING_PAGE)(((PCHAR)modePageHeader) + modePageHeaderSize);

    if (Srb->SrbStatus == SRB_STATUS_BUS_RESET) {
        return;
    }

    // update the mode page header
    if ( (modePageHeader == NULL) ||
         (srbDataBufferLength < ( modePageHeaderSize + sizeof(PMODE_CACHING_PAGE) ) ) ) {
        NT_ASSERT(modePageHeader != NULL);
        Srb->SrbStatus = SRB_STATUS_ERROR;
        return;
    }

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        cachePage->WriteCacheEnable = (UCHAR)identifyDeviceData->CommandSetActive.WriteCache;
    } else {
        Srb->SrbStatus = SRB_STATUS_ERROR;
    }

    // update the number of bytes we are returning.  If this function executes, the maximum size is being used.
    SrbSetDataTransferLength(Srb, srbDataBufferLength - modePageHeaderSize - sizeof(MODE_CACHING_PAGE));

    if (IsRemovableMedia(&ChannelExtension->DeviceExtension->DeviceParameters) &&
        IsMsnEnabled(ChannelExtension->DeviceExtension)) {
        // prepare to send no data command. reuse srbExtension area, clear it first
        AhciZeroMemory((PCHAR)srbExtension, sizeof(AHCI_SRB_EXTENSION));

        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
        SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_GET_MEDIA_STATUS);

        return;
    }

    return;
}

ULONG
AtaInitModePageHeaderWrapper (
    _In_ PCDB                                   Cdb,
    _In_reads_bytes_(DataBufferLength) PVOID    ModePageHeader,
    _In_ ULONG                                  DataBufferLength,
    _Out_ PMODE_PARAMETER_HEADER_WRAPPER        ModePageHeaderWrapper
)
/*++

Routine Description:

    This routine initializes a MODE_PARAMETER_HEADER_WRAPPER taking into account
    the command in Cdb (SCSI_OP_MODE_SENSE or SCSI_OP_MODE_SENSE10).

Arguments:

    Cdb - Cdb of a MODE SENSE SCSI command.
    ModePageHeader - A pointer to either a MODE_PARAMETER_HEADER or MODE_PARAMETER_HEADER_10 structure.
    ULONG DataBufferLength - The size of the buffer that contains ModePageHeader.
    ModePageHeaderWrapper - Pointer to a MODE_PARAMETER_HEADER_WRAPPER that will be filled by this routine.

Return Value:

    STOR_STATUS_SUCCESS if the routine is successful, or STOR_STATUS_INSUFFICIENT_RESOURCES if the
    ModePageHeader buffer isn't big enough.

--*/
{
    NT_ASSERT(ModePageHeader != NULL);

    AhciZeroMemory((PCHAR)ModePageHeader, DataBufferLength);

    ModePageHeaderWrapper->ModePageHeader = ModePageHeader;
    if (Cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE) {
        PMODE_PARAMETER_HEADER modePageHeader6 = (PMODE_PARAMETER_HEADER)ModePageHeader;
        ModePageHeaderWrapper->HeaderSize = sizeof(*modePageHeader6);

        if (DataBufferLength < ModePageHeaderWrapper->HeaderSize) {
            return STOR_STATUS_INVALID_PARAMETER;
        }

        ModePageHeaderWrapper->DeviceSpecificParameter = &modePageHeader6->DeviceSpecificParameter;
        ModePageHeaderWrapper->ModeDataLength = &modePageHeader6->ModeDataLength;

        *(ModePageHeaderWrapper->ModeDataLength) = sizeof(MODE_PARAMETER_HEADER) - FIELD_OFFSET(MODE_PARAMETER_HEADER, MediumType);

    } else {
        USHORT modeDataLength;
        PMODE_PARAMETER_HEADER10 modePageHeader10 = (PMODE_PARAMETER_HEADER10)ModePageHeader;

        NT_ASSERT(Cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE10);

        ModePageHeaderWrapper->HeaderSize = sizeof(*modePageHeader10);
        if (DataBufferLength < ModePageHeaderWrapper->HeaderSize) {
            return STOR_STATUS_INVALID_PARAMETER;
        }

        ModePageHeaderWrapper->DeviceSpecificParameter = &modePageHeader10->DeviceSpecificParameter;

        //
        // The ModeDataLength field of a MODE SENSE 10 command is 2 bytes long, but we only
        // expose one byte in the wrapper structure. This restriction allows code that uses
        // the wrapper to work independently of the underlying header structure, since a MODE SENSE 6
        // command only has 1 byte for ModeDataLength.
        //

        // The 2nd byte of ModeDataLength is the least significant.
        ModePageHeaderWrapper->ModeDataLength = &modePageHeader10->ModeDataLength[1];

        modeDataLength = sizeof(MODE_PARAMETER_HEADER10) - FIELD_OFFSET(MODE_PARAMETER_HEADER10, MediumType);
        NT_ASSERT(modeDataLength <= 0xFF);
        *(ModePageHeaderWrapper->ModeDataLength) = (UCHAR)modeDataLength;
    }

    return STOR_STATUS_SUCCESS;
}

ULONG
AtaModeSenseRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*
  NOTE: only Cache Page is supported by storahci driver
*/
{
    ULONG bytesLeft;
    ULONG status;
    PVOID modePageHeader = NULL;
    MODE_PARAMETER_HEADER_WRAPPER modePageHeaderWrapper = { 0 };

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    ULONG               srbDataBufferLength = SrbGetDataTransferLength(Srb);

    // only support page control for current values
    if (Cdb->MODE_SENSE.Pc != 0) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    // modePageHeader can be PMODE_PARAMETER_HEADER or PMODE_PARAMETER_HEADER10,
    // depending on the command opcode. This is abstracted away by using
    // a wrapper.
    modePageHeader = SrbGetDataBuffer(Srb);

    if (modePageHeader == NULL) {
        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        //Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
        return STOR_STATUS_INSUFFICIENT_RESOURCES;
    }

    // initialize to success
    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    // this initializes the wrapper structure and sets the ModeDataLength
    // field
    status = AtaInitModePageHeaderWrapper(Cdb, modePageHeader, srbDataBufferLength, &modePageHeaderWrapper);
    if (status != STOR_STATUS_SUCCESS) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return status;
    }

    // now service the cache page
    bytesLeft = srbDataBufferLength - modePageHeaderWrapper.HeaderSize;

    if ((Cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) || (Cdb->MODE_SENSE.PageCode == MODE_PAGE_CACHING)) {
        if (bytesLeft >= sizeof(MODE_CACHING_PAGE)) {
            // cache settings page
            PMODE_CACHING_PAGE cachePage;

            cachePage = (PMODE_CACHING_PAGE)(((PCHAR) modePageHeaderWrapper.ModePageHeader) + modePageHeaderWrapper.HeaderSize);

            cachePage->PageCode = MODE_PAGE_CACHING;
            cachePage->PageSavable = 0;
            cachePage->PageLength = 0xa;
            cachePage->ReadDisableCache = 0;
            cachePage->WriteCacheEnable = 0;  //initialized value, likely to change below

            if (IsFuaSupported(ChannelExtension)) {
                *(modePageHeaderWrapper.DeviceSpecificParameter) |= MODE_DSP_FUA_SUPPORTED;
            }
            bytesLeft -= sizeof(MODE_CACHING_PAGE);
            *(modePageHeaderWrapper.ModeDataLength) += sizeof(MODE_CACHING_PAGE);

          //Check to see if the Write Cache is enabled on this device, by issuing an IDENTIFY DEVICE command
            if (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.WriteCache != 0) {
                srbExtension->AtaFunction = ATA_FUNCTION_ATA_IDENTIFY;
                srbExtension->Flags |= ATA_FLAGS_DATA_IN;
                srbExtension->DataBuffer = ChannelExtension->DeviceExtension->IdentifyDeviceData;
                srbExtension->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
                srbExtension->LocalSgl.NumberOfElements = 1;
                srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = ChannelExtension->DeviceExtension->IdentifyDataPhysicalAddress.LowPart;
                srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = ChannelExtension->DeviceExtension->IdentifyDataPhysicalAddress.HighPart;
                srbExtension->LocalSgl.List[0].Length = sizeof(IDENTIFY_DEVICE_DATA);
                srbExtension->Sgl = &srbExtension->LocalSgl;
                srbExtension->CompletionRoutine = AtaModeSenseRequestCompletionWriteCache;

                SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_IDENTIFY);

                return STOR_STATUS_SUCCESS; //Get Media Protect Status will be preformed in AtaModeSenseRequestCompletionWriteCache.
           }
        } else {
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        }
    }

    // update the number of bytes we are returning
    SrbSetDataTransferLength(Srb, srbDataBufferLength - bytesLeft);

    if (IsRemovableMedia(&ChannelExtension->DeviceExtension->DeviceParameters) &&
        IsMsnEnabled(ChannelExtension->DeviceExtension)) {
        // this command does not transfer data
        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
        srbExtension->CompletionRoutine = AtaModeSenseRequestCompletionMediaStatus;

        SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_GET_MEDIA_STATUS);
    }

    return STOR_STATUS_SUCCESS;
}

ULONG
AtaModeSelectRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*
    NOTE: this is used to enable/disable device write cache.
*/
{
    ULONG                   status = STOR_STATUS_SUCCESS;
    PMODE_PARAMETER_HEADER  modePageHeader;
    PMODE_CACHING_PAGE      cachePage;
    ULONG                   pageOffset;
    ULONG                   bytesLeft;

    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);
    ULONG                   srbDataBufferLength = SrbGetDataTransferLength(Srb);

    NT_ASSERT( (Cdb->MODE_SELECT.OperationCode == SCSIOP_MODE_SELECT) ||
               (Cdb->MODE_SELECT.OperationCode == SCSIOP_MODE_SELECT10) );

    // only support scsi-2 mode select format
    if (Cdb->MODE_SELECT.PFBit != 1) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_DEVICE_REQUEST;
    }

    if (srbDataBufferLength < sizeof(MODE_PARAMETER_HEADER)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_DEVICE_REQUEST;
    }

    modePageHeader = (PMODE_PARAMETER_HEADER)SrbGetDataBuffer(Srb);

    if (modePageHeader == NULL) {
        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        //Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
        return STOR_STATUS_INSUFFICIENT_RESOURCES;
    }

    pageOffset = sizeof(MODE_PARAMETER_HEADER) + modePageHeader->BlockDescriptorLength;

    if (srbDataBufferLength > pageOffset) {
        bytesLeft = srbDataBufferLength - pageOffset;
    } else {
        bytesLeft = 0;
    }

    cachePage = (PMODE_CACHING_PAGE)(((PUCHAR) modePageHeader) + pageOffset);

    if ( (bytesLeft >= sizeof(MODE_CACHING_PAGE)) &&
         (cachePage->PageCode == MODE_PAGE_CACHING) &&
         (cachePage->PageLength == 0xa) ) {

        //if Write Cache is supported
         if (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.WriteCache != 0) {
            srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

            if (cachePage->WriteCacheEnable != 0) {
                SetFeaturesReg ((&srbExtension->TaskFile.Current), IDE_FEATURE_ENABLE_WRITE_CACHE);
            } else {
                SetFeaturesReg ((&srbExtension->TaskFile.Current), IDE_FEATURE_DISABLE_WRITE_CACHE);
            }

            SetCommandReg ((&srbExtension->TaskFile.Current), IDE_COMMAND_SET_FEATURE);
        } else {
            // we are in the same cache state
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STOR_STATUS_SUCCESS;
        }

    } else {
        // the request is not for the mode cache page
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
    }

    return status;
}


VOID
SelectDeviceGeometry(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATA_DEVICE_PARAMETERS DeviceParameters,
    _In_ PIDENTIFY_DEVICE_DATA  IdentifyDeviceData
    )
{
    LARGE_INTEGER   maxLba;

    UNREFERENCED_PARAMETER(ChannelExtension);

  // ignore non ata devices
    if (!IsAtaDevice(DeviceParameters)) {
        return;
    }

    maxLba.QuadPart = 0;

    if (Support48Bit(DeviceParameters)) {
        maxLba.LowPart = IdentifyDeviceData->Max48BitLBA[0];
        maxLba.HighPart = IdentifyDeviceData->Max48BitLBA[1];
    } else {
        maxLba.LowPart = IdentifyDeviceData->UserAddressableSectors;
    }

    DeviceParameters->MaxLba = maxLba;

    // assume 512 bytes/sector
    if( IdentifyDeviceData->PhysicalLogicalSectorSize.LogicalSectorLongerThan256Words ) {
        DeviceParameters->BytesPerLogicalSector = IdentifyDeviceData->WordsPerLogicalSector[0] +
                                                 (IdentifyDeviceData->WordsPerLogicalSector[1] << 16);
        DeviceParameters->BytesPerLogicalSector <<= 1;
    }

    if (DeviceParameters->BytesPerLogicalSector < ATA_BLOCK_SIZE) {
        DeviceParameters->BytesPerLogicalSector = ATA_BLOCK_SIZE;
    }

    if( IdentifyDeviceData->PhysicalLogicalSectorSize.MultipleLogicalSectorsPerPhysicalSector ){
        DeviceParameters->BytesPerPhysicalSector = (1 << IdentifyDeviceData->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector) *
                                                   DeviceParameters->BytesPerLogicalSector;
    }

    if (DeviceParameters->BytesPerPhysicalSector < DeviceParameters->BytesPerLogicalSector) {
        DeviceParameters->BytesPerPhysicalSector = DeviceParameters->BytesPerLogicalSector;
    }

    if( IdentifyDeviceData->BlockAlignment.Word209Supported) {
        DeviceParameters->BytesOffsetForSectorAlignment = IdentifyDeviceData->BlockAlignment.AlignmentOfLogicalWithinPhysical *
                                                          DeviceParameters->BytesPerLogicalSector;
    }

    return;
}

ULONG
AtaReadCapacityCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    ULONG   status;
    ULONG   bytesPerSector;
    ULONG64 maxLba;
    PVOID   srbDataBuffer = SrbGetDataBuffer(Srb);
    ULONG   srbDataBufferLength = SrbGetDataTransferLength(Srb);
    UCHAR   cdbLength = SrbGetCdbLength(Srb);

    NT_ASSERT(!IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters));

    maxLba = MaxUserAddressableLba(&ChannelExtension->DeviceExtension->DeviceParameters) - 1;

    bytesPerSector = BytesPerLogicalSector(&ChannelExtension->DeviceExtension->DeviceParameters);

    // fill in read capacity

    if (srbDataBuffer != NULL) {

        if (cdbLength == 0x10) {
            // 16 byte CDB
            PREAD_CAPACITY16_DATA readCap16 = (PREAD_CAPACITY16_DATA)srbDataBuffer;
            ULONG                 returnDataLength = 12;    //default to sizeof(READ_CAPACITY_DATA_EX)

            if (srbDataBufferLength < sizeof(READ_CAPACITY_DATA_EX)) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                return STOR_STATUS_BUFFER_TOO_SMALL;
            }
            NT_ASSERT(SrbGetCdb(Srb)->CDB10.OperationCode == SCSIOP_READ_CAPACITY16);

            REVERSE_BYTES_QUAD(&readCap16->LogicalBlockAddress.QuadPart, &maxLba);
            REVERSE_BYTES(&readCap16->BytesPerBlock, &bytesPerSector);

            if ((LONG)srbDataBufferLength >= FIELD_OFFSET(READ_CAPACITY16_DATA, Reserved3)) {
                // buffer is big enough for sector alignment info
                readCap16->ProtectionEnable = 0;    // not support Protection Types.
                readCap16->ProtectionType = 0;
                readCap16->LogicalPerPhysicalExponent = (UCHAR)ChannelExtension->DeviceExtension->IdentifyDeviceData->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector;
                if(ChannelExtension->DeviceExtension->IdentifyDeviceData->BlockAlignment.Word209Supported) {
                    USHORT logicalBlocksPerPhysicalBlock = (USHORT)(1 << readCap16->LogicalPerPhysicalExponent);
                    USHORT lowestAlignedBlock = (logicalBlocksPerPhysicalBlock - ChannelExtension->DeviceExtension->IdentifyDeviceData->BlockAlignment.AlignmentOfLogicalWithinPhysical) %
                                                logicalBlocksPerPhysicalBlock;

                    readCap16->LowestAlignedBlock_MSB = (UCHAR)((lowestAlignedBlock >> 8) & 0x00FF);
                    readCap16->LowestAlignedBlock_LSB = (UCHAR)(lowestAlignedBlock & 0x00FF);
                } else {
                    readCap16->LowestAlignedBlock_LSB = 0;
                    readCap16->LowestAlignedBlock_MSB = 0;
                }

                if (IsDeviceSupportsTrim(ChannelExtension)) {
                    readCap16->LBPME = ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.DeterministicReadAfterTrimSupported ? 1 : 0;
                    readCap16->LBPRZ = (ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.DeterministicReadAfterTrimSupported && 
                                        ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.ReadZeroAfterTrimSupported) ? 1 : 0;
                } else {
                    readCap16->LBPME = 0;
                    readCap16->LBPRZ = 0;
                }

                returnDataLength = FIELD_OFFSET(READ_CAPACITY16_DATA, Reserved3);
            }

            SrbSetDataTransferLength(Srb, returnDataLength);
        } else {
            // 12 byte CDB
            PREAD_CAPACITY_DATA readCap = (PREAD_CAPACITY_DATA)srbDataBuffer;

            NT_ASSERT(SrbGetCdb(Srb)->CDB10.OperationCode == SCSIOP_READ_CAPACITY);

            if (srbDataBufferLength < sizeof(READ_CAPACITY_DATA)) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                return STOR_STATUS_BUFFER_TOO_SMALL;
            }

            REVERSE_BYTES(&readCap->BytesPerBlock, &bytesPerSector);

            if (maxLba >= MAXULONG) {
                readCap->LogicalBlockAddress = MAXULONG;
            } else {
                ULONG tmp = (ULONG)(maxLba & MAXULONG);
                REVERSE_BYTES(&readCap->LogicalBlockAddress, &tmp);
            }

            SrbSetDataTransferLength(Srb, sizeof(READ_CAPACITY_DATA));
        }

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STOR_STATUS_SUCCESS;

    } else {
        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        status = STOR_STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


VOID
AtaReadCapacityRequestCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    ULONG               status = STOR_STATUS_SUCCESS;
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    PUCHAR              identifyDeviceData = (PUCHAR)srbExtension->DataBuffer;

    NT_ASSERT(IsRemovableMedia(&ChannelExtension->DeviceExtension->DeviceParameters));

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {

        ATA_DEVICE_PARAMETERS deviceParameters;

        // make a local copy
        StorPortCopyMemory(&deviceParameters, &ChannelExtension->DeviceExtension->DeviceParameters, sizeof(ATA_DEVICE_PARAMETERS));

        // modify the fields related to media geometry
        SelectDeviceGeometry(ChannelExtension, &deviceParameters, (PIDENTIFY_DEVICE_DATA)identifyDeviceData);

        if (deviceParameters.MaxLba.QuadPart > 1) {
            // the device returned some geometry
            // update the one in the device extension
            StorPortCopyMemory(&ChannelExtension->DeviceExtension->DeviceParameters, &deviceParameters, sizeof(ATA_DEVICE_PARAMETERS));
            status = AtaReadCapacityCompletion(ChannelExtension, Srb);
        } else {
            AhciSetSenseData(Srb, SRB_STATUS_ERROR, SCSI_SENSE_NOT_READY, SCSI_ADSENSE_NO_MEDIA_IN_DEVICE, 0);

            SrbSetDataTransferLength(Srb, 0);

            status = STOR_STATUS_UNSUCCESSFUL;
        }
    } else {
        SrbSetDataTransferLength(Srb, 0);

        if (Srb->SrbStatus != SRB_STATUS_BUS_RESET) {
            Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
        }
        status = STOR_STATUS_UNSUCCESSFUL;
    }

    UNREFERENCED_PARAMETER(status);

    return;
}

ULONG
AtaReadCapacityRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb,
    _In_ ULONG                   CdbLength
    )
{
    ULONG               status = STOR_STATUS_SUCCESS;
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    ULONG               srbDataBufferLength = SrbGetDataTransferLength(Srb);

    NT_ASSERT(!IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters));

    // Verify the data transfer length
    if (srbDataBufferLength < sizeof(READ_CAPACITY_DATA)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    }

    if ((CdbLength == 0x10) &&
        (srbDataBufferLength < sizeof(READ_CAPACITY_DATA_EX))) {
        // classpnp may send down this request with buffer size = sizeof(READ_CAPACITY_DATA_EX)
        NT_ASSERT(SrbGetCdb(Srb)->CDB10.OperationCode == SCSIOP_READ_CAPACITY16);
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    }

    if ((CdbLength == 0x10) && (Cdb->CDB16.OperationCode == SCSIOP_READ_CAPACITY16) &&
        (Cdb->READ_CAPACITY16.ServiceAction != SERVICE_ACTION_READ_CAPACITY16)) {

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_DEVICE_REQUEST;
    }

    if (IsRemovableMedia(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        // get identify data from the device as the media might be changed.
        srbExtension->AtaFunction = ATA_FUNCTION_ATA_IDENTIFY;
        srbExtension->Flags |= ATA_FLAGS_DATA_IN;
        srbExtension->DataBuffer = ChannelExtension->DeviceExtension->IdentifyDeviceData;
        srbExtension->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
        srbExtension->CompletionRoutine = AtaReadCapacityRequestCompletion;
        srbExtension->LocalSgl.NumberOfElements = 1;
        srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = ChannelExtension->DeviceExtension->IdentifyDataPhysicalAddress.LowPart;
        srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = ChannelExtension->DeviceExtension->IdentifyDataPhysicalAddress.HighPart;
        srbExtension->LocalSgl.List[0].Length = sizeof(IDENTIFY_DEVICE_DATA);
        srbExtension->Sgl = &srbExtension->LocalSgl;

        SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_IDENTIFY);

    } else {
        // fixed media
        status = AtaReadCapacityCompletion(ChannelExtension, Srb);
    }

    return status;
}

VOID
AtaGenerateInquiryData (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_reads_bytes_(ATA_INQUIRYDATA_SIZE) PINQUIRYDATA InquiryData
    )
{
    USHORT descriptor = VER_DESCRIPTOR_1667_NOVERSION;
    ULONG len;
    ULONG vendorIdLength;
    ULONG prodLen;

    AhciZeroMemory((PCHAR)InquiryData, ATA_INQUIRYDATA_SIZE);

    InquiryData->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE;
    InquiryData->RemovableMedia = IsRemovableMedia(&ChannelExtension->DeviceExtension->DeviceParameters) ? 1 : 0;
    InquiryData->ResponseDataFormat = 0x2; //data format is defined in SPC standard
    InquiryData->CommandQueue = 1;  //support NCQ for AHCI controller
    InquiryData->AdditionalLength = ATA_INQUIRYDATA_SIZE - RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

    // if there is blank space in first 8 chars, use the part before blank space as VendorId
    for (vendorIdLength = 8; vendorIdLength > 0; vendorIdLength--) {
        if (ChannelExtension->DeviceExtension->DeviceParameters.VendorId[vendorIdLength - 1] == ' ') {
            break;
        }
    }

    len = min(vendorIdLength, sizeof(ChannelExtension->DeviceExtension->DeviceParameters.VendorId));

    AhciFillMemory((PCHAR)InquiryData->VendorId, 8, ' ');

    // if there is no blank space in first 8 chars, leave blank spaces in VendorId. Otherwise, copy the string
    if (len > 0 && len < 9) {
        StorPortCopyMemory(InquiryData->VendorId,
                           ChannelExtension->DeviceExtension->DeviceParameters.VendorId,
                           len
                           );
    }

    prodLen = min(16, sizeof(ChannelExtension->DeviceExtension->DeviceParameters.VendorId) - len);

    AhciFillMemory((PCHAR)InquiryData->ProductId, 16, ' ');
    StorPortCopyMemory(InquiryData->ProductId,
                       (ChannelExtension->DeviceExtension->DeviceParameters.VendorId + len),
                       prodLen
                       );

    len = min(4, sizeof(ChannelExtension->DeviceExtension->DeviceParameters.RevisionId));

    AhciFillMemory((PCHAR)InquiryData->ProductRevisionLevel, 4, ' ');
    StorPortCopyMemory(InquiryData->ProductRevisionLevel,
                       ChannelExtension->DeviceExtension->DeviceParameters.RevisionId,
                       len
                       );

    if ( (ChannelExtension->DeviceExtension->IdentifyDeviceData->TrustedComputing.FeatureSupported == 1) &&
         (ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.IEEE1667 == 1) ) {
        // fill in 1667 descriptor
        REVERSE_BYTES_SHORT(&InquiryData->VersionDescriptors[0], &descriptor);
    }

    return;
}

VOID
IssueIdentifyCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
    This could be a macro.  broken out here to make the logic easier to read
It assumes:
    nothing
Called by:
    AtaInquiryRequest
    AtaReportLunsCommand

It performs:
    1 Fills in the local SRB with the Identify Device command

Affected Variables/Registers:
    none

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

  //1 Fills in the local SRB
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_IDENTIFY;
    srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    srbExtension->CompletionRoutine = AhciPortIdentifyDevice;

    //setup TaskFile
    srbExtension->TaskFile.Current.bFeaturesReg = 0;
    srbExtension->TaskFile.Current.bSectorCountReg = 0;
    srbExtension->TaskFile.Current.bSectorNumberReg = 0;
    srbExtension->TaskFile.Current.bCylLowReg = 0;
    srbExtension->TaskFile.Current.bCylHighReg = 0;
    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0;
    srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_NOT_VALID; //the command will be set when we can read PxSIG register.
    srbExtension->TaskFile.Current.bReserved = 0;
    srbExtension->TaskFile.Previous.bFeaturesReg = 0;
    srbExtension->TaskFile.Previous.bSectorCountReg = 0;
    srbExtension->TaskFile.Previous.bSectorNumberReg = 0;
    srbExtension->TaskFile.Previous.bCylLowReg = 0;
    srbExtension->TaskFile.Previous.bCylHighReg = 0;
    srbExtension->TaskFile.Previous.bDriveHeadReg = 0;
    srbExtension->TaskFile.Previous.bCommandReg = 0;
    srbExtension->TaskFile.Previous.bReserved = 0;

    Srb->SrbStatus = SRB_STATUS_PENDING;
    srbExtension->DataBuffer = (PVOID)ChannelExtension->DeviceExtension->IdentifyDeviceData;

    srbExtension->LocalSgl.NumberOfElements = 1;
    srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = ChannelExtension->DeviceExtension->IdentifyDataPhysicalAddress.LowPart;
    srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = ChannelExtension->DeviceExtension->IdentifyDataPhysicalAddress.HighPart;
    srbExtension->LocalSgl.List[0].Length = sizeof(IDENTIFY_DEVICE_DATA);
    srbExtension->Sgl = &srbExtension->LocalSgl;
    srbExtension->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
    return;
}

ULONG
InquiryComplete(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
    This is the completion point of INQUIRY command for ATA devices.

Arguments:

    ChannelExtension
    Srb

Return Value:
    ULONG - status
--*/
{
    ULONG   status = STOR_STATUS_SUCCESS;
    ULONG   dataTransferLength = 0;
    // this is a standard INQUIRY
    UCHAR   inquiryData[ATA_INQUIRYDATA_SIZE] = {0};
    PCDB    cdb = SrbGetCdb(Srb);
    PVOID   srbDataBuffer = SrbGetDataBuffer(Srb);

    UCHAR   pathId = 0;
    UCHAR   targetId = 0;
    UCHAR   lun = 0;

    STOR_UNIT_ATTRIBUTES attributes = {0};

    // report error back so that Storport may retry the command.
    // NOTE that a READ LOG EXT command can also reach this path for a hybrid disk.
    if ( (cdb != NULL) && (cdb->CDB10.OperationCode == SCSIOP_INQUIRY) &&
         (Srb->SrbStatus != SRB_STATUS_PENDING) &&
         (Srb->SrbStatus != SRB_STATUS_SUCCESS) &&
         (Srb->SrbStatus != SRB_STATUS_NO_DEVICE) ) {
        return STOR_STATUS_UNSUCCESSFUL;
    }

    // report error if no device connected
    if ( ChannelExtension->DeviceExtension->DeviceParameters.AtaDeviceType == DeviceNotExist ) {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        return STOR_STATUS_UNSUCCESSFUL;
    }

    SrbGetPathTargetLun(Srb, &pathId, &targetId, &lun);

    // Indicate the existence of a device
    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    status = STOR_STATUS_SUCCESS;

    // make up inquiry data for ata devices
    AtaGenerateInquiryData(ChannelExtension, (PINQUIRYDATA)inquiryData);

    dataTransferLength = min(SrbGetDataTransferLength(Srb), ATA_INQUIRYDATA_SIZE);

    if (dataTransferLength > 0) {
        if (srbDataBuffer != NULL) {
            StorPortCopyMemory(srbDataBuffer, inquiryData, dataTransferLength);
            SrbSetDataTransferLength(Srb, dataTransferLength);
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STOR_STATUS_SUCCESS;
        } else {
            Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            //Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
            status = STOR_STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        // this will be a programming error.
        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        status = STOR_STATUS_INVALID_PARAMETER;
    }

    StorPortSetDeviceQueueDepth(ChannelExtension->AdapterExtension,
                                pathId,
                                targetId,
                                lun,
                                ChannelExtension->MaxPortQueueDepth);

    // tell port driver if D3Cold is supported or not for this device
    attributes.D3ColdNotSupported = (ChannelExtension->AdapterExtension->StateFlags.SupportsAcpiDSM == 0);

    StorPortSetUnitAttributes(ChannelExtension->AdapterExtension,
                              (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                              attributes);

    return status;
}

ULONG
AtaInquiryRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
/*
  NOTE: the command should be completed after calling this function as no real command will be sent to device.
*/
{
    ULONG status = STOR_STATUS_SUCCESS;

    NT_ASSERT(!IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters));

    // Filter out all TIDs but 0 and 1 since this is an IDE interface which support up to two devices.
    if (SrbGetLun(Srb) != 0) {
        // Indicate no device found at this address.
        Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
        status = STOR_STATUS_INVALID_PARAMETER;
    } else if (Cdb->CDB6INQUIRY3.EnableVitalProductData == 0) {

        if (IsDumpMode(ChannelExtension->AdapterExtension) && !DeviceIdentificationComplete(ChannelExtension->AdapterExtension)) {
            // the enumeration command from dump stack.
            IssueIdentifyCommand(ChannelExtension, Srb);
        } else if (ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.NeedUpdateIdentifyDeviceData == 1) {
            ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.NeedUpdateIdentifyDeviceData = 0;
            IssueIdentifyCommand(ChannelExtension, Srb);
        } else {
            status = InquiryComplete(ChannelExtension, Srb);
        }
    } else {
        PVOID   srbDataBuffer = SrbGetDataBuffer(Srb);
        ULONG   srbDataBufferLength = SrbGetDataTransferLength(Srb);

        if (srbDataBuffer == NULL) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        // the INQUIRY is for VPD page
        AhciZeroMemory((PCHAR)srbDataBuffer, srbDataBufferLength);

        switch(Cdb->CDB6INQUIRY3.PageCode) {
        case VPD_SUPPORTED_PAGES: {
            //
            // Input buffer should be at least the size of page header plus count of supported pages, each page needs one byte.
            //
            ULONG   requiredBufferSize = sizeof(VPD_SUPPORTED_PAGES_PAGE) + 6;

            if (srbDataBufferLength < requiredBufferSize) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            } else {
                PVPD_SUPPORTED_PAGES_PAGE outputBuffer = (PVPD_SUPPORTED_PAGES_PAGE)srbDataBuffer;

                outputBuffer->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE; ;
                outputBuffer->DeviceTypeQualifier = 0;
                outputBuffer->PageCode = VPD_SUPPORTED_PAGES;
                outputBuffer->PageLength = 0x06;        // supports 6 VPD pages
                outputBuffer->SupportedPageList[0] = VPD_SUPPORTED_PAGES;
                outputBuffer->SupportedPageList[1] = VPD_SERIAL_NUMBER;
                outputBuffer->SupportedPageList[2] = VPD_ATA_INFORMATION;
                outputBuffer->SupportedPageList[3] = VPD_BLOCK_LIMITS;
                outputBuffer->SupportedPageList[4] = VPD_BLOCK_DEVICE_CHARACTERISTICS;
                outputBuffer->SupportedPageList[5] = VPD_LOGICAL_BLOCK_PROVISIONING;

                SrbSetDataTransferLength(Srb, requiredBufferSize);

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STOR_STATUS_SUCCESS;
            }
            break;
        }
        case VPD_BLOCK_LIMITS: {
            if (srbDataBufferLength < 0x14) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            } else {
                PVPD_BLOCK_LIMITS_PAGE outputBuffer = (PVPD_BLOCK_LIMITS_PAGE)srbDataBuffer;

                outputBuffer->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE; ;
                outputBuffer->DeviceTypeQualifier = 0;
                outputBuffer->PageCode = VPD_BLOCK_LIMITS;

                //
                // leave outputBuffer->Descriptors[0 : 15] as '0' indicating 'not supported' for those fields.
                //

                if ( (srbDataBufferLength >= 0x24) &&
                     IsDeviceSupportsTrim(ChannelExtension) ) {
                    // not worry about multiply overflow as max of DsmCapBlockCount is min(AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE, 0xFFFF)
                    // calculate how many LBA ranges can be associated with one DSM - Trim command
                    ULONG   maxLbaRangeEntryCountPerCmd = ChannelExtension->DeviceExtension[0].DeviceParameters.DsmCapBlockCount * (ATA_BLOCK_SIZE / sizeof(ATA_LBA_RANGE));
                    // calculate how many LBA can be associated with one DSM - Trim command
                    ULONG   maxLbaCountPerCmd = maxLbaRangeEntryCountPerCmd * MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE;

                    NT_ASSERT (maxLbaCountPerCmd > 0);

                    // buffer is big enough for UNMAP information.
                    outputBuffer->PageLength[1] = 0x3C;        // must be 0x3C per spec

                    // (16:19) MAXIMUM UNMAP LBA COUNT
                    REVERSE_BYTES(&outputBuffer->Descriptors[16], &maxLbaCountPerCmd);

                    // (20:23) MAXIMUM UNMAP BLOCK DESCRIPTOR COUNT
                    REVERSE_BYTES(&outputBuffer->Descriptors[20], &maxLbaRangeEntryCountPerCmd);

                    // (24:27) OPTIMAL UNMAP GRANULARITY
                    // (28:31) UNMAP GRANULARITY ALIGNMENT; (28) bit7: UGAVALID
                        //leave '0' indicates un-supported.

                    // keep original 'Srb->DataTransferLength' value.
                } else {
                    outputBuffer->PageLength[1] = 0x10;
                    SrbSetDataTransferLength(Srb, 0x14);
                }

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STOR_STATUS_SUCCESS;
            }
            break;
        }
        case VPD_BLOCK_DEVICE_CHARACTERISTICS: {
            if (srbDataBufferLength < 0x08) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            } else {
                PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE outputBuffer = (PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE)srbDataBuffer;

                outputBuffer->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE; ;
                outputBuffer->DeviceTypeQualifier = 0;
                outputBuffer->PageCode = VPD_BLOCK_DEVICE_CHARACTERISTICS;
                outputBuffer->PageLength = 0x3C;        // must be 0x3C per spec
                outputBuffer->MediumRotationRateMsb = (UCHAR)((ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalMediaRotationRate >> 8) & 0x00FF);
                outputBuffer->MediumRotationRateLsb = (UCHAR)(ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalMediaRotationRate & 0x00FF);
                outputBuffer->NominalFormFactor = (UCHAR)(ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalFormFactor);

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STOR_STATUS_SUCCESS;
            }
            break;
        }
        case VPD_LOGICAL_BLOCK_PROVISIONING: {
            if (srbDataBufferLength < 0x08) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            } else {
                PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE outputBuffer = (PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE)srbDataBuffer;

                outputBuffer->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE; ;
                outputBuffer->DeviceTypeQualifier = 0;
                outputBuffer->PageCode = VPD_LOGICAL_BLOCK_PROVISIONING;
                outputBuffer->PageLength[1] = 0x04;      // 8 bytes data in total
                outputBuffer->DP = 0;

                if (ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.DeterministicReadAfterTrimSupported == TRUE) {
                    outputBuffer->ANC_SUP = IsDeviceSupportsTrim(ChannelExtension) ? 1 : 0;
                    outputBuffer->LBPRZ = (IsDeviceSupportsTrim(ChannelExtension) && ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.ReadZeroAfterTrimSupported) ? 1 : 0;
                } else {
                    outputBuffer->ANC_SUP = 0;
                    outputBuffer->LBPRZ = 0;
                }

                outputBuffer->LBPWS10 = 0;               // does not support WRITE SAME(10)
                outputBuffer->LBPWS = 0;                 // does not support WRITE SAME
                outputBuffer->LBPU = IsDeviceSupportsTrim(ChannelExtension) ? 1 : 0;

                SrbSetDataTransferLength(Srb, 0x08);

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STOR_STATUS_SUCCESS;
            }
            break;
        }
        case VPD_SERIAL_NUMBER: {
            if (srbDataBufferLength < 24) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            } else {
                PVPD_SERIAL_NUMBER_PAGE outputBuffer = (PVPD_SERIAL_NUMBER_PAGE)srbDataBuffer;
                UCHAR i;

                outputBuffer->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE; ;
                outputBuffer->DeviceTypeQualifier = 0;
                outputBuffer->PageCode = VPD_SERIAL_NUMBER;
                outputBuffer->PageLength = 20;      // 24 bytes data in total

                for (i = 0; i < outputBuffer->PageLength; i += 2) {
                    REVERSE_BYTES_SHORT(&outputBuffer->SerialNumber[i], &ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialNumber[i]);
                }

                SrbSetDataTransferLength(Srb, outputBuffer->PageLength + 4);

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STOR_STATUS_SUCCESS;
            }
            break;
        }
        case VPD_ATA_INFORMATION: {
            if (srbDataBufferLength < sizeof(VPD_ATA_INFORMATION_PAGE)) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            } else {
                PVPD_ATA_INFORMATION_PAGE outputBuffer = (PVPD_ATA_INFORMATION_PAGE)srbDataBuffer;
                ULONG vendorIdLength;
                ULONG len;
                ULONG prodLen;

                AhciZeroMemory((PCHAR)outputBuffer, sizeof(VPD_ATA_INFORMATION_PAGE));

                outputBuffer->DeviceType = ChannelExtension->DeviceExtension->DeviceParameters.ScsiDeviceType; //DIRECT_ACCESS_DEVICE; ;
                outputBuffer->DeviceTypeQualifier = 0;
                outputBuffer->PageCode = VPD_ATA_INFORMATION;
                outputBuffer->PageLength[0] = 0x02;
                outputBuffer->PageLength[1] = 0x38;      //PageLength: 0x238 - fixed value

                // if there is blank space in first 8 chars, use the part before blank space as VendorId
                for (vendorIdLength = 8; vendorIdLength > 0; vendorIdLength--) {
                    if (ChannelExtension->DeviceExtension->DeviceParameters.VendorId[vendorIdLength - 1] == ' ') {
                        break;
                    }
                }

                len = min(vendorIdLength, sizeof(ChannelExtension->DeviceExtension->DeviceParameters.VendorId));

                AhciFillMemory((PCHAR)outputBuffer->VendorId, 8, ' ');

                // if there is no blank space in first 8 chars, leave blank spaces in VendorId. Otherwise, copy the string
                if (len > 0 && len < 9) {
                    StorPortCopyMemory(outputBuffer->VendorId,
                                       ChannelExtension->DeviceExtension->DeviceParameters.VendorId,
                                       len
                                       );
                }

                prodLen = min(16, sizeof(ChannelExtension->DeviceExtension->DeviceParameters.VendorId) - len);

                AhciFillMemory((PCHAR)outputBuffer->ProductId, 16, ' ');
                StorPortCopyMemory(outputBuffer->ProductId,
                                   (ChannelExtension->DeviceExtension->DeviceParameters.VendorId + len),
                                   prodLen
                                   );

                len = min(4, sizeof(ChannelExtension->DeviceExtension->DeviceParameters.RevisionId));

                AhciFillMemory((PCHAR)outputBuffer->ProductRevisionLevel, 4, ' ');
                StorPortCopyMemory(outputBuffer->ProductRevisionLevel,
                                   ChannelExtension->DeviceExtension->DeviceParameters.RevisionId,
                                   len
                                   );

                //outputBuffer->DeviceSignature     -- not supported in current version of StorAHCI
                outputBuffer->CommandCode = IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters) ? IDE_COMMAND_IDENTIFY : IDE_COMMAND_ATAPI_IDENTIFY;
                StorPortCopyMemory(outputBuffer->IdentifyDeviceData, ChannelExtension->DeviceExtension->IdentifyDeviceData, sizeof(IDENTIFY_DEVICE_DATA));

                SrbSetDataTransferLength(Srb, sizeof(VPD_ATA_INFORMATION_PAGE));

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STOR_STATUS_SUCCESS;
            }
            break;
        }

        default:
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            break;
        }
    }

Exit:
    return status;
}

ULONG
AtaReportLunsCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PVOID Context
    )
/*
Assumption: there will be no REPORT LUNS command received when the previous REPORT LUNS command is still in process.

  This function is used to respond to QDR.
  It send Identify (Device) command to device
*/
{
    ULONG status = STOR_STATUS_SUCCESS;
    PSTORAGE_REQUEST_BLOCK srb = (PSTORAGE_REQUEST_BLOCK)Context;

    // Filter out all TIDs but 0 since this is an AHCI interface without Port Multiplier which can only support one device.
    if (SrbGetLun(srb) != 0) {
        // Indicate no device found at this address.
        srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
        status = STOR_STATUS_INVALID_PARAMETER;
    } else {
        IssueIdentifyCommand(ChannelExtension, srb);
    }

    return status;
}


ULONG
AtaStartStopUnitRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    //for STOP UNIT or EJECT MEDIA
    if (Cdb->START_STOP.Start == 0) {

        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

        if (Cdb->START_STOP.LoadEject == 1) {
            SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_MEDIA_EJECT);
        } else {
            SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_STANDBY_IMMEDIATE);
        }
    } else {
        //no action required for AHCI
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    return STOR_STATUS_SUCCESS;
}

ULONG
AtaTestUnitReadyRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if (IsMsnEnabled(ChannelExtension->DeviceExtension)) {
        // this command does not transfer data
        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

        SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_GET_MEDIA_STATUS);
    } else {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    return STOR_STATUS_SUCCESS;
}

ULONG
AtaMediumRemovalRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if (IsRmfEnabled(ChannelExtension->DeviceExtension)) {
        // this command does not transfer data
        UCHAR commandReg;

        commandReg = (Cdb->MEDIA_REMOVAL.Prevent == 1) ? IDE_COMMAND_DOOR_LOCK : IDE_COMMAND_DOOR_UNLOCK;

        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

        SetCommandReg((&srbExtension->TaskFile.Current), commandReg);
    } else {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    return STOR_STATUS_SUCCESS;
}

VOID
AtaAlwaysSuccessRequestCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    UNREFERENCED_PARAMETER(ChannelExtension);

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    return;
}

ULONG
AtaFlushCommandRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if (NoFlushDevice(ChannelExtension->DeviceExtension)) {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    } else {
        //
        UCHAR commandReg;
        commandReg = Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters) ? IDE_COMMAND_FLUSH_CACHE_EXT : IDE_COMMAND_FLUSH_CACHE;

        srbExtension->AtaFunction = ATA_FUNCTION_ATA_FLUSH;
        srbExtension->CompletionRoutine = AtaAlwaysSuccessRequestCompletion;    //legacy behavior: Flush Command will be always completed successfully.

        if ((ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.SystemPoweringDown == TRUE) ) {
            //Final Flush goes down alone
            if (ChannelExtension->SlotManager.NormalQueueSlice != 0) {
                NT_ASSERT(FALSE);
            }
            //This is the last FLUSH.  No more IO.
            ChannelExtension->StateFlags.NoMoreIO = TRUE;
        }

        SetCommandReg((&srbExtension->TaskFile.Current), commandReg);
    }

    return STOR_STATUS_SUCCESS;
}

VOID
AtaPassThroughRequestCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_BUS_RESET) {
        //
        // Fill return registers, Storport will update Status and Error register values for reset case.
        //
        PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

        if ((srbExtension->ResultBuffer != NULL) && (srbExtension->ResultBufferLength >= sizeof(DESCRIPTOR_SENSE_DATA))) {
            // for ATA PASS THROUGH 16 command, return Descriptor Format Sense Data, including ATA Status Return info.
            PDESCRIPTOR_SENSE_DATA descriptorSenseData = (PDESCRIPTOR_SENSE_DATA)srbExtension->ResultBuffer;
            PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN ataStatus = (PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN)((PUCHAR)descriptorSenseData + FIELD_OFFSET(DESCRIPTOR_SENSE_DATA, DescriptorBuffer));

            AhciZeroMemory((PCHAR)srbExtension->ResultBuffer, srbExtension->ResultBufferLength);

            // fill sense data header, leave SenseKey, ASC, ASCQ as zero.
            descriptorSenseData->ErrorCode = SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT;
            descriptorSenseData->AdditionalSenseLength = sizeof(SCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN);

            // fill ATA Status Return Info.
            ataStatus->Header.DescriptorType = SCSI_SENSE_DESCRIPTOR_TYPE_ATA_STATUS_RETURN;
            ataStatus->Header.AdditionalLength = 0x0C;
            ataStatus->Extend = Is48BitCommand(srbExtension->Flags) ? 1 : 0;

            ataStatus->Error = ChannelExtension->ReceivedFIS->D2hRegisterFis.Error;
            ataStatus->SectorCount7_0 = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorCount;
            ataStatus->LbaLow7_0 = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorNumber;
            ataStatus->LbaMid7_0 = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylLow;
            ataStatus->LbaHigh7_0 = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylHigh;
            ataStatus->Device = ChannelExtension->ReceivedFIS->D2hRegisterFis.Dev_Head;
            ataStatus->Status = ChannelExtension->ReceivedFIS->D2hRegisterFis.Status;

            if (Is48BitCommand(srbExtension->Flags)) {
                ataStatus->SectorCount15_8 = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorCount_Exp;
                ataStatus->LbaLow15_8 = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorNum_Exp;
                ataStatus->LbaMid15_8 = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylLow_Exp;
                ataStatus->LbaHigh15_8 = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylHigh_Exp;
            }

            // set flag SRB_STATUS_AUTOSENSE_VALID so that Storport will copy it back to original Sense Buffer
            Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    } else if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {
        // keep SRB_STATUS_AUTOSENSE_VALID it's set by StorAHCI
        // this flag is checked by Storport to copy back SenseInfoBuffer to original sense buffer.
        Srb->SrbStatus = SRB_STATUS_SUCCESS | (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID);
    }

    return;
}


ULONG
AtaPassThroughRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    ULONG               srbFlags = SrbGetSrbFlags(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    // if it's a 48bit command but device doesn't support it, assert.
    // command issuer needs to make sure this doesn't happen. The command will be sent to device and let device fail it, so that there is ATA status and error returned to issuer.
    NT_ASSERT( Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters) || (Cdb->ATA_PASSTHROUGH16.Extend == 0) );

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

    if (Cdb->ATA_PASSTHROUGH16.CkCond == 1) {
        srbExtension->Flags |= ATA_FLAGS_RETURN_RESULTS;    // indicate task file content needs to be returned in SenseInfoBuffer
        RequestGetSrbScsiData(Srb, NULL, NULL, &srbExtension->ResultBuffer, (PUCHAR)&srbExtension->ResultBufferLength);
    }

    if (srbFlags & SRB_FLAGS_DATA_IN) {
        srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    }
    if (srbFlags & SRB_FLAGS_DATA_OUT) {
        srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    }

    if ( Support48Bit(&ChannelExtension->DeviceExtension->DeviceParameters) && (Cdb->ATA_PASSTHROUGH16.Extend == 1) ) {
        srbExtension->Flags |= ATA_FLAGS_48BIT_COMMAND;
    }

    // ATA command taskfile
    srbExtension->TaskFile.Current.bFeaturesReg = Cdb->ATA_PASSTHROUGH16.Features7_0;
    srbExtension->TaskFile.Current.bSectorCountReg = Cdb->ATA_PASSTHROUGH16.SectorCount7_0;
    srbExtension->TaskFile.Current.bSectorNumberReg = Cdb->ATA_PASSTHROUGH16.LbaLow7_0;
    srbExtension->TaskFile.Current.bCylLowReg = Cdb->ATA_PASSTHROUGH16.LbaMid7_0;
    srbExtension->TaskFile.Current.bCylHighReg = Cdb->ATA_PASSTHROUGH16.LbaHigh7_0;
    srbExtension->TaskFile.Current.bDriveHeadReg = Cdb->ATA_PASSTHROUGH16.Device;
    srbExtension->TaskFile.Current.bCommandReg = Cdb->ATA_PASSTHROUGH16.Command;

    srbExtension->TaskFile.Previous.bFeaturesReg =Cdb->ATA_PASSTHROUGH16.Features15_8 ;
    srbExtension->TaskFile.Previous.bSectorCountReg = Cdb->ATA_PASSTHROUGH16.SectorCount15_8;
    srbExtension->TaskFile.Previous.bSectorNumberReg = Cdb->ATA_PASSTHROUGH16.LbaLow15_8;
    srbExtension->TaskFile.Previous.bCylLowReg = Cdb->ATA_PASSTHROUGH16.LbaMid15_8;
    srbExtension->TaskFile.Previous.bCylHighReg = Cdb->ATA_PASSTHROUGH16.LbaHigh15_8;
    srbExtension->TaskFile.Previous.bDriveHeadReg = 0;
    srbExtension->TaskFile.Previous.bCommandReg = 0;

    srbExtension->CompletionRoutine = AtaPassThroughRequestCompletion;

    return STOR_STATUS_SUCCESS;
}

ULONG
ConvertUnmapBlockDescrToAtaLbaRanges(
    _Inout_ PUNMAP_BLOCK_DESCRIPTOR BlockDescr,
    _In_reads_bytes_(BufferSize) PCHAR DestBuffer,
    _In_ ULONG  BufferSize
    )
/*++

Routine Description:

    Convert UNMAP_BLOCK_DESCRIPTOR entry to be ATA_LBA_RANGE entries.

    UNMAP_BLOCK_DESCRIPTOR->LbaCount is 32 bits; ATA_LBA_RANGE->SectorCount is 16 bits.
    it's possible that one UNMAP_BLOCK_DESCRIPTOR entry needs multiple ATA_LBA_RANGE entries.

Arguments:

    BlockDescr - the UNMAP_BLOCK_DESCRIPTOR entry, value of fields will be updated in this function
    DestBuffer
    BufferSize

Return Value:

    Count of ATA_LBA_RANGE entries converted.

    NOTE: if UNMAP_BLOCK_DESCRIPTOR->LbaCount does not reach to 0, the conversion for UNMAP_BLOCK_DESCRIPTOR entry
          is not completed. Further conversion is needed by calling this function again.

Example:  Lba - 0Bh, Length - 08h
ATA:          0008_0000_0000_000B
      In memory(little-endian):  0B 00 00 00 00 00 - 08 00
SCSI:         0000_0000 0800_0000 0B00_0000_0000_0000
      In memory(little-endian):  00 00 00 00 00 00 00 0B - 00 00 00 08 - 00 00 00 00

--*/
{
    ULONG           convertedEntryCount;
    PATA_LBA_RANGE  lbaRangeEntry;

    ULONGLONG       blockDescrStartingLba;
    ULONG           blockDescrLbaCount;

    convertedEntryCount = 0;
    lbaRangeEntry = (PATA_LBA_RANGE)DestBuffer;

    REVERSE_BYTES_QUAD(&blockDescrStartingLba, BlockDescr->StartingLba);
    REVERSE_BYTES(&blockDescrLbaCount, BlockDescr->LbaCount);

    // 1. fill in ATA_LBA_RANGE entries as needed
    while ((blockDescrLbaCount > 0) &&
           (convertedEntryCount * sizeof(ATA_LBA_RANGE) < BufferSize)) {
        //
        ULONG sectorCount;

        if (blockDescrLbaCount > MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE) {
            sectorCount = MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE;
        } else {
            sectorCount = blockDescrLbaCount;
        }

        lbaRangeEntry[convertedEntryCount].StartSector = blockDescrStartingLba;
        lbaRangeEntry[convertedEntryCount].SectorCount = sectorCount;

        blockDescrStartingLba += sectorCount;
        blockDescrLbaCount -= sectorCount;

        convertedEntryCount++;
    }

    // 2. update the UNMAP Block Descriptor
    REVERSE_BYTES_QUAD(BlockDescr->StartingLba, &blockDescrStartingLba);
    REVERSE_BYTES(BlockDescr->LbaCount, &blockDescrLbaCount);

    return convertedEntryCount;
}

ULONG
__inline
GetDataBufferLengthForDsmCommand (
    _In_ ULONG MaxLbaRangeEntryCountPerCmd,
    _In_ ULONG NeededLbaRangeEntryCount
)
/*
    Input:
        MaxLbaRangeEntryCountPerCmd:
        NeededLbaRangeEntryCount:

    Return value:
        the size of buffer needed for DSM command, the size is rounded up to ATA Lba range block size (512 byes).

    Note that the buffer needs to be rounded up to ATA Lba Range block size (512 byes).
    also Note that data transfer length is much smaller than 4G, the following multiplications are safe.
*/
{
    ULONG bufferLength;

    if (NeededLbaRangeEntryCount >= MaxLbaRangeEntryCountPerCmd) {
        // 1 use the max buffer size for a DSM command
        bufferLength = MaxLbaRangeEntryCountPerCmd * sizeof(ATA_LBA_RANGE);
    }
    else {
        // 2 one single DSM command can satisfy the Unmap request
        bufferLength = NeededLbaRangeEntryCount * sizeof(ATA_LBA_RANGE);
    }

    // 3 buffer size round up to ATA Lba range block size (512 byes).
    bufferLength = ((bufferLength - 1) / ATA_BLOCK_SIZE + 1) * ATA_BLOCK_SIZE;

    return bufferLength;
}

VOID
DeviceProcessTrimRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
)
/*++

Routine Description:

    Process TRIM request that received from upper layer.

Arguments:

    ChannelExtension
    Srb

Return Value:

    status of the operation

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    BOOLEAN             completed = FALSE;

    PATA_TRIM_CONTEXT   trimContext = (PATA_TRIM_CONTEXT)srbExtension->CompletionContext;
    PCHAR               buffer = (PCHAR)srbExtension->DataBuffer;
    ULONG               bufferLength = 0;

    BOOLEAN             tempBlockDescrConverted = TRUE;
    ULONG               bufferLengthUsed = 0;
    ULONG               convertedEntryCount = 0;

    completed = ((Srb->SrbStatus != SRB_STATUS_PENDING) && (Srb->SrbStatus != SRB_STATUS_SUCCESS)) ||
                (trimContext->ProcessedLbaRangeEntryCount >= trimContext->NeededLbaRangeEntryCount);

    if (!completed) {
        AhciZeroMemory(buffer, trimContext->AllocatedBufferLength);

        // 1. calculate the buffer size needed for DSM command
        bufferLength = GetDataBufferLengthForDsmCommand(trimContext->MaxLbaRangeEntryCountPerCmd, trimContext->NeededLbaRangeEntryCount - trimContext->ProcessedLbaRangeEntryCount);

        // 2. prepare and send DSM command
        //    check if the Unmap request is satisfied
        while (trimContext->ProcessedLbaRangeEntryCount < trimContext->NeededLbaRangeEntryCount) {
            // when first time this function is called, trimContext->CurrentBlockDescr.LbaCount should be '0'
            // so that the first Block Descriptor will be copied to trimContext->CurrentBlockDescr
            ULONG   tempBlockDescrLbaCount;

            REVERSE_BYTES(&tempBlockDescrLbaCount, trimContext->CurrentBlockDescr.LbaCount);
            tempBlockDescrConverted = (tempBlockDescrLbaCount == 0)? TRUE : FALSE;

            // if the previous entry conversion completed, continue the next one;
            // otherwise, still process the left part of the un-completed entry.
            if (tempBlockDescrConverted) {
                StorPortCopyMemory(&trimContext->CurrentBlockDescr, &trimContext->BlockDescriptors[trimContext->BlockDescrIndex], sizeof(UNMAP_BLOCK_DESCRIPTOR));
                trimContext->BlockDescrIndex++;
            }

            convertedEntryCount = ConvertUnmapBlockDescrToAtaLbaRanges(&trimContext->CurrentBlockDescr,
                                                                       buffer + bufferLengthUsed,
                                                                       bufferLength - bufferLengthUsed
                                                                       );
            trimContext->ProcessedLbaRangeEntryCount += convertedEntryCount;

            bufferLengthUsed += convertedEntryCount * sizeof(ATA_LBA_RANGE);

            // send DSM trim command when the buffer is full or all unmap entries are converted.
            if ( (bufferLengthUsed == bufferLength) ||
                 (trimContext->ProcessedLbaRangeEntryCount >= trimContext->NeededLbaRangeEntryCount) ) {
                // get ATA block count, the value is needed for setting the DSM command.
                ULONG transferBlockCount = bufferLength / ATA_BLOCK_SIZE;

                srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
                srbExtension->DataBuffer = buffer;
                srbExtension->DataTransferLength = bufferLength;

                // ATA command taskfile
                AhciZeroMemory((PCHAR)&srbExtension->TaskFile, sizeof(ATA_TASK_FILE));

                srbExtension->TaskFile.Current.bFeaturesReg = IDE_DSM_FEATURE_TRIM;
                // For TRIM command: LBA bit (bit 6) needs to be set for Device Register;
                // bit 7 and bit 5 are obsolete and always set by ATAport;
                // bit 4 is to select device0 or device1
                srbExtension->TaskFile.Current.bDriveHeadReg = 0xE0;
                srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_DATA_SET_MANAGEMENT;

                srbExtension->TaskFile.Current.bSectorCountReg = (UCHAR)(0x00FF & transferBlockCount);
                srbExtension->TaskFile.Previous.bSectorCountReg = (UCHAR)(transferBlockCount >> 8);

                srbExtension->CompletionRoutine = DeviceProcessTrimRequest;
                srbExtension->CompletionContext = (PVOID)trimContext;

                // update the SGL to reflect the actual transfer length.
                srbExtension->LocalSgl.List[0].Length = bufferLength;

                return;
            }
        }
        // should not reach this path
        NT_ASSERT(FALSE);

    } else {
        if (buffer != NULL) {
            AhciFreeDmaBuffer((PVOID)ChannelExtension->AdapterExtension, trimContext->AllocatedBufferLength, buffer);
        }

        if (trimContext != NULL) {
            StorPortFreePool((PVOID)ChannelExtension->AdapterExtension, trimContext);
        }

        srbExtension->DataBuffer = NULL;
        srbExtension->CompletionContext = NULL;
        srbExtension->CompletionRoutine = NULL;
        srbExtension->AtaFunction = 0;
    }
    return;
}


ULONG
AtaUnmapRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    ULONG               status = STOR_STATUS_SUCCESS;
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    PUNMAP_LIST_HEADER  unmapList = NULL;
    USHORT              blockDescrDataLength = 0;

    PATA_TRIM_CONTEXT   trimContext = NULL;
    PCHAR               buffer = NULL;     // buffer allocated for DSM trim command

    PVOID               srbDataBuffer = SrbGetDataBuffer(Srb);
    ULONG               srbDataBufferLength = SrbGetDataTransferLength(Srb);

    unmapList = (PUNMAP_LIST_HEADER)srbDataBuffer;

    if (unmapList == NULL) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    REVERSE_BYTES_SHORT(&blockDescrDataLength, unmapList->BlockDescrDataLength);

    if ( !IsDeviceSupportsTrim(ChannelExtension) ||
         (srbDataBufferLength < (ULONG)(blockDescrDataLength + 8)) ) {
        // either trim is not supported; or the Block Descriptor Size is too big
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
    } else {
        // some preparation work before actually starting to process the request
        ULONG                 i = 0;
        ULONG                 length = 0;
        STOR_PHYSICAL_ADDRESS bufferPhysicalAddress;

        status = StorPortAllocatePool(ChannelExtension->AdapterExtension, sizeof(ATA_TRIM_CONTEXT), AHCI_POOL_TAG, (PVOID*)&trimContext);
        if ( (status != STOR_STATUS_SUCCESS) || (trimContext == NULL) ) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            if (status == STOR_STATUS_SUCCESS) {
                status = STOR_STATUS_INSUFFICIENT_RESOURCES;
            }
            goto Exit;
        }
        AhciZeroMemory((PCHAR)trimContext, sizeof(ATA_TRIM_CONTEXT));

        trimContext->BlockDescriptors = (PUNMAP_BLOCK_DESCRIPTOR)((PCHAR)srbDataBuffer + 8);
        trimContext->BlockDescrCount = blockDescrDataLength / sizeof(UNMAP_BLOCK_DESCRIPTOR);

        // 1.1 calculate how many ATA Lba entries can be sent per DSM command
        //     every device LBA entry takes 8 bytes. not worry about multiply overflow as max of DsmCapBlockCount is 0xFFFF
        trimContext->MaxLbaRangeEntryCountPerCmd = (ChannelExtension->DeviceExtension[0].DeviceParameters.DsmCapBlockCount * ATA_BLOCK_SIZE) / sizeof(ATA_LBA_RANGE);

        if (trimContext->MaxLbaRangeEntryCountPerCmd == 0) {
            // do not expect this to happen.
            NT_ASSERT(FALSE);
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        // 1.2 calculate how many ATA Lba entries needed to complete this Unmap request
        for (i = 0; i < trimContext->BlockDescrCount; i++) {
            ULONG blockDescrLbaCount;
            REVERSE_BYTES(&blockDescrLbaCount, trimContext->BlockDescriptors[i].LbaCount);
            // 1.2.1 the ATA Lba entry - SectorCount field is 16bits; the Unmap Lba entry - LbaCount field is 32bits.
            //       following calculation shows how many ATA Lba entries should be used to represent the Unmap Lba entry.
            if (blockDescrLbaCount > 0) {
                trimContext->NeededLbaRangeEntryCount += (blockDescrLbaCount - 1) / MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE + 1;
            }
        }

        // 1.3 calculate the buffer size needed for DSM command
        trimContext->AllocatedBufferLength = GetDataBufferLengthForDsmCommand(trimContext->MaxLbaRangeEntryCountPerCmd, trimContext->NeededLbaRangeEntryCount);

        if (trimContext->AllocatedBufferLength == 0) {
            // UNMAP without Block Descriptor is allowed, SBC spec requires to not consider this as error.
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STOR_STATUS_SUCCESS;
            goto Exit;
        }

        // 1.4 allocate buffer, this buffer will be used to store ATA LBA Ranges for DSM command
        status = AhciAllocateDmaBuffer((PVOID)ChannelExtension->AdapterExtension, trimContext->AllocatedBufferLength, (PVOID*)&buffer);

        if ( (status != STOR_STATUS_SUCCESS) || (buffer == NULL) ) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            if (status == STOR_STATUS_SUCCESS) {
                status = STOR_STATUS_INSUFFICIENT_RESOURCES;
            }
            goto Exit;
        }

        // save values before calling DeviceProcessTrimRequest()
        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
        srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
        srbExtension->DataBuffer = buffer;
        srbExtension->DataTransferLength = trimContext->AllocatedBufferLength;
        srbExtension->CompletionContext = (PVOID)trimContext;

        bufferPhysicalAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, buffer, &length);
        srbExtension->LocalSgl.NumberOfElements = 1;
        srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = bufferPhysicalAddress.LowPart;
        srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = bufferPhysicalAddress.HighPart;
        srbExtension->LocalSgl.List[0].Length = trimContext->AllocatedBufferLength;
        srbExtension->Sgl = &srbExtension->LocalSgl;

        // process the request, this function will set itself as completion routine to send multiple DSM commands one by one.
        DeviceProcessTrimRequest(ChannelExtension, Srb);
    }

Exit:
    // the process failed before DSM command can be sent. Free allocated resources.
    if (status != STOR_STATUS_SUCCESS) {
        if (buffer != NULL) {
            AhciFreeDmaBuffer((PVOID)ChannelExtension->AdapterExtension, trimContext->AllocatedBufferLength, buffer);
        }

        if (trimContext != NULL) {
            StorPortFreePool((PVOID)ChannelExtension->AdapterExtension, trimContext);
        }
    }

    return status;
}


ULONG
AtaSecurityProtocolRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    PCDB                securityCdb = Cdb;
    ULONG               dataLength = 0;
    UCHAR               commandReg;
    UCHAR               nonDataTrustedReceive = 0;

    if (ChannelExtension->DeviceExtension->IdentifyDeviceData->TrustedComputing.FeatureSupported == 0) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    } else if (securityCdb->SECURITY_PROTOCOL_IN.INC_512 == 0) {
        // Reject the command if INC_512 bit is set to 0. Some drivers use this answer to know the transfer size should be aligned to 512.
        // StorAHCI only supports the command with INC_512 bit set to 1, to avoid the complexity of handling partial sectors.
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
    } else {

        dataLength = (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[0] << 24) |
                     (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[0] << 16) |
                     (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[0] << 8) |
                     (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[0]);

        if (dataLength > 0xFFFF) {
            // ATA TRUSTED commands can only process 2 bytes of data transfer length.
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        } else {
            // get command to be used
            if ((securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[3] == 0) &&
                (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[2] == 0) &&
                (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[1] == 0) &&
                (securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[0] == 0)) {
                // Non-data transfer
                commandReg = IDE_COMMAND_TRUSTED_NON_DATA;
                nonDataTrustedReceive = (Cdb->CDB10.OperationCode == SCSIOP_SECURITY_PROTOCOL_IN) ? 1 : 0;
            } else {
                NT_ASSERT((SrbGetSrbFlags(Srb) & SRB_FLAGS_UNSPECIFIED_DIRECTION) != 0);
                if (Cdb->CDB10.OperationCode == SCSIOP_SECURITY_PROTOCOL_IN) {
                    commandReg = IDE_COMMAND_TRUSTED_RECEIVE_DMA;
                    srbExtension->Flags |= ATA_FLAGS_DATA_IN;
                } else {
                    commandReg = IDE_COMMAND_TRUSTED_SEND_DMA;
                    srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
                }
                srbExtension->Flags |= ATA_FLAGS_USE_DMA;
            }

            // Set up taskfile in SrbExtension.
            srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

            SetCommandReg((&srbExtension->TaskFile.Current), commandReg);
            SetFeaturesReg((&srbExtension->TaskFile.Current), securityCdb->SECURITY_PROTOCOL_IN.SecurityProtocol);

            SetSectorCount((&srbExtension->TaskFile.Current), securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[3]);           //low byte of transfer length
            SetSectorNumber((&srbExtension->TaskFile.Current), securityCdb->SECURITY_PROTOCOL_IN.AllocationLength[2]);          //high byte of transfer length

            SetCylinderHigh((&srbExtension->TaskFile.Current), securityCdb->SECURITY_PROTOCOL_IN.SecurityProtocolSpecific[0]);  //SILO_INDEX, high byte of protocol specific
            SetCylinderLow((&srbExtension->TaskFile.Current), securityCdb->SECURITY_PROTOCOL_IN.SecurityProtocolSpecific[1]);   //FUNCTION_ID, low byte of protocol specific

            SetDeviceReg((&srbExtension->TaskFile.Current), nonDataTrustedReceive);
        }
    }

    return STOR_STATUS_SUCCESS;
}


VOID
AtaWriteBufferFirmwareDownload (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    ULONG               bufferOffset = 0;
    ULONG               dataLength = 0;
    UCHAR               commandReg;

    bufferOffset = (ULONG)Cdb->WRITE_BUFFER.BufferOffset[0] << 16 |
                   (ULONG)Cdb->WRITE_BUFFER.BufferOffset[1] << 8 |
                   (ULONG)Cdb->WRITE_BUFFER.BufferOffset[2];

    dataLength = (ULONG)Cdb->WRITE_BUFFER.ParameterListLength[0] << 16 |
                 (ULONG)Cdb->WRITE_BUFFER.ParameterListLength[1] << 8 |
                 (ULONG)Cdb->WRITE_BUFFER.ParameterListLength[2];

    if (((bufferOffset % ATA_BLOCK_SIZE) != 0) ||
        ((dataLength % ATA_BLOCK_SIZE) != 0)) {
        // Report invalid command.
        AhciSetSenseData(Srb, SRB_STATUS_INVALID_REQUEST, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB, 0);
        goto exit;
    }

    // Convert offset and data length using unit of Block Size.
    bufferOffset /= ATA_BLOCK_SIZE;
    dataLength /= ATA_BLOCK_SIZE;

    if ((dataLength > ChannelExtension->DeviceExtension->FirmwareUpdate.DmMaxTransferBlocks) ||
        (dataLength < ChannelExtension->DeviceExtension->FirmwareUpdate.DmMinTransferBlocks)) {
        // Report invalid command.
        AhciSetSenseData(Srb, SRB_STATUS_INVALID_REQUEST, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB, 0);
        goto exit;
    }

    if (Cdb->WRITE_BUFFER.Mode == 0x0D) {
        if (((Cdb->WRITE_BUFFER.ModeSpecific & SCSI_WRITE_BUFFER_MODE_0D_MODE_SPECIFIC_PO_ACT) == 0) ||
            ((Cdb->WRITE_BUFFER.ModeSpecific & SCSI_WRITE_BUFFER_MODE_0D_MODE_SPECIFIC_HR_ACT) != 0)) {
            // Report invalid command.
            AhciSetSenseData(Srb, SRB_STATUS_INVALID_REQUEST, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB, 0);
            goto exit;
        }
    }

    NT_ASSERT((SrbGetSrbFlags(Srb) & SRB_FLAGS_UNSPECIFIED_DIRECTION) != 0);

    commandReg = GetFirmwareUpdateCommand(ChannelExtension);
    NT_ASSERT(commandReg != IDE_COMMAND_NOT_VALID);

    if (commandReg == IDE_COMMAND_DOWNLOAD_MICROCODE_DMA) {
        srbExtension->Flags |= ATA_FLAGS_USE_DMA;
    }

    srbExtension->Flags |= ATA_FLAGS_DATA_OUT;

    // Set up taskfile in SrbExtension.
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

    SetCommandReg((&srbExtension->TaskFile.Current), commandReg);

    SetFeaturesReg((&srbExtension->TaskFile.Current), 0x0E);

    SetSectorCount((&srbExtension->TaskFile.Current), (UCHAR)dataLength);               // low byte of transfer length
    SetSectorNumber((&srbExtension->TaskFile.Current), (UCHAR)(dataLength >> 8));       // high byte of transfer length

    SetCylinderLow((&srbExtension->TaskFile.Current), (UCHAR)bufferOffset);             // low byte of buffer offset
    SetCylinderHigh((&srbExtension->TaskFile.Current), (UCHAR)(bufferOffset >> 8));     // high byte of buffer offset

exit:

    // If the ATA command completes without error and the ATA device returns a COUNT field set to 01h, then the
    // SATL should send additional ATA download commands.

    // If the ATA command completes without error and the ATA device returns a COUNT field set to 03h, then the
    // SATL shall remember that the new microcode shall be activated by whichever event in table 48 occurs first.

    return;
}

VOID
AtaFirmwareActivateCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    //
    // When a new firmware is activated successfully, the next INQUIRY command should trigger
    // the cached Identify Device Data to be updated.
    //
    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.NeedUpdateIdentifyDeviceData = 1;
    }

    return;
}


VOID
AtaWriteBufferFirmwareActivate (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    UCHAR               commandReg;

    if ((Cdb->WRITE_BUFFER.ModeSpecific != 0) ||
        (Cdb->WRITE_BUFFER.BufferID != 0) ||
        (Cdb->WRITE_BUFFER.BufferOffset[0] != 0) ||
        (Cdb->WRITE_BUFFER.BufferOffset[1] != 0) ||
        (Cdb->WRITE_BUFFER.BufferOffset[2] != 0) ||
        (Cdb->WRITE_BUFFER.ParameterListLength[0] != 0) ||
        (Cdb->WRITE_BUFFER.ParameterListLength[1] != 0) ||
        (Cdb->WRITE_BUFFER.ParameterListLength[2] != 0)) {
        // Report invalid command.
        AhciSetSenseData(Srb, SRB_STATUS_INVALID_REQUEST, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB, 0);

    } else {
        NT_ASSERT((SrbGetSrbFlags(Srb) & SRB_FLAGS_UNSPECIFIED_DIRECTION) == 0);

        commandReg = GetFirmwareUpdateCommand(ChannelExtension);
        NT_ASSERT(commandReg != IDE_COMMAND_NOT_VALID);

        if (commandReg == IDE_COMMAND_DOWNLOAD_MICROCODE_DMA) {
            srbExtension->Flags |= ATA_FLAGS_USE_DMA;
        }

        // Set up taskfile in SrbExtension.
        srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
        srbExtension->CompletionRoutine = AtaFirmwareActivateCompletion;

        SetCommandReg((&srbExtension->TaskFile.Current), commandReg);
        SetFeaturesReg((&srbExtension->TaskFile.Current), 0x0F);
    }

    return;
}


ULONG
AtaWriteBufferRequest (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ PCDB                    Cdb
    )
{
    if (!IsFirmwareUpdateSupported(ChannelExtension)) {
        //the Subcommand 0Eh and 0Fh are supported bit of the DOWNLOAD  MICROCODE Capabilities field of ATA Identify Device Data log page 03h (DOWNLOAD MICROCODE OFFSETS
        //    DEFERRED SUPPORTED bit) is set to zero),
        // IDENTIFY DEVICE data log (Log Address 30h) - 03h Supported Capabilities (see A.11.5) A.11.5.3 DOWNLOAD MICROCODE Capabilities

        // then the SATL shall terminate the command with CHECK CONDITION status with the sense key set to ILLEGAL REQUST with the additional sense code set to INVALID FIELD IN CDB.
        AhciSetSenseData(Srb, SRB_STATUS_INVALID_REQUEST, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB, 0);

    } else if ((Cdb->WRITE_BUFFER.Mode == 0x0D) ||
               (Cdb->WRITE_BUFFER.Mode == 0x0E)) {
        AtaWriteBufferFirmwareDownload(ChannelExtension, Srb, Cdb);
    } else if (Cdb->WRITE_BUFFER.Mode == 0x0F) {
        AtaWriteBufferFirmwareActivate(ChannelExtension, Srb, Cdb);
    } else {
        AhciSetSenseData(Srb, SRB_STATUS_INVALID_REQUEST, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_INVALID_CDB, 0);
    }

    return STOR_STATUS_SUCCESS;
}

UCHAR
AtaMapError(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN FUAcommand
    )
/*++

Routine Description:

    Maps the ATA errors to SCSI and builds sense data for them

Arguments:

    ChannelExtension
    Srb

Return Value:

    SrbStatus

--*/

{
    BOOLEAN     removableMedia;
    SENSE_DATA  senseBuffer = {0};
    ULONG       length = sizeof(SENSE_DATA);
    PVOID      srbSenseBuffer = NULL;
    UCHAR      srbSenseBufferLength = 0;

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    RequestGetSrbScsiData(Srb, NULL, NULL, &srbSenseBuffer, &srbSenseBufferLength);

    if (IsReturnResults(srbExtension->Flags)) {
        // There is already something in the SenseInfoBuffer or this is an ATA PASS THRU.
        return Srb->SrbStatus;
    }

    // 1. special interpretation for FUA command
    if (FUAcommand == TRUE) {
       //if the first FUA command succeeds, remember so future failures don't disable FUA commands
       //if the first FUA command fails, leverage the class driver to not send anymore FUA commands
        if (ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.FuaSucceeded == 0) {
            if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
                ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.FuaSucceeded = 1;
            } else if (Srb->SrbStatus == SRB_STATUS_ERROR) {
                //
                Srb->SrbStatus = SRB_STATUS_ERROR;
                Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                SrbSetScsiStatus(Srb, SCSISTAT_CHECK_CONDITION);

                senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
                senseBuffer.Valid     = 1;
                senseBuffer.AdditionalSenseLength = 0xb;
                senseBuffer.SenseKey =  SCSI_SENSE_ILLEGAL_REQUEST;
                senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_INVALID_CDB;
                senseBuffer.AdditionalSenseCodeQualifier = 0;
            }
        }
    }

    if (Srb->SrbStatus != SRB_STATUS_ERROR) {
        // non device errors. Don't care
        ChannelExtension->DeviceExtension[0].IoRecord.SuccessCount++;

        return Srb->SrbStatus;
    }

    // 2. general process
    removableMedia = IsRemovableMedia(&ChannelExtension->DeviceExtension->DeviceParameters);

    if (senseBuffer.Valid == 0) {
        // senseBuffer is not set yet, start ...

        if (srbExtension->AtaError & IDE_ERROR_CRC_ERROR) {
            // bit 7: Interface CRC error

            Srb->SrbStatus = SRB_STATUS_PARITY_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey =  SCSI_SENSE_HARDWARE_ERROR;
            senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_LUN_COMMUNICATION;
            senseBuffer.AdditionalSenseCodeQualifier = SCSI_SESNEQ_COMM_CRC_ERROR;

            ChannelExtension->DeviceExtension[0].IoRecord.CrcErrorCount++;

        } else if (srbExtension->AtaError & IDE_ERROR_DATA_ERROR) {
            // bit 6: Uncorrectable Error

            Srb->SrbStatus = SRB_STATUS_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey = removableMedia ? SCSI_SENSE_DATA_PROTECT :
                                                    SCSI_SENSE_MEDIUM_ERROR ;
            senseBuffer.AdditionalSenseCode = 0;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            if (removableMedia) {
                ChannelExtension->DeviceExtension[0].IoRecord.OtherErrorCount++;
            } else {
                ChannelExtension->DeviceExtension[0].IoRecord.MediaErrorCount++;
            }

        } else if (srbExtension->AtaError & IDE_ERROR_MEDIA_CHANGE) {
            // bit 5: Media Changed (legacy)

            Srb->SrbStatus = SRB_STATUS_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            ChannelExtension->DeviceExtension[0].IoRecord.OtherErrorCount++;

        } else if (srbExtension->AtaError & IDE_ERROR_ID_NOT_FOUND) {
            // bit 4: ID Not Found

            Srb->SrbStatus = SRB_STATUS_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey =  SCSI_SENSE_ILLEGAL_REQUEST;
            senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_BLOCK;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            ChannelExtension->DeviceExtension[0].IoRecord.IllegalCommandCount++;

        } else if (srbExtension->AtaError & IDE_ERROR_MEDIA_CHANGE_REQ) {
            // bit 3: Media Change Request (legacy)

            Srb->SrbStatus = SRB_STATUS_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_OPERATOR_REQUEST;
            senseBuffer.AdditionalSenseCodeQualifier = SCSI_SENSEQ_MEDIUM_REMOVAL;

            ChannelExtension->DeviceExtension[0].IoRecord.OtherErrorCount++;

        } else if (srbExtension->AtaError & IDE_ERROR_COMMAND_ABORTED) {

            // bit 2: Command Aborted

            Srb->SrbStatus = SRB_STATUS_ABORTED;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
            senseBuffer.AdditionalSenseCode = 0;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            ChannelExtension->DeviceExtension[0].IoRecord.AbortedCommandCount++;

        } else if (srbExtension->AtaError & IDE_ERROR_END_OF_MEDIA) {
            // bit 1: End of Media (legacy)

            Srb->SrbStatus = SRB_STATUS_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey =  SCSI_SENSE_NOT_READY;
            senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            ChannelExtension->DeviceExtension[0].IoRecord.EndofMediaCount++;

        } else if (srbExtension->AtaError & IDE_ERROR_ADDRESS_NOT_FOUND) {
            // bit 0: Media Error (legacy)

            Srb->SrbStatus = SRB_STATUS_ERROR;

            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid     = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey = removableMedia ? SCSI_SENSE_DATA_PROTECT :
                                                    SCSI_SENSE_MEDIUM_ERROR ;
            senseBuffer.AdditionalSenseCode = 0;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            if (removableMedia) {
                ChannelExtension->DeviceExtension[0].IoRecord.OtherErrorCount++;
            } else {
                ChannelExtension->DeviceExtension[0].IoRecord.MediaErrorCount++;
            }

        } else if (IsReadWriteCommand(Srb) &&
                   (srbExtension->AtaStatus & IDE_STATUS_DEVICE_FAULT)) {
            //
            // If this is a read or write command and the Device Fault (DF)
            // bit in the Status is set, then translate this to a Hardware
            // Error, Internal Target Failure, according to SAT-3.
            //
            Srb->SrbStatus = SRB_STATUS_ERROR;
            senseBuffer.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
            senseBuffer.Valid = 1;
            senseBuffer.AdditionalSenseLength = 0xb;
            senseBuffer.SenseKey = SCSI_SENSE_HARDWARE_ERROR;
            senseBuffer.AdditionalSenseCode = SCSI_ADSENSE_INTERNAL_TARGET_FAILURE;
            senseBuffer.AdditionalSenseCodeQualifier = 0;

            ChannelExtension->DeviceExtension[0].IoRecord.DeviceFaultCount++;

        } else {
            Srb->SrbStatus = SRB_STATUS_ERROR;
            ChannelExtension->DeviceExtension[0].IoRecord.OtherErrorCount++;
        }
    }

    if ( (senseBuffer.Valid == 1) && (srbSenseBuffer != NULL) ) {

        if ( (srbSenseBufferLength > 0) &&
             (srbSenseBufferLength < length) ) {
            length = srbSenseBufferLength;
        }

        NT_ASSERT(length > 0);

        StorPortCopyMemory(srbSenseBuffer, (PVOID)&senseBuffer, length);

        Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        SrbSetScsiStatus(Srb, SCSISTAT_CHECK_CONDITION);
    }

    return Srb->SrbStatus;
}

VOID
AhciSetSenseData (
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_ UCHAR                  SrbStatus,
    _In_ UCHAR                  SenseKey,
    _In_ UCHAR                  ASC,
    _In_ UCHAR                  ASCQ
)
/*++

Routine Description:

    Set sense data information into Srb Sensedata buffer.

Arguments:

    Srb - 
    SrbStatus - 
    SenseKey -
    ASC - 
    ASCQ - 

Return Value:

    None

--*/
{
    SENSE_DATA  senseBuffer = {0};
    ULONG       length = sizeof(SENSE_DATA);

    PVOID       senseInfoBuffer = NULL;
    UCHAR       senseInfoBufferLength = 0;

    NT_ASSERT(SrbStatus != SRB_STATUS_SUCCESS);

    Srb->SrbStatus = SrbStatus;

    SetSenseData(&senseBuffer, SenseKey, ASC, ASCQ);

    //
    // Get data fields from Srb and fill in the status.
    //
    RequestGetSrbScsiData((PSTORAGE_REQUEST_BLOCK)Srb, NULL, NULL, &senseInfoBuffer, &senseInfoBufferLength);

    //
    // Copy sensedata into buffer associated with Srb.
    //
    if ((senseBuffer.Valid == 1) && 
        (senseInfoBuffer != NULL) && 
        (senseInfoBufferLength > 0)) {

        length = min(sizeof(SENSE_DATA), senseInfoBufferLength);

        StorPortCopyMemory(senseInfoBuffer, (PVOID)&senseBuffer, length);

        Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        SrbSetScsiStatus(Srb, SCSISTAT_CHECK_CONDITION);

        //
        // SenseBuffer has been zero-ed in BuildIo. There is no need to zero the left portion here.
        //
    }

    return;
}


VOID
CopyField(
    _Out_writes_bytes_(Count+1) PUCHAR Destination,
    _In_reads_bytes_(Count) PUCHAR Source,
    _In_ ULONG Count,
    _In_ UCHAR Change
    )

/*++

Routine Description:

    This routine will copy Count string bytes from Source to Destination.  If
    it finds a null byte in the Source it will translate that and any subsequent
    bytes into Change.  It will also replace non-printable characters with the
    specified character.

Arguments:

    Destination - the location to copy bytes

    Source - the location to copy bytes from

    Count - the number of bytes to be copied

Return Value:

    none

Notes:

    This routine will add a NULL char at Destination[Count].
--*/

{
    ULONG i = 0;
    BOOLEAN pastEnd = FALSE;

    for(i = 0; i < Count; i++) {
        if(!pastEnd) {
            if(Source[i] == 0) {
                pastEnd = TRUE;
                Destination[i] = Change;
            } else if ((Source[i] <= ' ') ||
                       (Source[i] > 0x7f) ||
                       (Source[i] == ',')) {
                Destination[i] = Change;
            } else {
                Destination[i] = Source[i];
            }
        } else {
            Destination[i] = Change;
        }
    }

    Destination[i] = '\0';

    return;
}

VOID
FormatAtaId (
    _Out_writes_bytes_(CharCount + 1) PUCHAR Destination,
    _In_reads_bytes_(CharCount)  PUCHAR Source,
    _In_ ULONG CharCount
    )
/*++

    Constructs and formats ATA string. Used for ModelNumber, FirmwareRevision and SerialNumber

--*/
{
    NT_ASSERT(CharCount > 0);

    Destination[0] = '\0';

    CopyField(Destination, Source, CharCount, ' ');

    ByteSwap(Destination, CharCount);

    // This will null terminate the string
    RemoveTrailingBlanks(Destination, CharCount + 1);

    return;
}


VOID
DeviceInitAtaIds(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyDeviceData
    )
{
    FormatAtaId(ChannelExtension->DeviceExtension->DeviceParameters.VendorId,
                IdentifyDeviceData->ModelNumber,
                sizeof(ChannelExtension->DeviceExtension->DeviceParameters.VendorId) - 1
                );

    FormatAtaId(ChannelExtension->DeviceExtension->DeviceParameters.RevisionId,
                IdentifyDeviceData->FirmwareRevision,
                sizeof(ChannelExtension->DeviceExtension->DeviceParameters.RevisionId) - 1
                );

    FormatAtaId(ChannelExtension->DeviceExtension->DeviceParameters.SerialNumber,
                IdentifyDeviceData->SerialNumber,
                sizeof(ChannelExtension->DeviceExtension->DeviceParameters.SerialNumber) - 1
                );
    return;
}

VOID
FormatAtapiVendorId(
    _In_ PINQUIRYDATA InquiryData,
    _Out_writes_bytes_(Length) PUCHAR VendorId,
    _In_ ULONG Length
    )
/*++

Routine Description:

    Constructs and formats the Vendor  Id from InquiryData

Arguments:

    InquiryData - the InquiryData from the device
    VendorId - destination buffer
    Length - Length of the destination buffer

Return Value:

    None.

--*/
{
    ULONG index;
    ULONG bytesLeft;

    VendorId[0] = '\0';
    bytesLeft = Length;

    if (bytesLeft > 8) {

        CopyField( VendorId,
                   InquiryData->VendorId,
                   8,
                   ' '
                   );

        NT_ASSERT(VendorId[8] == '\0');
        VendorId[8] = ' ';

        index = RemoveTrailingBlanks(VendorId, 9);

        if (index < Length) {
            NT_ASSERT(VendorId[index] == '\0');

            VendorId[index] = ' ';
            VendorId += index + 1;

            bytesLeft -= (index + 1);
        }
    }

    if (bytesLeft > 16) {

// At this point, VendorId points a buffer that has at least 16 bytes
#pragma warning (suppress: 6386)
        CopyField( VendorId,
                   InquiryData->ProductId,
                   16,
                   ' '
                   );

        NT_ASSERT(VendorId[16] == '\0');
        VendorId[16] = ' ';

        RemoveTrailingBlanks(VendorId, 17);
    }

    return;
}

VOID
FormatAtapiRevisionId(
    _In_ PINQUIRYDATA InquiryData,
    _Out_writes_bytes_(Length) PUCHAR RevisionId,
    _In_ ULONG Length
    )
/*++

Routine Description:

    Constructs and formats the  Revision Id from InquiryData

Arguments:

    InquiryData -  the InquiryDatadata from the device
    RevisionId - destination buffer
    Length - Length of the destination buffer

Return Value:

    None.

--*/
{
    RevisionId[0] = '\0';

    if (Length > 4) {

        CopyField( RevisionId,
                   InquiryData->ProductRevisionLevel,
                   4,
                   ' '
                   );

        NT_ASSERT(RevisionId[4] == '\0');
        RevisionId[4] = ' ';

        RemoveTrailingBlanks(RevisionId, 5);
    }

    return;
}

VOID
DeviceInitAtapiIds(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PINQUIRYDATA InquiryData
    )
{
    FormatAtapiVendorId(InquiryData,
                        ChannelExtension->DeviceExtension->DeviceParameters.VendorId,
                        sizeof(ChannelExtension->DeviceExtension->DeviceParameters.VendorId)
                        );

    FormatAtapiRevisionId(InquiryData,
                          ChannelExtension->DeviceExtension->DeviceParameters.RevisionId,
                          sizeof(ChannelExtension->DeviceExtension->DeviceParameters.RevisionId)
                          );
    return;
}

VOID
UpdateDeviceParameters(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    Central place to update device related information after device enumeration

    Assumption - IDENTIFY DATA has been retrieved from device

Arguments:

    ChannelExtension -

Return Value:

    None.

--*/
{
    PATA_DEVICE_PARAMETERS deviceParameters = &ChannelExtension->DeviceExtension->DeviceParameters;
    PIDENTIFY_DEVICE_DATA  identifyDeviceData = ChannelExtension->DeviceExtension->IdentifyDeviceData;
    //PINQUIRYDATA           inquiryData = (PINQUIRYDATA)ChannelExtension->DeviceExtension->InquiryData;

    if (IsAtapiDevice(deviceParameters)) {
        deviceParameters->MaximumLun = 0;
        // following two fields are not used for ATAPI device.
        //deviceParameters->ScsiDeviceType = inquiryData->DeviceType;
        //deviceParameters->StateFlags.RemovableMedia = inquiryData->RemovableMedia;

        deviceParameters->MaxDeviceQueueDepth = 1;
        deviceParameters->AddressTranslation = UnknownMode;

    } else {
        // this is an ATA device
        BOOLEAN isBigLba = FALSE;

        deviceParameters->MaximumLun = 0;
        deviceParameters->ScsiDeviceType = DIRECT_ACCESS_DEVICE;
        deviceParameters->StateFlags.RemovableMedia = identifyDeviceData->GeneralConfiguration.RemovableMedia;

        deviceParameters->MaxDeviceQueueDepth = min(ChannelExtension->MaxPortQueueDepth, (UCHAR)identifyDeviceData->QueueDepth);

        if (identifyDeviceData->CommandSetSupport.WriteFua && identifyDeviceData->CommandSetActive.WriteFua) {
          // FUA support
            deviceParameters->StateFlags.FuaSupported = 1;
        }

        // ATA/ATAPI Revision 6.0 or later trust the bit in word 49 in the identify data.
        // by spec this shall be set to 1
        NT_ASSERT(identifyDeviceData->Capabilities.LbaSupported == 0x1);

        // check if it supports 48 bit LBA
        if (identifyDeviceData->CommandSetSupport.BigLba && identifyDeviceData->CommandSetActive.BigLba) {
            LARGE_INTEGER tempLba;

            tempLba.LowPart = identifyDeviceData->Max48BitLBA[0];
            tempLba.HighPart = identifyDeviceData->Max48BitLBA[1];

            // Some disk drives seem to set the above bits but
            // fail to respond to 48 bit LBA commands. So enable
            // 48 bit lba only if it is absolutely necessary
            if (tempLba.QuadPart >= MAX_28BIT_LBA) {
                isBigLba = TRUE;
            }
        }

        if (isBigLba) {
            deviceParameters->AddressTranslation = Lba48BitMode;
        } else {
            deviceParameters->AddressTranslation = LbaMode;
        }

      //2.1 Negotiate NCQ features
        if (IsNCQSupported(ChannelExtension)) {
            if (!IsDumpMode(ChannelExtension->AdapterExtension)) {
                ChannelExtension->StateFlags.NCQ_Activated = 1;
                deviceParameters->AddressTranslation = Lba48BitMode;
                deviceParameters->StateFlags.FuaSupported = 1;
            } else if (IsDumpHiberMode(ChannelExtension->AdapterExtension) && IsDeviceHybridInfoEnabled(ChannelExtension)) {
                // allow NCQ and Hybrid Info conveyed by NCQ Write command during hibernation.
                ChannelExtension->StateFlags.NCQ_Activated = 1;
                ChannelExtension->StateFlags.HybridInfoEnabledOnHiberFile = 1;
                deviceParameters->AddressTranslation = Lba48BitMode;
                deviceParameters->StateFlags.FuaSupported = 1;
            }
        }

        //
        // Fill some firmware support information.
        //
        if (identifyDeviceData->MinBlocksPerDownloadMicrocodeMode03 > 0) {
            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMinTransferBlocks = (USHORT)min(identifyDeviceData->MinBlocksPerDownloadMicrocodeMode03, AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE);
        } else {
            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMinTransferBlocks = 1;
        }

        if (identifyDeviceData->MaxBlocksPerDownloadMicrocodeMode03 > 0) {
            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMaxTransferBlocks = (USHORT)min(identifyDeviceData->MaxBlocksPerDownloadMicrocodeMode03, AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE);
        } else {
            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMaxTransferBlocks = AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE;
        }
    }

    DeviceInitAtaIds(ChannelExtension, identifyDeviceData);

    SelectDeviceGeometry(ChannelExtension, deviceParameters, identifyDeviceData);

    return;
}

BOOLEAN
AdapterProcessIOCTL(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine processes SRB_IO_CONTROL for adapter.

Arguments:

    AdapterExtension - 
    Srb - 

Return Value:

    TRUE if the request is processed in this routine.

--*/
{
    BOOLEAN         processed = FALSE;
    PSRB_IO_CONTROL srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);

    switch (srbControl->ControlCode) {
        case IOCTL_STORAGE_QUERY_PROPERTY:
            if (CompareId(IOCTL_MINIPORT_SIGNATURE_QUERY_PHYSICAL_TOPOLOGY,
                          8,
                          (PSTR)srbControl->Signature,
                          8,
                          NULL)) {

                QueryPhysicalTopologyIoctlProcess(AdapterExtension, Srb);
                processed = TRUE;
            }

            break;

        default:
            break;
    }

    return processed;
}

ULONG
IOCTLtoATA(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*
    Note: If there is a need to send a command to device,
          the translation routine shall set appropriate value to srbExtension->AtaFunction.
            For example: srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
          Not setting this field can cause the Srb being completed earlier than expected.
*/
{
    PSRB_IO_CONTROL srbControl;
    ULONG           status;
    PVOID           srbDataBuffer = SrbGetDataBuffer(Srb);
    ULONG           srbDataBufferLength = SrbGetDataTransferLength(Srb);

    srbControl = (PSRB_IO_CONTROL)srbDataBuffer;
    status = STOR_STATUS_SUCCESS;

    switch (srbControl->ControlCode) {
        case IOCTL_SCSI_MINIPORT_SMART_VERSION:

            status = SmartVersion (ChannelExtension, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_IDENTIFY:

            status = SmartIdentifyData (ChannelExtension, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS:
        case IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS:
        case IOCTL_SCSI_MINIPORT_ENABLE_SMART:
        case IOCTL_SCSI_MINIPORT_DISABLE_SMART:
        case IOCTL_SCSI_MINIPORT_RETURN_STATUS:
        case IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE:
        case IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES:
        case IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS:
        case IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE:
        case IOCTL_SCSI_MINIPORT_READ_SMART_LOG:
        case IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG:

            status = SmartGeneric (ChannelExtension, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_NVCACHE: {

            // general NVCACHE parameter validation
            PNVCACHE_REQUEST_BLOCK      nvCacheRequest;
            
            if ( srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(NVCACHE_REQUEST_BLOCK)) ) {
                Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
                status = STOR_STATUS_BUFFER_TOO_SMALL;
                break;
            }

            nvCacheRequest = (PNVCACHE_REQUEST_BLOCK)(srbControl + 1);

            if ( ((srbDataBufferLength - sizeof(SRB_IO_CONTROL) - sizeof(NVCACHE_REQUEST_BLOCK)) < nvCacheRequest->DataBufSize) || 
                 (nvCacheRequest->DataBufSize > AHCI_MAX_TRANSFER_LENGTH) ) {

                nvCacheRequest->NRBStatus = NRB_INVALID_PARAMETER;
                Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
                status = STOR_STATUS_INVALID_PARAMETER;
                break;
            }

            // process NVCACHE request according to function
            switch (nvCacheRequest->Function) {


            default:
                status = NVCacheGeneric (ChannelExtension, Srb);
                break;

            }

            break;
        }



        case IOCTL_SCSI_MINIPORT_DSM_GENERAL:
            status = DsmGeneralIoctlProcess(ChannelExtension, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_HYBRID:
            status = HybridIoctlProcess(ChannelExtension, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_FIRMWARE:
            status = FirmwareIoctlProcess(ChannelExtension, Srb);
            break;

        case IOCTL_STORAGE_QUERY_PROPERTY:
            if (CompareId(IOCTL_MINIPORT_SIGNATURE_QUERY_PROTOCOL,
                          8,
                          (PSTR)srbControl->Signature,
                          8,
                          NULL)) {
                status = QueryProtocolInfoIoctlProcess(ChannelExtension, Srb);
            } else if (CompareId(IOCTL_MINIPORT_SIGNATURE_QUERY_TEMPERATURE,
                                 8,
                                 (PSTR)srbControl->Signature,
                                 8,
                                 NULL)) {
                status = QueryTemperatureInfoIoctlProcess(ChannelExtension, Srb);
            } else if (CompareId(IOCTL_MINIPORT_SIGNATURE_QUERY_PHYSICAL_TOPOLOGY,
                                 8,
                                 (PSTR)srbControl->Signature,
                                 8,
                                 NULL)) {
                status = QueryPhysicalTopologyIoctlProcess(ChannelExtension->AdapterExtension, Srb);
            } else {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                status = STOR_STATUS_INVALID_PARAMETER;
            }
            break;

        default:

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            break;
    }

    return status;
}

ULONG
SmartVersion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PGETVERSIONINPARAMS versionParameters;

    if (sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS) > SrbGetDataTransferLength(Srb)) {
        NT_ASSERT(FALSE);
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    }

    versionParameters = (PGETVERSIONINPARAMS)(((PUCHAR)SrbGetDataBuffer(Srb)) + sizeof(SRB_IO_CONTROL));

    //
    // Version and revision per SMART 1.03
    //
    versionParameters->bVersion = 1;
    versionParameters->bRevision = 1;
    versionParameters->bReserved = 0;

    //
    // Indicate that support for IDE IDENTIFY, ATAPI IDENTIFY and SMART commands.
    //
    versionParameters->fCapabilities = (CAP_ATA_ID_CMD |
                                        CAP_ATAPI_ID_CMD |
                                        CAP_SMART_CMD);

    //
    // the bIDEDeviceMap is a bit map, with the bits defined as follows
    // bit 0 - IDE drive as device0 on Primary channel
    // bit 1 - IDE drive as device1 on Primary channel
    // bit 2 - IDE drive as device0 on Secondary channel
    // bit 3 - IDE drive as device1 on Secondary Channel
    // bit 4 - ATAPI drive as device0 on Primary Channel
    // bit 5 - ATAPI drive as device1 on Primary Channel
    // bit 6 - ATAPI drive as device0 on secondary Channel
    // bit 7 - ATAPI drive as device1 on secondary Channel
    //
    // since this doesn't apply to SATA, we can only fill in the fields pertinent to this channel.
    //

    if (IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        versionParameters->bIDEDeviceMap = (1 << 4);
    } else {
        versionParameters->bIDEDeviceMap = (1 << 0);
    }

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STOR_STATUS_SUCCESS;
}

BOOLEAN
FillClippedSGL(
    _In_    PSTOR_SCATTER_GATHER_LIST SourceSgl,
    _Inout_ PSTOR_SCATTER_GATHER_LIST LocalSgl,
    _In_    ULONG BytesLeft,
    _In_    ULONG BytesNeeded
    )
/*
    This routine cuts the beginning 'BytesLeft' from 'SourceSgl' and copy the left ranges to 'LocalSgl'.

        BytesLeft - the bytes count from starting of SourceSgl that should not be used.

        BytesNeeded - the number of bytes to be needed for data transfer.

    This routine is typically called by IOCTL to ATA command translation, which the buffer contains some control information at the beginning.
    The real buffer for data transfer is following the control information.
*/
{
    ULONG i, j;

    if ( (SourceSgl == NULL) || (LocalSgl == NULL) || (BytesNeeded == 0) ) {
        return FALSE;
    }

    j = 0;
    for (i = 0; (i < SourceSgl->NumberOfElements) && (BytesNeeded > 0); i++) {
        if (BytesLeft > 0 ) {
            if (BytesLeft < SourceSgl->List[i].Length) {
                //Shrink this element
                LocalSgl->List[j].PhysicalAddress.LowPart = SourceSgl->List[i].PhysicalAddress.LowPart + BytesLeft;
                LocalSgl->List[j].PhysicalAddress.HighPart = SourceSgl->List[i].PhysicalAddress.HighPart;
                LocalSgl->List[j].Length = SourceSgl->List[i].Length - BytesLeft;
                if (LocalSgl->List[j].Length < BytesNeeded) {
                    //Not done. Need more elements.
                    BytesNeeded -= LocalSgl->List[j].Length;
                } else {
                    //Done! Get enough elements.
                    LocalSgl->List[j].Length = BytesNeeded;
                    BytesNeeded = 0;
                }
                BytesLeft = 0;
                j++;
                
            } else if (BytesLeft == SourceSgl->List[i].Length) {
                //Done! Cut off this element
                BytesLeft = 0;
            } else {
                //notDone.  Cut off this element and shrink bytesLeft
                BytesLeft = BytesLeft - SourceSgl->List[i].Length;
            }
        } else {
          //no modification necessary.  copy straight over.
            LocalSgl->List[j].PhysicalAddress.LowPart = SourceSgl->List[i].PhysicalAddress.LowPart;
            LocalSgl->List[j].PhysicalAddress.HighPart = SourceSgl->List[i].PhysicalAddress.HighPart;
            LocalSgl->List[j].Length = SourceSgl->List[i].Length;
            if (LocalSgl->List[j].Length < BytesNeeded) {
                //Not done. Need more elements.
                BytesNeeded -= LocalSgl->List[j].Length;
            } else {
                //Done! Get enough elements.
                LocalSgl->List[j].Length = BytesNeeded;
                BytesNeeded = 0;
            }
            j++;
        }
    }

    //record the number of elements left
    LocalSgl->NumberOfElements = j;

    if ((LocalSgl->NumberOfElements > 33) || (BytesLeft != 0) || (BytesNeeded != 0)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

ULONG
SmartIdentifyData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PSENDCMDOUTPARAMS   outParams;
    PAHCI_SRB_EXTENSION srbExtension;
    PUCHAR              buffer;     //to make the pointer arithmatic easier
    ULONG               srbDataBufferLength = SrbGetDataTransferLength(Srb);

    buffer = (PUCHAR)SrbGetDataBuffer(Srb) + sizeof(SRB_IO_CONTROL);
#pragma warning (suppress: 28930) // Temporarily suppress warning due to OACR false positive.
    outParams = (PSENDCMDOUTPARAMS)buffer;
    
    if ( srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(IDENTIFY_DEVICE_DATA)) ) {
        NT_ASSERT(FALSE);
        if ( srbDataBufferLength >= sizeof(SRB_IO_CONTROL) + RTL_SIZEOF_THROUGH_FIELD(SENDCMDOUTPARAMS, DriverStatus) ) {
            outParams->DriverStatus.bDriverError = SMART_INVALID_BUFFER;
            outParams->DriverStatus.bIDEError = 0;
        }
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    }

    //ATAPI devices cannot support SMART (see ATA8-ACS2 r2 Section 7.19 Table 40 word 82 bit 0)
    if (IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        NT_ASSERT(FALSE);
        outParams->DriverStatus.bDriverError = SMART_INVALID_DRIVE;
        outParams->DriverStatus.bIDEError = 0;
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_DEVICE_REQUEST;
    }
  //1 Fills in the local SGL
    srbExtension = GetSrbExtension(Srb);
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_IDENTIFY;
    srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    srbExtension->CompletionRoutine = AhciPortSmartCompletion;

    //setup TaskFile
    AhciZeroMemory((PCHAR) &srbExtension->TaskFile, sizeof(ATA_TASK_FILE));
    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0;
    srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_IDENTIFY;

    if (! FillClippedSGL( StorPortGetScatterGatherList(ChannelExtension->AdapterExtension, (PSCSI_REQUEST_BLOCK)Srb),
                          (PSTOR_SCATTER_GATHER_LIST) &srbExtension->LocalSgl,
                          sizeof(SRB_IO_CONTROL) + (sizeof(SENDCMDOUTPARAMS) - 1),
                          sizeof(IDENTIFY_DEVICE_DATA) ) ) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    } else {
        srbExtension->Sgl = &srbExtension->LocalSgl;
        srbExtension->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
    }

    Srb->SrbStatus = SRB_STATUS_PENDING;
    return STOR_STATUS_SUCCESS;
}

ULONG
SmartGeneric(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PSENDCMDOUTPARAMS           outParams;
    PSENDCMDINPARAMS            inParams;
    PAHCI_SRB_EXTENSION         srbExtension;
    PUCHAR                      buffer;//to make the pointer arithmatic easier
    ULONG                       srbDataBufferLength;

    buffer = (PUCHAR)SrbGetDataBuffer(Srb) + sizeof(SRB_IO_CONTROL);

    inParams  = (PSENDCMDINPARAMS ) buffer;
    outParams = (PSENDCMDOUTPARAMS) buffer;

    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    if (inParams->irDriveRegs.bCommandReg == SMART_CMD) {
        srbExtension = GetSrbExtension(Srb);

        //
        // Since at miniport driver layer, we have a single buffer represents both inputBuffer and outputBuffer,
        // we need to validate buffer size for both cases.
        //
        if ((srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1)) ||
            (srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1))) {

            if (srbDataBufferLength >= (sizeof(SRB_IO_CONTROL) + RTL_SIZEOF_THROUGH_FIELD(SENDCMDOUTPARAMS, DriverStatus))) {
                outParams->DriverStatus.bDriverError = SMART_INVALID_BUFFER;
                outParams->DriverStatus.bIDEError = 0;
            }

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            return STOR_STATUS_BUFFER_TOO_SMALL;
        }


        switch (inParams->irDriveRegs.bFeaturesReg) {
            case READ_ATTRIBUTES:
            case READ_THRESHOLDS:
            case SMART_READ_LOG:

              //Setup the output buffer to hold the data transfered
                //Ensure the PRDT will get set up
                srbExtension->Flags |= ATA_FLAGS_DATA_IN;

                //Create the SGL to use to set up the PRDT
                if (! FillClippedSGL( StorPortGetScatterGatherList(ChannelExtension->AdapterExtension, (PSCSI_REQUEST_BLOCK)Srb),
                                      (PSTOR_SCATTER_GATHER_LIST) &srbExtension->LocalSgl,
                                      sizeof(SRB_IO_CONTROL) + (sizeof(SENDCMDOUTPARAMS) - 1),
                                      srbDataBufferLength - sizeof(SRB_IO_CONTROL) - (sizeof(SENDCMDOUTPARAMS) - 1) ) ) {
                    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                    return STOR_STATUS_BUFFER_TOO_SMALL;
                } else {
                    srbExtension->Sgl = &srbExtension->LocalSgl;
                    srbExtension->DataTransferLength = srbDataBufferLength - sizeof(SRB_IO_CONTROL) - (sizeof(SENDCMDOUTPARAMS) - 1);
                }
                break;

            case SMART_WRITE_LOG:

              //Setup the input buffer with the data transfered
                //Ensure the PRDT will get set up
                srbExtension->Flags |= ATA_FLAGS_DATA_OUT;

                //Create the SGL to use to set up the PRDT
                if (! FillClippedSGL( StorPortGetScatterGatherList(ChannelExtension->AdapterExtension, (PSCSI_REQUEST_BLOCK)Srb),
                                      (PSTOR_SCATTER_GATHER_LIST) &srbExtension->LocalSgl,
                                      sizeof(SRB_IO_CONTROL) + (sizeof(SENDCMDINPARAMS) - 1),
                                      srbDataBufferLength - sizeof(SRB_IO_CONTROL) - (sizeof(SENDCMDINPARAMS) - 1) ) ) {
                    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                    return STOR_STATUS_BUFFER_TOO_SMALL;
                } else {
                    srbExtension->Sgl = &srbExtension->LocalSgl;
                    srbExtension->DataTransferLength = srbDataBufferLength - sizeof(SRB_IO_CONTROL) - (sizeof(SENDCMDINPARAMS) - 1);
                }
                break;

            case RETURN_SMART_STATUS:

                if (srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(ATAREGISTERS))) {
                    if (srbDataBufferLength >= (sizeof(SRB_IO_CONTROL) + RTL_SIZEOF_THROUGH_FIELD(SENDCMDOUTPARAMS, DriverStatus))) {
                        outParams->DriverStatus.bDriverError = SMART_INVALID_BUFFER;
                        outParams->DriverStatus.bIDEError = 0;
                    }
                    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                    return STOR_STATUS_BUFFER_TOO_SMALL;
                }

              //Setup output buffer to receive the return task file
                srbExtension->ResultBuffer = (PVOID)outParams->bBuffer;
                srbExtension->ResultBufferLength = sizeof(ATAREGISTERS);
                srbExtension->Flags |= ATA_FLAGS_RETURN_RESULTS;
              //there is no data transfer.
                break;

            case EXECUTE_OFFLINE_DIAGS:
              //Allow only the non-captive tests, for now.
                if ((inParams->irDriveRegs.bSectorNumberReg == SMART_SHORT_SELFTEST_CAPTIVE) ||
                    (inParams->irDriveRegs.bSectorNumberReg == SMART_EXTENDED_SELFTEST_CAPTIVE)) {

                    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                    return STOR_STATUS_INVALID_DEVICE_REQUEST;
                }
              //there is no data transfer.
                //Nothing to do for these
                break;

            case ENABLE_SMART:
            case DISABLE_SMART:
            case SAVE_ATTRIBUTE_VALUES:
            case ENABLE_DISABLE_AUTOSAVE:
            case ENABLE_DISABLE_AUTO_OFFLINE:
              //there is no data transfer.
                //Nothing to do for these
                break;

            default:
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                return STOR_STATUS_INVALID_DEVICE_REQUEST;
                break;

        }

    } else {
      //only smart commands are supported
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_INVALID_DEVICE_REQUEST;
    }

    //If we made it this far without changing the status, there is an ATA command to set up
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_SMART;
    srbExtension->CompletionRoutine = AhciPortSmartCompletion;

    AhciZeroMemory((PCHAR) &srbExtension->TaskFile, sizeof(ATA_TASK_FILE));
    srbExtension->TaskFile.Current.bFeaturesReg = inParams->irDriveRegs.bFeaturesReg;
    srbExtension->TaskFile.Current.bSectorCountReg = inParams->irDriveRegs.bSectorCountReg;
    srbExtension->TaskFile.Current.bSectorNumberReg = inParams->irDriveRegs.bSectorNumberReg;
    srbExtension->TaskFile.Current.bCylLowReg = inParams->irDriveRegs.bCylLowReg;
    srbExtension->TaskFile.Current.bCylHighReg = inParams->irDriveRegs.bCylHighReg;
    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0 | (inParams->irDriveRegs.bDriveHeadReg & 0x0F);
    srbExtension->TaskFile.Current.bCommandReg = inParams->irDriveRegs.bCommandReg;

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STOR_STATUS_SUCCESS;
}

ULONG
NVCacheGeneric(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PSRB_IO_CONTROL             srbControl;
    PNVCACHE_REQUEST_BLOCK      nRB;
    PAHCI_SRB_EXTENSION         srbExtension;
    ULONGLONG                   tempLBA;
    PNV_FEATURE_PARAMETER       idCacheData;

    ULONG                       status;
    ULONG                       srbDataBufferLength = SrbGetDataTransferLength(Srb);
    ULONG                       srbFlags = SrbGetSrbFlags(Srb);
    PVOID                       resultBuffer = NULL;
            
    srbExtension = GetSrbExtension(Srb);
    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    nRB = ((PNVCACHE_REQUEST_BLOCK) ( (PSRB_IO_CONTROL)srbControl + 1) );

    switch (nRB->Function) {

//feature discovery
        case NRB_FUNCTION_NVCACHE_INFO:
          //Fill in the return _NV_FEATURE_PARAMETER and complete the command
            if ( srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(NVCACHE_REQUEST_BLOCK) + sizeof(NV_FEATURE_PARAMETER)) ) {
                if ( srbDataBufferLength >= (sizeof(SRB_IO_CONTROL) + RTL_SIZEOF_THROUGH_FIELD(NVCACHE_REQUEST_BLOCK, NRBStatus)) ) {
                    nRB->NRBStatus = NRB_INVALID_PARAMETER;
                }
                Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
                return STOR_STATUS_BUFFER_TOO_SMALL;
            }
            idCacheData = (PNV_FEATURE_PARAMETER) ( ( (PNVCACHE_REQUEST_BLOCK)nRB + 1) );
            idCacheData->NVPowerModeEnabled   = ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheCapabilities.NVCachePowerModeEnabled;
            idCacheData->NVCmdEnabled         = ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheCapabilities.NVCacheFeatureSetEnabled;
            idCacheData->NVPowerModeVer       = ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheCapabilities.NVCachePowerModeVersion;
            idCacheData->NVCmdVer             = ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheCapabilities.NVCacheFeatureSetVersion;
            idCacheData->NVSize               = ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheSizeMSW;
            idCacheData->NVSize <<= 16;
            idCacheData->NVSize               += ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheSizeLSW;
            idCacheData->NVReadSpeed          = 0;  // this field doesn't exist in ATA spec.
            idCacheData->NVWrtSpeed           = 0;  // this field doesn't exist in ATA spec.
            idCacheData->DeviceSpinUpTime     = ChannelExtension->DeviceExtension->IdentifyDeviceData->NVCacheOptions.NVCacheEstimatedTimeToSpinUpInSeconds;
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            nRB->NRBStatus = NRB_SUCCESS;
            return STOR_STATUS_SUCCESS;
            break;
//non data
        case NRB_FUNCTION_SPINDLE_STATUS:
        case NRB_FUNCTION_NVCACHE_POWER_MODE_SET:
        case NRB_FUNCTION_NVCACHE_POWER_MODE_RETURN:
            //Make sure this is not a data transfer.
            srbFlags &= ~(SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT);
            SrbSetSrbFlags(Srb, srbFlags);

            srbExtension->Flags &= ~(SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT);

            break;

//data transfer
        case NRB_FUNCTION_QUERY_PINNED_SET:
        case NRB_FUNCTION_QUERY_CACHE_MISS:
        case NRB_FUNCTION_QUERY_HYBRID_DISK_STATUS:
        case NRB_FUNCTION_FLUSH_NVCACHE:
        case NRB_FUNCTION_ADD_LBAS_PINNED_SET:
        case NRB_FUNCTION_REMOVE_LBAS_PINNED_SET:
        case NRB_FUNCTION_QUERY_ASCENDER_STATUS:
          //Setup the output buffer to hold the data transfered
            //Ensure the PRDT will get set up

            if( (nRB->Function == NRB_FUNCTION_QUERY_PINNED_SET) ||
                (nRB->Function == NRB_FUNCTION_QUERY_CACHE_MISS) ||
                (nRB->Function == NRB_FUNCTION_QUERY_HYBRID_DISK_STATUS) ||
                (nRB->Function == NRB_FUNCTION_QUERY_ASCENDER_STATUS)
                ) {
                srbExtension->Flags |= ATA_FLAGS_DATA_IN;
            } else {
                srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
            }

            if (! FillClippedSGL( StorPortGetScatterGatherList(ChannelExtension->AdapterExtension, (PSCSI_REQUEST_BLOCK)Srb),
                                  (PSTOR_SCATTER_GATHER_LIST) &srbExtension->LocalSgl,
                                  sizeof(SRB_IO_CONTROL) + sizeof(NVCACHE_REQUEST_BLOCK),
                                  srbDataBufferLength - sizeof(SRB_IO_CONTROL) - sizeof(NVCACHE_REQUEST_BLOCK) ) ) {
                if ( srbDataBufferLength >= (sizeof(SRB_IO_CONTROL) + RTL_SIZEOF_THROUGH_FIELD(NVCACHE_REQUEST_BLOCK, NRBStatus)) ) {
                    nRB->NRBStatus = NRB_INVALID_PARAMETER;
                }
                Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
                return STOR_STATUS_BUFFER_TOO_SMALL; //there is no TOO_BIG status
            } else {
                srbExtension->Sgl = &srbExtension->LocalSgl;
                srbExtension->DataTransferLength = srbDataBufferLength - sizeof(SRB_IO_CONTROL) - sizeof(NVCACHE_REQUEST_BLOCK);
            }
            break;

        default:
            if ( srbDataBufferLength >= (sizeof(SRB_IO_CONTROL) + RTL_SIZEOF_THROUGH_FIELD(NVCACHE_REQUEST_BLOCK, NRBStatus)) ) {
                nRB->NRBStatus = NRB_ILLEGAL_REQUEST;
            }

            Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
            return STOR_STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    //
    // Allocate and set SenseInfo buffer pointer - DMA friendly
    //
    status = AhciAllocateDmaBuffer(ChannelExtension->AdapterExtension,
                                   sizeof(ATA_TASK_FILE),
                                   &resultBuffer);

    if ( (status != STOR_STATUS_SUCCESS) ||
         (resultBuffer == NULL) ) {
        //
        // Memory allocation failed
        //
        Srb->SrbStatus = SRB_STATUS_ERROR;
        return STOR_STATUS_INSUFFICIENT_RESOURCES;
    }

    AhciZeroMemory((PCHAR)resultBuffer, sizeof(ATA_TASK_FILE));
    srbExtension->ResultBuffer = resultBuffer;
    srbExtension->ResultBufferLength = sizeof(ATA_TASK_FILE);

    //
    // Make sure to receive the return task file
    //
    srbExtension->Flags |= ATA_FLAGS_RETURN_RESULTS;


    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
    srbExtension->CompletionRoutine = AhciPortNVCacheCompletion;

    AhciZeroMemory((PCHAR) &srbExtension->TaskFile, sizeof(ATA_TASK_FILE));

    srbExtension->TaskFile.Current.bFeaturesReg = (UCHAR) nRB->Function;

    srbExtension->TaskFile.Current.bSectorCountReg = (UCHAR) (0xFF & nRB->Count);
    srbExtension->TaskFile.Previous.bSectorCountReg =(UCHAR) (nRB->Count >> 8);

    tempLBA = nRB->LBA;
    srbExtension->TaskFile.Current.bSectorNumberReg =     (UCHAR) (0xFF & tempLBA);
    tempLBA >>= 8;
    srbExtension->TaskFile.Current.bCylLowReg =           (UCHAR) (0xFF & tempLBA);
    tempLBA >>= 8;
    srbExtension->TaskFile.Current.bCylHighReg =          (UCHAR) (0xFF & tempLBA);
    tempLBA >>= 8;
    srbExtension->TaskFile.Previous.bSectorNumberReg =    (UCHAR) (0xFF & tempLBA);
    tempLBA >>= 8;
    srbExtension->TaskFile.Previous.bCylLowReg =          (UCHAR) (0xFF & tempLBA);
    tempLBA >>= 8;
    srbExtension->TaskFile.Previous.bCylHighReg =         (UCHAR) (0xFF & tempLBA);

    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0;
    srbExtension->TaskFile.Current.bCommandReg = NVC_ATA_NV_CACHE_COMMAND;

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return  STOR_STATUS_SUCCESS;

}


VOID
HybridInfoCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PSRB_IO_CONTROL                     srbControl;
    PHYBRID_REQUEST_BLOCK               hybridRequest;
    PHYBRID_INFORMATION                 hybridInfo;
    PNVCACHE_PRIORITY_LEVEL_DESCRIPTOR  priorityLevel;
    PGP_LOG_HYBRID_INFORMATION          logPage;
    PAHCI_SRB_EXTENSION                 srbExtension;

    ULONG   bufferLeftSize;
    int     i;

    srbExtension = GetSrbExtension(Srb);

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);

    hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);
    hybridInfo = (PHYBRID_INFORMATION)((PUCHAR)srbControl + hybridRequest->DataBufferOffset);

    logPage = (PGP_LOG_HYBRID_INFORMATION)srbExtension->DataBuffer,

    NT_ASSERT(IsDeviceHybridInfoSupported(ChannelExtension));

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        //
        // Log ETW event about Hybrid Get Information.
        //
        if (ChannelExtension->AdapterExtension->TracingEnabled) {
            StorPortEtwEvent8(ChannelExtension->AdapterExtension,
                              (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                              AhciEtwEventUnitHybridGetInfo,
                              L"Hybrid Get Info",
                              STORPORT_ETW_EVENT_KEYWORD_IO,
                              StorportEtwLevelInformational,
                              StorportEtwEventOpcodeInfo,
                              (PSCSI_REQUEST_BLOCK)Srb,
                              L"Srb Status",
                              Srb->SrbStatus,
                              L"Hybrid Disk",
                              TRUE,
                              L"NvCache Size",
                              0,
                              L"NvCache Levels",
                              0,
                              L"NvCache Status",
                              0,
                              L"MaxLevelBehavior",
                              0,
                              L"D_ThresholdLow",
                              0,
                              L"D_ThresholdHigh",
                              0);
        }

        AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, ATA_BLOCK_SIZE, srbExtension->DataBuffer);
        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Hybrid Info log read failed. \n", ChannelExtension->PortNumber);
        return;
    }

    //
    // Cache Hybrid information.
    //
    StorPortCopyMemory(&ChannelExtension->DeviceExtension->HybridInfo, logPage, sizeof(GP_LOG_HYBRID_INFORMATION_HEADER));

    //
    // Fill data into output buffer.
    //
    AhciZeroMemory((PCHAR)((PUCHAR)srbControl + hybridRequest->DataBufferOffset), hybridRequest->DataBufferLength);

    hybridInfo->Version = HYBRID_REQUEST_INFO_STRUCTURE_VERSION;
    hybridInfo->Size = sizeof(HYBRID_INFORMATION);
    hybridInfo->HybridSupported = IsDeviceHybridInfoSupported(ChannelExtension);

    if (logPage->Header.Enabled == HYBRID_INFORMATION_DISABLED) {
        hybridInfo->Status = NvCacheStatusDisabled;
    } else if (logPage->Header.Enabled == HYBRID_INFORMATION_DISABLE_IN_PROCESS) {
        hybridInfo->Status = NvCacheStatusDisabling;
    } else if (logPage->Header.Enabled == HYBRID_INFORMATION_ENABLED) {
        hybridInfo->Status = NvCacheStatusEnabled;
    } else {
        NT_ASSERTMSG("StorAHCI: Hardware issue - Enabled value from Hybrid Information log should be 0x00 or 0x80, or 0xFF.\n", FALSE);
    }

    hybridInfo->CacheTypeEffective = NvCacheTypeWriteBack;
    hybridInfo->CacheTypeDefault = NvCacheTypeWriteBack;
    hybridInfo->FractionBase = 0xFF;
    hybridInfo->CacheSize = logPage->Header.NVMSize;

    hybridInfo->Attributes.WriteCacheChangeable = ChannelExtension->DeviceExtension->SupportedCommands.HybridControl;
    hybridInfo->Attributes.WriteThroughIoSupported = ChannelExtension->DeviceExtension->SupportedCommands.HybridEvict;
    hybridInfo->Attributes.FlushCacheSupported = ChannelExtension->DeviceExtension->SupportedCommands.HybridControl;
    hybridInfo->Attributes.Removable = FALSE;


    hybridInfo->Priorities.PriorityLevelCount = logPage->Header.MaximumHybridPriorityLevel + 1;
    hybridInfo->Priorities.MaxPriorityBehavior = logPage->Header.SupportedOptions.MaximumPriorityBehavior;
    hybridInfo->Priorities.OptimalWriteGranularity = logPage->Header.OptimalWriteGranularity;

    hybridInfo->Priorities.DirtyThresholdLow = logPage->Header.DirtyLowThreshold;
    hybridInfo->Priorities.DirtyThresholdHigh = logPage->Header.DirtyHighThreshold;

    hybridInfo->Priorities.SupportedCommands.CacheDisable = ChannelExtension->DeviceExtension->SupportedCommands.HybridControl;
    hybridInfo->Priorities.SupportedCommands.SetDirtyThreshold = ChannelExtension->DeviceExtension->SupportedCommands.HybridControl;
    hybridInfo->Priorities.SupportedCommands.PriorityDemoteBySize = ChannelExtension->DeviceExtension->SupportedCommands.HybridDemoteBySize;
    hybridInfo->Priorities.SupportedCommands.PriorityChangeByLbaRange = ChannelExtension->DeviceExtension->SupportedCommands.HybridChangeByLbaRange;
    hybridInfo->Priorities.SupportedCommands.Evict = ChannelExtension->DeviceExtension->SupportedCommands.HybridEvict;

    hybridInfo->Priorities.SupportedCommands.MaxEvictCommands = logPage->Header.MaximumEvictionCommands;
    hybridInfo->Priorities.SupportedCommands.MaxLbaRangeCountForEvict = (ATA_BLOCK_SIZE / sizeof(ATA_LBA_RANGE)) * logPage->Header.MaximumEvictionDataBlocks;

    hybridInfo->Priorities.SupportedCommands.MaxLbaRangeCountForChangeLba = GetHybridMaxLbaRangeCountForChangeLba(ChannelExtension);

    priorityLevel = hybridInfo->Priorities.Priority;
    bufferLeftSize = hybridRequest->DataBufferLength - sizeof(HYBRID_INFORMATION);

    for (i = 0; i < logPage->Header.HybridInfoDescrCount; i++) {
        if (bufferLeftSize >= sizeof(NVCACHE_PRIORITY_LEVEL_DESCRIPTOR)) {
            priorityLevel->PriorityLevel = logPage->Descriptor[i].HybridPriority;
            priorityLevel->ConsumedNVMSizeFraction = logPage->Descriptor[i].ConsumedNVMSizeFraction;
            priorityLevel->ConsumedMappingResourcesFraction = logPage->Descriptor[i].ConsumedMappingResourcesFraction;
            priorityLevel->ConsumedNVMSizeForDirtyDataFraction = logPage->Descriptor[i].ConsumedNVMSizeForDirtyDataFraction;
            priorityLevel->ConsumedMappingResourcesForDirtyDataFraction = logPage->Descriptor[i].ConsumedMappingResourcesForDirtyDataFraction;

            //
            // Update pointer and left buffer size
            //
            priorityLevel = priorityLevel + 1;
            bufferLeftSize -= sizeof(NVCACHE_PRIORITY_LEVEL_DESCRIPTOR);
        } else {
            break;
        }
    }

    if (i < logPage->Header.HybridInfoDescrCount) {
        srbControl->ReturnCode = HYBRID_STATUS_OUTPUT_BUFFER_TOO_SMALL;
        hybridRequest->DataBufferLength = sizeof(HYBRID_INFORMATION) + sizeof(NVCACHE_PRIORITY_LEVEL_DESCRIPTOR) * logPage->Header.HybridInfoDescrCount;

        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    }

    SrbSetDataTransferLength(Srb, hybridRequest->DataBufferOffset + hybridRequest->DataBufferLength);

    //
    // Log ETW event about Hybrid Information.
    //
    if (ChannelExtension->AdapterExtension->TracingEnabled) {
        StorPortEtwEvent8(ChannelExtension->AdapterExtension,
                          (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                          AhciEtwEventUnitHybridGetInfo,
                          L"Hybrid Get Info",
                          STORPORT_ETW_EVENT_KEYWORD_IO,
                          StorportEtwLevelInformational,
                          StorportEtwEventOpcodeInfo,
                          (PSCSI_REQUEST_BLOCK)Srb,
                          L"Srb Status",
                          Srb->SrbStatus,
                          L"Hybrid Disk",
                          TRUE,
                          L"NvCache Size",
                          hybridInfo->CacheSize,
                          L"NvCache Levels",
                          hybridInfo->Priorities.PriorityLevelCount,
                          L"NvCache Status",
                          logPage->Header.Enabled,
                          L"MaxLevelBehavior",
                          hybridInfo->Priorities.MaxPriorityBehavior,
                          L"D_ThresholdLow",
                          hybridInfo->Priorities.DirtyThresholdLow,
                          L"D_ThresholdHigh",
                          hybridInfo->Priorities.DirtyThresholdHigh);
    }

    AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, ATA_BLOCK_SIZE, srbExtension->DataBuffer);

    StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Hybrid Info log read successfully. \n", ChannelExtension->PortNumber);


    return;
}

ULONG
HybridGetInfo(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes HYBRID request - Get Info.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    ULONG                   status;
    PSRB_IO_CONTROL         srbControl;
    PHYBRID_REQUEST_BLOCK   hybridRequest;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);

    //
    // Validate buffer length
    //
    if (hybridRequest->DataBufferOffset < (sizeof(SRB_IO_CONTROL) +  sizeof(HYBRID_REQUEST_BLOCK))) {
        //
        // Output buffer should be after these two data structures.
        //
        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    if ((SrbGetDataTransferLength(Srb) < (sizeof(SRB_IO_CONTROL) + sizeof(HYBRID_REQUEST_BLOCK) + sizeof(HYBRID_INFORMATION))) ||
        (hybridRequest->DataBufferLength < sizeof(HYBRID_INFORMATION))) {
        //
        // Output buffer after HYBRID_REQUEST_BLOCK should be at least with length of sizeof(HYBRID_INFORMATION).
        //
        srbControl->ReturnCode = HYBRID_STATUS_OUTPUT_BUFFER_TOO_SMALL;
        hybridRequest->DataBufferLength = sizeof(HYBRID_INFORMATION);

        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        status = STOR_STATUS_BUFFER_TOO_SMALL;

        goto Exit;
    }

    if (!IsDeviceHybridInfoSupported(ChannelExtension)) {
        //
        // If Hybrid Information is not supported, complete the request with "HybridSupported" field set to "FALSE".
        //
        PHYBRID_INFORMATION hybridInfo;

        hybridInfo = (PHYBRID_INFORMATION)((PUCHAR)srbControl + hybridRequest->DataBufferOffset);

        hybridInfo->Version = HYBRID_REQUEST_INFO_STRUCTURE_VERSION;
        hybridInfo->Size = sizeof(HYBRID_INFORMATION);
        hybridInfo->HybridSupported = FALSE;

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STOR_STATUS_SUCCESS;

        goto Exit;

    } else {
        PVOID                   logPageBuffer = NULL;
        STOR_PHYSICAL_ADDRESS   logPagePhysialAddress;
        ULONG                   tempLength;

        //
        // We need to allocate a new data buffer for log page
        //
        status = AhciAllocateDmaBuffer(ChannelExtension->AdapterExtension, ATA_BLOCK_SIZE, &logPageBuffer);

        if ( (status != STOR_STATUS_SUCCESS) || (logPageBuffer == NULL) ) {
            //
            // memory allocation failed
            //
            Srb->SrbStatus = SRB_STATUS_ERROR;
            status = STOR_STATUS_INSUFFICIENT_RESOURCES;

            goto Exit;
        }

        AhciZeroMemory((PCHAR)logPageBuffer, ATA_BLOCK_SIZE);
        logPagePhysialAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, (PVOID)logPageBuffer, &tempLength);

        //
        // Note that logPageBuffer will be released in completion routine.
        //
        IssueReadLogExtCommand( ChannelExtension,
                                Srb,
                                IDE_GP_LOG_HYBRID_INFO_ADDRESS,
                                0,
                                1,
                                0,      // feature field
                                &logPagePhysialAddress,
                                logPageBuffer,
                                HybridInfoCompletion
                                );
    }

Exit:

    //
    // Log ETW event about Hybrid Get Information.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent8(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridGetInfo,
                            L"Hybrid Get Info",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Hybrid Disk",
                            IsDeviceHybridInfoSupported(ChannelExtension) ? TRUE : FALSE,
                            L"NvCache Size",
                            0,
                            L"NvCache Levels",
                            0,
                            L"NvCache Status",
                            0,
                            L"MaxLevelBehavior",
                            0,
                            L"D_ThresholdLow",
                            0,
                            L"D_ThresholdHigh",
                            0);
    }

    return status;
}

VOID
HybridControlCachingMediumCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    BOOLEAN             IsEnableRequest;

    //
    // This completion routine is only used by HybridDisableCachingMedium() and HybridEnableCachingMedium().
    //

    IsEnableRequest = ((srbExtension->Cfis.Command == IDE_COMMAND_SET_FEATURE) &&
                       (srbExtension->Cfis.Feature7_0 == IDE_FEATURE_ENABLE_SATA_FEATURE) &&
                       (srbExtension->Cfis.SectorCount == IDE_SATA_FEATURE_HYBRID_INFORMATION));

    //
    // Log ETW event about Hybrid Caching Medium Enable/Disable.
    //
    if (ChannelExtension->AdapterExtension->TracingEnabled) {
        if (IsEnableRequest) {
            StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                                (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                AhciEtwEventUnitHybridCachingMediumEnable,
                                L"Hybrid Caching Medium Enable",
                                STORPORT_ETW_EVENT_KEYWORD_IO,
                                StorportEtwLevelInformational,
                                StorportEtwEventOpcodeInfo,
                                (PSCSI_REQUEST_BLOCK)Srb,
                                L"Srb Status",
                                Srb->SrbStatus,
                                L"Device CMD Sent",
                                TRUE,
                                L"Post-RefCount",
                                ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs,
                                NULL,
                                0);
        } else {
            StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                                (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                AhciEtwEventUnitHybridCachingMediumDisable,
                                L"Hybrid Caching Medium Disable",
                                STORPORT_ETW_EVENT_KEYWORD_IO,
                                StorportEtwLevelInformational,
                                StorportEtwEventOpcodeInfo,
                                (PSCSI_REQUEST_BLOCK)Srb,
                                L"Srb Status",
                                Srb->SrbStatus,
                                L"Device CMD Sent",
                                TRUE,
                                L"Post-RefCount",
                                ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs,
                                NULL,
                                0);
        }
    }

    //
    // Command failed, bail out.
    //
    if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
        return;
    }

    //
    // Update cached data based on command completion status. This is a work around for not sending an IDENTIFY DEVICE command.
    //
    if (IsEnableRequest) {

        ChannelExtension->DeviceExtension[0].IdentifyDeviceData->SerialAtaFeaturesEnabled.HybridInformation = 1;

        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Caching Medium Enabled. \n", ChannelExtension->PortNumber);
    } else {
        //
        // Caching medium might be in Disabling state at this time.
        // Updating following value now to refresh storahci validation for future hybrid request.
        //
        ChannelExtension->DeviceExtension[0].IdentifyDeviceData->SerialAtaFeaturesEnabled.HybridInformation = 0;

        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Caching Medium is being Disabled. \n", ChannelExtension->PortNumber);
    }

    return;
}

__inline
VOID
BuildHybridControlDisableCacheCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS
    )
/*++
Routine Description:

    Build Hybrid Control command to Disable Caching Medium.

Arguments:
    CFIS - the buffer should be zero-ed before calling this function.

Return Value:

    None

--*/
{
    CFIS->Feature7_0 = IDE_NCQ_NON_DATA_HYBRID_CONTROL;
    CFIS->Feature7_0 |= 0x80;       // Disable Caching Medium bit.

    CFIS->Device |= (1 << 6);
    CFIS->Command = IDE_COMMAND_NCQ_NON_DATA;
}


ULONG
HybridDisableCachingMedium(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes HYBRID request - Disable Caching Medium.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    PSRB_IO_CONTROL         srbControl;
    PAHCI_SRB_EXTENSION     srbExtension;
    LONG                    hybridCachingMediumEnableRefs;
    ULONG                   status = STOR_STATUS_SUCCESS;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbExtension = GetSrbExtension(Srb);

    //
    // Fail the request if NCQ is not supported or hybrid information feature is not supported.
    //
    if (!IsNCQSupported(ChannelExtension) ||
        (ChannelExtension->StateFlags.NCQ_Activated == 0) ||
        !IsDeviceHybridInfoSupported(ChannelExtension) ) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Fail the request if command is not supported.
    //
    if (ChannelExtension->DeviceExtension->SupportedCommands.HybridControl == 0) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If Hybrid feature is disabled, complete the call as success.
    //
    if (!IsDeviceHybridInfoEnabled(ChannelExtension)) {
        NT_ASSERT(ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs == 0);

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Caching Medium is already disabled. No command sent to disk. \n", ChannelExtension->PortNumber);
        status = STOR_STATUS_SUCCESS;

        goto Exit;
    }

    //
    // If Hybrid feature is enabled and refcount is more than 1, complete the call.
    //
    hybridCachingMediumEnableRefs = InterlockedDecrement(&ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs);

    if (hybridCachingMediumEnableRefs > 0) {
        srbControl->ReturnCode = HYBRID_STATUS_ENABLE_REFCOUNT_HOLD;
        Srb->SrbStatus = SRB_STATUS_ABORTED;
        status = STOR_STATUS_UNSUCCESSFUL;

        goto Exit;
    }

    //
    // Add count back if it was 0 (or negative value) before decrement.
    //
    if (hybridCachingMediumEnableRefs < 0) {
        InterlockedIncrement(&ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs);
    }

    //
    // Build command into CFIS data structure.
    //
    BuildHybridControlDisableCacheCommand(&srbExtension->Cfis);

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;

    //
    // Note that if a synchronized behavior is wanted, the caller should check Hybrid Information log (or identify device data),
    // until the caching medium is in disabled state (or hybrid information feature is disabled).
    //
    srbExtension->CompletionRoutine = HybridControlCachingMediumCompletion;

    status = STOR_STATUS_SUCCESS;

Exit:

    //
    // Log ETW event about Hybrid Caching Medium Disable.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridCachingMediumDisable,
                            L"Hybrid Caching Medium Disable",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            FALSE,
                            L"Post-RefCount",
                            ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs,
                            NULL,
                            0);
    }

    return status;
}

__inline
VOID
BuildEnableFeatureHybridInfoCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS
    )
/*++
Routine Description:

    Build Set Feature command to enable Hybrid Information feature.

Arguments:
    CFIS - the buffer should be zero-ed before calling this function.

Return Value:

    None

--*/
{
    CFIS->Feature7_0 = IDE_FEATURE_ENABLE_SATA_FEATURE;
    CFIS->SectorCount = IDE_SATA_FEATURE_HYBRID_INFORMATION;

    CFIS->Command = IDE_COMMAND_SET_FEATURE;
}

ULONG
HybridEnableCachingMedium(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes HYBRID request - Enable Caching Medium.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    PSRB_IO_CONTROL         srbControl;
    PAHCI_SRB_EXTENSION     srbExtension;
    ULONG                   status = STOR_STATUS_SUCCESS;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbExtension = GetSrbExtension(Srb);

    //
    // Fail the request if NCQ is not supported, or hybrid information feature is not supported.
    //
    if (!IsNCQSupported(ChannelExtension) ||
        (ChannelExtension->StateFlags.NCQ_Activated == 0) ||
        !IsDeviceHybridInfoSupported(ChannelExtension)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Fail the request if command is not supported.
    //
    if (ChannelExtension->DeviceExtension->SupportedCommands.HybridControl == 0) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    InterlockedIncrement(&ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs);

    //
    // If Hybrid feature is enabled, complete the call as success.
    //
    if (IsDeviceHybridInfoEnabled(ChannelExtension)) {

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Caching Medium is already enabled. No command sent to disk. \n", ChannelExtension->PortNumber);
        status = STOR_STATUS_SUCCESS;

        goto Exit;
    }

    //
    // Build command into CFIS data structure.
    //
    BuildEnableFeatureHybridInfoCommand(&srbExtension->Cfis);

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;
    srbExtension->CompletionRoutine = HybridControlCachingMediumCompletion;

    status = STOR_STATUS_SUCCESS;

Exit:

    //
    // Log ETW event about Hybrid Caching Medium Enable.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridCachingMediumEnable,
                            L"Hybrid Caching Medium Enable",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            FALSE,
                            L"Post-RefCount",
                            ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs,
                            NULL,
                            0);
    }

    return status;
}

VOID
HybridSetThresholdCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    //
    // Log ETW event about Hybrid Set Dirty Threshold.
    //
    if (ChannelExtension->AdapterExtension->TracingEnabled) {
        PSRB_IO_CONTROL             srbControl;
        PHYBRID_REQUEST_BLOCK       hybridRequest;
        PHYBRID_DIRTY_THRESHOLDS    hybridDirtyThresholds;

        ULONG                       dirtyLowThreshold = 0;
        ULONG                       dirtyHighThreshold = 0;

        srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
        hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);
        hybridDirtyThresholds = (PHYBRID_DIRTY_THRESHOLDS)((PUCHAR)srbControl + hybridRequest->DataBufferOffset);

        dirtyLowThreshold = hybridDirtyThresholds->DirtyLowThreshold;
        dirtyHighThreshold = hybridDirtyThresholds->DirtyHighThreshold;

        StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridSetDirtyThreshold,
                            L"Hybrid Set Dirty Threshold",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            TRUE,
                            L"D_ThresholdLow",
                            dirtyLowThreshold,
                            L"D_ThresholdHigh",
                            dirtyHighThreshold);
    }

    return;
}


__inline
VOID
BuildHybridControlSetThresholdCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS,
    _In_ UCHAR                  DirtyLowThreshold,
    _In_ UCHAR                  DirtyHighThreshold
    )
/*++
Routine Description:

    Build Hybrid Control command to set Dirty Low and High Threshold.

Arguments:
    CFIS - the buffer should be zero-ed before calling this function.

Return Value:

    None

--*/
{
    CFIS->Feature7_0 = IDE_NCQ_NON_DATA_HYBRID_CONTROL;

    CFIS->LBA7_0 = DirtyLowThreshold;
    CFIS->LBA15_8 = DirtyHighThreshold;

    CFIS->Device |= (1 << 6);
    CFIS->Command = IDE_COMMAND_NCQ_NON_DATA;
}

ULONG
HybridSetDirtyThreshold(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes HYBRID request - Set Dirty Threshold.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    PSRB_IO_CONTROL             srbControl;
    PHYBRID_REQUEST_BLOCK       hybridRequest;
    PHYBRID_DIRTY_THRESHOLDS    hybridDirtyThresholds;
    PAHCI_SRB_EXTENSION         srbExtension;
    ULONG                       status = STOR_STATUS_SUCCESS;

    ULONG                       dirtyLowThreshold = 0;
    ULONG                       dirtyHighThreshold = 0;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);
    srbExtension = GetSrbExtension(Srb);

    if (hybridRequest->DataBufferOffset < (sizeof(SRB_IO_CONTROL) +  sizeof(HYBRID_REQUEST_BLOCK))) {
        //
        // HYBRID_DIRTY_THRESHOLDS should be after these two data structures.
        //
        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    if ((SrbGetDataTransferLength(Srb) < (sizeof(SRB_IO_CONTROL) + sizeof(HYBRID_REQUEST_BLOCK) + sizeof(HYBRID_DIRTY_THRESHOLDS))) ||
        (hybridRequest->DataBufferLength < sizeof(HYBRID_DIRTY_THRESHOLDS))) {
        //
        // Input buffer after HYBRID_REQUEST_BLOCK should be at least with length of sizeof(HYBRID_DIRTY_THRESHOLDS).
        //
        srbControl->ReturnCode = HYBRID_STATUS_OUTPUT_BUFFER_TOO_SMALL;
        hybridRequest->DataBufferLength = sizeof(HYBRID_DIRTY_THRESHOLDS);

        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        status = STOR_STATUS_BUFFER_TOO_SMALL;

        goto Exit;
    }

    hybridDirtyThresholds = (PHYBRID_DIRTY_THRESHOLDS)((PUCHAR)srbControl + hybridRequest->DataBufferOffset);

    dirtyLowThreshold = hybridDirtyThresholds->DirtyLowThreshold;
    dirtyHighThreshold = hybridDirtyThresholds->DirtyHighThreshold;

    if ((dirtyLowThreshold > 255) || (dirtyHighThreshold > 255) ||
        (dirtyLowThreshold > dirtyHighThreshold)) {

        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Fail the request if NCQ is not supported.
    //
    if (!IsNCQSupported(ChannelExtension) ||
        (ChannelExtension->StateFlags.NCQ_Activated == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If device doesn't support Hybrid or the feature is disabled, fail the request.
    //
    if (!IsDeviceHybridInfoSupported(ChannelExtension) ||
        !IsDeviceHybridInfoEnabled(ChannelExtension) ||
        (ChannelExtension->DeviceExtension->SupportedCommands.HybridControl == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Build command into CFIS data structure.
    //
    BuildHybridControlSetThresholdCommand(&srbExtension->Cfis, (UCHAR)dirtyLowThreshold, (UCHAR)dirtyHighThreshold);

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;

    //
    // The completion routine is only used to log ETW event
    //
    if (ChannelExtension->AdapterExtension->TracingEnabled) {
        srbExtension->CompletionRoutine = HybridSetThresholdCompletion;
    }

    StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Set Dirty Threshold, command sent to device. Low: 0x%02X, High: 0x%02X. \n", ChannelExtension->PortNumber,
                            hybridDirtyThresholds->DirtyLowThreshold, hybridDirtyThresholds->DirtyHighThreshold);

    status = STOR_STATUS_SUCCESS;

Exit:

    //
    // Log ETW event about Hybrid Set Dirty Threshold.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridSetDirtyThreshold,
                            L"Hybrid Set Dirty Threshold",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            FALSE,
                            L"D_ThresholdLow",
                            dirtyLowThreshold,
                            L"D_ThresholdHigh",
                            dirtyHighThreshold);
    }

    return status;
}

VOID
HybridDemoteBySizeCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    //
    // Log ETW event about Hybrid Demote By Size.
    //
    if (ChannelExtension->AdapterExtension->TracingEnabled) {
        PSRB_IO_CONTROL         srbControl;
        PHYBRID_REQUEST_BLOCK   hybridRequest;
        PHYBRID_DEMOTE_BY_SIZE  hybridDemoteBySize;

        UCHAR                   sourcePriority = 0;
        UCHAR                   targetPriority = 0;
        ULONGLONG               lbaCount = 0;

        srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
        hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);

        hybridDemoteBySize = (PHYBRID_DEMOTE_BY_SIZE)((PUCHAR)srbControl + hybridRequest->DataBufferOffset);

        sourcePriority = hybridDemoteBySize->SourcePriority;
        targetPriority = hybridDemoteBySize->TargetPriority;
        lbaCount = hybridDemoteBySize->LbaCount;

        StorPortEtwEvent8(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridDemoteBySize,
                            L"Hybrid Demote By Size",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            TRUE,
                            L"Source Priority",
                            sourcePriority,
                            L"Target Priority",
                            targetPriority,
                            L"LBA Count",
                            lbaCount,
                            NULL,
                            0,
                            NULL,
                            0,
                            NULL,
                            0);
    }

    return;
}

__inline
VOID
BuildHybridDemoteBySizeCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS,
    _In_ UCHAR                  SourcePriority,
    _In_ UCHAR                  TargetPriority,
    _In_ ULONG                  LbaCount
    )
/*++
Routine Description:

    Build Hybrid Control command to set Dirty Low and High Threshold.

Arguments:
    CFIS - the buffer should be zero-ed before calling this function.

Return Value:

    None

--*/
{
    ATA_HYBRID_INFO_FIELDS hybridInfo = {0};

    hybridInfo.InfoValid = 1;
    hybridInfo.HybridPriority = TargetPriority;

    CFIS->Feature7_0 = IDE_NCQ_NON_DATA_HYBRID_DEMOTE_BY_SIZE;
    CFIS->Feature7_0 |= (UCHAR)(SourcePriority << 4);

    CFIS->Feature15_8 = (UCHAR)LbaCount;
    CFIS->Count15_8 = (UCHAR)(LbaCount >> 8);
    CFIS->LBA7_0 = (UCHAR)(LbaCount >> 16);
    CFIS->LBA15_8 = (UCHAR)(LbaCount >> 24);

    CFIS->Auxiliary23_16 = hybridInfo.AsUchar;

    CFIS->Device |= (1 << 6);
    CFIS->Command = IDE_COMMAND_NCQ_NON_DATA;
}

ULONG
HybridDemoteBySize(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes HYBRID request - Demote By Size.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    PSRB_IO_CONTROL         srbControl;
    PHYBRID_REQUEST_BLOCK   hybridRequest;
    PHYBRID_DEMOTE_BY_SIZE  hybridDemoteBySize;
    PAHCI_SRB_EXTENSION     srbExtension;
    ULONG                   status = STOR_STATUS_SUCCESS;

    UCHAR                   sourcePriority = 0;
    UCHAR                   targetPriority = 0;
    ULONGLONG               lbaCount = 0;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);
    srbExtension = GetSrbExtension(Srb);

    if (hybridRequest->DataBufferOffset < (sizeof(SRB_IO_CONTROL) +  sizeof(HYBRID_REQUEST_BLOCK))) {
        //
        // HYBRID_DEMOTE_BY_SIZE should be after these two data structures.
        //
        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    if ((SrbGetDataTransferLength(Srb) < (sizeof(SRB_IO_CONTROL) + sizeof(HYBRID_REQUEST_BLOCK) + sizeof(HYBRID_DEMOTE_BY_SIZE))) ||
        (hybridRequest->DataBufferLength < sizeof(HYBRID_DEMOTE_BY_SIZE))) {
        //
        // Input buffer after HYBRID_REQUEST_BLOCK should be at least with length of sizeof(HYBRID_DEMOTE_BY_SIZE).
        //
        srbControl->ReturnCode = HYBRID_STATUS_OUTPUT_BUFFER_TOO_SMALL;
        hybridRequest->DataBufferLength = sizeof(HYBRID_DEMOTE_BY_SIZE);

        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        status = STOR_STATUS_BUFFER_TOO_SMALL;

        goto Exit;
    }

    hybridDemoteBySize = (PHYBRID_DEMOTE_BY_SIZE)((PUCHAR)srbControl + hybridRequest->DataBufferOffset);

    sourcePriority = hybridDemoteBySize->SourcePriority;
    targetPriority = hybridDemoteBySize->TargetPriority;
    lbaCount = hybridDemoteBySize->LbaCount;

    if ((sourcePriority > ChannelExtension->DeviceExtension->HybridInfo.MaximumHybridPriorityLevel) ||
        (targetPriority > ChannelExtension->DeviceExtension->HybridInfo.MaximumHybridPriorityLevel) ||
        (targetPriority >= hybridDemoteBySize->SourcePriority) ||
        (lbaCount == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Fail the request if NCQ is not supported.
    //
    if (!IsNCQSupported(ChannelExtension) ||
        (ChannelExtension->StateFlags.NCQ_Activated == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If device doesn't support Hybrid or the feature is disabled, fail the request.
    //
    if (!IsDeviceHybridInfoSupported(ChannelExtension) ||
        !IsDeviceHybridInfoEnabled(ChannelExtension) ||
        (ChannelExtension->DeviceExtension->SupportedCommands.HybridDemoteBySize == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Build command into CFIS data structure.
    //
    BuildHybridDemoteBySizeCommand(&srbExtension->Cfis,
                                   (UCHAR)sourcePriority,
                                   (UCHAR)targetPriority,
                                   (ULONG)lbaCount);

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;

    //
    // The completion routine is only used to log ETW event
    //
    if (ChannelExtension->AdapterExtension->TracingEnabled) {
        srbExtension->CompletionRoutine = HybridDemoteBySizeCompletion;
    }

    StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Demote by Size, command sent to device. Source Priority: %d, Target Priority: %d, LBA Count: %d. \n", ChannelExtension->PortNumber,
                           (UCHAR)hybridDemoteBySize->SourcePriority, (UCHAR)hybridDemoteBySize->TargetPriority, (ULONG)hybridDemoteBySize->LbaCount);

    status = STOR_STATUS_SUCCESS;

Exit:

    //
    // Log ETW event about Hybrid Demote By Size.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent8(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridDemoteBySize,
                            L"Hybrid Demote By Size",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            FALSE,
                            L"Source Priority",
                            sourcePriority,
                            L"Target Priority",
                            targetPriority,
                            L"LBA Count",
                            lbaCount,
                            NULL,
                            0,
                            NULL,
                            0,
                            NULL,
                            0);
    }

    return status;
}


ULONG
HybridIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL dispatch routine processes HYBRID request from storport

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    ULONG                   status;
    ULONGLONG               srbDataBufferLength;
    PSRB_IO_CONTROL         srbControl;
    PHYBRID_REQUEST_BLOCK   hybridRequest;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    //
    // A Hybrid request must have at least SRB_IO_CONTROL and HYBRID_REQUEST_BLOCK in input buffer.
    //
    if (srbDataBufferLength < ((ULONGLONG)sizeof(SRB_IO_CONTROL) + sizeof(HYBRID_REQUEST_BLOCK))) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    hybridRequest = (PHYBRID_REQUEST_BLOCK)(srbControl + 1);

    if (srbDataBufferLength < ((ULONGLONG)hybridRequest->DataBufferOffset + hybridRequest->DataBufferLength)) {
        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    // process NVCACHE request according to function
    switch (hybridRequest->Function) {

    case HYBRID_FUNCTION_GET_INFO:
        status = HybridGetInfo (ChannelExtension, Srb);
        break;

    case HYBRID_FUNCTION_DISABLE_CACHING_MEDIUM:
        status = HybridDisableCachingMedium (ChannelExtension, Srb);
        break;

    case HYBRID_FUNCTION_ENABLE_CACHING_MEDIUM:
        status = HybridEnableCachingMedium (ChannelExtension, Srb);
        break;

    case HYBRID_FUNCTION_SET_DIRTY_THRESHOLD:
        status = HybridSetDirtyThreshold (ChannelExtension, Srb);
        break;

    case HYBRID_FUNCTION_DEMOTE_BY_SIZE:
        status = HybridDemoteBySize (ChannelExtension, Srb);
        break;

    default:
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        break;

    }

    return status;
}

ULONG
ConvertDataSetRangeToAtaLbaRanges(
    _Inout_ PULONGLONG CurrentRangeStartLba,
    _Inout_ PULONGLONG CurrentRangeLbaCount,
    _In_reads_bytes_(BufferSize) PCHAR DestBuffer,
    _In_ ULONG  BufferSize
    )
/*++

Routine Description:

    Convert current DataSet Range entry to be ATA_LBA_RANGE entries.

    CurrentRangeLbaCount is 64 bits; ATA_LBA_RANGE->SectorCount is 16 bits.
    It's possible that current DataSet Range entry needs multiple ATA_LBA_RANGE entries.

Arguments:

    CurrentRangeStartLba
    CurrentRangeLbaCount
    DestBuffer
    BufferSize

Return Value:

    Count of ATA_LBA_RANGE entries converted.

    NOTE: if CurrentRangeLbaCount does not reach to 0, the conversion for DEVICE_DATA_SET_RANGE entry is not completed.
          Further conversion is needed by calling this function again.

--*/
{
    ULONG           convertedEntryCount = 0;
    PATA_LBA_RANGE  lbaRangeEntry = (PATA_LBA_RANGE)DestBuffer;

    ULONGLONG       dataSetRangeStartLba = *CurrentRangeStartLba;
    ULONGLONG       dataSetRangeLbaCount = *CurrentRangeLbaCount;

    //
    // fill in ATA_LBA_RANGE entries as needed
    //
    while ((dataSetRangeLbaCount > 0) &&
           (convertedEntryCount * sizeof(ATA_LBA_RANGE) < BufferSize)) {

        USHORT  sectorCount;

        if (dataSetRangeLbaCount > MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE) {
            sectorCount = MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE;
        } else {
            sectorCount = (USHORT)dataSetRangeLbaCount;
        }

        lbaRangeEntry[convertedEntryCount].StartSector = dataSetRangeStartLba;
        lbaRangeEntry[convertedEntryCount].SectorCount = sectorCount;

        dataSetRangeStartLba += sectorCount;
        dataSetRangeLbaCount -= sectorCount;

        convertedEntryCount++;
    }

    //
    // update current DataSet Range entry
    //
    *CurrentRangeStartLba = dataSetRangeStartLba;
    *CurrentRangeLbaCount = dataSetRangeLbaCount;

    return convertedEntryCount;
}


__inline
VOID
BuildHybridChangeByLbaCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS,
    _In_ UCHAR                  TargetPriority,
    _In_ ULONGLONG              StartLba,
    _In_ USHORT                 LbaCount,
    _In_ BOOLEAN                CacheBehavior
    )
/*++
Routine Description:

    Build Hybrid Control command to set Dirty Low and High Threshold.

Arguments:
    CFIS - the buffer should be zero-ed before calling this function.

Return Value:

    None

--*/
{
    ATA_HYBRID_INFO_FIELDS hybridInfo = {0};

    hybridInfo.InfoValid = 1;
    hybridInfo.HybridPriority = TargetPriority;

    CFIS->Feature7_0 = IDE_NCQ_NON_DATA_HYBRID_CHANGE_BY_LBA_RANGE;

    //
    // Set value of CB - CacheBehavior bit (bit4) into Features(7:0)
    //
    if (CacheBehavior == TRUE) {
        CFIS->Feature7_0 |= (1 << 4);
    }

    CFIS->Feature15_8 = (UCHAR)LbaCount;
    CFIS->Count15_8 = (UCHAR)(LbaCount >> 8);

    CFIS->LBA7_0 = (UCHAR)(StartLba);
    CFIS->LBA15_8 = (UCHAR)(StartLba >> 8);
    CFIS->LBA23_16 = (UCHAR)(StartLba >> 16);
    CFIS->LBA31_24 = (UCHAR)(StartLba >> 24);
    CFIS->LBA39_32 = (UCHAR)(StartLba >> 32);
    CFIS->LBA47_40 = (UCHAR)(StartLba >> 40);

    CFIS->Auxiliary23_16 = hybridInfo.AsUchar;

    CFIS->Device |= (1 << 6);
    CFIS->Command = IDE_COMMAND_NCQ_NON_DATA;
}

VOID
HybridChangeByLbaCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PAHCI_SRB_EXTENSION             srbExtension = GetSrbExtension(Srb);
    PHYBRID_CHANGE_BY_LBA_CONTEXT   lbaRangeContext = (PHYBRID_CHANGE_BY_LBA_CONTEXT)srbExtension->CompletionContext;

    BOOLEAN             completed = FALSE;
    ATA_LBA_RANGE       ataLbaRange = {0};

    BOOLEAN             tempDataSetRangeConverted = TRUE;
    ULONG               convertedEntryCount = 0;
    ULONG               logicalSectorSize = BytesPerLogicalSector(&ChannelExtension->DeviceExtension->DeviceParameters);

    completed = ((Srb->SrbStatus != SRB_STATUS_PENDING) && (Srb->SrbStatus != SRB_STATUS_SUCCESS)) ||
                (lbaRangeContext->ProcessedLbaRangeEntryCount >= lbaRangeContext->NeededLbaRangeEntryCount);

    if (!completed && (logicalSectorSize > 0)) {

        //
        // when first time this function is called, lbaRangeContext->CurrentDataSetRange.LengthInBytes should be '0' ( initialized to 0)
        // so that the first DataSet Range will be copied to lbaRangeContext->CurrentDataSetRange
        //
        tempDataSetRangeConverted = (lbaRangeContext->CurrentRangeLbaCount == 0)? TRUE : FALSE;

        //
        // if the previous entry conversion completed, continue the next one;
        // otherwise, still process the left part of the un-completed entry.
        //
        if (tempDataSetRangeConverted) {
            lbaRangeContext->CurrentRangeStartLba = lbaRangeContext->DataSetRanges[lbaRangeContext->DataSetRangeIndex].StartingOffset / logicalSectorSize;
            lbaRangeContext->CurrentRangeLbaCount = lbaRangeContext->DataSetRanges[lbaRangeContext->DataSetRangeIndex].LengthInBytes / logicalSectorSize;
            lbaRangeContext->DataSetRangeIndex++;
        }

        //
        // HybridChangeByLBA command can only carry one LBA range. So just convert one.
        //
        convertedEntryCount = ConvertDataSetRangeToAtaLbaRanges(&lbaRangeContext->CurrentRangeStartLba,
                                                                &lbaRangeContext->CurrentRangeLbaCount,
                                                                (PCHAR)&ataLbaRange,
                                                                sizeof(ATA_LBA_RANGE)
                                                                );
        NT_ASSERT(convertedEntryCount == 1);
        lbaRangeContext->ProcessedLbaRangeEntryCount += convertedEntryCount;

        //
        // Send HybridChangeByLBA command with converted LBA range.
        //
        AhciZeroMemory((PCHAR)&srbExtension->Cfis, sizeof(AHCI_H2D_REGISTER_FIS));

        BuildHybridChangeByLbaCommand(&srbExtension->Cfis,
                                      (UCHAR)lbaRangeContext->TargetPriority,
                                      ataLbaRange.StartSector,
                                      (USHORT)ataLbaRange.SectorCount,
                                      (ChannelExtension->DeviceExtension->HybridInfo.SupportedOptions.SupportCacheBehavior == 1));

        srbExtension->CompletionRoutine = HybridChangeByLbaCompletion;
        srbExtension->CompletionContext = lbaRangeContext;

        srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;

        return;

    } else {
        if (!completed && (logicalSectorSize == 0)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
        }

        //
        // Log ETW event about Hybrid Change By LBA.
        //
        if (ChannelExtension->AdapterExtension->TracingEnabled) {
            StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                                (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                AhciEtwEventUnitHybridChangePriorityByLBA,
                                L"Hybrid Change By LBA",
                                STORPORT_ETW_EVENT_KEYWORD_IO,
                                StorportEtwLevelInformational,
                                StorportEtwEventOpcodeInfo,
                                (PSCSI_REQUEST_BLOCK)Srb,
                                L"Srb Status",
                                Srb->SrbStatus,
                                L"Device CMD Sent",
                                TRUE,
                                L"Target Priority",
                                lbaRangeContext->TargetPriority,
                                L"LBA Range Count",
                                lbaRangeContext->DataSetRangeCount);
        }

        if (lbaRangeContext != NULL) {
            StorPortFreePool((PVOID)ChannelExtension->AdapterExtension, lbaRangeContext);
        }

        srbExtension->CompletionContext = NULL;
        srbExtension->CompletionRoutine = NULL;
        srbExtension->AtaFunction = 0;

        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Change by LBA command %s. \n", ChannelExtension->PortNumber,
                                (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? "completed successfully" : "failed");
    }

    return;
}

ULONG
HybridChangeByLba(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes HYBRID request - Change by LBA.

    NOTE that StorAHCI only supports one LBA Range from the request. This is based on device limitation.
    Supporting multiple LBA Ranges using loop is not desired considering the timeout limitation.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    PAHCI_SRB_EXTENSION srbExtension;

    PSRB_IO_CONTROL                                 srbControl;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES              dsmAttributes;
    PDEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS  changePriorty;
    PDEVICE_DATA_SET_RANGE                          dataSetRange;

    PHYBRID_CHANGE_BY_LBA_CONTEXT   lbaRangeContext = NULL;
    ULONG                           logicalSectorSize;
    ULONG                           i;
    ULONG                           status = STOR_STATUS_SUCCESS;

    srbExtension = GetSrbExtension(Srb);
    logicalSectorSize = BytesPerLogicalSector(&ChannelExtension->DeviceExtension->DeviceParameters);

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    dsmAttributes = (PDEVICE_MANAGE_DATA_SET_ATTRIBUTES)((PUCHAR)srbControl + ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID));
    changePriorty = (PDEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS)((PUCHAR)dsmAttributes + dsmAttributes->ParameterBlockOffset);
    dataSetRange = (PDEVICE_DATA_SET_RANGE)((PUCHAR)dsmAttributes + dsmAttributes->DataSetRangesOffset);

    if (dsmAttributes->ParameterBlockLength < sizeof(DEVICE_DSM_NVCACHE_CHANGE_PRIORITY_PARAMETERS)) {
        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    if ((dsmAttributes->DataSetRangesOffset < sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES)) ||
        (dsmAttributes->DataSetRangesLength < sizeof(DEVICE_DATA_SET_RANGE))) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    if (dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE) > GetHybridMaxLbaRangeCountForChangeLba(ChannelExtension)) {
        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    if (changePriorty->TargetPriority > ChannelExtension->DeviceExtension->HybridInfo.MaximumHybridPriorityLevel) {
        srbControl->ReturnCode = HYBRID_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Fail the request if NCQ is not supported.
    //
    if (!IsNCQSupported(ChannelExtension) ||
        (ChannelExtension->StateFlags.NCQ_Activated == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If device doesn't support Hybrid or the feature is disabled, fail the request.
    //
    if (!IsDeviceHybridInfoSupported(ChannelExtension) ||
        !IsDeviceHybridInfoEnabled(ChannelExtension) ||
        (ChannelExtension->DeviceExtension->SupportedCommands.HybridChangeByLbaRange == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If device is not recognized yet, fail the request.
    //
    if (logicalSectorSize == 0) {
        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // The request has LBA range less than 1 sector, nothing to do.
    //
    if ((dataSetRange->LengthInBytes / logicalSectorSize) == 0) {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STOR_STATUS_SUCCESS;

        goto Exit;
    }

    StorPortAllocatePool(ChannelExtension->AdapterExtension, sizeof(HYBRID_CHANGE_BY_LBA_CONTEXT), AHCI_POOL_TAG, (PVOID*)&lbaRangeContext);
    if (lbaRangeContext == NULL) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INSUFFICIENT_RESOURCES;

        goto Exit;
    }

    AhciZeroMemory((PCHAR)lbaRangeContext, sizeof(HYBRID_CHANGE_BY_LBA_CONTEXT));

    lbaRangeContext->TargetPriority = changePriorty->TargetPriority;

    lbaRangeContext->DataSetRanges = dataSetRange;
    lbaRangeContext->DataSetRangeCount = dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE);

    //
    // calculate how many ATA Lba entries needed to complete this request
    //
    for (i = 0; i < lbaRangeContext->DataSetRangeCount; i++) {
        ULONGLONG   dataSetRangeLbaCount = dataSetRange[i].LengthInBytes / logicalSectorSize;

        //
        // the ATA Lba entry - SectorCount field is 16bits;
        // following calculation shows how many ATA Lba entries should be used to represent the dataset range entry.
        //
        if (dataSetRangeLbaCount > 0) {
            lbaRangeContext->NeededLbaRangeEntryCount += (ULONG)((dataSetRangeLbaCount - 1) / MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE + 1);
        }
    }

    //
    // Prepare necessary data fields before directly calling HybridChangeByLbaCompletion().
    //
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;
    srbExtension->CompletionRoutine = HybridChangeByLbaCompletion;
    srbExtension->CompletionContext = (PVOID)lbaRangeContext;

    HybridChangeByLbaCompletion(ChannelExtension, Srb);

    status = STOR_STATUS_SUCCESS;

Exit:

    //
    // Log ETW event about Hybrid Change By LBA.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridChangePriorityByLBA,
                            L"Hybrid Change By LBA",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            FALSE,
                            L"Target Priority",
                            changePriorty->TargetPriority,
                            L"LBA Range Count",
                            dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE));
    }

    return status;
}

__inline
VOID
BuildHybridEvictCommand(
    _Inout_ PAHCI_H2D_REGISTER_FIS CFIS,
    _In_ USHORT                 BlockCount
    )
/*++
Routine Description:

    Build Hybrid Evict command to evict some Lba Ranges or whole caching medium.

Arguments:
    CFIS - the buffer should be zero-ed before calling this function.
    BlockCount - value 0 indicates the whole cache should be evicted.

Return Value:

    None

--*/
{
    if (BlockCount > 0) {
        CFIS->Feature7_0 = (UCHAR)BlockCount;
        CFIS->Feature15_8 = (UCHAR)(BlockCount >> 8);
    } else {
        CFIS->Auxiliary7_0 = 0x1;
    }

    CFIS->Count15_8 = IDE_NCQ_SEND_HYBRID_EVICT;

    CFIS->Device |= (1 << 6);
    CFIS->Command = IDE_COMMAND_SEND_FPDMA_QUEUED;
}


VOID
HybridEvictCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
)
/*++

Routine Description:

    Process EVICT request that received from upper layer.

Arguments:

    ChannelExtension
    Srb

Return Value:

    None

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    BOOLEAN             completed = FALSE;

    PHYBRID_EVICT_CONTEXT   evictContext = (PHYBRID_EVICT_CONTEXT)srbExtension->CompletionContext;
    PCHAR                   buffer = (PCHAR)srbExtension->DataBuffer;
    ULONG                   bufferLength = 0;

    BOOLEAN             tempDataSetRangeConverted = TRUE;
    ULONG               bufferLengthUsed = 0;
    ULONG               convertedEntryCount = 0;
    ULONG               logicalSectorSize = BytesPerLogicalSector(&ChannelExtension->DeviceExtension->DeviceParameters);

    completed = ((Srb->SrbStatus != SRB_STATUS_PENDING) && (Srb->SrbStatus != SRB_STATUS_SUCCESS)) ||
                (evictContext->ProcessedLbaRangeEntryCount >= evictContext->NeededLbaRangeEntryCount);

    if (!completed && (logicalSectorSize > 0)) {
        AhciZeroMemory(buffer, evictContext->AllocatedBufferLength);

        // 1. calculate the buffer size needed for EVICT command
        bufferLength = GetDataBufferLengthForDsmCommand(evictContext->MaxLbaRangeEntryCountPerCmd, evictContext->NeededLbaRangeEntryCount - evictContext->ProcessedLbaRangeEntryCount);

        // 2. prepare and send EVICT command
        //    check if the Unmap request is satisfied
        while (evictContext->ProcessedLbaRangeEntryCount < evictContext->NeededLbaRangeEntryCount) {
            // when first time this function is called, evictContext->CurrentDataSetRange.LenthInBytes should be '0' ( < logicalSectorSize)
            // so that the first DataSet Range will be copied to evictContext->CurrentDataSetRange
            tempDataSetRangeConverted = (evictContext->CurrentRangeLbaCount == 0)? TRUE : FALSE;

            // if the previous entry conversion completed, continue the next one;
            // otherwise, still process the left part of the un-completed entry.
            if (tempDataSetRangeConverted) {
                evictContext->CurrentRangeStartLba = evictContext->DataSetRanges[evictContext->DataSetRangeIndex].StartingOffset / logicalSectorSize;
                evictContext->CurrentRangeLbaCount = evictContext->DataSetRanges[evictContext->DataSetRangeIndex].LengthInBytes / logicalSectorSize;
                evictContext->DataSetRangeIndex++;
            }

            convertedEntryCount = ConvertDataSetRangeToAtaLbaRanges(&evictContext->CurrentRangeStartLba,
                                                                    &evictContext->CurrentRangeLbaCount,
                                                                    buffer + bufferLengthUsed,
                                                                    bufferLength - bufferLengthUsed
                                                                    );
            evictContext->ProcessedLbaRangeEntryCount += convertedEntryCount;

            bufferLengthUsed += convertedEntryCount * sizeof(ATA_LBA_RANGE);

            // send EVICT command when the buffer is full or all unmap entries are converted.
            if ( (bufferLengthUsed == bufferLength) ||
                 (evictContext->ProcessedLbaRangeEntryCount >= evictContext->NeededLbaRangeEntryCount) ) {
                // get ATA block count, the value is needed for setting the DSM command.
                USHORT transferBlockCount = (USHORT)(bufferLength / ATA_BLOCK_SIZE);

                // Clear the CFIS structure.
                AhciZeroMemory((PCHAR)&srbExtension->Cfis, sizeof(AHCI_H2D_REGISTER_FIS));

                BuildHybridEvictCommand(&srbExtension->Cfis, transferBlockCount);

                srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;
                srbExtension->DataBuffer = buffer;
                srbExtension->DataTransferLength = bufferLength;

                srbExtension->CompletionRoutine = HybridEvictCompletion;
                srbExtension->CompletionContext = (PVOID)evictContext;

                // update the SGL to reflect the actual transfer length.
                srbExtension->LocalSgl.List[0].Length = bufferLength;

                return;
            }
        }
        // should not reach this path
        NT_ASSERT(FALSE);

    } else {
        if (!completed && (logicalSectorSize == 0)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_LUN;
        }

        //
        // Log ETW event about Hybrid Evict.
        //
        if (ChannelExtension->AdapterExtension->TracingEnabled) {
            StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                                (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                AhciEtwEventUnitHybridEvict,
                                L"Hybrid Evict",
                                STORPORT_ETW_EVENT_KEYWORD_IO,
                                StorportEtwLevelInformational,
                                StorportEtwEventOpcodeInfo,
                                (PSCSI_REQUEST_BLOCK)Srb,
                                L"Srb Status",
                                Srb->SrbStatus,
                                L"Device CMD Sent",
                                TRUE,
                                L"Evict All",
                                FALSE,
                                L"LBA Range Count",
                                evictContext->DataSetRangeCount);
        }

        if (buffer != NULL) {
            AhciFreeDmaBuffer((PVOID)ChannelExtension->AdapterExtension, evictContext->AllocatedBufferLength, buffer);
        }

        if (evictContext != NULL) {
            StorPortFreePool((PVOID)ChannelExtension->AdapterExtension, evictContext);
        }

        srbExtension->DataBuffer = NULL;
        srbExtension->CompletionContext = NULL;
        srbExtension->CompletionRoutine = NULL;
        srbExtension->AtaFunction = 0;

        StorPortDebugPrint(3, "StorAHCI - Hybrid: Port %02d - Evict command %s. \n", ChannelExtension->PortNumber,
                                (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? "completed successfully" : "failed");
    }
    return;
}


ULONG
HybridEvict(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    ULONG               status = STOR_STATUS_SUCCESS;
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    PSRB_IO_CONTROL                     srbControl;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES  dsmAttributes;
    PDEVICE_DATA_SET_RANGE              dataSetRange;

    PHYBRID_EVICT_CONTEXT           evictContext = NULL;
    ULONG                           logicalSectorSize;

    ULONG                   i;
    PVOID                   buffer = NULL;     // DMA buffer allocated for EVICT command
    STOR_PHYSICAL_ADDRESS   bufferPhysicalAddress;

    logicalSectorSize = BytesPerLogicalSector(&ChannelExtension->DeviceExtension->DeviceParameters);

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    dsmAttributes = (PDEVICE_MANAGE_DATA_SET_ATTRIBUTES)((PUCHAR)srbControl + ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID));
    dataSetRange = (PDEVICE_DATA_SET_RANGE)((PUCHAR)dsmAttributes + dsmAttributes->DataSetRangesOffset);

    if ((dsmAttributes->Flags & DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE) == 0) {
        if ((dsmAttributes->DataSetRangesOffset < sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES)) ||
            (dsmAttributes->DataSetRangesLength < sizeof(DEVICE_DATA_SET_RANGE))) {

            srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
            Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
            status = STOR_STATUS_INVALID_PARAMETER;

            goto Exit;
        }
    }

    //
    // Fail the request if NCQ is not supported.
    //
    if (!IsNCQSupported(ChannelExtension) ||
        (ChannelExtension->StateFlags.NCQ_Activated == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If device doesn't support Hybrid or the feature is disabled, fail the request.
    //
    if (!IsDeviceHybridInfoSupported(ChannelExtension) ||
        !IsDeviceHybridInfoEnabled(ChannelExtension) ||
        (ChannelExtension->DeviceExtension->SupportedCommands.HybridEvict == 0)) {

        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // If device is not recognized yet, fail the request.
    //
    if (logicalSectorSize == 0) {
        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    //
    // Process EVICT request to evict the whole caching medium. Not supported.
    //
    if ((dsmAttributes->Flags & DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE) != 0) {
        srbControl->ReturnCode = HYBRID_STATUS_ILLEGAL_REQUEST;
        Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        status = STOR_STATUS_INVALID_PARAMETER;

        goto Exit;
    }

    StorPortAllocatePool(ChannelExtension->AdapterExtension, sizeof(HYBRID_EVICT_CONTEXT), AHCI_POOL_TAG, (PVOID*)&evictContext);

    if (evictContext == NULL) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INSUFFICIENT_RESOURCES;

        goto Exit;
    }

    AhciZeroMemory((PCHAR)evictContext, sizeof(HYBRID_EVICT_CONTEXT));

    evictContext->DataSetRanges = dataSetRange;
    evictContext->DataSetRangeCount = dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE);

    //
    // calculate how many ATA Lba entries can be sent by an EVICT command
    //
    evictContext->MaxLbaRangeEntryCountPerCmd = (ChannelExtension->DeviceExtension->HybridInfo.MaximumEvictionDataBlocks * ATA_BLOCK_SIZE) / sizeof(ATA_LBA_RANGE);

    if (evictContext->MaxLbaRangeEntryCountPerCmd == 0) {
        //
        // Reporting "Max Lba Range Entry Count per Command" value "0" is a disk firmware bug. Use one block as the value for this case.
        //
        NT_ASSERT(FALSE);
        evictContext->MaxLbaRangeEntryCountPerCmd = ATA_BLOCK_SIZE / sizeof(ATA_LBA_RANGE);
    }

    //
    // calculate how many ATA Lba entries needed to complete this EVICT request
    //
    for (i = 0; i < evictContext->DataSetRangeCount; i++) {
        ULONGLONG   dataSetRangeLbaCount = dataSetRange[i].LengthInBytes / logicalSectorSize;

        //
        // the ATA Lba entry - SectorCount field is 16bits;
        // following calculation shows how many ATA Lba entries should be used to represent the dataset range entry.
        if (dataSetRangeLbaCount > 0) {
            evictContext->NeededLbaRangeEntryCount += (ULONG)((dataSetRangeLbaCount - 1) / MAX_ATA_LBA_RANGE_SECTOR_COUNT_VALUE + 1);
        }
    }

    //
    // calculate the buffer size needed for EVICT command
    //
    evictContext->AllocatedBufferLength = GetDataBufferLengthForDsmCommand(evictContext->MaxLbaRangeEntryCountPerCmd, evictContext->NeededLbaRangeEntryCount);

    if (evictContext->AllocatedBufferLength == 0) {
        //
        // nothing to do. Complete the request.
        //
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STOR_STATUS_SUCCESS;
        goto Exit;
    }

    //
    // allocate DMA buffer, this buffer will be used to store ATA LBA Ranges for EVICT command
    //
    status = AhciAllocateDmaBuffer((PVOID)ChannelExtension->AdapterExtension, evictContext->AllocatedBufferLength, &buffer);

    if ( (status != STOR_STATUS_SUCCESS) || (buffer == NULL) ) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        if (status == STOR_STATUS_SUCCESS) {
            status = STOR_STATUS_INSUFFICIENT_RESOURCES;
        }
        goto Exit;
    }

    //
    // save values before calling HybridEvictCompletion()
    //
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_CFIS_PAYLOAD;
    srbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    srbExtension->DataBuffer = buffer;
    srbExtension->DataTransferLength = evictContext->AllocatedBufferLength;
    srbExtension->CompletionContext = (PVOID)evictContext;

    bufferPhysicalAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, buffer, &i);
    srbExtension->LocalSgl.NumberOfElements = 1;
    srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = bufferPhysicalAddress.LowPart;
    srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = bufferPhysicalAddress.HighPart;
    srbExtension->LocalSgl.List[0].Length = evictContext->AllocatedBufferLength;
    srbExtension->Sgl = &srbExtension->LocalSgl;

    HybridEvictCompletion(ChannelExtension, Srb);

    status = STOR_STATUS_SUCCESS;

Exit:

    //
    // Log ETW event about Hybrid Evict.
    //
    if ((Srb->SrbStatus != SRB_STATUS_PENDING) && (ChannelExtension->AdapterExtension->TracingEnabled)) {
        StorPortEtwEvent4(ChannelExtension->AdapterExtension,
                            (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                            AhciEtwEventUnitHybridEvict,
                            L"Hybrid Evict",
                            STORPORT_ETW_EVENT_KEYWORD_IO,
                            StorportEtwLevelInformational,
                            StorportEtwEventOpcodeInfo,
                            (PSCSI_REQUEST_BLOCK)Srb,
                            L"Srb Status",
                            Srb->SrbStatus,
                            L"Device CMD Sent",
                            FALSE,
                            L"Evict All",
                            ((dsmAttributes->Flags & DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE) != 0) ? TRUE : FALSE,
                            L"LBA Range Count",
                            dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE));
    }

    //
    // the process failed before EVICT command can be sent. Free allocated resources.
    //
    if (status != STOR_STATUS_SUCCESS) {
        if (buffer != NULL) {
            AhciFreeDmaBuffer((PVOID)ChannelExtension->AdapterExtension, evictContext->AllocatedBufferLength, buffer);
        }

        if (evictContext != NULL) {
            StorPortFreePool((PVOID)ChannelExtension->AdapterExtension, evictContext);
        }
    }

    return status;
}


ULONG
FirmwareGetInfo(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes FIRMWARE request - Get Info.

Arguments:
    ChannelExtension - 
    Srb - 

Return Value:

    STOR Status

--*/
{
    ULONG                       status = STOR_STATUS_SUCCESS;
    PSRB_IO_CONTROL             srbControl;
    PFIRMWARE_REQUEST_BLOCK     firmwareRequest;
    PSTORAGE_FIRMWARE_INFO_V2   firmwareInfo;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);

    firmwareRequest = (PFIRMWARE_REQUEST_BLOCK)(srbControl + 1);

    if (firmwareRequest->DataBufferLength < sizeof(STORAGE_FIRMWARE_INFO_V2)) {
        srbControl->ReturnCode = FIRMWARE_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        return status;
    }

    //
    // Buffer length has been validated. It's safe to access STORAGE_FIRMWARE_INFO data structure.
    //
    firmwareInfo = (PSTORAGE_FIRMWARE_INFO_V2)((PUCHAR)srbControl + firmwareRequest->DataBufferOffset);

    if ((firmwareInfo->Version < STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION_V2) ||
        (firmwareInfo->Size < sizeof(STORAGE_FIRMWARE_INFO_V2))) {
        //
        // Does not support V1 version of data structure.
        //
        srbControl->ReturnCode = FIRMWARE_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        return status;
    }

    AhciZeroMemory((PCHAR)firmwareInfo, firmwareRequest->DataBufferLength);

    //
    // Fill information into output data buffer.
    //
    firmwareInfo->Version = STORAGE_FIRMWARE_INFO_STRUCTURE_VERSION_V2;
    firmwareInfo->Size = sizeof(STORAGE_FIRMWARE_INFO_V2);

    firmwareInfo->UpgradeSupport = IsFirmwareUpdateSupported(ChannelExtension);

    firmwareInfo->SlotCount = 1;
    firmwareInfo->ActiveSlot = 0;
    firmwareInfo->PendingActivateSlot = STORAGE_FIRMWARE_INFO_INVALID_SLOT;

    firmwareInfo->FirmwareShared = FALSE;
    firmwareInfo->ImagePayloadAlignment = ATA_BLOCK_SIZE * ChannelExtension->DeviceExtension->FirmwareUpdate.DmMinTransferBlocks;
    firmwareInfo->ImagePayloadMaxSize = ATA_BLOCK_SIZE * ChannelExtension->DeviceExtension->FirmwareUpdate.DmMaxTransferBlocks;

    if (firmwareRequest->DataBufferLength >= (sizeof(STORAGE_FIRMWARE_INFO_V2) + sizeof(STORAGE_FIRMWARE_SLOT_INFO_V2))) {

        firmwareInfo->Slot[0].SlotNumber = 0;
        firmwareInfo->Slot[0].ReadOnly = FALSE;

        StorPortCopyMemory(&firmwareInfo->Slot[0].Revision, ChannelExtension->DeviceExtension->DeviceParameters.RevisionId, 8);

        srbControl->ReturnCode = FIRMWARE_STATUS_SUCCESS;
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STOR_STATUS_SUCCESS;
    } else {
        //
        // Data buffer is not big enough. Set desired buffer size.
        //
        NT_ASSERT(FALSE);
        firmwareRequest->DataBufferLength = sizeof(STORAGE_FIRMWARE_INFO_V2) + sizeof(STORAGE_FIRMWARE_SLOT_INFO_V2);

        srbControl->ReturnCode = FIRMWARE_STATUS_OUTPUT_BUFFER_TOO_SMALL;
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STOR_STATUS_SUCCESS;
    }

    return status;
}

ULONG
FirmwareIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL dispatch routine processes Firmware request from storport

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    ULONG                   status;
    ULONG                   srbDataBufferLength = 0;
    PSRB_IO_CONTROL         srbControl;
    PFIRMWARE_REQUEST_BLOCK firmwareRequest;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    //
    // A Firmware request must have at least SRB_IO_CONTROL and FIRMWARE_REQUEST_BLOCK in input buffer.
    //
    if (srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(FIRMWARE_REQUEST_BLOCK))) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    firmwareRequest = (PFIRMWARE_REQUEST_BLOCK)(srbControl + 1);

    if ((ULONGLONG)(srbDataBufferLength) < ((ULONGLONG)firmwareRequest->DataBufferOffset + firmwareRequest->DataBufferLength)) {
        srbControl->ReturnCode = FIRMWARE_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    if (firmwareRequest->Version < FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION) {
        srbControl->ReturnCode = FIRMWARE_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    //
    // the offset is from the beginning of SRB_IO_CONTROL. The value should be multiple of sizeof(PVOID).
    //
    if (firmwareRequest->DataBufferOffset < ALIGN_UP(sizeof(SRB_IO_CONTROL) + sizeof(FIRMWARE_REQUEST_BLOCK), PVOID)) {
        srbControl->ReturnCode = FIRMWARE_STATUS_INVALID_PARAMETER;
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        return STOR_STATUS_INVALID_PARAMETER;
    }

    //
    // process FIRMWARE request according to function
    //
    switch (firmwareRequest->Function) {

    case FIRMWARE_FUNCTION_GET_INFO:
        status = FirmwareGetInfo(ChannelExtension, Srb);
        break;

    case FIRMWARE_FUNCTION_DOWNLOAD:
    case FIRMWARE_FUNCTION_ACTIVATE:
    default:
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        break;

    }

    return status;
}


ULONG
DsmGeneralIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL worker routine processes Data Set Management request from storport

Arguments:
    ChannelExtension
    SRB

Return Value:

    NT Status

--*/
{
    ULONG               status = STOR_STATUS_SUCCESS;
    PSRB_IO_CONTROL     srbControl;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES  dsmAttributes;

    PVOID               srbDataBuffer = SrbGetDataBuffer(Srb);
    ULONGLONG           srbDataBufferLength = SrbGetDataTransferLength(Srb);

    //
    // Validate size of buffer
    //
    if (srbDataBufferLength < (ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID) + sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES))) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    }

    srbControl = (PSRB_IO_CONTROL)srbDataBuffer;
    dsmAttributes = (PDEVICE_MANAGE_DATA_SET_ATTRIBUTES)((PUCHAR)srbControl + ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID));

    if ( (srbDataBuffer == NULL) ||
         (srbDataBufferLength < ((ULONGLONG)ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID) + dsmAttributes->DataSetRangesOffset + dsmAttributes->DataSetRangesLength)) ||
         (srbDataBufferLength < ((ULONGLONG)ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID) + dsmAttributes->ParameterBlockOffset + dsmAttributes->ParameterBlockLength)) ||
         (srbDataBufferLength < ((ULONGLONG)ALIGN_UP(sizeof(SRB_IO_CONTROL), PVOID) + dsmAttributes->ParameterBlockLength + dsmAttributes->DataSetRangesLength)) ) {

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STOR_STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Handle the request
    //
    switch (dsmAttributes->Action) {
    case DeviceDsmAction_NvCache_Change_Priority:
        status = HybridChangeByLba(ChannelExtension, Srb);
        break;
    case DeviceDsmAction_NvCache_Evict:
        if ((dsmAttributes->Flags & DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE) != 0) {
            NT_ASSERT(dsmAttributes->DataSetRangesOffset == 0);
            NT_ASSERT(dsmAttributes->DataSetRangesLength == 0);
        }

        status = HybridEvict(ChannelExtension, Srb);
        break;

    default:
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_BUFFER_TOO_SMALL;

        break;
    }

    return status;
}

VOID
IssueSmartReadLogCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ UCHAR  LogAddress,
    _In_ USHORT PageNumber,
    _In_ USHORT BlockCount,
    _In_ PSTOR_PHYSICAL_ADDRESS PhysicalAddress,
    _In_ PVOID DataBuffer,
    _In_opt_ PSRB_COMPLETION_ROUTINE CompletionRoutine
    )
/*++
    Issue SMART READ LOG command to device

Parameters:
    ChannelExtension    - port that the command should be sent to
    Srb                 - Srb that carries SMART READ LOG command in SrbExtension
    LogAddress          - Log address
    PageNumber          - Page# of the log
    BlockCount          - How many blocks (in 512 bytes)
    PhysicalAddress     - Buffer physical address
    DataBuffer          - Buffer
    CompletionRoutine   - Routine that needs to be executed after SMART READ LOG command completed

Return Values:
    None

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);
    UNREFERENCED_PARAMETER(PageNumber);

    //
    // Setup SrbExtension fields.
    //
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_SMART;
    srbExtension->Flags |= ATA_FLAGS_DATA_IN ;
    srbExtension->CompletionRoutine = CompletionRoutine;

    //
    // Setup TaskFile. This is a 28bit command.
    //
    AhciZeroMemory((PCHAR) &srbExtension->TaskFile, sizeof(ATA_TASK_FILE));
    srbExtension->TaskFile.Current.bFeaturesReg = IDE_SMART_READ_LOG;
    srbExtension->TaskFile.Current.bSectorCountReg = (UCHAR)BlockCount;
    srbExtension->TaskFile.Current.bSectorNumberReg = LogAddress;
    srbExtension->TaskFile.Current.bCylLowReg = 0x4F;
    srbExtension->TaskFile.Current.bCylHighReg = 0xC2;
    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0;
    srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_SMART;

    Srb->SrbStatus = SRB_STATUS_PENDING;
    srbExtension->DataBuffer = DataBuffer;

    //
    // Setup SGL
    //
    if ( PhysicalAddress ) {
        srbExtension->LocalSgl.NumberOfElements = 1;
        srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = PhysicalAddress->LowPart;
        srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = PhysicalAddress->HighPart;
        srbExtension->LocalSgl.List[0].Length = IDE_GP_LOG_SECTOR_SIZE * BlockCount;
        srbExtension->Sgl = &srbExtension->LocalSgl;
        srbExtension->DataTransferLength = IDE_GP_LOG_SECTOR_SIZE * BlockCount;
    }

    return;
}


VOID
QueryProtocolInfoCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
  )
{
    PSRB_IO_CONTROL         srbIoControl;
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataRequest;

    UNREFERENCED_PARAMETER(ChannelExtension);

    srbIoControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    protocolDataRequest = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)(srbIoControl + 1);

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        protocolDataRequest->ProtocolSpecificData.ProtocolDataLength = srbExtension->DataTransferLength;
    } else {
        protocolDataRequest->ProtocolSpecificData.ProtocolDataLength = 0;
    }

    srbExtension->DataBuffer = NULL;
    srbExtension->CompletionContext = NULL;
    srbExtension->CompletionRoutine = NULL;
    srbExtension->AtaFunction = 0;

    StorPortDebugPrint(3, "StorAHCI - ProtocolInfo: Port %02d - command %s. \n", ChannelExtension->PortNumber,
                       (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? "succeeded" : "failed");

    return;
}

ULONG
QueryProtocolInfoIdentifyData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes Query Protocol Data - Identify Device data.

Arguments:
    ChannelExtension - 
    Srb - 

Return Value:

    STOR Status

--*/
{
    ULONG               status = STOR_STATUS_SUCCESS;
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    PSRB_IO_CONTROL     srbIoControl = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataRequest = NULL;

    srbIoControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    protocolDataRequest = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)(srbIoControl + 1);

    //
    // Identify device data command returns 512 bytes of data.
    //
    if (protocolDataRequest->ProtocolSpecificData.ProtocolDataLength < sizeof(IDENTIFY_DEVICE_DATA)) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        return status;
    }

    //
    // Set up SrbExtension fields
    //
    srbExtension = GetSrbExtension(Srb);
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_IDENTIFY;
    srbExtension->Flags |= ATA_FLAGS_DATA_IN;
    srbExtension->CompletionRoutine = QueryProtocolInfoCompletion;

    //
    // Set up TaskFile
    //
    AhciZeroMemory((PCHAR) &srbExtension->TaskFile, sizeof(ATA_TASK_FILE));
    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0;
    srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_IDENTIFY;

    if (!FillClippedSGL(StorPortGetScatterGatherList(ChannelExtension->AdapterExtension, (PSCSI_REQUEST_BLOCK)Srb),
                        (PSTOR_SCATTER_GATHER_LIST) &srbExtension->LocalSgl,
                        sizeof(SRB_IO_CONTROL) + FIELD_OFFSET(STORAGE_PROTOCOL_DATA_DESCRIPTOR, ProtocolSpecificData) + protocolDataRequest->ProtocolSpecificData.ProtocolDataOffset,
                        sizeof(IDENTIFY_DEVICE_DATA))) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_BUFFER_TOO_SMALL;
        return status;
    } else {
        srbExtension->Sgl = &srbExtension->LocalSgl;
        srbExtension->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
    }

    return status;
}


ULONG
QueryProtocolInfoLogPageData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes Query Protocol Data - Log Page data.

Arguments:
    ChannelExtension - 
    Srb - 

Return Value:

    STOR Status

--*/
{
    ULONG           status = STOR_STATUS_SUCCESS;
    PSRB_IO_CONTROL srbIoControl;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataRequest;

    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);
    PVOID                   logPageBuffer = NULL;
    ULONG                   logPageBlocks = 0;
    BOOLEAN                 smartLog = FALSE;

    srbIoControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    protocolDataRequest = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)(srbIoControl + 1);

    //
    // "ProtocolDataRequestValue" represents the Log Address.
    // "ProtocolDataRequestSubValue" represents the Log Page of a Log Address.
    //
    if ((protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue > 0xFF) ||
        (protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestSubValue > 0xFFFF)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        return status;
    }

    //
    // Log page is 512 bytes.
    //
    if (protocolDataRequest->ProtocolSpecificData.ProtocolDataLength < IDE_GP_LOG_SECTOR_SIZE) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        return status;
    }

    //
    // Check if a SMART READ LOG command is needed.
    //
    switch (protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue) {
    case IDE_GP_SUMMARY_SMART_ERROR:
    case IDE_GP_COMPREHENSIVE_SMART_ERROR:
    case IDE_GP_SMART_SELF_TEST:
    case IDE_GP_SELECTIVE_SELF_TEST:
        smartLog = TRUE;
        break;

    default:
        smartLog = FALSE;
        break;
    }

    //
    // Validate ATA Feature requirement for Log Addresses
    //
    if (smartLog) {
        if (!IsSmartFeatureSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
    } else {
        if (!IsDeviceGeneralPurposeLoggingSupported(ChannelExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
    }

    //
    // Validate additional ATA Feature requirement for different Log Addresses
    //
    switch (protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue) {
    //
    // Logs that require SMART feature support
    //
    case IDE_GP_SUMMARY_SMART_ERROR:
    case IDE_GP_COMPREHENSIVE_SMART_ERROR:
    case IDE_GP_EXTENDED_COMPREHENSIVE_SMART_ERROR:
        if (!IsSmartErrorLogSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
        break;

    case IDE_GP_SMART_SELF_TEST:
    case IDE_GP_EXTENDED_SMART_SELF_TEST:
    case IDE_GP_SELECTIVE_SELF_TEST:
        if (!IsSmartSelfTestSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
        break;

    //
    // Logs that require NCQ feature support
    //
    case IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS:
    case IDE_GP_LOG_NCQ_NON_DATA_ADDRESS:
    case IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS:
        if (!IsNcqFeatureSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
        break;

    //
    // Logs that require Streaming feature support
    //
    case IDE_GP_LOG_WRITE_STREAM_ERROR:
    case IDE_GP_LOG_READ_STREAM_ERROR:
        if (!IsStreamingFeatureSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
        break;

    //
    // Logs that require EPC feature support
    //
    case IDE_GP_LOG_POWER_CONDITIONS:
        if (!IsExtendedPowerConditionsFeatureSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
        break;

    //
    //  Following Logs fall into "default" category that doesn't need extra feature validation.
    //
    case IDE_GP_LOG_DIRECTORY_ADDRESS:

    //
    // Logs that require DSN feature support
    //
    case IDE_GP_DEVICE_STATISTICS_NOTIFICATION:

    //
    // Logs that require LPS feature support
    //
    case IDE_GP_LPS_MISALIGNMENT:

    //
    // Logs that require SCT feature support
    //
    case IDE_GP_LOG_SCT_COMMAND_STATUS:
    case IDE_GP_LOG_SCT_DATA_TRANSFER:

    default:
        break;
    }

    if ((protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue >= 0x80) &&
        (protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue < 0xDF)) {
        //
        // 80h - 9Fh are for Host Specific SMART logs.
        // A0h - DFh are for Device Vendor Specific SMART logs.
        //
        if (!IsSmartFeatureSupported(ChannelExtension->DeviceExtension)) {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STOR_STATUS_INVALID_PARAMETER;
            return status;
        }
    }

    //
    // Set up command to be sent to device.
    // Using NULL as physical address of buffer so that SGL won't be set in calling function.
    //
    logPageBuffer = (PVOID)((PUCHAR)protocolDataRequest +
                            FIELD_OFFSET(STORAGE_PROTOCOL_DATA_DESCRIPTOR, ProtocolSpecificData) +
                            protocolDataRequest->ProtocolSpecificData.ProtocolDataOffset);
    logPageBlocks = protocolDataRequest->ProtocolSpecificData.ProtocolDataLength / IDE_GP_LOG_SECTOR_SIZE;

    if (smartLog) {
        IssueSmartReadLogCommand(ChannelExtension,
                                 Srb,
                                 (UCHAR)protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue,
                                 (USHORT)protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestSubValue,
                                 (USHORT)logPageBlocks,
                                 NULL,
                                 logPageBuffer,
                                 QueryProtocolInfoCompletion);
    } else {
        IssueReadLogExtCommand(ChannelExtension,
                               Srb,
                               (UCHAR)protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestValue,
                               (USHORT)protocolDataRequest->ProtocolSpecificData.ProtocolDataRequestSubValue,
                               (USHORT)logPageBlocks,
                               0,      // feature field
                               NULL,
                               logPageBuffer,
                               QueryProtocolInfoCompletion
                               );
    }

    //
    // Set up SGL.
    //
    if (!FillClippedSGL(StorPortGetScatterGatherList(ChannelExtension->AdapterExtension, (PSCSI_REQUEST_BLOCK)Srb),
                        (PSTOR_SCATTER_GATHER_LIST) &srbExtension->LocalSgl,
                        sizeof(SRB_IO_CONTROL) + FIELD_OFFSET(STORAGE_PROTOCOL_DATA_DESCRIPTOR, ProtocolSpecificData) + protocolDataRequest->ProtocolSpecificData.ProtocolDataOffset,
                        logPageBlocks * IDE_GP_LOG_SECTOR_SIZE)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_BUFFER_TOO_SMALL;
        return status;
    } else {
        srbExtension->Sgl = &srbExtension->LocalSgl;
        srbExtension->DataTransferLength = logPageBlocks * IDE_GP_LOG_SECTOR_SIZE;
    }

    return status;
}


ULONG
QueryProtocolInfoIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
/*++
Routine Description:

    IOCTL dispatch routine processes Query Protocol Information from storport

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    ULONG                   status = STOR_STATUS_SUCCESS;
    ULONG                   srbDataBufferLength = 0;
    PSRB_IO_CONTROL         srbControl = NULL;
    ULONG                   srbFlags = SrbGetSrbFlags(Srb);
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataRequest = NULL;
    ULONGLONG               protocolSpecificDataBufferOffset = 0;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    //
    // Reject the request if it's for the adapter.
    //
    if ((srbFlags & SRB_IOCTL_FLAGS_ADAPTER_REQUEST) != 0) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // A protocol request must have at least SRB_IO_CONTROL and STORAGE_PROTOCOL_DATA_DESCRIPTOR in buffer.
    //
    if (srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    protocolDataRequest = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)(srbControl + 1);
    protocolSpecificDataBufferOffset = (ULONGLONG)sizeof(SRB_IO_CONTROL) +
                                       FIELD_OFFSET(STORAGE_PROTOCOL_DATA_DESCRIPTOR, ProtocolSpecificData) +
                                       protocolDataRequest->ProtocolSpecificData.ProtocolDataOffset;

    if ((ULONGLONG)srbDataBufferLength < (protocolSpecificDataBufferOffset + 
                                          protocolDataRequest->ProtocolSpecificData.ProtocolDataLength)) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Data buffer of Protocol Data should be pointer aligned.
    //
    if ((protocolSpecificDataBufferOffset % sizeof(PVOID)) != 0) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // The query protocol type should be ATA and data type shouldn't be unknown.
    //
    if ((protocolDataRequest->ProtocolSpecificData.ProtocolType != ProtocolTypeAta) ||
        (protocolDataRequest->ProtocolSpecificData.DataType == 0)) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // process the request according to requested data type.
    //
    switch (protocolDataRequest->ProtocolSpecificData.DataType) {

    case AtaDataTypeIdentify:
        status = QueryProtocolInfoIdentifyData(ChannelExtension, Srb);
        break;
    case AtaDataTypeLogPage:
        status = QueryProtocolInfoLogPageData(ChannelExtension, Srb);
        break;

    default:
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        break;

    }

exit:

    return status;
}

VOID
QueryTemperatureInfoCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    ULONG                   srbDataBufferLength;
    PSRB_IO_CONTROL         srbIoControl;
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);
    PSTORAGE_TEMPERATURE_DATA_DESCRIPTOR temperatureDescr;

    srbDataBufferLength = SrbGetDataTransferLength(Srb);
    srbIoControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    temperatureDescr = (PSTORAGE_TEMPERATURE_DATA_DESCRIPTOR)(srbIoControl + 1);

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        PGP_LOG_TEMPERATURE_STATISTICS temperatureLog = (PGP_LOG_TEMPERATURE_STATISTICS)srbExtension->DataBuffer;

        AhciZeroMemory((PCHAR)temperatureDescr, srbDataBufferLength - sizeof(SRB_IO_CONTROL));

        temperatureDescr->Size = sizeof(STORAGE_TEMPERATURE_DATA_DESCRIPTOR);
        temperatureDescr->Version = sizeof(STORAGE_TEMPERATURE_DATA_DESCRIPTOR);

        temperatureDescr->CriticalTemperature = STORAGE_TEMPERATURE_VALUE_NOT_REPORTED;
        temperatureDescr->WarningTemperature = STORAGE_TEMPERATURE_VALUE_NOT_REPORTED;
        temperatureDescr->InfoCount = 1;
        temperatureDescr->TemperatureInfo[0].Index = 0;
        temperatureDescr->TemperatureInfo[0].Temperature = STORAGE_TEMPERATURE_VALUE_NOT_REPORTED;
        temperatureDescr->TemperatureInfo[0].OverThreshold = STORAGE_TEMPERATURE_VALUE_NOT_REPORTED;
        temperatureDescr->TemperatureInfo[0].UnderThreshold = STORAGE_TEMPERATURE_VALUE_NOT_REPORTED;
        temperatureDescr->TemperatureInfo[0].OverThresholdChangable = FALSE;
        temperatureDescr->TemperatureInfo[0].UnderThresholdChangable = FALSE;

        //
        // StorAHCI doesn't support DNS feature to notify host yet.
        //
        temperatureDescr->TemperatureInfo[0].EventGenerated = FALSE;

        if ((temperatureLog->Header.PageNumber == IDE_GP_LOG_DEVICE_STATISTICS_TEMPERATURE_PAGE) &&
            (temperatureLog->Header.RevisionNumber == IDE_GP_LOG_VERSION)) {

            if ((temperatureLog->CurrentTemperature.Supported == 1) && 
                (temperatureLog->CurrentTemperature.ValidValue == 1)) {

                temperatureDescr->TemperatureInfo[0].Temperature = (SHORT)temperatureLog->CurrentTemperature.Value;
            }

            if ((temperatureLog->SpecifiedMaximumOperatingTemperature.Supported == 1) && 
                (temperatureLog->SpecifiedMaximumOperatingTemperature.ValidValue == 1)) {

                temperatureDescr->TemperatureInfo[0].OverThreshold = (SHORT)temperatureLog->SpecifiedMaximumOperatingTemperature.Value;
                temperatureDescr->WarningTemperature = (SHORT)temperatureLog->SpecifiedMaximumOperatingTemperature.Value;
            }

            if ((temperatureLog->SpecifiedMinimumOperatingTemperature.Supported == 1) && 
                (temperatureLog->SpecifiedMinimumOperatingTemperature.ValidValue == 1)) {

                temperatureDescr->TemperatureInfo[0].UnderThreshold = (SHORT)temperatureLog->SpecifiedMinimumOperatingTemperature.Value;
            }

        } else {
            NT_ASSERT(FALSE);
            Srb->SrbStatus = SRB_STATUS_ERROR;
        }
    } else {
        NT_ASSERT(FALSE);
    }

    srbExtension->DataBuffer = NULL;
    srbExtension->CompletionContext = NULL;
    srbExtension->CompletionRoutine = NULL;
    srbExtension->AtaFunction = 0;

    StorPortDebugPrint(3, "StorAHCI - TemperatureLogInfo: Port %02d - command %s. \n", ChannelExtension->PortNumber,
                            (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? "succeeded" : "failed");

    return;
}

ULONG
QueryTemperatureInfoIoctlProcess(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
/*++
Routine Description:

    IOCTL dispatch routine processes Query Temperature Information from port driver.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    ULONG                   status = STOR_STATUS_SUCCESS;
    ULONG                   srbDataBufferLength = 0;
    PSRB_IO_CONTROL         srbControl = NULL;
    ULONG                   srbFlags = SrbGetSrbFlags(Srb);

    PVOID                   buffer = NULL;
    STOR_PHYSICAL_ADDRESS   bufferPhysicalAddress = {0};
    ULONG                   tempLength = 0;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    //
    // Reject the request if it's for the adapter.
    //
    if ((srbFlags & SRB_IOCTL_FLAGS_ADAPTER_REQUEST) != 0) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // A query temperature information request must have at least SRB_IO_CONTROL and STORAGE_TEMPERATURE_DATA_DESCRIPTOR in output buffer.
    //
    if (srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(STORAGE_TEMPERATURE_DATA_DESCRIPTOR))) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Fail the request if it's not supported by device.
    //
    if (ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.TemperatureStatistics == 0) {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // allocate DMA buffer to retrieve temperature statistics log.
    //
    status = AhciAllocateDmaBuffer((PVOID)ChannelExtension->AdapterExtension, IDE_GP_LOG_SECTOR_SIZE, (PVOID*)&buffer);

    if ( (status != STOR_STATUS_SUCCESS) || (buffer == NULL) ) {
        Srb->SrbStatus = SRB_STATUS_ERROR;
        status = STOR_STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    AhciZeroMemory((PCHAR)buffer, IDE_GP_LOG_SECTOR_SIZE);

    bufferPhysicalAddress = StorPortGetPhysicalAddress(ChannelExtension->AdapterExtension, NULL, buffer, &tempLength);

    IssueReadLogExtCommand(ChannelExtension,
                           Srb,
                           IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS,
                           IDE_GP_LOG_DEVICE_STATISTICS_TEMPERATURE_PAGE,
                           1,
                           0,      // feature field
                           &bufferPhysicalAddress,
                           buffer,
                           QueryTemperatureInfoCompletion
                           );

exit:

    return status;
}

__inline
VOID
GetDevicePhysicalTopologyData(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Out_ PSTORAGE_PHYSICAL_DEVICE_DATA DeviceTopologyData
    )
{
    CHAR    vendorString[] = "ATA";

    DeviceTopologyData->DeviceId = ChannelExtension->PortNumber;
    DeviceTopologyData->Role = STORAGE_COMPONENT_ROLE_DATA;

    if (!IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters) &&
        !IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {

        DeviceTopologyData->HealthStatus = HealthStatusUnknown;
    } else if (ChannelExtension->StateFlags.Initialized == 0) {
        DeviceTopologyData->HealthStatus = HealthStatusUnknown;
    } else if (IsNCQSupported(ChannelExtension) && (ChannelExtension->StateFlags.NCQ_Activated == 0)) {
        DeviceTopologyData->HealthStatus = HealthStatusThrottled;
    } else {
        DeviceTopologyData->HealthStatus = HealthStatusNormal;
    }

    DeviceTopologyData->CommandProtocol = ProtocolTypeAta;
    DeviceTopologyData->SpecVersion.MajorVersion = ChannelExtension->DeviceExtension->IdentifyDeviceData->MajorRevision;
    DeviceTopologyData->SpecVersion.MinorVersion.AsUshort = ChannelExtension->DeviceExtension->IdentifyDeviceData->MinorRevision;
    DeviceTopologyData->FormFactor = AtaFormFactorToStorageFormFactor(ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalFormFactor);

    StorPortCopyMemory(DeviceTopologyData->Vendor,
                       vendorString,
                       sizeof(vendorString));

    StorPortCopyMemory(DeviceTopologyData->Model,
                       ChannelExtension->DeviceExtension->DeviceParameters.VendorId,
                       40);

    StorPortCopyMemory(DeviceTopologyData->FirmwareRevision,
                       ChannelExtension->DeviceExtension->DeviceParameters.RevisionId,
                       8);

    DeviceTopologyData->Capacity = (MaxUserAddressableLba(&ChannelExtension->DeviceExtension->DeviceParameters) / KB) *
                                    ChannelExtension->DeviceExtension->DeviceParameters.BytesPerLogicalSector;

    return;
}


ULONG
QueryPhysicalTopologyIoctlProcess(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
/*++
Routine Description:

    IOCTL handling routine processes Query Physical Topology Information from port driver.

    NOTE: This IOCTL is required for miniport driver that may virtualize storage devices.
          e.g. the storage devices reported to OS do not have 1:1 matching to actual physical devices.

          StorAHCI doesn't virtualize storage devices thus doesn't need to support this IOCTL.
          Implementation of this function is for WDK sample and testing purpose.

Arguments:
    ChannelExtension
    SRB

Return Value:

    STOR Status

--*/
{
    ULONG                   status = STOR_STATUS_SUCCESS;
    BOOLEAN                 queryAdapterTopology = FALSE;
    ULONG                   srbDataBufferLength = 0;
    ULONG                   topologyBufferLength = 0;
    PSRB_IO_CONTROL         srbControl = NULL;
    PSTORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR topologyDescr = NULL;

    ULONG                   deviceCount = 0;
    ULONG                   requiredBufferLength = 0;

    ULONG                   srbFlags = SrbGetSrbFlags(Srb);
    ULONG                   i;

    PSTORAGE_PHYSICAL_ADAPTER_DATA  adapterData = NULL;
    PSTORAGE_PHYSICAL_DEVICE_DATA   deviceData = NULL;

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    srbDataBufferLength = SrbGetDataTransferLength(Srb);

    //
    // A query physical topology request must have at least SRB_IO_CONTROL and STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR in output buffer.
    //
    if (srbDataBufferLength < (sizeof(SRB_IO_CONTROL) + sizeof(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR))) {
        Srb->SrbStatus = SRB_STATUS_BAD_SRB_BLOCK_LENGTH;
        status = STOR_STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Always return success if the buffer is big enough for basic information.
    // The caller can resend the request base on value in "topologyDescr->Size".
    //
    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    topologyDescr = (PSTORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR)(srbControl + 1);
    topologyBufferLength = srbDataBufferLength - sizeof(SRB_IO_CONTROL);
    queryAdapterTopology = ((srbFlags & SRB_IOCTL_FLAGS_ADAPTER_REQUEST) != 0);

    //
    // Calculate device count for adapter topology report.
    //
    if (queryAdapterTopology) {
        for (i = 0; i <= AdapterExtension->HighestPort; i++) {
            if (((AdapterExtension->PortImplemented & (1 << i)) != 0) &&
                (IsAtaDevice(&AdapterExtension->PortExtension[i]->DeviceExtension->DeviceParameters) ||
                    IsAtapiDevice(&AdapterExtension->PortExtension[i]->DeviceExtension->DeviceParameters))) {

                deviceCount++;
            }
        }
    } else {
        deviceCount = 1;
    }

    //
    // Output buffer contains: STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR (before field - Node), 
    //                         STORAGE_PHYSICAL_NODE_DATA,
    //                         STORAGE_PHYSICAL_ADAPTER_DATA * 1,
    //                         STORAGE_PHYSICAL_DEVICE_DATA * deviceCount
    //
    requiredBufferLength = FIELD_OFFSET(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR, Node) +
                           sizeof(STORAGE_PHYSICAL_NODE_DATA) +
                           sizeof(STORAGE_PHYSICAL_ADAPTER_DATA) +
                           sizeof(STORAGE_PHYSICAL_DEVICE_DATA) * deviceCount;

    //
    // Initialize the output buffer.
    //
    AhciZeroMemory((PCHAR)topologyDescr, topologyBufferLength);

    //
    // Fill in header information.
    //
    topologyDescr->Version = sizeof(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR);
    topologyDescr->Size = requiredBufferLength;
    topologyDescr->NodeCount = 1;

    //
    // Fill in Note[0] information. Assume buffer is big enough, we will update the values later if it's not the case.
    //
    topologyDescr->Node[0].NodeId = 0;
    topologyDescr->Node[0].AdapterCount = 1;
    topologyDescr->Node[0].AdapterDataLength = sizeof(STORAGE_PHYSICAL_ADAPTER_DATA);
    topologyDescr->Node[0].AdapterDataOffset = sizeof(STORAGE_PHYSICAL_NODE_DATA);

    topologyDescr->Node[0].DeviceCount = deviceCount;
    topologyDescr->Node[0].DeviceDataLength = sizeof(STORAGE_PHYSICAL_DEVICE_DATA) * deviceCount;
    topologyDescr->Node[0].DeviceDataOffset = sizeof(STORAGE_PHYSICAL_NODE_DATA) + sizeof(STORAGE_PHYSICAL_ADAPTER_DATA);

    //
    // Fill in Adapter information.
    //
    if (topologyBufferLength < (FIELD_OFFSET(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR, Node) +
                                sizeof(STORAGE_PHYSICAL_NODE_DATA) +
                                sizeof(STORAGE_PHYSICAL_ADAPTER_DATA))) {
        //
        // Update Length and Offset fields to indicate no data there.
        //
        topologyDescr->Node[0].AdapterDataLength = 0;
        topologyDescr->Node[0].AdapterDataOffset = 0;

        topologyDescr->Node[0].DeviceDataLength = 0;
        topologyDescr->Node[0].DeviceDataOffset = 0;

        goto exit;
    }

    adapterData = (PSTORAGE_PHYSICAL_ADAPTER_DATA)(topologyDescr->Node + 1);

    adapterData->AdapterId = AdapterExtension->AdapterNumber;

    if (AdapterExtension->ErrorFlags != 0) {
        adapterData->HealthStatus = HealthStatusFailed;
    } else if (AdapterExtension->StateFlags.StoppedState == 1) {
        adapterData->HealthStatus = HealthStatusDisabled;
    } else {
        adapterData->HealthStatus = HealthStatusNormal;
    }

    adapterData->CommandProtocol = ProtocolTypeAta;
    adapterData->SpecVersion.AsUlong = AdapterExtension->Version.AsUlong;

    //
    // AHCI adapter VendorID, DeviceID and RevisionID are in USHORT/UCHAR format. Convert into CHARs.
    //
    {
        CHAR    hexDigit[] = "0123456789ABCDEF";

        adapterData->Vendor[0] = hexDigit[(AdapterExtension->VendorID >> 12) & 0xF];
        adapterData->Vendor[1] = hexDigit[(AdapterExtension->VendorID >> 8) & 0xF];
        adapterData->Vendor[2] = hexDigit[(AdapterExtension->VendorID >> 4) & 0xF];
        adapterData->Vendor[3] = hexDigit[AdapterExtension->VendorID & 0xF];

        adapterData->Model[0] = hexDigit[(AdapterExtension->DeviceID >> 12) & 0xF];
        adapterData->Model[1] = hexDigit[(AdapterExtension->DeviceID >> 8) & 0xF];
        adapterData->Model[2] = hexDigit[(AdapterExtension->DeviceID >> 4) & 0xF];
        adapterData->Model[3] = hexDigit[AdapterExtension->DeviceID & 0xF];

        adapterData->FirmwareRevision[0] = hexDigit[(AdapterExtension->RevisionID >> 4) & 0xF];
        adapterData->FirmwareRevision[1] = hexDigit[AdapterExtension->RevisionID & 0xF];
    }

    adapterData->ExpanderConnected = FALSE;

    //
    // Fill in Device information.
    //
    if (topologyBufferLength < (FIELD_OFFSET(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR, Node) +
                                sizeof(STORAGE_PHYSICAL_NODE_DATA) +
                                sizeof(STORAGE_PHYSICAL_ADAPTER_DATA) +
                                sizeof(STORAGE_PHYSICAL_DEVICE_DATA))) {
        //
        // If the buffer left cannot hold even one device data structure, update Length and Offset fields to indicate no data there.
        //
        topologyDescr->Node[0].DeviceDataLength = 0;
        topologyDescr->Node[0].DeviceDataOffset = 0;

        goto exit;
    }

    deviceData = (PSTORAGE_PHYSICAL_DEVICE_DATA)(adapterData + 1);

    if (!queryAdapterTopology) {
        UCHAR   lun = 0;
        SrbGetPathTargetLun(Srb, NULL, NULL, &lun);

        GetDevicePhysicalTopologyData(AdapterExtension->PortExtension[lun], deviceData);
    } else {
        ULONG   availableBufferLength = topologyBufferLength - FIELD_OFFSET(STORAGE_PHYSICAL_TOPOLOGY_DESCRIPTOR, Node) -
                                        sizeof(STORAGE_PHYSICAL_NODE_DATA) - sizeof(STORAGE_PHYSICAL_ADAPTER_DATA);

        for (i = 0; i <= AdapterExtension->HighestPort; i++) {
            if (((AdapterExtension->PortImplemented & (1 << i)) != 0) &&
                (IsAtaDevice(&AdapterExtension->PortExtension[i]->DeviceExtension->DeviceParameters) ||
                IsAtapiDevice(&AdapterExtension->PortExtension[i]->DeviceExtension->DeviceParameters))) {

                GetDevicePhysicalTopologyData(AdapterExtension->PortExtension[i], deviceData);

                availableBufferLength -= sizeof(STORAGE_PHYSICAL_DEVICE_DATA);

                if (availableBufferLength < sizeof(STORAGE_PHYSICAL_DEVICE_DATA)) {
                    break;
                } else {
                    deviceData++;
                }
            }
        }
    }

exit:

    return status;
}



#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4214)
#pragma warning(default:4201)
#pragma warning(default:26015)
#endif


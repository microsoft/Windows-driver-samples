/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    util.h

Abstract:

    Internal support routines used in STORAHCI

Notes:

Revision History:

--*/


#pragma once

#define MS_TO_100NS(ms) ((ULONGLONG)10000 *(ms)) // Converts milliseconds to 100ns.
#define MS_TO_US(ms) ((ULONGLONG)1000 *(ms))      // Converts milliseconds to microseconds.
#define TICKS_TO_US(ticks) ((ticks / 10)) // Converts ticks (100ns) to microseconds.

_At_buffer_(Buffer, _I_, BufferSize, _Post_equal_to_(0))
__inline
VOID
AhciZeroMemory(
    _Out_writes_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize
    )
{
    ULONG i;

    for (i = 0; i < BufferSize; i++) {
        Buffer[i] = 0;
    }
}

_At_buffer_(Buffer, _I_, BufferSize, _Post_equal_to_(Fill))
__inline
VOID
AhciFillMemory(
    _Out_writes_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize,
    _In_ CHAR  Fill
    )
{
    ULONG i;

    for (i = 0; i < BufferSize; i++) {
        Buffer[i] = Fill;
    }
}

__inline
BOOLEAN
IsPortValid(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ ULONG PathId
    )
{
    return ( (PathId <= AdapterExtension->HighestPort) && (AdapterExtension->PortExtension[PathId] != NULL) );
}

__inline
BOOLEAN
IsPortStartCapable(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
/*
    Indicates if a port can be started or port can process IO
*/
{
    return ( (ChannelExtension != NULL) &&
             (ChannelExtension->AdapterExtension->StateFlags.StoppedState == 0) &&
             (ChannelExtension->AdapterExtension->StateFlags.PowerDown == 0) &&
             (ChannelExtension->StateFlags.Initialized == 1) );
}


__inline
PAHCI_SRB_EXTENSION
GetSrbExtension (
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
    PCHAR       tempBuffer;
    ULONG_PTR   leftBytes;

    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        tempBuffer = (PCHAR)Srb->MiniportContext;
    } else {
        tempBuffer = (PCHAR)((PSCSI_REQUEST_BLOCK)Srb)->SrbExtension;
    }

    //
    // Use lower 32bit is good enough for this calculation.
    // 
    leftBytes = ((ULONG_PTR)tempBuffer) % 128;

    if (leftBytes == 0) {
        //
        // the buffer is already aligned.
        //
        return (PAHCI_SRB_EXTENSION)tempBuffer;
    } else {
        //
        // need to align to 128 bytes.
        // 
        return (PAHCI_SRB_EXTENSION)(tempBuffer + 128 - leftBytes);
    }
}

__inline
VOID
MarkSrbToBeCompleted(
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    SETMASK(srbExtension->Flags, ATA_FLAGS_COMPLETE_SRB);
}

__inline
BOOLEAN
IsDataTransferNeeded(
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    return ( (srbExtension->Flags & (ATA_FLAGS_DATA_IN | ATA_FLAGS_DATA_OUT)) || (srbExtension->DataBuffer != NULL) );
}


__inline
BOOLEAN
IsDumpMode(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
/*
Return Value:
    TRUE: miniport works in crashdump, hibernate etc.
    FALSE: miniport works in normal mode.
*/
{
    return (AdapterExtension->DumpMode > 0);
}

__inline
BOOLEAN
IsDumpCrashMode(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    return (AdapterExtension->DumpMode == DUMP_MODE_CRASH);
}

__inline
BOOLEAN
IsDumpHiberMode(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    return (AdapterExtension->DumpMode == DUMP_MODE_HIBER);
}

__inline
BOOLEAN
IsDumpMarkMemoryMode(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    return (AdapterExtension->DumpMode == DUMP_MODE_MARK_MEMORY);
}

__inline
BOOLEAN
IsDumpResumeMode(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    return (AdapterExtension->DumpMode == DUMP_MODE_RESUME);
}


__inline
BOOLEAN
IsMsnEnabled(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->MsnSupport == 1) );
}

__inline
BOOLEAN
IsRmfEnabled(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.RemovableMediaFeature == 1) &&
             (DeviceExtension->IdentifyDeviceData->GeneralConfiguration.RemovableMedia == 1) );
}

__inline
BOOLEAN
NoFlushDevice(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.WriteCache == 0) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.FlushCache == 0) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.FlushCacheExt == 0) );
}

__inline
BOOLEAN
IsTapeDevice(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAtapi) &&
             (DeviceExtension->IdentifyPacketData->GeneralConfiguration.CommandPacketType == 1) );
}

__inline
BOOLEAN
IsSmartFeatureSupported(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.SmartCommands == 1) );
}

__inline
BOOLEAN
IsSmartErrorLogSupported(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( IsSmartFeatureSupported(DeviceExtension) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.SmartErrorLog == 1) );
}

__inline
BOOLEAN
IsSmartSelfTestSupported(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( IsSmartFeatureSupported(DeviceExtension) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.SmartSelfTest == 1) );
}

__inline
BOOLEAN
IsExtendedPowerConditionsFeatureSupported(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupportExt.ExtendedPowerConditions == 1) );
}

__inline
BOOLEAN
IsStreamingFeatureSupported(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->CommandSetSupport.StreamingFeature == 1) );
}

__inline
BOOLEAN
IsNcqFeatureSupported(
    _In_ PAHCI_DEVICE_EXTENSION DeviceExtension
    )
{
    return ( (DeviceExtension->DeviceParameters.AtaDeviceType == DeviceIsAta) &&
             (DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NCQ == 1) );
}


__inline
BOOLEAN
IsNCQCommand (
    _In_ PAHCI_SRB_EXTENSION SrbExtension
    )
/*++
    Determine if a command is NCQ command.

Return Value:
    TRUE if CommandRegister is 0x60, 0x61, 0x63, 0x64 or 0x65
--*/
{
    UCHAR command = IDE_COMMAND_NOT_VALID;

    if ( IsAtaCfisPayload(SrbExtension->AtaFunction) ) {
        command = SrbExtension->Cfis.Command;
    } else if ( IsAtaCommand(SrbExtension->AtaFunction) ) {
        command = SrbExtension->TaskFile.Current.bCommandReg;
    }

    if ( (command == IDE_COMMAND_READ_FPDMA_QUEUED) ||
         (command == IDE_COMMAND_WRITE_FPDMA_QUEUED) ||
         (command == IDE_COMMAND_NCQ_NON_DATA) ||
         (command == IDE_COMMAND_SEND_FPDMA_QUEUED) ||
         (command == IDE_COMMAND_RECEIVE_FPDMA_QUEUED) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsNcqReadWriteCommand (
    _In_ PAHCI_SRB_EXTENSION SrbExtension
    )
/*++
    Determine if a command is NCQ R/W command.

Return Value:
    TRUE if CommandRegister is 0x60, 0x61
--*/
{
    UCHAR command = IDE_COMMAND_NOT_VALID;

    if ( IsAtaCfisPayload(SrbExtension->AtaFunction) ) {
        command = SrbExtension->Cfis.Command;
    } else if ( IsAtaCommand(SrbExtension->AtaFunction) ) {
        command = SrbExtension->TaskFile.Current.bCommandReg;
    }

    if ((command == IDE_COMMAND_READ_FPDMA_QUEUED) || (command == IDE_COMMAND_WRITE_FPDMA_QUEUED)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsNCQWriteCommand (
    _In_ PAHCI_SRB_EXTENSION SrbExtension
    )
/*++
    Determine if a command is NCQ Write command.

Return Value:
    TRUE if CommandRegister is 0x61
--*/
{
    UCHAR command = IDE_COMMAND_NOT_VALID;

    if ( IsAtaCfisPayload(SrbExtension->AtaFunction) ) {
        command = SrbExtension->Cfis.Command;
    } else if ( IsAtaCommand(SrbExtension->AtaFunction) ) {
        command = SrbExtension->TaskFile.Current.bCommandReg;
    }

    if (command == IDE_COMMAND_WRITE_FPDMA_QUEUED) {
        return TRUE;
    } else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsNormalCommand (
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
/*++
    This could be a macro

Return Value:
    TRUE if any Command Register is any non NCQ IO command
--*/
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if ((srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ_DMA) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_DMA) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ_DMA_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_DMA_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_DMA_FUA_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ_DMA_QUEUED_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_DMA_QUEUED_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_DMA_QUEUED_FUA_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ_MULTIPLE) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_MULTIPLE) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_READ_MULTIPLE_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_MULTIPLE_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_MULTIPLE_FUA_EXT) ||
        (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_WRITE_DMA_QUEUED) ||
        (srbExtension->AtaFunction == ATA_FUNCTION_ATAPI_COMMAND) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsReadWriteCommand(
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
/*++
Return Value:
    TRUE if the command is a read or a write command.  This includes
    NCQ and non-NCQ commands.
--*/
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    UCHAR command = IDE_COMMAND_NOT_VALID;

    if ( IsAtaCfisPayload(srbExtension->AtaFunction) ) {
        command = srbExtension->Cfis.Command;
    } else if ( IsAtaCommand(srbExtension->AtaFunction) ) {
        command = srbExtension->TaskFile.Current.bCommandReg;
    }

    if ((command == IDE_COMMAND_READ_FPDMA_QUEUED) ||
        (command == IDE_COMMAND_WRITE_FPDMA_QUEUED) ||
        (command == IDE_COMMAND_READ) ||
        (command == IDE_COMMAND_WRITE) ||
        (command == IDE_COMMAND_READ_EXT) ||
        (command == IDE_COMMAND_WRITE_EXT) ||
        (command == IDE_COMMAND_READ_DMA) ||
        (command == IDE_COMMAND_WRITE_DMA) ||
        (command == IDE_COMMAND_READ_DMA_EXT) ||
        (command == IDE_COMMAND_WRITE_DMA_EXT) ||
        (command == IDE_COMMAND_WRITE_DMA_FUA_EXT) ||
        (command == IDE_COMMAND_READ_DMA_QUEUED_EXT) ||
        (command == IDE_COMMAND_WRITE_DMA_QUEUED_EXT) ||
        (command == IDE_COMMAND_WRITE_DMA_QUEUED_FUA_EXT) ||
        (command == IDE_COMMAND_READ_MULTIPLE) ||
        (command == IDE_COMMAND_WRITE_MULTIPLE) ||
        (command == IDE_COMMAND_READ_MULTIPLE_EXT) ||
        (command == IDE_COMMAND_WRITE_MULTIPLE_EXT) ||
        (command == IDE_COMMAND_WRITE_MULTIPLE_FUA_EXT) ||
        (command == IDE_COMMAND_WRITE_DMA_QUEUED) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}    

__inline
BOOLEAN
NeedRequestSense (
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    PVOID               srbSenseBuffer = NULL;
    UCHAR               srbSenseBufferLength = 0;

    RequestGetSrbScsiData(Srb, NULL, NULL, &srbSenseBuffer, &srbSenseBufferLength);

    return ( IsAtapiCommand(srbExtension->AtaFunction) &&
             !IsRequestSenseSrb(srbExtension->AtaFunction) &&
             (Srb->SrbStatus == SRB_STATUS_ERROR) &&
             (srbSenseBuffer != NULL) &&
             (srbSenseBufferLength > 0) );
}

__inline
VOID
SetSenseData (
    _In_ PSENSE_DATA    SenseBuffer,
    _In_ UCHAR          SenseKey,
    _In_ UCHAR          ASC,
    _In_ UCHAR          ASCQ
    )
{
    SenseBuffer->ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
    SenseBuffer->Valid     = 1;
    SenseBuffer->AdditionalSenseLength = sizeof(SENSE_DATA) - RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
    SenseBuffer->SenseKey =  SenseKey;
    SenseBuffer->AdditionalSenseCode = ASC;
    SenseBuffer->AdditionalSenseCodeQualifier = ASCQ;
}


BOOLEAN
__inline
IsDeviceSupportsTrim (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    // word 169: bit 0 -- Trim function is supported.
    return (ChannelExtension->DeviceExtension[0].IdentifyDeviceData->DataSetManagementFeature.SupportsTrim == 1);
}


__inline
BOOLEAN
IsDeviceSupportsAN(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData
    )
{
    // This bit is actually from IDENTIFY_PACKET_DATA structure for ATAPI devices.
    return (IdentifyPacketData->SerialAtaFeaturesSupported.AsynchronousNotification == TRUE);
}

__inline
BOOLEAN
IsDeviceEnabledAN(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData
    )
{
    // This bit is actually from IDENTIFY_PACKET_DATA structure for ATAPI devices.
    return (IdentifyPacketData->SerialAtaFeaturesEnabled.AsynchronousNotification == TRUE);
}

__inline
BOOLEAN
IsExternalPort(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if ( (ChannelExtension->Px->CMD.HPCP) ||
         ((ChannelExtension->AdapterExtension->CAP.SXS) && (ChannelExtension->Px->CMD.ESP)) ||
         ((ChannelExtension->AdapterExtension->CAP.SMPS) && (ChannelExtension->Px->CMD.MPSP)) ) {
        //1. Px->CMD.HPCP indicates that the port is hot-pluggable. (both signal and power cable)
        //2. CAP.SXS && Px->CMD.ESP indicates that it's an ESATA port. (only signal cable)
        //3. CAP.SMPS && Px->CMD.MPSP indicates that Mechanical Switch is implemented on the port.
        return TRUE;
    }

    return FALSE;
}

__inline
BOOLEAN
IsLPMCapablePort(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    // if a port is marked eSATA port but not having mechanical switch nor supporting Cold Presence Detection, don't turn on LPM on it.
    if ( (ChannelExtension->Px->CMD.HPCP) ||
         ((ChannelExtension->AdapterExtension->CAP.SXS) && (ChannelExtension->Px->CMD.ESP)) ) {

        if ( ((ChannelExtension->AdapterExtension->CAP.SMPS == 0) || (ChannelExtension->Px->CMD.MPSP == 0)) &&
             (ChannelExtension->Px->CMD.CPD == 0) ) {
            return FALSE;
        }
    }

    return TRUE;
}

__inline
BOOLEAN
IsDeviceHybridInfoSupported(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if ( (ChannelExtension->DeviceExtension[0].DeviceParameters.AtaDeviceType ==  DeviceIsAta) &&
         (ChannelExtension->DeviceExtension[0].IdentifyDeviceData->SerialAtaFeaturesSupported.HybridInformation == 1) ) {

        return TRUE;
    }

    return FALSE;
}

__inline
BOOLEAN
IsDeviceHybridInfoEnabled(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if ( IsDeviceHybridInfoSupported(ChannelExtension) &&
         (ChannelExtension->DeviceExtension[0].IdentifyDeviceData->SerialAtaFeaturesEnabled.HybridInformation == 1) ) {

        return TRUE;
    }

    return FALSE;
}

__inline
BOOLEAN
IsDeviceGeneralPurposeLoggingSupported(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if (IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters) &&
        (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.GpLogging == 1) &&
        (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.BigLba == 1) ) {

        return TRUE;
    }

    return FALSE;
}

__inline
BOOLEAN
DeviceIncursSeekPenalty(
_In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
)
{
    if (IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        if (ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalMediaRotationRate >= 0x0401 &&
            ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalMediaRotationRate <= 0xFFFE) {
            //
            // NominalMediaRotationRate = 0 means no rate is reported.
            // NominalMediaRotationRate = 1 means there is no seek penalty (e.g. pure SSD).
            // NominalMediaRotationRate = 0x0401-0xFFFE is the rotation rate (i.e. incurs seek penalty).
            // All other values are reserved.
            //
            return TRUE;
        }
    }
    return FALSE;
}

__inline
BOOLEAN
DeviceIncursNoSeekPenalty(
_In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
)
{
    if (IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        if (ChannelExtension->DeviceExtension->IdentifyDeviceData->NominalMediaRotationRate == 1) {
            //
            // NominalMediaRotationRate = 0 means no rate is reported.
            // NominalMediaRotationRate = 1 means there is no seek penalty (e.g. pure SSD).
            // NominalMediaRotationRate = 0x0401-0xFFFE is the rotation rate (i.e. incurs seek penalty).
            // All other values are reserved.
            //
            return TRUE;
        }
    }
    return FALSE;
}

__inline
BOOLEAN
IsD3ColdAllowed(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    BOOLEAN allowed = TRUE;

    UNREFERENCED_PARAMETER(AdapterExtension);


    return allowed;
}

__inline
BOOLEAN
IsPortD3ColdEnabled(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    // if device is D3Cold capable, return TRUE.
    if (ChannelExtension->StateFlags.D3ColdEnabled == 1) {
        return TRUE;
    }

    return FALSE;
}

__inline
BOOLEAN
IsReceivingSystemPowerHints(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
)
{
    return (AdapterExtension->SystemPowerHintState != RaidSystemPowerUnknown);
}








__inline
BOOLEAN
AdapterPoFxEnabled (
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
)
{
    return (AdapterExtension->StateFlags.PoFxEnabled == 1);
}

__inline
BOOLEAN
PortPoFxEnabled (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    return (ChannelExtension->StateFlags.PoFxEnabled == 1);
}

__inline
BOOLEAN
AdapterAcquireActiveReference (
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _Inout_opt_ PBOOLEAN         InIdle
    )
{
    BOOLEAN idle = FALSE;
    BOOLEAN referenceAcquired = FALSE;

    if (AdapterPoFxEnabled(AdapterExtension)) {
        ULONG status;
        status = StorPortPoFxActivateComponent(AdapterExtension,
                                               NULL,
                                               NULL,
                                               0,
                                               0);
        // STOR_STATUS_BUSY indicates that ActivateComponent is not completed yet.
        idle = (status == STOR_STATUS_BUSY);

        if ( (status == STOR_STATUS_SUCCESS) || (status == STOR_STATUS_BUSY) ) {
            referenceAcquired = TRUE;
        }
    }

    if (InIdle != NULL) {
        *InIdle = idle;
    }

    return referenceAcquired;
}

__inline
VOID
AdapterReleaseActiveReference (
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    if (AdapterPoFxEnabled(AdapterExtension)) {
        StorPortPoFxIdleComponent(AdapterExtension,
                                  NULL,
                                  NULL,
                                  0,
                                  0);
    }
}

__inline
BOOLEAN
PortAcquireActiveReference (
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb,
    _Inout_opt_ PBOOLEAN            InIdle
    )
{
    BOOLEAN idle = FALSE;
    BOOLEAN referenceAcquired = FALSE;

    if (PortPoFxEnabled(ChannelExtension)) {
        ULONG status;

        status = StorPortPoFxActivateComponent(ChannelExtension->AdapterExtension,
                                               (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                               (PSCSI_REQUEST_BLOCK)Srb,
                                               0,
                                               0);
        // STOR_STATUS_BUSY indicates that ActivateComponent is not completed yet.
        idle = (status == STOR_STATUS_BUSY);

        if ( (status == STOR_STATUS_SUCCESS) || (status == STOR_STATUS_BUSY) ) {
            if (Srb != NULL) {
                PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
                SETMASK(srbExtension->Flags, ATA_FLAGS_ACTIVE_REFERENCE);
            }

            referenceAcquired = TRUE;
        }

    } else {
        UNREFERENCED_PARAMETER(Srb);
    }

    if (InIdle != NULL) {
        *InIdle = idle;
    }

    return referenceAcquired;
}

__inline
VOID
PortReleaseActiveReference (
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
    if (PortPoFxEnabled(ChannelExtension)) {
        if (Srb != NULL) {
            PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
            CLRMASK(srbExtension->Flags, ATA_FLAGS_ACTIVE_REFERENCE);
        }

        StorPortPoFxIdleComponent(ChannelExtension->AdapterExtension,
                                  (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                  (PSCSI_REQUEST_BLOCK)Srb,
                                  0,
                                  0);
    } else {
        UNREFERENCED_PARAMETER(Srb);
    }
}

__inline
BOOLEAN
DeviceIdentificationComplete(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    ULONG i;

    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if ( (AdapterExtension->PortExtension[i] != NULL) &&
             (AdapterExtension->PortExtension[i]->DeviceExtension->DeviceParameters.AtaDeviceType == DeviceUnknown) ) {
            return FALSE;
        }
    }

    return TRUE;
}

__inline
ULONG
RequestGetDataTransferLength (
    _In_ PSTORAGE_REQUEST_BLOCK Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if (srbExtension->DataTransferLength > 0) {
        return srbExtension->DataTransferLength;
    } else {
        return SrbGetDataTransferLength(Srb);
    }
}

__inline
VOID
RequestSetDataTransferLength (
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_ ULONG                  Length
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    if (srbExtension->DataTransferLength > 0) {
        srbExtension->DataTransferLength = Length;
    } else {
        SrbSetDataTransferLength(Srb, Length);
    }

    return;
}

__inline
UCHAR
NumberOfSetBits (
    _In_ ULONG Value
    )
/*++
    This routine emulates the __popcnt intrinsic function.

Return Value:
    Count of '1's in the ULONG value
--*/
{
    //
    // Partition into groups of bit pairs. Compute population count for each
    // bit pair.
    //
    Value -= (Value >> 1) & 0x55555555;

    //
    // Sum population count of adjacent pairs into quads.
    //
    Value = (Value & 0x33333333) + ((Value >> 2) & 0x33333333);

    //
    // Sum population count of adjacent quads into octets. Lower quad in each
    // octet has desired sum and upper quad is garbage.
    //
    Value = (Value + (Value >> 4)) & 0x0F0F0F0F;

    //
    // The lower quads in each octet must now be accumulated by multiplying with
    // a magic multiplier:
    //
    //   0p0q0r0s * 0x01010101 =         :0p0q0r0s
    //                                 0p:0q0r0s
    //                               0p0q:0r0s
    //                             0p0q0r:0s
    //                           000pxxww:vvuutt0s
    //
    // The octet vv contains the final interesting result.
    //
    Value *= 0x01010101;

    return (UCHAR)(Value >> 24);
}

__inline
VOID
AhciInterruptSpinlockAcquire(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ ULONG                   MessageId,
    _In_ PSTOR_LOCK_HANDLE       LockHandle
    )
/*++
    This routine acquires spinlock based on interrupt mode StorAHCI operates in.

    It grabs the adapter interrupt spinlock if it's not in InterruptMessagePerPort mode, or the caller
    asks for it by using 0xFFFFFFFF as MessageId value.

Return Value:
    None

--*/
{
    if ((AdapterExtension->StateFlags.InterruptMessagePerPort == 0) ||
        (MessageId == MAXULONG)) {

        StorPortAcquireSpinLock(AdapterExtension, InterruptLock, NULL, LockHandle);
    } else {
        ULONG oldIrql = 0;

        NT_ASSERT(MessageId <= AdapterExtension->HighestPort);

        StorPortAcquireMSISpinLock(AdapterExtension, MessageId, &oldIrql);
        LockHandle->Context.OldIrql = (UCHAR)oldIrql;
    }

    return;
}

__inline
VOID
AhciInterruptSpinlockRelease(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ ULONG                   MessageId,
    _In_ PSTOR_LOCK_HANDLE       LockHandle
    )
/*++
    This routine releases spinlock based on interrupt mode StorAHCI operates in.

    It releases the adapter interrupt spinlock if it's not in InterruptMessagePerPort mode, or the caller
    asks for it by using 0xFFFFFFFF as MessageId value.

Return Value:
    None

--*/
{
    _Analysis_assume_lock_acquired_(LockHandle);

    if ((AdapterExtension->StateFlags.InterruptMessagePerPort == 0) ||
        (MessageId == MAXULONG)) {

        StorPortReleaseSpinLock(AdapterExtension, LockHandle);
    } else {

        StorPortReleaseMSISpinLock(AdapterExtension, MessageId, LockHandle->Context.OldIrql);
    }

    return;
}

VOID
RecordExecutionHistory(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    ULONG Function
  );

VOID
RecordInterruptHistory(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    ULONG PxIS,
    ULONG PxSSTS,
    ULONG PxSERR,
    ULONG PxCI,
    ULONG PxSACT,
    ULONG Function
  );

VOID
Set_PxIE(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_INTERRUPT_ENABLE IE
    );


__inline
BOOLEAN
IsFuaSupported(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    return (ChannelExtension->DeviceExtension->DeviceParameters.StateFlags.FuaSupported == 1);
}

__inline
BOOLEAN
IsDeviceSupportsHIPM(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyDeviceData
    )
{
    return (IdentifyDeviceData->SerialAtaCapabilities.HIPM == TRUE);
}

__inline
BOOLEAN
IsDeviceSupportsDIPM(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyDeviceData
    )
{
    return (IdentifyDeviceData->SerialAtaFeaturesSupported.DIPM == TRUE);
}

__inline
BOOLEAN
NoLpmSupport(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if (ChannelExtension->AdapterExtension->StateFlags.StoppedState == 1) {
        // adapter is stopped, Px registers are not accessible.
        return TRUE;
    }

    if (ChannelExtension->AdapterExtension->CAP.PSC == 0) {
        // adapter doesn't support Partial state
        return TRUE;
    }

    // return TRUE if HIPM and DIPM are not supported.
    return ( (ChannelExtension->AdapterExtension->CAP.SALP == 0) &&
             (!IsDeviceSupportsDIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData)) );
}


__inline
BOOLEAN
NeedToSetTransferMode(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    UNREFERENCED_PARAMETER(ChannelExtension); // make WDK sample build clean
    return FALSE;
}

__inline
BOOLEAN
IsSingleIoDevice(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    UNREFERENCED_PARAMETER(AdapterExtension); // make WDK sample build clean
    return FALSE;
}

__inline
BOOLEAN
AdapterResetInInit(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    UNREFERENCED_PARAMETER(AdapterExtension); // make WDK sample build clean
    return FALSE;
}

__inline
BOOLEAN
IgnoreHotPlug(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    return (ChannelExtension->StateFlags.IgnoreHotplugInterrupt == 1);
}

__inline
BOOLEAN
AdapterNoNonQueuedErrorRecovery(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    UNREFERENCED_PARAMETER(AdapterExtension); // make WDK sample build clean
    return FALSE;
}

__inline
BOOLEAN
CloResetEnabled(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    UNREFERENCED_PARAMETER(AdapterExtension); // make WDK sample build clean
    return (AdapterExtension->CAP.SCLO == 1);
}

__inline
BOOLEAN
IsNCQSupported(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if ( IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters) &&
         (ChannelExtension->AdapterExtension->CAP.SNCQ == 1) &&
         (ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NCQ == 1) &&
         (ChannelExtension->DeviceExtension->IdentifyDeviceData->QueueDepth > 1) ) {
        //Queue Depth is a 0 based value (i.e. 0x0 == 1). StorAHCI reserves slot 0 for internal use.
        return TRUE;
    }
    return FALSE;
}

__inline
BOOLEAN
IsFirmwareUpdateSupported(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    BOOLEAN support;


    if (ChannelExtension->DeviceExtension->FirmwareUpdate.DmOffsetsDeferredSupported) {
        support =  TRUE;
    } else {
        support = FALSE;
    }

    return support;
}

__inline
UCHAR
GetFirmwareUpdateCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if (ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.DownloadMicrocodeDmaSupported == 1) {
        return IDE_COMMAND_DOWNLOAD_MICROCODE_DMA;
    } else if (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.DownloadMicrocode == 1) {
        return IDE_COMMAND_DOWNLOAD_MICROCODE;
    } else {
        return IDE_COMMAND_NOT_VALID;
    }
}


__inline
ULONG
GetOccupiedSlots (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    return ( ChannelExtension->SlotManager.CommandsIssued |
             ChannelExtension->SlotManager.NCQueueSlice |
             ChannelExtension->SlotManager.NormalQueueSlice |
             ChannelExtension->SlotManager.SingleIoSlice |
             ChannelExtension->SlotManager.CommandsToComplete );
}

__inline
BOOLEAN
ErrorRecoveryIsPending (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    return ( (ChannelExtension->StateFlags.CallAhciReset == 1) ||
             (ChannelExtension->StateFlags.CallAhciNcqErrorRecovery == 1) ||
             (ChannelExtension->StateFlags.CallAhciReportBusChange == 1) ||
             (ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery == 1) );
}

__inline
BOOLEAN
IsMiniportInternalSrb (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    return ( (Srb == (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb) || 
             (Srb == (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Sense.Srb) );
}

__inline
VOID
PortClearPendingInterrupt (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong, (ULONG)~0);
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, (ULONG)~0);
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, ChannelExtension->AdapterExtension->IS, (1 << ChannelExtension->PortNumber));
}

__inline
BOOLEAN
PartialToSlumberTransitionIsAllowed (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ AHCI_COMMAND            CMD,
    _In_ ULONG                   CI,
    _In_ ULONG                   SACT
    )
{
    if ( (CI != 0) || (SACT != 0) ) {
        //device still has request pending
        return FALSE;
    }
    
    if ((ChannelExtension->LastUserLpmPowerSetting  & 0x3) == 0) {
        //Neither HIPM nor DIPM is allowed. e.g. LastUserLpmPowerSetting --- bit 0: HIPM; bit 1: DIPM
        return FALSE;
    }

    if ((ChannelExtension->AutoPartialToSlumberInterval == 0)) {
        //Software auto Partial to Slumber is not enabled yet.
        return FALSE;
    }

    if (ChannelExtension->StartState.ChannelNextStartState != StartComplete) {
        //port is not started yet
        return FALSE;
    }

    if ( NoLpmSupport(ChannelExtension) || !IsLPMCapablePort(ChannelExtension) ) {
        //link power management is not supported
        return FALSE;
    }

    if (!IsDeviceSupportsHIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData)) {
        // HIPM is not supported.
        return FALSE;
    }


    if ( ( (ChannelExtension->AdapterExtension->CAP.SALP == 0) || (CMD.ALPE == 0) ) &&
         (!IsDeviceSupportsDIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData)) ) {
        //Neither HIPM nor DIPM is enabled.
        return FALSE;
    }

    if ( ((ChannelExtension->LastUserLpmPowerSetting  & 0x2) != 0) &&
         (ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.DeviceAutoPS == 1) &&
         (ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaFeaturesEnabled.DeviceAutoPS == 1) )   {
        //DIPM is enabled; device supports and enabled Device auto Partial to Slumber.
        //note that IdentifyDeviceData applies to both ATA and ATAPI devices.
        return FALSE;
    }

    if ( (ChannelExtension->AdapterExtension->CAP.SALP == 1) &&
         (CMD.ALPE != 0) &&
         ( (CMD.ASP == 1) ||
           ( (ChannelExtension->AdapterExtension->CAP2.APST != 0) && (CMD.APSTE != 0) ) ) ) {
        // HIPM is enabled. AND
        //     either Host initiates Slumber automatically, OR
        //     Host supports and enabled auto Partial to Slumber.
        return FALSE;
    }

    return TRUE;
}

__inline
ULONG
GetHybridMaxLbaRangeCountForChangeLba (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    //
    // If CacheBehavior is supported, disk doesn't move data from primary medium to caching medium when
    // executes HybridChangeByLba command. Use 64 as default MaxLbaRangeCountForChangeLba value.
    // Otherwise, use 8 as default value.
    //
    if (ChannelExtension->DeviceExtension->HybridInfo.SupportedOptions.SupportCacheBehavior == 1) {
        return 64;
    } else {
        return 8;
    }
}

__inline
BOOLEAN
SupportsEnhancedNcqErrorRecovery (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    if ((ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.NcqCommandError == 1) &&
        (ChannelExtension->DeviceExtension->IoRecord.NcqReadLogErrorCount <= 5)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

__inline
VOID
SetReturnRegisterValues (
    _In_ PAHCI_CHANNEL_EXTENSION        ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK         Srb,
    _In_opt_ PGP_LOG_NCQ_COMMAND_ERROR  NcqErrorLog
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    PCDB                cdb = SrbGetCdb(Srb);

    if ((IsReturnResults(srbExtension->Flags) == FALSE) || (srbExtension->ResultBuffer == NULL)) {
        return;
    }

    if ((cdb != NULL) && (cdb->CDB10.OperationCode == SCSIOP_ATA_PASSTHROUGH16)) {
        // for ATA PASS THROUGH 16 command, return Descriptor Format Sense Data, including ATA Status Return info.
        if (srbExtension->ResultBufferLength >= sizeof(DESCRIPTOR_SENSE_DATA)) {
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

            if (NcqErrorLog == NULL) {
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
            } else {
                ataStatus->Error = NcqErrorLog->Error;
                ataStatus->SectorCount7_0 = NcqErrorLog->Count7_0;
                ataStatus->LbaLow7_0 = NcqErrorLog->LBA7_0;
                ataStatus->LbaMid7_0 = NcqErrorLog->LBA15_8;
                ataStatus->LbaHigh7_0 = NcqErrorLog->LBA23_16;
                ataStatus->Device = NcqErrorLog->Device;
                ataStatus->Status = NcqErrorLog->Status;

                if (Is48BitCommand(srbExtension->Flags)) {
                    ataStatus->SectorCount15_8 = NcqErrorLog->Count15_8;
                    ataStatus->LbaLow15_8 = NcqErrorLog->LBA31_24;
                    ataStatus->LbaMid15_8 = NcqErrorLog->LBA39_32;
                    ataStatus->LbaHigh15_8 = NcqErrorLog->LBA47_40;
                }
            }

        } else {
            NT_ASSERT(FALSE);
        }
    } else {
        PATA_TASK_FILE  returnTaskFile;
        returnTaskFile = (PATA_TASK_FILE)srbExtension->ResultBuffer;

        if (NcqErrorLog == NULL) {
            returnTaskFile->Current.bCommandReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.Status;
            returnTaskFile->Current.bFeaturesReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.Error;
            returnTaskFile->Current.bCylHighReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylHigh;
            returnTaskFile->Current.bCylLowReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylLow;
            returnTaskFile->Current.bDriveHeadReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.Dev_Head;
            returnTaskFile->Current.bSectorCountReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorCount;
            returnTaskFile->Current.bSectorNumberReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorNumber;
            //if 48bit, get all of it
            if(Is48BitCommand(srbExtension->Flags) && (srbExtension->ResultBufferLength >= sizeof(ATA_TASK_FILE))) {
                returnTaskFile->Previous.bCommandReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.Status;
                returnTaskFile->Previous.bFeaturesReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.Error;
                returnTaskFile->Previous.bCylHighReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylHigh_Exp;
                returnTaskFile->Previous.bCylLowReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.CylLow_Exp;
                returnTaskFile->Previous.bDriveHeadReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.Dev_Head;
                returnTaskFile->Previous.bSectorCountReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorCount_Exp;
                returnTaskFile->Previous.bSectorNumberReg = ChannelExtension->ReceivedFIS->D2hRegisterFis.SectorNum_Exp;
            }
        } else {
            returnTaskFile->Current.bCommandReg = NcqErrorLog->Status;
            returnTaskFile->Current.bFeaturesReg = NcqErrorLog->Error;
            returnTaskFile->Current.bCylHighReg = NcqErrorLog->LBA23_16;
            returnTaskFile->Current.bCylLowReg = NcqErrorLog->LBA15_8;
            returnTaskFile->Current.bDriveHeadReg = NcqErrorLog->Device;
            returnTaskFile->Current.bSectorCountReg = NcqErrorLog->Count7_0;
            returnTaskFile->Current.bSectorNumberReg = NcqErrorLog->LBA7_0;
            //if 48bit, get all of it
            if(Is48BitCommand(srbExtension->Flags) && (srbExtension->ResultBufferLength >= sizeof(ATA_TASK_FILE))) {
                returnTaskFile->Previous.bCommandReg = NcqErrorLog->Status;
                returnTaskFile->Previous.bFeaturesReg = NcqErrorLog->Error;
                returnTaskFile->Previous.bCylHighReg = NcqErrorLog->LBA47_40;
                returnTaskFile->Previous.bCylLowReg = NcqErrorLog->LBA39_32;
                returnTaskFile->Previous.bDriveHeadReg = NcqErrorLog->Device;
                returnTaskFile->Previous.bSectorCountReg = NcqErrorLog->Count15_8;
                returnTaskFile->Previous.bSectorNumberReg = NcqErrorLog->LBA31_24;
            }
        }
    }

    // set flag SRB_STATUS_AUTOSENSE_VALID so that Storport will copy it back to original Sense Buffer
    Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

    return;

}

__inline
BOOLEAN
NeedsPuisSpinUpOnPowerUp(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    //
    // If this is a non-hybrid rotational drive that supports Power Up in
    // Standby (PUIS) then we may need to send a PUIS spin-up command if PUIS
    // is enabled when we power on the device during runtime.
    //
    if (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.ManualPowerUp &&
        ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.PowerUpInStandby &&
        DeviceIncursSeekPenalty(ChannelExtension) &&
        IsDeviceHybridInfoSupported(ChannelExtension) == FALSE) {
        return TRUE;
    }

    return FALSE;
}

__inline
STORAGE_DEVICE_FORM_FACTOR
AtaFormFactorToStorageFormFactor(
    _In_ USHORT AtaFormFactor
    )
{
    STORAGE_DEVICE_FORM_FACTOR formFactor = FormFactorUnknown;

    switch (AtaFormFactor) {
    case 1:
    case 2:
        formFactor = FormFactor3_5;
        break;

    case 3:
        formFactor = FormFactor2_5;
        break;

    case 4:
        formFactor = FormFactor1_8;
        break;

    case 5:
        formFactor = FormFactor1_8Less;
        break;

    case 6:
        formFactor = FormFactormSata;
        break;

    case 7:
        formFactor = FormFactorM_2;
        break;

    case 8:
        formFactor = FormFactorEmbedded;
        break;

    case 9:
        formFactor = FormFactorMemoryCard;
        break;

    default:
        break;
    }

    return formFactor;
}

__inline
BOOLEAN
IsAdapterRemoved(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    //
    // When the adapter is removed, its registers are read as
    // all 0xF's by the PCI bus. We use the PxSSTS register because
    // it has several reserved bits which will unlikely be defined
    // in the lifetime of the spec.
    //
    ULONG ssts = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SSTS.AsUlong);
    return (ssts == MAXULONG);
}


VOID
PortBusChangeProcess (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

HW_TIMER_EX AhciBusChangeTimerCallback;

PAHCI_MEMORY_REGISTERS
GetABARAddress(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo
    );

VOID
AhciCompleteJustSlottedRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    );

VOID
AhciCompleteRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    );

BOOLEAN
UpdateSetFeatureCommands(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR OldFeatures,
    _In_ UCHAR NewFeatures,
    _In_ UCHAR OldSectorCount,
    _In_ UCHAR NewSectorCount
  );

VOID
GetAvailableSlot(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PSTORAGE_REQUEST_BLOCK  Srb
    );

VOID
ReleaseSlottedCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR SlotNumber,
    _In_ BOOLEAN AtDIRQL
    );

VOID
RestorePreservedSettings(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    );

VOID
AhciPortIssueInitCommands(
    PAHCI_CHANNEL_EXTENSION ChannelExtension
  );

_Success_(return != FALSE)
BOOLEAN
CompareId (
    _In_opt_ PSTR DeviceId,
    _In_ ULONG  DeviceIdLength,
    _In_opt_ PZZSTR TargetId,
    _In_ ULONG  TargetIdLength,
    _Inout_opt_ PULONG Value
);

ULONG
GetStringLength (
    _In_ PSTR   String,
    _In_ ULONG  MaxLength
    );

__inline
VOID
AhciUlongIncrement(
    _Inout_ PULONG Value
    )
{
    if (Value != NULL &&
        *Value != MAXULONG) {
        (*Value)++;
    }
}


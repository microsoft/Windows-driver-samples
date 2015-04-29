/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    pnppower.c

Abstract:

    This file contains function of pnp and power process of the AHCI miniport.


Notes:

Revision History:

--*/

#pragma warning(push)
#pragma warning(disable:26015) //26015: "Potential overflow using expression 'outParams->DriverStatus.bDriverError'. Buffer access is apparently unbounded by the buffer size.
                               //Output buffer cannot be checked for size.  ATAport provides this validation check as the input buffer size and output buffer size are 2 of the 4 parameters passed in on the SMART IRP.  Storport doesn’t do this and the miniport doesn’t get the IRP so it cannot do this for itself.  This is just the condition of a legacy IOCTL.
                                //26015: "Potential overflow using expression 'nRB->NRBStatus' Buffer access is apparently unbounded by the buffer size.
                                //The same is true for the NVCache IOCTL.  Instead of the output buffer, this time it is the NVCache_Request_Block.
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

#include "generic.h"
#include "acpiioct.h"

//_DSM for Link Power is uniquely identified by the following UUID:
//  E4DB149B-FCFE-425B-A6D8-92357D78FC7F
static const GUID LINK_POWER_ACPI_DSM_GUID = {
    0xE4DB149B,
    0xFCFE,
    0x425B,
    { 0xA6,0xD8,0x92,0x35,0x7D,0x78,0xFC,0x7F }
  };

VOID
LogPageDiscoveryCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    );


BOOLEAN
AhciPortInitialize(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
/*++
    This function is used to start an AHCI port
It assumes:

Called By:
    HwFindAdapter

Affected Variables/Registers:
    ChannelExtension->StateFlags
    ChannelExtension->CommandList
    ChannelExtension->ReceivedFIS

    PX.CLB,PX.CLBU
    PX.FB,PX.FBU
    CMD.ST
It performs:
    (overview)
    1. Start with some defensive structure checking and variable initialization
    2. Initialize the ChannelConfiguration structure (final steps)
    3. Enable the AHCI interface as per AHCI 1.1 section 10.1.2 (final steps)
    4. Allocate memory for the CommandList, the Receive FIS buffer, and SRB Extension.
    5. Start the channel processing commands.
    (details)
    1.1 Initialize variables
    1.2 Verify the Channel Configuration
    2.1 Initialize the ChannelConfiguration structure
    2.2 Initialize the Channel's base address and the controller's Interrupt Status register address
    3.1 Enable the AHCI interface
        AHCI 1.1 Section 10.1.2 - 5.
        "For each implemented port, system software shall allocate memory for and program:
            •    PxCLB and PxCLBU (if CAP.S64A is set to ‘1’)
            •    PxFB and PxFBU (if CAP.S64A is set to ‘1’)
            It is good practice for system software to ‘zero-out’ the memory allocated and referenced by PxCLB and PxFB.  After setting PxFB and PxFBU to the physical address of the FIS receive area, system software shall set PxCMD.FRE to ‘1’."
    3.2 Enable Interrupts on the Channel
        AHCI 1.1 Section 10.1.2 - 7.
        "Determine which events should cause an interrupt, and set each implemented port’s PxIE register with the appropriate enables."
        Note: Due to the multi-tiered nature of the AHCI HBA’s interrupt architecture, system software must always ensure that the PxIS (clear this first) and IS.IPS (clear this second) registers are cleared to ‘0’ before programming the PxIE and GHC.IE registers. This will prevent any residual bits set in these registers from causing an interrupt to be asserted.

    4.1 Allocate memory for the CommandList, the Receive FIS buffer and SRB Extension
        Now is the time to allocate memory that will be used for controller and per request structures.
        In AHCI, the controller structures are both the command header list and the received FIS buffer.
        The per request structure will be received through the SRB and will be used to make a Command Table
        The mechanism for requesting all of this memory is AtaPortGetUnCachedExtension.
        NOTE! AtaPortGetUnCachedExtension can only be called while processing a HwControl IdeStart.
        Also NOTE! In order to perform crashdump/hibernate the UncachedExtensionSize cannot be larger than 30K.

        The call to AtaPortGetUnCachedExtension is complicated by alignment restrictions that an AHCI controller has so here are the rules:
        - Command List Base Addresses must be 1K aligned, and the Command list is (sizeof (AHCI_COMMAND_HEADER) * cap.NCS), which is some multiple of 32 bytes in length.
        - The FIS Base Address must be 256 aligned, and the FIS Receive buffer is sizeof (AHCI_RECEIVED_FIS), 256 bytes in length.
        - The Command Table must be 128 aligned, and is sizeof(AHCI_COMMAND_TABLE), 1280 bytes in length thanks to some padding in the AHCI_COMMAND_TABLE structure.

        The alignment of the addresses (virtual and physical) returned by the function follow these rules
        - the address returned by AtaPortGetUnCachedExtension will have both its virtual and physical addresses page aligned
        - the memory received through the SRB will either be physically and virtually 4K aligned or SRBExtensionSize aligned. The first allocation will be on a 4K boundary the address of the second will be SRBExtensionSize larger than the first, the third will be SRBExtensionSize larger than the second, etc.

        Since the Command Header must be 1K aligned and the uncached extension starts 4K aligned, this works.
        However, the Command Header is variable and must be padded so the Received FIS is on a 256 boundary.
        Therefore the number of Command Headers must be 256/32 = 8. Round cap.NCS to the next multiple of 8
    4.2 Setup the CommandList
        Although the pointer returned from AtaPortGetUnCachedExtension is useful to this driver, it does the controller no good and can't be used in CLB. The VIRTUAL address must be translated into the PHYSICAL address before being written to the CLB register as the controller doesn't have the CPU's virtual address translation tables.  AtaPortGetPhysicalAddress Returns the physical address for the given Va. The va has to be an offset into any one of the following buffers.
            - SRB's data buffer
            - SRB's SrbExtension
            - Miniport's uncached extension
    4.3 Setup the Receive FIS buffer
        Handle the Receive FIS buffer the same as 4.2 Command List
    4.4 Setup the Local SRB Extension
    5.1 Enable Command Processing
    5.2 Initialize the ChannelConfiguration structure
        ChannelConfiguration->ChannelNumber and    ChannelConfiguration->ChannelResources are kept default values.
        If it is found that CI and/or SACT can be changed from a 1 to 0, Number of overlapped requests becomes 1.
        Number of overlapped requests is a 1 based number (1=1, 2=2, etc.), CAP.NCS is a 0 based number.
    5.3 START COMMAND PROCESSING
Return Values:
    The miniport driver returns TRUE if it successfully execute the whole function.
    Any errors causes the function to return FALSE and prevents the channel from loading. This ultimately causes a yellow '!' to show up on the channel in device manager.

NOTE: as this routine is invoked from FindAdapter where the adapter might not be fully initialized, do not retrieve registry information.
--*/
    PAHCI_ADAPTER_EXTENSION adapterExtension = NULL;
    PAHCI_MEMORY_REGISTERS  abar = NULL;

   //these are throw away variables
    ULONG   mappedLength = 0;

  //1.1 Initialize variables
    adapterExtension = ChannelExtension->AdapterExtension;
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000024);//AhciPortInitialize
    }

    ChannelExtension->CurrentCommandSlot = 1; //slot 0 is reserved for internal commands,
    ChannelExtension->StateFlags.IgnoreHotplugInterrupt = TRUE;

    abar = (PAHCI_MEMORY_REGISTERS)adapterExtension->ABAR_Address;
    ChannelExtension->Px = &abar->PortList[ChannelExtension->PortNumber];

  // NonCachedExtension is for CommandList, Receive FIS, SRB Extension for Local SRB and Sense SRB., READ_LOG/IDENTIFY buffer
  // (sizeof(AHCI_COMMAND_HEADER) * paddedNCS) + sizeof(AHCI_RECEIVED_FIS) + 2*sizeof(AHCI_SRB_EXTENSION) + sizeof(AHCI_READ_LOG_EXT_DATA);

  //4.2 Setup the CommandList
    ChannelExtension->CommandListPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension, NULL, (PVOID)ChannelExtension->CommandList, &mappedLength);
    if (ChannelExtension->CommandListPhysicalAddress.QuadPart == 0) {
        RecordExecutionHistory(ChannelExtension, 0xff02);//Command List Failed
        return FALSE;
    }
  //3.1.1 PxCLB and PxCLBU (AHCI 1.1 Section 10.1.2 - 5)
    if ( (ChannelExtension->CommandListPhysicalAddress.LowPart % 1024) == 0 ) {
        // validate the alignment is fine
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->CLB.AsUlong, ChannelExtension->CommandListPhysicalAddress.LowPart);
    }else{
        RecordExecutionHistory(ChannelExtension, 0xff03);//Command List alignment failed
        return FALSE;
    }
    if (adapterExtension->CAP.S64A) { //If the controller supports 64 bits, write the high part too
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->CLBU, ChannelExtension->CommandListPhysicalAddress.HighPart);
    }

  //4.3 Setup the Receive FIS buffer
    ChannelExtension->ReceivedFisPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension, NULL, (PVOID)ChannelExtension->ReceivedFIS, &mappedLength);
    if (ChannelExtension->ReceivedFisPhysicalAddress.QuadPart == 0) {
        RecordExecutionHistory(ChannelExtension, 0xff04);//Receive FIS failed
        return FALSE;
    }

  //3.1.2 PxFB and PxFBU (AHCI 1.1 Section 10.1.2 - 5)
    if ((ChannelExtension->ReceivedFisPhysicalAddress.LowPart % 256) == 0) {
        // validate the alignment is fine
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->FB.AsUlong, ChannelExtension->ReceivedFisPhysicalAddress.LowPart);
    } else {
        RecordExecutionHistory(ChannelExtension, 0xff05);//Receive FIS alignment failed
        return FALSE;
    }
    if (adapterExtension->CAP.S64A) { //If the controller supports 64 bits, write the high part too
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->FBU, ChannelExtension->ReceivedFisPhysicalAddress.HighPart);
    }

  //4.4 Setup the Local SRB Extension
    ChannelExtension->Local.SrbExtensionPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension, NULL, (PVOID)ChannelExtension->Local.SrbExtension, &mappedLength);
    ChannelExtension->Sense.SrbExtensionPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension, NULL, (PVOID)ChannelExtension->Sense.SrbExtension, &mappedLength);

  //4.6 Setup Device Identify Data and Inquiry Data buffers
    ChannelExtension->DeviceExtension[0].IdentifyDataPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension, NULL, (PVOID)ChannelExtension->DeviceExtension[0].IdentifyDeviceData, &mappedLength);
    ChannelExtension->DeviceExtension[0].InquiryDataPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension, NULL, (PVOID)ChannelExtension->DeviceExtension[0].InquiryData, &mappedLength);


  //4.8 Setup STOR_ADDRESS for the device. StorAHCI uses Bus/Target/Lun addressing model, thus uses STOR_ADDRESS_TYPE_BTL8.
  //        Port - not need to be set by miniport, Storport has this knowledge. miniport can get the value by calling StorPortGetSystemPortNumber().
  //        Path - StorAHCI reports (highest implemented port number + 1) as bus number, thus "port number" will be "Path" value.
  //        Target - StorAHCI only supports single device on each port, the "Target" value will be 0.
  //        Lun - StorAHCI only supports single device on each port, the "Lun" value will be 0.
    ChannelExtension->DeviceExtension[0].DeviceAddress.Type = STOR_ADDRESS_TYPE_BTL8;
    ChannelExtension->DeviceExtension[0].DeviceAddress.Port = 0;
    ChannelExtension->DeviceExtension[0].DeviceAddress.AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;
    ChannelExtension->DeviceExtension[0].DeviceAddress.Path = (UCHAR)ChannelExtension->PortNumber;
    ChannelExtension->DeviceExtension[0].DeviceAddress.Target = 0;
    ChannelExtension->DeviceExtension[0].DeviceAddress.Lun = 0;

    //
    // Initialize device power state to D0.
    //
    ChannelExtension->DevicePowerState = StorPowerDeviceD0;

  //3.2 Clear Enable Interrupts on the Channel (AHCI 1.1 Section 10.1.2 - 7)
  //We will enable interrupt after channel started
    PortClearPendingInterrupt(ChannelExtension);

  //5.1 Enable Command Processing
    ChannelExtension->StateFlags.Initialized = TRUE;

    if (adapterExtension->CAP.NCS > 0) { //this leaves one emergency slot free if possible, as CAP.NCS is 0-based.
        ChannelExtension->MaxPortQueueDepth = (UCHAR)adapterExtension->CAP.NCS;
    } else {
        ChannelExtension->MaxPortQueueDepth = 1;
    }

    if ( IsSingleIoDevice(adapterExtension) || IsDumpMode(adapterExtension) ) {
        ChannelExtension->MaxPortQueueDepth = 1;
    }

    ChannelExtension->LastActiveSlot = 0;
    ChannelExtension->DeviceExtension[0].DeviceParameters.MaxDeviceQueueDepth = ChannelExtension->MaxPortQueueDepth;

    if (!IsDumpMode(adapterExtension)) {
        if (AdapterResetInInit(adapterExtension)) {
            P_NotRunning(ChannelExtension, ChannelExtension->Px);
            AhciCOMRESET(ChannelExtension, ChannelExtension->Px);
        }
    }

    RecordExecutionHistory(ChannelExtension, 0x10000024);//Exit AhciPortInitialize
    return TRUE;
}


BOOLEAN
AhciAdapterPowerUp(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
/*++
    Indicates that the adapter is being powered up.
    Anything that doesn't persist across a power cycle needs to be done here.
It assumes:
    PCI Ensures the HBA is in D0 (Offset PMCAP + 4h: PMCS[0,1])
Called by:
    AhciAdapterControl

It performs:
    Enables the AHCI Interface and global Interrupts
Affected Variables/Registers:
    GHC.AE, GHC.IE
Return Values:
    TRUE always.
--*/                                    //Used to enable the AHCI interface
{
    ULONG   i;
    AHCI_Global_HBA_CONTROL ghc;
    PAHCI_MEMORY_REGISTERS  abar = (PAHCI_MEMORY_REGISTERS)AdapterExtension->ABAR_Address;

    AdapterExtension->StateFlags.PowerDown = 0;

    // adapter is on its way power up. there will be no power down request coming in before this function finishes.
    // thus there is no need to call AdapterAcquireActiveReference;

    ghc.AsUlong = StorPortReadRegisterUlong(AdapterExtension, &abar->GHC.AsUlong);
    if (ghc.AE == 0) {
        ghc.AsUlong = 0;
        ghc.AE = 1;
        StorPortWriteRegisterUlong(AdapterExtension, &abar->GHC.AsUlong, ghc.AsUlong);
    }
    if (ghc.IE == 0) {
        ghc.IE = 1;
        StorPortWriteRegisterUlong(AdapterExtension, &abar->GHC.AsUlong, ghc.AsUlong);
    }

    // Power up all ports that don't have a device present.
    // There is protection method in AhciPortPowerUp() to only allow it run once.
    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if ( (AdapterExtension->PortExtension[i] != NULL) &&
             (AdapterExtension->PortExtension[i]->StateFlags.PowerDown == TRUE) &&
             (AdapterExtension->PortExtension[i]->DeviceExtension[0].DeviceParameters.AtaDeviceType == DeviceNotExist)) {
            AhciPortPowerUp(AdapterExtension->PortExtension[i]);
        }
    }

    return TRUE;
}

BOOLEAN
AhciAdapterPowerDown(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
/*++
    Indicates that the adapter is being powered down.
It assumes:
    PCI powers down the HBA after this function returns: D3 Offset PMCAP + 4h: PMCS[0,1]
Called by:
    AhciAdapterControl

It performs:
    1. Clear GHC.IE
    AHCI 1.1 Section 8.3.3
    "Software must disable interrupts (GHC.IE must be cleared to ‘0’) prior to requesting a transition of the HBA to the D3 state.  This precaution by software avoids an interrupt storm if an interrupt occurs during the transition to the D3 state."
Affected Variables/Registers:
    GHC.IE
Return Values:
    TRUE always.
--*/
{
    ULONG   i;
    AHCI_Global_HBA_CONTROL ghc;
    PAHCI_MEMORY_REGISTERS  abar = (PAHCI_MEMORY_REGISTERS)AdapterExtension->ABAR_Address;

    // Power down all ports that don't have a device present.
    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if ( (AdapterExtension->PortExtension[i] != NULL) &&
             (AdapterExtension->PortExtension[i]->StateFlags.PowerDown == FALSE) &&
             (AdapterExtension->PortExtension[i]->DeviceExtension[0].DeviceParameters.AtaDeviceType == DeviceNotExist)) {
            AhciPortPowerDown(AdapterExtension->PortExtension[i]);
        }
    }

    ghc.AsUlong = StorPortReadRegisterUlong(AdapterExtension, &abar->GHC.AsUlong);
    ghc.IE = 0;
    StorPortWriteRegisterUlong(AdapterExtension, &abar->GHC.AsUlong, ghc.AsUlong);

    AdapterExtension->StateFlags.PowerDown = 1;

    return TRUE;
}

VOID
AhciPortStop(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
/*++

    The miniport driver should stop using the resources allocated for this port.
It assumes:
    AhciAdapterStop is called after all the ports are stopped.

    StartIo spin lock must be held before this function is invoked.
    
    Note: 
    Currently this function has only one caller - AdapterStop, which has the 
    following callers. Every respective code path has StartIo spin lock acquired.

    1. AhciAdapterStop. 

       It has one caller - AhciHwStartIo. Storport has already acquired StartIo spin lock
       before calling AhciHwStartIo.

    2. AhciHwAdapterControl.
             
       AhciHwAdapterControl has already acquired StartIo spin lock before calling AdapterStop.

Called by:

It performs:
    (overview)
    1. Stop the channel
    2. Undefine all references to anything within the Uncached Extension
Affected Variables/Registers:
    CMD.ST, CMD.CR, CMD.FRE, CMD.FR

Return Values:
    TRUE if the function executed completely.
    FALSE if the channel could not be stopped.
--*/

    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000025);//AhciPortStop
    }
  //1. Stop the channel
    if ( !P_NotRunning(ChannelExtension, ChannelExtension->Px) ) {
        // don't need RESET, the port will be tried to start when processing start device request
        RecordExecutionHistory(ChannelExtension, 0xff08);   //AhciPortStop Failed
    }

  //2. Disable Interrupt and disconnect with Port resources

    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IE.AsUlong, 0);    //disabling interrupts
    PortClearPendingInterrupt(ChannelExtension);

    ChannelExtension->Px = 0;

    ChannelExtension->StateFlags.Initialized = FALSE;
    ChannelExtension->StateFlags.NoMoreIO = FALSE;

    RecordExecutionHistory(ChannelExtension, 0x10000025);//Exit AhciPortStop

    return;
}

VOID
AhciPortPowerUp(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
/*++
    Indicates that the channel is being powered up.

Called by:
    AhciHwStartIo

It performs:
    (overview)
    1. Start the Port.
    2. If APM is supported, make sure the Link is Active as defined in AHCI1.0 8.3.1.2.
    3. If Port Multiplier is supported, powered it up.
    (details)
    1.1 Reload the CLB,CBU,FLB,FBU stored in the channel extension
    1.3 Reinitialize the StateFlags
    1.4 Start the channel
Affected Variables/Registers:
    PxCMD.ST, PxCMD.ICC
    PxCLB,PxCLBU,PxFB,PxFBU
    PxIE
Return Values:
    none
--*/

    AHCI_LPM_POWER_SETTINGS userLpmSettings;
    BOOLEAN                 portPowerDown;

    RecordExecutionHistory(ChannelExtension, 0x00000026);//Enter AhciPortPowerUp

  //1.0 Reinitialize the StateFlags. e.g. ChannelExtension->StateFlags.PowerDown = FALSE;
    portPowerDown = InterlockedBitTestAndReset((LONG*)&ChannelExtension->StateFlags, 12);    //StateFlags.PownDown field is at bit 12

    if (portPowerDown == FALSE) {
        RecordExecutionHistory(ChannelExtension, 0x00010026);//AhciPortPowerUp: port already powered up.
        return;
    }

  //1.1 Reload the CLB,CBU,FLB,FBU
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CLB.AsUlong, ChannelExtension->CommandListPhysicalAddress.LowPart);
    if (ChannelExtension->AdapterExtension->CAP.S64A) { //If the controller supports 64 bits, write high part
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CLBU, ChannelExtension->CommandListPhysicalAddress.HighPart);
    }
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->FB.AsUlong, ChannelExtension->ReceivedFisPhysicalAddress.LowPart);
    if (ChannelExtension->AdapterExtension->CAP.S64A) { //If the controller supports 64 bits, write high part
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->FBU, ChannelExtension->ReceivedFisPhysicalAddress.HighPart);
    }


    //
    // If D3 Cold is enabled and we are being powered up from D3, we need to be
    // a bit heavy-handed with powering up the port due to loss of context.
    //
    if (ChannelExtension->DevicePowerState == StorPowerDeviceD3 && IsPortD3ColdEnabled(ChannelExtension)) {
        
        // 1.4.1 Re-issue init commands (this will also restore preserved settings).
        AhciPortIssueInitCommands(ChannelExtension);
        
        // 1.4.2 Restore LPM settings
        userLpmSettings.AsUlong = ChannelExtension->LastUserLpmPowerSetting;
        AhciLpmSettingsModes(ChannelExtension, userLpmSettings);    //ignore the returned value, IO will be restarted anyway.


        // 1.5 Start the channel by issuing a reset to restore PHY communication.
        AhciPortReset(ChannelExtension, FALSE);
    
    } else {

        // 1.4.1 Restore Preserved Settings
        if (NeedToSetTransferMode(ChannelExtension)) {
            RestorePreservedSettings(ChannelExtension, FALSE);
        }

        // 1.5 Start the channel
        P_Running_StartAttempt(ChannelExtension, FALSE);
    }

    RecordExecutionHistory(ChannelExtension, 0x10000026);//Exit AhciPortPowerUp
    return;
}

VOID
AhciPortPowerDown(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
/*++
Indicates that the channel is being powered down.

It assumes:
    the device has been powered down through ATA commands
    All outstanding IO will be complete before the first power request is sent to the miniport
Called by:
    AhciHwStartIo

It performs:
    Then each port must be stopped. PxCMD.ST
    If APM is supported, the Link need to be put into Slumber as defined in AHCI 1.1 Section 8.3.1.2
    If Port Multiplier is support, it would need to be powered down next.
Affected Variables/Registers:
    none
Return Values:
    TRUE if the function executed completely.
    FALSE if the channel could not be stopped for Power Down.
    Neither return value is used by ATAport.
--*/
    ChannelExtension->StateFlags.PowerDown = TRUE;

    //
    // Cancel the StartPortTimer since we're going into a lower power state.
    //
    StorPortRequestTimer(ChannelExtension->AdapterExtension,
                         ChannelExtension->StartPortTimer,
                         P_Running_Callback,
                         ChannelExtension,
                         0, 0);

    if (ChannelExtension->StateFlags.PoFxEnabled == 1) {
        if (IsPortD3ColdEnabled(ChannelExtension)) {
            // the link will be inactive, ignore the hot plug noise.
            ChannelExtension->StateFlags.IgnoreHotplugInterrupt = TRUE;
        }
    }

    RecordExecutionHistory(ChannelExtension, 0x10000027);//Exit AhciPortPowerDown
}


VOID
ReportLunsComplete(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    //Port start completed. Prepare device list.
    ULONG       lunCount;
    ULONG       lunLength;
    PLUN_LIST   lunList;
    ULONG       i;

    PAHCI_SRB_EXTENSION srbExtension;
    ULONG               srbDataBufferLength = SrbGetDataTransferLength(Srb);

    srbExtension = GetSrbExtension(Srb);

    // clean up callback fields so that the SRB can be completed.
    srbExtension->AtaFunction = 0;
    srbExtension->CompletionRoutine = NULL;

    // report error back so that Storport may retry the command.
    // tolerate failure from IDE_COMMAND_READ_LOG_EXT during device enumeration as it's not part of device enumeration commands.
    if ( (srbExtension->TaskFile.Current.bCommandReg != IDE_COMMAND_READ_LOG_EXT) &&
         (Srb->SrbStatus != SRB_STATUS_PENDING) &&
         (Srb->SrbStatus != SRB_STATUS_SUCCESS) &&
         (Srb->SrbStatus != SRB_STATUS_NO_DEVICE) ) {
        return;
    }


    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    SrbSetScsiStatus(Srb, SCSISTAT_GOOD);

    lunList = (PLUN_LIST)SrbGetDataBuffer(Srb);

    if ( ChannelExtension->DeviceExtension->DeviceParameters.AtaDeviceType == DeviceNotExist ) {
        lunCount = 0;
    } else {
        //lunCount = ChannelExtension->DeviceExtension->DeviceParameters.MaximumLun + 1;
        lunCount = 1;
    }

    lunLength = lunCount * 8;

    if ( srbDataBufferLength < (sizeof(LUN_LIST) + lunLength) ) {
        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        if (srbDataBufferLength >= sizeof(ULONG)) {
            //fill in required buffer size
            lunList->LunListLength[0] = (UCHAR)(lunLength >> (8*3));
            lunList->LunListLength[1] = (UCHAR)(lunLength >> (8*2));
            lunList->LunListLength[2] = (UCHAR)(lunLength >> (8*1));
            lunList->LunListLength[3] = (UCHAR)(lunLength >> (8*0));
        }
    } else {
        lunList->LunListLength[0] = (UCHAR)(lunLength >> (8*3));
        lunList->LunListLength[1] = (UCHAR)(lunLength >> (8*2));
        lunList->LunListLength[2] = (UCHAR)(lunLength >> (8*1));
        lunList->LunListLength[3] = (UCHAR)(lunLength >> (8*0));

        for (i = 0; i < lunCount; i++) {
            lunList->Lun[i][0] = 0;
            lunList->Lun[i][1] = (UCHAR)i;
            lunList->Lun[i][2] = 0;
            lunList->Lun[i][3] = 0;
            lunList->Lun[i][4] = 0;
            lunList->Lun[i][5] = 0;
            lunList->Lun[i][6] = 0;
            lunList->Lun[i][7] = 0;
        }
    }

    return;
}

VOID
InitQueryLogPages (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
/*++
    Initialize Log Pages to Read.

    Note that this functions should only be called after Identify Device Data is retrieved.

Parameters:
    ChannelExtension    - port that log-pages-to-query should be inited.

Return Values:
    None

--*/
{
    PUSHORT index = &ChannelExtension->DeviceExtension->QueryLogPages.TotalPageCount;
    //
    // Log Page only applies to ATA device; General Purpose Logging feature should be supported; 
    // 48bit command should be supported as READ LOG EXT is a 48bit command.
    //
    if (!IsDeviceGeneralPurposeLoggingSupported(ChannelExtension)) {
        return;
    }

    AhciZeroMemory((PCHAR)&ChannelExtension->DeviceExtension->QueryLogPages, sizeof(ATA_GPL_PAGES_TO_QUERY));

    // read Log Directory
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_DIRECTORY_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = 0;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read Device Statistics log - supported page
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = IDE_GP_LOG_SUPPORTED_PAGES;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read Device Statistics log - general page
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read Identify Device Data log - supported page
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = IDE_GP_LOG_SUPPORTED_PAGES;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read Identify Device Data log - Supported Capabilities page
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read Identify Device Data log - SATA page
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read Saved Device Internal log
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_SAVED_DEVICE_INTERNAL_STATUS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = 0;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read NCQ non-Data log
    if ((ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NCQ == 1) &&
        (ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NcqQueueMgmt == 1)) {
        ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    } else {
        ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = FALSE;
    }
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_NCQ_NON_DATA_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = 0;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    // read NCQ Send Receive log
    if ((ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NCQ == 1) &&
        (ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NcqReceiveSend == 1)) {
        ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = TRUE;
    } else {
        ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].Query = FALSE;
    }
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].LogAddress = IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].PageNumber = 0;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].BlockCount = 1;
    ChannelExtension->DeviceExtension->QueryLogPages.LogPage[*index].FeatureField = 0;
    *index = *index + 1;

    NT_ASSERT(*index <= ATA_GPL_PAGES_QUERY_COUNT);

    return;
}

VOID
IssueReadLogExtCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ UCHAR  LogAddress,
    _In_ USHORT PageNumber,
    _In_ USHORT BlockCount,
    _In_ USHORT FeatureField,
    _In_ PSTOR_PHYSICAL_ADDRESS PhysicalAddress,
    _In_ PVOID DataBuffer,
    _In_opt_ PSRB_COMPLETION_ROUTINE CompletionRoutine
    )
/*++
    Issue READ LOG EXT command to device

Parameters:
    ChannelExtension    - port that the command should be sent to
    Srb                 - Srb that carries READ LOG EXT command in SrbExtension
    LogAddress          - Log address
    PageNumber          - Page# of the log
    BlockCount          - How many blocks (in 512 bytes)
    FeatureField        - log specific value
    PhysicalAddress     - Buffer physical address
    DataBuffer          - Buffer
    CompletionRoutine   - Routine that needs to be executed after READ LOG EXT command completed

Return Values:
    None

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    //1 Fills in the local SRB
    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;

    srbExtension->Flags |= ATA_FLAGS_DATA_IN ;
    srbExtension->Flags |= ATA_FLAGS_48BIT_COMMAND ;

    srbExtension->CompletionRoutine = CompletionRoutine;

    //setup TaskFile
    srbExtension->TaskFile.Current.bFeaturesReg = (UCHAR)(FeatureField & 0xFF);     //FeatureField, low part
    srbExtension->TaskFile.Current.bSectorCountReg = (UCHAR)(BlockCount & 0xFF);    //Number of blocks to read, low part
    srbExtension->TaskFile.Current.bSectorNumberReg = LogAddress;                   //Log address
    srbExtension->TaskFile.Current.bCylLowReg = (UCHAR)(PageNumber & 0xFF);         //Page#, low part
    srbExtension->TaskFile.Current.bCylHighReg = 0;
    srbExtension->TaskFile.Current.bDriveHeadReg = 0xA0 | IDE_LBA_MODE;
    srbExtension->TaskFile.Current.bCommandReg = IDE_COMMAND_READ_LOG_EXT;
    srbExtension->TaskFile.Current.bReserved = 0;

    srbExtension->TaskFile.Previous.bFeaturesReg = (UCHAR)((FeatureField >> 8) & 0xFF);     //FeatureField, high part
    srbExtension->TaskFile.Previous.bSectorCountReg = (UCHAR)((BlockCount >> 8) & 0xFF);    //Number of blocks to read, high part
    srbExtension->TaskFile.Previous.bSectorNumberReg = 0;
    srbExtension->TaskFile.Previous.bCylLowReg = (UCHAR)((PageNumber >> 8) & 0xFF);         //Page#, high part
    srbExtension->TaskFile.Previous.bCylHighReg = 0;
    srbExtension->TaskFile.Previous.bDriveHeadReg = 0;
    srbExtension->TaskFile.Previous.bCommandReg = 0;
    srbExtension->TaskFile.Previous.bReserved = 0;

    Srb->SrbStatus = SRB_STATUS_PENDING;
    srbExtension->DataBuffer = DataBuffer;

    //setup SGL
    if ( PhysicalAddress ) {
        srbExtension->LocalSgl.NumberOfElements = 1;
        srbExtension->LocalSgl.List[0].PhysicalAddress.LowPart = PhysicalAddress->LowPart;
        srbExtension->LocalSgl.List[0].PhysicalAddress.HighPart = PhysicalAddress->HighPart;
        srbExtension->LocalSgl.List[0].Length = ATA_BLOCK_SIZE * BlockCount;
        srbExtension->Sgl = &srbExtension->LocalSgl;
        srbExtension->DataTransferLength = ATA_BLOCK_SIZE * BlockCount;
    }


    return;
}

__inline
USHORT
GetNextQueryLogPageIndex (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
)
/*++
    Get an Index value that log page should be queried.

Parameters:
    ChannelExtension

Return Values:
    USHORT

--*/
{
    USHORT i;
    USHORT index = ChannelExtension->DeviceExtension->QueryLogPages.CurrentPageIndex;

    if (index >= ChannelExtension->DeviceExtension->QueryLogPages.TotalPageCount) {
        return ATA_GPL_PAGES_INVALID_INDEX;
    }

    for (i = index; i < ChannelExtension->DeviceExtension->QueryLogPages.TotalPageCount; i++) {
        if (ChannelExtension->DeviceExtension->QueryLogPages.LogPage[i].Query) {
            ChannelExtension->DeviceExtension->QueryLogPages.CurrentPageIndex = i;
            return i;
        }
    }

    return ATA_GPL_PAGES_INVALID_INDEX;
}

__inline
VOID
ReadQueryLogPage (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ USHORT                  Index
)
{
    IssueReadLogExtCommand( ChannelExtension,
                            Srb,
                            ChannelExtension->DeviceExtension->QueryLogPages.LogPage[Index].LogAddress,
                            ChannelExtension->DeviceExtension->QueryLogPages.LogPage[Index].PageNumber,
                            ChannelExtension->DeviceExtension->QueryLogPages.LogPage[Index].BlockCount,
                            ChannelExtension->DeviceExtension->QueryLogPages.LogPage[Index].FeatureField,
                            &ChannelExtension->DeviceExtension->ReadLogExtPageDataPhysicalAddress,
                            (PVOID)ChannelExtension->DeviceExtension->ReadLogExtPageData,
                            LogPageDiscoveryCompletion
                            );
}


__inline
VOID
UpdateQueryLogPageSupportive (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR                   LogAddress,
    _In_ USHORT                  PageNumber,
    _In_ BOOLEAN                 Supported
)
/*++
    Mark log page supportive information.

Parameters:
    ChannelExtension    - port that log-page-to-query should be updated.
    LogAddress          - Log address
    PageNumber          - Page# of the log
    Supported           - log page is supported or not by device

Return Values:
    None

--*/
{
    USHORT i;

    for (i = 0; i < ChannelExtension->DeviceExtension->QueryLogPages.TotalPageCount; i++) {
        if ((ChannelExtension->DeviceExtension->QueryLogPages.LogPage[i].LogAddress == LogAddress) &&
            (ChannelExtension->DeviceExtension->QueryLogPages.LogPage[i].PageNumber == PageNumber)) {
            
            ChannelExtension->DeviceExtension->QueryLogPages.LogPage[i].Query = Supported;
            return;
        }
    }

    return;
}


VOID
LogPageDiscoveryCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*
    This process is to discover all needed information from general log pages.
    The process is initiated by reading log directory when Identify Device command completes.


*/
{
    USHORT nextPageIndex = ATA_GPL_PAGES_INVALID_INDEX;

    UCHAR  completedLogAddress = 0;
    USHORT completedPageNumber = 0;

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    // get the Log Address and Log Page just received
    completedLogAddress = srbExtension->TaskFile.Current.bSectorNumberReg;
    completedPageNumber = ((USHORT)srbExtension->TaskFile.Previous.bCylLowReg << 8) | srbExtension->TaskFile.Current.bCylLowReg;

    if ( (completedLogAddress == IDE_GP_LOG_DIRECTORY_ADDRESS) && (completedPageNumber == 0) ) {
        // the issued command was for getting Log Directory

        // ACS8-3, 4.12.1 Devices that report support for the NCQ feature set shall also report support for the GPL feature set (see 4.9),
        // the General Purpose Log Directory log and the NCQ Command Error log.
        NT_ASSERT( (Srb->SrbStatus == SRB_STATUS_SUCCESS) ||
                   (ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaCapabilities.NCQ == 0) );


        if ( (Srb->SrbStatus == SRB_STATUS_SUCCESS) &&
             (ChannelExtension->DeviceExtension->ReadLogExtPageData[0] == IDE_GP_LOG_VERSION) ) {
            // per ACS spec: The value of the General Purpose Logging Version word shall be 0001h
            // preserve the log address supportive information
            ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.LogAddressSupported = (ChannelExtension->DeviceExtension->ReadLogExtPageData[IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS] > 0) ? 1 : 0;
            if (ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.LogAddressSupported == 0) {
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS, IDE_GP_LOG_SUPPORTED_PAGES, FALSE);
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS, IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE, FALSE);
            }

            ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.LogAddressSupported = (ChannelExtension->DeviceExtension->ReadLogExtPageData[IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS] > 0) ? 1 : 0;
            if (ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.LogAddressSupported == 0) {
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_SUPPORTED_PAGES, FALSE);
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE, FALSE);
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE, FALSE);
            }

            ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.NcqCommandError = (ChannelExtension->DeviceExtension->ReadLogExtPageData[IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS] > 0) ? 1 : 0;

            ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.NcqNonData = (ChannelExtension->DeviceExtension->ReadLogExtPageData[IDE_GP_LOG_NCQ_NON_DATA_ADDRESS] > 0) ? 1 : 0;
            if (ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.NcqNonData == 0) {
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_NCQ_NON_DATA_ADDRESS, 0, FALSE);
            }

            ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.NcqSendReceive = (ChannelExtension->DeviceExtension->ReadLogExtPageData[IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS] > 0) ? 1 : 0;
            if (ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.NcqSendReceive == 0) {
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS, 0, FALSE);
            }

            ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.HybridInfo = (ChannelExtension->DeviceExtension->ReadLogExtPageData[IDE_GP_LOG_HYBRID_INFO_ADDRESS] > 0) ? 1 : 0;


        } else {
            // Log Directory can be optional. Preset supportive info, they will be updated if the actual command fails later.
            // In case of the disk doesn't support NCQ and doesn't support Log Directory, still try to discover some log pages.

            // don't query all other log pages.
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS, IDE_GP_LOG_SUPPORTED_PAGES, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS, IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_SUPPORTED_PAGES, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_NCQ_NON_DATA_ADDRESS, 0, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS, 0, FALSE);
        }

    } else if ( (completedLogAddress == IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS) && (completedPageNumber == IDE_GP_LOG_SUPPORTED_PAGES) ) {
        // the issued command was for getting supported log pages of device statistics log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            PDEVICE_STATISTICS_LOG_PAGE_HEADER pageHeader = (PDEVICE_STATISTICS_LOG_PAGE_HEADER)ChannelExtension->DeviceExtension->ReadLogExtPageData;
            PUCHAR pageSupported = (PUCHAR)ChannelExtension->DeviceExtension->ReadLogExtPageData;

            // first byte after header is how many entries in following list.
            UCHAR pageCount = *(pageSupported + sizeof(DEVICE_STATISTICS_LOG_PAGE_HEADER));

            // The value of revision number word shall be 0001h. The first supported page shall be 00h.
            if ( (pageHeader->RevisionNumber == IDE_GP_LOG_VERSION) &&
                 (pageHeader->PageNumber == IDE_GP_LOG_SUPPORTED_PAGES) &&
                 (pageCount > 1) ) {

                int i;
                for (i = 1; i <= pageCount; i++) {
                    // if the page number is shown in supported list, mark it's supported.
                    if (*(pageSupported + sizeof(DEVICE_STATISTICS_LOG_PAGE_HEADER) + i) == IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE) {
                        ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.GeneralStatistics = 1;
                    }

                    if (*(pageSupported + sizeof(DEVICE_STATISTICS_LOG_PAGE_HEADER) + i) == IDE_GP_LOG_DEVICE_STATISTICS_TEMPERATURE_PAGE) {
                        ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.TemperatureStatistics = 1;
                        break;
                    }
                }
            }

            if (ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.GeneralStatistics == 0) {
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS, IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE, FALSE);
            }
        } else {
            ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.LogAddressSupported = 0;
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS, IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE, FALSE);
        }

    } else if ( (completedLogAddress == IDE_GP_LOG_DEVICE_STATISTICS_ADDRESS) && (completedPageNumber == IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE) ) {
        // the issued command was for getting General Statistics page of Device Statistics Log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            PGP_LOG_GENERAL_STATISTICS generalStatistics = (PGP_LOG_GENERAL_STATISTICS)ChannelExtension->DeviceExtension->ReadLogExtPageData;

            // The value of revision number word shall be 0002h. (It's changed to 0001h in ACS4)
            if ( ((generalStatistics->Header.RevisionNumber == 0x0002) || (generalStatistics->Header.RevisionNumber == IDE_GP_LOG_VERSION)) &&
                 (generalStatistics->Header.PageNumber == IDE_GP_LOG_DEVICE_STATISTICS_GENERAL_PAGE) ) {
                if (generalStatistics->DateAndTime.Supported == 1) {
                    ChannelExtension->DeviceExtension->SupportedCommands.SetDateAndTime = 1;
                }
            }
        } else {
            ChannelExtension->DeviceExtension->SupportedGPLPages.DeviceStatistics.GeneralStatistics = 0;
        }

    } else if ( (completedLogAddress == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS) && (completedPageNumber == IDE_GP_LOG_SUPPORTED_PAGES) ) {
        // the issued command was for getting supported log pages of identify device data log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            PIDENTIFY_DEVICE_DATA_LOG_PAGE_HEADER pageHeader = (PIDENTIFY_DEVICE_DATA_LOG_PAGE_HEADER)ChannelExtension->DeviceExtension->ReadLogExtPageData;
            PUCHAR pageSupported = (PUCHAR)ChannelExtension->DeviceExtension->ReadLogExtPageData;

            // first byte after header is how many entries in following list.
            UCHAR pageCount = *(pageSupported + sizeof(IDENTIFY_DEVICE_DATA_LOG_PAGE_HEADER));

            // The value of revision number word shall be 0001h. The first supported page shall be 00h.
            if ( (pageHeader->RevisionNumber == IDE_GP_LOG_VERSION) &&
                 (pageHeader->PageNumber == IDE_GP_LOG_SUPPORTED_PAGES) &&
                 (pageCount > 1) ) {

                int i;
                for (i = 1; i <= pageCount; i++) {
                    // if the page number is shown in supported list, mark it's supported.
                    if (*(pageSupported + sizeof(IDENTIFY_DEVICE_DATA_LOG_PAGE_HEADER) + i) == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE) {
                        ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.SupportedCapabilities = 1;
                    } else if (*(pageSupported + sizeof(IDENTIFY_DEVICE_DATA_LOG_PAGE_HEADER) + i) == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE) {
                        ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.SATA = 1;
                    }
                }
            }

            if (ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.SATA == 0) {
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE, FALSE);
                UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE, FALSE);
            }
        } else {
            ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.LogAddressSupported = 0;
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE, FALSE);
            UpdateQueryLogPageSupportive(ChannelExtension, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS, IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE, FALSE);
        }

    } else if ( (completedLogAddress == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS) && (completedPageNumber == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE) ) {
        // the issued command was for getting supported capabilities log page of identify device data log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            PIDENTIFY_DEVICE_DATA_LOG_PAGE_SUPPORTED_CAPABILITIES supportedCapabilities = (PIDENTIFY_DEVICE_DATA_LOG_PAGE_SUPPORTED_CAPABILITIES)ChannelExtension->DeviceExtension->ReadLogExtPageData;

            // The value of revision number word shall be 0001h.
            if ((supportedCapabilities->Header.RevisionNumber == IDE_GP_LOG_VERSION) &&
                (supportedCapabilities->Header.PageNumber == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SUPPORTED_CAPABILITIES_PAGE) &&
                (supportedCapabilities->DownloadMicrocodeCapabilities.Valid == 1)) {

                if ((ChannelExtension->DeviceExtension->IdentifyDeviceData->AdditionalSupported.DownloadMicrocodeDmaSupported == 1) ||
                    (ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetSupport.DownloadMicrocode == 1)) {

                    ChannelExtension->DeviceExtension->FirmwareUpdate.DmOffsetsDeferredSupported = (supportedCapabilities->DownloadMicrocodeCapabilities.DmOffsetsDeferredSupported == 1);

                    if (ChannelExtension->DeviceExtension->FirmwareUpdate.DmOffsetsDeferredSupported) {

                        if ((supportedCapabilities->DownloadMicrocodeCapabilities.DmMinTransferSize > 0) &&
                            (supportedCapabilities->DownloadMicrocodeCapabilities.DmMinTransferSize < 0xFFFF)) {
                            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMinTransferBlocks = (USHORT)min(supportedCapabilities->DownloadMicrocodeCapabilities.DmMinTransferSize, AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE);
                        } else {
                            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMinTransferBlocks = 1;
                        }

                        if ((supportedCapabilities->DownloadMicrocodeCapabilities.DmMaxTransferSize > 0) &&
                            (supportedCapabilities->DownloadMicrocodeCapabilities.DmMaxTransferSize < 0xFFFF)) {
                            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMaxTransferBlocks = (USHORT)min(supportedCapabilities->DownloadMicrocodeCapabilities.DmMaxTransferSize, AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE);
                        } else {
                            ChannelExtension->DeviceExtension->FirmwareUpdate.DmMaxTransferBlocks = AHCI_MAX_TRANSFER_LENGTH / ATA_BLOCK_SIZE;
                        }
                    }
                }
            }

        } else {
            ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.SupportedCapabilities = 0;
        }

    } else if ( (completedLogAddress == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_ADDRESS) && (completedPageNumber == IDE_GP_LOG_IDENTIFY_DEVICE_DATA_SATA_PAGE) ) {
        // the issued command was for getting SATA log page of identify device data log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        } else {
            ChannelExtension->DeviceExtension->SupportedGPLPages.IdentifyDeviceData.SATA = 0;
        }

    } else if ( (completedLogAddress == IDE_GP_LOG_SAVED_DEVICE_INTERNAL_STATUS) && (completedPageNumber == 0) ) {
        // the issued command was for getting saved device internal data log

        if (Srb->SrbStatus != SRB_STATUS_SUCCESS) {
            ChannelExtension->DeviceExtension->SupportedGPLPages.SinglePage.SavedDeviceInternalStatusData = 0;
        }
    } else if ( (completedLogAddress == IDE_GP_LOG_NCQ_NON_DATA_ADDRESS) && (completedPageNumber == 0) ) {
        // the issued command was for getting ncq non-data log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            PGP_LOG_NCQ_NON_DATA ncqNonData = (PGP_LOG_NCQ_NON_DATA)ChannelExtension->DeviceExtension->ReadLogExtPageData;

            ChannelExtension->DeviceExtension->SupportedCommands.HybridDemoteBySize = ncqNonData->SubCmd2.HybridDemoteBySize;
            ChannelExtension->DeviceExtension->SupportedCommands.HybridChangeByLbaRange = ncqNonData->SubCmd3.HybridChangeByLbaRange;
            ChannelExtension->DeviceExtension->SupportedCommands.HybridControl = ncqNonData->SubCmd4.HybridControl;

        } else {
            NT_ASSERT(FALSE);
        }
    } else if ( (completedLogAddress == IDE_GP_LOG_NCQ_SEND_RECEIVE_ADDRESS) && (completedPageNumber == 0) ) {
        // the issued command was for getting ncq send receive log

        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            PGP_LOG_NCQ_SEND_RECEIVE ncqSendReceive = (PGP_LOG_NCQ_SEND_RECEIVE)ChannelExtension->DeviceExtension->ReadLogExtPageData;

            ChannelExtension->DeviceExtension->SupportedCommands.HybridEvict = ncqSendReceive->SubCmd.HybridEvict;

        } else {
            NT_ASSERT(FALSE);
        }
    } else {
        // all log addresses and log pages in log page discovery process should be covered in above conditions.
        NT_ASSERT(FALSE);
    }

    //
    // Move index to the next one and check if there is any log pages pending to read.
    //
    ChannelExtension->DeviceExtension->QueryLogPages.CurrentPageIndex++;
    nextPageIndex = GetNextQueryLogPageIndex(ChannelExtension);

    if (nextPageIndex != ATA_GPL_PAGES_INVALID_INDEX) {
        ReadQueryLogPage(ChannelExtension, Srb, nextPageIndex);
    } else {
        ReportLunsComplete(ChannelExtension, Srb);
    }

    return;
}


VOID
AhciPortIdentifyDevice(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
  )
{
    PAHCI_SRB_EXTENSION srbExtension;
    PCDB                cdb = SrbGetCdb(Srb);

    srbExtension = GetSrbExtension(Srb);

    if (Srb->SrbStatus == SRB_STATUS_BUS_RESET) {
        return;
    }

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        if (srbExtension->TaskFile.Current.bCommandReg == IDE_COMMAND_ATAPI_IDENTIFY) {
            ChannelExtension->DeviceExtension->DeviceParameters.AtaDeviceType = DeviceIsAtapi;
        } else {
            ChannelExtension->DeviceExtension->DeviceParameters.AtaDeviceType = DeviceIsAta;
        }
    } else if (Srb->SrbStatus == SRB_STATUS_NO_DEVICE) {
        // command failed consider as no device
        ChannelExtension->DeviceExtension->DeviceParameters.AtaDeviceType = DeviceNotExist;
    }

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        //Re-initialize device specific information to avoid the values being reused after device switched.
        ChannelExtension->StateFlags.NCQ_Activated = 0;
        ChannelExtension->StateFlags.NCQ_Succeeded = 0;
        ChannelExtension->StateFlags.HybridInfoEnabledOnHiberFile = 0;

        ChannelExtension->DeviceExtension->HybridCachingMediumEnableRefs = 0;

        AhciZeroMemory((PCHAR)&ChannelExtension->DeviceExtension->SupportedGPLPages, sizeof(ATA_SUPPORTED_GPL_PAGES));
        AhciZeroMemory((PCHAR)&ChannelExtension->DeviceExtension->SupportedCommands, sizeof(ATA_COMMAND_SUPPORTED));
        AhciZeroMemory((PCHAR)&ChannelExtension->DeviceExtension->FirmwareUpdate, sizeof(DOWNLOAD_MICROCODE_CAPABILITIES));

        // identify completes, digest identify data / inquiry data
        UpdateDeviceParameters(ChannelExtension);


        //
        // Cache if PUIS is enabled or not.  We'll look at this later when
        // powering up the port to determine if we need to send the spin up
        // command first.
        //
        ChannelExtension->StateFlags.PuisEnabled = ChannelExtension->DeviceExtension->IdentifyDeviceData->CommandSetActive.PowerUpInStandby;

        // Initialize port properties
        ChannelExtension->PortProperties = 0;

        if (IsExternalPort(ChannelExtension)) {
            SETMASK(ChannelExtension->PortProperties, PORT_PROPERTIES_EXTERNAL_PORT);
        }
    }

    // Identify Device can only be triggered from REPORT LUNS command or
    //    INQUIRY command (for disk in dump environment)
    if ((cdb != NULL) && (cdb->CDB10.OperationCode == SCSIOP_REPORT_LUNS)) {

        if ( (Srb->SrbStatus == SRB_STATUS_SUCCESS) &&
             IsDeviceGeneralPurposeLoggingSupported(ChannelExtension) ) {

            // READ LOG EXT command is supported.
            USHORT index;

            InitQueryLogPages(ChannelExtension);

            index = GetNextQueryLogPageIndex(ChannelExtension);

            NT_ASSERT(index == 0);

            if (index != ATA_GPL_PAGES_INVALID_INDEX) {
                //First page should be log directory. Read it to get pages supported by device.
                ReadQueryLogPage(ChannelExtension, Srb, index);
            } else {
                ReportLunsComplete(ChannelExtension, Srb);
            }

        } else {
            ReportLunsComplete(ChannelExtension, Srb);
        }
    } else if (IsDumpMode(ChannelExtension->AdapterExtension) && 
               (cdb != NULL) && (cdb->CDB10.OperationCode == SCSIOP_INQUIRY)) {

        if (IsDumpResumeMode(ChannelExtension->AdapterExtension) &&
            (Srb->SrbStatus == SRB_STATUS_SUCCESS) &&
            IsDeviceGeneralPurposeLoggingSupported(ChannelExtension) &&
            IsDeviceHybridInfoSupported(ChannelExtension)) {
            //
            // Read Hybrid Information log during resume, so that disk can stop self-pinning.
            // In normal stack, suerfetch sends down HYBRID_FUNCTION_GET_INFO triggers the log to be read.
            //
            IssueReadLogExtCommand( ChannelExtension,
                                    Srb,
                                    IDE_GP_LOG_HYBRID_INFO_ADDRESS,
                                    0,
                                    1,
                                    0,      // feature field
                                    &ChannelExtension->DeviceExtension->ReadLogExtPageDataPhysicalAddress,
                                    (PVOID)ChannelExtension->DeviceExtension->ReadLogExtPageData,
                                    (PSRB_COMPLETION_ROUTINE)InquiryComplete
                                    );
        } else {
            InquiryComplete(ChannelExtension, Srb);
        }
    }
    return;
}

VOID
AhciPortNVCacheCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
  )
{
    PSRB_IO_CONTROL         srbControl;
    PNVCACHE_REQUEST_BLOCK  nRB;
    PATA_TASK_FILE          TaskFile;
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    srbControl = (PSRB_IO_CONTROL)SrbGetDataBuffer(Srb);
    nRB = ((PNVCACHE_REQUEST_BLOCK) ( (PSRB_IO_CONTROL) srbControl + 1) );  //26015: "Potential overflow using expression 'nRB->NRBStatus' Buffer access is apparently unbounded by the buffer size.

    // Return status success indicating that the request was handled by the device.
    nRB->NRBStatus = NRB_SUCCESS;

    TaskFile = (PATA_TASK_FILE)srbExtension->ResultBuffer;

    if ( TaskFile != NULL ) {

        nRB->NVCacheStatus = TaskFile->Current.bCommandReg;
        if (TaskFile->Current.bCommandReg & 1) {  // command failed
            nRB->NVCacheSubStatus = TaskFile->Current.bFeaturesReg;
        }

        nRB->Count = (TaskFile->Current.bSectorCountReg << 8) +
                     (TaskFile->Previous.bSectorCountReg);

        nRB->LBA = (ULONGLONG) TaskFile->Previous.bCylHighReg;
        nRB->LBA <<= 8;
        nRB->LBA += (ULONGLONG) TaskFile->Previous.bCylLowReg;
        nRB->LBA <<= 8;
        nRB->LBA += (ULONGLONG) TaskFile->Previous.bSectorNumberReg;
        nRB->LBA <<= 8;
        nRB->LBA += (ULONGLONG) TaskFile->Current.bCylHighReg;
        nRB->LBA <<= 8;
        nRB->LBA += (ULONGLONG) TaskFile->Current.bCylLowReg;
        nRB->LBA <<= 8;
        nRB->LBA += (ULONGLONG) TaskFile->Current.bSectorNumberReg;

        //
        // Free the buffer allocated as mode sense info buffer , holding task file
        //
        AhciFreeDmaBuffer(ChannelExtension->AdapterExtension, srbExtension->ResultBufferLength, TaskFile);

    } else {
        // in case TaskFile is not returned in SenseInfoBuffer, use cached ATA Status and Error register values.
        if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            // command succeeded
            nRB->NVCacheStatus = 0;
            nRB->NVCacheSubStatus = 0;
        } else {
            // command failed
            nRB->NVCacheStatus = srbExtension->AtaStatus;
            nRB->NVCacheSubStatus = srbExtension->AtaError;
        }
    }

    return;
}

VOID
AhciPortSmartCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
  )
{
    PSENDCMDOUTPARAMS           outParams;
    PAHCI_SRB_EXTENSION         srbExtension;
    PUCHAR                      buffer;//to make the pointer arithmatic easier

    UNREFERENCED_PARAMETER(ChannelExtension);

    buffer       = (PUCHAR)SrbGetDataBuffer(Srb) + sizeof(SRB_IO_CONTROL);
    outParams    = (PSENDCMDOUTPARAMS) buffer;                          //26015: "Potential overflow using expression 'outParams->DriverStatus.bDriverError'  Buffer access is apparently unbounded by the buffer size.
    srbExtension = GetSrbExtension(Srb);

    Srb->SrbStatus &= ~SRB_STATUS_AUTOSENSE_VALID;  // remove this flag as there is no data copy back to original Sense Buffer

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        outParams->DriverStatus.bDriverError = 0;
        outParams->DriverStatus.bIDEError = 0;

    } else  {
        // command failed
        outParams->DriverStatus.bDriverError = SMART_IDE_ERROR;
        outParams->DriverStatus.bIDEError = srbExtension->AtaStatus;
    }

    return;
}

__inline
VOID
BuildLocalCommand(
    _In_ PAHCI_CHANNEL_EXTENSION        ChannelExtension,
    _In_ PATA_TASK_FILE                 TaskFile,
    _In_opt_ PSRB_COMPLETION_ROUTINE    CompletionRountine
    )
/*++

It assumes:
    nothing
Called by:
    IssuePreservedSettingCommands

It performs:
    1 Fills in the local SRB with the ATA command

Affected Variables/Registers:
    none

--*/
{
    PSCSI_REQUEST_BLOCK srb;
    PAHCI_SRB_EXTENSION srbExtension;

    //
    // Local Srb still uses SCSI_REQUEST_BLOCK type.
    // do not touch field "srb->NextSrb". It should be only touched in queue related operations.
    //
    srb = &ChannelExtension->Local.Srb;
    srb->SrbStatus = SRB_STATUS_PENDING;
    srb->SrbExtension = (PVOID)ChannelExtension->Local.SrbExtension;
    srb->TimeOutValue = 1;      //as it's sent by miniport, no one monitors the timeout value.

  // Fills in the local SRB with the SetFeatures command
    srbExtension = ChannelExtension->Local.SrbExtension;
    AhciZeroMemory((PCHAR)srbExtension, sizeof(AHCI_SRB_EXTENSION));

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
    srbExtension->CompletionRoutine = CompletionRountine;

    //setup TaskFile
    StorPortCopyMemory(&srbExtension->TaskFile, TaskFile, sizeof(ATA_TASK_FILE));

    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x1000001d);//Exit BuildLocalCommand
    }

    return;
}

VOID
IssuePreservedSettingCommands(
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb
  )
/*++
    Uses the local SRB to send down the next Preserved Setting
It assumes:
    Local SRB is only used for restoring preserved settings
Called by:
    RestorePreservedSettings,
    IssueInitCommands,
    Itself indirectly through local SRB callback

It performs:
    1 Verify local SRB is not in use
    2 Find the next Preserved Setting
    3 Send it

Affected Variables/Registers:
    none
--*/
{
    UCHAR           i;
    ULONG           allocated;
    ATA_TASK_FILE   taskFile = {0};

    UNREFERENCED_PARAMETER(Srb);

  //1 Verify local SRB is not in use
    allocated = GetOccupiedSlots(ChannelExtension);

    if ((allocated & (1 << 0)) > 0) {
        // Already restoring preserved Settings
        return;
    }

  //2 find the next command to send
    for (i = 0; i < MAX_SETTINGS_PRESERVED; i++) {
        if ( (ChannelExtension->PersistentSettings.SlotsToSend & (1 << i)) > 0 ) {
            ChannelExtension->PersistentSettings.SlotsToSend &= ~(1 << i);
            break;
        }
    }

    //perhaps there is none.  Done.
    if ( i >= MAX_SETTINGS_PRESERVED) {
      // release active reference for process of restore preserved settings
        if (ChannelExtension->StateFlags.RestorePreservedSettingsActiveReferenced == 1) {
            PortReleaseActiveReference(ChannelExtension, NULL);
            ChannelExtension->StateFlags.RestorePreservedSettingsActiveReferenced = 0;
        }

        InterlockedBitTestAndReset((LONG*)&ChannelExtension->StateFlags, 3);    //ReservedSlotInUse field is at bit 3

        return;
    }

  //3 Otherwise use the LocalSRB to send the command. When it is done, call this routine again
    taskFile.Current.bFeaturesReg = ChannelExtension->PersistentSettings.CommandParams[i].Features;
    taskFile.Current.bSectorCountReg = ChannelExtension->PersistentSettings.CommandParams[i].SectorCount;
    taskFile.Current.bDriveHeadReg = 0xA0;
    taskFile.Current.bCommandReg = IDE_COMMAND_SET_FEATURE;

    BuildLocalCommand(ChannelExtension, &taskFile, IssuePreservedSettingCommands);

    return;
}

VOID
IssueInitCommands(
    _In_ PAHCI_CHANNEL_EXTENSION    ChannelExtension,
    _In_opt_ PSTORAGE_REQUEST_BLOCK Srb
  )
/*++
    Uses the local SRB to send down the next Init Command or Preserved Setting Command
It assumes:
    Local SRB is only used for restoring preserved settings
Called by:
    AhciIssueInitCommands,
    Itself indirectly through local SRB callback

It performs:
    1 Verify local SRB is not in use
    2 Find the next Init Command or Preserved Setting Command
    3 Send it

Affected Variables/Registers:
    none
--*/
{
    ULONG           allocated;
    PATA_TASK_FILE  taskFile;

    UNREFERENCED_PARAMETER(Srb);

  // Verify local SRB is not in use
    allocated = GetOccupiedSlots(ChannelExtension);

    if ((allocated & (1 << 0)) > 0) {
        // Already restoring preserved Settings
        return;
    }

  // if all Init commands have been sent, send Preserved Setting Commands
    if (ChannelExtension->DeviceInitCommands.CommandToSend >= ChannelExtension->DeviceInitCommands.ValidCommandCount) {
        ChannelExtension->PersistentSettings.SlotsToSend = ChannelExtension->PersistentSettings.Slots;
        IssuePreservedSettingCommands(ChannelExtension, NULL);
        return;
    }

  // find the next command to send
    taskFile = ChannelExtension->DeviceInitCommands.CommandTaskFile + ChannelExtension->DeviceInitCommands.CommandToSend;
    taskFile->Current.bDriveHeadReg = 0xA0;

    //
    // If this is a PUIS spin-up command and a spin-up is *not* needed then
    // skip to the next command.  This command is not needed if:
    //  * PUIS is not enabled; or
    //  * The device doesn't support PUIS, is a hybrid, or is an SSD; or
    //  * The device is currently powered up (not in D3)
    //
    if (taskFile->Current.bFeaturesReg == IDE_FEATURE_PUIS_SPIN_UP &&
        (ChannelExtension->StateFlags.PuisEnabled == FALSE ||
         NeedsPuisSpinUpOnPowerUp(ChannelExtension) == FALSE ||
         ChannelExtension->DevicePowerState != StorPowerDeviceD3)) {
        ChannelExtension->DeviceInitCommands.CommandToSend++;
        taskFile = ChannelExtension->DeviceInitCommands.CommandTaskFile + ChannelExtension->DeviceInitCommands.CommandToSend;
        taskFile->Current.bDriveHeadReg = 0xA0;
    }

    BuildLocalCommand(ChannelExtension, taskFile, IssueInitCommands);
    ChannelExtension->DeviceInitCommands.CommandToSend++;

    return;
}


VOID
SetDateAndTimeCompletion(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    AhciZeroMemory((PCHAR)srbExtension, sizeof(AHCI_SRB_EXTENSION));

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
    srbExtension->CompletionRoutine = NULL;
    SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_STANDBY_IMMEDIATE);

    return;
}

VOID
BuildSetDateAndTimeTaskFile(
    _In_ PATA_TASK_FILE  TaskFile
)
{
    LARGE_INTEGER       temp;
    ULONGLONG           now;

    AhciZeroMemory((PCHAR)TaskFile, sizeof(ATA_TASK_FILE));

    //setup TaskFile
    StorPortQuerySystemTime(&temp);

    now = (ULONGLONG) temp.QuadPart;
    now /= 10000;  //4 orders of magnitude

    // 2) subtract 369 years in seconds.
    //    Number of milliseconds in a Julian year = 31,557,600,000 (1millisecond * 1000second * 60minute * 60hour * 24day * 365.25year)
    //    369 * 31,557,600,000  = 11,644,754,400,000 (0xA97 4173 1300)
    now -= 0xA9741731300;

    // Example 2010-09-29 10 am = 0x1cb5ffd`22c1bf5e
    // 0x1cb5ffd`22c1bf5e/10000 = 0xbc2`8f496393 (12930255512467 or 12930255512467.8494) milliseconds
    // 0xbc2`8f496393 - 0xa97'41731300 = 0x012b`4dd65093
    // NOTE: this number won't roll over for another ~8700 years.

    TaskFile->Current.bSectorNumberReg =     (UCHAR) (0xFF & now);
    now >>= 8;
    TaskFile->Current.bCylLowReg =           (UCHAR) (0xFF & now);
    now >>= 8;
    TaskFile->Current.bCylHighReg =          (UCHAR) (0xFF & now);
    now >>= 8;
    TaskFile->Previous.bSectorNumberReg =    (UCHAR) (0xFF & now);
    now >>= 8;
    TaskFile->Previous.bCylLowReg =          (UCHAR) (0xFF & now);
    now >>= 8;
    TaskFile->Previous.bCylHighReg =         (UCHAR) (0xFF & now);

    TaskFile->Current.bDriveHeadReg = 0xA0;
    TaskFile->Current.bCommandReg = IDE_COMMAND_SET_DATE_AND_TIME;

    return;
}

VOID
IssueSetDateAndTimeCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN SendStandBy
  )
/*++
It assumes:
    Srb is not the local SRB.
Called by:
    AhciHwStartIo with SRB_FUNCTION_SHUTDOWN

It performs:
    1 Builds a Set Date & Time taskfile and associates it with the provided Srb.
Affected Variables/Registers:
    none

--*/
{

    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension((PSTORAGE_REQUEST_BLOCK)Srb);

    NT_ASSERT(Srb != &ChannelExtension->Local.Srb);

    UNREFERENCED_PARAMETER(ChannelExtension);

    //setup TaskFile
    BuildSetDateAndTimeTaskFile(&srbExtension->TaskFile);

    srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
    srbExtension->CompletionRoutine = (SendStandBy ? SetDateAndTimeCompletion : NULL);
}

BOOLEAN
AhciDeviceInitialize (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    STOR_LOCK_HANDLE lockhandle = {0};

    RecordExecutionHistory(ChannelExtension, 0x00000007);   //AhciDeviceInitialize

  //1 update preserved commands per device needs.
    if (IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {

        if (NeedToSetTransferMode(ChannelExtension)) {
            //1.1 Set DMA mode to this device
            UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_INVALID, IDE_FEATURE_SET_TRANSFER_MODE, 0, 0x44);
        }

        //1.2 Persist Write Cache
        UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_INVALID, IDE_FEATURE_ENABLE_WRITE_CACHE, 0, 0);

    } else if (IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
      //2.1 Persist SATA transfer mode for some SATAI/PATAPI bridge chips
        UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_INVALID, IDE_FEATURE_SET_TRANSFER_MODE, 0, 0x42);

        if ( IsDeviceSupportsAN(ChannelExtension->DeviceExtension->IdentifyPacketData) &&
             !IsDeviceEnabledAN(ChannelExtension->DeviceExtension->IdentifyPacketData) ) {
        //2.2 Enable Asynchronous Notification if supported
            UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_INVALID, IDE_FEATURE_ENABLE_SATA_FEATURE, 0, IDE_SATA_FEATURE_ASYNCHRONOUS_NOTIFICATION);
        }
    }

    //
    // Enable Power Up in Standby (PUIS) on hybrids if it's supported and not
    // already enabled.
    //
    if (IsDeviceHybridInfoSupported(ChannelExtension) &&
        ChannelExtension->DeviceExtension[0].IdentifyDeviceData->CommandSetSupport.PowerUpInStandby &&
        ChannelExtension->DeviceExtension[0].IdentifyDeviceData->CommandSetActive.PowerUpInStandby == FALSE) {
        UpdateSetFeatureCommands(ChannelExtension, IDE_FEATURE_DISABLE_PUIS, IDE_FEATURE_ENABLE_PUIS, 0, 0);
        ChannelExtension->StateFlags.PuisEnabled = TRUE;
    }

    //3.1 evaluate ACPI _SDD method informing information about device connected.
    AhciPortEvaluateSDDMethod(ChannelExtension);

    //3.2 retrieve _GTF commands and add needed commands in list.
    AhciPortGetInitCommands(ChannelExtension);

  //5.1 Configure device with init commands and persistent configuration commands
    AhciPortIssueInitCommands(ChannelExtension);

    AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    ActivateQueue(ChannelExtension, TRUE);
    AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);

    RecordExecutionHistory(ChannelExtension, 0x10000007);//Exit AhciDeviceInitialize
    return TRUE;
}

VOID
AhciDeviceStart (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
/*
    Running at PASSIVE_LEVEL

    This function is called when IRP_MN_START_DEVICE is being processed.
    device registry access, ACPI calls can be processed in this function.
*/
{
    if (ChannelExtension == NULL) {
        return;
    }

    AhciDeviceInitialize(ChannelExtension);

    return;
}


__inline
BOOLEAN
IsLpmModeSetting(
    _In_ PSTOR_POWER_SETTING_INFO PowerInfo
    )
{
    if (PowerInfo->PowerSettingGuid.Data1 == 0x0b2d69d7) {
        if (PowerInfo->PowerSettingGuid.Data2 == 0xa2a1){
            if (PowerInfo->PowerSettingGuid.Data3 == 0x449c){
                if (PowerInfo->PowerSettingGuid.Data4[0] == 0x96){
                    if (PowerInfo->PowerSettingGuid.Data4[1] == 0x80){
                        if (PowerInfo->PowerSettingGuid.Data4[2] == 0xf9){
                            if (PowerInfo->PowerSettingGuid.Data4[3] == 0x1c){
                                if (PowerInfo->PowerSettingGuid.Data4[4] == 0x70){
                                    if (PowerInfo->PowerSettingGuid.Data4[5] == 0x52){
                                        if (PowerInfo->PowerSettingGuid.Data4[6] == 0x1c){
                                            if (PowerInfo->PowerSettingGuid.Data4[7] == 0x60){
                                                return TRUE;
                                            }  }  }  }  }  }  } }  }  }  }

    return FALSE;
}

__inline
BOOLEAN
IsLpmAdaptiveSetting(
    _In_ PSTOR_POWER_SETTING_INFO PowerInfo
    )
{
    if (PowerInfo->PowerSettingGuid.Data1 == 0xDAB60367) {
        if (PowerInfo->PowerSettingGuid.Data2 == 0x53FE){
            if (PowerInfo->PowerSettingGuid.Data3 == 0x4fbc){
                if (PowerInfo->PowerSettingGuid.Data4[0] == 0x82){
                    if (PowerInfo->PowerSettingGuid.Data4[1] == 0x5E){
                        if (PowerInfo->PowerSettingGuid.Data4[2] == 0x52){
                            if (PowerInfo->PowerSettingGuid.Data4[3] == 0x1D){
                                if (PowerInfo->PowerSettingGuid.Data4[4] == 0x06){
                                    if (PowerInfo->PowerSettingGuid.Data4[5] == 0x9D){
                                        if (PowerInfo->PowerSettingGuid.Data4[6] == 0x24){
                                            if (PowerInfo->PowerSettingGuid.Data4[7] == 0x56){
                                                return TRUE;
                                            }  }  }  }  }  }  } }  }  }  }

    return FALSE;
}

UCHAR
SetAllowedLpmStates(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
   // Return Value: disabled modes;
{
    UCHAR lpm;
    AHCI_SERIAL_ATA_CONTROL sctl;

    // 0h  No interface restrictions
    // 1h  Transitions to the Partial state disabled
    // 2h  Transitions to the Slumber state disabled
    // 3h  Transitions to both Partial and Slumber states disabled
    // disable LPM for eSATA port as hot-plug cannot be detected in partial or slumber state.
    if ((ChannelExtension->LastUserLpmPowerSetting == 0) ||
        !IsLPMCapablePort(ChannelExtension)) {
        lpm = 0x03; // slumber and partial disallowed
    } else {
        AHCI_COMMAND cmd;
        cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);

        if ((ChannelExtension->AutoPartialToSlumberInterval == 0) &&    // No software auto partial to slumber
             ( (ChannelExtension->AdapterExtension->CAP2.APST == 0) ||      // Host auto Partial to Slumber is not supported.
               (cmd.APSTE == 0) ) ) {                                       // Host auto Partial to Slumber is not enabled.
            lpm = 0x02; // partial allowed; slumber disallowed
        } else {
            lpm = 0x00; // partial allowed; slumber allowed
        }

    }

    if (ChannelExtension->AdapterExtension->CAP.SSC == 0) {
        // disable Slumber if controller does not support it.
        lpm |= 0x02;
    }

    if (ChannelExtension->AdapterExtension->CAP.PSC == 0) {
        // storahci LPM is to put device into partial, then transit into slumber according to defined interval value.
        // do not enable LPM if partial is not supported.
        // the case of device supporting slumber but not partial is very rare.
        lpm = 0x03;
    }


    //Set PxSCTL.IPM to 3h to restrict slumber and partial interface power management state transitions.
    sctl.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong);
    sctl.IPM = lpm;
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong, sctl.AsUlong);

    return lpm;
}

BOOLEAN
AhciLpmSettingsModes(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ AHCI_LPM_POWER_SETTINGS LpmMode
    )
/*
    NOTE: this routine may prepared command in Local.Srb. Caller of this routine should try to start IO process.

Return Value:
    TRUE: the caller should start IO process
*/
{
    AHCI_COMMAND cmd;
    BOOLEAN      needStartIo = FALSE;

    //Make sure the configuration supports LPM, otherwise, don't touch anything.
    if (NoLpmSupport(ChannelExtension) || !IsLPMCapablePort(ChannelExtension)) {
        return needStartIo;
    }


    ChannelExtension->LastUserLpmPowerSetting = (UCHAR)LpmMode.AsUlong;

    if (LpmMode.AsUlong == 0) {
        // Active Mode.
        //Turn LPM off as Active is chosen
        if (ChannelExtension->AdapterExtension->CAP.SALP == 1) {
            cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);
            if (cmd.ALPE != 0) {
                cmd.ALPE = 0;
                cmd.ASP = 0;
                StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong, cmd.AsUlong);
            }
        }

        //Set PxSCTL.IPM to 3h to restrict slumber and partial interface power management state transitions.
        SetAllowedLpmStates(ChannelExtension);

        // Disable DIPM
        if (IsDeviceSupportsDIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData)) {
            //The enable/disable state for device initiated power management shall persist across software reset.
            //The enable/disable state shall be reset to its default disabled state upon COMRESET.
            UpdateSetFeatureCommands(ChannelExtension,
                                    IDE_FEATURE_ENABLE_SATA_FEATURE,
                                    IDE_FEATURE_DISABLE_SATA_FEATURE,
                                    IDE_SATA_FEATURE_DEVICE_INITIATED_POWER_MANAGEMENT,
                                    IDE_SATA_FEATURE_DEVICE_INITIATED_POWER_MANAGEMENT);

            //Configure device with persistent configuration commands
            RestorePreservedSettings(ChannelExtension, FALSE);
            needStartIo = TRUE;
        }

    } else {
        // link power management is allowed.

        //Set PxSCTL.IPM for LPM allowed states.
        UCHAR sctlIpm = SetAllowedLpmStates(ChannelExtension);

        // Setting HIPM if it's enabled.
        if ( (LpmMode.HipmEnabled > 0) &&
             (sctlIpm != 0x03) &&
             (ChannelExtension->AdapterExtension->CAP.SALP == 1) &&
             IsDeviceSupportsHIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData) ) {
            // If Partial is capable and device supports HIPM.

            // Turn on LPM and set it for Partial
            cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);
            cmd.ALPE = 1;
            cmd.ASP = 0; //0 = partial, 1 = slumber
            StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong, cmd.AsUlong);

        } else if (ChannelExtension->AdapterExtension->CAP.SALP == 1) {
            //Turn off HIPM if it's supported
            cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);
            if (cmd.ALPE != 0) {
                cmd.ALPE = 0;
                cmd.ASP = 0;
                StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong, cmd.AsUlong);
            }
        }

        // Setting DIPM if it's enabled.
        if ((LpmMode.DipmEnabled > 0) &&
            (sctlIpm != 0x03)) {
            // Enable DIPM feature.
            if (IsDeviceSupportsDIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData)) {

                //The enable/disable state for device initiated power management shall persist across software reset.
                //The enable/disable state shall be reset to its default disabled state upon COMRESET.
                UpdateSetFeatureCommands(ChannelExtension,
                                        IDE_FEATURE_DISABLE_SATA_FEATURE,
                                        IDE_FEATURE_ENABLE_SATA_FEATURE,
                                        IDE_SATA_FEATURE_DEVICE_INITIATED_POWER_MANAGEMENT,
                                        IDE_SATA_FEATURE_DEVICE_INITIATED_POWER_MANAGEMENT);

                //Configure device with persistent configuration commands
                RestorePreservedSettings(ChannelExtension, FALSE);
                needStartIo = TRUE;
            }

        } else {
            // Disable DIPM feature.
            if (IsDeviceSupportsDIPM(ChannelExtension->DeviceExtension[0].IdentifyDeviceData)) {

                //The enable/disable state for device initiated power management shall persist across software reset.
                //The enable/disable state shall be reset to its default disabled state upon COMRESET.
                UpdateSetFeatureCommands(ChannelExtension,
                                        IDE_FEATURE_ENABLE_SATA_FEATURE,
                                        IDE_FEATURE_DISABLE_SATA_FEATURE,
                                        IDE_SATA_FEATURE_DEVICE_INITIATED_POWER_MANAGEMENT,
                                        IDE_SATA_FEATURE_DEVICE_INITIATED_POWER_MANAGEMENT);

                //Configure device with persistent configuration commands
                RestorePreservedSettings(ChannelExtension, FALSE);
                needStartIo = TRUE;
            }
        }

    }

    return needStartIo;
}


BOOLEAN
AhciPortPowerSettingNotification(
    IN PAHCI_CHANNEL_EXTENSION ChannelExtension,
    IN PSTOR_POWER_SETTING_INFO PowerInfo
    )
{
    // do nothing if there is no device connected
    if ( (ChannelExtension->StartState.ChannelNextStartState == StartFailed) ||
         (ChannelExtension->DeviceExtension->DeviceParameters.AtaDeviceType == DeviceNotExist) ) {
        return FALSE;
    }

    //Make sure the configuration supports LPM, otherwise, don't touch anything.
    if (NoLpmSupport(ChannelExtension) || !IsLPMCapablePort(ChannelExtension)) {
        return FALSE;
    }

    // Validate input LPM data from the Power Manager
    if (PowerInfo->ValueLength != sizeof(ULONG)) {
        return FALSE;
    }

    if (!IsLpmModeSetting(PowerInfo) &&
        !IsLpmAdaptiveSetting(PowerInfo)) {
        // invalid power policy.
        return FALSE;
    }

    if (IsLpmAdaptiveSetting(PowerInfo)) {
        // max allowed value: 5 minutes (in ms)
        ULONG interval = (ULONG)*((PULONG)PowerInfo->Value);

        if (interval <= 300000) {
            ChannelExtension->AutoPartialToSlumberInterval = interval;

            //Set PxSCTL.IPM register for LPM allowed states.
            SetAllowedLpmStates(ChannelExtension);
        }

    } else if (IsLpmModeSetting(PowerInfo)) {
        BOOLEAN                 needRestartIo;
        AHCI_LPM_POWER_SETTINGS userLpmPowerSettings;

        userLpmPowerSettings.AsUlong = (ULONG)*((PULONG)PowerInfo->Value);

        needRestartIo = AhciLpmSettingsModes(ChannelExtension, userLpmPowerSettings);

        if (needRestartIo) {
            STOR_LOCK_HANDLE lockhandle = {0};
            AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
            ActivateQueue(ChannelExtension, TRUE);
            AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
        }
    }

    return TRUE;
}

VOID
AhciAutoPartialToSlumber(
    _In_ PVOID AdapterExtension,
    _In_opt_ PVOID ChannelExtension
)
/*
    NOTE: input parameter - Context is required as this is a callback function. But it's not used by this function.
*/
{
    PAHCI_CHANNEL_EXTENSION channelExtension = (PAHCI_CHANNEL_EXTENSION)ChannelExtension;

    AHCI_SERIAL_ATA_STATUS  ssts;
    AHCI_COMMAND            cmd;
    ULONG                   ci;
    ULONG                   sact;

    if (channelExtension == NULL) {
        NT_ASSERT(FALSE);
        return;
    }

    if (channelExtension->Px == NULL) {

        // The port has been stopped. Do not touch its registers.    
        // There is no need to transit the link power state to Slumber state.
        //
        // Note: 
        // Px is set to NULL in AhciPortStop function. StartIo spin lock is utilized to
        // prevent race condition with AhciPortStop function. StartIo spin lock is acquired 
        // before AhciPortStop is called. When we are here in AhciAutoPartialToSlumber, because
        // it is a timer callback function, StartIo spin lock is already held - Storport holds
        // StartIo spin lock before invoking miniport timer callback function.
        //       

        return;
    }


    NT_ASSERT(AdapterExtension == (PVOID)(channelExtension->AdapterExtension));

    UNREFERENCED_PARAMETER(AdapterExtension);

    // 1.1 check the Link Power State should be enabled.
    cmd.AsUlong = StorPortReadRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->CMD.AsUlong);
    ci = StorPortReadRegisterUlong(AdapterExtension, &channelExtension->Px->CI);
    sact = StorPortReadRegisterUlong(AdapterExtension, &channelExtension->Px->SACT);

    if (!PartialToSlumberTransitionIsAllowed(channelExtension, cmd, ci, sact)) {
        // validate again in case any condition changed that not allowing StorAHCI to perform Partial to Slumber transition.
        StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Transit into Slumber from Partial - bailed out, request outstanding: CI: 0x%08X, SACT: 0x%08X \n", channelExtension->PortNumber, ci, sact);

        return;
    }

    ssts.AsUlong = StorPortReadRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->SSTS.AsUlong);

    // 1.3 check the Link Power State, should be Partial (value 2).
    if (ssts.IPM != 2) {
        StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Transit into Slumber from Partial - bailed out, current link state is not Partial: %1x \n", channelExtension->PortNumber, ssts.IPM);

        return;
    }

    // 2. Change LPM State.
    // Link should be in idle state (able to accept new interface commands).
    if (cmd.ICC == 0) {
        ULONG waitTime;
        ULONG waitTimeLimit = AHCI_LINK_POWER_STATE_CHANGE_TIMEOUT_US;
        UCHAR iccAttempts = 0;

        AhciUlongIncrement(&(channelExtension->AutoPartialToSlumberDbgStats.InterfaceReady));

        //
        // Attempt to transition the link to Active.
        // By spec, Partial to Active transition should be completed in 10us. Reading register already takes sometime.
        // Poll for a little bit to give the link some time to go Active.
        // 
        cmd.ICC = 1;
        StorPortWriteRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->CMD.AsUlong, cmd.AsUlong);
        ssts.AsUlong = StorPortReadRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->SSTS.AsUlong);
        for (waitTime = 0; (waitTime < waitTimeLimit) && (ssts.IPM != 1); waitTime += 10) {
            //
            // Make a few attempts to program ICC if we haven't transitioned yet.
            //
            if (iccAttempts++ < 3) {
                cmd.ICC = 1;
                StorPortWriteRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->CMD.AsUlong, cmd.AsUlong);
            }

            StorPortStallExecution(10);  //10 microseconds
            ssts.AsUlong = StorPortReadRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->SSTS.AsUlong);
        }

        if (ssts.IPM != 1) {
            AhciUlongIncrement(&(channelExtension->AutoPartialToSlumberDbgStats.ActiveFailCount));
            StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Transit into Slumber from Partial - Failed to go to Active, SSTS.IPM = %u \n", channelExtension->PortNumber, ssts.IPM);
            return;
        }

        AhciUlongIncrement(&(channelExtension->AutoPartialToSlumberDbgStats.ActiveSuccessCount));

        //
        // Attempt to transition the link to Slumber.
        // Poll for a little bit to give the link some time to go Slumber.
        //
        iccAttempts = 0;
        cmd.ICC = 6;
        StorPortWriteRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->CMD.AsUlong, cmd.AsUlong);
        ssts.AsUlong = StorPortReadRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->SSTS.AsUlong);
        for (waitTime = 0; (waitTime < waitTimeLimit) && (ssts.IPM != 6); waitTime += 10) {
            //
            // Make a few attempts to program ICC if we haven't transitioned yet.
            //
            if (iccAttempts++ < 3) {
                cmd.ICC = 6;
                StorPortWriteRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->CMD.AsUlong, cmd.AsUlong);
            }

            StorPortStallExecution(10);  //10 microseconds
            ssts.AsUlong = StorPortReadRegisterUlong(channelExtension->AdapterExtension, &channelExtension->Px->SSTS.AsUlong);
        }

        if (ssts.IPM == 6) {
            AhciUlongIncrement(&(channelExtension->AutoPartialToSlumberDbgStats.SlumberSuccessCount));
            StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Transit into Slumber from Partial - Succeeded \n", channelExtension->PortNumber);
        } else {
            AhciUlongIncrement(&(channelExtension->AutoPartialToSlumberDbgStats.SlumberFailCount));
            StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Transit into Slumber from Partial - Failed, SSTS.IPM = %u \n", channelExtension->PortNumber, ssts.IPM);
        }
    } else {
        AhciUlongIncrement(&(channelExtension->AutoPartialToSlumberDbgStats.InterfaceNotReady));
    }

    return;
}

BOOLEAN
AhciAdapterPowerSettingNotification(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PSTOR_POWER_SETTING_INFO PowerSettingInfo
    )
{
    ULONG i;
    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if (AdapterExtension->PortExtension[i] != NULL) {
            AhciPortPowerSettingNotification(AdapterExtension->PortExtension[i], PowerSettingInfo);
        }
    }

    return TRUE;
}


VOID
AhciPortGetInitCommands(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    // Read _GTF from ACPI
    ULONG                    status = STOR_STATUS_SUCCESS;
    ACPI_EVAL_INPUT_BUFFER   inputData = {0};
    PACPI_EVAL_OUTPUT_BUFFER acpiData = NULL;
    PACPI_METHOD_ARGUMENT    argument = NULL;
    ULONG                    acpiDataSize = 256;     // initial size, should be good enough for most cases
    ULONG                    returnedLength = 0;
    UCHAR                    gtfCommandCount = 0;

    // send SECURE_FREEZE_LOCK by default
    BOOLEAN                  sendSecureFreezeLock = TRUE;


    // clear Init Commands area. need to do this for device removed previously (StorAHCI only knows about adapter removal, not device removal)
    ChannelExtension->DeviceInitCommands.CommandCount = 0;
    ChannelExtension->DeviceInitCommands.ValidCommandCount = 0;
    ChannelExtension->DeviceInitCommands.CommandToSend = 0;
    if (ChannelExtension->DeviceInitCommands.CommandTaskFile != NULL) {
        StorPortFreePool(ChannelExtension->AdapterExtension, (PVOID)ChannelExtension->DeviceInitCommands.CommandTaskFile);
        ChannelExtension->DeviceInitCommands.CommandTaskFile = NULL;
    }


    inputData.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputData.MethodNameAsUlong = ACPI_METHOD_GTF;

    status = StorPortAllocatePool(ChannelExtension->AdapterExtension,
                                  acpiDataSize,
                                  AHCI_POOL_TAG,
                                  (PVOID*)&acpiData);

    if (acpiData != NULL) {
        // call API to get required buffer size
        status = StorPortInvokeAcpiMethod(ChannelExtension->AdapterExtension,
                                          (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                          ACPI_METHOD_GTF,
                                          &inputData,
                                          sizeof(ACPI_EVAL_INPUT_BUFFER),
                                          (PVOID)acpiData,
                                          acpiDataSize,
                                          &returnedLength
                                          );

        // in case of the allocate buffer is too small, re-allocate buffer and retry the call
        if ( (status == STOR_STATUS_BUFFER_TOO_SMALL) && (acpiData->Length > acpiDataSize) ) {
            acpiDataSize = acpiData->Length;
            StorPortFreePool(ChannelExtension->AdapterExtension, (PVOID)acpiData);
            acpiData = NULL;
            // re-allocate a bigger buffer
            status = StorPortAllocatePool(ChannelExtension->AdapterExtension,
                                          acpiDataSize,
                                          AHCI_POOL_TAG,
                                          (PVOID*)&acpiData);

            if (acpiData != NULL) {
                status = StorPortInvokeAcpiMethod(ChannelExtension->AdapterExtension,
                                                  (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                                  ACPI_METHOD_GTF,
                                                  &inputData,
                                                  sizeof(ACPI_EVAL_INPUT_BUFFER),
                                                  (PVOID)acpiData,
                                                  acpiDataSize,
                                                  &returnedLength
                                                  );
            }
        }
    }

    // get _GTF commands count
    if ( (status == STOR_STATUS_SUCCESS) &&
         (acpiData != NULL) &&
         (acpiData->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) &&
         (acpiData->Count == 1) ) {

        argument = acpiData->Argument;

        if (argument->Type == ACPI_METHOD_ARGUMENT_BUFFER) {
            NT_ASSERT ((argument->DataLength % sizeof(ACPI_GTF_IDE_REGISTERS)) == 0);
            gtfCommandCount = (UCHAR)(argument->DataLength / sizeof(ACPI_GTF_IDE_REGISTERS));
        } else {
            NT_ASSERT(argument->Type == ACPI_METHOD_ARGUMENT_BUFFER);
        }
    }

    // Get Init Command count for devices


    //
    // calculate possible command count for memory allocation
    // an IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON command will be sent to device anyway.
    //
    ChannelExtension->DeviceInitCommands.CommandCount = gtfCommandCount + 1;

    if (IsAtaDevice(&ChannelExtension->DeviceExtension[0].DeviceParameters)) {
        if (ChannelExtension->DeviceExtension->SupportedCommands.SetDateAndTime == 0x1) {
            ChannelExtension->DeviceInitCommands.CommandCount++;
        }
        
        if (sendSecureFreezeLock) {
            ChannelExtension->DeviceInitCommands.CommandCount++;
        }

        if (NeedsPuisSpinUpOnPowerUp(ChannelExtension)) {
            ChannelExtension->DeviceInitCommands.CommandCount++;
        }
    }

    // copy _GTF commands into buffer
    if (ChannelExtension->DeviceInitCommands.CommandCount > 0) {
        PATA_TASK_FILE taskFile;
        ULONG i = 0, gtfIndex = 0;

        status = StorPortAllocatePool(ChannelExtension->AdapterExtension,
                                      ChannelExtension->DeviceInitCommands.CommandCount * sizeof(ATA_TASK_FILE),
                                      AHCI_POOL_TAG,
                                      (PVOID*)&ChannelExtension->DeviceInitCommands.CommandTaskFile);

        if ( (status != STOR_STATUS_SUCCESS) || (ChannelExtension->DeviceInitCommands.CommandTaskFile == NULL) ) {
            goto exit;
        }

        AhciZeroMemory((PCHAR)ChannelExtension->DeviceInitCommands.CommandTaskFile, ChannelExtension->DeviceInitCommands.CommandCount * sizeof(ATA_TASK_FILE));

        //
        // If we may need to send the PUIS spin-up command to this device later
        // then insert it here.  It needs to be first so that the drive is spun
        // up and subsequent commands can succeed.
        //
        if (NeedsPuisSpinUpOnPowerUp(ChannelExtension) &&
            (ChannelExtension->DeviceInitCommands.CommandCount - i) >= 1) {
            taskFile = ChannelExtension->DeviceInitCommands.CommandTaskFile + i;
            taskFile->Current.bCommandReg = IDE_COMMAND_SET_FEATURE;
            taskFile->Current.bFeaturesReg = IDE_FEATURE_PUIS_SPIN_UP;
            i++;
        }

        //
        // Now copy over the _GTF commands.
        //
        for (gtfIndex = 0; gtfIndex < gtfCommandCount; gtfIndex++, i++) {
            StorPortCopyMemory(ChannelExtension->DeviceInitCommands.CommandTaskFile + i,
                               argument->Data + (gtfIndex * sizeof(ACPI_GTF_IDE_REGISTERS)),
                               sizeof(ACPI_GTF_IDE_REGISTERS)
                               );
        }

        // add IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON for all devices
        if ((ChannelExtension->DeviceInitCommands.CommandCount - i) >= 1) {
            taskFile = ChannelExtension->DeviceInitCommands.CommandTaskFile + i;
            taskFile->Current.bFeaturesReg = IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON;
            taskFile->Current.bCommandReg = IDE_COMMAND_SET_FEATURE;
            i++;
        }

        // add more commands for ATA devices
        if (IsAtaDevice(&ChannelExtension->DeviceExtension[0].DeviceParameters)) {

            if (sendSecureFreezeLock) {
                
                if ((ChannelExtension->DeviceInitCommands.CommandCount - i) >= 1) {
                    taskFile = ChannelExtension->DeviceInitCommands.CommandTaskFile + i;
                    taskFile->Current.bCommandReg = IDE_COMMAND_SECURITY_FREEZE_LOCK;
                    i++;
                }
            }

            if ((ChannelExtension->DeviceInitCommands.CommandCount - i) >= 1) {
                if (ChannelExtension->DeviceExtension->SupportedCommands.SetDateAndTime == 0x1) {
                    taskFile = ChannelExtension->DeviceInitCommands.CommandTaskFile + i;
                    //setup TaskFile
                    BuildSetDateAndTimeTaskFile(taskFile);
                    i++;
                }
            }
        }

        ChannelExtension->DeviceInitCommands.ValidCommandCount = (UCHAR)i;
    }

exit:
    if (acpiData != NULL) {
        StorPortFreePool(ChannelExtension->AdapterExtension, (PVOID)acpiData);
        acpiData = NULL;
    }

    return;
}

VOID
AhciPortEvaluateSDDMethod(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    ULONG   status = STOR_STATUS_SUCCESS;
    ULONG   returnedLength = 0;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputData;
    PACPI_METHOD_ARGUMENT           argument;
    ULONG                           inputDataSize;

    // get the memory we need
    inputDataSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + sizeof(IDENTIFY_DEVICE_DATA);

    status = StorPortAllocatePool(ChannelExtension->AdapterExtension,
                                  inputDataSize,
                                  AHCI_POOL_TAG,
                                  (PVOID*)&inputData);

    if ( (status != STOR_STATUS_SUCCESS) || (inputData == NULL) ) {
        goto Exit;
    }

    AhciZeroMemory((PCHAR)inputData, inputDataSize);

    inputData->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    inputData->MethodNameAsUlong = ACPI_METHOD_SDD;
    inputData->Size = inputDataSize;
    inputData->ArgumentCount = 1;

    argument = inputData->Argument;
    argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    argument->DataLength = sizeof(IDENTIFY_DEVICE_DATA);
    StorPortCopyMemory(argument->Data, ChannelExtension->DeviceExtension[0].IdentifyDeviceData, sizeof(IDENTIFY_DEVICE_DATA));

    status = StorPortInvokeAcpiMethod(ChannelExtension->AdapterExtension,
                                      (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                      ACPI_METHOD_SDD,
                                      (PVOID)inputData,
                                      inputDataSize,
                                      NULL,
                                      0,
                                      &returnedLength
                                      );

Exit:
    // we don't care about the return status
    UNREFERENCED_PARAMETER(status);

    if (inputData != NULL) {
        StorPortFreePool(ChannelExtension->AdapterExtension, inputData);
    }

    return;
}

VOID
AhciAdapterEvaluateDSMMethod(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    ULONG   status = STOR_STATUS_SUCCESS;
    ULONG   returnedLength = 0;

    PACPI_METHOD_ARGUMENT           argument;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputData = NULL;
    ULONG                           inputDataSize;

    PACPI_EVAL_OUTPUT_BUFFER        outputData = NULL;
    ULONG                           outputDataSize;

    // 0. get output buffer ready, make sure the buffer is big enough.
    outputDataSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                     AHCI_MAX_PORT_COUNT * ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));

    status = StorPortAllocatePool(AdapterExtension,
                                  outputDataSize,
                                  AHCI_POOL_TAG,
                                  (PVOID*)&outputData);

    if ( (status != STOR_STATUS_SUCCESS) || (outputData == NULL) ) {
        goto Exit;
    }

    AhciZeroMemory((PCHAR)outputData, outputDataSize);

    // 1. check if Link Power Management is supported in ACPI

    // get the memory we need

    // N.B. 4 arguments are stored in the ACPI_EVAL_INPUT_BUFFER_COMPLEX
    //      and passed to acpi.sys to eval _DSM.
    //
    //      ACPI_EVAL_INPUT_BUFFER_COMPLEX with 4 arguments.
    //      0 - GUID
    //      1 - ULONG  (revision id)
    //      2 - ULONG  (function index)
    //      3 - unused (package)
    //
    inputDataSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));

    status = StorPortAllocatePool(AdapterExtension,
                                  inputDataSize,
                                  AHCI_POOL_TAG,
                                  (PVOID*)&inputData);

    if ( (status != STOR_STATUS_SUCCESS) || (inputData == NULL) ) {
        goto Exit;
    }

    AhciZeroMemory((PCHAR)inputData, inputDataSize);

    inputData->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    inputData->MethodNameAsUlong = ACPI_METHOD_DSM;
    inputData->Size = inputDataSize;
    inputData->ArgumentCount = 4;

    // argument 0 - Interface GUID
    argument = &inputData->Argument[0];
    argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    argument->DataLength = sizeof(GUID);
    StorPortCopyMemory(&argument->Data[0], &LINK_POWER_ACPI_DSM_GUID, sizeof(GUID));

    // argument 1 - Revision number
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument,
                                     ACPI_METHOD_DSM_LINKPOWER_REVISION);

    // argument 2 - Function Index
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument,
                                     ACPI_METHOD_DSM_LINKPOWER_FUNCTION_SUPPORT);

    // argument 3 - Function-dependent package. not used for ACPI_METHOD_DSM_LINKPOWER_FUNCTION_SUPPORT
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument, 0);
    argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;


    status = StorPortInvokeAcpiMethod(AdapterExtension,
                                      NULL,             // NULL for Address field means the request is for Adapter
                                      ACPI_METHOD_DSM,
                                      (PVOID)inputData,
                                      inputDataSize,
                                      outputData,
                                      outputDataSize,
                                      &returnedLength
                                      );

    if ( (status != STOR_STATUS_SUCCESS) ||
         (returnedLength < (FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)))) ) {
        goto Exit;
    }

    argument = outputData->Argument;

    if ( ((argument->Argument & ACPI_METHOD_DSM_LINKPOWER_FUNCTION_QUERY) != 0) &&
         ((argument->Argument & ACPI_METHOD_DSM_LINKPOWER_FUNCTION_CONTROL) != 0) ) {

        // If ACPI_METHOD_DSM_LINKPOWER_FUNCTION_CONTROL is supported, it supports value (-1) that apply action to all ports.
        // Mark adapter supports _DSM. This indicates that it has capability to power on all ports and connected devices.
        if (AdapterExtension->StateFlags.SupportsAcpiDSM != TRUE) {
            AdapterExtension->StateFlags.SupportsAcpiDSM = TRUE;
        }

    }

Exit:
    if (inputData != NULL) {
        StorPortFreePool(AdapterExtension, inputData);
    }

    if (outputData != NULL) {
        StorPortFreePool(AdapterExtension, outputData);
    }

    return;
}


VOID
AhciPortAcpiDSMControl(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ ULONG                   PortNumber,
    _In_ BOOLEAN                 Sleep
  )
{
    ULONG   status = STOR_STATUS_SUCCESS;
    ULONG   returnedLength = 0;

    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputData;
    PACPI_METHOD_ARGUMENT           argument;
    ULONG                           inputDataSize;

    // get the memory we need
    inputDataSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
                    ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +
                    FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Argument) +
                    2* ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));

    status = StorPortAllocatePool(AdapterExtension,
                                  inputDataSize,
                                  AHCI_POOL_TAG,
                                  (PVOID*)&inputData);

    if ( (status != STOR_STATUS_SUCCESS) || (inputData == NULL) ) {
        goto Exit;
    }

    AhciZeroMemory((PCHAR)inputData, inputDataSize);

    inputData->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    inputData->MethodNameAsUlong = ACPI_METHOD_DSM;
    inputData->Size = inputDataSize;
    inputData->ArgumentCount = 4;

    // argument 0 - Interface GUID
    argument = &inputData->Argument[0];
    argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    argument->DataLength = sizeof(GUID);
    StorPortCopyMemory(&argument->Data[0], &LINK_POWER_ACPI_DSM_GUID, sizeof(GUID));

    // argument 1 - Revision number
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument,
                                     ACPI_METHOD_DSM_LINKPOWER_REVISION);

    // argument 2 - Function Index
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument,
                                     ACPI_METHOD_DSM_LINKPOWER_FUNCTION_CONTROL);

    // argument 3 - Function-dependent package.
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE_EX;
    argument->DataLength = 2 * ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));
    // argument 3 - Package entry 0
    argument = (PACPI_METHOD_ARGUMENT)argument->Data;
    if (PortNumber == (ULONG)-1) {
        // all 1s indicates power on operation is for all ports/devices
        NT_ASSERT(Sleep == FALSE);
        ACPI_METHOD_SET_ARGUMENT_INTEGER(argument, PortNumber);
    } else {
        // convert PortNumber to be ACPI format of Address
        ACPI_METHOD_SET_ARGUMENT_INTEGER(argument, (PortNumber << 16) | 0xFFFF);
    }
    // argument 3 - Package entry 1
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(argument, Sleep ? 0 : 1);

    status = StorPortInvokeAcpiMethod(AdapterExtension,
                                      NULL,             // NULL for Address field means the request is for Adapter
                                      ACPI_METHOD_DSM,
                                      (PVOID)inputData,
                                      inputDataSize,
                                      NULL,
                                      0,
                                      &returnedLength
                                      );

Exit:
    UNREFERENCED_PARAMETER(status);

    if (inputData != NULL) {
        StorPortFreePool(AdapterExtension, inputData);
    }

    return;
}


#pragma warning(pop) // un-sets any local warning changes



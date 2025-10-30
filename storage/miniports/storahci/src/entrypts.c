/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    entrypts.c

Abstract:

    This file contains function of entry points to the AHCI miniport.


Notes:

Revision History:

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4152) // nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

#include "generic.h"


// storahci.sys global variables
PVOID   g_AdapterExtension[4] = {0};
UCHAR   g_AdapterExtensionIndex = 0;

//
// Boot device telemetry default initialization values, override by the registry table
//

ULONG
AhciPublicGPLogTableAddresses[TC_PUBLIC_DEVICEDUMP_CONTENT_GPLOG_MAX] =
    {
        //0x01,     // Summary SMART error log. Accessed by SMART command only
        //0x02,     // Comprehensive SMART error log. Accessed by SMART command only
        0x04,       // Device statistics page
        0x08,       // Power conditions
        0x0d,       // LPS Mis-alignment
        0x10,       // NCQ command error log
        0x11,       // SATA PHY event counters log
        0
    };

ULONG
AhciGPLogPageIntoPrivate =  IDE_GP_LOG_CURRENT_DEVICE_INTERNAL_STATUS;

//

ULONG
DriverEntry(
    _In_ PVOID Argument1,    //IN PDRIVER_OBJECT  DriverObject,
    _In_ PVOID Argument2     //IN PUNICODE_STRING  RegistryPath
    )

/*++

Routine Description:

    Initial entry point for miniport driver.

Arguments:

    Driver Object

Return Value:

    Status from StorPortInitialize()

--*/

{
    ULONG status;
    HW_INITIALIZATION_DATA hwInitializationData = {0};

    DebugPrint((1, "\tSTORAHCI, Storport AHCI Miniport Driver.\n"));

    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    // required miniport entry point routines.
    hwInitializationData.HwInitialize = AhciHwInitialize;
    hwInitializationData.HwStartIo = AhciHwStartIo;
    hwInitializationData.HwInterrupt = AhciHwInterrupt;
    hwInitializationData.HwFindAdapter = AhciHwFindAdapter;
    hwInitializationData.HwResetBus = AhciHwResetBus;
    hwInitializationData.HwAdapterControl = AhciHwAdapterControl;
    hwInitializationData.HwBuildIo = AhciHwBuildIo;
    hwInitializationData.HwTracingEnabled = AhciHwTracingEnabled;
    hwInitializationData.HwUnitControl = AhciHwUnitControl;

    // Specify adapter specific information.
    hwInitializationData.AutoRequestSense = TRUE;
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.NumberOfAccessRanges = NUM_ACCESS_RANGES;
    
    //
    // Support both PCI/ACPI enumerations on ARM64 platform for SATA device.
    // Miniport uses flag to indicate storport to query/override the interface type reported here.
    //
#if defined(_ARM_) || defined(_ARM64_)
    hwInitializationData.FeatureSupport |= STOR_FEATURE_SET_ADAPTER_INTERFACE_TYPE;
    hwInitializationData.AdapterInterfaceType = InterfaceTypeUndefined;
#else
    hwInitializationData.AdapterInterfaceType = PCIBus;
#endif
    hwInitializationData.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;
    hwInitializationData.TaggedQueuing = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;
    hwInitializationData.FeatureSupport |= STOR_FEATURE_ATA_PASS_THROUGH;                       // indicating this miniport driver supports ATA PASS-THROUGH(16) command.
    hwInitializationData.FeatureSupport |= STOR_FEATURE_FULL_PNP_DEVICE_CAPABILITIES;           // indicating this miniport driver supplies values for all PnP Device Capability fields.
    hwInitializationData.FeatureSupport |= STOR_FEATURE_DUMP_POINTERS;                          // indicating this miniport support the dump pointers SRBs
    hwInitializationData.FeatureSupport |= STOR_FEATURE_DUMP_RESUME_CAPABLE;                    // indicating this miniport driver supports resume capability for dump stack.
    hwInitializationData.FeatureSupport |= STOR_FEATURE_DEVICE_NAME_NO_SUFFIX;                  // indicating the miniport driver prefers device friendly name without suffix: "SCSI <type> Device"
    hwInitializationData.FeatureSupport |= STOR_FEATURE_DEVICE_DESCRIPTOR_FROM_ATA_INFO_VPD;    // indicating that port driver forms STORAGE_DEVICE_DESCRIPTOR from ATA Information VPD page rather than INQUIRY data
    hwInitializationData.FeatureSupport |= STOR_FEATURE_EXTRA_IO_INFORMATION;                   // Indicating that miniport driver wants SRBEX_DATA_IO_INFO in a SRBEX if available
    hwInitializationData.FeatureSupport |= STOR_FEATURE_ADAPTER_NOT_REQUIRE_IO_PORT;            // Indicating that miniport driver doesn't require IO Port resource for its adapter.
    hwInitializationData.FeatureSupport |= STOR_FEATURE_DUMP_INFO;                              // Indicating that the miniport driver supports the dump info SRBs.

    // Set required extension sizes.
    hwInitializationData.DeviceExtensionSize = sizeof(AHCI_ADAPTER_EXTENSION);

    // NOTE: Command Table (1st field in AHCI_SRB_EXTENSION structure) must align to 128 bytes as physical limitation.
    // StorPort does not have interface allowing miniport requiring this.
    // Adding 128 in SrbExtensionSize so that we can use the part starting from right alignment.
    hwInitializationData.SrbExtensionSize = sizeof(AHCI_SRB_EXTENSION) + 128 ; // SrbExtension contains AHCI_SRB_EXTENSION

    //
    // Support SrbEx by default.
    //
    hwInitializationData.SrbTypeFlags = SRB_TYPE_FLAG_STORAGE_REQUEST_BLOCK;

    // call StorPort to register HW init data
    status = StorPortInitialize(Argument1,
                                Argument2,
                                &hwInitializationData,
                                NULL);

    return status;

} // end DriverEntry()

VOID
GetInterruptMode (
_In_ PAHCI_ADAPTER_EXTENSION    AdapterExtension
)
/*++
    This function determines the interrupt/synchronization model that StorAHCI uses.
    StorAHCI supports two interrupt/synchronization model: 
    1. More than one port share interrupt (line based, single message MSI or there are ports share same interrupt message using MSI).
       StorAHCI looks into IS register to understand which port triggers the interrupt.
       Synchronization will acquire interrupt spinlock for the whole adapter.
    2. Each port has its own interrupt message in MSI mode.
       StorAHCI uses message ID as port number as interrupt source.
       Synchronization will acquire message spinlock.

Called by:
    AhciHwFindAdapter

Note:
    AdapterExtension->HighestPort should have been assigned value before calling this function.

Affected Variables/Registers:
    AdapterExtension->StateFlags.InterruptMessagePerPort

Return Values:
    None
--*/
{
    ULONG status;
    ULONG messageCount = 0;
    MESSAGE_INTERRUPT_INFORMATION messageInfo = {0};

    //
    // For MSI/MSI-X interrupt, use loop to get MSI message info until the API fails.
    // In dump mode, always use interrupt share mode.
    //
    if (!IsDumpMode(AdapterExtension)) {
        AHCI_Global_HBA_CONTROL ghc = {0};

        ghc.AsUlong = StorPortReadRegisterUlong(AdapterExtension, &AdapterExtension->ABAR_Address->GHC.AsUlong);

        //
        // If GHC.MRSM is set to 1, it means that adapter has reverted to single MSI mode.
        //
        if (ghc.MRSM == 0) {
            for (messageCount = 0;; messageCount++) {
                status = StorPortGetMSIInfo(AdapterExtension, messageCount, &messageInfo);

                if (status != STOR_STATUS_SUCCESS) {
                    break;
                }
            }
        }
    }

    if (messageCount > AdapterExtension->HighestPort) {
        AdapterExtension->StateFlags.InterruptMessagePerPort = 1;
    } else {
        AdapterExtension->StateFlags.InterruptMessagePerPort = 0;
    }

    StorPortDebugPrint(3, "StorAHCI - Interrupt Mode: Adapter 0x%04X-0x%04X --- %s\n",
                           AdapterExtension->VendorID,
                           AdapterExtension->DeviceID,
                           (AdapterExtension->StateFlags.InterruptMessagePerPort == 1) ? "MessagePerPort" : "Legacy");

    return;
}


BOOLEAN
AllocateResourcesForAdapter(
    _In_ PAHCI_ADAPTER_EXTENSION         AdapterExtension,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_range_(1, AHCI_MAX_PORT_COUNT) ULONG PortCount
    )
/*
    Internal function to allocate required memory for Ports

    In AHCI, the controller structures are both the command header list and the received FIS buffer.
        The mechanism for requesting all of this memory is StorPortGetUncachedExtension. This function returns page-aligned address.
        NOTE! StorPortGetUncachedExtension can only be called while in FindAdapter routine.
        NOTE! In order to perform crashdump/hibernate the UncachedExtensionSize should not be larger than 30K.

    Alignment requirement:
    -   Command List Base Addresses: must be 1K aligned, and the Command list is (sizeof (AHCI_COMMAND_HEADER) * cap.NCS), which is some multiple of 32 bytes in length.
    -   The FIS Base Address: must be 256 aligned, and the FIS Receive buffer is sizeof (AHCI_RECEIVED_FIS), 256 bytes in length.
        The Command Table: must be 128 aligned, and is sizeof(AHCI_COMMAND_TABLE), 1280 bytes in length thanks to some padding in the AHCI_COMMAND_TABLE structure.
                           Command Table is per request, resides in AHCI_SRB_EXTENSION for each request.

        Size of nonCachedExtensionSize for every channel should round up to KiloBytes, so that the pointer for next channel is starting at 1K boundary.

        the Command Header is variable and must be padded so the Received FIS is on a 256 boundary.
        Therefore the number of Command Headers must be 256/32 = 8. Round cap.NCS to the next multiple of 8
*/
{
    ULONG alignment = 0x400; // 1K
    BOOLEAN dumpMode = IsDumpMode(AdapterExtension);
    ULONG paddedNCS = 0;
    ULONG paddedSrbExtensionSize = 0;
    ULONG nonCachedExtensionSize = 0;
    ULONG channelExtensionSize = sizeof(AHCI_CHANNEL_EXTENSION);
    PVOID portsChannelExtension = NULL;
    ULONG executionHistorySize = sizeof(EXECUTION_HISTORY) * MAX_EXECUTION_HISTORY_ENTRY_COUNT;
    PVOID portsExecutionHistory = NULL;
    ULONG_PTR left = 0;
    ULONG i = 0;
    ULONG j = 0;

    NT_ASSERT(PortCount >= 1 && PortCount <= AHCI_MAX_PORT_COUNT);

    // 2.1 allocate nonCachedExtension for Hardware access

    // Allocate Identify Data buffer, CommandList, Receive FIS, SRB Extension for Local SRB, INQUIRY buffer
    // paddedNCS is (CAP.NCS+1) rounds up to the next multiple of 8.
    paddedNCS = ((AdapterExtension->CAP.NCS) / 8 + 1) * 8;

    // SrbExtension needs to align to 128 bytes, pad the size to be multiple of 128 bytes
    paddedSrbExtensionSize = ((sizeof(AHCI_SRB_EXTENSION) - 1) / 128 + 1) * 128;

    // size per Port
    nonCachedExtensionSize = sizeof(AHCI_COMMAND_HEADER) * paddedNCS +      // align to 1K, padded to multiple of 256 bytes. (nocachedextension is page aligned)
                             sizeof(AHCI_RECEIVED_FIS) +                    // align to 256 bytes.
                             paddedSrbExtensionSize * 2 +                   // align to 128 bytes. Local.SrbExtension and SenseSrbExtension
                             sizeof(IDENTIFY_DEVICE_DATA) +                 // 512 bytes
                             ATA_BLOCK_SIZE +                               // ReadLogExtPageData --- 512 bytes
                             INQUIRYDATABUFFERSIZE;                         // Inquiry Data

    // round up to KiloBytes if it's not dump mode. this makes sure that NonCachedExtension for next port can align to 1K.
    if (!dumpMode) {
        nonCachedExtensionSize = ((nonCachedExtensionSize - 1) / alignment + 1) * alignment;

        AdapterExtension->NonCachedExtension = StorPortGetUncachedExtension(AdapterExtension,
                                                                            ConfigInfo,
                                                                            nonCachedExtensionSize * PortCount);
    } else {
        // dump mode, address returned from StorPortGetUncachedExtension() is not guaranteed align with 1K.
        // adding 1K into length so that we can start from 1K alignment safely.
        //       NOTE: StorPortAllocatePool is not supported in dump stack, so we allocate ChannelExtension from UnCachedExtension as work around.
        nonCachedExtensionSize += alignment;

        AdapterExtension->NonCachedExtension = StorPortGetUncachedExtension(AdapterExtension,
                                                                            ConfigInfo,
                                                                            (nonCachedExtensionSize + channelExtensionSize) * PortCount);
    }

    if (AdapterExtension->NonCachedExtension == NULL) {

        // we cannot continue if cannot get nonCachedMemory for Channels.
        NT_ASSERT(FALSE);
        return FALSE;
    }

    AhciZeroMemory((PCHAR)AdapterExtension->NonCachedExtension, nonCachedExtensionSize * PortCount);

    // 2.2 allocate resources for Ports which need AHCI_CHANNEL_EXTENSION for each of them.
    if (!dumpMode) {
        ULONG status = STOR_STATUS_SUCCESS;
        // allocate pool and zero the content
        status = StorPortAllocatePool(AdapterExtension,
                                      channelExtensionSize * PortCount,
                                      AHCI_POOL_TAG,
                                      (PVOID*)&portsChannelExtension);

        if ((status != STOR_STATUS_SUCCESS) || (portsChannelExtension == NULL)) {
           // we cannot continue if cannot get memory for ChannelExtension.
            return FALSE;
        }

        status = StorPortAllocatePool(AdapterExtension,
                                      (ULONG)(executionHistorySize * PortCount),
                                      AHCI_POOL_TAG,
                                      (PVOID*)&portsExecutionHistory);

        if ((status != STOR_STATUS_SUCCESS) || (portsExecutionHistory == NULL)) {
           // Continue if cannot get memory for ExecutionHistory.
           NT_ASSERT(FALSE);
        }
    } else {
        // get channelExtension from UnCachedExtension
        portsChannelExtension = (PCHAR)AdapterExtension->NonCachedExtension + (nonCachedExtensionSize * PortCount);
    }

    AhciZeroMemory((PCHAR)portsChannelExtension, channelExtensionSize * PortCount);

    // assign allocated uncachedExension into ChannelExtensions
    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if ( (AdapterExtension->PortImplemented & (1 << i)) != 0 ) {
            PCHAR portsUncachedExtension = (PCHAR)AdapterExtension->NonCachedExtension + nonCachedExtensionSize * j;

            // this port is implemented, allocate and initialize extension for the port
            AdapterExtension->PortExtension[i] = (PAHCI_CHANNEL_EXTENSION)((PCHAR)portsChannelExtension + channelExtensionSize * j);
            AdapterExtension->PortExtension[i]->AdapterExtension = AdapterExtension;
            AdapterExtension->PortExtension[i]->PortNumber = i;
            // set ChannelExtension fields that use NonCachedExtension
            if (!dumpMode) {
                AdapterExtension->PortExtension[i]->CommandList = (PAHCI_COMMAND_HEADER)(portsUncachedExtension);
                if (portsExecutionHistory != NULL) {
                    AdapterExtension->PortExtension[i]->ExecutionHistory = (PEXECUTION_HISTORY)((PCHAR)portsExecutionHistory + executionHistorySize * j);
                }
            } else {
                //get the starting pointer; align the starting location to 1K.
                left = ((ULONG_PTR)portsUncachedExtension) % alignment;

                if (left > 0) {
                    AdapterExtension->PortExtension[i]->CommandList = (PAHCI_COMMAND_HEADER)(portsUncachedExtension + alignment - left);
                } else {
                    AdapterExtension->PortExtension[i]->CommandList = (PAHCI_COMMAND_HEADER)(portsUncachedExtension);
                }
            }

            AdapterExtension->PortExtension[i]->ReceivedFIS = (PAHCI_RECEIVED_FIS)((PCHAR)AdapterExtension->PortExtension[i]->CommandList + sizeof(AHCI_COMMAND_HEADER) * paddedNCS);
            AdapterExtension->PortExtension[i]->Local.SrbExtension = (PAHCI_SRB_EXTENSION)((PCHAR)AdapterExtension->PortExtension[i]->ReceivedFIS + sizeof(AHCI_RECEIVED_FIS));
            AdapterExtension->PortExtension[i]->Sense.SrbExtension = (PAHCI_SRB_EXTENSION)((PCHAR)AdapterExtension->PortExtension[i]->Local.SrbExtension + paddedSrbExtensionSize);
            AdapterExtension->PortExtension[i]->DeviceExtension[0].IdentifyDeviceData = (PIDENTIFY_DEVICE_DATA)((PCHAR)AdapterExtension->PortExtension[i]->Sense.SrbExtension + paddedSrbExtensionSize);
            AdapterExtension->PortExtension[i]->DeviceExtension[0].ReadLogExtPageData = (PUSHORT)((PCHAR)AdapterExtension->PortExtension[i]->DeviceExtension[0].IdentifyDeviceData + sizeof(IDENTIFY_DEVICE_DATA));
            AdapterExtension->PortExtension[i]->DeviceExtension[0].InquiryData = (PUCHAR)((PCHAR)AdapterExtension->PortExtension[i]->DeviceExtension[0].ReadLogExtPageData + ATA_BLOCK_SIZE);
            //
            j++;
        }
    }

    return TRUE;
}


ULONG AhciHwFindAdapter(
    _In_ PVOID AdapterExtension,
    _In_ PVOID HwContext,
    _In_ PVOID BusInformation,
    _In_z_ PCHAR ArgumentString,
    _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PBOOLEAN Reserved3
    )
/*++
    This function is called by the Storport driver indirectly when handling an IRP_MJ_PnP, IRP_MN_START_DEVICE.
    The adapter is being started. This function is called at PASSIVE LEVEL.
It assumes:
    Resources for the AHCI controller exist in a single Memory Mapped range.
    No Channels have been initialized or started; no IO is outstanding.
Called by:
    Storport

It performs:
    (overview)
    1. Check the working mode and retrieve adapter vendor/model info.
    2. Initialize ChannelExtension with Identifying memory mapped IO resources, Version and CAP.
    3. Enabling the AHCI interface as per AHCI 1.1 section 10.1.2 (part one)
    4. Initializing the IDE_CONTROLLER_CONFIGURATION structure with data from the AHCI Interface
    (details)
    1.1 Get dump mode
    1.2 Gather Vendor,Device,Revision IDs from PCI
    2.1 Initialize adapterExtension with AHCI abar
    2.2 Initialize adapterExtension with version & cap values
    3.1 Turn on AE, reset the controller if AE is already set
        AHCI 1.1 Section 10.1.2 - 1.
        "Indicate that system software is AHCI aware by setting GHC.AE to '1'."
    3.2 Determine which ports are implemented by the HBA
        AHCI 1.1 Section 10.1.2 - 2.
        "Determine which ports are implemented by the HBA, by reading the PI register. This bitmap value will aid software in determining how many ports are available and which port registers need to be initialized."
    3.3 get biggest port number
    3.4 Initializing the rest of PORT_CONFIGURATION_INFORMATION
    3.5 Register Power Setting Change Notification Guids
    4.1 Turn on IE, pending interrupts will be cleared when port starts
        This has to be done after 3.2 because we need to know the number of channels before we check each PxIS.
        Verify that none of the PxIS registers are loaded, but take no action
        Note: Due to the multi-tiered nature of the AHCI HBA's interrupt architecture, system software must always ensure that the PxIS (clear this first) and IS.IPS (clear this second) registers are cleared to '0' before programming the PxIE and GHC.IE registers. This will prevent any residual bits set in these registers from causing an interrupt to be asserted.
        However, the interrupt handler hasn't been hooked up by StorPort yet, so no interrupts will be handled by software until that happens.
    4.2 Initialize the remaining port configuration settings after getting registry settings.
    4.3 Allocate resources for both DMA use and all Channel/Port extensions.
    4.4 Initialize ports and start them. Dump stack will do this when receiving the INQUIRY command

Affected Variables/Registers:
    AdapterExtension->ABAR_Address
    AdapterExtension->CAP
    AdapterExtension->CAP2
    AdapterExtension->Version

    GHC.AE, GHC.IE, GHC.HR
    IS and all fields in the HBA's register memory space except PxFB/PxFBU/PxCLB/PxCLBU that are not HwInit

Return Values:
    The miniport driver returns TRUE if it successfully execute the whole function.
    Any errors causes the driver to return FALSE and prevents the driver from loading.

Note:
    HwStorFindAdapter must set the MaximumTransferLength and NumberOfPhysicalBreaks fields in the ConfigInfo structure.

    Other than these fields, the PORT_CONFIGURATION_INFORMATION (Storport) structure will always fully specify all adapter resources
    that are required to start the adapter.

--*/
{
    ULONG storStatus = STOR_STATUS_UNSUCCESSFUL;
    PAHCI_ADAPTER_EXTENSION adapterExtension = NULL;
    PAHCI_MEMORY_REGISTERS  abar = NULL;
    //Used to find the number of channels implemented
    ULONG                   i = 0;
    ULONG                   piMask = 0;
    ULONG                   numberOfHighestPort = 0;
    ULONG                   portCount = 0;
    ULONG                   portNumber;
    //Used to enable the AHCI interface
    AHCI_Global_HBA_CONTROL ghc = {0};
    //guids
    GUID                    powerSettingChangeGuids[2] = {0};
    BOOLEAN                 dumpMode = FALSE;
    PAHCI_DUMP_CONTEXT      dumpContext = (PAHCI_DUMP_CONTEXT)ConfigInfo->MiniportDumpData;

    UNREFERENCED_PARAMETER(HwContext);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ArgumentString);
    UNREFERENCED_PARAMETER(Reserved3);

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    // "StoppedState == 0" indicates it's the normal HwFindAdapter call;
    // "StoppedState == 1" indicates the adapter is restarting.
    if (adapterExtension->StateFlags.StoppedState == 0) {
        // use for debugging, assume no more than 4 AHCI adapters in system
        g_AdapterExtension[g_AdapterExtensionIndex++] = AdapterExtension;
        g_AdapterExtensionIndex %= 4;
    }

    //adapterExtension->AdapterNumber = ConfigInfo->SlotNumber;
    adapterExtension->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;
    adapterExtension->SlotNumber = ConfigInfo->SlotNumber;

    //1.1 Get dump mode
    adapterExtension->DumpMode = ConfigInfo->DumpMode;
    dumpMode = IsDumpMode(adapterExtension);

    if (dumpMode) {
        if (dumpContext != NULL) {
            // In dump/hibernation mode, need to mark ConfigInfo->MiniportDumpData
            // and any embedded memory buffer(s) in MiniportDumpData
            StorPortMarkDumpMemory(AdapterExtension,
                                   dumpContext,
                                   FIELD_OFFSET(AHCI_DUMP_CONTEXT, Ports) +
                                   (dumpContext->PortCount * sizeof(AHCI_DUMP_PORT_CONTEXT)),
                                   0);
        } else {
            NT_ASSERT(FALSE);
            return SP_RETURN_ERROR;
        }
    }

    //1.2 Gather Vendor,Device,Revision IDs from PCI
    if (!dumpMode) {

        //
        // Enumeration may be via either PCI or ACPI. Bus type is set by storport/miniport.
        // Check if we are enumerating via PCI, and retrieve the address appropriately.
        //
        if (ConfigInfo->AdapterInterfaceType == PCIBus) {
            ULONG pcicfgLen = 0;
            UCHAR pcicfgBuffer[0x30] = {0};

            pcicfgLen = StorPortGetBusData(adapterExtension,
                                           PCIConfiguration,
                                           ConfigInfo->SystemIoBusNumber,
                                           (ULONG)ConfigInfo->SlotNumber,
                                           (PVOID)pcicfgBuffer,
                                           (ULONG)0x30);
            if (pcicfgLen == 0x30) {
                PPCI_COMMON_CONFIG pciConfigData = (PPCI_COMMON_CONFIG)pcicfgBuffer;
                adapterExtension->VendorID = pciConfigData->VendorID;
                adapterExtension->DeviceID = pciConfigData->DeviceID;
                adapterExtension->RevisionID = pciConfigData->RevisionID;
                // on PCI bus, AHCI Base Address is BAR5. Bits 0-3 defined for other usages, not part of address value.
                adapterExtension->AhciBaseAddress = pciConfigData->u.type0.BaseAddresses[5] & (0xFFFFFFF0);
            } else {
                NT_ASSERT(FALSE);
                return SP_RETURN_ERROR;
            }
        }
    } else {
        adapterExtension->VendorID = dumpContext->VendorID;
        adapterExtension->DeviceID = dumpContext->DeviceID;
        adapterExtension->RevisionID = dumpContext->RevisionID;
        adapterExtension->AhciBaseAddress = dumpContext->AhciBaseAddress;

        adapterExtension->RegistryFlags = dumpContext->AdapterRegistryFlags;
        adapterExtension->LogFlags = dumpContext->LogFlags;
    }

    //2.1 Initialize adapterExtension with AHCI abar
    abar = GetABARAddress(adapterExtension, ConfigInfo);

    //2.1.1 If abar is still NULL after all of that, malformed resources.  We aren't going to get very far.
    if (abar == NULL) {
        return SP_RETURN_ERROR;
    } else {
        adapterExtension->ABAR_Address = abar;
    }

    //2.2 Initialize adapterExtension with version & cap values
    adapterExtension->Version.AsUlong = StorPortReadRegisterUlong(adapterExtension, &abar->VS.AsUlong);
    adapterExtension->CAP.AsUlong = StorPortReadRegisterUlong(adapterExtension, &abar->CAP.AsUlong);
    adapterExtension->CAP2.AsUlong = StorPortReadRegisterUlong(adapterExtension, &abar->CAP2.AsUlong);

    //3.1 Turn on AE (AHCI 1.1 Section 10.1.2 - 1)
    ghc.AsUlong = StorPortReadRegisterUlong(adapterExtension, &abar->GHC.AsUlong);
    if (ghc.AE == 1) {
        if (!AhciAdapterReset(adapterExtension)) {
            return SP_RETURN_ERROR;
        }
    } //AE is 0. Either through power up or reset we are now pretty sure the controller is in 5.2.2.1    H:Init
    ghc.AsUlong = 0;
    ghc.AE = 1;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC.AsUlong, ghc.AsUlong);

    adapterExtension->IS = &abar->IS;

    //3.2 Determine which ports are implemented by the HBA
    if (!dumpMode) {
        // Get implemented ports from PI register (AHCI 1.1 Section 10.1.2 - 2)
        adapterExtension->PortImplemented = StorPortReadRegisterUlong(adapterExtension, &abar->PI);
    } else {
        // Get implemented ports from the dump context
        adapterExtension->PortImplemented = 0;

        for (i = 0; i < dumpContext->PortCount; i++) {
            adapterExtension->PortImplemented |= 1 << dumpContext->Ports[i].PortNumber;
        }
    }

    // At least one port must be implemented
    if (adapterExtension->PortImplemented == 0) {
        return SP_RETURN_ERROR;
    }

    //3.3 Get biggest port number value and implemented port count
    if (!dumpMode) {
        numberOfHighestPort = AHCI_MAX_PORT_COUNT;
        //Check from highest bit to lowest bit for the first highest bit set
        for (piMask = (ULONG)(1 << (AHCI_MAX_PORT_COUNT - 1)); piMask != 0; piMask = (ULONG)(piMask >> 1)){
            numberOfHighestPort--;
            if ((adapterExtension->PortImplemented & piMask) != 0) {
                break;
            }
        }

        for (i = 0; i <= numberOfHighestPort; i++) {
            if ((adapterExtension->PortImplemented & (1 << i)) != 0) {
                portCount++;
            }
        }
    } else {
        numberOfHighestPort = 0;
        for (i = 0; i < dumpContext->PortCount; i++) {
            if (dumpContext->Ports[i].PortNumber > numberOfHighestPort) {
                numberOfHighestPort = (UCHAR)dumpContext->Ports[i].PortNumber;
            }
        }

        portCount = dumpContext->PortCount;
    }

    NT_ASSERT(numberOfHighestPort < AHCI_MAX_PORT_COUNT);
    adapterExtension->HighestPort = numberOfHighestPort;

    //
    // AHCI specification requires that implemented port count (number of bit set in PI register)
    // is less than or equal to CAP.NP + 1
    //
    // Currently StorAHCI does not utilize CAP.NP. It remains useful to identify platform BIOSes
    // which may violate the specification.
    //
    NT_ASSERT(portCount > 0 && portCount <= (ULONG)(adapterExtension->CAP.NP + 1));

    //3.4 Initializing the rest of PORT_CONFIGURATION_INFORMATION
    ConfigInfo->MaximumTransferLength = AHCI_MAX_TRANSFER_LENGTH_DEFAULT;
    ConfigInfo->NumberOfPhysicalBreaks = 0x21;  // Since "NumberOfPhysicalBreaks" has been used in storage stack as the count of entries, use value of physical breaks plus one.
    ConfigInfo->AlignmentMask = 1;              // ATA devices need WORD alignment
    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->NumberOfBuses = (UCHAR)(numberOfHighestPort + 1);
    ConfigInfo->MaximumNumberOfTargets = AHCI_MAX_DEVICE;
    ConfigInfo->MaximumNumberOfLogicalUnits = 1 /*AHCI_MAX_LUN*/;   //NOTE: only supports 1 for now. there is a legacy ATAPI device that may have 2 luns.
    // set driver to run in full duplex mode
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;
    ConfigInfo->BusResetHoldTime = 0;       // StorAHCI wait RESET to be completed by itself, no need for port driver to wait.
    ConfigInfo->MaxNumberOfIO = portCount * adapterExtension->CAP.NCS;

    ConfigInfo->FeatureSupport |= STOR_ADAPTER_FEATURE_STOP_UNIT_DURING_POWER_DOWN;
    ConfigInfo->FeatureSupport |= STOR_ADAPTER_FEATURE_RICH_TEMPERATURE_THRESHOLD;
    ConfigInfo->FeatureSupport |= STOR_ADAPTER_FEATURE_DEVICE_TELEMETRY;
    ConfigInfo->FeatureSupport |= STOR_ADAPTER_DMA_V3_PREFERRED;

    //
    // Set interrupt related entry points.
    //
    if (ConfigInfo->InterruptMode == Latched) {
        //
        // Both MSI and MSI-X are Latched interrupt mode.
        //
        ConfigInfo->HwMSInterruptRoutine = AhciHwMSIInterrupt;
        ConfigInfo->InterruptSynchronizationMode = InterruptSynchronizePerMessage;

        //
        // Allocate MessageGroupAffinity buffer for later use.
        //
        if (!dumpMode) {
            StorPortAllocatePool(AdapterExtension,
                                sizeof(GROUP_AFFINITY) * (adapterExtension->HighestPort + 1),
                                AHCI_POOL_TAG,
                                (PVOID*)&adapterExtension->MessageGroupAffinity);
        }

    } else {
        ConfigInfo->InterruptSynchronizationMode = InterruptSynchronizeAll;
    }

    //3.5 Register Power Setting Change Notification Guids
    if (!dumpMode) {
        /* 0b2d69d7-a2a1-449c-9680-f91c70521c60 -DIPM/HIPM */
        powerSettingChangeGuids[0].Data1 = 0x0b2d69d7;
        powerSettingChangeGuids[0].Data2 = 0xa2a1;
        powerSettingChangeGuids[0].Data3 = 0x449c;
        powerSettingChangeGuids[0].Data4[0] = 0x96;
        powerSettingChangeGuids[0].Data4[1] = 0x80;
        powerSettingChangeGuids[0].Data4[2] = 0xf9;
        powerSettingChangeGuids[0].Data4[3] = 0x1c;
        powerSettingChangeGuids[0].Data4[4] = 0x70;
        powerSettingChangeGuids[0].Data4[5] = 0x52;
        powerSettingChangeGuids[0].Data4[6] = 0x1c;
        powerSettingChangeGuids[0].Data4[7] = 0x60;

        /* DAB60367-53FE-4fbc-825E-521D069D2456 -Adaptive */
        powerSettingChangeGuids[1].Data1 = 0xDAB60367;
        powerSettingChangeGuids[1].Data2 = 0x53FE;
        powerSettingChangeGuids[1].Data3 = 0x4fbc;
        powerSettingChangeGuids[1].Data4[0] = 0x82;
        powerSettingChangeGuids[1].Data4[1] = 0x5E;
        powerSettingChangeGuids[1].Data4[2] = 0x52;
        powerSettingChangeGuids[1].Data4[3] = 0x1D;
        powerSettingChangeGuids[1].Data4[4] = 0x06;
        powerSettingChangeGuids[1].Data4[5] = 0x9D;
        powerSettingChangeGuids[1].Data4[6] = 0x24;
        powerSettingChangeGuids[1].Data4[7] = 0x56;

        StorPortSetPowerSettingNotificationGuids(AdapterExtension, 2, powerSettingChangeGuids);
    }

    //4.1 Turn on IE, pending interrupts will be cleared when port starts
    adapterExtension->LastInterruptedPort = (ULONG)(-1);
    ghc.IE = 1;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC.AsUlong, ghc.AsUlong);

    //4.2.2 Configure DMA 64 bit support settings.
    if (adapterExtension->CAP.S64A) {
#if defined (_ARM64_) 
        ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_FULL64BIT_NO_BOUNDARY_REQ_SUPPORTED;
#else
        ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_SUPPORTED;
#endif

        // increase size of SrbExtension to accommodate 64-bit move commands
        // and 1 extra Scripts instruction (turn off 64-bit mode)
        ConfigInfo->SrbExtensionSize += (ULONG)((ConfigInfo->NumberOfPhysicalBreaks + 2) * 4);
    }

    //4.3 allocate resources, implemented port information needs to be ready before calling the following routine.
    if (adapterExtension->StateFlags.StoppedState == 0) {
        if (!AllocateResourcesForAdapter(adapterExtension, ConfigInfo, portCount)) {
            return SP_RETURN_ERROR;
        }
    }

    //
    // Initialize timers.
    //
    // Notes:
    //
    // 1. If we are in dump mode, do not initialize timers. Interrupt is disabled.
    //
    // 2. This initialization has to be done before trying to getting ports into running state (i.e. by
    //    calling AhciAdapterRunAllPorts() later below in this function). The port start process
    //    utilizes StartPortTimer (Note: In dump mode the port start process will be using polling).
    //
    // 3. Currently WorkerTimer is used only for PartialToSlumber during interrupt servicing (not applicable in dump mode)
    //
    if (!dumpMode) {

        for (i = 0; i <= adapterExtension->HighestPort; i++) {

            if (adapterExtension->PortExtension[i] != NULL) {

                if (adapterExtension->PortExtension[i]->StartPortTimer == NULL) {

                    storStatus = StorPortInitializeTimer(AdapterExtension, &adapterExtension->PortExtension[i]->StartPortTimer);

                    if (storStatus != STOR_STATUS_SUCCESS) {
                        NT_ASSERT(FALSE);
                        return SP_RETURN_ERROR;
                    }
                }

                if (adapterExtension->PortExtension[i]->WorkerTimer == NULL) {

                    storStatus = StorPortInitializeTimer(AdapterExtension, &adapterExtension->PortExtension[i]->WorkerTimer);

                    if (storStatus != STOR_STATUS_SUCCESS) {
                        NT_ASSERT(FALSE);
                        return SP_RETURN_ERROR;
                    }
                }

            }
        }
    }

    // reset "StoppedState". NOTE: This field should not be referenced anymore in this function after following line.
    adapterExtension->StateFlags.StoppedState = 0;

    //4.4 initialize ports and start them.
    //4.4.1 initialize all AHCI ports
    for (i = 0; i <= adapterExtension->HighestPort; i++) {
        if (adapterExtension->PortExtension[i] != NULL) {
            // in case of PortInitialize fails, ChannelExtension->StateFlags.Initialized will remain as 'FALSE'. there will be not attempt to start the Port.
            AhciPortInitialize(adapterExtension->PortExtension[i]);
        }
    }

    if (dumpMode) {

        //
        // In dump mode copy registry flags and telemetry configuration from the dump context. If telemetry extends to more than one
        // device the logic should cover migration of per device settings, initially we are limited to a single boot device
        //

        for (i = 0; i < dumpContext->PortCount; i++) {
            portNumber = dumpContext->Ports[i].PortNumber;

            if (adapterExtension->PortExtension[portNumber] != NULL) {

                adapterExtension->PortExtension[portNumber]->RegistryFlags = dumpContext->Ports[i].PortRegistryFlags;

                //
                // If the table in dump context is empty (not filled) - keep static data intact
                //
                /*if (dumpContext->PublicGPLogTableAddresses[0]) {
                    for(Index=0;
                        Index < sizeof(AhciPublicGPLogTableAddresses) / sizeof(AhciPublicGPLogTableAddresses[0] );
                        Index++) {
                        AhciPublicGPLogTableAddresses[Index] = dumpContext->PublicGPLogTableAddresses[Index];
                    }

                    AhciGPLogPageIntoPrivate = dumpContext->PrivateGPLogPageAddress;
                }*/

                StorPortMoveMemory((PVOID)&adapterExtension->PortExtension[portNumber]->DeviceExtension->HybridInfo,
                                   (PVOID)&dumpContext->Ports[i].HybridInfo,
                                   sizeof(GP_LOG_HYBRID_INFORMATION_HEADER));
            }
        }
    }

    return SP_RETURN_FOUND;
}

BOOLEAN
AhciHwInitialize (
    _In_ PVOID AdapterExtension
    )
{
    ULONG                       status = STOR_STATUS_SUCCESS;
    PERF_CONFIGURATION_DATA     perfConfigData = {0};
    PAHCI_ADAPTER_EXTENSION     adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    //
    // Get adapter current interrupt mode. Interrupt Messages can be accessed after HwFindAdapter.
    //
    GetInterruptMode(adapterExtension);

    StorPortEnablePassiveInitialization(AdapterExtension, AhciHwPassiveInitialize);

    //
    // Query perf optimization information
    //
    perfConfigData.Version = STOR_PERF_VERSION;
    perfConfigData.Size = sizeof(PERF_CONFIGURATION_DATA);
    
    status = StorPortInitializePerfOpts(AdapterExtension, TRUE, &perfConfigData);

    //
    // Turn on DPC Redirection if it's supported.
    //
    if ((status == STOR_STATUS_SUCCESS) &&
        ((perfConfigData.Flags & STOR_PERF_DPC_REDIRECTION) != 0)) {

        AhciZeroMemory((PCHAR)&perfConfigData, sizeof(PERF_CONFIGURATION_DATA));
        perfConfigData.Version = STOR_PERF_VERSION;
        perfConfigData.Size = sizeof(PERF_CONFIGURATION_DATA);

        perfConfigData.Flags = STOR_PERF_DPC_REDIRECTION;

        if (adapterExtension->StateFlags.InterruptMessagePerPort == 1) {
            //
            // Limit interrupt redirection to useful messages only.
            //
            if (adapterExtension->MessageGroupAffinity != NULL) {
                AhciZeroMemory((PCHAR)adapterExtension->MessageGroupAffinity, sizeof(GROUP_AFFINITY)* (adapterExtension->HighestPort + 1));

                perfConfigData.Flags |= (STOR_PERF_INTERRUPT_MESSAGE_RANGES | STOR_PERF_ADV_CONFIG_LOCALITY);

                if (adapterExtension->HighestPort > 0) {
                    perfConfigData.FirstRedirectionMessageNumber = 0;
                    perfConfigData.LastRedirectionMessageNumber = adapterExtension->HighestPort;
                }
                else {
                    perfConfigData.FirstRedirectionMessageNumber = 0;
                    perfConfigData.LastRedirectionMessageNumber = 0;
                }

                perfConfigData.MessageTargets = adapterExtension->MessageGroupAffinity;
            }

        }

        //
        // Support concurrent channel so that ports can have StartIo routine running concurrently.
        //
        perfConfigData.Flags |= STOR_PERF_CONCURRENT_CHANNELS;
        perfConfigData.ConcurrentChannels = NumberOfSetBits(adapterExtension->PortImplemented);

        status = StorPortInitializePerfOpts(AdapterExtension, FALSE, &perfConfigData);

        NT_ASSERT(status == STOR_STATUS_SUCCESS);
    }

    //
    // async process to get all ports into running state
    //
    AhciAdapterRunAllPorts(adapterExtension);

    return TRUE;
}

BOOLEAN
AhciHwPassiveInitialize (
    _In_ PVOID AdapterExtension
    )
{
    ULONG i;
    ULONG status = STOR_STATUS_SUCCESS;
    BOOLEAN enableD3Cold = FALSE;
    BOOLEAN d3ColdSupported = FALSE;
    BOOLEAN reportF1State = FALSE;
    ULONG bufferSize = sizeof(STOR_POFX_DEVICE_V2);

    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    // 1. initialize DPC for IO completion
    for (i = 0; i <= adapterExtension->HighestPort; i++) {
        if (adapterExtension->PortExtension[i] != NULL) {
            StorPortInitializeDpc(AdapterExtension, &adapterExtension->PortExtension[i]->CompletionDpc, AhciPortSrbCompletionDpcRoutine);
            StorPortInitializeDpc(AdapterExtension, &adapterExtension->PortExtension[i]->BusChangeDpc, AhciPortBusChangeDpcRoutine);
        }
    }

    // 2. check if ACPI supports turning off power on link
    AhciAdapterEvaluateDSMMethod(adapterExtension);

    //
    // Get and cache D3Cold support.  
    //
    status = StorPortGetD3ColdSupport(AdapterExtension,
                                        NULL,
                                        &d3ColdSupported);
    if (status == STOR_STATUS_SUCCESS) {
        adapterExtension->StateFlags.D3ColdSupported = d3ColdSupported;
    }
    
    if (reportF1State) {
        bufferSize += STOR_POFX_COMPONENT_IDLE_STATE_SIZE;
    }

    // 3. allocate STOR_POFX_DEVICE data structure for adapter, initialize the structure and register for runtime power management.
    status = StorPortAllocatePool(AdapterExtension,
                                  bufferSize,
                                  AHCI_POOL_TAG,
                                  (PVOID*)&adapterExtension->PoFxDevice);

    if (status != STOR_STATUS_SUCCESS) {
        goto Exit;
    }

    AhciZeroMemory((PCHAR)adapterExtension->PoFxDevice, sizeof(STOR_POFX_DEVICE_V2));

    adapterExtension->PoFxDevice->Version = STOR_POFX_DEVICE_VERSION_V2;
    adapterExtension->PoFxDevice->Size = STOR_POFX_DEVICE_V2_SIZE;
    adapterExtension->PoFxDevice->ComponentCount = 1;

    if (IsD3ColdAllowed(adapterExtension)) {
        adapterExtension->PoFxDevice->Flags = STOR_POFX_DEVICE_FLAG_ENABLE_D3_COLD;
    }

    // Indicate miniport opt-in adapter D3 wake support.
    adapterExtension->PoFxDevice->Flags |= STOR_POFX_DEVICE_FLAG_ADAPTER_D3_WAKE;
    
    // Indicate dump miniport can't bring adapter to active
    adapterExtension->PoFxDevice->Flags |= STOR_POFX_DEVICE_FLAG_NO_DUMP_ACTIVE;

    adapterExtension->PoFxDevice->Components[0].Version = STOR_POFX_COMPONENT_VERSION_V1;
    adapterExtension->PoFxDevice->Components[0].Size = STOR_POFX_COMPONENT_SIZE;
    adapterExtension->PoFxDevice->Components[0].FStateCount = reportF1State ? 2 : 1;
    adapterExtension->PoFxDevice->Components[0].DeepestWakeableFState = reportF1State ? 1 : 0;
    adapterExtension->PoFxDevice->Components[0].Id = STORPORT_POFX_ADAPTER_GUID;

    adapterExtension->PoFxDevice->Components[0].FStates[0].Version = STOR_POFX_COMPONENT_IDLE_STATE_VERSION_V1;
    adapterExtension->PoFxDevice->Components[0].FStates[0].Size = STOR_POFX_COMPONENT_IDLE_STATE_SIZE;
    adapterExtension->PoFxDevice->Components[0].FStates[0].TransitionLatency = 0;
    adapterExtension->PoFxDevice->Components[0].FStates[0].ResidencyRequirement = 0;
    adapterExtension->PoFxDevice->Components[0].FStates[0].NominalPower = STOR_POFX_UNKNOWN_POWER;

    if (reportF1State) {
        adapterExtension->StateFlags.UseAdapterF1InsteadOfD3 = FALSE;
        adapterExtension->PoFxDevice->Components[0].FStates[1].Version = STOR_POFX_COMPONENT_IDLE_STATE_VERSION_V1;
        adapterExtension->PoFxDevice->Components[0].FStates[1].Size = STOR_POFX_COMPONENT_IDLE_STATE_SIZE;
        adapterExtension->PoFxDevice->Components[0].FStates[1].TransitionLatency = 1;
        adapterExtension->PoFxDevice->Components[0].FStates[1].ResidencyRequirement = 0;
        adapterExtension->PoFxDevice->Components[0].FStates[1].NominalPower = STOR_POFX_UNKNOWN_POWER;
    }

    // registry runtime power management for Adapter
    status = StorPortInitializePoFxPower(AdapterExtension, NULL, (PSTOR_POFX_DEVICE)adapterExtension->PoFxDevice, &enableD3Cold);

    if (status != STOR_STATUS_SUCCESS) {
        StorPortFreePool(AdapterExtension, adapterExtension->PoFxDevice);
        adapterExtension->PoFxDevice = NULL;
        adapterExtension->StateFlags.PoFxEnabled = FALSE;
        adapterExtension->StateFlags.PoFxActive = FALSE;

        goto Exit;
    }

    // register success
    adapterExtension->StateFlags.PoFxEnabled = TRUE;
    adapterExtension->StateFlags.PoFxActive = TRUE;

Exit:
    return TRUE;
}

VOID
AhciAdapterPrepareForBusReScan(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
/*++

Routine Description:

    This function do preparation work before StorAHCI can process any device enumeration commands

Arguments:

    AdapterExtension - Pointer to the device extension for adapter.

Return Value:

    None

--*/
{
    // 1. If a port/device is powered off, power it on.
    if (AdapterExtension->StateFlags.SupportsAcpiDSM == 1) {
        // ACPI method is implemented, invoke ACPI method to power on all ports and devices
        AhciPortAcpiDSMControl(AdapterExtension, (ULONG)-1, FALSE);
    }

    return;
}

__inline
VOID
AdapterStop (
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
)
/*++

Routine Description:

    This function stops all the ports of the adapter.

Arguments:

    AdapterExtension - Pointer to the device extension for adapter.

    Note: StartIo spin lock must be held before this function is invoked.

Return Value:

    None

--*/
{
    ULONG   i;

    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if (AdapterExtension->PortExtension[i] != NULL) {
            AhciPortStop(AdapterExtension->PortExtension[i]);
        }
    }

    // clear ABAR
    AdapterExtension->ABAR_Address = 0;

    AdapterExtension->StateFlags.StoppedState = 1;

    AdapterReleaseActiveReference(AdapterExtension);

    // clear this bit indicating the work has been finished
    AdapterExtension->PoFxPendingWork.AdapterStop = 0;
}

VOID
AhciAdapterStop (
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
)
/*
    Adapter is being stopped.
*/
{
    // stop all ports.
    ULONG   adapterStopInProcess;
    BOOLEAN adapterIdle = FALSE;

    if (AdapterExtension->StateFlags.StoppedState == 1) {
        // nothing to do if the adapter is already stopped.
        return;
    }

    adapterStopInProcess = InterlockedBitTestAndSet((LONG*)&AdapterExtension->PoFxPendingWork, 0);  //AdapterStop is at bit 0

    if (adapterStopInProcess == 1) {
        // adapter Stop is pending in another process.
        return;
    }

    AdapterAcquireActiveReference(AdapterExtension, &adapterIdle);

    if (!adapterIdle) {
        ULONG   adapterStopPending;
        adapterStopPending = InterlockedBitTestAndReset((LONG*)&AdapterExtension->PoFxPendingWork, 0);  //AdapterStop is at bit 0

        // adapter was in active state, perform AdapterStop.
        // if adapter was in idle state, this work will be done in PoFxActive callback
        if (adapterStopPending == 1) {
            // Reference will be released in AdapterStop() function
            AdapterStop(AdapterExtension);
        }
    }

    return;
}

VOID
AhciAdapterRemoval (
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
)
/*
    Release resources allocated for the adapter and its managed ports/devices
*/
{
    ULONG i;
    PVOID bufferToFree = NULL;
    PVOID executionHistoryToFree = NULL;

    if (IsDumpMode(AdapterExtension)) {
        return;
    }

    if (AdapterExtension->PoFxDevice != NULL) {
        StorPortFreePool(AdapterExtension, AdapterExtension->PoFxDevice);
        AdapterExtension->PoFxDevice = NULL;
        AdapterExtension->StateFlags.PoFxEnabled = FALSE;
        AdapterExtension->StateFlags.PoFxActive = FALSE;
    }

    if (AdapterExtension->MessageGroupAffinity != NULL) {
        StorPortFreePool(AdapterExtension, AdapterExtension->MessageGroupAffinity);
        AdapterExtension->MessageGroupAffinity = NULL;
    }

    // release portsChannelExtension. the whole buffer is sliced for Ports, only need to free the first ChannelExtension.
    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if (AdapterExtension->PortExtension[i] != NULL) {
            if (bufferToFree == NULL) {
                bufferToFree = AdapterExtension->PortExtension[i];
            }

            if (AdapterExtension->PortExtension[i]->StartPortTimer != NULL) {
                StorPortFreeTimer(AdapterExtension, AdapterExtension->PortExtension[i]->StartPortTimer);
                AdapterExtension->PortExtension[i]->StartPortTimer = NULL;
            }
            if (AdapterExtension->PortExtension[i]->WorkerTimer != NULL) {
                StorPortFreeTimer(AdapterExtension, AdapterExtension->PortExtension[i]->WorkerTimer);
                AdapterExtension->PortExtension[i]->WorkerTimer = NULL;
            }
            if (AdapterExtension->PortExtension[i]->BusChangeTimer != NULL) {
                StorPortFreeTimer(AdapterExtension, AdapterExtension->PortExtension[i]->BusChangeTimer);
                AdapterExtension->PortExtension[i]->BusChangeTimer = NULL;
            }

            if (AdapterExtension->PortExtension[i]->DeviceInitCommands.CommandTaskFile != NULL) {
                StorPortFreePool(AdapterExtension, (PVOID)AdapterExtension->PortExtension[i]->DeviceInitCommands.CommandTaskFile);
                AdapterExtension->PortExtension[i]->DeviceInitCommands.CommandTaskFile = NULL;
            }

            if (AdapterExtension->PortExtension[i]->PoFxDevice != NULL) {
                StorPortFreePool(AdapterExtension, AdapterExtension->PortExtension[i]->PoFxDevice);
                AdapterExtension->PortExtension[i]->PoFxDevice = NULL;
                AdapterExtension->PortExtension[i]->StateFlags.PoFxEnabled = FALSE;
                AdapterExtension->PortExtension[i]->StateFlags.PoFxActive = FALSE;
            }

            if ((executionHistoryToFree == NULL) && (AdapterExtension->PortExtension[i]->ExecutionHistory != NULL)) {
                executionHistoryToFree = AdapterExtension->PortExtension[i]->ExecutionHistory;
            }

            if (AdapterExtension->PortExtension[i]->ExecutionHistory != NULL) {
                AdapterExtension->PortExtension[i]->ExecutionHistory = NULL;
            }

            AdapterExtension->PortExtension[i] = NULL;
        }
    }

    if (bufferToFree != NULL) {
        StorPortFreePool(AdapterExtension, bufferToFree);
    }

    if (executionHistoryToFree != NULL) {
        StorPortFreePool(AdapterExtension, executionHistoryToFree);
    }

    return;
}


SCSI_ADAPTER_CONTROL_STATUS
AhciHwAdapterControl (
    _In_ PVOID AdapterExtension,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters
    )
/*++

Routine Description:

    This is a generic routine to allow for special adapter control activities.
    It mainly handles power change notifications for the adapter.

Arguments:

    AdapterExtension - Pointer to the device extension for adapter.

    ControlType - Specifies the type of call being made through this routine.

    Parameters - Pointer to parameters needed for this control type (optional).

Return Value:

    SCSI_ADAPTER_CONTROL_STATUS - currently either:
        ScsiAdapterControlSuccess (= 0)
        ScsiAdapterControlUnsuccessful (= 1)

--*/

{
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST controlTypeList;
    SCSI_ADAPTER_CONTROL_STATUS status = ScsiAdapterControlSuccess;

    switch (ControlType) {
        // determine which control types (routines) are supported
        case ScsiQuerySupportedControlTypes:
            // get pointer to control type list
            controlTypeList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;

            // Report StopAdapter and RestartAdapter are supported.
            if (ScsiQuerySupportedControlTypes < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiQuerySupportedControlTypes] = TRUE;
            }
            if (ScsiStopAdapter < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiStopAdapter] = TRUE;
            }
            if (ScsiRestartAdapter < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiRestartAdapter] = TRUE;
            }
            if (ScsiPowerSettingNotification < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiPowerSettingNotification] = TRUE;
            }
            if (ScsiAdapterPoFxPowerActive < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiAdapterPoFxPowerActive] = TRUE;
            }
            if (ScsiAdapterPoFxPowerSetFState < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiAdapterPoFxPowerSetFState] = TRUE;
            }
            if (ScsiAdapterPower < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiAdapterPower] = TRUE;
            }
            if (ScsiAdapterPrepareForBusReScan < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiAdapterPrepareForBusReScan] = TRUE;
            }
            if (ScsiAdapterSystemPowerHints < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiAdapterSystemPowerHints] = TRUE;
            }
            if (ScsiAdapterSurpriseRemoval < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiAdapterSurpriseRemoval] = TRUE;
            }

            break;

        // ScsiStopAdapter maybe called with PnP or Power activities.
        // StorAHCI supports PnP Srb and ScsiAdapterPower thus does nothing for ScsiStopAdapter.
        // NOTE: ScsiStopAdapter is called after PnP call -StorRemoveDevice or StorSurpriseRemoval,
        // thus resource release should be done here.
        case ScsiStopAdapter:
            if (adapterExtension->StateFlags.Removed == 1) {
                AhciAdapterRemoval(adapterExtension);
            }
            break;

        // ScsiRestartAdapter is called during adapter powering up.
        // StorAHCI supports PnP Srb and ScsiAdapterPower thus does nothing for ScsiRestartAdapter. return value is ScsiAdapterControlSuccess.
        case ScsiRestartAdapter:
            break;

        // ScsiPowerSettingNotification is called when Storport receives notification from Power Manager when power setting is changed.
        case ScsiPowerSettingNotification:
            AhciAdapterPowerSettingNotification(adapterExtension, (PSTOR_POWER_SETTING_INFO)Parameters);
            break;

        case ScsiAdapterPoFxPowerActive: {
            PSTOR_POFX_ACTIVE_CONTEXT activeContext = (PSTOR_POFX_ACTIVE_CONTEXT)Parameters;

            if (AdapterPoFxEnabled(adapterExtension)) {
                adapterExtension->StateFlags.PoFxActive = activeContext->Active ? 1 : 0;

                if (activeContext->Active) {
                    ULONG   adapterStopPending;
                    adapterStopPending = InterlockedBitTestAndReset((LONG*)&adapterExtension->PoFxPendingWork, 0);  //AdapterStop is at bit 0

                    if (adapterStopPending == 1) {
                        //perform pending Adapter Stop work
                        STOR_LOCK_HANDLE lockhandle = {StartIoLock, 0};

                        NT_ASSERT(adapterExtension->StateFlags.StoppedState == 0);

                        // Storport does not acquire any spinlock before calling 
                        // miniport's HwAdapterControl routine. Need to acquire
                        // StartIo spin lock before calling AdapterStop as this is 
                        // required by AdapterStop.
                        StorPortAcquireSpinLock(AdapterExtension, StartIoLock, NULL, &lockhandle);
                        AdapterStop(adapterExtension);
                        StorPortReleaseSpinLock(AdapterExtension, &lockhandle);
                    }
                }
            }
            break;
        }

        case ScsiAdapterPoFxPowerSetFState: {
            break;
        }

        case ScsiAdapterPower: {
            PSTOR_ADAPTER_CONTROL_POWER adapterPower = (PSTOR_ADAPTER_CONTROL_POWER)Parameters;

            NT_ASSERT(adapterPower != NULL);

            StorPortDebugPrint(3, "StorAHCI - LPM: Adapter SystemIoBusNumber:%d - %s\n", 
                adapterExtension->SystemIoBusNumber, adapterPower->PowerState == StorPowerDeviceD0 ? "D0" : "D3");

            if (adapterPower->PowerState == StorPowerDeviceD0) {
                AhciAdapterPowerUp(adapterExtension);                  //power up
            } else if (adapterPower->PowerState == StorPowerDeviceD3) {
                AhciAdapterPowerDown(adapterExtension);                //power down
            } else {
                // AHCI adapter does not support D1 or D2, assert but do not fail the call
                NT_ASSERT(FALSE);
            }
            break;
        }

        case ScsiAdapterPrepareForBusReScan:
            AhciAdapterPrepareForBusReScan(adapterExtension);
            break;

        case ScsiAdapterSystemPowerHints: {
            PSTOR_SYSTEM_POWER_HINTS powerHints = (PSTOR_SYSTEM_POWER_HINTS)Parameters;            

            if (powerHints->Size >= sizeof(STOR_SYSTEM_POWER_HINTS)) {

                StorPortDebugPrint(3, "StorAHCI - LPM: Adapter SystemIoBusNumber:%d - System Power Hint - State: %u - Latency: %u ms\n", 
                    adapterExtension->SystemIoBusNumber, powerHints->SystemPower, powerHints->ResumeLatencyMSec);

                adapterExtension->SystemPowerHintState = powerHints->SystemPower;
                adapterExtension->SystemPowerResumeLatencyMSec = powerHints->ResumeLatencyMSec;

                if (adapterExtension->TracingEnabled) {
                    StorPortEtwEvent2(AdapterExtension,
                                      NULL,
                                      AhciEtwEventSystemPowerHint,
                                      L"Sys Power Hint",
                                      STORPORT_ETW_EVENT_KEYWORD_POWER,
                                      StorportEtwLevelInformational,
                                      StorportEtwEventOpcodeInfo,
                                      NULL,
                                      L"State",
                                      powerHints->SystemPower,
                                      L"Latency (ms)",
                                      powerHints->ResumeLatencyMSec);
                }

            } else {
                status = ScsiAdapterControlUnsuccessful;
            }
            break;
        }

        case ScsiAdapterSurpriseRemoval: {
            //
            // For AHCI, all the cleanup that needs to happen for adapter surprise removals is done in
            // the several ScsiUnitSurpriseRemovals that will be sent down for each LUN. There is no
            // extra work needed to handle adapter surprise removals.
            //
            status = ScsiAdapterControlSuccess;
            break;
        }

        default:
            status = ScsiAdapterControlUnsuccessful;
            break;

    } // end of switch

    return status;

} // AhciHwAdapterControl

BOOLEAN
AhciHwResetBus (
    _In_ PVOID AdapterExtension,
    _In_ ULONG PathId
    )
/*
    Used to Reset the PathId/Port/Channel
*/
{
    BOOLEAN status = FALSE;
    STOR_LOCK_HANDLE lockhandle = {InterruptLock, 0};
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    if ( IsPortValid(adapterExtension, PathId) ) {
        AhciInterruptSpinlockAcquire(adapterExtension, MAXULONG, &lockhandle);
        status = AhciPortReset(adapterExtension->PortExtension[PathId], TRUE);
        AhciInterruptSpinlockRelease(adapterExtension, MAXULONG, &lockhandle);
        adapterExtension->PortExtension[PathId]->DeviceExtension[0].IoRecord.PortDriverResetCount++;

        AhciTelemetryLogResetErrorRecovery(adapterExtension->PortExtension[PathId],
                                          (PSTOR_ADDRESS)&(adapterExtension->PortExtension[PathId]->DeviceExtension->DeviceAddress),
                                          AhciTelemetryEventIdResetBus,
                                          "AhciHwResetBus",
                                          0,
                                          "PortResetStatus",
                                          status
                                          );
    }

    return status;
}

BOOLEAN
AhciHwBuildIo (
    _In_ PVOID AdapterExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
{
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    PAHCI_CHANNEL_EXTENSION channelExtension = NULL;
    UCHAR                   pathId = SrbGetPathId(Srb);
    ULONG                   function = SrbGetSrbFunction(Srb);
    ULONG                   srbFlags = SrbGetSrbFlags(Srb);
    PAHCI_SRB_EXTENSION     srbExtension = GetSrbExtension((PSTORAGE_REQUEST_BLOCK)Srb);

    //
    // Make sure the incoming Srb with the expected type
    //
    NT_ASSERT(Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK);

    //
    // SrbStatus value should have been set to pending.
    //
    NT_ASSERT(Srb->SrbStatus == SRB_STATUS_PENDING);

    // SrbExtension is not Null-ed by Storport, so do it here.
    // Zero PRDT later when elements number in the SGL is available.
    AhciZeroMemory((PCHAR)(&(srbExtension->AtaFunction)), (sizeof(AHCI_SRB_EXTENSION) - FIELD_OFFSET(AHCI_SRB_EXTENSION, AtaFunction)));
    AhciZeroMemory((PCHAR)srbExtension, FIELD_OFFSET(AHCI_COMMAND_TABLE, PRDT));

    channelExtension = adapterExtension->PortExtension[pathId];

    if ( IsPortValid(adapterExtension, pathId) &&
         (function == SRB_FUNCTION_EXECUTE_SCSI) &&
         (channelExtension->StateFlags.PowerDown == TRUE)) {
        AhciPortPowerUp(channelExtension);
    }

    //
    // Prepare the Device Command before StartIo routine for SRB_FUNCTION_IO_CONTROL and SRB_FUNCTION_EXECUTE_SCSI requests.
    // This can reduce the lock contention between StartIo and Interrupt locks.
    //
    if ((function == SRB_FUNCTION_EXECUTE_SCSI) ||
        ((function == SRB_FUNCTION_IO_CONTROL) && ((srbFlags & SRB_IOCTL_FLAGS_ADAPTER_REQUEST) == 0))) {

        if ( !IsPortValid(adapterExtension, pathId) ) {
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            StorPortEtwChannelEvent2(adapterExtension,
                                     NULL,
                                     StorportEtwEventOperational,
                                     AhciEtwEventBuildIO,
                                     L"No device",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     StorportEtwLevelError,
                                     StorportEtwEventOpcodeInfo,
                                     Srb,
                                     L"function",
                                     function,
                                     L"srbFlags",
                                     srbFlags);
            goto exit;
        }

        //
        // If the adapter was removed, fail this request.
        // We first check to see if the adapter is removable so that we can
        // avoid incurring a register access if it is not removable.
        //
        if (IsAdapterRemovable(adapterExtension) &&
            IsAdapterRemoved(adapterExtension)) {
            Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            StorPortEtwChannelEvent2(adapterExtension,
                                     (channelExtension != NULL) ? ((PSTOR_ADDRESS) &(channelExtension->DeviceExtension->DeviceAddress)) : (NULL),
                                     StorportEtwEventOperational,
                                     AhciEtwEventBuildIO,
                                     L"Adapter removed during BuildIo",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     StorportEtwLevelError,
                                     StorportEtwEventOpcodeInfo,
                                     Srb,
                                     L"function",
                                     function,
                                     L"srbFlags",
                                     srbFlags);
            goto exit;
        }
    }

    switch (function) {
        //
        // Get the flag from Storport that tells us if the adapter is removable or not.
        //
        case SRB_FUNCTION_PNP: {
            PSRBEX_DATA_PNP pnpData = (PSRBEX_DATA_PNP)SrbGetSrbExDataByType((PSTORAGE_REQUEST_BLOCK)Srb, SrbExDataTypePnP);
            if (((pnpData->SrbPnPFlags & SRB_PNP_FLAGS_ADAPTER_REQUEST)) &&
                (pnpData->PnPAction == StorQueryCapabilities) &&
                (SrbGetDataTransferLength(Srb) >= sizeof(STOR_DEVICE_CAPABILITIES_EX))) {
                PSTOR_DEVICE_CAPABILITIES_EX storCapabilities = (PSTOR_DEVICE_CAPABILITIES_EX)SrbGetDataBuffer(Srb);
                adapterExtension->StateFlags.Removable = storCapabilities->Removable;
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
            }
            break;
        }

        case SRB_FUNCTION_IO_CONTROL: {

            if ((srbFlags & SRB_IOCTL_FLAGS_ADAPTER_REQUEST) == 0) {

                IOCTLtoATA(adapterExtension->PortExtension[pathId], (PSTORAGE_REQUEST_BLOCK)Srb);

                if (srbExtension->AtaFunction != 0) {
                    if ( ( srbExtension->Sgl == NULL ) && ( IsDataTransferNeeded((PSTORAGE_REQUEST_BLOCK)Srb) )  ) {
                        srbExtension->Sgl = (PLOCAL_SCATTER_GATHER_LIST)StorPortGetScatterGatherList(adapterExtension, Srb);
                    }
                    if (srbExtension->Sgl != NULL) {
                        // Zero PRDT according to SGL elements number to avoid unnecessary CPU usage.
                        AhciZeroMemory((PCHAR)(srbExtension->CommandTable.PRDT), (srbExtension->Sgl->NumberOfElements * sizeof(AHCI_PRDT)));
                    }
                }
            }

            break;
        }

        case SRB_FUNCTION_EXECUTE_SCSI: {

            SCSItoATA(adapterExtension->PortExtension[pathId], (PSTORAGE_REQUEST_BLOCK)Srb);

            if (srbExtension->AtaFunction != 0) {
                if ( ( srbExtension->Sgl == NULL ) && ( IsDataTransferNeeded((PSTORAGE_REQUEST_BLOCK)Srb) )  ) {
                    srbExtension->Sgl = (PLOCAL_SCATTER_GATHER_LIST)StorPortGetScatterGatherList(adapterExtension, Srb);
                }
                if (srbExtension->Sgl != NULL) {
                    // Zero PRDT according to SGL elements number to avoid unnecessary CPU usage.
                    AhciZeroMemory((PCHAR)(srbExtension->CommandTable.PRDT), (srbExtension->Sgl->NumberOfElements * sizeof(AHCI_PRDT)));
                }
            }

            break;
        }

        default: {
            //
            // All other requests will be processed in StartIo routine.
            //
            break;
        }
    }

exit:

    if (Srb->SrbStatus != SRB_STATUS_PENDING) {
        //
        // This routine should return FALSE when SRB is completed.
        //
        StorPortNotification(RequestComplete, AdapterExtension, Srb);

        return FALSE;
    } else {
        //
        // This routine should return TRUE when SRB is not completed.
        //
        return TRUE;
    }
}

BOOLEAN
AhciHwStartIo (
    _In_ PVOID AdapterExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
/*
    1. Process Adapter request
    2. Bail out if it's adapter request
    3. Validate Port Number, if not valid, bail out.
    4. Process Device/Port request

*/
{
    STOR_LOCK_HANDLE lockhandle = {InterruptLock, 0};
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    ULONG function = SrbGetSrbFunction(Srb);
    UCHAR pathId = SrbGetPathId(Srb);
    BOOLEAN adapterRequest = FALSE;
    BOOLEAN processIO = FALSE;

    //1 Work on Adapter requests
    switch (function) {
        case SRB_FUNCTION_PNP: {
            PSRBEX_DATA_PNP pnpData = (PSRBEX_DATA_PNP)SrbGetSrbExDataByType((PSTORAGE_REQUEST_BLOCK)Srb, SrbExDataTypePnP);

            NT_ASSERT(pnpData != NULL);

            if ( (pnpData->SrbPnPFlags & SRB_PNP_FLAGS_ADAPTER_REQUEST ) != 0 ) {
                // only process PnP request for adapter here.
                adapterRequest = TRUE;

                if ( (pnpData->PnPAction == StorRemoveDevice) || (pnpData->PnPAction == StorSurpriseRemoval) ) {
                    // the adapter is going to be removed, mark the state and release all resources allocated later in AdapterControl - ScsiStopAdapter.
                    adapterExtension->StateFlags.Removed = 1;
                    Srb->SrbStatus = SRB_STATUS_SUCCESS;
                } else if (pnpData->PnPAction == StorStopDevice) {
                    AhciAdapterStop(adapterExtension);
                    Srb->SrbStatus = SRB_STATUS_SUCCESS;
                } else {
                    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                }

                StorPortEtwChannelEvent2(adapterExtension,
                                         NULL,
                                         (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwEventDiagnostic : StorportEtwEventOperational,
                                         AhciEtwEventStartIO,
                                         L"Processing PNP",
                                         STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                         (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwLevelInformational : StorportEtwLevelError,
                                         StorportEtwEventOpcodeInfo,
                                         Srb,
                                         L"PnPAction",
                                         pnpData->PnPAction,
                                         L"SrbStatus",
                                         Srb->SrbStatus);

                //complete all Adapter PnP request
                StorPortNotification(RequestComplete, AdapterExtension, Srb);
            }
            break;
        }
        case SRB_FUNCTION_POWER: {
            PSRBEX_DATA_POWER powerData = (PSRBEX_DATA_POWER)SrbGetSrbExDataByType((PSTORAGE_REQUEST_BLOCK)Srb, SrbExDataTypePower);

            NT_ASSERT(powerData != NULL);

            if ((powerData->SrbPowerFlags & SRB_POWER_FLAGS_ADAPTER_REQUEST) != 0) {
                // only process Power request for adapter here.
                adapterRequest = TRUE;

                // StorAHCI supports ScsiAdapterPower, thus Storport should not send adapter power Srb to this miniport.
                // Complete the request as SUCCESS anyway.
                NT_ASSERT(FALSE);
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                StorPortNotification(RequestComplete, AdapterExtension, Srb);
            }
            break;
        }

        case SRB_FUNCTION_FLUSH:  {
            adapterRequest = TRUE;
            //FLUSH Adapter cache. No cache, complete the request.
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            StorPortNotification(RequestComplete, AdapterExtension, Srb);
            break;
        }

        case SRB_FUNCTION_IO_CONTROL: {
            ULONG   srbFlags = SrbGetSrbFlags(Srb);

            if ((srbFlags & SRB_IOCTL_FLAGS_ADAPTER_REQUEST) != 0) {
                adapterRequest = AdapterProcessIOCTL(adapterExtension, (PSTORAGE_REQUEST_BLOCK)Srb);

                if (adapterRequest) {
                    StorPortNotification(RequestComplete, AdapterExtension, Srb);
                }
            }

            break;
        }
    }

    // 1.1 if the request is for adapter, it has been processed. Return from here.
    if (adapterRequest) {
        return TRUE;
    }

    // 2.1 All requests reach here should be for devices
    if ( !IsPortValid(adapterExtension, pathId) ) {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;

        StorPortEtwChannelEvent8(adapterExtension,
                                 NULL,
                                 StorportEtwEventOperational,
                                 AhciEtwEventStartIO,
                                 L"Port has no device",
                                 STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                 StorportEtwLevelError,
                                 StorportEtwEventOpcodeInfo,
                                 Srb,
                                 L"AdapterNumber",
                                 adapterExtension->AdapterNumber,
                                 L"SystemIoBusNumber",
                                 adapterExtension->SystemIoBusNumber,
                                 L"SlotNumber",
                                 adapterExtension->SlotNumber,
                                 L"AhciBaseAddress",
                                 adapterExtension->AhciBaseAddress,
                                 L"PathId",
                                 pathId,
                                 L"Function",
                                 Srb->Function,
                                 L"SrbStatus",
                                 Srb->SrbStatus,
                                 NULL,
                                 0);

        StorPortNotification(RequestComplete, AdapterExtension, Srb);
        return TRUE;
    }

    // 2.2 work on device requests
    // Get channelExtension and storAddress to use for ETW Event logging
    PAHCI_CHANNEL_EXTENSION channelExtension = adapterExtension->PortExtension[pathId];
    PSTOR_ADDRESS storAddress = (PSTOR_ADDRESS)&(channelExtension->DeviceExtension->DeviceAddress);

    switch (function) {
        case SRB_FUNCTION_RESET_BUS:    // this one may come from class driver, not port driver. same as AhciHwResetBus
        case SRB_FUNCTION_RESET_DEVICE:
        case SRB_FUNCTION_RESET_LOGICAL_UNIT: {
            
            // these reset requests target to Port
            AhciInterruptSpinlockAcquire(adapterExtension, pathId, &lockhandle);
            Srb->SrbStatus = AhciPortReset(adapterExtension->PortExtension[pathId], TRUE) ? SRB_STATUS_SUCCESS : SRB_STATUS_ERROR;
            StorPortNotification(RequestComplete, AdapterExtension, Srb);
            AhciInterruptSpinlockRelease(adapterExtension, pathId, &lockhandle);
            adapterExtension->PortExtension[pathId]->DeviceExtension[0].IoRecord.PortDriverResetCount++;

            AhciTelemetryLogResetErrorRecovery(adapterExtension->PortExtension[pathId],
                                              (PSTOR_ADDRESS)&(adapterExtension->PortExtension[pathId]->DeviceExtension->DeviceAddress),
                                              AhciTelemetryEventIdResetDeviceRequest,
                                              "AhciHwStartIo RESET Request",
                                              0,
                                              "PortResetStatus",
                                              Srb->SrbStatus
                                              );
            break;
        }

        case SRB_FUNCTION_PNP: {
            PSRBEX_DATA_PNP pnpData = (PSRBEX_DATA_PNP)SrbGetSrbExDataByType((PSTORAGE_REQUEST_BLOCK)Srb, SrbExDataTypePnP);

            NT_ASSERT(pnpData != NULL);

            if ( ((pnpData->SrbPnPFlags & SRB_PNP_FLAGS_ADAPTER_REQUEST) == 0) &&
                 (pnpData->PnPAction == StorQueryCapabilities) &&
                 (SrbGetDataTransferLength(Srb) >= sizeof(STOR_DEVICE_CAPABILITIES_EX)) ) {
                // process StorQueryCapabilities request for device, not adapter.
                // fill in fields of STOR_DEVICE_CAPABILITIES_EX
                PSTOR_DEVICE_CAPABILITIES_EX storCapabilities = (PSTOR_DEVICE_CAPABILITIES_EX)SrbGetDataBuffer(Srb);
                ULONG portProperties = adapterExtension->PortExtension[pathId]->PortProperties;

                storCapabilities->Removable = (portProperties & PORT_PROPERTIES_EXTERNAL_PORT) ? 1 : 0;
                storCapabilities->DeviceD1 = 0;
                storCapabilities->DeviceD2 = 0;
                storCapabilities->LockSupported = 0;
                storCapabilities->EjectSupported = 0;
                storCapabilities->DockDevice = 0;
                storCapabilities->UniqueID = 0;         //no uniqueID, let PnP generates one
                storCapabilities->SilentInstall = 1;
                storCapabilities->SurpriseRemovalOK = 0;
                storCapabilities->NoDisplayInUI = 0;

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                StorPortNotification(RequestComplete, AdapterExtension, Srb);

            } else {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                StorPortNotification(RequestComplete, AdapterExtension, Srb);
            }

            StorPortEtwChannelEvent2(adapterExtension,
                                     storAddress,
                                     (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwEventDiagnostic : StorportEtwEventOperational,
                                     AhciEtwEventStartIO,
                                     L"Completed PnP request",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwLevelInformational : StorportEtwLevelError,
                                     StorportEtwEventOpcodeInfo,
                                     Srb,
                                     L"SrbPnPFlags",
                                     pnpData->SrbPnPFlags,
                                     L"PnPAction",
                                     pnpData->PnPAction);

            break;
        }

        case SRB_FUNCTION_SHUTDOWN: {
            //This request is for the device to flush device data from adapter cache.
            //The miniport driver must hold on to the shutdown request until no data remains in the HBA's internal cache for the target logical unit and,
            //then, complete the shutdown request.
            PATA_DEVICE_PARAMETERS  deviceParameters = &adapterExtension->PortExtension[pathId]->DeviceExtension->DeviceParameters;
            BOOLEAN                 sendStandby = (IsDumpHiberMode(adapterExtension) || IsDumpCrashMode(adapterExtension));

            deviceParameters->StateFlags.SystemPoweringDown = TRUE;

            USHORT *eventDescription;
            if (adapterExtension->PortExtension[pathId]->DeviceExtension->SupportedCommands.SetDateAndTime == 0x1) {
                IssueSetDateAndTimeCommand(adapterExtension->PortExtension[pathId], Srb, sendStandby);
                processIO = TRUE;
                eventDescription = L"Shutdown set date and time";
            } else if (sendStandby) {
                //in dump mode, this is the last Srb sent after SYNC CACHE, spin down the disk
                PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension((PSTORAGE_REQUEST_BLOCK)Srb);
                srbExtension->AtaFunction = ATA_FUNCTION_ATA_COMMAND;
                SetCommandReg((&srbExtension->TaskFile.Current), IDE_COMMAND_STANDBY_IMMEDIATE);
                processIO = TRUE;
                eventDescription = L"Shutdown standby immediate";
            } else {
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                eventDescription = L"Shutdown";
                StorPortNotification(RequestComplete, AdapterExtension, Srb);
            }

            StorPortEtwEvent2(adapterExtension,
                              storAddress,
                              AhciEtwEventStartIO,
                              eventDescription,
                              STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                              StorportEtwLevelInformational,
                              StorportEtwEventOpcodeInfo,
                              Srb,
                              NULL,
                              0,
                              NULL,
                              0);

            break;
        }

        case SRB_FUNCTION_IO_CONTROL:
        case SRB_FUNCTION_EXECUTE_SCSI: {
                processIO = TRUE;
                break;
            }

        case SRB_FUNCTION_DUMP_POINTERS: {
            ULONG status = STOR_STATUS_SUCCESS;
            ULONG size = 0;
            PAHCI_DUMP_CONTEXT dumpContext = NULL;
            PMINIPORT_DUMP_POINTERS dumpPointers = (PMINIPORT_DUMP_POINTERS)SrbGetDataBuffer(Srb);

            USHORT *eventDescription = L"Completed dump pointers";

            if ( (dumpPointers == NULL) ||
                 (SrbGetDataTransferLength(Srb) < RTL_SIZEOF_THROUGH_FIELD(MINIPORT_DUMP_POINTERS, MiniportPrivateDumpData)) ) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                eventDescription = L"Invalid dump pointers request";
            } else {

                size = sizeof(AHCI_DUMP_CONTEXT);

                // allocate pool and zero the content
                status = StorPortAllocatePool(AdapterExtension,
                                              size,
                                              AHCI_POOL_TAG,
                                              (PVOID*)&dumpContext);

                if ((status != STOR_STATUS_SUCCESS) || (dumpContext == NULL)) {
                   // we cannot continue if cannot get memory for ChannelExtension.
                    Srb->SrbStatus = SRB_STATUS_ERROR;
                    eventDescription = L"Allocate dump context failed";
                } else {
                    dumpPointers->Version = DUMP_MINIPORT_VERSION_1;
                    dumpPointers->Size = sizeof(MINIPORT_DUMP_POINTERS);

                    AhciZeroMemory((PCHAR)dumpContext, size);

                    dumpContext->VendorID = adapterExtension->VendorID;
                    dumpContext->DeviceID = adapterExtension->DeviceID;
                    dumpContext->RevisionID = adapterExtension->RevisionID;
                    dumpContext->AhciBaseAddress = adapterExtension->AhciBaseAddress;
                    dumpContext->LogFlags = adapterExtension->LogFlags;
                    dumpContext->AdapterRegistryFlags = adapterExtension->RegistryFlags;

                    dumpContext->PortCount = 1;

                    dumpContext->Ports[0].PortNumber = adapterExtension->PortExtension[pathId]->PortNumber;
                    dumpContext->Ports[0].PortRegistryFlags = adapterExtension->PortExtension[pathId]->RegistryFlags;

                    //
                    // Preserve Hybrid Disk Information.
                    //
                    if (IsDeviceHybridInfoSupported(adapterExtension->PortExtension[pathId])) {
                        StorPortMoveMemory((PVOID)&dumpContext->Ports[0].HybridInfo,
                                           (PVOID)&adapterExtension->PortExtension[pathId]->DeviceExtension->HybridInfo,
                                           sizeof(GP_LOG_HYBRID_INFORMATION_HEADER));
                    }

                    //
                    // Fill telemetry collection context - shared with diskdump.sys stack
                    //
                    //dumpContext->MaxDumpLevelEnabled = 1;
                    //dumpContext->MaxDeviceDumpSize = 100;

                    AhciZeroMemory((PCHAR)dumpContext->PublicGPLogTableAddresses,sizeof(dumpContext->PublicGPLogTableAddresses));
                    dumpContext->PrivateGPLogPageAddress = 0;

                    //
                    // Override from the registry settings if passed by the storport miniport
                    // If private address is not set we will use T13 defined one . Consider always trying default one and resort to registry if it is not supported
                    //
                    if (dumpContext->PrivateGPLogPageAddress == 0) {
                        dumpContext->PrivateGPLogPageAddress = IDE_GP_LOG_CURRENT_DEVICE_INTERNAL_STATUS;
                    }

                    dumpPointers->MiniportPrivateDumpData = (PVOID)dumpContext;

                    Srb->SrbStatus = SRB_STATUS_SUCCESS;
                }
            }

            StorPortEtwChannelEvent2(adapterExtension,
                                     storAddress,
                                     (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwEventDiagnostic : StorportEtwEventOperational,
                                     AhciEtwEventStartIO,
                                     eventDescription,
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwLevelInformational : StorportEtwLevelError,
                                     StorportEtwEventOpcodeInfo,
                                     Srb,
                                     L"SrbStatus",
                                     Srb->SrbStatus,
                                     L"TransferLen",
                                     SrbGetDataTransferLength(Srb));

            StorPortNotification(RequestComplete, AdapterExtension, Srb);
            break;
        }

        case SRB_FUNCTION_FREE_DUMP_POINTERS: {
            ULONG status = STOR_STATUS_SUCCESS;
            PMINIPORT_DUMP_POINTERS dumpPointers = (PMINIPORT_DUMP_POINTERS)SrbGetDataBuffer(Srb);

            if ( (dumpPointers == NULL) ||
                 (SrbGetDataTransferLength(Srb) < RTL_SIZEOF_THROUGH_FIELD(MINIPORT_DUMP_POINTERS, MiniportPrivateDumpData)) ) {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            } else {
                status = StorPortFreePool(AdapterExtension, dumpPointers->MiniportPrivateDumpData);
                Srb->SrbStatus = (status == STOR_STATUS_SUCCESS) ? SRB_STATUS_SUCCESS : SRB_STATUS_ERROR;
            }

            StorPortEtwChannelEvent2(adapterExtension,
                                     storAddress,
                                     (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwEventDiagnostic : StorportEtwEventOperational,
                                     AhciEtwEventStartIO,
                                     L"Completed free dump pointers",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     (Srb->SrbStatus == SRB_STATUS_SUCCESS) ? StorportEtwLevelInformational : StorportEtwLevelError,
                                     StorportEtwEventOpcodeInfo,
                                     Srb,
                                     L"SrbStatus",
                                     Srb->SrbStatus,
                                     L"TransferLen",
                                     SrbGetDataTransferLength(Srb));

            StorPortNotification(RequestComplete, AdapterExtension, Srb);
            break;
        }

        default: {
            // for unsupported SRB: complete with status: SRB_STATUS_INVALID_REQUEST
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            
            StorPortEtwEvent2(adapterExtension,
                              storAddress,
                              AhciEtwEventStartIO,
                              L"Invalid request",
                              STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                              StorportEtwLevelInformational,
                              StorportEtwEventOpcodeInfo,
                              Srb,
                              L"Function",
                              function,
                              NULL,
                              0);

            StorPortNotification(RequestComplete, AdapterExtension, Srb);
            break;
        }

    } //end of switch (function)

    if (processIO) {
        // get active reference for port/device and adapter if this request
        // isn't part of D3 processing and isn't a SCSI command.
        // Storport already make sure that the Unit in active state before a SCSI command is sent to miniport.
        if ((function != SRB_FUNCTION_EXECUTE_SCSI) && (SrbGetSrbFlags(Srb) & SRB_FLAGS_D3_PROCESSING) == 0) {
            // for incoming request, port driver should make sure it's active.
            PortAcquireActiveReference(adapterExtension->PortExtension[pathId], (PSTORAGE_REQUEST_BLOCK)Srb, NULL);
        }

        AhciProcessIo(adapterExtension->PortExtension[pathId], (PSTORAGE_REQUEST_BLOCK)Srb, FALSE);
        ActivateQueue(adapterExtension->PortExtension[pathId], FALSE);
    }

    return TRUE;
}

VOID
__inline
AhciInterruptHandler(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
/*++
Routine Description:

    AhciInterruptHandler is the interrupt handler for a port.

    If the miniport driver requires a large amount of time to process the interrupt it must defer processing to a worker routine.
    This routine must attempt one clear the interrupt on the HBA before it returns TRUE.

NOTE:
    The following StorPort routines shall not be called from the AhciHwInterrupt routine - StorPortCompleteAllRequests and StorPortDeviceBusy.
    The miniport could however request for a worker routine and make the calls in the worker routine.

Called by:
    AhciHwInterrupt
    AhciHwMSIInterrupt

It performs:
    (overview)
    1. Prepare for handling the interrupt
    2. Handle interrupts on this port
    3. Clear port interrupt
    4. Complete outstanding commands
    5. Handle error processing if necessary

    (details)
    1.1 Verify the interrupt is for this adapter
    1.2 round robin for selecting the port to process in case of multi-ports have interrupt pending.
    1.3 Initialize Variables
    AHCI 1.1 Section 5.5.3 - 1.
    "Software determines the cause of the interrupt by reading the PxIS register.  It is possible for multiple bits to be set"
    2.1 Understand interrupts on this channel
    AHCI 1.1 Section 5.5.3 - 2.
    "Software clears appropriate bits in the PxIS register corresponding to the cause of the interrupt."
    2.1.1 Handle Fatal Errors
    Clear the PxIS Fatal Error bits
    And prep for error handling
    2.1.3 Handle non Native HotPlug Events
    Clear the PxSERR bits to clear PxIS
    2.1.3.1 Handle Serial ATA Errors (Hotplug insertion or unsolicited COMINIT)
    2.1.3.2 Handle Serial ATA Errors (Hotplug removal or PHY Power Management event)
    2.1.3.3 Handle Serial ATA Errors (everything else)
    2.1.4 Handle Datalength Mismatch Error
    Clear the PxIS Overflow bit bits
    And prep for dataLengthMisMatch handling
    2.1.15 Handle NonFatal Errors
    2.2 Handle Normal Command Completion
    3.1 Clear channel interrupt
    AHCI 1.1 Section 5.5.3 - 3.
    "Software clears the interrupt bit in IS.IPS corresponding to the port."
    4.1 Complete outstanding commands
    AHCI 1.1 Section 5.5.3 - 4.
    "If executing non-queued commands, software reads the PxCI register, and compares the current value to the list of commands previously issued by software that are still outstanding.  If executing native queued commands, software reads the PxSACT register and compares the current value to the list of commands previously issued by software.  Software completes with success any outstanding command whose corresponding bit has been cleared in the respective register. PxCI and PxSACT are volatile registers;
    software should only use their values to determine commands that have completed, not to determine which commands have previously been issued."
    4.1.1 Determine if there was an ATA error on the command completed. Error register is only valid if bit0 of Status is 1
    4.1.2 Check to see if the right amount of data transfered
    set channelExtension->Slot[i]->DataTransferLength = the amount transfered
    4.1.3 Otherwise, command completed successfully
    5.1 Handle error processing
    Until the error recovery can be completed, don't let any more IO's come through
    AHCI 1.1 Section 5.5.3 - 5.
    "If there were errors, noted in the PxIS register, software performs error recovery actions (see section 6.2.2)."
    AHCI 1.1 Section 6.2.2.1 Non-Queued Error Recovery (this may take a while, better queue a DPC)
    Complete further processing in the worker routine and enable interrupts on the channel

Affected Variables/Registers:

Return Values:
    None

--*/
{
    AHCI_INTERRUPT_STATUS   pxis;
    AHCI_INTERRUPT_STATUS   pxisMask;
    AHCI_SERIAL_ATA_STATUS  ssts;
    AHCI_SERIAL_ATA_ERROR   serr;
    AHCI_SERIAL_ATA_ERROR   serrMask;
    AHCI_COMMAND            cmd;
    ULONG                   is;
    ULONG                   ci;
    ULONG                   sact;
    ULONG                   outstanding;
    ULONG                   storStatus;
    ULONGLONG               asyncNotifyFlags;

    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000005);     //AhciInterruptHandler Enter
    }

    //1.3 Initialize Variables
    sact = 0;
    cmd.AsUlong = 0;
    ssts.AsUlong = 0;
    pxisMask.AsUlong = serrMask.AsUlong = 0;
    serr.AsUlong = 0;

    pxis.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong);
    
    if (pxis.IFS || pxis.HBDS || pxis.HBFS || pxis.TFES || pxis.PCS || pxis.PRCS) {
        serr.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong);
    }

    //2.1 Understand interrupts on this channel
    //2.1.1 Handle Fatal Errors: Interface Fatal Error Status || Host Bus Data Error Status || Host Bus Fatal Error Status || Task File Error Status
    if (pxis.IFS || pxis.HBDS || pxis.HBFS || pxis.TFES) {
        pxisMask.AsUlong = 0;
        pxisMask.IFS = pxisMask.HBDS = pxisMask.HBFS = pxisMask.TFES = 1;
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, pxisMask.AsUlong);

        //call the correct error handling based on current hw queue workload type
        sact = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SACT);

        if ((sact == MAXULONG) && IsAdapterRemoved(ChannelExtension->AdapterExtension)) {
            // controller has been surprise removed
            StorPortEtwChannelEvent8(ChannelExtension->AdapterExtension,
                                     (ChannelExtension != NULL) ? (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress) : (NULL),
                                     StorportEtwEventOperational,
                                     AhciEtwEventHandleInterrupt,
                                     L"Adapter surprise removed",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     StorportEtwLevelWarning,
                                     StorportEtwEventOpcodeInfo,
                                     NULL,
                                     L"pxis",
                                     pxis.AsUlong,
                                     L"ssts",
                                     ssts.AsUlong,
                                     L"serr",
                                     serr.AsUlong,
                                     L"cmd",
                                     cmd.AsUlong,
                                     L"sact",
                                     sact,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     NULL,
                                     0);
            return;
        } else if (sact != 0) {
            //5.1 NCQ, Handle error processing
            ChannelExtension->StateFlags.CallAhciNcqErrorRecovery = 1;

            //Give NCQ one chance
            if (ChannelExtension->StateFlags.NCQ_Succeeded == 0) {
                ChannelExtension->StateFlags.NCQ_Activated = 0;
            }
        } else {
            //5.1 Non-NCQ, Handle error processing
            ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery = 1;
        }
    }

    //2.1.2 Handle non Native HotPlug Events
    // Cold Port Detect Status
    if (pxis.CPDS) {
        pxisMask.AsUlong = 0;
        pxisMask.CPDS = 1;
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, pxisMask.AsUlong);
        // Handle bus rescan processing
        ChannelExtension->StateFlags.CallAhciReportBusChange = 1;
    }

    if (pxis.DMPS || pxis.PCS) {
        cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);

        if ((cmd.AsUlong == MAXULONG) && IsAdapterRemoved(ChannelExtension->AdapterExtension)) {
            StorPortEtwChannelEvent8(ChannelExtension->AdapterExtension,
                                     (ChannelExtension != NULL) ? (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress) : (NULL),
                                     StorportEtwEventOperational,
                                     AhciEtwEventHandleInterrupt,
                                     L"Adapter surprise removed",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     StorportEtwLevelWarning,
                                     StorportEtwEventOpcodeInfo,
                                     NULL,
                                     L"pxis",
                                     pxis.AsUlong,
                                     L"ssts",
                                     ssts.AsUlong,
                                     L"serr",
                                     serr.AsUlong,
                                     L"cmd",
                                     cmd.AsUlong,
                                     L"sact",
                                     sact,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     NULL,
                                     0);
            return;
        }

        // Device Mechanical Presence Status
        if (pxis.DMPS) {
            pxisMask.AsUlong = 0;
            pxisMask.DMPS = 1;
            StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, pxisMask.AsUlong);
            // Mechanical Presence Switch Attached to Port
            if (cmd.MPSP) {
                // Handle bus rescan processing
                ChannelExtension->StateFlags.CallAhciReportBusChange = 1;
            }
        }

        //2.1.3.1 Handle Serial ATA Errors (Hotplug insertion or unsolicited COMINIT)
        // Port Connect Change Status
        if (pxis.PCS) {
            //When PxSERR.DIAG.X is set to one this bit indicates a COMINIT signal was received.  This bit is reflected in the PxIS.PCS bit.
            serrMask.DIAG.X = 1;
            StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong, serrMask.AsUlong);
            // PCS = 1 could be an unsolicited COMINIT on an already detected drive. See AHCI 6.2.2.3    Recovery of Unsolicited COMINIT
            if (!IgnoreHotPlug(ChannelExtension) && (cmd.ST == 0)) {
                // Handle bus rescan processing
                ChannelExtension->StateFlags.CallAhciReportBusChange = 1;
            }
        }
    }

    //2.1.3.2 Handle Serial ATA Errors (Hotplug removal or PHY Power Management event)
    // PhyRdy Change Status
    if (pxis.PRCS) {
        //Hot plug removals are detected via the PxIS.PRCS bit that directly reflects the PxSERR.DIAG.N bit.
        //Note that PxSERR.DIAG.N is also set to '1' on insertions and during interface power management entry/exit.
        serrMask.DIAG.N = 1;
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong, serrMask.AsUlong);

        ssts.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SSTS.AsUlong);

        if ((ssts.AsUlong == MAXULONG) && IsAdapterRemoved(ChannelExtension->AdapterExtension)) {
            // controller has been surprise removed
            return;
        }

        if (!IgnoreHotPlug(ChannelExtension)) {
            //If a ZPODD drive has already been found and it is a ZPODD system
            if ((ChannelExtension->AdapterExtension->StateFlags.SupportsAcpiDSM == 1) &&
                IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters) &&
                (ChannelExtension->DeviceExtension[0].IdentifyPacketData->SerialAtaCapabilities.SlimlineDeviceAttention)) {

                if (ssts.DET == 0) {
                    // ... (*) and there is no presence on the wire ...
                    // ... try to stop the port.
                    if (P_NotRunning(ChannelExtension, ChannelExtension->Px)) {
                        // If that succeeds, complete all outstanding commands.  CI is now cleared.
                        // This is precautionary as there shall be no IO when D3 occurred, but the miniport may always create its own commands.
                        ChannelExtension->SlotManager.CommandsToComplete = GetOccupiedSlots(ChannelExtension);
                        ChannelExtension->SlotManager.CommandsIssued = 0;
                        ChannelExtension->SlotManager.NCQueueSliceIssued = 0;
                        ChannelExtension->SlotManager.NormalQueueSliceIssued = 0;
                        ChannelExtension->SlotManager.SingleIoSliceIssued = 0;
                        ChannelExtension->SlotManager.NCQueueSlice = 0;
                        ChannelExtension->SlotManager.NormalQueueSlice = 0;
                        ChannelExtension->SlotManager.SingleIoSlice = 0;
                        ChannelExtension->SlotManager.HighPriorityAttribute = 0;
                    } else {
                        NT_ASSERT(FALSE);     // Looks like a hardware issue, will recover in P_Running_StartAttempt() when ZPODD is powered on again.
                    }
                } else if (ssts.DET == 3) {
                    // ... (*) and there is presence on the wire ...
                    // ... try to get the channel started.
                    P_Running_StartAttempt(ChannelExtension, TRUE);
                }
            } else if ((ssts.DET == 0) && (ssts.IPM == 0)) {
                // Handle bus rescan processing
                ChannelExtension->StateFlags.CallAhciReportBusChange = 1;
            }
        }
    }

    //2.1.3.3 Handle other Serial ATA Errors (everything else)
    if (serr.AsUlong > 0) {
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong, (ULONG)~0);
    }

    //2.1.4 Handle Datalength Mismatch Error
    // Overflow Status
    if (pxis.OFS) {
        pxisMask.AsUlong = 0;
        pxisMask.OFS = 1;
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, pxisMask.AsUlong);
        //5.1 Handle error processing
        // AHCI 6.1.5 COMRESET is required by software to clean up from this serious error
        ChannelExtension->StateFlags.CallAhciReset = 1;
    }

    //2.1.5 Handle NonFatal Errors
    // Interface Non-fatal Error Status
    if (pxis.INFS) {
        pxisMask.AsUlong = 0;
        pxisMask.INFS = 1;
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, pxisMask.AsUlong);
    }

    //2.1.6 Handle Asynchronous Notification, only ATAPI device supports this feature.
    // Set Device Bits Interrupt
    if ((pxis.SDBS == 1) && (ChannelExtension->ReceivedFIS->SetDeviceBitsFis.N) &&
        IsAtapiDevice(&ChannelExtension->DeviceExtension->DeviceParameters) &&
        IsDeviceSupportsAN(ChannelExtension->DeviceExtension->IdentifyPacketData)) {
        // Asynchronous Notification is received. Notify Port Driver.
        // This async notification could be for media status, device status or device operation events.
        // Notification failure of STOR_STATUS_BUSY is ok as it means that notifications are being coalesced
        // and this is the only async notification.
        // Notification failure of STOR_STATUS_INVALID_DEVICE_REQUEST is ok as Storport may no longer find
        // the unit as it may have been surprise removed.
        asyncNotifyFlags = (RAID_ASYNC_NOTIFY_FLAG_MEDIA_STATUS |
                            RAID_ASYNC_NOTIFY_FLAG_DEVICE_STATUS |
                            RAID_ASYNC_NOTIFY_FLAG_DEVICE_OPERATION);

        storStatus = StorPortAsyncNotificationDetected(ChannelExtension->AdapterExtension,
                                                        (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                                        asyncNotifyFlags);

        NT_ASSERT((storStatus == STOR_STATUS_SUCCESS) ||
                  (storStatus == STOR_STATUS_BUSY) ||
                  (storStatus == STOR_STATUS_INVALID_DEVICE_REQUEST));

        // not actually use this variable.
        UNREFERENCED_PARAMETER(storStatus);
    }

    //2.3 Handle Normal Command Completion
    pxisMask.AsUlong = 0;
    // Device to Host Register FIS Interrupt
    if (pxis.DHRS) {
        pxisMask.DHRS = 1;
    }
    // PIO Setup FIS Interrupt
    if (pxis.PSS) {
        pxisMask.PSS = 1;
    }
    // DMA Setup FIS Interrupt
    if (pxis.DSS) {
        pxisMask.DSS = 1;
    }
    // Set Device Bits Interrupt
    if (pxis.SDBS) {
        pxisMask.SDBS = 1;
    }
    // Descriptor Processed (A PRD with the 'I' bit set has transferred all of its data)
    if (pxis.DPS) {
        pxisMask.DPS = 1;
    }
    if (pxisMask.AsUlong != 0) {
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong, pxisMask.AsUlong);
    }

    //3. error process
    if (ErrorRecoveryIsPending(ChannelExtension)) {
        // preserve taskfile for using in command completion process
        ChannelExtension->TaskFileData.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->TFD.AsUlong);

        if ((ChannelExtension->TaskFileData.AsUlong == MAXULONG) && IsAdapterRemoved(ChannelExtension->AdapterExtension)) {
            // controller has been surprise removed
            StorPortEtwChannelEvent8(ChannelExtension->AdapterExtension,
                                     (ChannelExtension != NULL) ? (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress) : (NULL),
                                     StorportEtwEventOperational,
                                     AhciEtwEventHandleInterrupt,
                                     L"Adapter surprise removed",
                                     STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                     StorportEtwLevelWarning,
                                     StorportEtwEventOpcodeInfo,
                                     NULL,
                                     L"pxis",
                                     pxis.AsUlong,
                                     L"ssts",
                                     ssts.AsUlong,
                                     L"serr",
                                     serr.AsUlong,
                                     L"cmd",
                                     cmd.AsUlong,
                                     L"sact",
                                     sact,
                                     L"StateFlags",
                                     *(ULONGLONG *)&(ChannelExtension->StateFlags),
                                     NULL,
                                     0,
                                     NULL,
                                     0);
            return;
        }

        AhciPortErrorRecovery(ChannelExtension);
    }
    else {
        ChannelExtension->TaskFileData.AsUlong = 0x40;
    }

    //4. Clear channel interrupt
    is = 0;
    is |= (1 << ChannelExtension->PortNumber);
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, ChannelExtension->AdapterExtension->IS, is);

    //5. Complete outstanding commands
    ci = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CI);
    sact = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SACT);

    if (((ci == MAXULONG) || (sact == MAXULONG)) && IsAdapterRemoved(ChannelExtension->AdapterExtension)) {
        // controller has been surprise removed
        StorPortEtwChannelEvent8(ChannelExtension->AdapterExtension,
                                 (ChannelExtension != NULL) ? (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress) : (NULL),
                                 StorportEtwEventOperational,
                                 AhciEtwEventHandleInterrupt,
                                 L"Adapter removed: complete cmd",
                                 STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                                 StorportEtwLevelWarning,
                                 StorportEtwEventOpcodeInfo,
                                 NULL,
                                 L"pxis",
                                 pxis.AsUlong,
                                 L"ssts",
                                 ssts.AsUlong,
                                 L"serr",
                                 serr.AsUlong,
                                 L"cmd",
                                 cmd.AsUlong,
                                 L"is",
                                 is,
                                 L"ci",
                                 ci,
                                 L"sact",
                                 sact,
                                 NULL,
                                 0);
        return;
    }

    outstanding = ci | sact;

    if ((ChannelExtension->SlotManager.CommandsIssued & (~outstanding)) > 0) {
        // all completed commands by hardware will be marked completed
        ChannelExtension->SlotManager.CommandsToComplete |= (ChannelExtension->SlotManager.CommandsIssued & ~outstanding);
        ChannelExtension->SlotManager.CommandsIssued &= outstanding;

        ChannelExtension->SlotManager.NCQueueSliceIssued &= outstanding;
        ChannelExtension->SlotManager.NormalQueueSliceIssued &= outstanding;
        ChannelExtension->SlotManager.SingleIoSliceIssued &= outstanding;

        // recording execution history for completing SRB
        RecordInterruptHistory(ChannelExtension, pxis.AsUlong, ssts.AsUlong, serr.AsUlong, ci, sact, 0x20000005);   //AhciHwInterrupt complete IO

        AhciCompleteIssuedSRBs(ChannelExtension, SRB_STATUS_SUCCESS, TRUE);
    } else {
        // recording execution history for no SRB to be completed
        RecordInterruptHistory(ChannelExtension, pxis.AsUlong, ssts.AsUlong, serr.AsUlong, ci, sact, 0x20010005);   //AhciHwInterrupt No IO completed
    }

    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x10000005);//Exit AhciInterruptHandler
    }

    return;
}

BOOLEAN
AhciHwInterrupt (
    _In_ PVOID AdapterExtension
    )
/*++

Routine Description:

    Line Interrupt handler

Arguments:

    AdapterExtension -
    MessageId -

Called by:
    external

It performs:
    1. Find the port with pending interrupt
    2. Invoke interrupt handler routine to process

Return Values:
    TRUE if the interrupt is handled.
    If the adapter (or port) did not generate interrupt, this routine should return FALSE as soon as possible.
--*/
{
    ULONG                   is;
    ULONG                   interruptPorts;
    ULONG                   i;
    UCHAR                   interruptPortCount;

    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    is = StorPortReadRegisterUlong(AdapterExtension, adapterExtension->IS);
        
    if (adapterExtension->StateFlags.Removed) {
        return FALSE;
    }

    interruptPorts = (is & adapterExtension->PortImplemented);

    //
    // Verify the interrupt is for this adapter
    //
    if (interruptPorts == 0) {
        return FALSE;
    }

    interruptPortCount = NumberOfSetBits(interruptPorts);

    //
    // In case of multi-ports have interrupt pending, using round robin to choose a port handle its interrupt.
    // After this routine exits, other pending interrupts will trigger interrupt again.
    //
    if (interruptPortCount > 1) {
        i = (adapterExtension->LastInterruptedPort + 1) % (adapterExtension->HighestPort + 1);
    } else {
        i = adapterExtension->LastInterruptedPort;
    }

    do {
        if ( ((interruptPorts & (1 << i)) != 0) && IsPortStartCapable(adapterExtension->PortExtension[i]) ) {
            break;
        }
        i = (i + 1) % (adapterExtension->HighestPort + 1);

    } while (i != adapterExtension->LastInterruptedPort);

    if ( (i == adapterExtension->LastInterruptedPort) &&
         (((interruptPorts & (1 << i)) == 0) || !IsPortStartCapable(adapterExtension->PortExtension[i])) ) {
        //
        // interrupt is not for this adapter
        //
        return FALSE;
    }

    adapterExtension->LastInterruptedPort = i;

    AhciInterruptHandler(adapterExtension->PortExtension[i]);

    return TRUE;
}

BOOLEAN
AhciHwMSIInterrupt (
    _In_ PVOID AdapterExtension,
    _In_ ULONG MessageId
    )
/*++

Routine Description:

    MSI Interrupt handler

Arguments:

    AdapterExtension -
    MessageId -

Return Value:

    BOOLEAN - TRUE indicates it's interrupt for AHCI adapter

--*/
{
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    if (adapterExtension->StateFlags.InterruptMessagePerPort == 0) {
        //
        // In case message ID is shared by ports, call legacy ISR to determine if it's a valid AHCI interrupt.
        //
        return AhciHwInterrupt(AdapterExtension);
    }

    //
    // Per AHCI 1.3, Message ID is the same value as the port number.
    //
    AhciInterruptHandler(adapterExtension->PortExtension[MessageId]);

    return TRUE;
}


VOID
AhciHwTracingEnabled (
    _In_ PVOID AdapterExtension,
    _In_ BOOLEAN Enabled
    )
{
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    adapterExtension->TracingEnabled = Enabled;

    return;
}

SCSI_UNIT_CONTROL_STATUS
AhciHwUnitControl (
    _In_ PVOID AdapterExtension,
    _In_ SCSI_UNIT_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters
    )
/*++

Routine Description:

    This is a generic routine to allow for special unit control activities.
    It mainly handles device start irp for a specific device.

Arguments:

    AdapterExtension - Pointer to the device extension for adapter.

    ControlType - Specifies the type of call being made through this routine.

    Parameters - Pointer to parameters needed for this control type (optional).

Return Value:

    SCSI_UNIT_CONTROL_STATUS - currently either:
        ScsiUnitControlSuccess = 0,
        ScsiUnitControlUnsuccessful

--*/

{
    SCSI_UNIT_CONTROL_STATUS status = ScsiUnitControlSuccess;
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST controlTypeList;

    switch (ControlType) {
        // determine which control types (routines) are supported
        case ScsiQuerySupportedUnitControlTypes: {
            // get pointer to control type list
            controlTypeList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;

            // Report ScsiQuerySupportedUnitControlTypes, ScsiUnitStart and ScsiUnitPower are supported.
            if (ScsiQuerySupportedControlTypes < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiQuerySupportedUnitControlTypes] = TRUE;
            }
            if (ScsiUnitStart < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiUnitStart] = TRUE;
            }
            if (ScsiUnitPower < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiUnitPower] = TRUE;
            }
            if (ScsiUnitPoFxPowerInfo < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiUnitPoFxPowerInfo] = TRUE;
            }
            if (ScsiUnitPoFxPowerActive < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiUnitPoFxPowerActive] = TRUE;
            }
            if (ScsiUnitPoFxPowerSetFState < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiUnitPoFxPowerSetFState] = TRUE;
            }
            if (ScsiUnitSurpriseRemoval < controlTypeList->MaxControlType) {
                controlTypeList->SupportedTypeList[ScsiUnitSurpriseRemoval] = TRUE;
            }
            status = ScsiUnitControlSuccess;
            break;
        }

        // ScsiUnitStart is called during Storport processes IRP_MN_START_DEVICE
        case ScsiUnitStart: {
            PSTOR_ADDR_BTL8 storAddrBtl8 = (PSTOR_ADDR_BTL8)Parameters;
            // this is the PnP call IRP_MN_START_DEVICE for the LUN. Need to do device initialization work.
            if (IsPortValid(adapterExtension, storAddrBtl8->Path)) {
                AhciDeviceStart(adapterExtension->PortExtension[storAddrBtl8->Path]);
                status = ScsiUnitControlSuccess;
            } else {
                status = ScsiUnitControlUnsuccessful;
            }
            break;
        }

        // ScsiUnitPower is called during Storport processes power Irp for the unit
        case ScsiUnitPower: {
            PSTOR_UNIT_CONTROL_POWER unitControlPower = (PSTOR_UNIT_CONTROL_POWER)Parameters;
            PSTOR_ADDR_BTL8 storAddrBtl8 = (PSTOR_ADDR_BTL8)unitControlPower->Address;
            // this is the power call for the LUN. Need to power up or power down the device as necessary.
            if (IsPortValid(adapterExtension, storAddrBtl8->Path)) {
                PAHCI_CHANNEL_EXTENSION channelExtension = adapterExtension->PortExtension[storAddrBtl8->Path];

                StorPortDebugPrint(3, "StorAHCI - LPM: SystemIoBusNumber:%d Port:%02d - %s\n", 
                    adapterExtension->SystemIoBusNumber, storAddrBtl8->Path, unitControlPower->PowerState == StorPowerDeviceD0 ? "D0" : "D3");
                
                if (unitControlPower->PowerState == StorPowerDeviceD0) {
                    AhciPortPowerUp(channelExtension);
                    channelExtension->DevicePowerState = StorPowerDeviceD0;
                    status = ScsiUnitControlSuccess;

                } else if (unitControlPower->PowerState == StorPowerDeviceD3) {

                    AhciPortPowerDown(channelExtension);
                    channelExtension->DevicePowerState = StorPowerDeviceD3;
                    status = ScsiUnitControlSuccess;
                } else {
                    NT_ASSERT(FALSE);
                    status = ScsiUnitControlUnsuccessful;
                }
            } else {
                status = ScsiUnitControlUnsuccessful;
            }
            break;
        }

        case ScsiUnitPoFxPowerInfo: {
            PSTOR_POFX_UNIT_POWER_INFO unitPowerInfo = (PSTOR_POFX_UNIT_POWER_INFO)Parameters;
            PSTOR_ADDR_BTL8 storAddrBtl8 = (PSTOR_ADDR_BTL8)unitPowerInfo->Header.Address;
            BOOLEAN d3ColdEnabled = FALSE;

            if (IsPortValid(adapterExtension, storAddrBtl8->Path)) {
                ULONG storStatus = STOR_STATUS_SUCCESS;
                ULONG bufferLength = STOR_POFX_DEVICE_V3_SIZE + STOR_POFX_COMPONENT_V2_SIZE + STOR_POFX_COMPONENT_IDLE_STATE_SIZE;
                PAHCI_CHANNEL_EXTENSION channelExtension = adapterExtension->PortExtension[storAddrBtl8->Path];
                BOOLEAN reportF1State = FALSE;

                //
                // IdlePowerEnabled == TRUE indicates this unit is being
                // registered for runtime power management.  However, the call
                // to StorPortInitializePoFxPower() must succeed before the unit
                // is truly and successfully registered.
                //
                if (unitPowerInfo->IdlePowerEnabled) {

                    //
                    // Don't register more than once.
                    //
                    if (channelExtension->StateFlags.PoFxEnabled) {
                        status = ScsiUnitControlUnsuccessful;
                        break;
                    }

                    if (reportF1State) {
                        // the buffer should be big enough to contain one more F-State (F1)
                        bufferLength += STOR_POFX_COMPONENT_IDLE_STATE_SIZE;
                    }

                    // allocate STOR_POFX_DEVICE data structure for Port, initialize the structure and register for runtime power management.
                    if (channelExtension->PoFxDevice == NULL) {
                        storStatus = StorPortAllocatePool(AdapterExtension,
                                                          bufferLength,
                                                          AHCI_POOL_TAG,
                                                          (PVOID*)&channelExtension->PoFxDevice);
                    }

                    if (storStatus == STOR_STATUS_SUCCESS) {
                        PSTOR_POFX_COMPONENT_V2 component = (PSTOR_POFX_COMPONENT_V2)&(channelExtension->PoFxDevice->Components[0]);

                        AhciZeroMemory((PCHAR)channelExtension->PoFxDevice, bufferLength);

                        channelExtension->PoFxDevice->Version = STOR_POFX_DEVICE_VERSION_V3;
                        channelExtension->PoFxDevice->Size = STOR_POFX_DEVICE_V3_SIZE;
                        channelExtension->PoFxDevice->ComponentCount = 1;

                        //
                        // The dump version of StorAHCI is not able to bring the
                        // unit out of D3Cold.
                        //
                        channelExtension->PoFxDevice->Flags |= STOR_POFX_DEVICE_FLAG_NO_DUMP_ACTIVE;

                        //
                        // Disable idle debouncing if it's possible for us to
                        // initiate a Slumber transition.
                        // We set the Slumber timer when we get the Idle
                        // condition callback and cancel it when we get the
                        // Active condition callback.  The slumber timer is
                        // typically set to 100ms and idle debouncing can add
                        // a bias of up to 1s.  Significantly delaying Slumber
                        // entry can have a negative impact on power so we
                        // disable idle debouncing.
                        // CAP.SSC indicates if the HBA supports Slumber.
                        // CAP.SALP indicates if the HBA can automatically
                        // transition to Partial when idle, which is needed for
                        // us to subsequently initiate a Slumber transition.
                        //
                        if ((channelExtension->AdapterExtension->CAP.SSC) &&
                            (channelExtension->AdapterExtension->CAP.SALP) &&
                            IsDeviceSupportsHIPM(channelExtension->DeviceExtension[0].IdentifyDeviceData)) {
                            channelExtension->PoFxDevice->Flags |= STOR_POFX_DEVICE_FLAG_NO_IDLE_DEBOUNCE;
                        }

                        //
                        // If this is a drive with rotational media then enable adaptive D3 idle timeout, else if
                        // this is a SSD on an AOAC system, set the idle timeout flag and provide a minimum unit idle timeout.
                        //
                        if (DeviceIncursSeekPenalty(channelExtension)) {
                            channelExtension->PoFxDevice->Flags |= STOR_POFX_DEVICE_FLAG_ADAPTIVE_D3_IDLE_TIMEOUT;
                        } else if (IsReceivingSystemPowerHints(adapterExtension) && DeviceIncursNoSeekPenalty(channelExtension)) {
                            channelExtension->PoFxDevice->Flags |= STOR_POFX_DEVICE_FLAG_IDLE_TIMEOUT;
                        }

                        component->Version = STOR_POFX_COMPONENT_VERSION_V2;
                        component->Size = STOR_POFX_COMPONENT_V2_SIZE;
                        component->FStateCount = reportF1State ? 2 : 1;
                        component->DeepestWakeableFState = 0;
                        component->DeepestAdapterPowerRequiredFState = 0;
                        component->DeepestCrashDumpReadyFState = 0;
                        component->Id = STORPORT_POFX_LUN_GUID;

                        // F0 State
                        component->FStates[0].Version = STOR_POFX_COMPONENT_IDLE_STATE_VERSION_V1;
                        component->FStates[0].Size = STOR_POFX_COMPONENT_IDLE_STATE_SIZE;
                        component->FStates[0].TransitionLatency = 0;
                        component->FStates[0].ResidencyRequirement = 0;
                        component->FStates[0].NominalPower = STOR_POFX_UNKNOWN_POWER;

                        // registry runtime power management for Unit
                        storStatus = StorPortInitializePoFxPower(AdapterExtension,
                                                                 (PSTOR_ADDRESS)storAddrBtl8,
                                                                 (PSTOR_POFX_DEVICE)channelExtension->PoFxDevice,
                                                                 &d3ColdEnabled);

                        channelExtension->StateFlags.D3ColdEnabled = d3ColdEnabled;
                    }

                    if (storStatus != STOR_STATUS_SUCCESS) {
                        StorPortFreePool(AdapterExtension, channelExtension->PoFxDevice);
                        channelExtension->PoFxDevice = NULL;
                        channelExtension->StateFlags.PoFxEnabled = FALSE;
                        channelExtension->StateFlags.PoFxActive = FALSE;

                        status = ScsiUnitControlUnsuccessful;
                    } else {
                        channelExtension->StateFlags.PoFxEnabled = TRUE;
                        channelExtension->StateFlags.PoFxActive = TRUE;

                    }

                    channelExtension->PoFxFState = 0;

                } else {
                    //
                    // IdlePowerEnabled = FALSE, which means this unit is no
                    // longer registered for runtime power management.
                    //
                    if (channelExtension->PoFxDevice) {
                        StorPortFreePool(AdapterExtension, channelExtension->PoFxDevice);
                        channelExtension->PoFxDevice = NULL;
                    }
                    channelExtension->StateFlags.PoFxEnabled = FALSE;
                    channelExtension->StateFlags.PoFxActive = FALSE;

                    status = ScsiUnitControlSuccess;
                }
            } else {
                status = ScsiUnitControlUnsuccessful;
            }
            break;
        }

        case ScsiUnitPoFxPowerActive: {
            PSTOR_POFX_ACTIVE_CONTEXT activeContext = (PSTOR_POFX_ACTIVE_CONTEXT)Parameters;
            PSTOR_ADDR_BTL8 storAddrBtl8 = (PSTOR_ADDR_BTL8)activeContext->Header.Address;

            if (IsPortValid(adapterExtension, storAddrBtl8->Path) && PortPoFxEnabled(adapterExtension->PortExtension[storAddrBtl8->Path])) {
                PAHCI_CHANNEL_EXTENSION channelExtension = adapterExtension->PortExtension[storAddrBtl8->Path];

                channelExtension->StateFlags.PoFxActive = activeContext->Active ? 1 : 0;

                StorPortDebugPrint(3, "StorAHCI - LPM: SystemIoBusNumber:%d Port:%02d - %s\n", 
                    adapterExtension->SystemIoBusNumber, channelExtension->PortNumber, activeContext->Active ? "ACTIVE" : "IDLE");

                if (activeContext->Active) {
                    ULONG busChangePending;
                    ULONG restorePreservedSettingsPending;

                    //
                    // Cancel the Slumber timer.
                    //
                    StorPortRequestTimer(channelExtension->AdapterExtension,
                                         channelExtension->WorkerTimer,
                                         AhciAutoPartialToSlumber,
                                         channelExtension,
                                         0, 0);
                    StorPortDebugPrint(3, "StorAHCI - LPM: SystemIoBusNumber:%d Port:%02d - Transit into Slumber from Partial - Canceled\n", 
                        adapterExtension->SystemIoBusNumber, channelExtension->PortNumber);

                    busChangePending = InterlockedBitTestAndReset((LONG*)&channelExtension->PoFxPendingWork, 1);  //BusChange is at bit 1
                    restorePreservedSettingsPending = InterlockedBitTestAndReset((LONG*)&channelExtension->PoFxPendingWork, 0);  //RestorePreservedSettings is at bit 0

                    //perform pending Unit PoFx work
                    if (busChangePending == 1) {
                        PortBusChangeProcess(channelExtension);
                    }

                    if (restorePreservedSettingsPending == 1) {
                        // continue on processing RestorePreservedSettings

                        RecordExecutionHistory(channelExtension, 0x10050041); // RestorePreservedSettings continue in Unit Active

                        // Start the first command
                        IssuePreservedSettingCommands(channelExtension, NULL);

                        // Starts processing the command. Only need to do the first command if it exists. all others will be done by processing completion routine.
                        if (channelExtension->Local.Srb.SrbExtension != NULL) {
                            PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension((PSTORAGE_REQUEST_BLOCK)&channelExtension->Local.Srb);
                            if (srbExtension->AtaFunction != 0) {
                                AhciProcessIo(channelExtension, (PSTORAGE_REQUEST_BLOCK)&channelExtension->Local.Srb, FALSE);
                                ActivateQueue(channelExtension, FALSE);
                            }
                        }
                    }
                } else {
                    //
                    // Attempt to start the Slumber timer now that we're idle.
                    // Use a tolerable delay of 20ms to allow coalescing with
                    // other timers.
                    //
                    if (PartialToSlumberTransitionIsAllowed(channelExtension, NULL)) {

                        status = StorPortRequestTimer(channelExtension->AdapterExtension,
                                                            channelExtension->WorkerTimer,
                                                            AhciAutoPartialToSlumber,
                                                            channelExtension,
                                                            channelExtension->AutoPartialToSlumberInterval * 1000, 20000);

                        if (status == STOR_STATUS_SUCCESS) {
                            StorPortDebugPrint(3, "StorAHCI - LPM: SystemIoBusNumber:%d Port:%02d - Transit into Slumber from Partial - Scheduled\n", 
                                adapterExtension->SystemIoBusNumber, channelExtension->PortNumber);
                        }
                    }
                    
                }
            } else {
                status = ScsiUnitControlUnsuccessful;
            }

            break;
        }

        case ScsiUnitPoFxPowerSetFState: {
            PSTOR_POFX_FSTATE_CONTEXT fStateContext = (PSTOR_POFX_FSTATE_CONTEXT)Parameters;
            PSTOR_ADDR_BTL8 storAddrBtl8 = (PSTOR_ADDR_BTL8)fStateContext->Header.Address;
            PAHCI_CHANNEL_EXTENSION channelExtension = adapterExtension->PortExtension[storAddrBtl8->Path];

            if (IsPortValid(adapterExtension, storAddrBtl8->Path) && PortPoFxEnabled(channelExtension)) {

                StorPortDebugPrint(3, "StorAHCI - LPM: SystemIoBusNumber:%d Port:%02d - Transition from F%u to F%u\n", 
                    adapterExtension->SystemIoBusNumber, channelExtension->PortNumber, channelExtension->PoFxFState, fStateContext->FState);

                channelExtension->PoFxFState = (UCHAR)fStateContext->FState;                

                if (fStateContext->FState == 0) {
                } else if (fStateContext->FState == 1) {
                } else {
                    NT_ASSERT(FALSE);
                    status = ScsiUnitControlUnsuccessful;
                }

            } else {
                status = ScsiUnitControlUnsuccessful;
            }

            break;
        }

        //
        // ScsiUnitSurpriseRemoval is called when Storport processes the surprise removal IRP for the unit.
        //
        case ScsiUnitSurpriseRemoval: {
            PSTOR_ADDR_BTL8 storAddrBtl8 = (PSTOR_ADDR_BTL8)Parameters;
            STOR_LOCK_HANDLE lockhandle = {InterruptLock, 0};

            if (IsPortValid(adapterExtension, storAddrBtl8->Path)) {

                PAHCI_CHANNEL_EXTENSION channelExtension = adapterExtension->PortExtension[storAddrBtl8->Path];
                AhciInterruptSpinlockAcquire(adapterExtension, channelExtension->PortNumber, &lockhandle);

                //
                // Complete all issued commands.
                //
                channelExtension->SlotManager.CommandsToComplete = channelExtension->SlotManager.CommandsIssued;
                channelExtension->SlotManager.CommandsIssued = 0;
                channelExtension->SlotManager.NCQueueSliceIssued = 0;
                channelExtension->SlotManager.NormalQueueSliceIssued = 0;
                channelExtension->SlotManager.SingleIoSliceIssued = 0;
                channelExtension->SlotManager.HighPriorityAttribute &= ~channelExtension->SlotManager.CommandsToComplete;
                
                AhciCompleteIssuedSRBs(channelExtension, SRB_STATUS_NO_DEVICE, TRUE); 

                //
                // Complete all other commands miniport owns for this device.
                //
                AhciPortFailAllIos(channelExtension, SRB_STATUS_NO_DEVICE, TRUE);

                AhciInterruptSpinlockRelease(adapterExtension, channelExtension->PortNumber, &lockhandle);

                ++(channelExtension->TotalCountSurpriseRemove);
                AhciTelemetryLogResetErrorRecovery(channelExtension,
                                                  (PSTOR_ADDRESS)&(channelExtension->DeviceExtension->DeviceAddress),
                                                  AhciTelemetryEventIdSurpriseRemove,
                                                  "ScsiUnitSurpriseRemoval",
                                                  AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING,
                                                  NULL,
                                                  0
                                                  );
            }

            break;
        }

        default:
            status = ScsiUnitControlUnsuccessful;
            break;

    } // end of switch

    return status;
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4152)
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif


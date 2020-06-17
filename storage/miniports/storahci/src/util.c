/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    util.c

Abstract:

    This file contains utility functions of AHCI miniport.


Notes:

Revision History:

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4054)
#pragma warning(disable:4055)
#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

#include "generic.h"

PAHCI_MEMORY_REGISTERS
GetABARAddress(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
/*++
    Search a set of resources (I/O and memory) for an entry corresponding to an AHCI ABAR address.

It assumes:
    AdapterExtension->AhciBaseAddress has been set
Called by:
    AhciHwFindAdapter
It performs:
    1. If any resources are defined, loop over all resource entries, looking for the AHCI ABAR address. If found, return it.
    2. If this point is reached, ABAR was not found. Return NULL.
Affected Variables/Registers:
    none
ReturnValue:
    Address of AHCI controller memory registers if found, NULL otherwise

--*/
{
    PAHCI_MEMORY_REGISTERS  abar;
    ULONG i;

    abar = NULL;

    // loop over resource entries
    if (ConfigInfo->NumberOfAccessRanges > 0) {
        for (i = 0; i < ConfigInfo->NumberOfAccessRanges; i++) {
            BOOLEAN bAccessRangeFound = FALSE;

            // Test to see if this is the needed resource entry, depending on the interface type
            if (ConfigInfo->AdapterInterfaceType == PCIBus) {
                if ((*(ConfigInfo->AccessRanges))[i].RangeStart.QuadPart == AdapterExtension->AhciBaseAddress) {
                    bAccessRangeFound = TRUE;
                }
            } else {
                if ((*(ConfigInfo->AccessRanges))[i].RangeStart.QuadPart != 0) {
                    bAccessRangeFound = TRUE;
                }
            }

            if (bAccessRangeFound) {
                abar = (PAHCI_MEMORY_REGISTERS)StorPortGetDeviceBase(AdapterExtension,
                                                                     ConfigInfo->AdapterInterfaceType,
                                                                     ConfigInfo->SystemIoBusNumber,
                                                                     (*(ConfigInfo->AccessRanges))[i].RangeStart,
                                                                     (*(ConfigInfo->AccessRanges))[i].RangeLength,
                                                                     (BOOLEAN)!(*(ConfigInfo->AccessRanges))[i].RangeInMemory);
                break;
            }
        }
    }

    return abar;
}


VOID
RecordExecutionHistory(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    ULONG Function
  )
/*++
    Takes a snapshot of the AHCI registers for debugging
It assumes:
    nothing
Called by:
    Everything

It performs:
    (overview)
    1 Select the next available index
    2 Take the snapshot
    (details)
    1.1 Select the next available index
    2.1 Copy over the Channel Extension header
    2.2 Copy over the AHCI registers
Affected Variables/Registers:
    none
Return Value:
    none
--*/
{
    if (ChannelExtension->ExecutionHistory == NULL) {
        return;
    }

    //1.1 Select the next available index
    // Actually, ExecutionHistoryNextAvailableIndex is used for current index.
    if (ChannelExtension->ExecutionHistoryNextAvailableIndex >= (MAX_EXECUTION_HISTORY_ENTRY_COUNT - 1)) {
        ChannelExtension->ExecutionHistoryNextAvailableIndex = 0;
    } else {
        ChannelExtension->ExecutionHistoryNextAvailableIndex++;
    }

    StorPortQuerySystemTime(&(ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].TimeStamp));

    //2.1 Copy over the Channel Extension header
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Function = Function;

    if (ChannelExtension->AdapterExtension->IS) {
        // Keep using the old field "IS" to save StateFlags information.
        StorPortCopyMemory(&ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].IS, &ChannelExtension->StateFlags, sizeof(ULONG));
    } else {
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].IS = (ULONG)~0;
    }

    StorPortCopyMemory(&ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].SlotManager, &ChannelExtension->SlotManager, sizeof(SLOT_MANAGER));

    //2.2 Copy over the AHCI registers
    if (ChannelExtension->Px) {
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[0] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CLB.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[1] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CLBU);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[2] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->FB.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[3] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->FBU);

        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[4] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[5] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IE.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[6] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[7] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->DW7_Reserved);

        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[8] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->TFD.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[9] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SIG.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[10] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SSTS.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[11] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong);

        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[12] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[13] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SACT);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[14] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CI);
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[15] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SNTF.AsUlong);

    } else {
        for(int i = 0; i < 0x10; i++) {
            ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[i] = (ULONG)~0;
        }
    }
}

VOID
RecordInterruptHistory(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    ULONG PxIS,
    ULONG PxSSTS,
    ULONG PxSERR,
    ULONG PxCI,
    ULONG PxSACT,
    ULONG Function
  )
/*++
    Takes a snapshot of the Interrupt for issue analysis.
It assumes:
    nothing
Called by:
    Everything

Affected Variables/Registers:
    none
Return Value:
    none
--*/
{
    if (ChannelExtension->ExecutionHistory == NULL) {
        return;
    }

    //1. Select the next available index
    // Actually, ExecutionHistoryNextAvailableIndex is used for current index.
    if (ChannelExtension->ExecutionHistoryNextAvailableIndex >= (MAX_EXECUTION_HISTORY_ENTRY_COUNT - 1)) {
        ChannelExtension->ExecutionHistoryNextAvailableIndex = 0;
    }else{
        ChannelExtension->ExecutionHistoryNextAvailableIndex++;
    }

    StorPortQuerySystemTime(&(ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].TimeStamp));

    //2. Copy data
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Function = Function;

    if (ChannelExtension->AdapterExtension->IS) {
        // Keep using the old field "IS" to save StateFlags information.
        StorPortCopyMemory(&ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].IS, &ChannelExtension->StateFlags, sizeof(ULONG));
    } else {
        ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].IS = (ULONG)~0;
    }

    StorPortCopyMemory(&ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].SlotManager, &ChannelExtension->SlotManager, sizeof(SLOT_MANAGER));

    AhciZeroMemory((PCHAR)ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px, sizeof(ULONG)*16);
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[4] = PxIS;
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[10] = PxSSTS;
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[12] = PxSERR;
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[13] = PxSACT;
    ChannelExtension->ExecutionHistory[ChannelExtension->ExecutionHistoryNextAvailableIndex].Px[14] = PxCI;
}

VOID
Set_PxIE(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_INTERRUPT_ENABLE IE
    )
/*++
    This is just a space saver function that makes the interrupt configuration more readable

Called by:

--*/
{
    AHCI_INTERRUPT_ENABLE ie;
    AHCI_COMMAND cmd;
    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

    ie.AsUlong = StorPortReadRegisterUlong(adapterExtension, &IE->AsUlong);
    ie.DHRE = 1; //Device to Host Register FIS Interrupt (DHRS):  A D2H Register FIS has been received with the �I� bit set, and has been copied into system memory.
    ie.PSE  = 1; //PIO Setup FIS Interrupt (PSS):  A PIO Setup FIS has been received with the �I� bit set, it has been copied into system memory, and the data related to that FIS has been transferred.  This bit shall be set even if the data transfer resulted in an error.
    ie.DSE  = 1; //DMA Setup FIS Interrupt (DSS):  A DMA Setup FIS has been received with the �I� bit set and has been copied into system memory.
    ie.SDBE = 1; //Set Device Bits Interrupt (SDBS):  A Set Device Bits FIS has been received with the �I� bit set and has been copied into system memory.

    ie.UFE  = 0; //Unknown FIS Interrupt (UFS): When set to �1�, indicates that an unknown FIS was received and has been copied into system memory.  This bit is cleared to �0� by software clearing the PxSERR.DIAG.F bit to �0�.  Note that this bit does not directly reflect the PxSERR.DIAG.F bit.  PxSERR.DIAG.F is set immediately when an unknown FIS is detected, whereas this bit is set when that FIS is posted to memory.  Software should wait to act on an unknown FIS until this bit is set to �1� or the two bits may become out of sync.
    ie.DPE  = 0; //Descriptor Processed (DPS):  A PRD with the �I� bit set has transferred all of its data.  Refer to section 5.4.2.
    ie.PCE  = 1; //Port Connect Change Status (PCS): 1=Change in Current Connect Status. 0=No change in Current Connect Status.  This bit reflects the state of PxSERR.DIAG.X.  This bit is only cleared when PxSERR.DIAG.X is cleared.
    if(adapterExtension->CAP.SMPS) {
        ie.DMPE  = 1; //Device Mechanical Presence Status (DMPS): When set, indicates that a mechanical presence switch attached to this port has been opened or closed, which may lead to a change in the connection state of the device.  This bit is only valid if both CAP.SMPS and P0CMD.MPSP are set to �1�.
    } else {
        ie.DMPE  = 0;
    }

    //Reserved :14;
    ie.PRCE = 1; //PhyRdy Change Status (PRCS): When set to �1� indicates the internal PhyRdy signal changed state.  This bit reflects the state of P0SERR.DIAG.N.  To clear this bit, software must clear P0SERR.DIAG.N to �0�.
    ie.IPME = 0; //Incorrect Port Multiplier Status (IPMS):  Indicates that the HBA received a FIS from a device whose Port Multiplier field did not match what was expected.  The IPMS bit may be set during enumeration of devices on a Port Multiplier due to the normal Port Multiplier enumeration process.  It is recommended that IPMS only be used after enumeration is complete on the Port Multiplier.

    ie.OFE  = 1; //Overflow Status (OFS):  Indicates that the HBA received more bytes from a device than was specified in the PRD table for the command.
    //Reserved2 :1;
    ie.INFE = 1; //Interface Non-fatal Error Status (INFS):  Indicates that the HBA encountered an error on the Serial ATA interface but was able to continue operation.  Refer to section 6.1.2.
    ie.IFE  = 1; //Interface Fatal Error Status (IFS):  Indicates that the HBA encountered an error on the Serial ATA interface which caused the transfer to stop.  Refer to section 6.1.2.

    ie.HBDE = 1; //Host Bus Data Error Status (HBDS):  Indicates that the HBA encountered a data error (uncorrectable ECC / parity) when reading from or writing to system memory.
    ie.HBFE = 1; //Host Bus Fatal Error Status (HBFS):  Indicates that the HBA encountered a host bus error that it cannot recover from, such as a bad software pointer.  In PCI, such an indication would be a target or master abort.
    ie.TFEE = 1; //Task File Error Status (TFES):  This bit is set whenever the status register is updated by the device and the error bit (bit 0) is set.
    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong);
    if(cmd.CPD) {    //check for PxCMD.CPD set to �1� before setting CPDE
        ie.CPDE = 1; //Cold Port Detect Status (CPDS): When set, a device status has changed as detected by the cold presence detect logic.  This bit can either be set due to a non-connected port receiving a device, or a connected port having its device removed.  This bit is only valid if the port supports cold presence detect as indicated by PxCMD.CPD set to �1�.
    } else {
        ie.CPDE = 0;
    }
    StorPortWriteRegisterUlong(adapterExtension, &IE->AsUlong, ie.AsUlong);
}


ULONG
GetStringLength (
    _In_ PSTR   String,
    _In_ ULONG  MaxLength
    )
/*
    This function returns the length of string. (not include the "NULL" terminator)
*/
{
    ULONG i;

    for (i = 0; i < MaxLength; i++) {
        if (String[i] == '\0') {
            break;
        }
    }

    return min(i, MaxLength);
}

VOID
WMultiStringToAscii(
    _Inout_ PZZSTR Strings,
    _In_  ULONG StringBufferLength
    )
/*+++

    This routine is used to convert the Source buffer in Wide Multi_SZ into ASCII.
    Note that StringBufferLength should contain the double '0' as the ending of Multi_SZ
         This routine only works when the input string is WCHARs.

--*/
{
    PWCHAR  source = (PWCHAR)Strings;
    PUCHAR  destination = (PUCHAR)Strings;
    ULONG   charCount;

    // the Strings buffer should at least with length of 2 wchars.
    if ( (Strings != NULL) && (StringBufferLength >= 4) ) {

        // Run through the Source buffer and convert the WCHAR to ASCII, placing
        // the converted value in Destination. Ensure that the destination buffer isn't overflowed.
        for (charCount = 0; (charCount < (StringBufferLength / 2 - 1)) && ((*source != '\0') || (*(source + 1) != '\0')); charCount++) {
            *destination = (UCHAR)*source;
            destination++;
            source++;
        }

        // zero the reset of buffer. 
        // "StringBufferLehgth" is length in bytes; "charCount" is count of chars from converted ACSII string.
        if (StringBufferLength > charCount) {
            AhciZeroMemory((PCHAR)destination, (StringBufferLength - charCount));
        }
    }

    return;
}

_Success_(return != FALSE)
BOOLEAN
StringToULONG (
    _In_ PSTR InputString,
    _Out_ PULONG Value
    )
/*
    This function converts string into an ULONG value
    The string should consist of characters of '0' ~ '9' and a '\0' as ending.

Return Value:
    TRUE - when the whole string is converted and the value is in range 0 ~ 0xFFFFFFFF
    FALSE - otherwise
*/
{
    BOOLEAN result = TRUE;
    PSTR    tempString = InputString;
    ULONG   tempValue = 0;

    if ( (InputString == NULL) || (Value == NULL) ) {
        return FALSE;
    }

    while (*tempString != '\0') {
        if ( (*tempString >= '0') && (*tempString <= '9') ) {
            tempValue = tempValue * 10 + (*tempString++ - '0');
        } else {
            result = FALSE;
            break;
        }

        if ( (*tempString != '\0') && (MAXULONG / 10 < tempValue) ) {
            // the value will overflow, bail out
            result = FALSE;
            break;
        }
    }

    if (result == TRUE) {
        *Value = tempValue;
    }

    return result;
}

_Success_(return != FALSE)
BOOLEAN
CompareId (
    _In_opt_ PSTR DeviceId,
    _In_ ULONG  DeviceIdLength,
    _In_opt_ PZZSTR TargetId,
    _In_ ULONG  TargetIdLength,
    _Inout_opt_ PULONG Value
)
/*++
Character comparison between DeviceId and TargetId to the min(DeviceIdLength, TargetIdLength)

Note that TargetId is a multi_sz string. DeviceId will be used to compare with each of strings in TargetId.

When string matches and 'Value' is asked, put value of remaining TargetId into "Value" field

DeviceId -- source string for comparison, in format of ASCII string.

TargetId -- target string for comparison, in format of multi_sz string.
    TargetId format: xxxxxxxx nnn (first part is the ID to compare, second part is the value)

    Special character in TargetId:
        '?' - matches any character in DeviceId
        '*' - indicating end of TargetId, matches any characters left in DeviceId. It should be used only at the end of string.

Return Value:
    TRUE - On a match or contain
    FALSE - On a mismatch
--*/
{
    ULONG   compareLength;
    ULONG   i;
    PUCHAR  localDeviceId;
    PSTR    localStrings = TargetId;
    PUCHAR  localTargetId = (PUCHAR)localStrings;
    BOOLEAN matches = FALSE;

    if ((DeviceId == NULL) || (TargetId == NULL)) {
        // nothing to compare, return TRUE
        return TRUE;
    }

    while ( (*localStrings != '\0') || (*(localStrings + 1) != '\0') ) {
        localDeviceId = (PUCHAR)DeviceId;
        compareLength = min(DeviceIdLength, GetStringLength(localStrings, TargetIdLength));
        if (compareLength == 0) {
            return FALSE;
        }

        matches = TRUE;
        for(i = 0; i < compareLength; i++) {
            if (*localTargetId == '*') {
                localTargetId++;
                break;  // whole string match
            } else if ( (*localDeviceId == '\0') || (*localTargetId == '\0') ) {
                matches = FALSE;
                break;
            } else if ( (*localDeviceId == *localTargetId) ||
                        (*localTargetId == '?') ) {
                localDeviceId++;
                localTargetId++;
            } else {
                matches = FALSE;
                break;
            }
        }

        if (matches) {
            break;
        }

        localStrings+= GetStringLength(localStrings, TargetIdLength) + 1;
        localTargetId = (PUCHAR)localStrings;
    }

    if ( (Value != NULL) &&
         (matches == TRUE) &&
         (*localTargetId == ' ') ) {

         //convert the left part of localTargetId into a value
         localTargetId++;
         if (!StringToULONG((PSTR)localTargetId, Value)) {
             matches = FALSE;
         }
    }

    return matches;
}

VOID
AhciBusChangeCallback(
    _In_ PVOID AdapterExtension,
    _In_opt_ PVOID Context,
    _In_ SHORT AddressType,
    _In_ PVOID Address,
    _In_ ULONG Status
    )
{
    PAHCI_ADAPTER_EXTENSION adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    PSTOR_ADDR_BTL8         storAddrBtl8 = (PSTOR_ADDR_BTL8)Address;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(AddressType);

    NT_ASSERT(AddressType == STOR_ADDRESS_TYPE_BTL8);

    if (Status == STOR_STATUS_SUCCESS) {
        // targeted rescan completed successfully
        return;
    }

    // Previous targeted rescan failed, retry it.
    if (IsPortValid(adapterExtension, storAddrBtl8->Path)) {

        StorPortStateChangeDetected(AdapterExtension,
                                    STATE_CHANGE_TARGET,
                                    (PSTOR_ADDRESS)&adapterExtension->PortExtension[storAddrBtl8->Path]->DeviceExtension[0].DeviceAddress,
                                    0,
                                    AhciBusChangeCallback,
                                    NULL);
    }

    return;
}

VOID
PortBusChangeProcess (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
    )
{
    AHCI_SERIAL_ATA_CONTROL sctl;
    STOR_LOCK_HANDLE lockhandle = { InterruptLock, 0 };
    ULONG status = STOR_STATUS_UNSUCCESSFUL;

    // 1 if link speed was limited, restore the supported value.
    sctl.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong);
    if ((sctl.SPD != 0) &&
        (sctl.SPD != ChannelExtension->AdapterExtension->CAP.ISS)) {

        sctl.SPD = 0;   //AHCI 3.3.11 --- 0h  No speed negotiation restrictions
        StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong, sctl.AsUlong);
    }

    // 2 Kicks off the Start Channel state machine
    AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    AhciPortReset(ChannelExtension, TRUE);    // all requests should be completed
    AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);

    // Mark that cached Log Page information need to be updated.
    ChannelExtension->DeviceExtension[0].UpdateCachedLogPageInfo = TRUE;

    // 3 Requests rescan on this port. Using STATE_CHANGE_TARGET to make sure a REPORT LUNS command coming from Storport.
    status = StorPortStateChangeDetected(ChannelExtension->AdapterExtension,
                                         STATE_CHANGE_TARGET,
                                         (PSTOR_ADDRESS)&ChannelExtension->DeviceExtension[0].DeviceAddress,
                                         0,
                                         AhciBusChangeCallback,
                                         NULL);

    // If the state change notification fails, set a timer to retry the notification after a delay
    if (status == STOR_STATUS_UNSUCCESSFUL) {

        if (ChannelExtension->BusChangeTimer == NULL) {
            StorPortInitializeTimer(ChannelExtension->AdapterExtension, &ChannelExtension->BusChangeTimer);
        }

        status = StorPortRequestTimer(ChannelExtension->AdapterExtension, 
                                     ChannelExtension->BusChangeTimer, 
                                     AhciBusChangeTimerCallback,
                                     ChannelExtension, 
                                     100000, // 100 milliseconds
                                     0);

        if (status != STOR_STATUS_SUCCESS) {
            PortReleaseActiveReference(ChannelExtension, NULL);
        }
    } else {

        if (ChannelExtension->BusChangeTimer != NULL) {

            StorPortFreeTimer(ChannelExtension->AdapterExtension, ChannelExtension->BusChangeTimer);
            ChannelExtension->BusChangeTimer = NULL;
        }

        PortReleaseActiveReference(ChannelExtension, NULL);
    }
}

VOID
AhciPortBusChangeDpcRoutine(
    _In_ PSTOR_DPC  Dpc,
    _In_ PVOID      AdapterExtension,
    _In_opt_ PVOID  SystemArgument1,
    _In_opt_ PVOID  SystemArgument2
    )
/*++
    Asks port driver to request a QDR on behalf of the miniport
It assumes:
    nothing
Called by:
    Indirectly by AhciHwInterrupt

It performs:
    1 Kicks off the Start Channel state machine
    2 Requests QDR
Affected Variables/Registers:
    none
--*/
{
    PAHCI_CHANNEL_EXTENSION channelExtension = (PAHCI_CHANNEL_EXTENSION)SystemArgument1;
    ULONG                   busChangeInProcess;
    BOOLEAN                 portIdle = FALSE;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(AdapterExtension);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (channelExtension == NULL) {
        NT_ASSERT(channelExtension != NULL);
        return;
    }

    busChangeInProcess = InterlockedBitTestAndSet((LONG*)&channelExtension->PoFxPendingWork, 1);  //BusChange is at bit 1

    if (busChangeInProcess == 1) {
        // bus change is pending in another process.
        return;
    }

    RecordExecutionHistory(channelExtension, 0x00000030);//AhciPortBusChangeDpcRoutine

    PortAcquireActiveReference(channelExtension, NULL, &portIdle);

    // if port is in Active state, continue to process bus change notification.
    // otherwise, it will be processed when port gets into Active state
    if (!portIdle) {
        ULONG   busChangePending;
        busChangePending = InterlockedBitTestAndReset((LONG*)&channelExtension->PoFxPendingWork, 1);  //BusChange is at bit 1

        if (busChangePending == 1) {
            PortBusChangeProcess(channelExtension);
        }
    }

    return;
}

VOID
AhciBusChangeTimerCallback(
    _In_ PVOID AdapterExtension,
    _In_opt_ PVOID ChannelExtension
    )
{
    PAHCI_CHANNEL_EXTENSION channelExtension = (PAHCI_CHANNEL_EXTENSION)ChannelExtension;

    if ( (channelExtension == NULL) || (channelExtension->Px == NULL) ) {

        // The port has been stopped. 
        //
        // Note: 
        // Px is set to NULL in AhciPortStop function. StartIo spin lock is utilized to
        // prevent race condition with AhciPortStop function. StartIo spin lock is acquired 
        // before AhciPortStop is called. When we are here in AhciBusChangeTimerCallback, because
        // it is a timer callback function, StartIo spin lock is already held - Storport holds
        // StartIo spin lock before invoking miniport timer callback function.
        //       

        return;
    }

    AhciPortBusChangeDpcRoutine(&channelExtension->BusChangeDpc, AdapterExtension, channelExtension, NULL);
}

VOID
ReleaseSlottedCommand(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR SlotNumber,
    _In_ BOOLEAN AtDIRQL
    )
/*++
    Performs Completion Data marshaling back into the SRB
It assumes:
    SRB STATUS has already been filled out
    ATA status from the AHCI registers is valid for this command
    SlotNumber points to a fully filled out Slot entry
Called by:
    AhciHwStartIo
    AhciNonQueuedErrorRecovery
    AhciCompleteIssuedSRBs

It performs:
    (overview)
    1 Initialize
    2 Marshal completion data
    3 Complete the Command
    (details)
    1.1 Keep track of the completion for our debugging records
    2.1 Handle Request Sense marshaling
    2.2.1 Handle everything else's marshaling
    2.2.2 If this was marked as needing return data, fill in the return Task File
    3.1 Make the slot available again
    3.2 Perform the Slot's last request, if there was one
    3.3.1 IO is completing, that IO may have paused the queue so unpause the queue
    3.3.2  Complete the command
Affected Variables/Registers:
    SRB
--*/
{
    PSLOT_CONTENT           slotContent;
    PAHCI_SRB_EXTENSION     srbExtension;
    PSTORAGE_REQUEST_BLOCK  srbToRelease;
    BOOLEAN                 isSenseSrb;
    BOOLEAN                 retrySrb = FALSE;
    BOOLEAN                 isReadLogExtSrbIssuedInNCQErrorRecovery = FALSE;

    slotContent = &ChannelExtension->Slot[SlotNumber];
    if (slotContent->Srb == NULL) {
        return;
    }

    srbExtension = GetSrbExtension(slotContent->Srb);
    isSenseSrb = IsRequestSenseSrb(srbExtension->AtaFunction);

    if ((slotContent->Srb == (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Sense.Srb) && 
        (ChannelExtension->StateFlags.NcqErrorRecoveryInProcess == 1)) {

        isReadLogExtSrbIssuedInNCQErrorRecovery = TRUE;
    }

    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x10000053);//ReleaseSlottedCommand
    }

  //2/2 Log command completion part for our debugging records
    if (LogCommand(ChannelExtension->AdapterExtension->LogFlags)) {
        PCOMMAND_HISTORY         cmdHistory;
        slotContent->CommandHistoryIndex %= 64; // should not be 64 or bigger. do it anyway to make sure we are safe

        cmdHistory = &ChannelExtension->CommandHistory[slotContent->CommandHistoryIndex];
        StorPortCopyMemory((PVOID)&cmdHistory->CompletionFIS, (PVOID)&ChannelExtension->ReceivedFIS->D2hRegisterFis, sizeof(AHCI_D2H_REGISTER_FIS));

        cmdHistory->CompletionPx[0] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CLB.AsUlong);
        cmdHistory->CompletionPx[1] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CLBU);
        cmdHistory->CompletionPx[2] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->FB.AsUlong);
        cmdHistory->CompletionPx[3] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->FBU);

        cmdHistory->CompletionPx[4] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IS.AsUlong);
        cmdHistory->CompletionPx[5] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->IE.AsUlong);
        cmdHistory->CompletionPx[6] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);
        cmdHistory->CompletionPx[7] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->DW7_Reserved);

        cmdHistory->CompletionPx[8] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->TFD.AsUlong);
        cmdHistory->CompletionPx[9] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SIG.AsUlong);
        cmdHistory->CompletionPx[10] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SSTS.AsUlong);
        cmdHistory->CompletionPx[11] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong);

        cmdHistory->CompletionPx[12] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong);
        cmdHistory->CompletionPx[13] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SACT);
        cmdHistory->CompletionPx[14] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CI);
        cmdHistory->CompletionPx[15] = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SNTF.AsUlong);

        cmdHistory->SrbStatus = slotContent->Srb->SrbStatus;

    }

  //2. Then complete the command
    if (isSenseSrb) {
      //2.1 Handle Request Sense marshaling
        srbToRelease = (PSTORAGE_REQUEST_BLOCK)SrbGetOriginalRequest(slotContent->Srb);

        //that original SRB must have SRB_STATUS_AUTOSENSE_VALID set appropriately
        if (slotContent->Srb->SrbStatus == SRB_STATUS_SUCCESS) {
            srbToRelease->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            SrbSetScsiStatus(srbToRelease, SCSISTAT_CHECK_CONDITION);
        } else {
            srbToRelease->SrbStatus &= ~SRB_STATUS_AUTOSENSE_VALID;
        }

        //finish use of Sense.Srb
        srbExtension->AtaFunction = 0;
    } else {
      //2.2.1 Handle everything else's marshaling
        srbToRelease = slotContent->Srb;

        if ((srbExtension->Flags & ATA_FLAGS_SENSEDATA_SET) == 0) {
           //Record error and status
            srbExtension->AtaStatus = ChannelExtension->TaskFileData.STS.AsUchar;
            srbExtension->AtaError = ChannelExtension->TaskFileData.ERR;

          //2.2.2 If this was marked as needing return data, fill in the return Task File.
            if( IsReturnResults(srbExtension->Flags) ) {
                SetReturnRegisterValues(ChannelExtension, srbToRelease, NULL);
            }

            // interpret ATA error to be SCSI error and fill SenseBuffer
            // also log IO Record
            if (IsAtaCommand(srbExtension->AtaFunction) ||
                IsAtaCfisPayload(srbExtension->AtaFunction)) {
                // IO Record for ATA device is logged in AtaMapError()
                AtaMapError(ChannelExtension, srbToRelease, slotContent->StateFlags.FUA);
            } else {
                if (SRB_STATUS(srbToRelease->SrbStatus) == SRB_STATUS_SUCCESS) {
                    ChannelExtension->DeviceExtension[0].IoRecord.SuccessCount++;
                } else {
                    ChannelExtension->DeviceExtension[0].IoRecord.OtherErrorCount++;
                }
            }
        }
    }

  //3.1 Make the slot available again
    slotContent->CmdHeader = NULL;
    slotContent->CommandHistoryIndex = 0;
    slotContent->Srb = NULL;
    slotContent->StateFlags.FUA = FALSE;

    //Clear the CommandsToComplete bit
    ChannelExtension->SlotManager.CommandsToComplete &= ~(1 << SlotNumber);
    ChannelExtension->SlotManager.HighPriorityAttribute &= ~(1 << SlotNumber);

  //3.3.1 IO is completing, that IO may have paused the queue so unpause the queue
  //      During NCQ Error Recovery process, this flag will be reset in the error recovery completion process, either through reset or through slot release again.
    if (ChannelExtension->StateFlags.NcqErrorRecoveryInProcess == 0) {
        ChannelExtension->StateFlags.QueuePaused = FALSE;
    }

  //3.3.2 Complete the command
    if ( (srbToRelease->SrbStatus != SRB_STATUS_SUCCESS) && !isSenseSrb && (srbExtension->RetryCount == 0) ) {
        if ( (ChannelExtension->StateFlags.HybridInfoEnabledOnHiberFile == 1) && 
             IsNCQWriteCommand(srbExtension) ) {
            // NCQ WRITE with Hybrid Information failed, retry once without Hybrid Information.
            ChannelExtension->StateFlags.HybridInfoEnabledOnHiberFile = 0;
            retrySrb = TRUE;
        } else if (srbToRelease->SrbStatus == SRB_STATUS_PARITY_ERROR) {
            // if a command encounters CRC error, retry once
            retrySrb = TRUE;
        }
    }

    //
    // There is a bad timing will break the error recovery and cause IO hang eventually, typically happen in SSD but not limited to.
    // 1. NCQ error happen first, which will issue the ReadLogExtCommand;
    // 2. Then Nonqueued error happen before the ReadLogExtCommand complete.
    //    In ReleaseSlottedCommand, ReadLogExtCommand may got chance to be retried, which will result in SingleIoSlice set to 1, 
    //    and SingleIoSlice will skip the PreservedSettingCommand(the ReservedSlotInUse is already set).
    // 3. Then ReservedSlotInUse never got chance to be cleared because it relies on PreservedSettingCommand execution.
    //    ReservedSlotInUse will block the queue activation, finally the IO will hang and AhciPortReset will be repeatedly issued by upper layer.
    //   
    // Do not retry ReadLogExtCommand when NCQ error recovery in process, which makes sense since ReadLogExtCommand is sense-like command as well.
    //
    if (retrySrb && isReadLogExtSrbIssuedInNCQErrorRecovery) {
        retrySrb = FALSE;
    }

    if (retrySrb) {
        srbToRelease->SrbStatus = SRB_STATUS_PENDING;
        srbExtension->RetryCount++;
        AhciProcessIo(ChannelExtension, srbToRelease, AtDIRQL);
    } else {
        // otherwise, complete it.
        AhciCompleteRequest(ChannelExtension, srbToRelease, AtDIRQL);
    }
}

VOID
AhciCompleteJustSlottedRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    )
/*
    This routine is to complete a request that occupies a slot but has not been issued to adapter yet.
    Srb status should be set before calling this routine.
*/
{
    PSLOT_CONTENT           slotContent;
    PAHCI_SRB_EXTENSION     srbExtension;
    BOOLEAN                 isSenseSrb;
    PSTORAGE_REQUEST_BLOCK  srbToComplete;
    STOR_LOCK_HANDLE        lockHandle = {InterruptLock, 0};

    srbExtension = GetSrbExtension(Srb);
    slotContent = &ChannelExtension->Slot[srbExtension->QueueTag];

    NT_ASSERT(slotContent->Srb == Srb);

    isSenseSrb = IsRequestSenseSrb(srbExtension->AtaFunction);

    if (isSenseSrb) {
      // Handle Request Sense marshaling
        srbToComplete = (PSTORAGE_REQUEST_BLOCK)SrbGetOriginalRequest(Srb);

        //Sense Srb doesn't have chance to run yet, clear Sense Valid flag from original SRB.
        srbToComplete->SrbStatus &= ~SRB_STATUS_AUTOSENSE_VALID;

        //finish use of Sense.Srb
        srbExtension->AtaFunction = 0;
    } else {
        srbToComplete = Srb;
    }

    if (!AtDIRQL) {
        AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockHandle);
    }

  //1. Make the slot available again
    slotContent->CmdHeader = NULL;
    slotContent->CommandHistoryIndex = 0;
    slotContent->Srb = NULL;
    slotContent->StateFlags.FUA = FALSE;

  //2. this function can be called from places that don't call ActivateQueue yet. Clear the bit in IO Slices
    if ((ChannelExtension->SlotManager.HighPriorityAttribute & (1 << srbExtension->QueueTag)) != 0) {
        ChannelExtension->SlotManager.HighPriorityAttribute &= ~(1 << srbExtension->QueueTag);
    }
    if ((ChannelExtension->SlotManager.NCQueueSlice & (1 << srbExtension->QueueTag)) != 0) {
        ChannelExtension->SlotManager.NCQueueSlice &= ~(1 << srbExtension->QueueTag);
    }
    if ((ChannelExtension->SlotManager.NormalQueueSlice & (1 << srbExtension->QueueTag)) != 0) {
        ChannelExtension->SlotManager.NormalQueueSlice &= ~(1 << srbExtension->QueueTag);
    }
    if ((ChannelExtension->SlotManager.SingleIoSlice & (1 << srbExtension->QueueTag)) != 0) {
        ChannelExtension->SlotManager.SingleIoSlice &= ~(1 << srbExtension->QueueTag);
    }
    if ((ChannelExtension->SlotManager.CommandsToComplete & (1 << srbExtension->QueueTag)) != 0) {
        ChannelExtension->SlotManager.CommandsToComplete &= ~(1 << srbExtension->QueueTag);
    }

    if (!AtDIRQL) {
        AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockHandle);
    }

  //3. Complete the command
    AhciCompleteRequest(ChannelExtension, srbToComplete, AtDIRQL);

    return;
}

VOID
AhciCompleteRequest(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    )
/*++
    Wrapper for ComleteRequest to protect against completing the local SRB back to port driver who doesn't know anything about local SRB
It assumes:
    SRB is completely ready to be completed back to the SRB generator
It performs:
    1. If Srb has completion routine, put it in completion queue.
    2. Complete the command back to the owner. Do not complete the Local Srb.
Called by:
    AhciHwStartIo
    RequiresControllerResources
    ReleaseSlottedCommand

NOTE:
    The caller of this routine should call ActiveQueue to program the command (from Srb completion routine) to adapter

--*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);

  //1. If Srb acquired active reference or has completion routine, put it in completion queue. Otherwise, complete it.
    if ( ((srbExtension->Flags & ATA_FLAGS_ACTIVE_REFERENCE) != 0) ||
         (srbExtension->CompletionRoutine != NULL) ) {

        if (AtDIRQL == FALSE) {
            STOR_LOCK_HANDLE lockhandle = {InterruptLock, 0};

            AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
            AddQueue(ChannelExtension, &ChannelExtension->CompletionQueue, Srb, 0xDEADBEEF, 0x90);
            AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
        } else {
            AddQueue(ChannelExtension, &ChannelExtension->CompletionQueue, Srb, 0xDEADBEEF, 0x90);
        }

        if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
            RecordExecutionHistory(ChannelExtension, 0x30000045); //Exit AhciCompleteRequest, SRB is put in CompletionQueue
        }

        if (!IsDumpMode(ChannelExtension->AdapterExtension)) {
            StorPortIssueDpc(ChannelExtension->AdapterExtension, &ChannelExtension->CompletionDpc, ChannelExtension, Srb);
        } else {
            AhciPortSrbCompletionDpcRoutine(&ChannelExtension->CompletionDpc, ChannelExtension->AdapterExtension, ChannelExtension, Srb);
        }

    } else {
      //2. Complete the command back to the owner
        if (IsMiniportInternalSrb(ChannelExtension, Srb)) {
            NT_ASSERT(((PSCSI_REQUEST_BLOCK)Srb == &ChannelExtension->Sense.Srb) || (ChannelExtension->StateFlags.ReservedSlotInUse == 0));
            RecordExecutionHistory(ChannelExtension, 0x20000045); //Exit AhciCompleteRequest,  Local SRB
            return;
        } else {
            NT_ASSERT(Srb->SrbStatus != SRB_STATUS_PENDING);
            StorPortNotification(RequestComplete, ChannelExtension->AdapterExtension, Srb);

            if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
                RecordExecutionHistory(ChannelExtension, 0x10000045); //Exit AhciCompleteRequest, SRB from port driver
            }
        }
    }

    return;
}

VOID
GetAvailableSlot(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PSTORAGE_REQUEST_BLOCK  Srb
    )
/*++
    Returns the next available slot that can be used for an ATA command in the QueueTag field of the SRB
It assumes:

Called by:

It performs:
    1.1 Initialize variables
    2.1 Special case the slot for the local SRB
    2.2 Chose the slot circularly starting with CurrentCommandSlot
    3.1 Update CurrentCommandSlot

Affected Variables/Registers:

Return Value:
    Next available slot number.  If SRB is localSRB then slot is 0.
    If no slot is available tag returned is 0xFF.
--*/
{
    ULONG               allocated;
    UCHAR               limit;
    UCHAR               i;
    PAHCI_SRB_EXTENSION srbExtension;

    srbExtension = GetSrbExtension(Srb);

  //1.1 Initialize variables
    srbExtension->QueueTag = 0xFF;
    limit = ChannelExtension->CurrentCommandSlot;

    allocated = GetOccupiedSlots(ChannelExtension);

  //2.1 Use slot 0 for internal commands, don't increment CCS
    if (Srb == (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb ) {
        if ((allocated & (1 << 0)) > 0) {
            srbExtension->QueueTag = 0xFF;
        } else {
            srbExtension->QueueTag = 0;
        }
        return;
    }

    // case of Sense.Srb used for NCQ error recovery.
    if ((Srb == (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Sense.Srb) &&
        IsAtaCommand(srbExtension->AtaFunction)) {

        if ((allocated & (1 << 0)) == 0) {
            srbExtension->QueueTag = 0;
            return;
        }
    }

  //2.2 Chose the slot circularly starting with CCS
    for (i = limit; i <= ChannelExtension->AdapterExtension->CAP.NCS; i++) {
        if ( (allocated & (1 << i)) == 0 ) {
            srbExtension->QueueTag = i;
            goto getout;
        }
    }

    for (i = 1; i < limit; i++) {
        if ( (allocated & (1 << i)) == 0 ) {
            srbExtension->QueueTag = i;
            goto getout;
        }
    }

getout:
  //3.1 Update CurrentCommandSlot
    if (IsRequestSenseSrb(srbExtension->AtaFunction)) {
      //If this SRB is for Request Sense, make sure it is given the next chance to run during ActivateQueue by not incrementing CCS.
        return;
    }

    ChannelExtension->CurrentCommandSlot++;
    if (ChannelExtension->CurrentCommandSlot == (ChannelExtension->AdapterExtension->CAP.NCS + 1)) {
        ChannelExtension->CurrentCommandSlot = 1;
    }

    return;
}


BOOLEAN
UpdateSetFeatureCommands(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR OldFeatures,
    _In_ UCHAR NewFeatures,
    _In_ UCHAR OldSectorCount,
    _In_ UCHAR NewSectorCount
  )
/*++
    Edit a PersistentSetting command
It assumes:
    0xFF means invalid feature
Called by:
    RequiresControllerResources

It performs:
    1.1 Find the location of the given command
    2.1 Replace it with the new command
Affected Variables/Registers:
    PersistentSettings
Return Value:
    TRUE if the edit succeeded
    FALSE if the command did not already exist
--*/
{
    BOOLEAN done = FALSE;
    UCHAR   i = 0;

    for (i = 0; i < MAX_SETTINGS_PRESERVED; i++) {
      //
        if ( (ChannelExtension->PersistentSettings.CommandParams[i].Features == OldFeatures) &&
             (ChannelExtension->PersistentSettings.CommandParams[i].SectorCount == OldSectorCount) ) {
            //Replace old one it with the new command
            ChannelExtension->PersistentSettings.CommandParams[i].Features = NewFeatures;
            ChannelExtension->PersistentSettings.CommandParams[i].SectorCount = NewSectorCount;
            done = TRUE;
            break;
        } else if ( (ChannelExtension->PersistentSettings.CommandParams[i].Features == NewFeatures) &&
                    (ChannelExtension->PersistentSettings.CommandParams[i].SectorCount == NewSectorCount) ) {
            //the new command is already in array
            done = TRUE;
            break;
        }
    }

    if (!done) {
        //try to find an empty slot
        ULONG slotsAvailable = ChannelExtension->PersistentSettings.Slots;
        i = 0;
        while (slotsAvailable) {
            slotsAvailable >>= 1;
            i++;
        }
        //Add the new command into the empty slot
        if (i < MAX_SETTINGS_PRESERVED) {
            ChannelExtension->PersistentSettings.CommandParams[i].Features = NewFeatures;
            ChannelExtension->PersistentSettings.CommandParams[i].SectorCount = NewSectorCount;
            //mark the Persistent settings slot as programmed
            ChannelExtension->PersistentSettings.Slots |= (1 << i);

            done = TRUE;
        } else {
            NT_ASSERT(FALSE);  //we don't expect so much commands for persistent settings.
        }
    }

    return done;
}

VOID
RestorePreservedSettings(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
  )
/*++
    Send each of the commands in the PersistentSettings structure one at a time using the local SRB
It assumes:
    The local SRB is not currently in use
Called by:
    AhciPortIssueInitCommands
    AhciPortReset
    AhciNonQueuedErrorRecovery

It performs:
    1 Reinitialize all the commands to send
    2 Start the first command

Affected Variables/Registers:
--*/
{
    BOOLEAN reservedSlotInUse = 0;

    RecordExecutionHistory(ChannelExtension, 0x00000041); // RestorePreservedSettings

    //1 Reinitialize all the commands to send
    ChannelExtension->PersistentSettings.SlotsToSend = ChannelExtension->PersistentSettings.Slots;
    reservedSlotInUse = InterlockedBitTestAndSet((LONG*)&ChannelExtension->StateFlags, 3);    //ReservedSlotInUse field is at bit 3

    if (reservedSlotInUse == 1) {

        // the process has been started.
        RecordExecutionHistory(ChannelExtension, 0x10010041); // RestorePreservedSettings process already started
        return;
    }

    // acquire active reference for process of restore preserved settings
    if (!AtDIRQL) {
        // this is only needed if function is called at lower level than DIRQL.
        // the places calling this function at DIRQL already made sure the unit is active.
        ULONG   restorePreservedSettingsInProcess;
        BOOLEAN portIdle = FALSE;

        restorePreservedSettingsInProcess = InterlockedBitTestAndSet((LONG*)&ChannelExtension->PoFxPendingWork, 0);  //RestorePreservedSettings is at bit 0

        if (restorePreservedSettingsInProcess == 1) {

            // it's already in process, return from here
            RecordExecutionHistory(ChannelExtension, 0x10020041); // RestorePreservedSettings already in process
            return;
        }

        if (PortAcquireActiveReference(ChannelExtension, NULL, &portIdle)) {
            ChannelExtension->StateFlags.RestorePreservedSettingsActiveReferenced = 1;
        }

        if (portIdle) {

            // Unit Active process will continue on RestorePreservedSettings process
            RecordExecutionHistory(ChannelExtension, 0x10030041); // RestorePreservedSettings will continue in Unit Active
            return;
        } else {
            ULONG   restorePreservedSettingsPending;
            restorePreservedSettingsPending = InterlockedBitTestAndReset((LONG*)&ChannelExtension->PoFxPendingWork, 0);  //RestorePreservedSettings is at bit 0

            if (restorePreservedSettingsPending == 0) {

                // Unit Active process already started on RestorePreservedSettings process
                RecordExecutionHistory(ChannelExtension, 0x10040041); // RestorePreservedSettings already started in Unit Active
                return;
            }

        }
    }

    //2 Start the first command
    IssuePreservedSettingCommands(ChannelExtension, NULL);    

    //3 Starts processing the command. Only need to do the first command if it exists. all others will be done by processing completion routine.
    if (ChannelExtension->Local.Srb.SrbExtension != NULL) {
        PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension((PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb);
        if (srbExtension->AtaFunction != 0) {
            AhciProcessIo(ChannelExtension, (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb, AtDIRQL);
        }
    }

    return;
}

VOID
AhciPortIssueInitCommands(
    PAHCI_CHANNEL_EXTENSION ChannelExtension
  )
/*++
    Send each of the commands in the Init Command list and PersistentSettings structure, one at a time using the local SRB
It assumes:
    The local SRB is not currently in use
Called by:
    AhciDeviceInitialize

It performs:
    1 Reinitialize all the commands to send
    2 Start the first command

Affected Variables/Registers:
--*/
{
    BOOLEAN reservedSlotInUse = 0;

    RecordExecutionHistory(ChannelExtension, 0x00000042); // AhciPortIssueInitCommands

    //1 Reinitialize all the commands to send
    ChannelExtension->DeviceInitCommands.CommandToSend = 0;
    reservedSlotInUse = InterlockedBitTestAndSet((LONG*)&ChannelExtension->StateFlags, 3);    //ReservedSlotInUse field is at bit 3

    if (reservedSlotInUse == 1) {

        // the process has been started.
        RecordExecutionHistory(ChannelExtension, 0x10010042); // AhciPortIssueInitCommands process already started
        return;
    }

    // acquire active reference for process of issuing init commands
    // note: Port should be in Active state during device start.
    if (PortAcquireActiveReference(ChannelExtension, NULL, NULL)) {
        ChannelExtension->StateFlags.RestorePreservedSettingsActiveReferenced = 1;
    }

    //2 Start the first command
    IssueInitCommands(ChannelExtension, NULL);

    //3 Starts processing the command. Only need to do the first command if it exists. all others will be done by processing completion routine.
    if (ChannelExtension->Local.Srb.SrbExtension != NULL) {
        PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension((PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb);
        if (srbExtension->AtaFunction != 0) {
            AhciProcessIo(ChannelExtension, (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb, FALSE);
        }
    }

    return;
}

VOID
AhciPortStartTelemetry(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
/*++

Routine Description:

    This function logs telemetry for AHCI port start status.

Arguments:

    AdapterExtension - Pointer to the device extension for adapter.

Return Value:

    None.

--*/
{
    NT_ASSERT(AdapterExtension->InRunningPortsProcess == FALSE);

    for (ULONG i = 0; i <= AdapterExtension->HighestPort; ++i) {
        PAHCI_CHANNEL_EXTENSION channelExtension = AdapterExtension->PortExtension[i];
        if (channelExtension != NULL) {
            BOOLEAN portStartSuccess = (channelExtension->StartState.ChannelNextStartState == StartComplete) ? (TRUE) : (FALSE);
            AhciTelemetryLogPortStart(channelExtension,
                                      (PSTOR_ADDRESS)&(channelExtension->DeviceExtension->DeviceAddress),
                                      ((portStartSuccess) ? (AhciTelemetryEventIdPortStartSuccess) : (AhciTelemetryEventIdPortRunningStartFail)),
                                      ((portStartSuccess) ? ("Port Start Success") : ("Port Start Failed")),
                                      ((portStartSuccess) ? (0) : (AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING)),
                                      NULL,
                                      0
                                      );
        }
    }

    return;
}

VOID
AhciPortMarkDeviceFailed(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ AHCI_DEVICE_FAILURE_REASON FailureReason,
    _In_ ULONG Flags,
    _In_ BOOLEAN SuppressFailure
    )
/*++

Routine Description:

    This function will call storport API to mark failed device.

    When caller passes TRUE for SuppressFailure, this function will
    only call storport API to mark failed device once; otherwise,
    this function will always call storport API to mark failed device.

Arguments:

    ChannelExtension - Pointer to the device extension for channel.

    FailureReason - Indicates device failure reason.

    Flags - Indicates whether remove failed device.

    SuppressFailure - Indicates whether suppress failures.

Return Value:

    None.

--*/
{
    NT_ASSERT(ChannelExtension != NULL);

    //
    // Ignore Non-ATA device failures.
    //
    if (!IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters)) {
        return;
    }

    switch (FailureReason) {

        case AhciDeviceFailureTooManyBusChange: {
            if ((!SuppressFailure) ||
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_BUS_CHANGE_WARNING_THROTTLE_MASK) == 0)) {
                StorPortMarkDeviceFailed(ChannelExtension->AdapterExtension,
                                         (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                         Flags,
                                         L"Too many bus change");
            }

            if (SuppressFailure &&
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_BUS_CHANGE_WARNING_THROTTLE_MASK) == 0)) {
                ChannelExtension->DeviceFailureThrottleFlag |= AHCI_BUS_CHANGE_WARNING_THROTTLE_MASK;
            }

            break;
        }

        case AhciDeviceFailureTooManyNCQError: {
            if ((!SuppressFailure) ||
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_NCQ_ERROR_WARNING_THROTTLE_MASK) == 0)) {
                StorPortMarkDeviceFailed(ChannelExtension->AdapterExtension,
                                         (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                         Flags,
                                         L"Too many NCQ error");
            }

            if (SuppressFailure &&
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_NCQ_ERROR_WARNING_THROTTLE_MASK) == 0)) {
                ChannelExtension->DeviceFailureThrottleFlag |= AHCI_NCQ_ERROR_WARNING_THROTTLE_MASK;
            }

            break;
        }

        case AhciDeviceFailureTooManyNonQueuedError: {
            if ((!SuppressFailure) ||
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_NON_QUEUED_ERROR_WARNING_THROTTLE_MASK) == 0)) {
                StorPortMarkDeviceFailed(ChannelExtension->AdapterExtension,
                                         (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                         Flags,
                                         L"Too many NonQueue error");
            }

            if (SuppressFailure &&
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_NON_QUEUED_ERROR_WARNING_THROTTLE_MASK) == 0)) {
                ChannelExtension->DeviceFailureThrottleFlag |= AHCI_NON_QUEUED_ERROR_WARNING_THROTTLE_MASK;
            }

            break;
        }

        case AhciDeviceFailureDeviceStuck: {
            if ((!SuppressFailure) ||
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_DEVICE_STUCK_WARNING_THROTTLE_MASK) == 0)) {
                StorPortMarkDeviceFailed(ChannelExtension->AdapterExtension,
                                         (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                         0,
                                         L"Device may stuck");
            }

            if (SuppressFailure &&
                ((ChannelExtension->DeviceFailureThrottleFlag & AHCI_DEVICE_STUCK_WARNING_THROTTLE_MASK) == 0)) {
                ChannelExtension->DeviceFailureThrottleFlag |= AHCI_DEVICE_STUCK_WARNING_THROTTLE_MASK;
            }

            break;
        }

        default: {
            NT_ASSERT(FALSE);
            break;
        }
    }
}

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4054)
#pragma warning(default:4055)
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif

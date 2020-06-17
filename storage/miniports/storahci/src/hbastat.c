/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    hbastat.c

Abstract:
    This file contains the core logic for managing AHCI controller state transitions.
    Primarily it focuses on starting and stopping the channel and performing resets.

Notes:

Revision History: 

--*/

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning(disable:4214) // bit field types other than int
#pragma warning(disable:4201) // nameless struct/union

#include "generic.h"


__inline
VOID
ForceGenxSpeed (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ ULONG GenNumber
    )
{
    AHCI_SERIAL_ATA_CONTROL sctl;
    AHCI_SERIAL_ATA_ERROR   serr;
    UCHAR                   spd;

    if (GenNumber == 0) {
        return;
    }

    spd = (UCHAR)min(GenNumber, ChannelExtension->AdapterExtension->CAP.ISS);

    sctl.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong);
    if (sctl.SPD == spd) {
        return;
    }

    //Set speed limitation.
    sctl.SPD = spd;
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SCTL.AsUlong, sctl.AsUlong);
    //Reset the port
    AhciCOMRESET(ChannelExtension, ChannelExtension->Px);
    //and clear out SERR
    serr.AsUlong = (ULONG)~0;
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SERR.AsUlong, serr.AsUlong);

    return;
}


BOOLEAN
AhciAdapterReset(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
/*
This function brings the HBA and all ports to the H:Idle and P:Idle states by way of HBA Reset and P:Reset

Called By:
    AhciHwFindAdapter if needed.

It assumes:
    AdapterExtension->ABAR_Address is valid.

It performs:
    //10.4.3    HBA Reset
    //5.3.2.1    P:Reset
    //5.3.2.2    P:Init

Return value:
    BOOLEAN - success or not.  note the many controller registers that hold reset values.
*/
{
    AHCI_Global_HBA_CONTROL ghc;
    PAHCI_MEMORY_REGISTERS  abar = AdapterExtension->ABAR_Address;
    int i;

    if (abar == NULL) {
        return FALSE;
    }

//10.4.3    HBA Reset
  //If the HBA becomes unusable for multiple ports, and a software reset or port reset does not correct the problem, software may reset the entire HBA by setting GHC.HR to ‘1’.  When software sets the GHC.HR bit to ‘1’, the HBA shall perform an internal reset action.
    ghc.HR = 1;
    StorPortWriteRegisterUlong(AdapterExtension, &abar->GHC.AsUlong, ghc.AsUlong);

  //The bit shall be cleared to ‘0’ by the HBA when the reset is complete.  A software write of ‘0’ to GHC.HR shall have no effect.  To perform the HBA reset, software sets GHC.HR to ‘1’ and may poll until this bit is read to be ‘0’, at which point software knows that the HBA reset has completed.
    ghc.AsUlong = StorPortReadRegisterUlong(AdapterExtension, &abar->GHC.AsUlong);
    //5.2.2.1    H:Init
    //5.2.2.2    H:WaitForAhciEnable
    for (i = 0;(i < 50) && (ghc.HR == 1); i++) {
        StorPortStallExecution(20000);  //20 milliseconds
        ghc.AsUlong = StorPortReadRegisterUlong(AdapterExtension, &abar->GHC.AsUlong);
    }

    //If the HBA has not cleared GHC.HR to ‘0’ within 1 second of software setting GHC.HR to ‘1’, the HBA is in a hung or locked state.
    if(i == 50) {
        AdapterExtension->ErrorFlags = (1 << 29);
        return FALSE;
    }

    //When GHC.HR is set to ‘1’, GHC.AE, GHC.IE, the IS register, and all port register fields (except PxFB/PxFBU/PxCLB/PxCLBU) that are not HwInit in the HBA’s register memory space are reset.  The HBA’s configuration space and all other global registers/bits are not affected by setting GHC.HR to ‘1’.  Any HwInit bits in the port specific registers are not affected by setting GHC.HR to ‘1’.  The port specific registers PxFB, PxFBU, PxCLB, and PxCLBU are not affected by setting GHC.HR to ‘1’.  If the HBA supports staggered spin-up, the PxCMD.SUD bit will be reset to ‘0’; software is responsible for setting the PxCMD.SUD and PxSCTL.DET fields appropriately such that communication can be established on the Serial ATA link.  If the HBA does not support staggered spin-up, the HBA reset shall cause a COMRESET to be sent on the port

    return TRUE;
}


VOID
AhciCOMRESET(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_PORT Px
    )
/*
PHY Reset:COMRESET
    SCTL.DET Controls the HBA’s device detection and interface initialization.
    DET=1 Performs interface communication initialization sequence to establish communication. This is functionally equivalent to a hard reset and results in the interface being reset and communications reinitialized.  While this field is 1h, COMRESET is transmitted on the interface.
    Software should leave the DET field set to 1h for a minimum of 1 millisecond to ensure that a COMRESET is sent on the interface.
    since we are in 5.3.2.3    P:NotRunning and PxCMD.SUD = ‘0’ does this still take us to P:StartComm?
Called By:
    AhciPortReset,AhciNonQueuedErrorRecovery,P_Running_WaitWhileDET1,AhciHwControlIdeStart
It assumes:
    nothing
It performs:
    5.3.2.11    P:StartComm
    (overview)
    1 Prepare for COMRSET
    2 Perform COMRESET
    3 Clean up after COMRESET
    (details)
    1.1 make sure ST is 0.  DET cannot be altered while ST == 1 as per AHCI 1.1 section 5.3.2.3.
    1.2 Don't allow a comm init to trigger a hotplug
    1.3 Ignore Hotplug events until the channel is started again
    2.1 Perform COMRESET
    2.2 Wait for DET to be set
    3.1 Clear SERR

Affected Variables,Registers:
    CMD.ST, SCTL.DET, CI, SACT, SERR

Return Values:
    none
*/
{
    AHCI_SERIAL_ATA_CONTROL sctl;
    AHCI_COMMAND            cmd;
    AHCI_INTERRUPT_ENABLE   ieOrig;
    AHCI_INTERRUPT_ENABLE   ieTemp;
    AHCI_SERIAL_ATA_STATUS  ssts;
    UCHAR                   i;

    RecordExecutionHistory(ChannelExtension, 0x00000010);//AhciCOMRESET

  //1.1 make sure ST is 0.  DET cannot be altered while ST == 1 as per AHCI 1.1 section 5.3.2.3.
    cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &Px->CMD.AsUlong);
    if(cmd.ST == 1) {
        RecordExecutionHistory(ChannelExtension, 0x10fa0010);   //AhciCOMRESET, PxCMD.ST is 1. Abort
        return;
    }

  //1.2 Don't allow a COMINIT to trigger a hotplug
    ieTemp.AsUlong = ieOrig.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &Px->IE.AsUlong);
    ieTemp.PRCE = 0;        // "PhyRdy Change Interrupt Enable", no generate interrupt with PxIS.PRCS
    ieTemp.PCE = 0;         // "Port Change Interrupt Enable", no generate interrupt with PxIS.PCS
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &Px->IE.AsUlong, ieTemp.AsUlong);

  //1.3 Ignore Hotplug events until the channel is started again
    ChannelExtension->StateFlags.IgnoreHotplugInterrupt = TRUE;

  //2.1 Perform COMRESET
    sctl.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &Px->SCTL.AsUlong);
    sctl.DET = 1;
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &Px->SCTL.AsUlong, sctl.AsUlong);
    //DET=1 Performs interface communication initialization sequence to establish communication. This is functionally equivalent to a hard reset and results in the interface being reset and communications reinitialized.  While this field is 1h, COMRESET is transmitted on the interface.
    //Software should leave the DET field set to 1h for a minimum of 1 millisecond to ensure that a COMRESET is sent on the interface.
    StorPortStallExecution(1000);

    sctl.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &Px->SCTL.AsUlong);
    sctl.DET = 0;
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &Px->SCTL.AsUlong, sctl.AsUlong);

  //2.2 Wait for DET to be set
    // AHCI 10.4.2 After clearing PxSCTL.DET to 0h, software should wait for communication to be re-established as indicated by bit 0 of PxSSTS.DET being set to ‘1’.
    // typically, it will be done in 10ms. max wait 30ms to be safe
    StorPortStallExecution(50);
    ssts.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &Px->SSTS.AsUlong);

    if (ssts.DET == 0) {
        for (i = 0; i < (AHCI_PORT_WAIT_ON_DET_COUNT * 10); i++) {
            StorPortStallExecution(1000);
            ssts.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &Px->SSTS.AsUlong);
            if (ssts.DET != 0) {
                break;
            }
        }
    }

  //2.3 Enable Hotplug again if it was enabled before
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &Px->IE.AsUlong, ieOrig.AsUlong);

  //3.1 Clear SERR
    // AHCI 10.4.2 software should write all 1s to the PxSERR register to clear any bits that were set as part of the port reset.
    StorPortWriteRegisterUlong(ChannelExtension->AdapterExtension, &Px->SERR.AsUlong, (ULONG)~0);

  //3.2 record the count of reset (both internal or asked by port driver)
    ChannelExtension->DeviceExtension[0].IoRecord.TotalResetCount++;

    RecordExecutionHistory(ChannelExtension, 0x10000010);//Exit AhciCOMRESET
}


BOOLEAN
P_NotRunning(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PAHCI_PORT Px
    )
/*
Called By:

It assumes:
    nothing

It performs:
    5.3.2.3    P:NotRunning
    (overview)
    1 Clear CMD.ST
    2 Clear CMD.FRE
    3 Update the Channel Start State

    (details)
    1.1 Clear CMD.ST
    1.2 Verify CR cleared
    1.3 Verify CI cleared
    2.1 Clear CMD.FRE
    2.2 Verify FR cleared
    3.1 Update the Channel Start State

Affected Variables,Registers:
    cmd.ST cmd.CR cmd.FRE cmd.FR are 0 and the controller is in P:NotRunning

Return value:
    FALSE if the ST,CR,FRE, and FRE could not be cleared.  TRUE if success.
    If FALSE is returned, the caller may issue RESET to recover
*/
{
    PAHCI_ADAPTER_EXTENSION adapterExtension;
    BOOLEAN                 supportsCLO;
    AHCI_COMMAND            cmd;
    AHCI_TASK_FILE_DATA     tfd;
    ULONG                   ci;
    UCHAR                   i;

    adapterExtension = ChannelExtension->AdapterExtension;
    supportsCLO = CloResetEnabled(adapterExtension);     //3.1.1 make sure HBA supports Command List Override.

    if (ChannelExtension->StartState.ChannelNextStartState == Stopped) {
        RecordExecutionHistory(ChannelExtension, 0x00030011);   //P_NotRunning, already stopped, nothing to do.
        return TRUE;
    }

    // record this function is called.
    if (supportsCLO) {
        RecordExecutionHistory(ChannelExtension, 0x10810011);   //P_NotRunning, CLO is enabled.
    } else {
        RecordExecutionHistory(ChannelExtension, 0x00000011);   //P_NotRunning, CLO isn't enabled.
    }

  //1.1 Clear CMD.ST
    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &Px->CMD.AsUlong);
    //AHCI 10.3.2 on FRE it says:
    //Software shall not clear this [FRE] bit while PxCMD.ST remains set to ‘1’."
    //System software places a port into the idle state by clearing PxCMD.ST and waiting for PxCMD.CR to return ‘0’ when read.
    cmd.ST = 0;
    StorPortWriteRegisterUlong(adapterExtension, &Px->CMD.AsUlong, cmd.AsUlong);

  //1.1.1 Update the Channel Start State after we ask the channel to stop
    ChannelExtension->StartState.ChannelNextStartState = Stopped;

    StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Port Stopped\n", ChannelExtension->PortNumber);

    if (supportsCLO) {
        // AHCI 3.3.7 make sure PxCMD.ST is 0.
        for (i = 1; i < 101; i++) {
            cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &Px->CMD.AsUlong);
            if (cmd.ST == 0) {
                break;
            }
            StorPortStallExecution(5000);  //5 milliseconds
        }
        if (i == 101) {
            RecordExecutionHistory(ChannelExtension, 0x10820011); //P_NotRunning, After 500ms of writing 0 to PxCMD.ST, it's still 1.
        }

        tfd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->TFD.AsUlong);
        if (tfd.STS.BSY) {
          // AHCI 3.3.7 Command List Override (CLO):  Setting this bit to ‘1’ causes PxTFD.STS.BSY and PxTFD.STS.DRQ to be cleared to ‘0’.
          // This allows a software reset to be transmitted to the device regardless of whether the BSY and DRQ bits are still set in the PxTFD.STS register.

          // Do this to make sure the port can stop
            cmd.CLO = 1;
            StorPortWriteRegisterUlong(adapterExtension, &Px->CMD.AsUlong, cmd.AsUlong);
        }
    }

  //1.2 Verify CR cleared
    //AHCI 10.1.2 - 3: wait cmd.CR to be 0. Software should wait at least 500 milliseconds for this to occur.
    for (i = 1; i < 101; i++) {
        cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &Px->CMD.AsUlong);
        if ( (cmd.CR == 0) && (cmd.ST == 0) ) {
            break;
        }
        StorPortStallExecution(5000);  //5 milliseconds
    }

    //AHCI 10.4.2 If PxCMD.CR or PxCMD.FR do not clear to ‘0’ correctly, then software may attempt a port reset or a full HBA reset to recover.
    if (i == 101) {
        RecordExecutionHistory(ChannelExtension, 0x10350011); // P_NotRunning, PxCMD.CR or PxCMD.FR do not clear to ‘0’ correctly
        return FALSE;
    }

  //1.3 Verify CI cleared
    //This must have the effect of clearing CI as per AHCI section 3.3.14
    for (i = 1; i < 101; i++) {
        ci = StorPortReadRegisterUlong(adapterExtension, &Px->CI);
        if (ci == 0) {
            break;
        }
        StorPortStallExecution(50);  //50 microseconds
    }

    //If CI does not clear to ‘0’ correctly abort the stop
    if (i == 101) {
        RecordExecutionHistory(ChannelExtension, 0x10360011); // P_NotRunning, CI does not clear to ‘0’
        return FALSE;
    }

  //2.1 Clear CMD.FRE
    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &Px->CMD.AsUlong);
    if( (cmd.FRE | cmd.FR) != 0 ) {
        //If PxCMD.FRE is set to ‘1’, software should clear it to ‘0’ and wait at least 500 milliseconds for PxCMD.FR to return ‘0’ when read.
        //AHCI 10.3.2 Software shall not clear this bit while PxCMD.ST or PxCMD.CR is set to ‘1’.
        cmd.FRE = 0;
        StorPortWriteRegisterUlong(adapterExtension, &Px->CMD.AsUlong, cmd.AsUlong);

        if (supportsCLO) {
            tfd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->TFD.AsUlong);
            if (tfd.STS.BSY) {
                NT_ASSERT(FALSE);     //don't expect BSY is set again
                cmd.CLO = 1;
                StorPortWriteRegisterUlong(adapterExtension, &Px->CMD.AsUlong, cmd.AsUlong);
            }
        }

        //Software should wait at least 500 milliseconds for this to occur.
        for (i = 1; i < 101; i++) {
            cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &Px->CMD.AsUlong);
            if( (cmd.CR == 0) && (cmd.FR == 0) && (cmd.ST == 0) && (cmd.FRE == 0) ) {
                break;
            }
            StorPortStallExecution(5000);    //  5 milliseconds
        }

        if ( i == 101 ) {  //If PxCMD.CR or PxCMD.FR do not clear to ‘0’ correctly, then software may attempt a port reset or a full HBA reset to recover.
            if (supportsCLO) {
                RecordExecutionHistory(ChannelExtension, 0x10380011); // P_NotRunning, PxCMD.CR or PxCMD.FR do not clear to ‘0’, CLO enabled
            } else {
                RecordExecutionHistory(ChannelExtension, 0x10370011); // P_NotRunning, PxCMD.CR or PxCMD.FR do not clear to ‘0’, CLO not enabled
            }
            return FALSE;
        }
    }

  //2.2 wait for CLO to clear
    // AHCI 3.3.7 Software must wait for CLO to be cleared to ‘0’ before setting PxCMD.ST to ‘1’.
    // register bit CLO might be set in this function, and it should have been cleared before function exit.
    if ( supportsCLO && (cmd.CLO == 0) ) {
        for (i = 1; i < 101; i++) {
            cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &Px->CMD.AsUlong);
            if( (cmd.CLO == 0) ) {
                break;
            }
            StorPortStallExecution(5000);    //  5 milliseconds
        }

        if ( i == 101 ) {
            RecordExecutionHistory(ChannelExtension, 0x10390011); // P_NotRunning, PxCMD.CLO not clear to ‘0’, CLO enabled
            return FALSE;
        }
    }

    RecordExecutionHistory(ChannelExtension, 0x10000011); //Exit P_NotRunning, port stopped

    return TRUE;
}


VOID
AhciAdapterRunAllPorts(
    _In_ PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    ULONG i;

    //only start with the first one. callback routine will go over all ports.
    for (i = 0; i <= AdapterExtension->HighestPort; i++) {
        if (AdapterExtension->PortExtension[i] != NULL) {
            AdapterExtension->InRunningPortsProcess = TRUE;
            P_Running_StartAttempt(AdapterExtension->PortExtension[i], TRUE);
            return;
        }
    }

    return;
}

VOID
RunNextPort(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    )
{
    PAHCI_ADAPTER_EXTENSION adapterExtension;
    PAHCI_CHANNEL_EXTENSION nextPortExtension;
    ULONG i;

    adapterExtension = ChannelExtension->AdapterExtension;
    nextPortExtension = NULL;

    //starting all ports process either not started or already completed.
    if (!adapterExtension->InRunningPortsProcess) {
        return;
    }

    //get next port extension
    for (i = (ChannelExtension->PortNumber + 1); i <= adapterExtension->HighestPort; i++) {
        if (adapterExtension->PortExtension[i] != NULL) {
            nextPortExtension = adapterExtension->PortExtension[i];
            break;
        }
    }

    // run next port or declare the ports running process completed.
    if (nextPortExtension != NULL) {
        if ((nextPortExtension->StartState.ChannelNextStartState == 0) ||
            (nextPortExtension->StartState.ChannelNextStartState == Stopped)) {
            P_Running_StartAttempt(nextPortExtension, AtDIRQL);
        } else {
            // return, do nothing here as the Port Start effort has been made.
            // RunNextPort() is only used in AhciAdapterRunAllPorts() which is called by AhciHwFindAdapter.
            // In other cases, P_Running_StartAttempt() will be called directly for each port.
            return;
        }
    } else {
        //starting all ports process is completed.
        adapterExtension->InRunningPortsProcess = FALSE;

        //
        // Note: If we are at DIRQL when get here, it implies previous port start finishes quickly since no
        //       timer scheduled during port start. Then there is no need to log the port start time.
        //
        if (!AtDIRQL) {
            AhciPortStartTelemetry(adapterExtension);
        }
    }

    return;
}

VOID
P_Running_StartAttempt(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    )
/*

Called By:
It assumes:
    Channel Start may already be in progress (not function reentrant)
It performs:
    1 Initializes the Channel Start state machine
    2 Starts the Channel Start state machine

Affected Variables/Registers:
    none
*/
{
    STOR_LOCK_HANDLE    lockhandle = {InterruptLock, 0};

    if (!AtDIRQL) {
        AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    }

  //1 Initializes the Channel Start state machine
    ChannelExtension->StartState.ChannelNextStartState = WaitOnDET;
    ChannelExtension->StartState.ChannelStateDETCount = 0;
    ChannelExtension->StartState.ChannelStateDET1Count = 0;
    ChannelExtension->StartState.ChannelStateDET3Count = 0;
    ChannelExtension->StartState.ChannelStateFRECount = 0;
    ChannelExtension->StartState.ChannelStateBSYDRQCount = 0;
    ChannelExtension->StartState.AtDIRQL = AtDIRQL ? 1 : 0;
    ChannelExtension->StartState.DirectStartInProcess = 1;

    if (!AtDIRQL) {
        AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    }

  //2 Starts the Channel Start state machine
    P_Running(ChannelExtension, FALSE);
    return;
}

VOID
P_Running_Callback(
    _In_ PVOID AdapterExtension,
    _In_opt_ PVOID ChannelExtension
    )
{
    PAHCI_CHANNEL_EXTENSION channelExtension = (PAHCI_CHANNEL_EXTENSION)ChannelExtension;
    ULONG                   callbackIndex = 0;

    if (channelExtension == NULL) {
        NT_ASSERT(FALSE);
        return;
    }

    NT_ASSERT(AdapterExtension == (PVOID)(channelExtension->AdapterExtension));

    UNREFERENCED_PARAMETER(AdapterExtension);

    callbackIndex = channelExtension->StartState.ChannelStateDETCount +
                    channelExtension->StartState.ChannelStateDET1Count +
                    channelExtension->StartState.ChannelStateDET3Count +
                    channelExtension->StartState.ChannelStateFRECount +
                    channelExtension->StartState.ChannelStateBSYDRQCount;

    // only clear the bit if this is the first timer callback in Port Start process
    if (callbackIndex == 1) {
        STOR_LOCK_HANDLE    lockhandle = {InterruptLock, 0};

        AhciInterruptSpinlockAcquire(AdapterExtension, channelExtension->PortNumber, &lockhandle);

        if (channelExtension->StartState.AtDIRQL == 1) {
            channelExtension->StartState.AtDIRQL = 0;
        }
        if (channelExtension->StartState.DirectStartInProcess == 1) {
            channelExtension->StartState.DirectStartInProcess = 0;
        }

        AhciInterruptSpinlockRelease(AdapterExtension, channelExtension->PortNumber, &lockhandle);
    }

    P_Running(channelExtension, TRUE);

    return;
}


BOOLEAN
P_Running(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*
    The purpose of this function is to verify and drive the Start Channel state machine

Called By:
    P_Running_StartAttempt

    Through StorPortRequestTimer which is set up by each of the Start Channel state functions:
            P_Running_WaitOnDET
            P_Running_WaitWhileDET1
            P_Running_WaitOnDET3
            P_Running_WaitOnFRE
            P_Running_WaitOnBSYDRQ
It assumes:
    Nothing

It performs:
    (overview)
    1 Verify Start can/needs to be done
    2 Run the Start Channel state machine

    (details)
    1.1 First, is the channel initialized.  Would turning start on blow up the machine?
    1.2 Next, is the port somehow already running?
    1.3 Make sure the device knows it is supposed to be spun up
    1.4 Check to make sure that FR and CR are both 0.  Attempt to bring the controller into a consistent state by stopping the controller.
    1.5 CMD.ST has to be set, when that happens check to see that PxSSTS.DET is not ‘4h’

    2.1 Dispatch to the current state (states are responsible for selecting the next state)

Affected Variables/Registers:
    CMD

Return Values:
    TRUE if the state machine can be run
    FALSE if the state machine can't
*/
{

    AHCI_COMMAND            cmd;
    AHCI_SERIAL_ATA_STATUS  ssts;
    PAHCI_PORT              px = ChannelExtension->Px;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000015);//P_Running
    }

  //1.1 First, is the channel initialized.  Would turning start on blow up the machine?
    if( !IsPortStartCapable(ChannelExtension) ) {
        RecordExecutionHistory(ChannelExtension, 0x10120015);//No Channel Resources
        RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));
        return FALSE;
    }

    if ( TimerCallbackProcess && (ChannelExtension->StartState.DirectStartInProcess == 1) ) {
        RecordExecutionHistory(ChannelExtension, 0x10130015);//This is timer callback and a direct port start process has been started separately, bail out this one.
        RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));
        return FALSE;
    }

  //1.2 Next, is the port somehow already running?
    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &px->CMD.AsUlong);
    if( (cmd.ST == 1) && (cmd.CR == 1) && (cmd.FRE == 1) && (cmd.FR == 1) ) {
        ChannelExtension->StartState.ChannelNextStartState = StartComplete;
        RecordExecutionHistory(ChannelExtension, 0x30000015);//Channel Already Running
        RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));
        return TRUE;
    }

  //1.3 Make sure the device knows it is supposed to be spun up
    cmd.SUD = 1;
    StorPortWriteRegisterUlong(adapterExtension, &px->CMD.AsUlong, cmd.AsUlong);

  //1.4 Check to make sure that FR and CR are both 0.  If not then ST and/or FRE are 0 which a bad scenario.
    if ( ( (cmd.FR == 1) && (cmd.FRE == 0) ) ||
         ( (cmd.CR == 1) && (cmd.ST  == 0) ) ) {
        //Attempt to bring the controller into a consistent state by stopping the controller.
        if ( !P_NotRunning(ChannelExtension, ChannelExtension->Px) ) {
            RecordExecutionHistory(ChannelExtension, 0x10880015);   //CR or FR 1 when ST or FRE 0
            AhciCOMRESET(ChannelExtension, ChannelExtension->Px);   //Issue COMRESET to recover

            if (((cmd.FR == 1) && (cmd.FRE == 0)) ||
                ((cmd.CR == 1) && (cmd.ST == 0))) {
                //In case the error cannot be recovered, fail the port start process.
                P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
                RecordExecutionHistory(ChannelExtension, 0x10150014);   //cmd.FR or cmd.CR cannot be stopped.
                return FALSE;
            } else {
                //Reset StartState to start the port again.
                ChannelExtension->StartState.ChannelNextStartState = WaitOnDET;
            }
        } else {
            //Reset StartState to start the port again.
            ChannelExtension->StartState.ChannelNextStartState = WaitOnDET;
        }
    }

  //1.5 CMD.ST has to be set, when that happens check to see that PxSSTS.DET is not ‘4h’
    ssts.AsUlong = StorPortReadRegisterUlong(adapterExtension, &px->SSTS.AsUlong);
    if( ssts.DET == 0x4) {
        P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
        RecordExecutionHistory(ChannelExtension, 0x10150015);   //Channel Disabled
        return FALSE;
    }

  //2.1 Ok, run the Start Channel State Machine
    switch(ChannelExtension->StartState.ChannelNextStartState) {

        case WaitOnDET:
            P_Running_WaitOnDET(ChannelExtension, TimerCallbackProcess);
            break;
        case WaitWhileDET1:
            P_Running_WaitWhileDET1(ChannelExtension, TimerCallbackProcess);
            break;
        case WaitOnDET3:
            P_Running_WaitOnDET3(ChannelExtension, TimerCallbackProcess);
            break;
        case WaitOnFRE:
            P_Running_WaitOnFRE(ChannelExtension, TimerCallbackProcess);
            break;
        case WaitOnBSYDRQ:
            P_Running_WaitOnBSYDRQ(ChannelExtension, TimerCallbackProcess);
            break;
        default:
            // Above cases cover all intermediate states. If a port start process has been completed, just exit.
            break;
    }

    // record this function is called.
    RecordExecutionHistory(ChannelExtension, 0x10000015);   //Exit P_Running
    return TRUE;
}

VOID
P_Running_WaitOnDET(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*
    Search for Device Activity phase.  Use DET and IPM for any signs of life as defined in AHCI 1.2 section 10.1.2 and 10.3.1.
    Polled 3 times in 30ms

Called By:
    P_Running
It assumes:
    Controller is start capable
It performs:
    (overview)
    1 Look for signs of life while DET == 0
    2 Look for signs of life while DET == 1
    3 Look for signs of life which is DET == 3
    (details)
    1.1 When looking for device presence, seeing link level power management shall count as device present as per 10.3.1
    1.2 Wait for 1 second for DET to show signs of life
    1.3 After 30ms give up
    2.1 When looking for device presence, seeing link level power management shall count as device present as per 10.3.1
    2.2 Otherwise continue on to WaitOnDET1
    3.1 If DET==3 we are ready for WaitOnFRE already.

Affected Variables/Registers:
    none
*/
{
    AHCI_SERIAL_ATA_STATUS ssts;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;
    UCHAR                   waitMaxCount = AHCI_PORT_WAIT_ON_DET_COUNT;

    if (IsPortD3ColdEnabled(ChannelExtension)) {
        // if D3 Cold is enabled, a device was there; Add additional wait time to make sure device is able to get ready after a possible power on.
        waitMaxCount += 30;     // wait timer is 10ms, add 300ms for safe
    } else if (IsDumpMode(adapterExtension)) {
        //
        // If we're on the dump stack we may have crashed while the device was
        // in an indeterminant state so give it some more time to get ready.
        //
        waitMaxCount += 30;
    }

WaitOnDET_Start:
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000016);//P_Running_WaitOnDET
    }

  //1 Look for signs of life while DET == 0
    ssts.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->SSTS.AsUlong);

    if (ssts.DET == 0)  { //When a COMRESET is sent to the device the PxSSTS.DET field shall be cleared to 0h.

      //1.1 When looking for device presence, seeing link level power management shall count as device present as per 10.3.1
        if ((ssts.IPM == 0x2) || (ssts.IPM == 0x6) || (ssts.IPM == 0x8)) {
            ChannelExtension->StartState.ChannelNextStartState = WaitOnFRE;
            RecordExecutionHistory(ChannelExtension, 0x10800016);//P_Running_WaitOnDET is 0 w/ LPM activity, goto FRE
            P_Running_WaitOnFRE(ChannelExtension, TimerCallbackProcess);
            return;
      //1.3 After 30ms give up (spec requires 10ms, use 30ms for safety). There is no device connected.
        //from SATA-IO spec 3.0 - 15.2.2.2:
        //If a device is present, the Phy shall detect device presence within 10 ms of a power-on reset (i.e. COMINIT shall be returned within 10 ms of an issued COMRESET).
        } else if (ChannelExtension->StartState.ChannelStateDETCount > waitMaxCount) {
            P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
            RecordExecutionHistory(ChannelExtension, 0x10ff0016);//P_Running_WaitOnDET Timed out, No Device
            return;
      //1.2 Wait for 30ms for DET to show signs of life
        } else {
            ChannelExtension->StartState.ChannelStateDETCount++;
            RecordExecutionHistory(ChannelExtension, 0x10810016);//P_Running_WaitOnDET is 0, still waiting

            if (IsDumpMode(adapterExtension)) {
                StorPortStallExecution(10000); //10 milliseconds
                goto WaitOnDET_Start;
            } else {
                ULONG status;
                status = StorPortRequestTimer(adapterExtension, ChannelExtension->StartPortTimer, P_Running_Callback, ChannelExtension, 10000, 0);     //10 milliseconds
                if ((status != STOR_STATUS_SUCCESS) && (status != STOR_STATUS_BUSY)) {
                    StorPortStallExecution(10000); //10 milliseconds
                    goto WaitOnDET_Start;
                }
            }
            return;
        }

    } else {
      //2 Look for signs of life while DET == 1
        if (ssts.DET == 1)  {
          //2.1 When looking for device presence, seeing link level power management shall count as device present as per 10.3.1
            if ((ssts.IPM == 0x2) || (ssts.IPM == 0x6) || (ssts.IPM == 0x8)) {
                ChannelExtension->StartState.ChannelNextStartState = WaitOnFRE;
                RecordExecutionHistory(ChannelExtension, 0x10a00016);//P_Running_WaitOnDET is 1 w/ LPM activity, goto FRE
                P_Running_WaitOnFRE(ChannelExtension, TimerCallbackProcess);
                return;
          //2.2 Otherwise continue on to WaitOnDET1
            } else {
                ChannelExtension->StartState.ChannelNextStartState = WaitWhileDET1;
                RecordExecutionHistory(ChannelExtension, 0x10a10016);//P_Running_WaitOnDET is 1
                P_Running_WaitWhileDET1(ChannelExtension, TimerCallbackProcess);
                return;
            }
      //3.1 If DET==3 we are ready for WaitOnFRE already.
        } else if (ssts.DET == 3)  {
            ChannelExtension->StartState.ChannelNextStartState = WaitOnFRE;
            RecordExecutionHistory(ChannelExtension, 0x10a20016);//P_Running_WaitOnDET is 3
            P_Running_WaitOnFRE(ChannelExtension, TimerCallbackProcess);
            return;

        } else {
            P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
            RecordExecutionHistory(ChannelExtension, 0x10a30016);//P_Running_WaitOnDET is Bogus, aborting
            return;
        }
    }
}

VOID
P_Running_WaitWhileDET1(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*
    Waiting on establishment of link level communications phase.
    A '1' should become a '3'.  If it doesn't, help it along.
    NOTE:   When a COMINIT is received, the PxSSTS.DET field shall be set to 1h.
            That means a device is detected, but communications has not finished
    Polled 100 times in 1 second

Called By:
    P_Running
It assumes:
    Channel is start capable
    DET was previously the value of 1
It performs:
    (overview)
    1 Look for signs of life which is DET == 3
    2 Look for signs of live while DET == 1
    (details)
    1.1 If DET moves from 1 to 3, go to WaitOnFRE
    2.1 Wait for 1 second for DET to become 3
    2.2 After 1 second of waiting, force the controller to 150MB/s speeds and go to WaitOnDET3
Affected Variables/Registers:
    SCTL, SERR
*/
{
    AHCI_SERIAL_ATA_STATUS  ssts;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

WaitWhileDET1_Start:
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000017);//P_Running_WaitWhileDET1
    }

  //1.1 If DET moves from 1 to 3, go to WaitOnFRE
    ssts.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->SSTS.AsUlong);

    if (ssts.DET == 3) {
        ChannelExtension->StartState.ChannelNextStartState = WaitOnFRE;
        RecordExecutionHistory(ChannelExtension, 0x10a20017);//P_Running_WaitWhileDET1 done
        P_Running_WaitOnFRE(ChannelExtension, TimerCallbackProcess);
        return;

    } else {
      //2.2 After 1 second of waiting, force the controller to 150MB/s speeds and go to WaitOnDET3
        if (ChannelExtension->StartState.ChannelStateDET1Count > 100) {
            //A very wise woman once taught me this trick
            //it is possible that the device is not handling speed negotiation very well
            //help it out by allowing only 150MB/s
            ForceGenxSpeed(ChannelExtension, 1);

            ChannelExtension->StartState.ChannelNextStartState = WaitOnDET3;
            RecordExecutionHistory(ChannelExtension, 0x100a0017);//P_Running_WaitWhileDET1 timed out, speed stepping down
            if (IsDumpMode(adapterExtension)) {
                StorPortStallExecution(10000); //10 milliseconds
                P_Running_WaitOnDET3(ChannelExtension, TimerCallbackProcess);
            } else {
                ULONG status;
                status = StorPortRequestTimer(adapterExtension, ChannelExtension->StartPortTimer, P_Running_Callback, ChannelExtension, 10000, 0);     //10 milliseconds
                if ((status != STOR_STATUS_SUCCESS) && (status != STOR_STATUS_BUSY)) {
                    StorPortStallExecution(10000); //10 milliseconds
                    P_Running_WaitOnDET3(ChannelExtension, TimerCallbackProcess);
                }
            }
            return;
      //2.1 Wait for DET to become 3 for 1 second
        } else {
            ChannelExtension->StartState.ChannelStateDET1Count++;
            RecordExecutionHistory(ChannelExtension, 0x10600017);//P_Running_WaitWhileDET1 still waiting
            if (IsDumpMode(adapterExtension)) {
                StorPortStallExecution(10000); //10 milliseconds
                goto WaitWhileDET1_Start;
            } else {
                ULONG status;
                status = StorPortRequestTimer(adapterExtension, ChannelExtension->StartPortTimer, P_Running_Callback, ChannelExtension, 10000, 0);     //10 milliseconds
                if ((status != STOR_STATUS_SUCCESS) && (status != STOR_STATUS_BUSY)) {
                    StorPortStallExecution(10000); //10 milliseconds
                    goto WaitWhileDET1_Start;
                }
            }
            return;
        }
    }
}

VOID
P_Running_WaitOnDET3(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*
    Done monkeying phase.
    From here on out only a DET of 3 will do

Called By:
    P_Running

It assumes:
    Channel is start capable

It performs:
    (overview)
    1 Look for signs of life which are DET == 3
    (details)
    1.1 Wait for 1 second until DET becomes 3
    1.2 After waiting for 1 second, give up on starting the channel
    1.3 If DET==3 we are ready for WaitOnFRE

Affected Variables/Registers:
    none
*/
{
    AHCI_SERIAL_ATA_STATUS  ssts;
    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

WaitOnDET3_Start:
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000018);//P_Running_WaitOnDET3
    }

  //1.2 After waiting for 1 second, give up on starting the channel
    ssts.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->SSTS.AsUlong);

    if (ssts.DET != 3) {

        if (ChannelExtension->StartState.ChannelStateDET3Count > 100) {
            P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
            RecordExecutionHistory(ChannelExtension, 0x10ff0018);//P_Running_WaitOnDET3 timed out
            return;
      //1.1 Wait for 1 second until DET becomes 3
        } else {
            ChannelExtension->StartState.ChannelStateDET3Count++;
            RecordExecutionHistory(ChannelExtension, 0x10080018);//P_Running_WaitOnDET3 still waiting
            if (IsDumpMode(adapterExtension)) {
                StorPortStallExecution(10000); //10 milliseconds
                goto WaitOnDET3_Start;
            } else {
                ULONG status;
                status = StorPortRequestTimer(adapterExtension, ChannelExtension->StartPortTimer, P_Running_Callback, ChannelExtension, 10000, 0);     //10 milliseconds
                if ((status != STOR_STATUS_SUCCESS) && (status != STOR_STATUS_BUSY)) {
                    StorPortStallExecution(10000); //10 milliseconds
                    goto WaitOnDET3_Start;
                }
            }
            return;
        }
  //1.3 If DET==3 we are ready for WaitOnFRE
    } else {
        ChannelExtension->StartState.ChannelNextStartState = WaitOnFRE;
        RecordExecutionHistory(ChannelExtension, 0x100a0018);//P_Running_WaitOnDET3 Success
        P_Running_WaitOnFRE(ChannelExtension, TimerCallbackProcess);
        return;
    }
}

VOID
P_Running_WaitOnFRE(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*
    Start the Receive buffer phase
    Wait for confirmation is a 'nice to have' but isn't necessary.

Called By:
    P_Running

It assumes:
    DET == 3 or IPM == 02 or 06

It performs:
    (overview)
    1 Set FRE
    2 Wait for 50ms for FR to reflect running status
    3 Move to WaitOnBSYDRQ
    (details)
    1.1 Set FRE
    2.1 Wait for 50ms for FR to reflect running status
    3.1 Move to WaitOnBSYDRQ
Affected Variables/Registers:
    CMD
*/
{
    AHCI_COMMAND cmd;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

WaitOnFRE_Start:
    if (LogExecuteFullDetail(adapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x00000019);//P_Running_FRE
    }

    // There is a device connected when we get here. Trace how long the port need to set DET from 0 to 1.
    if (ChannelExtension->StartState.ChannelStateDETCount > 1) {
        DebugPrint((1, "STORAHCI: Channel %u: waited more than %u ms for DET 0 -> 1\n",
                      ChannelExtension->PortNumber,
                      10 * ChannelExtension->StartState.ChannelStateDETCount));
    }

  //1.1 Set FRE
    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong);
    cmd.FRE = 1;
    StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong, cmd.AsUlong);

    cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong);

    if (cmd.FR == 1) {
      //3.1 Move to WaitOnBSYDRQ
        ChannelExtension->StartState.ChannelNextStartState = WaitOnBSYDRQ;
        RecordExecutionHistory(ChannelExtension, 0x000a0019);//P_Running_WaitOnFRE Success
        P_Running_WaitOnBSYDRQ(ChannelExtension, TimerCallbackProcess);
        return;

    } else {
      //3.1 Move to WaitOnBSYDRQ
        if (ChannelExtension->StartState.ChannelStateFRECount > 5) {
            ChannelExtension->StartState.ChannelNextStartState = WaitOnBSYDRQ;
            RecordExecutionHistory(ChannelExtension, 0x00ff0019);//P_Running_WaitOnFRE timed out
            P_Running_WaitOnBSYDRQ(ChannelExtension, TimerCallbackProcess);
            return;
      //2.1 Wait for 50ms for FR to reflect running status
        } else {
            ChannelExtension->StartState.ChannelStateFRECount++;
            RecordExecutionHistory(ChannelExtension, 0x00080019);//P_Running_WaitOnFRE still waiting
            if (IsDumpMode(adapterExtension)) {
                StorPortStallExecution(10000); //10 milliseconds
                goto WaitOnFRE_Start;
            } else {
                ULONG status;
                status = StorPortRequestTimer(adapterExtension, ChannelExtension->StartPortTimer, P_Running_Callback, ChannelExtension, 10000, 0);     //10 milliseconds
                if ((status != STOR_STATUS_SUCCESS) && (status != STOR_STATUS_BUSY)) {
                    StorPortStallExecution(10000); //10 milliseconds
                    goto WaitOnFRE_Start;
                }
            }
            return;
        }
    }
}

VOID
P_Running_WaitOnBSYDRQ(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*
    Home stretch

Called By:
    P_Running
It assumes:
    DET == 3 or IPM == 02 or 06
It performs:
    (overview)
    1 Enable BSY and DRQ to go to 0
    2 Wait for BSY and DRQ to go to 0
    3 Set ST to 1
    (details)
    1.1 Clear serr.DIAG.X
    2.1 Wait for the rest of the 10 seconds for BSY and DRQ to clear
    2.2 After a total amount of 3 seconds working on the start COMRESET
    2.3 After waiting for the rest of the 4 seconds for BSY and DRQ to clear, give up
    3.1 Set ST to 1
Affected Variables/Registers:

*/
{
    AHCI_COMMAND cmd;
    AHCI_TASK_FILE_DATA tfd;
    AHCI_SERIAL_ATA_ERROR serr;
    USHORT totalstarttime;

    PAHCI_ADAPTER_EXTENSION adapterExtension = ChannelExtension->AdapterExtension;

WaitOnBSYDRQ_Start:
    RecordExecutionHistory(ChannelExtension, 0x0000001a);//P_Running_WaitOnBSYDRQ
    tfd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->TFD.AsUlong);

    //1.1 Enable BSY and DRQ to go to 0
    if ( (tfd.STS.BSY) || (tfd.STS.DRQ) ) {
        //When [serr.DIAG.X is] set to one this bit indicates a COMINIT signal was received.  This bit is reflected in the P0IS.PCS bit.
        //to allow the TFD to be updated serr.DIAG.X must be cleared.
        serr.AsUlong = 0;
        serr.DIAG.X = 1;
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->SERR.AsUlong, serr.AsUlong);
    }
    //3.1 Set ST to 1
    if ( ( tfd.STS.BSY == 0) && ( tfd.STS.DRQ == 0) ) {
        STOR_LOCK_HANDLE lockhandle = { InterruptLock, 0 };
        BOOLEAN needSpinLock;

        if ( TimerCallbackProcess && (ChannelExtension->StartState.DirectStartInProcess == 1) ) {
            //This is timer callback and a direct port start process has been started separately, bail out this one.
            RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));
            return;
        }

        needSpinLock = TimerCallbackProcess || (ChannelExtension->StartState.AtDIRQL == 0);

        if (needSpinLock) {
            AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
        }

        //3.2 Enable Interrupts on the channel (AHCI 1.1 Section 10.1.2 - 7)
        PortClearPendingInterrupt(ChannelExtension);
        Set_PxIE(ChannelExtension, &ChannelExtension->Px->IE);

        //We made it!  Set ST and start the IO we have collected!
        cmd.AsUlong = StorPortReadRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong);
        cmd.ST = 1;
        StorPortWriteRegisterUlong(adapterExtension, &ChannelExtension->Px->CMD.AsUlong, cmd.AsUlong);

        ChannelExtension->StartState.ChannelNextStartState = StartComplete;
        ChannelExtension->StateFlags.IgnoreHotplugInterrupt = FALSE;

        StorPortDebugPrint(3, "StorAHCI - LPM: Port %02d - Port Started\n", ChannelExtension->PortNumber);

        //
        // Note: If ChannelStateBSYDRQCount is larger than 50, which implies the other counts are
        // all cleared to 0 according to port start state machine implementation. But typically
        // WaitOnBSYDRQ is always the most significant time consuming phase of port start, so the
        // total port start time here is still able to reflect the rough port start status.
        //
        ChannelExtension->TotalPortStartTime = (ULONG)(ChannelExtension->StartState.ChannelStateDETCount +
                                                       ChannelExtension->StartState.ChannelStateDET1Count +
                                                       ChannelExtension->StartState.ChannelStateDET3Count +
                                                       ChannelExtension->StartState.ChannelStateFRECount +
                                                       (ChannelExtension->StartState.ChannelStateBSYDRQCount * 2)) * 10;

        //Start requests on this Port
        ActivateQueue(ChannelExtension, TRUE);

        ChannelExtension->StartState.DirectStartInProcess = 0;

        RecordExecutionHistory(ChannelExtension, 0x100a001a);//Exit P_Running_WaitOnBSYDRQ Succeeded

        if (needSpinLock) {
            AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
        }

        if (ChannelExtension->StateFlags.NeedQDR) {
            StorPortIssueDpc(ChannelExtension->AdapterExtension, &(ChannelExtension->BusChangeDpc), ChannelExtension, NULL);
            ChannelExtension->StateFlags.NeedQDR = FALSE;
        }

        RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));

        return;

    } else {
        //2.3 After waiting for the remainder of the 60 second maximum Channel Start time for BSY and DRQ to clear, give up
        //Some big HDDs takes close to 20 second to spin up
        ULONG portStartTimeoutIn10MS = (AHCI_PORT_START_TIMEOUT_IN_SECONDS * 100);

        // calculate the total time in unit of 10 ms.
        totalstarttime = ChannelExtension->StartState.ChannelStateDETCount  +
                         ChannelExtension->StartState.ChannelStateDET1Count +
                         ChannelExtension->StartState.ChannelStateDET3Count +
                         ChannelExtension->StartState.ChannelStateFRECount  +
                        (ChannelExtension->StartState.ChannelStateBSYDRQCount * 2);

        if ( totalstarttime > portStartTimeoutIn10MS ) {
            P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
            RecordExecutionHistory(ChannelExtension, 0x00ff001a);//P_Running_WaitOnBSYDRQ timed out
           return;
        //2.2 After a total amount of 1 second working on the start COMRESET
        } else if ( ChannelExtension->StartState.ChannelStateBSYDRQCount == 50 ){
            //Stop FRE,FR in preparation for RESET
            if ( !P_NotRunning(ChannelExtension, ChannelExtension->Px) ){
                //It takes 1/2 second for stop to fail.
                //This is taking way too long and the controller is not responding properly. Abort the start.
                P_Running_StartFailed(ChannelExtension, TimerCallbackProcess);
                RecordExecutionHistory(ChannelExtension, 0x00fc001a);//P_Running_WaitOnBSYDRQ Stop Failed on COMRESET
                return;
            }

            //All set, bring down the hammer
            AhciCOMRESET(ChannelExtension, ChannelExtension->Px);

            //Set the timers for time remaining.  This is a best case scenario and the best that can be offered.
            ChannelExtension->StartState.ChannelStateDETCount = 0;
            ChannelExtension->StartState.ChannelStateDET1Count = 0;
            ChannelExtension->StartState.ChannelStateDET3Count = 0;     // won't exceed 100 * 10ms for 1 second
            ChannelExtension->StartState.ChannelStateFRECount = 0;      // won't exceed 5 * 10ms for 50 ms
            ChannelExtension->StartState.ChannelStateBSYDRQCount = 51;  // won't exceed 3000 * 20ms for 60 seconds minus what DET3 and FRE use.

            //Go back to WaitOnDet3
            ChannelExtension->StartState.ChannelNextStartState = WaitOnDET3;
            RecordExecutionHistory(ChannelExtension, 0x00fd001a);//P_Running_WaitOnBSYDRQ crossing 1 second.  COMRESET done, going to WaitOnDET3
            P_Running_WaitOnDET3(ChannelExtension, TimerCallbackProcess);
            return;
        //2.1 Wait for the rest of the 4 seconds for BSY and DRQ to clear
        } else {
            ChannelExtension->StartState.ChannelStateBSYDRQCount++;
            RecordExecutionHistory(ChannelExtension, 0x0008001a);//P_Running_WaitOnBSYDRQ still waiting
            if (IsDumpMode(adapterExtension)) {
                StorPortStallExecution(20000); //20 milliseconds
                goto WaitOnBSYDRQ_Start;
            } else {
                ULONG status;
                status = StorPortRequestTimer(adapterExtension, ChannelExtension->StartPortTimer, P_Running_Callback, ChannelExtension, 20000, 0);     //10 milliseconds
                if ((status != STOR_STATUS_SUCCESS) && (status != STOR_STATUS_BUSY)) {
                    StorPortStallExecution(20000); //20 milliseconds
                    goto WaitOnBSYDRQ_Start;
                }
            }
            return;
        }
    }
}

VOID
P_Running_StartFailed(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN TimerCallbackProcess
    )
/*

Called By:
    P_Running_WaitOnDET
    P_Running_WaitOnDET3
    P_Running_WaitOnBSYDRQ

It assumes:
    nothing

It performs:
    (overview)
    1 clean up internal state structures
    2 complete all the commands that showed up while the start channel failed
    (details)
    1.1 update state machines
    1.2 clear out the programmed slots
    2.1 complete all outstanding commands

Affected Variables/Registers:
    none
*/
{
    STOR_LOCK_HANDLE lockhandle = { InterruptLock, 0 };
    BOOLEAN needSpinLock;

    ++(ChannelExtension->TotalCountRunningStartFailed);
    //
    // Note: If ChannelStateBSYDRQCount is larger than 50, which implies the other counts are
    // all cleared to 0 according to port start state machine implementation. But typically
    // WaitOnBSYDRQ is always the most significant time consuming phase of port start, so the
    // total port start time here is still able to reflect the rough port start status.
    //
    ChannelExtension->TotalPortStartTime = (ULONG)(ChannelExtension->StartState.ChannelStateDETCount +
                                                   ChannelExtension->StartState.ChannelStateDET1Count +
                                                   ChannelExtension->StartState.ChannelStateDET3Count +
                                                   ChannelExtension->StartState.ChannelStateFRECount +
                                                   (ChannelExtension->StartState.ChannelStateBSYDRQCount * 2)) * 10;

    if ( TimerCallbackProcess && (ChannelExtension->StartState.DirectStartInProcess == 1) ) {
        //This is timer callback and a direct port start process has been started separately, bail out this one.
        RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));
        return;
    }

    needSpinLock = TimerCallbackProcess || (ChannelExtension->StartState.AtDIRQL == 0);

    if (needSpinLock) {
        AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    }

    //1.1 update state machines
    ChannelExtension->StartState.ChannelNextStartState = StartFailed;
    ChannelExtension->StateFlags.IgnoreHotplugInterrupt = FALSE;
    ChannelExtension->StateFlags.D3ColdEnabled = FALSE;     //this flag may be set to "TRUE" for last connected device, clear it when no device connected.

    //1.1.1 release initial commands for previous device.
    if (ChannelExtension->DeviceInitCommands.CommandTaskFile != NULL) {
        StorPortFreePool(ChannelExtension->AdapterExtension, (PVOID)ChannelExtension->DeviceInitCommands.CommandTaskFile);
        ChannelExtension->DeviceInitCommands.CommandTaskFile = NULL;
        ChannelExtension->DeviceInitCommands.CommandCount = 0;
        ChannelExtension->DeviceInitCommands.ValidCommandCount = 0;
        ChannelExtension->DeviceInitCommands.CommandToSend = 0;
    }

    if (ChannelExtension->PersistentSettings.Slots > 0) {
        AhciZeroMemory((PCHAR)&ChannelExtension->PersistentSettings, sizeof(PERSISTENT_SETTINGS));
    }

    //1.1.2 clean up cached device information
    AhciZeroMemory((PCHAR)&ChannelExtension->DeviceExtension->SupportedGPLPages, sizeof(ATA_SUPPORTED_GPL_PAGES));
    AhciZeroMemory((PCHAR)&ChannelExtension->DeviceExtension->SupportedCommands, sizeof(ATA_COMMAND_SUPPORTED));

    //1.2 clear out the programmed slots
    ChannelExtension->SlotManager.CommandsToComplete = GetOccupiedSlots(ChannelExtension);
    ChannelExtension->SlotManager.CommandsIssued = 0;
    ChannelExtension->SlotManager.NCQueueSliceIssued = 0;
    ChannelExtension->SlotManager.NormalQueueSliceIssued = 0;
    ChannelExtension->SlotManager.SingleIoSliceIssued = 0;
    ChannelExtension->SlotManager.NCQueueSlice = 0;
    ChannelExtension->SlotManager.NormalQueueSlice = 0;
    ChannelExtension->SlotManager.SingleIoSlice = 0;
    ChannelExtension->SlotManager.HighPriorityAttribute = 0;

    //1.3 Enable Interrupts on the Channel (AHCI 1.1 Section 10.1.2 - 7)
    PortClearPendingInterrupt(ChannelExtension);
    Set_PxIE(ChannelExtension, &ChannelExtension->Px->IE);

    //2.1 Call AhciPortFailAllIos to complete all outstanding commands now that ChannelNextStartState is StartFailed
    AhciPortFailAllIos(ChannelExtension, SRB_STATUS_NO_DEVICE, TRUE);

    ChannelExtension->StartState.DirectStartInProcess = 0;

    if (needSpinLock) {
        AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    }

    RunNextPort(ChannelExtension, (!TimerCallbackProcess && ChannelExtension->StartState.AtDIRQL));

    return;
}

VOID
AhciNonQueuedErrorRecovery(
    PAHCI_CHANNEL_EXTENSION ChannelExtension
  )
{
/*
AHCI 1.1 Section 6.2.2.1
"The flow for system software to recover from an error when non-queued commands are issued is as follows:
    Reads PxCI to see which commands are still outstanding
    Reads PxCMD.CCS to determine the slot that the HBA was processing when the error occurred
    Clears PxCMD.ST to ‘0’ to reset the PxCI register, waits for PxCMD.CR to clear to ‘0’
    Clears any error bits in PxSERR to enable capturing new errors.
    Clears status bits in PxIS as appropriate
    If PxTFD.STS.BSY or PxTFD.STS.DRQ is set to ‘1’, issue a COMRESET to the device to put it in an idle state
    Sets PxCMD.ST to ‘1’ to enable issuing new commands

It assumes:
    Called asynchronously

Called by:
    AhciPortErrorRecovery <-- AhciHwInterrupt


It performs:
    (overview)
    1 Recover from an error
    2 Complete the Failed Command
    3 Complete Succeeded Commands

    (details)
    1.1 Initialize
    1.2 Reads PxCI to see which commands are still outstanding
    1.3 Reads PxCMD.CCS to determine the slot that the HBA was processing when the error occurred
    1.4.1 Clears PxCMD.ST to ‘0’ to reset the PxCI register, waits for PxCMD.CR to clear to ‘0’
     1.5 Clears any error bits in PxSERR to enable capturing new errors.
     1.6 Clears status bits in PxIS as appropriate
    1.4.2 If PxTFD.STS.BSY or PxTFD.STS.DRQ is set to ‘1’, issue a COMRESET to the device to put it in an idle state
    1.4.3 If a COMRESET was issued, restore Preserved Settings
    1.4.4 Start the channel

    2.1 Complete the command being issued if there was one
    2.2 Restore the unsent programmed commands for careful reprocessing
    2.3 If a request sense SRB was created due to this error

    3.1 If there were commands that are ready to complete, complete them now

Affected Variables/Registers:
    CI, CMD, TFD
    Channel Extension
*/
    BOOLEAN performedCOMRESET;
    ULONG   ci;
    ULONG   localCommandsIssued;
    UCHAR   numberCommandsOutstanding;
    ULONG   failingCommand;

    AHCI_COMMAND        cmd;
    AHCI_TASK_FILE_DATA tfd;
    PSCSI_REQUEST_BLOCK senseSrb;
    PAHCI_SRB_EXTENSION srbExtension;

  //1.1 Initialize variables
    RecordExecutionHistory(ChannelExtension, 0x00000013);//AhciNonQueuedErrorRecovery

    ++(ChannelExtension->TotalCountNonQueuedErrorRecovery);
    AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                       (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                       AhciTelemetryEventIdNonqueuedErrorRecovery,
                                       "AhciNonQueuedErrorRecovery",
                                       0,
                                       NULL,
                                       0
                                       );

    senseSrb = NULL;
    performedCOMRESET = FALSE;

  //1.2 Reads PxCI to see which commands are still outstanding
    ci = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CI);

  //1.3 Reads PxCMD.CCS to determine the slot that the HBA was processing when the error occurred
    cmd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CMD.AsUlong);

  //1.4 Clears PxCMD.ST to ‘0’ to reset the PxCI register, waits for PxCMD.CR to clear to ‘0’
    if ( !P_NotRunning(ChannelExtension, ChannelExtension->Px) ){ //This clears PxCI
        RecordExecutionHistory(ChannelExtension, 0x10160013);//AhciNonQueuedErrorRecovery, Port Stop Failed
        AhciPortReset(ChannelExtension, FALSE);
        return;
    }

  //2.1 Complete the command being issued if there was one

    //Determine how many commands are outstanding
    localCommandsIssued = ChannelExtension->SlotManager.CommandsIssued;
    numberCommandsOutstanding = 0;
    failingCommand = 0;
    while (localCommandsIssued) {
        if (localCommandsIssued & 1) {
            numberCommandsOutstanding++;
        }
        //To keep from having to walk through the localCommandsIssued mask again,
        //if there is only 1 command, failing commands will be 1 greater than that slot after this while loop.
        failingCommand++;
        localCommandsIssued >>= 1;
    }

    //If there is only one command outstanding, then we know which one to complete.
    if (numberCommandsOutstanding == 1) {
        failingCommand--;
        //As long as there is an SRB in that command slot.
        if (ChannelExtension->Slot[failingCommand].Srb == NULL) {
            RecordExecutionHistory(ChannelExtension, 0x10170013);//AhciNonQueuedErrorRecovery, didn't find the expected command from slot.
            AhciPortReset(ChannelExtension, FALSE);
            return;
        }
    //If NonQueuedErrorRecovery should not be done for more than 1 command stop here.
    } else if ( (cmd.ST == 1) && AdapterNoNonQueuedErrorRecovery(ChannelExtension->AdapterExtension) ) {
        RecordExecutionHistory(ChannelExtension, 0x10180013);//AhciNonQueuedErrorRecovery, NCQ_NeverNonQueuedErrorRecovery
        AhciPortReset(ChannelExtension, FALSE);
        return;
    } else {
        //otherwise the failing command is in CCS, as long as there is an SRB in that command slot.
        if( ((ChannelExtension->SlotManager.CommandsIssued & (1 << cmd.CCS)) > 0) &&
            (ChannelExtension->Slot[cmd.CCS].Srb) ) {
            failingCommand = cmd.CCS;
        } else {
            RecordExecutionHistory(ChannelExtension, 0x10190013);//AhciNonQueuedErrorRecovery, command to be failed is not indicated by PxCMD.CCS
            AhciPortReset(ChannelExtension, FALSE);
            return;
        }
    }

    // Keep in mind although Px.CMD.ST was just shut off, we are working with the previous snapshot of ST.
    if (cmd.ST == 1) {

        ULONG storStatus = STOR_STATUS_NOT_IMPLEMENTED;
        STOR_REQUEST_INFO requestInfo = {0};

        srbExtension = GetSrbExtension(ChannelExtension->Slot[failingCommand].Srb);
        // Remove the errant command from the Commands Issued.
        ci &= ~(1 << failingCommand);
        // Move the errant command from Issued list to the 'to complete' list
        ChannelExtension->SlotManager.CommandsIssued &= ~(1 << failingCommand);
        ChannelExtension->SlotManager.NCQueueSliceIssued &= ~(1 << failingCommand);
        ChannelExtension->SlotManager.NormalQueueSliceIssued &= ~(1 << failingCommand);
        ChannelExtension->SlotManager.SingleIoSliceIssued &= ~(1 << failingCommand);
        ChannelExtension->SlotManager.HighPriorityAttribute &= ~(1 << failingCommand);
        ChannelExtension->SlotManager.CommandsToComplete |= (1 << failingCommand);

        // Fill in the status of the command
        srbExtension->AtaStatus = ChannelExtension->TaskFileData.STS.AsUchar;
        srbExtension->AtaError = ChannelExtension->TaskFileData.ERR;

        // Check whether the failed command is paging IO, if so, return busy status to indicate storport to retry it.
        storStatus = StorPortGetRequestInfo(ChannelExtension, (PSCSI_REQUEST_BLOCK)ChannelExtension->Slot[failingCommand].Srb, &requestInfo);
        if ((storStatus == STOR_STATUS_SUCCESS) && 
            (requestInfo.Flags & REQUEST_INFO_PAGING_IO_FLAG)) {
            ChannelExtension->Slot[failingCommand].Srb->SrbStatus = SRB_STATUS_BUSY;
        } else {
            ChannelExtension->Slot[failingCommand].Srb->SrbStatus = SRB_STATUS_ERROR;
        }

        // Handle Request Sense if needed
        if ( NeedRequestSense(ChannelExtension->Slot[failingCommand].Srb) ) {
            // This is for ATAPI device only
            senseSrb = BuildRequestSenseSrb(ChannelExtension, ChannelExtension->Slot[failingCommand].Srb); // it will be programmed to slot later in this routine.
            if (senseSrb != NULL) {
                //Make the slot available again now, its slot content will not be cleaned up later.
                ChannelExtension->Slot[failingCommand].CmdHeader = NULL;
                ChannelExtension->Slot[failingCommand].CommandHistoryIndex = 0;
                ChannelExtension->Slot[failingCommand].Srb = NULL;
                //Do not complete this command normally, it will be completed when it's request sense is completed
                ChannelExtension->SlotManager.CommandsToComplete &= ~(1 << failingCommand);
                RecordExecutionHistory(ChannelExtension, 0x10730013);//NonNCQ Error Received RequestSense
            }
        }

        if (senseSrb == NULL) {
            // if not need Request Sense or cannot get Request Sense Srb, Complete the command manually
            ReleaseSlottedCommand(ChannelExtension, (UCHAR)failingCommand, TRUE); //&ChannelExtension->Slot[cmd.CCS] );
            RecordExecutionHistory(ChannelExtension, 0x10630013);//NonNCQ Error Completed failed command
        }
    }

    //1.5 Clears any error bits in PxSERR to enable capturing new errors.
        //Handled in the Interrupt routine
    //1.6 Clears status bits in PxIS as appropriate
        //Handled in the Interrupt routine

    //1.4.2 If PxTFD.STS.BSY or PxTFD.STS.DRQ is set to ‘1’, issue a COMRESET to the device to put it in an idle state
    tfd.AsUlong = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->TFD.AsUlong);

    if(tfd.STS.BSY || tfd.STS.DRQ) {
        AhciCOMRESET(ChannelExtension, ChannelExtension->Px);
        performedCOMRESET = TRUE;
    }

    //2.2 Restore the unsent programmed commands for careful reprocessing
    ChannelExtension->SlotManager.NormalQueueSlice |= ci;   //put the commands that didn't get a chance to finish back into the normal queue
    ChannelExtension->SlotManager.CommandsIssued &= ~ci;    //Remove the unfinished commands from the 'issued' list
    ChannelExtension->SlotManager.NCQueueSliceIssued &= ~ci;
    ChannelExtension->SlotManager.NormalQueueSliceIssued &= ~ci;
    ChannelExtension->SlotManager.SingleIoSliceIssued &= ~ci;

    //2.3 If there were commands that are ready to complete, complete them
    if (ChannelExtension->SlotManager.CommandsToComplete) {
        AhciCompleteIssuedSRBs(ChannelExtension, SRB_STATUS_SUCCESS, TRUE); //complete the successful commands to start the RS SRB.
    }

    //3.1 If a request sense SRB was created due to this error
    if (senseSrb != NULL) {
        ChannelExtension->StateFlags.QueuePaused = FALSE;
        AhciProcessIo(ChannelExtension, (PSTORAGE_REQUEST_BLOCK)senseSrb, TRUE);  //program Sense Srb into Slot
    }

    //3.2 If a COMRESET was issued, restore Preserved Settings
    if (performedCOMRESET == TRUE) {
        RestorePreservedSettings(ChannelExtension, TRUE);
    }

    //3.3 Start the channel, IO will be resumed after port start complete.
    P_Running_StartAttempt(ChannelExtension, TRUE);

    RecordExecutionHistory(ChannelExtension, 0x10000013);//Exit AhciNonQueuedErrorRecovery
}

VOID
NcqErrorRecoveryCompletion (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb
    )
/*
    This completion routine continue handling NCQ Error Recovery after NCQ Error Log is read.

    It finds out the failing command and completes it back.
    Other commands are resent to adapter in miniport level.

*/
{
    PAHCI_SRB_EXTENSION srbExtension = GetSrbExtension(Srb);
    ULONG issuedCommands = (ULONG)(ULONG_PTR)srbExtension->CompletionContext;
    STOR_LOCK_HANDLE lockhandle = {InterruptLock, 0};
    BOOLEAN fallBacktoReset = FALSE;

    RecordExecutionHistory(ChannelExtension, 0x0000001b);   //Enter NcqErrorRecoveryCompletion

    ++(ChannelExtension->TotalCountNCQErrorRecoveryComplete);
    AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                       (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                       AhciTelemetryEventIdNCQErrorRecoveryComplete,
                                       "NcqErrorRecoveryCompletion",
                                       0,
                                       NULL,
                                       0
                                       );

    //
    // In case of reset happened when Read Log command was executing, all requests have been returned back to port driver.
    // There is no more recovery work this routine needs to do.
    //
    if (Srb->SrbStatus == SRB_STATUS_BUS_RESET) {
        StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: NCQ Error Log read - reset\n", ChannelExtension->PortNumber);
        ChannelExtension->StateFlags.NcqErrorRecoveryInProcess = 0;
        ChannelExtension->DeviceExtension->IoRecord.NcqReadLogErrorCount++;

        RecordExecutionHistory(ChannelExtension, 0x1001001b);   //NcqErrorRecoveryCompletion RESET happened on read NCQ Error Log command.

        return;
    }

    if (Srb->SrbStatus == SRB_STATUS_SUCCESS) {
        //
        // Read NCQ Error Log succeeded. Only complete the failing command.
        // Other commands will be put back into NCQ slices and will be issued again to adapter.
        //
        PGP_LOG_NCQ_COMMAND_ERROR ncqErrorLog = (PGP_LOG_NCQ_COMMAND_ERROR)ChannelExtension->DeviceExtension->ReadLogExtPageData;
        UCHAR failingCommand;
        PSTORAGE_REQUEST_BLOCK failingSrb = NULL;
        PAHCI_SRB_EXTENSION failingSrbExtension = NULL;

        NT_ASSERT(ncqErrorLog->NonQueuedCmd == 0);
        NT_ASSERT(ncqErrorLog->NcqTag != 0);

        failingCommand = ncqErrorLog->NcqTag;

        //
        // The failing command should be one of the issued commands.
        // Otherwise, it's a device issue and we fall back to use reset process.
        //
        if (((1 << failingCommand) & issuedCommands) != 0) {

            failingSrb = ChannelExtension->Slot[failingCommand].Srb;

            //
            // If the surprise remove happens just after the NCQ error handling, then
            // there is chance that the failingSrb is NULL here.
            //
            if (failingSrb != NULL) {
                failingSrbExtension = GetSrbExtension(failingSrb);

                //
                // Record error and status
                // 
                failingSrbExtension->AtaStatus = ncqErrorLog->Status;
                failingSrbExtension->AtaError = ncqErrorLog->Error;

                if( IsReturnResults(failingSrbExtension->Flags) ) {
                    SetReturnRegisterValues(ChannelExtension, failingSrb, NULL);
                }

                //
                // Get Sense Data from log, or from translation.
                //
                if ((ChannelExtension->DeviceExtension->IdentifyDeviceData->SerialAtaFeaturesSupported.NCQAutosense == 1) &&
                    (ncqErrorLog->SenseKey != SCSI_SENSE_NO_SENSE)) {

                    StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: NCQ Error Log read successfully. Sense Data returned: %02X/%02X/%02X\n", 
                                           ChannelExtension->PortNumber, ncqErrorLog->SenseKey, ncqErrorLog->ASC, ncqErrorLog->ASCQ);

                    AhciSetSenseData(failingSrb, SRB_STATUS_ERROR, ncqErrorLog->SenseKey, ncqErrorLog->ASC, ncqErrorLog->ASCQ);
                } else {
                    //
                    // Set SrbStatus indicates it's an error condition.
                    // Sense data will be translated from Error register in ReleaseSlottedCommand()
                    //
                    StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: NCQ Error Log read successfully. No Sense Data returned\n", ChannelExtension->PortNumber);

                    failingSrb->SrbStatus = SRB_STATUS_ERROR;
                    AtaMapError(ChannelExtension, failingSrb, ChannelExtension->Slot[failingCommand].StateFlags.FUA);
                }

                //
                // Inform ReleaseSlottedCommand() function that sense data has been set.
                //
                SETMASK(failingSrbExtension->Flags, ATA_FLAGS_SENSEDATA_SET);

                //
                // Acquire spinlock before touching Slots.
                //
                AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);

                //
                // Remove the failing command from NCQ Slice.
                // Put back other previous outstanding commands.
                //
                ChannelExtension->SlotManager.NCQueueSlice &= ~(1 << failingCommand);

                //
                // Clear the NCQ Error Recovery state flag, so that queue will be resumed in ReleaseSlottedCommand().
                //
                ChannelExtension->StateFlags.NcqErrorRecoveryInProcess = 0;

                //
                // Complete the failing command.
                //
                ChannelExtension->SlotManager.CommandsToComplete |= (1 << failingCommand);
                ReleaseSlottedCommand(ChannelExtension, (UCHAR)failingCommand, TRUE);

                ActivateQueue(ChannelExtension, TRUE);

                AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);

            } else {
                //
                // If failingSrb is NULL, clear the NCQ Error Recovery state flag.
                //
                ChannelExtension->StateFlags.NcqErrorRecoveryInProcess = 0;

                StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: NCQ Error Log read successfully, but the failing srb is NULL!\n", ChannelExtension->PortNumber);

                //
                // Capture the scenarios that failing srb is NULL, need investigate cases other than surprise remove.
                //
                NT_ASSERT(failingSrb != NULL);
            }

        } else {
            StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: NCQ Tag doesn't belong to issued commands.\n", ChannelExtension->PortNumber);
            NT_ASSERT(((1 << failingCommand) & issuedCommands) != 0);
            fallBacktoReset = TRUE;

            RecordExecutionHistory(ChannelExtension, 0x1002001b);   //NcqErrorRecoveryCompletion, NCQ Tag doesn't belong to issued commands.
        }

    } else if ((Srb->SrbStatus == SRB_STATUS_BUSY) &&
               (srbExtension->QueueTag == 0xFF)) {
        //
        // Read NCQ Error Log cannot be issued, as all slots are occupied.
        //
        fallBacktoReset = TRUE;

        RecordExecutionHistory(ChannelExtension, 0x1004001b);   //NcqErrorRecoveryCompletion, Read NCQ Error Log cannot be issued, as all slots are occupied..

    } else {
        //
        // Read NCQ Error Log failed. Completes all NCQ commands back to upper layer.
        //
        StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: NCQ Error Log read - Failed\n", ChannelExtension->PortNumber);
        fallBacktoReset = TRUE;
        ChannelExtension->DeviceExtension->IoRecord.NcqReadLogErrorCount++;

        RecordExecutionHistory(ChannelExtension, 0x1003001b);   //NcqErrorRecoveryCompletion, read NCQ Error Log failed.
    }

    if (fallBacktoReset) {
        AhciInterruptSpinlockAcquire(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);

        ChannelExtension->StateFlags.NcqErrorRecoveryInProcess = 0;

        //
        // Recover SlotManager context.
        //
        ChannelExtension->SlotManager.NCQueueSlice &= ~issuedCommands;
        ChannelExtension->SlotManager.CommandsIssued |= issuedCommands;
        ChannelExtension->SlotManager.NCQueueSliceIssued |= issuedCommands;
        AhciPortReset(ChannelExtension, FALSE);

        AhciInterruptSpinlockRelease(ChannelExtension->AdapterExtension, ChannelExtension->PortNumber, &lockhandle);
    }

    if (LogExecuteFullDetail(ChannelExtension->AdapterExtension->LogFlags)) {
        RecordExecutionHistory(ChannelExtension, 0x1000001b);   //Exit NcqErrorRecoveryCompletion.
    }

    return;
}

VOID
AhciNcqErrorRecovery(
    PAHCI_CHANNEL_EXTENSION ChannelExtension
  )
/*
AHCI 1.1 Section 6.2.2.2 Native Command Queuing Error Recovery
    The flow for system software to recover from an error when native command queuing commands are
    issued is as follows:

    • Reads PxSACT to see which commands have not yet completed
    • Clears PxCMD.ST to ‘0’ to reset the PxCI and PxSACT registers, waits for PxCMD.CR to clear to ‘0’
    • Clears any error bits in PxSERR to enable capturing new errors.
    • Clears status bits in PxIS as appropriate
    • If PxTFD.STS.BSY or PxTFD.STS.DRQ is set to ‘1’, issue a COMRESET to the device to put it in an idle state
    • Sets PxCMD.ST to ‘1’ to enable issuing new commands
    • Issue READ LOG EXT to determine the cause of the error condition if software did not have to perform a reset (COMRESET or software reset) as part of the error recovery

    Software then either completes commands that did not finish with error to higher level software, or reissues
    them to the device.

    To be able to finish this process, StorAHCI needs to hold all other requests until READ LOG EXT command is completed.
    This is achieved by utilizing "ChannelExtension->StateFlags.QueuePaused" flag.
        1. READ LOG EXT command will be issued using miniport reserved slot 0, so it will be the next command being executed (by StorAHCI slot management design).
        2. This command is a SingleSlice command. StorAHCI will set "QueuePaused" flag to be TRUE.
        3. "QueuePaused" flag will be set to FALSE in NcqErrorRecoveryCompletion after process is completed.

It assumes:
    Called asynchronously

Called by:
    AhciPortErrorRecovery <-- AhciHwInterrupt

*/
{
    BOOLEAN needCOMRESET = FALSE;
    ULONG   allocated;

#ifdef DBG
    ULONG   ci;
    ULONG   sact;
#endif

    //
    // Record history for NCQ error recovery
    //
    RecordExecutionHistory(ChannelExtension, 0x00000014);   //AhciNcqErrorRecovery

    AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                       (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                       AhciTelemetryEventIdNCQErrorRecovery,
                                       "AhciNcqErrorRecovery Enter",
                                       0,
                                       NULL,
                                       0
                                       );

    //
    // Reads PxSACT and PxCI to see which commands are still outstanding
    //
#ifdef DBG
    ci = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->CI);
    sact = StorPortReadRegisterUlong(ChannelExtension->AdapterExtension, &ChannelExtension->Px->SACT);
#endif

    //
    // Clears PxCMD.ST to ‘0’, waits for PxCMD.CR to clear to ‘0’
    // If it doesn't succeed, issue COMRESET to recover.
    //
    if ( !P_NotRunning(ChannelExtension, ChannelExtension->Px) ) {
        needCOMRESET = TRUE;
    }

    //
    // Clears any error bits in PxSERR to enable capturing new errors.
    // It's handled in the Interrupt routine
    //

    //
    // Clears status bits in PxIS as appropriate
    // It's handled in the Interrupt routine
    //

    if (needCOMRESET == FALSE) {
        //
        // If PxTFD.STS.BSY or PxTFD.STS.DRQ is set to ‘1’, issue a COMRESET to the device to put it in an idle state
        // ChannelExtension->TaskFileData has been read from register in ISR.
        //
        if ((ChannelExtension->TaskFileData.STS.BSY != 0) || (ChannelExtension->TaskFileData.STS.DRQ != 0)) {
            needCOMRESET = TRUE;
        }
    }

    if (needCOMRESET == TRUE) {
        //
        // COMRESET is issued, bail out the function.
        //
        RecordExecutionHistory(ChannelExtension, 0x10160014);       //AhciNcqErrorRecovery, RESET is being issued.
        AhciPortReset(ChannelExtension, FALSE);
        return;
    }

    //
    // See if local SRB is in use.  If yes, fall back to reset the port.
    //
    allocated = GetOccupiedSlots(ChannelExtension);
    if ((allocated & (1 << 0)) > 0) {
        //
        // COMRESET is issued, bail out the function.
        //
        RecordExecutionHistory(ChannelExtension, 0x10170014);       //AhciNcqErrorRecovery, RESET is being issued because slot 0 is in use.
        AhciPortReset(ChannelExtension, FALSE);
        return;
    }

    StorPortDebugPrint(3, "StorAHCI - Port %02d - NCQ Error Recovery: Send READ LOG EXT to read NCQ Error Log. Issued Requests:: %08X\n",
                       ChannelExtension->PortNumber, ChannelExtension->SlotManager.CommandsIssued);

    //
    // Only one NCQ Error Recovery should be happening at a time.
    // If we get another error before we finished processing the previous one
    // then this is likely a device issue so do reset.
    //
    NT_ASSERT(ChannelExtension->StateFlags.NcqErrorRecoveryInProcess == 0);
    if (ChannelExtension->StateFlags.NcqErrorRecoveryInProcess) {
        RecordExecutionHistory(ChannelExtension, 0x10170015);       //AhciNcqErrorRecovery, RESET is being issued because NCQ error recovery is already in progress.
        AhciPortReset(ChannelExtension, FALSE);
        return;
    }    

    //
    // Set the state flag indicating it's in NCQ Error Recovery process.
    // This doesn't get cleared until NcqErrorRecoverCompletion is called,
    // which happens after the local Sense.Srb is removed from the completion
    // queue. (It also gets cleared on a port reset.)
    //
    ChannelExtension->StateFlags.NcqErrorRecoveryInProcess = 1;

    //
    // When COMRESET is not issued, adapter still has information about error.
    // Issue READ LOG EXT command to read NCQ Error Log (10h). From the log, we can
    // find which command fails and the sense data if NcqAutoSense is supported.
    //

    //
    // For ATA device, use Sense.Srb to retrieve NCQ Log.
    // NOTE: When device is ATAPI, Sense.Srb is used to retrieve Sense Data.
    //
    NT_ASSERT(IsAtaDevice(&ChannelExtension->DeviceExtension->DeviceParameters));

    AhciZeroMemory((PCHAR)&ChannelExtension->Sense.Srb, sizeof(SCSI_REQUEST_BLOCK));
    AhciZeroMemory((PCHAR)ChannelExtension->Sense.SrbExtension, sizeof(AHCI_SRB_EXTENSION));

    ChannelExtension->Sense.Srb.SrbExtension = (PVOID)ChannelExtension->Sense.SrbExtension;
    ChannelExtension->Sense.Srb.OriginalRequest = &ChannelExtension->Sense.Srb;

    NT_ASSERT(((ci | sact) & ~ChannelExtension->SlotManager.CommandsIssued) == 0);
    ChannelExtension->Sense.SrbExtension->CompletionContext = (PVOID)ChannelExtension->SlotManager.CommandsIssued;

    //
    // Put the issued commands back to NCQ Slices.
    //
    ChannelExtension->SlotManager.NCQueueSlice |= ChannelExtension->SlotManager.CommandsIssued;
    ChannelExtension->SlotManager.CommandsIssued = 0;
    ChannelExtension->SlotManager.NCQueueSliceIssued = 0;

    IssueReadLogExtCommand( ChannelExtension,
                            (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Sense.Srb,
                            IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS,
                            0,
                            1,
                            0,      // feature field
                            &ChannelExtension->DeviceExtension->ReadLogExtPageDataPhysicalAddress,
                            (PVOID)ChannelExtension->DeviceExtension->ReadLogExtPageData,
                            NcqErrorRecoveryCompletion
                            );

    ChannelExtension->StateFlags.QueuePaused = FALSE;
    AhciProcessIo(ChannelExtension, (PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Sense.Srb, TRUE);  //program Sense Srb into Slot

    //
    // Start the channel, IO will be resumed after port start complete.
    //
    P_Running_StartAttempt(ChannelExtension, TRUE);

    RecordExecutionHistory(ChannelExtension, 0x10000014);   //Exit AhciNcqErrorRecovery

    AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                       (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                       AhciTelemetryEventIdNCQErrorRecovery,
                                       "AhciNcqErrorRecovery Exit",
                                       0,
                                       NULL,
                                       0
                                       );

    return;
}

VOID
AhciPortErrorRecovery(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension
  )
/*
    Error recovery routine for Port
    AHCI: 6.2.2.1 Non-Queued Error Recovery
          6.2.2.2 Native Command Queuing Error Recovery
          6.2.2.3 Recovery of Unsolicited COMINIT

It assumes:
    nothing

Called by:
    AhciHwInterrupt

Return Values:
    None
*/
{
    ++(ChannelExtension->TotalCountPortErrorRecovery);
    AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                       (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                       AhciTelemetryEventIdPortErrorRecovery,
                                       "AhciPortErrorRecovery",
                                       0,
                                       NULL,
                                       0
                                       );

    // Log the StateFlags
    StorPortEtwChannelEvent8(ChannelExtension->AdapterExtension,
                             (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                             StorportEtwEventOperational,
                             AhciEtwEventHandleInterrupt,
                             L"AhciPortErrorRecovery",
                             STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                             StorportEtwLevelError,
                             StorportEtwEventOpcodeInfo,
                             NULL,
                             L"StateFlags",
                             *(ULONGLONG *)&(ChannelExtension->StateFlags),
                             L"PortErrRecovery",
                             ChannelExtension->TotalCountPortErrorRecovery,
                             L"StartState",
                             *(ULONGLONG *)&(ChannelExtension->StartState),
                             L"PortReset",
                             ChannelExtension->TotalCountPortReset,
                             L"RunningStrtFail",
                             ChannelExtension->TotalCountRunningStartFailed,
                             L"NonQ'dErrRecov",
                             ChannelExtension->TotalCountNonQueuedErrorRecovery,
                             L"NCQError|NCQErrorComplete",
                             (((ULONGLONG)ChannelExtension->TotalCountNCQError << 32) |
                             ((ULONGLONG)ChannelExtension->TotalCountNCQErrorRecoveryComplete)),
                             L"BusChange",
                             ChannelExtension->TotalCountBusChange);

    if (ChannelExtension->StateFlags.CallAhciReportBusChange == 1) {
        // Handle AHCI 6.2.2.3 Recovery of Unsolicited COMINIT and hot plug
        // AhciPortBusChangeDpcRoutine() will issue RESET, ignore other error recovery marks.
        ChannelExtension->StateFlags.CallAhciReportBusChange = 0;
        ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery = 0;
        ChannelExtension->StateFlags.CallAhciReset = 0;
        ChannelExtension->StateFlags.CallAhciNcqErrorRecovery = 0;

        ++(ChannelExtension->TotalCountBusChange);
        if (ChannelExtension->TotalCountBusChange >= AHCI_BUS_CHANGE_COUNT_WARNING_THRESHOLD) {
            AhciPortMarkDeviceFailed(ChannelExtension, AhciDeviceFailureTooManyBusChange, 0, TRUE);
        }

        if (!IsDumpMode(ChannelExtension->AdapterExtension)) {
            StorPortIssueDpc(ChannelExtension->AdapterExtension, &ChannelExtension->BusChangeDpc, ChannelExtension, NULL);
        } else {
            AhciPortBusChangeDpcRoutine(&ChannelExtension->BusChangeDpc, ChannelExtension->AdapterExtension, ChannelExtension, NULL);
        }
    }

    if (ChannelExtension->StateFlags.CallAhciNcqErrorRecovery == 1) {
        //
        // Handle NCQ Errors by not issuing COMRESET if possible;
        // and READ LOG to understand which command fails.
        //
        ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery = 0;
        ChannelExtension->StateFlags.CallAhciReset = 0;
        ChannelExtension->StateFlags.CallAhciNcqErrorRecovery = 0;

        ++(ChannelExtension->TotalCountNCQError);
        if (ChannelExtension->TotalCountNCQError >= AHCI_NCQ_ERROR_COUNT_WARNING_THRESHOLD) {
            AhciPortMarkDeviceFailed(ChannelExtension, AhciDeviceFailureTooManyNCQError, 0, TRUE);
        }

        if (SupportsEnhancedNcqErrorRecovery(ChannelExtension)) {
            AhciNcqErrorRecovery(ChannelExtension);
        } else {
            //
            // In case Enhanced NCQ Error Recovery is not supported, use the legacy method to reset device.
            //
            ChannelExtension->StateFlags.CallAhciReset = 1;
        }
    }

    if (ChannelExtension->StateFlags.CallAhciReset == 1) {
        // Handle AHCI 6.2.2.2 Native Command Queuing Error Recovery and other events require RESET.
        ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery = 0;
        ChannelExtension->StateFlags.CallAhciReset = 0;

        AhciPortReset(ChannelExtension, FALSE);
    }

    if (ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery == 1) {
        // Handle AHCI 6.2.2.1 Non-Queued Error Recovery
        ChannelExtension->StateFlags.CallAhciNonQueuedErrorRecovery = 0;

        if (ChannelExtension->TotalCountNonQueuedErrorRecovery >= AHCI_NON_QUEUED_ERROR_COUNT_WARNING_THRESHOLD) {
            AhciPortMarkDeviceFailed(ChannelExtension, AhciDeviceFailureTooManyNonQueuedError, 0, TRUE);
        }

        AhciNonQueuedErrorRecovery(ChannelExtension);
    }

    return;
}

BOOLEAN
AhciPortReset(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN CompleteAllRequests
    )
/*++
    AhciPortReset can be called even if the miniport driver is not ready for another request.
    The miniport driver should complete all pending requests and must reset the given channel.

It assumes:
    DIRQL

Called by:

It performs:
    COMRESET along with completing commands to be retried and restoring settings that may be be persistent across a COMRESET

    (overview)
    1 Initialize
    2 Perform COMReset
    3 Complete all outstanding commands
    4 Restore device configuration

Affected Variables/Registers:
    SCTL, CI, SACT
    Channel Extension

Return Values:
    AhciPortReset return TRUE if the reset operation succeeded.
    If the reset failed the routine must return FALSE.
--*/
{
    ULONG commandsToCompleteCount = 0;
    BOOLEAN isPortStartInProgress = FALSE;
    BOOLEAN isCompletionDPCQueued = FALSE;
    BOOLEAN isCompletionDPCInterrupted = FALSE;

    //1.1 Initialize Variables
    ++(ChannelExtension->TotalCountPortReset);

    if ((CompleteAllRequests) &&
        (ChannelExtension->StartState.ChannelNextStartState != StartComplete)) {

        NT_ASSERT(FALSE);

        isPortStartInProgress = TRUE;
        RecordExecutionHistory(ChannelExtension, 0x10040050); //AhciPortReset, Port Start still in progress, device probably hang

    } else {
        RecordExecutionHistory(ChannelExtension, 0x00000050); //AhciPortReset
    }

    AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                       (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                       AhciTelemetryEventIdPortReset,
                                       (isPortStartInProgress ? "PortStartTakeTooLong" : "AhciPortReset"),
                                       (isPortStartInProgress ? AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING : 0),
                                       NULL,
                                       0
                                       );

    if (ChannelExtension->CompletionQueue.LastTimeStampAddQueue.QuadPart > ChannelExtension->LastTimeStampDpcStart.QuadPart) {
        isCompletionDPCQueued = TRUE;
    }

    if (ChannelExtension->LastTimeStampDpcStart.QuadPart > ChannelExtension->LastTimeStampDpcCompletion.QuadPart) {
        isCompletionDPCInterrupted = TRUE;
    }

    if (isCompletionDPCQueued || isCompletionDPCInterrupted) {

        LARGE_INTEGER currentTimeStamp;
        StorPortQuerySystemTime(&currentTimeStamp);
        ULONGLONG duration = 0;

        NT_ASSERT(FALSE);

        if (isCompletionDPCQueued && isCompletionDPCInterrupted) {
            if (ChannelExtension->LastTimeStampDpcStart.QuadPart < ChannelExtension->CompletionQueue.LastTimeStampRemoveQueue.QuadPart) {
                duration = (ULONGLONG)(currentTimeStamp.QuadPart - ChannelExtension->CompletionQueue.LastTimeStampRemoveQueue.QuadPart);
            } else {
                duration = (ULONGLONG)(currentTimeStamp.QuadPart - ChannelExtension->LastTimeStampDpcStart.QuadPart);
            }
        } else if (isCompletionDPCQueued) {
            duration = (ULONGLONG)(currentTimeStamp.QuadPart - ChannelExtension->CompletionQueue.LastTimeStampAddQueue.QuadPart);
        } else if (isCompletionDPCInterrupted) {
            duration = (ULONGLONG)(currentTimeStamp.QuadPart - ChannelExtension->LastTimeStampDpcStart.QuadPart);
        }

        RecordExecutionHistory(ChannelExtension, 0x10050050); //AhciPortReset, DPC queued or interrupted

        AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                           (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                           AhciTelemetryEventIdPortReset,
                                           "DPC queued or interrupted",
                                           AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING,
                                           "Duration(in 100ns)",
                                           duration
                                           );
    }

    StorPortEtwChannelEvent8(ChannelExtension->AdapterExtension,
                             (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                             StorportEtwEventOperational,
                             AhciEtwEventPortReset,
                             L"Port reset",
                             STORPORT_ETW_EVENT_KEYWORD_COMMAND_TRACE,
                             StorportEtwLevelInformational,
                             StorportEtwEventOpcodeInfo,
                             NULL,
                             L"PortReset",
                             ChannelExtension->TotalCountPortReset,
                             L"RunningStrtFail",
                             ChannelExtension->TotalCountRunningStartFailed,
                             L"PortErrRecovery",
                             ChannelExtension->TotalCountPortErrorRecovery,
                             L"NonQ'dErrRecov",
                             ChannelExtension->TotalCountNonQueuedErrorRecovery,
                             L"NCQErrRecovery",
                             ChannelExtension->TotalCountNCQError,
                             L"NCQErrRecovCmpl",
                             ChannelExtension->TotalCountNCQErrorRecoveryComplete,
                             L"SurpriseRemove",
                             ChannelExtension->TotalCountSurpriseRemove,
                             L"StateFlags",
                             *(ULONGLONG *)&(ChannelExtension->StateFlags));

    //2.1 Stop the channel
    P_NotRunning(ChannelExtension, ChannelExtension->Px);

    //2.2 Perform the COMRESET
    // AHCI 10.1.2 - 3: If PxCMD.CR or PxCMD.FR do not clear to '0' correctly, then software may attempt a port reset or a full HBA reset to recover.
    AhciCOMRESET(ChannelExtension, ChannelExtension->Px);

    //
    // Clear NCQ Error Recovery state.
    //
    if (ChannelExtension->StateFlags.NcqErrorRecoveryInProcess == 1) {
        ChannelExtension->StateFlags.NcqErrorRecoveryInProcess = 0;
        ChannelExtension->StateFlags.QueuePaused = 0;
    }

    if ((CompleteAllRequests) &&
        (ChannelExtension->StateFlags.QueuePaused == 1) &&
        (ChannelExtension->SlotManager.CommandsIssued == 0)) {

        //
        // This should never happen. Resetting state.
        //

        NT_ASSERT(FALSE);

        ChannelExtension->StateFlags.QueuePaused = 0;

        RecordExecutionHistory(ChannelExtension, 0x10010050); //AhciPortReset, Unpausing queue
        AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                           (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                           AhciTelemetryEventIdPortReset,
                                           "Unpausing queue",
                                           AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING,
                                           NULL,
                                           0
                                           );
    }

    //2.3 If either Init Command or PreservedSettings Command is being processed, reset the command index to start from the first one again.
    //    The process will be continued by the Srb's completion routine.
    if (ChannelExtension->StateFlags.ReservedSlotInUse == 1) {
        ChannelExtension->DeviceInitCommands.CommandToSend = 0;
        ChannelExtension->PersistentSettings.SlotsToSend = ChannelExtension->PersistentSettings.Slots;
    }

    if ((CompleteAllRequests) &&
        (ChannelExtension->StateFlags.ReservedSlotInUse == 1) &&
        (ChannelExtension->SlotManager.SingleIoSliceIssued == 0) &&
        ((ChannelExtension->SlotManager.SingleIoSlice & 0x1) == 0)) {

        NT_ASSERT(FALSE);

        //
        // This could happen if init/preserved setting command completed by device, but the completion DPC is queued(or interrupted) for long time.
        // If there is other scenario, it is needed to further triage.
        //
        // Note: Force clear ReservedSlotInUse flag only if corresponding registry key is set.
        //       Otherwise, there maybe add queue issue. E.g. Buggy device repeatedly generate bus change interrupt,
        //       which could result in add queue bugcheck eventually.
        //

        if (!(isCompletionDPCQueued || isCompletionDPCInterrupted)) {
            RecordExecutionHistory(ChannelExtension, 0x10020050); //AhciPortReset, ReservedSlotInUse flag maybe stuck
            //
            // There maybe possibility that init/restore preserved settings command stuck in device and never complete.
            //
            AhciPortMarkDeviceFailed(ChannelExtension, AhciDeviceFailureDeviceStuck, 0, TRUE);
        } else {
            RecordExecutionHistory(ChannelExtension, 0x10060050); //AhciPortReset, ReservedSlotInUse flag outstanding
        }

    }

    if (((ChannelExtension->SlotManager.SingleIoSliceIssued & 0x1) != 0) &&
        (ChannelExtension->Slot[0].Srb != NULL) &&
        ((PSTORAGE_REQUEST_BLOCK)&ChannelExtension->Local.Srb == ChannelExtension->Slot[0].Srb)) {

        if ((ChannelExtension->Local.SrbExtension != NULL) &&
            ((ChannelExtension->Local.SrbExtension->CompletionRoutine == IssuePreservedSettingCommands) ||
             (ChannelExtension->Local.SrbExtension->CompletionRoutine == IssueInitCommands))) {

            BOOLEAN isInitCommand = (ChannelExtension->Local.SrbExtension->CompletionRoutine == IssueInitCommands) ? (TRUE) : (FALSE);

            NT_ASSERT(FALSE);

            RecordExecutionHistory(ChannelExtension, 0x10030050); //AhciPortReset, Init/Preserved Setting command timeout
            AhciTelemetryLogResetErrorRecovery(ChannelExtension,
                                               (PSTOR_ADDRESS)&(ChannelExtension->DeviceExtension->DeviceAddress),
                                               AhciTelemetryEventIdPortReset,
                                               "Slot 0 command timeout",
                                               AHCI_TELEMETRY_FLAG_NOT_SUPPRESS_LOGGING,
                                               (isInitCommand ? "Init Command" : "Preserved Setting Command"),
                                               0
                                               );
        }
    }

    //3.1 Complete all issued commands
    ChannelExtension->SlotManager.CommandsToComplete = ChannelExtension->SlotManager.CommandsIssued;
    ChannelExtension->SlotManager.CommandsIssued = 0;
    ChannelExtension->SlotManager.NCQueueSliceIssued = 0;
    ChannelExtension->SlotManager.NormalQueueSliceIssued = 0;
    ChannelExtension->SlotManager.SingleIoSliceIssued = 0;
    ChannelExtension->SlotManager.HighPriorityAttribute &= ~ChannelExtension->SlotManager.CommandsToComplete;

    commandsToCompleteCount = NumberOfSetBits(ChannelExtension->SlotManager.CommandsToComplete);

    if (commandsToCompleteCount > 0) {
        UCHAR completeStatus = SRB_STATUS_BUS_RESET;

        if (commandsToCompleteCount == 1) {
            ULONG   i;

            for (i = 0; i <= ChannelExtension->AdapterExtension->CAP.NCS; i++) {
                if (((ChannelExtension->SlotManager.CommandsToComplete & (1 << i)) != 0) &&
                    (ChannelExtension->Slot[i].Srb != NULL) &&
                    (!IsMiniportInternalSrb(ChannelExtension, ChannelExtension->Slot[i].Srb))) {
                    //
                    // we know the error is for this command as it's the only one programmed to adapter,
                    // set status to be SRB_STATUS_ERROR for external Srb to make sure the function - AtaMapError() assigns the real error to Srb
                    // It's preferred to return SRB_STATUS_BUS_RESET to internal request to make it aware of reset happened.
                    //
                    completeStatus = SRB_STATUS_ERROR;
                }
            }
        }

        AhciCompleteIssuedSRBs(ChannelExtension, completeStatus, TRUE); //AhciPortReset is under Interrupt spinlock
    }

    //3.2 Complete all other commands miniport own for this device, when it's necessary or when the Reset is requested by Storport
    if (CompleteAllRequests) {
        AhciPortFailAllIos(ChannelExtension, SRB_STATUS_BUS_RESET, TRUE);
    }

    //4.1 Restore device configuration
    if (ChannelExtension->StateFlags.ReservedSlotInUse == 0) {
        RestorePreservedSettings(ChannelExtension, TRUE);
    }

    //2.3 Start the channel
    P_Running_StartAttempt(ChannelExtension, TRUE); //AhciPortReset is under Interrupt spinlock

    // record that the channel is reset.
    RecordExecutionHistory(ChannelExtension, 0x10000050);//Exit AhciPortReset

    return TRUE;
}


#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4152)
#pragma warning(default:4214)
#pragma warning(default:4201)
#endif

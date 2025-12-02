/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/


#include "precomp.h"

#if defined(EVENT_TRACING)
#include "nic_pm.tmh"
#endif


// Things to note:
// PME_ena bit should be active before the 82558 is set into low power mode
// Default for WOL should generate wake up event after a HW Reset

// Fixed Packet Filtering
// Need to verify that the micro code is loaded and Micro Machine is active
// Clock signal is active on PCI clock


// Address Matching
// Need to enable IAMatch_Wake_En bit and the MCMatch_Wake_En bit is set

// ARP Wakeup
// Need to set BRCST DISABL bet to 0 (broadcast enable)
// To handle VLAN set the VLAN_ARP bit
// IP address needs to be configured with 16 least significant bits
// Set the IP Address in the IP_Address configuration word.

// Fixed WakeUp Filters:
// There are 3ight different fixed WakeUp Filters
// ( Unicast, Multicast, Arp. etc).


// Link Status Event
// Set Link_Status_Wakeup Enable bit.

// Flexible filtering:
// Supports: ARP packets, Directed, Magic Packet and Link Event

// Flexible Filtering Overview:
// driver should program micro-code before setting card into low power
// Incoming packets are compared against the loadable microcode. If PME is
// is enabled then, the system is woken up.


// Segments are defined in book - but not implemented here.

// WakeUp Packet -that causes the machine to wake up will be stored
// in the Micro Machine temporary storage area so that the driver can read it.


// Software Work:
// Power Down:
// OS requests the driver to go to a low power state
// SW sets CU and RU to idle by issuing a Selective Reset to the device
//      3rd portion .- Wake Up Segments defintion
// The above three segments are loaded as on chain. The last CB must have
// its EL bit set.
// Device can now be powered down.
// Software driver completes OS request
// OS then physically switches the Device to low power state
//

// Power Up:
// OS powers up the Device
// driver should NOT initialize the Device. It should NOT issue a Self Test
// Driver Initiates a PORT DUMP command
// Device dumps its internal registers including the wakeup frame storage area
// SW reads the PME register
// SW reads the WakeUp Frame Data, analyzes it and acts accordingly
// SW restores its cvonfiguration and and resumes normal operation.
//

//
// Power Management definitions from the Intel Handbook
//

//
// Definitions from Table 4.2, Pg 4.9
// of the 10/100 Mbit Ethernet Family Software Technical
// Reference Manual
//

#define PMC_Offset  0xDE
#define E100_PMC_WAKE_FROM_D0       0x1
#define E100_PMC_WAKE_FROM_D1       0x2
#define E100_PMC_WAKE_FROM_D2       0x4
#define E100_PMC_WAKE_FROM_D3HOT    0x8
#define E100_PMC_WAKE_FROM_D3_AUX   0x10

//
// Load Programmable filter definintions.
// Taken from C-19 from the Software Reference Manual.
// It has examples too. The opcode used for load is 0x80000
//

#define BIT_15_13                   0xA000

#define CB_LOAD_PROG_FILTER         BIT_3
#define CU_LOAD_PROG_FILTER_EL      BIT_7
#define CU_SUCCEED_LOAD_PROG_FILTER BIT_15_13
#define CB_FILTER_EL                BIT_7
#define CB_FILTER_PREDEFINED_FIX    BIT_6
#define CB_FILTER_ARP_WAKEUP        BIT_3
#define CB_FILTER_IA_WAKEUP         BIT_1

#define CU_SCB_NULL                 ((UINT)-1)


#pragma pack( push, enter_include1, 1 )

//
// Define the PM Capabilities register in the device
// portion of the PCI config space
//
typedef struct _MP_PM_CAP_REG {

    #pragma warning(disable:4214)  // bit field types other than int warning

    USHORT UnInteresting:11;
    USHORT PME_Support:5;

    #pragma warning(default:4214)

} MP_PM_CAP_REG;


//
// Define the PM Control/Status Register
//
typedef struct  _MP_PMCSR {

    #pragma warning(disable:4214)  // bit field types other than int warning

    USHORT PowerState:2;    // Power State;
    USHORT Res:2;           // reserved
    USHORT DynData:1;       // Ignored
    USHORT Res1:3;            // Reserved
    USHORT PME_En:1;        // Enable device to set the PME Event;
    USHORT DataSel:4;       // Unused
    USHORT DataScale:2;     // Data Scale - Unused
    USHORT PME_Status:1;    // PME Status - Sticky bit;

    #pragma warning(default:4214)

} MP_PMCSR ;

typedef struct _MP_PM_PCI_SPACE {

    UCHAR Stuff[PMC_Offset];

    // PM capabilites

    MP_PM_CAP_REG   PMCaps;

    // PM Control Status Register

    MP_PMCSR        PMCSR;


} MP_PM_PCI_SPACE , *PMP_PM_PCI_SPACE ;


//
// This is the Programmable Filter Command Structure
//
typedef struct _MP_PROG_FILTER_COMM_STRUCT
{
    // CB Status Word
    USHORT CBStatus;

    // CB Command Word
    USHORT CBCommand;

    //Next CB PTR == ffff ffff
    ULONG NextCBPTR;

    //Programmable Filters
    ULONG FilterData[16];


} MP_PROG_FILTER_COMM_STRUCT,*PMP_PROG_FILTER_COMM_STRUCT;

typedef struct _MP_PMDR
{
    #pragma warning(disable:4214)  // bit field types other than int warning

    // Status of the PME bit
    UCHAR PMEStatus:1;

    // Is the TCO busy
    UCHAR TCORequest:1;

    // Force TCO indication
    UCHAR TCOForce:1;

    // Is the TCO Ready
    UCHAR TCOReady:1;

    // Reserved
    UCHAR Reserved:1;

    // Has an InterestingPacket been received
    UCHAR InterestingPacket:1;

    // Has a Magic Packet been received
    UCHAR MagicPacket:1;

    // Has the Link Status been changed
    UCHAR LinkStatus:1;

    #pragma warning(default:4214)

} MP_PMDR , *PMP_PMDR;

//-------------------------------------------------------------------------
// Structure used to set up a programmable filter.
// This is overlayed over the Control/Status Register (CSR)
//-------------------------------------------------------------------------
typedef struct _CSR_FILTER_STRUC {

    // Status- used to  verify if the load prog filter command
    // has been accepted .set to 0xa000
    USHORT      ScbStatus;              // SCB Status register

    // Set to an opcode of  0x8
    //
    UCHAR       ScbCommandLow;          // SCB Command register (low byte)

    // 80. Low + High gives the required opcode 0x80080000
    UCHAR       ScbCommandHigh;         // SCB Command register (high byte)

    // Set to NULL ff ff ff ff
    ULONG       NextPointer;      // SCB General pointer

    // Set to a hardcoded filter, Arp + IA Match, + IP address

    union
    {
        ULONG u32;

        struct {
            UCHAR   IPAddress[2];
            UCHAR   Reserved;
            UCHAR   Set;

        }PreDefined;

    }Programmable;     // Wake UP Filter    union

} CSR_FILTER_STRUC, *PCSR_FILTER_STRUC;

#pragma pack( pop, enter_include1 )

#define MP_CLEAR_PMDR(pPMDR)  (*pPMDR) = ((*pPMDR) | 0xe0);  // clear the 3 uppermost bits in the PMDR


//-------------------------------------------------------------------------
// L O C A L    P R O T O T Y P E S
//-------------------------------------------------------------------------

__inline
NTSTATUS
MPIssueScbPoMgmtCommand(
    IN PFDO_DATA Adapter,
    IN PCSR_FILTER_STRUC pFilter,
    IN BOOLEAN WaitForScb
    );


VOID
MPCreateProgrammableFilter (
    IN PMP_WAKE_PATTERN     pMpWakePattern ,
    IN PUCHAR pFilter,
    IN OUT PULONG pNext
    );



//-------------------------------------------------------------------------
// P O W E R    M G M T    F U N C T I O N S
//-------------------------------------------------------------------------

PUCHAR
HwReadPowerPMDR(
    IN  PFDO_DATA     Adapter
    )
/*++
Routine Description:

    This routine will read Hardware's PM registers

Arguments:

    Adapter     Pointer to our adapter

Return Value:

    STATUS_SUCCESS
    NTSTATUS_HARD_ERRORS

--*/
{
    UCHAR PMDR =0;
    PUCHAR pPMDR = NULL;

#define CSR_SIZE sizeof (*Adapter->CSRAddress)



    ASSERT (CSR_SIZE == 0x18);

    pPMDR =  0x18 + (PUCHAR)Adapter->CSRAddress ;

    PMDR = *pPMDR;

    return pPMDR;

}


NTSTATUS
MpClearPME_En (
    IN PFDO_DATA FdoData,
    IN MP_PMCSR PMCSR
    )
{
    NTSTATUS status;
    UINT ulResult;

    PMCSR.PME_En = 0;

    ulResult = FdoData->BusInterface.SetBusData(
                    FdoData->BusInterface.Context,
                    PCI_WHICHSPACE_CONFIG,
                    (PVOID)&PMCSR,
                    FIELD_OFFSET(MP_PM_PCI_SPACE, PMCSR),
                    sizeof(PMCSR));

    ASSERT (ulResult == sizeof(PMCSR));
    if (ulResult == sizeof(PMCSR)) {
        status = STATUS_SUCCESS;

    } else {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}



VOID
MPSetPowerLowPrivate(
    WDFINTERRUPT WdfInterrupt,
    PFDO_DATA    FdoData
    )
/*++
Routine Description:

    The section follows the steps mentioned in
    Section C.2.6.2 of the Reference Manual.


Arguments:

    Adapter     Pointer to our adapter

Return Value:

--*/
{
    CSR_FILTER_STRUC    Filter;
    USHORT              IntStatus;
    MP_PMCSR            PMCSR = {0};
    ULONG               ulResult;

    UNREFERENCED_PARAMETER( WdfInterrupt );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "-->MPSetPowerLowPrivate\n");
    RtlZeroMemory (&Filter, sizeof (Filter));

    //
    // Before issue the command to low power state, we should ack all the
    // pending interrupts, then set the adapter's power to low state.
    //
    NIC_ACK_INTERRUPT(FdoData, IntStatus);

    //
    // If the driver should wake up the machine
    //
    if (FdoData->AllowWakeArming)
    {
        //
        // Send the WakeUp Pattern to the nic
        MPIssueScbPoMgmtCommand(FdoData, &Filter, TRUE);


        //
        // Section C.2.6.2 - The driver needs to wait for the CU to idle
        // The above function already waits for the CU to idle
        //
        ASSERT((FdoData->CSRAddress->ScbStatus & SCB_CUS_MASK) == SCB_CUS_IDLE);
    }
    else
    {

        ulResult = FdoData->BusInterface.GetBusData(
                                FdoData->BusInterface.Context,
                                PCI_WHICHSPACE_CONFIG,
                                (PVOID)&PMCSR,
                                FIELD_OFFSET(MP_PM_PCI_SPACE, PMCSR),
                                sizeof(PMCSR));

        if(ulResult != sizeof(PMCSR)){
            ASSERT(ulResult == sizeof(PMCSR));
            TraceEvents(TRACE_LEVEL_ERROR, DBG_POWER, "GetBusData for PMCSR failed\n");
            return;
        }
        if (PMCSR.PME_En == 1)
        {
            //
            // PME is enabled. Clear the PME_En bit.
            // So that it is not asserted
            //
            MpClearPME_En (FdoData,PMCSR);

        }
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<--MPSetPowerLowPrivate\n");

}

NTSTATUS
MPSetPowerD0Private (
    IN PFDO_DATA FdoData
    )
{
    PUCHAR pPMDR;
    NTSTATUS status;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "-->MPSetPowerD0Private\n");

    // Dump the packet if necessary
    //Cause of Wake Up

    pPMDR = HwReadPowerPMDR(FdoData);

    status = NICInitializeAdapter(FdoData);

    // Clear the PMDR
    MP_CLEAR_PMDR(pPMDR);

    NICIssueSelectiveReset(FdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<--MPSetPowerD0Private\n");

    return status;
}


VOID
HwSetWakeUpConfigure(
    IN PFDO_DATA FdoData,
    PUCHAR       pPoMgmtConfigType,
    UINT         WakeUpParameter
    )
{
    UNREFERENCED_PARAMETER( WakeUpParameter );

    if (IsPoMgmtSupported( FdoData) == TRUE)
    {
        (*pPoMgmtConfigType)= ((*pPoMgmtConfigType) |
                               CB_WAKE_ON_LINK_BYTE9 |
                               CB_WAKE_ON_ARP_PKT_BYTE9  );
    }
}

NTSTATUS
MPSetUpFilterCB(
    IN PFDO_DATA FdoData
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCB_HEADER_STRUC    NonTxCmdBlockHdr = (PCB_HEADER_STRUC)FdoData->NonTxCmdBlock;
    PFILTER_CB_STRUC    pFilterCb = (PFILTER_CB_STRUC)NonTxCmdBlockHdr;
    ULONG               Curr = 0;
    ULONG               Next = 0;
    PLIST_ENTRY         pPatternEntry = ListNext(&FdoData->PoMgmt.PatternList) ;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> MPSetUpFilterCB\n");

    RtlZeroMemory (pFilterCb, sizeof(*pFilterCb));

    // Individual Address Setup
    NonTxCmdBlockHdr->CbStatus = 0;
    NonTxCmdBlockHdr->CbCommand = CB_EL_BIT | CB_LOAD_PROG_FILTER;
    NonTxCmdBlockHdr->CbLinkPointer = DRIVER_NULL;

    // go through each filter in the list.

    while (pPatternEntry != (&FdoData->PoMgmt.PatternList))
    {
        PMP_WAKE_PATTERN            pWakeUpPattern = NULL;
        //PNDIS_PM_PACKET_PATTERN     pCurrPattern = NULL;;

        // initialize local variables
        pWakeUpPattern = CONTAINING_RECORD(pPatternEntry, MP_WAKE_PATTERN, linkListEntry);

        // increment the iterator
        pPatternEntry = ListNext (pPatternEntry);

        // Update the Curr Array Pointer
        Curr = Next;

        // Create the Programmable filter for this device.
        MPCreateProgrammableFilter (pWakeUpPattern , (PUCHAR)&pFilterCb->Pattern[Curr], &Next);

        if (Next >=16)
        {
            break;
        }

    }

    {
        // Set the EL bit on the last pattern
        PUCHAR pLastPattern = (PUCHAR) &pFilterCb->Pattern[Curr];

        // Get to bit 31
        pLastPattern[3] |= CB_FILTER_EL ;


    }

    ASSERT(FdoData->CSRAddress->ScbCommandLow == 0);

    //  Wait for the CU to Idle before giving it this command
    if(!WaitScb(FdoData))
    {
        status = STATUS_DEVICE_DATA_ERROR;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- MPSetUpFilterCB\n");

    return status;


}

NTSTATUS
MPIssueScbPoMgmtCommand(
    IN PFDO_DATA          FdoData,
    IN PCSR_FILTER_STRUC  pNewFilter,
    IN BOOLEAN            WaitForScb
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER( pNewFilter );
    UNREFERENCED_PARAMETER( WaitForScb );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER,
                "--> MPIssueScbPoMgmtCommand\n");

    do
    {
        // Set up SCB to issue this command

        status = MPSetUpFilterCB(FdoData);

        if (status != STATUS_SUCCESS)
        {
            break;
        }

        // Submit the configure command to the chip, and wait for
        // it to complete.

        FdoData->CSRAddress->ScbGeneralPointer = FdoData->NonTxCmdBlockPhys;

        status = D100SubmitCommandBlockAndWait(FdoData);

        if(status != STATUS_SUCCESS)
        {
            status = STATUS_DEVICE_DATA_ERROR;
            break;
        }

    } WHILE (FALSE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER,
                "<-- MPIssueScbPoMgmtCommand %x\n", status);

    return status;
}



NTSTATUS
MPCalculateE100PatternForFilter (
    IN PUCHAR pFrame,
    IN ULONG FrameLength,
    IN PUCHAR pMask,
    IN ULONG MaskLength,
    OUT PULONG pSignature
    )
/*++
Routine Description:

    This function outputs the E100 specific Pattern Signature
    used to wake up the machine.

    Section C.2.4 - CRC word calculation of a Flexible Filer


Arguments:

    pFrame                  - Pattern Set by the protocols
    FrameLength             - Length of the Pattern
    pMask                   - Mask set by the Protocols
    MaskLength              - Length of the Mask
    pSignature              - caller allocated return structure

Return Value:
    Returns Success
    Failure - if the Pattern is greater than 129 bytes

--*/
{

    const ULONG Coefficients  = 0x04c11db7;
    ULONG Signature = 0;
    ULONG n = 0;
    ULONG i= 0;
    PUCHAR pCurrentMaskByte = pMask - 1; // init to -1
    ULONG MaskOffset = 0;
    ULONG BitOffsetInMask = 0;
    ULONG MaskBit = 0;
    ULONG ShiftBy = 0;
    UCHAR FrameByte = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> MPCalculateE100PatternForFilter\n");

    *pSignature = 0;

    do
    {
        if (FrameLength > 128)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        // The E100 driver can only accept 3 DWORDS of Mask in a single pattern
        if (MaskLength > (3*sizeof(ULONG)))
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        for (n=i=0;(n<128) && (n < FrameLength); ++n)
        {

            // The first half deals with the question -
            // Is the nth Frame byte to be included in the Filter
            //

            BitOffsetInMask =  (n % 8);

            if (BitOffsetInMask == 0)
            {
                //
                // We need to move to a new byte.
                // [0] for 0th byte, [1] for 8th byte, [2] for 16th byte, etc.
                //
                MaskOffset = n/8; // This is the new byte we need to go

                //
                //
                if (MaskOffset == MaskLength)
                {
                    break;
                }

                pCurrentMaskByte ++;
                ASSERT (*pCurrentMaskByte == pMask[n/8]);
            }


            // Now look at the actual bit in the mask
            MaskBit = 1 << BitOffsetInMask ;

            // If the current Mask Bit is set in the Mask then
            // we need to use it in the CRC calculation, otherwise we ignore it

            if (! (MaskBit & pCurrentMaskByte[0]))
            {
                continue;
            }

            // We are suppossed to take in the current byte as part of the CRC calculation
            // Initialize the variables
            FrameByte = pFrame[n];
            ShiftBy = (i % 3 )  * 8;

            ASSERT (ShiftBy!= 24); // Bit 24 is never used

            if (Signature & 0x80000000)
            {
                Signature = ((Signature << 1) ^ ( FrameByte << ShiftBy) ^ Coefficients);
            }
            else
            {
                Signature = ((Signature << 1 ) ^ (FrameByte << ShiftBy));
            }
            ++i;

        }

        // Clear bits 22-31
        Signature &= 0x00ffffff;

        // Update the result
        *pSignature = Signature;

        // We have succeeded
        status = STATUS_SUCCESS;

    } WHILE (FALSE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- MPCalculateE100PatternForFilter\n");

    return status;
}


VOID
MPCreateProgrammableFilter (
    IN PMP_WAKE_PATTERN     pMpWakePattern ,
    IN PUCHAR pFilter,
    IN OUT PULONG pNext
    )
/*++
Routine Description:

    This function outputs the E100 specific Pattern Signature
    used to wake up the machine.

    Section C.2.4 - Load Programmable Filter page C.20


Arguments:

    pMpWakePattern    - Filter will be created for this pattern,
    pFilter         - Filter will be stored here,
    pNext           - Used for validation . This Ulong will also be incremented by the size
                        of the filter (in ulongs)

Return Value:

--*/
{
    PUCHAR pCurrentByte = pFilter;
    ULONG NumBytesWritten = 0;
    PULONG pCurrentUlong = (PULONG)pFilter;
    PNDIS_PM_PACKET_PATTERN pNdisPattern = (PNDIS_PM_PACKET_PATTERN)(&pMpWakePattern->Pattern[0]);
    ULONG LengthOfFilter = 0;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> MPCreateProgrammableFilter\n");

    // Is there enough room for this pattern
    //
    {
        // Length in DWORDS
        LengthOfFilter = pNdisPattern->MaskSize /4;

        if (pNdisPattern->MaskSize % 4 != 0)
        {
            LengthOfFilter++;
        }

        // Increment LengthOfFilter to account for the 1st DWORD
        LengthOfFilter++;

        // We are only allowed 16 DWORDS in a filter
        if (*pNext + LengthOfFilter >= 16)
        {
            // Failure - early exit
            return;
        }

    }
    // Clear the Predefined bit; already cleared in the previous function.
    // first , initialize    -
    *pCurrentUlong = 0;

    // Mask Length goes into Bits 27-29 of the 1st DWORD. MaskSize is measured in DWORDs
    {
        ULONG dwMaskSize = pNdisPattern->MaskSize /4;
        ULONG dwMLen = 0;


        // If there is a remainder a remainder then increment
        if (pNdisPattern->MaskSize % 4 != 0)
        {
            dwMaskSize++;
        }


        //
        // If we fail this assertion, it means our
        // MaskSize is greater than 16 bytes.
        // This filter should have been failed upfront at the time of the request
        //

        ASSERT (0 < dwMaskSize && dwMaskSize < 5);
        //
        // In the Spec, 0 - Single DWORD maske, 001 -  2 DWORD mask,
        // 011 - 3 DWORD  mask, 111 - 4 Dword Mask.
        //

        if (dwMaskSize == 1) dwMLen = 0;
        if (dwMaskSize == 2) dwMLen = 1;
        if (dwMaskSize == 3) dwMLen = 3;
        if (dwMaskSize == 4) dwMLen = 7;

        // Adjust the Mlen, so it is in the correct position

        dwMLen = (dwMLen << 3);



        if (dwMLen != 0)
        {
            ASSERT (dwMLen <= 0x38 && dwMLen >= 0x08);
        }

        // These go into bits 27,28,29 (bits 3,4 and 5 of the 4th byte)
        pCurrentByte[3] |=  dwMLen ;


    }

    // Add  the signature to bits 0-23 of the 1st DWORD
    {
        PUCHAR pSignature = (PUCHAR)&pMpWakePattern->Signature;


        // Bits 0-23 are also the 1st three bytes of the DWORD
        pCurrentByte[0] = pSignature[0];
        pCurrentByte[1] = pSignature[1];
        pCurrentByte[2] = pSignature[2];

    }


    // Lets move to the next DWORD. Init variables
    pCurrentByte += 4 ;
    NumBytesWritten = 4;
    pCurrentUlong = (PULONG)pCurrentByte;

    // We Copy in the Mask over here
    {
        // The Mask is at the end of the pattern

        PUCHAR pMask = (PUCHAR)pNdisPattern + sizeof(*pNdisPattern);

        //Dump (pMask,pNdisPattern->MaskSize, 0,1);

        RtlMoveMemory (pCurrentByte, pMask, pNdisPattern->MaskSize);

        NumBytesWritten += pNdisPattern->MaskSize;

    }


    // Update the output value
    {
        ULONG NumUlongs = (NumBytesWritten /4);

        if ((NumBytesWritten %4) != 0)
        {
            NumUlongs ++;
        }

        ASSERT (NumUlongs == LengthOfFilter);

        *pNext = *pNext + NumUlongs;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- MPCreateProgrammableFilter\n");

    return;
}

NTSTATUS
MPSetPowerD0(
    PFDO_DATA  FdoData
    )
/*++
Routine Description:

    This routine is called when the adapter receives a SetPower
    to D0.

Arguments:

    Adapter                 Pointer to the adapter structure
    PowerState              NewPowerState

Return Value:


--*/
{
    NTSTATUS            status;
    //KIRQL               oldIrql;

    //
    // MPSetPowerD0Private Initializes the adapte, issues a selective reset.
    //
    MPSetPowerD0Private (FdoData);
    ASSERT(FdoData->DevicePowerState == PowerDeviceD0);
    //
    // Set up the packet filter
    //

    WdfSpinLockAcquire(FdoData->Lock);
    status = NICSetPacketFilter(
                 FdoData,
                 FdoData->OldPacketFilter);
    //
    // If Set Packet Filter succeeds, restore the old packet filter
    //
    if (status == STATUS_SUCCESS)
    {
        FdoData->PacketFilter = FdoData->OldPacketFilter;
    }


    WdfSpinLockRelease(FdoData->Lock);

    //
    // Set up the multicast list address
    //

    WdfSpinLockAcquire(FdoData->RcvLock);

    status = NICSetMulticastList(FdoData);

    NICStartRecv(FdoData);


    WdfSpinLockRelease(FdoData->RcvLock);

    return status;
}

NTSTATUS
MPSetPowerLow(
    PFDO_DATA              FdoData,
    WDF_POWER_DEVICE_STATE PowerState
    )
/*++
Routine Description:

    This routine is called when the FdoData receives a SetPower
    to a PowerState > D0

Arguments:

    FdoData                 Pointer to the FdoData structure
    PowerState              NewPowerState

Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    STATUS_DEVICE_DATA_ERROR

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( PowerState );

    //
    // Stop sending packets. Create a new flag and make it part
    // of the Send Fail Mask.  TODO: Does something need to happen here?
    //

    //
    // Stop hardware from receiving packets - Set the RU to idle.
    // TODO: Does something need to happen here?
    //

    //
    // Check the current status of the receive unit
    //
    if ((FdoData->CSRAddress->ScbStatus & SCB_RUS_MASK) != SCB_RUS_IDLE)
    {
        //
        // Issue an RU abort.  Since an interrupt will be issued, the
        // RU will be started by the DPC.
        //
        status = D100IssueScbCommand(FdoData, SCB_RUC_ABORT, TRUE);
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // MPSetPowerLowPrivate first disables the interrupt, acknowledges all the pending
    // interrupts and sets FdoData->DevicePowerState to the given low power state
    // then starts Hardware specific part of the transition to low power state
    // Setting up wake-up patterns, filters, wake-up events etc
    //
    // Interrupt is disabled and disconnected before entering D0Exit, so no need to
    // sychronize.
    //

    MPSetPowerLowPrivate(NULL, FdoData);

    return STATUS_SUCCESS;
}

BOOLEAN
MPAreTwoPatternsEqual(
    IN PNDIS_PM_PACKET_PATTERN pNdisPattern1,
    IN PNDIS_PM_PACKET_PATTERN pNdisPattern2
    )
/*++
Routine Description:

    This routine will compare two wake up patterns to see if they are equal

Arguments:

    pNdisPattern1 - Pattern1
    pNdisPattern2 - Pattern 2


Return Value:

    True - if patterns are equal
    False - Otherwise
--*/
{
    BOOLEAN bEqual = FALSE;

    // Local variables used later in the compare section of this function
    PUCHAR  pMask1, pMask2;
    PUCHAR  pPattern1, pPattern2;
    UINT    MaskSize, PatternSize;

    do
    {

        bEqual = (BOOLEAN)(pNdisPattern1->Priority == pNdisPattern2->Priority);

        if (bEqual == FALSE)
        {
            break;
        }

        bEqual = (BOOLEAN)(pNdisPattern1->MaskSize == pNdisPattern2->MaskSize);
        if (bEqual == FALSE)
        {
            break;
        }

        //
        // Verify the Mask
        //
        MaskSize = pNdisPattern1->MaskSize ;
        pMask1 = (PUCHAR) pNdisPattern1 + sizeof (NDIS_PM_PACKET_PATTERN);
        pMask2 = (PUCHAR) pNdisPattern2 + sizeof (NDIS_PM_PACKET_PATTERN);

        bEqual = (BOOLEAN)RtlEqualMemory (pMask1, pMask2, MaskSize);

        if (bEqual == FALSE)
        {
            break;
        }

        //
        // Verify the Pattern
        //
        bEqual = (BOOLEAN)(pNdisPattern1->PatternSize == pNdisPattern2->PatternSize);

        if (bEqual == FALSE)
        {
            break;
        }

        PatternSize = pNdisPattern2->PatternSize;
        pPattern1 = (PUCHAR) pNdisPattern1 + pNdisPattern1->PatternOffset;
        pPattern2 = (PUCHAR) pNdisPattern2 + pNdisPattern2->PatternOffset;

        bEqual  = (BOOLEAN)RtlEqualMemory (pPattern1, pPattern2, PatternSize );

        if (bEqual == FALSE)
        {
            break;
        }

    } WHILE (FALSE);

    return bEqual;
}

VOID
NICExtractPMInfoFromPciSpace(
    PFDO_DATA FdoData,
    PUCHAR pPciConfig
    )
/*++
Routine Description:

    Looks at the PM information in the
    device specific section of the PCI Config space.

    Interprets the register values and stores it
    in the adapter structure

    Definitions from Table 4.2 & 4.3, Pg 4-9 & 4-10
    of the 10/100 Mbit Ethernet Family Software Technical
    Reference Manual


Arguments:

    Adapter     Pointer to our adapter
    pPciConfig  Pointer to Common Pci Space

Return Value:

--*/
{
    PMP_PM_PCI_SPACE    pPmPciConfig = (PMP_PM_PCI_SPACE )pPciConfig;
    MP_PMCSR PMCSR;

    //
    // First interpret the PM Capabities register
    //
    {
        MP_PM_CAP_REG   PmCaps;

        PmCaps = pPmPciConfig->PMCaps;

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D0)
        {
            FdoData->PoMgmt.bWakeFromD0 = TRUE;
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D1)
        {
            FdoData->PoMgmt.bWakeFromD1 = TRUE;
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D2)
        {
            FdoData->PoMgmt.bWakeFromD2 = TRUE;
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D3HOT)
        {
            FdoData->PoMgmt.bWakeFromD3Hot = TRUE;
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D3_AUX)
        {
            FdoData->PoMgmt.bWakeFromD3Aux = TRUE;
        }

    }

    //
    // Interpret the PM Control/Status Register
    //
    {
        PMCSR = pPmPciConfig->PMCSR;

        if (PMCSR.PME_En == 1)
        {
            //
            // PME is enabled. Clear the PME_En bit.
            // So that it is not asserted
            //
            MpClearPME_En (FdoData,PMCSR);

        }

    }

}


NTSTATUS
NICSetPower(
    PFDO_DATA     FdoData ,
    WDF_POWER_DEVICE_STATE   PowerState
    )
/*++
Routine Description:

    This routine is called when the FdoData receives a SetPower
    request. It redirects the call to an appropriate routine to
    Set the New PowerState

Arguments:

    FdoData                 Pointer to the FdoData structure
    PowerState              NewPowerState

Return Value:

    NTSTATUS Code

--*/
{
    NTSTATUS      status = STATUS_SUCCESS;

    if(IsPoMgmtSupported(FdoData)){

        if (PowerState == PowerDeviceD0)
        {
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "Entering fully on state\n");
            MPSetPowerD0 (FdoData);
        }
        else
        {
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "Entering a deeper sleep state\n");
            status = MPSetPowerLow (FdoData, PowerState);
        }
    }

    return status;
}



NTSTATUS
NICAddWakeUpPattern(
    IN PFDO_DATA  FdoData,
    IN PVOID        InformationBuffer,
    IN UINT         InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded
    )
/*++
Routine Description:

    This routine will allocate a local memory structure, copy the pattern,
    insert the pattern into a linked list and return success

    We are gauranteed that we wll get only one request at a time, so this is implemented
    without locks.

Arguments:

    FdoData                 FdoData structure
    InformationBuffer       Wake up Pattern
    InformationBufferLength Wake Up Pattern Length

Return Value:

    STATUS_Success - if successful.
    STATUS_UNSUCCESSFUL - if memory allocation fails.

--*/
{

    NTSTATUS             status = STATUS_UNSUCCESSFUL;
    PMP_WAKE_PATTERN        pWakeUpPattern = NULL;
    ULONG                   AllocationLength = 0;
    PNDIS_PM_PACKET_PATTERN pPmPattern = NULL;
    ULONG                   Signature = 0;
    ULONG                   CopyLength = 0;
    ULONG                   safeAddResult;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> NICAddWakeUpPattern\n");

    do
    {

        if(!FdoData->AllowWakeArming) {
            status =  STATUS_NOT_SUPPORTED;
            break;
        }

        pPmPattern = (PNDIS_PM_PACKET_PATTERN) InformationBuffer;

        if (InformationBufferLength < sizeof(NDIS_PM_PACKET_PATTERN))
        {
            status = STATUS_BUFFER_TOO_SMALL;

            *BytesNeeded = sizeof(NDIS_PM_PACKET_PATTERN);
            break;
        }

        //
        // safeAddResult = pPmPattern->PatternOffset + pPmPattern->PatternSize
        //
        status = RtlULongAdd(
            pPmPattern->PatternOffset,
            pPmPattern->PatternSize,
            &safeAddResult) ;
        if (!NT_SUCCESS(status))
        {
            break;
        }
        if (InformationBufferLength < safeAddResult)
        {
            status = STATUS_BUFFER_TOO_SMALL;

            *BytesNeeded = safeAddResult;
            break;
        }

        *BytesRead = safeAddResult;


        //
        // Calculate the e100 signature
        //
        status = MPCalculateE100PatternForFilter (
            (PUCHAR)pPmPattern+ pPmPattern->PatternOffset,
            pPmPattern->PatternSize,
            (PUCHAR)pPmPattern +sizeof(NDIS_PM_PACKET_PATTERN),
            pPmPattern->MaskSize,
            &Signature );

        if ( status != STATUS_SUCCESS)
        {
            break;
        }

        CopyLength = safeAddResult;

        //
        // Allocate the memory to hold the WakeUp Pattern
        //
        // AllocationLength = sizeof (MP_WAKE_PATTERN) + CopyLength;
        //
        status = RtlULongAdd(
            sizeof(MP_WAKE_PATTERN),
            CopyLength,
            &AllocationLength);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        pWakeUpPattern = ExAllocatePool2(POOL_FLAG_NON_PAGED, AllocationLength, PCIDRV_POOL_TAG);

        if (!pWakeUpPattern)
        {
            break;
        }

        //
        // Initialize pWakeUpPattern
        //
        pWakeUpPattern->AllocationSize = AllocationLength;

        pWakeUpPattern->Signature = Signature;

        //
        // Copy the pattern into local memory
        //
        RtlMoveMemory (&pWakeUpPattern->Pattern[0], InformationBuffer, CopyLength);

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

        //
        // Insert the pattern into the list
        //
       /* ExInterlockedInsertHeadList (&FdoData->PoMgmt.PatternList,
                                        &pWakeUpPattern->linkListEntry,
                                        &FdoData->Lock);
                                        */

        WdfSpinLockAcquire(FdoData->Lock);
        InsertHeadList(&FdoData->PoMgmt.PatternList,&pWakeUpPattern->linkListEntry );
        WdfSpinLockRelease(FdoData->Lock);


        status = STATUS_SUCCESS;

    } WHILE (FALSE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- NICAddWakeUpPattern\n");

    return status;
}

NTSTATUS
NICRemoveWakeUpPattern(
    IN PFDO_DATA  FdoData,
    IN PVOID        InformationBuffer,
    IN UINT         InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded
    )
/*++
Routine Description:

    This routine will walk the list of wake up pattern and attempt to match the wake up pattern.
    If it finds a copy , it will remove that WakeUpPattern

Arguments:

    FdoData                 FdoData structure
    InformationBuffer       Wake up Pattern
    InformationBufferLength Wake Up Pattern Length

Return Value:

    Success - if successful.
    STATUS_UNSUCCESSFUL - if memory allocation fails.

--*/
{

    NTSTATUS              status = STATUS_UNSUCCESSFUL;
    PNDIS_PM_PACKET_PATTERN  pReqPattern = (PNDIS_PM_PACKET_PATTERN)InformationBuffer;
    PLIST_ENTRY              pPatternEntry = ListNext(&FdoData->PoMgmt.PatternList) ;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> NICRemoveWakeUpPattern\n");

    do
    {
        if(!FdoData->AllowWakeArming) {
            status =  STATUS_NOT_SUPPORTED;
            break;
        }

        if (InformationBufferLength < sizeof(NDIS_PM_PACKET_PATTERN))
        {
            status = STATUS_BUFFER_TOO_SMALL;

            *BytesNeeded = sizeof(NDIS_PM_PACKET_PATTERN);
            break;
        }
        if (InformationBufferLength < pReqPattern->PatternOffset + pReqPattern->PatternSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;

            *BytesNeeded = pReqPattern->PatternOffset + pReqPattern->PatternSize;
            break;
        }

        *BytesRead = pReqPattern->PatternOffset + pReqPattern->PatternSize;

        while (pPatternEntry != (&FdoData->PoMgmt.PatternList))
        {
            BOOLEAN                  bIsThisThePattern = FALSE;
            PMP_WAKE_PATTERN         pWakeUpPattern = NULL;
            PNDIS_PM_PACKET_PATTERN  pCurrPattern = NULL;;

            //
            // initialize local variables
            //
            pWakeUpPattern = CONTAINING_RECORD(pPatternEntry, MP_WAKE_PATTERN, linkListEntry);

            pCurrPattern = (PNDIS_PM_PACKET_PATTERN)&pWakeUpPattern->Pattern[0];

            //
            // increment the iterator
            //
            pPatternEntry = ListNext (pPatternEntry);

            //
            // Begin Check : Is (pCurrPattern  == pReqPattern)
            //
            bIsThisThePattern = MPAreTwoPatternsEqual(pReqPattern, pCurrPattern);


            if (bIsThisThePattern == TRUE)
            {
                //
                // we have a match - remove the entry
                //
                RemoveEntryList (&pWakeUpPattern->linkListEntry);

                //
                // Free the entry
                //
                ExFreePoolWithTag(pWakeUpPattern, PCIDRV_POOL_TAG);

                status = STATUS_SUCCESS;
                break;
            }

        }

    } WHILE (FALSE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- NICRemoveWakeUpPattern\n");

    return status;
}



VOID
NICRemoveAllWakeUpPatterns(
    PFDO_DATA FdoData
    )
/*++
Routine Description:

    This routine will walk the list of wake up pattern and free it

Arguments:

    FdoData                 FdoData structure

Return Value:

    Success - if successful.

--*/
{

    PLIST_ENTRY  pPatternEntry = ListNext(&FdoData->PoMgmt.PatternList) ;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "--> NICRemoveAllWakeUpPatterns\n");

    while (pPatternEntry != (&FdoData->PoMgmt.PatternList))
    {
        PMP_WAKE_PATTERN  pWakeUpPattern = NULL;

        //
        // initialize local variables
        //
        pWakeUpPattern = CONTAINING_RECORD(pPatternEntry, MP_WAKE_PATTERN,linkListEntry);

        //
        // increment the iterator
        //
        pPatternEntry = ListNext (pPatternEntry);

        //
        // Remove the entry from the list
        //
        RemoveEntryList (&pWakeUpPattern->linkListEntry);

        //
        // Free the memory
        //
        ExFreePoolWithTag(pWakeUpPattern, PCIDRV_POOL_TAG);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "<-- NICRemoveAllWakeUpPatterns\n");

}


NTSTATUS
NICConfigureForWakeUp(
    IN PFDO_DATA FdoData,
    IN BOOLEAN  AddPattern
    )
/*++
Routine Description:


Arguments:

    FdoData                 FdoData structure

Return Value:

    Success - if successful.

--*/
{
#define MAX_WAKEUP_PATTERN_LENGTH  128

    UCHAR           Buffer[sizeof(NDIS_PM_PACKET_PATTERN) +
                                MAX_WAKEUP_PATTERN_LENGTH];
    PCHAR           patternBuffer, nextMask, nextPattern;
    ULONG           maskLen;
    PNDIS_PM_PACKET_PATTERN ndisPattern;
    ULONG           bufLen;
    NTSTATUS        status;
    ULONG           unUsed;
    CHAR            wakePattern[]={0xff,0xff,0xff,0xff,0xff,0xff}; //broadcast address

    patternBuffer = (PCHAR)&Buffer[0];

    ndisPattern = (PNDIS_PM_PACKET_PATTERN)patternBuffer;
    RtlZeroMemory(ndisPattern, sizeof(NDIS_PM_PACKET_PATTERN));


    ndisPattern->PatternSize = sizeof(wakePattern);

    maskLen = (ndisPattern->PatternSize-1)/8 + 1;

    nextMask = (PCHAR)patternBuffer + sizeof(NDIS_PM_PACKET_PATTERN);

    nextPattern = nextMask + maskLen;

    *nextMask = 0x3f;

    ndisPattern->MaskSize = maskLen;
    ndisPattern->PatternOffset = (ULONG) ((ULONG_PTR) nextPattern - (ULONG_PTR) patternBuffer);

    bufLen = sizeof(NDIS_PM_PACKET_PATTERN) + maskLen + ndisPattern->PatternSize;

    RtlCopyMemory(nextPattern, FdoData->CurrentAddress, ETHERNET_ADDRESS_LENGTH);

    if(AddPattern){
        status = NICAddWakeUpPattern(FdoData, Buffer, bufLen, &unUsed, &unUsed);
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "NICAddWakeupPattern failed %x\n", status);
        }
    }else{
        status = NICRemoveWakeUpPattern(FdoData, Buffer, bufLen, &unUsed, &unUsed);
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "NICRemoveWakeUpPattern failed %x\n", status);
        }
    }

    return status;
}

#if 0
NTSTATUS
NICConfigureForWakeUp(
    IN PFDO_DATA FdoData,
    IN BOOLEAN  AddPattern
    )
{
#define MAX_WAKEUP_PATTERN_LENGTH  128
#define ETHER_IP_ICMP_HEADER_SIZE  14+20+8

    UCHAR           Buffer[sizeof(NDIS_PM_PACKET_PATTERN) +
                                MAX_WAKEUP_PATTERN_LENGTH];
    PCHAR           patternBuffer, nextMask, nextPattern;
    ULONG           maskLen;
    PNDIS_PM_PACKET_PATTERN ndisPattern;
    ULONG           bufLen;
    NTSTATUS        status;
    ULONG           unUsed;
    CHAR            pingPattern[]={'a','b','c','d','e','f','g','h'};

    patternBuffer = (PCHAR)&Buffer[0];

    ndisPattern = (PNDIS_PM_PACKET_PATTERN)patternBuffer;
    RtlZeroMemory(ndisPattern, sizeof(NDIS_PM_PACKET_PATTERN));


    ndisPattern->PatternSize = ETHER_IP_ICMP_HEADER_SIZE + sizeof(pingPattern);

    maskLen = (ndisPattern->PatternSize-1)/8 + 1;

    nextMask = (PCHAR)patternBuffer + sizeof(NDIS_PM_PACKET_PATTERN);

    nextPattern = nextMask + maskLen;

    *nextMask = 0x0;nextMask++;
    *nextMask = 0x0;nextMask++;
    *nextMask = 0x0;nextMask++;
    *nextMask = 0x0;nextMask++;
    *nextMask = 0x0;nextMask++;
    *nextMask = 0x3f;nextMask++;
    *nextMask = 0x0C;

    ndisPattern->MaskSize = maskLen;
    ndisPattern->PatternOffset = (ULONG) ((ULONG_PTR) nextPattern - (ULONG_PTR) patternBuffer);

    bufLen = sizeof(NDIS_PM_PACKET_PATTERN) + maskLen + ndisPattern->PatternSize;

    RtlZeroMemory(nextPattern, ETHER_IP_ICMP_HEADER_SIZE);
    nextPattern += ETHER_IP_ICMP_HEADER_SIZE;

    RtlCopyMemory(nextPattern, pingPattern, sizeof(pingPattern));

    if(AddPattern){
        status = MPAddWakeUpPattern(FdoData, Buffer, bufLen, &unUsed, &unUsed);
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "MpAddWakeupPattern failed %x\n", status);
        }
    }else{
        status = MPRemoveWakeUpPattern(FdoData, Buffer, bufLen, &unUsed, &unUsed);
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_POWER, "MPRemoveWakeUpPattern failed %x\n", status);
        }
    }

    return status;
}

#endif



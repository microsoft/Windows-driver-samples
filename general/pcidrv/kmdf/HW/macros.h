/****************************************************************************
** COPYRIGHT (C) 1994-1997 INTEL CORPORATION                               **
** DEVELOPED FOR MICROSOFT BY INTEL CORP., HILLSBORO, OREGON               **
** HTTP://WWW.INTEL.COM/                                                   **
** THIS FILE IS PART OF THE INTEL ETHEREXPRESS PRO/100B(TM) AND            **
** ETHEREXPRESS PRO/100+(TM) NDIS 5.0 MINIPORT SAMPLE DRIVER               **
****************************************************************************/

/****************************************************************************
Module Name:
     macros.h     (inlinef.h)

This driver runs on the following hardware:
     - 82558 based PCI 10/100Mb ethernet adapters
     (aka Intel EtherExpress(TM) PRO Adapters)

Environment:
     Kernel Mode - Or whatever is the equivalent on WinNT

*****************************************************************************/

__inline BOOLEAN
WaitScb(
    IN PFDO_DATA FdoData
    );


//-----------------------------------------------------------------------------
// Procedure:   D100IssueScbCommand
//
// Description: This general routine will issue a command to the D100.
//
// Arguments:
//      FdoData - ptr to FdoData object instance.
//      ScbCommand - The command that is to be issued
//      WaitForSCB - A boolean value indicating whether or not a wait for SCB
//                   must be done before the command is issued to the chip
//
// Returns:
//      TRUE if the command was issued to the chip successfully
//      FALSE if the command was not issued to the chip
//-----------------------------------------------------------------------------
__inline NTSTATUS
D100IssueScbCommand(
    IN PFDO_DATA FdoData,
    IN UCHAR ScbCommandLow,
    IN BOOLEAN WaitForScb
    )
{
    if(WaitForScb == TRUE)
    {
        if(!WaitScb(FdoData))
        {
            return(STATUS_DEVICE_DATA_ERROR);
        }
    }

    FdoData->CSRAddress->ScbCommandLow = ScbCommandLow;

    return(STATUS_SUCCESS);
}


__inline NTSTATUS
MP_GET_STATUS_FROM_FLAGS(
    IN PFDO_DATA FdoData
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if(MP_TEST_FLAG(FdoData, fMP_ADAPTER_RESET_IN_PROGRESS))
    {
        Status = STATUS_DEVICE_NOT_READY;
    }
    else if(MP_TEST_FLAG(FdoData, fMP_ADAPTER_HARDWARE_ERROR))
    {
        Status = STATUS_DEVICE_OFF_LINE;
    }
    else if(MP_TEST_FLAG(FdoData, fMP_ADAPTER_NO_CABLE))
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }

    return Status;
}

__inline VOID
NICDisableInterrupt(
    IN PFDO_DATA FdoData
    )
{
   FdoData->CSRAddress->ScbCommandHigh = SCB_INT_MASK;
}

EVT_WDF_INTERRUPT_SYNCHRONIZE NICEnableInterrupt;

__inline BOOLEAN NICEnableInterrupt(
    IN WDFINTERRUPT WdfInterrupt,
    IN WDFCONTEXT Context
    )
{
    PFDO_DATA FdoData = (PFDO_DATA)Context;

    UNREFERENCED_PARAMETER(WdfInterrupt);

    FdoData->CSRAddress->ScbCommandHigh = 0;

    return TRUE;
}

__inline
BOOLEAN
IsPoMgmtSupported(
   IN PFDO_DATA FdoData
   )
{

    if (FdoData->RevsionID  >= E100_82559_A_STEP
         /*&& FdoData->RevsionID <= E100_82559_C_STEP*/)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

__inline
USHORT
NICReadPortUShort (
    IN  USHORT * x
    )
{
    return READ_PORT_USHORT (x);
}
__inline
VOID
NICWritePortUShort (
    IN  USHORT * x,
    IN  USHORT   y
    )
{
    WRITE_PORT_USHORT (x,y);
}

__inline
USHORT
NICReadRegisterUShort (
    IN  USHORT * x
    )
{
    return READ_REGISTER_USHORT (x);
}

__inline
VOID
NICWriteRegisterUShort (
    IN  USHORT * x,
    IN  USHORT   y
    )
{
    WRITE_REGISTER_USHORT (x,y);
}



// routines.c

BOOLEAN
MdiRead(
    IN PFDO_DATA Adapter,
    IN ULONG RegAddress,
    IN ULONG PhyAddress,
    IN BOOLEAN  Recoverable,
    IN OUT PUSHORT DataValue
    );

VOID
MdiWrite(
    IN PFDO_DATA FdoData,
    IN ULONG RegAddress,
    IN ULONG PhyAddress,
    IN USHORT DataValue
    );

NTSTATUS
D100IssueScbCommand(
    IN PFDO_DATA FdoData,
    IN UCHAR ScbCommandLow,
    IN BOOLEAN WaitForScb
    );

MEDIA_STATE
GetMediaState(
    IN PFDO_DATA Adapter
    );

NTSTATUS
D100SubmitCommandBlockAndWait(
    IN PFDO_DATA Adapter
    );

VOID
NICIssueFullReset(
    PFDO_DATA Adapter
    );

VOID
NICIssueSelectiveReset(
    PFDO_DATA Adapter
    );

VOID
DumpStatsCounters(
    IN PFDO_DATA Adapter
    );



// physet.c

VOID
ResetPhy(
    IN PFDO_DATA FdoData
    );

NTSTATUS
PhyDetect(
    IN PFDO_DATA FdoData
    );

NTSTATUS
ScanAndSetupPhy(
    IN PFDO_DATA FdoData
    );

VOID
SelectPhy(
    IN PFDO_DATA FdoData,
    IN UINT SelectPhyAddress,
    IN BOOLEAN WaitAutoNeg
    );

NTSTATUS
SetupPhy(
    IN PFDO_DATA FdoData
    );

VOID
FindPhySpeedAndDpx(
    IN PFDO_DATA FdoData,
    IN UINT PhyId
    );



// eeprom.c
USHORT
GetEEpromAddressSize(
    IN USHORT Size
    );

USHORT
GetEEpromSize(
    IN PFDO_DATA FdoData,
    IN PUCHAR CSRBaseIoAddress
    );

USHORT
ReadEEprom(
    IN PFDO_DATA FdoData,
    IN PUCHAR CSRBaseIoAddress,
    IN USHORT Reg,
    IN USHORT AddressSize
    );

VOID
ShiftOutBits(
    IN PFDO_DATA FdoData,
    IN USHORT data,
    IN USHORT count,
    IN PUCHAR CSRBaseIoAddress
    );

USHORT
ShiftInBits(
    IN PFDO_DATA FdoData,
    IN PUCHAR CSRBaseIoAddress
    );

VOID
RaiseClock(
    IN PFDO_DATA FdoData,
    IN OUT USHORT *x,
    IN PUCHAR CSRBaseIoAddress
    );

VOID
LowerClock(
    IN PFDO_DATA FdoData,
    IN OUT USHORT *x,
    IN PUCHAR CSRBaseIoAddress
    );

VOID
EEpromCleanup(
    IN PFDO_DATA FdoData,
    IN PUCHAR CSRBaseIoAddress
    );


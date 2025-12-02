/*++

Module Name:

    lsi_u3.c

Abstract:

    This is a sample driver that contains intentionally placed
    code defects in order to illustrate how Static Driver Verifier
    works. This driver is not functional and not intended as a 
    sample for real driver development projects.

Environment:

    Kernel mode

--*/


/*
*************************************************************************
*                                                                       *
*   Copyright 1994-2008 LSI Corporation. All rights reserved.           *
*                                                                       *
************************************************************************/

#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4213) // nonstandard extension used : cast on l-value
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int
#pragma warning(disable:4701) // potentially uninitialized local variable 'n' used

/**********************************************************************/

#define FORCE_SYNC  // this define will default the driver to force sync.
/**********************************************************************/

//
// include files used by the Miniport
//
#include "miniport.h"
#include "storport.h"
#include "lsi_u3.h"

#include "scr_u3m.h"    // memory mapped scripts

#include "lsisiop.h"
#include "lsinvm.h"
#include "lsisvdt.h"
#include "lsiver.h"

// headers for DMI support
#include "ntddscsi.h"
#include "lsidmi.h"

// Define the SRB Extension.
//

typedef struct _SRB_EXTENSION {
    UCHAR PhysBreakCount;                       // physical break count
    UCHAR SrbExtFlags;                          // negot & other flags
    UCHAR autoReqSns;                           // autoReqSns flag
    UCHAR trackEntry;                           // I/O track array entry
    PSVARS_DESCRIPTOR_TABLE svdt;               // pointer to this svdt
    SVARS_DESCRIPTOR_TABLE svarsDescriptorTable; // descriptor table
    UCHAR AlignPad[4];

}SRB_EXTENSION, *PSRB_EXTENSION;


#define SRB_EXT(x) ((PSRB_EXTENSION)(x->SrbExtension))



// Define the noncached extension.  Data items are placed in the noncached
// extension because they are accessed via DMA.

typedef struct _HW_NONCACHED_EXTENSION {

    ULONG       ScsiScripts[ sizeof(SCRIPT) / 4 ];
    SVARS_DESCRIPTOR    NegotMsgBufDesc[SYM_MAX_TARGETS];
    NEGOT_BUF   NegotMsg[SYM_MAX_TARGETS];
    ULONG       dataXferParms[SYM_MAX_TARGETS];
    START_QUEUE_ENTRY  startQueue[START_Q_DEPTH];
    START_QUEUE_ENTRY  caQueue[CA_Q_DEPTH];
    DONE_QUEUE_ENTRY   doneQueue[DONE_Q_DEPTH];
    NEXUS_PTR   ITLnexusPtrs[256];
    UCHAR       alignPad[4];

} HW_NONCACHED_EXTENSION, *PHW_NONCACHED_EXTENSION;

//
// Define the LSI_U3 Device Extension structure
//

typedef struct _HW_DEVICE_EXTENSION {

    PHW_NONCACHED_EXTENSION NonCachedExtension; // pointer to noncached
                                                // device extension
    PSIOP_REGISTER_BASE SIOPRegisterBase;       // 53C1010 SIOP register base.
    ULONG SIOPRegisterBasePhys;                 // physical ptr for patching

    PSVARS  svars;                      // ptr to svarsTable structure
    ULONG   *dxp;                       // ptr to data xfer table, all devices
    ULONG   *dxpSR;                     // ptr to data xfer table in SRam
    ULONG   old_dxp;                    // dxp value at start of req sns negots

    PSTART_QUEUE_ENTRY  ioStartQueue;   // ptr to scripts start queue
    ULONG               ioStartQPhys;   // physical ptr
    PSTART_QUEUE_ENTRY  caStartQueue;   // ptr to scripts ca queue
    ULONG               caStartQPhys;   // physical ptr
    PDONE_QUEUE_ENTRY   ioDoneQueue;    // ptr to scripts completion queue
    ULONG               ioDoneQPhys;    // physical ptr
    ULONG               ioStartQIndex;  // offset with in start queue
    ULONG               ioDoneQIndex;   // offset with in completion queue
    PULONG              DQ_entrySR;     // ptr to DQ_entry in Scripts RAM
    PULONG              scriptStopFlag; // ptr to scriptStopFlag in Scripts RAM

    PNEXUS_PTR  ITQnexusTable;          // ptr to tagged nexus pointer array
    ULONG       ITQnexusTablePhys;      // physical ptr
    PNEXUS_PTR  ITLnexusTable;          // ptr to untagged nexus pointer array
    ULONG       ITLnexusTablePhys;      // physical ptr

    PSVARS_DESCRIPTOR_TABLE localSvdt;  // pointer to local svdt in SRam
    ULONG       localIovPhys;           // physical ptr to local iov list

    SVARS_DESCRIPTOR   deviceDesc;      // template for deviceDescriptor
    SVARS_DESCRIPTOR   cmdBufDesc;      // template for cmdBufDescriptor
    SVARS_DESCRIPTOR   msgOutDesc;      // template for msgOutDescriptor

    USHORT  DeviceFlags;        // bus specific flags
    UCHAR   SIOPBusID;          // SCSI bus ID in integer form.

// script physical address entry points follow...

    ULONG RestartScriptPhys;      // phys ptr to restart script
    ULONG CommandScriptPhys;      // phys ptr to cmd start script
    ULONG SendIDEScriptPhys;      // phys ptr to IDE message script
    ULONG RejectScriptPhys;       // phys ptr to reject message script
    ULONG ContNegScriptPhys;      // phys ptr to continue negotiations script
    ULONG Add2CaQScriptPhys;      // phys ptr to add to auto req sns queue
    ULONG PhaseMisJump1Phys;      // phys ptr to phase mismatch jump 1
    ULONG PhaseMisJump2Phys;      // phys ptr to phase mismatch jump 2
    ULONG PhaseMisJump64Phys;     // phys ptr to phase mismatch jump 64
    ULONG DQ_entryPhys;           // phys ptr to DQ_entry storage
    ULONG DataOutJump1Phys;       // phys ptr to DataOutJump1

// define pointers to the active logical unit object

    PSCSI_REQUEST_BLOCK ActiveRequest;      // pointer to active LU

// logical unit specific flags and logical unit index

    USHORT LuFlags[SYM_MAX_TARGETS];        // logical unit spec. flags

    USHORT hbaCapability;           // HBA capabilities bit-field
    UCHAR chip_rev;                 // chip revision

    UCHAR ClockSpeed;               // SIOP clock speed
    BOOLEAN IO_blocked;             // flag to indicate I/O's are blocked
    BOOLEAN StopAdapter;            // flag to indicate HBA is shutdown

// target intiated negotiation values from target
    UCHAR tin_rec_period;           // requested period
    UCHAR tin_rec_offset;           // requested offset
    UCHAR tin_rec_DT;               // requested DT transfers

    ULONG ScriptRamPhys;            // start of Scripts RAM (physical)
    ULONG_PTR ScriptRamVirt;        // start of Scripts RAM (virtual)
    ULONG_PTR ScriptStartVirt;      // start of Scripts instructions (virtual)

// extra resources for the nvram/NVS values
    DEVICE_TABLE    DeviceTable[HW_MAX_DEVICES];
    UINT8   Gpio[5];            // GPIO pin usage table (pin 0 placeholder)
    UINT8   HostSCSIId;         // HBA initiator ID
    USHORT  TerminatorState;    // SCSI termination state
    ULONG   BiosCodeSpacePtr;   // pointer to BIOS code space (read NVS)
    ULONG   NVSDataOffset;      // offset of first NVS data area
    ULONG   IoPortAddress;      // saved off to identify device in NVS area
    ULONG   PciBusSlot;         // saved PCI bus/slot numbers for NVConfig

// crash dump indicator         
    ULONG   crash_dump;         // flag to indicate memory dump mode

// reset active flag
    ULONG   ResetActive;        // non-zero indicates a device reset is active

// queue tag FIFO and pointers
    UCHAR   QTagFIFO[START_Q_DEPTH];    // FIFO for assigning queue tag values
    ULONG   QTagFree;                   // pointer to next free queue tag
    ULONG   QTagPost;                   // pointer to next tag re-post entry

// I/O tracking array and track array FIFO
    IO_TRACK_ENTRY  IoTrackArray[START_Q_DEPTH];    // I/O tracking array
    UCHAR           IoTrackFIFO[START_Q_DEPTH];     // FIFO to find next entry
    ULONG           TrackFree;                      // pointer to next free
    ULONG           TrackPost;                      // pointer to next post

// data and clock masks for NVRAM access
    UCHAR data_mask;            // data mask for reading NVRAM
    UCHAR clock_mask;           // clock mask for reading MVRAM

// flag for NTLDR context
    UCHAR ntldr_flag;           // flag indicating NTLDR mode

// flag for IRQ_not_connected check
    BOOLEAN IRQ_received;       // flag indicating we have received interrupts

// DMI data structure
    DMI_DATA DmiData;           // DMI (CIM) data for IOCTL

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

// predefined message buffers for wide/sync negotiation messages
UCHAR narrow_msg[4] = { 0x01, 0x02, 0x03, 0x00 };
UCHAR async_msg[5] = { 0x01, 0x03, 0x01, 0x19, 0x00 };
UCHAR ppr_msg[8] = { 0x01, 0x06, 0x04, 0x19, 0x00, 0x00, 0x00, 0x00 };

// version string for DMI
UCHAR driver_version[] = LSI_VERSION_LABEL;   // driver version string

//
// LSI_U3 miniport driver function declarations.
//

BOOLEAN  NvmDetect( PHW_DEVICE_EXTENSION DeviceExtension );
void  NvmSendStop( PHW_DEVICE_EXTENSION DeviceExtension );
void  NvmSendStart( PHW_DEVICE_EXTENSION DeviceExtension );
UINT  NvmSendData( PHW_DEVICE_EXTENSION DeviceExtension, UINT Value );
UINT8  NvmReadData( PHW_DEVICE_EXTENSION DeviceExtension );
void  NvmSendAck( PHW_DEVICE_EXTENSION DeviceExtension );
UINT  NvmReceiveAck( PHW_DEVICE_EXTENSION DeviceExtension );
void  NvmSendNoAck( PHW_DEVICE_EXTENSION DeviceExtension);
BOOLEAN  NVSDetect( PHW_DEVICE_EXTENSION DeviceExtension);
BOOLEAN  RetrieveNVSData( PHW_DEVICE_EXTENSION DeviceExtension);
VOID FillNvmData( PHW_DEVICE_EXTENSION DeviceExtension,
                  PTR_NON_VOLATILE_SETTINGS pNVM);
BOOLEAN ReadNVM( PHW_DEVICE_EXTENSION DeviceExtension, PVOID dataBuf,
                 PULONG ret_length);
BOOLEAN WriteNVM( PHW_DEVICE_EXTENSION DeviceExtension, _In_reads_bytes_(length) PVOID dataBuf,
                   _In_range_(1, sizeof(NON_VOLATILE_SETTINGS)) ULONG length);
MEMORY_STATUS  HwReadNonVolatileMemory( PHW_DEVICE_EXTENSION DeviceExtension,
                                    UINT8 *Buffer, UINT Offset, UINT Length );
UINT16 CalculateCheckSum(UINT8 * PNvmData, UINT16 Length);
UINT8 CalculateMfgCheckSum(UINT8 * PNvmData, UINT16 Length);
VOID RetrieveMfgData( PHW_DEVICE_EXTENSION DeviceExtension);
void HwInitGpioPins( PHW_DEVICE_EXTENSION DeviceExtension);
char HwFindGpioPin( PHW_DEVICE_EXTENSION DeviceExtension, char usageCode,
                    PUCHAR ptrActiveLevel );
char HwReadGpioPin( PHW_DEVICE_EXTENSION DeviceExtension, char usageCode);
char HwSetGpioPin( PHW_DEVICE_EXTENSION DeviceExtension, char usageCode,
                   char setting);
BOOLEAN RetrieveNvmData( PHW_DEVICE_EXTENSION DeviceExtension);
void InvalidateNvmData( PHW_DEVICE_EXTENSION DeviceExtension );
void data_output( PHW_DEVICE_EXTENSION DeviceExtension);
void data_input( PHW_DEVICE_EXTENSION DeviceExtension);
void set_data( PHW_DEVICE_EXTENSION DeviceExtension);
void reset_data( PHW_DEVICE_EXTENSION DeviceExtension);
void set_clock( PHW_DEVICE_EXTENSION DeviceExtension);
void reset_clock( PHW_DEVICE_EXTENSION DeviceExtension);
UCHAR set_1010_clock(PHW_DEVICE_EXTENSION DeviceExtension);
UCHAR set_sync_speed(PHW_DEVICE_EXTENSION DeviceExtension, UCHAR period);
VOID ISR_Service_Next(PHW_DEVICE_EXTENSION DeviceExtension,UCHAR ISRDisposition);
VOID doneQRemove(PHW_DEVICE_EXTENSION DeviceExtension, UCHAR IntStatus);
BOOLEAN EatInts(PHW_DEVICE_EXTENSION DeviceExtension);
void delay_mils( USHORT counter);
    
void NvmPageWrite( PHW_DEVICE_EXTENSION DeviceExtension,
                    UINT8 *Buffer, UINT Offset, UINT Pages );
void NvmByteWrite( PHW_DEVICE_EXTENSION DeviceExtension,
                    UINT8 *Buffer, UINT Offset, UINT Length );
MEMORY_STATUS  HwWriteNonVolatileMemory( PHW_DEVICE_EXTENSION DeviceExtension,
                                    UINT8 *Buffer, UINT Offset, UINT Length );

VOID
AbortCurrentScript(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
PreAbortScripts(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
BusResetPostProcess(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
CheckSubsysID(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ USHORT SubsystemID
    );

VOID
ComputeSCSIScriptVectors(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

/*ULONG
DriverEntry(
    _In_ PVOID DriverObject,
    _In_ PVOID Argument2
    );*/

VOID
InitializeSIOP(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID 
initializeSvdtQueue(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

SCSI_ADAPTER_CONTROL_STATUS
LsiU3AdapterControl(
    _In_ PVOID DevExt,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters
    );

/*ULONG
LsiU3FindAdapter(
    _In_ PVOID Context,
    _In_ PVOID Reserved1,
    _In_ PVOID Reserved2,
	// DDK has argument #4 as a PCHAR
    //_In_ PCSTR ArgumentString,
    _In_ PCHAR ArgumentString,
	_Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PUCHAR Reserved3
    );

BOOLEAN
LsiU3HWInitialize(
    _In_ PVOID Context
    );

BOOLEAN
LsiU3ISR(
    _In_ PVOID Context
    );

BOOLEAN
LsiU3Reset(
    _In_ PVOID DeviceExtension,
    _In_ ULONG PathId
    );*/

BOOLEAN
SynchronizeReset(
    _In_ PVOID Context,
    _In_ PVOID ptrPathId
    );

/*BOOLEAN
LsiU3BuildIo(
    _In_ PVOID Context,
    _In_ PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
LsiU3StartIo(
    _In_ PVOID Context,
    _In_ PSCSI_REQUEST_BLOCK Srb
    );*/

UCHAR
ProcessBadDataDirection(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessBusResetReceived(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessCheckCondition(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
ProcessCommandComplete(
    PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG statXferLen
    );

UCHAR
ProcessDeviceResetFailed(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessDeviceResetOccurred(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessDiffSenseChange(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessDMAInterrupt(
    PHW_DEVICE_EXTENSION DeviceExtension,
    UCHAR DmaStatus
    );

UCHAR
ProcessErrorMsgSent(
    VOID
    );

UCHAR
ProcessGrossError(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessIgnoreWideResidue(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessIllegalInstruction(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessInvalidReselect(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessNVConfigIoctl(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessParityError(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessPhaseMismatch(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

BOOLEAN
ProcessParseArgumentString(
	//_In_ PCSTR String,
	_In_ PCHAR String,
    //_In_ PCSTR WantedString,
	_In_ PCHAR WantedString,
    _Out_ PULONG ValueFound
    );

UCHAR
ProcessPprNegotComplete(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessRejectReceived(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessSaveDataPointers(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessSCSIInterrupt(
    PHW_DEVICE_EXTENSION DeviceExtension,
    UCHAR ScsiStatus
    );

UCHAR
ProcessSelectionTimeout(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessSynchNegotComplete(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessNegotNotSupported(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
ProcessUnexpectedDisconnect(
    PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
ScheduleReinit(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
TimerReinit(
    _In_ PVOID Context
    );

BOOLEAN
SynchronizeISRReinit(
    _In_ PVOID Context,
    _In_ PVOID dummy
    );

VOID
ResetSCSIBus(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

USHORT
ScatterGatherScriptSetup(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN hostSvdt
    );

VOID
SetChipModes (
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
SetupLuFlags(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR ResetFlag
    );

VOID
SetupNegotBuf(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR target,
    _In_ UCHAR negot_type
    );

UCHAR
ProcessWideNegotComplete(
    PHW_DEVICE_EXTENSION DeviceExtension
    );
      
VOID
StartAbortResetRequest(
    PSCSI_REQUEST_BLOCK Srb,
    PHW_DEVICE_EXTENSION DeviceExtension
    );

UCHAR
StartNegotiations(
    PHW_DEVICE_EXTENSION DeviceExtension,
    PSCSI_REQUEST_BLOCK Srb,
    UCHAR MessageCount,
    _In_ BOOLEAN hostSvdt
    );

VOID
StartNVConfigRequest(
    PSCSI_REQUEST_BLOCK Srb,
    PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
StartSCSIRequest(
    PSCSI_REQUEST_BLOCK Srb,
    PHW_DEVICE_EXTENSION DeviceExtension
    );

VOID
StartSIOP(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG ScriptPhysAddr
    );

VOID
UpdateStartQDesc(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
    );


VOID
AbortCurrentScript(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:


    This routine aborts the scripts to make sure they are NOT running.

Arguments:

    DeviceExtension - Supplies the device Extension for the SCSI bus adapter.

Return Value:

    none

--*/

{
    UCHAR IntStat;
    UCHAR AbortIteration = 0;
    UCHAR i;

    // mask off all interrupts
    WRITE_SIOP_UCHAR(SIEN0, 0);
    WRITE_SIOP_UCHAR(SIEN1, 0);
    WRITE_SIOP_UCHAR(DIEN, 0);

    // try to abort scripts a maximum of ABORT_SCRIPTS_TRIES only
    for (i = 0; i < ABORT_SCRIPTS_TRIES; i++)
    {
        // set abort bit to stop scripts
        WRITE_SIOP_UCHAR( ISTAT0, ISTAT_ABORT);

        do {
            IntStat = READ_SIOP_UCHAR( ISTAT0);

            // if we are in the second or greater iteration of this loop..
            if (AbortIteration++)  {

                // wait a moment
                StorPortStallExecution( ABORT_STALL_TIME);

                if (AbortIteration > MAX_ABORT_TRIES)  {

                    DebugPrint((1, "LsiU3(%2x):  AbortCurrentScript timeout - ISTAT0 = %x\n",
                        DeviceExtension->SIOPRegisterBase, IntStat));

                    // If can't get DMA_INT to set, assume abort worked and
                    // break to check DSTAT below.  If not set, will loop again.
                    break;

                }  // if
            } // if
        } while ( !( IntStat & ( ISTAT_SCSI_INT | ISTAT_DMA_INT)));

        if ( IntStat & ISTAT_SCSI_INT) {

            //  read SCSI interrupts to clear ISTAT.

            READ_SIOP_UCHAR(SIST0);
            READ_SIOP_UCHAR(SIST1);

            // retry aborting of scripts
            continue;
        } // if

        // A DMA interrupt has occured.
        WRITE_SIOP_UCHAR( ISTAT0, 0);

        // if interrupt was an ABORT, we're done so just break out of loop.
        if ( READ_SIOP_UCHAR( DSTAT) & DSTAT_ABORTED)
            break;

    } // for

} // AbortCurrentScript


VOID
PreAbortScripts(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine attempts to stop the Scripts via a flag in Scripts RAM, tested
    after issuing a SIGP.  This is to prevent doing an Abort via ISTAT0 while
    the Scripts are executing.  This is called prior to AbortCurrentScript.

Arguments:

    DeviceExtension - Supplies a pointer to device extension.

Return Value:

    None.

--*/

{
    UCHAR IntStat;
    int i;

    // mask off all interrupts
    WRITE_SIOP_UCHAR(SIEN0, 0);
    WRITE_SIOP_UCHAR(SIEN1, 0);
    WRITE_SIOP_UCHAR(DIEN, 0);

    // write the scriptsStopFlag
    StorPortWriteRegisterUlong( DeviceExtension,
                                DeviceExtension->scriptStopFlag, 1);
    // write SIGP
    WRITE_SIOP_UCHAR(ISTAT0, ISTAT_SIGP);
    // wait for an interrupt (anything but an INT_FLY)
    for (i = 0; i < 2000; i++)
    {
        StorPortStallExecution(999);
        IntStat = READ_SIOP_UCHAR( ISTAT0);
        // check for INT_FLY
        if ( IntStat & ISTAT_INTF )
        {
            // clear the INT_FLY interrupt
            WRITE_SIOP_UCHAR(ISTAT0, IntStat);
        }
        else if ( IntStat & (ISTAT_SCSI_INT | ISTAT_DMA_INT) )
        {
            // we have a hard interrupt, Scripts are stopped
            break;
        }
    }

    // clear the SIGP just in case
    WRITE_SIOP_UCHAR( ISTAT0, 0);

    // clear the scriptsStopFlag
#ifdef AMD64
    StorPortWriteRegisterUlong64( DeviceExtension,
                                  NULL,
                                  MAXULONG64);
#else
    StorPortWriteRegisterUlong( DeviceExtension,
                                DeviceExtension->scriptStopFlag,
                                0);
#endif

    // if we didn't get an interrupt either Scripts aren't running or we're
    // hung on the bus. In either case, just return and allow the Abort to
    // happen because there is no DMA occurring.

    return;

} // PreAbortScripts


VOID
BusResetPostProcess(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine aborts any pending requests after a bus reset is received.
    It also resets the LuFlags and re-initializes all the svdt queues.

Arguments:

    DeviceExtension - Supplies a pointer to device extension for the bus that
        was reset.

Return Value:

    None.

--*/

{
    SetupLuFlags(DeviceExtension,1);

    initializeSvdtQueue(DeviceExtension);

    DeviceExtension->ActiveRequest = NULL;

//***********************************************************************************************
//*     
//*     WARNING: Do not use StorPortCompleteRequest in your own miniport.     
//*     
//*     As discussed at the 2008 Microsoft Windows Driver Developer Conference, using
//*     StorPortCompleteRequest will lead to race conditions that could result in
//*     double-completions and never-completions. The recommended approach to handling
//*     resets is to loop through all requests currently owned by the miniport and complete
//*     each one using StorPortNotification( RequestComplete ).
//*
//***********************************************************************************************
    
    //_____________________________________________________________________
    // Defect injection for: StorPortDeprecated.slic 
    // (this call was already in the driver)
    // Part B, see part A in LsiU3ISR.
    StorPortCompleteRequest(DeviceExtension,
                            0,
                            SP_UNTAGGED,
                            SP_UNTAGGED,
                            SRB_STATUS_BUS_RESET
                            );
    //_____________________________________________________________________
	
    StorPortStallExecution(POST_RESET_STALL_TIME);

    return;

} // BusResetPostProcess


VOID
CheckSubsysID(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ USHORT SubsystemID
    )
/*++

Routine Description:

    This procedure decodes the PCI Subsystem ID bits to determine selected
    features and operation of the 1010 device and sets the appropriate bits
    in DeviceFlags.

Arguments:

    DeviceExtension - Pointer to the device extension for this adapter.

    SubsystemID - PCI Subsystem ID value retrieved from PCI config space.

Return Value:

    None

--*/

{
    union _subsysID {
        struct {
            USHORT Features : 7;
            USHORT SubType : 3;
            USHORT AdapType : 2;
            USHORT ConfigType : 3;
            USHORT ExcludeFlag : 1;
        } b;
        USHORT ssid;
    } subsysID;
    UCHAR cfg_typ;
    UCHAR adap_typ;
    UCHAR sub_typ;
    UCHAR feat;

    // move PCI Subsystem ID into local structure and variables
    subsysID.ssid = SubsystemID;
    cfg_typ = (UCHAR)subsysID.b.ConfigType;
    adap_typ = (UCHAR)subsysID.b.AdapType;
    sub_typ = (UCHAR)subsysID.b.SubType;
    feat = (UCHAR)subsysID.b.Features;

    // we don't look at the exclude flag since the miniport cannot exclude
    // a device once it is called.

    // check configuration type for non-supported values
    if ( !(cfg_typ == 1 || cfg_typ > 5) )
        return;

    // check adapter type for non-supported values
    if ( adap_typ > 1 || (adap_typ == 1 && sub_typ > 0) )
        return;

    // check for swapped NVM clock/data lines
    if ( cfg_typ == 7 )
        DeviceExtension->DeviceFlags |= DFLAGS_SWAP_NVM_LINES;

    // check adapter features common to all supported configurations
    // check for narrow only mode
    if ( feat & 1 )
        DeviceExtension->DeviceFlags |= DFLAGS_FORCE_NARROW;
    // check for forcing of half speed
    if ( feat & 2 )
        DeviceExtension->DeviceFlags |= DFLAGS_HALF_SPEED;
    // check for flag to not use NVM (non-LSI)
    if ( feat & 4 )
        DeviceExtension->DeviceFlags |= DFLAGS_NO_NVM_ACCESS;

    return;
} // CheckSubsysID


VOID
ComputeSCSIScriptVectors(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine computes 53C1010 script physical addresses, and fills in
    device extension fields used by scripts.  It also patches labels and
    variables into the scripts, and moves the scripts to scripts RAM if
    available.  Note that all patching is done in the SCRIPTS array, then
    they are moved to the aligned scripts location or scripts RAM.

Arguments:

    PHW_DEVICE_EXTENSION DeviceExtension

Return Value:

    None.

--*/

{
    ULONG SegmentLength;   // receives length of physical memory segment
    PHW_NONCACHED_EXTENSION NonCachedExtPtr =
                                        DeviceExtension->NonCachedExtension;
    PULONG ScriptArrayPtr, srcPtr, dstPtr;
    ULONG svdtPhys, svdtIovOffset, ScriptRegPtr;
    ULONG ScriptPhys, SvarsPhys, dxpPhys, SRamPhys;
    ULONG NegotBufDescPhys, NegotMsgPhys;
    ULONG_PTR SRamVirt, ScriptVirt;
    USHORT i;

    // Non-cached extension is always allocated on a 4 byte boundary, so
    // alignment is not necessary.  The svars and ITQnexus tables are located
    // in Scripts RAM and are not part of the non-cached extension.

    ScriptArrayPtr = (PULONG)DeviceExtension->NonCachedExtension->ScsiScripts;
    ScriptPhys = StorPortConvertPhysicalAddressToUlong( 
            StorPortGetPhysicalAddress(DeviceExtension, NULL,
                        (PVOID)DeviceExtension->NonCachedExtension,
                        &SegmentLength));
    
    // move scripts to NonCachedExtension for patching
    StorPortMoveMemory( ScriptArrayPtr, SCRIPT, sizeof(SCRIPT) );

    // set NegotMsgBufDesc array & MegotMsg array phys pointers
    NegotBufDescPhys = ScriptPhys + sizeof(SCRIPT);
    NegotMsgPhys = NegotBufDescPhys + sizeof(NonCachedExtPtr->NegotMsgBufDesc);

    // get non cached extension pointers into device extension
    DeviceExtension->dxp = NonCachedExtPtr->dataXferParms;
    DeviceExtension->ioStartQueue = NonCachedExtPtr->startQueue;
    DeviceExtension->caStartQueue = NonCachedExtPtr->caQueue;
    DeviceExtension->ioDoneQueue = NonCachedExtPtr->doneQueue;
    DeviceExtension->ITLnexusTable = NonCachedExtPtr->ITLnexusPtrs;

    // get physical pointers for use by scripts.
    DeviceExtension->ioStartQPhys = ScriptPhys +
                FIELD_OFFSET(HW_NONCACHED_EXTENSION, startQueue);
    DeviceExtension->caStartQPhys = ScriptPhys +
                FIELD_OFFSET(HW_NONCACHED_EXTENSION, caQueue);
    DeviceExtension->ioDoneQPhys = ScriptPhys +
                FIELD_OFFSET(HW_NONCACHED_EXTENSION, doneQueue);
    DeviceExtension->ITLnexusTablePhys = ScriptPhys +
                FIELD_OFFSET(HW_NONCACHED_EXTENSION, ITLnexusPtrs);

    // align ITLnexusTable on 8 byte boundary (scripts efficiency)
    if (DeviceExtension->ITLnexusTablePhys & 0x00000007)
    {
        DeviceExtension->ITLnexusTablePhys += 4;
        DeviceExtension->ITLnexusTable =
                (PNEXUS_PTR)((ULONG_PTR)(DeviceExtension->ITLnexusTable) + 4);
    }

    // initialize the negotiation buffer table descriptor physical addresses
    for (i = 0; i < SYM_MAX_TARGETS; i++)
    {
        NonCachedExtPtr->NegotMsgBufDesc[i].paddr =
                NegotMsgPhys + (i * sizeof(NEGOT_BUF));
    }

    // set virtual/physical pointers to top of Scripts RAM
    SRamPhys = DeviceExtension->ScriptRamPhys;
    SRamVirt = DeviceExtension->ScriptRamVirt;
    // put ITQnexusTable at top of Scripts RAM
    // (ITQ and ITL tables are the same size)
    DeviceExtension->ITQnexusTable = (PNEXUS_PTR)SRamVirt;
    DeviceExtension->ITQnexusTablePhys = SRamPhys;
    SRamVirt += sizeof(NonCachedExtPtr->ITLnexusPtrs);
    SRamPhys += sizeof(NonCachedExtPtr->ITLnexusPtrs);
    // put dxp table after ITQnexusTable (insures 256 byte alignment)
    DeviceExtension->dxpSR = (PULONG)SRamVirt;
    dxpPhys = SRamPhys;
    SRamVirt += sizeof(NonCachedExtPtr->dataXferParms);
    SRamPhys += sizeof(NonCachedExtPtr->dataXferParms);
    // put svars table after the dxp table
    DeviceExtension->svars = (PSVARS)SRamVirt;
    SvarsPhys = SRamPhys;
    SRamVirt += sizeof(SVARS);
    SRamPhys += sizeof(SVARS);
    // set area for scripts below previous elements, update saved values
    ScriptVirt = SRamVirt;
    ScriptPhys = SRamPhys;
    DeviceExtension->ScriptStartVirt = ScriptVirt;
    // calculate virtual/physical address of local (SRam) svdt
    DeviceExtension->localSvdt = 
                        (PSVARS_DESCRIPTOR_TABLE)(ScriptVirt + Ent_svdt);
    svdtPhys = ScriptPhys + Ent_svdt;
    // calculate virtual address of local (SRam) DQ_entry
    DeviceExtension->DQ_entrySR = (PULONG)(ScriptVirt + Ent_DQ_entry);
    // calculate virtual address of local (SRam) scriptStopFlag
    DeviceExtension->scriptStopFlag = (PULONG)(ScriptVirt + Ent_scriptStopFlag);

    // calcuate physical address of local iov list
    svdtIovOffset = FIELD_OFFSET(SVARS_DESCRIPTOR_TABLE, iovList);
    DeviceExtension->localIovPhys = svdtPhys + svdtIovOffset;

    // fill in template table descriptor physical addresses
    DeviceExtension->deviceDesc.paddr = 0;      // NULL for deviceDesc
    DeviceExtension->cmdBufDesc.paddr = svdtPhys + 
        FIELD_OFFSET(SVARS_DESCRIPTOR_TABLE, Cdb);
    DeviceExtension->msgOutDesc.paddr = svdtPhys + 
        FIELD_OFFSET(SVARS_DESCRIPTOR_TABLE, msgOutBuf);

    // the following code computes physical addresses of script entry points.

    DeviceExtension->CommandScriptPhys = ScriptPhys + Ent_CommandScriptStart;
    DeviceExtension->RestartScriptPhys = ScriptPhys + Ent_RestartScript;
    DeviceExtension->RejectScriptPhys  = ScriptPhys + Ent_RejectMessage;
    DeviceExtension->SendIDEScriptPhys = ScriptPhys + Ent_SendErrorMessage;
    DeviceExtension->ContNegScriptPhys = ScriptPhys + Ent_ContNegScript;
    DeviceExtension->Add2CaQScriptPhys = ScriptPhys + Ent_Add2CaQueue;
    DeviceExtension->PhaseMisJump1Phys = ScriptPhys + Ent_PhaseMisJump1;
    DeviceExtension->PhaseMisJump2Phys = ScriptPhys + Ent_PhaseMisJump2;
    DeviceExtension->PhaseMisJump64Phys = ScriptPhys + Ent_PhaseMisJump64;
    DeviceExtension->DQ_entryPhys      = ScriptPhys + Ent_DQ_entry;
    DeviceExtension->DataOutJump1Phys  = ScriptPhys + Ent_DataOutJump1;

    // Use memory mapped base address
    ScriptRegPtr = DeviceExtension->SIOPRegisterBasePhys;

    // patches for scripts in system memory

    for (i = 0; i < sizeof(LABELPATCHES) / 4; i++)
    {
        ScriptArrayPtr[(int)LABELPATCHES[(int)i]] = ScriptPhys +
                    SCRIPT[(int)LABELPATCHES[(int)i]];
    }

    for ( i = 0; i < sizeof(R_vars_Used) /4; i++ )
    {
        ScriptArrayPtr[(int)R_vars_Used[(int)i]] = SvarsPhys + 
                    SCRIPT[(int)R_vars_Used[(int)i]];
    }

    for ( i = 0; i < sizeof(R_base_Used) /4; i++ )
    {
        ScriptArrayPtr[(int)R_base_Used[i]] = ScriptRegPtr +
                    SCRIPT[(int)R_base_Used[(int)i]];
    }

    for ( i = 0; i < sizeof(R_dataXferParmTable_Used) /4; i++ )
    {
        ScriptArrayPtr[(int)R_dataXferParmTable_Used[i]] = dxpPhys +
                    SCRIPT[(int)R_dataXferParmTable_Used[i]];
    }

    ScriptArrayPtr[(int)(Ent_burstRS_PA / 4)] = MEMORY_MOVE_CMD +
                    svdtIovOffset + 36;

    // if in NTLDR context, patch jump instruction to force all I/O's to
    // interrupt at completion
    if (DeviceExtension->ntldr_flag)
        ScriptArrayPtr[(int)(Ent_JumpPatch / 4)] = JUMP_REL_SCRIPT;

    // if using 64-bit addresses, patch nop to set bit 1 in CCNTL1
    if ( DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS )
        ScriptArrayPtr[(int)(Ent_NopPatch / 4)] = ENABLE_64BITS;

    // For 1010-66 patch 2 jump instructions to jump to the previous instruction
    // (this previous instruction is the SCNTL4 settings instruction)
    if ( (DeviceExtension->hbaCapability & HBA_CAPABILITY_1010_66) &&
         (DeviceExtension->chip_rev == 0) )
    {
        ScriptArrayPtr[(int)(Ent_DataOutJump1 / 4) + 1] -= SCRIPT_INS_SIZE;
        ScriptArrayPtr[(int)(Ent_DataOutJump2 / 4) + 1] -= SCRIPT_INS_SIZE;
    }

    // a for loop is used here due to errata on the device.
    // Systems with "CPU to PCI Posted Write" enabled will not
    // work with a StorPortWriteRegisterBuffer command
    srcPtr = ScriptArrayPtr;
    dstPtr = (PULONG)ScriptVirt;
    for (i = 0; i < (sizeof(SCRIPT)/4); i++)
    {
        StorPortWriteRegisterUlong( DeviceExtension, dstPtr++, *(srcPtr++));
    }

} // ComputeSCSIScriptVectors


ULONG
DriverEntry(
    _In_ PVOID DriverObject,
    _In_ PVOID Argument2
    )

/*++

Routine Description:

    Initial entry point for LSI_U3 miniport driver.

Arguments:

    Driver Object

Return Value:

    Status indicating whether adapter(s) were found and initialized.

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG i, Status;

    DebugPrint((1, "\nLSI Ultra160 SCSI Miniport Driver.\n\n"));

    // Initialize the hardware initialization data structure.
    for ( i = 0; i < sizeof( HW_INITIALIZATION_DATA); i++)
    {
        ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    // Set size of hardware initialization structure.
    hwInitializationData.HwInitializationDataSize =
                                            sizeof(HW_INITIALIZATION_DATA);

    // Identify required miniport entry point routines.
    hwInitializationData.HwInitialize = LsiU3HWInitialize;
    hwInitializationData.HwStartIo = LsiU3StartIo;
    hwInitializationData.HwInterrupt = LsiU3ISR;
    hwInitializationData.HwFindAdapter = LsiU3FindAdapter;
    hwInitializationData.HwResetBus = LsiU3Reset;
    hwInitializationData.HwAdapterControl = LsiU3AdapterControl;
    hwInitializationData.HwBuildIo = LsiU3BuildIo;

    // Specifiy adapter specific information.
    hwInitializationData.AutoRequestSense = TRUE;
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.NumberOfAccessRanges = 3;
    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;
    hwInitializationData.TaggedQueuing = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;

    // Set required extension sizes.
    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SrbExtensionSize = sizeof(SRB_EXTENSION);

    // call StorPort to register our HW init data
    Status =  StorPortInitialize( DriverObject,
                                  Argument2,
                                  &hwInitializationData,
                                  NULL);

    return(Status);

} // end DriverEntry()


VOID
InitializeSIOP(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This function initializes the LSI SCSI adapter chip.  It sets up all
    registers, clocks, and chip features.

Arguments:

    DeviceExtension - Pointer to the specific device extension for this SCSI
        bus.

Return Value:

    NONE

--*/

{
    UCHAR HbaId = 0;
    UCHAR scntl3Value;
    UCHAR sst0, sst1, dst;
    UCHAR sig_type = STEST4_SNGL_ENDED;
    USHORT i, termState;
    ULONG jumpAddr;
    PULONG srcPtr, dstPtr, selAddr;

    DebugPrint((3, "LsiU3(%2x):  Reinitializing chip now... \n",
        DeviceExtension->SIOPRegisterBase ));

    WRITE_SIOP_UCHAR( ISTAT0, 0x40 );

    for ( i = 0; i < MAX_CLEAR_FIFO_LOOP; i++ ) {

        StorPortStallExecution( POST_RESET_STALL_TIME );
        WRITE_SIOP_UCHAR( ISTAT0, 0 );
        if ( !(READ_SIOP_UCHAR( ISTAT0 ) & 0x40) )
            break; 
    }

    // Insure that Flush DMA FIFO, Clear DMA FIFO, and Fetch Pin Mode
    // bits are all cleared.
    WRITE_SIOP_UCHAR( CTEST3, 0x00);

    // Set DMA control values:
    WRITE_SIOP_UCHAR (DCNTL, 0x81);
    WRITE_SIOP_UCHAR (DMODE, 0x02);

    // Set chip modes according to chip revisions and registry settings
    SetChipModes(DeviceExtension);

    // set SCSI Control Register 0 bits
    WRITE_SIOP_UCHAR( SCNTL0, SCNTL0_ARB_MODE_1 + SCNTL0_ARB_MODE_0 +
                              SCNTL0_ENA_PARITY_CHK + SCNTL0_ASSERT_ATN_PAR );

    // set SCSI clock frequency
    scntl3Value = set_1010_clock (DeviceExtension);

    // initialize data transfer parameter table in table indirect format
    // (dxp) points to table in system memory
    for (i = 0; i < SYM_MAX_TARGETS; i++)
    {
        // ID in byte lane 2
        DeviceExtension->dxp[(int)i]  = (ULONG)(i << 16);
        // SCNTL3 value in byte lane 3
        DeviceExtension->dxp[(int)i] |= ((ULONG)scntl3Value << 24);
    }
    // copy initizlied table to dxp table in Scripts RAM
    srcPtr = DeviceExtension->dxp;
    dstPtr = DeviceExtension->dxpSR;
    for (i = 0; i < 16; i++)
        StorPortWriteRegisterUlong( DeviceExtension, dstPtr++, *(srcPtr++));
    
    // Enable data phase mismatch processing internal to 1010
    WRITE_SIOP_UCHAR( CCNTL0, CCNTL0_DISABLE_PIPE_REQ + CCNTL0_ENA_PM_JUMP);
    // Disable 64-bit subordinate cycles
    WRITE_SIOP_UCHAR( CCNTL1, 0x10);
    // Set phase mismatch jump registers
    WRITE_SIOP_ULONG( PMJAD1, DeviceExtension->PhaseMisJump1Phys);
    // select correct WSR jump routine based on 32 or 64 bit
    if ( DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS )
        jumpAddr = DeviceExtension->PhaseMisJump64Phys;
    else
        jumpAddr = DeviceExtension->PhaseMisJump2Phys;
    WRITE_SIOP_ULONG( PMJAD2, jumpAddr);

    // Clear all 64-bit Scripts selectors (in case EFI BIOS set them)
    selAddr = &(DeviceExtension->SIOPRegisterBase)->MMRS;
    for ( i = 0; i < 6; i++ )
        StorPortWriteRegisterUlong( DeviceExtension, selAddr++, 0);
    
    // clear the scriptsStopFlag
    StorPortWriteRegisterUlong( DeviceExtension,
                                DeviceExtension->scriptStopFlag, 0);

    // set SCSI bus SCSI ID - enable response to reselections
    WRITE_SIOP_UCHAR( SCID, (UCHAR)(DeviceExtension->SIOPBusID + 0x40 ));

    // set reselection ID
    if ( DeviceExtension->SIOPBusID < 0x08 ) {
        WRITE_SIOP_UCHAR( RESPID0, (UCHAR) (1 << DeviceExtension->SIOPBusID ));
    } else {
        HbaId = DeviceExtension->SIOPBusID - 8;
        WRITE_SIOP_UCHAR( RESPID1, (UCHAR) (1 << HbaId ));
    }

    // on 1010-66, disable AIP (on by default)
    if ( DeviceExtension->hbaCapability & HBA_CAPABILITY_1010_66 )
        WRITE_SIOP_UCHAR( AIPCNTL1, 0x08);

    // Clear all interrupt status registers.  Diffsense interrupt (and others)
    for ( i = 0; i < 100; i++ )
    {
        sst0 = READ_SIOP_UCHAR(SIST0);
        sst1 = READ_SIOP_UCHAR(SIST1);
        dst = READ_SIOP_UCHAR(DSTAT) & 0x7F;    // mask off DMA FIFO full
        if ( !(sst0 || sst1 || dst) )
            break;
    }

    // Enable appropriate SCSI interrupts
    WRITE_SIOP_UCHAR( SIEN0, SIEN0_PHASE_MISMATCH
                                + SIEN0_SCSI_GROSS_ERROR
                                + SIEN0_UNEXPECTED_DISCON
                                + SIEN0_RST_RECEIVED
                                + SIEN0_PARITY_ERROR
                                );

    // enable appropriate DMA interrupts
    WRITE_SIOP_UCHAR( DIEN, DIEN_ENA_ABRT_INT
                                + DIEN_BUS_FAULT
                                + DIEN_ENABLE_INT_RCVD
                                + DIEN_ENABLE_ILL_INST
                                );

    // Enable additional SCSI interrupts
    //    Selection or relection time-out & LDVS difsense interrupts
    WRITE_SIOP_UCHAR( SIEN1, SIEN1_SEL_RESEL_TIMEOUT
                                + SIEN1_BUS_MODE_CHANGE
                                );

    // set ScratchB register to 0 for use by scripts
    WRITE_SIOP_ULONG( SCRATCHB, 0 );

    // Set the SCSI timer values
    WRITE_SIOP_UCHAR( STIME0, 0x0c );

    // Enable TolerANT
    WRITE_SIOP_UCHAR( STEST3, 0x80 );

    // Initialize input/output settings of GPIO pins
    HwInitGpioPins( DeviceExtension);

    // see if termination should be programmed ON or OFF
    termState = DeviceExtension->TerminatorState;
    if ( termState != TS_CANT_PROGRAM )
    {
        if ( termState == TS_ENABLED )
            // turn on termination
            HwSetGpioPin( DeviceExtension, GPIO_TERMINATION, GPIO_ON);
        else if ( termState == TS_DISABLED )
            // turn off termination
            HwSetGpioPin( DeviceExtension, GPIO_TERMINATION, GPIO_OFF);
    }

    // see if in LVDS mode
    if (DeviceExtension->DeviceFlags & DFLAGS_LVDS_MODE)
        sig_type = STEST4_LOWV_DIFF;
    // put signal type into DMI structure
    DeviceExtension->DmiData.SignalType = sig_type >> 6;

    // enable internal connected signal on GPIO0 to be LED activity signal
    WRITE_SIOP_UCHAR(GPCNTL,(UCHAR)(READ_SIOP_UCHAR(GPCNTL) | GPCNTL_LED_CNTL));

} // InitializeSIOP


VOID 
initializeSvdtQueue (
    _In_ PHW_DEVICE_EXTENSION DeviceExtension)
/*++

Routine Description:

    This routine initializes the scripts queues and variables.

Arguments:

    DeviceExtension - Supplies the device Extension for the SCSI bus adapter.

Return Value:

    None.

--*/
{
    ULONG       i, linkPtr;
    PULONG      tmp_ptr;
    PSVARS      pS = DeviceExtension->svars;
    PIO_TRACK_ENTRY pEntry;

    // initialize queue ptrs in svars (located in Scripts RAM)
    StorPortWriteRegisterUlong( DeviceExtension,
                                DeviceExtension->DQ_entrySR, 0);
    tmp_ptr =
        (PULONG)(DeviceExtension->ScriptStartVirt + Ent_CommandScriptStart + 4);
    StorPortWriteRegisterUlong( DeviceExtension,
                                tmp_ptr, DeviceExtension->ioStartQPhys);
    StorPortWriteRegisterUlong( DeviceExtension, &pS->lockedQPhysPtr, 1);
    tmp_ptr = (PULONG)(DeviceExtension->ScriptStartVirt + Ent_postDoneQ + 8);
    StorPortWriteRegisterUlong( DeviceExtension,
                                tmp_ptr, DeviceExtension->ioDoneQPhys);
    StorPortWriteRegisterUlong( DeviceExtension, &pS->caPutQPhysPtr,
                                DeviceExtension->caStartQPhys);
    StorPortWriteRegisterUlong( DeviceExtension, &pS->caStartQPhysPtr,
                                DeviceExtension->caStartQPhys);
    StorPortWriteRegisterUlong( DeviceExtension, &pS->ITQnexusTablePhysPtr,
                                DeviceExtension->ITQnexusTablePhys);
    StorPortWriteRegisterUlong( DeviceExtension, &pS->ITLnexusTablePhysPtr,
                                DeviceExtension->ITLnexusTablePhys);

    // initialize start queue
    DeviceExtension->ioStartQIndex = 0;
    linkPtr = DeviceExtension->ioStartQPhys + 12;
    for (i = 0; i < (START_Q_DEPTH-1); i++)
    {
        DeviceExtension->ioStartQueue[i].svdtPhysSem = 0;
        DeviceExtension->ioStartQueue[i].linkPhys = linkPtr;
        linkPtr += 12;
    }
    // link last element in queue back to start
    DeviceExtension->ioStartQueue[START_Q_DEPTH-1].svdtPhysSem = 0;
    DeviceExtension->ioStartQueue[START_Q_DEPTH-1].linkPhys =
                                                DeviceExtension->ioStartQPhys;
    
    // initialize completion queue
    DeviceExtension->ioDoneQIndex = 0;
    linkPtr = DeviceExtension->ioDoneQPhys + 12;
    for (i = 0; i < (DONE_Q_DEPTH-1); i++)
    {
        DeviceExtension->ioDoneQueue[i].context = 0;
        DeviceExtension->ioDoneQueue[i].linkPhys = linkPtr;
        linkPtr += 12;
    }
    // link last element in queue back to start
    DeviceExtension->ioDoneQueue[DONE_Q_DEPTH-1].context = 0;
    DeviceExtension->ioDoneQueue[DONE_Q_DEPTH-1].linkPhys =
                                                DeviceExtension->ioDoneQPhys;
    
    // initialize ca queue
    linkPtr = DeviceExtension->caStartQPhys + 12;
    for (i = 0; i < (CA_Q_DEPTH-1); i++)
    {
        DeviceExtension->caStartQueue[i].svdtPhysSem = SVDT_SEM_UNLOCK;
        DeviceExtension->caStartQueue[i].linkPhys = linkPtr;
        linkPtr += 12;
    }
    // link last element in queue back to start
    DeviceExtension->caStartQueue[CA_Q_DEPTH-1].svdtPhysSem = SVDT_SEM_UNLOCK;
    DeviceExtension->caStartQueue[CA_Q_DEPTH-1].linkPhys =
                                                DeviceExtension->caStartQPhys;

    // initialize the queue tag FIFO
    DeviceExtension->QTagFree = 0;
    DeviceExtension->QTagPost = 0;
    for (i = 0; i < START_Q_DEPTH; i++)
    {
        DeviceExtension->QTagFIFO[i] = (UCHAR)i;
    }

    // initialize the I/O tracking array (set all SRB addresses to NULL)
    pEntry = DeviceExtension->IoTrackArray;
    for (i = 0; i < START_Q_DEPTH; i++)
    {
        pEntry->Srb = NULL;
        pEntry++;
    }

    // initialize I/O track array FIFO and pointers
    DeviceExtension->TrackFree = 0;
    DeviceExtension->TrackPost = 0;
    for (i = 0; i < START_Q_DEPTH; i++)
    {
        DeviceExtension->IoTrackFIFO[i] = (UCHAR)i;
    }
    
} // initializeSvdtQueue


VOID ISR_Service_Next(PHW_DEVICE_EXTENSION DeviceExtension,UCHAR ISRDisposition)
/*++

Routine Description:

    This routine handles the return value from various ISR service procedures
    and restarts the scripts processor as required.

Arguments:

    DeviceExtension - Supplies the device Extension for the SCSI bus adapter.
    ISRDisposition - Method needed for restarting the scripts

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;

    DebugPrint((3, "LsiU3: Entering ISR_Service_Next... \n"));

    if ( ISRDisposition != ISR_EXIT )
    {
        if ( ISRDisposition == ISR_START_SCRIPT )
        {
            if (!DeviceExtension->ntldr_flag)   // restart if not NTLDR context
                StartSIOP( DeviceExtension, DeviceExtension->CommandScriptPhys);
        }
        else
        {
            if ( ISRDisposition == ISR_RELOAD_SCRIPT )
            {
                WRITE_SIOP_UCHAR(SCNTL3,
                        (UCHAR)(DeviceExtension->dxp[Srb->TargetId] >> 24));
                WRITE_SIOP_UCHAR(SXFER,
                        (UCHAR)(DeviceExtension->dxp[Srb->TargetId] >> 8));
                WRITE_SIOP_UCHAR(SCNTL4,
                        (UCHAR)(DeviceExtension->dxp[Srb->TargetId]));
            }
            StartSIOP( DeviceExtension, DeviceExtension->RestartScriptPhys);
        }
    }

} //isr_service_next


SCSI_ADAPTER_CONTROL_STATUS
LsiU3AdapterControl(
    _In_ PVOID DevExt,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters
    )
/*++

Routine Description:

    This is a generic routine to allow for special adapter control routines
    to be implemented without changing the HW_INITIALIZATION_DATA structure.
    Currently, it contains the StopAdapter routine and the ScsiRestartAdapter
    routine which replaces FindAdapter and HwInitialize for adapters going
    through a power management cycle.  Since the DeviceExtension is maintained
    through the power cycle, it is not necessary to use ScsiSetRunningConfig
    to access system memory.

Arguments:

    DevExt - Pointer to the device extension for this SCSI bus.

    ControlType - Specifies the type of call being made through this routine.

    Parameters - Pointer to parameters needed for this control type (optional).

Return Value:

    SCSI_ADAPTER_CONTROL_STATUS - currently either:
        ScsiAdapterControlSuccess (= 0)
        ScsiAdapterControlUnsuccessful (= 1)
    (additional status codes can be added with new control codes)

--*/

{
    PHW_DEVICE_EXTENSION DeviceExtension = DevExt;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST pCtlTypList;

    // do a switch on the ControlType
    switch (ControlType)
    {
        // determine which control types (routines) are supported
        case ScsiQuerySupportedControlTypes:
            // get pointer to control type list
            pCtlTypList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST)Parameters;

            // Mark the types that we support. Note that we must take precautions
            // to not overrun the type list buffer.
            if (ScsiQuerySupportedControlTypes < pCtlTypList->MaxControlType) {
                pCtlTypList->SupportedTypeList[ScsiQuerySupportedControlTypes] = TRUE;
            }    
            if (ScsiStopAdapter < pCtlTypList->MaxControlType) {
                pCtlTypList->SupportedTypeList[ScsiStopAdapter] = TRUE;
            }    
            if (ScsiRestartAdapter < pCtlTypList->MaxControlType) {
                pCtlTypList->SupportedTypeList[ScsiRestartAdapter] = TRUE;
            }    
            break;


        // StopAdapter routine called just before power down of adapter
        case ScsiStopAdapter:
            // if StopAdapter flag is already set, we did out shutdown via
            // the SRB_FUNCTION_POWER request, just return
            if ( DeviceExtension->StopAdapter )
                break;
            // make sure device is still accessable (not already removed)
            if ( READ_SIOP_UCHAR(ISTAT0) != 0xFF )
            {
                // just call routine to abort scripts running since this
                // routine first turns off all interrupts.
                AbortCurrentScript(DeviceExtension);

                // get rid of any interrupts that may be pending on the adapter
                // no need to test return, adapter being powered off anyway
                EatInts(DeviceExtension);
            }
            // set StopAdapter flag
            DeviceExtension->StopAdapter = TRUE;
            break;

        // routine to reinitialize adapter while system in running.  Since
        // the adapter DeviceExtension is maintained through a power management
        // cycle, we can just restore the scripts and reinitialize the chip.
        case ScsiRestartAdapter:
            // clear StopAdapter flag
            DeviceExtension->StopAdapter = FALSE;
            // reinitialize H/W
            ComputeSCSIScriptVectors( DeviceExtension );    // setup scripts
            InitializeSIOP( DeviceExtension );

            if ( !EatInts( DeviceExtension ))     // eat any pending interrupts
            {
                // if clearing interrupts fails, try resetting the bus
                // and reinitializing the chip one time only.  StorPort is
                // notified of reset through this call.
                ResetSCSIBus( DeviceExtension);
                InitializeSIOP( DeviceExtension);
                // ignore return this time to prevent infinite loop
                EatInts( DeviceExtension);
            }

            SetupLuFlags( DeviceExtension, 1 );        // init all LU flags
            initializeSvdtQueue( DeviceExtension );    // init shared queues
            // restart the scripts
            StartSIOP( DeviceExtension, DeviceExtension->CommandScriptPhys);
            // 2 second delay to allow some drives to become ready before
            // I/O's start
            delay_mils(2000);
            break;

    } // end of switch

    //_____________________________________________________________________
    // Defect injection for: StorPortAdapterControl.slic
    return ScsiAdapterControlUnsuccessful;
    //_____________________________________________________________________

} // LsiU3AdapterControl


ULONG LsiU3FindAdapter(
    _In_ PVOID Context,
    _In_ PVOID Reserved1,
    _In_ PVOID Reserved2,
    _In_z_ PCHAR ArgumentString,
    _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PBOOLEAN Reserved3
    )
/*++

Routine Description:

    This function fills in the configuration information structure

Arguments:

    Context - Supplies a pointer to the device extension.

    Reserved1 - Unused.

    Reserved2 - Unused.

    ArgumentString - DriverParameter string.

    ConfigInfo - Pointer to the configuration information structure to be
        filled in.
    
    Reserved3 - Unused.

Return Value:

    Returns status based upon results of adapter parameter acquisition.

--*/

{
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
    PACCESS_RANGE AccessRange, SAccessRange, MM_Range, IO_Range;
    PPCI_COMMON_CONFIG pPciConf = NULL;
    PUCHAR ptr;
#ifndef FORCE_SYNC    
    ULONG force_sync;
#endif    
    ULONG pci_cfg_len, loop;
    ULONG load_context;
    UCHAR pci_cfg_buf[48], reqId, i, hbaDeviceID;
    USHORT hbaCap = 0;
    BOOLEAN foundNVM;
    BOOLEAN found_SymNVM = FALSE;
    ULONG crash_dump = 0;
    PDMI_DATA pDmi = &DeviceExtension->DmiData;

    PSTORPORT_EXTENDED_FUNCTIONS pExtFuncTable = NULL; // defect injection
    BOOLEAN validateRange; // defect injection

    UNREFERENCED_PARAMETER( Reserved1 );
    UNREFERENCED_PARAMETER( Reserved2 );
    UNREFERENCED_PARAMETER( Reserved3 );

    RtlZeroMemory(&pci_cfg_buf, 48);
	
    //_____________________________________________________________________
    // Defect injection for: StorPortNotification2.slic
    StorPortNotification(GetExtendedFunctionTable, DeviceExtension, &pExtFuncTable);
    //_____________________________________________________________________

    // see if the DeviceExtension has not been zeroed out
    if ( DeviceExtension->NonCachedExtension )
    {
        // no, zero it out
        ptr = (PUCHAR)DeviceExtension;
        for ( loop = 0; loop < sizeof(HW_DEVICE_EXTENSION); loop++ )
             *ptr++ = 0;
    }

    //_____________________________________________________________________
    // Defect injection for: StorPortDeprecated.slic
    validateRange = StorPortValidateRange(
		DeviceExtension, 
		ConfigInfo->AdapterInterfaceType,
		ConfigInfo->SystemIoBusNumber,
		(*ConfigInfo->AccessRanges)[0].RangeStart,
		(*ConfigInfo->AccessRanges)[0].RangeLength,
		(*ConfigInfo->AccessRanges)[0].RangeInMemory );
    //_____________________________________________________________________

    // get memory-mapped and port I/O access range info.
    MM_Range = IO_Range = NULL;
    for ( i = 0; i < 2; i++ )     // scan ranges 0 & 1
    {
        AccessRange = &((*(ConfigInfo->AccessRanges))[i]);
        if ( !(AccessRange->RangeLength) )       // check for NULL entry
            continue;
        if ( AccessRange->RangeInMemory )     // memory-mapped or port I/O
            MM_Range = AccessRange;
        else
            IO_Range = AccessRange;
    }

    AccessRange = MM_Range;

    //_____________________________________________________________________
    // Defect injection for: StorPortVirtualDevice.slic
    ConfigInfo->VirtualDevice = TRUE;
    //_____________________________________________________________________

    if (!AccessRange)       // if desired access range is not available
    {
        return (SP_RETURN_NOT_FOUND);
    }

    DeviceExtension->SIOPRegisterBase =
        (PSIOP_REGISTER_BASE)StorPortGetDeviceBase(DeviceExtension,
                    ConfigInfo->AdapterInterfaceType,
                    ConfigInfo->SystemIoBusNumber,
                    AccessRange->RangeStart,
                    AccessRange->RangeLength,
                    (BOOLEAN)!AccessRange->RangeInMemory);
    DeviceExtension->SIOPRegisterBasePhys = AccessRange->RangeStart.LowPart;

    // check for error in mapping adapter registers to a virtual address
    if (!DeviceExtension->SIOPRegisterBase)
    {
        DebugPrint((3,"LsiU3(%2x) LsiU3FindAdapter: StorPortGetDeviceBase (reg) Failed\n"));
        return (SP_RETURN_NOT_FOUND);
    }

    // Save off I/O Post Address for this device for NVS detection
    DeviceExtension->IoPortAddress = 0x00;
    if (IO_Range)       // save it if we have an IO range
        DeviceExtension->IoPortAddress = IO_Range->RangeStart.LowPart;



    // get PCI config space header for future use
    pci_cfg_len = StorPortGetBusData ( DeviceExtension, PCIConfiguration,
                                       ConfigInfo->SystemIoBusNumber,
                                       (ULONG)ConfigInfo->SlotNumber,
                                       (PVOID)pci_cfg_buf, (ULONG)0x30 );

    // if chip is not accessable, return not found status
    if ( READ_SIOP_UCHAR(ISTAT0) == 0xFF )
    {
        return (SP_RETURN_NOT_FOUND);
    }

    // set pointer if we got valid PCI config data
    if (pci_cfg_len == 0x30)
    {
        pPciConf = (PPCI_COMMON_CONFIG)pci_cfg_buf;
        // save chip revision from PCI config space
        DeviceExtension->chip_rev = pPciConf->RevisionID;
    }
        

    // check Subsystem ID settings
    if (pPciConf)
        CheckSubsysID( DeviceExtension, pPciConf->u.type0.SubSystemID);

    // get PCI device ID from config space, if no config info default to 1010
    hbaDeviceID = pPciConf ? (UCHAR)pPciConf->DeviceID : 0x20;

    // use device ID to set proper HBA capabilities
    switch (hbaDeviceID)
    {
        case 0x20:  // 53C1010-33
            hbaCap = HBA_CAPABILITY_WIDE + HBA_CAPABILITY_64_BITS +
                     HBA_CAPABILITY_8K_SCR_RAM + HBA_CAPABILITY_SCRIPT_RAM;
            break;

        case 0x21:  // 53C1010-66
            hbaCap = HBA_CAPABILITY_WIDE + HBA_CAPABILITY_64_BITS +
                     HBA_CAPABILITY_8K_SCR_RAM + HBA_CAPABILITY_SCRIPT_RAM +
                     HBA_CAPABILITY_1010_66;
            break;
    }

    DeviceExtension->ScriptRamPhys = 0;
    if (hbaCap & HBA_CAPABILITY_SCRIPT_RAM)
    {
        SAccessRange = &((*(ConfigInfo->AccessRanges))[2]);
        if ( SAccessRange->RangeLength )
        {
            // just map the Script Ram area here
            // save the needed address into ScriptRamPhys
            DeviceExtension->ScriptRamVirt = 
                (ULONG_PTR)StorPortGetDeviceBase(DeviceExtension,
                    ConfigInfo->AdapterInterfaceType,
                    ConfigInfo->SystemIoBusNumber,
                    SAccessRange->RangeStart,
                    SAccessRange->RangeLength,
                    (BOOLEAN)!SAccessRange->RangeInMemory);
            DeviceExtension->ScriptRamPhys = SAccessRange->RangeStart.LowPart;
        }

        // if no Scripts RAM access range or if mapping of Scripts RAM to a
        // virtual address fails, we can't run - return failure
        if (!DeviceExtension->ScriptRamPhys || !DeviceExtension->ScriptRamVirt)
        {
            DebugPrint((3,"LsiU3(%2x) LsiU3FindAdapter: StorPortGetDeviceBase (SR) Failed\n"));
            return (SP_RETURN_NOT_FOUND);
        }
    }

    // if Subsystem ID is forcing narrow, turn off wide capability
    if (DeviceExtension->DeviceFlags & DFLAGS_FORCE_NARROW)
        hbaCap &= ~HBA_CAPABILITY_WIDE;

    // set hbaCapability
    DeviceExtension->hbaCapability = hbaCap;

    if (ArgumentString != NULL)
    {
        // check if doing crash dump
        if (ProcessParseArgumentString(ArgumentString,"dump", &crash_dump))
            DeviceExtension->crash_dump = crash_dump;

#ifndef FORCE_SYNC
        // check for force sync registry entry
        if (ProcessParseArgumentString(ArgumentString,"forcesync",
                                                &force_sync))
        {
            if (force_sync)
            {
                DeviceExtension->DeviceFlags |= DFLAGS_FORCE_SYNC;
            }
        }
#endif

        // check if in NTLDR context
        if (ProcessParseArgumentString(ArgumentString,"ntldr", &load_context))
            DeviceExtension->ntldr_flag = (UCHAR)load_context;
    }

#ifdef FORCE_SYNC
    // set force sync flag to ignore disable_sync
    // (if not in crash dump context)
    if (!crash_dump)
        DeviceExtension->DeviceFlags |= DFLAGS_FORCE_SYNC;
#endif

    // make sure scripts aren't running (from a reboot)
    AbortCurrentScript(DeviceExtension);

    // initialize data & clock mask values for NVM detection
    DeviceExtension->data_mask = 0x01;
    DeviceExtension->clock_mask = 0x02;

    // look for NVM image in NVM and NVS
    foundNVM = FALSE;       // set flag to invalidate NVM
    if (NvmDetect(DeviceExtension) == SUCCESS)
    {
        found_SymNVM = TRUE;
        foundNVM = RetrieveNvmData(DeviceExtension);
        if (foundNVM)
        {
            DeviceExtension->DeviceFlags |= DFLAGS_NVM_FOUND;
            // get GPIO pin usage data from Mfg data area
            RetrieveMfgData(DeviceExtension);
        }
    }
#ifdef _X86_
    // do this only on X86 platforms
    else if (NVSDetect(DeviceExtension) == SUCCESS)
    {
        foundNVM = RetrieveNVSData(DeviceExtension);
    }
#endif
    if (!foundNVM)  // if NVM image not found, use default values
        InvalidateNvmData(DeviceExtension);

    // Set SCSI ID obtained from NVRAM (or defaulted to 7)
    DeviceExtension->SIOPBusID = (UCHAR)DeviceExtension->HostSCSIId;

    // device has clock quadrupler (40Mhz * 4 = 160Mhz)
    DeviceExtension->ClockSpeed = 160;

    //_____________________________________________________________________
    // Defect injection for: StorPortFindAdapter.slic
    // commenting out following line so value stays at SP_UNINITIALIZED_VALUE
    // ConfigInfo->MaximumTransferLength = MAX_XFER_LENGTH;
    //_____________________________________________________________________

    // if doing crash dump we only support 64K SG list to minimize
    // non-paged pool usage (needs to be < 32K)
    if (crash_dump)
        ConfigInfo->NumberOfPhysicalBreaks = 0x11;
    else
        // use compiled default (256K)
        ConfigInfo->NumberOfPhysicalBreaks = MAX_SG_ELEMENTS;

    if (hbaCap & HBA_CAPABILITY_WIDE)
        ConfigInfo->MaximumNumberOfTargets = SYM_MAX_TARGETS;
    else
        ConfigInfo->MaximumNumberOfTargets = SYM_NARROW_MAX_TARGETS;
     
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->ScatterGather = TRUE;

    //_____________________________________________________________________
    // Defect injection for: StorPortMSILock.slic
    // See LsiU3StartIo for part B of this defect.
    ConfigInfo->InterruptSynchronizationMode = InterruptSynchronizeAll;
    //_____________________________________________________________________



    // see if user asked for specific host ID
    // if not, InitiatorBusId[0] == SP_UNINITIALIED_VALUE (0xFF)
    // by checking against highest valid ID, can get both cases
    //   and validate users requested ID
    reqId = ConfigInfo->InitiatorBusId[0];
    if ( reqId > ConfigInfo->MaximumNumberOfTargets - 1 )
        // either uninitialized or outside valid range, use our default
        ConfigInfo->InitiatorBusId[0] = DeviceExtension->SIOPBusID;
    else
        // requested ID is good, store it
        DeviceExtension->SIOPBusID = reqId;

    // set ResetTargetSupported and increase number of LUNs supported to 16.
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->MaximumNumberOfLogicalUnits = 16;
    // set driver to run in full duplex mode
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;
	
    // if system and adapter support 64-bit addressing, set ConfigInfo,
    // local flag, and adjust SrbExtension size for 64-bit move commands.
    if ((ConfigInfo->Dma64BitAddresses == SCSI_DMA64_SYSTEM_SUPPORTED) &&
        (hbaCap & HBA_CAPABILITY_64_BITS))
    {
        ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_SUPPORTED;
        DeviceExtension->DeviceFlags |= DFLAGS_64BIT_ADDRESS;
        // increase size of SrbExtension to accomodate 64-bit move commands
        // and 1 extra Scripts instruction (turn off 64-bit mode)
        ConfigInfo->SrbExtensionSize +=
            (ULONG)((ConfigInfo->NumberOfPhysicalBreaks + 2) * 4);
    }

    DeviceExtension->NonCachedExtension = StorPortGetUncachedExtension(
                                            DeviceExtension,
                                            ConfigInfo,
                                            sizeof(HW_NONCACHED_EXTENSION) );

    if (DeviceExtension->NonCachedExtension == NULL)
    {
        DebugPrint((3,"LsiU3(%2x) LsiU3FindAdapter: NonCachedExt Failed\n",
            DeviceExtension->SIOPRegisterBase));
        return (SP_RETURN_ERROR);
    }

    DebugPrint((1, "LsiU3(%2x) Irq=%x  HBA Id=%x \n",
        DeviceExtension->SIOPRegisterBase,
        ConfigInfo->BusInterruptLevel,
        ConfigInfo->InitiatorBusId[0] ));

    DebugPrint((1, "LsiU3(%2x) Sys Bus#=%x  Slot#=%x \n",
        DeviceExtension->SIOPRegisterBase,
        ConfigInfo->SystemIoBusNumber,
        ConfigInfo->SlotNumber ));

    DebugPrint((3, "LsiU3(%2x) LsiU3FindAdapter: DeviceExtension at 0x%x \n",
        DeviceExtension->SIOPRegisterBase,
        DeviceExtension ));
    DebugPrint((3, "LsiU3(%2x) LsiU3FindAdapter: SCSI SCRIPTS at 0x%x \n",
        DeviceExtension->SIOPRegisterBase,
        DeviceExtension->NonCachedExtension->ScsiScripts ));

    // Save local hbaCapability
    DeviceExtension->hbaCapability = hbaCap;

    // save DMI data
    pDmi->MaxAttachments = pDmi->MaxWidth = ConfigInfo->MaximumNumberOfTargets;
    pDmi->DeviceId = hbaDeviceID;
    pDmi->MaxXferRate = DeviceExtension->ClockSpeed / 2;
    pDmi->ScsiBusId = DeviceExtension->SIOPBusID;

    // save PCI bus and slot numbers (shifted for NVConfig return)
    DeviceExtension->PciBusSlot = (ConfigInfo->SystemIoBusNumber << 24) +
                                  (ConfigInfo->SlotNumber << 16);

    // test if 1010 is connected on SCSI bus.  if so, loop waiting for
    // disconnect.  if no disconnect, reset the bus.
    for (i = 0; i < 100; i++ ) {    // loop up to 100 ms (.1 sec)
        if ( !(READ_SIOP_UCHAR(ISTAT0) & ISTAT_CON) )
            break;
        StorPortStallExecution( 999 );  // delay 1 msec
    }

    if (i == 100) { // never disconnected, reset SCSI bus
        // set the bus reset line high
        WRITE_SIOP_UCHAR(SCNTL1, SCNTL1_RESET_SCSI_BUS);
        // Delay the minimum assertion time for a SCSI bus reset
        StorPortStallExecution( RESET_STALL_TIME);
        // set the bus reset line low
        WRITE_SIOP_UCHAR(SCNTL1, 0);

        // tell StorPort bus was reset
        StorPortNotification(ResetDetected, DeviceExtension, 0);
        StorPortStallExecution( POST_RESET_STALL_TIME);
    }   

    // Initialize SIOP

    ComputeSCSIScriptVectors(DeviceExtension);
    InitializeSIOP(DeviceExtension);
    if ( !EatInts( DeviceExtension ))     // eat any pending interrupts
    {
        // if clearing interrupts fails, try resetting the bus
        // and reinitializing the chip one time only.  StorPort is
        // notified of reset through this call.
        ResetSCSIBus( DeviceExtension);
        InitializeSIOP( DeviceExtension);
        // ignore return this time to prevent infinite loop
        EatInts( DeviceExtension);
    }

    return(SP_RETURN_FOUND);

} // LsiU3FindAdapter


BOOLEAN
LsiU3HWInitialize(
    _In_ PVOID Context
    )
/*++

Routine Description:

    This function initializes the LU flags and all of the svdt and I/O
    tracking queues, then starts the scripts waiting for commands.

Arguments:

    Context - Pointer to the device extension for this SCSI bus.

Return Value:

    TRUE

--*/

{
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
	
    PERF_CONFIGURATION_DATA ConfigData;	// defect injection

    SetupLuFlags( DeviceExtension, 0 );     // Prepare the LU Flags for work.
    // Initialize svdt queues and I/O tracking.
    initializeSvdtQueue(DeviceExtension);
    
    RtlZeroMemory(&ConfigData, sizeof(ConfigData));

    // Start scripts on SIOP if not in NTLDR context
    if (!DeviceExtension->ntldr_flag)
        StartSIOP( DeviceExtension, DeviceExtension->CommandScriptPhys);

	

    //_____________________________________________________________________
    // Defect injection for: StorPortPerfOpts.slic
    // SDV Result: TIMEOUT/SPACEOUT
    StorPortInitializePerfOpts(DeviceExtension, TRUE, &ConfigData);
    //_____________________________________________________________________
	
    return(TRUE);

} // LsiU3HWInitialize


BOOLEAN
LsiU3ISR(  
    PVOID Context
    )
/*++

Routine Description:

    This is the interrupt service routine for the LSI 53C1010 SCSI chip.
    This routine checks for interrupt-on-the-fly first, then for either a
    DMA or SCSI core interrupt.  All routines return a disposition code
    indicating what action to take next.

Arguments:

    Context - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    TRUE - Indicates that an interrupt was pending on adapter.

    FALSE - Indicates the interrupt was not ours.

--*/

{
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
    UCHAR IntStatus;
    UCHAR ScsiStatus;
    UCHAR DmaStatus = 0;
    UCHAR ScsiStatus1;
    UCHAR ISRDisposition;
#ifdef _WIN64
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
    STOR_PHYSICAL_ADDRESS svdtPhys;
#endif


    //_____________________________________________________________________
    // Defect injection for: StorPortIrql.slic
    KAFFINITY mask;
    StorPortGetGroupAffinity(DeviceExtension, 0, &mask);
    //_____________________________________________________________________
	
    // check if adapter is shutdown
    if ( DeviceExtension->StopAdapter )
        return(FALSE);

    // Get interrupt status
    IntStatus = READ_SIOP_UCHAR(ISTAT0);

    // Check for IntFly first
    if ( IntStatus & ISTAT_INTF )
    {
        DebugPrint((3, "LsiU3(%2x) LsiU3ISR: interrupt Fly --\n ",
            DeviceExtension->SIOPRegisterBase ));

        // Scan completion queue, processing all completed commands.
        doneQRemove(DeviceExtension, IntStatus);
        return(TRUE);
    }

    // Next, check for phase mismatch
    if ( IntStatus & ISTAT_SCSI_INT )
    {
        ScsiStatus = READ_SIOP_UCHAR(SIST0);
        ScsiStatus1 = READ_SIOP_UCHAR(SIST1);
        if ( ScsiStatus & SSTAT0_PHASE_MISMATCH )
        {
            ISRDisposition = ProcessPhaseMismatch(DeviceExtension);
            if ( ISRDisposition == ISR_RESTART_SCRIPT )
                WRITE_SIOP_ULONG( DSP, DeviceExtension->RestartScriptPhys);
            return(TRUE);
        }
    }
    else
        ScsiStatus = ScsiStatus1 = 0;   // need to initialize, just in case

    // Here begins processing of all the other types of interrupts (not in
    // the main I/O path)

    //_____________________________________________________________________
    // Defect injection for: StorPortEnablePassive.slic    
    if (StorPortEnablePassiveInitialization(DeviceExtension, LsiU3PassiveRoutine)) 
    {
	
    }
    //_____________________________________________________________________

    //_____________________________________________________________________
    // Defect for: StorPortDeprecated.slic
    // Part A, see part B in BusResetPostProcess.
    BusResetPostProcess(DeviceExtension);
    //_____________________________________________________________________

    if (!(IntStatus & (ISTAT_SCSI_INT | ISTAT_DMA_INT)))
    {
	// Reject this interrupt. This could be the loader or diskdump polling
        // or a shared interrupt.
        DebugPrint((2, "LsiU3(%2x) LsiU3ISR: Unexpected interrupt. Device Extension = %x \n",
            DeviceExtension->SIOPRegisterBase,
            DeviceExtension ));
        return(FALSE);
    }

    // if 64-bit address being used, ensure that 64-bit mode is off
    if ( DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS )
        WRITE_SIOP_UCHAR(CCNTL1,(UCHAR)(READ_SIOP_UCHAR(CCNTL1) & 0xFE));

    // Determine the type of interrupt received.
    if ( IntStatus & ISTAT_DMA_INT)
    {
        DmaStatus = READ_SIOP_UCHAR(DSTAT);
        if (DmaStatus & DSTAT_ILLEGAL_INSTRUCTION)
        {
            ISRDisposition = ProcessIllegalInstruction( DeviceExtension );
        }
        else
        {
#ifdef _WIN64
            // context is physical svdt address, convert to virtual address
            // and pull Srb out of svdt
            svdtPhys.HighPart = 0;
            svdtPhys.LowPart = StorPortReadRegisterUlong( DeviceExtension, 
                                                DeviceExtension->DQ_entrySR);
            svdtPtr = StorPortGetVirtualAddress(DeviceExtension, svdtPhys);
            if ( svdtPtr )
                DeviceExtension->ActiveRequest = svdtPtr->Srb;
            else
                DeviceExtension->ActiveRequest = NULL;
#else
            // context is Srb address
            DeviceExtension->ActiveRequest =
                (PSCSI_REQUEST_BLOCK)StorPortReadRegisterUlong( DeviceExtension, 
                                                   DeviceExtension->DQ_entrySR);
#endif
            ISRDisposition = ProcessDMAInterrupt( DeviceExtension, DmaStatus);
        }
    }
    else if ( IntStatus & ISTAT_SCSI_INT )
    {
        // check for bus reset interrupt first (we don't have a valid context
        // for an external interrupt)
        if (ScsiStatus & SSTAT0_RESET)
        {
            ISRDisposition = ProcessBusResetReceived( DeviceExtension);
        }
        else if (ScsiStatus1 & SIST1_DIFF_SENSE_CHANGE)
        {
            ISRDisposition = ProcessDiffSenseChange( DeviceExtension);
        }
        else
        {
#ifdef _WIN64
            // context is physical svdt address, convert to virtual address
            // and pull Srb out of svdt
            svdtPhys.HighPart = 0;
            svdtPhys.LowPart = StorPortReadRegisterUlong( DeviceExtension,
                                                 DeviceExtension->DQ_entrySR);
            svdtPtr = StorPortGetVirtualAddress(DeviceExtension, svdtPhys);
            if ( svdtPtr )
                DeviceExtension->ActiveRequest = svdtPtr->Srb;
            else
                DeviceExtension->ActiveRequest = NULL;
#else
            // context is Srb address
            DeviceExtension->ActiveRequest =
                (PSCSI_REQUEST_BLOCK)StorPortReadRegisterUlong( DeviceExtension, 
                                                   DeviceExtension->DQ_entrySR);
#endif
            if (ScsiStatus1 & SIST1_SEL_RESEL_TIMEOUT)
            {
                ISRDisposition = ProcessSelectionTimeout( DeviceExtension);
            }
            else
            {
                ISRDisposition = ProcessSCSIInterrupt( DeviceExtension, ScsiStatus);
            }
        }
    }
    else
    {
        DebugPrint((2, "LsiU3(%2x) LsiU3ISR: Unexpected interrupt -- neither DIP or SIP set\n",
            DeviceExtension->SIOPRegisterBase ));
        return FALSE;
    }           
    DebugPrint((2, "LsiU3(%2x) LsiU3ISR: ISTAT0=%x, DSTAT=%x, SIST0=%x, SIST1=%x \n",
        DeviceExtension->SIOPRegisterBase,
        IntStatus, DmaStatus, ScsiStatus, ScsiStatus1));
         
    ISR_Service_Next(DeviceExtension,ISRDisposition);
    
    return(TRUE);

} // LsiU3ISR


BOOLEAN
LsiU3Reset(
    _In_ PVOID Context,
    _In_ ULONG PathId
    )
/*++

Routine Description:

    This externally called routine resets the SIOP and the SCSI bus.

Arguments:

    DeviceExtension  - Supplies a pointer to the specific device extension.

    PathId - Indicates adapter to reset.

Return Value:

    TRUE - bus successfully reset

--*/

{
    UCHAR IntStatus;
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
    PSCSI_REQUEST_BLOCK  srb;

    STOR_LOCK_HANDLE IsrLockHandle={0}; // defect injection

    UNREFERENCED_PARAMETER( PathId );

    //_____________________________________________________________________
    // Defect injection for: StorPortSpinLock.slic
    StorPortAcquireSpinLock(DeviceExtension, InterruptLock, NULL, &IsrLockHandle);
    //_____________________________________________________________________

    //
    // Check to see if an interrupt is pending on the card.
    // (only if an interrupt have not been received yet)
    //

    if ( !DeviceExtension->IRQ_received )
    {
        IntStatus = READ_SIOP_UCHAR(ISTAT0);
        if (IntStatus & (ISTAT_SCSI_INT | ISTAT_DMA_INT | ISTAT_INTF))
        {
            DebugPrint((1,"LsiU3Reset: Interrupt pending on chip. ISTAT0 %x.\n",
                IntStatus));
            // Interrupt is there. Assume that the chip is disabled,
            // but still assigned resources.
            srb = DeviceExtension->ActiveRequest;

            // Set flag to ensure that the rest are caught in startIo
            DeviceExtension->DeviceFlags |= DFLAGS_IRQ_NOT_CONNECTED;

            // Log this.
            StorPortLogError(Context,  srb, 0, 0, 0, SP_IRQ_NOT_RESPONDING,
                             (1 << 8) | IntStatus);

            // Fall through and execute rest of reset code, to ensure that
            // the scripts and chip are coherent.
        }
    }

    DebugPrint((1, "LsiU3(%2x):  O/S requested SCSI bus reset\n",
        DeviceExtension->SIOPRegisterBase));

    // pause the adapter to block any new I/Os
    if ( !DeviceExtension->crash_dump )
        StorPortPause( DeviceExtension, 60);
    // set ResetActive
    DeviceExtension->ResetActive = 1;

    StorPortSynchronizeAccess( DeviceExtension, SynchronizeReset, NULL);
    return (TRUE);

} // LsiU3Reset


BOOLEAN
SynchronizeReset(
    _In_ PVOID Context,
    _In_ PVOID dummy
    )
/*++

Routine Description:

    This is the syncrhonized routine that resets the SIOP and the SCSI bus.

Arguments:

    Context  - Supplies a pointer to the specific device extension.

    dummy - NULL pointer (not used).

Return Value:

    TRUE - bus successfully reset

--*/
{
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
    
    UNREFERENCED_PARAMETER( dummy );

    // perform the reset operations
    PreAbortScripts( DeviceExtension);
    AbortCurrentScript(DeviceExtension);
    InitializeSIOP( DeviceExtension);
    ResetSCSIBus( DeviceExtension);

    // resume I/Os
    StorPortResume( DeviceExtension);
    // clear ResetActive
    DeviceExtension->ResetActive = 0;

    return (TRUE);
}


BOOLEAN
LsiU3BuildIo(
    _In_ PVOID Context,
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine receives requests from the port driver.  This routine has no
    locks held (any number of BuildIo threads can be running concurrently).
    Build as much of the I/O structure as possible without modifying shared
    memory.

Arguments:

    Context - pointer to the device extension for the adapter.

    Srb - pointer to the request to be started.

Return Value:

    TRUE - the request was accepted.

    FALSE - the request must be submitted later.

--*/

{
    UCHAR target, lun, msg0, MessageCount, tagValue;
    USHORT iovLen = 0;
    ULONG Length, svdtPAdd, srbFlgs, svdtMove;
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
    STOR_PHYSICAL_ADDRESS PhysAddr;

   //_____________________________________________________________________
   // Defect injection for: StorPortAllocatePool2.slic
   // Part A (see part B below).
   // The driver fails the rule because it doesn't check the return value
   // of the allocation and unconditionally attempts to free the buffer.
   PVOID bufferPointer;
   StorPortAllocatePool(DeviceExtension, 4096, 'SPOO', &bufferPointer);
   Srb->Function = SRB_FUNCTION_EXECUTE_SCSI; // I added this line to bypass
   // the return statement below.
   //_____________________________________________________________________
	
    // if not a normal SCSI I/O, handle it in StartIo
    if ( Srb->Function != SRB_FUNCTION_EXECUTE_SCSI )
    {
        return(TRUE);
    }

    // do all I/O setup possible in BuildIo
    // (can't modify any shared structures or elements)

    // get local variables
    target = Srb->TargetId;
    lun = Srb->Lun;

    // align svdt on quadword boundary, if necessary
    svdtPtr = &SrbExtension->svarsDescriptorTable;
    if ( (ULONG_PTR)(svdtPtr) & 0x00000007 )
        svdtPtr = (PSVARS_DESCRIPTOR_TABLE)((ULONG_PTR)svdtPtr + 4);
    SrbExtension->svdt = svdtPtr;

    // get physical address of svdt in system memory
    PhysAddr = StorPortGetPhysicalAddress(DeviceExtension, Srb, 
                                          (PVOID)svdtPtr, &Length);
    svdtPAdd = PhysAddr.LowPart;

    // fill in svdt elements
#ifdef _WIN64
    // context is physical address of svdt, save Srb in svdt
    svdtPtr->context = svdtPAdd;
    svdtPtr->Srb = Srb;
#else
    // context is Srb address
    svdtPtr->context = (ULONG)Srb;
#endif
    svdtPtr->runningByteCount = 0;  // initialize total byte count
    svdtPtr->sysSvdtPhys = svdtPAdd;        // phys ptr to svdt is sys mem
    svdtPtr->iovPhys = DeviceExtension->localIovPhys;   // iovPhys address
    // copy CDB into svdt
    StorPortMoveMemory(svdtPtr->Cdb, Srb->Cdb, Srb->CdbLength);
    // copy table descriptor templates into svdt
    StorPortMoveMemory( &svdtPtr->deviceDescriptor,
                        &DeviceExtension->deviceDesc, 24);
    // update table descriptor entries
    svdtPtr->deviceDescriptor.count = DeviceExtension->dxp[target];
    svdtPtr->cmdBufDescriptor.count = Srb->CdbLength;   // set CDB length

    // Clear auto request sense flag
    SrbExtension->autoReqSns = 0;
    
    // If there is data to transfer set up scatter/gather.
    if ( (srbFlgs = Srb->SrbFlags) & SRB_FLAGS_UNSPECIFIED_DIRECTION)
        iovLen = ScatterGatherScriptSetup( DeviceExtension, Srb, TRUE);

    // build Scripts command to move svdt
    svdtMove = MEMORY_MOVE_CMD +
        FIELD_OFFSET(SVARS_DESCRIPTOR_TABLE, iovList) + iovLen;
    svdtPtr->svdtMoveCmd = svdtMove;        // mem move command + len

    // Set up the identify message.  If disconnect is disabled reset DSCPRV.
    msg0 = (UCHAR) SCSIMESS_IDENTIFY_WITH_DISCON + lun;
    if ( srbFlgs & SRB_FLAGS_DISABLE_DISCONNECT) 
    {
        msg0 &= ~SCSIMESS_IDENTIFY_DISC_PRIV_MASK;
    } // if
    svdtPtr->msgOutBuf[0] = msg0;

    SrbExtension->SrbExtFlags = 0;  // clear negotiation flags

    MessageCount = 1;

    DebugPrint((3, "LsiU3(%2x) LsiU3BuildIo: Building request for Id=%2x  Lun=%2x \n",
        DeviceExtension->SIOPRegisterBase,
        Srb->TargetId,
        Srb->Lun ));

    if (srbFlgs & SRB_FLAGS_QUEUE_ACTION_ENABLE)
    {
        // The queue tag message is two bytes the first is the queue action
        // and the second is the queue tag.  However, we must use a driver
        // assigned queue tag (index into start queue), so that can't be
        // determined until the StartSCSIRequest (StartIo) routine.
        svdtPtr->msgOutBuf[1] = Srb->QueueAction;
        MessageCount = 3;
        DebugPrint((3, "LsiU3(%2x) Tagged I/O request \n",
            DeviceExtension->SIOPRegisterBase));
    }
    else
    {
        // put svdt physical address into ITLnexus table
        tagValue = (target * 16) + lun;
        // put temp storage address in for copy of nexus info in Scripts
        svdtPtr->nexusEntryPhys = DeviceExtension->DQ_entryPhys;
        DeviceExtension->ITLnexusTable[tagValue].nexusPtr = svdtPAdd;
        DeviceExtension->ITLnexusTable[tagValue].svdtMoveCmd = svdtMove;
        DebugPrint((3, "LsiU3(%2x) Untagged I/O request \n",
            DeviceExtension->SIOPRegisterBase));
    }

    DebugPrint((3, " CDB = %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",
        Srb->Cdb[0], Srb->Cdb[1], Srb->Cdb[2], Srb->Cdb[3],
        Srb->Cdb[4], Srb->Cdb[5], Srb->Cdb[6], Srb->Cdb[7],
        Srb->Cdb[8], Srb->Cdb[9], Srb->Cdb[10], Srb->Cdb[11] ));

    // Check to see if negotiations are needed.  Always negotiate on
    // an OS issued Request Sense command
    if ( (DeviceExtension->LuFlags[target] & LF_NEG_NEEDED) ||
         (Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) )
    {
        MessageCount = StartNegotiations(DeviceExtension, Srb, MessageCount,
                                         TRUE);
    }
    
    // indicate message length
    svdtPtr->msgOutBufDescriptor.count = (ULONG)MessageCount;

    //_____________________________________________________________________
    // Defect injection for: StorPortAllocatePool2.slic
    // Part B (see part A above).
    StorPortFreePool(DeviceExtension, bufferPointer);
    //_____________________________________________________________________
	
    // ready for StartIo, return TRUE
    return(TRUE);

} // LsiU3BuildIo


BOOLEAN
LsiU3StartIo(
    _In_ PVOID Context,
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine receives requests from the port driver after they have been
    processed by BuildIo.  This routine is synchronized with the StartIo lock
    so shared memory (for the StartIo thread) can be modified.

Arguments:

    Context - pointer to the device extension for the adapter.

    Srb - pointer to the request to be started.

Return Value:

    TRUE - the request was accepted.

    FALSE - the request must be submitted later.

--*/

{

    UCHAR function;
    ULONG DmiDataLength;
    UCHAR *p;
    PHW_DEVICE_EXTENSION DeviceExtension = Context;
    PSRB_IO_CONTROL pSic;
    PSCSI_POWER_REQUEST_BLOCK powerSrb;

    PVOID bufferPointer; // defect injection
	
    //_____________________________________________________________________
    // Defect injection for: StorPortMSILock.slic
    // See LsiU3FindAdapter for part A of this defect.
    // This driver does not use MSI interrupts, so it's not supposed to acquire MSI locks.
    // There are no message IDs in HW_DEVICE_EXTENSION, so I'm using SIOPBusID.
    ULONG oldIrql;

    //oldIrql=KeGetCurrentIrql();
    StorPortAcquireMSISpinLock(DeviceExtension, DeviceExtension->SIOPBusID, &oldIrql);
    //_____________________________________________________________________

    // check if flag set for not receiving any interrupts
    // (this is to eliminate extremely long boot times if we're not
    //  receiving interrupts)
    if ( DeviceExtension->DeviceFlags & DFLAGS_IRQ_NOT_CONNECTED )
    {
        function = Srb->Function;
        // set return status according to function
        if ( function == SRB_FUNCTION_RESET_BUS )
            Srb->SrbStatus = SRB_STATUS_BUS_RESET;
        else if ( function == SRB_FUNCTION_IO_CONTROL )
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        else
            Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
 
		StorPortNotification( RequestComplete, DeviceExtension, Srb );

		return(TRUE);
    }


    //_____________________________________________________________________
    // Defect injection for: StorPortStartIo.slic					
    StorPortStallExecution(1000);
    //_____________________________________________________________________

    // Normal SCSI I/O path, optimized for execution speed
    if ( Srb->Function == SRB_FUNCTION_EXECUTE_SCSI )
    {
            StartSCSIRequest( Srb, DeviceExtension );
            return(TRUE);
    }

    // Exception SRB functions are handled below
    function = Srb->Function;
    if ( function == SRB_FUNCTION_RESET_BUS )
    {
        DebugPrint((2, "LsiU3(%2x) LsiU3StartIO: ResetBus received.\n",
            DeviceExtension->SIOPRegisterBase ));

        // pause the adapter to block any new I/Os
        if ( !DeviceExtension->crash_dump )
            StorPortPause( DeviceExtension, 60);
        // set ResetActive
        DeviceExtension->ResetActive = 2;

        StorPortSynchronizeAccess( DeviceExtension, SynchronizeReset, NULL);
        return(TRUE);
    }

    if ( (function == SRB_FUNCTION_ABORT_COMMAND) ||
         (function == SRB_FUNCTION_TERMINATE_IO) ||
         (function == SRB_FUNCTION_RESET_DEVICE) ||
         (function == SRB_FUNCTION_RESET_LOGICAL_UNIT) )
    {
        DebugPrint((1, "LsiU3(%2x) LsiU3StartIO: Abort, terminate, or reset device received.\n",
            DeviceExtension->SIOPRegisterBase ));
        DebugPrint((1, "     Srb function = %x\n", function ));

        // Send appropriate message via StartAbortResetRequest.  This uses
        // the scripts normal start queue routine, but will get an unexpected
        // disconnect after the message is sent.  ProcessUnexpectedDisconnect
        // will check for these funtions and complete the request back to the
        // OS.

        StartAbortResetRequest( Srb, DeviceExtension);
        return(TRUE);
    }

    //_____________________________________________________________________
    // Defect injection for: StorPortStatusPending.slic					
    Srb->SrbStatus = SRB_STATUS_PENDING;
    StorPortNotification( RequestComplete, DeviceExtension, Srb );
    //_____________________________________________________________________

    // handle SRB request to shut down adapter
    if ( function == SRB_FUNCTION_POWER )
    {
        powerSrb = (PSCSI_POWER_REQUEST_BLOCK)Srb;
        // check for a shutdown power action
        if ( powerSrb->DevicePowerState == StorPowerDeviceD3 )
        {
            DebugPrint((1, "LsiU3(%2x) LsiU3StartIO: POWER request received.\n",
                DeviceExtension->SIOPRegisterBase ));
            // make sure device is still accessable (not already removed)
            if ( READ_SIOP_UCHAR(ISTAT0) != 0xFF )
            {
                // just call routine to abort scripts running since this
                // routine first turns off all interrupts.
                AbortCurrentScript(DeviceExtension);

                // get rid of any interrupts that may be pending on the adapter
                // no need to test return, adapter being powered off anyway
                EatInts(DeviceExtension);
            }
            // set StopAdapter flag
            DeviceExtension->StopAdapter = TRUE;
            StorPortNotification( RequestComplete, DeviceExtension, Srb );
			
            return(TRUE);
        }
        // fall through to bad function
    }

    // Handle SRB_IO_CONTROL to obtain DMI info
    if ( function == SRB_FUNCTION_IO_CONTROL )
    {
        pSic = (PSRB_IO_CONTROL)(Srb->DataBuffer);
        p = pSic->Signature;
        if ( pSic->ControlCode == DMI_GET_DATA && p[0] == '4' &&
             p[1] == '.' && p[2] == '0' &&
             (p[3] == '0' || p[3] == '1') )
        {
            // copy driver version string to DmiData
            StorPortMoveMemory(DeviceExtension->DmiData.DriverVersion,
                                driver_version, (ULONG)sizeof driver_version);
            DmiDataLength = sizeof(DeviceExtension->DmiData);
            // adjust length for version 4.00 structure
            if (p[3] == '0')
                DmiDataLength--;
            if (pSic->Length < DmiDataLength)
            {
                DebugPrint((1,"IO_Control buffer too small"));
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            }
            else
            {
                // copy DmiData structure to SIC buffer
                StorPortMoveMemory(((PSRB_BUFFER)(Srb->DataBuffer))->ucDataBuffer, &DeviceExtension->DmiData, DmiDataLength);
                pSic->Length = DmiDataLength;
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                // clear PCI Hot Swap flag
                DeviceExtension->DmiData.HotSwap = FALSE;
            }
        }
        else if ( pSic->ControlCode == NVCONFIG_IOCTL && p[1] == '4' &&
                  p[2] == '0' && p[3] == '0' )
        {
            // put device ID in 2nd byte of return code
            pSic->ReturnCode = DeviceExtension->DmiData.DeviceId << 8;
            // put PCI bus and slot number in 4th and 3rd bytes
            pSic->ReturnCode |= DeviceExtension->PciBusSlot;
            // see if we have NVRAM on this device
            if (DeviceExtension->DeviceFlags & DFLAGS_NVM_FOUND)
            {
                // put NVConfig request on start queue
                StartNVConfigRequest( Srb, DeviceExtension);
                // don't do any notifications, just return true
                return(TRUE);
            }
            else
            {
                // read of NVRAM failed
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                pSic->ReturnCode |= SRB_ERROR_NO_NVM;
            }
        }
        else
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
             // injected defect set DataTransferLength to 11 and set SRB status != STATUS_SUCCESS
             Srb->DataTransferLength=11;
             StorPortNotification( RequestComplete, DeviceExtension, Srb );
             return(TRUE);
    }

    DebugPrint((1, "LsiU3(%2x) LsiU3StartIO: Unknown function code received.\n",
            DeviceExtension->SIOPRegisterBase ));
	
    //_____________________________________________________________________
    // Defect injection for: StorPortAllocatePool.slic
    StorPortAllocatePool(DeviceExtension, 1024, 'SGUB', &bufferPointer);
    //_____________________________________________________________________
	
    Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
    StorPortNotification( RequestComplete, DeviceExtension, Srb );
    return(TRUE);

} // LsiU3StartIO


UCHAR
ProcessBadDataDirection(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when the data phase of the target does not match
    the direction of the original S/G move commands.  The routine traverses
    the S/G list switching the move commands to the opposite direction, or
    in the case of a ST/DT mismatch, it just changes the move commands to
    the proper phase.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT since scripts are restarted in this routine

--*/

{
    UCHAR breaks, i, move_len, sbcl_reg;
    PULONG iovPtr, iovStart;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    ULONG srbflgs = Srb->SrbFlags;
    ULONG phase_mask, iovEntry;

    // Set pointer into iov list
    iovStart = (PULONG)&DeviceExtension->localSvdt->iovList;

    // set move command length for either 32-bit or 64-bit move commands
    move_len = (DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS) ? 3 : 2;

    // get number of move elements
    breaks = SrbExtension->PhysBreakCount;

    // read SBCL and generate current phase mask
    sbcl_reg = READ_SIOP_UCHAR(SBCL);
    phase_mask = (ULONG)((sbcl_reg & 7) << 24);

    // check current C/D and I/O lines against int command phase
    if ((StorPortReadRegisterUlong( DeviceExtension, iovStart) & 0x03000000) ==
        (phase_mask & 0x03000000))
    {
        // this is a DT vs. ST phase mismatch
        iovPtr = iovStart;

#ifdef AMD64
        iovEntry = ((StorPortReadRegisterUlong64( DeviceExtension, (volatile ULONG64 *)iovPtr) &
            0xF8FFFFFF) | phase_mask);

        // sampling call
        StorPortReadRegisterBufferUlong64 (DeviceExtension, NULL, NULL, 0);
        StorPortWriteRegisterBufferUlong64(DeviceExtension, NULL, NULL, 0);
#else
        // change phase in int command
        iovEntry = ((StorPortReadRegisterUlong( DeviceExtension, iovPtr) &
                     0xF8FFFFFF) | phase_mask);
#endif

        StorPortWriteRegisterUlong( DeviceExtension, iovPtr, iovEntry);
        iovPtr += 2;    // point to next S/G entry

        // change phase in all S/G list move instructions
        for ( i = 0; i < breaks; i++ )
        {
            iovEntry = ((StorPortReadRegisterUlong( DeviceExtension, iovPtr) &
                         0xF8FFFFFF) | phase_mask);
            StorPortWriteRegisterUlong( DeviceExtension, iovPtr, iovEntry);
            iovPtr += move_len;     // point to next S/G entry
        }

    }
    else
    {
        // data in vs. data out direction mismatch

        // Set data direction flags in Srb to proper value
        srbflgs &= ~SRB_FLAGS_UNSPECIFIED_DIRECTION;    // clear both flags
        if (sbcl_reg & SBCL_IO)    // check I/O control line
            srbflgs |= SRB_FLAGS_DATA_IN;
        else
            srbflgs |= SRB_FLAGS_DATA_OUT;
    
        // save in Srb
        Srb->SrbFlags = srbflgs;
    
        // move start point past int command
        iovStart += 2;
    
        // loop through all S/G list move instructions
        // exclusive or instruction with 0x01000000 to change direction
        iovPtr = iovStart;
        for ( i = 1; i <= breaks; i++ )
        {
            iovEntry = StorPortReadRegisterUlong( DeviceExtension, iovPtr) ^
                                                                DATA_IN_SCRIPT;
            StorPortWriteRegisterUlong( DeviceExtension, iovPtr, iovEntry);
            iovPtr += move_len;
        }
    
        // if breaks > 1, need to switch MOVE command from top to bottom or
        // bottom to top, exclusive or instruction with 0x08000000
        if (breaks > 1)
        {
            iovEntry = StorPortReadRegisterUlong( DeviceExtension, iovStart) ^
                                                                MOVE_CMD_SCRIPT;
            StorPortWriteRegisterUlong( DeviceExtension, iovStart, iovEntry);
            iovPtr = iovStart + ((breaks - 1) * move_len);
            iovEntry = StorPortReadRegisterUlong( DeviceExtension, iovPtr) ^
                                                                MOVE_CMD_SCRIPT;
            StorPortWriteRegisterUlong( DeviceExtension, iovPtr, iovEntry);
        }
    }

    // if 64-bit address being used, ensure that 64-bit mode turned back on
    if ( DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS )
        WRITE_SIOP_UCHAR(CCNTL1,(UCHAR)(READ_SIOP_UCHAR(CCNTL1) | 1));

    // restart scripts at first iov_list move instruction (bypass int)
    StartSIOP( DeviceExtension, SrbExtension->svdt->iovPhys + 8);

    return( ISR_EXIT);

} // ProcessBadDataDirection


UCHAR
ProcessBusResetReceived(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine processes SCSI bus resets detected by the SIOP.  This routine
    is also called if the driver issues a bus reset (due to the bus reset
    interrupt).

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_START_SCRIPT to restart the scripts

--*/

{
    StorPortNotification(
            ResetDetected,
            DeviceExtension,
            NULL );

    // Reset the SIOP after every bus reset
    InitializeSIOP( DeviceExtension);

    // if the bus reset was internal, we will not try to start a new
    // request, since the routine that issued the reset would have already
    // done this.
    if ( DeviceExtension->DeviceFlags & DFLAGS_BUS_RESET) {

        // clear the flag indicating internal bus reset.
        DeviceExtension->DeviceFlags &= ~DFLAGS_BUS_RESET;
    } else {

        // clear all outstanding commands, reset lu flags, reinitialize queues
        BusResetPostProcess( DeviceExtension);
    } // if

    return(ISR_START_SCRIPT);

} // ProcessBusResetReceived


UCHAR
ProcessCheckCondition(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine handles commands that return with a check condition.
    The Srb and registry flags are checked to see if auto request sense is
    allowed.  Also, if the autoReqSns flag is set, we have done an auto
    request sense.  Since we can't nest those, return this command back to
    the OS and let it do the request sense.  Otherwise, we call
    ProcessCommandComplete to complete the command to the OS.
    NOTE:  We do not perform any contingence alligance blocking.
    If auto request sense is allowed, we create the req sns iov_list and
    setup the req sns CDB.  Then, we restart the scripts at Add2CaQScriptPhys.

    NT loader workaround:  During NTLDR, scripts will return every completed
    command through here (in order to stop scripts after every command).

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    if not doing auto request sense -
        ISR_START_SCRIPT to start scripts to process next command

    if doing auto request sense -
        ISR_EXIT since this routine starts scripts at Add2CaQScriptPhys

    if doing NTLDR -
        ISR_EXIT to keep scripts stopped

--*/

{
    ULONG Length, xferLen;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
    PHW_NONCACHED_EXTENSION NonCachedExtPtr =  DeviceExtension->NonCachedExtension;
    PULONG iovPtr;
    STOR_PHYSICAL_ADDRESS PhysAddress;
    USHORT luflags;
    UCHAR ret_value, target, negot_flag;
    UCHAR ReqSnsCmd[REQ_SNS_CMD_LENGTH] = { 3, 0, 0, 0, 0, 0 };

    PSCSI_REQUEST_BLOCK invalidSrb; // defect injection
    int * Mdl = 0;// defect injection

    // check for NULL SrbExtension
    if ( !SrbExtension )
    {
        // command has already been completed, just restart the scripts
        // if not in NTLDR context
        ret_value = DeviceExtension->ntldr_flag ? ISR_EXIT : ISR_START_SCRIPT;
        return( ret_value );
    }
    svdtPtr = SrbExtension->svdt;

    // check if a device "rejected" a PPR negotiation by issuing a check
    // condition
    target = Srb->TargetId;
    negot_flag = SrbExtension->SrbExtFlags & LF_PPR_NEG_PENDING;
    if ( negot_flag == LF_PPR_NEG_PENDING )
    {
        // this command did a PPR negotiation but saw no PPR response
        // instead, the device issued a check condition
        // clear the pending negotiation flags
        SrbExtension->SrbExtFlags &= ~LF_PPR_NEG_PENDING;
        luflags = DeviceExtension->LuFlags[target];
        luflags &= ~LF_PPR_NEG_PENDING;
        // mark this as a PPR rejection
        luflags |= LF_PPR_NEG_REJECT;
        DeviceExtension->LuFlags[target] = luflags;
    }

    // get true data transfer length + SCSI status in upper byte
    xferLen = READ_SIOP_ULONG( CSBC);

    // see if we should NOT do auto request sense or if in NTLDR context
    if ( (Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) ||
         SrbExtension->autoReqSns || DeviceExtension->ntldr_flag ||
         Srb->SenseInfoBuffer == NULL )
    {
        // turn off autoReqSns flag since there is no sense data
        SrbExtension->autoReqSns = 0;
        
	//_____________________________________________________________________
	// Defect injection for: StorPortNotification.slic
	// ProcessCommandComplete (typically) will call StorPortNotification(RequestComplete),
	// at which point the Srb should no longer be accessed.
	invalidSrb = DeviceExtension->ActiveRequest;
	//_____________________________________________________________________

	// complete the command as normal
        ProcessCommandComplete(DeviceExtension, xferLen);

	//_____________________________________________________________________
	StorPortGetOriginalMdl(DeviceExtension, invalidSrb, &Mdl);
	//_____________________________________________________________________			

        // restart scripts if not in NTLDR context
        ret_value = DeviceExtension->ntldr_flag ? ISR_EXIT : ISR_START_SCRIPT;
        return( ret_value );
    }

    // save actual data transfer length now (strip off SCSI status)
    Srb->DataTransferLength = xferLen & 0x00FFFFFF;

    // get SenseInfoBuffer phys addr and build iovList
    // use actual buffer length and call StorPort to get buffer phys address

    iovPtr = (PULONG)&SrbExtension->svdt->iovList;

    *iovPtr++ = (ULONG)(MOVE_IN_SCRIPT | Srb->SenseInfoBufferLength);

    PhysAddress = StorPortGetPhysicalAddress(DeviceExtension, NULL, 
                    (PVOID)Srb->SenseInfoBuffer, &Length);
    *iovPtr++ = PhysAddress.LowPart;

    // if doing 64-bit addresses, add upper 32-bits and code to disable 64 bits
    if ( DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS )
    {
        *iovPtr++ = PhysAddress.HighPart;
        *iovPtr++ = (ULONG)DISABLE_64BITS;
        *iovPtr++ = 0;
    }

    *iovPtr++ = (ULONG)RETURN_SCRIPT;
    *iovPtr = 0;

    // reset iovPhys pointer to beginning of iovList (might have been modified
    // by data phase mismatch processing in scripts)
    svdtPtr->iovPhys = DeviceExtension->localIovPhys;

    // move request sense CDB into svdt
    StorPortMoveMemory(svdtPtr->Cdb, ReqSnsCmd, REQ_SNS_CMD_LENGTH);
    // setup request sense command table indirect entry
    svdtPtr->cmdBufDescriptor.count = REQ_SNS_CMD_LENGTH;

    // stuff sense info buffer length into req sns CDB
    svdtPtr->Cdb[4] = Srb->SenseInfoBufferLength;

    // copy NegotMsgBufDesc for this ID into svdt msgoutDesc
    svdtPtr->msgOutBufDescriptor.count = Length =
                            NonCachedExtPtr->NegotMsgBufDesc[target].count;
    svdtPtr->msgOutBufDescriptor.paddr =
                            NonCachedExtPtr->NegotMsgBufDesc[target].paddr;

    // put Identify byte (with lun or'ed in) into NegotMsg
    NonCachedExtPtr->NegotMsg[target].Buf[0] = SCSIMESS_IDENTIFY | Srb->Lun;

    // turn on auto request sense flag in SrbExtension
    // 1 = negotiations will be done, 2 = no negotiations (at async/narrow)
    SrbExtension->autoReqSns = (Length == 1) ? 2 : 1;

    // restart the scripts to add this command to auto req sense queue
    StartSIOP( DeviceExtension, DeviceExtension->Add2CaQScriptPhys);

    return (ISR_EXIT);

} // ProcessCheckCondition


VOID
ProcessCommandComplete(
    PHW_DEVICE_EXTENSION DeviceExtension,
    ULONG statXferLen
    )
/*++

Routine Description:

    This routine handles normal SCSI request completion.  Routine first checks
    SCSI status returned from device, and if some permutation of GOOD sets
    error flag in SRB.  Routine then does port notification of request
    completion.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

    statXferLen - SCSI status in MSB or'ed in with true data transfer length.
Return Value:

    None

--*/

{
    UCHAR status, trackEntry;
    ULONG xferLen, tagIndex, trackFIFO;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
    PIO_TRACK_ENTRY pEntry;

    int * Mdl = 0; // defect injection

    DebugPrint((3, "LsiU3(%2x) LsiU3Command: Completing request for Id=%2x  Lun=%2x\n",
        DeviceExtension->SIOPRegisterBase,
        Srb->TargetId,
        Srb->Lun ));

    // check for NULL SrbExtension
    if ( !SrbExtension )
    {
        DebugPrint((0, "LsiU3(%2x) LsiU3Command: NULL SrbExtension for Id=%2x  Lun=%2x\n",
            DeviceExtension->SIOPRegisterBase,
            Srb->TargetId,
            Srb->Lun ));
        // indicate no request is active.
        DeviceExtension->ActiveRequest = NULL;
        return;
    }
    svdtPtr = SrbExtension->svdt;

    // if a negotiation is pending, the target was nice enough to not
    // acknowledge our negotiation message.  Process the lack of support.
    if ( SrbExtension->SrbExtFlags &
                (LF_SYNC_NEG_PENDING + LF_WIDE_NEG_PENDING) )
        ProcessNegotNotSupported( DeviceExtension);

    // set status/length data from statXferLen.
    status = (UCHAR)((statXferLen & 0xFF000000) >> 24);
    xferLen = statXferLen & 0x00FFFFFF;
    Srb->ScsiStatus = status;

    // check for nominal good I/O status first
    if (status == SCSISTAT_GOOD && !SrbExtension->autoReqSns &&
            Srb->DataTransferLength == xferLen )
    {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;

        // if this is a standard Inquiry command, set the device
        // queue depth to 31 (most U160 devices can handle this)
        if ( (Srb->Cdb[0] == SCSIOP_INQUIRY) && !(Srb->Cdb[1] & 1) )
        {
            StorPortSetDeviceQueueDepth( DeviceExtension, Srb->PathId,
                                         Srb->TargetId, Srb->Lun, 31);
        }
    }
    else
    {
        // check for bad status.
        if ( status != SCSISTAT_GOOD &&
            status != SCSISTAT_CONDITION_MET &&
            status != SCSISTAT_INTERMEDIATE &&
            status != SCSISTAT_INTERMEDIATE_COND_MET )
        {
            // indicate error occurred.
            if ( (status == SCSISTAT_BUSY) || (status == SCSISTAT_QUEUE_FULL) )
            {
                // see if auto request sense got busy or queue full status
                if ( SrbExtension->autoReqSns )
                {
                    DebugPrint((1, "LsiU3(%2x): Auto request sense returned status: 0x%x\n",
                        DeviceExtension->SIOPRegisterBase,
                        status));
                    // return check condition status, let OS do request sense
                    Srb->SrbStatus = SRB_STATUS_ERROR;
                    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                }
                else
                    Srb->SrbStatus = SRB_STATUS_BUSY;
            }
            else
                Srb->SrbStatus = SRB_STATUS_ERROR;

            DebugPrint((1, "LsiU3(%2x): Request failed for Id=%2x  Lun=%2x \n",
                DeviceExtension->SIOPRegisterBase,
                Srb->TargetId,
                Srb->Lun));

            DebugPrint((1, "  ScsiStatus: 0x%x   SrbStatus: 0x%x\n  SrbFlags: 0x%x",
                Srb->ScsiStatus,
                Srb->SrbStatus,
                Srb->SrbFlags));
        } else {
            // Status is good... but need to check if we did auto request sense
            if ( SrbExtension->autoReqSns )
            {
                Srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_ERROR;
                Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                Srb->SenseInfoBufferLength = (UCHAR)xferLen;
            }
            else
            {
                // Check for data underrun.
                if ( Srb->DataTransferLength > xferLen )
                {
                    Srb->DataTransferLength = xferLen;
                    Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
                }
                else
                    Srb->SrbStatus = SRB_STATUS_SUCCESS;
            }
        }
    }

    // free I/O tracking array entry
    trackEntry = SrbExtension->trackEntry;
    pEntry = &DeviceExtension->IoTrackArray[trackEntry];
    if ( !(pEntry->Srb) )
    {
        DebugPrint((3, "LsiU3(%2x): ProcessCommandComplete - SRB is NULL!!\n",
            DeviceExtension->SIOPRegisterBase));
        // this may have been completed already by a LUN or target reset
        // indicate no request is active.
        DeviceExtension->ActiveRequest = NULL;
        return;
    }
    // reset Srb address
    pEntry->Srb = NULL;
    // get I/O tracking entry post index
    trackFIFO = DeviceExtension->TrackPost;
    DeviceExtension->IoTrackFIFO[trackFIFO++] = trackEntry;
    // check for index wrap
    if ( trackFIFO == START_Q_DEPTH )
        trackFIFO = 0;
    // save index
    DeviceExtension->TrackPost = trackFIFO;

    // post queue tag value back to FIFO
    if ( Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE )
    {
        tagIndex = DeviceExtension->QTagPost;
        DeviceExtension->QTagFIFO[tagIndex++] = pEntry->queueTag;
        // check for index wrap
        if ( tagIndex == START_Q_DEPTH )
            tagIndex = 0;
        // save index
        DeviceExtension->QTagPost = tagIndex;
    }

    // indicate no request is active.
    DeviceExtension->ActiveRequest = NULL;

    // indicate request completed to port driver.
    StorPortNotification(
            RequestComplete,
            DeviceExtension,
            Srb );

    //_____________________________________________________________________
    // Defect injection for: StorPortNotification.slic
    StorPortGetOriginalMdl(DeviceExtension, Srb, &Mdl);
    //_____________________________________________________________________			

} // ProcessCommandComplete


UCHAR
ProcessDeviceResetFailed(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when scripts try to reset a wayward device, but
    cannot.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT - no restart of scripts needed (BusResetPostProcess will restart)

--*/

{
    DebugPrint((1, "LsiU3(%2x) Bus Device Reset or Abort failed \n",
        DeviceExtension->SIOPRegisterBase));

    // set ResetActive
    DeviceExtension->ResetActive = 3;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT );

} // ProcessDeviceResetFailed


UCHAR
ProcessDeviceResetOccurred(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when scripts sucessfully sent a Bus Device
        Reset message to a device.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_START_SCRIPT to restart the scripts

--*/

{
    UCHAR function, lun, target;
    ULONG index, trackFIFO, tagIndex;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PIO_TRACK_ENTRY pEntry;

    target = Srb->TargetId;
    function = Srb->Function;

    if ( function == SRB_FUNCTION_RESET_LOGICAL_UNIT )
    {
        lun = Srb->Lun;
        DebugPrint((1, "LsiU3(%2x) LUN Reset occurred on ID %d, LUN %d\n",
            DeviceExtension->SIOPRegisterBase, target, lun));
    }
    else
    {
        DebugPrint((1, "LsiU3(%2x) Bus Device Reset occurred on ID %d\n",
            DeviceExtension->SIOPRegisterBase, target));
    }

    // scan I/O tracking array to complete all outstanding I/Os to this
    // target or LUN back to the port driver
    pEntry = DeviceExtension->IoTrackArray;
    for ( index = 0; index < START_Q_DEPTH; index++ )
    {
        Srb = pEntry->Srb;
        if ( Srb )
        {
            if ( ((function == SRB_FUNCTION_RESET_LOGICAL_UNIT) &&
                  (pEntry->target == target) && (pEntry->lun == lun)) ||
                 (pEntry->target == target) )
            {
                pEntry->Srb = NULL;
                // get I/O tracking entry post index
                trackFIFO = DeviceExtension->TrackPost;
                DeviceExtension->IoTrackFIFO[trackFIFO++] = (UCHAR)index;
                // check for index wrap
                if ( trackFIFO == START_Q_DEPTH )
                    trackFIFO = 0;
                // save index
                DeviceExtension->TrackPost = trackFIFO;

                // post queue tag value back to FIFO
                if ( Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE )
                {
                    tagIndex = DeviceExtension->QTagPost;
                    DeviceExtension->QTagFIFO[tagIndex++] = pEntry->queueTag;
                    // check for index wrap
                    if ( tagIndex == START_Q_DEPTH )
                        tagIndex = 0;
                    // save index
                    DeviceExtension->QTagPost = tagIndex;
                }
                // complete the Srb back with reset status
                Srb->SrbStatus = SRB_STATUS_BUS_RESET;
                StorPortNotification( RequestComplete, DeviceExtension, Srb);
            }
        }
        pEntry++;
    }

    // complete the reset request (if there was one)
    if ( (function == SRB_FUNCTION_RESET_LOGICAL_UNIT) ||
         (function == SRB_FUNCTION_RESET_DEVICE) )
    {
        Srb = DeviceExtension->ActiveRequest;
        // complete the lun/target reset Srb
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        StorPortNotification( RequestComplete, DeviceExtension, Srb);
    }

    // reset active request
    DeviceExtension->ActiveRequest = NULL;

    return( ISR_START_SCRIPT );
} // ProcessDeviceResetOccurred


UCHAR
ProcessDiffSenseChange(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when the 1010 chip generates an interrupt due
        to a change in the SCSI bus mode between SE and LVDS.  We keep a
        flag to indicate if we're in LVDS mode or not, and another flag
        to indicate that be dropped back from LVDS to SE.

    NOTE:  This routine assumes mode changes of LVDS to SE and SE to LVDS
           only.  No change from or to HV differential is handled.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT - SCSI bus reset will cause scripts to restart

--*/

{
    // see if we're in LVDS mode and set flag accordingly
    if ((READ_SIOP_UCHAR(STEST4) & STEST4_BUS_TYPE_MASK) == STEST4_LOWV_DIFF)
    {
        DeviceExtension->DeviceFlags |= DFLAGS_LVDS_MODE;
        DeviceExtension->DeviceFlags &= ~DFLAGS_LVDS_DROPBACK;
    }
    else
    {
        DeviceExtension->DeviceFlags &= ~DFLAGS_LVDS_MODE;
        DeviceExtension->DeviceFlags |= DFLAGS_LVDS_DROPBACK;
    }

    // set ResetActive
    DeviceExtension->ResetActive = 4;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT);
}


UCHAR
ProcessDMAInterrupt(
    PHW_DEVICE_EXTENSION DeviceExtension,
    UCHAR DmaStatus
    )
/*++

Routine Description:

    The routine processes interrupts from the DMA core of the 53C1010 SIOP.

Arguments:

    Context - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    An ISR_ return value for ISR_Service_Next to restart the scripts.

--*/

{
    ULONG i;
    UCHAR ScriptIntOpcode;

    if ( DmaStatus & DSTAT_SCRPTINT) {

        // Check for residual data in the C1010 DMA FIFO and
        // flush it as neccessary.
        if (!(DmaStatus & 0x80)) {
            WRITE_SIOP_UCHAR( CTEST3, (UCHAR)(READ_SIOP_UCHAR(CTEST3) |
                                              CTEST3_FLUSH_FIFO));
            for (i=0; i < 1000; i++) {
                if (READ_SIOP_UCHAR(DSTAT) & 0x80) {
                    break;
                }
                StorPortStallExecution(5);
            }

            if (i >= 1000) {
                // set ResetActive
                DeviceExtension->ResetActive = 5;
                // call routine to schedule reinitialization of the SIOP
                ScheduleReinit( DeviceExtension);
                return( ISR_EXIT);
            }
            WRITE_SIOP_UCHAR( CTEST3, (UCHAR)(READ_SIOP_UCHAR(CTEST3) & 0x01));
        }

        // read the register that contains the script interrupt opcode.
        ScriptIntOpcode = READ_SIOP_UCHAR( DSPS[0]);

        // The following DMA interrupts should only occur when we have an
        // active SRB.  To be safe, we check for one.  If there is not an
        // active SRB, the hardware has interrupted inappropriately,
        // so reset everything.

        if ( DeviceExtension->ActiveRequest == NULL ) {

            DebugPrint((1, "LsiU3(%2x) ProcessDMAInterrupt unknown request\n",
                DeviceExtension->SIOPRegisterBase));
            DebugPrint((1, "              ActiveRequest: %lx  DmaStatus: %x\n",
                DeviceExtension->ActiveRequest, DmaStatus));
            DebugPrint((1, "              DSPS[0]: %x\n",
                ScriptIntOpcode));

            // set ResetActive
            DeviceExtension->ResetActive = 6;
            // call routine to schedule reinitialization of the SIOP
            ScheduleReinit( DeviceExtension);
            return( ISR_EXIT);
        };

        DebugPrint((3, "LsiU3(%2x) ProcessDMAInterrupt ...ScriptIntOpcode=%x\n",
            DeviceExtension->SIOPRegisterBase, ScriptIntOpcode));

        switch( ScriptIntOpcode) {

            // call appropriate routine to process script interrupt.

            case SCRIPT_INT_SYNC_NEGOT_COMP:

                // process SCRIPT_INT_SYNC_NEGOT_COMP script interrupt.
                // return disposition code to ISR.
                return( ProcessSynchNegotComplete( DeviceExtension));
         
            case SCRIPT_INT_WIDE_NEGOT_COMP:

                // process SCRIPT_INT_WIDE_NEGOT_COMP script interrupt.
                // return disposition code to ISR.
                return( ProcessWideNegotComplete( DeviceExtension));

            case SCRIPT_INT_CHECK_CONDITION:

                // process SCRIPT_INT_CHECK_CONDITION script interrupt.
                // return disposition code to ISR.
                return( ProcessCheckCondition( DeviceExtension));

            case SCRIPT_INT_PPR_NEGOT_COMP:

                // process SCRIPT_INT_PPR_NEGOT_COMP script interrupt.
                // return disposition code to ISR.
                return( ProcessPprNegotComplete( DeviceExtension));

            case SCRIPT_INT_REJECT_MSG_RECEIVED:

                // process SCRIPT_INT_REJECT_MSG_RECEIVED script interrupt.
                // return disposition code to ISR.
                return( ProcessRejectReceived( DeviceExtension));

            case SCRIPT_INT_SAVE_DATA_PTRS:

                // most of the time, a SAVE DATA POINTERS is followed
                // by a DISCONNECT message.  The scripts already handle
                // this, saving us a second interrupt for the disconnect.
                return( ProcessSaveDataPointers( DeviceExtension));

            case SCRIPT_INT_DEV_RESET_OCCURRED:

                // process SCRIPT_INT_DEV_RESET_OCCURRED script interrupt. return
                // disposition code to ISR.
                return( ProcessDeviceResetOccurred( DeviceExtension));

            case SCRIPT_INT_DEV_RESET_FAILED:
            case SCRIPT_INT_ABORT_FAILED:

                // process SCRIPT_INT_DEV_RESET_FAILED script interrupt. return
                // disposition code to ISR.
                return( ProcessDeviceResetFailed( DeviceExtension));

            case SCRIPT_INT_IDE_MSG_SENT:

                // process SCRIPT_INT_IDE_MSG_SENT script interrupt. return
                // disposition code to ISR.
                return( ProcessErrorMsgSent( ));

            case SCRIPT_INT_INVALID_RESELECT:
            case SCRIPT_INT_INVALID_TAG_MESSAGE:

                // process SCRIPT_INT_INVALID_RESELECT script interrupt.
                // return disposition code to ISR.
                return( ProcessInvalidReselect( DeviceExtension) );

            case SCRIPT_INT_BAD_XFER_DIRECTION:

                // S/G move commands are wrong direction.  Switch commands
                // to opposite direction and restart scripts at S/G moves.
                return( ProcessBadDataDirection( DeviceExtension) );

            case SCRIPT_INT_IGNORE_WIDE_RESIDUE:

                // process SCRIPT_INT_IGNORE_WIDE_RESIDUE script interrupt.
                // return disposition code to ISR.
                return( ProcessIgnoreWideResidue( DeviceExtension) );

            case SCRIPT_INT_NVCONFIG_IOCTL:

                // process request to read or write NVRAM.
                return( ProcessNVConfigIoctl( DeviceExtension) );

            case SCRIPT_INT_CA_QUEUE_FULL:

                // The CA queue is full and no more req sns commands can
                // be issued.  We're hosed, so we need to reset the bus.

                DebugPrint((1, "LsiU3(%2x): CA Queue Full... Reset Bus.\n",
                            DeviceExtension->SIOPRegisterBase));
                // fall through to reset SCSI bus

            default:

                // something went really wrong.
                // perform drastic error recovery.
                // set ResetActive
                DeviceExtension->ResetActive = 7;
                // call routine to schedule reinitialization of the SIOP
                ScheduleReinit( DeviceExtension);
                return( ISR_EXIT);
        } // switch ( SCRIPT_INT_OPCODE)
    } // if

    // all other cases indicate that things are really wrong, since we mask
    // off all other types of DMA interrupts.  perform drastic error recovery.
    // set ResetActive
    DeviceExtension->ResetActive = 8;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT);

} // ProcessDMAInterrupt


UCHAR
ProcessErrorMsgSent(
    VOID
    )
/*++

Routine Description:

    This routine is called when scripts sucessfully sent an IDE or MPE
    message to a device.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_RESTART_SCRIPT to restart the scripts.

--*/

{
    // Target devices should either:
    // a) return CHECK CONDITION status after receiving an IDE message, or
    //
    // b) resend the entire message in the case of an MPE message.
    //
    // Therefore, we simply restart the script state machine.
    //
    // tell ISR to restart the script
    return( ISR_RESTART_SCRIPT);
} // ProcessErrorMsgSent


UCHAR
ProcessGrossError(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine processes gross scsi errors.  See 53C1010 data manual for
    a description of gross errors.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT - no restart of scripts needed (BusResetPostProcess will restart)

--*/

{
    UCHAR cso, ct4;
    ULONG logValue, sh_st0, sh_st1;

    DebugPrint((3, "LsiU3(%2x) SCSI Gross Error occurred \n",
        DeviceExtension->SIOPRegisterBase));
    // A gross error implies the hardware or SCSI device is in an unknown
    // state.  We reset the chip and SCSI bus in hopes that the problem will
    // not recur.
    // reset SIOP
    //

    // 1010 Errata - writing SCNTL3 can cause a spurious Residual Data in SCSI
    // FIFO gross error.  If this is the SGE, just read/write the DSP to
    // restart Scripts and continue.  Otherwise, log the error.

    // read current inbound SCSI offset register
    cso = READ_SIOP_UCHAR(CSO);
    // turn on shadow register test mode bit in CTEST4
    ct4 = READ_SIOP_UCHAR(CTEST4);
    WRITE_SIOP_UCHAR( CTEST4, (UCHAR)(ct4 | CTEST4_SHADOW_REG_MODE));
    // read shadowed SIST0 and SIST1 registers
    sh_st0 = (ULONG)READ_SIOP_UCHAR(SIST0);
    sh_st1 = (ULONG)READ_SIOP_UCHAR(SIST1);
    // turn off shadow register test mode bit in CTEST4
    WRITE_SIOP_UCHAR( CTEST4, ct4);
    // check for 1010 residual data errata
    if ( (sh_st0 == (ULONG)SGEST0_RD) && !sh_st1 && !cso )
    {
        // spurious SGE interrupt, read/write DSP
        WRITE_SIOP_ULONG( DSP, READ_SIOP_ULONG(DSP));
    }
    else
    {
        // build the value to log
        // (sh_st0, sh_st1, 0x101 for SCSI gross error)
        logValue = (sh_st0 << 24) + (sh_st1 << 16) + 0x101;
        // log the error (can't rely on having an Srb)
        StorPortLogError(DeviceExtension, NULL, 0, 0, 0,
                         SP_INTERNAL_ADAPTER_ERROR, logValue);
        // set ResetActive
        DeviceExtension->ResetActive = 9;
        // call routine to schedule reinitialization of the SIOP
        ScheduleReinit( DeviceExtension);
    }

    return( ISR_EXIT);
} // ProcessGrossError


UCHAR
ProcessIgnoreWideResidue(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when scripts receive an Ignore Wide Residue message.
    It will decrement the data length by 1, due to the ignore wide residue
    message, and build a new scatter/gather list.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_RESTART_SCRIPT to restart the scripts.

--*/

{
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr = SrbExtension->svdt;
    ULONG count, dataLength;
    PVOID dataBuf;
    USHORT Flgs, dispose;

    // get device flags for this target
    Flgs = DeviceExtension->LuFlags[Srb->TargetId];

    // send message reject if not in wide mode
    if ( Flgs & LF_WIDE_NEG_FAILED || !(Flgs & LF_WIDE_NEG_DONE) )
    {
        StartSIOP( DeviceExtension, DeviceExtension->RejectScriptPhys);
        return( ISR_EXIT);
    }

    // get cummulative byte count, decrement
    count = READ_SIOP_ULONG( CSBC);
    count--;

    // store updated byte count in CSBC register and svdt
    WRITE_SIOP_ULONG( CSBC, count);
    svdtPtr->runningByteCount = count;

    // check if all data transferred.  If so, just restart scripts.
    if ( count == Srb->DataTransferLength )
        return( ISR_RESTART_SCRIPT);

    // save original Srb values, adjust for data transferred so far
    dataBuf = Srb->DataBuffer;
    dataLength = Srb->DataTransferLength;
    (ULONG_PTR)Srb->DataBuffer += count;
    Srb->DataTransferLength -= count;

    // generate new scatter/gather list for rest of transfer
    dispose = ScatterGatherScriptSetup(DeviceExtension, Srb, FALSE);

    // restore original Srb values
    Srb->DataBuffer = dataBuf;
    Srb->DataTransferLength = dataLength;

    return( ISR_RESTART_SCRIPT);
} // ProcessIgnoreWideResidue



UCHAR
ProcessIllegalInstruction(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when illegal script instruction is fetched by
    the 53C1010 or if REQ is received while executing a WAIT DISCONNECT.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT - scripts either already started or will be by BusResetPostProcess

--*/

{
    ULONG ScriptPhysAddr;

    DebugPrint((1, "LsiU3(%2x) LsiU3DMAInterrupt: Illegal script instruction \n",
        DeviceExtension->SIOPRegisterBase));
    // if a WAIT DISCONNECT has generated an ILLEGAL INSTRUCTION interrupt,
    // meaning that we have been reselected before the WAIT DISCONNECT could
    // be fetched and processed, we just restart the scripts at the next
    // instruction.

    if ( READ_SIOP_UCHAR( DCMD) == DCMD_WAIT_DISCONNECT)  {
        DebugPrint((1, "LsiU3(%2x) LsiU3DMAInterrupt: Illegal WAIT DISCONNECT \n",
            DeviceExtension->SIOPRegisterBase));

        // get the physical address of the next script instruction.
        ScriptPhysAddr = READ_SIOP_ULONG(DSP);

        // start the script instruction.
        StartSIOP( DeviceExtension, ScriptPhysAddr);
        return( ISR_EXIT);
    }

    // if we reach here, either scripts have been corrupted in memory or the
    // hardware is hosed.  since we can't do anything about the former case,
    // we will assume the latter and reset everything.

    // log the error (can't rely on having an Srb)
    StorPortLogError(DeviceExtension, NULL, 0, 0, 0,
                     SP_INTERNAL_ADAPTER_ERROR, 0x102);
    // set ResetActive
    DeviceExtension->ResetActive = 10;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT);

} // ProcessIllegalInstruction


UCHAR
ProcessInvalidReselect(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when a device reselects that did not disconnect.
    Since something is really broken at this point, we just reset everything
    we can, and hope for the best.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT - no restart of scripts needed (BusResetPostProcess will restart)

--*/

{
    DebugPrint((1, "LsiU3(%2x) LsiU3DMAInterrupt: Invalid Reselect \n",
        DeviceExtension->SIOPRegisterBase));

    // set ResetActive
    DeviceExtension->ResetActive = 11;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT);

} // ProcessInvalidReselect


UCHAR
ProcessNVConfigIoctl(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Perform either a read or write of NVRAM, as requested by the IOCTL
    structure in the Srb.  Read/write to/from the data buffer pointed
    to by the structure.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_START_SCRIPT - Restart scripts to check for next I/O to process

--*/

{
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_IO_CONTROL pSic;
    ULONG nvLength, ret_code;

    pSic = (PSRB_IO_CONTROL)(Srb->DataBuffer);
    if( pSic->Signature[0] != 'W' )
    {
        // set return code to error
        ret_code = SRB_ERROR_ON_READ;
        // read NVM data
        if ( ReadNVM( DeviceExtension,
                      ((PSRB_BUFFER)(Srb->DataBuffer))->ucDataBuffer,
                      &nvLength) )
        {
            // set return code and length
            ret_code = SRB_GOOD_ACCESS;
            pSic->Length = nvLength;
        }
    }
    else    // writing NVRAM
    {
        // set return code to errpr
        ret_code = SRB_ERROR_ON_WRITE;
        // get length of NvmData
        nvLength = pSic->Length;
        // write NVM data
        if ( WriteNVM( DeviceExtension,
                       ((PSRB_BUFFER)(Srb->DataBuffer))->ucDataBuffer,
                       nvLength) )
        {
            ret_code = SRB_GOOD_ACCESS;
        }
    }

    // set return code
    pSic->ReturnCode |= ret_code;
    // set Srb status
    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    // now do notification
    StorPortNotification( RequestComplete, DeviceExtension, Srb );

    return( ISR_START_SCRIPT);
}


UCHAR
ProcessParityError(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine processes parity errors detected on the SCSI bus by the
    host adapter.

Arguments:

     DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_EXIT - Scripts already restarted by StartSIOP.

--*/

{
    UCHAR msgChar;
    PSVARS_DESCRIPTOR_TABLE svdtPtr = DeviceExtension->localSvdt;

    // we must determine if we are in message in phase, or some other phase,
    // since we must send a different message for each case.
    DebugPrint((1, "LsiU3(%2x) ProcessParityError: Parity error detected \n",
        DeviceExtension->SIOPRegisterBase));

    // check if SCSI bus message and C/D lines are high.  if so, send MESSAGE
    // PARITY message, and if not, send INITIATOR DETECTED ERROR message.
    if ((READ_SIOP_UCHAR(SOCL) & (SBCL_MSG + SBCL_CD)) == (SBCL_MSG + SBCL_CD)) 
        msgChar = SCSIMESS_MESS_PARITY_ERROR;
    else
        msgChar = SCSIMESS_INIT_DETECTED_ERROR;
    StorPortWriteRegisterUchar( DeviceExtension, svdtPtr->msgOutBuf, msgChar);
    StorPortWriteRegisterUlong( DeviceExtension,
                                &svdtPtr->msgOutBufDescriptor.count, 1);

    StartSIOP( DeviceExtension, DeviceExtension->SendIDEScriptPhys);

    return( ISR_EXIT);

} // ProcessParityError


UCHAR
ProcessPhaseMismatch(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when a phase mismatch occurs during a non-data
    transfer.  This normally occurs during message out phases.  Data phase
    mismatches are handled totally by the 1010 Scripts.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    An ISR_ return value for ISR_Service_Next to restart the scripts.

--*/

{
    DebugPrint((3, "LsiU3(%2x) ProcessPhaseMismatch: Mismatch Occurred \n",
        DeviceExtension->SIOPRegisterBase));

    // the phase mismatch did not occur during a data phase.
    // this will happen in cases such as a phase change during an
    // extended message.  clear the FIFO's and restart the scripts.
    WRITE_SIOP_UCHAR( CTEST3, (UCHAR)(READ_SIOP_UCHAR(CTEST3) | CTEST3_CLEAR_FIFO));

    return(ISR_RESTART_SCRIPT);

}  // ProcessPhaseMismatch


char
Lowercase( char c )
{
    //
    //  If character is uppercase, lowercase it
    //
    if (c >= 'A' && c <= 'Z') {
        c -= ('a' - 'A');
    }

    return c;
}


BOOLEAN
ProcessParseArgumentString(
	//_In_ PCSTR String,
    _In_ PCHAR String,
    //_In_ PCSTR WantedString,
	_In_ PCHAR WantedString,
    _Out_ PULONG ValueFound
    )

/*++

Routine Description:

    This routine will parse the string for a match on the wanted string, then
    calculate the value for the wanted string and return it to the caller.
    
    Each character of the argument string is converted to lowercase before comparison,
    though the original string is unmodified.

Arguments:

    String - The ASCII string to parse.
    WantedString - The keyword for the value desired (case-sensitive).
    ValueFound - address where the value found is placed

Return Values:

    TRUE if WantedString found, FALSE if not
    ValueFound converted from ASCII to binary.

--*/

{
	//PCSTR cptr;
    //PCSTR kptr;
    PCHAR cptr;
	PCHAR kptr;
	ULONG stringLength = 0;
    ULONG WantedStringLength = 0;
    ULONG index;

	RtlZeroMemory(&ValueFound, sizeof(ULONG));
	
    // Calculate the string length.
    cptr = String;
	while (*cptr)
    {
        cptr++;
        stringLength++;
    }

    // Calculate the wanted strings length.
	cptr = WantedString;
    while (*cptr)
    {
        cptr++;
        WantedStringLength++;
    }

    if (WantedStringLength > stringLength)
        // Can't possibly have a match.
        return FALSE;
    
    // Now setup and start the compare.
    cptr = String;
    
ContinueSearch:

    // The input string may start with white space.  Skip it.
    while (*cptr == ' ' || *cptr == '\t')
        cptr++;
    
    if (*cptr == '\0')
        // end of string.
        return FALSE;
    
    kptr = WantedString;
    
	while (Lowercase( *cptr++ ) == *kptr++)
    {
        if (*(cptr - 1) == '\0')
            // end of string
            return FALSE;
    }

    if (*(kptr - 1) == '\0')
    {
        // May have a match backup and check for blank or equals.
        cptr--;
        while (*cptr == ' ' || *cptr == '\t')
        cptr++;

        // Found a match.  Make sure there is an equals.
        if (*cptr != '=')
        {
            // Not a match so move to the next semicolon.
            while (*cptr)
            {
                if (*cptr++ == ';')
                    goto ContinueSearch;
            }
            return FALSE;
        }

        // Skip the equals sign.
        cptr++;

        // Skip white space.
        while ((*cptr == ' ') || (*cptr == '\t'))
            cptr++;

        if (*cptr == '\0')
            // Early end of string, return not found
            return FALSE;

        if (*cptr == ';')
        {
            // This isn't it either.
            cptr++;
            goto ContinueSearch;
        }

        *ValueFound = 0;
        if ((*cptr == '0') && (Lowercase( *(cptr + 1) ) == 'x'))
        {
            // Value is in Hex.  Skip the "0x"
            cptr += 2;
            for (index = 0; *(cptr + index); index++)
            {
        if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';')
                        break;

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9'))
                    *ValueFound = (16 * (*ValueFound)) + (*(cptr + index) - '0');
                else
                {
                    if ((Lowercase( *(cptr + index) ) >= 'a') && (Lowercase( *(cptr + index) ) <= 'f'))
                        *ValueFound = (16 * (*ValueFound)) + (Lowercase( *(cptr + index) ) - 'a' + 10);
                    else
                        // Syntax error, return not found.
                        return FALSE;
                }
            }
        }
        else
        {
            // Value is in Decimal.
            for (index = 0; *(cptr + index); index++)
            {
                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';')
                        break;
                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9'))
                    *ValueFound = (10 * (*ValueFound)) + (*(cptr + index) - '0');
                else
                    // Syntax error return not found.
                    return FALSE;
            }
        }
    return TRUE;
    }
    else
    {
        // Not a match check for ';' to continue search.
        while (*cptr)
        {
            if (*cptr++ == ';')
                goto ContinueSearch;
        }
    }
    return FALSE;
}  // ProcessParseArgumentString


UCHAR
ProcessPprNegotComplete(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine handles successful PPR negotiation, as well as
    target initiated negotiation.  The routine first retrieves the
    PPR period, offset, and width from the message in buffer, then massages
    the parameters into a form the SIOP can use.  It also sets up the PPR
    message to be sent by a request sense command.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    An ISR_ return value for ISR_Service_Next to restart the scripts.

--*/

{
    UCHAR RecvdSynchPeriod, RecvdSynchOffset, RecvdWideWidth, RecvdDT_flag;
    UCHAR SynchPeriod = 0, SynchOffset = 0, i;
    UCHAR Scntl3Value, scf;
    UCHAR MaxOffset, MinPeriod;
    UCHAR negot_flag, target;
    PUCHAR msgPtr;
    USHORT luflags;
    ULONG dxp_value, script_addr;
    BOOLEAN dis_sync = FALSE;
    BOOLEAN AutoReqSns = FALSE;
    BOOLEAN reject_flag = FALSE;
    BOOLEAN tin_flag = FALSE;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PDMI_DATA pDmi = &DeviceExtension->DmiData;
    UCHAR sync_values[5] = { 0x09, 0x0A, 0x0C, 0x19, 0x32};
    UCHAR scf_values[5] = { 0x10, 0x10, 0x30, 0x50, 0x70};

    target = Srb->TargetId;
    luflags = DeviceExtension->LuFlags[target];
    dxp_value = DeviceExtension->dxp[target];
    // if doing OS issued request sense, process like auto request sense
    if (SrbExtension->autoReqSns || (Srb->Cdb[0] == SCSIOP_REQUEST_SENSE))
        AutoReqSns = TRUE;

    if (!((SrbExtension->SrbExtFlags & LF_PPR_NEG_PENDING) ==
          LF_PPR_NEG_PENDING) && !AutoReqSns )
    {
        tin_flag = TRUE;
        SrbExtension->SrbExtFlags |= LF_TIN_PPR_PENDING;
        DebugPrint((1, "LsiU3(%2x) ProcessPprNegotComplete: Target initiated PPR negotiation\n",
            DeviceExtension->SIOPRegisterBase));
    }
    else
    {
        luflags &= ~LF_PPR_NEG_PENDING;
        SrbExtension->SrbExtFlags &= ~LF_PPR_NEG_PENDING;
    }

    if ( AutoReqSns || tin_flag )
        // save dxp to check for changes
        DeviceExtension->old_dxp = dxp_value;

    if ( SrbExtension->SrbExtFlags & LF_DISABLE_SYNC )
        dis_sync = TRUE;
    
    // pick up period, offset, width, and DT flag from Script message buffer
    msgPtr = DeviceExtension->svars->msgInBuf;
    RecvdSynchPeriod = StorPortReadRegisterUchar( DeviceExtension, msgPtr);
    RecvdSynchOffset = StorPortReadRegisterUchar( DeviceExtension, &msgPtr[2]);
    RecvdWideWidth = StorPortReadRegisterUchar( DeviceExtension, &msgPtr[3]);
    RecvdDT_flag = StorPortReadRegisterUchar( DeviceExtension, &msgPtr[4]);

    DebugPrint((1, "LsiU3(%2x) PPR Negotiation Received - Id=%2x \n",
        DeviceExtension->SIOPRegisterBase,
        target
        ));
    DebugPrint((1, "     Period: %x  Offset: %x  Width: %x  DT_flag %x\n",
        RecvdSynchPeriod, RecvdSynchOffset, RecvdWideWidth, RecvdDT_flag));

    MaxOffset = MAX_1010_ST_OFFSET;

    if (RecvdSynchOffset == 0)
        MinPeriod = 0;
    else
    {
        // start with minimum period for Ultra160
        MinPeriod = 0x09;
        // can do Ultra160 only if wide
        if ( RecvdSynchPeriod == 0x09 && RecvdWideWidth != 1)
        {
            MinPeriod = 0x0A;   // reset to Ultra 2
            DebugPrint((1, "LsiU3(%2x) Can't do Ultra160 if narrow - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        // can do Ultra160 only if DT on
        if ( RecvdSynchPeriod == 0x09 && RecvdDT_flag != 2)
        {
            MinPeriod = 0x0A;
            DebugPrint((1, "LsiU3(%2x) Can't do Ultra160 in ST mode - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        // check for LVDS_DROPBACK and half speed mode
        MinPeriod = set_sync_speed( DeviceExtension, MinPeriod);
    }

    // if doing TIN, limit the requested sync period and offset to our device
    // maximums, if necessary
    if ( tin_flag )
    {
        if (RecvdSynchOffset > MaxOffset)
            RecvdSynchOffset = MaxOffset;
        if (RecvdSynchPeriod < MinPeriod)
            RecvdSynchPeriod = MinPeriod;
    }

    // WIDE negotiation part of PPR
    if (RecvdWideWidth == 0)    // target agreed only to narrow
    {
        dxp_value &= DISABLE_WIDE;
        if (dis_sync || AutoReqSns || tin_flag)
        {
            // we wanted narrow, negotiation ok
            luflags |= LF_NARROW_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) NarrowNegotiation Agreed - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        else                    // we wanted wide, negotiation failed
        {
            luflags |= LF_WIDE_NEG_DONE + LF_WIDE_NEG_FAILED;
            DebugPrint((1, "LsiU3(%2x) Wide Disabled - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
    }
    else                        // target agreed with wide
    {
        // if not doing TIN or sync not disabled, and we have wide capability
        if ( (!tin_flag || !dis_sync) && !(luflags & LF_WIDE_NEG_FAILED) )
        {
            dxp_value |= ENABLE_WIDE;
            luflags |= LF_WIDE_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) WideNegotiation Agreed - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        else        // doing TIN and sync is disabled, or no wide capability
        {
            dxp_value &= DISABLE_WIDE;
            DebugPrint((1, "LsiU3(%2x) Returned Narrow Negotiation- Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
    }

    // set DMI device width field according to new width
    RecvdWideWidth = 8;
    if (dxp_value & ENABLE_WIDE)
        RecvdWideWidth = 16;
    pDmi->DevWidth[target] = RecvdWideWidth;
 
    // SYNC negotiation part of PPR

    if ( RecvdSynchOffset == 0 )    // target agreed only to async
    {
        dxp_value &= CLEAR_OFFSET_VALUES;
        // set DMI device speed to async
        pDmi->DevSpeed[target] = SYNC_NONE;
        // need to set the sync failed flag in either case since it's used
        // to see if we check sstat1_orf on phase mismatches
        luflags |= LF_SYNC_NEG_FAILED;

        if ( dis_sync || AutoReqSns )   // we wanted async, negotiation ok
        {
            luflags |= LF_ASYNC_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) AsynNegotiation Received - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        else                        // we want sync, negotiation failed
        {
            luflags |= LF_SYNC_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) Synchronous Disabled - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
    }
    else                            // target agreed to sync
    {
        if ( dis_sync && !AutoReqSns )
        {
            // we wanted async, negotiation falied
            // need to ignore dis_sync on auto request sense
            dxp_value &= CLEAR_OFFSET_VALUES;
            // set DMI device speed to async
            pDmi->DevSpeed[target] = SYNC_NONE;

            if ( tin_flag )         // doing TIN, will send async response
            {
                DebugPrint((1, "LsiU3(%2x) Returned Async Negotiation- Target: %x\n",
                    DeviceExtension->SIOPRegisterBase, target));
            }
            else                    // not doing TIN, send message reject
            {
                luflags |= LF_ASYNC_NEG_DONE + LF_SYNC_NEG_FAILED;
                DebugPrint((1, "LsiU3(%2x) Rejecting SDTR message.  Asynch failed\n",
                    DeviceExtension->SIOPRegisterBase));
                reject_flag = TRUE;     // set flag to send reject message
            }
        }
        else                        // we got sync, check on values
        {

            // we negotiated for sync, target has responded with sync params

            luflags |= LF_SYNC_NEG_DONE;
            Scntl3Value = (UCHAR)(dxp_value >> 24) & 0x0F; 
            dxp_value &= CLEAR_OFFSET_VALUES;
            SynchPeriod = RecvdSynchPeriod;
            SynchOffset = RecvdSynchOffset;

            // set DMI sevice speed to sync value (4 * received value)
            pDmi->DevSpeed[target] = RecvdSynchPeriod << 2;

            // normalize received period to 10Mb or 5Mb periods
            // between Fast-10 and Ultra, use Fast-10
            if (SynchPeriod > 0x0C && SynchPeriod <= 0x19)
                SynchPeriod = 0x19;
            // between Fast-5 and Fast-10, use Fast-5
            if (SynchPeriod > 0x19 && SynchPeriod <= 0x32)
                SynchPeriod = 0x32;
            // slower than Fast-5, use async
            if (SynchPeriod > 0x32)
                SynchOffset = 0;

            // find SCF value in table
            for ( i = 0; i < 5; i++ )
            {
                if ( SynchPeriod == sync_values[i])
                {
                    scf = scf_values[i];
                    // if doing DT xfers below 80MT/sec, need to use the
                    // next lower scf value
                    if ((SynchPeriod > 0x09) && (RecvdDT_flag == 2) && (i < 4))
                        scf = scf_values[i+1];
                    Scntl3Value |= scf;
                    dxp_value |= RecvdSynchOffset << 8;
                    break;
                }
            }

            // turn on U3EN for DT transfers, set DT offset
            if ( RecvdDT_flag == 2 )
            {
                dxp_value |= 0x00000080;
                MaxOffset = MAX_1010_DT_OFFSET;
            }

            // put new sync period value in dxp_value
            dxp_value &= CLEAR_PERIOD_VALUES;
            dxp_value |= (ULONG)(Scntl3Value) << 24;

            // check for valid values, reject target if not OK
            if ( (RecvdSynchOffset > MaxOffset) ||
                 (RecvdSynchPeriod < MinPeriod) ||
                 (SynchOffset == 0) )
            {
                SynchPeriod = 0x19;     // default value for async
                dxp_value &= CLEAR_OFFSET_VALUES;
                // set DMI device speed to async
                pDmi->DevSpeed[target] = SYNC_NONE;

                if ( tin_flag )
                {
                    DebugPrint((1, "LsiU3(%2x) Returned async negotiation. Rate too slow\n",
                        DeviceExtension->SIOPRegisterBase));
                }
                else
                {
                    DebugPrint((1, "LsiU3(%2x) Rejecting SDTR message. Rate too slow\n",
                        DeviceExtension->SIOPRegisterBase));
                    luflags |= LF_SYNC_NEG_FAILED;
                    reject_flag = TRUE;     // set flag to send reject message
                }
            }
        }
    }

    // store updated dxp_value
    DeviceExtension->dxp[target] = dxp_value;

    // store updated luflags, if not doing auto request sense or TIN
    if (!AutoReqSns && !tin_flag)
    {
        DeviceExtension->LuFlags[target] = luflags;
    }

    if ( !tin_flag )        // if not doing TIN
    {
        // setup negotiation buffer for next auto request sense negotiation
        negot_flag = SETUP_PPR_FIRST;
    }
    else                    // doing TIN
    {
        // if negotiation is OK, turn on sync_neg_done flag if no sync
        // negotiation is currently pending or disable sync flag is not set.
        // Reset sync reject and sync failed flags.
        if (!reject_flag && !(luflags & LF_SYNC_NEG_PENDING) && !dis_sync)
        {
            DeviceExtension->LuFlags[target] |= LF_SYNC_NEG_DONE;
            DeviceExtension->LuFlags[target] &=
                            ~(LF_SYNC_NEG_REJECT + LF_SYNC_NEG_FAILED);
        }
        // save asked for period, offset, and DT flag
        DeviceExtension->tin_rec_period = SynchPeriod;
        DeviceExtension->tin_rec_offset = SynchOffset;
        DeviceExtension->tin_rec_DT = RecvdDT_flag;
        // setup msgBufOut for TIN response
        negot_flag = SETUP_PPR_TIN;
    }
    SetupNegotBuf( DeviceExtension, target, negot_flag);

    // see if we need to update device descriptors for all outstanding commands
    if (!AutoReqSns || (DeviceExtension->old_dxp != dxp_value))
        UpdateStartQDesc( DeviceExtension, Srb);

    // copy updated dxp entry to Scripts RAM dxp
    StorPortWriteRegisterUlong( DeviceExtension,
                                &DeviceExtension->dxpSR[target],
                                DeviceExtension->dxp[target]);

    // if negotiation truly failed, send reject message or if doing TIN
    // restart scripts at ContNegScript
    if ( reject_flag || tin_flag )
    {
        // reset wide/sync parameters, send a message reject
        WRITE_SIOP_UCHAR(SCNTL3, (UCHAR)(dxp_value >> 24));
        WRITE_SIOP_UCHAR(SXFER, (UCHAR)(dxp_value >> 8));
        WRITE_SIOP_UCHAR(SCNTL4, (UCHAR)dxp_value);
        script_addr = (tin_flag) ? DeviceExtension->ContNegScriptPhys :
                                    DeviceExtension->RejectScriptPhys;
        StartSIOP( DeviceExtension, script_addr);
        return( ISR_EXIT );
    }

    return( ISR_RELOAD_SCRIPT);

} // ProcessPprNegotComplete


UCHAR
ProcessRejectReceived(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is called when a device rejects a message sent in scripts.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_RESTART_SCRIPT to restart the scripts.

--*/

{
    UCHAR function;
    PSCSI_REQUEST_BLOCK Srb;
    PSRB_EXTENSION SrbExtension;

    DebugPrint((1, "LsiU3(%2x) ProcessDMAInterrupt: Message reject received \n",
        DeviceExtension->SIOPRegisterBase));

    Srb = DeviceExtension->ActiveRequest;
    SrbExtension = Srb->SrbExtension;

    // process a negotiation not supported if any negotiations are pending
    // or if doing auto request sense or if OS issued request sense
    if ( SrbExtension->SrbExtFlags &
            (LF_WIDE_NEG_PENDING + LF_SYNC_NEG_PENDING +
             LF_TIN_WIDE_PENDING + LF_TIN_SYNC_PENDING) ||
            SrbExtension->autoReqSns || (Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) )
        return( ProcessNegotNotSupported( DeviceExtension));

    function = Srb->Function;
    if ( (function == SRB_FUNCTION_RESET_LOGICAL_UNIT) ||
         (function == SRB_FUNCTION_RESET_DEVICE) )
    {
        // reset message byte was rejected, fail reset Srb
        Srb->SrbStatus = SRB_STATUS_ERROR;
        StorPortNotification( RequestComplete, DeviceExtension, Srb);
    }

    return( ISR_RESTART_SCRIPT );

} // ProcessRejectReceived


UCHAR
ProcessSaveDataPointers(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++                               

Routine Description:

    This routine is essentially a NOP, since we save the data pointers as
    part of the phase mismatch handling in scripts.  We just restart the
    scripts engine.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_RESTART_SCRIPT to restart the scripts.

--*/

{
#if DBG
    DebugPrint((3, "LsiU3(%2x) Save Data Pointers\n",DeviceExtension->SIOPRegisterBase));
#else
    UNREFERENCED_PARAMETER( DeviceExtension );
#endif

    return( ISR_RESTART_SCRIPT);

} // ProcessSaveDataPointers


UCHAR
ProcessSCSIInterrupt(
    PHW_DEVICE_EXTENSION DeviceExtension,
    UCHAR ScsiStatus
    )
/*++

Routine Description:

    This routine processes interrupts from the SCSI core of the 53C1010 SIOP.

Arguments:

    Context - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    TRUE

--*/

{
    // The following SCSI interrupts should only occur when we have an
    // active SRB.  To be safe, we check for one.  If there is not an
    // active SRB, the hardware has interrupted inappropriately,
    // so reset everything.

    if (DeviceExtension->ActiveRequest == NULL)
    {
        DebugPrint((1, "LsiU3(%2x) ProcessSCSIInterrupt unknown request\n",
            DeviceExtension->SIOPRegisterBase));
        DebugPrint((1, "              ActiveRequest: %lx  ScsiStatus: %x\n",
            DeviceExtension->ActiveRequest, ScsiStatus));

        // set ResetActive
        DeviceExtension->ResetActive = 12;
        // call routine to schedule reinitialization of the SIOP
        ScheduleReinit( DeviceExtension);
        return( ISR_EXIT);
    }

    // if a SCSI gross error occurred call routine to process, and return
    // disposition code to ISR.
    if ( ScsiStatus & SSTAT0_GROSS_ERROR)
        return( ProcessGrossError( DeviceExtension));

    // if an unexpected disconnect occurred call routine to process, and
    // return disposition code to ISR.
    if ( ScsiStatus & SSTAT0_UNEXPECTED_DISCONNECT)
        return( ProcessUnexpectedDisconnect( DeviceExtension));

    // if a parity error was detected call routine to process, and return
    // disposition code to ISR.
    if ( ScsiStatus & SSTAT0_PARITY_ERROR)
        return( ProcessParityError( DeviceExtension));

    // if none of the above, the hardware is in an unknown state.  Perform
    // drastic error recovery.
    // set ResetActive
    DeviceExtension->ResetActive = 13;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT);

} // ProcessSCSIInterrupt


UCHAR
ProcessSelectionTimeout(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine processes selection timeouts.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_RESTART_SCRIPT to restart the scripts.

--*/

{
    UCHAR target, trackEntry;
    USHORT lflags;
    ULONG tagIndex, trackFIFO;
    PSCSI_REQUEST_BLOCK Srb;
    PSRB_EXTENSION SrbExtension;
    PIO_TRACK_ENTRY pEntry;

    // clear DMA FIFO to eliminate residual data if interrupted during
    // a table indirect move
    WRITE_SIOP_UCHAR( CTEST3, (UCHAR)(READ_SIOP_UCHAR(CTEST3) | CTEST3_CLEAR_FIFO));

    // set IRQ received flag
    DeviceExtension->IRQ_received = TRUE;

    // the 53C1010 SIOP generates an UNEXPECTED DISCONNECT interrupt along
    // with SELECTION TIMEOUT.  We check the SCSI STATUS0 register to see if
    // one has been stacked.  If so, read SCSI STATUS1 register also to clear
    // it.  This is safe since no additional SCSI interrupt should have
    // been generated at this time.

    if (READ_SIOP_UCHAR(SIST0) == SSTAT0_UNEXPECTED_DISCONNECT)
        READ_SIOP_UCHAR(SIST1);

    // get the logical unit extension and SRB for the request that timed out.
    Srb = DeviceExtension->ActiveRequest;

    if (!Srb) {
        return (ISR_START_SCRIPT);
    }

    target = Srb->TargetId;
    DebugPrint((1, "LsiU3(%2x) SelectionTimeout: Timeout for Id=%2x \n",
        DeviceExtension->SIOPRegisterBase,
        target)); 

    // restore LuFlags for this device to initial state (to prepare for a
    // future device arrival)
    lflags = LF_SYNC_NEG_FAILED + LF_NEG_NEEDED;    // these need to be set
    // check if wide should be disabled
    if ( !(DeviceExtension->hbaCapability & HBA_CAPABILITY_WIDE) ||
         DeviceExtension->DeviceTable[target].WideDataBits == WIDE_NONE )
        lflags |= LF_NARROW_NEG_DONE + LF_WIDE_NEG_DONE +
                    LF_WIDE_NEG_FAILED;
    // if sync should be turned off, set sync negotiation done flag
    if ( DeviceExtension->DeviceTable[target].SyncPeriodNs == SYNC_NONE )
        lflags |= LF_SYNC_NEG_DONE;
    // save local lflags value
    DeviceExtension->LuFlags[target] = lflags;


    // free I/O tracking array entry
    SrbExtension = Srb->SrbExtension;
    if ( !SrbExtension )
    {
        DebugPrint((3, "LsiU3(%2x): ProcessSelectionTimeout - SrbExtension is NULL!!\n",
            DeviceExtension->SIOPRegisterBase));
        // this may have been completed already by a LUN or target reset
        // indicate no request is active.
        DeviceExtension->ActiveRequest = NULL;
        return (ISR_START_SCRIPT);
    }
    trackEntry = SrbExtension->trackEntry;
    pEntry = &DeviceExtension->IoTrackArray[trackEntry];
    if ( !(pEntry->Srb) )
    {
        DebugPrint((3, "LsiU3(%2x): ProcessSelectionTimeout -  SRB is NULL!!\n",
            DeviceExtension->SIOPRegisterBase));
        // this may have been completed already by a LUN or target reset
        // indicate no request is active.
        DeviceExtension->ActiveRequest = NULL;
        return (ISR_START_SCRIPT);
    }
    pEntry->Srb = NULL;
    // get I/O tracking entry post index
    trackFIFO = DeviceExtension->TrackPost;
    DeviceExtension->IoTrackFIFO[trackFIFO++] = trackEntry;
    // check for index wrap
    if ( trackFIFO == START_Q_DEPTH )
        trackFIFO = 0;
    // save index
    DeviceExtension->TrackPost = trackFIFO;

    // post queue tag value back to FIFO
    if ( Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE )
    {
        tagIndex = DeviceExtension->QTagPost;
        DeviceExtension->QTagFIFO[tagIndex++] = pEntry->queueTag;
        // check for index wrap
        if ( tagIndex == START_Q_DEPTH )
            tagIndex = 0;
        // save index
        DeviceExtension->QTagPost = tagIndex;
    }

    // indicate this request is no longer active.
    DeviceExtension->ActiveRequest = NULL;

    // indicate selection timeout occurred and notify port driver.

    Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

    StorPortNotification(
                    RequestComplete,
                    DeviceExtension,
                    Srb);
 
    // tell ISR to start new request.

    return( ISR_START_SCRIPT);

}  // ProcessSelectionTimeout


UCHAR
ProcessSynchNegotComplete(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine handles successful async/sync negotiation, as well as
    target initiated negotiation.  The routine first retrieves the
    synchronous period and offset from the message in buffer, then massages
    the parameters into a form the SIOP can use.  It also sets up the wide
    (if doing wide) or sync message to be sent first by a request sense
    command.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    An ISR_ return value for ISR_Service_Next to restart the scripts.

--*/

{
    UCHAR RecvdSynchPeriod, RecvdSynchOffset;
    UCHAR SynchPeriod = 0, SynchOffset = 0, i;
    UCHAR Scntl3Value;
    UCHAR MaxOffset, MinPeriod;
    UCHAR negot_flag, target;
    USHORT luflags, hbaCap;
    ULONG dxp_value, script_addr;
    BOOLEAN dis_sync = FALSE;
    BOOLEAN AutoReqSns = FALSE;
    BOOLEAN reject_flag = FALSE;
    BOOLEAN tin_flag = FALSE;
    PUCHAR  msgPtr;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PDMI_DATA pDmi = &DeviceExtension->DmiData;
    UCHAR sync_values[5] = { 0x09, 0x0A, 0x0C, 0x19, 0x32};
    UCHAR scf_values[5] = { 0x10, 0x10, 0x30, 0x50, 0x70};

    target = Srb->TargetId;
    luflags = DeviceExtension->LuFlags[target];
    hbaCap = DeviceExtension->hbaCapability;

    // set IRQ received flag
    DeviceExtension->IRQ_received = TRUE;

    // if doing OS issued request sense, process like auto request sense
    if ((SrbExtension->autoReqSns || (Srb->Cdb[0] == SCSIOP_REQUEST_SENSE)) &&
                    (SrbExtension->autoReqSns != 2))
    {
        AutoReqSns = TRUE;
        if ( luflags & LF_WIDE_NEG_FAILED )
            DeviceExtension->old_dxp = DeviceExtension->dxp[target];
    }

    if ( !(SrbExtension->SrbExtFlags & LF_SYNC_NEG_PENDING) && !AutoReqSns )
    {
        tin_flag = TRUE;
        SrbExtension->SrbExtFlags |= LF_TIN_SYNC_PENDING;
        DebugPrint((1, "LsiU3(%2x) ProcessSynchNegotComplete: Target initiated sync negotiation\n",
            DeviceExtension->SIOPRegisterBase));
    }
    else
    {
        luflags &= ~LF_SYNC_NEG_PENDING;
        SrbExtension->SrbExtFlags &= ~LF_SYNC_NEG_PENDING;
    }

    if ( SrbExtension->SrbExtFlags & LF_DISABLE_SYNC )
        dis_sync = TRUE;
    
    // Clear the failed negotiation flag. If needed, it will be set on failure.
    // if we have done sync. values, now need to check sstat1_orf
    luflags &= ~LF_SYNC_NEG_FAILED;
    
    // pick up synch period and offset from Script message buffer
    msgPtr = DeviceExtension->svars->msgInBuf;
    RecvdSynchPeriod = StorPortReadRegisterUchar( DeviceExtension, msgPtr);
    RecvdSynchOffset = StorPortReadRegisterUchar( DeviceExtension, &msgPtr[1]);

    MaxOffset = MAX_1010_ST_OFFSET;

    if (RecvdSynchOffset == 0)
        MinPeriod = 0;
    else
    {
        MinPeriod = 0x0A;       // FAST40
        // check for LVDS_DROPBACK and half speed mode
        MinPeriod = set_sync_speed( DeviceExtension, MinPeriod);
    }

    DebugPrint((1, "LsiU3(%2x) SynchronousNegotiation Received - Id=%2x \n",
        DeviceExtension->SIOPRegisterBase,
        target
        ));
    DebugPrint((1, "              Period: %x  Offset: %x\n",
        RecvdSynchPeriod, RecvdSynchOffset));

    // if doing TIN, limit the requested sync period and offset to our device
    // maximums, if necessary
    if ( tin_flag )
    {
        if (RecvdSynchOffset > MaxOffset)
            RecvdSynchOffset = MaxOffset;
        if (RecvdSynchPeriod < MinPeriod)
            RecvdSynchPeriod = MinPeriod;
    }

    if ( RecvdSynchOffset == 0 )    // target agreed only to async
    {
        DeviceExtension->dxp[target] &= CLEAR_OFFSET_VALUES;
        // set DMI device speed to async
        pDmi->DevSpeed[target] = SYNC_NONE;
        // need to set the sync failed flag in either case since it's used
        // to see if we check sstat1_orf on phase mismatches
        luflags |= LF_SYNC_NEG_FAILED;

        if ( dis_sync || AutoReqSns )   // we wanted async, negotiation ok
        {
            luflags |= LF_ASYNC_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) AsynNegotiation Received - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        else                        // we want sync, negotiation failed
        {
            luflags |= LF_SYNC_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) Synchronous Disabled - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
    }
    else                            // target agreed to sync
    {
        if ( dis_sync && !AutoReqSns )
        {
            // we wanted async, negotiation falied
            // need to ignore dis_sync on auto request sense
            DeviceExtension->dxp[target] &= CLEAR_OFFSET_VALUES;
            // set DMI device speed to async
            pDmi->DevSpeed[target] = SYNC_NONE;

            if ( tin_flag )         // doing TIN, will send async response
            {
                DebugPrint((1, "LsiU3(%2x) Returned Async Negotiation- Target: %x\n",
                    DeviceExtension->SIOPRegisterBase, target));
            }
            else                    // not doing TIN, send message reject
            {
                luflags |= LF_ASYNC_NEG_DONE + LF_SYNC_NEG_FAILED;
                DebugPrint((1, "LsiU3(%2x) Rejecting SDTR message.  Asynch failed\n",
                    DeviceExtension->SIOPRegisterBase));
                reject_flag = TRUE;     // set flag to send reject message
            }
        }
        else                        // we got sync, check on values
        {

            // we negotiated for sync, target has responded with sync params

            luflags |= LF_SYNC_NEG_DONE;
            dxp_value = DeviceExtension->dxp[target];
            Scntl3Value = (UCHAR)(dxp_value >> 24) & 0x0F; 
            dxp_value &= CLEAR_OFFSET_VALUES;
            SynchPeriod = RecvdSynchPeriod;
            SynchOffset = RecvdSynchOffset;

            // set DMI sevice speed to sync value (4 * received value)
            pDmi->DevSpeed[target] = RecvdSynchPeriod << 2;

            // normalize received period to 10Mb or 5Mb periods
            // between Fast-10 and Ultra, use Fast-10
            if (SynchPeriod > 0x0C && SynchPeriod <= 0x19)
                SynchPeriod = 0x19;
            // between Fast-5 and Fast-10, use Fast-5
            if (SynchPeriod > 0x19 && SynchPeriod <= 0x32)
                SynchPeriod = 0x32;
            // slower than Fast-5, use async
            if (SynchPeriod > 0x32)
                SynchOffset = 0;

            // find SCF value in table
            for ( i = 0; i < 5; i++ )
            {
                if ( SynchPeriod == sync_values[i])
                {
                    Scntl3Value = (UCHAR)(Scntl3Value | scf_values[i]);
                    dxp_value |= RecvdSynchOffset << 8;
                    break;
                }
            }

            // put new sync period value in dxp_value
            dxp_value &= CLEAR_PERIOD_VALUES;
            dxp_value |= (ULONG)(Scntl3Value) << 24;

            // check for valid values, reject target if not OK
            // workaround for 1010-66 errata at Fast-10
            // don't allow Fast-10 rate, drop back to async
            if ( (RecvdSynchOffset > MaxOffset) ||
                 (RecvdSynchPeriod < MinPeriod) ||
                 (SynchOffset == 0) ||
                 ((hbaCap & HBA_CAPABILITY_1010_66) && (SynchPeriod >= 0x19)) )
            {
                dxp_value &= CLEAR_OFFSET_VALUES;
                // set DMI device speed to async
                pDmi->DevSpeed[target] = SYNC_NONE;

                if ( tin_flag )
                {
                    DebugPrint((1, "LsiU3(%2x) Returned async negotiation. Rate too slow\n",
                        DeviceExtension->SIOPRegisterBase));
                }
                else
                {
                    DebugPrint((1, "LsiU3(%2x) Rejecting SDTR message. Rate too slow\n",
                        DeviceExtension->SIOPRegisterBase));
                    DeviceExtension->LuFlags[target] = luflags + LF_SYNC_NEG_FAILED;
                    reject_flag = TRUE;     // set flag to send reject message
                }
            }

            // store updated dxp_value
            DeviceExtension->dxp[target] = dxp_value;
        }
    }

    // store updated luflags, if not doing auto request sense or TIN
    if (!AutoReqSns && !tin_flag)
    {
        DeviceExtension->LuFlags[target] = luflags;
    }

    if ( !tin_flag )        // if not doing TIN
    {
        // setup negotiation buffer for next auto request sense negotiation
        negot_flag = (luflags & LF_WIDE_NEG_FAILED) ?
                     SETUP_SYNC_FIRST : SETUP_WIDE_FIRST;
    }
    else                    // doing TIN
    {
        // if negotiation is OK, turn on sync_neg_done flag if no sync
        // negotiation is currently pending or disable sync flag is not set.
        // Reset sync reject and sync failed flags.
        if (!reject_flag && !(luflags & LF_SYNC_NEG_PENDING) && !dis_sync)
        {
            DeviceExtension->LuFlags[target] |= LF_SYNC_NEG_DONE;
            DeviceExtension->LuFlags[target] &=
                            ~(LF_SYNC_NEG_REJECT + LF_SYNC_NEG_FAILED);
        }
        // save asked for period and offset
        DeviceExtension->tin_rec_period = SynchPeriod;
        DeviceExtension->tin_rec_offset = SynchOffset;
        // setup msgBufOut for TIN response
        negot_flag = SETUP_SYNC_TIN;
    }
    SetupNegotBuf( DeviceExtension, target, negot_flag);

    // see if we need to update device descriptors for all outstanding commands
    if (!AutoReqSns || (DeviceExtension->old_dxp != DeviceExtension->dxp[target]))
        UpdateStartQDesc( DeviceExtension, Srb);

    // copy updated dxp entry to Scripts RAM dxp
    StorPortWriteRegisterUlong( DeviceExtension,
                                &DeviceExtension->dxpSR[target],
                                DeviceExtension->dxp[target]);

    // if negotiation truly failed, send reject message or if doing TIN
    // restart scripts at ContNegScript
    if ( reject_flag || tin_flag )
    {
        // reset sync parameters in sxfer, send a message reject
        WRITE_SIOP_UCHAR(SXFER, (UCHAR)(DeviceExtension->dxp[target] >> 8));
        script_addr = (tin_flag) ? DeviceExtension->ContNegScriptPhys :
                                    DeviceExtension->RejectScriptPhys;
        StartSIOP( DeviceExtension, script_addr);
        return( ISR_EXIT );
    }

    return( ISR_RELOAD_SCRIPT);

} // ProcessSynchNegotComplete


UCHAR
ProcessNegotNotSupported(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine handles unsuccessful negotiations (sync, wide, & PPR).
    For WDTR or SDTR, this routine sets the synchronous parameters to
    asynchronous, or wide parameters to narrow, and sets the appropriate flags.
    For a PPR rejection, the routine will setup to perform wide negotiations.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_RELOAD_SCRIPT to reload dxp parameters and restart script

--*/

{
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr = DeviceExtension->localSvdt;
    UCHAR target, MessageCount, negot_flag;
    UCHAR neg_flags = SrbExtension->SrbExtFlags;
    USHORT luflags;
    BOOLEAN dis_sync = FALSE;
    BOOLEAN sync_reject = FALSE;
    BOOLEAN autoSnsWide = FALSE;
    BOOLEAN OSRS_Wide = FALSE;
    BOOLEAN contNeg = FALSE;
    PDMI_DATA pDmi = &DeviceExtension->DmiData;

    target = Srb->TargetId;
    luflags = DeviceExtension->LuFlags[target];
    if ( neg_flags & LF_DISABLE_SYNC )
        dis_sync = TRUE;
    
    // set IRQ received flag
    DeviceExtension->IRQ_received = TRUE;

    // check for host initiated or target initiated PPR negotiations
    negot_flag = neg_flags & (LF_PPR_NEG_PENDING | LF_TIN_PPR_PENDING);
    if ( (negot_flag == LF_PPR_NEG_PENDING) ||
         (negot_flag == LF_TIN_PPR_PENDING) )
    {
        luflags &= ~LF_PPR_NEG_PENDING;
        SrbExtension->SrbExtFlags &=
            ~(LF_PPR_NEG_PENDING + LF_TIN_PPR_PENDING);
        if ( neg_flags & LF_PPR_NEG_PENDING )
        {
            // host initiated PPR negotiation, but was rejected
            // set flags to never try PPR negotiation again
            luflags |= LF_PPR_NEG_REJECT;
            DeviceExtension->LuFlags[target] = luflags;
            DebugPrint((1, "LsiU3(%2x) PPR negotiation failed, Id=%x \n",
                DeviceExtension->SIOPRegisterBase,
                target));
            // setup negotiations again, this time with WDTR/SDTR
            MessageCount = 0;
            MessageCount = StartNegotiations(DeviceExtension, Srb,
                                              MessageCount, FALSE);
            // if MessageCount = 0, then no wide/sync negotiations are needed
            if (MessageCount)
            {
                StorPortWriteRegisterUlong( DeviceExtension,
                                            &svdtPtr->msgOutBufDescriptor.count,
                                           (ULONG)MessageCount);
                contNeg = TRUE;
            }
        }
        else
        {
            // target initiated PPR negotiation, but rejected our response
            DebugPrint((1, "LsiU3(%2x) Target initiated PPR negotiation rejected, Id=%x \n",
                DeviceExtension->SIOPRegisterBase,
                target));
            // probably need something here to get to common parameters
        }
    }
    else    // not a PPR negotiation
    {
        // check if doing a wide negotiation during auto request sense
        // (3rd byte of NegotMsg buffer = 2 if doing wide negotiation)
        if ( SrbExtension->autoReqSns ) 
        {
            if (DeviceExtension->NonCachedExtension->NegotMsg[target].Buf[2] == 2)
                autoSnsWide = TRUE;
            else
                sync_reject = TRUE;
        }
    
        // check if doing a wide negotiation during OS issued request sense
        // (3rd byte of msgOutBuf = 2 if doing wide negotiation)
        if ( Srb->Cdb[0] == SCSIOP_REQUEST_SENSE ) 
        {
            if ( SrbExtension->svdt->msgOutBuf[2] == 2 )
                OSRS_Wide = TRUE;
            else
                sync_reject = TRUE;
        }
    
        // if doing sync negotiations during either auto or OS issued request
        // sense, set flags to never try sync negotiations again and setup
        // NegotBuf for next request sense command
        if ( sync_reject )
        {
            // set flags to never try sync negotiation again
            DeviceExtension->LuFlags[target] |= LF_ASYNC_NEG_DONE +
                LF_SYNC_NEG_DONE + LF_SYNC_NEG_FAILED + LF_SYNC_NEG_REJECT;
            // need to setup NegotBuf in preparation for next req sns cmd
            // don't do wide if previous wide negotation has failed
            negot_flag = (luflags & LF_WIDE_NEG_FAILED) ?
                        SETUP_SYNC_FIRST : SETUP_WIDE_FIRST;
            SetupNegotBuf( DeviceExtension, target, negot_flag);
            // set DMI device speed to async
            pDmi->DevSpeed[target] = SYNC_NONE;
            return(ISR_RESTART_SCRIPT);
        }
            
        // if host initiated or target initiated sync negotiation
        if (neg_flags & (LF_SYNC_NEG_PENDING + LF_TIN_SYNC_PENDING) )
        {
            luflags &= ~LF_SYNC_NEG_PENDING;
            // need to set sync failed for either async or sync, used as a flag
            // to check sstat1_orf in phase mismatch
            luflags |= LF_SYNC_NEG_FAILED;
            SrbExtension->SrbExtFlags &=
                ~(LF_SYNC_NEG_PENDING + LF_TIN_SYNC_PENDING);
            DeviceExtension->dxp[target] &= CLEAR_OFFSET_VALUES;
            // set DMI device speed to async
            pDmi->DevSpeed[target] = SYNC_NONE;
    
            if ( neg_flags & LF_SYNC_NEG_PENDING )
            {
                // host initiated sync negotiation, but was rejected
                // set flags to never try sync negotiation again
                luflags |= LF_ASYNC_NEG_DONE + LF_SYNC_NEG_DONE +
                                LF_SYNC_NEG_REJECT;
                DeviceExtension->LuFlags[target] = luflags;
                // need to setup NegotBuf in preparation for next req sns cmd
                // don't do wide if previous wide negotation has failed
                negot_flag = (luflags & LF_WIDE_NEG_FAILED) ?
                            SETUP_SYNC_FIRST : SETUP_WIDE_FIRST;
                SetupNegotBuf( DeviceExtension, target, negot_flag);
                DebugPrint((1, "LsiU3(%2x) Sync/Async negotiation failed, Id=%x \n",
                    DeviceExtension->SIOPRegisterBase,
                    target));
            }
            else
            {
                // target initiated sync negotiation, but rejected our response
                DebugPrint((1, "LsiU3(%2x) Target initiated sync negotiation rejected, Id=%x \n",
                    DeviceExtension->SIOPRegisterBase,
                    target));
            }
        }
        else        // wide negotiation (possibly during auto request sense)
        {
            luflags &= ~LF_WIDE_NEG_PENDING;
            // if we get a reject on a narrow or wide negotiation, set both
            // done along with the failed flag, so we won't try wide again
            luflags |= LF_NARROW_NEG_DONE + LF_WIDE_NEG_DONE +
                       LF_WIDE_NEG_FAILED;
            SrbExtension->SrbExtFlags &=
                ~(LF_WIDE_NEG_PENDING + LF_TIN_WIDE_PENDING);
            DeviceExtension->dxp[target] &= DISABLE_WIDE; 
            // set DMI device width to narrow
            pDmi->DevWidth[target] = 8;
    
            // if host initiated wide negotiation or during auto request sense
            // or during OS issued request sense
            if ((neg_flags & LF_WIDE_NEG_PENDING) || autoSnsWide || OSRS_Wide)
            {
                DeviceExtension->LuFlags[target] = luflags;
    
                DebugPrint((1, "LsiU3(%2x) Narrow/wide negotiation failed, Id=%x \n",
                    DeviceExtension->SIOPRegisterBase,
                    target));
    
                // although wide negotiation was rejected, continue with
                // synchronous negotiation on this same I/O
                // check if doing auto request sense or not
                if (!autoSnsWide)
                {
                    MessageCount = 0;
                    MessageCount = StartNegotiations(DeviceExtension, Srb,
                                                      MessageCount, FALSE);
                    if (MessageCount)
                        StorPortWriteRegisterUlong( DeviceExtension,
                                        &svdtPtr->msgOutBufDescriptor.count,
                                                   (ULONG)MessageCount);
                }
                else
                {
                    SetupNegotBuf( DeviceExtension, target, SETUP_SYNC_CONT);
                    MessageCount = 1;   // needed for next test
                }
                // if MessageCount = 0, then no sync negotiations are needed
                if (MessageCount)
                    contNeg = TRUE;
            }
            else
            {
                // target initiated wide negotiation, but rejected our response
                DebugPrint((1, "LsiU3(%2x) Target initiated wide negotiation rejected, Id=%x \n",
                    DeviceExtension->SIOPRegisterBase,
                    target));
            }
        }
    
        // copy updated dxp entry to Scripts RAM dxp
        StorPortWriteRegisterUlong( DeviceExtension,
                                    &DeviceExtension->dxpSR[target],
                                    DeviceExtension->dxp[target]);
    } // end of non PPR negotiation

    // see if we need to continue negotiations
    if ( contNeg )
    {
        StartSIOP( DeviceExtension, DeviceExtension->ContNegScriptPhys);
        return( ISR_EXIT);
    }
    return( ISR_RELOAD_SCRIPT);

} // ProcessNegotNotSupported



UCHAR
ProcessWideNegotComplete(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine handles successful narrow/wide negotiation, as well as
    responding to target initiated negotiation.  The routine first retrieves
    the wide width from the message in buffer, then massages the parameters
    into a form the SIOP can use.  It also sets up the continuation of
    sync negotiations if doing auto request sense.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    An ISR_ return value for ISR_Service_Next to restart the scripts.

--*/

{     
    UCHAR RecvdWideWidth, target, MessageCount, negot_type;
    USHORT luflags;
    BOOLEAN dis_sync = FALSE;
    BOOLEAN AutoReqSns = FALSE;
    BOOLEAN OS_ReqSns = FALSE;
    BOOLEAN tin_flag = FALSE;
    PSCSI_REQUEST_BLOCK Srb = DeviceExtension->ActiveRequest;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;

    target = Srb->TargetId;
    luflags = DeviceExtension->LuFlags[target];

    // set IRQ received flag
    DeviceExtension->IRQ_received = TRUE;

    // see if doing auto request sense
    if ( SrbExtension->autoReqSns == 1 )
        AutoReqSns = TRUE;

    // is OS doing request sense?
    if ((Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) &&
                (SrbExtension->autoReqSns != 2) )
        OS_ReqSns = TRUE;

    if ( !(SrbExtension->SrbExtFlags & LF_WIDE_NEG_PENDING) &&
                !AutoReqSns && !OS_ReqSns )
    {
        tin_flag = TRUE;
        SrbExtension->SrbExtFlags |= LF_TIN_WIDE_PENDING;
        // since we're doing wide negotiation, reset sync params to async
        // set sync not done and negotiations needed
        DeviceExtension->dxp[target] &= CLEAR_OFFSET_VALUES;
        luflags &= ~(LF_SYNC_NEG_DONE + LF_SYNC_NEG_REJECT +
                     LF_WIDE_NEG_FAILED);
        DeviceExtension->LuFlags[target] =
                luflags | (LF_NEG_NEEDED + LF_SYNC_NEG_FAILED);
        DebugPrint((1, "LsiU3(%2x) ProcessWideNegotComplete: Target initiated wide negotiation\n",
            DeviceExtension->SIOPRegisterBase));
    }
    else
    {
        luflags &= ~LF_WIDE_NEG_PENDING;
        SrbExtension->SrbExtFlags &= ~LF_WIDE_NEG_PENDING;
        DebugPrint((1, "LsiU3(%2x) WideNegotiation Received - Target: %x\n",
            DeviceExtension->SIOPRegisterBase, target));
    }

    if ( AutoReqSns || OS_ReqSns || tin_flag )
        // save dxp to check for changes
        DeviceExtension->old_dxp = DeviceExtension->dxp[target];

    if ( SrbExtension->SrbExtFlags & LF_DISABLE_SYNC )
        dis_sync = TRUE;

    RecvdWideWidth = StorPortReadRegisterUchar( DeviceExtension,
                                            DeviceExtension->svars->msgInBuf);

    if (RecvdWideWidth == 0)    // target agreed only to narrow
    {
        DeviceExtension->dxp[target] &= DISABLE_WIDE;
        if (dis_sync || AutoReqSns || OS_ReqSns || tin_flag)
        {
            // we wanted narrow, negotiation ok
            luflags |= LF_NARROW_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) NarrowNegotiation Agreed - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        else                    // we wanted wide, negotiation failed
        {
            luflags |= LF_WIDE_NEG_DONE + LF_WIDE_NEG_FAILED;
            DebugPrint((1, "LsiU3(%2x) Wide Disabled - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
    }
    else                        // target agreed with wide
    {
        // if not doing TIN or sync not disabled, and we have wide capability
        if ( (!tin_flag || !dis_sync) && !(luflags & LF_WIDE_NEG_FAILED) )
        {
            DeviceExtension->dxp[target] |= ENABLE_WIDE;
            luflags |= LF_WIDE_NEG_DONE;
            DebugPrint((1, "LsiU3(%2x) WideNegotiation Agreed - Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
        else        // doing TIN and sync is disabled, or no wide capability
        {
            DeviceExtension->dxp[target] &= DISABLE_WIDE;
            DebugPrint((1, "LsiU3(%2x) Returned Narrow Negotiation- Target: %x\n",
                DeviceExtension->SIOPRegisterBase, target));
        }
    }

    if (!AutoReqSns && !tin_flag)   // if not doing auto request sense or TIN
    {
        // update LuFlags from local variable if not doing OS issued req sns
        if ( !OS_ReqSns )
            DeviceExtension->LuFlags[target] = luflags;

        // setup to continue sync negotiations on this I/O
        MessageCount = 0;
        MessageCount = StartNegotiations( DeviceExtension, Srb,  MessageCount,
                                          FALSE);
        // if MessageCount = 0, don't need to do sync negotiation
        if (MessageCount)
            StorPortWriteRegisterUlong( DeviceExtension, 
                    &DeviceExtension->localSvdt->msgOutBufDescriptor.count,
                                        (ULONG)MessageCount);
        else
            // won't do sync, so update message buffer now
            SetupNegotBuf( DeviceExtension, target, SETUP_WIDE_FIRST);
    }
    else                    // if doing auto request sense or TIN
    {
        // setup message buffer to continue with sync neg for auto req sns
        // or to respond to wide TIN
        negot_type = (tin_flag) ? SETUP_WIDE_TIN : SETUP_SYNC_CONT;
        SetupNegotBuf( DeviceExtension, target, negot_type);
        MessageCount = 1;   // need non-zero value as flag for later
    }

    // see if we need to update device descriptors in the start queue
    if ( tin_flag && (DeviceExtension->old_dxp != DeviceExtension->dxp[target]))
        UpdateStartQDesc( DeviceExtension, Srb);

    // copy updated dxp entry to Scripts RAM dxp
    StorPortWriteRegisterUlong(  DeviceExtension,
                                 &DeviceExtension->dxpSR[target],
                                 DeviceExtension->dxp[target]);

    // set DMI device width field according to new width
    RecvdWideWidth = 8;
    if (DeviceExtension->dxp[target] & ENABLE_WIDE)
        RecvdWideWidth = 16;
    DeviceExtension->DmiData.DevWidth[target] = RecvdWideWidth;

    // restart scripts to continue negotiations if sync negotiation is needed,
    // or to restart scripts to continue this I/O
    if (MessageCount)
    {
        // restart to continue negotiations
        StartSIOP( DeviceExtension, DeviceExtension->ContNegScriptPhys);
        return (ISR_EXIT);
    }
    else
    {
        // restart to continue I/O
        return (ISR_RELOAD_SCRIPT);
    }

} // ProcessWideNegotComplete


UCHAR
ProcessUnexpectedDisconnect(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine processes unexpected disconnects.  An unexpected disconnect
    is defined as a disconnect occurring before a disconnect message is
    received.  The SCSI bus is reset since we can't reliably determine which
    I/O failed due to the unexpected disconnect.

Arguments:

    DeviceContext - Supplies a pointer to the device extension for the
        interrupting adapter.

Return Value:

    ISR_START_SCRIPT to restart the scripts.

--*/

{
    UCHAR function, id;
    ULONG ScriptPhysAddr;
    PSCSI_REQUEST_BLOCK Srb;

    // clear DMA FIFO to eliminate residual data if interrupted during
    // a table indirect move
    WRITE_SIOP_UCHAR( CTEST3, (UCHAR)(READ_SIOP_UCHAR(CTEST3) | CTEST3_CLEAR_FIFO));

    // need to check validity of our context pointer before using it
    // get current Scripts pointer
    ScriptPhysAddr = READ_SIOP_ULONG(DSP);
    // test for script address in area of valid context
    if ( (ScriptPhysAddr > DeviceExtension->CommandScriptPhys) &&
         (ScriptPhysAddr < DeviceExtension->DataOutJump1Phys) )
    {
        // set Srb and function
        Srb = DeviceExtension->ActiveRequest;
        function = Srb->Function;

        if ( (function == SRB_FUNCTION_RESET_DEVICE) ||
             (function == SRB_FUNCTION_RESET_LOGICAL_UNIT) )
            return( ProcessDeviceResetOccurred( DeviceExtension));

        if ( (function == SRB_FUNCTION_ABORT_COMMAND) ||
             (function == SRB_FUNCTION_TERMINATE_IO) )
        {
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            StorPortNotification( RequestComplete, DeviceExtension, Srb);
            DeviceExtension->ActiveRequest = NULL;
            return( ISR_START_SCRIPT);
        }
    }

    // reset the active request
    DeviceExtension->ActiveRequest = NULL;
    // log unexpected disconnect
    id = READ_SIOP_UCHAR(SSID) & 0x0F;  // get id that reselected
    StorPortLogError(DeviceExtension, NULL, 0, id, 0,
                     SP_UNEXPECTED_DISCONNECT, 0);
    // reinitialize SIOP (can't determine reliably which I/O disconnected)
    // set ResetActive
    DeviceExtension->ResetActive = 14;
    // call routine to schedule reinitialization of the SIOP
    ScheduleReinit( DeviceExtension);
    return( ISR_EXIT);

} // ProcessUnexpectedDisconnected


VOID
ScheduleReinit(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine prepares to reinitialize the SIOP.  It pauses the adapter to
    block new I/O's, then does a timer routine call to a TimerReinit routine
    which calls SynchronizeAccess to a SyncrhonizeISRReinit routine that does
    the actual reinitialization.  This is necessary because we need to
    synchronize our StartIo and ISR threads during the reinitialization, and
    a SynchronizeAccess call cannot be done from the ISR thread.

Arguments:

    DeviceExtension - Supplies a pointer to the specific device extension.

Return Value:

    None

--*/
{
    // pause the adapter to block any new I/Os
    StorPortPause( DeviceExtension, 60);

    // request a timer call to the TimerReinit routine in 100 msec
    StorPortNotification( RequestTimerCall, DeviceExtension,
                          TimerReinit, (ULONG)100000);
}

VOID
TimerReinit(
    _In_ PVOID Context
    )
/*++

Routine Description:

    This routine is called via a RequestTimerCall from an ISR thread to
    perform a SynchronizeAccess call to SynchronizeISRReinit.  This routine
    will reinitialize the SIOP with both StartIo and ISR threads syncrhonized.

Arguments:

    Context - Supplies a pointer to the specific adapter device extension.

Return Value:

    None

--*/
{
    PHW_DEVICE_EXTENSION DeviceExtension = Context;

    // call SyncrhonizeISRReinit via a SyncrhonizeAccess call
    StorPortSynchronizeAccess( DeviceExtension,
                               SynchronizeISRReinit,
                               NULL);
}

BOOLEAN
SynchronizeISRReinit(
    _In_ PVOID Context,
    _In_ PVOID dummy
    )
/*++

Routine Description:

    This routine will reinitialize the SIOP with both StartIo and ISR threads
    syncrhonized.  The ResetSCSIBus routine will perform a bus reset.  This
    will generate an interrupt which will be processed after leaving this
    routine.  The processing of the bus reset interrupt will cause the Scripts
    to be restarted.

Arguments:

    Context - Supplies a pointer to the specific adapter device extension.
    
    dummy - Pointer value supplied via SyncrhonizeAccess (not used)

Return Value:

    TRUE (not used)

--*/
{
    PHW_DEVICE_EXTENSION DeviceExtension = Context;

    UNREFERENCED_PARAMETER( dummy );    

    // reinitialize the SIOP
    InitializeSIOP( DeviceExtension);
    // reset the SCSI bus
    ResetSCSIBus( DeviceExtension);

    // resume I/Os
    StorPortResume( DeviceExtension);
    // clear ResetActive
    DeviceExtension->ResetActive = 0;

    return (TRUE);
}


VOID
ResetSCSIBus(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine resets the SCSI bus and calls the bus reset postprocess
    routine.

    Note that since the 53C1010 generates a bus reset interrupt when we reset
    the bus, we set a flag indicating we have reset the bus so the reset
    postprocess routine will not be called twice.

Arguments:

    DeviceExtension - Supplies a pointer to the specific device extension.

Return Value:

    None

--*/

{
    DebugPrint((1, "LsiU3(%2x) ResetSCSIBus\n",
        DeviceExtension->SIOPRegisterBase));

    // set the bus reset line high
    WRITE_SIOP_UCHAR(SCNTL1, (UCHAR) ( READ_SIOP_UCHAR(SCNTL1)
            | (UCHAR) SCNTL1_RESET_SCSI_BUS));

    // Delay the minimum assertion time for a SCSI bus reset to make sure a
    // valid reset signal is sent.

    StorPortStallExecution( RESET_STALL_TIME);

    WRITE_SIOP_UCHAR(SCNTL1, (UCHAR) ( READ_SIOP_UCHAR(SCNTL1)
                         & (UCHAR) ~SCNTL1_RESET_SCSI_BUS));

    // indicate that we reset the bus locally.
    DeviceExtension->DeviceFlags |= DFLAGS_BUS_RESET;

    BusResetPostProcess(DeviceExtension);

}  // ResetSCSIBus


USHORT
ScatterGatherScriptSetup(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN hostSvdt
    )
/*++

Routine Description:

    This routine calculates physical break pointers and transfer lengths to
    create move instructions (scripts) for each S/G element.

    NOTE:  The first instruction in the S/G list is an int 0x20 when data
    [in|out] to test for a wrong transfer direction.  This handles cases
    where the data direction flags in SrbFlags are wrong or unspecified.

Arguments:

    DeviceExtension - Supplies the device Extension for the SCSI bus adapter.

    Srb - Supplies the Srb pointer for this I/O.

    hostSvdt - Flag to determine which svdt structure to use:
                    TRUE  = use svdt located in SrbExtension (host memory)
                    FALSE = use svdt located in Scripts RAM (adapter memory)
                    
Return Value:

    Length of total IOV list in bytes.

--*/

{
    BOOLEAN dataIn, do64bit;
    USHORT iovLen, i;
    ULONG scriptCmd, numElements, loop;
    ULONG ElementLength;
    ULONG_PTR iovStart;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PULONG iovPtr, iovSR;
    PSTOR_SCATTER_GATHER_LIST pSpSGStruct;
    PSTOR_SCATTER_GATHER_ELEMENT pSpSGL;

    // set flag if we should do 64 bit addressing
    do64bit = (DeviceExtension->DeviceFlags & DFLAGS_64BIT_ADDRESS) ?
                    TRUE : FALSE;

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
    {
        dataIn = TRUE;
        scriptCmd = DATA_IN_SCRIPT;
    }
    else
    {
        dataIn = FALSE;
        scriptCmd = DATA_OUT_SCRIPT;
    }

    // Set pointer into Srbextension iov list
    // if iovList needs to be in Scripts RAM, we'll copy it later
    iovPtr = (PULONG)&SrbExtension->svdt->iovList;

    // save starting address for later length calculation
    iovStart = (ULONG_PTR)iovPtr;

    // build int instruction to test data phase direction in first element
    *iovPtr++ = scriptCmd | INT_CMD_MASK;
    *iovPtr++ = SCRIPT_INT_BAD_XFER_DIRECTION;

     // get pointer to StorPort scatter/gather list
    pSpSGStruct = StorPortGetScatterGatherList( DeviceExtension, Srb);
    numElements = pSpSGStruct->NumberOfElements;
    pSpSGL = pSpSGStruct->List;

    // build the SG move instructions
    for ( loop = 1; loop <= numElements; loop++ )
    {
        ElementLength = pSpSGL->Length;
        // for data out, last element needs to be a MOVE instead of a CHMOV
        // for data in, all elements must be a CHMOV (1010 errata)
        if ( (loop == numElements) && !dataIn )
        {
            *iovPtr++ = scriptCmd | MOVE_CMD_SCRIPT | ElementLength;
        }
        else
        {
            *iovPtr++ = scriptCmd | ElementLength;
        }

        // next dword is low 32-bits of physical address
        *iovPtr++ = pSpSGL->PhysicalAddress.LowPart;

        // if using 64-bit addresses, next dword is high 32-bits
        if (do64bit)
            *iovPtr++ = pSpSGL->PhysicalAddress.HighPart;

        // bump SGL pointer
        pSpSGL++;
    }

    // if using 64-bit addresses, insert instruction to turn off 64-bit mode
    if (do64bit)
    {
        *iovPtr++ = (ULONG)DISABLE_64BITS;
        *iovPtr++ = 0;
    }

    // finish up SG list with return instruction
    *iovPtr++ = (ULONG)RETURN_SCRIPT;
    *iovPtr++ = 0;

    SrbExtension->PhysBreakCount = (UCHAR)numElements;

    iovLen = (USHORT)((ULONG_PTR)iovPtr - iovStart);

    // if hostSvdt flag is false, move the new iovList to Scripts RAM
    if (!hostSvdt)
    {
        iovPtr = (PULONG)&SrbExtension->svdt->iovList;
        iovSR = (PULONG)&DeviceExtension->localSvdt->iovList;
        for ( i = 0; i < (iovLen / 4); i++)
            StorPortWriteRegisterUlong( DeviceExtension, iovSR++, *(iovPtr++));
    }

    DebugPrint((3, "LsiU3(%2x) LsiU3ScatterGather: Phys breaks = %2x, total size = %8x \n",
        DeviceExtension->SIOPRegisterBase,
        numElements,
        Srb->DataTransferLength));

    return(iovLen);

} // ScatterGatherScriptSetup


VOID
SetChipModes (
    _In_ PHW_DEVICE_EXTENSION DeviceExtension
    )
/********************************************************************************

Routine Description:

    This routine sets the appropriate chip options for the 1010 chip.  These
    chip options are:  Burst Size, Read Line Enable, Read Multiple Enable,
    Prefetch Enable, and Write & Invalidate Enable.

Arguments:

    DeviceExtension - Supplies a pointer to device extension for the bus that
        is being initialized.

Return Value:

    none.

--*/
{
    // save chip revision in DMI data
    DeviceExtension->DmiData.HW_Revision = DeviceExtension->chip_rev;

    // Prefetch Enable
    WRITE_SIOP_UCHAR(DCNTL, (UCHAR)(READ_SIOP_UCHAR(DCNTL) |
                  (DCNTL_PREFETCH_FLUSH + DCNTL_PREFETCH_ENABLE)));

    // Write & Invalidate Enable
    WRITE_SIOP_UCHAR(CTEST3, (UCHAR)(READ_SIOP_UCHAR(CTEST3) |
                        CTEST3_WRITE_INVALIDATE));

    // enable Read Line & Read Multiple, set burst size (along with CTEST5)
    WRITE_SIOP_UCHAR(DMODE, (UCHAR)(READ_SIOP_UCHAR(DMODE) |
            (DMODE_BURST_0 + DMODE_ENA_READ_LINE + DMODE_ENA_READ_MULT)));

    // enable 64-transfer burst
    WRITE_SIOP_UCHAR(CTEST5, (UCHAR)(READ_SIOP_UCHAR(CTEST5) | CTEST5_BURST));

} // SetChipModes


VOID
SetupLuFlags(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR ResetFlag
    )
/*++

Routine Description:

    This routine sets up the LU flags which hold information such as whether
    a peripheral device supports wide or synchronous.  It also sets up initial
    values for the NegotMsg buffers for negotiation on request sense.

Arguments:

    PHW_DEVICE_EXTENSION DeviceExtension

    UCHAR ResetFlag - 0 = called during initialization, 1 = called after reset

Return Value:

    None.

--*/

{
    PHW_NONCACHED_EXTENSION NonCachedExtPtr =  DeviceExtension->NonCachedExtension;
    UCHAR target;
    UCHAR max_targets;
    UCHAR *msgptr;
    UINT8 wide_bits;
    USHORT lflags, save_lflags;
    ULONG length;
    BOOLEAN wide = TRUE;
    PNEGOT_BUF bufPtr = NonCachedExtPtr->NegotMsg;
    PSVARS_DESCRIPTOR negotDescPtr = NonCachedExtPtr->NegotMsgBufDesc;
    PDMI_DATA pDmi = &DeviceExtension->DmiData;

    //
    // Indicate that no negotiations have been done.  However, if the chip
    // can't do wide or the NVRAM is set for narrow only, mark wide
    // negotiations as being done so we won't keep checking.  If bus has
    // been reset, mark narrow & async negotiations as done.
    //
    if ( DeviceExtension->hbaCapability & HBA_CAPABILITY_WIDE)
    {
        max_targets = SYM_MAX_TARGETS;
    }
    else
    {
        max_targets = SYM_NARROW_MAX_TARGETS;
        wide = FALSE;
    }

    for (target = 0; target < max_targets; target++) {

        lflags = save_lflags = DeviceExtension->LuFlags[target];
        wide_bits = DeviceExtension->DeviceTable[target].WideDataBits;
        if ( ResetFlag )
        {
            // set negotiation flags after a bus reset, save wide failed status
            if ( !wide || (save_lflags & LF_WIDE_NEG_FAILED) ||
                    (wide_bits == WIDE_NONE) )
            {
                lflags = LF_NARROW_NEG_DONE + LF_WIDE_NEG_DONE +
                            LF_WIDE_NEG_FAILED;
            }
            else
            {
                lflags = 0;
            }

            lflags |= LF_ASYNC_NEG_DONE + LF_NARROW_NEG_DONE;
            // if sync has been rejected before, set flags to not negotiate
            if ( save_lflags & LF_SYNC_NEG_REJECT )
                lflags |= LF_SYNC_NEG_DONE + LF_SYNC_NEG_REJECT;

            // if PPR has been rejected before, set flag
            if ( save_lflags & LF_PPR_NEG_REJECT )
                lflags |= LF_PPR_NEG_REJECT;
        }
        else
        {
        // set negotiation flags during driver initialization
            if ( !wide || (wide_bits == WIDE_NONE) )
                lflags = LF_NARROW_NEG_DONE + LF_WIDE_NEG_DONE +
                            LF_WIDE_NEG_FAILED;
            else
                lflags = 0;
        }

        // check NVRAM setting to see if sync is turned off.  if so,
        // set sync negotiation done and reject flags
        if ( DeviceExtension->DeviceTable[target].SyncPeriodNs == SYNC_NONE )
            lflags |= LF_SYNC_NEG_DONE + LF_SYNC_NEG_REJECT + 
                      LF_PPR_NEG_REJECT;

        // needed as a check so we only look at sstat1_orf when we are sync
        // set failed bit as a check to see when we actually get to go sync.
        lflags |= LF_SYNC_NEG_FAILED;

        // if both wide & sync haven't been rejected, set neg_needed flag
        // for quick check to call start negotiation.
        if ( !(lflags & LF_WIDE_NEG_FAILED) || !(lflags & LF_SYNC_NEG_REJECT) )
            lflags |= LF_NEG_NEEDED;

        // save local lflags value
        DeviceExtension->LuFlags[target] = lflags;

        // Setup NegotMsg buffers for negotiations on request sense commands.
        // If PPR has not been rejected before, beging with PPR negotiation.
        // If adapter can do wide, will default to negotiate narrow (unless
        // this ID has failed a wide negotiation in the past).  Otherwise,
        // will setup for async negotiation.

        if ( !(lflags & LF_PPR_NEG_REJECT) )
        {
            length = (ULONG)(sizeof ppr_msg);
            msgptr = ppr_msg;
        }
        else if ( lflags & LF_WIDE_NEG_FAILED )
        {
            length = (ULONG)(sizeof async_msg);
            msgptr = async_msg;
            if (lflags & LF_SYNC_NEG_REJECT)    // do no negotiations
                length = 0;                     // only identify byte
        }
        else
        {
            length = (ULONG)(sizeof narrow_msg);
            msgptr = narrow_msg;
        }
        negotDescPtr[target].count = length + 1;  // add 1 for identify byte
        // leave room for identify byte during move
        if (length)
            StorPortMoveMemory( &bufPtr[target].Buf[1], msgptr, length);
    }

    // at initialization set all DMI device speeds to FF, not negotiated,
    // and all device widths to 8.
    if (!ResetFlag)
        for (target = 0; target < SYM_MAX_TARGETS; target++)
        {
            pDmi->DevSpeed[target] = 0xFF;
            pDmi->DevWidth[target] = 8;
        }

} // SetupLuFlags


VOID
SetupNegotBuf(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR target,
    _In_ UCHAR negot_type
    )
/*++

Routine Description:

    This routine sets up the negotiation buffer used during auto request sense
    negotiations or responding to target initiated negotiation (TIN) for a
    particular target.

    Note:  When responding to TIN, use the SrbExtension msgOutBuf in order to
    keep the NegotMsgBuf array intact for any future request sense commands.

Arguments:

    PHW_DEVICE_EXTENSION DeviceExtension

    UCHAR target - SCSI ID for this particular device

    UCHAR negot_type - SETUP_WIDE_FIRST - 1st negotiation of wide
                       SETUP_WIDE_TIN   - wide TIN response
                       SETUP_SYNC_FIRST - 1st negotiation of sync
                       SETUP_SYNC_CONT  - sync negotiation after wide
                       SETUP_SYNC_TIN   - sync TIN response
                       SETUP_PPR_FIRST  - 1st negotiation of PPR
                       SETUP_PPR_TIN    - PPR TIN response

Return Value:

    None.

--*/

{
    PHW_NONCACHED_EXTENSION NonCachedExtPtr =  DeviceExtension->NonCachedExtension;
    UCHAR period, nv_period, offset, ptr, DT_flag, i;
    UCHAR tmpBuf[8];
    ULONG length;
    BOOLEAN wide = FALSE;
    PUCHAR msgPtr;
    PNEGOT_BUF bufPtr = NonCachedExtPtr->NegotMsg;
    PSVARS_DESCRIPTOR negotDescPtr = NonCachedExtPtr->NegotMsgBufDesc;
    PSVARS_DESCRIPTOR_TABLE svdtPtr = DeviceExtension->localSvdt;

	RtlZeroMemory(&tmpBuf, 8*sizeof(UCHAR));

    // get period NVRAM setting
    nv_period = (UCHAR)DeviceExtension->DeviceTable[target].SyncPeriodNs / 4;

    // check if wide (16 bits) is enabled,
    if ( DeviceExtension->dxp[target] & ENABLE_WIDE )
        wide = TRUE;        // set to negotiate 16 bits

    // check for setting up PPR negotiations first
    if ( negot_type >= SETUP_PPR_FIRST )
    {
        // if not doing TIN, setup for our maximums
        if ( negot_type != SETUP_PPR_TIN )
        {
            // set sync period to NVRAM setting, check LVDS_DROPBACK/half speed
            period = nv_period;
            // Initialize sync offset to maximum
            offset = MAX_1010_DT_OFFSET;
            DT_flag = 2;
        }
        else
        {
        // doing TIN, so setup to what was asked for (if values are out of our
        // range, the next test for async will reset offset and period).
    
            // limit period to max set in NVRAM
            period = (DeviceExtension->tin_rec_period < nv_period)
                            ? nv_period : DeviceExtension->tin_rec_period;
            offset = DeviceExtension->tin_rec_offset;
            DT_flag = DeviceExtension->tin_rec_DT;
        }

        // see if we are currently in async (this means params not accepted)
        if ( !(DeviceExtension->dxp[target] & PERIOD_MASK) )
        {
            // set period and offset for async
            period = 25;    // use non-zero period when negotiating async
            offset = 0;
        }
    
        // check for LVDS_DROPBACK and half speed mode
        period = set_sync_speed( DeviceExtension, period);
        // can do Ultra160 only if wide
        if (period == 0x09 && !wide)
            period = 0x0A;
        // can do Ultra160 only if DT on
        if ( period == 0x09 && DT_flag != 2)
            period = 0x0A;
        // if not doing Ultra160 don't use DT
        if ( period > 0x09 )
            DT_flag = 0;
        // if not doing DT, use ST offset
        if ( !DT_flag )
            offset = MAX_1010_ST_OFFSET;
    
        // move wide negotiation bytes and set count in descriptor
        length = (ULONG)(sizeof ppr_msg);

        if ( negot_type == SETUP_PPR_TIN )
        // setup PPR TIN response in msgOutBuf
        {
            StorPortMoveMemory( tmpBuf, ppr_msg, length);
            tmpBuf[3] = period;
            tmpBuf[5] = offset;
            if (wide)       // set to 16 bits if doing wide
                tmpBuf[6] = 1;
            tmpBuf[7] = DT_flag;
            msgPtr = svdtPtr->msgOutBuf;
            for ( i = 0; i < length; i++)
                StorPortWriteRegisterUchar( DeviceExtension, msgPtr++,
                                            tmpBuf[i]);
            StorPortWriteRegisterUlong( DeviceExtension,
                                        &svdtPtr->msgOutBufDescriptor.count,
                                        length);
        }

        // setup NegotMsg buffer for PPR if doing either PPR_FIRST or
        // PPR_TIN (do for PPR_TIN also since PPR params may have changed)
        msgPtr = bufPtr[target].Buf;
        StorPortMoveMemory( &msgPtr[1], ppr_msg, length);
        negotDescPtr[target].count = length + 1;  // add 1, identify byte
        msgPtr[4] = period;
        msgPtr[6] = offset;
        if (wide)       // set to 16 bits if doing wide
            msgPtr[7] = 1;
        msgPtr[8] = DT_flag;

        return;
    } // end of PPR negotiations

    else
    {
        // check for setting up wide negotiations
        if ( negot_type <= SETUP_WIDE_TIN )
        {
            // move wide negotiation bytes and set count in descriptor
            length = (ULONG)(sizeof narrow_msg);
    
            if ( negot_type == SETUP_WIDE_TIN)
            // setup wide TIN response in msgOutBuf
            {
                StorPortMoveMemory( tmpBuf, narrow_msg, length);
                if (wide)       // set to 16 bits if doing wide
                    tmpBuf[3] = 1;
                msgPtr = svdtPtr->msgOutBuf;
                for ( i = 0; i < length; i++)
                    StorPortWriteRegisterUchar( DeviceExtension, msgPtr++,
                                                tmpBuf[i]);
                StorPortWriteRegisterUlong( DeviceExtension,
                                            &svdtPtr->msgOutBufDescriptor.count,
                                            length);
                StorPortWriteRegisterUlong( DeviceExtension,
                                            &svdtPtr->msgOutBufDescriptor.paddr,
                                            DeviceExtension->msgOutDesc.paddr);
            }
    
            // setup NegotMsg buffer for wide if doing either WIDE_FIRST or
            // WIDE_TIN (do for WIDE_TIN also since wide param may have changed)
            StorPortMoveMemory( &bufPtr[target].Buf[1], narrow_msg, length);
            negotDescPtr[target].count = length + 1;  // add 1, identify byte
            if (wide)       // set to 16 bits if doing wide
                bufPtr[target].Buf[4] = 1;
    
            return;
        }
    
        // setup for synchronous negotiations
    
        // check if doing only sync and sync has been rejected
        if ( (negot_type == SETUP_SYNC_FIRST) &&
                (DeviceExtension->LuFlags[target] & LF_SYNC_NEG_REJECT) )
        {
            // don't do any more sync negotiations, just send identify byte
            negotDescPtr[target].count = 1;  // 1 for just identify byte
            return;
        }
    
        // if not doing TIN, setup for our maximums
        if ( negot_type != SETUP_SYNC_TIN )
        {
            // set sync period to NVRAM setting, check LVDS_DROPBACK/half speed
            period = set_sync_speed( DeviceExtension, nv_period);
            // can't use period of 9 for SDTR
            if ( period == 0x09 )
                period = 0x0A;
            // Initialize sync offset to maximum
            offset = MAX_1010_ST_OFFSET;
        }
        else
        {
        // doing TIN, so setup to what was asked for (if values are out of our
        // range, the next test for async will reset offset and period).
    
            // limit period to max set in NVRAM
            period = (DeviceExtension->tin_rec_period < nv_period)
                            ? nv_period : DeviceExtension->tin_rec_period;
            // check LVDS and half speed modes
            period = set_sync_speed( DeviceExtension, period);
            offset = DeviceExtension->tin_rec_offset;
        }
    
        // see if we are currently in async (this means params not accepted)
        if ( !(DeviceExtension->dxp[target] & PERIOD_MASK) )
        {
            // set period and offset for async
            period = 25;    // use non-zero period when negotiating async
            offset = 0;
        }
    
        // set sync negotiation message byte length
        length = (ULONG)(sizeof async_msg);
    
        ptr = 0;            // use this as a flag to patch period and offset
        if ( negot_type == SETUP_SYNC_CONT )
        {
            // setup continuing negotiations in NegotMsg buffer, but beware that
            // the length needs to be placed in the svdt msgOutBufDesciptor
            // because the descriptor in the NegotMsgBufDesc array has already
            // been copied into the svdt.
            StorPortMoveMemory( &bufPtr[target], async_msg, length);
            StorPortWriteRegisterUlong( DeviceExtension,
                                        &svdtPtr->msgOutBufDescriptor.count,
                                        length);
            ptr = 3;
        }
    
        if ( negot_type == SETUP_SYNC_FIRST || ( negot_type == SETUP_SYNC_TIN &&
            (DeviceExtension->LuFlags[target] & LF_WIDE_NEG_FAILED) ) )
        {
            // setup NegotMsg buffer if doing SYNC_FIRST or if doing SYNC_TIN and
            // wide negotiation has failed.  (need to do this for SYNC_TIN since
            // the sync params may have changed)
            StorPortMoveMemory( &bufPtr[target].Buf[1], async_msg, length);
            negotDescPtr[target].count = length + 1;  // add 1 for identify byte
            ptr = 4;    // point to 4th element for patching
        }
    
        // if necessary, patch proper period and offset into buffer
        if ( ptr )
        {
            bufPtr[target].Buf[ptr++] = period;
            bufPtr[target].Buf[ptr] = offset;
        }
    
        if ( negot_type == SETUP_SYNC_TIN )
        // setup sync TIN response in msgOutBuf
        {
            StorPortMoveMemory( tmpBuf, async_msg, length);
            tmpBuf[3] = period;
            tmpBuf[4] = offset;
            msgPtr = svdtPtr->msgOutBuf;
            for ( i = 0; i < length; i++)
                StorPortWriteRegisterUchar( DeviceExtension, msgPtr++,
                                            tmpBuf[i]);
            StorPortWriteRegisterUlong( DeviceExtension, 
                                        &svdtPtr->msgOutBufDescriptor.count,
                                        length);
            StorPortWriteRegisterUlong( DeviceExtension, 
                                        &svdtPtr->msgOutBufDescriptor.paddr,
                                        DeviceExtension->msgOutDesc.paddr);
        }
    } // end of WDTR/SDTR negotiations

}  // SetupNegotBuf


VOID
StartAbortResetRequest(
    PSCSI_REQUEST_BLOCK Srb,
    PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This procedure starts an abort, terminate, bus device reset or LUN reset
    request using the same start queue method.  This is a subset of the
    StartSCSIRequest procedure, but is cloned here to keep the overhead for
    normal I/O processing as low as possible.

Arguments:

    Srb - Pointer to the request to be started.

    DeviceExtension - Pointer to the device extension for this adapter.

Return Value:

    None

--*/

{
    ULONG Length, srbFlgs;
    ULONG svdtPAdd;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
    ULONG index, svdtMove;
    UCHAR function, target, lun, msg0, msg1, count;
    UCHAR turCdb[] = { 0, 0, 0, 0, 0, 0 };

    // make sure start queue isn't full
    index = DeviceExtension->ioStartQIndex;
    if (DeviceExtension->ioStartQueue[index].svdtPhysSem & SVDT_SEM_MASK)
    {
        DebugPrint((3,"StartAbortResetRequest: Start Queue Full... \n"));
        Srb->SrbStatus = SRB_STATUS_BUSY;
        // notify port driver of adapter busy status
        // resume IO's after 20 requests have completed
        StorPortBusy( DeviceExtension, 20);
        StorPortNotification( RequestComplete, DeviceExtension, Srb );
        return;
    }

    // get local variables
    lun = msg0 = 0;         // initialize just in case
    count = 0;
    function = Srb->Function;
    srbFlgs = Srb->SrbFlags;
    target = Srb->TargetId;
    svdtMove = MEMORY_MOVE_CMD +
                    FIELD_OFFSET(SVARS_DESCRIPTOR_TABLE, iovList);
    if (function != SRB_FUNCTION_RESET_DEVICE)
        lun = Srb->Lun;

    SrbExtension->SrbExtFlags = 0;  // clear negotiation flags

    // align svdt on quadword boundary, if necessary
    svdtPtr = &SrbExtension->svarsDescriptorTable;
    if ( (ULONG_PTR)(svdtPtr) & 0x00000007 )
        svdtPtr = (PSVARS_DESCRIPTOR_TABLE)((ULONG_PTR)svdtPtr + 4);
    SrbExtension->svdt = svdtPtr;

    // get physical address of svdt in system memory
    svdtPAdd = StorPortConvertPhysicalAddressToUlong(
                        StorPortGetPhysicalAddress(DeviceExtension, NULL, 
                            (PVOID)svdtPtr, &Length));

    // context and deviceDescriptor entries need no physical addresses
#ifdef _WIN64
    // context is physical address of svdt, save Srb in svdt
    svdtPtr->context = svdtPAdd;
    svdtPtr->Srb = Srb;
#else
    // context is Srb address
    svdtPtr->context = (ULONG)Srb;
#endif
    svdtPtr->sysSvdtPhys = svdtPAdd;        // phys ptr to svdt is sys mem
    svdtPtr->svdtMoveCmd = svdtMove;        // mem move command + len
    // copy CDB into svdt
    StorPortMoveMemory(svdtPtr->Cdb, turCdb, 6);
    // copy table descriptor templates into svdt
    StorPortMoveMemory( &svdtPtr->deviceDescriptor,
                        &DeviceExtension->deviceDesc, 24);
    // update table descriptor entries
    svdtPtr->deviceDescriptor.count = DeviceExtension->dxp[target];
    svdtPtr->cmdBufDescriptor.count = 6;   // set CDB length

    // Clear auto request sense flag
    SrbExtension->autoReqSns = 0;
    
    // check for LUN reset
    if ( function == SRB_FUNCTION_RESET_LOGICAL_UNIT )
    {
        msg0 = (UCHAR)SCSIMESS_IDENTIFY_WITH_DISCON + lun;
        msg1 = SCSIMESS_LOGICAL_UNIT_RESET;
        count = 2;
    }
    else
    {
        // set proper message byte for this function
        switch (function)
        {
            case SRB_FUNCTION_ABORT_COMMAND:
                // see if trying to abort a tagged command
                if (srbFlgs & SRB_FLAGS_QUEUE_ACTION_ENABLE)
                    msg0 = SCSIMESS_ABORT_WITH_TAG;
                else
                    msg0 = SCSIMESS_ABORT;
                break;

            case SRB_FUNCTION_TERMINATE_IO:
                msg0 = SCSIMESS_TERMINATE_IO_PROCESS;
                break;

            case SRB_FUNCTION_RESET_DEVICE:
                msg0 = SCSIMESS_BUS_DEVICE_RESET;
                break;
        }
        count = 1;
    }

    svdtPtr->msgOutBuf[0] = msg0;
    if ( count == 2)
        svdtPtr->msgOutBuf[1] = msg1;

    // indicate message length
    svdtPtr->msgOutBufDescriptor.count = count;

    DebugPrint((3, "LsiU3(%2x) StartAbortResetRequest: Starting request for Id=%2x\n",
        DeviceExtension->SIOPRegisterBase,
        Srb->TargetId ));

    // insert this command into the start queue

    DeviceExtension->ioStartQueue[index].svdtMoveCmd = svdtMove;
    // write of svdtPhysSem must be done last
    DeviceExtension->ioStartQueue[index].svdtPhysSem = svdtPAdd | SVDT_SEM_START;

    WRITE_SIOP_UCHAR(ISTAT0, ISTAT_SIGP);

    index++;
    if (index == START_Q_DEPTH)
    {
        index = 0;
    }
    DeviceExtension->ioStartQIndex = index;

} // StartAbortResetRequest


UCHAR
StartNegotiations(
    PHW_DEVICE_EXTENSION DeviceExtension,
    PSCSI_REQUEST_BLOCK Srb,
    UCHAR MessageCount,
    _In_ BOOLEAN hostSvdt
    )

/*++

Routine Description:

    This procedure checks to see if negotiations need to be done and
    sets up the message buffer if they are.

Arguments:

    DeviceExtension - Pointer to the device extension for this adapter.
    
    Srb - Pointer to I/O request

    MessageCount - Message count variable for msgOut bytes
    
    hostSvdt - TRUE if message is built in host memory, FALSE if message
               is to be copied to Scripts RAM

Return Value:

    MessageCount - Updated value of message count

--*/

{
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PUCHAR msgOutBufPtr, msgOutBufSR;
    USHORT luFlgs, hbaCap;
    UCHAR period, offset, target, width, wide_bits, initCount, i;
    BOOLEAN do_PPR, PPRwideSync;
    BOOLEAN dis_sync = FALSE;
    BOOLEAN OS_ReqSns = FALSE;
    BOOLEAN wide_OSRS = FALSE;
    BOOLEAN sync_OSRS = FALSE;
    BOOLEAN ppr_OSRS = FALSE;

    // Setup local variables

    initCount = MessageCount;
    hbaCap = DeviceExtension->hbaCapability;
    target = Srb->TargetId;
    luFlgs = DeviceExtension->LuFlags[target];
    wide_bits = DeviceExtension->DeviceTable[target].WideDataBits;
    // set sync period to NVRAM setting
    period = (UCHAR)DeviceExtension->DeviceTable[target].SyncPeriodNs / 4;
    // modify sync speed for LVDS or half speed mode
    period = set_sync_speed( DeviceExtension, period);
    // see if we should do PPR negotiations (never do for async negotiations,
    // if in narrow mode, or if period not 80MB/s)
    do_PPR = !(luFlgs & LF_PPR_NEG_REJECT) && (luFlgs & LF_ASYNC_NEG_DONE) &&
             (wide_bits == WIDE_16) && (period == 9);

    // set message out buffer pointer to SrbExtension svdt
    // if needed in local svdt, we'll move it later
    msgOutBufPtr = SrbExtension->svdt->msgOutBuf;

    // are we doing an OS issued Request Sense command?
    // if so, setup flags for PPR, or wide and sync
    if ( Srb->Cdb[0] == SCSIOP_REQUEST_SENSE )
    {
        OS_ReqSns = TRUE;
        if ( do_PPR )
        {
            ppr_OSRS = TRUE;
        }
        else
        {
            if ( !(luFlgs & LF_WIDE_NEG_FAILED) &&
                 !(SrbExtension->SrbExtFlags & LF_OSRS_WIDE_DONE) )
                wide_OSRS = TRUE;
            if ( !(luFlgs & LF_SYNC_NEG_REJECT) )
                sync_OSRS = TRUE;
        }
    }

    // If async, sync, and wide have been done, clear negotiations needed flag
    // and return
    if ( (luFlgs & LF_WIDE_NEG_DONE) && (luFlgs & LF_ASYNC_NEG_DONE) &&
            (luFlgs & LF_SYNC_NEG_DONE) && !OS_ReqSns )
    {
        DeviceExtension->LuFlags[target] &= ~LF_NEG_NEEDED;
        return (MessageCount);
    }

    // see if force sync flag is set and async negotiations have been done
    if (!((DeviceExtension->DeviceFlags & DFLAGS_FORCE_SYNC) &&
        (luFlgs & LF_ASYNC_NEG_DONE)) )
    {
        // if not, set local disable wide/sync transfer flag
        if ( Srb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER )
        {
            SrbExtension->SrbExtFlags |= LF_DISABLE_SYNC;
            dis_sync = TRUE;
        }
    }

    // check for doing PPR negotiations
    if ( do_PPR )
    {
        // make flag for PPR wide/sync done
        PPRwideSync = ((luFlgs & (LF_WIDE_NEG_DONE | LF_SYNC_NEG_DONE)) ==
                       (LF_WIDE_NEG_DONE | LF_SYNC_NEG_DONE));
        // Decide if PPR negotiations are needed
        if ( (!OS_ReqSns && ((!(luFlgs & LF_ASYNC_NEG_DONE) && dis_sync) ||
             (!PPRwideSync && !dis_sync))) || ppr_OSRS )
        {
            // fill in the parameters for PPR extended message
            msgOutBufPtr[MessageCount++] = SCSIMESS_EXTENDED_MESSAGE;
            msgOutBufPtr[MessageCount++] = 6;       // 6 message bytes
            msgOutBufPtr[MessageCount++] = 4;       // PPR message

            width = 0;      // default to narrow

            // Initialize sync offset to maximum for ST
            offset = MAX_1010_ST_OFFSET;
    
            // setup for async/sync and narrow/wide depending on dis_sync flag
            // or current transfer parameters if doing OS issued request sense
            if ( OS_ReqSns )
            {
                if ( !(DeviceExtension->dxp[target] & PERIOD_MASK) )
                    offset = 0;     // currently in async, set offset for async
                // see if we are already in wide, if so negot wide
                if ( DeviceExtension->dxp[target] & ENABLE_WIDE )
                    width = 1;
            }
            else    // normal I/O, set pending flags & check dis_sync flag
            {
                // indicate PPR negotiation is pending
                DeviceExtension->LuFlags[target] |= LF_PPR_NEG_PENDING;
                // set PPR pending flag for this I/O
                SrbExtension->SrbExtFlags |= LF_PPR_NEG_PENDING;
                // check for disable sync or sync rate of 0
                if ( dis_sync || period == 0 )
                {
                    period = 25;
                    offset = 0;
                }
                // setup for narrow/disable_sync or wide
                if ( !dis_sync && (wide_bits != WIDE_NONE) )
                    width = 1;
            }

            // 80 MT/sec speed requires wide, if narrow set to 40MT/sec
            if ( !width && (period == 9) )
                period = 10;

            // if 80MT/sec & offset not 0, set offset to DT maximum
            if ( period == 9 && offset )
                offset = MAX_1010_DT_OFFSET;

            msgOutBufPtr[MessageCount++] = period;
            msgOutBufPtr[MessageCount++] = 0;       // reserved byte
            msgOutBufPtr[MessageCount++] = offset;
            msgOutBufPtr[MessageCount++] = width;

            // set DT transfers only for 80MT/sec
            msgOutBufPtr[MessageCount++] = (period == 9) ? 2 : 0;
        }
    }
    else    // not doing PPR, do WDTR and SDTR
    {
        // Decide if narrow/wide negotiations are needed
        if ( (!OS_ReqSns && (((!(luFlgs & LF_NARROW_NEG_DONE) && dis_sync) ||
             (!(luFlgs & LF_WIDE_NEG_DONE) && !dis_sync)) &&
             !(luFlgs & LF_WIDE_NEG_PENDING))) || wide_OSRS )
        {
            // fill in the parameters for WDTR extended message
            msgOutBufPtr[MessageCount++] = SCSIMESS_EXTENDED_MESSAGE;
            msgOutBufPtr[MessageCount++] = 2;       // 2 message bytes
            msgOutBufPtr[MessageCount++] = SCSIMESS_WIDE_DATA_REQUEST;
    
            width = 0;          // initialize to narrow
    
            if ( OS_ReqSns )    // doing OS issued request sense
            {
                // set flag to indicate wide negotiation done
                SrbExtension->SrbExtFlags |= LF_OSRS_WIDE_DONE;
                // see if we are already in wide, if so negot wide
                if ( DeviceExtension->dxp[target] & ENABLE_WIDE )
                    width = 1;
            }
            else        // normal negotiations
            {
                // indicate narrow or wide negotiation is pending
                DeviceExtension->LuFlags[target] |= LF_WIDE_NEG_PENDING;
                // set narrow or wide pending flag for this I/O
                SrbExtension->SrbExtFlags |= LF_WIDE_NEG_PENDING;
    
                // setup for narrow or wide depending on dis_sync flag & NVRAM
                if ( dis_sync || wide_bits == WIDE_NONE )
                {
                    DebugPrint((1, "LsiU3(%2x):  NarrowNegotiation Requested - Target: %x\n",
                        DeviceExtension->SIOPRegisterBase, target));
                }
                else if (wide_bits == WIDE_16)
                {
                    width = 1;
                    DebugPrint((1, "LsiU3(%2x):  WideNegotiation Requested - Target: %x\n",
                        DeviceExtension->SIOPRegisterBase, target));
                }
            }
    
            msgOutBufPtr[MessageCount++] = width;
        }
        
        // else, check on async/sync negotiations
        else if ( (((!(luFlgs & LF_ASYNC_NEG_DONE) && dis_sync) ||
                  (!(luFlgs & LF_SYNC_NEG_DONE) && !dis_sync)) &&
                  !(luFlgs & (LF_WIDE_NEG_PENDING + LF_SYNC_NEG_PENDING))) ||
                  sync_OSRS )
        {
            // fill in the parameters for SDTR extended message
            msgOutBufPtr[MessageCount++] = SCSIMESS_EXTENDED_MESSAGE;
            msgOutBufPtr[MessageCount++] = 3;       // 3 message bytes
            msgOutBufPtr[MessageCount++] = SCSIMESS_SYNCHRONOUS_DATA_REQ;
    
            // if period is 9, set it to 10 (can't use 9 with SDTR)
            if ( period == 9 )
                period = 10;
    
            // Initialize sync offset to ST maximum
            offset = MAX_1010_ST_OFFSET;
    
            // setup for async or sync depending on dis_sync flag
            // or current transfer parameters if doing OS issued request sense
            if ( OS_ReqSns )
            {
                if ( !(DeviceExtension->dxp[target] & PERIOD_MASK) )
                    offset = 0;     // currently in async, set offset for async
            }
            else    // normal I/O, set pending flags & check dis_sync flag
            {
                // indicate async or sync negotiation is pending
                DeviceExtension->LuFlags[target] |= LF_SYNC_NEG_PENDING;
                // set async or sync pending flag for this I/O
                SrbExtension->SrbExtFlags |= LF_SYNC_NEG_PENDING;
                // check for disable_sync or async NVRAM setting
                if ( dis_sync || period == 0 )
                {
                    period = 25;        // some devices need a non-zero period
                    offset = 0;
                    DebugPrint((1, "LsiU3(%2x):  AsynchronousNegotiation Requested - Target: %x\n",
                        DeviceExtension->SIOPRegisterBase, target));
                }
                else
                {
                    DebugPrint((1, "LsiU3(%2x):  SynchronousNegotiation Requested - Target: %x\n",
                        DeviceExtension->SIOPRegisterBase, target));
                }
            }
    
            msgOutBufPtr[MessageCount++] = period;
            msgOutBufPtr[MessageCount++] = offset;
        }
    } // end of if do_PPR

    // move buffer to Scripts RAM if hostSvdt is false
    if ( !hostSvdt && (MessageCount > initCount) )
    {
        msgOutBufPtr = &SrbExtension->svdt->msgOutBuf[initCount];
        msgOutBufSR = &DeviceExtension->localSvdt->msgOutBuf[initCount];
        for ( i = initCount; i < MessageCount; i++)
            StorPortWriteRegisterUchar( DeviceExtension, msgOutBufSR++,
                                        *(msgOutBufPtr++));
    }
    
    return (MessageCount);
} // StartNegotiations


VOID
StartNVConfigRequest(
    PSCSI_REQUEST_BLOCK Srb,
    PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This procedure puts an entry on the start queue to perform an NVConfig
    IOCTL task (either read or write NVRAM).  ProcessNVConfigIoctl is the
    companion routine that performs the task.

Arguments:

    Srb - Pointer to the request to be started.

    DeviceExtension - Pointer to the device extension for this adapter.

Return Value:

    None

--*/

{
    ULONG Length;
    ULONG svdtPAdd;
    ULONG index, svdtMove;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;

    // make sure start queue isn't full
    index = DeviceExtension->ioStartQIndex;
    if (DeviceExtension->ioStartQueue[index].svdtPhysSem & SVDT_SEM_MASK)
    {
        DebugPrint((3,"StartNVConfigRequest: Start Queue Full... \n"));
        Srb->SrbStatus = SRB_STATUS_BUSY;
        // notify port driver of adapter busy status
        // resume IO's after 20 requests have completed
        StorPortBusy( DeviceExtension, 20);
        StorPortNotification( RequestComplete, DeviceExtension, Srb );
        return;
    }
    svdtMove = MEMORY_MOVE_CMD +
                    FIELD_OFFSET(SVARS_DESCRIPTOR_TABLE, iovList);

    // align svdt on quadword boundary, if necessary
    svdtPtr = &SrbExtension->svarsDescriptorTable;
    if ( (ULONG_PTR)(svdtPtr) & 0x00000007 )
        svdtPtr = (PSVARS_DESCRIPTOR_TABLE)((ULONG_PTR)svdtPtr + 4);
    SrbExtension->svdt = svdtPtr;

    // get physical address of svdt in system memory
    svdtPAdd = StorPortConvertPhysicalAddressToUlong(
                        StorPortGetPhysicalAddress(DeviceExtension, NULL, 
                            (PVOID)svdtPtr, &Length));

#ifdef _WIN64
    // context is physical address of svdt, save Srb in svdt
    svdtPtr->context = svdtPAdd;
    svdtPtr->Srb = Srb;
#else
    // context is Srb address
    svdtPtr->context = (ULONG)Srb;
#endif

    DebugPrint((3, "LsiU3(%2x) StartNVConfigRequest: Starting NVConfig IOCTL request\n",
        DeviceExtension->SIOPRegisterBase ));

    // insert this command into the start queue

    index = DeviceExtension->ioStartQIndex;
    DeviceExtension->ioStartQueue[index].svdtMoveCmd = svdtMove;
    // write of svdtPhysSem must be done last
    DeviceExtension->ioStartQueue[index].svdtPhysSem = svdtPAdd | SVDT_SEM_NVCONFIG;

    WRITE_SIOP_UCHAR(ISTAT0, ISTAT_SIGP);

    index++;
    if (index == START_Q_DEPTH)
    {
        index = 0;
    }
    DeviceExtension->ioStartQIndex = index;

} // StartNVConfigRequest


VOID
StartSCSIRequest(
    PSCSI_REQUEST_BLOCK Srb,
    PHW_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This procedure starts a request if possible, and also checks if
    negotiations need to be started.  This routine is called by StartIo and
    most of the I/O processing has already been done previously in BuildIo.

Arguments:

    Srb - Pointer to the request to be started.

    DeviceExtension - Pointer to the device extension for this adapter.

Return Value:

    None

--*/

{
    UCHAR qTag, trackEntry;
    ULONG svdtPAdd, index, svdtMove, tagIndex, trackFIFO;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
    PIO_TRACK_ENTRY pEntry;

    // check for ResetActive
    if ( DeviceExtension->ResetActive )
    {
        // return the I/O with Busy status
        DebugPrint((3,"StartSCSIRequest: ResetActive, returning Busy... \n"));
        Srb->SrbStatus = SRB_STATUS_BUSY;
        StorPortNotification( RequestComplete, DeviceExtension, Srb );
        return;
    }

    // make sure start queue isn't full
    index = DeviceExtension->ioStartQIndex;
    if (DeviceExtension->ioStartQueue[index].svdtPhysSem & SVDT_SEM_MASK)
    {
        DebugPrint((3,"StartSCSIRequest: Start Queue Full... \n"));
        // return the I/O with Busy status
        Srb->SrbStatus = SRB_STATUS_BUSY;
        // notify port driver of adapter busy status
        // resume IO's after 20 requests have completed
        StorPortBusy( DeviceExtension, 20);
        StorPortNotification( RequestComplete, DeviceExtension, Srb );
        return;
    }

    // get svdtMove command and svdt physical address from svdt
    svdtPtr = SrbExtension->svdt;
    svdtMove = svdtPtr->svdtMoveCmd;
    svdtPAdd = svdtPtr->sysSvdtPhys;

    // get next free I/O tracking entry index
    trackFIFO = DeviceExtension->TrackFree;
    trackEntry = DeviceExtension->IoTrackFIFO[trackFIFO++];
    // check for index wrap
    if ( trackFIFO == START_Q_DEPTH )
        trackFIFO = 0;
    // save index
    DeviceExtension->TrackFree = trackFIFO;
    // put this I/O into the tracking array
    pEntry = &DeviceExtension->IoTrackArray[trackEntry];
#if DBG
    if ( pEntry->Srb )
        DebugPrint((3, "LsiU3(%2x) SRB is not NULL - SRB: %x\n",
            DeviceExtension->SIOPRegisterBase, pEntry->Srb));

#endif
    pEntry->Srb = Srb;
    pEntry->target = Srb->TargetId;
    pEntry->lun = Srb->Lun;
    // save entry index in SrbExtension
    SrbExtension->trackEntry = trackEntry;

    // assign driver determined queue tag value (if queue action enabled)
    if ( Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE )
    {
        // get next free queue tag value
        tagIndex = DeviceExtension->QTagFree;
        qTag = DeviceExtension->QTagFIFO[tagIndex++];
        // check for index wrap
        if ( tagIndex == START_Q_DEPTH )
            tagIndex = 0;
        // save index
        DeviceExtension->QTagFree = tagIndex;
        // put queue tag value in message bytes and save in SrbExtension
        svdtPtr->msgOutBuf[2] = qTag;
        // put nexus entry address into svdt
        svdtPtr->nexusEntryPhys = DeviceExtension->ITQnexusTablePhys +
            (qTag * 8);
        // save queue tag in tracking array
        pEntry->queueTag = qTag;
    }

    // insert this command into the start queue
    DeviceExtension->ioStartQueue[index].svdtMoveCmd = svdtMove;
    // NOTE: write of address/semaphore must be done last
    DeviceExtension->ioStartQueue[index].svdtPhysSem = svdtPAdd | SVDT_SEM_START;

    WRITE_SIOP_UCHAR(ISTAT0, ISTAT_SIGP);

    index++;
    if (index == START_Q_DEPTH)
    {
        index = 0;
    }
    DeviceExtension->ioStartQIndex = index;

} // StartSCSIRequest


VOID
StartSIOP(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG ScriptPhysAddr                              
    )
/*++

Routine Description:

    This routine starts the scripts at the instruction whose physical
    address is passed to it.

Arguments:

    DeviceExtension - Supplies the device Extension for the SCSI bus adapter.
    ScriptPhysAddr  - Supplies the address of the script routine to start.

Return Value:

    None

--*/

{
    WRITE_SIOP_ULONG( DSP, ScriptPhysAddr);

} // StartSIOP


VOID 
doneQRemove(
    PHW_DEVICE_EXTENSION DeviceExtension,
    UCHAR IntStatus
    )
/*++

Routine Description:

    This function removes a descriptor table pointer from the completion queue
    and processes the completed command.

Arguments:

    DeviceExtension - Pointer to the device extension for this adapter.
    
    IntStatus - Value of ISTAT0 when ISR was entered.

Return Value:

    none

--*/

{
    ULONG index, statLen;
#ifdef _WIN64
    STOR_PHYSICAL_ADDRESS svdtPhys;
    PSVARS_DESCRIPTOR_TABLE svdtPtr;
#else
    PSCSI_REQUEST_BLOCK Srb;
#endif

    // NOTE: Do NOT clear IntFly until processing all entries on the done
    // queue.  Scripts test IntFly and will post one entry without checking
    // that the entry is empty.

    // Load local copy of ioDoneQIndex
    index = DeviceExtension->ioDoneQIndex;

    // we loop on completing commands until the done_queue is empty
    do {

#ifdef _WIN64
      while ((svdtPhys.LowPart = DeviceExtension->ioDoneQueue[index].context) != 0) {

        // get virtual svdt address
        svdtPhys.HighPart = 0;
        svdtPtr = StorPortGetVirtualAddress(DeviceExtension, svdtPhys);
        if ( svdtPtr )
        {   
            // get active request from svdt and complete the command
            DeviceExtension->ActiveRequest = svdtPtr->Srb;
            // get the status/true transfer length from the done queue entry
            statLen = DeviceExtension->ioDoneQueue[index].statXferLen;
            ProcessCommandComplete(DeviceExtension, statLen);
        }
        else
        {
            // bad svdt virtual address, log controller error, don't complete
            StorPortLogError(DeviceExtension, NULL, 0, 0, 0,
                             SP_INTERNAL_ADAPTER_ERROR, 0);
        }
#else
      while ((Srb = (PSCSI_REQUEST_BLOCK)DeviceExtension->ioDoneQueue[index].context) != NULL) {

        // get the status/true transfer length from the done queue entry
        statLen = DeviceExtension->ioDoneQueue[index].statXferLen;

        // set active pointers
        DeviceExtension->ActiveRequest = Srb;

        ProcessCommandComplete(DeviceExtension, statLen);
#endif

        // clear queue entry
        DeviceExtension->ioDoneQueue[index].context = 0;

        index++;
        if (index == DONE_Q_DEPTH)
        {
          index = 0;
        }

      } //while

      // clear INT FLY after removing all items from the queue
      WRITE_SIOP_UCHAR(ISTAT0, IntStatus);
        
    // check the current queue entry to see if scripts posted to it after
    // we were done checking.  Need to do this in case scripts post an
    // entry just before we clear IntFly.  If we don't check, we could
    // leave an entry sitting on the queue with no IntFly.
    } while (DeviceExtension->ioDoneQueue[index].context);

    // Save local copy of index into ioDoneQIndex
    DeviceExtension->ioDoneQIndex = index;

} // doneQRemove


VOID
UpdateStartQDesc(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine updates the device descriptors for all oustanding
    commands in the start queue for this device id.  This is necessary
    to make sure commands queued after a command which does negotiations
    all have the proper dxp data.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension for the
                      interrupting adapter.
    
    Srb - Pointer to the current Scsi request block

Return Value:

    None

--*/

{
    UCHAR id, target;
    ULONG i, svdtPhys, new_dxp;
    PSVARS_DESCRIPTOR_TABLE svdt;
    STOR_PHYSICAL_ADDRESS svdtScsiPhys;

    // setup local variables
    target = Srb->TargetId;
    new_dxp = DeviceExtension->dxp[target];
    svdtScsiPhys.HighPart = 0;  // setup for getting virtual addresses

    // search through start queue looking at all active I/O's which
    // are for this device id
    for (i = 0; i < START_Q_DEPTH; i++)
    {
        // get svdt physical address + semaphore, check for start semaphore
        svdtPhys = DeviceExtension->ioStartQueue[i].svdtPhysSem;
        if ( svdtPhys & SVDT_SEM_START )
        {
            // mask off semaphore, store in scsi_physical_address structure
            svdtScsiPhys.LowPart = svdtPhys & SVDT_PHYS_MASK;

            // convert physical svdt addres to virtual
            svdt = (PSVARS_DESCRIPTOR_TABLE)
                StorPortGetVirtualAddress(DeviceExtension, svdtScsiPhys);

            // check for valid virtual address
            if ( svdt )
            {
                // extract device id from current dxp
                id = (UCHAR)(svdt->deviceDescriptor.count >> 16);

                // check if I/O is for this target
                if ( id == target )
                {
                    // update device descriptor with new dxp parameters
                    svdt->deviceDescriptor.count = new_dxp;
                }
            }
        }
    } // for

} // UpdateStartQDesc


// start of NVRAM useage code
// NVRAM_CODE
/*  The following procedures are used to simplify reading of the nvram code.
 *
 *  data_mask - This is the GPREG bit used as a data line.
 *  clock_mask - This is the GPREG bit used as a clock line.
 */

void data_output( PHW_DEVICE_EXTENSION DeviceExtension)
{
    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)(READ_SIOP_UCHAR( GPCNTL )
                                        & (~DeviceExtension->data_mask)) )
}

void data_input( PHW_DEVICE_EXTENSION DeviceExtension)
{
    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)(READ_SIOP_UCHAR( GPCNTL )
                                        | DeviceExtension->data_mask) )
}

void set_data( PHW_DEVICE_EXTENSION DeviceExtension)
{
    WRITE_SIOP_UCHAR( GPREG, (UCHAR)(READ_SIOP_UCHAR( GPREG )
                                        | DeviceExtension->data_mask) )
}

void reset_data( PHW_DEVICE_EXTENSION DeviceExtension)
{
    WRITE_SIOP_UCHAR( GPREG, (UCHAR)(READ_SIOP_UCHAR( GPREG )
                                        & (~DeviceExtension->data_mask)) )
}

void set_clock( PHW_DEVICE_EXTENSION DeviceExtension)
{
    WRITE_SIOP_UCHAR( GPREG, (UCHAR)(READ_SIOP_UCHAR( GPREG )
                                        | DeviceExtension->clock_mask) )
}

void reset_clock( PHW_DEVICE_EXTENSION DeviceExtension)
{
    WRITE_SIOP_UCHAR( GPREG, (UCHAR)(READ_SIOP_UCHAR( GPREG )
                                        & (~DeviceExtension->clock_mask)) )
}

/*  BOOLEAN  NvmDetect( PHW_DEVICE_EXTENSION DeviceExtension )
 *
 *  Input:
 *
 *      DeviceExtension - Pointer to the device extension for this adapter.
 *
 *  Returns:
 *
 *      BOOLEAN - SUCCESS if NVM was detected
 *                FAILURE if NVM was not detected
 *
 *  Purpose:
 *
 *      This routine is used to test the adapter to see if NVM is installed on
 *      the GPIO pins of the 1010 chip.
 */


BOOLEAN  NvmDetect( PHW_DEVICE_EXTENSION DeviceExtension )
{
    UINT  flag;
    UINT  retries;
    UCHAR dmask = DeviceExtension->data_mask;
    UCHAR cmask = DeviceExtension->clock_mask;

    /*  Check for no NVM access flag (set via PCI Subsystem ID).
     *  If set, return with FAILURE.
     */

    if (DeviceExtension->DeviceFlags & DFLAGS_NO_NVM_ACCESS)
        return( FAILURE );

    /*  Turn the data & clock lines into outputs and turn off H/W LED,
     *  then send a stop signal to the I2C chip to reset it to a known state.
     */

    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)(READ_SIOP_UCHAR(GPCNTL) &
                 (~(dmask | cmask | GPCNTL_LED_CNTL))) );
    NvmSendStop(DeviceExtension);                      /* Reset the I2C chip */

    /*  Attempt to issue a read for retries number of times.  If the ACK is not
     *  received, then return that the I2C chip is not present or not
     *  functional.
     */

    flag = 1;
    retries = 100;

    do
    {
        NvmSendStart(DeviceExtension);
        /* Send a dummy write          1010 | A2 A1 A0 | Write */
    } while ( --retries && (NvmSendData( DeviceExtension, 0xA0 | 0x00 | 0x00 ) != 0x00) );

    if (retries != 0)
    {
        flag = NvmSendData( DeviceExtension, 0x00 );         /* Address zero */
        NvmSendStart(DeviceExtension);
        /*                 1010 | A2 A1 A0 | Read */
        flag += NvmSendData( DeviceExtension, 0xA0 | 0x00 | 0x01 );   /* read */
        (void)NvmReadData(DeviceExtension);
        NvmSendNoAck(DeviceExtension);                     /* Also sends stop */
    }

    /*  Turn the clock back into an input signal so that the I2C won't
     *  recognize our LED line (same as the data line) toggling.
     *  Re-enable the H/W control of the LED.
     */

    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)((READ_SIOP_UCHAR( GPCNTL )
                        & (~dmask)) | (cmask | GPCNTL_LED_CNTL) ));

    return( (flag == 0) ? SUCCESS : FAILURE );
}

/*  void  NvmSendStop( PHW_DEVICE_EXTENSION )
 *
 *  Input:
 *
 *      DATA line is an output
 *      CLOCK line is an output
 *
 *  Output:
 *
 *      An I2C 'stop' signal is sent.
 *      DATA line is asserted
 *      CLOCK line is deasserted
 *
 *  Returns
 *
 *      NONE
 *
 *  Purpose:
 *
 *      This routine is used to send an I2C stop signal.
 */

void  NvmSendStop( PHW_DEVICE_EXTENSION DeviceExtension )
{
    reset_data(DeviceExtension);
    StorPortStallExecution(10L);
    set_clock(DeviceExtension);
    StorPortStallExecution(10L);
    set_data(DeviceExtension);
    StorPortStallExecution(10L);
    reset_clock(DeviceExtension);
}


/*  void  NvmSendStart( PHW_DEVICE_EXTENSION )
 *
 *  Input:
 *
 *      DATA line is an output
 *      CLOCK line is an output
 *
 *  Output:
 *
 *      An I2C 'start' signal is sent.
 *      DATA line is deasserted
 *      CLOCK line is deasserted
 *
 *  Returns
 *
 *      NONE
 *
 *  Purpose:
 *
 *      This routine is used to send an I2C start signal.
 */

void  NvmSendStart( PHW_DEVICE_EXTENSION DeviceExtension )
{
    set_data(DeviceExtension);
    StorPortStallExecution(10L);
    set_clock(DeviceExtension);
    StorPortStallExecution(10L);
    reset_data(DeviceExtension);
    StorPortStallExecution(10L);
    reset_clock(DeviceExtension);
}

/*  UINT  NvmSendData( PHW_DEVICE_EXTENSION, UINT Value )
 *
 *  Input:
 *
 *      UINT Value - This is the data value to send (lower 8 bits only)
 *
 *      ???
 *
 *  Output:
 *
 *      ???
 *
 *  Returns
 *
 *      UINT - == 0 if no acknowledge signal is present.
 *             != 0 if an acknowledge signal is present.
 *
 *  Purpose:
 *
 *      This routine is used to send a single data byte to the I2C interface.
 */

UINT  NvmSendData( PHW_DEVICE_EXTENSION DeviceExtension, UINT Value )
{
    UINT   i;
    UINT8  bit;


    for (i = 0, bit = 0x80; i < 8; i++, bit >>= 1)
    {
        if (Value & bit)
        {
            set_data(DeviceExtension);
        }
        else
        {
            reset_data(DeviceExtension);
        }

        StorPortStallExecution(10L);
        set_clock(DeviceExtension);
        StorPortStallExecution(10L);
        reset_clock(DeviceExtension);
    }
    return( NvmReceiveAck(DeviceExtension) );
}

/*  UINT8  NvmReadData( PHW_DEVICE_EXTENSION )
 *
 *  Input:
 *
 *      ???
 *
 *  Output:
 *
 *      ???
 *
 *  Returns
 *
 *      UINT8 - The data byte read from the I2C interface.
 *
 *  Purpose:
 *
 *      This routine is used to read a single data byte from the I2C interface.
 */

UINT8  NvmReadData( PHW_DEVICE_EXTENSION DeviceExtension )
{
    UINT   i;
    UINT8  value, bit;
    UCHAR  dmask = DeviceExtension->data_mask;

    value = 0;
    data_input(DeviceExtension);
    for (i = 0; i < 8; i++)
    {
        StorPortStallExecution(10L);
        set_clock(DeviceExtension);
        StorPortStallExecution(10L);

        /*  Read in the next bit and shift it into place.
         *
         *  NOTE: Need to check which line is data_mask.  If not bit 1, then
         *        we need to shift the data bit to bit 1 before or'ing it in.
         */

        bit = READ_SIOP_UCHAR( GPREG ) & dmask;
        if (dmask == 0x02)  // if data in bit 2 need to shift right 1
            bit >>= 1;
        value = (UINT8)(value << 1) | bit;
        reset_clock(DeviceExtension);
    }
    data_output(DeviceExtension);
    return( value );
}

/*  void  NvmSendAck( PHW_DEVICE_EXTENSION )
 *
 *  Input:
 *
 *      ???
 *
 *  Output:
 *
 *      ???
 *
 *  Returns
 *
 *      NONE
 *
 *  Purpose:
 *
 *      This routine is used to send an acknowledge signal to the I2C part.
 */

void  NvmSendAck( PHW_DEVICE_EXTENSION DeviceExtension )
{
    StorPortStallExecution(10L);
    reset_data(DeviceExtension);
    set_clock(DeviceExtension);
    StorPortStallExecution(10L);
    reset_clock(DeviceExtension);
    reset_data(DeviceExtension);
}

/*  UINT  NvmReceiveAck( PHW_DEVICE_EXTENSION )
 *
 *  Input:
 *
 *      ???
 *
 *  Output:
 *
 *      ???
 *
 *  Returns
 *
 *      UINT - == 0 if no acknowledge signal is present.
 *             != 0 if an acknowledge signal is present.
 *
 *  Purpose:
 *
 *      This routine is used to check for an acknowledge signal from the I2C
 *      part.
 */

UINT  NvmReceiveAck( PHW_DEVICE_EXTENSION DeviceExtension )
{
    UINT  status;


    data_input(DeviceExtension);
    StorPortStallExecution(10L);
    set_clock(DeviceExtension);
    status = READ_SIOP_UCHAR( GPREG ) & DeviceExtension->data_mask;
    StorPortStallExecution(10L);
    reset_clock(DeviceExtension);
    data_output(DeviceExtension);
    return( status );
}

/*  void  NvmSendNoAck( PHW_DEVICE_EXTENSION )
 *
 *  Input:
 *
 *      ???
 *
 *  Output:
 *
 *      ???
 *
 *  Returns
 *
 *      NONE
 *
 *  Purpose:
 *
 *      This routine is used to send a 'no acknowledge' signal to the I2C part.
 */

void  NvmSendNoAck( PHW_DEVICE_EXTENSION DeviceExtension)
{
    StorPortStallExecution(10L);
    set_data(DeviceExtension);
    set_clock(DeviceExtension);
    StorPortStallExecution(10L);
    reset_clock(DeviceExtension);
    reset_data(DeviceExtension);
    NvmSendStop(DeviceExtension);
}


/*  MEMORY_STATUS  HwReadNonVolatileMemory( PHW_DEVICE_EXTENSION DeviceExtension,
 *                                          UINT8 *Buffer, UINT Offset,
 *                                          UINT Length )
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM is being read.  If
 *          the ACF_NO_NON_VOLATILE_MEMORY bit in the Public.ControlFlags field
 *          is set, then it is illegal to call this routine.
 *
 *      UINT8 far *Buffer - The data buffer in which to store the data being
 *          accessed.  This buffer must be Length UINT8 elements in size.
 *
 *      UINT Offset - The non-volatile memory offset to start reading at.
 *
 *      UINT Length - The number of UINT8 elements to read from the NVM.
 *
 *  Output:
 *
 *      UINT8 far *Buffer - If this routine returns MS_GOOD, then this buffer
 *          is filled with Length UINT8 elements from the NVM.
 *
 *  Returns:
 *
 *      MS_GOOD - If the operation completed successfully.
 *
 *      Another MEMORY_STATUS - If the operation failed for some reason.
 *
 *  Purpose:
 *
 *      This routine is used to read the non-volatile memory of a particular
 *      adapter.
 */

MEMORY_STATUS  HwReadNonVolatileMemory( PHW_DEVICE_EXTENSION DeviceExtension,
                                    UINT8 *Buffer, UINT Offset, UINT Length )
{
    UINT  i;
    UINT  nvmAddress;
    UCHAR dmask = DeviceExtension->data_mask;
    UCHAR cmask = DeviceExtension->clock_mask;


    /* Make sure that the requested addresses are in range */

    if (Offset + Length > 2048)
    {
        return( MS_ILLEGAL_ADDRESS );
    }


    /*  Turn the data & clock lines into outputs and turn off H/W LED,
     *  then send a stop signal to the I2C chip to reset it to a known state.
     */

    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)(READ_SIOP_UCHAR(GPCNTL) &
                 (~(dmask | cmask | GPCNTL_LED_CNTL))) );
    NvmSendStop(DeviceExtension);                      // Reset the I2C chip

    /* Now read in all of the requested data */

    nvmAddress = 0xA0 | ((Offset & 0x700) >> 7);
    do
    {
        NvmSendStart(DeviceExtension);
    } while ( NvmSendData( DeviceExtension, nvmAddress | 0x00 ) != 0x00 );   // dummy write

    (void)NvmSendData( DeviceExtension, Offset & 0x00FF );   // address
    NvmSendStart(DeviceExtension);
    (void)NvmSendData( DeviceExtension, nvmAddress | 0x01 ); // read

    *Buffer = NvmReadData(DeviceExtension);
    for (i = 1; i < Length; i++)
    {
        NvmSendAck(DeviceExtension);
        Buffer++;
        *Buffer = NvmReadData(DeviceExtension);
    }
    NvmSendNoAck(DeviceExtension);                         // Also sends Stop

    /*  Turn the clock back into an input signal so that the I2C won't
     *  recognize our LED line (same as the data line) toggling.
     *  Re-enable the H/W control of the LED.
     */

    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)((READ_SIOP_UCHAR( GPCNTL )
                        & (~dmask)) | (cmask | GPCNTL_LED_CNTL) ));

    return( MS_GOOD );
}


/* void InvalidateNvmData(PHW_DEVICE_EXTENSION DeviceExtension)
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM is being used.
 *
 *  Returns:
 *      None
 *
 *  Purpose:
 *
 *      This routine is used to set defaults the nv ram fields so the rest
 *      of the driver can still use them.
 */
void InvalidateNvmData( PHW_DEVICE_EXTENSION DeviceExtension )
{
    UINT8 WideBits, OffsetValue, i;
    UINT16 SyncValue;
    USHORT hbaCap = DeviceExtension->hbaCapability;

    if ( hbaCap & HBA_CAPABILITY_WIDE )
        WideBits = WIDE_16;
    else
        WideBits = WIDE_NONE;

    // Initialize sync period to maximum supported
    SyncValue = SYNC_80;

    // Initialize sync offset to DT maximum
    OffsetValue = MAX_1010_DT_OFFSET;

    for (i=0; i < HW_MAX_DEVICES; i++)
    {
        DeviceExtension->DeviceTable[i].SyncPeriodNs = SyncValue;
        DeviceExtension->DeviceTable[i].SyncOffset = OffsetValue;
        DeviceExtension->DeviceTable[i].WideDataBits = WideBits;
    }

    DeviceExtension->HostSCSIId = 0x07;

    DeviceExtension->TerminatorState = TS_CANT_PROGRAM;
}

/* BOOLEAN RetrieveNvmData( PHW_DEVICE_EXTENSION)
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM is being read.  
 *
 *
 *  Returns:
 *
 *      BOOLEAN
 *          SUCCESS if nvram data is of correct type, correct sumcheck and correct major\minor numbers
 *          FAILURE if any flaws are found.
 *
 *
 *  Purpose:
 *
 *      This routine is used to read the non-volatile memory of a particular
 *      adapter, verify it as valid and to fill the DeviceExtension fields housed within the
 *      nvram structure.
 */
 BOOLEAN RetrieveNvmData( PHW_DEVICE_EXTENSION DeviceExtension)
 {
    NVM_HEADER  NvmHeader;
    NON_VOLATILE_SETTINGS   NvmData;
    BOOLEAN     Status = FAILURE;

    if (HwReadNonVolatileMemory(DeviceExtension, (UINT8 *)&NvmHeader,
                             (0 + NVMDATAOFFSET), sizeof(NvmHeader)) == MS_GOOD)
    {
        if (NvmHeader.Type == HT_BOOT_ROM)
        {
            if (HwReadNonVolatileMemory(DeviceExtension, (UINT8 *)&NvmData,
                    (sizeof(NvmHeader) + NVMDATAOFFSET), sizeof(NvmData)) == MS_GOOD)
            {
                if (CalculateCheckSum((UINT8 *)&NvmData, NvmHeader.Length) == NvmHeader.CheckSum)
                {
                    if ( (NvmData.VersionMajor == NVS_VERSION_MAJOR) &&
                         (NvmData.VersionMinor == NVS_VERSION_MINOR) )
                    {
                        // fill host structures with NVM data
                        FillNvmData( DeviceExtension, &NvmData);
                        Status = SUCCESS;
                    }
                }
            }
        }
    }
    return (Status);
}

/* UINT16 CalculateCheckSum( UINT8 * PNvmData, UINT16 Length)
 *
 *  Input:
 *
 *      UINT8 *  PNvmData Pointer to the NVRAM data just read      
 *      UINT16  Length length of the data to calculate sum check against
 *
 *
 *  Returns:
 *
 *      UINT16  returns the 16 bit sum of NVRAM data ( read as all 8 bit members)
 *
 *
 *  Purpose:
 *
 *      This routine is used calculate the sum check of the nvram data area to insure
 *      it is valid before its use.
 */

UINT16 CalculateCheckSum(UINT8 * PNvmData, UINT16 Length)
{
    UINT16  i;
    UINT16  CheckSum = 0;

    for ( i = 0; i < Length; i++, PNvmData++)
        CheckSum += *PNvmData;

    return ( CheckSum);

}

/* UINT8 CalculateMfgCheckSum( UINT8 * PNvmData, UINT16 Length)
 *
 *  Input:
 *
 *      UINT8 *  PNvmData Pointer to the NVRAM data just read      
 *      UINT16  Length length of the data to calculate sum check against
 *
 *  Returns:
 *
 *      UINT8  returns the 16 bit sum of Mfg data (read as all 8 bit members)
 *
 *  Purpose:
 *
 *      This routine is used calculate the sum check of the Mfg data area to
 *      insure it is valid before its use.
 */

UINT8 CalculateMfgCheckSum(UINT8 * PNvmData, UINT16 Length)
{
    UINT16  i;
    UINT8  CheckSum = 0x55;

    for ( i = 0; i < Length; i++, PNvmData++)
        CheckSum += *PNvmData;

    return ( (UINT8)(~CheckSum + 1) );
}

/* GPIO Pin Usage Routines - Allow OEM's to tailor the use of GPIO pins via
                             the Mfg data area in NVRAM                     */                           

/* VOID RetrieveMfgData( PHW_DEVICE_EXTENSION)
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM Mfg data
 *      is being read.  
 *
 *  Returns:
 *      None
 *
 *  Purpose:
 *
 *      This routine is used to read the Mfg data from non-volatile memory of a
 *      particular adapter, verify it as valid and to fill the DeviceExtension
 *      GPIO pin usage array.  If not found, default GPIO pin settings are
 *      used.  This routine is called only if NVRAM is found.
 */
VOID RetrieveMfgData( PHW_DEVICE_EXTENSION DeviceExtension)
{
    UINT8 MfgDataBuffer[sizeof(NVM_MFG_DATA)];
    PTR_NVM_MFG_DATA ptrMfgData = (PTR_NVM_MFG_DATA)MfgDataBuffer;
    UINT8 *ptrGpio = DeviceExtension->Gpio;

    // read mfg data
    if (HwReadNonVolatileMemory(DeviceExtension, (UINT8 *)ptrMfgData,
                    (NVM_OFFSET_MFG_DATA), sizeof(NVM_MFG_DATA)) == MS_GOOD)
    {
        // read was good, check table length (larger is OK as it may be
        // a later version of the table)
        if (ptrMfgData->Table.TableLen >= sizeof(ptrMfgData->Table) )
        {
            // check mfg data checksum
            if (CalculateMfgCheckSum((UINT8 *)&ptrMfgData->Table,
                (sizeof(NVM_MFG_DATA) - sizeof(ptrMfgData->Checksum))) ==
                        ptrMfgData->Checksum)
            {
                // we have good mfg data, fill in the GPIO array
                StorPortMoveMemory(ptrGpio, &ptrMfgData->Table, 5);
                return;
            }
        }
    }

    // if we get here, we couldn't get the mfg data for some reason
    // just use defaults.
    ptrGpio[1] = GPIO_UNUSED;                   // Active n/a
    ptrGpio[2] = GPIO_UNUSED;                   // Active n/a
    ptrGpio[3] = GPIO_HVD_SENSE;                // Active Low
    ptrGpio[4] = GPIO_FLASH_PROGRAM | 0x80;     // Active High
}

/* setup GPIO pins as inputs or outputs based on their usage */
void HwInitGpioPins( PHW_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR i, setMask, usage;
    PUCHAR pGpio = DeviceExtension->Gpio;

    setMask = 0;                // calculate mask for GPCNTL register
    for (i = 1; i < 5; i++)
    {
        usage = pGpio[i] & GPIO_USAGE_MASK;     // get usage code
        // check for function requiring input pin
        if ( !(usage == GPIO_FLASH_PROGRAM || usage == GPIO_TERMINATION) )
            setMask |= 1<<i;
    }

    // Write GPCNTL with setMask to enable pins as input/output as appropriate
    WRITE_SIOP_UCHAR(GPCNTL, setMask );
}

/* find GPIO assigned for a particular usage code */
char HwFindGpioPin( PHW_DEVICE_EXTENSION DeviceExtension, char usageCode,
                   PUCHAR ptrActiveLevel )
{
    char i;
    char pin = 0;
    UCHAR activeLevel = 0;
    PUCHAR pGpio = DeviceExtension->Gpio;

    // Determine which GPIO pin has been defined for the specified usageCode.
    // If a match isn't found, return -1
    for (i = 1; i < 5; i++)
    {
        if ( (pGpio[i] & GPIO_USAGE_MASK) == usageCode )
        {
            pin = 1<<i;
            activeLevel = pGpio[i] & GPIO_LEVEL_MASK;
            break;
        }
    }

    if (i == 5)
        return( -1 );   // No pin is defined for usageCode

    // return activeLevel if supplied pointer is not NULL
    if ( ptrActiveLevel != NULL )
        *ptrActiveLevel = activeLevel;

    return( pin );      // return the GPIO pin mask
}

/* read GPIO pin for a particular usage code */
char HwReadGpioPin( PHW_DEVICE_EXTENSION DeviceExtension, char usageCode)
{
    char readPin, setting;
    UCHAR activeLevel;

    // find the GPIO pin defined for this function
    if ( (readPin = HwFindGpioPin( DeviceExtension, usageCode, &activeLevel))
            == -1 )
        return( -1 );   // no pin defined for usageCode

    setting = (READ_SIOP_UCHAR(GPREG) & readPin) ? GPIO_ON : GPIO_OFF;

    // if active level is low, need to reverse value of setting
    if (!activeLevel)
        setting ^= 1;   // xor with 1 reverses setting

    return( setting );
}

/* set GPIO pin for a particular usage code to specified value */
char HwSetGpioPin( PHW_DEVICE_EXTENSION DeviceExtension, char usageCode,
                   char setting)
{
    char setPin, currentReg;
    UCHAR activeLevel;

    // find the GPIO pin defined for this function
    if ( (setPin = HwFindGpioPin( DeviceExtension, usageCode, &activeLevel))
            == -1 )
        return( -1 );   // no pin defined for usageCode

    if (!activeLevel)   // active low, reverse setting
        setting ^= 1;

    currentReg = READ_SIOP_UCHAR(GPREG);    // get current GPREG value
    if (setting)                            // if setting is GPIO_ON (true)
        currentReg |= setPin;               //    OR in pin value
    else                                    // else (if GPIO_OFF)
        currentReg &= ~setPin;              //    AND off pin value

    WRITE_SIOP_UCHAR(GPREG, currentReg);    // write new value to GPREG

    return( 0 );
}
/* end of GPIO usage routines */

/*  void  NvmPageWrite( PHW_DEVICE_EXTENSION DeviceExtension,
 *                        UINT8 *Buffer, UINT Offset, UINT Pages )
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DeviceExtension - The adapter whose NVM is
 *      being written.
 *
 *      UINT8 *Buffer - This references the data to write.
 *
 *      UINT Offset - This references the offset in the I2C part to start
 *          writing data at.  It must be on a page boundary.
 *
 *      UINT Pages - This is the number of pages to write to the I2C.
 *
 *  Output:
 *
 *      The data is written to the I2C part.
 *
 *  Returns:
 *
 *      NONE
 *
 *  Purpose:
 *
 *      This routine is used to perform an I2C page write with a page size of
 *      16 bytes.
 */

void  NvmPageWrite( PHW_DEVICE_EXTENSION DeviceExtension,
                        UINT8 *Buffer, UINT Offset, UINT Pages )
{
    UINT  pageNum;                      // Current page
    UINT  byteNum;                      // Current byte within the page
    UINT  nvmAddress;

    for (pageNum = 0; pageNum < Pages; pageNum++)
    {
        nvmAddress = 0xA0 | ((Offset & 0x700) >> 7);
        do
        {
            NvmSendStart(DeviceExtension);
            // Send write and check for an acknowledge
        } while ( NvmSendData( DeviceExtension, nvmAddress | 0x00 ) != 0x00 );

        (void)NvmSendData( DeviceExtension, Offset & 0x00FF );   // address

        for (byteNum = 0; byteNum < 16; byteNum++)
        {
            (void)NvmSendData( DeviceExtension, *Buffer );
            Buffer++;
        }
        Offset += 16;
        NvmSendStop(DeviceExtension);
    }
}

/*  void  NvmByteWrite( PHW_DEVICE_EXTENSION DeviceExtension,
 *                       UINT8 *Buffer, UINT Offset, UINT Length )
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DeviceExtension - The adapter whose NVM is
 *      being written.
 *
 *      UINT8 *Buffer - This references the data to write.
 *
 *      UINT Offset - This references the offset in the I2C part to start
 *          writing data at.
 *
 *      UINT Length - This is the number of bytes to write to the I2C.
 *
 *  Output:
 *
 *      The data is written to the I2C part.
 *
 *  Returns:
 *
 *      NONE
 *
 *  Purpose:
 *
 *      This routine is used to perform an I2C byte write.
 */

void  NvmByteWrite( PHW_DEVICE_EXTENSION DeviceExtension,
                        UINT8 *Buffer, UINT Offset, UINT Length )
{
    UINT  byteNum;                      // Current byte number
    UINT  nvmAddress;

    for (byteNum = 0; byteNum < Length; byteNum++)
    {
        nvmAddress = 0xA0 | ((Offset & 0x700) >> 7);
        do
        {
            NvmSendStart(DeviceExtension);
            // Send write and check for an acknowledge
        } while ( NvmSendData( DeviceExtension, nvmAddress | 0x00 ) != 0x00 );

        (void)NvmSendData( DeviceExtension, Offset & 0x00FF );   // address
        (void)NvmSendData( DeviceExtension, *Buffer );        // Data byte
        Buffer++;
        Offset++;
        NvmSendStop(DeviceExtension);
    }
}

/*  MEMORY_STATUS HwWriteNonVolatileMemory(PHW_DEVICE_EXTENSION DeviceExtension,
 *                                           UINT8 *Buffer, UINT Offset,
 *                                           UINT Length )
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DeviceExtension - The adapter whose NVM is
 *      being written.
 *
 *      UINT8 *Buffer - The data buffer from which to write data.  This
 *          buffer must be Length UINT8 elements in size.
 *
 *      UINT Offset - The non-volatile memory offset to start writing at.
 *
 *      UINT Length - The number of UINT8 elements to write to the NVM.
 *
 *  Output:
 *
 *      UINT8 *Buffer - If this routine returns MS_GOOD, then this
 *          buffer has been written to the NVM.
 *
 *  Returns:
 *
 *      MS_GOOD - If the operation completed successfully.
 *
 *      Another MEMORY_STATUS - If the operation failed for some reason.
 *
 *  Purpose:
 *
 *      This routine is used to write the non-volatile memory of a particular
 *      adapter.
 */

MEMORY_STATUS  HwWriteNonVolatileMemory( PHW_DEVICE_EXTENSION DeviceExtension,
                                    UINT8 *Buffer, UINT Offset, UINT Length )
{
    UINT  index;
    UINT  segmentLength;
    UCHAR dmask = DeviceExtension->data_mask;
    UCHAR cmask = DeviceExtension->clock_mask;


    /* Make sure that the requested addresses are in range */

    if ( Offset + Length > 2048)
    {
        return( MS_ILLEGAL_ADDRESS );
    }

    /*  Turn the data & clock lines into outputs and turn off H/W LED,
     *  then send a stop signal to the I2C chip to reset it to a known state.
     */

    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)(READ_SIOP_UCHAR(GPCNTL) &
                 (~(dmask | cmask | GPCNTL_LED_CNTL))) );
    NvmSendStop(DeviceExtension);                      // Reset the I2C chip

    index = 0;
    segmentLength = 16 - (Offset % 16);
    if (segmentLength != 16)
    {
        if (segmentLength > Length)
        {
            segmentLength = Length;
        }
        Length -= segmentLength;

        NvmByteWrite(DeviceExtension, &Buffer[index], Offset, segmentLength);
        Offset += segmentLength;
        index += segmentLength;
    }

    if (Length >= 16)
    {
        NvmPageWrite(DeviceExtension, &Buffer[index], Offset, Length / 16 );
        Offset += Length - (Length % 16);
        index  += Length - (Length % 16);
        Length %= 16;
    }

    if (Length != 0)
    {
        NvmByteWrite(DeviceExtension, &Buffer[index], Offset, Length );
    }

    /*  Turn the clock back into an input signal so that the I2C won't
     *  recognize our LED line (same as the data line) toggling.
     *  Re-enable the H/W control of the LED.
     */

    WRITE_SIOP_UCHAR( GPCNTL, (UCHAR)((READ_SIOP_UCHAR( GPCNTL )
                        & (~dmask)) | (cmask | GPCNTL_LED_CNTL) ));

    return( MS_GOOD );
}

//
//
// start of NVS usage code
//
/*  BOOLEAN  NVSDetect( PHW_DEVICE_EXTENSION DeviceExtension )
 *
 *  Input:
 *
 *      DeviceExtension - Pointer to the device extension for this adapter.
 *
 *  Returns:
 *
 *      BOOLEAN - SUCCESS if NVS was detected
 *                FAILURE if NVS was not detected
 *
 *  Purpose:
 *
 *      This routine is used to search through ROM Bios area (C0000-FF000)
 *      to determine if there is NVS data (copy of our NVRAM data) available.
 */
BOOLEAN  NVSDetect( PHW_DEVICE_EXTENSION DeviceExtension )
{
    UINT  offset;
    ULONG rawadrs;
    UCHAR *tmp;
    PUCHAR BiosCodeSpace;

    rawadrs = 0x000C0000;       // Start the search at this address.
    do
    {
        // Get a mapped system address to the physical address of a BIOS code
        // space area.  We'll look through it 2k chunks at a time unless a ROM
        // signature is found and it's not ours.

        BiosCodeSpace = (PUCHAR)StorPortGetDeviceBase(DeviceExtension,
                                Internal,
                                0, // must use bus 0 for system memory
                                StorPortConvertUlongToPhysicalAddress(rawadrs),
                                0x800,  // BIOS space mapped on 2k boundries
                                FALSE );  // not in I/O space

        tmp = BiosCodeSpace;
        if ( (tmp) && ((tmp[0] == 0x55) && (tmp[1] == 0xaa) &&
                       (tmp[2] != 0x00)) )
        {
            // Found a ROM signature at least, is it ours?
            // check for 3.X BIOS
            if (tmp[12] == 'P' && tmp[13] == 'C')
            {
                // it has no NVS support
                break;
            }

            // check for 4.X BIOS
            if (tmp[12] == 'P' && tmp[13] == 'X')
            {
                // Seems to be... 'PX' at offsets 12 & 13
                // figure offset to the PCI Data Struct
                offset = (((UINT) tmp[25]) << 8) + tmp[24];
                tmp = BiosCodeSpace + offset;
        
                // bump offset past the PCI Data Struct & past the
                // 2nd pair code ver. nums
                offset += 0x1a;

                // offset now is the total offset from the start of this BIOS
                // image to the pointer to the 1st of the NVS information
                // structures.  The ptr/offset stored here & added to the raw
                // address we have currently mapped is what we want to keep.

                tmp = BiosCodeSpace + offset;
                if ((tmp[0] == 0) && (tmp[1] == 0))
                {
                    // the pointer/offset is null; No NVS on this platform
                    break;
                }
                else
                {
                    // save pointers to start of Bios area and offset to NVS
                    DeviceExtension->BiosCodeSpacePtr = rawadrs;
                    DeviceExtension->NVSDataOffset =
                            ((((ULONG)tmp[1]) << 8) + tmp[0]);
                    StorPortFreeDeviceBase(DeviceExtension, BiosCodeSpace);
                    return (SUCCESS);
                }
            
            }

            // did not find our signature, advance to next option ROM
            // and continue search.
            rawadrs += (ULONG)(BiosCodeSpace[2] * 0x200);  
        }
        else    // no option ROM header detected
        {
            // if not at a 16K boundary, keep checking every 512 bytes

            // For an unknown reason, the call to StorPortGetDeviceBase()
            // fails on 0xC0000.  However, the below IF will let us continue.

            if ( (rawadrs <= 0xC8000) || (rawadrs & 0x00003FFF) )
            {
                rawadrs += 512;
            }
            else
            {
                // no option rom at 16K boundary, quit
                break;
            }
        }

        // free current mapping
        StorPortFreeDeviceBase( DeviceExtension, (PVOID) BiosCodeSpace );

    } while (rawadrs < 0x000ff000);

    DeviceExtension->BiosCodeSpacePtr = 0L;
    DeviceExtension->NVSDataOffset = 0L;
    StorPortFreeDeviceBase(DeviceExtension, BiosCodeSpace);
    return (FAILURE);
}

/* BOOLEAN RetrieveNVSData( PHW_DEVICE_EXTENSION DeviceExtension )
 *
 *  Input:
 *
 *      DevcieExtesnion - The adapter whose NVM data is to be setup.  
 *
 *
 *  Returns:
 *
 *      BOOLEAN
 *          SUCCESS if nvram data is of correct type, correct sumcheck and
 *                  correct major\minor numbers
 *          FAILURE if any flaws are found.
 *
 *
 *  Purpose:
 *
 *      This routine is used to retrieve the copy of non-volatile memory data
 *      of a particular adapter out of the BIOS code space and to fill the
 *      DeviceExtension fields housed within the nvram structure.
 */
 BOOLEAN RetrieveNVSData( PHW_DEVICE_EXTENSION DeviceExtension )
 {
    BOOLEAN                 Status = FAILURE;
    PTR_SINGLE_HBA_NVS      pNVS;
    UCHAR                   *tmp;
    ULONG                   offset;

    if ( !(DeviceExtension->NVSDataOffset) )   // if NULL, return FAILURE
        return (Status);

    offset = DeviceExtension->NVSDataOffset; // offset to NVS data struct.

    // Get a mapped system address to the BIOS code area found earlier.
    // Map the BIOS code
    while(TRUE)
    {
        tmp = (UCHAR *) StorPortGetDeviceBase( DeviceExtension,
                            Internal,
                            0,  // must use bus 0 for system memory
                            StorPortConvertUlongToPhysicalAddress(DeviceExtension->BiosCodeSpacePtr + offset),
                            (sizeof(SINGLE_HBA_NVS)),  
                            FALSE );    // not in I/O space

        if ( !tmp ) // if it comes back a NULL pointer, exit w/FAILURE
            break;

        pNVS = (PTR_SINGLE_HBA_NVS) tmp; // point to NVS data segment
        if (pNVS->header1.Type == HT_BOOT_ROM)
        {
            if ((pNVS->settings.VersionMajor == NVS_VERSION_MAJOR) &&
                (pNVS->settings.VersionMinor == NVS_VERSION_MINOR))
            {
                // this NVS data for this board??
                if (pNVS->IoPort == DeviceExtension->IoPortAddress)
                {
                    // fill host data structures with NVS data
                    FillNvmData( DeviceExtension, &pNVS->settings);
                    Status = SUCCESS;
                    break;
                }
                else
                {
                    // advance to the next NVS data struct if present.
                    if ( pNVS->NextHBANVS ) 
                    {
                        offset = (ULONG)pNVS->NextHBANVS;
                        StorPortFreeDeviceBase( DeviceExtension, tmp );
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else    // not correct major & minor version number
            {
                break;
            }
        }
        else    // not our Boot ROM
        {
            break;
        }
    } // end while(TRUE)
	if(tmp!=NULL)
	{
        StorPortFreeDeviceBase( DeviceExtension, (PVOID) tmp );
	}
    return (Status);
}

/* VOID FillNvmData( PHW_DEVICE_EXTENSION, PTR_NON_VOLATILE_SETTINGS)
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM or NVS
 *      data is being read.  
 *
 *      PTR_NON_VOLATILE_SETTINGS pNVM - Pointer to the NVM or NVS structure.
 *
 *  Returns:
 *
 *      None.
 *
 *  Purpose:
 *
 *      This routine fills in the DeviceExtension fields from the NVRAM or
 *      NVS data structures.
 */
 VOID FillNvmData( PHW_DEVICE_EXTENSION DeviceExtension,
                   PTR_NON_VOLATILE_SETTINGS pNVM)
{
    USHORT                  maxid = (SYM_NARROW_MAX_TARGETS - 1);
    PVOID                   SrcBuffer,DestBuffer;

    // get termination state
    DeviceExtension->TerminatorState = pNVM->TerminatorState;
    // get host SCSI ID, check against board limits
    DeviceExtension->HostSCSIId = pNVM->HostScsiId;
    if ( DeviceExtension->hbaCapability & HBA_CAPABILITY_WIDE )
        maxid = (SYM_MAX_TARGETS - 1);
    if ((DeviceExtension->HostSCSIId > maxid))
            DeviceExtension->HostSCSIId = 0x07;
    // move DeviceTable info into DeviceExtension storage
    DestBuffer = (PVOID)DeviceExtension->DeviceTable;
    SrcBuffer = (PVOID)pNVM->DeviceTable;
    StorPortMoveMemory (DestBuffer, SrcBuffer,
            (ULONG)sizeof(DeviceExtension->DeviceTable));
}


/* BOOLEAN ReadNVM( PHW_DEVICE_EXTENSION, PVOID, PULONG)
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM or NVS
 *      data is being read.  
 *
 *      PVOID - Pointer to the receiving data buffer.
 *
 *      PULONG - Pointer to a ULONG to store length of NVM data read.
 *
 *  Returns:
 *
 *      TRUE is successful, FALSE if not.
 *
 *  Purpose:
 *
 *      This routine reads the NVM data into a buffer.
 */
 BOOLEAN ReadNVM( PHW_DEVICE_EXTENSION DeviceExtension, PVOID dataBuf,
                  PULONG ret_length)
{
    NVM_HEADER  NvmHeader;
    NON_VOLATILE_SETTINGS   NvmData;
    ULONG length;

    // read NVM header to obtain length
    if (HwReadNonVolatileMemory(DeviceExtension, (UINT8 *)&NvmHeader,
                NVMDATAOFFSET, sizeof(NvmHeader)) == MS_GOOD)
    {
        *ret_length = length = NvmHeader.Length;
        // read NVRAM into data buffer
        if (HwReadNonVolatileMemory(DeviceExtension, (UINT8 *)&NvmData,
            (sizeof(NvmHeader) + NVMDATAOFFSET), length) == MS_GOOD)
        {
            // copy NVM data into IOCTL buffer
            StorPortMoveMemory(dataBuf, &NvmData, length);
            return(TRUE);
        }
    }

    return(FALSE);
}


/* BOOLEAN WriteNVM( PHW_DEVICE_EXTENSION, PVOID, ULONG)
 *
 *  Input:
 *
 *      PHW_DEVICE_EXTENSION DevcieExtesnion - The adapter whose NVM or NVS
 *      data is being written.  
 *
 *      PVOID - Pointer to the source data buffer.
 *
 *      ULONG - Length of NVM data to write.
 *
 *  Returns:
 *
 *      TRUE is successful, FALSE if not.
 *
 *  Purpose:
 *
 *      This routine writes the NVM data from a buffer.
 */
 BOOLEAN WriteNVM( PHW_DEVICE_EXTENSION DeviceExtension, _In_reads_bytes_(length) PVOID dataBuf,
                   _In_range_(1, sizeof(NON_VOLATILE_SETTINGS)) ULONG length)
{
    NVM_HEADER  NvmHeader;
    NON_VOLATILE_SETTINGS   NvmData;
    USHORT checksum;


	RtlZeroMemory(&NvmData, sizeof(NON_VOLATILE_SETTINGS));
    // move data buffer into NvmData structure
    StorPortMoveMemory( &NvmData, dataBuf, length);
    // calculate checksum
    checksum = CalculateCheckSum((UINT8 *)&NvmData, (USHORT)length);
    // write checksum bytes only to NVRAM
    if (HwWriteNonVolatileMemory(DeviceExtension, (UINT8 *)&checksum,
        FIELD_OFFSET(NVM_HEADER, CheckSum) + NVMDATAOFFSET,
        sizeof(checksum)) == MS_GOOD)
    {
        // write NVRAM data
        if (HwWriteNonVolatileMemory(DeviceExtension, (UINT8 *)&NvmData,
            (sizeof(NvmHeader) + NVMDATAOFFSET), length) == MS_GOOD)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}


/*******************************************************
**                                                    **
**                  set_sync_speed                    **
**                                                    **
*******************************************************/

UCHAR set_sync_speed(PHW_DEVICE_EXTENSION DeviceExtension, UCHAR period)
{
    USHORT dFlgs;

    dFlgs = DeviceExtension->DeviceFlags;

    // if async (0 period), just return
    if ( !period )
        return (period);

    // check for half speed set
    if ( (dFlgs & DFLAGS_HALF_SPEED) && (period == 9) )
        period = 10;

    // if not in LVDS mode, period is limited to 20 MB/sec
    if ( !(dFlgs & DFLAGS_LVDS_MODE) && (period < 12) )
        period = 12;

    // if we've dropped back from LVDS mode, then limit is 10MB/sec
    // (and LVD cable plant may not support Ultra speeds)
    if ( (dFlgs & DFLAGS_LVDS_DROPBACK) && (period < 25) )
        period = 25;

    // workaround for 1010-66 errata at Fast-10
    // don't allow Fast-10 rate, drop back to async
    if ( (DeviceExtension->hbaCapability & HBA_CAPABILITY_1010_66) &&
         (period >= 25) )
        period = 0;

    return (period);
}

/*******************************************************
**                                                    **
**                  set_1010_clock                     **
**                                                    **
*******************************************************/

UCHAR set_1010_clock(PHW_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR scntl3Value;

    DeviceExtension->DeviceFlags &= ~DFLAGS_LVDS_MODE;  // not using LVDS

    WRITE_SIOP_UCHAR( STEST1, 
            (UCHAR)(READ_SIOP_UCHAR(STEST1) | STEST1_DOUBLER_ENABLE));

    // wait at least 100 usec to allow quadruler frequency to lock
    StorPortStallExecution(125);

    WRITE_SIOP_UCHAR( STEST3, 
            (UCHAR)(READ_SIOP_UCHAR(STEST3) | STEST3_HALT_CLOCK));
            
    if ((READ_SIOP_UCHAR(STEST4) & STEST4_BUS_TYPE_MASK) ==
                STEST4_LOWV_DIFF)
    {
        DeviceExtension->DeviceFlags |= DFLAGS_LVDS_MODE;   // use LVDS
    }
    scntl3Value = 0x70;

    WRITE_SIOP_UCHAR( SCNTL3, scntl3Value );
            
    WRITE_SIOP_UCHAR( STEST1, 
            (UCHAR)(READ_SIOP_UCHAR(STEST1) | STEST1_DOUBLER_SELECT));

    WRITE_SIOP_UCHAR( STEST3, 
            (UCHAR)(READ_SIOP_UCHAR(STEST3) & ~STEST3_HALT_CLOCK));

    return (scntl3Value);
}


/******************************************************/
/* This routine eats any interrupts pending.          */
/******************************************************/

BOOLEAN EatInts(PHW_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR dispose;
    UCHAR istat;
    UCHAR sist0;
    UCHAR retries = 100;

#define DIP                     0x01
#define SIP                     0x02

    istat=READ_SIOP_UCHAR(ISTAT0);
    sist0=READ_SIOP_UCHAR(SIST0);

    /* Spin until no DMA or SCSI interrupts left */
    while ( ((istat & (DIP + SIP)) || (sist0 & 0x02)) && retries )
    {
        retries--;      // decrement retries

        if (sist0 & 0x02)
        {
            dispose=READ_SIOP_UCHAR(SIST0);
            dispose=READ_SIOP_UCHAR(SIST1);
        }

        if (istat & SIP)
        {
            dispose=READ_SIOP_UCHAR(SIST0);
            dispose=READ_SIOP_UCHAR(SIST1);
        }
        
        if (istat & DIP)
        {
            dispose=READ_SIOP_UCHAR(DSTAT);
        }
     
        delay_mils(5);
        istat=READ_SIOP_UCHAR(ISTAT0);
        sist0=READ_SIOP_UCHAR(SIST0);
    }
    return(retries ? TRUE : FALSE);
}

/*****************************************************************************/
/* delay_mils is used to delay code X amount of milliseconds by
**    using the system call of storportstallexecution
**
******************************************************************************/
void delay_mils( USHORT counter)
{
    USHORT i;

    for ( i = counter; i > 0; i--)
        StorPortStallExecution(999);
}

/*
	This routine is part of the defect injection for the StorPortEnablePassive.slic rule.
*/
BOOLEAN 
LsiU3PassiveRoutine(
	_In_ PVOID DeviceExtension) 
{
        UNREFERENCED_PARAMETER(DeviceExtension);
	return TRUE;
}

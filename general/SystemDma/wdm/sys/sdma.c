/*++

Copyright (c) 2011  Microsoft Corporation All Rights Reserved

Module Name:

    sdma.c

Abstract:

    The purpose of this driver is to demonstrate V3 system DMA.

Environment:

    Kernel mode only.

--*/


//
// Include files.
//

#include <ntddk.h>          // various NT definitions
#include <string.h>

#include "sdma.h"

#define NT_DEVICE_NAME      L"\\Device\\SDMA"
#define DOS_DEVICE_NAME     L"\\DosDevices\\DmaTest"

#if DBG
#define SDMA_KDPRINT(_x_) \
                DbgPrint("SDMA.SYS: ");\
                DbgPrint _x_;

#else
#define SDMA_KDPRINT(_x_)
#endif

//
// These are the states a PDO or FDO transition upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted,                // Device has received the REMOVE_DEVICE IRP
    UnKnown                 // Unknown state

} DEVICE_PNP_STATE;

typedef struct _COMMON_DEVICE_DATA
{
    // A back pointer to the device object for which this is the extension

    PDEVICE_OBJECT  Self;

    // This flag helps distinguish between PDO and FDO

    BOOLEAN         IsFDO;

    // We track the state of the device with every PnP Irp
    // that affects the device through these two variables.
    
    DEVICE_PNP_STATE DevicePnPState;

    DEVICE_PNP_STATE PreviousPnPState;

    
    ULONG           DebugLevel;

    // Stores the current system power state
    
    SYSTEM_POWER_STATE  SystemPowerState;

    // Stores current device power state
    
    DEVICE_POWER_STATE  DevicePowerState;

    ULONG           ulVariationFlags;

} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA CommonData;

    PDEVICE_OBJECT  UnderlyingPDO;
    
    // The underlying bus PDO and the actual device object to which our
    // FDO is attached

    PDEVICE_OBJECT  NextLowerDriver;

    // List of PDOs created so far
    
    LIST_ENTRY      ListOfPDOs;
    
    // The PDOs currently enumerated.

    ULONG           NumPDOs;

    // A synchronization for access to the device extension.

    FAST_MUTEX      Mutex;

    //
    // The number of IRPs sent from the bus to the underlying device object
    //
    
    ULONG           OutstandingIO; // Biased to 1

    //
    // On remove device plug & play request we must wait until all outstanding
    // requests have been completed before we can actually delete the device
    // object. This event is when the Outstanding IO count goes to zero
    //

    KEVENT          RemoveEvent;

    //
    // This event is set when the Outstanding IO count goes to 1.
    //
    
    KEVENT          StopEvent;
    
    // The name returned from IoRegisterDeviceInterface,
    // which is used as a handle for IoSetDeviceInterfaceState.

    UNICODE_STRING      InterfaceName;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

//
// Define minimum and maximum macros.
//

#define Minimum(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#define Maximum(_a, _b) (((_a) > (_b)) ? (_a) : (_b))

//
// Define the structure that we'll use to communicate with our
// AllocateAdapterChannel callback.
//

typedef struct _SDMA_CALLBACK_CONTEXT {
    IO_ALLOCATION_ACTION Action;
    ULONG  NumberOfMapRegisters;
    PVOID  MapRegisterBase;
    KEVENT CallBackEvent;
    KEVENT CompletionEvent;
} SDMA_CALLBACK_CONTEXT, *PSDMA_CALLBACK_CONTEXT;

//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH SDmaCreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH SDmaDeviceControl;

DRIVER_UNLOAD SDmaUnloadDriver;

VOID
PrintIrpInfo(
    PIRP Irp
    );
VOID
PrintChars(
    _In_reads_(CountChars) PCHAR BufferAddress,
    _In_ size_t CountChars
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, SDmaCreateClose)
#pragma alloc_text( PAGE, SDmaDeviceControl)
#pragma alloc_text( PAGE, SDmaUnloadDriver)
#pragma alloc_text( PAGE, PrintIrpInfo)
#pragma alloc_text( PAGE, PrintChars)
#endif // ALLOC_PRAGMA


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if initialized; an error otherwise.

--*/

{
    NTSTATUS        ntStatus;
    UNICODE_STRING  ntUnicodeString;    // NT Device Name "\Device\SDMA"
    UNICODE_STRING  ntWin32NameString;    // Win32 Name "\DosDevices\DmaTest"
    PDEVICE_OBJECT  deviceObject = NULL;    // ptr to device object

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString( &ntUnicodeString, NT_DEVICE_NAME );

    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        0,                              // We don't use a device extension
        &ntUnicodeString,               // Device name "\Device\SDMA"
        FILE_DEVICE_UNKNOWN,            // Device type
        FILE_DEVICE_SECURE_OPEN,     // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SDMA_KDPRINT(("Couldn't create the device object\n"));
        return ntStatus;
    }

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SDmaCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SDmaCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SDmaDeviceControl;
    DriverObject->DriverUnload = SDmaUnloadDriver;

    //
    // Initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString( &ntWin32NameString, DOS_DEVICE_NAME );

    //
    // Create a symbolic link between our device name  and the Win32 name
    //

    ntStatus = IoCreateSymbolicLink(
                        &ntWin32NameString, &ntUnicodeString );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        //
        // Delete everything that this routine has allocated.
        //
        SDMA_KDPRINT(("Couldn't create symbolic link\n"));
        IoDeleteDevice( deviceObject );
    }


    return ntStatus;
}


NTSTATUS
SDmaCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the driver is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID
SDmaUnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );


    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    IoDeleteSymbolicLink( &uniWin32NameString );

    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }



}

//
// These helper routines allow MDL creation and destruction
// as well as providing simple DMA callback routines for
// channel allocation and DMA completion.
//

ULONG
MDL_SPAN(
    _In_ PMDL  Mdl,
    _In_ PVOID CurrentVa,
         ULONG Length
    )

/*++

Routine Description:

    This function determines the number of pages spanned by the given MDL.
    We have to account for the case where the MDL is actually the head of
    a chain.

Arguments:

    Mdl - Supplies the MDL being checked.

    CurrentVa - Provides our starting address within the first MDL.

    Length - Supplies the total number of bytes from the given MDL chain that
        are associated with this transfer.

Return Value:

    This function returns the number of pages spanned by the specified
    transfer.

--*/

{
    PVOID VirtualAddress;
    ULONG Span;
    ULONG RemainingLength;
    ULONG ChunkLength;
    ULONG MdlOffset;

    if (Length == 0)
    {
        return 0;
    }

    if (Mdl->Next == NULL)
    {
        return ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, Length);
    }

    Span = 0;
    RemainingLength = Length;
    VirtualAddress = CurrentVa;

    MdlOffset = (ULONG) (((ULONG_PTR) CurrentVa)
                         - ((ULONG_PTR) MmGetMdlVirtualAddress(Mdl)));

    while (Mdl != NULL)
    {
        ChunkLength = Minimum(MmGetMdlByteCount(Mdl) - MdlOffset,
                              RemainingLength);

        Span += ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, ChunkLength);

        Mdl = Mdl->Next;
        MdlOffset = 0;
        RemainingLength -= ChunkLength;

        if (Mdl != NULL)
        {
            VirtualAddress = MmGetMdlVirtualAddress(Mdl);
        }

    }

    return Span;
}

VOID
SDmaFreeMdl(
    _In_opt_ PMDL MdlHead
    )

/*++

Routine Description:

    This function frees an MDL that was allocated earlier with a call to
    DmaUnitAllocateMdl.

Arguments:

    MdlHead - Supplies the MDL to free.  This could be the head of a chain
        of MDLs.

Return Value:

    None.

--*/

{
    PMDL Mdl;
    PMDL NextMdl;

    Mdl = MdlHead;

    while (Mdl != NULL) 
    {
        if ((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0)
        {
            MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
        }

        MmFreePagesFromMdl(Mdl);

        NextMdl = Mdl->Next;
        ExFreePool(Mdl);

        Mdl = NextMdl;
    }

    return;
}

VOID SDmaFillMdl(
    _In_ PMDL Mdl,
    _In_ PVOID CurrentVa,
    _In_ ULONG Length,
    _In_ UCHAR Fill
    )

/*++

Routine Description:

    This function fills an MDL with a data byte.

Arguments:

    Mdl - Supplies the MDL that describes the system pages associated with
        this transfer.

    CurrentVa - Provides the starting address of the transfer within
        the given MDL.

    Length - Supplies the length of the transfer in bytes.

    Fill   - The byte to write into the MDL.

Return Value:

    None

--*/

{
    ULONG MdlOffset;
    ULONG PageOffset;
    ULONG ChunkLength;
    PCHAR VirtualAddress;
    ULONG RemainingLength;

    RemainingLength = Length;
    while (Mdl != NULL)
    {

        MdlOffset = (ULONG) (((ULONG_PTR) CurrentVa)
                    - ((ULONG_PTR) MmGetMdlVirtualAddress(Mdl)));

        ASSERT((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0);

        PageOffset = BYTE_OFFSET(CurrentVa);
        VirtualAddress = PAGE_ALIGN(((PCHAR) Mdl->MappedSystemVa) + MdlOffset);

        while ((MdlOffset != MmGetMdlByteCount(Mdl))
               && (RemainingLength != 0))
        {

            ChunkLength = Minimum(MmGetMdlByteCount(Mdl) - MdlOffset,
                                  RemainingLength);

            ChunkLength = Minimum(ChunkLength, PAGE_SIZE - PageOffset);

            ASSERT((PageOffset + ChunkLength) <= PAGE_SIZE);

            RtlFillMemory(((PCHAR) VirtualAddress) + PageOffset, ChunkLength,
                         Fill);

            PageOffset = 0;
            RemainingLength -= ChunkLength;
            MdlOffset += ChunkLength;
            VirtualAddress = VirtualAddress + ChunkLength;
        }

        Mdl = Mdl->Next;
    }
}

VOID SDmaPrintMdl(
    _In_ PMDL Mdl,
    _In_ PVOID CurrentVa,
    _In_ ULONG Length,
    _In_ PCHAR TitleStr
    )

/*++

Routine Description:

    This function prints MDL information to the debugger.

Arguments:

    Mdl - Supplies the MDL to be displayed.

    CurrentVa - Provides the starting address of the transfer within
        the given MDL.

    Length - Supplies the length of the transfer in bytes.

    TitleStr - A text tile to print describing this MDL.

Return Value:

    None

--*/

{
    ULONG MdlOffset;
    PCHAR VirtualAddress;
    ULONG MdlCount = 0;
    ULONG TotalLength = 0;
    PPFN_NUMBER PageFrame;
    ULONG NumPages = 0;
    ULONG PageCount = 0;
    PMDL MdlHead;
    PHYSICAL_ADDRESS PageAddress;
    ULONG RemainingLength;
    ULONG ChunkLength;

    MdlOffset = (ULONG) (((ULONG_PTR) CurrentVa)
                         - ((ULONG_PTR) MmGetMdlVirtualAddress(Mdl)));

    MdlHead = Mdl;
    NumPages = MDL_SPAN(MdlHead, CurrentVa, Length);
    while (Mdl != NULL)
    {

        TotalLength += Minimum(MmGetMdlByteCount(Mdl) - MdlOffset,
                               Length);

        Mdl = Mdl->Next;
        MdlCount++;
    }

    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "########### %s ########\n",
               TitleStr == NULL ? "MDL" : TitleStr );
    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "Mdl Head = 0x%p\n", MdlHead);
    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "Mdl Count = %d\n", MdlCount);
    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "Total Byte Count = %d\n", TotalLength);
    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "Total Physical Pages = %d\n", NumPages);

    Mdl = MdlHead;
    MdlCount = 0;
    RemainingLength = Length;
    while (Mdl != NULL)
    {

        MdlOffset = (ULONG) (((ULONG_PTR) CurrentVa)
                            - ((ULONG_PTR) MmGetMdlVirtualAddress(Mdl)));

        ASSERT((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0);

        VirtualAddress = PAGE_ALIGN(((PCHAR) Mdl->MappedSystemVa) + MdlOffset);
        ChunkLength = Minimum(MmGetMdlByteCount(Mdl) - MdlOffset,
                              RemainingLength);

        NumPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, ChunkLength);

        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "########################################\n");
        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "Mdl #%d: 0x%p\n", MdlCount, Mdl);
        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "    Mdl Offset = %d\n", MdlOffset);
        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "    Mdl Length = %d\n", Mdl->ByteCount);
        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "    Mdl Base VA = 0x%p\n", VirtualAddress);
        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "------------ PHYSICAL PAGES ------------\n");
        PageFrame = MmGetMdlPfnArray( Mdl );

        for (PageCount = 0; PageCount < NumPages; PageCount++)
        {
            PageAddress.QuadPart = (ULONGLONG)(*(PageFrame + PageCount)) << PAGE_SHIFT;
            DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "    Page #%d: 0x%I64x\n", PageCount, PageAddress.QuadPart);
        }

        Mdl = Mdl->Next;
        RemainingLength -= ChunkLength;
        MdlCount++;
    }
    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "########################################\n");
}

PMDL
SDmaAllocateMdl(
    _Out_ PVOID *CurrentVa,
    _Out_ PULONG Length,
    _In_  BOOLEAN ForceContiguous
    )

/*++

Routine Description:

    This function allocates a single MDL.

Arguments:

    CurrentVa - Supplies the location where we should store the starting
        virtual address of the transfer within the allocated MDL.

    Length - Supplies the location where we should store the length of
        the requested transfer.

Return Value:

    This function returns a pointer to a single MDL.

--*/

{
    PMDL MdlHead;
    PMDL Mdl;
    ULONG Span;
    ULONG RequestBytes;
    PHYSICAL_ADDRESS LowAddress;
    PHYSICAL_ADDRESS HighAddress;
    PHYSICAL_ADDRESS SkipAddress;
    PVOID MappingAddress;
    ULONG TransferLength;
    ULONG Offset;

	*Length = 0;
	*CurrentVa = NULL;
    LowAddress.QuadPart = 0;
    HighAddress.QuadPart = 0xffffffffffffffffI64;
    SkipAddress.QuadPart = 0;

    //
    // Add one MDL.
    //

    MdlHead = NULL;
    TransferLength = 0;

    {
        Span = 2;
        RequestBytes = Span << PAGE_SHIFT;

        if (ForceContiguous)
        {
            Mdl = MmAllocatePagesForMdlEx(LowAddress,
                                          HighAddress,
                                          SkipAddress,
                                          RequestBytes,
                                          MmCached,
                                          MM_ALLOCATE_REQUIRE_CONTIGUOUS_CHUNKS
                                          );
        }
        else
        {
            Mdl = MmAllocatePagesForMdl(LowAddress,
                                        HighAddress,
                                        SkipAddress,
                                        RequestBytes
                                        );
        }

        if (Mdl == NULL)
        {
            DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "MmAllocatePagesForMdl failed. \n" );
            goto Cleanup;
        }

        if (MmGetMdlByteCount(Mdl) < RequestBytes)
        {
            DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "MmAllocatePagesForMdl allocated less than requested bytes. \n" );
            goto Cleanup;
        }

        //
        // Set the byte count and byte offset appropriately for this MDL.
        // Note that this is safe since we allocated enough memory to cover
        // the entire page span up above.
        //

        Mdl->ByteCount = RequestBytes;
        Mdl->ByteOffset = 0;

        MappingAddress = MmMapLockedPagesSpecifyCache(
                                Mdl,
                                KernelMode,
                                MmCached,
                                NULL,
                                FALSE,
                                HighPagePriority);

        if (MappingAddress == NULL)
        {
            DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "MmMapLockedPagesSpecifyCache failed. \n" );
            goto Cleanup;
        }

        if (MdlHead == NULL)
        {
            MdlHead = Mdl;
        } 

        TransferLength += RequestBytes;
    }

    Offset = 0;

    *CurrentVa =
        (PVOID) (((ULONG_PTR) MmGetMdlVirtualAddress(MdlHead)) + Offset);

    *Length = TransferLength - Offset;

    return MdlHead;

    //
    // We'll branch to this point when we need to free the active MDL and
    // the MDL that we've built up to this point.
    //

Cleanup:

    if (Mdl != NULL)
    {
        MmFreePagesFromMdl(Mdl);
        ExFreePool(Mdl);
    }

	if (MdlHead != NULL)
	{
		SDmaFreeMdl(MdlHead);
	}

    return NULL;
}

_Function_class_(DRIVER_CONTROL)
IO_ALLOCATION_ACTION
SDmaAdapterControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the AdapterControl routine that we'll use to complete calls
    to AllocateAdapterChannelEx.

Arguments:

    DeviceObject - Supplies the FDO associated with this AllocateAdapterChannelEx
        call.

    Irp - Unused.

    MapRegisterBase - Supplies the handle to our allocated map registers.

    Context - Supplies the callback context that was passed to
        AllocateAdapterChannelEx.  In our case this will be a
        context structure.

Return Value:

    This function returns the Action parameter from the given callback
    context structure.

Environment:

    DISPATCH_LEVEL.

--*/

{
    IO_ALLOCATION_ACTION Action;
    PSDMA_CALLBACK_CONTEXT CallbackContext;

    UNREFERENCED_PARAMETER(Irp);
	UNREFERENCED_PARAMETER(DeviceObject);

    CallbackContext = (PSDMA_CALLBACK_CONTEXT) Context;

    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "AllocateAdapterChannelEx called back."
               " MapRegisterBase = 0x%IX\n", (ULONG_PTR)MapRegisterBase
        );

    //
    // Make a note of the allocated map register base and return the handle
    // associated with this map register allocation to the caller.
    //
    CallbackContext->MapRegisterBase    = MapRegisterBase;

    Action = CallbackContext->Action;

    KeSetEvent(&(CallbackContext->CallBackEvent), IO_NO_INCREMENT, FALSE);

    return Action;
}

_Function_class_(DMA_COMPLETION_ROUTINE)
VOID
SDmaCompletion(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context,
    IN DMA_COMPLETION_STATUS Status
    )

/*++

Routine Description:

    This is the DMA completion routine that we'll use to complete calls
    to MapTransferEx.

Arguments:

    DeviceObject - Supplies the FDO associated with this MapTransferEx
        call.

    Context - Supplies the callback context that was passed to
        MapTransferEx.  In our case this will be a
        context structure.

Return Value:

    None.

Environment:

    DISPATCH_LEVEL.

--*/

{
    PSDMA_CALLBACK_CONTEXT CallbackContext;

    UNREFERENCED_PARAMETER(DmaAdapter);
    UNREFERENCED_PARAMETER(DeviceObject);

    CallbackContext = (PSDMA_CALLBACK_CONTEXT) Context;

    DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "MapTransferEx called back. Status = %d\n",
        Status
        );

    KeSetEvent(&(CallbackContext->CompletionEvent), IO_NO_INCREMENT, FALSE);
}

NTSTATUS
SDmaWrite(
    _In_    PFDO_DEVICE_DATA pFdoData
    )

/*++

Routine Description:

    This function initiates simple system DMA against a
    non-existent DMA controller. The code is provided
	only for example purposes and is not intended as product
	code.

Arguments:

    pFdoData - Supplies the FDO extension

Return Value:

    This function returns a NTSTATUS value.

--*/

{
    KIRQL Irql;
    ULONG DmaRequestLine = 0x1000;
    NTSTATUS Status = STATUS_SUCCESS;
    PDMA_ADAPTER pWriteAdapter = NULL;
    ULONG AllocatableMapRegisters = 0, RequestedMapRegisters = 2;
    DEVICE_DESCRIPTION Description;
    SDMA_CALLBACK_CONTEXT WriteContext;
    CHAR WriteDmaTransferContext[ DMA_TRANSFER_CONTEXT_SIZE_V1 ];
    PMDL Mdl0 = NULL;
    PVOID CurrentVa = NULL;
    ULONG Length = 0;

    //
    // Allocate a system DMA adapter to write data to a device
    // (for instance the Tx register of a UART) using system DMA.
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    RtlZeroMemory(&Description, sizeof(Description));
    Description.Version           = DEVICE_DESCRIPTION_VERSION3;
    Description.DmaAddressWidth   = 32;
    Description.DmaRequestLine    = DmaRequestLine;
    Description.DmaChannel        = DmaRequestLine;
    Description.InterfaceType     = ACPIBus;
    Description.Master            = FALSE;
    Description.ScatterGather     = TRUE;
    Description.MaximumLength     = RequestedMapRegisters << PAGE_SHIFT;

    pWriteAdapter = IoGetDmaAdapter(pFdoData->UnderlyingPDO,
                                     &Description,
                                     &AllocatableMapRegisters);

    //
    // This is the expected return path on all systems, since no
    // system DMA controller supports request line 0x1000.
    //
    if ( pWriteAdapter == NULL )
    {
        DbgPrintEx(DPFLTR_IHVBUS_ID, 0, "IoGetDmaAdapter failed for request line %d. \n", Description.DmaRequestLine );
        Status = STATUS_NO_MATCH;
        goto Exit;
    }

    RtlZeroMemory( &WriteContext, sizeof( WriteContext ) );
    WriteContext.Action = KeepObject;
    WriteContext.NumberOfMapRegisters = AllocatableMapRegisters;
    KeInitializeEvent(&(WriteContext.CallBackEvent), NotificationEvent, FALSE);
    KeInitializeEvent(&(WriteContext.CompletionEvent), NotificationEvent, FALSE);

    pWriteAdapter->DmaOperations->InitializeDmaTransferContext(
        pWriteAdapter,
        (PVOID)&(WriteDmaTransferContext));

    DbgPrintEx(DPFLTR_IHVBUS_ID, 0,
        "%p WRITE V3 System DmaAdapter created: 32 bit, Scatter-Gather capable, %d MapRegs (%d requested)\n", 
        pWriteAdapter, AllocatableMapRegisters, RequestedMapRegisters
        );

    //
    // Call AllocateAdapterChannelEx for the write adapter.
    //
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);

    Status = pWriteAdapter->DmaOperations->AllocateAdapterChannelEx(
        pWriteAdapter,
        pFdoData->CommonData.Self,
        &(WriteDmaTransferContext),
        AllocatableMapRegisters,
        0,
        SDmaAdapterControl,
        (PVOID) &WriteContext,
        NULL);
    KeLowerIrql(Irql);

    if (Status != STATUS_SUCCESS)
    {
        goto Exit;
    }

    //
    // Wait for the channel to be allocated to our driver. A real driver would
    // not block this IRP to wait for AllocateAdapterChannelEx to call
    // back the AdapterControl routine. Instead all the steps below (including
    // the MapTransferEx) would be staged from with the AdapterControl
    // routine.
    //

    KeWaitForSingleObject(&(WriteContext.CallBackEvent), Executive, KernelMode, FALSE, NULL);

    //
    // We have acquired a channel for Write adapter successfully.
    //

    //
    // Build a fake MDL to represent user data.
    // Mdl0 is the address for the Write channel, and is the
    // data (likely from usermode in real world scenarios) that
    // will be written to the device address (this address may
    // be the Tx register of a UART device for instance).
    //

    Mdl0 = SDmaAllocateMdl(&CurrentVa, &Length, FALSE);

    //
    // Fill the MDL with 0xDA bytes.
    //

    SDmaPrintMdl( Mdl0, CurrentVa, Length, "Mdl0" );
    SDmaFillMdl( Mdl0, CurrentVa, Length, 0xDA );

    //
    // Signal the DMA engine to write the entire length of the
    // MDL using V3 system DMA.
    // We just use a 0 offset since our CurrentVa starts at the 
    // beginning of the MDL.
    //
    
    pWriteAdapter->DmaOperations->MapTransferEx(
        pWriteAdapter,
        Mdl0,
        WriteContext.MapRegisterBase,
        0,
        0x70006200, // Physical address of hardware
                    // device
                    // (e.g. UART Tx register)
        &Length,
        TRUE,
        NULL,
        0,
        SDmaCompletion,
        &WriteContext);

    //
    // Wait for DMA to complete on the channel. A real driver
    // would not block this IRP to wait but would instead handle the
    // FlushAdapterBuffersEx/FreeAdapterChannel steps during the DMA
    // completion routine.
    //

    KeWaitForSingleObject(&(WriteContext.CompletionEvent), Executive, KernelMode, FALSE, NULL);
    
    //
    // Copy the data in the map buffers (if any) back to the MDL.
    //
    
    pWriteAdapter->DmaOperations->FlushAdapterBuffersEx(
        pWriteAdapter,
        Mdl0,
        WriteContext.MapRegisterBase,
        0,
        Length,
        TRUE);

    //
    // Now we can free the MDL associated with this transfer.
    //

    SDmaFreeMdl(Mdl0);

    //
    // Return the channel back to the HAL. If this action is
    // not taken, then nothing else in the OS will be able to
    // use this channel again until it is freed. This includes
    // our own driver handling another IOCTL.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    pWriteAdapter->DmaOperations->FreeAdapterChannel( pWriteAdapter );
    KeLowerIrql(Irql);

Exit:

    //
    // Free the DMA adapter. Typically a driver will not Get and Put
    // a DMA adapter for each DMA transaction, but instead will perform
    // these actions once upon device driver construction and destruction.
    //

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    
    if (pWriteAdapter != NULL)
    {
        pWriteAdapter->DmaOperations->PutDmaAdapter(pWriteAdapter);
    }

    //
    // Send our return status.
    //

    return Status;
}

NTSTATUS
SDmaDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    PIO_STACK_LOCATION  irpSp;// Pointer to current stack location
    NTSTATUS            ntStatus = STATUS_SUCCESS;// Assume success
    ULONG               inBufLength; // Input buffer length
    ULONG               outBufLength; // Output buffer length
    PCHAR               inBuf; // pointer to Input buffer
    PCHAR               data = "This String is from Device Driver !!!";
    size_t              datalen = strlen(data)+1;//Length of data including null
    PFDO_DEVICE_DATA    fdoData;

    PAGED_CODE();

    fdoData = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!inBufLength || !outBufLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Determine which I/O control code was specified.
    //

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode )
    {
    case IOCTL_SDMA_WRITE:

        //
        // In this method the I/O manager allocates a buffer large enough to
        // to accommodate larger of the user input buffer and output buffer,
        // assigns the address to Irp->AssociatedIrp.SystemBuffer, and
        // copies the content of the user input buffer into this SystemBuffer
        //

        SDMA_KDPRINT(("Called IOCTL_SDMA_WRITE\n"));
        PrintIrpInfo(Irp);

        //
        // Input buffer and output buffer is same in this case, read the
        // content of the buffer before writing to it
        //

        inBuf = Irp->AssociatedIrp.SystemBuffer;
#if 0
        outBuf = Irp->AssociatedIrp.SystemBuffer;
#endif

        //
        // Read the data from the buffer
        //

        SDMA_KDPRINT(("\tData from User :"));
        //
        // We are using the following function to print characters instead
        // of DebugPrint with %s format because the string we get may or
        // may not be null terminated.
        //
        PrintChars(inBuf, inBufLength);

        //
        // Call the example V3 system DMA routine. The routine
        // is expected to fail because no system DMA controller
        // will match the one requested within the routine.
        //

        SDmaWrite( fdoData );

#if 0
        //
        // Write to the buffer over-writes the input buffer content
        //

        RtlCopyBytes(outBuf, data, outBufLength);

        SDMA_KDPRINT(("\tData to User : "));
        PrintChars(outBuf, datalen  );
#endif

        //
        // Assign the length of the data copied to IoStatus.Information
        // of the Irp and complete the Irp.
        //

        Irp->IoStatus.Information = (outBufLength<datalen?outBufLength:datalen);

        //
        // When the Irp is completed the content of the SystemBuffer
        // is copied to the User output buffer and the SystemBuffer is
        // is freed.
        //

       break;

    default:

        //
        // The specified I/O control code is unrecognized by this driver.
        //

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        SDMA_KDPRINT(("ERROR: unrecognized IOCTL %x\n",
            irpSp->Parameters.DeviceIoControl.IoControlCode));
        break;
    }

End:
    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    //

    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}

VOID
PrintIrpInfo(
    PIRP Irp)
{
    PIO_STACK_LOCATION  irpSp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    SDMA_KDPRINT(("\tIrp->AssociatedIrp.SystemBuffer = 0x%p\n",
        Irp->AssociatedIrp.SystemBuffer));
    SDMA_KDPRINT(("\tIrp->UserBuffer = 0x%p\n", Irp->UserBuffer));
    SDMA_KDPRINT(("\tirpSp->Parameters.DeviceIoControl.Type3InputBuffer = 0x%p\n",
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer));
    SDMA_KDPRINT(("\tirpSp->Parameters.DeviceIoControl.InputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.InputBufferLength));
    SDMA_KDPRINT(("\tirpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.OutputBufferLength ));

	UNREFERENCED_PARAMETER(irpSp);
    return;
}

VOID
PrintChars(
    _In_reads_(CountChars) PCHAR BufferAddress,
    _In_ size_t CountChars
    )
{
    PAGED_CODE();

    if (CountChars) {

        while (CountChars--) {

            if (*BufferAddress > 31
                 && *BufferAddress != 127) {

                KdPrint (( "%c", *BufferAddress) );

            } else {

                KdPrint(( ".") );

            }
            BufferAddress++;
        }
        KdPrint (("\n"));
    }
    return;
}



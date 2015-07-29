/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    NIC_INIT.c

Abstract:

    Contains rotuines to do resource allocation and hardware
    initialization & shutdown.

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if defined(EVENT_TRACING)
#include "nic_init.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NICAllocateSoftwareResources)
#pragma alloc_text (PAGE, NICFreeSoftwareResources)
#pragma alloc_text (PAGE, NICMapHWResources)
#pragma alloc_text (PAGE, NICUnmapHWResources)
#pragma alloc_text (PAGE, NICGetDeviceInformation)
#pragma alloc_text (PAGE, NICReadAdapterInfo)
#pragma alloc_text (PAGE, NICAllocAdapterMemory)
#pragma alloc_text (PAGE, NICFreeAdapterMemory)
#pragma alloc_text (PAGE, NICInitRecvBuffers)
#pragma alloc_text (PAGE, NICSelfTest)
#pragma alloc_text (PAGE, HwClearAllCounters)
#pragma alloc_text (PAGE, NICAllocRfd)
#pragma alloc_text (PAGE, NICFreeRfd)
#pragma alloc_text (PAGE, NICFreeRfdWorkItem)
#endif

PVOID LocalMmMapIoSpace(
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ SIZE_T NumberOfBytes
    )
{
    typedef
    PVOID
    (*PFN_MM_MAP_IO_SPACE_EX) (
        _In_ PHYSICAL_ADDRESS PhysicalAddress,
        _In_ SIZE_T NumberOfBytes,
        _In_ ULONG Protect
        );

    UNICODE_STRING         name;
    PFN_MM_MAP_IO_SPACE_EX pMmMapIoSpaceEx;

    RtlInitUnicodeString(&name, L"MmMapIoSpaceEx");
    pMmMapIoSpaceEx = (PFN_MM_MAP_IO_SPACE_EX) (ULONG_PTR)MmGetSystemRoutineAddress(&name);

    if (pMmMapIoSpaceEx != NULL){
        //
        // Call WIN10 API if available
        //        
        return pMmMapIoSpaceEx(PhysicalAddress,
                               NumberOfBytes,
                               PAGE_READWRITE | PAGE_NOCACHE); 
    }

    //
    // Supress warning that MmMapIoSpace allocates executable memory.
    // This function is only used if the preferred API, MmMapIoSpaceEx
    // is not present. MmMapIoSpaceEx is available starting in WIN10.
    //
    #pragma warning(suppress: 30029)
    return MmMapIoSpace(PhysicalAddress, NumberOfBytes, MmNonCached); 
}

NTSTATUS
NICAllocateSoftwareResources(
    IN OUT PFDO_DATA FdoData
    )
/*++
Routine Description:

    This routine creates two parallel queues and 3 manual queues to hold
    Read, Write and IOCTL requests. It also creates the interrupt object and
    DMA object, and performs some additional initialization.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{
    NTSTATUS                        status;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig;
    WDF_DMA_ENABLER_CONFIG          dmaConfig;
    ULONG                           maximumLength, maxLengthSupported;
    WDF_OBJECT_ATTRIBUTES           attributes;
    ULONG                           maxMapRegistersRequired, miniMapRegisters;
    ULONG                           mapRegistersAllocated;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "-->NICAllocateSoftwareResources\n");

    PAGED_CODE();

    //
    // Initialize all the static data first to make sure we don't touch
    // uninitialized list in the ContextCleanup callback if the
    // AddDevice fails for any reason.
    //
    InitializeListHead(&FdoData->PoMgmt.PatternList);
    InitializeListHead(&FdoData->RecvList);

    //
    // This a global lock, to synchonize access to device context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = FdoData->WdfDevice;
    status = WdfSpinLockCreate(&attributes,&FdoData->Lock);
    if(!NT_SUCCESS(status)){
        return status;
    }


    //
    // Get the BUS_INTERFACE_STANDARD for our device so that we can
    // read & write to PCI config space.
    //
    status = WdfFdoQueryForInterface(FdoData->WdfDevice,
                                   &GUID_BUS_INTERFACE_STANDARD,
                                   (PINTERFACE) &FdoData->BusInterface,
                                   sizeof(BUS_INTERFACE_STANDARD),
                                   1, // Version
                                   NULL); //InterfaceSpecificData
    if (!NT_SUCCESS (status)){
        return status;
    }

    //
    // First make sure this is our device before doing whole lot
    // of other things.
    //
    status = NICGetDeviceInformation(FdoData);
    if (!NT_SUCCESS (status)){
        return status;
    }

    NICGetDeviceInfSettings(FdoData);

    //
    // We will create and configure a queue for receiving
    // write requests. If these requests have to be pended for any
    // reason, they will be forwarded to a manual queue created after this one.
    // Framework automatically takes the responsibility of handling
    // cancellation when the requests are waiting in the queue. This is
    // a managed queue. So the framework will take care of queueing
    // incoming requests when the pnp/power state transition takes place.
    // Since we have configured the queue to dispatch all the specific requests
    // we care about, we don't need a default queue. A default queue is
    // used to receive requests that are not preconfigured to go to
    // a specific queue.
    //
    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchParallel
    );

    ioQueueConfig.EvtIoWrite = PciDrvEvtIoWrite;

    status = WdfIoQueueCreate(
                 FdoData->WdfDevice,
                 &ioQueueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &FdoData->WriteQueue // queue handle
             );

    if (!NT_SUCCESS (status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfIoQueueCreate failed 0x%x\n", status);
        return status;
    }

     status = WdfDeviceConfigureRequestDispatching(
                    FdoData->WdfDevice,
                    FdoData->WriteQueue,
                    WdfRequestTypeWrite);

    if(!NT_SUCCESS (status)){
        ASSERT(NT_SUCCESS(status));
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Error in config'ing write Queue 0x%x\n", status);
        return status;
    }

    //
    // Manual internal queue for write reqeusts. This will be used to queue
    // the write requests presented to us from the parallel default queue
    // when we are low in TCB resources or when the device
    // is busy doing link detection.
    // Requests can be canceled while waiting in the queue without any
    // notification to the driver.
    //
    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchManual
        );

    status = WdfIoQueueCreate (
                   FdoData->WdfDevice,
                   &ioQueueConfig,
                   WDF_NO_OBJECT_ATTRIBUTES,
                   &FdoData->PendingWriteQueue
                   );

    if(!NT_SUCCESS (status)){
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Error Creating manual write Queue 0x%x\n", status);
        return status;
    }


    //
    // Manual queue for read requests (WdfRequestTypeRead). We will configure the queue
    // so that incoming read requests are directly dispatched to this queue. We will
    // manually remove the requests from the queue and service them in our recv
    // interrupt handler.  WDF_IO_QUEUE_CONFIG_INIT initializes queues to be
    // auto managed by default.
    //
    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchManual
        );

    status = WdfIoQueueCreate (
                   FdoData->WdfDevice,
                   &ioQueueConfig,
                   WDF_NO_OBJECT_ATTRIBUTES,
                   &FdoData->PendingReadQueue
                   );

    if(!NT_SUCCESS (status)){
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Error Creating read Queue 0x%x\n", status);
        return status;
    }

    status = WdfDeviceConfigureRequestDispatching(
                    FdoData->WdfDevice,
                    FdoData->PendingReadQueue,
                    WdfRequestTypeRead);

    if(!NT_SUCCESS (status)){
        ASSERT(NT_SUCCESS(status));
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Error in config'ing read Queue 0x%x\n", status);
        return status;
    }


    //
    // Parallel queue for device I/O control (WdfRequestTypeDeviceControl) requests.
    // We will configure the queue so that all the incoming ioctl requests
    // go directly to this queue. We will try to service the requests immediately.
    // If we can't, we will forward the request to a manual queue created below
    // and try to service it from a DPC when the appropriate event happens.
    //
    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchParallel
        );

    ioQueueConfig.EvtIoDeviceControl = PciDrvEvtIoDeviceControl;

    status = WdfIoQueueCreate (
                   FdoData->WdfDevice,
                   &ioQueueConfig,
                   WDF_NO_OBJECT_ATTRIBUTES,
                   &FdoData->IoctlQueue
                   );

    if(!NT_SUCCESS (status)){
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Error Creating ioctl Queue 0x%x\n", status);
        return status;
    }

    status = WdfDeviceConfigureRequestDispatching(
                    FdoData->WdfDevice,
                    FdoData->IoctlQueue,
                    WdfRequestTypeDeviceControl);

    if(!NT_SUCCESS (status)){
        ASSERT(NT_SUCCESS(status));
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Error in config'ing ioctl Queue 0x%x\n", status);
        return status;
    }

    //
    // Manual internal queue for device I/O control requests. This will be used to
    // queue the ioctl requests presented to us from the parallel ioctl queue
    // when we cannot handle them immediately. This is a managed queue.
    // Requests be get canceled while waiting in the queue without any
    // notification to the driver.
    //

    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchManual
        );

    status = WdfIoQueueCreate (
                   FdoData->WdfDevice,
                   &ioQueueConfig,
                   WDF_NO_OBJECT_ATTRIBUTES,
                   &FdoData->PendingIoctlQueue
                   );

    if(!NT_SUCCESS (status)){
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                    "Error Creating manual Ioctl Queue 0x%x\n", status);
        return status;
    }

#ifndef PCIDRV_CREATE_INTERRUPT_IN_PREPARE_HARDWARE
    {
        WDF_INTERRUPT_CONFIG interruptConfig;
        
        //
        // Create WDFINTERRUPT object.
        //
        WDF_INTERRUPT_CONFIG_INIT(&interruptConfig,
                                  NICEvtInterruptIsr,
                                  NICEvtInterruptDpc);

        //
        // These first two callbacks will be called at DIRQL.  Their job is to
        // enable and disable interrupts.
        //
        interruptConfig.EvtInterruptEnable  = NICEvtInterruptEnable;
        interruptConfig.EvtInterruptDisable = NICEvtInterruptDisable;

        status = WdfInterruptCreate(FdoData->WdfDevice,
                            &interruptConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &FdoData->WdfInterrupt);

        if (!NT_SUCCESS (status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                        "WdfInterruptCreate failed: %!STATUS!\n", status);
            return status;
        }
    }
#endif

    //
    // Alignment requirement must be 16-byte for this device. This alignment
    // value will be inherits by the DMA enabler and used when you allocate
    // common buffers.
    //
    WdfDeviceSetAlignmentRequirement( FdoData->WdfDevice, FILE_OCTA_ALIGNMENT);

    //
    // Bare minimum number of map registers required to do
    // a single NIC_MAX_PACKET_SIZE transfer.
    //
    miniMapRegisters = BYTES_TO_PAGES(NIC_MAX_PACKET_SIZE) + 1;

    //
    // Maximum map registers required to do simultaneous transfer
    // of all TCBs assuming each packet spanning NIC_MAX_PHYS_BUF_COUNT
    // Buffer can span multiple MDLs.
    //
    maxMapRegistersRequired = FdoData->NumTcb * NIC_MAX_PHYS_BUF_COUNT;

    //
    // The maximum length of buffer for maxMapRegistersRequired number of
    // map registers would be.
    //
    maximumLength = (maxMapRegistersRequired-1) << PAGE_SHIFT;

    //
    // Create a new DMA Object for Scatter/Gather DMA mode.
    //

    WDF_DMA_ENABLER_CONFIG_INIT( &dmaConfig,
                                 WdfDmaProfileScatterGather,
                                 maximumLength );

    status = WdfDmaEnablerCreate( FdoData->WdfDevice,
                                  &dmaConfig,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  &FdoData->WdfDmaEnabler );

    if (!NT_SUCCESS (status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                    "WdfDmaEnblerCreate failed: %08X\n", status);
        return status;
    }

    maxLengthSupported = (ULONG) WdfDmaEnablerGetFragmentLength(FdoData->WdfDmaEnabler,
                                                        WdfDmaDirectionReadFromDevice);

    mapRegistersAllocated = BYTES_TO_PAGES(maxLengthSupported) + 1;

    if(mapRegistersAllocated < miniMapRegisters) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                    "Not enough map registers: Allocated %d, Required %d\n",
                    mapRegistersAllocated, miniMapRegisters);
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Adjust our TCB count based on the MapRegisters we got. We will
    // take the best case scenario where the packet is going to span
    // no more than 2 pages.
    //
    FdoData->NumTcb = mapRegistersAllocated/miniMapRegisters;

    //
    // Make sure it doesn't exceed NIC_MAX_TCBS.
    //
    FdoData->NumTcb = min(FdoData->NumTcb, NIC_MAX_TCBS);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "MapRegisters Allocated %d\n", mapRegistersAllocated);
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "Adjusted TCB count is %d\n", FdoData->NumTcb);

    //
    // Set the maximum allowable DMA Scatter/Gather list fragmentation size.
    //
    WdfDmaEnablerSetMaximumScatterGatherElements( FdoData->WdfDmaEnabler,
                                                  NIC_MAX_PHYS_BUF_COUNT );

    //
    // Create a lock to protect all the write-related buffer lists.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = FdoData->WdfDevice;
    status = WdfSpinLockCreate(&attributes,&FdoData->SendLock);
    if(!NT_SUCCESS(status)){
        return status;
    }

    //
    // Create a lock to protect all the read-related buffer lists
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = FdoData->WdfDevice;
    status = WdfSpinLockCreate(&attributes,&FdoData->RcvLock);
    if(!NT_SUCCESS(status)){
        return status;
    }

    status = NICAllocAdapterMemory(FdoData);

    if (NT_SUCCESS(status)) {

        //
        // This sets up send buffers.  It doesn't actually touch hardware.
        //

        NICInitSendBuffers(FdoData);

        //
        // This sets up receive buffers.  It doesn't actually touch hardware.
        //

        status = NICInitRecvBuffers(FdoData);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "<-- NICAllocateSoftwareResources\n");

    return status;
}


NTSTATUS
NICFreeSoftwareResources(
    IN OUT PFDO_DATA FdoData
    )
/*++
Routine Description:

    Free all the software resources. We shouldn't touch the hardware.
    This functions is called in the context of EvtDeviceContextCleanup.
    Most of the resources created in NICAllocateResources such as queues,
    DMA enabler, SpinLocks, CommonBuffer, are already freed by
    framework because they are associated with the WDFDEVICE directly
    or indirectly as child objects.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "-->NICFreeSoftwareResources\n");

    PAGED_CODE();

    NICFreeAdapterMemory(FdoData);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "<--NICFreeSoftwareResources\n");

    return STATUS_SUCCESS;

}

NTSTATUS
NICMapHWResources(
    IN OUT PFDO_DATA FdoData,
    IN WDFCMRESLIST  ResourcesRaw,
    IN WDFCMRESLIST  ResourcesTranslated
    )
/*++
Routine Description:

    Gets the HW resources assigned by the bus driver and:
    1) Maps them to system address space. 
    2) If PCIDRV_CREATE_INTERRUPT_IN_PREPARE_HARDWARE is defined, 
        it creates a WDFINTERRUPT object.

    Called during EvtDevicePrepareHardware callback.

    Three base address registers are supported by the 8255x:
    1) CSR Memory Mapped Base Address Register (BAR 0 at offset 10)
    2) CSR I/O Mapped Base Address Register (BAR 1 at offset 14)
    3) Flash Memory Mapped Base Address Register (BAR 2 at offset 18)

    The 8255x requires one BAR for I/O mapping and one BAR for memory
    mapping of these registers anywhere within the 32-bit memory address space.
    The driver determines which BAR (I/O or Memory) is used to access the
    Control/Status Registers.

    Just for illustration, this driver maps both memory and I/O registers and
    shows how to use READ_PORT_xxx or READ_REGISTER_xxx functions to perform
    I/O in a platform independent basis. On some platforms, the I/O registers
    can get mapped into memory space and your driver should be able to handle
    this transparently.

    One BAR is also required to map the accesses to an optional Flash memory.
    The 82557 implements this register regardless of the presence or absence
    of a Flash chip on the adapter. The 82558 and 82559 implement this
    register only if a bit is set in the EEPROM. The size of the space requested
    by this register is 1Mbyte, and it is always mapped anywhere in the 32-bit
    memory address space.

    Note: Although the 82558 only supports up to 64 Kbytes of Flash memory
    and the 82559 only supports 128 Kbytes of Flash memory, the driver
    requests 1 Mbyte of address space. Software should not access Flash
    addresses above 64 Kbytes for the 82558 or 128 Kbytes for the 82559
    because Flash accesses above the limits are aliased to lower addresses.

Arguments:

    FdoData     Pointer to our FdoData
    ResourcesRaw - Pointer to list of raw resources passed to 
                        EvtDevicePrepareHardware callback
    ResourcesTranslated - Pointer to list of translated resources passed to
                        EvtDevicePrepareHardware callback

Return Value:

    NTSTATUS

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    ULONG       i;
    NTSTATUS    status = STATUS_SUCCESS;
    BOOLEAN     bResPort      = FALSE;
    BOOLEAN     bResInterrupt = FALSE;
    BOOLEAN     bResMemory    = FALSE;
    ULONG       numberOfBARs  = 0;

    UNREFERENCED_PARAMETER(ResourcesRaw);

    PAGED_CODE();

    for (i=0; i<WdfCmResourceListGetCount(ResourcesTranslated); i++) {

        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        if(!descriptor){
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfResourceCmGetDescriptor");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        switch (descriptor->Type) {

        case CmResourceTypePort:
            //
            // We will increment the BAR count only for valid resources. We will
            // not count the private device types added by the PCI bus driver.
            //
            numberOfBARs++;

            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "I/O mapped CSR: (%x) Length: (%d)\n",
                descriptor->u.Port.Start.LowPart,
                descriptor->u.Port.Length);

            //
            // The resources are listed in the same order the as
            // BARs in the config space, so this should be the second one.
            //
            if(numberOfBARs != 2) {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "I/O mapped CSR is not in the right order\n");
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
                return status;
            }

            //
            // The port is in I/O space on this machine.
            // We should use READ_PORT_Xxx, and WRITE_PORT_Xxx routines
            // to read or write to the port.
            //

            FdoData->IoBaseAddress = ULongToPtr(descriptor->u.Port.Start.LowPart);
            FdoData->IoRange = descriptor->u.Port.Length;
            //
            // Since all our accesses are USHORT wide, we will create an accessor
            // table just for these two functions.
            //
            FdoData->ReadPort = NICReadPortUShort;
            FdoData->WritePort = NICWritePortUShort;

            bResPort = TRUE;
            FdoData->MappedPorts = FALSE;
            break;

        case CmResourceTypeMemory:

            numberOfBARs++;

            if(numberOfBARs == 1) {
                TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Memory mapped CSR:(%x:%x) Length:(%d)\n",
                                        descriptor->u.Memory.Start.LowPart,
                                        descriptor->u.Memory.Start.HighPart,
                                        descriptor->u.Memory.Length);
                //
                // Our CSR memory space should be 0x1000 in size.
                //
                ASSERT(descriptor->u.Memory.Length == 0x1000);
                FdoData->MemPhysAddress = descriptor->u.Memory.Start;
                FdoData->CSRAddress = LocalMmMapIoSpace(
                                                descriptor->u.Memory.Start,
                                                NIC_MAP_IOSPACE_LENGTH);
                TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "CSRAddress=%p\n", FdoData->CSRAddress);

                bResMemory = TRUE;

            } else if(numberOfBARs == 2){

                TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                    "I/O mapped CSR in Memory Space: (%x) Length: (%d)\n",
                    descriptor->u.Memory.Start.LowPart,
                    descriptor->u.Memory.Length);
                //
                // The port is in memory space on this machine.
                // We should call LocalMmMapIoSpace to map the physical to virtual
                // address, and also use the READ/WRITE_REGISTER_xxx function
                // to read or write to the port.
                //

                FdoData->IoBaseAddress = LocalMmMapIoSpace(
                                                descriptor->u.Memory.Start,
                                                descriptor->u.Memory.Length);

                FdoData->ReadPort = NICReadRegisterUShort;
                FdoData->WritePort = NICWriteRegisterUShort;
                FdoData->MappedPorts = TRUE;
                bResPort = TRUE;

            } else if(numberOfBARs == 3){

                TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Flash memory:(%x:%x) Length:(%d)\n",
                                        descriptor->u.Memory.Start.LowPart,
                                        descriptor->u.Memory.Start.HighPart,
                                        descriptor->u.Memory.Length);
                //
                // Our flash memory should be 1MB in size. Since we don't
                // access the memory, let us not bother mapping it.
                //
                //ASSERT(descriptor->u.Memory.Length == 0x100000);
            } else {
                TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                            "Memory Resources are not in the right order\n");
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
                return status;
            }

            break;

        case CmResourceTypeInterrupt:

            ASSERT(!bResInterrupt);
            
#ifdef PCIDRV_CREATE_INTERRUPT_IN_PREPARE_HARDWARE
            {
                WDF_INTERRUPT_CONFIG interruptConfig;
                
                //
                // Create WDFINTERRUPT object.
                //
                WDF_INTERRUPT_CONFIG_INIT(&interruptConfig,
                                          NICEvtInterruptIsr,
                                          NICEvtInterruptDpc);

                //
                // These first two callbacks will be called at DIRQL.  Their job is to
                // enable and disable interrupts.
                //
                interruptConfig.EvtInterruptEnable  = NICEvtInterruptEnable;
                interruptConfig.EvtInterruptDisable = NICEvtInterruptDisable;
                interruptConfig.InterruptTranslated = descriptor;
                interruptConfig.InterruptRaw = 
                       WdfCmResourceListGetDescriptor(ResourcesRaw, i);  

                status = WdfInterruptCreate(FdoData->WdfDevice,
                                    &interruptConfig,
                                    WDF_NO_OBJECT_ATTRIBUTES,
                                    &FdoData->WdfInterrupt);

                if (!NT_SUCCESS (status))
                {
                    TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                                "WdfInterruptCreate failed: %!STATUS!\n", status);
                    return status;
                }
            }
#endif

            bResInterrupt = TRUE;

            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "Interrupt level: 0x%0x, Vector: 0x%0x\n",
                descriptor->u.Interrupt.Level,
                descriptor->u.Interrupt.Vector);

            break;

        default:
            //
            // This could be device-private type added by the PCI bus driver. We
            // shouldn't filter this or change the information contained in it.
            //
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Unhandled resource type (0x%x)\n",
                                        descriptor->Type);
            break;
        }

    }

    //
    // Make sure we got all the 3 resources to work with.
    //
    if (!(bResPort && bResInterrupt && bResMemory)) {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        return status;
    }

    //
    // Read additional info from NIC such as MAC address
    //
    status = NICReadAdapterInfo(FdoData);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    //
    // Test our adapter hardware
    //
    status = NICSelfTest(FdoData);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return status;

}

NTSTATUS
NICUnmapHWResources(
    IN OUT PFDO_DATA FdoData
    )
/*++
Routine Description:

    Disconnect the interrupt and unmap all the memory and I/O resources.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{
    PAGED_CODE();

    //
    // Free hardware resources
    //
    if (FdoData->CSRAddress)
    {
        MmUnmapIoSpace(FdoData->CSRAddress, NIC_MAP_IOSPACE_LENGTH);
        FdoData->CSRAddress = NULL;
    }

    if(FdoData->MappedPorts){
        MmUnmapIoSpace(FdoData->IoBaseAddress, FdoData->IoRange);
        FdoData->IoBaseAddress = NULL;
    }

    return STATUS_SUCCESS;

}


NTSTATUS
NICGetDeviceInformation(
    IN PFDO_DATA FdoData
    )
/*++
Routine Description:

    This function reads the PCI config space and make sure that it's our
    device and stores the device IDs and power information in the device
    extension. Should be done in the StartDevice.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR buffer[NIC_PCI_E100_HDR_LENGTH ];
    PPCI_COMMON_CONFIG  pPciConfig = (PPCI_COMMON_CONFIG) buffer;
    USHORT              usPciCommand;
    ULONG               bytesRead =0;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "---> NICGetDeviceInformation\n");

    PAGED_CODE();

    RtlZeroMemory(buffer, sizeof(buffer));
    bytesRead = FdoData->BusInterface.GetBusData(
                        FdoData->BusInterface.Context,
                         PCI_WHICHSPACE_CONFIG, //READ
                         buffer,
                         FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                         NIC_PCI_E100_HDR_LENGTH);

    if (bytesRead != NIC_PCI_E100_HDR_LENGTH) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                        "GetBusData (NIC_PCI_E100_HDR_LENGTH) failed =%d\n",
                         bytesRead);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Is this our device?
    //

    if (pPciConfig->VendorID != NIC_PCI_VENDOR_ID ||
        pPciConfig->DeviceID != NIC_PCI_DEVICE_ID)
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                        "VendorID/DeviceID don't match - %x/%x\n",
                        pPciConfig->VendorID, pPciConfig->DeviceID);
        //return STATUS_DEVICE_DOES_NOT_EXIST;

    }

    //
    // save TRACE_LEVEL_INFORMATION from config space
    //
    FdoData->RevsionID = pPciConfig->RevisionID;
    FdoData->SubVendorID = pPciConfig->u.type0.SubVendorID;
    FdoData->SubSystemID = pPciConfig->u.type0.SubSystemID;

    NICExtractPMInfoFromPciSpace (FdoData, (PUCHAR)pPciConfig);

    usPciCommand = pPciConfig->Command;

    if ((usPciCommand & PCI_ENABLE_WRITE_AND_INVALIDATE) && (FdoData->MWIEnable)){
        FdoData->MWIEnable = TRUE;
    } else {
        FdoData->MWIEnable = FALSE;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICGetDeviceInformation\n");

    return status;
}

NTSTATUS
NICReadAdapterInfo(
    IN PFDO_DATA FdoData
    )
/*++
Routine Description:

    Read the mac addresss from the adapter

Arguments:

    FdoData     Pointer to our device context

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS     status = STATUS_SUCCESS;
    USHORT          usValue;
    int             i;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICReadAdapterInfo\n");

    PAGED_CODE();

    FdoData->EepromAddressSize = GetEEpromAddressSize(
            GetEEpromSize(FdoData, (PUCHAR)FdoData->IoBaseAddress));

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "EepromAddressSize = %d\n",
                                FdoData->EepromAddressSize);


    //
    // Read node address from the EEPROM
    //
    for (i=0; i< ETH_LENGTH_OF_ADDRESS; i += 2)
    {
        usValue = ReadEEprom(FdoData, (PUCHAR)FdoData->IoBaseAddress,
                      (USHORT)(EEPROM_NODE_ADDRESS_BYTE_0 + (i/2)),
                      FdoData->EepromAddressSize);

        *((PUSHORT)(&FdoData->PermanentAddress[i])) = usValue;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
        "Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n",
        FdoData->PermanentAddress[0], FdoData->PermanentAddress[1],
        FdoData->PermanentAddress[2], FdoData->PermanentAddress[3],
        FdoData->PermanentAddress[4], FdoData->PermanentAddress[5]);

    if (ETH_IS_MULTICAST(FdoData->PermanentAddress) ||
        ETH_IS_BROADCAST(FdoData->PermanentAddress))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Permanent address is invalid\n");

        status = STATUS_INVALID_ADDRESS;
    }
    else
    {
        if (!FdoData->bOverrideAddress)
        {
            ETH_COPY_NETWORK_ADDRESS(FdoData->CurrentAddress, FdoData->PermanentAddress);
        }

        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
            "Current Address = %02x-%02x-%02x-%02x-%02x-%02x\n",
            FdoData->CurrentAddress[0], FdoData->CurrentAddress[1],
            FdoData->CurrentAddress[2], FdoData->CurrentAddress[3],
            FdoData->CurrentAddress[4], FdoData->CurrentAddress[5]);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICReadAdapterInfo, status=%x\n",
                    status);

    return status;
}



NTSTATUS
NICAllocAdapterMemory(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Allocate all the memory blocks for send, receive and others

Arguments:

    FdoData     Pointer to our adapter

Return Value:


--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PUCHAR          pMem;
    ULONG           MemPhys;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICAllocAdapterMemory\n");

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "NumTcb=%d\n", FdoData->NumTcb);

    do
    {
        //
        // Send + Misc
        //
        //
        // Allocate MP_TCB's
        //
        status = RtlULongMult(FdoData->NumTcb,
                         sizeof(MP_TCB),
                         &FdoData->MpTcbMemSize);
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                    "RtlUlongMult failed 0x%x\n", status);
            break;
        }

        pMem = ExAllocatePoolWithTag(NonPagedPool,
                            FdoData->MpTcbMemSize, PCIDRV_POOL_TAG);
        if (NULL == pMem )
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "Failed to allocate MP_TCB's\n");
            break;
        }

        RtlZeroMemory(pMem, FdoData->MpTcbMemSize);
        FdoData->MpTcbMem = pMem;

        // HW_START

        //
        // Allocate shared memory for send
        //
        FdoData->HwSendMemAllocSize = FdoData->NumTcb * (sizeof(TXCB_STRUC) +
                                      NIC_MAX_PHYS_BUF_COUNT * sizeof(TBD_STRUC));

        _Analysis_assume_(FdoData->HwSendMemAllocSize > 0);
        status = WdfCommonBufferCreate( FdoData->WdfDmaEnabler,
                                        FdoData->HwSendMemAllocSize,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &FdoData->WdfSendCommonBuffer );

        if (status != STATUS_SUCCESS)
        {
            FdoData->HwSendMemAllocSize = 0;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfCommonBufferCreate(Send) "
                                        "failed %08X\n", status );
            break;
        }

        FdoData->HwSendMemAllocVa = WdfCommonBufferGetAlignedVirtualAddress(
                                               FdoData->WdfSendCommonBuffer);

        FdoData->HwSendMemAllocLa = WdfCommonBufferGetAlignedLogicalAddress(
                                               FdoData->WdfSendCommonBuffer);

        RtlZeroMemory(FdoData->HwSendMemAllocVa,
                      FdoData->HwSendMemAllocSize);

        //
        // Allocate shared memory for other uses
        //
        // FIXME-NOTE: The WdfCommonBufferGetAlignedVirtualAddress functions
        //             return device-specified aligned pointers...use them.
        //
        FdoData->HwMiscMemAllocSize =
            sizeof(SELF_TEST_STRUC) + ALIGN_16 +
            sizeof(DUMP_AREA_STRUC) + ALIGN_16 +
            sizeof(NON_TRANSMIT_CB) + ALIGN_16 +
            sizeof(ERR_COUNT_STRUC) + ALIGN_16;

        //
        // Allocate the shared memory for the command block data structures.
        //
        status = WdfCommonBufferCreate( FdoData->WdfDmaEnabler,
                                        FdoData->HwMiscMemAllocSize,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &FdoData->WdfMiscCommonBuffer );

        if (status != STATUS_SUCCESS)
        {
            FdoData->HwMiscMemAllocSize = 0;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfCommonBufferCreate(Misc) "
                                        "failed %08X\n", status );
            break;
        }

        FdoData->HwMiscMemAllocVa = WdfCommonBufferGetAlignedVirtualAddress(
                                               FdoData->WdfMiscCommonBuffer);

        FdoData->HwMiscMemAllocLa = WdfCommonBufferGetAlignedLogicalAddress(
                                               FdoData->WdfMiscCommonBuffer);

        RtlZeroMemory(FdoData->HwMiscMemAllocVa,
                      FdoData->HwMiscMemAllocSize);

        pMem = FdoData->HwMiscMemAllocVa;
        MemPhys = FdoData->HwMiscMemAllocLa.LowPart ;

        FdoData->SelfTest = (PSELF_TEST_STRUC)MP_ALIGNMEM(pMem, ALIGN_16);
        FdoData->SelfTestPhys = MP_ALIGNMEM_PHYS(MemPhys, ALIGN_16);
        pMem = (PUCHAR)FdoData->SelfTest + sizeof(SELF_TEST_STRUC);
        MemPhys = FdoData->SelfTestPhys + sizeof(SELF_TEST_STRUC);

        FdoData->NonTxCmdBlock = (PNON_TRANSMIT_CB)MP_ALIGNMEM(pMem, ALIGN_16);
        FdoData->NonTxCmdBlockPhys = MP_ALIGNMEM_PHYS(MemPhys, ALIGN_16);
        pMem = (PUCHAR)FdoData->NonTxCmdBlock + sizeof(NON_TRANSMIT_CB);
        MemPhys = FdoData->NonTxCmdBlockPhys + sizeof(NON_TRANSMIT_CB);

        FdoData->DumpSpace = (PDUMP_AREA_STRUC)MP_ALIGNMEM(pMem, ALIGN_16);
        FdoData->DumpSpacePhys = MP_ALIGNMEM_PHYS(MemPhys, ALIGN_16);
        pMem = (PUCHAR)FdoData->DumpSpace + sizeof(DUMP_AREA_STRUC);
        MemPhys = FdoData->DumpSpacePhys + sizeof(DUMP_AREA_STRUC);

        FdoData->StatsCounters = (PERR_COUNT_STRUC)MP_ALIGNMEM(pMem, ALIGN_16);
        FdoData->StatsCounterPhys = MP_ALIGNMEM_PHYS(MemPhys, ALIGN_16);

        // HW_END

        //
        // Recv
        //

        // set the max number of RFDs
        // disable the RFD grow/shrink scheme if user specifies a NumRfd value
        // larger than NIC_MAX_GROW_RFDS
        FdoData->MaxNumRfd = max(FdoData->NumRfd, NIC_MAX_GROW_RFDS);

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "NumRfd = %d\n", FdoData->NumRfd);
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "MaxNumRfd = %d\n", FdoData->MaxNumRfd);

        //
        // The driver should allocate more data than sizeof(RFD_STRUC) to allow the
        // driver to align the data(after ethernet header) at 8 byte boundary
        //
        FdoData->HwRfdSize = sizeof(RFD_STRUC) + MORE_DATA_FOR_ALIGN;

        status = STATUS_SUCCESS;

    } WHILE( FALSE );

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "<-- NICAllocAdapterMemory, status=%x\n", status);

    return status;

}

VOID
NICFreeAdapterMemory(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Free all the resources and MP_ADAPTER data block

Arguments:

    FdoData     Pointer to our adapter

Return Value:

    None

--*/
{
    PMP_RFD         pMpRfd;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICFreeAdapterMemory\n");

    PAGED_CODE();

    // No active and waiting sends
    ASSERT(FdoData->nBusySend == 0);
    ASSERT(FdoData->nWaitSend == 0);

    ASSERT(FdoData->nReadyRecv == FdoData->CurrNumRfd);

    while (!IsListEmpty(&FdoData->RecvList))
    {
        pMpRfd = (PMP_RFD)RemoveHeadList(&FdoData->RecvList);

        pMpRfd->DeleteCommonBuffer = FALSE;

        NICFreeRfd(FdoData, pMpRfd);
    }

    FdoData->WdfSendCommonBuffer = NULL;
    FdoData->HwSendMemAllocVa = NULL;

    FdoData->WdfMiscCommonBuffer = NULL;
    FdoData->HwMiscMemAllocVa = NULL;
    FdoData->SelfTest = NULL;
    FdoData->StatsCounters = NULL;
    FdoData->NonTxCmdBlock = NULL;
    FdoData->DumpSpace = NULL;

    // Free the memory for MP_TCB structures
    if (FdoData->MpTcbMem)
    {
        ExFreePoolWithTag(FdoData->MpTcbMem, PCIDRV_POOL_TAG);
        FdoData->MpTcbMem = NULL;
    }

    //Free all the wake up patterns on this adapter
    NICRemoveAllWakeUpPatterns(FdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICFreeAdapterMemory\n");
}



VOID
NICInitSendBuffers(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Initialize send data structures. Can be called at DISPATCH_LEVEL.

Arguments:

    FdoData - Pointer to our adapter context

Return Value:

    None

--*/
{
    PMP_TCB         pMpTcb;
    PHW_TCB         pHwTcb;
    ULONG           HwTcbPhys;
    ULONG           TcbCount;

    PTBD_STRUC      pHwTbd;
    ULONG           HwTbdPhys;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICInitSendBuffers\n");

    FdoData->TransmitIdle = TRUE;
    FdoData->ResumeWait = TRUE;

    // Setup the initial pointers to the SW and HW TCB data space
    pMpTcb = (PMP_TCB) FdoData->MpTcbMem;
    pHwTcb = (PHW_TCB) FdoData->HwSendMemAllocVa;
    HwTcbPhys = FdoData->HwSendMemAllocLa.LowPart;

    // Setup the initial pointers to the TBD data space.
    // TBDs are located immediately following the TCBs
    pHwTbd = (PTBD_STRUC) (FdoData->HwSendMemAllocVa +
                 (sizeof(TXCB_STRUC) * FdoData->NumTcb));
    HwTbdPhys = HwTcbPhys + (sizeof(TXCB_STRUC) * FdoData->NumTcb);

    _Analysis_assume_(FdoData->HwSendMemAllocSize >= (FdoData->NumTcb * sizeof(HW_TCB)));
    _Analysis_assume_(FdoData->MpTcbMemSize >= (FdoData->NumTcb * sizeof(MP_TCB)));

    // Go through and set up each TCB
    for (TcbCount = 0; TcbCount < FdoData->NumTcb; TcbCount++)
    {
        pMpTcb->HwTcb = pHwTcb;                 // save ptr to HW TCB
        pMpTcb->HwTcbPhys = HwTcbPhys;      // save HW TCB physical address
        pMpTcb->HwTbd = pHwTbd;                 // save ptr to TBD array
        pMpTcb->HwTbdPhys = HwTbdPhys;      // save TBD array physical address

        if (TcbCount){
            pMpTcb->PrevHwTcb = pHwTcb - 1;
        }
        else {
            pMpTcb->PrevHwTcb   = (PHW_TCB)((PUCHAR)FdoData->HwSendMemAllocVa +
                                  ((FdoData->NumTcb - 1) * sizeof(HW_TCB)));
        }
        pHwTcb->TxCbHeader.CbStatus = 0;        // clear the status
        pHwTcb->TxCbHeader.CbCommand = CB_EL_BIT | CB_TX_SF_BIT | CB_TRANSMIT;


        // Set the link pointer in HW TCB to the next TCB in the chain.
        // If this is the last TCB in the chain, then set it to the first TCB.
        if (TcbCount < FdoData->NumTcb - 1)
        {
            pMpTcb->Next = pMpTcb + 1;
            pHwTcb->TxCbHeader.CbLinkPointer = HwTcbPhys + sizeof(HW_TCB);
        }
        else
        {
            pMpTcb->Next = (PMP_TCB) FdoData->MpTcbMem;
            pHwTcb->TxCbHeader.CbLinkPointer =
                FdoData->HwSendMemAllocLa.LowPart;
        }

        pHwTcb->TxCbThreshold = (UCHAR) FdoData->AiThreshold;
        pHwTcb->TxCbTbdPointer = HwTbdPhys;

        pMpTcb++;
        pHwTcb++;
        HwTcbPhys += sizeof(TXCB_STRUC);
        pHwTbd = (PTBD_STRUC)((PUCHAR)pHwTbd + sizeof(TBD_STRUC) * NIC_MAX_PHYS_BUF_COUNT);
        HwTbdPhys += sizeof(TBD_STRUC) * NIC_MAX_PHYS_BUF_COUNT;
    }

    // set the TCB head/tail indexes
    // head is the olded one to free, tail is the next one to use
    FdoData->CurrSendHead = (PMP_TCB) FdoData->MpTcbMem;
    FdoData->CurrSendTail = (PMP_TCB) FdoData->MpTcbMem;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICInitSendBuffers\n");
}

NTSTATUS
NICInitRecvBuffers(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Initialize receive data structures

Arguments:

    FdoData - Pointer to our adapter context

Return Value:

--*/
{
    NTSTATUS        status = STATUS_INSUFFICIENT_RESOURCES;
    PMP_RFD         pMpRfd;
    ULONG           RfdCount;
    WDFMEMORY       memoryHdl;
    PDRIVER_CONTEXT  driverContext = GetDriverContext(WdfGetDriver());

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICInitRecvBuffers\n");

    PAGED_CODE();

    // Setup each RFD
    for (RfdCount = 0; RfdCount < FdoData->NumRfd; RfdCount++)
    {
        status = WdfMemoryCreateFromLookaside(
                driverContext->RecvLookaside,
                &memoryHdl
                );
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Failed to get lookaside buffer\n");
            continue;
        }
        pMpRfd = WdfMemoryGetBuffer(memoryHdl, NULL);
        if (!pMpRfd)
        {
            //ErrorValue = ERRLOG_OUT_OF_LOOKASIDE_MEMORY;
            continue;
        }
        pMpRfd->LookasideMemoryHdl = memoryHdl;
        //
        // Allocate the shared memory for this RFD.
        //
        _Analysis_assume_(FdoData->HwRfdSize > 0);
        status = WdfCommonBufferCreate( FdoData->WdfDmaEnabler,
                                        FdoData->HwRfdSize,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &pMpRfd->WdfCommonBuffer );

        if (status != STATUS_SUCCESS)
        {
            pMpRfd->WdfCommonBuffer = NULL;
            WdfObjectDelete(pMpRfd->LookasideMemoryHdl);
            continue;
        }

        pMpRfd->OriginalHwRfd =
            WdfCommonBufferGetAlignedVirtualAddress(pMpRfd->WdfCommonBuffer);

        pMpRfd->OriginalHwRfdLa =
            WdfCommonBufferGetAlignedLogicalAddress(pMpRfd->WdfCommonBuffer);

        //
        // Get a 8-byts aligned memory from the original HwRfd
        //
        pMpRfd->HwRfd = (PHW_RFD)DATA_ALIGN(pMpRfd->OriginalHwRfd);

        //
        // Now HwRfd is already 8-bytes aligned, and the size of HwPfd
        // header(not data part) is a multiple of 8,
        // If we shift HwRfd 0xA bytes up, the Ethernet header size
        // is 14 bytes long, then the data will be at
        // 8 byte boundary.
        //
        pMpRfd->HwRfd = (PHW_RFD)((PUCHAR)(pMpRfd->HwRfd) + HWRFD_SHIFT_OFFSET);

        //
        // Update physical address accordingly
        //
        pMpRfd->HwRfdLa.QuadPart = pMpRfd->OriginalHwRfdLa.QuadPart +
                         BYTES_SHIFT(pMpRfd->HwRfd, pMpRfd->OriginalHwRfd);

        status = NICAllocRfd(FdoData, pMpRfd);
        if (!NT_SUCCESS(status))
        {
            WdfObjectDelete(pMpRfd->LookasideMemoryHdl);
            continue;
        }
        //
        // Add this RFD to the RecvList
        //
        FdoData->CurrNumRfd++;
        NICReturnRFD(FdoData, pMpRfd);
    }

    if (FdoData->CurrNumRfd > NIC_MIN_RFDS)
    {
        status = STATUS_SUCCESS;
    }

    //
    // FdoData->CurrNumRfd < NIC_MIN_RFDs
    //
    if (status != STATUS_SUCCESS)
    {
        // TODO: Log an entry into the eventlog
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICInitRecvBuffers, status=%x\n", status);

    return status;
}

NTSTATUS
NICAllocRfd(
    IN  PFDO_DATA     FdoData,
    IN  PMP_RFD       pMpRfd
    )
/*++
Routine Description:

    Allocate NDIS_PACKET and NDIS_BUFFER associated with a RFD.
    Can be called at DISPATCH_LEVEL.

Arguments:

    FdoData     Pointer to our adapter
    pMpRfd      pointer to a RFD

Return Value:


--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PHW_RFD     pHwRfd;

    UNREFERENCED_PARAMETER(FdoData);

    PAGED_CODE();

    do{
        pHwRfd = pMpRfd->HwRfd;
        pMpRfd->HwRfdPhys = pMpRfd->HwRfdLa.LowPart;

        pMpRfd->Flags = 0;

        pMpRfd->Mdl = IoAllocateMdl((PVOID)&pHwRfd->RfdBuffer.RxMacHeader,
                                    NIC_MAX_PACKET_SIZE,
                                    FALSE,
                                    FALSE,
                                    NULL);
        if (!pMpRfd->Mdl)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        MmBuildMdlForNonPagedPool(pMpRfd->Mdl);

        pMpRfd->Buffer = &pHwRfd->RfdBuffer.RxMacHeader;

        // Init each RFD header
        pHwRfd->RfdRbdPointer = DRIVER_NULL;
        pHwRfd->RfdSize = NIC_MAX_PACKET_SIZE;

    } WHILE (FALSE);


    if (!NT_SUCCESS(status))
    {
        if (pMpRfd->WdfCommonBuffer)
        {
            //
            // Free HwRfd, we need to free the original memory
            // pointed by OriginalHwRfd.
            //
            WdfObjectDelete( pMpRfd->WdfCommonBuffer );

            pMpRfd->WdfCommonBuffer  = NULL;
            pMpRfd->HwRfd            = NULL;
            pMpRfd->OriginalHwRfd    = NULL;

            pMpRfd->DeleteCommonBuffer = TRUE;
        }
    }

    return status;

}

VOID
NICFreeRfd(
    IN  PFDO_DATA     FdoData,
    IN  PMP_RFD       pMpRfd
    )
/*++
Routine Description:

    Free a RFD.
    Can be called at DISPATCH_LEVEL.

Arguments:

    FdoData     Pointer to our adapter
    pMpRfd      Pointer to a RFD

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER(FdoData);
    PAGED_CODE();

    ASSERT(pMpRfd->HwRfd);
    ASSERT(pMpRfd->Mdl);

    IoFreeMdl(pMpRfd->Mdl);

    //
    // Free HwRfd, we need to free the original memory pointed
    // by OriginalHwRfd.
    //
    if (pMpRfd->DeleteCommonBuffer == TRUE) {

        WdfObjectDelete( pMpRfd->WdfCommonBuffer );
    }

    pMpRfd->WdfCommonBuffer = NULL;
    pMpRfd->HwRfd = NULL;
    pMpRfd->OriginalHwRfd = NULL;

    WdfObjectDelete(pMpRfd->LookasideMemoryHdl);
}

VOID
NICAllocRfdWorkItem(
    IN WDFWORKITEM  WorkItem
)
/*++

Routine Description:

   Worker routine to allocate memory for RFD at PASSIVE_LEVEL.

Arguments:

    WorkItem - Handle to framework item object.

Return Value:

   VOID

--*/
{
    PFDO_DATA               FdoData;
    //KIRQL                   oldIrql;
    PMP_RFD                 TempMpRfd;
    NTSTATUS                status;
    PWORKER_ITEM_CONTEXT    item;
    WDFMEMORY               tempMpRfdMemHdl;
    PDRIVER_CONTEXT  driverContext = GetDriverContext(WdfGetDriver());

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ, "---> NICAllocRfdWorkItem\n");

    item = GetWorkItemContext(WorkItem);
    FdoData = item->FdoData;

    status = WdfMemoryCreateFromLookaside(driverContext->RecvLookaside, &tempMpRfdMemHdl);
    if(!NT_SUCCESS(status)){
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Failed to get lookaside buffer\n");
        return ;
    }
    TempMpRfd = WdfMemoryGetBuffer(tempMpRfdMemHdl, NULL);
    if (TempMpRfd)
    {
        TempMpRfd->LookasideMemoryHdl = tempMpRfdMemHdl;
        //
        // Allocate the shared memory for this RFD.
        //
        _Analysis_assume_(FdoData->HwRfdSize > 0);
        status = WdfCommonBufferCreate(FdoData->WdfDmaEnabler,
                                       FdoData->HwRfdSize,
                                       WDF_NO_OBJECT_ATTRIBUTES,
                                       &TempMpRfd->WdfCommonBuffer);

        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ,
                       "WdfCommonBufferCreate failed %X\n", status);
            WdfObjectDelete(TempMpRfd->LookasideMemoryHdl);
            goto Exit;
        }

        TempMpRfd->OriginalHwRfd =
            WdfCommonBufferGetAlignedVirtualAddress(
                                        TempMpRfd->WdfCommonBuffer);

        TempMpRfd->OriginalHwRfdLa =
            WdfCommonBufferGetAlignedLogicalAddress(
                                        TempMpRfd->WdfCommonBuffer);

        //
        // First get a HwRfd at 8 byte boundary from OriginalHwRfd
        //
        TempMpRfd->HwRfd = (PHW_RFD)DATA_ALIGN(TempMpRfd->OriginalHwRfd);
        //
        // Then shift HwRfd so that the data(after ethernet header) is at 8 bytes boundary
        //
        TempMpRfd->HwRfd = (PHW_RFD)((PUCHAR)TempMpRfd->HwRfd + HWRFD_SHIFT_OFFSET);
        //
        // Update physical address as well
        //
        TempMpRfd->HwRfdLa.QuadPart = TempMpRfd->OriginalHwRfdLa.QuadPart +
                            BYTES_SHIFT(TempMpRfd->HwRfd, TempMpRfd->OriginalHwRfd);

        status = NICAllocRfd(FdoData, TempMpRfd);
        if (!NT_SUCCESS(status))
        {
            //
            // NICAllocRfd frees common buffer when it returns an TRACE_LEVEL_ERROR.
            // So, let us not worry about freeing that here.
            //
            WdfObjectDelete(TempMpRfd->LookasideMemoryHdl);
            TraceEvents(TRACE_LEVEL_ERROR, DBG_READ, "Recv: NICAllocRfd failed %x\n", status);
            goto Exit;
        }


        WdfSpinLockAcquire(FdoData->RcvLock);

        //
        // Add this RFD to the RecvList
        //
        FdoData->CurrNumRfd++;
        NICReturnRFD(FdoData, TempMpRfd);


        WdfSpinLockRelease(FdoData->RcvLock);

        ASSERT(FdoData->CurrNumRfd <= FdoData->MaxNumRfd);
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ,
                    "CurrNumRfd=%d\n", FdoData->CurrNumRfd);

    }

Exit:
    FdoData->AllocNewRfd = FALSE;

    WdfObjectDelete(WorkItem);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ, "<--- NICAllocRfdWorkItem\n");

    return;
}

VOID
NICFreeRfdWorkItem(
    IN WDFWORKITEM  WorkItem
)
/*++

Routine Description:

   Worker routine to RFD memory at PASSIVE_LEVEL.

Arguments:

    WorkItem - Handle to framework item object.

Return Value:

   VOID

--*/
{
    PFDO_DATA               fdoData;
    PMP_RFD                 pMpRfd;
    PWORKER_ITEM_CONTEXT      item;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ, "---> NICFreeRfdWorkItem\n");

    PAGED_CODE();

    item = GetWorkItemContext(WorkItem);
    fdoData = item->FdoData;
    pMpRfd = (PMP_RFD)item->Argument1;

    NICFreeRfd(fdoData, pMpRfd);

    WdfObjectDelete(WorkItem);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_READ, "<--- NICFreeRfdWorkItem\n");

    return;
}



NTSTATUS
NICSelfTest(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Perform a NIC self-test

Arguments:

    FdoData     Pointer to our adapter

Return Value:

    NT status code

--*/
{
    NTSTATUS     status = STATUS_SUCCESS;
    ULONG           SelfTestCommandCode;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICSelfTest\n");

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "SelfTest=%p, SelfTestPhys=%x\n",
        FdoData->SelfTest, FdoData->SelfTestPhys);

    //
    // Issue a software reset to the adapter
    //
    HwSoftwareReset(FdoData);

    //
    // Execute The PORT Self Test Command On The 82558.
    //
    ASSERT(FdoData->SelfTestPhys != 0);
    SelfTestCommandCode = FdoData->SelfTestPhys;

    //
    // Setup SELF TEST Command Code in D3 - D0
    //
    SelfTestCommandCode |= PORT_SELFTEST;

    //
    // Initialize the self-test signature and results DWORDS
    //
    FdoData->SelfTest->StSignature = 0;
    FdoData->SelfTest->StResults = 0xffffffff;

    //
    // Do the port command
    //
    FdoData->CSRAddress->Port = SelfTestCommandCode;

    MP_STALL_EXECUTION(NIC_DELAY_POST_SELF_TEST_MS);

    //
    // if The First Self Test DWORD Still Zero, We've timed out.  If the second
    // DWORD is not zero then we have an TRACE_LEVEL_ERROR.
    //
    if ((FdoData->SelfTest->StSignature == 0) || (FdoData->SelfTest->StResults != 0))
    {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "StSignature=%x, StResults=%x\n",
            FdoData->SelfTest->StSignature, FdoData->SelfTest->StResults);

       status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICSelfTest, status=%x\n", status);

    return status;
}

NTSTATUS
NICInitializeAdapter(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Initialize the adapter and set up everything

Arguments:

    FdoData     Pointer to our adapter

Return Value:

 NT Status Code

--*/
{
    NTSTATUS     status;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "--> NICInitializeAdapter\n");

    do
    {

        // set up our link indication variable
        // it doesn't matter what this is right now because it will be
        // set correctly if link fails
        FdoData->MediaState = Connected;

        // Issue a software reset to the D100
        HwSoftwareReset(FdoData);

        // Load the CU BASE (set to 0, because we use linear mode)
        FdoData->CSRAddress->ScbGeneralPointer = 0;
        status = D100IssueScbCommand(FdoData, SCB_CUC_LOAD_BASE, FALSE);
        if (status != STATUS_SUCCESS)
        {
            break;
        }

        // Wait for the SCB command word to clear before we set the general pointer
        if (!WaitScb(FdoData))
        {
            status = STATUS_DEVICE_DATA_ERROR;
            break;
        }

        // Load the RU BASE (set to 0, because we use linear mode)
        FdoData->CSRAddress->ScbGeneralPointer = 0;
        status = D100IssueScbCommand(FdoData, SCB_RUC_LOAD_BASE, FALSE);
        if (status != STATUS_SUCCESS)
        {
            break;
        }

        // Configure the adapter
        status = HwConfigure(FdoData);
        if (status != STATUS_SUCCESS)
        {
            break;
        }

        status = HwSetupIAAddress(FdoData);
        if (status != STATUS_SUCCESS)
        {
            break;
        }

        // Clear the internal counters
        HwClearAllCounters(FdoData);


    } WHILE (FALSE);

    if (status != STATUS_SUCCESS)
    {
        // TODO: Log an entry into the eventlog
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "<-- NICInitializeAdapter, Status=%x\n", status);

    return status;
}

VOID
NICShutdown(
    IN  PFDO_DATA     FdoData)
/*++

Routine Description:

    Shutdown the device

Arguments:

    FdoData -  Pointer to our adapter

Return Value:

    None

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "---> NICShutdown\n");

    if(FdoData->CSRAddress) {
        //
        // Disable interrupt and issue a full reset
        //
        NICDisableInterrupt(FdoData);
        NICIssueFullReset(FdoData);
        //
        // Reset the PHY chip.  We do this so that after a warm boot, the PHY will
        // be in a known state, with auto-negotiation enabled.
        //
        ResetPhy(FdoData);
    }
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "<--- NICShutdown\n");
}

VOID
HwSoftwareReset(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Issue a software reset to the hardware

Arguments:

    FdoData     Pointer to our adapter

Return Value:

    None

--*/
{
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "--> HwSoftwareReset\n");

    // Issue a PORT command with a data word of 0
    FdoData->CSRAddress->Port = PORT_SOFTWARE_RESET;

    // wait after the port reset command
    KeStallExecutionProcessor(NIC_DELAY_POST_RESET);

    // Mask off our interrupt line -- its unmasked after reset
    NICDisableInterrupt(FdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "<-- HwSoftwareReset\n");
}



NTSTATUS
HwConfigure(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Configure the hardware

Arguments:

    FdoData     Pointer to our adapter

Return Value:

    NT Status


--*/
{
    NTSTATUS         status;
    PCB_HEADER_STRUC    NonTxCmdBlockHdr =
                                (PCB_HEADER_STRUC)FdoData->NonTxCmdBlock;
    UINT                i;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "--> HwConfigure\n");

    //
    // Init the packet filter to nothing.
    //
    FdoData->OldPacketFilter = FdoData->PacketFilter;
    FdoData->PacketFilter = 0;

    //
    // Store the current setting for BROADCAST/PROMISCUOS modes
    FdoData->OldParameterField = CB_557_CFIG_DEFAULT_PARM15;

    // Setup the non-transmit command block header for the configure command.
    NonTxCmdBlockHdr->CbStatus = 0;
    NonTxCmdBlockHdr->CbCommand = CB_CONFIGURE;
    NonTxCmdBlockHdr->CbLinkPointer = DRIVER_NULL;

    // Fill in the configure command data.

    // First fill in the static (end user can't change) config bytes
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[0] = CB_557_CFIG_DEFAULT_PARM0;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[2] = CB_557_CFIG_DEFAULT_PARM2;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[3] = CB_557_CFIG_DEFAULT_PARM3;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[6] = CB_557_CFIG_DEFAULT_PARM6;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[9] = CB_557_CFIG_DEFAULT_PARM9;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[10] = CB_557_CFIG_DEFAULT_PARM10;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[11] = CB_557_CFIG_DEFAULT_PARM11;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[12] = CB_557_CFIG_DEFAULT_PARM12;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[13] = CB_557_CFIG_DEFAULT_PARM13;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[14] = CB_557_CFIG_DEFAULT_PARM14;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[16] = CB_557_CFIG_DEFAULT_PARM16;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[17] = CB_557_CFIG_DEFAULT_PARM17;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[18] = CB_557_CFIG_DEFAULT_PARM18;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[20] = CB_557_CFIG_DEFAULT_PARM20;
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[21] = CB_557_CFIG_DEFAULT_PARM21;

    // Now fill in the rest of the configuration bytes (the bytes that contain
    // user configurable parameters).

    // Set the Tx and Rx Fifo limits
    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[1] =
        (UCHAR) ((FdoData->AiTxFifo << 4) | FdoData->AiRxFifo);

    if (FdoData->MWIEnable)
    {
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[3] |= CB_CFIG_B3_MWI_ENABLE;
    }

    // Set the Tx and Rx DMA maximum byte count fields.
    if ((FdoData->AiRxDmaCount) || (FdoData->AiTxDmaCount))
    {
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[4] =
            FdoData->AiRxDmaCount;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[5] =
            (UCHAR) (FdoData->AiTxDmaCount | CB_CFIG_DMBC_EN);
    }
    else
    {
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[4] =
            CB_557_CFIG_DEFAULT_PARM4;
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[5] =
            CB_557_CFIG_DEFAULT_PARM5;
    }


    FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[7] =
        (UCHAR) ((CB_557_CFIG_DEFAULT_PARM7 & (~CB_CFIG_URUN_RETRY)) |
        (FdoData->AiUnderrunRetry << 1)
        );

    // Setup for MII or 503 operation.  The CRS+CDT bit should only be set
    // when operating in 503 mode.
    if (FdoData->PhyAddress == 32)
    {
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[8] =
            (CB_557_CFIG_DEFAULT_PARM8 & (~CB_CFIG_503_MII));
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[15] =
            (CB_557_CFIG_DEFAULT_PARM15 | CB_CFIG_CRS_OR_CDT);
    }
    else
    {
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[8] =
            (CB_557_CFIG_DEFAULT_PARM8 | CB_CFIG_503_MII);
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[15] =
            ((CB_557_CFIG_DEFAULT_PARM15 & (~CB_CFIG_CRS_OR_CDT)) | CB_CFIG_BROADCAST_DIS);
    }


    // Setup Full duplex stuff

    // If forced to half duplex
    if (FdoData->AiForceDpx == 1)
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
            (CB_557_CFIG_DEFAULT_PARM19 &
            (~(CB_CFIG_FORCE_FDX| CB_CFIG_FDX_ENABLE)));

    // If forced to full duplex
    else if (FdoData->AiForceDpx == 2)
        FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
            (CB_557_CFIG_DEFAULT_PARM19 | CB_CFIG_FORCE_FDX);

    // If auto-duplex
    else
    {
        // We must force full duplex on if we are using PHY 0, and we are
        // supposed to run in FDX mode.  We do this because the D100 has only
        // one FDX# input pin, and that pin will be connected to PHY 1.
        if ((FdoData->PhyAddress == 0) && (FdoData->usDuplexMode == 2))
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                (CB_557_CFIG_DEFAULT_PARM19 | CB_CFIG_FORCE_FDX);
        else
            FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
            CB_557_CFIG_DEFAULT_PARM19;
    }


    // display the config TRACE_LEVEL_INFORMATION to the debugger
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "   Issuing Configure command\n");
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "   Config Block at virt addr %p phys address %x\n",
        &NonTxCmdBlockHdr->CbStatus, FdoData->NonTxCmdBlockPhys);

    for (i=0; i < CB_CFIG_BYTE_COUNT; i++)
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "   Config byte %x = %.2x\n",
            i, FdoData->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[i]);

    // Wait for the SCB command word to clear before we set the general pointer
    if (!WaitScb(FdoData))
    {
        status = STATUS_DEVICE_DATA_ERROR;
    }
    else
    {
        ASSERT(FdoData->CSRAddress->ScbCommandLow == 0);
        FdoData->CSRAddress->ScbGeneralPointer = FdoData->NonTxCmdBlockPhys;

        // Submit the configure command to the chip, and wait for it to complete.
        status = D100SubmitCommandBlockAndWait(FdoData);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS,
                            "<-- HwConfigure, Status=%x\n", status);

    return status;
}


NTSTATUS
HwSetupIAAddress(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    Set up the individual MAC address

Arguments:

    FdoData     Pointer to our adapter

Return Value:

    NT Status code

--*/
{
    NTSTATUS         status;
    UINT                i;
    PCB_HEADER_STRUC    NonTxCmdBlockHdr =
                            (PCB_HEADER_STRUC)FdoData->NonTxCmdBlock;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "--> HwSetupIAAddress\n");

    // Individual Address Setup
    NonTxCmdBlockHdr->CbStatus = 0;
    NonTxCmdBlockHdr->CbCommand = CB_IA_ADDRESS;
    NonTxCmdBlockHdr->CbLinkPointer = DRIVER_NULL;

    // Copy in the station's individual address
    for (i = 0; i < ETH_LENGTH_OF_ADDRESS; i++)
        FdoData->NonTxCmdBlock->NonTxCb.Setup.IaAddress[i] = FdoData->CurrentAddress[i];

    // Update the command list pointer.  We don't need to do a WaitSCB here
    // because this command is either issued immediately after a reset, or
    // after another command that runs in polled mode.  This guarantees that
    // the low byte of the SCB command word will be clear.  The only commands
    // that don't run in polled mode are transmit and RU-start commands.
    ASSERT(FdoData->CSRAddress->ScbCommandLow == 0);
    FdoData->CSRAddress->ScbGeneralPointer = FdoData->NonTxCmdBlockPhys;

    // Submit the IA configure command to the chip, and wait for it to complete.
    status = D100SubmitCommandBlockAndWait(FdoData);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "<-- HwSetupIAAddress, Status=%x\n", status);

    return status;
}

NTSTATUS
HwClearAllCounters(
    IN  PFDO_DATA     FdoData
    )
/*++
Routine Description:

    This routine will clear the hardware TRACE_LEVEL_ERROR statistic counters

Arguments:

    FdoData     Pointer to our adapter

Return Value:

    NT Status code


--*/
{
    NTSTATUS     status;
    BOOLEAN         bResult;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS, "--> HwClearAllCounters\n");

    PAGED_CODE();

    do
    {
        // Load the dump counters pointer.  Since this command is generated only
        // after the IA setup has complete, we don't need to wait for the SCB
        // command word to clear
        ASSERT(FdoData->CSRAddress->ScbCommandLow == 0);
        FdoData->CSRAddress->ScbGeneralPointer = FdoData->StatsCounterPhys;

        // Issue the load dump counters address command
        status = D100IssueScbCommand(FdoData, SCB_CUC_DUMP_ADDR, FALSE);
        if (status != STATUS_SUCCESS)
            break;

        // Now dump and reset all of the statistics
        status = D100IssueScbCommand(FdoData, SCB_CUC_DUMP_RST_STAT, TRUE);
        if (status != STATUS_SUCCESS)
            break;

        // Now wait for the dump/reset to complete, timeout value 2 secs
        MP_STALL_AND_WAIT(FdoData->StatsCounters->CommandComplete == 0xA007, 2000, bResult);
        if (!bResult)
        {
            MP_SET_HARDWARE_ERROR(FdoData);
            status = STATUS_DEVICE_DATA_ERROR;
            break;
        }

        // init packet counts
        FdoData->GoodTransmits = 0;
        FdoData->GoodReceives = 0;

        // init transmit error counts
        FdoData->TxAbortExcessCollisions = 0;
        FdoData->TxLateCollisions = 0;
        FdoData->TxDmaUnderrun = 0;
        FdoData->TxLostCRS = 0;
        FdoData->TxOKButDeferred = 0;
        FdoData->OneRetry = 0;
        FdoData->MoreThanOneRetry = 0;
        FdoData->TotalRetries = 0;

        // init receive error counts
        FdoData->RcvCrcErrors = 0;
        FdoData->RcvAlignmentErrors = 0;
        FdoData->RcvResourceErrors = 0;
        FdoData->RcvDmaOverrunErrors = 0;
        FdoData->RcvCdtFrames = 0;
        FdoData->RcvRuntErrors = 0;

    } WHILE (FALSE);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_HW_ACCESS,
                "<-- HwClearAllCounters, Status=%x\n", status);

    return status;
}

VOID
NICGetDeviceInfSettings(
    IN OUT  PFDO_DATA   FdoData
    )
{

    //
    // Number of ReceiveFrameDescriptors
    //
    if(!PciDrvReadRegistryValue(FdoData,
                                L"NumRfd",
                                &FdoData->NumRfd)){
        FdoData->NumRfd = 32;
    }

    FdoData->NumRfd = min(FdoData->NumRfd, NIC_MAX_RFDS);
    FdoData->NumRfd = max(FdoData->NumRfd, 1);

    //
    // Number of Transmit Control Blocks
    //
    if(!PciDrvReadRegistryValue(FdoData,
                                L"NumTcb",
                                &FdoData->NumTcb)){
        FdoData->NumTcb  = NIC_DEF_TCBS;

    }

    FdoData->NumTcb = min(FdoData->NumTcb, NIC_MAX_TCBS);
    FdoData->NumTcb = max(FdoData->NumTcb, 1);

    //
    // Max number of buffers required for coalescing fragmented packet.
    // Not implemented in this sample
    //
    if(!PciDrvReadRegistryValue(FdoData,
                                L"NumCoalesce",
                                &FdoData->NumBuffers)){
        FdoData->NumBuffers  = 8;
    }

    FdoData->NumBuffers = min(FdoData->NumBuffers, 32);
    FdoData->NumBuffers = max(FdoData->NumBuffers, 1);

    //
    // Get the Link Speed & Duplex.
    //
    if(!PciDrvReadRegistryValue(FdoData,
                                L"SpeedDuplex",
                                &FdoData->SpeedDuplex)){
        FdoData->SpeedDuplex  = 0;
    }
    FdoData->SpeedDuplex = min(FdoData->SpeedDuplex, 4);
    FdoData->SpeedDuplex = max(FdoData->SpeedDuplex, 0);
    //
    // Decode SpeedDuplex
    // Value 0 means Auto detect
    // Value 1 means 10Mb-Half-Duplex
    // Value 2 means 10Mb-Full-Duplex
    // Value 3 means 100Mb-Half-Duplex
    // Value 4 means 100Mb-Full-Duplex
    //
    switch(FdoData->SpeedDuplex)
    {
        case 1:
        FdoData->AiTempSpeed = 10; FdoData->AiForceDpx = 1;
        break;

        case 2:
        FdoData->AiTempSpeed = 10; FdoData->AiForceDpx = 2;
        break;

        case 3:
        FdoData->AiTempSpeed = 100; FdoData->AiForceDpx = 1;
        break;

        case 4:
        FdoData->AiTempSpeed = 100; FdoData->AiForceDpx = 2;
        break;
    }

    //
    // Rest of these values are currently not configured thru INF.
    //
    FdoData->PhyAddress  = 0xFF;
    FdoData->Connector  = 0;
    FdoData->AiTxFifo  = DEFAULT_TX_FIFO_LIMIT;
    FdoData->AiRxFifo  = DEFAULT_RX_FIFO_LIMIT;
    FdoData->AiTxDmaCount  = 0;
    FdoData->AiRxDmaCount  = 0;
    FdoData->AiUnderrunRetry  = DEFAULT_UNDERRUN_RETRY;
    FdoData->AiThreshold  = 200;
    FdoData->MWIEnable = 1;
    FdoData->Congest = 0;

    return;
 }



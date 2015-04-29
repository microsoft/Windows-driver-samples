/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    Ramdisk.c

Abstract:

    This is the Ramdisk sample driver.  This version of the driver has been
    modified to support the driver frameworks. This driver basically creates
    a nonpaged pool and exposes that as a storage media. User can
    find the device in the disk manager and format the media to use
    as FAT or NTFS volume.

Environment:

    Kernel mode only.

--*/

#include "ramdisk.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, RamDiskEvtDeviceAdd)
#pragma alloc_text(PAGE, RamDiskEvtDeviceContextCleanup)
#pragma alloc_text(PAGE, RamDiskQueryDiskRegParameters)
#pragma alloc_text(PAGE, RamDiskFormatDisk)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful.

--*/

{
    WDF_DRIVER_CONFIG config;

    KdPrint(("Windows Ramdisk Driver - Driver Framework Edition.\n"));


    WDF_DRIVER_CONFIG_INIT( &config, RamDiskEvtDeviceAdd );

    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

VOID
RamDiskEvtIoRead(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t Length
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_READ request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    Length - Length of the data buffer associated with the request.
             The default property of the queue is to not dispatch
             zero length read & write requests to the driver and
             complete is with status success. So we will never get
             a zero length request.

Return Value:

    VOID

--*/
{
    PDEVICE_EXTENSION      devExt = QueueGetExtension(Queue)->DeviceExtension;
    NTSTATUS               Status = STATUS_INVALID_PARAMETER;
    WDF_REQUEST_PARAMETERS Parameters;
    LARGE_INTEGER          ByteOffset;
    WDFMEMORY              hMemory;

    _Analysis_assume_(Length > 0);

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);

    ByteOffset.QuadPart = Parameters.Parameters.Read.DeviceOffset;

    if (RamDiskCheckParameters(devExt, ByteOffset, Length)) {

        Status = WdfRequestRetrieveOutputMemory(Request, &hMemory);
        if(NT_SUCCESS(Status)){

            Status = WdfMemoryCopyFromBuffer(hMemory,   // Destination
                                             0,         // Offset into the destination
                                             devExt->DiskImage + ByteOffset.LowPart, // source
                                             Length);
        }
    }

    WdfRequestCompleteWithInformation(Request, Status, (ULONG_PTR)Length);
}

VOID
RamDiskEvtIoWrite(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t Length
    )

/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_WRITE request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    Length - Length of the data buffer associated with the request.
             The default property of the queue is to not dispatch
             zero length read & write requests to the driver and
             complete is with status success. So we will never get
             a zero length request.

Return Value:

    VOID

--*/
{
    PDEVICE_EXTENSION      devExt = QueueGetExtension(Queue)->DeviceExtension;
    NTSTATUS               Status = STATUS_INVALID_PARAMETER;
    WDF_REQUEST_PARAMETERS Parameters;
    LARGE_INTEGER          ByteOffset;
    WDFMEMORY              hMemory;

    _Analysis_assume_(Length > 0);

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);

    ByteOffset.QuadPart = Parameters.Parameters.Write.DeviceOffset;

    if (RamDiskCheckParameters(devExt, ByteOffset, Length)) {

        Status = WdfRequestRetrieveInputMemory(Request, &hMemory);
        if(NT_SUCCESS(Status)){

            Status = WdfMemoryCopyToBuffer(hMemory, // Source
                                    0,              // offset in Source memory where the copy has to start
                                    devExt->DiskImage + ByteOffset.LowPart, // destination
                                    Length);
        }

    }

    WdfRequestCompleteWithInformation(Request, Status, (ULONG_PTR)Length);
}

VOID
RamDiskEvtIoDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.


Return Value:

    VOID

--*/
{
    NTSTATUS          Status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR         information = 0;
    size_t            bufSize;
    PDEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    switch (IoControlCode) {
    case IOCTL_DISK_GET_PARTITION_INFO: {

            PPARTITION_INFORMATION outputBuffer;
            PBOOT_SECTOR bootSector = (PBOOT_SECTOR) devExt->DiskImage;

            information = sizeof(PARTITION_INFORMATION);

            Status = WdfRequestRetrieveOutputBuffer(Request, sizeof(PARTITION_INFORMATION), &outputBuffer, &bufSize);
            if(NT_SUCCESS(Status) ) {

                outputBuffer->PartitionType =
                    (bootSector->bsFileSystemType[4] == '6') ? PARTITION_FAT_16 : PARTITION_FAT_12;

                outputBuffer->BootIndicator       = FALSE;
                outputBuffer->RecognizedPartition = TRUE;
                outputBuffer->RewritePartition    = FALSE;
                outputBuffer->StartingOffset.QuadPart = 0;
                outputBuffer->PartitionLength.QuadPart = devExt->DiskRegInfo.DiskSize;
                outputBuffer->HiddenSectors       = (ULONG) (1L);
                outputBuffer->PartitionNumber     = (ULONG) (-1L);

                Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:  {

            PDISK_GEOMETRY outputBuffer;

            //
            // Return the drive geometry for the ram disk. Note that
            // we return values which were made up to suit the disk size.
            //
            information = sizeof(DISK_GEOMETRY);

            Status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DISK_GEOMETRY), &outputBuffer, &bufSize);
            if(NT_SUCCESS(Status) &&
               bufSize >= sizeof(DISK_GEOMETRY)) {

                RtlCopyMemory(outputBuffer, &(devExt->DiskGeometry), sizeof(DISK_GEOMETRY));
                Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_DISK_IS_WRITABLE:

        //
        // Return status success
        //

        Status = STATUS_SUCCESS;
        break;
    }

    WdfRequestCompleteWithInformation(Request, Status, information);
}

VOID
RamDiskEvtDeviceContextCleanup(
    IN WDFOBJECT Device
    )
/*++

Routine Description:

   EvtDeviceContextCleanup event callback cleans up anything done in
   EvtDeviceAdd, except those things that are automatically cleaned
   up by the Framework.

   In the case of this sample, everything is automatically handled.  In a
   driver derived from this sample, it's quite likely that this function could
   be deleted.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    PDEVICE_EXTENSION pDeviceExtension = DeviceGetExtension(Device);

    PAGED_CODE();

    if(pDeviceExtension->DiskImage) {
        ExFreePool(pDeviceExtension->DiskImage);
    }
}

NTSTATUS
RamDiskEvtDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    NTSTATUS                status;
    WDFDEVICE               device;
    WDF_OBJECT_ATTRIBUTES   queueAttributes;
    WDF_IO_QUEUE_CONFIG     ioQueueConfig;
    PDEVICE_EXTENSION       pDeviceExtension;
    PQUEUE_EXTENSION        pQueueContext = NULL;
    WDFQUEUE                queue;
    DECLARE_CONST_UNICODE_STRING(ntDeviceName, NT_DEVICE_NAME);

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Driver);

    //
    // Storage drivers have to name their FDOs. Since we are not unique'fying
    // the device name, we wouldn't be able to install more than one instance
    // of this ramdisk driver.
    //
    status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_DISK);
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    //
    // Since this is a pure software only driver, there is no need to register
    // any PNP/Power event callbacks. Framework will respond to these
    // events appropriately.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_EXTENSION);
    deviceAttributes.EvtCleanupCallback = RamDiskEvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Now that the WDF device object has been created, set up any context
    // that it requires.
    //

    pDeviceExtension = DeviceGetExtension(device);

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE (
        &ioQueueConfig,
        WdfIoQueueDispatchSequential
        );

    ioQueueConfig.EvtIoDeviceControl = RamDiskEvtIoDeviceControl;
    ioQueueConfig.EvtIoRead          = RamDiskEvtIoRead;
    ioQueueConfig.EvtIoWrite         = RamDiskEvtIoWrite;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_EXTENSION);

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests or 
    // forward them to other drivers. This driver completes the requests 
    // directly in the queue's handlers. If the EvtIoStop callback is not 
    // implemented, the framework waits for all driver-owned requests to be
    // done before moving in the Dx/sleep states or before removing the 
    // device, which is the correct behavior for this type of driver.
    // If the requests were taking an indeterminate amount of time to complete,
    // or if the driver forwarded the requests to a lower driver/another stack,
    // the queue should have an EvtIoStop/EvtIoResume.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate( device,
                               &ioQueueConfig,
                               &queueAttributes,
                               &queue );
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    if (!NT_SUCCESS(status)) {
        return status;
    }

     // Context is the Queue handle
    pQueueContext = QueueGetExtension(queue);

    //
    // Set the context for our default queue as our device extension.
    //
    pQueueContext->DeviceExtension = pDeviceExtension;

#if KMDF_VERSION_MINOR >= 9

    //
    // Enable forward progress on the queue we just created.
    // NOTE: If you are planning to use this code without forward progress,
    // comment out the call to SetForwardProgressOnQueue below. 
    //
    status = SetForwardProgressOnQueue(queue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

#endif

    //
    // Now do any RAM-Disk specific initialization
    //
    pDeviceExtension->DiskRegInfo.DriveLetter.Buffer =
        (PWSTR) &pDeviceExtension->DriveLetterBuffer;
    pDeviceExtension->DiskRegInfo.DriveLetter.MaximumLength =
        sizeof(pDeviceExtension->DriveLetterBuffer);

    //
    // Get the disk parameters from the registry
    //
    RamDiskQueryDiskRegParameters(
        WdfDriverGetRegistryPath(WdfDeviceGetDriver(device)),
        &pDeviceExtension->DiskRegInfo
        );

    //
    // Allocate memory for the disk image.
    //
    pDeviceExtension->DiskImage = ExAllocatePoolWithTag(
        NonPagedPool,
        pDeviceExtension->DiskRegInfo.DiskSize,
        RAMDISK_TAG
        );

    if (pDeviceExtension->DiskImage) {

        UNICODE_STRING deviceName;
        UNICODE_STRING win32Name;

        RamDiskFormatDisk(pDeviceExtension);

        status = STATUS_SUCCESS;

        //
        // Now try to create a symbolic link for the drive letter.
        //
        RtlInitUnicodeString(&win32Name, DOS_DEVICE_NAME);
        RtlInitUnicodeString(&deviceName, NT_DEVICE_NAME);

        pDeviceExtension->SymbolicLink.Buffer = (PWSTR)
            &pDeviceExtension->DosDeviceNameBuffer;
        pDeviceExtension->SymbolicLink.MaximumLength =
            sizeof(pDeviceExtension->DosDeviceNameBuffer);
        pDeviceExtension->SymbolicLink.Length = win32Name.Length;

        RtlCopyUnicodeString(&pDeviceExtension->SymbolicLink, &win32Name);
        RtlAppendUnicodeStringToString(&pDeviceExtension->SymbolicLink,
                                       &pDeviceExtension->DiskRegInfo.DriveLetter);

        status = WdfDeviceCreateSymbolicLink(device,
                                             &pDeviceExtension->SymbolicLink);
    }

    return status;
}

VOID
RamDiskQueryDiskRegParameters(
    _In_ PWSTR RegistryPath,
    _In_ PDISK_INFO DiskRegInfo
    )

/*++

Routine Description:

    This routine is called from the DriverEntry to get the debug
    parameters from the registry. If the registry query fails, then
    default values are used.

Arguments:

    RegistryPath    - Points the service path to get the registry parameters

Return Value:

    None

--*/

{

    RTL_QUERY_REGISTRY_TABLE rtlQueryRegTbl[5 + 1];  // Need 1 for NULL
    NTSTATUS                 Status;
    DISK_INFO                defDiskRegInfo;

    PAGED_CODE();

    ASSERT(RegistryPath != NULL);

    // Set the default values

    defDiskRegInfo.DiskSize          = DEFAULT_DISK_SIZE;
    defDiskRegInfo.RootDirEntries    = DEFAULT_ROOT_DIR_ENTRIES;
    defDiskRegInfo.SectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;

    RtlInitUnicodeString(&defDiskRegInfo.DriveLetter, DEFAULT_DRIVE_LETTER);

    RtlZeroMemory(rtlQueryRegTbl, sizeof(rtlQueryRegTbl));

    //
    // Setup the query table
    //

    rtlQueryRegTbl[0].Flags         = RTL_QUERY_REGISTRY_SUBKEY;
    rtlQueryRegTbl[0].Name          = L"Parameters";
    rtlQueryRegTbl[0].EntryContext  = NULL;
    rtlQueryRegTbl[0].DefaultType   = (ULONG_PTR)NULL;
    rtlQueryRegTbl[0].DefaultData   = NULL;
    rtlQueryRegTbl[0].DefaultLength = (ULONG_PTR)NULL;

    //
    // Disk paramters
    //

    rtlQueryRegTbl[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[1].Name          = L"DiskSize";
    rtlQueryRegTbl[1].EntryContext  = &DiskRegInfo->DiskSize;
    rtlQueryRegTbl[1].DefaultType   = REG_DWORD;
    rtlQueryRegTbl[1].DefaultData   = &defDiskRegInfo.DiskSize;
    rtlQueryRegTbl[1].DefaultLength = sizeof(ULONG);

    rtlQueryRegTbl[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[2].Name          = L"RootDirEntries";
    rtlQueryRegTbl[2].EntryContext  = &DiskRegInfo->RootDirEntries;
    rtlQueryRegTbl[2].DefaultType   = REG_DWORD;
    rtlQueryRegTbl[2].DefaultData   = &defDiskRegInfo.RootDirEntries;
    rtlQueryRegTbl[2].DefaultLength = sizeof(ULONG);

    rtlQueryRegTbl[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[3].Name          = L"SectorsPerCluster";
    rtlQueryRegTbl[3].EntryContext  = &DiskRegInfo->SectorsPerCluster;
    rtlQueryRegTbl[3].DefaultType   = REG_DWORD;
    rtlQueryRegTbl[3].DefaultData   = &defDiskRegInfo.SectorsPerCluster;
    rtlQueryRegTbl[3].DefaultLength = sizeof(ULONG);

    rtlQueryRegTbl[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[4].Name          = L"DriveLetter";
    rtlQueryRegTbl[4].EntryContext  = &DiskRegInfo->DriveLetter;
    rtlQueryRegTbl[4].DefaultType   = REG_SZ;
    rtlQueryRegTbl[4].DefaultData   = defDiskRegInfo.DriveLetter.Buffer;
    rtlQueryRegTbl[4].DefaultLength = 0;


    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                 RegistryPath,
                 rtlQueryRegTbl,
                 NULL,
                 NULL
             );

    if (NT_SUCCESS(Status) == FALSE) {

        DiskRegInfo->DiskSize          = defDiskRegInfo.DiskSize;
        DiskRegInfo->RootDirEntries    = defDiskRegInfo.RootDirEntries;
        DiskRegInfo->SectorsPerCluster = defDiskRegInfo.SectorsPerCluster;
        RtlCopyUnicodeString(&DiskRegInfo->DriveLetter, &defDiskRegInfo.DriveLetter);
    }

    KdPrint(("DiskSize          = 0x%lx\n", DiskRegInfo->DiskSize));
    KdPrint(("RootDirEntries    = 0x%lx\n", DiskRegInfo->RootDirEntries));
    KdPrint(("SectorsPerCluster = 0x%lx\n", DiskRegInfo->SectorsPerCluster));
    KdPrint(("DriveLetter       = %wZ\n",   &(DiskRegInfo->DriveLetter)));

    return;
}

NTSTATUS
RamDiskFormatDisk(
    IN PDEVICE_EXTENSION devExt
    )

/*++

Routine Description:

    This routine formats the new disk.


Arguments:

    DeviceObject - Supplies a pointer to the device object that represents
                   the device whose capacity is to be read.

Return Value:

    status is returned.

--*/
{

    PBOOT_SECTOR bootSector = (PBOOT_SECTOR) devExt->DiskImage;
    PUCHAR       firstFatSector;
    ULONG        rootDirEntries;
    ULONG        sectorsPerCluster;
    USHORT       fatType;        // Type FAT 12 or 16
    USHORT       fatEntries;     // Number of cluster entries in FAT
    USHORT       fatSectorCnt;   // Number of sectors for FAT
    PDIR_ENTRY   rootDir;        // Pointer to first entry in root dir

    PAGED_CODE();
    ASSERT(sizeof(BOOT_SECTOR) == 512);
    ASSERT(devExt->DiskImage != NULL);

    RtlZeroMemory(devExt->DiskImage, devExt->DiskRegInfo.DiskSize);

    devExt->DiskGeometry.BytesPerSector = 512;
    devExt->DiskGeometry.SectorsPerTrack = 32;     // Using Ramdisk value
    devExt->DiskGeometry.TracksPerCylinder = 2;    // Using Ramdisk value

    //
    // Calculate number of cylinders.
    //

    devExt->DiskGeometry.Cylinders.QuadPart = devExt->DiskRegInfo.DiskSize / 512 / 32 / 2;

    //
    // Our media type is RAMDISK_MEDIA_TYPE
    //

    devExt->DiskGeometry.MediaType = RAMDISK_MEDIA_TYPE;

    KdPrint((
        "Cylinders: %I64d\n TracksPerCylinder: %lu\n SectorsPerTrack: %lu\n BytesPerSector: %lu\n",
        devExt->DiskGeometry.Cylinders.QuadPart, devExt->DiskGeometry.TracksPerCylinder,
        devExt->DiskGeometry.SectorsPerTrack, devExt->DiskGeometry.BytesPerSector
        ));

    rootDirEntries = devExt->DiskRegInfo.RootDirEntries;
    sectorsPerCluster = devExt->DiskRegInfo.SectorsPerCluster;

    //
    // Round Root Directory entries up if necessary
    //

    if (rootDirEntries & (DIR_ENTRIES_PER_SECTOR - 1)) {

        rootDirEntries =
            (rootDirEntries + (DIR_ENTRIES_PER_SECTOR - 1)) &
                ~ (DIR_ENTRIES_PER_SECTOR - 1);
    }

    KdPrint((
        "Root dir entries: %lu\n Sectors/cluster: %lu\n",
        rootDirEntries, sectorsPerCluster
        ));

    //
    // We need to have the 0xeb and 0x90 since this is one of the
    // checks the file system recognizer uses
    //

    bootSector->bsJump[0] = 0xeb;
    bootSector->bsJump[1] = 0x3c;
    bootSector->bsJump[2] = 0x90;

    //
    // Set OemName to "RajuRam "
    // NOTE: Fill all 8 characters, eg. sizeof(bootSector->bsOemName);
    //
    bootSector->bsOemName[0] = 'R';
    bootSector->bsOemName[1] = 'a';
    bootSector->bsOemName[2] = 'j';
    bootSector->bsOemName[3] = 'u';
    bootSector->bsOemName[4] = 'R';
    bootSector->bsOemName[5] = 'a';
    bootSector->bsOemName[6] = 'm';
    bootSector->bsOemName[7] = ' ';

    bootSector->bsBytesPerSec = (SHORT)devExt->DiskGeometry.BytesPerSector;
    bootSector->bsResSectors  = 1;
    bootSector->bsFATs        = 1;
    bootSector->bsRootDirEnts = (USHORT)rootDirEntries;

    bootSector->bsSectors     = (USHORT)(devExt->DiskRegInfo.DiskSize /
                                         devExt->DiskGeometry.BytesPerSector);
    bootSector->bsMedia       = (UCHAR)devExt->DiskGeometry.MediaType;
    bootSector->bsSecPerClus  = (UCHAR)sectorsPerCluster;

    //
    // Calculate number of sectors required for FAT
    //

    fatEntries =
        (bootSector->bsSectors - bootSector->bsResSectors -
            bootSector->bsRootDirEnts / DIR_ENTRIES_PER_SECTOR) /
                bootSector->bsSecPerClus + 2;

    //
    // Choose between 12 and 16 bit FAT based on number of clusters we
    // need to map
    //

    if (fatEntries > 4087) {
        fatType =  16;
        fatSectorCnt = (fatEntries * 2 + 511) / 512;
        fatEntries   = fatEntries + fatSectorCnt;
        fatSectorCnt = (fatEntries * 2 + 511) / 512;
    }
    else {
        fatType =  12;
        fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
        fatEntries   = fatEntries + fatSectorCnt;
        fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
    }

    bootSector->bsFATsecs       = fatSectorCnt;
    bootSector->bsSecPerTrack   = (USHORT)devExt->DiskGeometry.SectorsPerTrack;
    bootSector->bsHeads         = (USHORT)devExt->DiskGeometry.TracksPerCylinder;
    bootSector->bsBootSignature = 0x29;
    bootSector->bsVolumeID      = 0x12345678;

    //
    // Set Label to "RamDisk    "
    // NOTE: Fill all 11 characters, eg. sizeof(bootSector->bsLabel);
    //
    bootSector->bsLabel[0]  = 'R';
    bootSector->bsLabel[1]  = 'a';
    bootSector->bsLabel[2]  = 'm';
    bootSector->bsLabel[3]  = 'D';
    bootSector->bsLabel[4]  = 'i';
    bootSector->bsLabel[5]  = 's';
    bootSector->bsLabel[6]  = 'k';
    bootSector->bsLabel[7]  = ' ';
    bootSector->bsLabel[8]  = ' ';
    bootSector->bsLabel[9]  = ' ';
    bootSector->bsLabel[10] = ' ';

    //
    // Set FileSystemType to "FAT1?   "
    // NOTE: Fill all 8 characters, eg. sizeof(bootSector->bsFileSystemType);
    //
    bootSector->bsFileSystemType[0] = 'F';
    bootSector->bsFileSystemType[1] = 'A';
    bootSector->bsFileSystemType[2] = 'T';
    bootSector->bsFileSystemType[3] = '1';
    bootSector->bsFileSystemType[4] = '?';
    bootSector->bsFileSystemType[5] = ' ';
    bootSector->bsFileSystemType[6] = ' ';
    bootSector->bsFileSystemType[7] = ' ';

    bootSector->bsFileSystemType[4] = ( fatType == 16 ) ? '6' : '2';

    bootSector->bsSig2[0] = 0x55;
    bootSector->bsSig2[1] = 0xAA;

    //
    // The FAT is located immediately following the boot sector.
    //

    firstFatSector    = (PUCHAR)(bootSector + 1);
    firstFatSector[0] = (UCHAR)devExt->DiskGeometry.MediaType;
    firstFatSector[1] = 0xFF;
    firstFatSector[2] = 0xFF;

    if (fatType == 16) {
        firstFatSector[3] = 0xFF;
    }

    //
    // The Root Directory follows the FAT
    //
    rootDir = (PDIR_ENTRY)(bootSector + 1 + fatSectorCnt);

    //
    // Set device name to "MS-RAMDR"
    // NOTE: Fill all 8 characters, eg. sizeof(rootDir->deName);
    //
    rootDir->deName[0] = 'M';
    rootDir->deName[1] = 'S';
    rootDir->deName[2] = '-';
    rootDir->deName[3] = 'R';
    rootDir->deName[4] = 'A';
    rootDir->deName[5] = 'M';
    rootDir->deName[6] = 'D';
    rootDir->deName[7] = 'R';

    //
    // Set device extension name to "IVE"
    // NOTE: Fill all 3 characters, eg. sizeof(rootDir->deExtension);
    //
    rootDir->deExtension[0] = 'I';
    rootDir->deExtension[1] = 'V';
    rootDir->deExtension[2] = 'E';

    rootDir->deAttributes = DIR_ATTR_VOLUME;

    return STATUS_SUCCESS;
}

BOOLEAN
RamDiskCheckParameters(
    IN PDEVICE_EXTENSION devExt,
    IN LARGE_INTEGER ByteOffset,
    IN size_t Length
    )

{
    //
    // Check for invalid parameters.  It is an error for the starting offset
    // + length to go past the end of the buffer, or for the length to
    // not be a proper multiple of the sector size.
    //
    // Others are possible, but we don't check them since we trust the
    // file system.
    //

    if( devExt->DiskRegInfo.DiskSize < Length ||
        ByteOffset.QuadPart < 0 || // QuadPart is signed so check for negative values
        ((ULONGLONG)ByteOffset.QuadPart > (devExt->DiskRegInfo.DiskSize - Length)) ||
            (Length & (devExt->DiskGeometry.BytesPerSector - 1))) {

        //
        // Do not give an I/O boost for parameter errors.
        //

        KdPrint((
            "Error invalid parameter\n"
            "ByteOffset: %I64x\n"
            "Length: %lu\n",
            ByteOffset.QuadPart,
            Length
         ));

        return FALSE;
    }

    return TRUE;
}


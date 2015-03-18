/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    CdoProc.h

Abstract:

    This is the header file defining the functions of the kernel mode
    filter driver implementing the CDO sample.


Environment:

    Kernel mode


--*/


//
//  Functions implemented in operations.c
//



//
//  Functions implemented in CdoOperations.c
//

_Function_class_(DRIVER_INITIALIZE)
NTSTATUS
CdoCreateControlDeviceObject(
    _Inout_ PDRIVER_OBJECT DriverObject
    );

VOID
CdoDeleteControlDeviceObject(
    VOID
    );

//
//  Functions implemented in CdoOperations.c
//

DRIVER_DISPATCH CdoMajorFunction;
NTSTATUS
CdoMajorFunction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    );

NTSTATUS
CdoHandlePrivateOpen(
    _In_ PIRP Irp
    );

NTSTATUS
CdoHandlePrivateCleanup(
    _In_ PIRP Irp
    );

NTSTATUS
CdoHandlePrivateClose(
    _In_ PIRP Irp
    );

NTSTATUS
CdoHandlePrivateFsControl (
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_opt_ PIRP Irp
    );

BOOLEAN
CdoFastIoCheckIfPossible (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ BOOLEAN CheckForReadOperation,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoRead (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoWrite (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_reads_bytes_(Length) PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoQueryBasicInfo (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_BASIC_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoQueryStandardInfo (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_STANDARD_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoLock (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ PEPROCESS ProcessId,
    _In_ ULONG Key,
    _In_ BOOLEAN FailImmediately,
    _In_ BOOLEAN ExclusiveLock,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoUnlockSingle (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ PEPROCESS ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoUnlockAll (
    _In_ PFILE_OBJECT FileObject,
    _In_ PEPROCESS ProcessId,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoUnlockAllByKey (
    _In_ PFILE_OBJECT FileObject,
    _In_ PVOID ProcessId,
    _In_ ULONG Key,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoDeviceControl (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ ULONG IoControlCode,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject);

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoQueryNetworkOpenInfo (
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoMdlRead (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoMdlReadComplete (
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject );

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoPrepareMdlWrite (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject );

BOOLEAN
CdoFastIoMdlWriteComplete (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject );

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoReadCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _Out_writes_bytes_(CompressedDataInfoLength) struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    _In_ ULONG CompressedDataInfoLength,
    _In_ PDEVICE_OBJECT DeviceObject);

_Success_(IoStatus->Status == 0)
BOOLEAN
CdoFastIoWriteCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ ULONG LockKey,
    _In_reads_bytes_(Length) PVOID Buffer,
    _Outptr_ PMDL *MdlChain,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_reads_bytes_(CompressedDataInfoLength) struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    _In_ ULONG CompressedDataInfoLength,
    _In_ PDEVICE_OBJECT DeviceObject);

BOOLEAN
CdoFastIoMdlReadCompleteCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject);

BOOLEAN
CdoFastIoMdlWriteCompleteCompressed (
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ PMDL MdlChain,
    _In_ PDEVICE_OBJECT DeviceObject);

_Success_(Irp->IoStatus.Status == 0)
BOOLEAN
CdoFastIoQueryOpen (
    _In_ PIRP Irp,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    _In_ PDEVICE_OBJECT DeviceObject);




//
//  Resource support
//

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
CdoAcquireResourceExclusive (
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)
    PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               !ExIsResourceAcquiredSharedLite(Resource));

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusiveLite( Resource, TRUE );
}

FORCEINLINE
VOID
_Acquires_lock_(_Global_critical_region_)
CdoAcquireResourceShared (
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_shared_lock_(*_Curr_)
    PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceSharedLite( Resource, TRUE );
}

FORCEINLINE
VOID
_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(_Global_critical_region_)
CdoReleaseResource (
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
	PERESOURCE Resource
    )
{
    FLT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    FLT_ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
               ExIsResourceAcquiredSharedLite(Resource));

    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}



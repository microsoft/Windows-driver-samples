/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Private.h

Abstract:

Environment:

    Kernel mode

--*/


#if !defined(_PCI9656_H_)
#define _PCI9659_H_

//
// The device extension for the device object
//
typedef struct _DEVICE_EXTENSION {

    WDFDEVICE               Device;

    // Following fields are specific to the hardware
    // Configuration

    // HW Resources
    PPCI9656_REGS           Regs;             // Registers address
    PUCHAR                  RegsBase;         // Registers base address
    ULONG                   RegsLength;       // Registers base length

    PUCHAR                  PortBase;         // Port base address
    ULONG                   PortLength;       // Port base length

    PUCHAR                  SRAMBase;         // SRAM base address
    ULONG                   SRAMLength;       // SRAM base length

    PUCHAR                  SRAM2Base;        // SRAM (alt) base address
    ULONG                   SRAM2Length;      // SRAM (alt) base length

    WDFINTERRUPT            Interrupt;     // Returned by InterruptCreate

    union {
        INT_CSR bits;
        ULONG   ulong;
    }                       IntCsr;

    union {
        DMA_CSR bits;
        UCHAR   uchar;
    }                       Dma0Csr;

    union {
        DMA_CSR bits;
        UCHAR   uchar;
    }                       Dma1Csr;

    // DmaEnabler
    WDFDMAENABLER           DmaEnabler;
    ULONG                   MaximumTransferLength;

    // IOCTL handling
    WDFQUEUE                ControlQueue;
    BOOLEAN                 RequireSingleTransfer;

#ifdef SIMULATE_MEMORY_FRAGMENTATION
    PMDL                    WriteMdlChain;
    PMDL                    ReadMdlChain;
#endif

    // Write
    WDFQUEUE                WriteQueue;
    WDFDMATRANSACTION       WriteDmaTransaction;

    ULONG                   WriteTransferElements;
    WDFCOMMONBUFFER         WriteCommonBuffer;
    size_t                  WriteCommonBufferSize;
    _Field_size_(WriteCommonBufferSize) PUCHAR WriteCommonBufferBase;
    PHYSICAL_ADDRESS        WriteCommonBufferBaseLA;  // Logical Address

    // Read
    ULONG                   ReadTransferElements;
    WDFCOMMONBUFFER         ReadCommonBuffer;
    size_t                  ReadCommonBufferSize;
    _Field_size_(ReadCommonBufferSize) PUCHAR ReadCommonBufferBase;
    PHYSICAL_ADDRESS        ReadCommonBufferBaseLA;   // Logical Address

    WDFDMATRANSACTION       ReadDmaTransaction;
    WDFQUEUE                ReadQueue;

    ULONG                   HwErrCount;

}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// This will generate the function named PLxGetDeviceContext to be use for
// retreiving the DEVICE_EXTENSION pointer.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, PLxGetDeviceContext)

#if !defined(ASSOC_WRITE_REQUEST_WITH_DMA_TRANSACTION)
//
// The context structure used with WdfDmaTransactionCreate
//
typedef struct TRANSACTION_CONTEXT {

    WDFREQUEST     Request;

} TRANSACTION_CONTEXT, * PTRANSACTION_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TRANSACTION_CONTEXT, PLxGetTransactionContext)

#endif

//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD PLxEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP PlxEvtDeviceCleanup;

EVT_WDF_OBJECT_CONTEXT_CLEANUP PlxEvtDriverContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY PLxEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT PLxEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE PLxEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE PLxEvtDeviceReleaseHardware;

EVT_WDF_IO_QUEUE_IO_READ PLxEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE PLxEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PLxEvtIoDeviceControl;

EVT_WDF_INTERRUPT_ISR PLxEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC PLxEvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE PLxEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE PLxEvtInterruptDisable;

NTSTATUS
PLxSetIdleAndWakeSettings(
    IN PDEVICE_EXTENSION FdoData
    );

NTSTATUS
PLxInitializeDeviceExtension(
    IN PDEVICE_EXTENSION DevExt
    );

VOID
PlxCleanupDeviceExtension(
    _In_ PDEVICE_EXTENSION DevExt
    );

NTSTATUS
PLxPrepareHardware(
    IN PDEVICE_EXTENSION DevExt,
    IN WDFCMRESLIST     ResourcesTranslated
    );

NTSTATUS
PLxInitRead(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
PLxInitWrite(
    IN PDEVICE_EXTENSION DevExt
    );

//
// WDFINTERRUPT Support
//
NTSTATUS
PLxInterruptCreate(
    IN PDEVICE_EXTENSION DevExt
    );

VOID
PLxReadRequestComplete(
    IN WDFDMATRANSACTION  DmaTransaction,
    IN NTSTATUS           Status
    );

VOID
PLxWriteRequestComplete(
    IN WDFDMATRANSACTION  DmaTransaction,
    IN NTSTATUS           Status
    );

NTSTATUS
PLxInitializeHardware(
    IN PDEVICE_EXTENSION DevExt
    );

VOID
PLxShutdown(
    IN PDEVICE_EXTENSION DevExt
    );

EVT_WDF_PROGRAM_DMA PLxEvtProgramReadDma;
EVT_WDF_PROGRAM_DMA PLxEvtProgramWriteDma;

VOID
PLxHardwareReset(
    IN PDEVICE_EXTENSION    DevExt
    );

NTSTATUS
PLxInitializeDMA(
    IN PDEVICE_EXTENSION DevExt
    );

#ifdef SIMULATE_MEMORY_FRAGMENTATION

#define POOL_TAG 'x5x9'

//
// Constants for the MDL chains we use to simulate memory fragmentation.
//
#define MDL_CHAIN_LENGTH 8
#define MDL_BUFFER_SIZE  ((PCI9656_MAXIMUM_TRANSFER_LENGTH) / MDL_CHAIN_LENGTH)

C_ASSERT(MDL_CHAIN_LENGTH * MDL_BUFFER_SIZE == PCI9656_MAXIMUM_TRANSFER_LENGTH);

NTSTATUS
BuildMdlChain(
    _Out_ PMDL* MdlChain
    );

VOID
DestroyMdlChain(
    _In_ PMDL MdlChain
    );

VOID
CopyBufferToMdlChain(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ size_t Length,
    _In_ PMDL MdlChain
    );

VOID
CopyMdlChainToBuffer(
    _In_ PMDL MdlChain,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ size_t Length
    );
#endif // SIMULATE_MEMORY_FRAGMENTATION

#pragma warning(disable:4127) // avoid conditional expression is constant error with W4

#endif  // _PCI9656_H_


/******************************Module*Header*******************************\
* Module Name: blthw.cxx
*
* Sample display driver functions for a HW blt simulation. This file is 
* only provided to simulate how a real hardware-accelerated display-only 
* driver functions, and should not be used in a real driver.
*
* Copyright (c) 2011 Microsoft Corporation
\**************************************************************************/

#include "BDD.hxx"

typedef struct
{
    CONST DXGKRNL_INTERFACE*        DxgkInterface;
    DXGKARGCB_NOTIFY_INTERRUPT_DATA NotifyInterrupt;
} SYNC_NOTIFY_INTERRUPT;

KSYNCHRONIZE_ROUTINE SynchronizeVidSchNotifyInterrupt;

BOOLEAN SynchronizeVidSchNotifyInterrupt(_In_opt_ PVOID params)
{
    // This routine is non-paged code called at the device interrupt level (DIRQL)
    // to notify VidSch and schedule a DPC. It is meant as a demonstration of handling 
    // a real hardware interrupt, even though it is actually called from asynchronous 
    // present worker threads in this sample.
    SYNC_NOTIFY_INTERRUPT* pParam = reinterpret_cast<SYNC_NOTIFY_INTERRUPT*>(params);

    // The context is known to be non-NULL
    __analysis_assume(pParam != NULL);

    // Update driver information related to fences
    switch(pParam->NotifyInterrupt.InterruptType)
    {
    case DXGK_INTERRUPT_DISPLAYONLY_VSYNC:
    case DXGK_INTERRUPT_DISPLAYONLY_PRESENT_PROGRESS:
        break;
    default:
        NT_ASSERT(FALSE);
        return FALSE;
    }

    // Callback OS to report about the interrupt
    pParam->DxgkInterface->DxgkCbNotifyInterrupt(pParam->DxgkInterface->DeviceHandle,&pParam->NotifyInterrupt);

    // Now queue a DPC for this interrupt (to callback schedule at DCP level and let it do more work there)
    // DxgkCbQueueDpc can return FALSE if there is already a DPC queued
    // this is an acceptable condition
    pParam->DxgkInterface->DxgkCbQueueDpc(pParam->DxgkInterface->DeviceHandle);

    return TRUE;
}

#pragma code_seg("PAGE")

KSTART_ROUTINE HwContextWorkerThread;

struct DoPresentMemory
{
    PVOID                     DstAddr;
    UINT                      DstStride;
    ULONG                     DstBitPerPixel;
    UINT                      SrcWidth;
    UINT                      SrcHeight;
    BYTE*                     SrcAddr;
    LONG                      SrcPitch;
    ULONG                     NumMoves;             // in:  Number of screen to screen moves
    D3DKMT_MOVE_RECT*         Moves;               // in:  Point to the list of moves
    ULONG                     NumDirtyRects;        // in:  Number of direct rects
    RECT*                     DirtyRect;           // in:  Point to the list of dirty rects
    D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation;
    BOOLEAN                   SynchExecution;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  SourceID;
    HANDLE                    hAdapter;
    PMDL                      Mdl;
    BDD_HWBLT*                DisplaySource;
};

void
HwExecutePresentDisplayOnly(
    HANDLE Context);

NTSTATUS
StartHwBltPresentWorkerThread(
    _In_ PKSTART_ROUTINE StartRoutine,
    _In_ _When_(return==0, __drv_aliasesMem) PVOID StartContext)
/*++

  Routine Description:

    This routine creates the worker thread to execute a single present 
    command. Creating a new thread on every asynchronous present is not 
    efficient, but this file is only meant as a simulation, not an example 
    of implementation.

  Arguments:

    StartRoutine - start routine
    StartContext - start context

  Return Value:

    Status

--*/
{
    PAGED_CODE();

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    HANDLE hWorkerThread = NULL;

    // copy data from the context which is need here, as it will be deleted in separate thread
    DoPresentMemory* ctx = reinterpret_cast<DoPresentMemory*>(StartContext);
    BDD_HWBLT* displaySource = ctx->DisplaySource;

    NTSTATUS Status = PsCreateSystemThread(
        &hWorkerThread,
        THREAD_ALL_ACCESS,
        &ObjectAttributes,
        NULL,
        NULL,
        StartRoutine,
        StartContext);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    // wait for thread to start - infinite wait -
    // need to make sure the tread is running before OS stars submitting the work items to it
    KeWaitForSingleObject(&displaySource->m_hThreadStartupEvent, Executive, KernelMode, FALSE, NULL);

    // Handle is passed to the parent object which must close it
    displaySource->SetPresentWorkerThreadInfo(hWorkerThread);

    // Resume context thread, this is done by setting the event the thread is waiting on
    KeSetEvent(&displaySource->m_hThreadSuspendEvent, 0, FALSE);

    return STATUS_PENDING;
}

BDD_HWBLT::BDD_HWBLT():m_DevExt (NULL),
                m_SynchExecution(TRUE),
                m_hPresentWorkerThread(NULL),
                m_pPresentWorkerThread(NULL)
{
    PAGED_CODE();

    KeInitializeEvent(&m_hThreadStartupEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&m_hThreadSuspendEvent, SynchronizationEvent, FALSE);

}


BDD_HWBLT::~BDD_HWBLT()
/*++

  Routine Description:

    This routine waits on present worker thread to exit before
    destroying the object

  Arguments:

    None

  Return Value:

    None

--*/
{
    PAGED_CODE();

    // make sure the worker thread has exited
    SetPresentWorkerThreadInfo(NULL);
}

#pragma warning(push)
#pragma warning(disable:26135) // The function doesn't lock anything

void
BDD_HWBLT::SetPresentWorkerThreadInfo(
    HANDLE hWorkerThread)
/*++

  Routine Description:

    The method is updating present worker information
    It is called in following cases:
     - In ExecutePresent to update worker thread information
     - In Dtor to wait on worker thread to exit

  Arguments:

    hWorkerThread - handle of the present worker thread

  Return Value:

    None

--*/
{

    PAGED_CODE();

    if (m_pPresentWorkerThread)
    {
        // Wait for thread to exit
        KeWaitForSingleObject(m_pPresentWorkerThread, Executive, KernelMode,
                              FALSE, NULL);
        // Dereference thread object
        ObDereferenceObject(m_pPresentWorkerThread);
        m_pPresentWorkerThread = NULL;

        NT_ASSERT(m_hPresentWorkerThread);
        ZwClose(m_hPresentWorkerThread);
        m_hPresentWorkerThread = NULL;
    }

    if (hWorkerThread)
    {
        // Make sure that thread's handle would be valid even if the thread exited
        ObReferenceObjectByHandle(hWorkerThread, THREAD_ALL_ACCESS, NULL,
                                  KernelMode, &m_pPresentWorkerThread, NULL);
        NT_ASSERT(m_pPresentWorkerThread);
        m_hPresentWorkerThread = hWorkerThread;
    }

}
#pragma warning(pop)

NTSTATUS
BDD_HWBLT::ExecutePresentDisplayOnly(
    _In_ BYTE*             DstAddr,
    _In_ UINT              DstBitPerPixel,
    _In_ BYTE*             SrcAddr,
    _In_ UINT              SrcBytesPerPixel,
    _In_ LONG              SrcPitch,
    _In_ ULONG             NumMoves,
    _In_ D3DKMT_MOVE_RECT* Moves,
    _In_ ULONG             NumDirtyRects,
    _In_ RECT*             DirtyRect,
    _In_ D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation)
/*++

  Routine Description:

    The method creates present worker thread and provides context
    for it filled with present commands

  Arguments:

    DstAddr - address of destination surface
    DstBitPerPixel - color depth of destination surface
    SrcAddr - address of source surface
    SrcBytesPerPixel - bytes per pixel of source surface
    SrcPitch - source surface pitch (bytes in a row)
    NumMoves - number of moves to be copied
    Moves - moves' data
    NumDirtyRects - number of rectangles to be copied
    DirtyRect - rectangles' data
    Rotation - roatation to be performed when executing copy
    CallBack - callback for present worker thread to report execution status

  Return Value:

    Status

--*/
{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    SIZE_T sizeMoves = NumMoves*sizeof(D3DKMT_MOVE_RECT);
    SIZE_T sizeRects = NumDirtyRects*sizeof(RECT);
    SIZE_T size = sizeof(DoPresentMemory) + sizeMoves + sizeRects;

    DoPresentMemory* ctx = reinterpret_cast<DoPresentMemory*>
                                (new (PagedPool) BYTE[size]);

    if (!ctx)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(ctx,size);

    const CURRENT_BDD_MODE* pModeCur = m_DevExt->GetCurrentMode(m_SourceId);

    ctx->DstAddr          = DstAddr;
    ctx->DstBitPerPixel   = DstBitPerPixel;
    ctx->DstStride        = pModeCur->DispInfo.Pitch;
    ctx->SrcWidth         = pModeCur->SrcModeWidth;
    ctx->SrcHeight        = pModeCur->SrcModeHeight;
    ctx->SrcAddr          = NULL;
    ctx->SrcPitch         = SrcPitch;
    ctx->Rotation         = Rotation;
    ctx->NumMoves         = NumMoves;
    ctx->Moves            = Moves;
    ctx->NumDirtyRects    = NumDirtyRects;
    ctx->DirtyRect        = DirtyRect;
    ctx->SourceID         = m_SourceId;
    ctx->hAdapter         = m_DevExt;
    ctx->Mdl              = NULL;
    ctx->DisplaySource    = this;

    // Alternate between synch and asynch execution, for demonstrating 
    // that a real hardware implementation can do either
    m_SynchExecution = !m_SynchExecution;

    ctx->SynchExecution   = m_SynchExecution;

    {
        // Map Source into kernel space, as Blt will be executed by system worker thread
        UINT sizeToMap = SrcBytesPerPixel*pModeCur->SrcModeWidth*pModeCur->SrcModeHeight;

        PMDL mdl = IoAllocateMdl((PVOID)SrcAddr, sizeToMap,  FALSE, FALSE, NULL);
        if(!mdl)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KPROCESSOR_MODE AccessMode = static_cast<KPROCESSOR_MODE>(( SrcAddr <=
                        (BYTE* const) MM_USER_PROBE_ADDRESS)?UserMode:KernelMode);
        __try
        {
            // Probe and lock the pages of this buffer in physical memory.
            // We need only IoReadAccess.
            MmProbeAndLockPages(mdl, AccessMode, IoReadAccess);
        }
        #pragma prefast(suppress: __WARNING_EXCEPTIONEXECUTEHANDLER, "try/except is only able to protect against user-mode errors and these are the only errors we try to catch here");
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetExceptionCode();
            IoFreeMdl(mdl);
            return Status;
        }

        // Map the physical pages described by the MDL into system space.
        // Note: double mapping the buffer this way causes lot of system
        // overhead for large size buffers.
        ctx->SrcAddr = reinterpret_cast<BYTE*>
            (MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority ));

        if(!ctx->SrcAddr) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            MmUnlockPages(mdl);
            IoFreeMdl(mdl);
            return Status;
        }

        // Save Mdl to unmap and unlock the pages in worker thread
        ctx->Mdl = mdl;
    }

    BYTE* rects = reinterpret_cast<BYTE*>(ctx+1);

    // copy moves and update pointer
    if (Moves)
    {
        memcpy(rects,Moves,sizeMoves);
        ctx->Moves = reinterpret_cast<D3DKMT_MOVE_RECT*>(rects);
        rects += sizeMoves;
    }

    // copy dirty rects and update pointer
    if (DirtyRect)
    {
        memcpy(rects,DirtyRect,sizeRects);
        ctx->DirtyRect = reinterpret_cast<RECT*>(rects);
    }


    if (m_SynchExecution)
    {
        HwExecutePresentDisplayOnly((PVOID)ctx);
        return STATUS_SUCCESS;
    }
    else
    {
        // Create a worker thread to perform the present asynchronously
        // Ctx will be deleted in worker thread (on exit)
        return StartHwBltPresentWorkerThread(HwContextWorkerThread,(PVOID)ctx);
    }
}


void
ReportPresentProgress(
    _In_ HANDLE Adapter,
    _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _In_ BOOLEAN CompletedOrFailed)
/*++

  Routine Description:

    This routine runs a fake interrupt routine in order to tell the OS about the present progress.

  Arguments:

    Adapter - Handle to the adapter (Device Extension)
    VidPnSourceId - Video present source id for the callback
    CompletedOrFailed - Present progress status for the source

  Return Value:

    None

--*/
{
    PAGED_CODE();

    BASIC_DISPLAY_DRIVER* pDevExt =
        reinterpret_cast<BASIC_DISPLAY_DRIVER*>(Adapter);

    SYNC_NOTIFY_INTERRUPT SyncNotifyInterrupt = {};
    SyncNotifyInterrupt.DxgkInterface = pDevExt->GetDxgkInterface();
    SyncNotifyInterrupt.NotifyInterrupt.InterruptType = DXGK_INTERRUPT_DISPLAYONLY_PRESENT_PROGRESS;
    SyncNotifyInterrupt.NotifyInterrupt.DisplayOnlyPresentProgress.VidPnSourceId = VidPnSourceId;

    SyncNotifyInterrupt.NotifyInterrupt.DisplayOnlyPresentProgress.ProgressId =
                            (CompletedOrFailed)?DXGK_PRESENT_DISPLAYONLY_PROGRESS_ID_COMPLETE:
                            DXGK_PRESENT_DISPLAYONLY_PROGRESS_ID_FAILED;

    // Execute the SynchronizeVidSchNotifyInterrupt function at the interrupt 
    // IRQL in order to fake a real present progress interrupt
    BOOLEAN bRet = FALSE;
    NT_VERIFY(NT_SUCCESS(pDevExt->GetDxgkInterface()->DxgkCbSynchronizeExecution(
                                    pDevExt->GetDxgkInterface()->DeviceHandle,
                                    (PKSYNCHRONIZE_ROUTINE)SynchronizeVidSchNotifyInterrupt,
                                    (PVOID)&SyncNotifyInterrupt,0,&bRet)));
    NT_ASSERT(bRet);
}


void
HwContextWorkerThread(
    HANDLE Context)
{
    PAGED_CODE();

    DoPresentMemory* ctx = reinterpret_cast<DoPresentMemory*>(Context);
    BDD_HWBLT* displaySource = ctx->DisplaySource;

    // Signal event to indicate that the tread has started
    KeSetEvent(&displaySource->m_hThreadStartupEvent, 0, FALSE);

    // Suspend context thread, do this by waiting on the suspend event
    KeWaitForSingleObject(&displaySource->m_hThreadSuspendEvent, Executive, KernelMode, FALSE, NULL);

    HwExecutePresentDisplayOnly(Context);
}


void
HwExecutePresentDisplayOnly(
    HANDLE Context)
/*++

  Routine Description:

    The routine executes present's commands and report progress to the OS

  Arguments:

    Context - Context with present's command

  Return Value:

    None

--*/
{
    PAGED_CODE();

    DoPresentMemory* ctx = reinterpret_cast<DoPresentMemory*>(Context);

    // Set up destination blt info
    BLT_INFO DstBltInfo;
    DstBltInfo.pBits = ctx->DstAddr;
    DstBltInfo.Pitch = ctx->DstStride;
    DstBltInfo.BitsPerPel = ctx->DstBitPerPixel;
    DstBltInfo.Offset.x = 0;
    DstBltInfo.Offset.y = 0;
    DstBltInfo.Rotation = ctx->Rotation;
    DstBltInfo.Width = ctx->SrcWidth;
    DstBltInfo.Height = ctx->SrcHeight;

    // Set up source blt info
    BLT_INFO SrcBltInfo;
    SrcBltInfo.pBits = ctx->SrcAddr;
    SrcBltInfo.Pitch = ctx->SrcPitch;
    SrcBltInfo.BitsPerPel = 32;
    SrcBltInfo.Offset.x = 0;
    SrcBltInfo.Offset.y = 0;
    SrcBltInfo.Rotation = D3DKMDT_VPPR_IDENTITY;
    if (ctx->Rotation == D3DKMDT_VPPR_ROTATE90 ||
        ctx->Rotation == D3DKMDT_VPPR_ROTATE270)
    {
        SrcBltInfo.Width = DstBltInfo.Height;
        SrcBltInfo.Height = DstBltInfo.Width;
    }
    else
    {
        SrcBltInfo.Width = DstBltInfo.Width;
        SrcBltInfo.Height = DstBltInfo.Height;
    }


    // Copy all the scroll rects from source image to video frame buffer.
    for (UINT i = 0; i < ctx->NumMoves; i++)
    {
        BltBits(&DstBltInfo,
        &SrcBltInfo,
        1, // NumRects
        &ctx->Moves[i].DestRect);
    }

    // Copy all the dirty rects from source image to video frame buffer.
    for (UINT i = 0; i < ctx->NumDirtyRects; i++)
    {

        BltBits(&DstBltInfo,
        &SrcBltInfo,
        1, // NumRects
        &ctx->DirtyRect[i]);
    }

    // Unmap unmap and unlock the pages.
    if (ctx->Mdl)
    {
        MmUnlockPages(ctx->Mdl);
        IoFreeMdl(ctx->Mdl);
    }

    if(ctx->SynchExecution)
    {
        // This code simulates Blt executed synchronously
        // nothing should be done here, just exit
        ;
    }
    else
    {
        // TRUE == completed
        // This code is emulates interrupt which HW should generate
        ReportPresentProgress(ctx->hAdapter,ctx->SourceID,TRUE);
    }

    delete [] reinterpret_cast<BYTE*>(ctx);
}

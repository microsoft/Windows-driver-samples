/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Extension.cpp

Abstract:

    SDCA XU functions

Environment:

    Kernel mode

--*/

#include "private.h"

#ifndef __INTELLISENSE__
#include "Extension.tmh"
#endif

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetJackOverride
(
    _In_ PVOID      Context,    // SDCA Context
    _In_ BOOLEAN    Override    // TRUE:  Override
                                // FALSE: Default SDCA behavior
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Override);
    NTSTATUS status = STATUS_SUCCESS;

    PCODEC_DEVICE_CONTEXT devCtx;

    devCtx = GetCodecDeviceContext(Context);
    ASSERT(devCtx != NULL);

    devCtx->SdcaXuData.bExtensionJackOVerride = Override;

    return status;
}

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetJackSelectedMode
(
    _In_ PVOID Context,         // SDCA Context
    _In_ ULONG GroupEntityId,   // SDCA Group Entity ID for Jack(s)
    _In_ ULONG SelectedMode     // Type of jack type overriden by XU
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(GroupEntityId);
    UNREFERENCED_PARAMETER(SelectedMode);
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

#pragma code_seg()
NTSTATUS Codec_SdcaXuPDEPowerReferenceAcquire
(
    _In_ PVOID              Context,                // SDCA Context
    _In_ ULONG              PowerDomainEntityId,    // SDCA Entity ID for entity
    _In_ SDCAXU_POWER_STATE RequiredState           // Power state the PowerDomain needs to be in
)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PowerDomainEntityId);
    UNREFERENCED_PARAMETER(RequiredState);
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

#pragma code_seg()
NTSTATUS Codec_SdcaXuPDEPowerReferenceRelease
(
    _In_ PVOID              Context,                // SDCA Context
    _In_ ULONG              PowerDomainEntityId,    // SDCA Entity ID for entity
    _In_ SDCAXU_POWER_STATE ReleasedState           // Power state the PowerDomain no longer needs to be in
)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PowerDomainEntityId);
    UNREFERENCED_PARAMETER(ReleasedState);
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

#pragma code_seg()
NTSTATUS Codec_SdcaXuReadDeferredAudioControls
(
    _In_ PVOID                      Context,                // SDCA Context
    _Inout_ PSDCA_AUDIO_CONTROLS    Controls                // Array of SDCA Audio Controls
)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Controls);
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

#pragma code_seg()
NTSTATUS Codec_SdcaXuWriteDeferredAudioControls
(
    _In_ PVOID                      Context,                // SDCA Context
    _Inout_ PSDCA_AUDIO_CONTROLS    Controls                // Array of SDCA Audio Controls
)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Controls);
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuSetXUEntities
(
    _In_        PVOID  Context,
    _In_        ULONG  NumEntities,
    _In_reads_(NumEntities)
                ULONG EntityIDs[]
    )
{
    PAGED_CODE();

    DrvLogEnter(g_SDCAVCodecLog);

    NTSTATUS status = STATUS_SUCCESS;

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext((WDFDEVICE)Context);

    if (NumEntities)
    {
        PULONG pXUEntities = (PULONG)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ULONG) * NumEntities, DRIVER_TAG);
        RETURN_NTSTATUS_IF_TRUE(NULL == pXUEntities, STATUS_INSUFFICIENT_RESOURCES);

        for (ULONG i = 0; i < NumEntities; i++)
        {
            pXUEntities[i] = EntityIDs[i];
        }

        devCtx->SdcaXuData.numXUEntities = NumEntities;
        devCtx->SdcaXuData.XUEntities = pXUEntities;
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS Codec_SdcaXuRegisterForInterrupts
(
    _In_ PVOID                  Context,
    _In_ PSDCAXU_INTERRUPT_INFO InterruptInfo
)
{
    PAGED_CODE();

    DrvLogEnter(g_SDCAVCodecLog);

    NTSTATUS status = STATUS_SUCCESS;

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext((WDFDEVICE)Context);

    RETURN_NTSTATUS_IF_TRUE(InterruptInfo->Size != sizeof(SDCAXU_INTERRUPT_INFO), STATUS_INVALID_PARAMETER_1);

    PSDCAXU_INTERRUPT_INFO pInterruptInfo = (PSDCAXU_INTERRUPT_INFO)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        InterruptInfo->Size,
        DRIVER_TAG);
    RETURN_NTSTATUS_IF_TRUE(NULL == pInterruptInfo, STATUS_MEMORY_NOT_ALLOCATED);

    RtlCopyMemory(pInterruptInfo, InterruptInfo, InterruptInfo->Size);

    devCtx->SdcaXuData.InterruptInfo = pInterruptInfo;

    return status;
}

PAGED_CODE_SEG
NTSTATUS Codec_GetSdcaXu(_In_ WDFDEVICE Device)
{
    PAGED_CODE();
    NTSTATUS status = STATUS_SUCCESS;
    PCODEC_DEVICE_CONTEXT devCtx;

    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);

    RtlZeroMemory(&devCtx->SdcaXuData, sizeof(devCtx->SdcaXuData));

    // Initialize Interface for requesting correct version
    devCtx->SdcaXuData.ExtensionInterface.InterfaceHeader.Size = sizeof(SDCAXU_INTERFACE_V0101);
    devCtx->SdcaXuData.ExtensionInterface.InterfaceHeader.Version = SDCAXU_INTERFACE_VERSION_0101;

    //
    // Provide SDCA Interface that XU driver can call into
    // XU driver will copy these function addresses while
    // handling Query Interface
    //
    devCtx->SdcaXuData.ExtensionInterface.EvtSetXUEntities = Codec_SdcaXuSetXUEntities;
    devCtx->SdcaXuData.ExtensionInterface.EvtRegisterForInterrupts = Codec_SdcaXuRegisterForInterrupts;
    devCtx->SdcaXuData.ExtensionInterface.EvtSetJackOverride = Codec_SdcaXuSetJackOverride;
    devCtx->SdcaXuData.ExtensionInterface.EvtSetJackSelectedMode = Codec_SdcaXuSetJackSelectedMode;
    devCtx->SdcaXuData.ExtensionInterface.EvtPDEPowerReferenceAcquire = Codec_SdcaXuPDEPowerReferenceAcquire;
    devCtx->SdcaXuData.ExtensionInterface.EvtPDEPowerReferenceRelease = Codec_SdcaXuPDEPowerReferenceRelease;
    devCtx->SdcaXuData.ExtensionInterface.EvtReadDeferredAudioControls = Codec_SdcaXuReadDeferredAudioControls;
    devCtx->SdcaXuData.ExtensionInterface.EvtWriteDeferredAudioControls = Codec_SdcaXuWriteDeferredAudioControls;

    status = WdfFdoQueryForInterface(
        Device,
        &SDCAXU_INTERFACE,
        (PINTERFACE)&(devCtx->SdcaXuData.ExtensionInterface),
        sizeof(SDCAXU_INTERFACE_V0101),
        SDCAXU_INTERFACE_VERSION_0101,
        Device
    );

    if (NT_SUCCESS(status))
    {
        devCtx->SdcaXuData.bSdcaXu = TRUE;
    }

    return status;
}

PAGED_CODE_SEG
NTSTATUS Codec_SetSdcaXuHwConfig(_In_ WDFDEVICE Device)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogEnter(g_SDCAVCodecLog);

    PCODEC_DEVICE_CONTEXT devCtx;
    devCtx = GetCodecDeviceContext(Device);
    ASSERT(devCtx != NULL);
    PSDCAXU_INTERFACE_V0101 exInterface = &devCtx->SdcaXuData.ExtensionInterface;
    PVOID exContext = devCtx->SdcaXuData.ExtensionInterface.InterfaceHeader.Context;

    SdcaXuAcpiBlob acpiBlob;
    acpiBlob.NumEndpoints = 2;
    RETURN_NTSTATUS_IF_FAILED(exInterface->EvtSetHwConfig(exContext, SdcaXuHwConfigTypeAcpiBlob, &acpiBlob, sizeof(acpiBlob)));

    return status;
}


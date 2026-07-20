/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Render.cpp

Abstract:

    Plug and Play module. This file contains routines to handle pnp requests.

Environment:

    Kernel mode

--*/

#include "private.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>

#include "render.h"

#include "streamengine.h"

#include "AudioFormats.h"

#include "audiomodule.h"

#include "audioaggregation.h"

#ifndef __INTELLISENSE__
#include "render.tmh"
#endif

PAGED_CODE_SEG
NTSTATUS
SdcaXuR_EvtAcxPinSetDataFormat (
    _In_    ACXPIN          Pin,
    _In_    ACXDATAFORMAT   DataFormat
    )
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(DataFormat);


    return STATUS_NOT_SUPPORTED;
}

#pragma code_seg()
VOID
SdcaXuR_EvtPinContextCleanup(
    _In_ WDFOBJECT      WdfPin
   )
/*++

Routine Description:

    In this callback, it cleans up pin context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    UNREFERENCED_PARAMETER(WdfPin);
}

VOID SdcaXuR_EvtCircuitContextCleanup(_In_ WDFOBJECT Object)
{
    ACXCIRCUIT circuit = (ACXCIRCUIT)Object;
    PSDCAXU_RENDER_CIRCUIT_CONTEXT cirCtx = GetRenderCircuitContext(circuit);

    if (cirCtx->CircuitConfig)
    {
        ExFreePoolWithTag(cirCtx->CircuitConfig, DRIVER_TAG);
        cirCtx->CircuitConfig = NULL;
    }

#ifdef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
    if (cirCtx->WdfIoNotificationTarget)
    {
        WdfObjectDelete(cirCtx->WdfIoNotificationTarget);
        cirCtx->WdfIoNotificationTarget = nullptr;
    }

    cirCtx->PnpNotificationId = GUID_NULL;
    cirCtx->InstanceId = 0;
    cirCtx->EndpointDevice = nullptr;
#endif

    return;
}

PAGED_CODE_SEG
VOID
SdcaXuR_EvtCircuitRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
    )
/*++

Routine Description:

    This function is an example of a preprocess routine.

--*/
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(DriverContext);
    
    ASSERT(Object != NULL);
    ASSERT(DriverContext);
    ASSERT(Request);


// to handle module notifications directly, the following needs to be intercepted
// with the values cached, so that the XU has them available to compose the stream
// module notifications.
#ifdef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
    if (Object != nullptr)
    {
        NTSTATUS status = STATUS_SUCCESS;
        ACX_REQUEST_PARAMETERS params;
        PSDCAXU_RENDER_CIRCUIT_CONTEXT cirCtx;

        cirCtx = GetRenderCircuitContext(Object);

        ACX_REQUEST_PARAMETERS_INIT(&params);
        AcxRequestGetParameters(Request, &params);

        if(params.Type == AcxRequestTypeProperty && 
            params.Parameters.Property.Set == KSPROPERTYSETID_AcxCircuit)
        {
            switch(params.Parameters.Property.Id)
            {
                case KSPROPERTY_ACXCIRCUIT_SETNOTIFICATIONDEVICE:
                {
                    WDF_OBJECT_ATTRIBUTES       ioTargetAttrib;
                    WDFIOTARGET                 ioTarget = nullptr;
                    WDF_IO_TARGET_OPEN_PARAMS   openParams;
                    UNICODE_STRING              symbolicLinkName = {0};

                    if (params.Parameters.Property.Verb != AcxPropertyVerbSet)
                    {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                        goto exit;
                    }

                    if (params.Parameters.Property.ValueCb == 0)
                    {
                        status = STATUS_INVALID_PARAMETER;
                        goto exit;
                    }

                    status = RtlStringCbLengthW(
                        (LPCWSTR) params.Parameters.Property.Value,
                        params.Parameters.Property.ValueCb,     // maximum buffer size, including NULL termination
                        NULL                                    // optional returned lenght, not used.
                        );
                    if (!NT_SUCCESS(status))
                    {
                        goto exit;
                    }
                    
                    status = RtlUnicodeStringInitEx(&symbolicLinkName, (LPCWSTR) params.Parameters.Property.Value, 0);
                    if (!NT_SUCCESS(status))
                    {
                        goto exit;
                    }

                    WDF_OBJECT_ATTRIBUTES_INIT(&ioTargetAttrib);
                    ioTargetAttrib.ParentObject = cirCtx->EndpointDevice;
                    status = WdfIoTargetCreate(
                                               cirCtx->EndpointDevice,
                                               &ioTargetAttrib,
                                               &ioTarget
                                               );
                    if (!NT_SUCCESS(status))
                    {
                        goto exit;
                    }
                    
                    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                                                                &openParams,
                                                                &symbolicLinkName,
                                                                STANDARD_RIGHTS_ALL
                                                                );
                    status = WdfIoTargetOpen(
                                             ioTarget,
                                             &openParams
                                             );
                    if (!NT_SUCCESS(status))
                    {
                        WdfObjectDelete(ioTarget);
                        goto exit;
                    }

                    if (cirCtx->WdfIoNotificationTarget)
                    {
                        WdfObjectDelete(cirCtx->WdfIoNotificationTarget);
                        cirCtx->WdfIoNotificationTarget = nullptr;
                    }

                    cirCtx->WdfIoNotificationTarget = ioTarget;
                    ioTarget = nullptr;
                }
                break;
                case KSPROPERTY_ACXCIRCUIT_SETNOTIFICATIONID:
                {
                    if (params.Parameters.Property.Verb != AcxPropertyVerbSet)
                    {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                        goto exit;
                    }
                    
                    if (params.Parameters.Property.ValueCb != sizeof(GUID))
                    {
                        status = STATUS_INVALID_PARAMETER;
                        goto exit;
                    }

                    cirCtx->PnpNotificationId = *((LPGUID) params.Parameters.Property.Value);
                }
                break;
                case KSPROPERTY_ACXCIRCUIT_SETINSTANCEID:
                {
                    if (params.Parameters.Property.Verb != AcxPropertyVerbSet)
                    {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                        goto exit;
                    }
                    
                    if (params.Parameters.Property.ValueCb != sizeof(DWORD))
                    {
                        status = STATUS_INVALID_PARAMETER;
                        goto exit;
                    }

                    cirCtx->InstanceId = *((PDWORD) params.Parameters.Property.Value);
                }
                break;
            }
        }

        if (!NT_SUCCESS(status))
        {
            DrvLogWarning(g_SDCAVXuLog, FLAG_INIT, L"SdcaXuR_EvtCircuitRequestPreprocess - failure, %!STATUS!", status);
        }
    }
exit:
#endif

    //
    // Just give the request back to ACX.
    //
    (VOID)AcxCircuitDispatchAcxRequest((ACXCIRCUIT)Object, Request);
}

PAGED_CODE_SEG
_Use_decl_annotations_
VOID
SdcaXuR_EvtStreamRequestPreprocess(
    _In_    ACXOBJECT  Object,
    _In_    ACXCONTEXT DriverContext,
    _In_    WDFREQUEST Request
    )
/*++

Routine Description:

    This function is an example of a preprocess routine.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverContext);
    
    ASSERT(Object != NULL);
    ASSERT(DriverContext);
    ASSERT(Request);


    //
    // Just give the request back to ACX.
    //
    (VOID)AcxStreamDispatchAcxRequest((ACXSTREAM)Object, Request);
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuR_SetPowerPolicy(
    _In_ WDFDEVICE      Device
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    //WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

    PAGED_CODE();

    //
    // Init the idle policy structure.
    //
    //WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = IDLE_POWER_TIMEOUT;
    idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuR_EvtDevicePrepareHardware(
    _In_ WDFDEVICE      Device,
    _In_ WDFCMRESLIST   ResourceList,
    _In_ WDFCMRESLIST   ResourceListTranslated
)
/*++

Routine Description:

    In this callback, the driver does whatever is necessary to make the
    hardware ready to use.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    PSDCAXU_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);

    if (!devCtx->FirstTimePrepareHardware)
    {
        //
        // This is a rebalance. Validate the circuit resources and 
        // if needed, delete and re-create the circuit.
        // The sample driver doens't use resources, thus the existing 
        // circuits are kept.
        //
        return STATUS_SUCCESS;
    }

    //
    // Set child's power policy.
    //
    RETURN_NTSTATUS_IF_FAILED(SdcaXuR_SetPowerPolicy(Device));

    //
    // Add circuit to child's list.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuit(Device, devCtx->Circuit));

    //
    // Keep track this is not the first time this callback was called.
    //
    devCtx->FirstTimePrepareHardware = FALSE;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuR_EvtDeviceReleaseHardware(
    _In_ WDFDEVICE      Device,
    _In_ WDFCMRESLIST   ResourceListTranslated
)
/*++

Routine Description:

    In this callback, the driver releases the h/w resources allocated in the
    prepare h/w callback.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    PAGED_CODE();

    PSDCAXU_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);


    return status;
}

PAGED_CODE_SEG
VOID
SdcaXuR_EvtDeviceContextCleanup(
    _In_ WDFOBJECT      WdfDevice
)
/*++

Routine Description:

    In this callback, it cleans up render device context.

Arguments:

    WdfDevice - WDF device object

Return Value:

    NULL

--*/
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(WdfDevice);
}

PAGED_CODE_SEG
NTSTATUS
SdcaXuR_EvtDeviceSelfManagedIoInit(
    _In_ WDFDEVICE      Device
)
/*++

Routine Description:

     In this callback, the driver does one-time init of self-managed I/O data.

Arguments:

    Device - handle to a device

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    PSDCAXU_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
#pragma code_seg("PAGE")
NTSTATUS
SdcaXu_EvtProcessCommand0(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    BOOL                            fNewValue = FALSE;
    PVOID                           currentValue = nullptr;
    PVOID                           inBuffer = nullptr;
    ULONG                           inBufferCb = 0;
    PSDCAXU_AUDIOMODULE0_CONTEXT    audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *    parameterInfo = nullptr;
    AUDIOMODULE_CUSTOM_COMMAND *    command = nullptr;
    
    PAGED_CODE();

    audioModuleCtx = GetSdcaXuAudioModule0Context(AudioModule);
    RETURN_NTSTATUS_IF_TRUE(nullptr == audioModuleCtx, STATUS_INTERNAL_ERROR);

    //
    // Basic parameter validation (module specific).
    //
    RETURN_NTSTATUS_IF_TRUE(InBuffer == nullptr || InBufferCb == 0, STATUS_INVALID_PARAMETER);
    RETURN_NTSTATUS_IF_TRUE(InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND), STATUS_INVALID_PARAMETER);

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    RETURN_NTSTATUS_IF_TRUE(command->ParameterId >= SIZEOF_ARRAY(AudioModule0_ParameterInfo), STATUS_INVALID_PARAMETER);

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule0_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule0_ParameterInfo[AudioModuleParameter2];
            break;
        default:
            RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = NULL;
    }
    
    RETURN_NTSTATUS_IF_FAILED(AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue));
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

#ifndef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
#else
        if (audioModuleCtx->Circuit == nullptr)
        {
            RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
        }
        else
        {
            PSDCAXU_RENDER_CIRCUIT_CONTEXT cirCtx;

            cirCtx = GetRenderCircuitContext(audioModuleCtx->Circuit);

            // AcxPnpEventGenerateEvent will target the wrong PnpNotificationId, InstanceId, and IoTarget due to a lack
            // of acx framework support. Compose the PNP notification manually and send it.
            USHORT sizeRequired = FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer) + sizeof(KSAUDIOMODULE_NOTIFICATION) + sizeof(customNotification);
            PTARGET_DEVICE_CUSTOM_NOTIFICATION pCustomNotify = (PTARGET_DEVICE_CUSTOM_NOTIFICATION) new(POOL_FLAG_NON_PAGED, DRIVER_TAG) BYTE[sizeRequired];
            if (pCustomNotify != nullptr)
            {
                RtlZeroMemory(pCustomNotify, sizeRequired);
            
                pCustomNotify->NameBufferOffset = -1;
                pCustomNotify->Version = 1;
                pCustomNotify->Size = sizeRequired;
                pCustomNotify->Event = KSNOTIFICATIONID_AudioModule;
            
                PKSAUDIOMODULE_NOTIFICATION pModuleNotify = (PKSAUDIOMODULE_NOTIFICATION) &(pCustomNotify->CustomDataBuffer[0]);
                pModuleNotify->ProviderId.DeviceId = cirCtx->PnpNotificationId;
                pModuleNotify->ProviderId.ClassId = AudioModule0Id;
                pModuleNotify->ProviderId.InstanceId = (AUDIOMODULE_INSTANCE_ID(0,0) | cirCtx->InstanceId);
                RtlCopyMemory(pModuleNotify + 1, &customNotification, sizeof(customNotification));

                if (cirCtx->WdfIoNotificationTarget)
                {
                    PDEVICE_OBJECT devObj = WdfIoTargetWdmGetTargetPhysicalDevice(cirCtx->WdfIoNotificationTarget);
                
                    // if the notification target is invalidated, retrieving the physical device will fail and we can't send the event.
                    if (devObj)
                    {
                        IoReportTargetDeviceChangeAsynchronous(devObj,
                                                                   pCustomNotify,
                                                                   NULL,
                                                                   NULL);
                    }
                }
            
                delete[] pCustomNotify;
            }
        }
#endif
    }
   
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
#pragma code_seg("PAGE")
NTSTATUS
SdcaXu_EvtProcessCommand1(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    BOOL                            fNewValue = FALSE;
    PVOID                           currentValue = nullptr;
    PVOID                           inBuffer = nullptr;
    ULONG                           inBufferCb = 0;
    PSDCAXU_AUDIOMODULE1_CONTEXT    audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *    parameterInfo = nullptr;
    AUDIOMODULE_CUSTOM_COMMAND *    command = nullptr;
    
    PAGED_CODE();

    audioModuleCtx = GetSdcaXuAudioModule1Context(AudioModule);
    RETURN_NTSTATUS_IF_TRUE(nullptr == audioModuleCtx, STATUS_INTERNAL_ERROR);

    //
    // Basic parameter validation (module specific).
    //
    RETURN_NTSTATUS_IF_TRUE(InBuffer == nullptr || InBufferCb == 0, STATUS_INVALID_PARAMETER);
    RETURN_NTSTATUS_IF_TRUE(InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND), STATUS_INVALID_PARAMETER);

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    RETURN_NTSTATUS_IF_TRUE(command->ParameterId >= SIZEOF_ARRAY(AudioModule1_ParameterInfo), STATUS_INVALID_PARAMETER);

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModuleParameter2];
            break;
        case AudioModuleParameter3:
            currentValue = &audioModuleCtx->Parameter3;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModuleParameter3];
            break;
        default:
            RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = nullptr;
    }
    
    RETURN_NTSTATUS_IF_FAILED(AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue));
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

#ifndef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
#else
        if (audioModuleCtx->Circuit == nullptr)
        {
            RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
        }
        else
        {
            PSDCAXU_RENDER_CIRCUIT_CONTEXT cirCtx;

            cirCtx = GetRenderCircuitContext(audioModuleCtx->Circuit);

            // AcxPnpEventGenerateEvent will target the wrong PnpNotificationId, InstanceId, and IoTarget due to a lack
            // of acx framework support. Compose the PNP notification manually and send it.
            USHORT sizeRequired = FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer) + sizeof(KSAUDIOMODULE_NOTIFICATION) + sizeof(customNotification);
            PTARGET_DEVICE_CUSTOM_NOTIFICATION pCustomNotify = (PTARGET_DEVICE_CUSTOM_NOTIFICATION) new(POOL_FLAG_NON_PAGED, DRIVER_TAG) BYTE[sizeRequired];
            if (pCustomNotify != nullptr)
            {
                RtlZeroMemory(pCustomNotify, sizeRequired);
            
                pCustomNotify->NameBufferOffset = -1;
                pCustomNotify->Version = 1;
                pCustomNotify->Size = sizeRequired;
                pCustomNotify->Event = KSNOTIFICATIONID_AudioModule;
            
                PKSAUDIOMODULE_NOTIFICATION pModuleNotify = (PKSAUDIOMODULE_NOTIFICATION) &(pCustomNotify->CustomDataBuffer[0]);
                pModuleNotify->ProviderId.DeviceId = cirCtx->PnpNotificationId;
                pModuleNotify->ProviderId.ClassId = AudioModule1Id;
                pModuleNotify->ProviderId.InstanceId = (AUDIOMODULE_INSTANCE_ID(0,0) | cirCtx->InstanceId);
                RtlCopyMemory(pModuleNotify + 1, &customNotification, sizeof(customNotification));

                if (cirCtx->WdfIoNotificationTarget)
                {
                    PDEVICE_OBJECT devObj = WdfIoTargetWdmGetTargetPhysicalDevice(cirCtx->WdfIoNotificationTarget);
                
                    // if the notification target is invalidated, retrieving the physical device will fail and we can't send the event.
                    if (devObj)
                    {
                        IoReportTargetDeviceChangeAsynchronous(devObj,
                                                                   pCustomNotify,
                                                                   NULL,
                                                                   NULL);
                    }
                }
            
                delete[] pCustomNotify;
            }
        }
#endif
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
#pragma code_seg("PAGE")
NTSTATUS
SdcaXu_EvtProcessCommand2(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    BOOL                            fNewValue = FALSE;
    PVOID                           currentValue = nullptr;
    PVOID                           inBuffer = nullptr;
    ULONG                           inBufferCb = 0;
    PSDCAXU_AUDIOMODULE2_CONTEXT    audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *    parameterInfo = nullptr;
    AUDIOMODULE_CUSTOM_COMMAND *    command = nullptr;
    
    PAGED_CODE();

    audioModuleCtx = GetSdcaXuAudioModule2Context(AudioModule);
    RETURN_NTSTATUS_IF_TRUE(nullptr == audioModuleCtx, STATUS_INTERNAL_ERROR);

    //
    // Basic parameter validation (module specific).
    //
    RETURN_NTSTATUS_IF_TRUE(InBuffer == nullptr || InBufferCb == 0, STATUS_INVALID_PARAMETER);
    RETURN_NTSTATUS_IF_TRUE(InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND), STATUS_INVALID_PARAMETER);

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    RETURN_NTSTATUS_IF_TRUE(command->ParameterId >= SIZEOF_ARRAY(AudioModule2_ParameterInfo), STATUS_INVALID_PARAMETER);

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule2_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule2_ParameterInfo[AudioModuleParameter2];
            break;
        default:
            RETURN_NTSTATUS(STATUS_INVALID_PARAMETER);
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = nullptr;
    }
    
    RETURN_NTSTATUS_IF_FAILED(AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue));
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

#ifndef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
        RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
#else
        if (audioModuleCtx->Circuit == nullptr)
        {
            RETURN_NTSTATUS_IF_FAILED(AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification)));
        }
        else
        {
            PSDCAXU_RENDER_CIRCUIT_CONTEXT cirCtx;

            cirCtx = GetRenderCircuitContext(audioModuleCtx->Circuit);

            // AcxPnpEventGenerateEvent will target the wrong PnpNotificationId, InstanceId, and IoTarget due to a lack
            // of acx framework support. Compose the PNP notification manually and send it.
            USHORT sizeRequired = FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer) + sizeof(KSAUDIOMODULE_NOTIFICATION) + sizeof(customNotification);
            PTARGET_DEVICE_CUSTOM_NOTIFICATION pCustomNotify = (PTARGET_DEVICE_CUSTOM_NOTIFICATION) new(POOL_FLAG_NON_PAGED, DRIVER_TAG) BYTE[sizeRequired];
            if (pCustomNotify != nullptr)
            {
                RtlZeroMemory(pCustomNotify, sizeRequired);
            
                pCustomNotify->NameBufferOffset = -1;
                pCustomNotify->Version = 1;
                pCustomNotify->Size = sizeRequired;
                pCustomNotify->Event = KSNOTIFICATIONID_AudioModule;
            
                PKSAUDIOMODULE_NOTIFICATION pModuleNotify = (PKSAUDIOMODULE_NOTIFICATION) &(pCustomNotify->CustomDataBuffer[0]);
                pModuleNotify->ProviderId.DeviceId = cirCtx->PnpNotificationId;
                pModuleNotify->ProviderId.ClassId = AudioModule2Id;
                pModuleNotify->ProviderId.InstanceId = (AUDIOMODULE_INSTANCE_ID(1,0) | cirCtx->InstanceId);
                RtlCopyMemory(pModuleNotify + 1, &customNotification, sizeof(customNotification));

                if (cirCtx->WdfIoNotificationTarget)
                {
                    PDEVICE_OBJECT devObj = WdfIoTargetWdmGetTargetPhysicalDevice(cirCtx->WdfIoNotificationTarget);
                
                    // if the notification target is invalidated, retrieving the physical device will fail and we can't send the event.
                    if (devObj)
                    {
                        IoReportTargetDeviceChangeAsynchronous(devObj,
                                                                   pCustomNotify,
                                                                   NULL,
                                                                   NULL);
                    }
                }
            
                delete[] pCustomNotify;
            }
        }
#endif
    }

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
NTSTATUS
SdcaXu_CreateCircuitModules(
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit
    )
/*++

Routine Description:

    This routine creates all of the audio module elements and adds them to the circuit

Return Value:

    NT status value

--*/
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    ACX_AUDIOMODULE_CALLBACKS       audioModuleCallbacks;
    ACX_AUDIOMODULE_CONFIG          audioModuleCfg;
    ACXAUDIOMODULE                  audioModuleElement;
    PSDCAXU_AUDIOMODULE0_CONTEXT    audioModule0Ctx;
    PSDCAXU_AUDIOMODULE1_CONTEXT    audioModule1Ctx;
    PSDCAXU_AUDIOMODULE2_CONTEXT    audioModule2Ctx;
    ACX_PNPEVENT_CONFIG             audioModuleEventCfg;
    ACXPNPEVENT                     audioModuleEvent;

    PAGED_CODE();

    // Now add audio modules to the circuit
    // module 0

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand0;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule0Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule0Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(0,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE0_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE0_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE0DESCRIPTION,
                wcslen(AUDIOMODULE0DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE0_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule0Ctx = GetSdcaXuAudioModule0Context(audioModuleElement);
    ASSERT(audioModule0Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule0Ctx->Event = audioModuleEvent;
    audioModule0Ctx->Circuit = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1));

    // module 1

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand1;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule1Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule1Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(0,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE1_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE1_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE1DESCRIPTION,
                wcslen(AUDIOMODULE1DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE1_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule1Ctx = GetSdcaXuAudioModule1Context(audioModuleElement);
    ASSERT(audioModule1Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule1Ctx->Event = audioModuleEvent;
    audioModule1Ctx->Circuit = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1));

    // module 2

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand2;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule2Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule2Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE2_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE2_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE2DESCRIPTION,
                wcslen(AUDIOMODULE2DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE2_CONTEXT);
    attributes.ParentObject = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule2Ctx = GetSdcaXuAudioModule2Context(audioModuleElement);
    ASSERT(audioModule2Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule2Ctx->Event = audioModuleEvent;
    audioModule2Ctx->Circuit = Circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateRenderDevice(
    _In_ WDFDEVICE      Device,
    _Out_ WDFDEVICE* RenderDevice
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE renderDevice = NULL;

    PAGED_CODE();

    auto exit = scope_exit([&status, &renderDevice]() {
        if (!NT_SUCCESS(status))
        {
            if (renderDevice != NULL)
            {
                WdfObjectDelete(renderDevice);
            }
        }
    });

    *RenderDevice = NULL;

    //
    // Create a child audio device for this circuit.
    //
    PWDFDEVICE_INIT devInit = NULL;
    devInit = WdfPdoInitAllocate(Device);
    RETURN_NTSTATUS_IF_TRUE(NULL == devInit, STATUS_INSUFFICIENT_RESOURCES);

    auto devInit_free = scope_exit([&devInit, &status]() {
        WdfDeviceInitFree(devInit);
    });

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddHardwareID(devInit, &RenderHardwareId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignDeviceID(devInit, &RenderDeviceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddCompatibleID(devInit, &RenderCompatibleId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignInstanceID(devInit, &RenderInstanceId));

    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAssignContainerID(devInit, &RenderContainerId));


    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    RETURN_NTSTATUS_IF_FAILED(WdfPdoInitAddDeviceText(devInit,
        &RenderDeviceDescription,
        &RenderDeviceLocation,
        0x409));

    WdfPdoInitSetDefaultLocale(devInit, 0x409);

    //
    // Allow ACX to add any pre-requirement it needs on this device.
    //
    ACX_DEVICEINIT_CONFIG acxDevInitCfg;
    ACX_DEVICEINIT_CONFIG_INIT(&acxDevInitCfg);
    acxDevInitCfg.Flags |= AcxDeviceInitConfigRawDevice;
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitInitialize(devInit, &acxDevInitCfg));

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = SdcaXuR_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = SdcaXuR_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = SdcaXuR_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(devInit, &pnpPowerCallbacks);

    //
    // Specify a context for this render device.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_RENDER_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuR_EvtDeviceContextCleanup;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;
    RETURN_NTSTATUS_IF_FAILED(WdfDeviceCreate(&devInit, &attributes, &renderDevice));

    //
    // devInit attached to device, no need to free
    //
    devInit_free.release();

    //
    // Tell the framework to set the NoDisplayInUI in the DeviceCaps so
    // that the device does not show up in Device Manager.
    //
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.NoDisplayInUI = WdfTrue;
    WdfDeviceSetPnpCapabilities(renderDevice, &pnpCaps);

    //
    // Init render's device context.
    //
    PSDCAXU_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(renderDevice);
    ASSERT(devCtx != NULL);
    UNREFERENCED_PARAMETER(devCtx);

    //
    // Allow ACX to add any post-requirement it needs on this device.
    //
    ACX_DEVICE_CONFIG devCfg;
    ACX_DEVICE_CONFIG_INIT(&devCfg);
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceInitialize(renderDevice, &devCfg));

    //
    // Set output value.
    //
    *RenderDevice = renderDevice;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddDynamicRender(
    _In_ WDFDEVICE                      Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG     CircuitConfig
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Create a device to associated with this circuit.
    //
    WDFDEVICE renderDevice = NULL;
    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateRenderDevice(Device, &renderDevice));
    auto deviceFree = scope_exit([&renderDevice]() {
        WdfObjectDelete(renderDevice);
    });

    ASSERT(renderDevice);
    PSDCAXU_RENDER_DEVICE_CONTEXT renderDevCtx;
    renderDevCtx = GetRenderDeviceContext(renderDevice);
    ASSERT(renderDevCtx);

    //
    // Create a render circuit associated with this child device.
    //
    ACXCIRCUIT renderCircuit = NULL;
    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateRenderCircuit(renderDevice, CircuitConfig, &renderCircuit));

    renderDevCtx->Circuit = renderCircuit;
    renderDevCtx->FirstTimePrepareHardware = TRUE;

    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateCircuitModules(Device, renderCircuit));

    //
    // Add circuit to device's dynamic circuit device list.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxDeviceAddCircuitDevice(Device, renderDevice));

    // Successfully created circuit for dynamic deivce
    // Do not delete
    deviceFree.release();

    PSDCAXU_DEVICE_CONTEXT devCtx = GetSdcaXuDeviceContext(Device);
    for (ULONG i = 0; i < ARRAYSIZE(devCtx->EndpointDevices); ++i)
    {
        if (devCtx->EndpointDevices[i].CircuitDevice == nullptr)
        {
            DrvLogInfo(g_SDCAVXuLog, FLAG_DDI, L"XU Device %p adding render circuit device %p with component ID %!GUID! and Uri %ls",
                Device, renderDevice, &CircuitConfig->ComponentID,
                CircuitConfig->ComponentUri.Buffer ? CircuitConfig->ComponentUri.Buffer : L"<none>");
            devCtx->EndpointDevices[i].CircuitDevice = renderDevice;
            devCtx->EndpointDevices[i].CircuitId = CircuitConfig->ComponentID;
            if (CircuitConfig->ComponentUri.Length > 0)
            {
                USHORT cbAlloc = CircuitConfig->ComponentUri.Length + sizeof(WCHAR);
                // protect against overflow
                if (CircuitConfig->ComponentUri.Length % 2 != 0 ||
                    cbAlloc < CircuitConfig->ComponentUri.Length)
                {
                    RETURN_NTSTATUS_IF_FAILED(STATUS_INVALID_PARAMETER);
                }

                PWCHAR circuitUri = (PWCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, cbAlloc, DRIVER_TAG);
                if (!circuitUri)
                {
                    RETURN_NTSTATUS_IF_FAILED(STATUS_INSUFFICIENT_RESOURCES);
                }

                devCtx->EndpointDevices[i].CircuitUri.Buffer = circuitUri;
                devCtx->EndpointDevices[i].CircuitUri.MaximumLength = cbAlloc;
                devCtx->EndpointDevices[i].CircuitUri.Length = 0;
                RtlCopyUnicodeString(&devCtx->EndpointDevices[i].CircuitUri, &CircuitConfig->ComponentUri);
            }
            break;
        }
    }

    return status;
}

// {3CE41646-9BF2-4A9E-B851-D711CAE9AEA8}
DEFINE_GUID(SDCAVADPropsetId,
    0x3ce41646, 0x9bf2, 0x4a9e, 0xb8, 0x51, 0xd7, 0x11, 0xca, 0xe9, 0xae, 0xa8);

typedef enum {
    SDCAVAD_PROPERTY_TEST1,
    SDCAVAD_PROPERTY_TEST2,
    SDCAVAD_PROPERTY_TEST3,
    SDCAVAD_PROPERTY_TEST4,
    SDCAVAD_PROPERTY_TEST5,
    SDCAVAD_PROPERTY_TEST6,
} SDCAVAD_Properties;

PAGED_CODE_SEG
NTSTATUS
SdcaXu_SDCAVADPropertyTest3(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pValue);
    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVXuLog, FLAG_STREAM, L"SDCAVXu: SDCAVAD_PROPERTY_TEST3");

    *ValueCbOut = 0;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_SDCAVADPropertyTest4(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVXuLog, FLAG_STREAM, L"SDCAVXu: SDCAVAD_PROPERTY_TEST4");

    *((PULONG)pValue) = 11;
    *ValueCbOut = sizeof(ULONG);

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_SDCAVADPropertyTest5(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pValue);
    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVXuLog, FLAG_STREAM, L"SDCAVXu: SDCAVAD_PROPERTY_TEST5");

    *ValueCbOut = 0;

    return status;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_SDCAVADPropertyTest6(
    _Inout_ PVOID pValue,
    _In_ ULONG ValueCb,
    _Out_ PULONG ValueCbOut
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ValueCb);

    NTSTATUS status = STATUS_SUCCESS;

    DrvLogInfo(g_SDCAVXuLog, FLAG_STREAM, L"SDCAVXu: SDCAVAD_PROPERTY_TEST6");

    *((PULONG)pValue) = 13;
    *ValueCbOut = sizeof(ULONG);

    return status;
}

PAGED_CODE_SEG
VOID
SdcaXu_EvtPropertyCallback(
    _In_    WDFOBJECT   Object,
    _In_    WDFREQUEST  Request
)
{
    UNREFERENCED_PARAMETER(Object);

    PAGED_CODE();

    ACX_REQUEST_PARAMETERS params;
    ACX_REQUEST_PARAMETERS_INIT(&params);

    AcxRequestGetParameters(Request, &params);

    NTSTATUS status = STATUS_SUCCESS;
    PVOID Value = params.Parameters.Property.Value;
    ULONG ValueCb = params.Parameters.Property.ValueCb;
    ULONG ValueCbOut = 0;

    if (IsEqualGUID(params.Parameters.Property.Set, SDCAVADPropsetId))
    {
        switch (params.Parameters.Property.Id)
        {
        case SDCAVAD_PROPERTY_TEST3:
            status = SdcaXu_SDCAVADPropertyTest3(Value, ValueCb, &ValueCbOut);
            break;
        case SDCAVAD_PROPERTY_TEST4:
            status = SdcaXu_SDCAVADPropertyTest4(Value, ValueCb, &ValueCbOut);
            break;
        case SDCAVAD_PROPERTY_TEST5:
            status = SdcaXu_SDCAVADPropertyTest5(Value, ValueCb, &ValueCbOut);
            break;
        case SDCAVAD_PROPERTY_TEST6:
            status = SdcaXu_SDCAVADPropertyTest6(Value, ValueCb, &ValueCbOut);
            break;
        default:
            break;
        }
    }

    WdfRequestCompleteWithInformation(Request, status, ValueCbOut);
}

static ACX_PROPERTY_ITEM CircuitProperties[] =
{
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST3,
        ACX_PROPERTY_ITEM_FLAG_SET,
        SdcaXu_EvtPropertyCallback
    },
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST4,
        ACX_PROPERTY_ITEM_FLAG_GET,
        SdcaXu_EvtPropertyCallback
    },
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST5,
        ACX_PROPERTY_ITEM_FLAG_SET,
        SdcaXu_EvtPropertyCallback
    },
    {
        &SDCAVADPropsetId,
        SDCAVAD_PROPERTY_TEST6,
        ACX_PROPERTY_ITEM_FLAG_GET,
        SdcaXu_EvtPropertyCallback
    },
};

PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateRenderCircuit(
    _In_ WDFDEVICE                      Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG     CircuitConfig,
    _Out_ ACXCIRCUIT                    *Circuit
)
/*++

Routine Description:

    This routine builds the SdcaXu render circuit.

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    //
    // Get a CircuitInit structure.
    //
    PACXCIRCUIT_INIT circuitInit = NULL;
    circuitInit = AcxCircuitInitAllocate(Device);
    RETURN_NTSTATUS_IF_TRUE(NULL == circuitInit, STATUS_MEMORY_NOT_ALLOCATED);
    auto circuitInit_free = scope_exit([&circuitInit]() {
        AcxCircuitInitFree(circuitInit);
    });

    //
    // Init output value.
    //
    *Circuit = NULL;

    //
    // Copy Circuit configuration
    //
    PSDCAXU_ACX_CIRCUIT_CONFIG pCircuitConfig = NULL;
    pCircuitConfig = (PSDCAXU_ACX_CIRCUIT_CONFIG)ExAllocatePool2(POOL_FLAG_NON_PAGED,
        CircuitConfig->cbSize,
        DRIVER_TAG);
    RETURN_NTSTATUS_IF_TRUE(NULL == pCircuitConfig, STATUS_MEMORY_NOT_ALLOCATED);
    auto circuitConfig_free = scope_exit([&pCircuitConfig]() {
        ExFreePoolWithTag(pCircuitConfig, DRIVER_TAG);
        });

    RtlCopyMemory(pCircuitConfig, CircuitConfig, CircuitConfig->cbSize);

    // Remap UNICODE_STRING.Buffer
    // buffer for unicode string begins immediately after SdcaXuAcxCircuitConfig
    RETURN_NTSTATUS_IF_TRUE_MSG(pCircuitConfig->cbSize < (sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) + pCircuitConfig->CircuitName.MaximumLength),
        STATUS_INVALID_PARAMETER, L"CircuitConfig->cbSize = %d Required = %d",
        pCircuitConfig->cbSize,
        (int)(sizeof(SDCAXU_ACX_CIRCUIT_CONFIG) + pCircuitConfig->CircuitName.MaximumLength));

    pCircuitConfig->CircuitName.Buffer = (PWCH)(pCircuitConfig + 1);

    //
    // Create a circuit.
    //

    //
    // Add circuit identifiers.
    //
    if (!IsEqualGUID(pCircuitConfig->ComponentID, GUID_NULL))
    {
        AcxCircuitInitSetComponentId(circuitInit, &pCircuitConfig->ComponentID);
    }

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignComponentUri(circuitInit, &pCircuitConfig->ComponentUri));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignName(circuitInit, &pCircuitConfig->CircuitName));

    //
    // Add circuit type.
    //
    AcxCircuitInitSetCircuitType(circuitInit, AcxCircuitTypeRender);

    //
    // Assign the circuit's pnp-power callbacks.
    //
    {
        ACX_CIRCUIT_PNPPOWER_CALLBACKS powerCallbacks;
        ACX_CIRCUIT_PNPPOWER_CALLBACKS_INIT(&powerCallbacks);
        powerCallbacks.EvtAcxCircuitPowerUp = SdcaXuR_EvtCircuitPowerUp;
        powerCallbacks.EvtAcxCircuitPowerDown = SdcaXuR_EvtCircuitPowerDown;
        AcxCircuitInitSetAcxCircuitPnpPowerCallbacks(circuitInit, &powerCallbacks);
    }

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxRequestPreprocessCallback(
        circuitInit,
        SdcaXuR_EvtCircuitRequestPreprocess,
        (ACXCONTEXT)AcxRequestTypeAny, // dbg only
        AcxRequestTypeAny,
        NULL,
        AcxItemIdNone));

    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignAcxCreateStreamCallback(
        circuitInit,
        SdcaXuR_EvtCircuitCreateStream));

    //
    // Disable default Stream Bridge handling in ACX
    // Create stream handler will add Stream Bridge
    // to support Object-bag forwarding
    //
    AcxCircuitInitDisableDefaultStreamBridgeHandling(circuitInit);

    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitInitAssignProperties(circuitInit,
        CircuitProperties,
        SIZEOF_ARRAY(CircuitProperties)));

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    ACXCIRCUIT circuit;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_RENDER_CIRCUIT_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuR_EvtCircuitContextCleanup;
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitCreate(Device, &attributes, &circuitInit, &circuit));

    // circuitInit is now associated with circuit and will be managed with
    // circuit lifetime.
    circuitInit_free.release();

    SDCAXU_RENDER_CIRCUIT_CONTEXT *circuitCtx;
    ASSERT(circuit != NULL);
    circuitCtx = GetRenderCircuitContext(circuit);
    ASSERT(circuitCtx);

#ifdef ACX_WORKAROUND_AGGREGATED_MODULE_NOTIFICATIONS
    // cache the parent device for later, unreferenced since
    // this is the parent
    circuitCtx->EndpointDevice = Device;
#endif

    circuitCtx->CircuitConfig = pCircuitConfig;
    circuitConfig_free.release();

    //
    // Post circuit creation initialization.
    //

    //
    // Add two custom circuit elements. Note that driver doesn't need to 
    // perform this step if it doesn't want to expose any circuit elements.
    //

    //
    // Create 1st custom circuit-element.
    //
    ACX_ELEMENT_CONFIG elementCfg;
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    const int numElements = 2;
    ACXELEMENT elements[numElements] = { 0 };
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    SDCAXU_ELEMENT_CONTEXT* elementCtx;
    elementCtx = GetSdcaXuElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Create 2nd custom circuit-element.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = circuit;

    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(circuit, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetSdcaXuElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add the circuit elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddElements(circuit, elements, SIZEOF_ARRAY(elements)));

    //
    // Create render pin. AcxCircuit creates the other pin by default.
    //

    ACX_PIN_CALLBACKS pinCallbacks;
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinSetDataFormat = SdcaXuR_EvtAcxPinSetDataFormat;

    ACX_PIN_CONFIG pinCfg;
    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSink;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSCATEGORY_AUDIO;
    pinCfg.PinCallbacks = &pinCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PIN_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuR_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    ACXPIN pin;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    SDCAXU_PIN_CONTEXT* pinCtx;
    pinCtx = GetSdcaXuPinContext(pin);
    ASSERT(pinCtx);

    // When the downstream pin connects to the Class driver, we'll
    // copy formats from the Class driver (instead of hardcoding
    // formats here)

    //
    // Add render pin, using default pin id (0) 
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    ///////////////////////////////////////////////////////////
    //
    // Create bridge pin. AcxCircuit creates the other pin by default.
    //
    ACX_PIN_CALLBACKS_INIT(&pinCallbacks);
    pinCallbacks.EvtAcxPinConnected = SdcaXu_EvtPinConnected;

    ACX_PIN_CONFIG_INIT(&pinCfg);
    pinCfg.Type = AcxPinTypeSource;
    pinCfg.Communication = AcxPinCommunicationNone;
    pinCfg.Category = &KSCATEGORY_AUDIO;
    pinCfg.PinCallbacks = &pinCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PIN_CONTEXT);
    attributes.EvtCleanupCallback = SdcaXuR_EvtPinContextCleanup;
    attributes.ParentObject = circuit;

    pin = NULL;
    RETURN_NTSTATUS_IF_FAILED(AcxPinCreate(circuit, &attributes, &pinCfg, &pin));

    ASSERT(pin != NULL);
    pinCtx = GetSdcaXuPinContext(pin);
    ASSERT(pinCtx);

    //
    // Add brige pin, using default pin id (1)
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddPins(circuit, &pin, 1));

    //
    // Add a stream bridge to the bridge pin to propagate the stream obj-bags.
    //
    {
        PCGUID  inModes[] =
        {
            &NULL_GUID, // Match every mode.
        };

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = pin;

        ACX_STREAM_BRIDGE_CONFIG streamBridgeConfig;
        ACX_STREAM_BRIDGE_CONFIG_INIT(&streamBridgeConfig);

        streamBridgeConfig.Flags |= AcxStreamBridgeForwardInStreamVarArguments;
        streamBridgeConfig.InModesCount = ARRAYSIZE(inModes);
        streamBridgeConfig.InModes = inModes;
        streamBridgeConfig.OutMode = &NULL_GUID; // Use the MODE associated the in-stream.

        ACXSTREAMBRIDGE streamBridge = NULL;
        RETURN_NTSTATUS_IF_FAILED(AcxStreamBridgeCreate(circuit, &attributes, &streamBridgeConfig, &streamBridge));

        RETURN_NTSTATUS_IF_FAILED(AcxPinAddStreamBridges(pin, &streamBridge, 1));
    }

    //
    // Explicitly connect the circuit/elements. Note that driver doens't 
    // need to perform this step when circuit/elements are connected in the 
    // same order as they were added to the circuit. By default ACX connects
    // the elements starting from the sink circuit pin and ending with the 
    // source circuit pin for both render and capture devices.
    //
    // circuit.pin[default_sink]    -> 1st element.pin[default_in]
    // 1st element.pin[default_out] -> 2nd element.pin[default_in]
    // 2nd element.pin[default_out] -> circuit.pin[default_source]
    //
    const int numConnections = numElements + 1;
    ACX_CONNECTION connections[numConnections];
    ACX_CONNECTION_INIT(&connections[0], circuit, elements[0]);
    ACX_CONNECTION_INIT(&connections[1], elements[0], elements[1]);
    ACX_CONNECTION_INIT(&connections[2], elements[1], circuit);

    //
    // Add the connections linking circuit to elements.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxCircuitAddConnections(circuit, connections, SIZEOF_ARRAY(connections)));

    //
    // Set output value.
    //
    *Circuit = circuit;

    return STATUS_SUCCESS;
}

#pragma code_seg()
_Use_decl_annotations_
NTSTATUS
SdcaXuR_EvtCircuitPowerUp (
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
{
    // Do not page out.
    
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(PreviousState);
    
    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
_Use_decl_annotations_
NTSTATUS
SdcaXuR_EvtCircuitPowerDown (
    _In_ WDFDEVICE  Device,
    _In_ ACXCIRCUIT Circuit,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(TargetState);
    
    return STATUS_SUCCESS;
}


#pragma code_seg("PAGE")
NTSTATUS
SdcaXu_CreateStreamModules(
    _In_ WDFDEVICE  Device,
    _In_ ACXSTREAM  Stream
    )
/*++

Routine Description:

    This routine creates all of the audio module elements and adds them to the stream

Return Value:

    NT status value

--*/
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    ACX_AUDIOMODULE_CALLBACKS       audioModuleCallbacks;
    ACX_AUDIOMODULE_CONFIG          audioModuleCfg;
    ACXAUDIOMODULE                  audioModuleElement;
    PSDCAXU_AUDIOMODULE0_CONTEXT    audioModule0Ctx;
    PSDCAXU_AUDIOMODULE1_CONTEXT    audioModule1Ctx;
    PSDCAXU_AUDIOMODULE2_CONTEXT    audioModule2Ctx;
    ACX_PNPEVENT_CONFIG             audioModuleEventCfg;
    ACXPNPEVENT                     audioModuleEvent;

    PAGED_CODE();

    // Now add audio modules to the circuit
    // module 0
    // for simplicity of the example, we implement the same modules on the stream as is
    // on the circuit
    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand0;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule0Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule0Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE0_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE0_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE0DESCRIPTION,
                wcslen(AUDIOMODULE0DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE0_CONTEXT);
    attributes.ParentObject = Stream;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Stream, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule0Ctx = GetSdcaXuAudioModule0Context(audioModuleElement);
    ASSERT(audioModule0Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule0Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(Stream, (ACXELEMENT *) &audioModuleElement, 1));

    // module 1

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand1;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule1Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule1Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE1_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE1_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE1DESCRIPTION,
                wcslen(AUDIOMODULE1DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE1_CONTEXT);
    attributes.ParentObject = Stream;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Stream, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule1Ctx = GetSdcaXuAudioModule1Context(audioModuleElement);
    ASSERT(audioModule1Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule1Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(Stream, (ACXELEMENT *) &audioModuleElement, 1));

    // module 2

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand2;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModule2Id;
    audioModuleCfg.Descriptor.ClassId = AudioModule2Id;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(1,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE2_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE2_MINOR;
    RETURN_NTSTATUS_IF_FAILED(RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULE2DESCRIPTION,
                wcslen(AUDIOMODULE2DESCRIPTION)));

    audioModuleCfg.Callbacks = &audioModuleCallbacks;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE2_CONTEXT);
    attributes.ParentObject = Stream;

    RETURN_NTSTATUS_IF_FAILED(AcxAudioModuleCreate(Stream, &attributes, &audioModuleCfg, &audioModuleElement));

    audioModule2Ctx = GetSdcaXuAudioModule2Context(audioModuleElement);
    ASSERT(audioModule2Ctx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    RETURN_NTSTATUS_IF_FAILED(AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent));

    audioModule2Ctx->Event = audioModuleEvent;

    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(Stream, (ACXELEMENT *) &audioModuleElement, 1));

    return STATUS_SUCCESS;
}


PAGED_CODE_SEG
NTSTATUS
SdcaXuR_EvtCircuitCreateStream(
    _In_    WDFDEVICE       Device,
    _In_    ACXCIRCUIT      Circuit,
    _In_    ACXPIN          Pin,
    _In_    PACXSTREAM_INIT StreamInit,
    _In_    ACXDATAFORMAT   StreamFormat,
    _In_    const GUID    * SignalProcessingMode,
    _In_    ACXOBJECTBAG    VarArguments
)
/*++

Routine Description:

    This routine create a stream for the specified circuit.

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(SignalProcessingMode);
    UNREFERENCED_PARAMETER(VarArguments);

    ASSERT(IsEqualGUID(*SignalProcessingMode, AUDIO_SIGNALPROCESSINGMODE_RAW));
    
    PSDCAXU_RENDER_DEVICE_CONTEXT devCtx;
    devCtx = GetRenderDeviceContext(Device);
    ASSERT(devCtx != NULL);

    //
    // Set circuit-callbacks.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxRequestPreprocessCallback(
                                            StreamInit, 
                                            SdcaXuR_EvtStreamRequestPreprocess,
                                            (ACXCONTEXT)AcxRequestTypeAny, // dbg only
                                            AcxRequestTypeAny,
                                            NULL, 
                                            AcxItemIdNone));

    /*
    //
    // Add properties, events and methods.
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignProperties(StreamInit,
                                         StreamProperties,
                                         StreamPropertiesCount));
    */
    
    //
    // Init streaming callbacks.
    //
    ACX_STREAM_CALLBACKS streamCallbacks;
    ACX_STREAM_CALLBACKS_INIT(&streamCallbacks);
    streamCallbacks.EvtAcxStreamPrepareHardware     = SdcaXu_EvtStreamPrepareHardware;
    streamCallbacks.EvtAcxStreamReleaseHardware     = SdcaXu_EvtStreamReleaseHardware;
    streamCallbacks.EvtAcxStreamRun                 = SdcaXu_EvtStreamRun;
    streamCallbacks.EvtAcxStreamPause               = SdcaXu_EvtStreamPause;
    streamCallbacks.EvtAcxStreamAssignDrmContentId  = SdcaXu_EvtStreamAssignDrmContentId;
        
    RETURN_NTSTATUS_IF_FAILED(AcxStreamInitAssignAcxStreamCallbacks(StreamInit, &streamCallbacks));

    //
    // Create the stream.
    //
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_STREAM_CONTEXT);
    attributes.EvtDestroyCallback = SdcaXu_EvtStreamDestroy;
    ACXSTREAM stream;
    RETURN_NTSTATUS_IF_FAILED(AcxStreamCreate(Device, Circuit, &attributes, &StreamInit, &stream));

    CRenderStreamEngine *streamEngine = NULL;
    streamEngine = new(POOL_FLAG_NON_PAGED, DRIVER_TAG) CRenderStreamEngine(stream, StreamFormat);
    RETURN_NTSTATUS_IF_TRUE(NULL == streamEngine, STATUS_MEMORY_NOT_ALLOCATED);
    auto stream_scope = scope_exit([&streamEngine]() {
        delete streamEngine;
    });

    SDCAXU_STREAM_CONTEXT *streamCtx;
    streamCtx = GetSdcaXuStreamContext(stream);
    ASSERT(streamCtx);
    streamCtx->StreamEngine = (PVOID)streamEngine;
    stream_scope.release();

    //
    // Post stream creation initialization.
    //

    ACXELEMENT elements[2] = {0};
    ACX_ELEMENT_CONFIG elementCfg;
    //
    // Create 1st custom stream-elements.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;
    
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[0]));

    ASSERT(elements[0] != NULL);
    SDCAXU_ELEMENT_CONTEXT *elementCtx;
    elementCtx = GetSdcaXuElementContext(elements[0]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);
    
    //
    // Create 2nd custom stream-elements.
    //
    ACX_ELEMENT_CONFIG_INIT(&elementCfg);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_ELEMENT_CONTEXT);
    attributes.ParentObject = stream;
    
    RETURN_NTSTATUS_IF_FAILED(AcxElementCreate(stream, &attributes, &elementCfg, &elements[1]));

    ASSERT(elements[1] != NULL);
    elementCtx = GetSdcaXuElementContext(elements[1]);
    ASSERT(elementCtx);
    UNREFERENCED_PARAMETER(elementCtx);

    //
    // Add stream elements
    //
    RETURN_NTSTATUS_IF_FAILED(AcxStreamAddElements(stream, elements, SIZEOF_ARRAY(elements)));

    //
    // Add stream modules
    //
    RETURN_NTSTATUS_IF_FAILED(SdcaXu_CreateStreamModules(Device, stream));

    return STATUS_SUCCESS;
}

PAGED_CODE_SEG
NTSTATUS
SdcaXu_AddRenders(
    _In_ WDFDEVICE                      Device,
    _In_ PSDCAXU_ACX_CIRCUIT_CONFIG     CircuitConfig
)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Add dynamic render circuit using raw PDO
    //
    status = SdcaXu_AddDynamicRender(Device, CircuitConfig);

    return status;
}



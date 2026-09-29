/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ModuleCircuit.cpp

Abstract:

    Circuit implementation that hosts an AudioModule

Environment:

    Kernel mode

--*/

#include "private.h"
#include "audiomodule.h"
#include <devguid.h>
#include "stdunk.h"
#include <ks.h>
#include <mmsystem.h>
#include <ksmedia.h>

#include "ModuleCircuit.h"

#ifndef __INTELLISENSE__
#include "ModuleCircuit.tmh"
#endif

DEFINE_GUID(SDCAXU_MODULECIRCUIT_GUID, 
0x63434534, 0xBD84, 0x8DFE, 0x7A, 0xAA, 0xFF, 0x84, 0xD8, 0x23, 0xAB, 0xBD);

DEFINE_GUID(KSCATEGORY_ACXCIRCUIT,
0x2c6bb644L, 0xe1ae, 0x47f8, 0x9a, 0x2b, 0x1d, 0x1f, 0xa7, 0x50, 0xf2, 0xfa);

DEFINE_GUID(SDCAXU_FACTORY_CATEGORY,
0x1983badd, 0x5cd, 0x4dc8, 0x83, 0xe5, 0x84, 0xaf, 0x83, 0xdf, 0xb0, 0xc3);

//
// Name of circuit hosting an XU module.
//
DECLARE_CONST_UNICODE_STRING(s_ModuleCircuitName, L"ExtensionModuleCircuit");


PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtProcessCommand(
    _In_    ACXAUDIOMODULE  AudioModule,
    _In_    PVOID           InBuffer,
    _In_    ULONG           InBufferCb,
    _In_    PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferCb
   )
{
    PAGED_CODE();

    NTSTATUS                            status = STATUS_SUCCESS;
    BOOL                                fNewValue = FALSE;
    PVOID                               currentValue;
    PVOID                               inBuffer = NULL;
    ULONG                               inBufferCb = 0;
    PSDCAXU_AUDIOMODULE_CONTEXT         audioModuleCtx;
    AUDIOMODULE_PARAMETER_INFO *        parameterInfo = NULL;
    AUDIOMODULE_CUSTOM_COMMAND *        command = NULL;
    
    audioModuleCtx = GetSdcaXuAudioModuleContext(AudioModule);
    if (audioModuleCtx == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        status = STATUS_INTERNAL_ERROR;
        goto exit;
    }

    //
    // Basic parameter validation (module specific).
    //
    if (InBuffer == NULL || InBufferCb == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (InBufferCb < sizeof(AUDIOMODULE_CUSTOM_COMMAND))
    {
        return STATUS_INVALID_PARAMETER;
    }

    command = (AUDIOMODULE_CUSTOM_COMMAND*)InBuffer;  

    if (command->ParameterId >= SIZEOF_ARRAY(AudioModule_ParameterInfo))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModuleParameter1:
            currentValue = &audioModuleCtx->Parameter1;
            parameterInfo = &AudioModule_ParameterInfo[AudioModuleParameter1];
            break;
        case AudioModuleParameter2:
            currentValue = &audioModuleCtx->Parameter2;
            parameterInfo = &AudioModule_ParameterInfo[AudioModuleParameter2];
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            goto exit;
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
    
    status = AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue);

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
    if (fNewValue &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        AUDIOMODULE_CUSTOM_NOTIFICATION customNotification = {0};

        customNotification.Type = AudioModuleParameterChanged;
        customNotification.ParameterChanged.ParameterId = command->ParameterId;

        status = AcxPnpEventGenerateEvent(audioModuleCtx->Event, &customNotification, (USHORT)sizeof(customNotification));
        if (!NT_SUCCESS(status))
        {
            goto exit;
        }
    }

    // Normalize error code.
    status = STATUS_SUCCESS;
    
exit:
    return status;
}


PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateModuleCircuitModules(
    _In_ WDFDEVICE          Device,
    _In_ ACXCIRCUIT         Circuit
    )
/*++

Routine Description:

    This routine creates all of the audio module elements and adds them to the circuit

Return Value:

    NT status value

--*/
{
    PAGED_CODE();

    NTSTATUS                            status;
    WDF_OBJECT_ATTRIBUTES               attributes;
    ACX_AUDIOMODULE_CALLBACKS           audioModuleCallbacks;
    ACX_AUDIOMODULE_CONFIG              audioModuleCfg;
    ACXAUDIOMODULE                      audioModuleElement;
    PSDCAXU_AUDIOMODULE_CONTEXT         audioModuleCtx;
    ACX_PNPEVENT_CONFIG                 audioModuleEventCfg;
    ACXPNPEVENT                         audioModuleEvent;

    ACX_AUDIOMODULE_CALLBACKS_INIT(&audioModuleCallbacks);
    audioModuleCallbacks.EvtAcxAudioModuleProcessCommand = SdcaXu_EvtProcessCommand;

    ACX_AUDIOMODULE_CONFIG_INIT(&audioModuleCfg);
    audioModuleCfg.Name = &AudioModuleId;
    audioModuleCfg.Descriptor.ClassId = AudioModuleId;
    audioModuleCfg.Descriptor.InstanceId = AUDIOMODULE_INSTANCE_ID(0,0);
    audioModuleCfg.Descriptor.VersionMajor = AUDIOMODULE_MAJOR;
    audioModuleCfg.Descriptor.VersionMinor = AUDIOMODULE_MINOR;
    status = RtlStringCchCopyNW(audioModuleCfg.Descriptor.Name,
                ACX_AUDIOMODULE_MAX_NAME_CCH_SIZE,
                AUDIOMODULEDESCRIPTION,
                wcslen(AUDIOMODULEDESCRIPTION));
    if (!NT_SUCCESS(status))
    {
        ASSERT(FALSE);
        goto exit;
    }

    audioModuleCfg.Callbacks = &audioModuleCallbacks;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_AUDIOMODULE_CONTEXT);
    attributes.ParentObject = Circuit;

    status = AcxAudioModuleCreate(Circuit, &attributes, &audioModuleCfg, &audioModuleElement);
    if (!NT_SUCCESS(status))
    {
        ASSERT(FALSE);
        goto exit;
    }

    audioModuleCtx = GetSdcaXuAudioModuleContext(audioModuleElement);
    ASSERT(audioModuleCtx);

    ACX_PNPEVENT_CONFIG_INIT(&audioModuleEventCfg);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_PNPEVENT_CONTEXT);
    attributes.ParentObject = audioModuleElement;
    status = AcxPnpEventCreate(Device, audioModuleElement, &attributes, &audioModuleEventCfg, &audioModuleEvent);
    if (!NT_SUCCESS(status)) 
    {
        ASSERT(FALSE);
        goto exit;
    }

    audioModuleCtx->Event = audioModuleEvent;

    status = AcxCircuitAddElements(Circuit, (ACXELEMENT *) &audioModuleElement, 1);
    if (!NT_SUCCESS(status)) 
    {
        ASSERT(FALSE);
        goto exit;
    }

    //
    // Done. 
    //
    status = STATUS_SUCCESS;

exit:
    return status;
}

_Function_class_(EVT_ACX_CIRCUIT_CREATE_STREAM)
PAGED_CODE_SEG
NTSTATUS
SdcaXu_EvtAcxCircuitCreateStream(
    _In_    WDFDEVICE       Device,
    _In_    ACXCIRCUIT      Circuit,
    _In_    ACXPIN          Pin,
    _In_    PACXSTREAM_INIT StreamInit,
    _In_    ACXDATAFORMAT   StreamFormat,
    _In_    const GUID *    SignalProcessingMode,
    _In_    ACXOBJECTBAG    VarArguments
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Circuit);
    UNREFERENCED_PARAMETER(Pin);
    UNREFERENCED_PARAMETER(StreamInit);
    UNREFERENCED_PARAMETER(StreamFormat);
    UNREFERENCED_PARAMETER(SignalProcessingMode);
    UNREFERENCED_PARAMETER(VarArguments);

    return STATUS_NOT_SUPPORTED;
}


PAGED_CODE_SEG
NTSTATUS
SdcaXu_CreateModuleCircuit(
    _In_ WDFDEVICE              Device
    )
{
    PAGED_CODE();

    NTSTATUS                                        status;
    WDF_OBJECT_ATTRIBUTES                           attributes;
    ACXCIRCUIT                                      circuit;
    PACXCIRCUIT_INIT                                circuitInit = NULL;
    SDCAXU_MODULECIRCUIT_CONTEXT *                  circuitCtx;

    //
    // ACX expects an 'other' circuit to start with KSCATEGORY_ACXCIRCUIT and also have
    // KSCATEGORY_AUDIO. The XU driver can add other categories after these.
    //
    GUID categories[] = {
        KSCATEGORY_ACXCIRCUIT,
        KSCATEGORY_AUDIO,
        SDCAXU_FACTORY_CATEGORY
    };

    //
    // Get a CircuitInit structure.
    //
    circuitInit = AcxCircuitInitAllocate(Device);

    //
    // Add circuit identifiers.
    //
    AcxCircuitInitSetComponentId(circuitInit, &SDCAXU_MODULECIRCUIT_GUID);
    AcxCircuitInitAssignCategories(circuitInit, categories, ARRAYSIZE(categories));
    AcxCircuitInitSetCircuitType(circuitInit, AcxCircuitTypeOther);
    AcxCircuitInitAssignName(circuitInit, &s_ModuleCircuitName);
    AcxCircuitInitAssignAcxCreateStreamCallback(circuitInit, SdcaXu_EvtAcxCircuitCreateStream);

    //
    // Create the circuit.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, SDCAXU_MODULECIRCUIT_CONTEXT);
    attributes.ParentObject = Device;
    status = AcxCircuitCreate(Device, &attributes, &circuitInit, &circuit);
    if (!NT_SUCCESS(status)) 
    {
        ASSERT(FALSE);
        goto exit;
    }

    // circuitInit has been freed by AcxCircuitCreate at this point
    ASSERT(circuitInit == NULL);

    ASSERT(circuit != NULL);
    circuitCtx = GetModuleCircuitContext(circuit);
    ASSERT(circuitCtx);
    UNREFERENCED_PARAMETER(circuitCtx);

    //
    // Create and add the audio modules to the circuit
    //
    status = SdcaXu_CreateModuleCircuitModules(Device, circuit);
    if (!NT_SUCCESS(status)) 
    {
        ASSERT(FALSE);
        goto exit;
    }

    //
    // Add circuit to device.
    //
    status = AcxDeviceAddCircuit(Device, circuit);
    if (!NT_SUCCESS(status))
    {
        ASSERT(FALSE);
        goto exit;
    }

    // Done
    status = STATUS_SUCCESS;

exit:
    if (circuitInit)
    {
        AcxCircuitInitFree(circuitInit);
    }
    return status;
}


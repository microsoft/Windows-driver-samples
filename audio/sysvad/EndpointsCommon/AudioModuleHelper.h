/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    AudioModuleHelper.h

Abstract:

    Simple helper for Audio Modules logic.

--*/

#ifndef _SYSVAD_AUDIOMODULEHELPER_H_
#define _SYSVAD_AUDIOMODULEHELPER_H_

//
// Module helper functions.
//
#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_BasicSupport(
    _In_ PAUDIOMODULE_PARAMETER_INFO            ParameterInfo,
    _Out_writes_bytes_opt_(*BufferCb) PVOID     Buffer,
    _Inout_ ULONG *                             BufferCb
    );

#pragma code_seg("PAGE")
BOOLEAN
IsAudioModuleParameterValid(
    _In_ PAUDIOMODULE_PARAMETER_INFO        ParameterInfo,
    _In_reads_bytes_opt_(BufferCb) PVOID    Buffer,
    _In_  ULONG                             BufferCb
    );

#pragma code_seg("PAGE")
AUDIOMODULE *
AudioModule_FindModuleInList(
    _In_ AUDIOMODULE *          AudioModules,
    _In_ ULONG                  AudioModuleCount,
    _In_ GUID *                 ClassId,
    _In_ ULONG                  InstanceId
    );

#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler(
    _In_  ULONG                                         Verb,
    _In_  ULONG                                         ParameterId,
    _In_  PAUDIOMODULE_PARAMETER_INFO                   ParameterInfo,
    _Inout_updates_bytes_(ParameterInfo->Size) PVOID    CurrentValue,
    _In_reads_bytes_opt_(InBufferCb) PVOID              InBuffer,
    _In_  ULONG                                         InBufferCb,
    _Out_writes_bytes_opt_(*OutBufferCb) PVOID          OutBuffer,
    _Inout_ ULONG *                                     OutBufferCb, 
    _In_ BOOL *                                         ParameterChanged
    );

//
// Miniport/Stream helper functions.
//
#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_ModulesListRequest(
    _In_ PPCPROPERTY_REQUEST    PropertyRequest,
    _In_ AUDIOMODULE *          AudioModules,
    _In_ ULONG                  AudioModuleCount
    );

#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_ModuleCommand(
    _In_ PPCPROPERTY_REQUEST    PropertyRequest,
    _In_ AUDIOMODULE *          AudioModules,
    _In_ ULONG                  AudioModuleCount
    );

#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_ModuleNotificationDeviceId(
    _In_ PPCPROPERTY_REQUEST    PropertyRequest,
    _In_ const GUID *           NotificationDeviceId
    );

#pragma code_seg("PAGE")
VOID
AudioModule_SendNotification(
    _In_ PPORTCLSNOTIFICATIONS          PortNotifications,
    _In_ PVOID                          NotificationBuffer, 
    _In_ USHORT                         NotificationBufferCb
    );
#endif // _SYSVAD_AUDIOMODULEHELPER_H_



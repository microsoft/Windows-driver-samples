/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    AudioModule0.h

Abstract:

    Declaration of Audio Module 0 (system module).

--*/

#ifndef _SYSVAD_AUDIOMODULE0_H_
#define _SYSVAD_AUDIOMODULE0_H_

#include "AudioModuleHelper.h"

#define AUDIOMODULE0_MAJOR  0x1
#define AUDIOMODULE0_MINOR  0X0

static const GUID AudioModule0Id = 
{ 0xe24c8b6f, 0xede7, 0x4255, 0x8b, 0x39, 0x77, 0x97, 0x60, 0x15, 0xd5, 0x93 };

enum AudioModule0_Parameter {
    AudioModule0Parameter1 = 0,
    AudioModule0Parameter2
};

typedef struct _AUDIOMODULE0_CUSTOM_COMMAND {
    ULONG                   Verb;       // get, set and support
    AudioModule0_Parameter  ParameterId;
} AUDIOMODULE0_CUSTOM_COMMAND, *PAUDIOMODULE0_CUSTOM_COMMAND;

enum AudioModule0_Notification_Type {
    AudioModule0ParameterChanged = 0,
};

typedef struct _AUDIOMODULE0_CUSTOM_NOTIFICATION {
    ULONG           Type;
    union {
        struct {
            ULONG   ParameterId;  
        } ParameterChanged;
    };
} AUDIOMODULE0_CUSTOM_NOTIFICATION, *PAUDIOMODULE0_CUSTOM_NOTIFICATION;

typedef struct _AUDIOMODULE0_NOTIFICATION {
    KSAUDIOMODULE_NOTIFICATION          Header;
    AUDIOMODULE0_CUSTOM_NOTIFICATION    CustomNotification;
} AUDIOMODULE0_NOTIFICATION, *PAUDIOMODULE0_NOTIFICATION;

typedef struct _AUDIOMODULE0_CONTEXT {
    ULONG                       Parameter1;
    BYTE                        Parameter2;
    ULONG                       InstanceId;
    AUDIOMODULE0_NOTIFICATION   Notification;  
    PPORTCLSNOTIFICATIONS       PortNotifications;
} AUDIOMODULE0_CONTEXT, *PAUDIOMODULE0_CONTEXT;

static
ULONG AudioModule0_ValidParameter1List[] =
{
    1, 2, 5
};

static
AUDIOMODULE_PARAMETER_INFO  AudioModule0_ParameterInfo[] =
{
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE0_CONTEXT, Parameter1),
        VT_UI4,
        AudioModule0_ValidParameter1List,
        SIZEOF_ARRAY(AudioModule0_ValidParameter1List)
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        0, 
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE0_CONTEXT, Parameter2),
        VT_UI1,
        NULL,
        0
    },
};

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule0_InitClass(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        KSAUDIOMODULE_NOTIFICATION * NotificationHeader,
    _In_opt_    PPORTCLSNOTIFICATIONS PortNotifications
    )
{
    PAUDIOMODULE0_CONTEXT ctx = (PAUDIOMODULE0_CONTEXT)Context;
    
    UNREFERENCED_PARAMETER(Module);

    PAGED_CODE();

    if (Context == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        return STATUS_INTERNAL_ERROR;
    }
    
    ASSERT(Size != 0);    
    RtlZeroMemory(Context, Size);

    ctx->Notification.Header = *NotificationHeader;
    ctx->PortNotifications = PortNotifications;
    if (ctx->PortNotifications != NULL)
    {
        ctx->PortNotifications->AddRef();
    }
    
    return STATUS_SUCCESS;
}

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule0_InitInstance(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _In_opt_    PVOID           TemplateContext,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        ULONG           InstanceId
    )
{
    PAUDIOMODULE0_CONTEXT ctx = (PAUDIOMODULE0_CONTEXT)Context;
    
    UNREFERENCED_PARAMETER(Module);

    PAGED_CODE();

    if (Context == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        return STATUS_INTERNAL_ERROR;
    }
    
    ASSERT(TemplateContext != NULL);
    ASSERT(Size != 0); 
    
    RtlCopyMemory(Context, TemplateContext, Size);
    
    ctx->InstanceId = InstanceId;
    ctx->Notification.Header.ProviderId.InstanceId = InstanceId;
    if (ctx->PortNotifications != NULL)
    {
        ctx->PortNotifications->AddRef();
    }
    
    return STATUS_SUCCESS;
}

inline
#pragma code_seg("PAGE")
VOID
AudioModule0_Cleanup(
    _In_  PVOID           Context
    )
{
    PAUDIOMODULE0_CONTEXT ctx = (PAUDIOMODULE0_CONTEXT)Context;
    
    PAGED_CODE();

    if (Context == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        return;
    }
    
    SAFE_RELEASE(ctx->PortNotifications);
}

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule0_Handler(
    _Inout_opt_                          PVOID   Context,
    _In_reads_bytes_(InBufferCb)         PVOID   InBuffer,
    _In_                                 ULONG   InBufferCb,
    _Out_writes_bytes_opt_(*OutBufferCb) PVOID   OutBuffer,
    _Inout_                              ULONG * OutBufferCb 
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    BOOL        fNewValue = FALSE;
    PVOID       currentValue;
    PVOID       inBuffer = NULL;
    ULONG       inBufferCb = 0;
        
    PAUDIOMODULE0_CONTEXT ctx = (PAUDIOMODULE0_CONTEXT)Context;
    AUDIOMODULE_PARAMETER_INFO * parameterInfo = NULL;
    AUDIOMODULE0_CUSTOM_COMMAND * command = NULL;
    
    PAGED_CODE();
    
    if (ctx == NULL)
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

    if (InBufferCb < sizeof(AUDIOMODULE0_CUSTOM_COMMAND))
    {
        return STATUS_INVALID_PARAMETER;
    }

    command = (AUDIOMODULE0_CUSTOM_COMMAND*)InBuffer;  
    
    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModule0Parameter1:
            currentValue = &ctx->Parameter1;
            parameterInfo = &AudioModule0_ParameterInfo[AudioModule0Parameter1];
            break;

        case AudioModule0Parameter2:
            currentValue = &ctx->Parameter2;
            parameterInfo = &AudioModule0_ParameterInfo[AudioModule0Parameter2];
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            goto exit;
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE0_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE0_CUSTOM_COMMAND);

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
    
    if (fNewValue && ctx->PortNotifications &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        // This logic assumes that access to this function is serialized.
        ctx->Notification.CustomNotification.Type = AudioModule0ParameterChanged;
        ctx->Notification.CustomNotification.ParameterChanged.ParameterId = 
            command->ParameterId;

        AudioModule_SendNotification(ctx->PortNotifications,
                                     (PVOID)&ctx->Notification,
                                     (USHORT)sizeof(ctx->Notification));
    }

    // Normalize error code.
    status = STATUS_SUCCESS;
    
exit:
    return status;
}

#endif // _SYSVAD_AUDIOMODULE0_H_


